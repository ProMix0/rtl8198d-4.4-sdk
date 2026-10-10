
#include <linux/igmp.h>
#include <net/mld.h>
//#include <uapi/linux/igmp.h>
//#include <linux/if_bridge.h>
#include <br_private.h>

#include <rtk_igmp_snooping.h>
#include <rtk_igmp_hook.h>
#include <rtk_igmp_debug.h>
#include <rtk_igmp_struct.h>
//#include <rtk_igmp_proc.h>


#define IGMP_HASH_TBL_SIZE 	(1<<IGMP_HASH_TBL_BIT)
#define IGMP_HASH_TBL_BIT 	5


#define IGMP_MAX_FLOW_COUNT		512
#define IGMP_MAX_GROUP_COUNT	512	/* default max group entry count **/
#define IGMP_MAX_SOURCE_COUNT	512	/*default max source entry count*/
#define IGMP_MAX_CLIENT_COUNT	512

#define IGMP_MAX_WHITE_COUNT	1024
#define IGMP_MAX_USER_REV2PS_GROUP_COUNT	512
#define IGMP_MAX_USER_GROUP_WEIGHT	512


#define IGMP_MCAST_FLOW_EXPIRE_TIME 	140
#define SYS_EXPIRED_NORMAL 	0
#define MODULE_NUM 0


rtk_igmp_hwCbEvt_t hwCb;
rtk_igmp_globalConfdb_t gloConf;

//hardware dependency callback event


struct rtk_igmp_multicastModule rtk_igmp_mCastModuleArray[IGMP_MAX_BR_MODULE_NUM];
rtk_igmp_multicastDeviceInfo_t  rtk_igmp_wanDevInfo[IGMP_MAX_WAN_DEV_NUM];
rtk_igmp_multicastDeviceInfo_t  rtk_igmp_lanDevInfo[IGMP_MAX_LAN_DEV_NUM];

static struct rtk_igmp_groupEntry *rtk_igmp_groupEntryPool=NULL;
static struct rtk_igmp_clientEntry *rtk_igmp_clientEntryPool=NULL;
static struct rtk_igmp_sourceEntry *rtk_igmp_sourceEntryPool=NULL;
static struct rtk_igmp_mcastFlowEntry *rtk_igmp_mcastFlowEntryPool=NULL;

struct rtk_igmp_clientEntry clientEntryPool[IGMP_MAX_CLIENT_COUNT];
struct rtk_igmp_groupEntry groupEntryPool[IGMP_MAX_GROUP_COUNT];
struct rtk_igmp_sourceEntry sourceEntryPool[IGMP_MAX_SOURCE_COUNT];
struct rtk_igmp_mcastFlowEntry mcastFlowEntryPool[IGMP_MAX_FLOW_COUNT];

rtk_igmp_whiteListEntry_t whiteList[IGMP_MAX_WHITE_COUNT];
struct list_head whiteListFreeListHead;
rtk_igmp_userResvGroup_t gorup2Ps[IGMP_MAX_USER_REV2PS_GROUP_COUNT]; //user define reserved group to protocol-stack table
struct list_head gorup2PsFreeListHead;
struct list_head gorup2PsListHead;

rtk_igmp_groupWeight_t groupWeight[IGMP_MAX_USER_REV2PS_GROUP_COUNT];
struct list_head groupWeightFreeListHead;
struct list_head groupWeightListHead;


//static uint32 igmpSnoopingCounterVer_Port[IGMP_MAX_DEV_NUM][IGMP_MLD_VER_MAX];


static uint8 igmpSnoopingCounterVer_1=0; ////: add
static uint8 igmpSnoopingCounterVer_2=0;
static uint8 igmpSnoopingCounterVer_3=0;
static uint8 MLDCounterVer_1=0;
static uint8 MLDCounterVer_2=0;

/*the system up time*/
static uint32 rtk_igmp_startTime;
static uint32 rtk_igmp_sysUpSeconds;
//static unsigned long int last_query_jiffies;	/*record the system jiffie of last query send*/

struct timer_list igmpSysTimer;	/*igmp timer*/
struct timer_list mCastQuerytimer;	/*igmp query timer*/
struct timer_list mCastForceReporttimer;	/*igmp mld force response timer*/
static unsigned long lastjiffies=0;
static uint32 lastTimer=0; //start for 0 sec


atomic_t groupIncIdx=ATOMIC_INIT(1);


/*******************************internal function declaration*****************************/
/**************************
	resource managment
**************************/

static  struct rtk_igmp_groupEntry* rtk_igmp_initGroupEntryPool(uint32 poolSize);
static  struct rtk_igmp_groupEntry* rtk_igmp_allocateGroupEntry(void);
static  void rtk_igmp_freeGroupEntry(struct rtk_igmp_groupEntry* groupEntryPtr) ;


static  struct rtk_igmp_clientEntry* rtk_igmp_initClientEntryPool(uint32 poolSize);
static  struct rtk_igmp_clientEntry* rtk_igmp_allocateClientEntry(void);
static  void rtk_igmp_freeClientEntry(struct rtk_igmp_clientEntry* clientEntryPtr) ;

static  struct rtk_igmp_sourceEntry* rtk_igmp_initSourceEntryPool(uint32 poolSize);
static  struct rtk_igmp_sourceEntry* rtk_igmp_allocateSourceEntry(void);
static  void rtk_igmp_freeSourceEntry(struct rtk_igmp_sourceEntry* sourceEntryPtr) ;
#ifdef CONFIG_RECORD_MCAST_FLOW
static  struct rtk_igmp_mcastFlowEntry* rtk_igmp_initMcastFlowEntryPool(uint32 poolSize);
static  struct rtk_igmp_mcastFlowEntry* rtk_igmp_allocateMcastFlowEntry(void);
static  void rtk_igmp_freeMcastFlowEntry(struct rtk_igmp_mcastFlowEntry* mcastFlowEntry) ;
#endif
/**********************************Structure Maintenance*************************/

//Group
struct rtk_igmp_groupEntry* rtk_igmp_searchGroupEntry(int32 moduleIndex,uint32 ipVersion,uint32 *multicastAddr, uint16 vlanId);
static void rtk_igmp_linkGroupEntry(struct rtk_igmp_groupEntry* entryNode ,  struct rtk_igmp_groupEntry ** hashTable);
static void rtk_igmp_unlinkGroupEntry(struct rtk_igmp_groupEntry* entryNode,  struct rtk_igmp_groupEntry ** hashTable);
static void rtk_igmp_clearGroupEntry(struct rtk_igmp_groupEntry* groupEntryPtr);
static void rtk_igmp_deleteGroupEntry(int32 moduleIndex ,struct rtk_igmp_groupEntry* groupEntry,struct rtk_igmp_groupEntry ** hashTable);

//Client
static struct rtk_igmp_clientEntry* rtk_igmp_searchClientEntry(uint32 ipVersion,struct rtk_igmp_groupEntry* groupEntry, uint32 portNum, uint32 *clientAddr);
static void rtk_igmp_linkClientEntry(struct rtk_igmp_groupEntry *groupEntry, struct rtk_igmp_clientEntry* clientEntry);
static void rtk_igmp_unlinkClientEntry(struct rtk_igmp_groupEntry *groupEntry, struct rtk_igmp_clientEntry* clientEntry);
static void rtk_igmp_clearClientEntry(struct rtk_igmp_clientEntry* clientEntryPtr);
static void rtk_igmp_deleteClientEntry(struct rtk_igmp_groupEntry * groupEntry, struct rtk_igmp_clientEntry * clientEntry);
static void rtk_igmp_deleteClientList(struct rtk_igmp_groupEntry* groupEntry);
static int32 rtk_igmp_getClientFwdIfIdx(struct rtk_igmp_clientEntry * clientEntry, uint32 sysTime);

//Client-Source
static struct rtk_igmp_sourceEntry* rtk_igmp_searchSourceEntry(uint32 ipVersion, uint32 *sourceAddr, struct rtk_igmp_clientEntry *clientEntry);
static void rtk_igmp_linkSourceEntry(struct rtk_igmp_clientEntry *clientEntry,  struct rtk_igmp_sourceEntry* entryNode);
static void rtk_igmp_unlinkSourceEntry(struct rtk_igmp_clientEntry *clientEntry, struct rtk_igmp_sourceEntry* entryNode);
static void rtk_igmp_clearSourceEntry(struct rtk_igmp_sourceEntry* sourceEntryPtr);
static void rtk_igmp_deleteSourceEntry(struct rtk_igmp_clientEntry *clientEntry, struct rtk_igmp_sourceEntry* sourceEntry);
static void rtk_igmp_deleteSourceList(struct rtk_igmp_clientEntry* clientEntry);

//Flow
static struct rtk_igmp_mcastFlowEntry* rtk_igmp_searchMcastFlowEntry(int32 moduleIndex, uint32 ipVersion, uint32 *serverAddr,uint32 *groupAddr,int32 sport,int32 dport,int16 ingressCvid,int16 l4proto);
static void  rtk_igmp_linkMcastFlowEntry(struct rtk_igmp_mcastFlowEntry* mcastFlowEntry ,  struct rtk_igmp_mcastFlowEntry ** hashTable);
static void rtk_igmp_unlinkMcastFlowEntry(struct rtk_igmp_mcastFlowEntry* mcastFlowEntry,  struct rtk_igmp_mcastFlowEntry ** hashTable);
static void rtk_igmp_clearMcastFlowEntry(struct rtk_igmp_mcastFlowEntry* mcastFlowEntry);
static void rtk_igmp_deleteMcastFlowEntry( struct rtk_igmp_mcastFlowEntry* mcastFlowEntry, struct rtk_igmp_mcastFlowEntry ** hashTable);
static void rtk_igmp_invalidateMCastGroupFlow(int32 moduleIndex,uint32 ipVersion, uint32 *groupAddr);
static  struct rtk_igmp_mcastFlowEntry* rtk_igmp_initMcastFlowEntryPool(uint32 poolSize);
static int32 rtk_igmp_flow_reflush_update(int32 moduleIndex,uint32* groupAddr);
static int _rtk_igmp_getMulticastDataFwdInfo(int32 moduleIndex,uint32 searchCacheFlowFirst, struct rtk_igmp_multicastDataInfo *multicastDataInfo, struct rtk_igmp_multicastFwdInfo *multicastFwdInfo);
static int rtk_igmp_agingMcastFlow(int32 moduleIndex);
static int rtk_igmp_deleteUnknownFloodMacstFlow(int32 moduleIndex);
void rtk_igmp_deleteFloodMcastFlowEntry( struct rtk_igmp_mcastFlowEntry* mcastFlowEntry, struct rtk_igmp_mcastFlowEntry ** hashTable);


//Timer
static void rtk_igmp_multicastSysTimerDestroy(void);
void rtk_igmp_multicastSysTimerInit(void);


rtk_igmp_nf_ret_e_t rtk_igmp_group_limitCheck(rtk_igmp_pktHdr_t *pPkthdr,uint32 ipVersion, uint32 *groupAddr , uint32 *clientAddr ,uint32 *mcSourceAddr,uint8 *saMac,uint32 vlanId ,uint32 portNum,rtk_igmp_nf_ret_e_t overLimitAction);
static int rtk_igmp_netdev_event(struct notifier_block *this, unsigned long event, void *ptr);
rtk_igmp_multicastDeviceInfo_t* _rtk_igmp_devIfidx_to_devInfo(int32 devIfindex);
struct rtk_igmp_multicastModule* _rtk_igmp_devIfidx_to_BrDevInfo(int32 brIfindex);
int32 rtk_igmp_clearDataBaseByDev(int32 devIdx);

//white list
int rtk_igmp_whiteEntry_free(rtk_igmp_whiteListEntry_t* WhilteEntry);
int _rtk_igmp_checkGroupWeightTbl(uint32 ifIdx,uint32 ipVersion,uint32 *_groupIp);


#define RE_INIT

//flush Dev multicast data base
//inidx fail to flush all
int rtk_igmp_flush_Br_mdb(int brifidx ,int isIpv6)
{
	int32 moduleIndex,hashIndex;
	struct rtk_igmp_groupEntry *groupEntryPtr;
	struct rtk_igmp_groupEntry *tmp_groupEntryPtr;

	for( moduleIndex=0 ; moduleIndex< IGMP_MAX_BR_MODULE_NUM; moduleIndex++)
	{

		if(brifidx!=FAIL && rtk_igmp_mCastModuleArray[moduleIndex].brDevInfo.ifindex!=brifidx)
			continue;

		if(rtk_igmp_mCastModuleArray[moduleIndex].validBit==0)
			continue;

		if(isIpv6==0)
		{
			/* IPv4 */
			for (hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
			{
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[hashIndex];
				while (groupEntryPtr!=NULL)
				{
					tmp_groupEntryPtr=groupEntryPtr->next;
					rtk_igmp_deleteGroupEntry(moduleIndex,groupEntryPtr,rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable);
					groupEntryPtr=tmp_groupEntryPtr;
				}

			}
		}
		else
		{
			/* IPv6 */
			for (hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
			{
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[hashIndex];
				while (groupEntryPtr!=NULL)
				{
					tmp_groupEntryPtr=groupEntryPtr->next;
					rtk_igmp_deleteGroupEntry(moduleIndex,groupEntryPtr,rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable);
					groupEntryPtr=tmp_groupEntryPtr;
				}

			}
		}


	}



	return SUCCESS;
}


int rtk_igmp_reset_devConfig(int ifidx,int isIpv6)
{

	rtk_igmp_whiteListEntry_t *white_entry;
	rtk_igmp_whiteListEntry_t *white_entry_tmp;
	rtk_igmp_multicastDeviceInfo_t* devInfo = _rtk_igmp_devIfidx_to_devInfo(ifidx);

	if(devInfo)
	{
		if(isIpv6)
		{
			list_for_each_entry_safe(white_entry,white_entry_tmp,&devInfo->devConf.igmp6.white,entry)
			{
				rtk_igmp_whiteEntry_free(white_entry);
			}
			memset(&devInfo->devConf.igmp6,0,sizeof(devInfo->devConf.igmp6));
			devInfo->devConf.igmp6.igmp6McMemberAgingInterval = IGMP6_MEMBER_AGING;
			devInfo->devConf.igmp6.igmp6McLastMemberAgingInterval = IGMP6_LAST_MEMBER_AGING;
			INIT_LIST_HEAD(&devInfo->devConf.igmp6.white);
			INIT_LIST_HEAD(&devInfo->devConf.igmp6.block);

		}
		else
		{
			list_for_each_entry_safe(white_entry,white_entry_tmp,&devInfo->devConf.igmp.white,entry)
			{
				rtk_igmp_whiteEntry_free(white_entry);
			}
			memset(&devInfo->devConf.igmp,0,sizeof(devInfo->devConf.igmp));
			devInfo->devConf.igmp.igmpMcMemberAgingInterval= IGMP_MEMBER_AGING;
			devInfo->devConf.igmp.igmpMcLastMemberAgingInterval= IGMP_LAST_MEMBER_AGING;
			INIT_LIST_HEAD(&devInfo->devConf.igmp.white);
			INIT_LIST_HEAD(&devInfo->devConf.igmp.block);
		}
	}


	return SUCCESS;
}



#define WHITE_LIST
rtk_igmp_whiteListEntry_t * rtk_igmp_whiteEntry_malloc(void)
{
	rtk_igmp_whiteListEntry_t *white_entry;
	rtk_igmp_whiteListEntry_t *white_entry_tmp;

	if(list_empty(&whiteListFreeListHead))
	{
		IGMP_WARNING("all free IGMP whiteListFreeList are allocated...");
		return (NULL);
	}
	list_for_each_entry_safe(white_entry,white_entry_tmp,&whiteListFreeListHead,entry)		//just return the first entry right behind of head
	{
		list_del_init(&white_entry->entry);
		break;
	}
	memset(&white_entry->patten,0,sizeof(rt_igmpHook_whiteList_t));
	white_entry->valid=1;
	return white_entry;
}


int rtk_igmp_whiteEntry_free(rtk_igmp_whiteListEntry_t* WhilteEntry)
{

	list_del_init(&WhilteEntry->entry);
	list_add(&WhilteEntry->entry, &whiteListFreeListHead);
	WhilteEntry->valid=0;
	return (SUCCESS);
}



int rtk_igmp_whiteEntry_flashInit(void)
{
	int i;
	INIT_LIST_HEAD(&whiteListFreeListHead);

	for(i=0 ; i<IGMP_MAX_WHITE_COUNT;i++ )
	{
		memset(&whiteList[i],0,sizeof(rtk_igmp_whiteListEntry_t));
		whiteList[i].whiteListIdx=i;
		INIT_LIST_HEAD(&whiteList[i].entry);
		list_add_tail(&whiteList[i].entry, &whiteListFreeListHead);
	}
	return (SUCCESS);
}

int rtk_igmp_whiteListAdd( rt_igmpHook_whiteList_t *patten,int32 ifidx , int32 *index)
{

	rtk_igmp_multicastDeviceInfo_t *devInfo =_rtk_igmp_devIfidx_to_devInfo(ifidx);
	rtk_igmp_whiteListEntry_t *white_entry =NULL;



	if(patten->isIpv6==0 )
	{
		//Error checking
		if((patten->wlSipEnd[0]>0) &&((patten->wlSipEnd[0] < patten->wlSipStart[0]) ))
			return FAIL;
		if((patten->wlGroupIpEnd[0]>0) &&((patten->wlGroupIpEnd[0] < patten->wlGroupIpStart[0]) ))
			return FAIL;
		if((patten->wlMcSipEnd[0]>0) &&((patten->wlMcSipEnd[0] < patten->wlMcSip[0])))
			return FAIL;
	}
	if(devInfo)
	{
		if(patten->isIpv6)
		{
			list_for_each_entry(white_entry,&devInfo->devConf.igmp6.white,entry)
			{
				if(memcmp(&white_entry->patten,patten,sizeof(rtk_igmp_whiteListEntry_t))==0)
				{
					*index = white_entry->whiteListIdx;
					return SUCCESS;
				}
			}
		}
		else
		{
			list_for_each_entry(white_entry,&devInfo->devConf.igmp.white,entry)
			{
				if(memcmp(&white_entry->patten,patten,sizeof(rt_igmpHook_whiteList_t))==0)
				{
					*index = white_entry->whiteListIdx;
					return SUCCESS;
				}
			}
		}


	}
	else
		return FAIL;



	white_entry = rtk_igmp_whiteEntry_malloc();
	if(devInfo && white_entry)
	{
		if(patten->isIpv6)
			list_add(&white_entry->entry,&devInfo->devConf.igmp6.white);
		else
			list_add(&white_entry->entry,&devInfo->devConf.igmp.white);

		memcpy(&white_entry->patten,patten,sizeof(white_entry->patten));
		*index = white_entry->whiteListIdx;
		return SUCCESS;
	}
	else
		return FAIL;
}


int rtk_igmp_whiteListDel(rt_igmpHook_whiteList_t *patten,int32 ifidx,int32 index)
{
	if(patten==NULL)
	{
		if( index < 0 || IGMP_MAX_WHITE_COUNT <= index)
			return FAIL;
		if(!whiteList[index].valid)
			return FAIL;

		rtk_igmp_whiteEntry_free(&whiteList[index]);
		return SUCCESS;
	}
	else
	{
		rtk_igmp_multicastDeviceInfo_t *devInfo =_rtk_igmp_devIfidx_to_devInfo(ifidx);
		rtk_igmp_whiteListEntry_t *white_entry =NULL;
		rtk_igmp_whiteListEntry_t *white_entry_tmp =NULL;

		if(devInfo==NULL)
			return FAIL;

		if(patten->isIpv6)
		{
			list_for_each_entry_safe(white_entry,white_entry_tmp,&devInfo->devConf.igmp6.white,entry)
			{
				if(memcmp(&white_entry->patten,patten,sizeof(rt_igmpHook_whiteList_t))==0)
				{
					if(!whiteList[white_entry->whiteListIdx].valid)
						return FAIL;
					rtk_igmp_whiteEntry_free(&whiteList[white_entry->whiteListIdx]);
					return SUCCESS;
				}
			}
		}
		else
		{
			list_for_each_entry_safe(white_entry,white_entry_tmp,&devInfo->devConf.igmp.white,entry)
			{
				if(memcmp(&white_entry->patten,patten,sizeof(rt_igmpHook_whiteList_t))==0)
				{
					if(!whiteList[white_entry->whiteListIdx].valid)
						return FAIL;
					rtk_igmp_whiteEntry_free(&whiteList[white_entry->whiteListIdx]);
					return SUCCESS;
				}
			}
		}
		return FAIL;
	}
}


int rtk_igmp_filterWhiteListCheck(rtk_igmp_pktHdr_t *pPkthdr ,uint32 ipVersion, uint32 *groupAddr , uint32 *clientAddr,uint32 *mcSourceAddr,uint8 *saMac)
{
	int ret=FAIL;
	int whiteListEnable=0;
	int smacHit=0,sipHit=0,gipHit=0,mcSipHit=0;
	rtk_igmp_whiteListEntry_t *white_entry;
	rtk_igmp_whiteListEntry_t *white_entry_tmp;


	if(ipVersion==IP_VERSION4)
	{
		list_for_each_entry_safe(white_entry,white_entry_tmp,&pPkthdr->devInfo->devConf.igmp.white,entry)
		{
			smacHit=0,sipHit=0,gipHit=0,mcSipHit=0; 
			if(white_entry->patten.smacChk)
			{
				if(saMac==NULL)//ingore this rule because we don't have this info
					continue;
				IGMP_CTRL("check smac");
				if(   saMac && 
					((white_entry->patten.smac[0]&white_entry->patten.smacMask[0])== (saMac[0]&white_entry->patten.smacMask[0])) &&
					((white_entry->patten.smac[1]&white_entry->patten.smacMask[1])== (saMac[1]&white_entry->patten.smacMask[1])) &&
					((white_entry->patten.smac[2]&white_entry->patten.smacMask[2])== (saMac[2]&white_entry->patten.smacMask[2])) &&
					((white_entry->patten.smac[3]&white_entry->patten.smacMask[3])== (saMac[3]&white_entry->patten.smacMask[3])) &&
					((white_entry->patten.smac[4]&white_entry->patten.smacMask[4])== (saMac[4]&white_entry->patten.smacMask[4])) &&
					((white_entry->patten.smac[5]&white_entry->patten.smacMask[5])== (saMac[5]&white_entry->patten.smacMask[5])) )
				{
					smacHit=1;
				}
			}
			else
				smacHit=1;


			if(white_entry->patten.gipChk)
			{
				if(groupAddr==NULL)//ingore this rule because we don't have this info
					continue;
				
				IGMP_CTRL("check group");
				if(groupAddr && (white_entry->patten.wlGroupIpStart[0] <= groupAddr[0])  &&  (groupAddr[0] <=white_entry->patten.wlGroupIpEnd[0]))
					gipHit=1;
				
			}
			else
				gipHit=1;

			if(white_entry->patten.sipChk)
			{
				if(clientAddr==NULL)//ingore this rule because we don't have this info
					continue;
				
				IGMP_CTRL("check clientAddr");			
				if(clientAddr && (white_entry->patten.wlSipStart[0] <= clientAddr[0])  &&  (clientAddr[0] <=white_entry->patten.wlSipEnd[0]))
					sipHit=1;
			}
			else
				sipHit=1;

			if(white_entry->patten.mcSipChk)
			{
				if(mcSourceAddr==NULL)//ingore this rule because we don't have this info
					continue;				
				IGMP_CTRL("check mcSourceAddr");
				if(mcSourceAddr && (white_entry->patten.wlMcSip[0] <= mcSourceAddr[0]) && (mcSourceAddr[0] <= white_entry->patten.wlMcSipEnd[0]))
					mcSipHit=1;
			}
			else
				mcSipHit=1;


			whiteListEnable=1;
			IGMP_CTRL("check rule[%d] [smacHit:%d] [sipHit:%d] [mcSipHit:%d] [gipHit:%d]",white_entry->whiteListIdx,smacHit,sipHit,mcSipHit,gipHit);
			if(smacHit && sipHit && mcSipHit && gipHit)
			{
				IGMP_CTRL("Hit v4 WhiteList[%d]",white_entry->whiteListIdx);
				ret= SUCCESS;
				break;
			}
		}
	}
	else if(ipVersion==IP_VERSION6)
	{
		list_for_each_entry_safe(white_entry,white_entry_tmp,&pPkthdr->devInfo->devConf.igmp6.white,entry)
		{
			smacHit=0,sipHit=0,gipHit=0,mcSipHit=0;			

			if(white_entry->patten.smacChk)
			{
				if(saMac==NULL)//ingore this rule because we don't have this info
					continue;
				if( saMac&&
					((white_entry->patten.smac[0]&white_entry->patten.smacMask[0])== (saMac[0]&white_entry->patten.smacMask[0])) &&
					((white_entry->patten.smac[1]&white_entry->patten.smacMask[1])== (saMac[1]&white_entry->patten.smacMask[1])) &&
					((white_entry->patten.smac[2]&white_entry->patten.smacMask[2])== (saMac[2]&white_entry->patten.smacMask[2])) &&
					((white_entry->patten.smac[3]&white_entry->patten.smacMask[3])== (saMac[3]&white_entry->patten.smacMask[3])) &&
					((white_entry->patten.smac[4]&white_entry->patten.smacMask[4])== (saMac[4]&white_entry->patten.smacMask[4])) &&
					((white_entry->patten.smac[5]&white_entry->patten.smacMask[5])== (saMac[5]&white_entry->patten.smacMask[5])) )
				{
					smacHit=1;
				}
			}
			else
				smacHit=1;


			if(white_entry->patten.gipChk)
			{
				int msbDif=3;
				if(groupAddr)
				{
					for(msbDif=3;msbDif>=0;msbDif--)
					{
						if(white_entry->patten.wlGroupIpStart[msbDif]!=groupAddr[msbDif])
							break;
					}
					if((white_entry->patten.wlGroupIpStart[msbDif] <= groupAddr[msbDif])  &&  (groupAddr[msbDif] <=white_entry->patten.wlGroupIpEnd[msbDif]))
						gipHit=1;
				}
				else
					continue;
			}
			else
				gipHit=1;

			if(white_entry->patten.sipChk)
			{
				int msbDif=3;
				if(clientAddr)
				{
					for(msbDif=3;msbDif>=0;msbDif--)
					{
						if(white_entry->patten.wlSipStart[msbDif]!=clientAddr[msbDif])
							break;
					}
					if((white_entry->patten.wlSipStart[msbDif] <= clientAddr[msbDif])  &&  (clientAddr[msbDif] <=white_entry->patten.wlSipEnd[msbDif]))
						sipHit=1;
				}
				else
					continue;
			}
			else
				sipHit=1;

			if(white_entry->patten.mcSipChk)
			{
				int msbDif=3;
				if(mcSourceAddr)
				{
					for(msbDif=3;msbDif>=0;msbDif--)
					{
						if(white_entry->patten.wlMcSip[msbDif]!=mcSourceAddr[msbDif])
							break;
					}
					if((white_entry->patten.wlMcSip[msbDif] <= mcSourceAddr[msbDif])  &&  (mcSourceAddr[msbDif] <=white_entry->patten.wlMcSipEnd[msbDif]))
						mcSipHit=1;
				}
				else
					continue;
			}
			else
				mcSipHit=1;

			whiteListEnable=1;
			if(smacHit && sipHit && gipHit && mcSipHit)
			{
				IGMP_CTRL("Hit v6 WhiteList[%d]",white_entry->whiteListIdx);
				ret= SUCCESS;
				break;
			}

		}

	}

	if(whiteListEnable==0)
		ret=SUCCESS;

	return ret;
}


#define RESERVED_GROUP
int rtk_igmp_groupToPsTbl_flashInit(void)
{
	int i;

	INIT_LIST_HEAD(&gorup2PsFreeListHead);
	INIT_LIST_HEAD(&gorup2PsListHead);
	
	for(i=0 ; i<IGMP_MAX_USER_REV2PS_GROUP_COUNT;i++ )
	{
		memset(&gorup2Ps[i],0,sizeof(rtk_igmp_userResvGroup_t));
		gorup2Ps[i].resvGrpListIdx = i;
		INIT_LIST_HEAD(&gorup2Ps[i].entry);
		list_add_tail(&gorup2Ps[i].entry, &gorup2PsFreeListHead);
	}
	return (SUCCESS);
}


int rtk_igmp_groupToPsTbl_free(rtk_igmp_userResvGroup_t* rsvEntry)
{

	list_del_init(&rsvEntry->entry);
	list_add(&rsvEntry->entry, &gorup2PsFreeListHead);
	rsvEntry->valid=0;
	return (SUCCESS);
}


rtk_igmp_userResvGroup_t * rtk_igmp_groupToPsTbl_malloc(void)
{
	rtk_igmp_userResvGroup_t *group2Psentry;
	rtk_igmp_userResvGroup_t *group2Psentry_tmp;

	if(list_empty(&gorup2PsFreeListHead))
	{
		IGMP_WARNING("all free IGMP rtk_igmp_groupToPsTbl_malloc are allocated...");
		return (NULL);
	}
	list_for_each_entry_safe(group2Psentry,group2Psentry_tmp,&gorup2PsFreeListHead,entry)		//just return the first entry right behind of head
	{
		list_del_init(&group2Psentry->entry);
		break;
	}
	memset(&group2Psentry->patten,0,sizeof(group2Psentry->patten));
	group2Psentry->valid=1;
	return group2Psentry;
}


int rtk_igmp_groupToPsTblAdd(rt_igmpHook_ignoreGroup_t* patten,uint32 *index)
{
	rtk_igmp_userResvGroup_t *group2Psentry;
	if(index==NULL)
		return FAIL;
	if(memcmp(patten->ignGroupIpStart,patten->ignGroupIpEnd,sizeof(patten->ignGroupIpStart))!=0)
	{
		IGMP_CTRL("Not support multi-group yet %pI6 != %pI6",patten->ignGroupIpStart,patten->ignGroupIpEnd);
		return FAIL;
	}
	
	list_for_each_entry(group2Psentry,&gorup2PsListHead,entry)
	{
		if(memcmp(&group2Psentry->patten,patten,sizeof(rt_igmpHook_ignoreGroup_t))==0)
		{
			*index = group2Psentry->resvGrpListIdx;
			return SUCCESS;
		}
	}

	group2Psentry = rtk_igmp_groupToPsTbl_malloc();
	if(group2Psentry)
	{
		list_add(&group2Psentry->entry,&gorup2PsListHead);
		memcpy(&group2Psentry->patten,patten,sizeof(group2Psentry->patten));
		*index = group2Psentry->resvGrpListIdx;
	}
	else
		return FAIL;


	if(hwCb.groupAddCbEvt)
	{
		rtk_igmp_groupCbEvt_t groupCb;
		memset(&groupCb,0,sizeof(groupCb));
		groupCb.isIPv6= patten->isIpv6;
		memcpy(groupCb.group ,patten->ignGroupIpStart,sizeof(groupCb.group));
		IGMP_CTRL("GROUP ADD CALL BACK2 HW");
		hwCb.groupAddCbEvt(&groupCb);
	}


	return SUCCESS;
}

int rtk_igmp_groupToPsTblDel(rt_igmpHook_ignoreGroup_t* patten,uint32 index)
{
	if(patten==NULL)
	{
		if(index<0 || IGMP_MAX_USER_REV2PS_GROUP_COUNT <= index)
			return FAIL;
		if(gorup2Ps[index].valid==0)
			return FAIL;

		if(hwCb.groupDelCbEvt)
		{
			rtk_igmp_groupCbEvt_t groupCb;
			memset(&groupCb,0,sizeof(groupCb));
			groupCb.isIPv6= gorup2Ps[index].patten.isIpv6;
			memcpy(groupCb.group ,gorup2Ps[index].patten.ignGroupIpStart,sizeof(groupCb.group));
			IGMP_CTRL("GROUP DEL CALL BACK2 HW");
			hwCb.groupDelCbEvt(&groupCb);
		}
		return rtk_igmp_groupToPsTbl_free(&gorup2Ps[index]);

	}
	else
	{
		rtk_igmp_userResvGroup_t *groupToPs_entry =NULL;
		rtk_igmp_userResvGroup_t *group2Psentry_tmp=NULL;
		
		list_for_each_entry_safe(groupToPs_entry,group2Psentry_tmp,&gorup2PsListHead,entry)
		{
			if(memcmp(&groupToPs_entry->patten,patten,sizeof(rt_igmpHook_ignoreGroup_t))==0)
			{
				if(hwCb.groupDelCbEvt)
				{
					rtk_igmp_groupCbEvt_t groupCb;
					memset(&groupCb,0,sizeof(groupCb));
					groupCb.isIPv6= groupToPs_entry->patten.isIpv6;
					memcpy(groupCb.group ,groupToPs_entry->patten.ignGroupIpStart,sizeof(groupCb.group));
					IGMP_CTRL("GROUP DEL CALL BACK2 HW");
					hwCb.groupDelCbEvt(&groupCb);
				}

				return rtk_igmp_groupToPsTbl_free(groupToPs_entry);
			}
		}
		return FAIL;
	}

	return SUCCESS;
}


int _rtk_igmp_checkGroupToPsTbl(uint32 ipVersion,uint32 *_groupIp)
{
	
	rtk_igmp_userResvGroup_t *group2PsEt;
	int32 groupHit=0;
	
	list_for_each_entry(group2PsEt,&gorup2PsListHead,entry)
	{
		groupHit=0;
		if(ipVersion==IP_VERSION6 && group2PsEt->patten.isIpv6==1)
		{
			int msbDiffLo=FAIL;
			int msbDiffUp=FAIL;
			int msbDif=3;
			for(msbDif=3;msbDif>=0;msbDif--)
			{
				if(msbDiffLo==FAIL && group2PsEt->patten.ignGroupIpStart[msbDif]!=_groupIp[msbDif])
					msbDiffLo=msbDif;

				if(msbDiffUp==FAIL && group2PsEt->patten.ignGroupIpEnd[msbDif]!=_groupIp[msbDif])
					msbDiffUp=msbDif;
			}
			if((group2PsEt->patten.ignGroupIpStart[msbDiffLo] <= _groupIp[msbDiffLo])	&&	(_groupIp[msbDiffUp] <=group2PsEt->patten.ignGroupIpEnd[msbDiffUp]))
				groupHit=1;

			if(groupHit)
				return TRUE;
			
		}
		else if ((ipVersion==IP_VERSION4 && group2PsEt->patten.isIpv6==0))
		{
			if(group2PsEt->patten.ignGroupIpEnd[0]==0)
			{
				if((group2PsEt->patten.ignGroupIpStart[0] ) == (_groupIp[0]))
					groupHit=1;
			}
			else
			{
				if((group2PsEt->patten.ignGroupIpStart[0] <= _groupIp[0])	&&	(_groupIp[0] <=group2PsEt->patten.ignGroupIpEnd[0]))
					groupHit=1;
			}

			if(groupHit)
				return TRUE;

		}
	}

	return FALSE;
}

int rtk_igmp_groupToPsDump(struct seq_file *s, void *v)
{
	int i;
	PROC_PRINTF("GroupToPs Table:\n");
	for (i=0 ; i<IGMP_MAX_USER_REV2PS_GROUP_COUNT ;i++)
	{
		if(!gorup2Ps[i].valid)
			continue;

		if(gorup2Ps[i].patten.isIpv6)
		{
			PROC_PRINTF("[%d]ignGroupIpStart=%pI6   ignGroupIpEnd=%pI6\n",i,gorup2Ps[i].patten.ignGroupIpStart,gorup2Ps[i].patten.ignGroupIpEnd);
		}
		else
		{
			PROC_PRINTF("[%d]ignGroupIpStart=%pI4h  ignGroupIpEnd=%pI4h\n",i,gorup2Ps[i].patten.ignGroupIpStart,gorup2Ps[i].patten.ignGroupIpEnd);
		}
	}
	return SUCCESS;
}

#define GROUP_WEIGHT

int rtk_igmp_groupWeight_flashInit(void)
{
	int i;

	INIT_LIST_HEAD(&groupWeightFreeListHead);
	INIT_LIST_HEAD(&groupWeightListHead);
	
	for(i=0 ; i<IGMP_MAX_USER_GROUP_WEIGHT;i++ )
	{
		memset(&groupWeight[i],0,sizeof(rtk_igmp_groupWeight_t));
		groupWeight[i].groupWeightListIdx = i;
		INIT_LIST_HEAD(&groupWeight[i].entry);
		list_add_tail(&groupWeight[i].entry, &groupWeightFreeListHead);
	}
	return (SUCCESS);
}


int rtk_igmp_groupWeightTbl_free(rtk_igmp_groupWeight_t* grpEntry)
{

	list_del_init(&grpEntry->entry);
	list_add(&grpEntry->entry, &groupWeightFreeListHead);
	grpEntry->valid=0;
	return (SUCCESS);
}


rtk_igmp_groupWeight_t * rtk_igmp_groupWeightTbl_malloc(void)
{
	rtk_igmp_groupWeight_t *groupWeightentry;
	rtk_igmp_groupWeight_t *groupWeightentry_tmp;

	if(list_empty(&groupWeightFreeListHead))
	{
		IGMP_WARNING("all free IGMP rtk_igmp_groupWeightTbl_malloc are allocated...");
		return (NULL);
	}
	list_for_each_entry_safe(groupWeightentry,groupWeightentry_tmp,&groupWeightFreeListHead,entry)		//just return the first entry right behind of head
	{
		list_del_init(&groupWeightentry->entry);
		break;
	}
	memset(&groupWeightentry->patten,0,sizeof(groupWeightentry->patten));
	groupWeightentry->valid=1;
	return groupWeightentry;
}


int rtk_igmp_groupWeightTblAdd(rt_igmpHook_devInfo_t devInfo,rt_igmpHook_groupWeight_t* patten,uint32 *index)
{
	rtk_igmp_groupWeight_t *groupWeightentry;
	int32 ifIdx=FAIL;
	if(index==NULL)
		return FAIL;

	ifIdx = rtk_igmp_devInfoToIfIdx(devInfo);
	if(ifIdx==FAIL) return FAIL;


	list_for_each_entry(groupWeightentry,&groupWeightListHead,entry)
	{
		if(    memcmp(groupWeightentry->patten.weightGroupIpStart,patten->weightGroupIpStart,sizeof(groupWeightentry->patten.weightGroupIpStart))==0 
			&& memcmp(groupWeightentry->patten.weightGroupIpEnd,patten->weightGroupIpEnd,sizeof(groupWeightentry->patten.weightGroupIpEnd))==0
			&& groupWeightentry->patten.isIpv6 ==patten->isIpv6
			&& ifIdx==groupWeightentry->devIdx)
		{
			*index = groupWeightentry->groupWeightListIdx;
			return SUCCESS;
		}
	}

	groupWeightentry = rtk_igmp_groupWeightTbl_malloc();
	if(groupWeightentry)
	{
		list_add(&groupWeightentry->entry,&groupWeightListHead);
		memcpy(&groupWeightentry->patten,patten,sizeof(groupWeightentry->patten));
		groupWeightentry->devIdx = ifIdx;
		*index = groupWeightentry->groupWeightListIdx;
	}
	else
		return FAIL;


	return SUCCESS;
}

int rtk_igmp_groupWeightTblDel(rt_igmpHook_devInfo_t devInfo,rt_igmpHook_groupWeight_t* patten,uint32 index)
{
	if(patten==NULL)
	{
		if(index<0 || IGMP_MAX_USER_GROUP_WEIGHT <= index)
			return FAIL;
		if(groupWeight[index].valid==0)
			return FAIL;

		return rtk_igmp_groupWeightTbl_free(&groupWeight[index]);

	}
	else
	{
		rtk_igmp_groupWeight_t *groupWeight_entry =NULL;
		rtk_igmp_groupWeight_t *groupWeightentry_tmp=NULL;
		int32 ifIdx=FAIL;

		ifIdx = rtk_igmp_devInfoToIfIdx(devInfo);
		if(ifIdx==FAIL) return FAIL;

		
		list_for_each_entry_safe(groupWeight_entry,groupWeightentry_tmp,&groupWeightListHead,entry)
		{
			if(memcmp(&groupWeight_entry->patten,patten,sizeof(rt_igmpHook_groupWeight_t))==0 && ifIdx==groupWeight_entry->devIdx)
			{

				return rtk_igmp_groupWeightTbl_free(groupWeight_entry);
			}
		}
		return FAIL;
	}

	return SUCCESS;
}


int rtk_igmp_weightOverCheck(rtk_igmp_multicastDeviceInfo_t* devInfocfg,rtk_igmp_multicastBrDeviceInfo_t* brDevInfocfg ,int32 ipVersion,uint32 *groupAddr)
{
	rt_igmpHook_devInfo_t devInfo={{0}};
	rt_igmpHook_devInfo_t brdevInfo={{0}};
	int32 devGetIdx=0,brGetIdx=0;
	int32 isIpv6=0;
	int32 devTotalWeight=0,brTotalWeigh=0;
	int32 devGroupWeight=0,brGroupWeight=0;
	int32 devOverWeightDrop=0,brOverWeightDrop=0;
	int32 devMaxWeight=0,brMaxWeight=0;
	rt_igmpHook_groupStatistic_t statisticInfo;
	int32 ret=SUCCESS;

	devInfo.devIfidx=devInfocfg->ifindex;
	brdevInfo.devIfidx =brDevInfocfg->ifindex;
	if(ipVersion==IP_VERSION6)
	{
		isIpv6=1;
		devMaxWeight=devInfocfg->devConf.igmp6.igmp6MaxWeight;
		brMaxWeight=brDevInfocfg->brDevConf.sysIp6MaxWeight;
		devOverWeightDrop = devInfocfg->devConf.igmp6.igmp6OverWeightDrop;
		brOverWeightDrop = brDevInfocfg->brDevConf.sysIp6OverWeightDrop;
	}
	else
	{
		isIpv6=0;
		devMaxWeight=devInfocfg->devConf.igmp.igmpMaxWeight;
		brMaxWeight=brDevInfocfg->brDevConf.sysIp4MaxWeight;		
		devOverWeightDrop = devInfocfg->devConf.igmp.igmpOverWeightDrop;
		brOverWeightDrop = brDevInfocfg->brDevConf.sysIp4OverWeightDrop;
	}

	devGroupWeight =_rtk_igmp_checkGroupWeightTbl(devInfo.devIfidx,ipVersion,groupAddr);
	brGroupWeight =_rtk_igmp_checkGroupWeightTbl(brdevInfo.devIfidx,ipVersion,groupAddr);

	while(1)
	{
		if(rtk_igmp_nextValidGroupStatistic_get(devInfo,&devGetIdx,isIpv6,&statisticInfo)==SUCCESS && devGetIdx>=0)
		{
			devTotalWeight+=_rtk_igmp_checkGroupWeightTbl(devInfo.devIfidx,ipVersion,statisticInfo.groupAddr);
			devGetIdx++;
		}
		else
			break;
	}


	IGMP_CTRL("devTotalWeight=%d WeightLimit=%d devGroupWeight=%d",devTotalWeight,devMaxWeight,devGroupWeight);
	if(devMaxWeight && ((devTotalWeight+devGroupWeight)>devMaxWeight))
	{
		if(isIpv6)
			devInfocfg->devStatistic.igmp6WeightExceedCnt++;
		else		
			devInfocfg->devStatistic.igmp4WeightExceedCnt++;

		if(devOverWeightDrop)
			ret=FAIL;
	}

	while(1)
	{
		if(rtk_igmp_nextValidGroupStatistic_get(brdevInfo,&brGetIdx,isIpv6,&statisticInfo)==SUCCESS && brGetIdx>=0)
		{
			brTotalWeigh+=_rtk_igmp_checkGroupWeightTbl(brdevInfo.devIfidx,ipVersion,statisticInfo.groupAddr);
			brGetIdx++;
		}
		else
			break;
	}

	IGMP_CTRL("brTotalWeigh=%d WeightLimit=%d brGroupWeight=%d",brTotalWeigh,brMaxWeight,brGroupWeight);
	if(brMaxWeight && ((brTotalWeigh+brGroupWeight)>brMaxWeight))
	{
		if(isIpv6)
			brDevInfocfg->brStatistic.igmp6WeightExceedCnt++;
		else		
			brDevInfocfg->brStatistic.igmp4WeightExceedCnt++;

		if(brOverWeightDrop)
			ret=FAIL;
	}

	return ret;

}

//return weight
int _rtk_igmp_checkGroupWeightTbl(uint32 ifIdx,uint32 ipVersion,uint32 *_groupIp)
{
	
	rtk_igmp_groupWeight_t *groupWeightEt;
	int32 groupHit=0;

	list_for_each_entry(groupWeightEt,&groupWeightListHead,entry)
	{
		if(groupWeightEt->devIdx!=ifIdx)
			continue;
		groupHit=0;
		if(ipVersion==IP_VERSION6 && groupWeightEt->patten.isIpv6==1)
		{
			int msbDiffLo=FAIL;
			int msbDiffUp=FAIL;
			int msbDif=3;
			for(msbDif=3;msbDif>=0;msbDif--)
			{
				if(msbDiffLo==FAIL && groupWeightEt->patten.weightGroupIpStart[msbDif]!=_groupIp[msbDif])
					msbDiffLo=msbDif;

				if(msbDiffUp==FAIL && groupWeightEt->patten.weightGroupIpEnd[msbDif]!=_groupIp[msbDif])
					msbDiffUp=msbDif;

			}
			if((groupWeightEt->patten.weightGroupIpStart[msbDiffLo] <= _groupIp[msbDiffLo])	&&	(_groupIp[msbDiffUp] <=groupWeightEt->patten.weightGroupIpEnd[msbDiffUp]))
				groupHit=1;

			if(groupHit)
				return groupWeightEt->patten.weight;
			
		}
		else if ((ipVersion==IP_VERSION4 && groupWeightEt->patten.isIpv6==0))
		{
			if(groupWeightEt->patten.weightGroupIpEnd[0]==0)
			{
				if((groupWeightEt->patten.weightGroupIpStart[0] ) == (_groupIp[0]))
					groupHit=1;
			}
			else
			{
				if((groupWeightEt->patten.weightGroupIpStart[0] <= _groupIp[0])	&&	(_groupIp[0] <=groupWeightEt->patten.weightGroupIpEnd[0]))
					groupHit=1;
			}

			if(groupHit)
				return groupWeightEt->patten.weight;

		}
	}

	IGMP_CTRL("return Default Weight =0 ");
	//default weight
	return 0;
}

int rtk_igmp_groupWeightDump(struct seq_file *s, void *v)
{
	int i;
	struct net_device * dev;

	PROC_PRINTF("groupWeight Table:\n");
	for (i=0 ; i<IGMP_MAX_USER_GROUP_WEIGHT ;i++)
	{
		if(!groupWeight[i].valid)
			continue;

		dev=rtk_igmp_devIfidx_to_dev(groupWeight[i].devIdx);

		if(dev)
		{
			if(groupWeight[i].patten.isIpv6)
			{
				PROC_PRINTF("[%d]%s weightGroupIpStart=%pI6   weightGroupIpEnd=%pI6\n",i,dev->name,groupWeight[i].patten.weightGroupIpStart,groupWeight[i].patten.weightGroupIpEnd);
			}
			else
			{
				PROC_PRINTF("[%d]%s weightGroupIpStart=%pI4h  weightGroupIpEnd=%pI4h\n",i,dev->name,groupWeight[i].patten.weightGroupIpStart,groupWeight[i].patten.weightGroupIpEnd);
			}
		}
	}
	return SUCCESS;
}

#define DEV


int rtk_igmp_BrDevConfig_set(int devifid ,rt_igmpHook_br_control_list_t type , uint32 value)
{
	struct rtk_igmp_multicastModule* BrDevInfo = _rtk_igmp_devIfidx_to_BrDevInfo(devifid);
	struct net_bridge *br =NULL;
	struct net_bridge_port *p;
	if(BrDevInfo== NULL) return FAIL;
	switch ( type )
	{

	    case RT_BR_MAX_GROUP_SIZE:
			BrDevInfo->brDevInfo.brDevConf.sysMaxGroupSize = value;
	        break;

	    case RT_BR_MAX_IP4_GROUP_SIZE:
			BrDevInfo->brDevInfo.brDevConf.sysIp4MaxGroupSize= value;

	        break;

	    case RT_BR_MAX_IP6_GROUP_SIZE:
			BrDevInfo->brDevInfo.brDevConf.sysIp6MaxGroupSize= value;

	        break;

	    case RT_BR_MAX_CLIENT_SIZE:
			BrDevInfo->brDevInfo.brDevConf.sysMaxClientSize= value;

	        break;

	    case RT_BR_MAX_IP4_CLIENT_SIZE:
			BrDevInfo->brDevInfo.brDevConf.sysIp4MaxClientSize= value;

	        break;

	    case RT_BR_MAX_IP6_CLIENT_SIZE:
			BrDevInfo->brDevInfo.brDevConf.sysIp6MaxClientSize= value;

	        break;
		case RT_BR_PER_IP4_CLIENT_MAX_JOIN_GROUP:
			BrDevInfo->brDevInfo.brDevConf.perIp4ClientMaxJoinGroupSize= value;

			break;
		case RT_BR_PER_IP6_CLIENT_MAX_JOIN_GROUP:
			BrDevInfo->brDevInfo.brDevConf.perIp6ClientMaxJoinGroupSize= value;

			break;

		case RT_BR_MAX_IP4_WEIGHT:
			BrDevInfo->brDevInfo.brDevConf.sysIp4MaxWeight= value;
		
			break;
		case RT_BR_MAX_IP6_WEIGHT:
			BrDevInfo->brDevInfo.brDevConf.sysIp6MaxWeight= value;
			
		case RT_BR_DROP_IP4_OVER_WEIGHT:
			BrDevInfo->brDevInfo.brDevConf.sysIp4OverWeightDrop= value;
		
			break;
		case RT_BR_DROP_IP6_OVER_WEIGHT:
			BrDevInfo->brDevInfo.brDevConf.sysIp6OverWeightDrop= value;		
			break;


		case RT_BR_DROP_IGMPV1:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_devConfig_set(p->dev->ifindex, 0,RT_DROP_IGMPV1 ,value);
				}
			}

			break;
		case RT_BR_DROP_IGMPV2:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_devConfig_set(p->dev->ifindex, 0,RT_DROP_IGMPV2 ,value);
				}
			}

			break;
		case RT_BR_DROP_IGMPV3:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_devConfig_set(p->dev->ifindex, 0,RT_DROP_IGMPV3 ,value);
				}
			}

			break;
		case RT_BR_DROP_MLDV1:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_devConfig_set(p->dev->ifindex, 1,RT_DROP_MLDV1 ,value);
				}
			}

			break;
		case RT_BR_DROP_MLDV2:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_devConfig_set(p->dev->ifindex, 1,RT_DROP_MLDV2 ,value);
				}
			}

			break;
		case RT_BR_SNOOPING_DISABLE:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_devConfig_set(p->dev->ifindex, 0,RT_SNOOPING_DISABLE ,value);
					rtk_igmp_devConfig_set(p->dev->ifindex, 1,RT_SNOOPING_DISABLE ,value);
				}
			}
			break;
		case RT_BR_FLUSH_CONF:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_reset_devConfig(p->dev->ifindex, 0);
					rtk_igmp_reset_devConfig(p->dev->ifindex, 1);
				}
			}

			break;
		case RT_BR_FLUSH_MDB:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_flush_Br_mdb(BrDevInfo->brDevInfo.ifindex,0);
					rtk_igmp_flush_Br_mdb(BrDevInfo->brDevInfo.ifindex,1);
				}
			}

			break;
		case RT_BR_RATE_LIMIT:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_devConfig_set(p->dev->ifindex, 0,RT_RATE_LIMIT, value);
					rtk_igmp_devConfig_set(p->dev->ifindex, 1,RT_RATE_LIMIT, value);
				}
			}

			break;
		case RT_BR_FAST_LEAVE:
			br = netdev_priv(BrDevInfo->brDevInfo.dev);
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list) {
					rtk_igmp_devConfig_set(p->dev->ifindex, 0,RT_FAST_LEAVE, value);
					rtk_igmp_devConfig_set(p->dev->ifindex, 1,RT_FAST_LEAVE, value);
				}
			}

			break;

	    default:
			IGMP_WARNING("not support type");
	}
	return SUCCESS;
}


int rtk_igmp_BrDevConfig_get(int devifid ,rt_igmpHook_br_control_list_t type , uint32* value)
{
	struct rtk_igmp_multicastModule* BrDevInfo = _rtk_igmp_devIfidx_to_BrDevInfo(devifid);
	if(BrDevInfo== NULL) return FAIL;
	switch ( type )
	{

	    case RT_BR_MAX_GROUP_SIZE:
			*value = BrDevInfo->brDevInfo.brDevConf.sysMaxGroupSize;
	        break;

	    case RT_BR_MAX_IP4_GROUP_SIZE:
			*value = BrDevInfo->brDevInfo.brDevConf.sysIp4MaxGroupSize;

	        break;

	    case RT_BR_MAX_IP6_GROUP_SIZE:
			*value = BrDevInfo->brDevInfo.brDevConf.sysIp6MaxGroupSize;

	        break;

	    case RT_BR_MAX_CLIENT_SIZE:
			*value = BrDevInfo->brDevInfo.brDevConf.sysMaxClientSize;

	        break;

	    case RT_BR_MAX_IP4_CLIENT_SIZE:
			*value = BrDevInfo->brDevInfo.brDevConf.sysIp4MaxClientSize;

	        break;

	    case RT_BR_MAX_IP6_CLIENT_SIZE:
			*value = BrDevInfo->brDevInfo.brDevConf.sysIp6MaxClientSize;

	        break;
		case RT_BR_PER_IP4_CLIENT_MAX_JOIN_GROUP:
			*value =BrDevInfo->brDevInfo.brDevConf.perIp4ClientMaxJoinGroupSize;

			break;
		case RT_BR_PER_IP6_CLIENT_MAX_JOIN_GROUP:
			*value = BrDevInfo->brDevInfo.brDevConf.perIp6ClientMaxJoinGroupSize;

			break;

	    default:
			IGMP_WARNING("not support type:%d to get");
			return FAIL;
	}
	return SUCCESS;
}


int rtk_igmp_anyDevDisSnooping(uint32 isIpv6)
{

	int i;
	for( i=0 ; i< IGMP_MAX_LAN_DEV_NUM; i++)
	{
		if(rtk_igmp_lanDevInfo[i].dev==NULL)
			continue;
		if(isIpv6)
		{
			if(rtk_igmp_lanDevInfo[i].devConf.igmp6.igmp6SnoopingDisable)
				return TRUE;
		}
		else
		{
			if(rtk_igmp_lanDevInfo[i].devConf.igmp.igmpSnoopingDisable)
				return TRUE;
		}
	}

	for( i=0 ; i< IGMP_MAX_WAN_DEV_NUM; i++)
	{
		if(rtk_igmp_wanDevInfo[i].dev==NULL)
			continue;
		if(isIpv6)
		{
			if(rtk_igmp_wanDevInfo[i].devConf.igmp6.igmp6SnoopingDisable)
				return TRUE;
		}
		else
		{
			if(rtk_igmp_wanDevInfo[i].devConf.igmp.igmpSnoopingDisable)
				return TRUE;
		}
	}


	return FALSE;
}

int rtk_igmp_devConfig_set(int devifid , uint32 isIpv6 ,rt_igmpHook_control_list_t type , uint32 value)
{

	rtk_igmp_multicastDeviceInfo_t* devInfo = _rtk_igmp_devIfidx_to_devInfo(devifid);
	if(devInfo== NULL) return FAIL;


	switch ( type )
	{

	    case RT_SNOOPING_DISABLE:
			{
				uint32 statusChange=0;
				if(isIpv6)
				{
					if(devInfo->devConf.igmp6.igmp6SnoopingDisable!=value)
					{
						statusChange=1;
						devInfo->devConf.igmp6.igmp6SnoopingDisable=value;
					}
				}
				else
				{
					if(devInfo->devConf.igmp.igmpSnoopingDisable!=value)
					{
						statusChange=1;
						devInfo->devConf.igmp.igmpSnoopingDisable=value;
					}
				}
				if(statusChange)
				{
					rtk_igmp_flow_reflush_update(FAIL,NULL);
					if(rtk_igmp_anyDevDisSnooping(isIpv6))
					{
						if(hwCb.hwUnknownIpMcAction)
						{
							rtk_igmp_unknownMcCbEvt_t unknownCtrl;
							unknownCtrl.isIpv6=isIpv6;
							unknownCtrl.unknownAct=UNKNOWN_MC_TRAP;
							hwCb.hwUnknownIpMcAction(&unknownCtrl);
						}
					}
					else
					{
						if(hwCb.hwUnknownIpMcAction)
						{
							rtk_igmp_unknownMcCbEvt_t unknownCtrl;
							unknownCtrl.isIpv6=isIpv6;
							unknownCtrl.unknownAct=UNKNOWN_MC_DROP;
							hwCb.hwUnknownIpMcAction(&unknownCtrl);
						}
					}
				}
	 	   	}
	        break;

	    case RT_DROP_IGMPV1:
			if(!isIpv6)
				devInfo->devConf.igmp.igmpVerDrp_v1=value;
	        break;

	    case RT_DROP_IGMPV2:
			if(!isIpv6)
				devInfo->devConf.igmp.igmpVerDrp_v2=value;
	        break;

	    case RT_DROP_IGMPV3:
			if(!isIpv6)
				devInfo->devConf.igmp.igmpVerDrp_v3=value;
	        break;

	    case RT_IGNORE_IGMPV1:
			if(!isIpv6)
				devInfo->devConf.igmp.igmpVerIgr_v1=value;
	        break;

	    case RT_IGNORE_IGMPV2:
			if(!isIpv6)
				devInfo->devConf.igmp.igmpVerIgr_v2=value;
	        break;

		case RT_IGNORE_IGMPV3:
			if(!isIpv6)
				devInfo->devConf.igmp.igmpVerIgr_v3=value;
			break;

		case RT_DROP_MLDV1:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6VerDrp_v1=value;
			break;

		case RT_DROP_MLDV2:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6VerDrp_v2=value;
			break;

		case RT_IGNORE_MLDV1:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6VerIgr_v1=value;
			break;

		case RT_IGNORE_MLDV2:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6VerIgr_v2=value;
			break;

		case RT_REPORT_DROP:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6ReportDrop=value;
			else
				devInfo->devConf.igmp.igmpReportDrop=value;
			break;

		case RT_FAST_LEAVE:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6fastLeave=value;
			else
				devInfo->devConf.igmp.igmpfastLeave=value;
			break;

		case RT_CARE_SOURCE:
			if(isIpv6)
				devInfo->devConf.igmp6.mldv2CareSource=value;
			else
				devInfo->devConf.igmp.igmpv3CareSource=value;
			break;

		case RT_MAX_GROUP_SIZE:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6MaxGroupSize=value;
			else
				devInfo->devConf.igmp.igmpMaxGroupSize=value;
			break;

		case RT_MC_MEMBER_AGING_INTERVAL:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6McMemberAgingInterval=value;
			else
				devInfo->devConf.igmp.igmpMcMemberAgingInterval=value;
			break;
			
		case RT_MC_LAST_MEMBER_AGING_INTERVAL:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6McLastMemberAgingInterval=value;
			else
				devInfo->devConf.igmp.igmpMcLastMemberAgingInterval=value;
			break;

		case RT_RATE_LIMIT:
			if(isIpv6){
				devInfo->devConf.igmp6.ratelimit.rate=value;
				devInfo->devConf.igmp6.ratelimit.counter=0;
				devInfo->devConf.igmp6.ratelimit.cycle_start_time=jiffies;
			}else{
				devInfo->devConf.igmp.ratelimit.rate=value;
				devInfo->devConf.igmp.ratelimit.counter=0;
				devInfo->devConf.igmp.ratelimit.cycle_start_time=jiffies;
			}
			break;

		case RT_FLUSH_CONF:
			if(isIpv6)
				rtk_igmp_reset_devConfig(devifid, 1);
			else
				rtk_igmp_reset_devConfig(devifid, 0);
			break;

		case RT_FLUSH_MDB:
			//clear data base
			rtk_igmp_clearDataBaseByDev(devifid);
			rtk_igmp_flow_reflush_update(FAIL,NULL);

			break;

		case RT_MAX_WEIGHT:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6MaxWeight=value;
			else
				devInfo->devConf.igmp.igmpMaxWeight=value;
			break;		

		case RT_DROP_OVER_WEIGHT:
			if(isIpv6)
				devInfo->devConf.igmp6.igmp6OverWeightDrop=value;
			else
				devInfo->devConf.igmp.igmpOverWeightDrop=value;
			break;		


	    default:
			IGMP_WARNING("not support type");
	}


	return SUCCESS;

}


int rtk_igmp_devConfig_get(int devifid , uint32 isIpv6 ,rt_igmpHook_control_list_t type , uint32* value)
{

	rtk_igmp_multicastDeviceInfo_t* devInfo = _rtk_igmp_devIfidx_to_devInfo(devifid);
	if(devInfo== NULL) return FAIL;

	switch ( type )
	{

	    case RT_SNOOPING_DISABLE:
			{
				if(isIpv6)
				{
					*value=devInfo->devConf.igmp6.igmp6SnoopingDisable;
				}
				else
				{
					*value=devInfo->devConf.igmp.igmpSnoopingDisable;
				}
	 	   	}
	        break;

	    case RT_DROP_IGMPV1:
			if(!isIpv6)
				*value=devInfo->devConf.igmp.igmpVerDrp_v1;
			else
				return FAIL;
	        break;

	    case RT_DROP_IGMPV2:
			if(!isIpv6)
				*value=devInfo->devConf.igmp.igmpVerDrp_v2;
			else
				return FAIL;
	        break;

	    case RT_DROP_IGMPV3:
			if(!isIpv6)
				*value=devInfo->devConf.igmp.igmpVerDrp_v3;
			else
				return FAIL;
	        break;

	    case RT_IGNORE_IGMPV1:
			if(!isIpv6)
				*value=devInfo->devConf.igmp.igmpVerIgr_v1;
			else
				return FAIL;
	        break;

	    case RT_IGNORE_IGMPV2:
			if(!isIpv6)
				*value=devInfo->devConf.igmp.igmpVerIgr_v2;
			else
				return FAIL;			
	        break;

		case RT_IGNORE_IGMPV3:
			if(!isIpv6)
				*value=devInfo->devConf.igmp.igmpVerIgr_v3;
			else
				return FAIL;			
			break;

		case RT_DROP_MLDV1:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6VerDrp_v1;
			else
				return FAIL;			
			break;

		case RT_DROP_MLDV2:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6VerDrp_v2;
			break;

		case RT_IGNORE_MLDV1:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6VerIgr_v1;
			else
				return FAIL;			
			break;

		case RT_IGNORE_MLDV2:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6VerIgr_v2;
			else
				return FAIL;			
			break;

		case RT_REPORT_DROP:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6ReportDrop;
			else
				*value=devInfo->devConf.igmp.igmpReportDrop;
			break;

		case RT_FAST_LEAVE:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6fastLeave;
			else
				*value=devInfo->devConf.igmp.igmpfastLeave;
			break;

		case RT_CARE_SOURCE:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.mldv2CareSource;
			else
				*value=devInfo->devConf.igmp.igmpv3CareSource;
			break;

		case RT_MAX_GROUP_SIZE:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6MaxGroupSize;
			else
				*value=devInfo->devConf.igmp.igmpMaxGroupSize;
			break;

		case RT_MC_MEMBER_AGING_INTERVAL:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6McMemberAgingInterval;
			else
				*value=devInfo->devConf.igmp.igmpMcMemberAgingInterval;
			break;
			
		case RT_MC_LAST_MEMBER_AGING_INTERVAL:
			if(isIpv6)
				*value=devInfo->devConf.igmp6.igmp6McLastMemberAgingInterval;
			else
				*value=devInfo->devConf.igmp.igmpMcLastMemberAgingInterval;
			break;

		case RT_RATE_LIMIT:
			if(isIpv6){
				*value=devInfo->devConf.igmp6.ratelimit.rate;
				devInfo->devConf.igmp6.ratelimit.counter=0;
				devInfo->devConf.igmp6.ratelimit.cycle_start_time=jiffies;
			}else{
				*value=devInfo->devConf.igmp.ratelimit.rate;
				devInfo->devConf.igmp.ratelimit.counter=0;
				devInfo->devConf.igmp.ratelimit.cycle_start_time=jiffies;
			}
			break;

	    default:
			IGMP_WARNING("not support type:%d to get",type);
			return FAIL;
	}


	return SUCCESS;

}



int rtk_igmp_dump_devConfig(int ifidx,struct seq_file *s, void *v)
{
	rtk_igmp_DevConfdb_t *devconf;
	rtk_igmp_whiteListEntry_t *white_entry;
	int i;


	PROC_PRINTF("=================================================\n");
	PROC_PRINTF("BR INFO:\n");
	for( i=0 ; i< IGMP_MAX_BR_MODULE_NUM; i++)
	{
		if(!rtk_igmp_mCastModuleArray[i].validBit)
			continue;
		if(ifidx !=FAIL && rtk_igmp_mCastModuleArray[i].brDevInfo.ifindex!=ifidx)
			continue;
		PROC_PRINTF("BrDev:[%s] ifidx=%d \n\tGROUP_LINIT:[total/ipv4/ipv6][%d/%d/%d] CLIENT_LIMIT[total/ipv4/ipv6][%d/%d/%d] PER_CLIENT_JOIN_GROUP_LIMIT[ipv4/ipv6][%d/%d]\n",rtk_igmp_mCastModuleArray[i].brDevInfo.devName,rtk_igmp_mCastModuleArray[i].brDevInfo.ifindex
			,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysMaxGroupSize,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysIp4MaxGroupSize,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysIp6MaxGroupSize
			,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysMaxClientSize,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysIp4MaxClientSize,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysIp6MaxClientSize
			,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.perIp4ClientMaxJoinGroupSize,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.perIp6ClientMaxJoinGroupSize);
		PROC_PRINTF("MAX_WEIGHT:[ipv4/ipv6][%d/%d] DROP_OVER_WEIGHT[ipv4/ipv6][%d/%d]",
			rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysIp4MaxWeight,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysIp6MaxWeight,
			rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysIp4OverWeightDrop,rtk_igmp_mCastModuleArray[i].brDevInfo.brDevConf.sysIp6OverWeightDrop);
		PROC_PRINTF("\n");
	}


	PROC_PRINTF("=================================================\n");
	PROC_PRINTF("WAN INFO:\n");
	for( i=0 ; i< IGMP_MAX_WAN_DEV_NUM; i++)
	{
		if(rtk_igmp_wanDevInfo[i].dev==NULL)
			continue;
		if(ifidx !=FAIL && rtk_igmp_wanDevInfo[i].ifindex!=ifidx)
			continue;

		PROC_PRINTF("WAN-Dev:[%s]  ifidx=%d ",rtk_igmp_wanDevInfo[i].devName,rtk_igmp_wanDevInfo[i].ifindex);
		if(rtk_igmp_wanDevInfo[i].bind2BrModuleIdx!=FAIL)
			PROC_PRINTF("Mapping-to [%s] \n",rtk_igmp_mCastModuleArray[rtk_igmp_wanDevInfo[i].bind2BrModuleIdx].brDevInfo.devName);
		else
			PROC_PRINTF("\n");

		devconf = &rtk_igmp_wanDevInfo[i].devConf;
		PROC_PRINTF("\tIGMP:\n");
		PROC_PRINTF("\t");

		if(devconf->igmp.igmpVerIgr_v1)
			PROC_PRINTF("/IGNORE_IGMPV1");
		if(devconf->igmp.igmpVerIgr_v2)
			PROC_PRINTF("/IGNORE_IGMPV2");
		if(devconf->igmp.igmpVerIgr_v3)
			PROC_PRINTF("/IGNORE_IGMPV3");
		if(devconf->igmp.igmpVerDrp_v1)
			PROC_PRINTF("/DROP_IGMPV1");
		if(devconf->igmp.igmpVerDrp_v2)
			PROC_PRINTF("/DROP_IGMPV2");
		if(devconf->igmp.igmpVerDrp_v3)
			PROC_PRINTF("/DROP_IGMPV3");
		if(devconf->igmp.igmpReportDrop)
			PROC_PRINTF("/REPORT_DROP");
		if(devconf->igmp.igmpSnoopingDisable)
			PROC_PRINTF("/SNOOPING_DISABLE");
		if(devconf->igmp.igmpfastLeave)
			PROC_PRINTF("/FAST_LEAVE");
		if(devconf->igmp.igmpv3CareSource)
			PROC_PRINTF("/CARE_SOURCE");
		if(devconf->igmp.igmpMaxGroupSize)
			PROC_PRINTF("/MAX_GROUP_SIZE:%d",devconf->igmp.igmpMaxGroupSize);
		if(devconf->igmp.igmpMcMemberAgingInterval)
			PROC_PRINTF("/MC_MEMBER_AGING_INTERVAL:%d",devconf->igmp.igmpMcMemberAgingInterval);
		if(devconf->igmp.igmpMcLastMemberAgingInterval)
			PROC_PRINTF("/MC_LAST_MEMBER_AGING_INTERVAL:%d",devconf->igmp.igmpMcLastMemberAgingInterval);		
		if(devconf->igmp.ratelimit.rate)
			PROC_PRINTF("/RATE_LIMIT:%d",devconf->igmp.ratelimit.rate);
		if(devconf->igmp.igmpMaxWeight)
			PROC_PRINTF("/MAX_WEIGHT:%d",devconf->igmp.igmpMaxWeight);		
		if(devconf->igmp.igmpOverWeightDrop==0)
			PROC_PRINTF("/DROP_OVER_WEIGHT:%d",devconf->igmp.igmpOverWeightDrop);
		
		PROC_PRINTF("\n");
		PROC_PRINTF("\tIGMP6:\n");
		PROC_PRINTF("\t");
		if(devconf->igmp6.igmp6VerIgr_v1)
			PROC_PRINTF("/IGNORE_MLDV1");
		if(devconf->igmp6.igmp6VerIgr_v2)
			PROC_PRINTF("/IGNORE_MLDV2");
		if(devconf->igmp6.igmp6VerDrp_v1)
			PROC_PRINTF("/DROP_MLDV1");
		if(devconf->igmp6.igmp6VerDrp_v2)
			PROC_PRINTF("/DROP_MLDV2");
		if(devconf->igmp6.igmp6ReportDrop)
			PROC_PRINTF("/REPORT_DROP");
		if(devconf->igmp6.igmp6SnoopingDisable)
			PROC_PRINTF("/SNOOPING_DISABLE");
		if(devconf->igmp6.igmp6fastLeave)
			PROC_PRINTF("/FAST_LEAVE");
		if(devconf->igmp6.mldv2CareSource)
			PROC_PRINTF("/CARE_SOURCE");
		if(devconf->igmp6.igmp6MaxGroupSize)
			PROC_PRINTF("/MAX_GROUP_SIZE:%d",devconf->igmp6.igmp6MaxGroupSize);
		if(devconf->igmp6.igmp6McMemberAgingInterval)
			PROC_PRINTF("/MC_MEMBER_AGING_INTERVAL:%d",devconf->igmp6.igmp6McMemberAgingInterval);
		if(devconf->igmp6.igmp6McLastMemberAgingInterval)
			PROC_PRINTF("/MC_LAST_MEMBER_AGING_INTERVAL:%d",devconf->igmp6.igmp6McLastMemberAgingInterval);		
		if(devconf->igmp6.ratelimit.rate)
			PROC_PRINTF("/RATE_LIMIT:%d",devconf->igmp6.ratelimit.rate);
		if(devconf->igmp6.igmp6MaxWeight)
			PROC_PRINTF("/MAX_WEIGHT:%d",devconf->igmp6.igmp6MaxWeight);		
		if(devconf->igmp6.igmp6OverWeightDrop==0)
			PROC_PRINTF("/DROP_OVER_WEIGHT:%d",devconf->igmp6.igmp6OverWeightDrop);		
		PROC_PRINTF("\n");

		PROC_PRINTF("\tWhiteList: \n");
		list_for_each_entry(white_entry,&rtk_igmp_wanDevInfo[i].devConf.igmp.white,entry)
		{
			PROC_PRINTF("\t[%d]",white_entry->whiteListIdx);
			if(white_entry->patten.gipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("gip:%pI6 gipE:%pI6 ",white_entry->patten.wlGroupIpStart,white_entry->patten.wlGroupIpEnd);
				else
					PROC_PRINTF("gip:%pI4h gipE:%pI4h ",white_entry->patten.wlGroupIpStart,white_entry->patten.wlGroupIpEnd);
			}
			if(white_entry->patten.sipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("sip:%pI6 sipEnd:%pI6 ",white_entry->patten.wlSipStart,white_entry->patten.wlSipEnd);
				else
					PROC_PRINTF("sip:%pI4h sipEnd:%pI4h ",white_entry->patten.wlSipStart,white_entry->patten.wlSipEnd);
			}
			if(white_entry->patten.mcSipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("mcSip:%pI6 mcSipEnd:%pI6 ",white_entry->patten.wlMcSip,white_entry->patten.wlMcSipEnd);
				else
					PROC_PRINTF("mcSip:%pI4h mcSipEnd:%pI4h ",white_entry->patten.wlMcSip,white_entry->patten.wlMcSipEnd);
			}
			if(white_entry->patten.smacChk)
			{

				PROC_PRINTF("smac:%pM smacM:%pM ",white_entry->patten.smac,white_entry->patten.smacMask);
			}
			PROC_PRINTF("\n");
		}
		list_for_each_entry(white_entry,&rtk_igmp_wanDevInfo[i].devConf.igmp6.white,entry)
		{
			PROC_PRINTF("\t[%d]",white_entry->whiteListIdx);
			if(white_entry->patten.gipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("gip:%pI6 gipE:%pI6 ",white_entry->patten.wlGroupIpStart,white_entry->patten.wlGroupIpEnd);
				else
					PROC_PRINTF("gip:%pI4h gipE:%pI4h ",white_entry->patten.wlGroupIpStart,white_entry->patten.wlGroupIpEnd);
			}
			if(white_entry->patten.sipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("sip:%pI6 sipEnd:%pI6 ",white_entry->patten.wlSipStart,white_entry->patten.wlSipEnd);
				else
					PROC_PRINTF("sip:%pI4h sipEnd:%pI4h ",white_entry->patten.wlSipStart,white_entry->patten.wlSipEnd);
			}
			if(white_entry->patten.mcSipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("mcSip:%pI6 mcSipEnd:%pI6 ",white_entry->patten.wlMcSip,white_entry->patten.wlMcSipEnd);
				else
					PROC_PRINTF("mcSip:%pI4h mcSipEnd:%pI4h ",white_entry->patten.wlMcSip,white_entry->patten.wlMcSipEnd);
			}
			if(white_entry->patten.smacChk)
			{

				PROC_PRINTF("smac:%pM smacM:%pM ",white_entry->patten.smac,white_entry->patten.smacMask);
			}
			PROC_PRINTF("\n");
		}		

	}

	PROC_PRINTF("=================================================\n");
	PROC_PRINTF("LAN INFO:\n");
	for( i=0 ; i< IGMP_MAX_LAN_DEV_NUM; i++)
	{
		if(rtk_igmp_lanDevInfo[i].dev==NULL)
			continue;
		if(ifidx !=FAIL && rtk_igmp_lanDevInfo[i].ifindex!=ifidx)
			continue;

		PROC_PRINTF("LAN-Dev:[%s]  ifidx=%d ",rtk_igmp_lanDevInfo[i].devName,rtk_igmp_lanDevInfo[i].ifindex);
		PROC_PRINTF("\n");

		devconf = &rtk_igmp_lanDevInfo[i].devConf;
		PROC_PRINTF("\tIGMP:\n");
		PROC_PRINTF("\t");

		if(devconf->igmp.igmpVerIgr_v1)
			PROC_PRINTF("/IGNORE_IGMPV1");
		if(devconf->igmp.igmpVerIgr_v2)
			PROC_PRINTF("/IGNORE_IGMPV2");
		if(devconf->igmp.igmpVerIgr_v3)
			PROC_PRINTF("/IGNORE_IGMPV3");
		if(devconf->igmp.igmpVerDrp_v1)
			PROC_PRINTF("/DROP_IGMPV1");
		if(devconf->igmp.igmpVerDrp_v2)
			PROC_PRINTF("/DROP_IGMPV2");
		if(devconf->igmp.igmpVerDrp_v3)
			PROC_PRINTF("/DROP_IGMPV3");
		if(devconf->igmp.igmpReportDrop)
			PROC_PRINTF("/REPORT_DROP");
		if(devconf->igmp.igmpSnoopingDisable)
			PROC_PRINTF("/SNOOPING_DISABLE");
		if(devconf->igmp.igmpfastLeave)
			PROC_PRINTF("/FAST_LEAVE");
		if(devconf->igmp.igmpv3CareSource)
			PROC_PRINTF("/CARE_SOURCE");
		if(devconf->igmp.igmpMaxGroupSize)
			PROC_PRINTF("/MAX_GROUP_SIZE:%d",devconf->igmp.igmpMaxGroupSize);
		if(devconf->igmp.igmpMcMemberAgingInterval)
			PROC_PRINTF("/MC_MEMBER_AGING_INTERVAL:%d",devconf->igmp.igmpMcMemberAgingInterval);
		if(devconf->igmp.igmpMcLastMemberAgingInterval)
			PROC_PRINTF("/MC_LAST_MEMBER_AGING_INTERVAL:%d",devconf->igmp.igmpMcLastMemberAgingInterval);		
		if(devconf->igmp.ratelimit.rate)
			PROC_PRINTF("/RATE_LIMIT:%d",devconf->igmp.ratelimit.rate);
		if(devconf->igmp.igmpMaxWeight)
			PROC_PRINTF("/MAX_WEIGHT:%d",devconf->igmp.igmpMaxWeight);		
		if(devconf->igmp.igmpOverWeightDrop==0)
			PROC_PRINTF("/DROP_OVER_WEIGHT:%d",devconf->igmp.igmpOverWeightDrop);
		
		PROC_PRINTF("\n");
		PROC_PRINTF("\tIGMP6:\n");
		PROC_PRINTF("\t");
		if(devconf->igmp6.igmp6VerIgr_v1)
			PROC_PRINTF("/IGNORE_MLDV1");
		if(devconf->igmp6.igmp6VerIgr_v2)
			PROC_PRINTF("/IGNORE_MLDV2");
		if(devconf->igmp6.igmp6VerDrp_v1)
			PROC_PRINTF("/DROP_MLDV1");
		if(devconf->igmp6.igmp6VerDrp_v2)
			PROC_PRINTF("/DROP_MLDV2");
		if(devconf->igmp6.igmp6ReportDrop)
			PROC_PRINTF("/REPORT_DROP");
		if(devconf->igmp6.igmp6SnoopingDisable)
			PROC_PRINTF("/SNOOPING_DISABLE");
		if(devconf->igmp6.igmp6fastLeave)
			PROC_PRINTF("/FAST_LEAVE");
		if(devconf->igmp6.mldv2CareSource)
			PROC_PRINTF("/CARE_SOURCE");
		if(devconf->igmp6.igmp6MaxGroupSize)
			PROC_PRINTF("/MAX_GROUP_SIZE:%d",devconf->igmp6.igmp6MaxGroupSize);
		if(devconf->igmp6.igmp6McMemberAgingInterval)
			PROC_PRINTF("/MC_MEMBER_AGING_INTERVAL:%d",devconf->igmp6.igmp6McMemberAgingInterval);
		if(devconf->igmp6.igmp6McLastMemberAgingInterval)
			PROC_PRINTF("/MC_LAST_MEMBER_AGING_INTERVAL:%d",devconf->igmp6.igmp6McLastMemberAgingInterval);		
		if(devconf->igmp6.ratelimit.rate)
			PROC_PRINTF("/RATE_LIMIT:%d",devconf->igmp6.ratelimit.rate);
		if(devconf->igmp6.igmp6MaxWeight)
			PROC_PRINTF("/MAX_WEIGHT:%d",devconf->igmp6.igmp6MaxWeight);		
		if(devconf->igmp6.igmp6OverWeightDrop==0)
			PROC_PRINTF("/DROP_OVER_WEIGHT:%d",devconf->igmp6.igmp6OverWeightDrop);			
		
		PROC_PRINTF("\n");

		PROC_PRINTF("\tWhiteList: \n");

		list_for_each_entry(white_entry,&rtk_igmp_lanDevInfo[i].devConf.igmp.white,entry)
		{

			PROC_PRINTF("\t[%d]",white_entry->whiteListIdx);
			if(white_entry->patten.gipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("gip:%pI6 gipE:%pI6 ",white_entry->patten.wlGroupIpStart,white_entry->patten.wlGroupIpEnd);
				else
					PROC_PRINTF("gip:%pI4h gipE:%pI4h ",white_entry->patten.wlGroupIpStart,white_entry->patten.wlGroupIpEnd);
			}
			if(white_entry->patten.sipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("sip:%pI6 sipEnd:%pI6 ",white_entry->patten.wlSipStart,white_entry->patten.wlSipEnd);
				else
					PROC_PRINTF("sip:%pI4h sipEnd:%pI4h ",white_entry->patten.wlSipStart,white_entry->patten.wlSipEnd);
			}
			if(white_entry->patten.mcSipChk)
			{
				if(white_entry->patten.isIpv6)
					PROC_PRINTF("mcSip:%pI6 mcSipEnd:%pI6 ",white_entry->patten.wlMcSip,white_entry->patten.wlMcSipEnd);
				else
					PROC_PRINTF("mcSip:%pI4h mcSipEnd:%pI4h ",white_entry->patten.wlMcSip,white_entry->patten.wlMcSipEnd);
			}
			if(white_entry->patten.smacChk)
			{

				PROC_PRINTF("smac:%pM smacM:%pM ",white_entry->patten.smac,white_entry->patten.smacMask);
			}
			PROC_PRINTF("\n");
		}

	}

	return SUCCESS;
}


//dump ALL
int rtk_igmp_dump_allDevConfig(struct seq_file *s, void *v)
{
	return rtk_igmp_dump_devConfig(FAIL,s,v);
}


void _rtk_igmp_showValidToDevInfo(struct seq_file *s, void *v,rtk_igmp_flowToDevInfo_t *flowToDevInfo)
{
	int i;
	rtk_igmp_multicastDeviceInfo_t *pDevInfo;

	for (i=0 ;i< IGMP_MAX_DEV_NUM;i++)
	{
		pDevInfo = (_rtk_igmp_devIfidx_to_devInfo(flowToDevInfo[i].devIfIdx));
		if(pDevInfo == NULL)
			continue;

		if(flowToDevInfo[i].valid)
			PROC_PRINTF("  %s[%d]%s%s/",pDevInfo->devName,flowToDevInfo[i].devIfIdx,pDevInfo->devConf.igmp.igmpSnoopingDisable?"[disIGMP]":"",pDevInfo->devConf.igmp6.igmp6SnoopingDisable?"[disMLD]":"");
		else
			break;
	}
	return;
}

int rtk_igmp_allDump(struct seq_file *s, void *v)
{
	return rtk_igmp_devDump(FAIL,s,v);
}

int rtk_igmp_devDump(int ifidx,struct seq_file *s, void *v)
{
	int32 moduleIndex;
	int32 hashIndex,clientCnt;
	struct rtk_igmp_groupEntry *groupEntryPtr;
	struct rtk_igmp_clientEntry* clientEntry=NULL;
	struct rtk_igmp_sourceEntry *sourceEntryPtr;
	int32 flowCnt;
	struct rtk_igmp_mcastFlowEntry *mcastFlowEntry=NULL;

	PROC_PRINTF("IGMP Snooping:\n");
	PROC_PRINTF("=================================================\n");

	for( moduleIndex=0 ; moduleIndex< IGMP_MAX_BR_MODULE_NUM; moduleIndex++)
	{
		if(rtk_igmp_mCastModuleArray[moduleIndex].validBit==0)
			continue;

		PROC_PRINTF("BR-Dev:[%s]  ifidx=%d  Mac[%pM]\n",rtk_igmp_mCastModuleArray[moduleIndex].brDevInfo.devName,rtk_igmp_mCastModuleArray[moduleIndex].brDevInfo.ifindex,rtk_igmp_mCastModuleArray[moduleIndex].brDevInfo.rtk_igmp_gatewayMac);


		/* IPv4 */
		for (hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
		{
			groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[hashIndex];
			while (groupEntryPtr!=NULL)
			{

				PROC_PRINTF( "	 [%d] Group address:%pI4h vlanID[%d]\n",groupEntryPtr->groupIdx,groupEntryPtr->groupAddr,groupEntryPtr->vlanId);

				clientEntry=groupEntryPtr->clientList;

				clientCnt=0;
				while (clientEntry!=NULL)
				{

					clientCnt++;
					{
						if(_rtk_igmp_devIfidx_to_devInfo(clientEntry->inIfidx))
							PROC_PRINTF( "		  <%d>%pI4h\\ %s[%d]\\IGMPv%d\\",clientCnt, (clientEntry->clientAddr),_rtk_igmp_devIfidx_to_devInfo(clientEntry->inIfidx)->devName,clientEntry->inIfidx, (clientEntry->igmpVersion-IGMP_V1 + 1));
						else
							PROC_PRINTF( "		  <%d>%pI4h\\ none[%d]\\IGMPv%d\\",clientCnt, (clientEntry->clientAddr),clientEntry->inIfidx, (clientEntry->igmpVersion-IGMP_V1 + 1));
					}

					PROC_PRINTF( "%s",(clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds)?"EXCLUDE":"INCLUDE");
					if(clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds)
					{
						PROC_PRINTF( ":%ds",clientEntry->groupFilterTimer-rtk_igmp_sysUpSeconds);
					}
					else
					{
						PROC_PRINTF( ":0s");
					}
					PROC_PRINTF( ":%s ",clientEntry->pppoePassthroughEntry?"PPPoE Passthrough Entry":"");
					if(clientEntry->reportCtagif)
						PROC_PRINTF( "CTag:%d",clientEntry->reportVlanId);
					else
						PROC_PRINTF( "unTag");

					sourceEntryPtr=clientEntry->sourceList;
					if(sourceEntryPtr!=NULL)
					{
						PROC_PRINTF( "\\source list:");
					}

					while(sourceEntryPtr!=NULL)
					{
						PROC_PRINTF( "%pI4h:",sourceEntryPtr->sourceAddr);

						if(sourceEntryPtr->portTimer>rtk_igmp_sysUpSeconds)
						{
							PROC_PRINTF( "%ds",sourceEntryPtr->portTimer-rtk_igmp_sysUpSeconds);
						}
						else
						{
							PROC_PRINTF( "0s");
						}

						if(sourceEntryPtr->next!=NULL)
						{
							PROC_PRINTF( ", ");
						}

						sourceEntryPtr=sourceEntryPtr->next;
					}


					PROC_PRINTF( "\n");
					clientEntry = clientEntry->next;
				}

				PROC_PRINTF( "\n");
				groupEntryPtr=groupEntryPtr->next;
			}

		}

		/* IPv6 */
		for (hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
		{
			groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[hashIndex];
			while (groupEntryPtr!=NULL)
			{
				PROC_PRINTF( "	 [%d] Group address:%pI6 vlanID[%d]\n",groupEntryPtr->groupIdx,groupEntryPtr->groupAddr,groupEntryPtr->vlanId);

				clientEntry=groupEntryPtr->clientList;

				clientCnt=0;
				while (clientEntry!=NULL)
				{

					clientCnt++;
					{
						if(_rtk_igmp_devIfidx_to_devInfo(clientEntry->inIfidx))
							PROC_PRINTF( "		  <%d>%pI6\\ %s[%d]\\MLDv%d\\",clientCnt, (clientEntry->clientAddr),_rtk_igmp_devIfidx_to_devInfo(clientEntry->inIfidx)->devName,clientEntry->inIfidx, (clientEntry->igmpVersion-MLD_V1 + 1));
						else
							PROC_PRINTF( "		  <%d>%pI6\\ none[%d]\\MLDv%d\\",clientCnt, (clientEntry->clientAddr),clientEntry->inIfidx, (clientEntry->igmpVersion-MLD_V1 + 1));
					}

					PROC_PRINTF( "%s",(clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds)?"EXCLUDE":"INCLUDE");
					if(clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds)
					{
						PROC_PRINTF( ":%ds",clientEntry->groupFilterTimer-rtk_igmp_sysUpSeconds);
					}
					else
					{
						PROC_PRINTF( ":0s");
					}
					PROC_PRINTF( ":%s ",clientEntry->pppoePassthroughEntry?"PPPoE Passthrough Entry":"");
					if(clientEntry->reportCtagif)
						PROC_PRINTF( "CTag:%d",clientEntry->reportVlanId);
					else
						PROC_PRINTF( "unTag");

					sourceEntryPtr=clientEntry->sourceList;
					if(sourceEntryPtr!=NULL)
					{
						PROC_PRINTF( "\\source list:");
					}

					while(sourceEntryPtr!=NULL)
					{
						PROC_PRINTF( "%pI6:",sourceEntryPtr->sourceAddr);

						if(sourceEntryPtr->portTimer>rtk_igmp_sysUpSeconds)
						{
							PROC_PRINTF( "%ds",sourceEntryPtr->portTimer-rtk_igmp_sysUpSeconds);
						}
						else
						{
							PROC_PRINTF( "0s");
						}

						if(sourceEntryPtr->next!=NULL)
						{
							PROC_PRINTF( ", ");
						}

						sourceEntryPtr=sourceEntryPtr->next;
					}


					PROC_PRINTF( "\n");
					clientEntry = clientEntry->next;
				}

				PROC_PRINTF( "\n");
				groupEntryPtr=groupEntryPtr->next;
			}

		}




		PROC_PRINTF("=================================================\n");
		PROC_PRINTF("IPv4 Flow List:\n");
		flowCnt=1;
		for(hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
		{
			/*to dump multicast flow information*/
			mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[hashIndex];
			while(mcastFlowEntry!=NULL)
			{
				if(mcastFlowEntry->ipVersion==IP_VERSION4)
				{
					PROC_PRINTF( "	  [%d] %pI4h:(%d)-->%pI4h:(%d) \n \t\t l4proto:%d InCvid:%d swFwdCnt=%u defCnt=%u",
						flowCnt,&mcastFlowEntry->serverAddr[0],mcastFlowEntry->sport,&mcastFlowEntry->groupAddr[0],mcastFlowEntry->dport,mcastFlowEntry->l4proto,mcastFlowEntry->ingressCvlan,mcastFlowEntry->fwdPktCnt,mcastFlowEntry->defineFwdPktCnt);
					_rtk_igmp_showValidToDevInfo(s,v,mcastFlowEntry->multicastFwdInfo.egressDevInfo);
					PROC_PRINTF("\n");
				}
				flowCnt++;
				mcastFlowEntry=mcastFlowEntry->next;
			}
		}

		PROC_PRINTF( "IPv6 Flow List:\n");
		flowCnt=1;
		for(hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
		 {
			/*to dump multicast flow information*/
			mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[hashIndex];
			while(mcastFlowEntry!=NULL)
			{
				if(mcastFlowEntry->ipVersion==IP_VERSION6)
				{
					PROC_PRINTF( "	  [%d] %pI6:(%d)-->",flowCnt,&mcastFlowEntry->serverAddr[0],mcastFlowEntry->sport);
					PROC_PRINTF( "%pI6:(%d) \n \t\t l4proto:%d InCvid:%d swFwdCnt=%u defCnt=%u",&mcastFlowEntry->groupAddr[0],mcastFlowEntry->dport,mcastFlowEntry->l4proto,mcastFlowEntry->ingressCvlan,mcastFlowEntry->fwdPktCnt,mcastFlowEntry->defineFwdPktCnt);
					_rtk_igmp_showValidToDevInfo(s,v,mcastFlowEntry->multicastFwdInfo.egressDevInfo);
					PROC_PRINTF("\n");
				}
				flowCnt++;
				mcastFlowEntry=mcastFlowEntry->next;
			}
		}

	}
	PROC_PRINTF("************************************************\n\n");


	return SUCCESS;
}



static struct notifier_block rtk_igmp_netdev_notifier = {
	.notifier_call = rtk_igmp_netdev_event,
};


static int32 rtk_igmp_initHashTable(int32 moduleIndex, int32 hashTableSize)
{
	uint32 i=0;

	/* Allocate memory */
	rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable = (struct rtk_igmp_groupEntry **)rtk_igmp_malloc(sizeof(struct rtk_igmp_groupEntry *) * hashTableSize);
	if (rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable!= NULL)
	{
		for (i = 0 ; i < hashTableSize ; i++)
		{
			rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[i]=NULL;
		}
	}
	else
	{
		return FAIL;
	}

	rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable=  (struct rtk_igmp_groupEntry **)rtk_igmp_malloc(sizeof(struct rtk_igmp_groupEntry *) * hashTableSize);
	if (rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable!=NULL)
	{
		for (i = 0 ; i < hashTableSize ; i++)
		{
			rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[i]=NULL;
		}
	}
	else
	{
		if(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable!=NULL)
		{
			rtk_igmp_free(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable);
		}
		return FAIL;
	}

	rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable= (struct rtk_igmp_mcastFlowEntry **)rtk_igmp_malloc(sizeof(struct rtk_igmp_mcastFlowEntry *) * hashTableSize);
	if (rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable!=NULL)
	{
		for (i = 0 ; i < hashTableSize ; i++)
		{
			rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[i]=NULL;
		}
	}
	else
	{
		if(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable!=NULL)
		{
			rtk_igmp_free(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable);
		}

		if(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable!=NULL)
		{
			rtk_igmp_free(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable);
		}

		return FAIL;
	}

	return SUCCESS;
}



//size of name follow IFNAMSIZ
int rtk_igmp_devNameToBrDevIfidx(char *devName)
{
	int i;
	int ifidx=FAIL;

	for(i=0 ; i< IGMP_MAX_BR_MODULE_NUM ; i++)
	{
		if(!rtk_igmp_mCastModuleArray[i].validBit)
			continue;
		if(strcmp(rtk_igmp_mCastModuleArray[i].brDevInfo.devName,devName)==0)
		{
			ifidx =rtk_igmp_mCastModuleArray[i].brDevInfo.ifindex;
			break;
		}
	}

	return ifidx;
}


//size of name follow IFNAMSIZ
int rtk_igmp_devNameToDevIfidx(char *devName)
{
	int i;
	int ifidx=FAIL;


	for(i=0 ; i< IGMP_MAX_WAN_DEV_NUM ; i++)
	{
		if(!rtk_igmp_wanDevInfo[i].dev)
			continue;
		if(strcmp(rtk_igmp_wanDevInfo[i].devName,devName)==0)
		{
			ifidx =rtk_igmp_wanDevInfo[i].ifindex;
			break;
		}
	}

	if(ifidx==FAIL)
	{
		for(i=0 ; i<IGMP_MAX_LAN_DEV_NUM ;i++ )
		{
			if(!rtk_igmp_lanDevInfo[i].dev)
				continue;
			if(strcmp(rtk_igmp_lanDevInfo[i].devName,devName) ==0)
			{
				ifidx = rtk_igmp_lanDevInfo[i].ifindex;
				break;
			}
		}
	}

	return ifidx;
}


//return FAIL if not found
int rtk_igmp_devInfoToIfIdx(rt_igmpHook_devInfo_t devInfo)
{
	
	int32 devIdx=FAIL;
	rtk_igmp_multicastDeviceInfo_t *devcfg=NULL;
	rtk_igmp_multicastModule_t *brcfg=NULL;
	
	if(devIdx==FAIL)
	{
		devIdx = rtk_igmp_devNameToDevIfidx(devInfo.devIfname);
	}
	if(devIdx==FAIL)
	{
		devIdx = rtk_igmp_devNameToBrDevIfidx(devInfo.devIfname);
	}
	if(devIdx ==FAIL)
	{
		devIdx=devInfo.devIfidx;
	}

	devcfg =_rtk_igmp_devIfidx_to_devInfo(devIdx);
	brcfg = _rtk_igmp_devIfidx_to_BrDevInfo(devIdx);

	if(devIdx==FAIL || (devcfg==NULL && brcfg==NULL))
	{
		IGMP_CTRL("can't get devIdx=%d  devInfo.devIfname=[%s] devInfo.devIfidx=%d",devIdx,devInfo.devIfname,devInfo.devIfidx);
		if(!devcfg) IGMP_CTRL("devcfg NULL");
		if(!brcfg) IGMP_CTRL("brcfg NULL");
	}

	if(devcfg==NULL && brcfg==NULL)
		return FAIL;
	else
		return devIdx;
	
}

//size of name follow IFNAMSIZ
int rtk_igmp_wanIfNameBindToBrName(char *wanDevName , char *brDevName)
{
	int i;
	int wanEntIdx=FAIL,brEntIdx=FAIL;


	for(i=0 ; i< IGMP_MAX_WAN_DEV_NUM ; i++)
	{
		if(strcmp(rtk_igmp_wanDevInfo[i].devName,wanDevName)==0)
		{
			wanEntIdx = i;
			break;
		}
	}

	for(i=0 ; i<IGMP_MAX_BR_MODULE_NUM ;i++ )
	{
		if(rtk_igmp_mCastModuleArray[i].validBit &&strcmp(rtk_igmp_mCastModuleArray[i].brDevInfo.devName,brDevName) ==0)
		{
			brEntIdx = i;
			break;
		}
	}

	if(wanEntIdx ==FAIL || brEntIdx==FAIL ){IGMP_WARNING("wanEntIdx:%d ofr brEntIdx:%d illegall",wanEntIdx,brEntIdx); return FAIL;}

	rtk_igmp_wanDevInfo[wanEntIdx].bind2BrModuleIdx = brEntIdx;

	return SUCCESS;


}

int rtk_igmp_wanIdxBindToBrIdx(int32 wanifidx,int32 brIfidx)
{
	int i;
	int wanEntIdx=FAIL,brEntIdx=FAIL;

	if(wanifidx<0 || brIfidx<0){IGMP_WARNING("wanifidx:%d  or brIfidx:%d illegall",wanifidx,brIfidx); return FAIL;}

	for(i=0 ; i< IGMP_MAX_WAN_DEV_NUM ; i++)
	{
		if(rtk_igmp_wanDevInfo[i].ifindex==wanifidx)
		{
			wanEntIdx = i;
			break;
		}
	}

	for(i=0 ; i<IGMP_MAX_BR_MODULE_NUM ;i++ )
	{
		if(rtk_igmp_mCastModuleArray[i].validBit && rtk_igmp_mCastModuleArray[i].brDevInfo.ifindex ==brIfidx)
		{
			brEntIdx = i;
			break;
		}
	}

	if(wanEntIdx ==FAIL || brEntIdx==FAIL ){IGMP_WARNING("wanEntIdx:%d ofr brEntIdx:%d illegall",wanEntIdx,brEntIdx); return FAIL;}

	rtk_igmp_wanDevInfo[wanEntIdx].bind2BrModuleIdx = brEntIdx;

	return SUCCESS;

}


int _rtk_igmp_registerWanDev(struct net_device *wan_dev)
{

	int i;
	int entIdx=FAIL;
	for(i=0 ; i< IGMP_MAX_WAN_DEV_NUM ; i++)
	{
		if(rtk_igmp_wanDevInfo[i].dev==NULL)
		{
			entIdx = i;
			break;
		}

	}
	if(entIdx == FAIL){
		IGMP_WARNING("TABLE FULL");
		return RTK_IGMP_TABLE_FULL;
	}

	memset(&rtk_igmp_wanDevInfo[entIdx],0,sizeof(rtk_igmp_multicastDeviceInfo_t));
	rtk_igmp_wanDevInfo[entIdx].ifindex = wan_dev->ifindex;
	memcpy(rtk_igmp_wanDevInfo[entIdx].devName,wan_dev->name,sizeof(wan_dev->name));
	rtk_igmp_wanDevInfo[entIdx].dev = wan_dev;
	rtk_igmp_wanDevInfo[entIdx].bind2BrModuleIdx=FAIL;

	//init devconfig default valud
	rtk_igmp_wanDevInfo[entIdx].devConf.igmp.igmpMcMemberAgingInterval =IGMP_MEMBER_AGING;
	rtk_igmp_wanDevInfo[entIdx].devConf.igmp.igmpMcLastMemberAgingInterval =IGMP_LAST_MEMBER_AGING;	
	rtk_igmp_wanDevInfo[entIdx].devConf.igmp.igmpReportDrop=1;

	rtk_igmp_wanDevInfo[entIdx].devConf.igmp6.igmp6McMemberAgingInterval =IGMP6_MEMBER_AGING;
	rtk_igmp_wanDevInfo[entIdx].devConf.igmp6.igmp6McLastMemberAgingInterval =IGMP6_LAST_MEMBER_AGING;	
	rtk_igmp_wanDevInfo[entIdx].devConf.igmp6.igmp6ReportDrop=1;

	rtk_igmp_wanDevInfo[entIdx].devConf.igmp6.igmp6OverWeightDrop=1;
	rtk_igmp_wanDevInfo[entIdx].devConf.igmp.igmpOverWeightDrop=1;


	INIT_LIST_HEAD(&rtk_igmp_wanDevInfo[entIdx].devConf.igmp.white);
	INIT_LIST_HEAD(&rtk_igmp_wanDevInfo[entIdx].devConf.igmp.block);
	INIT_LIST_HEAD(&rtk_igmp_wanDevInfo[entIdx].devConf.igmp6.white);
	INIT_LIST_HEAD(&rtk_igmp_wanDevInfo[entIdx].devConf.igmp6.block);


	IGMP_CTRL("Register rtk_igmp_wanDevInfo[%d] %s on igmp",entIdx,rtk_igmp_wanDevInfo[entIdx].devName);


	return RTK_IGMP_ERR_OK;
}

int _rtk_igmp_unregisterWanDev(struct net_device *wan_dev)
{
	int entIdx=FAIL;
	int i;
	for(i=0 ; i< IGMP_MAX_WAN_DEV_NUM ; i++)
	{
		if(!rtk_igmp_wanDevInfo[i].dev)
			continue;
		if(rtk_igmp_wanDevInfo[i].dev==wan_dev && rtk_igmp_wanDevInfo[i].ifindex==wan_dev->ifindex)
		{
			entIdx = i;
			break;
		}

	}
	if(entIdx == FAIL){
		IGMP_WARNING("TABLE NOT FOUND");
		return RTK_IGMP_ENTRY_NOT_FOUND;
	}

	memset(&rtk_igmp_wanDevInfo[entIdx],0,sizeof(rtk_igmp_multicastDeviceInfo_t));
	return RTK_IGMP_ERR_OK;

}

int _rtk_igmp_registerLanDev(struct net_device *lan_dev)
{

	int i;
	int entIdx=FAIL;
	for(i=0 ; i< IGMP_MAX_LAN_DEV_NUM ; i++)
	{
		if(rtk_igmp_lanDevInfo[i].dev==NULL)
		{
			entIdx = i;
			break;
		}

	}
	if(entIdx == FAIL){
		IGMP_WARNING("TABLE FULL");
		return RTK_IGMP_TABLE_FULL;
	}

	memset(&rtk_igmp_lanDevInfo[entIdx],0,sizeof(rtk_igmp_multicastDeviceInfo_t));
	rtk_igmp_lanDevInfo[entIdx].ifindex = lan_dev->ifindex;
	memcpy(rtk_igmp_lanDevInfo[entIdx].devName,lan_dev->name,sizeof(lan_dev->name));
	rtk_igmp_lanDevInfo[entIdx].dev = lan_dev;
	rtk_igmp_lanDevInfo[entIdx].bind2BrModuleIdx=FAIL;

	//init devconfig default valud
	rtk_igmp_lanDevInfo[entIdx].devConf.igmp.igmpMcMemberAgingInterval =IGMP_MEMBER_AGING;
	rtk_igmp_lanDevInfo[entIdx].devConf.igmp6.igmp6McMemberAgingInterval =IGMP6_MEMBER_AGING;
	rtk_igmp_lanDevInfo[entIdx].devConf.igmp.igmpMcLastMemberAgingInterval =IGMP_LAST_MEMBER_AGING;
	rtk_igmp_lanDevInfo[entIdx].devConf.igmp6.igmp6McLastMemberAgingInterval =IGMP6_LAST_MEMBER_AGING;

	rtk_igmp_lanDevInfo[entIdx].devConf.igmp6.igmp6OverWeightDrop=1;
	rtk_igmp_lanDevInfo[entIdx].devConf.igmp.igmpOverWeightDrop=1;

	INIT_LIST_HEAD(&rtk_igmp_lanDevInfo[entIdx].devConf.igmp.white);
	INIT_LIST_HEAD(&rtk_igmp_lanDevInfo[entIdx].devConf.igmp.block);
	INIT_LIST_HEAD(&rtk_igmp_lanDevInfo[entIdx].devConf.igmp6.white);
	INIT_LIST_HEAD(&rtk_igmp_lanDevInfo[entIdx].devConf.igmp6.block);



	IGMP_CTRL("Register rtk_igmp_lanDevInfo[%d] %s on igmp",entIdx,rtk_igmp_lanDevInfo[entIdx].devName);


	return RTK_IGMP_ERR_OK;
}

int32 rtk_igmp_clearDataBaseByDev(int32 devIdx)
{
	struct rtk_igmp_groupEntry* groupEntryPtr=NULL;
	struct rtk_igmp_groupEntry* nextEntry=NULL;
	struct rtk_igmp_clientEntry *clientEntry=NULL;
	struct rtk_igmp_clientEntry *nextClientEntry=NULL;	
	int32 moduleIndex;
	int i;
	
	for(moduleIndex=0; moduleIndex<IGMP_MAX_BR_MODULE_NUM; moduleIndex++)
	{
		if(rtk_igmp_mCastModuleArray[moduleIndex].validBit==0)
			continue;

		/*maintain ipv4 group entry  timer */
		for(i=0; i<IGMP_HASH_TBL_SIZE; i++)
		{
			/*scan the hash table*/
			if(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable!=NULL)
			{
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[i];
				while(groupEntryPtr)              /*traverse each group list*/
				{
					nextEntry=groupEntryPtr->next;
					clientEntry = groupEntryPtr->clientList;
					while(clientEntry!=NULL)
					{
						nextClientEntry=clientEntry->next;
						if(devIdx == clientEntry->inIfidx)
							rtk_igmp_deleteClientEntry(groupEntryPtr,clientEntry);
						clientEntry=nextClientEntry;
					}
					groupEntryPtr=nextEntry;/*because expired group entry  will be cleared*/
				}
			}
		}

		/*maintain ipv6 group entry  timer */
		for(i=0; i<IGMP_HASH_TBL_SIZE; i++)
		{
			  /*scan the hash table*/
			if(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable!=NULL)
			{
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[i];
				while(groupEntryPtr)              /*traverse each group list*/
				{
					nextEntry=groupEntryPtr->next;
					clientEntry = groupEntryPtr->clientList;
					while(clientEntry!=NULL)
					{
						nextClientEntry=clientEntry->next;
						if(devIdx == clientEntry->inIfidx)
							rtk_igmp_deleteClientEntry(groupEntryPtr,clientEntry);
						clientEntry=nextClientEntry;
					}
					groupEntryPtr=nextEntry;/*because expired group entry  will be cleared*/
				}
			}
		}

	}

	return RTK_IGMP_ERR_OK;

}


int _rtk_igmp_unregisterLanDev(struct net_device *lan_dev)
{
	int entIdx=FAIL;
	int i;
	for(i=0 ; i< IGMP_MAX_LAN_DEV_NUM ; i++)
	{
		if(!rtk_igmp_lanDevInfo[i].dev)
			continue;
		if(rtk_igmp_lanDevInfo[i].dev==lan_dev && rtk_igmp_lanDevInfo[i].ifindex==lan_dev->ifindex)
		{
			entIdx = i;
			break;
		}

	}
	if(entIdx == FAIL){
		IGMP_WARNING("TABLE NOT FOUND");
		return RTK_IGMP_ENTRY_NOT_FOUND;
	}

	//clear data base
	rtk_igmp_clearDataBaseByDev(rtk_igmp_lanDevInfo[i].ifindex);
	rtk_igmp_flow_reflush_update(FAIL,NULL);

	memset(&rtk_igmp_lanDevInfo[entIdx],0,sizeof(rtk_igmp_multicastDeviceInfo_t));
	return RTK_IGMP_ERR_OK;

}




int _rtk_igmp_registerBrDev(struct net_device *br_dev)
{
	int i;
	int entIdx=FAIL;

	for(i=0 ; i<IGMP_MAX_BR_MODULE_NUM ;i++ )
	{
		if(rtk_igmp_mCastModuleArray[i].validBit==0)
		{
			entIdx = i;
			break;
		}
	}
	if(entIdx == FAIL){
		IGMP_WARNING("TABLE FULL");
		return RTK_IGMP_TABLE_FULL;
	}

	//regist moudle
	memset(&rtk_igmp_mCastModuleArray[entIdx],0,sizeof(struct rtk_igmp_multicastModule));
	/*initialize hash table*/
	ASSERT_EQ(rtk_igmp_initHashTable(entIdx, IGMP_HASH_TBL_SIZE),SUCCESS);

	rtk_igmp_mCastModuleArray[entIdx].validBit=1;
	rtk_igmp_mCastModuleArray[entIdx].brDevInfo.dev = br_dev;
	memcpy(rtk_igmp_mCastModuleArray[entIdx].brDevInfo.devName,br_dev->name,sizeof(br_dev->name));
	rtk_igmp_mCastModuleArray[entIdx].brDevInfo.ifindex = br_dev->ifindex;

	if(br_dev->dev_addr[0]|br_dev->dev_addr[1]|br_dev->dev_addr[2]|br_dev->dev_addr[3]|br_dev->dev_addr[4]|br_dev->dev_addr[5])
	{
		memcpy(rtk_igmp_mCastModuleArray[entIdx].brDevInfo.rtk_igmp_gatewayMac,br_dev->dev_addr,ETH_ALEN);
	}
	//default drop overWeight
	rtk_igmp_mCastModuleArray[entIdx].brDevInfo.brDevConf.sysIp6OverWeightDrop=1;
	rtk_igmp_mCastModuleArray[entIdx].brDevInfo.brDevConf.sysIp4OverWeightDrop=1;


	IGMP_CTRL("Register mCastModule[%d] %s on igmp",entIdx,rtk_igmp_mCastModuleArray[entIdx].brDevInfo.devName);

	return RTK_IGMP_ERR_OK;

}

int _rtk_igmp_unregisterBrDev(struct net_device *br_dev)
{
	int moduleIndex;
	int i;
	struct rtk_igmp_groupEntry *groupEntryPtr;
	struct rtk_igmp_mcastFlowEntry *mcastFlowEntryPtr;
	for(moduleIndex=0 ; moduleIndex<IGMP_MAX_BR_MODULE_NUM ;moduleIndex++ )
	{
		if(rtk_igmp_mCastModuleArray[moduleIndex].validBit && rtk_igmp_mCastModuleArray[moduleIndex].brDevInfo.ifindex==br_dev->ifindex)
		{
			rtk_igmp_mCastModuleArray[moduleIndex].validBit =0 ;
			/*delete ipv4 multicast entry*/
			for (i=0;i<IGMP_HASH_TBL_SIZE;i++)
			{
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[i];

				while(groupEntryPtr!=NULL)
				{
					rtk_igmp_deleteGroupEntry(moduleIndex,groupEntryPtr, rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable);
					groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[i];
				}
			}
			rtk_igmp_free(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable);
			rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable=NULL;

			/*delete ipv6 multicast entry*/
			for(i=0; i<IGMP_HASH_TBL_SIZE; i++)
			{

				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[i];
				while(groupEntryPtr!=NULL)
				{
					rtk_igmp_deleteGroupEntry(moduleIndex,groupEntryPtr, rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable);
					groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[i];
				}
			}
			rtk_igmp_free(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable);
			rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable=NULL;


			/*delete multicast flow entry*/
			for (i=0;i<IGMP_HASH_TBL_SIZE;i++)
			{
				mcastFlowEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[i];

				while(mcastFlowEntryPtr!=NULL)
				{
					rtk_igmp_deleteMcastFlowEntry(mcastFlowEntryPtr, rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);
					mcastFlowEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[i];
				}
			}
			rtk_igmp_free(rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);
			rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable=NULL;

			//invalid wan mapping info
			for(i=0 ; i< IGMP_MAX_WAN_DEV_NUM ; i++)
			{
				if(rtk_igmp_wanDevInfo[i].dev && rtk_igmp_wanDevInfo[i].bind2BrModuleIdx==moduleIndex)
				{
					rtk_igmp_wanDevInfo[i].bind2BrModuleIdx=FAIL;
				}
			}


			IGMP_CTRL("UnRegister mCastModule[%d] %s on igmp",i,br_dev->name);
			return RTK_IGMP_ERR_OK;
		}
	}
	return RTK_IGMP_ENTRY_NOT_FOUND;
}


static int rtk_igmp_netdev_event(struct notifier_block *this, unsigned long event, void *ptr)
{
	struct net_device *netDev = netdev_notifier_info_to_dev(ptr);

	//ARPHRD_ETHER
	IGMP_CTRL("[NETDEVICE EVENT] netdevice:%s  type: %d event:0x%x  priv_flags:0x%x\n", netDev->name, netDev->type, event, netDev->priv_flags);
	IGMP_CTRL("[NETDEVICE EVENT] ifinedx=%d	if_port=%d",netDev->ifindex,netDev->if_port);

	igmp_spin_lock_bh(igmpSysdb.lock_igmp);

	if(netDev->priv_flags & IFF_EBRIDGE )
	{
		//register BrDev
		if(event==NETDEV_UP)
			_rtk_igmp_registerBrDev(netDev);
		else if(event==NETDEV_DOWN)
			_rtk_igmp_unregisterBrDev(netDev);
	}
	else if (netDev->priv_flags & IFF_DOMAIN_WAN )
	{
		//register WanDev
		if(event==NETDEV_UP)
			_rtk_igmp_registerWanDev(netDev);
		else if (event==NETDEV_DOWN)
			_rtk_igmp_unregisterWanDev(netDev);

	}
	else if ((netDev->priv_flags & IFF_DOMAIN_ELAN)||(netDev->priv_flags & IFF_DOMAIN_WLAN))
	{
		//register LanDev
		if(event==NETDEV_UP)
			_rtk_igmp_registerLanDev(netDev);
		else if (event==NETDEV_DOWN)
			_rtk_igmp_unregisterLanDev(netDev);

	}
	igmp_spin_unlock_bh(igmpSysdb.lock_igmp);


	return RTK_IGMP_ERR_OK;

}

struct rtk_igmp_multicastModule* _rtk_igmp_devIfidx_to_BrDevInfo(int32 brIfindex)
{
	int i ;
	for(i=0 ; i <IGMP_MAX_BR_MODULE_NUM ;i++)
	{
		if(!rtk_igmp_mCastModuleArray[i].validBit)
			continue;
		if(rtk_igmp_mCastModuleArray[i].brDevInfo.ifindex==brIfindex)
			return &rtk_igmp_mCastModuleArray[i];
	}
	return NULL;
}


rtk_igmp_multicastDeviceInfo_t* _rtk_igmp_devIfidx_to_devInfo(int32 devIfindex)
{
	int i ;
	for(i=0 ; i <IGMP_MAX_LAN_DEV_NUM ;i++)
	{
		
		if(rtk_igmp_lanDevInfo[i].dev==NULL)
			continue;		
		if(rtk_igmp_lanDevInfo[i].ifindex == devIfindex)
			return &rtk_igmp_lanDevInfo[i];
	}
	if(i == IGMP_MAX_LAN_DEV_NUM)
	{
		for(i=0 ; i <IGMP_MAX_WAN_DEV_NUM ;i++)
		{
			if(rtk_igmp_wanDevInfo[i].dev==NULL)
				continue; 	
			if(rtk_igmp_wanDevInfo[i].ifindex == devIfindex)
				return &rtk_igmp_wanDevInfo[i];
		}

		if( i == IGMP_MAX_WAN_DEV_NUM)
			return NULL;
	}
	return NULL;
}



struct net_device * rtk_igmp_devIfidx_to_dev(int32 devIfindex)
{
	int i ;
	for(i=0 ; i <IGMP_MAX_LAN_DEV_NUM ;i++)
	{
		if(!rtk_igmp_lanDevInfo[i].dev)
			continue;
		if(rtk_igmp_lanDevInfo[i].ifindex == devIfindex)
			return rtk_igmp_lanDevInfo[i].dev;
	}

	for(i=0 ; i <IGMP_MAX_WAN_DEV_NUM ;i++)
	{
		if(!rtk_igmp_wanDevInfo[i].dev)
			continue;		
		if(rtk_igmp_wanDevInfo[i].ifindex == devIfindex)
			return rtk_igmp_wanDevInfo[i].dev;
	}
	
	for(i=0 ; i<IGMP_MAX_BR_MODULE_NUM ;i++ )
	{
		if((rtk_igmp_mCastModuleArray[i].validBit) && (devIfindex == rtk_igmp_mCastModuleArray[i].brDevInfo.ifindex))
		{
			return rtk_igmp_mCastModuleArray[i].brDevInfo.dev;
		}
	}

	
	return NULL;
}


//for br device mapping to ModuleIdx
int rtk_igmp_dstNetDevToModuleIdx(const struct net_device  *dstDev)
{
	int32 i;
	int32 moduleIndex=FAIL;

	for(i=0 ; i<IGMP_MAX_BR_MODULE_NUM ;i++ )
	{
		if((rtk_igmp_mCastModuleArray[i].validBit) && (dstDev->ifindex == rtk_igmp_mCastModuleArray[i].brDevInfo.ifindex))
		{
			IGMP_CTRL("Dst %s mapping to  moduleIndex:%d",dstDev->name,i);
			moduleIndex = i;
			break;
		}
	}

	return moduleIndex;
}


/*
	record to relative br device
*/
int rtk_igmp_srcNetDevToModuleIdx(const struct net_device  *srcDev,rtk_igmp_multicastDeviceInfo_t *devInfo)
{
	int32 i;
	struct net_bridge_port *p=NULL;
	struct net_bridge		*br=NULL;
	int32 moduleIndex=FAIL;

	if(srcDev==NULL)
		return moduleIndex;

	rcu_read_lock();
	p = rcu_dereference(srcDev->rx_handler_data);
	if(p)
	 	br = p->br;


	if(br)
	{
		for(i=0 ; i<IGMP_MAX_BR_MODULE_NUM ;i++ )
		{
			if((rtk_igmp_mCastModuleArray[i].validBit) && (br->dev->ifindex == rtk_igmp_mCastModuleArray[i].brDevInfo.ifindex))
			{
				IGMP_CTRL(" %s in %s get br search moduleIndex:%d",srcDev->name,br->dev->name,i);
				moduleIndex = i;
				break;
			}
		}
	}
	else
	{

		if(devInfo)
		{
			moduleIndex = devInfo->bind2BrModuleIdx;
		}
		else
		{
			devInfo = _rtk_igmp_devIfidx_to_devInfo(srcDev->ifindex);
			if(devInfo)
				moduleIndex = devInfo->bind2BrModuleIdx;
		}
		IGMP_CTRL("not in any br find in config table to do multicast routing %s ==> mouduleIdx:%d",srcDev->name,moduleIndex);

	}
	rcu_read_unlock();
	IGMP_CTRL("mapping moduleIndex=%d ",moduleIndex);

	return moduleIndex;

}


int32 rtk_igmp_hwCbReg(void)
{
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	hwCb.flowDelCbEvt 	= rtk_igmp_flowDelCbEvt;
	hwCb.flowSetCbEvt 	= rtk_igmp_flowSetCbEvt;
	hwCb.groupAddCbEvt	= rtk_igmp_groupAddCbEvt;
	hwCb.groupDelCbEvt	= rtk_igmp_groupDelCbEvt;
	hwCb.hwRegInit		= rtk_igmp_hwRegInit;
	hwCb.hwUnRegClear	= rtk_igmp_hwUnRegClear;
	hwCb.hwUnknownIpMcAction= rtk_igmp_unknownIpMc_Action;
	if(hwCb.hwRegInit)
		hwCb.hwRegInit();
#endif

	return SUCCESS;
}

int32 rtk_igmp_hwCbUnReg(void)
{

	if(hwCb.hwUnRegClear)
		hwCb.hwUnRegClear();
	hwCb.flowDelCbEvt 	= NULL;
	hwCb.flowSetCbEvt 	= NULL;
	hwCb.groupAddCbEvt	= NULL;
	hwCb.groupDelCbEvt	= NULL;
	hwCb.hwRegInit		= NULL;
	hwCb.hwUnRegClear	= NULL;
	hwCb.hwUnknownIpMcAction= NULL;
	return SUCCESS;
}


int rtk_igmp_init(void)
{

	memset(rtk_igmp_mCastModuleArray,0,sizeof(rtk_igmp_mCastModuleArray));
	rtk_igmp_groupEntryPool=rtk_igmp_initGroupEntryPool(IGMP_MAX_GROUP_COUNT);
	rtk_igmp_clientEntryPool=rtk_igmp_initClientEntryPool(IGMP_MAX_CLIENT_COUNT);
	rtk_igmp_mcastFlowEntryPool=rtk_igmp_initMcastFlowEntryPool(IGMP_MAX_FLOW_COUNT);
	rtk_igmp_sourceEntryPool=rtk_igmp_initSourceEntryPool(IGMP_MAX_SOURCE_COUNT);

	memset(&gloConf,0,sizeof(gloConf));
	spin_lock_init(&igmpSysdb.lock_igmp);


	gloConf.igmp_sys_timer_sec = IGMP_EXPIRE_TIMER;

	//init timer
	rtk_igmp_multicastSysTimerInit();

	//init whilte list
	rtk_igmp_whiteEntry_flashInit();

	//init user define reserved group to protocol-stack table
	rtk_igmp_groupToPsTbl_flashInit();

	//init group Weight tbl
	rtk_igmp_groupWeight_flashInit();

	rtk_igmp_hwCbReg();

	//last register netdevice
	register_netdevice_notifier(&rtk_igmp_netdev_notifier);

	return 0;
}

int rtk_igmp_exit(void)
{
	unregister_netdevice_notifier(&rtk_igmp_netdev_notifier);
	igmp_spin_lock_bh(igmpSysdb.lock_igmp);
	rtk_igmp_multicastSysTimerDestroy();
	rtk_igmp_hwCbUnReg();
	igmp_spin_unlock_bh(igmpSysdb.lock_igmp);
	return SUCCESS;
}



static inline uint32 rtk_igmp_hashAlgorithm(uint32 ipVersion,uint32 *groupAddr)
{
	uint32 hashIndex=0;

	if (ipVersion==IP_VERSION4)
	{
		/*to do:change hash algorithm*/
		hashIndex= (IGMP_HASH_TBL_SIZE-1)&groupAddr[0];
	}
	else
	{
		hashIndex=(IGMP_HASH_TBL_SIZE-1)&(groupAddr[3]^groupAddr[2]^groupAddr[1]^groupAddr[0]);
	}

	return hashIndex;
}

#define TIMER

uint32 rtk_igmp_getTimeSec(void)
{
	int32 jiffiesDiff;
	if(jiffies > lastjiffies)
		jiffiesDiff =jiffies-lastjiffies;
	else
		jiffiesDiff =(0xffffffff-(lastjiffies-jiffies));
	lastjiffies=jiffies;
	lastTimer +=(jiffiesDiff/CONFIG_HZ);
	return lastTimer;
}

static int32 rtk_igmp_checkSourceTimer(struct rtk_igmp_clientEntry * clientEntry , struct rtk_igmp_sourceEntry * sourceEntry)
{
	uint8 deleteFlag=FALSE; //delte group? client? source?
	uint8 oldFwdState,newFwdState;

	//IGMP("Check GIP.Clt.Src Timer");
	oldFwdState=sourceEntry->fwdState;

	if(sourceEntry->portTimer<=rtk_igmp_sysUpSeconds) /*means time out*/
	{
		if (clientEntry->groupFilterTimer<=rtk_igmp_sysUpSeconds) /* include mode*/
		{
			deleteFlag=TRUE;
		}
		sourceEntry->fwdState=FALSE;
	}
	else
	{
		//deleteFlag=FALSE; // redundant code
		sourceEntry->fwdState=TRUE;
	}

	newFwdState=sourceEntry->fwdState;

	//IGMP("GIP.Clt.SrcIP fwdState by {%s}"
	//, (deleteFlag==TRUE)?"<del SrcIP>":((newFwdState!=oldFwdState)&&(newFwdState==TRUE))?"<SrcIP stop fwd>":"SrcIP go on fwd");

	if (deleteFlag==TRUE) /*means INCLUDE mode and expired*/
	{
		rtk_igmp_deleteSourceEntry(clientEntry,sourceEntry);
	}

	if ((deleteFlag==TRUE) || (newFwdState!=oldFwdState))
	{
		IGMP_CTRL("sourceTimer change");
		return TRUE;
	}

	return FALSE;
}


static void rtk_igmp_checkClientEntryTimer(int32 moduleIndex,struct rtk_igmp_groupEntry * groupEntry, struct rtk_igmp_clientEntry * clientEntry)
{

	uint32 oldFwdIfidx=0;
	uint32 newFwdIfidx=0;
	struct rtk_igmp_sourceEntry *sourceEntry=clientEntry->sourceList;
	struct rtk_igmp_sourceEntry *nextSourceEntry=NULL;
	uint32 clientStatusChange=0,sourceStatusChange=0;

	oldFwdIfidx=rtk_igmp_getClientFwdIfIdx(clientEntry, rtk_igmp_sysUpSeconds);
	while (sourceEntry!=NULL)
	{
		nextSourceEntry=sourceEntry->next;
		if(rtk_igmp_checkSourceTimer(clientEntry, sourceEntry))
			sourceStatusChange=1; //if any source change ,we will change to hardware
		sourceEntry=nextSourceEntry;
	}

	newFwdIfidx=rtk_igmp_getClientFwdIfIdx(clientEntry, rtk_igmp_sysUpSeconds);
	//IGMP_CTRL("oldFwdIfidx=%d newFwdIfidx=%d",oldFwdIfidx,newFwdIfidx);
	if (newFwdIfidx==FAIL) /*none active port*/
	{
		rtk_igmp_deleteClientEntry(groupEntry,clientEntry);
	}

	//Client status exclude mode -> include mode
	if( (clientEntry->groupFilterTimer<=rtk_igmp_sysUpSeconds) &&  ((rtk_igmp_sysUpSeconds - clientEntry->groupFilterTimer) <=  gloConf.igmp_sys_timer_sec))
		clientStatusChange=1;

	if ((oldFwdIfidx!=newFwdIfidx) || (newFwdIfidx==FAIL) || clientStatusChange || sourceStatusChange)
	{
		IGMP_CTRL("clientStatusChange=%d sourceStatusChange=%d oldFwdIfidx=%d newFwdIfidx=%d ",clientStatusChange,sourceStatusChange,oldFwdIfidx,newFwdIfidx);
		rtk_igmp_flow_reflush_update(moduleIndex,groupEntry->groupAddr);
	}

}


static void rtk_igmp_checkGroupEntryTimer(int32 moduleIndex ,struct rtk_igmp_groupEntry * groupEntry, struct rtk_igmp_groupEntry ** hashTable)
{
	struct rtk_igmp_clientEntry *clientEntry=groupEntry->clientList;
	struct rtk_igmp_clientEntry *nextClientEntry=NULL;
	//IGMP_CTRL("In rtk_igmp_checkGroupEntryTimer");
	while(clientEntry!=NULL)
	{
		nextClientEntry=clientEntry->next;
		rtk_igmp_checkClientEntryTimer(moduleIndex,groupEntry, clientEntry);
		clientEntry=nextClientEntry;
	}

	if(groupEntry->clientList==NULL) /*none active client*/
	{
		rtk_igmp_deleteGroupEntry(moduleIndex,groupEntry,hashTable);
		//IGMP_CTRL("Check GIP.Clientt Timer event");
	}

}

int32 rtk_igmp_maintainMulticastSnoopingTimerList(uint32 currentSystemTime)
{
	/* maintain current time */
	uint32 i=0;
	uint32 maxTime=0xffffffff;

	struct rtk_igmp_groupEntry* groupEntryPtr=NULL;
	struct rtk_igmp_groupEntry* nextEntry=NULL;
	int32 moduleIndex;

	//IGMP_CTRL("Timer Expire");

	/*handle timer conter overflow*/
	if(currentSystemTime>rtk_igmp_startTime)
	{
		rtk_igmp_sysUpSeconds=currentSystemTime-rtk_igmp_startTime;
	}
	else
	{
		rtk_igmp_sysUpSeconds=(maxTime-rtk_igmp_startTime)+currentSystemTime+1;
	}

	for(moduleIndex=0; moduleIndex<IGMP_MAX_BR_MODULE_NUM; moduleIndex++)
	{
		if(!rtk_igmp_mCastModuleArray[moduleIndex].validBit)
			continue;

		/*maintain ipv4 group entry  timer */
		for(i=0; i<IGMP_HASH_TBL_SIZE; i++)
		{

			/*scan the hash table*/
			if(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable!=NULL)
			{
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[i];
				while(groupEntryPtr)              /*traverse each group list*/
				{
					nextEntry=groupEntryPtr->next;
					rtk_igmp_checkGroupEntryTimer(moduleIndex,groupEntryPtr, rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable);
					groupEntryPtr=nextEntry;/*because expired group entry  will be cleared*/

				}
			}
		}

		/*maintain ipv6 group entry  timer */
		for(i=0; i<IGMP_HASH_TBL_SIZE; i++)
		{
			  /*scan the hash table*/
			if(rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable!=NULL)
			{
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[i];
				while(groupEntryPtr)              /*traverse each group list*/
				{
					nextEntry=groupEntryPtr->next;
					rtk_igmp_checkGroupEntryTimer(moduleIndex,groupEntryPtr, rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable);
					groupEntryPtr=nextEntry;/*because expired group entry  will be cleared*/

				}
			}
		}

		rtk_igmp_agingMcastFlow(moduleIndex);
		if(igmpSysdb.unknownFlood)
			rtk_igmp_deleteUnknownFloodMacstFlow(moduleIndex);
		

	}
	return SUCCESS;
}



void rtk_igmp_multicastSysTimerExpired(uint32 expireDada)
{
	igmp_spin_lock_bh(igmpSysdb.lock_igmp);
	rtk_igmp_maintainMulticastSnoopingTimerList((uint32)rtk_igmp_getTimeSec());
	mod_timer(&igmpSysTimer,jiffies+gloConf.igmp_sys_timer_sec*CONFIG_HZ);
	igmp_spin_unlock_bh(igmpSysdb.lock_igmp);

}

void rtk_igmp_multicastSysTimerInit(void)
{
	rtk_igmp_startTime=0;
	rtk_igmp_sysUpSeconds=0;
	lastjiffies=jiffies;
	init_timer(&igmpSysTimer);
	igmpSysTimer.data=SYS_EXPIRED_NORMAL;
	igmpSysTimer.expires=jiffies+gloConf.igmp_sys_timer_sec*CONFIG_HZ;
	igmpSysTimer.function=(void*)rtk_igmp_multicastSysTimerExpired;
	mod_timer(&igmpSysTimer,jiffies+gloConf.igmp_sys_timer_sec*CONFIG_HZ);
}

static void rtk_igmp_multicastSysTimerDestroy(void)
{
	if(timer_pending(&igmpSysTimer))
		del_timer(&igmpSysTimer);
}






#define GROUP_OPERATION
// allocate a group entry from the group entry pool

/*********************************************
			Group list operation
 *********************************************/

/*       find a group address in a group list    */

struct rtk_igmp_groupEntry* rtk_igmp_searchGroupEntry(int32 moduleIndex,uint32 ipVersion,uint32 *multicastAddr, uint16 vlanId)
{
	struct rtk_igmp_groupEntry* groupPtr = NULL;
	int32 hashIndex=0;

	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, multicastAddr);

	if (ipVersion==IP_VERSION4)
	{
		groupPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[hashIndex];
	}
	else
	{
		groupPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[hashIndex];
	}

	while (groupPtr!=NULL)
	{
		if (ipVersion==IP_VERSION4)
		{

			if (multicastAddr[0]==groupPtr->groupAddr[0])
			{

				return groupPtr;

			}
		}
		else
		{
			if(	(multicastAddr[0]==groupPtr->groupAddr[0])&&
				(multicastAddr[1]==groupPtr->groupAddr[1])&&
				(multicastAddr[2]==groupPtr->groupAddr[2])&&
				(multicastAddr[3]==groupPtr->groupAddr[3])
			)
			{

				return groupPtr;
			}
		}

		groupPtr = groupPtr->next;

	}

	return NULL;
}

/*group entry memory management*/
static  struct rtk_igmp_groupEntry* rtk_igmp_initGroupEntryPool(uint32 poolSize)
{
	uint32 idx=0;
	struct rtk_igmp_groupEntry *poolHead=NULL;
	struct rtk_igmp_groupEntry *entryPtr=NULL;

	if (poolSize != IGMP_MAX_GROUP_COUNT)
	{
		goto out;
	}

	/* Allocate memory */
	poolHead = &groupEntryPool[0];

	if (poolHead != NULL)
	{
		memset(poolHead, 0,  (IGMP_MAX_GROUP_COUNT  * sizeof(struct rtk_igmp_groupEntry)));
		entryPtr = poolHead;

		/* link the whole group entry pool */
		for (idx = 0 ; idx < IGMP_MAX_GROUP_COUNT ; idx++, entryPtr++)
		{
			if(idx==0)
			{
				entryPtr->previous=NULL;
				if(idx == (IGMP_MAX_GROUP_COUNT - 1)) //poolSize is 1
				{
					entryPtr->next=NULL;
				}
				else
				{
					entryPtr->next = entryPtr + 1;
				}
			}
			else
			{
				entryPtr->previous=entryPtr-1;
				if (idx == (IGMP_MAX_GROUP_COUNT - 1))
				{
					entryPtr->next = NULL;
				}
				else
				{
					entryPtr->next = entryPtr + 1;
				}
			}
		}
	}

out:

	return poolHead;
}


// free a group entry and link it back to the group entry pool, default is link to the pool head
static  void rtk_igmp_freeGroupEntry(struct rtk_igmp_groupEntry* groupEntryPtr)
{
	if (!groupEntryPtr)
	{
		return;
	}

		groupEntryPtr->next = rtk_igmp_groupEntryPool;
		if(rtk_igmp_groupEntryPool!=NULL)
		{
			rtk_igmp_groupEntryPool->previous=groupEntryPtr;
		}
		rtk_igmp_groupEntryPool=groupEntryPtr;
}

/* clear the content of group entry */
static void rtk_igmp_clearGroupEntry(struct rtk_igmp_groupEntry* groupEntry)
{
	if (NULL!=groupEntry)
	{
		memset(groupEntry, 0, sizeof(struct rtk_igmp_groupEntry));
	}
}

/* unlink a group entry from group list */
static void rtk_igmp_unlinkGroupEntry(struct rtk_igmp_groupEntry* groupEntry,  struct rtk_igmp_groupEntry ** hashTable)
{
	uint32 hashIndex=0;

	if(NULL==groupEntry)
	{
		return;
	}

	/* not sync to hw at here ,cause we need check if group in other modules
	if(hwCb.groupDelCbEvt)
	{
		rtk_igmp_groupCbEvt_t groupCb;
		memset(&groupCb,0,sizeof(groupCb));
		groupCb.isIPv6= groupEntry->ipVersion==IP_VERSION6?1:0;
		groupCb.vlan= groupEntry->vlanId ;
		memcpy(groupCb.group ,groupEntry->groupAddr,sizeof(groupCb.group));
		IGMP_CTRL("GROUP DEL CALL BACK2 HW");
		hwCb.groupDelCbEvt(&groupCb);
	}*/

	hashIndex=rtk_igmp_hashAlgorithm(groupEntry->ipVersion, groupEntry->groupAddr);
	/* unlink entry node*/
	if(groupEntry==hashTable[hashIndex]) /*unlink group list head*/
	{
		hashTable[hashIndex]=groupEntry->next;
		if(hashTable[hashIndex]!=NULL)
		{
			hashTable[hashIndex]->previous=NULL;
		}
	}
	else
	{
		if(groupEntry->previous!=NULL)
		{
			groupEntry->previous->next=groupEntry->next;
		}

		if(groupEntry->next!=NULL)
		{
			groupEntry->next->previous=groupEntry->previous;
		}
	}

	groupEntry->previous=NULL;
	groupEntry->next=NULL;


}

struct rtk_igmp_groupEntry* rtk_igmp_searchGroupEntryAllModule(uint32 ipVersion,uint32 *multicastAddr)
{
	struct rtk_igmp_groupEntry* groupPtr = NULL;
	int32 hashIndex=0;
	int32 moduleIndex = 0;

	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, multicastAddr);

	for(moduleIndex=0 ; moduleIndex<IGMP_MAX_BR_MODULE_NUM ;moduleIndex++ )
	{
		if(rtk_igmp_mCastModuleArray[moduleIndex].validBit==0)
			continue;

		if (ipVersion==IP_VERSION4)
		{
			groupPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[hashIndex];
		}
		else
		{
			groupPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[hashIndex];
		}

		while (groupPtr!=NULL)
		{
			if (ipVersion==IP_VERSION4)
			{

				if (multicastAddr[0]==groupPtr->groupAddr[0])
				{
					return groupPtr;
				}	
			}
			else
			{
				if((multicastAddr[0]==groupPtr->groupAddr[0])&&
					(multicastAddr[1]==groupPtr->groupAddr[1])&&
					(multicastAddr[2]==groupPtr->groupAddr[2])&&
					(multicastAddr[3]==groupPtr->groupAddr[3]))
				{
					return groupPtr;
				}
			}

			groupPtr = groupPtr->next;

		}	

	}
	return NULL;
}



static void rtk_igmp_deleteGroupEntry( int32 moduleIndex, struct rtk_igmp_groupEntry* groupEntry,struct rtk_igmp_groupEntry ** hashTable)
{

	uint32 multicastAddr[4];	
	uint32 ipVersion;	
	uint16 vlanId ;

	memset(multicastAddr, 0, sizeof(multicastAddr));

	if (groupEntry!=NULL)
	{
	
		ipVersion=groupEntry->ipVersion;
		vlanId = groupEntry->vlanId ;
		memcpy(multicastAddr,groupEntry->groupAddr,sizeof(multicastAddr));
		
		rtk_igmp_deleteClientList(groupEntry);
		rtk_igmp_invalidateMCastGroupFlow(moduleIndex, groupEntry->ipVersion, groupEntry->groupAddr);
		rtk_igmp_unlinkGroupEntry(groupEntry, hashTable);
		rtk_igmp_clearGroupEntry(groupEntry);
		rtk_igmp_freeGroupEntry(groupEntry);

		//check if group still in all modules 
		if(rtk_igmp_searchGroupEntryAllModule(ipVersion,multicastAddr)== NULL)
		{
			if(hwCb.groupDelCbEvt)
			{
				rtk_igmp_groupCbEvt_t groupCb;
				memset(&groupCb,0,sizeof(groupCb));
				groupCb.isIPv6 = ipVersion==IP_VERSION6?1:0;
				groupCb.vlan = vlanId ;
				memcpy(groupCb.group ,multicastAddr,sizeof(groupCb.group));
				IGMP_CTRL("GROUP DEL CALL BACK2 HW");
				hwCb.groupDelCbEvt(&groupCb);
			}

		}
		else
		{
			//groupRefCnt--;
			if(hwCb.groupDelCbEvt)
			{
				rtk_igmp_groupCbEvt_t groupCb;
				memset(&groupCb,0,sizeof(groupCb));
				groupCb.isIPv6 = ipVersion==IP_VERSION6?1:0;
				groupCb.vlan = vlanId ;
				memcpy(groupCb.group ,multicastAddr,sizeof(groupCb.group));
				groupCb.minusGroupRefCntOnly = 1;
				IGMP_CTRL("GROUP RefCnt minus 1 CALL BACK2 HW");
				hwCb.groupDelCbEvt(&groupCb);
			}
		}
	}

}


static  struct rtk_igmp_groupEntry* rtk_igmp_allocateGroupEntry(void)
{
	struct rtk_igmp_groupEntry *ret = NULL;

	if (rtk_igmp_groupEntryPool!=NULL)
	{
		ret = rtk_igmp_groupEntryPool;
		if(rtk_igmp_groupEntryPool->next!=NULL)
		{
			rtk_igmp_groupEntryPool->next->previous=NULL;
		}
		rtk_igmp_groupEntryPool = rtk_igmp_groupEntryPool->next;
		memset(ret, 0, sizeof(struct rtk_igmp_groupEntry));
	}

	return ret;
}


/* link group Entry in the front of a group list */
static void  rtk_igmp_linkGroupEntry(struct rtk_igmp_groupEntry* groupEntry ,  struct rtk_igmp_groupEntry ** hashTable)
{
	uint32 hashIndex=0;

	if(NULL==groupEntry)
	{
		return;
	}
	atomic_inc(&groupIncIdx);
	groupEntry->groupIdx=atomic_read(&groupIncIdx);

	hashIndex=rtk_igmp_hashAlgorithm(groupEntry->ipVersion, groupEntry->groupAddr);
	if(hashTable[hashIndex]!=NULL)
	{
		hashTable[hashIndex]->previous=groupEntry;
	}
	groupEntry->next = hashTable[hashIndex];
	hashTable[hashIndex]=groupEntry;
	hashTable[hashIndex]->previous=NULL;

	if(hwCb.groupAddCbEvt)
	{
		rtk_igmp_groupCbEvt_t groupCb;
		memset(&groupCb,0,sizeof(groupCb));
		groupCb.isIPv6= groupEntry->ipVersion==IP_VERSION6?1:0;
		groupCb.vlan= groupEntry->vlanId ;
		memcpy(groupCb.group ,groupEntry->groupAddr,sizeof(groupCb.group));
		IGMP_CTRL("GROUP ADD CALL BACK2 HW");
		hwCb.groupAddCbEvt(&groupCb);
	}

}


#define CLIENT_OPERATION
/*********************************************
			Client list operation
 *********************************************/
static struct rtk_igmp_clientEntry* rtk_igmp_searchClientEntry(uint32 ipVersion, struct rtk_igmp_groupEntry* groupEntry, uint32 inIfidx, uint32 *clientAddr)
{
	struct rtk_igmp_clientEntry* clientPtr = groupEntry->clientList;

	if(clientAddr==NULL)
	{
		return NULL;
	}
	while (clientPtr!=NULL)
	{
		if(ipVersion==IP_VERSION4)
		{
			if((clientPtr->clientAddr[0]==clientAddr[0]))
			{
				if( (clientPtr->inIfidx!= inIfidx))
				{
					/*update port number,in case of client change port*/
					IGMP_CTRL("Update Client Port form ifidx %d to %d",clientPtr->inIfidx ,inIfidx);
					clientPtr->inIfidx=inIfidx;
				}
				return clientPtr;
			}

		}
		else
		{
			if(	((clientPtr->clientAddr[0]==clientAddr[0])
				&&(clientPtr->clientAddr[1]==clientAddr[1])
				&&(clientPtr->clientAddr[2]==clientAddr[2])
				&&(clientPtr->clientAddr[3]==clientAddr[3])))
			{

				if((clientPtr->inIfidx != inIfidx))
				{
					/*update port number,in case of client change port*/
					IGMP_CTRL("Update Client Port form ifidx %d to %d",clientPtr->inIfidx ,inIfidx);
					clientPtr->inIfidx=inIfidx;
				}
				return clientPtr;
			}
		}

		clientPtr = clientPtr->next;

	}

	return NULL;
}


static int32 rtk_igmp_getClientFwdIfIdx(struct rtk_igmp_clientEntry * clientEntry, uint32 sysTime)
{
	int32 fwdIfidx=FAIL;
	struct rtk_igmp_sourceEntry * sourcePtr=NULL;;

	 /*exclude mode or dummyClient never expired*/
	if (clientEntry->groupFilterTimer>sysTime )
	{
		fwdIfidx = clientEntry->inIfidx;
	}
	else/*include mode*/
	{
		sourcePtr=clientEntry->sourceList;
		while (sourcePtr!=NULL)
		{
			if (sourcePtr->portTimer>sysTime)
			{
				fwdIfidx = clientEntry->inIfidx;
				break;
			}
			sourcePtr=sourcePtr->next;
		}
	}
	return fwdIfidx;
}


/*client entry memory management*/
static  struct rtk_igmp_clientEntry* rtk_igmp_initClientEntryPool(uint32 poolSize)
{

	uint32 idx=0;
	struct rtk_igmp_clientEntry *poolHead=NULL;
	struct rtk_igmp_clientEntry *entryPtr=NULL;

	if (poolSize != IGMP_MAX_CLIENT_COUNT)
	{
		goto out;
	}

	/* Allocate memory */
	poolHead = &clientEntryPool[0];

	if (poolHead != NULL)
	{
		memset(poolHead, 0,  (IGMP_MAX_CLIENT_COUNT  * sizeof(struct rtk_igmp_clientEntry)));
		entryPtr = poolHead;

		/* link the whole group entry pool */
		for (idx = 0 ; idx < IGMP_MAX_CLIENT_COUNT ; idx++, entryPtr++)
		{
			if(idx==0)
			{
				entryPtr->previous=NULL;
				if(idx == (IGMP_MAX_CLIENT_COUNT - 1))
				{
					entryPtr->next=NULL;
				}
				else
				{
					entryPtr->next = entryPtr + 1;
				}
			}
			else
			{
				entryPtr->previous=entryPtr-1;
				if (idx == (IGMP_MAX_CLIENT_COUNT - 1))
				{
					entryPtr->next = NULL;
				}
				else
				{
					entryPtr->next = entryPtr + 1;
				}
			}
		}
	}

out:


	return poolHead;

}


/* unlink a client entry from group client list */
static void rtk_igmp_unlinkClientEntry(struct rtk_igmp_groupEntry *groupEntry, struct rtk_igmp_clientEntry* clientEntry)
{
	//IGMP_CTRL("unlink %pI4",&clientEntry->clientAddr);
	if(NULL==clientEntry)
	{
		return;
	}

	if(NULL==groupEntry)
	{
		return;
	}


	/* unlink entry node*/
	if(clientEntry==groupEntry->clientList) /*unlink group list head*/
	{
		groupEntry->clientList=groupEntry->clientList->next;
		if(groupEntry->clientList!=NULL)
		{
			groupEntry->clientList->previous=NULL;
		}

	}
	else
	{
		if(clientEntry->previous!=NULL)
		{
			clientEntry->previous->next=clientEntry->next;
		}

		if(clientEntry->next!=NULL)
		{
			clientEntry->next->previous=clientEntry->previous;
		}
	}

	clientEntry->previous=NULL;
	clientEntry->next=NULL;

#if 0
	{
		switch(clientEntry->igmpVersion)
		{
			case IGMP_V1:	////++
				igmpSnoopingCounterVer_1--;
				break;
			case IGMP_V2:
				igmpSnoopingCounterVer_2--;
				break;
			case IGMP_V3:
				igmpSnoopingCounterVer_3--;
				break;
			case MLD_V1:
				MLDCounterVer_1--;
				break;
			case MLD_V2:
				MLDCounterVer_2--;
				break;
			default:
				break;
		}

		{
			IGMP_WARNING("FIXME");
			//igmpSnoopingCounterVer_Port[clientEntry->portNum][clientEntry->igmpVersion - IGMP_V1]--;
		}

	}
#endif


}

// free a client entry and link it back to the client entry pool, default is link to the pool head
static  void rtk_igmp_freeClientEntry(struct rtk_igmp_clientEntry* clientEntryPtr)
{
	if (!clientEntryPtr)
	{
		return;
	}
	clientEntryPtr->next = rtk_igmp_clientEntryPool;
	if(rtk_igmp_clientEntryPool!=NULL)
	{
		rtk_igmp_clientEntryPool->previous=clientEntryPtr;
	}
	rtk_igmp_clientEntryPool=clientEntryPtr;
}


/* clear the content of client entry */
static void rtk_igmp_clearClientEntry(struct rtk_igmp_clientEntry* clientEntry)
{

	if (NULL!=clientEntry)
	{
		memset(clientEntry, 0, sizeof(struct rtk_igmp_clientEntry));
	}
}

static void rtk_igmp_deleteClientEntry(struct rtk_igmp_groupEntry* groupEntry,struct rtk_igmp_clientEntry *clientEntry)
{

	if (NULL==clientEntry)
	{
		return;
	}

	if (NULL==groupEntry)
	{
		return;
	}
	rtk_igmp_deleteSourceList(clientEntry);
	rtk_igmp_unlinkClientEntry(groupEntry,clientEntry);
	rtk_igmp_clearClientEntry(clientEntry);
	rtk_igmp_freeClientEntry(clientEntry);
	return;

}

static void rtk_igmp_deleteClientList(struct rtk_igmp_groupEntry* groupEntry)
{

	struct rtk_igmp_clientEntry *clientEntry=NULL;
	struct rtk_igmp_clientEntry *nextClientEntry=NULL;

	if (NULL==groupEntry)
	{
		return;
	}

	clientEntry=groupEntry->clientList;
	while (clientEntry!=NULL)
	{
		nextClientEntry=clientEntry->next;
		rtk_igmp_deleteClientEntry(groupEntry,clientEntry);
		clientEntry=nextClientEntry;
	}
}


// allocate a client entry from the client entry pool
static  struct rtk_igmp_clientEntry* rtk_igmp_allocateClientEntry(void)
{
	struct rtk_igmp_clientEntry *ret = NULL;

	if (rtk_igmp_clientEntryPool!=NULL)
	{
		ret = rtk_igmp_clientEntryPool;

		if(rtk_igmp_clientEntryPool->next!=NULL)
		{

			rtk_igmp_clientEntryPool->next->previous=NULL;
		}
		rtk_igmp_clientEntryPool = rtk_igmp_clientEntryPool->next;
		memset(ret, 0, sizeof(struct rtk_igmp_clientEntry));
	}

	ret->joinSystemSecTime = rtk_igmp_sysUpSeconds;

	return ret;
}


/* link client Entry in the front of group client list */
static void  rtk_igmp_linkClientEntry(struct rtk_igmp_groupEntry *groupEntry, struct rtk_igmp_clientEntry* clientEntry )
{
	if(NULL==clientEntry)
	{
		return;
	}

	if(NULL==groupEntry)
	{
		return;
	}

	switch(clientEntry->igmpVersion)
	{
		case IGMP_V1:	////++
			igmpSnoopingCounterVer_1++;

			break;
		case IGMP_V2:
			igmpSnoopingCounterVer_2++;
			break;
		case IGMP_V3:
			igmpSnoopingCounterVer_3++;
			break;
		case MLD_V1:
			MLDCounterVer_1++;
			break;
		case MLD_V2:
			MLDCounterVer_2++;
			break;
		default:
			break;
	}


//ADD_CLIENT_LINK:

	if(groupEntry->clientList!=NULL)
	{
		groupEntry->clientList->previous=clientEntry;
	}
	clientEntry->next = groupEntry->clientList;

	groupEntry->clientList=clientEntry;
	groupEntry->clientList->previous=NULL;


}


#define SOURCE_OPERATION

/*********************************************
			source list operation
 *********************************************/

static struct rtk_igmp_sourceEntry* rtk_igmp_searchSourceEntry(uint32 ipVersion, uint32 *sourceAddr, struct rtk_igmp_clientEntry *clientEntry)
{
	struct rtk_igmp_sourceEntry *sourcePtr=clientEntry->sourceList;
	while(sourcePtr!=NULL)
	{
		if(ipVersion==IP_VERSION4)
		{
			if(sourceAddr[0]==sourcePtr->sourceAddr[0])
			{
				return sourcePtr;
			}
		}
		else
		{
			if(	(sourceAddr[0]==sourcePtr->sourceAddr[0])&&
				(sourceAddr[1]==sourcePtr->sourceAddr[1])&&
				(sourceAddr[2]==sourcePtr->sourceAddr[2])&&
				(sourceAddr[3]==sourcePtr->sourceAddr[3])
			)
			{
				return sourcePtr;
			}
		}
		sourcePtr=sourcePtr->next;
	}

	return NULL;
}



static void rtk_igmp_linkSourceEntry(struct rtk_igmp_clientEntry *clientEntry,  struct rtk_igmp_sourceEntry* entryNode)
{
	if(NULL==entryNode)
	{
		return;
	}

	if(NULL==clientEntry)
	{
		return;
	}


	if(clientEntry->sourceList!=NULL)
	{
		clientEntry->sourceList->previous=entryNode;
	}
	entryNode->next=clientEntry->sourceList;
	clientEntry->sourceList=entryNode;
	clientEntry->sourceList->previous=NULL;

}


static void rtk_igmp_unlinkSourceEntry(struct rtk_igmp_clientEntry *clientEntry, struct rtk_igmp_sourceEntry* sourceEntry)
{
	if(NULL==sourceEntry)
	{
		return;
	}

	if(NULL==clientEntry)
	{
		return;
	}

	/* unlink entry node*/
	if(sourceEntry==clientEntry->sourceList) /*unlink group list head*/
	{
		clientEntry->sourceList=sourceEntry->next;
		if(clientEntry->sourceList!=NULL)
		{
			clientEntry->sourceList ->previous=NULL;
		}
	}
	else
	{
		if(sourceEntry->previous!=NULL)
		{
			sourceEntry->previous->next=sourceEntry->next;
		}

		if(sourceEntry->next!=NULL)
		{
			sourceEntry->next->previous=sourceEntry->previous;
		}
	}

	sourceEntry->previous=NULL;
	sourceEntry->next=NULL;

}


// allocate a source entry from the source entry pool
static  struct rtk_igmp_sourceEntry* rtk_igmp_allocateSourceEntry(void)
{
	struct rtk_igmp_sourceEntry *ret = NULL;


		if (rtk_igmp_sourceEntryPool!=NULL)
		{
			ret = rtk_igmp_sourceEntryPool;
			if(rtk_igmp_sourceEntryPool->next!=NULL)
			{
				rtk_igmp_sourceEntryPool->next->previous=NULL;
			}
			rtk_igmp_sourceEntryPool = rtk_igmp_sourceEntryPool->next;
			memset(ret, 0, sizeof(struct rtk_igmp_sourceEntry));
		}

	return ret;
}


/*source entry memory management*/
static  struct rtk_igmp_sourceEntry* rtk_igmp_initSourceEntryPool(uint32 poolSize)
{
	uint32 idx=0;
	struct rtk_igmp_sourceEntry *poolHead=NULL;
	struct rtk_igmp_sourceEntry *entryPtr=NULL;
	if (poolSize != IGMP_MAX_SOURCE_COUNT)
	{
		goto out;
	}

	/* Allocate memory */
	poolHead = &sourceEntryPool[0];

	if (poolHead != NULL)
	{
		memset(poolHead, 0,  (IGMP_MAX_SOURCE_COUNT  * sizeof(struct rtk_igmp_sourceEntry)));
		entryPtr = poolHead;

		/* link the whole source entry pool */
		for (idx = 0 ; idx < IGMP_MAX_SOURCE_COUNT ; idx++, entryPtr++)
		{
			if(idx==0)
			{
				entryPtr->previous=NULL;
				if(idx == (IGMP_MAX_SOURCE_COUNT - 1))
				{
					entryPtr->next=NULL;
				}
				else
				{
					entryPtr->next = entryPtr + 1;
				}
			}
			else
			{
				entryPtr->previous=entryPtr-1;
				if (idx == (IGMP_MAX_SOURCE_COUNT - 1))
				{
					entryPtr->next = NULL;
				}
				else
				{
					entryPtr->next = entryPtr + 1;
				}
			}

		}
	}

out:

	return poolHead;
}


static void rtk_igmp_clearSourceEntry(struct rtk_igmp_sourceEntry* sourceEntryPtr)
{
	if (NULL!=sourceEntryPtr)
	{
		memset(sourceEntryPtr, 0, sizeof(struct rtk_igmp_sourceEntry));
	}
}

// free a source entry and link it back to the source entry pool, default is link to the pool head
static  void rtk_igmp_freeSourceEntry(struct rtk_igmp_sourceEntry* sourceEntryPtr)
{
	if (!sourceEntryPtr)
	{
		return;
	}

		sourceEntryPtr->next = rtk_igmp_sourceEntryPool;
		if(rtk_igmp_sourceEntryPool!=NULL)
		{
			rtk_igmp_sourceEntryPool->previous=sourceEntryPtr;
		}

		rtk_igmp_sourceEntryPool=sourceEntryPtr;

}

/*****source entry/client entry/group entry/flow entry operation*****/
static void rtk_igmp_deleteSourceEntry(struct rtk_igmp_clientEntry *clientEntry, struct rtk_igmp_sourceEntry* sourceEntry)
{
	if (clientEntry==NULL)
	{
		return;
	}

	if (sourceEntry!=NULL)
	{
		rtk_igmp_unlinkSourceEntry(clientEntry,sourceEntry);
		rtk_igmp_clearSourceEntry(sourceEntry);
		rtk_igmp_freeSourceEntry(sourceEntry);
	}
}


static void rtk_igmp_deleteSourceList(struct rtk_igmp_clientEntry* clientEntry)
{
	struct rtk_igmp_sourceEntry *sourceEntry=NULL;
	struct rtk_igmp_sourceEntry *nextSourceEntry=NULL;

	sourceEntry=clientEntry->sourceList;
	while (sourceEntry!=NULL)
	{
		nextSourceEntry=sourceEntry->next;
		rtk_igmp_deleteSourceEntry(clientEntry,sourceEntry);
		sourceEntry=nextSourceEntry;
	}
}

#define FLOW_OPERATION
/*********************************************
			multicast flow list operation
 *********************************************/
// allocate a multicast flow entry	from the multicast flow pool
static	struct rtk_igmp_mcastFlowEntry* rtk_igmp_allocateMcastFlowEntry(void)
{
	struct rtk_igmp_mcastFlowEntry *ret = NULL;

		if (rtk_igmp_mcastFlowEntryPool!=NULL)
		{
			ret = rtk_igmp_mcastFlowEntryPool;
			if(rtk_igmp_mcastFlowEntryPool->next!=NULL)
			{
				rtk_igmp_mcastFlowEntryPool->next->previous=NULL;
			}
			rtk_igmp_mcastFlowEntryPool = rtk_igmp_mcastFlowEntryPool->next;
			memset(ret, 0, sizeof(struct rtk_igmp_mcastFlowEntry));
		}


	return ret;
}


static struct rtk_igmp_mcastFlowEntry* rtk_igmp_searchMcastFlowEntry(int32 moduleIndex, uint32 ipVersion, uint32 *serverAddr,uint32 *groupAddr,int32 sport,int32 dport,int16 ingressCvid,int16 l4proto)
{
	struct rtk_igmp_mcastFlowEntry* mcastFlowPtr = NULL;
	uint32 hashIndex=0;

	if(NULL==serverAddr)
	{
		return NULL;
	}

	if(NULL==groupAddr)
	{
		return NULL;
	}

	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddr);

	mcastFlowPtr=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[hashIndex];
	while (mcastFlowPtr!=NULL)
	{

		if(mcastFlowPtr->ipVersion!=ipVersion)
		{
			goto nextFlow;
		}


		if(ipVersion==IP_VERSION4)
		{
			//IGMP_CTRL("serverAddr =%pI4	mcastFlowPtr->serverAddr=%pI4",&serverAddr[0],&mcastFlowPtr->serverAddr[0]);
			//IGMP_CTRL("groupAddr=%pI4	groupAddr->groupAddr=%pI4",&groupAddr[0],&mcastFlowPtr->groupAddr[0]);
			//IGMP_CTRL("sport=%d dport=%d  mcastFlowPtr->sport=%d  mcastFlowPtr->dport=%d",sport,dport,mcastFlowPtr->sport,mcastFlowPtr->dport);

			if( (serverAddr[0]==mcastFlowPtr->serverAddr[0]) && (groupAddr[0]==mcastFlowPtr->groupAddr[0])&& (sport ==mcastFlowPtr->sport) && (dport ==mcastFlowPtr->dport) &&(ingressCvid ==mcastFlowPtr->ingressCvlan)&&(l4proto ==mcastFlowPtr->l4proto) )
			{
				mcastFlowPtr->refreshTime=rtk_igmp_sysUpSeconds;
				return mcastFlowPtr;
			}
		}
		else
		{
			//IGMP_CTRL("serverAddr =%pI6	mcastFlowPtr->serverAddr=%pI6",&serverAddr[0],&mcastFlowPtr->serverAddr[0]);
			//IGMP_CTRL("groupAddr=%pI6	groupAddr->groupAddr=%pI6",&groupAddr[0],&mcastFlowPtr->groupAddr[0]);
			//IGMP_CTRL("sport=%d dport=%d  mcastFlowPtr->sport=%d  mcastFlowPtr->dport=%d",sport,dport,mcastFlowPtr->sport,mcastFlowPtr->dport);
			if(	(serverAddr[0]==mcastFlowPtr->serverAddr[0])
				&&(serverAddr[1]==mcastFlowPtr->serverAddr[1])
				&&(serverAddr[2]==mcastFlowPtr->serverAddr[2])
				&&(serverAddr[3]==mcastFlowPtr->serverAddr[3])
				&&(groupAddr[0]==mcastFlowPtr->groupAddr[0])
				&&(groupAddr[1]==mcastFlowPtr->groupAddr[1])
				&&(groupAddr[2]==mcastFlowPtr->groupAddr[2])
				&&(groupAddr[3]==mcastFlowPtr->groupAddr[3])
				&&(sport ==mcastFlowPtr->sport)
				&&(dport ==mcastFlowPtr->dport)
				&&(ingressCvid ==mcastFlowPtr->ingressCvlan)
				&&(l4proto ==mcastFlowPtr->l4proto))
			{
				mcastFlowPtr->refreshTime=rtk_igmp_sysUpSeconds;
				return mcastFlowPtr;
			}
		}

nextFlow:

		mcastFlowPtr = mcastFlowPtr->next;

	}

	return NULL;
}

#define UNCTAG -1
#define IF_SOURCECARE 1
#define IF_VLANCARE 1

//we only delete timeout multicast flow entry
static int rtk_igmp_deleteUnknownFloodMacstFlow(int32 moduleIndex)
{
	uint32 i;
	struct rtk_igmp_mcastFlowEntry* mcastFlowEntry = NULL;
	struct rtk_igmp_mcastFlowEntry* nextMcastFlowEntry = NULL;
	rtk_igmp_flowCbEvt_t flowCb;

	if(rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable==NULL)
		return SUCCESS;

	for (i = 0 ; i < IGMP_HASH_TBL_SIZE ; i++)
	{
		mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[i];

		while (mcastFlowEntry!=NULL)
		{
			nextMcastFlowEntry=mcastFlowEntry->next;

			memset(&flowCb,0,sizeof(flowCb));
            
			if(mcastFlowEntry->ipVersion == IP_VERSION6)
				flowCb.isIPv6=1;
			memcpy(flowCb.group,mcastFlowEntry->groupAddr,sizeof(flowCb.group));
			memcpy(flowCb.sourceAddr,mcastFlowEntry->serverAddr,sizeof(flowCb.sourceAddr));
			memcpy(flowCb.egressDevInfo,mcastFlowEntry->multicastFwdInfo.egressDevInfo,sizeof(flowCb.egressDevInfo));
			flowCb.l4proto = mcastFlowEntry->l4proto;
			flowCb.sport = mcastFlowEntry->sport;
			flowCb.dport = mcastFlowEntry->dport;
			flowCb.ingressCvlan = mcastFlowEntry->ingressCvlan;
	
			//hwCb.flowCheckCbEvt(&flowCb);

			//printk("%s:%d, will check group ip is %x ,flowCb.floodflowdelete =%d\n",__FUNCTION__,__LINE__,mcastFlowEntry->groupAddr[0],flowCb.floodflowdelete);
				
			if(flowCb.floodflowdelete)
				rtk_igmp_deleteFloodMcastFlowEntry(mcastFlowEntry, rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);

			mcastFlowEntry=nextMcastFlowEntry;
		}
	}


	return SUCCESS;
}


//we only want to aging out the dev which disable snooping .
static int rtk_igmp_agingMcastFlow(int32 moduleIndex)
{
	uint32 i,j;
	uint32 delFlow=1;
	struct rtk_igmp_mcastFlowEntry* mcastFlowEntry = NULL;
	struct rtk_igmp_mcastFlowEntry* nextMcastFlowEntry = NULL;
	rtk_igmp_multicastDeviceInfo_t* devconf=NULL;

	if(rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable==NULL)
		return SUCCESS;

	for (i = 0 ; i < IGMP_HASH_TBL_SIZE ; i++)
	{
		mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[i];

		while (mcastFlowEntry!=NULL)
		{
			nextMcastFlowEntry=mcastFlowEntry->next;
			/*keep the most recently used entry*/
			if ((mcastFlowEntry->refreshTime+IGMP_MCAST_FLOW_EXPIRE_TIME) < rtk_igmp_sysUpSeconds)
			{
				delFlow=1;
				for( j=0 ; j < IGMP_MAX_DEV_NUM ; j++)
				{
					if(mcastFlowEntry->multicastFwdInfo.egressDevInfo[j].valid)
					{
						devconf = _rtk_igmp_devIfidx_to_devInfo(mcastFlowEntry->multicastFwdInfo.egressDevInfo[j].devIfIdx);
						if( (mcastFlowEntry->ipVersion==IP_VERSION4 && devconf && devconf->devConf.igmp.igmpSnoopingDisable==0)   || 
							(mcastFlowEntry->ipVersion==IP_VERSION6 && devconf && devconf->devConf.igmp6.igmp6SnoopingDisable==0))
						{
							delFlow=0;
						}
					}
					else
						break;
				}
				if(delFlow)
					rtk_igmp_deleteMcastFlowEntry(mcastFlowEntry,  rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);
			}
			mcastFlowEntry=nextMcastFlowEntry;
		}
	}


	return SUCCESS;
}


static void rtk_igmp_doMcastFlowRecycle(int32 moduleIndex, uint32 ipVersion)
{
	uint32 i;
	uint32 freeCnt=0;
	struct rtk_igmp_mcastFlowEntry* mcastFlowEntry = NULL;
	struct rtk_igmp_mcastFlowEntry* nextMcastFlowEntry = NULL;
	struct rtk_igmp_mcastFlowEntry* oldestMcastFlowEntry = NULL;

	if(rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable==NULL)
		return ;

	for (i = 0 ; i < IGMP_HASH_TBL_SIZE ; i++)
	{
		mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[i];

		if (oldestMcastFlowEntry==NULL)
		{
			oldestMcastFlowEntry=mcastFlowEntry;
		}

		while (mcastFlowEntry!=NULL)
		{
			nextMcastFlowEntry=mcastFlowEntry->next;
			/*keep the most recently used entry*/
			if ((mcastFlowEntry->refreshTime+IGMP_MCAST_FLOW_EXPIRE_TIME) < rtk_igmp_sysUpSeconds)
			{
				rtk_igmp_deleteMcastFlowEntry(mcastFlowEntry,  rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);
				freeCnt++;
			}
			mcastFlowEntry=nextMcastFlowEntry;
		}
	}

	if(freeCnt>0)
	{
		return;
	}

	/*if too many concurrent flow,we have to do LRU*/
	for (i = 0 ; i < IGMP_HASH_TBL_SIZE ; i++)
	{
		mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[i];

		if (oldestMcastFlowEntry==NULL)
		{
			oldestMcastFlowEntry=mcastFlowEntry;
		}

		while (mcastFlowEntry!=NULL)
		{
			nextMcastFlowEntry=mcastFlowEntry->next;
			if (mcastFlowEntry->refreshTime < oldestMcastFlowEntry->refreshTime)
			{
				oldestMcastFlowEntry=mcastFlowEntry;
			}

			mcastFlowEntry=nextMcastFlowEntry;
		}
	}

	if (oldestMcastFlowEntry!=NULL)
	{
		rtk_igmp_deleteMcastFlowEntry(oldestMcastFlowEntry,  rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);
	}

	return ;
}

static int32 _rtk_igmp_IsflowToDevInfoIsNonValidPort(rtk_igmp_flowToDevInfo_t * egressDevInfo)
{
	if(egressDevInfo[0].valid)
		return FALSE;
	else
		return TRUE;
}

static int32 rtk_igmp_recordMcastFlow(int32 moduleIndex, struct rtk_igmp_multicastDataInfo *multicastDataInfo, struct rtk_igmp_multicastFwdInfo *multicastFwdInfo)
{
	struct rtk_igmp_mcastFlowEntry *mcastFlowEntry=NULL;
	int32 ret,flowIdx;

	IGMP_CTRL("got in rtk_igmp_recordMcastFlow");
	ret= FAIL;
	flowIdx = FAIL;

	if(multicastFwdInfo==NULL)
	{
		return FAIL;
	}

	if( ( (multicastDataInfo->ipVersion==IP_VERSION4) && (multicastDataInfo->sourceIp[0]==0 || multicastDataInfo->groupAddr[0]==0)) ||
		( (multicastDataInfo->ipVersion==IP_VERSION6) && ((multicastDataInfo->sourceIp[0]==0 &&multicastDataInfo->sourceIp[1]==0 && multicastDataInfo->sourceIp[2]==0 && multicastDataInfo->sourceIp[3]==0) ||
		  (multicastDataInfo->groupAddr[0]==0 && multicastDataInfo->groupAddr[1]==0 && multicastDataInfo->groupAddr[2]==0 && multicastDataInfo->groupAddr[3]==0))))
		{IGMP_CTRL("Error v4/v6 source/group address zero"); return FAIL; }

	mcastFlowEntry=rtk_igmp_searchMcastFlowEntry(moduleIndex, multicastDataInfo->ipVersion, multicastDataInfo->sourceIp, multicastDataInfo->groupAddr,multicastDataInfo->sport,multicastDataInfo->dport,multicastDataInfo->vlanTagif?multicastDataInfo->vlanId:FAIL,multicastDataInfo->l4proto);

	if(mcastFlowEntry==NULL)
	{


		if(_rtk_igmp_IsflowToDevInfoIsNonValidPort(multicastFwdInfo->egressDevInfo)) {IGMP_CTRL("Pmsk ==0 not record to igmpMcflow"); return FAIL;}


		mcastFlowEntry=rtk_igmp_allocateMcastFlowEntry();
		if(mcastFlowEntry==NULL)
		{
			rtk_igmp_doMcastFlowRecycle(moduleIndex, multicastDataInfo->ipVersion);

			mcastFlowEntry=rtk_igmp_allocateMcastFlowEntry();
			if(mcastFlowEntry==NULL)
			{
				IGMP_CTRL("run out of multicast flow entry!\n");
				return FAIL;
			}
		}

		if(multicastDataInfo->ipVersion==IP_VERSION4)
		{
			mcastFlowEntry->serverAddr[0]=multicastDataInfo->sourceIp[0];
			mcastFlowEntry->groupAddr[0]=multicastDataInfo->groupAddr[0];

		}
		else
		{
			mcastFlowEntry->serverAddr[0]=multicastDataInfo->sourceIp[0];
			mcastFlowEntry->serverAddr[1]=multicastDataInfo->sourceIp[1];
			mcastFlowEntry->serverAddr[2]=multicastDataInfo->sourceIp[2];
			mcastFlowEntry->serverAddr[3]=multicastDataInfo->sourceIp[3];

			mcastFlowEntry->groupAddr[0]=multicastDataInfo->groupAddr[0];
			mcastFlowEntry->groupAddr[1]=multicastDataInfo->groupAddr[1];
			mcastFlowEntry->groupAddr[2]=multicastDataInfo->groupAddr[2];
			mcastFlowEntry->groupAddr[3]=multicastDataInfo->groupAddr[3];
		}


		mcastFlowEntry->ipVersion=multicastDataInfo->ipVersion;
		mcastFlowEntry->sport=multicastDataInfo->sport;
		mcastFlowEntry->dport=multicastDataInfo->dport;
		mcastFlowEntry->l4proto = multicastDataInfo->l4proto;
		if(multicastDataInfo->vlanTagif)
			mcastFlowEntry->ingressCvlan = multicastDataInfo->vlanId;
		else
			mcastFlowEntry->ingressCvlan=FAIL;

		memcpy(&mcastFlowEntry->multicastFwdInfo, multicastFwdInfo, sizeof(struct rtk_igmp_multicastFwdInfo ));

		mcastFlowEntry->refreshTime=rtk_igmp_sysUpSeconds;
		mcastFlowEntry->fwdPktCnt=1;

		rtk_igmp_linkMcastFlowEntry(mcastFlowEntry, rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);

		//return SUCCESS;

	}
	else
	{

		if(_rtk_igmp_IsflowToDevInfoIsNonValidPort(multicastFwdInfo->egressDevInfo))
		{
			IGMP_CTRL("forward to no-member in software should update to hardware");
			rtk_igmp_deleteMcastFlowEntry(mcastFlowEntry,rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable); //forward to no-member in software should update to hardware
			return SUCCESS;
		}



		/*update forward port mask information */
		memcpy(&mcastFlowEntry->multicastFwdInfo, multicastFwdInfo, sizeof(struct rtk_igmp_multicastFwdInfo ));
		mcastFlowEntry->refreshTime=rtk_igmp_sysUpSeconds;
		mcastFlowEntry->fwdPktCnt++;
		//return SUCCESS;
	}

	/* add/update flow*/
	if(multicastDataInfo->privateField & SKIP_LEARNING)
	{
		IGMP_CTRL("Do not flowSetCbEvt to hwcb because FC SKIP_LEARNING");
	}
	else if(hwCb.flowSetCbEvt)
	{
		rtk_igmp_flowCbEvt_t flowCb;
		memset(&flowCb,0,sizeof(flowCb));
		if(mcastFlowEntry->ipVersion == IP_VERSION6)
			flowCb.isIPv6=1;
		memcpy(flowCb.group,mcastFlowEntry->groupAddr,sizeof(flowCb.group));
		memcpy(flowCb.sourceAddr,mcastFlowEntry->serverAddr,sizeof(flowCb.sourceAddr));
		memcpy(flowCb.egressDevInfo,mcastFlowEntry->multicastFwdInfo.egressDevInfo,sizeof(flowCb.egressDevInfo));
		flowCb.l4proto = mcastFlowEntry->l4proto;
		flowCb.sport = mcastFlowEntry->sport;
		flowCb.dport = mcastFlowEntry->dport;
		flowCb.ingressCvlan = mcastFlowEntry->ingressCvlan;
		hwCb.flowSetCbEvt(&flowCb);
		mcastFlowEntry->multicastFwdInfo.hwCbAcc=1;
	}


	return SUCCESS;
}


// moduleIndex==FAIL search all Br Module
// groupAddress ==NULL search all  group
static int32 rtk_igmp_flow_reflush_update(int32 moduleIndex,uint32* groupAddr)
{
	struct rtk_igmp_multicastDataInfo multicastDataInfo;
	struct rtk_igmp_multicastFwdInfo multicastFwdInfo;
	struct rtk_igmp_mcastFlowEntry *mcastFlowEntry=NULL,*tmp_mcastFlowEntry=NULL;

	int32 hashIndex;
	int32 moduleIndex_start,moduleIndex_end;


	if( 0 <= moduleIndex && moduleIndex < IGMP_MAX_BR_MODULE_NUM )
	{
		moduleIndex_start = moduleIndex;
		moduleIndex_end = moduleIndex+1;
	}
	else
	{
		moduleIndex_start = 0;
		moduleIndex_end =IGMP_MAX_BR_MODULE_NUM;
	}

	for(;moduleIndex_start < moduleIndex_end;moduleIndex_start++)
	{
		if(rtk_igmp_mCastModuleArray[moduleIndex_start].validBit==0)
			continue;

		for(hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
		{
			mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex_start].flowHashTable[hashIndex];
			while(mcastFlowEntry!=NULL)
			{
				tmp_mcastFlowEntry = mcastFlowEntry->next;
				if(groupAddr==NULL || (groupAddr && memcmp(groupAddr,mcastFlowEntry->groupAddr,sizeof(mcastFlowEntry->groupAddr))==0))
				{
					memset(&multicastDataInfo,0,sizeof(multicastDataInfo));
					multicastDataInfo.dport = mcastFlowEntry->dport;
					multicastDataInfo.sport = mcastFlowEntry->sport;
					multicastDataInfo.ipVersion = mcastFlowEntry->ipVersion;
					multicastDataInfo.l4proto = mcastFlowEntry->l4proto;
					multicastDataInfo.vlanTagif =mcastFlowEntry->ingressCvlan==FAIL?0:1;
					if(multicastDataInfo.vlanTagif)
						multicastDataInfo.vlanId = mcastFlowEntry->ingressCvlan;
					memcpy(multicastDataInfo.groupAddr,mcastFlowEntry->groupAddr,sizeof(mcastFlowEntry->groupAddr));
					memcpy(multicastDataInfo.sourceIp,mcastFlowEntry->serverAddr,sizeof(mcastFlowEntry->serverAddr));

					_rtk_igmp_getMulticastDataFwdInfo(moduleIndex_start,0,&multicastDataInfo,&multicastFwdInfo);

				}
				mcastFlowEntry=tmp_mcastFlowEntry;
			}
		}

	}
	return SUCCESS;
}

/*multicast flow entry memory management*/
static  struct rtk_igmp_mcastFlowEntry* rtk_igmp_initMcastFlowEntryPool(uint32 poolSize)
{

	uint32 idx=0;
	struct rtk_igmp_mcastFlowEntry *poolHead=NULL;
	struct rtk_igmp_mcastFlowEntry *entryPtr=NULL;


	if (poolSize != IGMP_MAX_FLOW_COUNT)
	{
		goto out;
	}

	/* Allocate memory */
	poolHead = &mcastFlowEntryPool[0];


	if (poolHead != NULL)
	{
		memset(poolHead, 0,  (IGMP_MAX_FLOW_COUNT  * sizeof(struct rtk_igmp_mcastFlowEntry)));
		entryPtr = poolHead;

		/* link the whole group entry pool */
		for (idx = 0 ; idx < IGMP_MAX_FLOW_COUNT ; idx++, entryPtr++)
		{
			if(idx==0)
			{
				entryPtr->previous=NULL;
				if(idx == (IGMP_MAX_FLOW_COUNT - 1))
				{
					entryPtr->next=NULL;
				}
				else
				{
					entryPtr->next = entryPtr + 1;
				}
			}
			else
			{
				entryPtr->previous=entryPtr-1;
				if (idx == (IGMP_MAX_FLOW_COUNT - 1))
				{
					entryPtr->next = NULL;
				}
				else
				{
					entryPtr->next = entryPtr + 1;
				}
			}
		}
	}

out:


	return poolHead;

}

/* link multicast flow entry in the front of a forwarding flow list */

static void  rtk_igmp_linkMcastFlowEntry(struct rtk_igmp_mcastFlowEntry* mcastFlowEntry ,  struct rtk_igmp_mcastFlowEntry ** hashTable)
{
	uint32 hashIndex=0;

	if(NULL==mcastFlowEntry)
	{
		return;
	}

	if(NULL==hashTable)
	{
		return;
	}


	hashIndex=rtk_igmp_hashAlgorithm(mcastFlowEntry->ipVersion, mcastFlowEntry->groupAddr);
	if(hashTable[hashIndex]!=NULL)
	{
		hashTable[hashIndex]->previous=mcastFlowEntry;
	}
	mcastFlowEntry->next = hashTable[hashIndex];
	hashTable[hashIndex]=mcastFlowEntry;
	hashTable[hashIndex]->previous=NULL;

	return;

}


static void rtk_igmp_invalidateMCastGroupFlow(int32 moduleIndex,uint32 ipVersion, uint32 *groupAddr)
{
	uint32 hashIndex;
	struct rtk_igmp_mcastFlowEntry* mcastFlowEntry = NULL;
	struct rtk_igmp_mcastFlowEntry* nextMcastFlowEntry = NULL;

	if (NULL==groupAddr)
	{
		return ;
	}

	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddr);

	mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[hashIndex];

	while (mcastFlowEntry!=NULL)
	{
		nextMcastFlowEntry=mcastFlowEntry->next;

		if(ipVersion==mcastFlowEntry->ipVersion)
		{

			if ((groupAddr[0]==0)&&(groupAddr[1]==0)&&(groupAddr[2]==0)&&(groupAddr[3]==0))
			{
				rtk_igmp_deleteMcastFlowEntry(mcastFlowEntry, rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);
			}
			else
			{
				if(ipVersion ==IP_VERSION4)
				{
					if ((mcastFlowEntry->groupAddr[0]==groupAddr[0]))
					{
						rtk_igmp_deleteMcastFlowEntry(mcastFlowEntry, rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);
					}

				}
				else if (ipVersion ==IP_VERSION6)
				{
					if ((mcastFlowEntry->groupAddr[0]==groupAddr[0])&&(mcastFlowEntry->groupAddr[1]==groupAddr[1])&&
					(mcastFlowEntry->groupAddr[2]==groupAddr[2])&&(mcastFlowEntry->groupAddr[3]==groupAddr[3]))
					{
						rtk_igmp_deleteMcastFlowEntry(mcastFlowEntry, rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable);
					}

				}
			}
		}

		mcastFlowEntry = nextMcastFlowEntry;
	}

	return ;
}


/* unlink a multicast flow entry*/
static void rtk_igmp_unlinkMcastFlowEntry(struct rtk_igmp_mcastFlowEntry* mcastFlowEntry,  struct rtk_igmp_mcastFlowEntry ** hashTable)
{
	uint32 hashIndex=0;
	if(NULL==mcastFlowEntry)
	{
		return;
	}

	hashIndex=rtk_igmp_hashAlgorithm(mcastFlowEntry->ipVersion, mcastFlowEntry->groupAddr);
	/* unlink entry node*/
	if(mcastFlowEntry==hashTable[hashIndex]) /*unlink flow list head*/
	{
		hashTable[hashIndex]=mcastFlowEntry->next;
		if(hashTable[hashIndex]!=NULL)
		{
			hashTable[hashIndex]->previous=NULL;
		}
	}
	else
	{
		if(mcastFlowEntry->previous!=NULL)
		{
			mcastFlowEntry->previous->next=mcastFlowEntry->next;
		}

		if(mcastFlowEntry->next!=NULL)
		{
			mcastFlowEntry->next->previous=mcastFlowEntry->previous;
		}
	}

	mcastFlowEntry->previous=NULL;
	mcastFlowEntry->next=NULL;


}


/* clear the content of multicast flow entry */
static void rtk_igmp_clearMcastFlowEntry(struct rtk_igmp_mcastFlowEntry* mcastFlowEntry)
{
	if (NULL!=mcastFlowEntry)
	{
		memset(mcastFlowEntry, 0, sizeof(struct rtk_igmp_mcastFlowEntry));
	}
}


// free a multicast flow entry and link it back to the multicast flow entry pool, default is link to the pool head
static  void rtk_igmp_freeMcastFlowEntry(struct rtk_igmp_mcastFlowEntry* mcastFlowEntry)
{
	if (NULL==mcastFlowEntry)
	{
		return;
	}

	mcastFlowEntry->next = rtk_igmp_mcastFlowEntryPool;
	if(rtk_igmp_mcastFlowEntryPool!=NULL)
	{
		rtk_igmp_mcastFlowEntryPool->previous=mcastFlowEntry;
	}
	rtk_igmp_mcastFlowEntryPool=mcastFlowEntry;

}

struct rtk_igmp_mcastFlowEntry* rtk_igmp_searchMcastFlowEntryAllModule(uint32 ipVersion, uint32 *serverAddr,uint32 *groupAddr,int32 sport,int32 dport,int16 ingressCvid,int16 l4proto)
{	
	struct rtk_igmp_mcastFlowEntry* mcastFlowPtr = NULL;
	int32 hashIndex=0;
	int32 moduleIndex = 0;

	if(NULL==serverAddr)
	{
		return NULL;
	}

	if(NULL==groupAddr)
	{
		return NULL;
	}

	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddr);

	for(moduleIndex=0 ; moduleIndex<IGMP_MAX_BR_MODULE_NUM ;moduleIndex++ )
	{
		if(rtk_igmp_mCastModuleArray[moduleIndex].validBit==0)
			continue;

		mcastFlowPtr=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[hashIndex];

		while(mcastFlowPtr!=NULL)
		{
			if(mcastFlowPtr->ipVersion!=ipVersion)
			{
				goto nextMcFlow;
			}


			if(ipVersion==IP_VERSION4)
			{
				//IGMP_CTRL("serverAddr =%pI4	mcastFlowPtr->serverAddr=%pI4",&serverAddr[0],&mcastFlowPtr->serverAddr[0]);
				//IGMP_CTRL("groupAddr=%pI4	groupAddr->groupAddr=%pI4",&groupAddr[0],&mcastFlowPtr->groupAddr[0]);
				//IGMP_CTRL("sport=%d dport=%d  mcastFlowPtr->sport=%d  mcastFlowPtr->dport=%d",sport,dport,mcastFlowPtr->sport,mcastFlowPtr->dport);

				if( (serverAddr[0]==mcastFlowPtr->serverAddr[0]) && (groupAddr[0]==mcastFlowPtr->groupAddr[0])&& (sport ==mcastFlowPtr->sport) && (dport ==mcastFlowPtr->dport) &&(ingressCvid ==mcastFlowPtr->ingressCvlan)&&(l4proto ==mcastFlowPtr->l4proto) )
				{
					return mcastFlowPtr;
				}
			}
			else
			{
				//IGMP_CTRL("serverAddr =%pI6	mcastFlowPtr->serverAddr=%pI6",&serverAddr[0],&mcastFlowPtr->serverAddr[0]);
				//IGMP_CTRL("groupAddr=%pI6	groupAddr->groupAddr=%pI6",&groupAddr[0],&mcastFlowPtr->groupAddr[0]);
				//IGMP_CTRL("sport=%d dport=%d  mcastFlowPtr->sport=%d  mcastFlowPtr->dport=%d",sport,dport,mcastFlowPtr->sport,mcastFlowPtr->dport);
				if((serverAddr[0]==mcastFlowPtr->serverAddr[0])
					&&(serverAddr[1]==mcastFlowPtr->serverAddr[1])
					&&(serverAddr[2]==mcastFlowPtr->serverAddr[2])
					&&(serverAddr[3]==mcastFlowPtr->serverAddr[3])
					&&(groupAddr[0]==mcastFlowPtr->groupAddr[0])
					&&(groupAddr[1]==mcastFlowPtr->groupAddr[1])
					&&(groupAddr[2]==mcastFlowPtr->groupAddr[2])
					&&(groupAddr[3]==mcastFlowPtr->groupAddr[3])
					&&(sport ==mcastFlowPtr->sport)
					&&(dport ==mcastFlowPtr->dport)
					&&(ingressCvid ==mcastFlowPtr->ingressCvlan)
					&&(l4proto ==mcastFlowPtr->l4proto))
				{
					return mcastFlowPtr;
				}
			}

nextMcFlow:

			mcastFlowPtr = mcastFlowPtr->next;
		}
	}
	return NULL;
	
}

static void rtk_igmp_deleteMcastFlowEntry( struct rtk_igmp_mcastFlowEntry* mcastFlowEntry, struct rtk_igmp_mcastFlowEntry ** hashTable)
{
	struct rtk_igmp_mcastFlowEntry* mcastFlowPtr = NULL;
	uint32 groupAddr[4];	
	uint32 serverAddr[4];	
	uint32 ipVersion;
	int32 sport,dport;
	int32 ingressCvid;
	int16 l4proto;
	uint8 hwCbAcc;

	memset(groupAddr, 0, sizeof(groupAddr));
	memset(serverAddr, 0, sizeof(serverAddr));

	ipVersion = mcastFlowEntry->ipVersion;
	memcpy(groupAddr,mcastFlowEntry->groupAddr,sizeof(groupAddr));
	memcpy(serverAddr,mcastFlowEntry->serverAddr,sizeof(serverAddr));
	sport = mcastFlowEntry->sport;
	dport = mcastFlowEntry->dport;
	ingressCvid = mcastFlowEntry->ingressCvlan;
	l4proto = mcastFlowEntry->l4proto;
	hwCbAcc = mcastFlowEntry->multicastFwdInfo.hwCbAcc;

	/*if(hwCb.flowDelCbEvt && mcastFlowEntry->multicastFwdInfo.hwCbAcc)
	{
		rtk_igmp_flowCbEvt_t flowCb;
		memset(&flowCb,0,sizeof(flowCb));
		if(mcastFlowEntry->ipVersion == IP_VERSION6)
			flowCb.isIPv6=1;
		memcpy(flowCb.group,mcastFlowEntry->groupAddr,sizeof(flowCb.group));
		memcpy(flowCb.sourceAddr,mcastFlowEntry->serverAddr,sizeof(flowCb.sourceAddr));
		flowCb.sport = mcastFlowEntry->sport;
		flowCb.dport = mcastFlowEntry->dport;
		flowCb.ingressCvlan = mcastFlowEntry->ingressCvlan;
		flowCb.l4proto = mcastFlowEntry->l4proto;
		hwCb.flowDelCbEvt(&flowCb);
	}*/

	if(mcastFlowEntry!=NULL)
	{
		rtk_igmp_unlinkMcastFlowEntry(mcastFlowEntry, hashTable);
		rtk_igmp_clearMcastFlowEntry(mcastFlowEntry);
		rtk_igmp_freeMcastFlowEntry(mcastFlowEntry);

		mcastFlowPtr = rtk_igmp_searchMcastFlowEntryAllModule(ipVersion, serverAddr, groupAddr, sport, dport, ingressCvid, l4proto);

		if (mcastFlowPtr)
		{
			rtk_igmp_flow_reflush_update(FAIL, groupAddr);
		}
		else
		{
			if(hwCb.flowDelCbEvt && hwCbAcc)
			{
				rtk_igmp_flowCbEvt_t flowCb;
				memset(&flowCb,0,sizeof(flowCb));
				if(ipVersion == IP_VERSION6)
					flowCb.isIPv6=1;
				memcpy(flowCb.group,groupAddr,sizeof(flowCb.group));
				memcpy(flowCb.sourceAddr,serverAddr,sizeof(flowCb.sourceAddr));
				flowCb.sport = sport;
				flowCb.dport = dport;
				flowCb.ingressCvlan = ingressCvid;
				flowCb.l4proto = l4proto;
				hwCb.flowDelCbEvt(&flowCb);
			}
		}
	}

	return;
}

void rtk_igmp_deleteFloodMcastFlowEntry( struct rtk_igmp_mcastFlowEntry* mcastFlowEntry, struct rtk_igmp_mcastFlowEntry ** hashTable)
{

	if(mcastFlowEntry!=NULL)
	{
		rtk_igmp_unlinkMcastFlowEntry(mcastFlowEntry, hashTable);
		rtk_igmp_clearMcastFlowEntry(mcastFlowEntry);
		rtk_igmp_freeMcastFlowEntry(mcastFlowEntry);
	}

	return;
}


#define IGMP_RECORED

int32 rtk_igmp_client_cnt(rtk_igmp_pktHdr_t *pPkthdr,uint32 ipVersion, uint32 *groupAddr, uint32 *clientAddr)
{

	int32 hashIndex;
	struct rtk_igmp_groupEntry *groupEntryPtr;
	struct rtk_igmp_clientEntry* clientEntry=NULL;


	if (ipVersion==IP_VERSION4)
	{

		for (hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
		{
			groupEntryPtr=rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable[hashIndex];
			while (groupEntryPtr!=NULL)
			{
				clientEntry=groupEntryPtr->clientList;
				while (clientEntry!=NULL)
				{
					if((clientAddr[0])==clientEntry->clientAddr[0])
					{
						if(groupAddr[0]==groupEntryPtr->groupAddr[0])
							return SUCCESS;
						else
							return FAIL;
					}
					clientEntry = clientEntry->next;
				}
				groupEntryPtr=groupEntryPtr->next;
			}

		}
	}


	if(ipVersion==IP_VERSION6 )
	{
		for(hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
		{
			groupEntryPtr=rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable[hashIndex];
			while(groupEntryPtr!=NULL)
			{
				clientEntry=groupEntryPtr->clientList;
				while (clientEntry!=NULL)
				{
					if(memcmp(clientAddr,clientEntry->clientAddr,sizeof(clientEntry->clientAddr)))
					{
						if(memcmp(groupAddr,groupEntryPtr->groupAddr,sizeof(groupEntryPtr->groupAddr)))
							return SUCCESS;
						else
							return FAIL;
					}
					clientEntry = clientEntry->next;
				}
				groupEntryPtr=groupEntryPtr->next;
			}
		}
	}


	return FAIL;



}


rtk_igmp_nf_ret_e_t rtk_igmp_group_limitCheck(rtk_igmp_pktHdr_t *pPkthdr,uint32 ipVersion, uint32 *groupAddr , uint32 *clientAddr ,uint32 *mcSourceAddr,uint8 *saMac,uint32 vlanId ,uint32 devIfindex,rtk_igmp_nf_ret_e_t overLimitAction)
{

	//struct rtk_igmp_groupEntry* groupEntry=NULL;
	int32 hashIndex;
	int32 igmpTotalGroupCnt=0,mldTotalGroupCnt=0,totalGroupCnt=0;
	int32 igmpTotalClientCnt=0,mldTotalClientCnt=0;					//we should deduct same Client
	int32 specificClientJoinIGMPCnt=0,specificClientJoinMLDCnt=0;
	int32 igmpDevGroupCnt=0,mldDevGroupCnt=0;
	int32 igmpGroupAndDevHit=0,mldGroupAndDevHit=0;
	struct rtk_igmp_groupEntry *groupEntryPtr;
	struct rtk_igmp_clientEntry* clientEntry=NULL;

	rtk_igmp_DevConfdb_t *devConf = &pPkthdr->devInfo->devConf;
	rtk_igmp_brConfdb_t *brDevconf = &rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.brDevConf;

	if(ipVersion==IP_VERSION4)
	{
		pPkthdr->devInfo->devStatistic.igmp4JoinCnt++;
		rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.brStatistic.igmp4JoinCnt++;
	}
	else
	{
		pPkthdr->devInfo->devStatistic.igmp6JoinCnt++;
		rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.brStatistic.igmp6JoinCnt++;
	}


	for (hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
	{
		groupEntryPtr=rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable[hashIndex];
		while (groupEntryPtr!=NULL)
		{
			igmpTotalGroupCnt++;
			clientEntry=groupEntryPtr->clientList;
			while (clientEntry!=NULL)
			{
				if(rtk_igmp_client_cnt(pPkthdr,IP_VERSION4,groupEntryPtr->groupAddr,clientEntry->clientAddr)==SUCCESS)
					igmpTotalClientCnt++;
				if(ipVersion==IP_VERSION4 &&  (clientAddr[0])==clientEntry->clientAddr[0])
					specificClientJoinIGMPCnt++;
				if(clientEntry->inIfidx== devIfindex)
					igmpDevGroupCnt++;
				if(ipVersion==IP_VERSION4 && groupAddr[0]==groupEntryPtr->groupAddr[0] && clientEntry->inIfidx== devIfindex)
					igmpGroupAndDevHit=1;

				clientEntry = clientEntry->next;
			}
			groupEntryPtr=groupEntryPtr->next;
		}

	}

	for(hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
	{
		groupEntryPtr=rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable[hashIndex];
		while(groupEntryPtr!=NULL)
		{
			mldTotalGroupCnt++;
			clientEntry=groupEntryPtr->clientList;
			while (clientEntry!=NULL)
			{
				if(rtk_igmp_client_cnt(pPkthdr,IP_VERSION6,groupEntryPtr->groupAddr,clientEntry->clientAddr)==SUCCESS)
					mldTotalClientCnt++;
				if(ipVersion==IP_VERSION6 && !memcmp(clientAddr,clientEntry->clientAddr,sizeof(clientEntry->clientAddr)))
					specificClientJoinMLDCnt++;
				if( clientEntry->inIfidx== devIfindex)
					mldDevGroupCnt++;
				if(ipVersion==IP_VERSION6 && !memcmp(groupAddr,groupEntryPtr->groupAddr,sizeof(groupEntryPtr->groupAddr)) && clientEntry->inIfidx== devIfindex)
					mldGroupAndDevHit=1;


				clientEntry = clientEntry->next;
			}
			groupEntryPtr=groupEntryPtr->next;
		}
	}

	IGMP_CTRL("igmpTotalGroupCnt:%d mldTotalGroupCnt:%d",igmpTotalGroupCnt,mldTotalGroupCnt);
	IGMP_CTRL("igmpTotalClientCnt:%d mldTotalClientCnt:%d",igmpTotalClientCnt,mldTotalClientCnt);
	IGMP_CTRL("igmpDevGroupCnt:%d mldDevGroupCnt:%d",igmpDevGroupCnt,mldDevGroupCnt);
	IGMP_CTRL("specificClientJoinIGMPCnt:%d specificClientJoinMLDCnt:%d",specificClientJoinIGMPCnt,specificClientJoinMLDCnt);

	/* Max system group check*/
	totalGroupCnt = igmpTotalGroupCnt+ mldTotalGroupCnt;
	groupEntryPtr=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex, ipVersion, groupAddr,vlanId);


	//(this is new group) or (group exist but not this port)
	if(!(groupEntryPtr&& ( igmpGroupAndDevHit || mldGroupAndDevHit)))
	{
		if(ipVersion==IP_VERSION4 && (devConf->igmp.igmpMaxGroupSize) && (igmpDevGroupCnt >= devConf->igmp.igmpMaxGroupSize ))
		{
			IGMP_CTRL("GorupPer Dev:%s out of limit igmpMaxGroupSize=%d  igmpPortGroupCnt=%d",pPkthdr->devInfo->devName,devConf->igmp.igmpMaxGroupSize,igmpDevGroupCnt);
			return overLimitAction;
		}

		if(ipVersion==IP_VERSION6 && (devConf->igmp6.igmp6MaxGroupSize) && (mldDevGroupCnt >= devConf->igmp6.igmp6MaxGroupSize))
		{
			IGMP_CTRL("GorupPer Dev:%s out of limit igmpMaxGroupSize=%d  mldPortGroupCnt=%d",pPkthdr->devInfo->devName,devConf->igmp6.igmp6MaxGroupSize,mldDevGroupCnt);
			return overLimitAction;
		}

		if(brDevconf->sysIp4MaxGroupSize  && (igmpTotalGroupCnt >=brDevconf->sysIp4MaxGroupSize))
		{
			IGMP_CTRL("Group out of DevBr:%s Limit sysIp4MaxGroupSize=%d	igmpTotalGroupCnt=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,brDevconf->sysIp4MaxGroupSize,igmpTotalGroupCnt);
			return overLimitAction;
		}
		if(brDevconf->sysIp6MaxGroupSize && (mldTotalGroupCnt >=brDevconf->sysIp6MaxGroupSize))
		{
			IGMP_CTRL("Group out of DevBr:%s Limit sysIp6MaxGroupSize=%d	mldTotalGroupCnt=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,brDevconf->sysIp6MaxGroupSize,mldTotalGroupCnt);
			return overLimitAction;

		}
		if(brDevconf->sysMaxGroupSize && (totalGroupCnt >= brDevconf->sysMaxGroupSize))
		{
			IGMP_CTRL("Group out of DevBr:%s Limit sysMaxGroupSize=%d	totalGroupCnt=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,brDevconf->sysMaxGroupSize,totalGroupCnt);
			return overLimitAction;
		}
		
		if(rtk_igmp_weightOverCheck(pPkthdr->devInfo,&rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo,ipVersion,groupAddr)!=SUCCESS)
		{
			IGMP_CTRL("Ingore Learning By weightOverCheck %s",overLimitAction==RTK_IGMP_NF_DROP?"DROP":overLimitAction==RTK_IGMP_NF_ACCEPT?"ACCEPT":"CONTINUE");
			return overLimitAction;
		}

		
	}

	if(specificClientJoinIGMPCnt)
	{/* Old Client , Per Client join group Number limit check*/
		uint32 clientInGroup=0;
		if(groupEntryPtr!=NULL)
		{
			clientEntry=groupEntryPtr->clientList;
			while (clientEntry!=NULL)
			{
				if(groupEntryPtr->ipVersion ==IP_VERSION4 &&  (clientAddr[0])==clientEntry->clientAddr[0])
				{
					clientInGroup=1;
				}
				else if (groupEntryPtr->ipVersion ==IP_VERSION6  && memcmp(clientAddr,clientEntry->clientAddr,sizeof(clientEntry->clientAddr)))
				{
					clientInGroup=1;
				}
				clientEntry = clientEntry->next;
			}

		}

		if((groupEntryPtr==NULL || clientInGroup==0) && (brDevconf->perIp4ClientMaxJoinGroupSize) && (specificClientJoinIGMPCnt >= brDevconf->perIp4ClientMaxJoinGroupSize))
		{
			IGMP_CTRL("System DevBr:%s Client out of Limit specificClientJoinIGMPCnt:%d perIp4ClientMaxJoinGroupSize=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,specificClientJoinIGMPCnt,brDevconf->perIp4ClientMaxJoinGroupSize);
			return overLimitAction;
		}
	}
	else
	{ /*New Client Max system Client check*/
		if((brDevconf->sysIp4MaxClientSize) && (igmpTotalClientCnt>=brDevconf->sysIp4MaxClientSize))
		{
			IGMP_CTRL("System DevBr:%s Client out of Limit igmpTotalClientCnt=%d sysIp4MaxClientSize=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,igmpTotalClientCnt,brDevconf->sysIp4MaxClientSize);
			return overLimitAction;
		}

		if((brDevconf->sysMaxClientSize) && ((igmpTotalClientCnt+mldTotalClientCnt)>=brDevconf->sysMaxClientSize))
		{
			IGMP_CTRL("System DevBr:%s Client out of Limit igmpTotalClientCnt+mldTotalClientCnt=%d sysMaxClientSize=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,(igmpTotalClientCnt+mldTotalClientCnt),brDevconf->sysMaxClientSize);
			return overLimitAction;
		} 

	}



	if(specificClientJoinMLDCnt)
	{/* Old Client , Per Client join group Number limit check*/
		uint32 clientInGroup=0;
		if(groupEntryPtr!=NULL)
		{
			clientEntry=groupEntryPtr->clientList;
			while (clientEntry!=NULL)
			{
				if(groupEntryPtr->ipVersion ==IP_VERSION4 &&  (clientAddr[0])==clientEntry->clientAddr[0])
				{
					clientInGroup=1;
				}
				else if (groupEntryPtr->ipVersion ==IP_VERSION6  && memcmp(clientAddr,clientEntry->clientAddr,sizeof(clientEntry->clientAddr)))
				{
					clientInGroup=1;
				}
				clientEntry = clientEntry->next;
			}

		}

		if((groupEntryPtr==NULL || clientInGroup==0) && (brDevconf->perIp6ClientMaxJoinGroupSize) && (specificClientJoinMLDCnt >= brDevconf->perIp6ClientMaxJoinGroupSize))
		{
			IGMP_CTRL("System DevBr:%s Client out of Limit specificClientJoinMLDCnt:%d perClientMaxJoinGroupSize=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,specificClientJoinMLDCnt,brDevconf->perIp6ClientMaxJoinGroupSize);
			return overLimitAction;
		}

	}
	else
	{ /*New Client Max system Client check*/
		if((brDevconf->sysIp6MaxClientSize) && (mldTotalClientCnt>=brDevconf->sysIp6MaxClientSize))
		{
			IGMP_CTRL("System DevBr:%s Client out of Limit mldTotalClientCnt=%d sysIp6MaxClientSize=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,mldTotalClientCnt,brDevconf->sysIp6MaxClientSize);
			return overLimitAction;
		}

		if((brDevconf->sysMaxClientSize) && ((igmpTotalClientCnt+mldTotalClientCnt)>=brDevconf->sysMaxClientSize))
		{
			IGMP_CTRL("System DevBr:%s Client out of Limit igmpTotalClientCnt+mldTotalClientCnt=%d sysMaxClientSize=%d",rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,(igmpTotalClientCnt+mldTotalClientCnt),brDevconf->sysMaxClientSize);
			return overLimitAction;
		}
	}

	return RTK_IGMP_NF_CONTINUE;
}



static rtk_igmp_nf_ret_e_t rtk_igmp_processJoin( rtk_igmp_pktHdr_t *pPkthdr, const struct net_device *SrcDev)
{
	rtk_igmp_nf_ret_e_t ret=RTK_IGMP_NF_CONTINUE;
	uint32 groupAddress[4]={0, 0, 0, 0};
	uint32 clientAddr[4]={0, 0, 0, 0};
	struct rtk_igmp_groupEntry* groupEntry=NULL;
	struct rtk_igmp_groupEntry* newGroupEntry=NULL;
	struct rtk_igmp_clientEntry* clientEntry=NULL;
	struct rtk_igmp_clientEntry* newClientEntry=NULL;
	uint32 groupReFlush=0;
	uint32 ipVersion;
	uint8 * pktBuf;
	uint32 hashIndex=0;
	uint32 clientAgingTime=0;

	if(pPkthdr->iph && pPkthdr->ip6h==NULL)
	{
		ipVersion= IP_VERSION4;
		clientAgingTime = pPkthdr->devInfo->devConf.igmp.igmpMcMemberAgingInterval;
		pktBuf=(uint8 *)pPkthdr->igmph;
		clientAddr[0]=ntohl(pPkthdr->iph->saddr);
	}
	else if (pPkthdr->iph==NULL && pPkthdr->ip6h)
	{
		ipVersion= IP_VERSION6;
		clientAgingTime = pPkthdr->devInfo->devConf.igmp6.igmp6McMemberAgingInterval;
		pktBuf=(uint8 *)pPkthdr->icmp6h;
		memcpy(&clientAddr[0],&pPkthdr->ip6h->saddr.s6_addr32[0],sizeof(clientAddr));
	}
	else if (pPkthdr->iph && pPkthdr->ip6h)
	{
		ipVersion= BOTH_IPV4_IPV6;
		IGMP_CTRL("Not support");
		return RTK_IGMP_NF_ACCEPT;
	}
	else
	{
		IGMP_CTRL("Not support");
		return RTK_IGMP_NF_ACCEPT;
	}


	if(ipVersion==IP_VERSION4)
	{
		if(pktBuf[0]==0x12)
		{
			IGMP_CTRL("IGMPv1_Report");
			groupAddress[0]=ntohl(((struct igmpv1Pkt *)pktBuf)->groupAddr);
		}

		if(pktBuf[0]==0x16)
		{
			IGMP_CTRL("IGMPv2_Join");
			groupAddress[0]=ntohl(((struct igmpv2Pkt *)pktBuf)->groupAddr);
		}

	}
	else
	{
		IGMP_CTRL("MLDv1_Join");
		memcpy(groupAddress,((struct mldv1Pkt *)pktBuf)->mCastAddr,sizeof(groupAddress));
	}
	if((ipVersion==IP_VERSION4 && IN_MULTICAST_RESV_IPV4_ALL(groupAddress[0])) ||
		(ipVersion==IP_VERSION6 && IN_MULTICAST_RESV_IPV6_ALL(groupAddress[0])))
	{
		IGMP_CTRL("IGNORE RESERVED ADDRESS");
		return RTK_IGMP_NF_ACCEPT;
	}

	ret = rtk_igmp_group_limitCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL,0,pPkthdr->devIfindex,RTK_IGMP_NF_DROP);
	if(ret!=RTK_IGMP_NF_CONTINUE)
	{
		IGMP_CTRL("Limit check FAIL Ingore Snooping learning ret=%d",ret);
		return ret;
	}

	if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL)!= SUCCESS)
	{
		IGMP_CTRL("Ingore Learning By White List check  DROP");
		return RTK_IGMP_NF_DROP;
	}


	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddress);

	groupEntry=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex,ipVersion, groupAddress, 0);


	if (groupEntry==NULL)   /*means new group address, create new group entry*/
	{
		IGMP_CTRL("record new gip");
		newGroupEntry=rtk_igmp_allocateGroupEntry();

		if(newGroupEntry==NULL)
		{
			IGMP_CTRL("run out of group entry!\n");
			goto out;
		}

		//assert(newGroupEntry->clientList==NULL);

		newGroupEntry->groupAddr[0]=groupAddress[0];
		newGroupEntry->groupAddr[1]=groupAddress[1];
		newGroupEntry->groupAddr[2]=groupAddress[2];
		newGroupEntry->groupAddr[3]=groupAddress[3];

		newGroupEntry->ipVersion=ipVersion;
		newGroupEntry->vlanId= 0;


		if(ipVersion==IP_VERSION4)
		{
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[ pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable);
		}
		else
		{
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[ pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable);
		}

		groupEntry=newGroupEntry;
	}

	clientEntry=rtk_igmp_searchClientEntry(ipVersion, groupEntry, pPkthdr->devIfindex, clientAddr);
	if (clientEntry==NULL)
	{
		IGMP_CTRL("record new client");
		newClientEntry=rtk_igmp_allocateClientEntry();
		if (newClientEntry==NULL)
		{
			IGMP_CTRL("run out of client entry!\n");
			goto out;
		}

		//assert(newClientEntry->sourceList==NULL);
		newClientEntry->inIfidx = pPkthdr->devIfindex;

		if (pktBuf[0]==0x12) ////++
			newClientEntry->igmpVersion=IGMP_V1;	////++
		else	////++
			newClientEntry->igmpVersion=IGMP_V2;

		if (ipVersion==IP_VERSION4)
		{
			newClientEntry->clientAddr[0]=clientAddr[0];

		}
		else
		{
			newClientEntry->igmpVersion=MLD_V1;
			newClientEntry->clientAddr[0]=clientAddr[0];
			newClientEntry->clientAddr[1]=clientAddr[1];
			newClientEntry->clientAddr[2]=clientAddr[2];
			newClientEntry->clientAddr[3]=clientAddr[3];

		}

		rtk_igmp_linkClientEntry(groupEntry, newClientEntry);
		clientEntry=newClientEntry;
		groupReFlush =1;


	}

	if(pPkthdr->ingressCtagif )
	{
		clientEntry->reportCtagif = 1;
		clientEntry->reportVlanId = pPkthdr->ingressCvid;
	}

	clientEntry->reportUpdateJiffies=jiffies;
	//IGMP_CTRL("update client[0x%x] jiffies to %lu",clientEntry->clientAddr[0],clientEntry->reportUpdateJiffies);

	rtk_igmp_deleteSourceList(clientEntry);
	//clientEntry->igmpVersion=IGMP_V2;
	clientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds+clientAgingTime;

	rtk_igmp_flow_reflush_update(FAIL,groupAddress);


out:
	return RTK_IGMP_NF_ACCEPT;//(multicastRouterPortMask & (~(1<<portNum)) & IGMP_SUPPORT_PORT_MASK);
}

static  uint32 rtk_igmp_processLeave(rtk_igmp_pktHdr_t *pPkthdr, const struct net_device *SrcDev)
{
	uint32 groupAddress[4]={0, 0, 0, 0};
	uint32 clientAddr[4]={0, 0, 0, 0};
	struct rtk_igmp_groupEntry* groupEntry=NULL;
	struct rtk_igmp_clientEntry *clientEntry=NULL;
	struct rtk_igmp_sourceEntry *sourceEntry=NULL;
	struct rtk_igmp_sourceEntry *nextSourceEntry=NULL;

	uint32 ipVersion;
	uint8 * pktBuf;
	uint32 hashIndex=0;
	uint32 _fastLeave=0;
	uint32 lastAgingTime=0;

	if(pPkthdr->iph && pPkthdr->ip6h==NULL)
	{
		ipVersion= IP_VERSION4;
		_fastLeave = pPkthdr->devInfo->devConf.igmp.igmpfastLeave;
		pktBuf=(uint8 *)pPkthdr->igmph;
		clientAddr[0]=ntohl(pPkthdr->iph->saddr);
		lastAgingTime =  pPkthdr->devInfo->devConf.igmp.igmpMcLastMemberAgingInterval;
	}
	else if (pPkthdr->iph==NULL && pPkthdr->ip6h)
	{
		ipVersion= IP_VERSION6;
		_fastLeave = pPkthdr->devInfo->devConf.igmp6.igmp6fastLeave;
		pktBuf=(uint8 *)pPkthdr->icmp6h;
		memcpy(&clientAddr[0],&pPkthdr->ip6h->saddr.s6_addr32[0],sizeof(clientAddr));
		lastAgingTime = pPkthdr->devInfo->devConf.igmp6.igmp6McLastMemberAgingInterval;
	}
	else if (pPkthdr->iph && pPkthdr->ip6h)
	{
		ipVersion= BOTH_IPV4_IPV6;
		IGMP_CTRL("Not support");
		return RTK_IGMP_NF_ACCEPT;
	}
	else
	{
		IGMP_CTRL("Not support");
		return RTK_IGMP_NF_ACCEPT;
	}

	//uint32 multicastRouterPortMask=rtk_igmp_getMulticastRouterPortMask(moduleIndex, ipVersion, rtk_igmp_sysUpSeconds);
	IGMP_CTRL("IGMP/MLD Leave packet");

	if(ipVersion==IP_VERSION4)
	{
		groupAddress[0]=ntohl(((struct igmpv2Pkt *)pktBuf)->groupAddr);
	}
	else
	{
		memcpy(groupAddress,((struct mldv1Pkt *)pktBuf)->mCastAddr,IPV6ADDRLEN);
	}


	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddress);

	groupEntry=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex, ipVersion, groupAddress, 0);

	if(groupEntry!=NULL)
	{
		clientEntry=rtk_igmp_searchClientEntry( ipVersion, groupEntry, pPkthdr->devIfindex, clientAddr);
		if(clientEntry!=NULL)
		{
			if(_fastLeave)
			{
				rtk_igmp_deleteClientEntry(groupEntry, clientEntry);
			}
			else
			{
				while(sourceEntry!=NULL)
				{
					nextSourceEntry=sourceEntry->next;
					if(sourceEntry->portTimer>rtk_igmp_sysUpSeconds+lastAgingTime)
					{
						sourceEntry->portTimer=rtk_igmp_sysUpSeconds+lastAgingTime;
					}
					sourceEntry=nextSourceEntry;
				}

				if(clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds+lastAgingTime)
				{
					clientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds+lastAgingTime;
				}

			}

		}

	}


	if((groupEntry!=NULL) && (groupEntry->clientList==NULL))
		{
			if(ipVersion==IP_VERSION4)
			{
				rtk_igmp_deleteGroupEntry(pPkthdr->moduleIndex,groupEntry,rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable);
			}
			else
			{
				rtk_igmp_deleteGroupEntry(pPkthdr->moduleIndex,groupEntry,rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable);
			}
		}


	if(_fastLeave)
	{
		rtk_igmp_flow_reflush_update(FAIL,groupAddress);
	}

	return RTK_IGMP_NF_ACCEPT;
}

static  int32 rtk_igmp_processIsInclude(rtk_igmp_pktHdr_t *pPkthdr, uint32 ipVersion, uint32 *clientAddr, uint8 *pktBuf)
{
	rtk_igmp_nf_ret_e_t ret;
	uint32 j=0;
	uint32 groupAddress[4]={0, 0, 0, 0};
	struct rtk_igmp_groupEntry* groupEntry=NULL;
	struct rtk_igmp_groupEntry* newGroupEntry=NULL;
	struct rtk_igmp_clientEntry* clientEntry=NULL;
	struct rtk_igmp_clientEntry* newClientEntry=NULL;
	struct rtk_igmp_sourceEntry *newSourceEntry=NULL;

	uint32 hashIndex=0;
	uint32 clientAgingTime=0;
	uint16 numOfSrc=0;
	uint32 *p_NsourceAddr=NULL;				//Net endian
	uint32 _HsourceAddr[4]={0, 0, 0, 0};	//Host endian
	uint32 _fastLeave=0;

	if(ipVersion==IP_VERSION4)
	{
		IGMP_CTRL("process IGMPv3-IsInclude");
		_fastLeave = pPkthdr->devInfo->devConf.igmp.igmpfastLeave;
		groupAddress[0]=ntohl(((struct groupRecord *)pktBuf)->groupAddr);
		numOfSrc=ntohs(((struct groupRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct groupRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp.igmpMcMemberAgingInterval;

	}
	else
	{
		IGMP_CTRL("process MLDv2-IsInclude");
		_fastLeave = pPkthdr->devInfo->devConf.igmp6.igmp6fastLeave;
		memcpy(groupAddress,((struct mCastAddrRecord *)pktBuf)->mCastAddr,IPV6ADDRLEN);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp6.igmp6McMemberAgingInterval;
		numOfSrc=ntohs(((struct mCastAddrRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct mCastAddrRecord *)pktBuf)->srcList);
	}
	if((ipVersion==IP_VERSION4 && IN_MULTICAST_RESV_IPV4_ALL(groupAddress[0])) ||
		(ipVersion==IP_VERSION6 && IN_MULTICAST_RESV_IPV6_ALL(groupAddress[0])))
	{
		IGMP_CTRL("IGNORE RESERVED ADDRESS");
		return RTK_IGMP_NF_CONTINUE;
	}

	ret=rtk_igmp_group_limitCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL,0,pPkthdr->devIfindex,RTK_IGMP_NF_DROP);
	if(ret!=RTK_IGMP_NF_CONTINUE)
	{
		IGMP_CTRL("Limit check FAIL Ingore Snooping learning ret=%d",ret);
		return ret;
	}

	if(numOfSrc==1)
	{
		if(ipVersion==IP_VERSION4)
			_HsourceAddr[0] = ntohl(*p_NsourceAddr);
		else
			memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));
		
		if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,_HsourceAddr,NULL)!= SUCCESS)
		{
			IGMP_CTRL("Ingore Learning By White List check DROP");
			return RTK_IGMP_NF_DROP;
		}
	}
	else
	{
		if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL)!= SUCCESS)
		{
			IGMP_CTRL("Ingore Learning By White List check  DROP");
			return RTK_IGMP_NF_DROP;
		}
	}
	
	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddress);

	groupEntry=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex, ipVersion, groupAddress, 0);

	if (groupEntry==NULL)   /*means new group address, create new group entry*/
	{
		IGMP_CTRL("New GIP");
		newGroupEntry=rtk_igmp_allocateGroupEntry();
		if(newGroupEntry==NULL)
		{
			IGMP_CTRL("run out of group entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		/*set new multicast entry*/

		newGroupEntry->groupAddr[0]=groupAddress[0];
		newGroupEntry->groupAddr[1]=groupAddress[1];
		newGroupEntry->groupAddr[2]=groupAddress[2];
		newGroupEntry->groupAddr[3]=groupAddress[3];

		newGroupEntry->ipVersion=ipVersion;
		newGroupEntry->vlanId=0;

		if(ipVersion==IP_VERSION4)
		{
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable);
		}
		else
		{
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable);
		}

		groupEntry=newGroupEntry;
	}

	/*from here groupEntry is the same as newGroupEntry*/
	clientEntry=rtk_igmp_searchClientEntry(ipVersion, groupEntry, pPkthdr->devIfindex, clientAddr);
	if (clientEntry==NULL)
	{
		IGMP_CTRL("New GIP.Clt");
		newClientEntry=rtk_igmp_allocateClientEntry();
		if (newClientEntry==NULL)
		{
			IGMP_CTRL("run out of client entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		newClientEntry->sourceList=NULL;
		newClientEntry->igmpVersion=IGMP_V3;
		newClientEntry->inIfidx=pPkthdr->devIfindex;

		if(ipVersion==IP_VERSION4)
		{
			newClientEntry->clientAddr[0]=clientAddr[0];
		}
		else
		{
			newClientEntry->igmpVersion=MLD_V2;
			newClientEntry->clientAddr[0]=clientAddr[0];
			newClientEntry->clientAddr[1]=clientAddr[1];
			newClientEntry->clientAddr[2]=clientAddr[2];
			newClientEntry->clientAddr[3]=clientAddr[3];
		}

		rtk_igmp_linkClientEntry(groupEntry, newClientEntry);
		clientEntry=newClientEntry;
		clientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds; ////
	}

	if(pPkthdr->ingressCtagif )
	{
		clientEntry->reportCtagif = 1;
		clientEntry->reportVlanId = pPkthdr->ingressCvid;
	}


	if (ipVersion==IP_VERSION4)
		clientEntry->igmpVersion=IGMP_V3;
	else //if (ipVersion==IP_VERSION6)
		clientEntry->igmpVersion=MLD_V2;


	IGMP_CTRL("GIP.Clt");
	/*here to handle the source list*/

	if (_fastLeave)
	{
		IGMP_CTRL("enableFastLeave==TRUE");
		clientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds;
		rtk_igmp_deleteSourceList(clientEntry);
		/*link the new source list*/
		for (j=0; j<numOfSrc; j++)
		{

			if(ipVersion==IP_VERSION4)
				_HsourceAddr[0] = ntohl(*p_NsourceAddr);
			else
				memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));

			newSourceEntry=rtk_igmp_allocateSourceEntry();
			if (newSourceEntry==NULL)
			{
				IGMP_CTRL("run out of source entry!\n");
				return RTK_IGMP_NF_DROP;
			}

			if (ipVersion==IP_VERSION4)
			{
				newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
				p_NsourceAddr++;
			}
			else
			{
				newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
				newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
				newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
				newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
				p_NsourceAddr=p_NsourceAddr+4;
			}
			newSourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
			rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
		}

	}
	else
	{

		for (j=0; j<numOfSrc; j++)
		{
			if(ipVersion==IP_VERSION4)
				_HsourceAddr[0] = ntohl(*p_NsourceAddr);
			else
				memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));

			newSourceEntry=rtk_igmp_searchSourceEntry(ipVersion, _HsourceAddr, clientEntry);
			if (clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds)
			{
				if (newSourceEntry!=NULL) //had recorded source-ip
				{

					newSourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
				}
				else
				{
					IGMP_CTRL("New GIP.Clt.Src(%d)",j);
					newSourceEntry=rtk_igmp_allocateSourceEntry();
					if (newSourceEntry==NULL)
					{
						IGMP_CTRL("run out of source entry!\n");
						return RTK_IGMP_NF_DROP;
					}

					if (ipVersion==IP_VERSION4)
					{
						newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
						IGMP_CTRL("SrcList[%d]=%pI4h" , j, (newSourceEntry->sourceAddr));
					}
					else
					{
						newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
						newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
						newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
						newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
					}
					rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
					newSourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
				}
			}
			else
			{
				if (newSourceEntry==NULL)
				{
					IGMP_CTRL("New GIP.Clt.Src(%d)",j);
					newSourceEntry=rtk_igmp_allocateSourceEntry();
					if (newSourceEntry==NULL)
					{
						IGMP_CTRL("run out of source entry!\n");
						return RTK_IGMP_NF_DROP;
					}

					if (ipVersion==IP_VERSION4)
					{
						newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
						IGMP_CTRL("SrcList[%d]=%pI4h"  , j, (newSourceEntry->sourceAddr));
					}
					else
					{
						newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
						newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
						newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
						newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
					}

					rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
				}
				newSourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
			}
			if (ipVersion==IP_VERSION4)
				p_NsourceAddr++;
			else
				p_NsourceAddr=p_NsourceAddr+4;

		} ////: considerate per M-SIP

	}


	rtk_igmp_flow_reflush_update(FAIL,groupAddress);
	

	return RTK_IGMP_NF_CONTINUE;
}

static  int32 rtk_igmp_processIsExclude(rtk_igmp_pktHdr_t *pPkthdr, uint32 ipVersion, uint32 *clientAddr, uint8 *pktBuf)
{
	rtk_igmp_nf_ret_e_t ret;
	uint32 j=0;
	uint32 groupAddress[4]={0, 0, 0, 0};
	struct rtk_igmp_groupEntry* groupEntry=NULL;
	struct rtk_igmp_groupEntry* newGroupEntry=NULL;
	struct rtk_igmp_clientEntry* clientEntry=NULL;
	struct rtk_igmp_clientEntry* newClientEntry=NULL;
	struct rtk_igmp_sourceEntry *newSourceEntry=NULL;

	uint32 hashIndex=0;
	uint16 numOfSrc=0;
	uint32 *p_NsourceAddr=NULL;				//Net endian
	uint32 _HsourceAddr[4]={0, 0, 0, 0};	//Host endian
	uint32 clientAgingTime=0;


	if(ipVersion==IP_VERSION4)
	{
		groupAddress[0]=ntohl(((struct groupRecord *)pktBuf)->groupAddr);
		numOfSrc=ntohs(((struct groupRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct groupRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp.igmpMcMemberAgingInterval;
		IGMP_CTRL("process IGMPv3-IsExclude");
	}
	else
	{

		memcpy(groupAddress,((struct mCastAddrRecord *)pktBuf)->mCastAddr,IPV6ADDRLEN);
		numOfSrc=ntohs(((struct mCastAddrRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct mCastAddrRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp6.igmp6McMemberAgingInterval;
		IGMP_CTRL("process MLDv2-IsExclude %pI6",groupAddress);
	}
	if((ipVersion==IP_VERSION4 && IN_MULTICAST_RESV_IPV4_ALL(groupAddress[0])) ||
		(ipVersion==IP_VERSION6 && IN_MULTICAST_RESV_IPV6_ALL(groupAddress[0])))
	{
		IGMP_CTRL("IGNORE RESERVED ADDRESS");
		return RTK_IGMP_NF_CONTINUE;
	}

	ret = rtk_igmp_group_limitCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL,0,pPkthdr->devIfindex,RTK_IGMP_NF_DROP);
	if(ret!=RTK_IGMP_NF_CONTINUE)
	{
		IGMP_CTRL("Limit check FAIL Ingore Snooping learning ret=%d",ret);
		return ret;
	}
	

	if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL)!= SUCCESS)
	{
		IGMP_CTRL("Ingore Learning By White List check  DROP");
		return RTK_IGMP_NF_DROP;
	}



	hashIndex=rtk_igmp_hashAlgorithm( ipVersion, groupAddress);

	groupEntry=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex, ipVersion, groupAddress, 0);
	if (groupEntry==NULL)   /*means new group address, create new group entry*/
	{
		newGroupEntry=rtk_igmp_allocateGroupEntry();
		if (newGroupEntry==NULL)
		{
			IGMP_CTRL("run out of group entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		/*set new multicast entry*/
		newGroupEntry->vlanId=0;
		newGroupEntry->ipVersion=ipVersion;
		if (ipVersion==IP_VERSION4)
		{
			newGroupEntry->groupAddr[0]=groupAddress[0];
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable);
		}
		else
		{
			newGroupEntry->groupAddr[0]=groupAddress[0];
			newGroupEntry->groupAddr[1]=groupAddress[1];
			newGroupEntry->groupAddr[2]=groupAddress[2];
			newGroupEntry->groupAddr[3]=groupAddress[3];
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable);
		}

		groupEntry=newGroupEntry;
	}

	/*from here groupEntry is the same as  newGroupEntry*/
	clientEntry=rtk_igmp_searchClientEntry(ipVersion, groupEntry, pPkthdr->devIfindex, clientAddr);
	if (clientEntry==NULL)
	{
		newClientEntry=rtk_igmp_allocateClientEntry();
		if (newClientEntry==NULL)
		{
			IGMP_CTRL("run out of client entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		newClientEntry->sourceList=NULL;
		newClientEntry->igmpVersion=IGMP_V3;
		newClientEntry->inIfidx=pPkthdr->devIfindex;

		if(ipVersion==IP_VERSION4)
		{
			newClientEntry->clientAddr[0]=clientAddr[0];
		}
		else
		{
			newClientEntry->igmpVersion=MLD_V2;
			newClientEntry->clientAddr[0]=clientAddr[0];
			newClientEntry->clientAddr[1]=clientAddr[1];
			newClientEntry->clientAddr[2]=clientAddr[2];
			newClientEntry->clientAddr[3]=clientAddr[3];
		}

		rtk_igmp_linkClientEntry(groupEntry, newClientEntry);
		clientEntry=newClientEntry;
	}

	if(pPkthdr->ingressCtagif )
	{
		clientEntry->reportCtagif = 1;
		clientEntry->reportVlanId = pPkthdr->ingressCvid;
	}


	/*from here clientEntry  is the same as newClientEntry*/
	if (ipVersion==IP_VERSION4)
		clientEntry->igmpVersion=IGMP_V3;
	else //if (ipVersion==IP_VERSION6)
		clientEntry->igmpVersion=MLD_V2;


	/*flush the old source list*/
	// delete ((X-A) | (Y-A)) or (A-B)
	{
		struct rtk_igmp_sourceEntry *sourceEntry=clientEntry->sourceList;
		while (sourceEntry!=NULL)
		{
			sourceEntry->setOpFlag=1;
			sourceEntry=sourceEntry->next;
		}
	}


	/*link the new source list*/
	for (j=0; j<numOfSrc; j++)
	{
		if(ipVersion==IP_VERSION4)
			_HsourceAddr[0] = ntohl(*p_NsourceAddr);
		else
			memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));
		newSourceEntry=rtk_igmp_searchSourceEntry(ipVersion, _HsourceAddr, clientEntry);
		if (clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds) //ex-mo
		{
			if (newSourceEntry!=NULL) // A*X or A*Y
			{
				if (newSourceEntry->portTimer <= rtk_igmp_sysUpSeconds) // A*Y
				{
					//rtk_igmp_deleteSourceEntry(clientEntry,newSourceEntry);
					newSourceEntry->portTimer=rtk_igmp_sysUpSeconds;
				}
				else //A*X
				{
				}
				newSourceEntry->setOpFlag=0;
			}
			else //A-X-Y = GMI
			{
				IGMP_CTRL("New GIP.Clt.Src(%d)",j);
				newSourceEntry=rtk_igmp_allocateSourceEntry();
				if (newSourceEntry==NULL)
				{
					IGMP_CTRL("run out of source entry!\n");
					return RTK_IGMP_NF_DROP;
				}

				if (ipVersion==IP_VERSION4)
				{
					newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
					IGMP_CTRL("SrcList[%d]=%pI4h"  , j, (newSourceEntry->sourceAddr));
				}
				else
				{
					newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
					newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
					newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
					newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
				}
				rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
				newSourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
				newSourceEntry->setOpFlag=0;
			}
		}
		else
		{
			if (newSourceEntry==NULL) //B-A
			{
				IGMP_CTRL("New GIP.Clt.Src(Record index:%d)",j);
				newSourceEntry=rtk_igmp_allocateSourceEntry();
				if (newSourceEntry==NULL)
				{
					IGMP_CTRL("run out of source entry!\n");
					return RTK_IGMP_NF_DROP;
				}

				if (ipVersion==IP_VERSION4)
				{
					newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
					IGMP_CTRL("SrcList[%d]=%pI4h"  , j, (newSourceEntry->sourceAddr));
				}
				else
				{
					newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
					newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
					newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
					newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
				}
				rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
				newSourceEntry->portTimer=rtk_igmp_sysUpSeconds;
			}
			else //B*A
			{
			}
			newSourceEntry->setOpFlag=0;
		}
		if (ipVersion==IP_VERSION4)
			p_NsourceAddr++;
		else
			p_NsourceAddr=p_NsourceAddr+4;

	} ////: considerate per M-SIP

	// delete ((X-A) | (Y-A)) or (A-B)
	{
		struct rtk_igmp_sourceEntry *sourceEntry=clientEntry->sourceList;
		struct rtk_igmp_sourceEntry *nextSourceEntry;
		while (sourceEntry!=NULL)
		{
			nextSourceEntry=sourceEntry->next;
			if (sourceEntry->setOpFlag)
			{
				rtk_igmp_deleteSourceEntry(clientEntry, sourceEntry);
			}
			sourceEntry=nextSourceEntry;
		}
	}

	clientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds+clientAgingTime;

	rtk_igmp_flow_reflush_update(FAIL,groupAddress);

	//} ////: considerate per M-SIP

	return RTK_IGMP_NF_CONTINUE;

}

static int32 rtk_igmp_processToInclude(rtk_igmp_pktHdr_t *pPkthdr, uint32 ipVersion, uint32 *clientAddr, uint8 *pktBuf)
{
	rtk_igmp_nf_ret_e_t ret;
	uint32 j=0;
	uint32 groupAddress[4]={0, 0, 0, 0};
	struct rtk_igmp_groupEntry* groupEntry=NULL;
	struct rtk_igmp_groupEntry* newGroupEntry=NULL;
	struct rtk_igmp_clientEntry* clientEntry=NULL;
	struct rtk_igmp_clientEntry* newClientEntry=NULL;
	struct rtk_igmp_sourceEntry *sourceEntry=NULL;
	//struct rtk_igmp_sourceEntry *nextSourceEntry=NULL;
	struct rtk_igmp_sourceEntry *newSourceEntry=NULL;

	uint32 _fastLeave=0;
	uint32 hashIndex=0;
	uint16 numOfSrc=0;
	uint32 *p_NsourceAddr=NULL; 			//Net endian
	uint32 _HsourceAddr[4]={0, 0, 0, 0};	//Host endian
	uint32 clientAgingTime=0;
	uint32 lastAgingTime=0;

	uint16 numOfQuerySrc=0;

	if (ipVersion==IP_VERSION4)
	{
		IGMP_CTRL("process IGMPv3-ToInclude");
		_fastLeave = pPkthdr->devInfo->devConf.igmp.igmpfastLeave;
		groupAddress[0]=ntohl(((struct groupRecord *)pktBuf)->groupAddr);
		numOfSrc=ntohs(((struct groupRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct groupRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp.igmpMcMemberAgingInterval;
		lastAgingTime = pPkthdr->devInfo->devConf.igmp.igmpMcLastMemberAgingInterval;
	}
	else
	{
		IGMP_CTRL("process MLDv2-ToInclude");
		_fastLeave = pPkthdr->devInfo->devConf.igmp6.igmp6fastLeave;
		memcpy(groupAddress,((struct mCastAddrRecord *)pktBuf)->mCastAddr,IPV6ADDRLEN);
		numOfSrc=ntohs(((struct mCastAddrRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct mCastAddrRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp6.igmp6McMemberAgingInterval;
		lastAgingTime = pPkthdr->devInfo->devConf.igmp6.igmp6McLastMemberAgingInterval;
	}

	if((ipVersion==IP_VERSION4 && IN_MULTICAST_RESV_IPV4_ALL(groupAddress[0])) ||
		(ipVersion==IP_VERSION6 && IN_MULTICAST_RESV_IPV6_ALL(groupAddress[0])))
	{
		IGMP_CTRL("IGNORE RESERVED ADDRESS");
		return RTK_IGMP_NF_CONTINUE;
	}

	ret=rtk_igmp_group_limitCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL,0,pPkthdr->devIfindex,RTK_IGMP_NF_DROP);
	if(ret!=RTK_IGMP_NF_CONTINUE)
	{
		IGMP_CTRL("Limit check FAIL Ingore Snooping learning ret=%d",ret);
		return ret;
	}

	if(numOfSrc==1)
	{
		if(ipVersion==IP_VERSION4)
			_HsourceAddr[0] = ntohl(*p_NsourceAddr);
		else
			memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));
		
		if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,_HsourceAddr,NULL)!= SUCCESS)
		{
			IGMP_CTRL("Ingore Learning By White List check DROP");
			return RTK_IGMP_NF_DROP;
		}
	}
	else
	{
		if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL)!= SUCCESS)
		{
			IGMP_CTRL("Ingore Learning By White List check  DROP");
			return RTK_IGMP_NF_DROP;
		}
	}
	
	if (numOfSrc==0)
		IGMP_CTRL("UPDATE ALL Source timer for Leave TO_IN{0}");

	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddress);

	groupEntry=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex, ipVersion, groupAddress, 0);
	if (groupEntry==NULL)	/*means new group address, create new group entry*/
	{
		newGroupEntry=rtk_igmp_allocateGroupEntry();
		if (newGroupEntry==NULL)
		{
			IGMP_CTRL("run out of group entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		newGroupEntry->vlanId=0;
		newGroupEntry->ipVersion=ipVersion;
		if (ipVersion==IP_VERSION4)
		{
			newGroupEntry->groupAddr[0]=groupAddress[0];
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable);
		}
		else
		{
			newGroupEntry->groupAddr[0]=groupAddress[0];
			newGroupEntry->groupAddr[1]=groupAddress[1];
			newGroupEntry->groupAddr[2]=groupAddress[2];
			newGroupEntry->groupAddr[3]=groupAddress[3];
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable);
		}

		groupEntry=newGroupEntry;
	}

	/*from here groupEntry is the same as newGroupEntry*/
	clientEntry=rtk_igmp_searchClientEntry(ipVersion, groupEntry, pPkthdr->devIfindex, clientAddr);
	if (clientEntry==NULL)
	{
		newClientEntry=rtk_igmp_allocateClientEntry();
		if (newClientEntry==NULL)
		{
			IGMP_CTRL("run out of client entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		newClientEntry->sourceList=NULL;
		newClientEntry->inIfidx=pPkthdr->devIfindex;
		if (ipVersion==IP_VERSION4)
		{
			newClientEntry->igmpVersion=IGMP_V3;
			newClientEntry->clientAddr[0]=clientAddr[0];
		}
		else
		{
			newClientEntry->igmpVersion=MLD_V2;
			newClientEntry->clientAddr[0]=clientAddr[0];
			newClientEntry->clientAddr[1]=clientAddr[1];
			newClientEntry->clientAddr[2]=clientAddr[2];
			newClientEntry->clientAddr[3]=clientAddr[3];
		}

		rtk_igmp_linkClientEntry(groupEntry, newClientEntry);
		clientEntry=newClientEntry;
	}

	if(pPkthdr->ingressCtagif )
	{
		clientEntry->reportCtagif = 1;
		clientEntry->reportVlanId = pPkthdr->ingressCvid;
	}


	/*here to handle the source list*/
	if (ipVersion==IP_VERSION4)
		clientEntry->igmpVersion=IGMP_V3;
	else //if (ipVersion==IP_VERSION6)
		clientEntry->igmpVersion=MLD_V2;


	if (_fastLeave)
	{
		IGMP_CTRL("enableFastLeave==TRUE");
		clientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds;
		rtk_igmp_deleteSourceList(clientEntry);
		/*link the new source list*/
		for (j=0; j<numOfSrc; j++)
		{

			if(ipVersion==IP_VERSION4)
				_HsourceAddr[0] = ntohl(*p_NsourceAddr);
			else
				memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));
			
			newSourceEntry=rtk_igmp_allocateSourceEntry();
			if (newSourceEntry==NULL)
			{
				IGMP_CTRL("run out of source entry!\n");
				return RTK_IGMP_NF_DROP;
			}

			if (ipVersion==IP_VERSION4)
			{
				newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
				p_NsourceAddr++;
			}
			else
			{
				newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
				newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
				newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
				newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
				p_NsourceAddr=p_NsourceAddr+4;
			}
			newSourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
			rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
		}

	}
	else
	{

		// query in-mo for Q(A-B)
		// query ex-mo for Q(G,X-A) & Q(G)
		//struct rtk_igmp_sourceEntry *sourceEntry=clientEntry->sourceList;
		sourceEntry=clientEntry->sourceList;
		while (sourceEntry!=NULL)
		{
			if (sourceEntry->portTimer>rtk_igmp_sysUpSeconds) //ex-mo X or in-mo A
			{
				sourceEntry->setOpFlag=1;
				numOfQuerySrc++;
			}
			else
				sourceEntry->setOpFlag=0;

			//sync all source timer to rtk_igmp_sysUpSeconds+rtk_igmp_mCastTimerParas.lastMemberAgingTimel
			if(sourceEntry->portTimer >rtk_igmp_sysUpSeconds+lastAgingTime)
			{
				sourceEntry->portTimer = rtk_igmp_sysUpSeconds+lastAgingTime;
			}
			
			sourceEntry=sourceEntry->next;
		}

		IGMP_CTRL("numOfQuerySrc=%d, numOfSrc=%d", numOfQuerySrc, numOfSrc);


		/*add new source list*/
		for (j=0; j<numOfSrc; j++)
		{
			if(ipVersion==IP_VERSION4)
				_HsourceAddr[0] = ntohl(*p_NsourceAddr);
			else
				memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));

			sourceEntry=rtk_igmp_searchSourceEntry(ipVersion, _HsourceAddr, clientEntry);
			if (sourceEntry!=NULL)
			{
				sourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
				if (sourceEntry->setOpFlag)
				{
					numOfQuerySrc--;
					sourceEntry->setOpFlag=0;
				}
			}
			else
			{
				newSourceEntry=rtk_igmp_allocateSourceEntry();
				if (newSourceEntry==NULL)
				{
					IGMP_CTRL("run out of source entry!\n");
					return RTK_IGMP_NF_DROP;
				}

				if (ipVersion==IP_VERSION4)
				{
					newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
				}
				else
				{
					newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
					newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
					newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
					newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
				}

				newSourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;


				newSourceEntry->setOpFlag=0;
				rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
			}

			if (ipVersion==IP_VERSION4)
			{
				p_NsourceAddr++;
			}
			else
			{
				p_NsourceAddr=p_NsourceAddr+4;
			}
		}

		if (clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds+lastAgingTime) //ex-mo
		{
			clientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds+lastAgingTime;
			IGMP_CTRL("Shrink ex-mo client to lastMemberAgingTime=%d groupFilterTimer=%d rtk_igmp_sysUpSeconds=%d", lastAgingTime,clientEntry->groupFilterTimer,rtk_igmp_sysUpSeconds);
		}
#if 0
		// query in-mo for Q(G,A-B)
		// query ex-mo for Q(G,X-A) & Q(G)
		IGMP_CTRL("Group-Source Specific Query");
		rtk_igmp_igmpQueryTimerExpired(0, IGMP_V3, (char *)groupAddress, numOfQuerySrc, clientEntry->sourceList, NULL);
		if (clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds && numOfQuerySrc!=0) //ex-mo
		{
			IGMP_CTRL("Group Specific Query");
			rtk_igmp_igmpQueryTimerExpired(0, IGMP_V3, (char *)groupAddress, 0, NULL, NULL);
		}
#endif

	}

		rtk_igmp_flow_reflush_update(FAIL,groupAddress);


	return RTK_IGMP_NF_CONTINUE;
}

static  int32 rtk_igmp_processToExclude(rtk_igmp_pktHdr_t *pPkthdr, uint32 ipVersion, uint32 *clientAddr, uint8 *pktBuf)
{
	rtk_igmp_nf_ret_e_t ret;
	uint32 j=0;
	uint32 groupAddress[4]={0, 0, 0, 0};
	struct rtk_igmp_groupEntry* groupEntry=NULL;
	struct rtk_igmp_groupEntry* newGroupEntry=NULL;
	struct rtk_igmp_clientEntry* clientEntry=NULL;
	struct rtk_igmp_clientEntry* newClientEntry=NULL;
	struct rtk_igmp_sourceEntry *sourceEntry=NULL;
	struct rtk_igmp_sourceEntry *newSourceEntry=NULL;

	uint32 hashIndex=0;
	uint16 numOfSrc=0;
	uint32 *p_NsourceAddr=NULL;				//Net endian
	uint32 _HsourceAddr[4]={0, 0, 0, 0};	//Host endian
	uint32 tmpAddress[4]={0, 0, 0, 0};
	uint16 numOfQuerySrc=0;
	uint32 clientAgingTime=0;

	if (ipVersion==IP_VERSION4)
	{
		IGMP_CTRL("process IGMPv3-ToExclude");
		groupAddress[0]=ntohl(((struct groupRecord *)pktBuf)->groupAddr);
		numOfSrc=ntohs(((struct groupRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct groupRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp.igmpMcMemberAgingInterval;
	}
	else
	{
		IGMP_CTRL("process MLDv2-ToExclude");
		memcpy(groupAddress,((struct mCastAddrRecord *)pktBuf)->mCastAddr,IPV6ADDRLEN);
		numOfSrc=ntohs(((struct mCastAddrRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct mCastAddrRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp6.igmp6McMemberAgingInterval;
	}
	if((ipVersion==IP_VERSION4 && IN_MULTICAST_RESV_IPV4_ALL(groupAddress[0])) ||
		(ipVersion==IP_VERSION6 && IN_MULTICAST_RESV_IPV6_ALL(groupAddress[0])))
	{
		IGMP_CTRL("IGNORE RESERVED ADDRESS");
		return RTK_IGMP_NF_CONTINUE;
	}

	ret=rtk_igmp_group_limitCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL,0,pPkthdr->devIfindex,RTK_IGMP_NF_DROP);
	if(ret!=RTK_IGMP_NF_CONTINUE)
	{
		IGMP_CTRL("Limit check FAIL Ingore Snooping learning ret=%d",ret);
		return ret;
	}

	if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL)!= SUCCESS)
	{
		IGMP_CTRL("Ingore Learning By White List check  DROP");
		return RTK_IGMP_NF_DROP;
	}



	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddress);

	groupEntry=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex, ipVersion, groupAddress, 0);
	if (groupEntry==NULL)   /*means new group address, create new group entry*/
	{
		newGroupEntry=rtk_igmp_allocateGroupEntry();
		if(newGroupEntry==NULL)
		{
			IGMP_CTRL("run out of group entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		newGroupEntry->vlanId=0;
		newGroupEntry->ipVersion=ipVersion;
		if (ipVersion==IP_VERSION4)
		{
			newGroupEntry->groupAddr[0]=groupAddress[0];
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable);
		}
		else
		{
			newGroupEntry->groupAddr[0]=groupAddress[0];
			newGroupEntry->groupAddr[1]=groupAddress[1];
			newGroupEntry->groupAddr[2]=groupAddress[2];
			newGroupEntry->groupAddr[3]=groupAddress[3];
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable);
		}

		groupEntry=newGroupEntry;
	}

	clientEntry=rtk_igmp_searchClientEntry(ipVersion, groupEntry, pPkthdr->devIfindex, clientAddr);
	if (clientEntry==NULL)
	{
		newClientEntry=rtk_igmp_allocateClientEntry();
		if (newClientEntry==NULL)
		{
			IGMP_CTRL("run out of client entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		newClientEntry->sourceList=NULL;
		newClientEntry->inIfidx=pPkthdr->devIfindex;

		if (ipVersion==IP_VERSION4)
		{
			newClientEntry->igmpVersion=IGMP_V3;
			newClientEntry->clientAddr[0]=clientAddr[0];
		}
		else
		{
			newClientEntry->igmpVersion=MLD_V2;
			newClientEntry->clientAddr[0]=clientAddr[0];
			newClientEntry->clientAddr[1]=clientAddr[1];
			newClientEntry->clientAddr[2]=clientAddr[2];
			newClientEntry->clientAddr[3]=clientAddr[3];
		}

		rtk_igmp_linkClientEntry(groupEntry, newClientEntry);
		clientEntry=newClientEntry;
	}

	if(pPkthdr->ingressCtagif )
	{
		clientEntry->reportCtagif = 1;
		clientEntry->reportVlanId = pPkthdr->ingressCvid;
	}



	/*flush the old source list*/

	// delete ((X-A) | (Y-A)) or (A-B)
	sourceEntry=clientEntry->sourceList;
	while (sourceEntry!=NULL)
	{
		sourceEntry->setOpFlag=1;
		sourceEntry=sourceEntry->next;
	}

	if (ipVersion==IP_VERSION4)
		clientEntry->igmpVersion=IGMP_V3;
	else //if(ipVersion==IP_VERSION6)
		clientEntry->igmpVersion=MLD_V2;

	/*link the new source list*/
	for (j=0; j<numOfSrc; j++)
	{
		if(ipVersion==IP_VERSION4)
			_HsourceAddr[0] = ntohl(*p_NsourceAddr);
		else
			memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));

		sourceEntry=rtk_igmp_searchSourceEntry(ipVersion, _HsourceAddr, clientEntry);
		if (clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds) //ex-mo
		{
			if (sourceEntry!=NULL) // A*X + A*Y
			{
				sourceEntry->setOpFlag=0;
				if (sourceEntry->portTimer>rtk_igmp_sysUpSeconds) //A*X
				{
				}
				else
				{
					if (ipVersion==IP_VERSION4)
						p_NsourceAddr++;
					else
						p_NsourceAddr=p_NsourceAddr+4;

					continue;
				}
			}
			else	// A-X-Y
			{
				newSourceEntry=rtk_igmp_allocateSourceEntry();
				if (newSourceEntry==NULL)
				{
					IGMP_CTRL("run out of source entry!\n");
					return RTK_IGMP_NF_DROP;
				}

				if (ipVersion==IP_VERSION4)
				{
					newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
				}
				else //if (ipVersion==IP_VERSION4)
				{
					newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
					newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
					newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
					newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
				}

				/*time out the sources included in the MODE_IS_EXCLUDE report*/
				newSourceEntry->portTimer=rtk_igmp_sysUpSeconds;
				rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);

				newSourceEntry->portTimer=clientEntry->groupFilterTimer;
				newSourceEntry->setOpFlag=0;
			}

			if (ipVersion==IP_VERSION4)
			{
				//querySourceAddr
				IGMP_CTRL("Block.GIP.Clt.SrcIP(%pI4n) in A-Y",(p_NsourceAddr));////

				*tmpAddress = *p_NsourceAddr;
				*p_NsourceAddr = *(p_NsourceAddr + (numOfSrc-j-1));
				*(p_NsourceAddr + (numOfSrc-j-1)) = *tmpAddress;
				IGMP_CTRL("sourceAddr Head(%pI4n) Tail(%pI4n)",(p_NsourceAddr), ((p_NsourceAddr+(numOfSrc-j-1))));
			}
			else
			{
				int i ;
				IGMP_CTRL("Block.GIP6.Clt.SrcIP6(%pI6) in A-Y",(p_NsourceAddr));////

				for (i=0; i<4; i++)
				{
					*(tmpAddress+i) = *(p_NsourceAddr+i);
					*(p_NsourceAddr+i) = *(p_NsourceAddr+i + ((numOfSrc -j - 1)));
					*(p_NsourceAddr+i + ((numOfSrc - j - 1))) = *(tmpAddress+i);
				}
			}


			j--;
			numOfSrc--;
			numOfQuerySrc++;
			continue;	//no need to move sourceAddr

		}
		else //in-mo
		{
			if (sourceEntry!=NULL) // A*B
			{
				sourceEntry->setOpFlag=0;

				if (ipVersion==IP_VERSION4)
				{
					//querySourceAddr
					IGMP_CTRL("To_Ex.GIP.Clt.SrcIP(%pI4n) in A*B",(p_NsourceAddr));////

					*tmpAddress = *p_NsourceAddr;
					*p_NsourceAddr = *(p_NsourceAddr + (numOfSrc-j-1));
					*(p_NsourceAddr + (numOfSrc-j-1)) = *tmpAddress;
					IGMP_CTRL("sourceAddr Head(%pI4n) Tail(%pI4n)",(p_NsourceAddr), ((p_NsourceAddr+(numOfSrc-j-1))));
				}
				else
				{
					int i ;
					IGMP_CTRL("To_Ex.GIP6.Clt.SrcIP6(%pI6) in A*B",(p_NsourceAddr));////

					for (i=0; i<4; i++)
					{
						*(tmpAddress+i) = *(p_NsourceAddr+i);
						*(p_NsourceAddr+i) = *(p_NsourceAddr+i + ((numOfSrc -j - 1)));
						*(p_NsourceAddr+i + ((numOfSrc - j - 1))) = *(tmpAddress+i);
					}
				}

				j--;
				numOfSrc--;
				numOfQuerySrc++;
				continue;	//no need to move sourceAddr

			}
			else //B-A
			{
				newSourceEntry=rtk_igmp_allocateSourceEntry();
				if (newSourceEntry==NULL)
				{
					IGMP_CTRL("run out of source entry!\n");
					return RTK_IGMP_NF_DROP;
				}

				if (ipVersion==IP_VERSION4)
				{
					newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
					p_NsourceAddr++;
				}
				else //if (ipVersion==IP_VERSION4)
				{
					newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
					newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
					newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
					newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
					p_NsourceAddr=p_NsourceAddr+4;
				}
				/*time out the sources included in the MODE_IS_EXCLUDE report*/
				newSourceEntry->portTimer=rtk_igmp_sysUpSeconds;
				rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
				newSourceEntry->setOpFlag=0;
			}

		}
	}

	// delete ((X-A) | (Y-A)) or (A-B)
	{
		struct rtk_igmp_sourceEntry *nextSourceEntry;
		sourceEntry=clientEntry->sourceList;
		while (sourceEntry!=NULL)
		{
			nextSourceEntry=sourceEntry->next;
			if (sourceEntry->setOpFlag)
			{
				rtk_igmp_deleteSourceEntry(clientEntry, sourceEntry);
			}
			sourceEntry=nextSourceEntry;
		}
	}

	clientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds+clientAgingTime;

#if 0
	if (ipVersion==IP_VERSION4)
	{
		int i ;
		for (i=0; i<numOfQuerySrc; i++)
		{
			IGMP_CTRL("Query GIP(%pI4h)-SrcIP(%pI4n)", (groupAddress), ((p_NsourceAddr+i)));
		}
		//IGMPQueryVersion=3;
		//rtk_igmp_igmpQueryTimerExpired(0, IGMP_V3, (char *)groupAddress, numOfQuerySrc, NULL, (char *)p_NsourceAddr);
	}
	else
	{
		int i ;
		for (i=0; i<numOfQuerySrc; i+=4)
		{
			IGMP_CTRL("Query GIP6(%pI6)-SrcIP(%pI6)", (groupAddress), ((p_NsourceAddr+i)));
		}
		//IGMPQueryVersion=3; ???????????????????????????????????????????????
		//rtk_igmp_mldQueryTimerExpired(0);
	}
#endif


	rtk_igmp_flow_reflush_update(FAIL,groupAddress);


	return RTK_IGMP_NF_CONTINUE;
}

static  int32 rtk_igmp_processAllow(rtk_igmp_pktHdr_t *pPkthdr, uint32 ipVersion, uint32 *clientAddr, uint8 *pktBuf)
{
	rtk_igmp_nf_ret_e_t ret;
	uint32 j=0;
	uint32 groupAddress[4]={0, 0, 0, 0};
	struct rtk_igmp_groupEntry* groupEntry=NULL;
	struct rtk_igmp_groupEntry* newGroupEntry=NULL;
	struct rtk_igmp_clientEntry* clientEntry=NULL;
	struct rtk_igmp_clientEntry* newClientEntry=NULL;
	struct rtk_igmp_sourceEntry *sourceEntry=NULL;
	struct rtk_igmp_sourceEntry *newSourceEntry=NULL;

	uint32 hashIndex=0;
	uint16 numOfSrc=0;
	uint32 *p_NsourceAddr=NULL;				//Net endian
	uint32 _HsourceAddr[4]={0, 0, 0, 0};	//Host endian
	uint32 clientAgingTime=0;

	if (ipVersion==IP_VERSION4)
	{
		IGMP_CTRL("process IGMPv3 Allow packet");
		groupAddress[0]=ntohl(((struct groupRecord *)pktBuf)->groupAddr);
		numOfSrc=ntohs(((struct groupRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct groupRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp.igmpMcMemberAgingInterval;

	}
	else
	{
		IGMP_CTRL("process MLDv2 Allow packet");
		memcpy(groupAddress,((struct mCastAddrRecord *)pktBuf)->mCastAddr,IPV6ADDRLEN);
		numOfSrc=ntohs(((struct mCastAddrRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct mCastAddrRecord *)pktBuf)->srcList);
		clientAgingTime = pPkthdr->devInfo->devConf.igmp6.igmp6McMemberAgingInterval;
	}
	if((ipVersion==IP_VERSION4 && IN_MULTICAST_RESV_IPV4_ALL(groupAddress[0])) ||
		(ipVersion==IP_VERSION6 && IN_MULTICAST_RESV_IPV6_ALL(groupAddress[0])))
	{
		IGMP_CTRL("IGNORE RESERVED ADDRESS");
		return RTK_IGMP_NF_CONTINUE;
	}

	ret = rtk_igmp_group_limitCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL,0,pPkthdr->devIfindex,RTK_IGMP_NF_DROP);
	if(ret!=RTK_IGMP_NF_CONTINUE)
	{
		IGMP_CTRL("Limit check FAIL Ingore Snooping learning ret=%d",ret);
		return ret;
	}
	if(numOfSrc==1)
	{
		if(ipVersion==IP_VERSION4)
			_HsourceAddr[0] = ntohl(*p_NsourceAddr);
		else
			memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));
		
		if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,_HsourceAddr,NULL)!= SUCCESS)
		{
			IGMP_CTRL("Ingore Learning By White List check DROP");
			return RTK_IGMP_NF_DROP;
		}
	}
	else
	{
		if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL)!= SUCCESS)
		{
			IGMP_CTRL("Ingore Learning By White List check  DROP");
			return RTK_IGMP_NF_DROP;
		}
	}

	hashIndex=rtk_igmp_hashAlgorithm( ipVersion, groupAddress);
	groupEntry=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex, ipVersion, groupAddress, 0);
	if (groupEntry==NULL)   /*means new group address, create new group entry*/
	{
		newGroupEntry=rtk_igmp_allocateGroupEntry();
		if (newGroupEntry==NULL)
		{
			IGMP_CTRL("run out of group entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		newGroupEntry->vlanId=0;
		newGroupEntry->ipVersion=ipVersion;
		if (ipVersion==IP_VERSION4)
		{
			newGroupEntry->groupAddr[0]=groupAddress[0];
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv4HashTable);
		}
		else
		{
			newGroupEntry->groupAddr[0]=groupAddress[0];
			newGroupEntry->groupAddr[1]=groupAddress[1];
			newGroupEntry->groupAddr[2]=groupAddress[2];
			newGroupEntry->groupAddr[3]=groupAddress[3];
			rtk_igmp_linkGroupEntry(newGroupEntry, rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].rtk_igmp_ipv6HashTable);
		}

		groupEntry=newGroupEntry;
	}

	clientEntry=rtk_igmp_searchClientEntry(ipVersion, groupEntry, pPkthdr->devIfindex, clientAddr);
	if (clientEntry==NULL)
	{
		newClientEntry=rtk_igmp_allocateClientEntry();
		if(newClientEntry==NULL)
		{
			IGMP_CTRL("run out of client entry!\n");
			return RTK_IGMP_NF_DROP;
		}

		//assert(newClientEntry->sourceList==NULL);
		newClientEntry->sourceList=NULL;
		newClientEntry->inIfidx=pPkthdr->devIfindex;

		newClientEntry->igmpVersion=IGMP_V3;
		newClientEntry->groupFilterTimer=rtk_igmp_sysUpSeconds;
		if(ipVersion==IP_VERSION4)
		{
			newClientEntry->clientAddr[0]=clientAddr[0];
		}
		else
		{
			newClientEntry->igmpVersion=MLD_V2;
			newClientEntry->clientAddr[0]=clientAddr[0];
			newClientEntry->clientAddr[1]=clientAddr[1];
			newClientEntry->clientAddr[2]=clientAddr[2];
			newClientEntry->clientAddr[3]=clientAddr[3];
		}

		rtk_igmp_linkClientEntry(groupEntry, newClientEntry);
		clientEntry=newClientEntry;
	}

	if(pPkthdr->ingressCtagif )
	{
		clientEntry->reportCtagif = 1;
		clientEntry->reportVlanId = pPkthdr->ingressCvid;
	}


	if (ipVersion==IP_VERSION4)
		clientEntry->igmpVersion=IGMP_V3;
	else	//if (ipVersion==IP_VERSION6)
		clientEntry->igmpVersion=MLD_V2;


	/*here to handle the source list*/
	for (j=0; j<numOfSrc; j++)
	{
		if(ipVersion==IP_VERSION4)
			_HsourceAddr[0] = ntohl(*p_NsourceAddr);
		else
			memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));

		sourceEntry=rtk_igmp_searchSourceEntry(ipVersion, _HsourceAddr, clientEntry);
		if (sourceEntry==NULL)
		{
			newSourceEntry=rtk_igmp_allocateSourceEntry();
			if (newSourceEntry==NULL)
			{
				IGMP_CTRL("run out of source entry!\n");
				return RTK_IGMP_NF_DROP;
			}

			if (ipVersion==IP_VERSION4)
			{
				newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
			}
			else
			{
				newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
				newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
				newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
				newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
			}
			newSourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
			rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
		}
		else
		{
			/*just update source timer*/
			sourceEntry->portTimer=rtk_igmp_sysUpSeconds+clientAgingTime;
		}

		if (ipVersion==IP_VERSION4)
			p_NsourceAddr++;
		else
			p_NsourceAddr=p_NsourceAddr+4;

	}


	rtk_igmp_flow_reflush_update(FAIL,groupAddress);

	return RTK_IGMP_NF_CONTINUE;
}

static int32 rtk_igmp_processBlock(rtk_igmp_pktHdr_t *pPkthdr, uint32 ipVersion, uint32 *clientAddr, uint8 *pktBuf)
{
	rtk_igmp_nf_ret_e_t ret;
	uint32 j=0;
	uint32 groupAddress[4]={0, 0, 0, 0};

	struct rtk_igmp_groupEntry* groupEntry=NULL;
	struct rtk_igmp_clientEntry* clientEntry=NULL;
	//struct rtk_igmp_clientEntry* newClientEntry=NULL;
	struct rtk_igmp_sourceEntry *sourceEntry=NULL;
	struct rtk_igmp_sourceEntry *newSourceEntry=NULL;

	uint32 hashIndex=0;
	uint16 numOfSrc=0;
	uint32 *p_NsourceAddr=NULL;				//Net endian
	uint32 _HsourceAddr[4]={0, 0, 0, 0};	//Host endian
	uint32 _fastLeave=0;

	uint16 numOfQuerySrc=0;
	//uint32 *querySourceAddr=NULL;
	uint32 tmpAddress[4]={0, 0, 0, 0};

	if (ipVersion==IP_VERSION4)
	{
		_fastLeave = pPkthdr->devInfo->devConf.igmp.igmpfastLeave;
		IGMP_CTRL("process IGMPv3-Block");
		groupAddress[0]=ntohl(((struct groupRecord *)pktBuf)->groupAddr);
		numOfSrc=ntohs(((struct groupRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct groupRecord *)pktBuf)->srcList);
	}
	else
	{
		IGMP_CTRL("process MLDv2-Block");
		_fastLeave = pPkthdr->devInfo->devConf.igmp6.igmp6fastLeave;
		memcpy(groupAddress,((struct mCastAddrRecord *)pktBuf)->mCastAddr,IPV6ADDRLEN);

		numOfSrc=ntohs(((struct mCastAddrRecord *)pktBuf)->numOfSrc);
		p_NsourceAddr=&(((struct mCastAddrRecord *)pktBuf)->srcList);
	}

	if((ipVersion==IP_VERSION4 && IN_MULTICAST_RESV_IPV4_ALL(groupAddress[0])) ||
		(ipVersion==IP_VERSION6 && IN_MULTICAST_RESV_IPV6_ALL(groupAddress[0])))
	{
		IGMP_CTRL("IGNORE RESERVED ADDRESS");
		return RTK_IGMP_NF_CONTINUE;
	}

	ret = rtk_igmp_group_limitCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL,0,pPkthdr->devIfindex,RTK_IGMP_NF_DROP);
	if(ret!=RTK_IGMP_NF_CONTINUE)
	{
		IGMP_CTRL("Limit check FAIL Ingore Snooping learning ret=%d",ret);
		return ret;
	}

	if(rtk_igmp_filterWhiteListCheck(pPkthdr,ipVersion,groupAddress,clientAddr,NULL,NULL)!= SUCCESS)
	{
		IGMP_CTRL("Ingore Learning By White List check  DROP");
		return RTK_IGMP_NF_DROP;
	}



	hashIndex=rtk_igmp_hashAlgorithm(ipVersion, groupAddress);

	groupEntry=rtk_igmp_searchGroupEntry(pPkthdr->moduleIndex, ipVersion, groupAddress, 0);
	if (groupEntry==NULL)
	{
		goto out;
	}

	clientEntry=rtk_igmp_searchClientEntry(ipVersion, groupEntry, pPkthdr->devIfindex, clientAddr);
	if (clientEntry==NULL)
	{
		goto out;
	}

	if (ipVersion==IP_VERSION4)
		clientEntry->igmpVersion=IGMP_V3;
	else //if (ipVersion==IP_VERSION6)
		clientEntry->igmpVersion=MLD_V2;


	if (clientEntry->groupFilterTimer>rtk_igmp_sysUpSeconds) /*means exclude mode*/
	{
		IGMP_CTRL("<Ex-Mo> GIP.Clt");
		numOfQuerySrc=0;
		for (j=0; j<numOfSrc; j++)
		{
			IGMP_CTRL("current j(%d), numOfSrc(%d), numOfQuerySrc(%d)", j, numOfSrc, numOfQuerySrc);////
			if(ipVersion==IP_VERSION4)
				_HsourceAddr[0] = ntohl(*p_NsourceAddr);
			else
				memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));

			sourceEntry=rtk_igmp_searchSourceEntry(ipVersion, _HsourceAddr, clientEntry);
			if (sourceEntry==NULL) // A-X-Y
			{
				newSourceEntry=rtk_igmp_allocateSourceEntry();
				if (newSourceEntry==NULL)
				{
					IGMP_CTRL("run out of source entry!\n");
					return RTK_IGMP_NF_DROP;
				}

				if (ipVersion==IP_VERSION4)
				{
					newSourceEntry->sourceAddr[0]=ntohl(p_NsourceAddr[0]);
				}
				else
				{
					newSourceEntry->sourceAddr[0]=p_NsourceAddr[0];
					newSourceEntry->sourceAddr[1]=p_NsourceAddr[1];
					newSourceEntry->sourceAddr[2]=p_NsourceAddr[2];
					newSourceEntry->sourceAddr[3]=p_NsourceAddr[3];
				}

				newSourceEntry->portTimer=clientEntry->groupFilterTimer;

				rtk_igmp_linkSourceEntry(clientEntry,newSourceEntry);
			}
			else // A*X+A*Y
			{
				if (_fastLeave)
				{
					sourceEntry->portTimer=rtk_igmp_sysUpSeconds;
				}


				if (sourceEntry->portTimer>rtk_igmp_sysUpSeconds) // A*X
				{
				}
				else	//A*Y
				{
					if (ipVersion==IP_VERSION4)
					{
						p_NsourceAddr++;
					}
					else
					{
						p_NsourceAddr=p_NsourceAddr+4;
					}
					continue;
				}

			}

			// A-X-Y + A*X = A-Y
			if (ipVersion==IP_VERSION4)
			{
				//querySourceAddr
				IGMP_CTRL("Block.GIP.Clt.SrcIP(%pI4n) in A-Y",(p_NsourceAddr));////

				*tmpAddress = *p_NsourceAddr;
				*p_NsourceAddr = *(p_NsourceAddr + (numOfSrc-j-1));
				*(p_NsourceAddr + (numOfSrc-j-1)) = *tmpAddress;
				IGMP_CTRL("sourceAddr Head(%pI4n) Tail(%pI4n)",(p_NsourceAddr), ((p_NsourceAddr+(numOfSrc-j-1))));
			}
			else
			{
				int i ;
				IGMP_CTRL("Block.GIP6.Clt.SrcIP6(%pI6) in A-Y",(p_NsourceAddr));////

				for (i=0; i<4; i++)
				{
					*(tmpAddress+i) = *(p_NsourceAddr+i);
					*(p_NsourceAddr+i) = *(p_NsourceAddr+i + ((numOfSrc -j - 1)));
					*(p_NsourceAddr+i + ((numOfSrc - j - 1))) = *(tmpAddress+i);
				}
			}

			j--;
			numOfSrc--;
			numOfQuerySrc++;
			continue;	//no need to move sourceAddr


			if (ipVersion==IP_VERSION4)
			{
				p_NsourceAddr++;
			}
			else
			{
				p_NsourceAddr=p_NsourceAddr+4;
			}
		}
	}
	else           /*means include mode*/
	{
		IGMP_CTRL("<In-Mo> GIP.Clt");

		numOfQuerySrc=0;
		for (j=0; j<numOfSrc; j++)
          	{
			IGMP_CTRL("current j(%d), numOfSrc(%d), numOfQuerySrc(%d)", j, numOfSrc, numOfQuerySrc);////
			if(ipVersion==IP_VERSION4)
				_HsourceAddr[0] = ntohl(*p_NsourceAddr);
			else
				memcpy(_HsourceAddr,p_NsourceAddr,sizeof(_HsourceAddr));

			sourceEntry=rtk_igmp_searchSourceEntry(ipVersion, _HsourceAddr,clientEntry);
			if (sourceEntry!=NULL) //
			{
				if (_fastLeave)
				{
					sourceEntry->portTimer=rtk_igmp_sysUpSeconds;
				}


				if (ipVersion==IP_VERSION4)
				{
					//querySourceAddr
					IGMP_CTRL("Block.GIP.Clt.SrcIP(%pI4n) in A*B",(p_NsourceAddr));////

					*tmpAddress = *p_NsourceAddr;

					*p_NsourceAddr = *(p_NsourceAddr + (numOfSrc-j-1));
					*(p_NsourceAddr + (numOfSrc-j-1)) = *tmpAddress;
					IGMP_CTRL("sourceAddr Head(%pI4n) Tail(%pI4n)",(p_NsourceAddr), ((p_NsourceAddr+(numOfSrc-j-1))));

				}
				else
				{
					int i ;
					IGMP_CTRL("Block.GIP6.Clt.SrcIP6(%pI6) in A*B",(p_NsourceAddr));////

					for (i=0; i<4; i++)
					{
						*(tmpAddress+i) = *(p_NsourceAddr+i);

						*(p_NsourceAddr+i) = *(p_NsourceAddr+i + ((numOfSrc-j-1)));
						*(p_NsourceAddr+i + ((numOfSrc-j-1))) = *(tmpAddress+i);

					}
				}
				j--;
				numOfSrc--;
				numOfQuerySrc++;
				continue;	//no need to move sourceAddr
			}
			else
			{
				if (ipVersion==IP_VERSION4)
				{
					//querySourceAddr
					IGMP_CTRL("Block.GIP.Clt.SrcIP(%pI4n) no in A*B",(p_NsourceAddr));////
				}
				else
				{
					IGMP_CTRL("Block.GIP6.Clt.SrcIP6(%pI6) no in A*B",(p_NsourceAddr));////
				}

			}

			if (ipVersion==IP_VERSION4)
			{
				p_NsourceAddr++;
			}
			else
			{
				p_NsourceAddr=p_NsourceAddr+4;
			}

			//IGMP_CTRL("");
 		}

	}

#if 0
	if (ipVersion==IP_VERSION4)
	{
		int i ;
		for (i=0; i<numOfQuerySrc; i++)
		{
			IGMP_CTRL("Query GIP(%pI4h)-SrcIP(%pI4n)", (groupAddress), ((p_NsourceAddr+i)));
		}
		//IGMPQueryVersion=3;
		//rtk_igmp_igmpQueryTimerExpired(0, IGMP_V3, (char *)groupAddress, numOfQuerySrc, NULL, (char *)p_NsourceAddr);
	}
	else
	{
		int i ;
		for (i=0; i<numOfQuerySrc; i+=4)
		{
			IGMP_CTRL("Query GIP6(%pI6)-SrcIP(%pI6)", (groupAddress), ((p_NsourceAddr+i)));
		}
		//IGMPQueryVersion=3; ???????????????????????????????????????????????
		//rtk_igmp_mldQueryTimerExpired(0);
	}
#endif


out:

	rtk_igmp_flow_reflush_update(FAIL,groupAddress);

	return RTK_IGMP_NF_CONTINUE;
}

//we only return RTK_IGMP_NF_DROP/RTK_IGMP_NF_ACCEPT
static uint32 rtk_igmp_processIgmpv3Mldv2Reports(rtk_igmp_pktHdr_t *pPkthdr, const struct net_device *SrcDev)
{
	uint32 clientAddr[4]={0, 0, 0, 0};
	uint32 i=0;
	uint16 numOfRecords=0;
	uint8 *groupRecords=NULL;
	uint8 recordType=0xff;
	uint16 numOfSrc=0;
	int32 returnVal=0;
	uint32 ipVersion;
	uint8 * pktBuf;

//	uint32 multicastRouterPortMask=rtk_igmp_getMulticastRouterPortMask(moduleIndex, ipVersion, rtk_igmp_sysUpSeconds);
	IGMP_CTRL("IGMPv3/MLDv2 Report packet");

	if(pPkthdr->iph && pPkthdr->ip6h==NULL)
	{
		ipVersion= IP_VERSION4;
		pktBuf=(uint8 *)pPkthdr->igmph;
		clientAddr[0]=ntohl(pPkthdr->iph->saddr);
	}
	else if (pPkthdr->iph==NULL && pPkthdr->ip6h)
	{
		ipVersion= IP_VERSION6;
		pktBuf=(uint8 *)pPkthdr->icmp6h;
		memcpy(&clientAddr[0],&pPkthdr->ip6h->saddr.s6_addr32[0],sizeof(clientAddr));
	}
	else if (pPkthdr->iph && pPkthdr->ip6h)
	{
		ipVersion= BOTH_IPV4_IPV6;
		IGMP_CTRL("Not support");
		return RTK_IGMP_NF_ACCEPT;
	}
	else
	{
		IGMP_CTRL("Not support");
		return RTK_IGMP_NF_ACCEPT;
	}


	if (ipVersion==IP_VERSION4)
	{
		numOfRecords=ntohs(((struct igmpv3Report *)pktBuf)->numOfRecords);
		if (numOfRecords!=0)
		{
			groupRecords=(uint8 *)(&(((struct igmpv3Report *)pktBuf)->recordList));
		}
	}
	else
	{
		numOfRecords=ntohs(((struct mldv2Report *)pktBuf)->numOfRecords);
		if (numOfRecords!=0)
		{
			groupRecords=(uint8 *)(&(((struct mldv2Report *)pktBuf)->recordList));
		}
	}


	for (i=0; i<numOfRecords; i++)
	{
		if (ipVersion==IP_VERSION4)
		{
			recordType=((struct groupRecord *)groupRecords)->type;
		}
		else
		{
			recordType=((struct mCastAddrRecord *)groupRecords)->type;
		}


		switch (recordType)
		{
			case MODE_IS_INCLUDE:
				returnVal=rtk_igmp_processIsInclude(pPkthdr, ipVersion, clientAddr, groupRecords);
			break;

			case MODE_IS_EXCLUDE:
				returnVal=rtk_igmp_processIsExclude(pPkthdr, ipVersion, clientAddr, groupRecords);
			break;

			case CHANGE_TO_INCLUDE_MODE:
				returnVal=rtk_igmp_processToInclude(pPkthdr, ipVersion, clientAddr, groupRecords);
			break;

			case CHANGE_TO_EXCLUDE_MODE:
				returnVal=rtk_igmp_processToExclude(pPkthdr, ipVersion, clientAddr, groupRecords);
			break;

			case ALLOW_NEW_SOURCES:
				returnVal=rtk_igmp_processAllow(pPkthdr, ipVersion, clientAddr, groupRecords);
			break;

			case BLOCK_OLD_SOURCES:
				returnVal=rtk_igmp_processBlock(pPkthdr, ipVersion, clientAddr ,groupRecords);
			break;

			default:break;

		}

		if( (numOfRecords==1)&&(returnVal==RTK_IGMP_NF_DROP || returnVal==RTK_IGMP_NF_ACCEPT))
			return returnVal;

		if (ipVersion==IP_VERSION4)
		{
			numOfSrc=ntohs(((struct groupRecord *)groupRecords)->numOfSrc);
			/*shift pointer to another group record*/
			groupRecords=groupRecords+8+numOfSrc*4+(((struct groupRecord *)(groupRecords))->auxLen)*4;
		}
		else
		{
			numOfSrc=ntohs(((struct mCastAddrRecord *)groupRecords)->numOfSrc);
			/*shift pointer to another group record*/
			groupRecords=groupRecords+20+numOfSrc*16+(((struct mCastAddrRecord *)(groupRecords))->auxLen)*4;
		}

	}

	return RTK_IGMP_NF_ACCEPT;//(multicastRouterPortMask & (~(1<<portNum)) & IGMP_SUPPORT_PORT_MASK);

}

rtk_rate_limit_ret_t check_rate_limit(rtk_igmp_pktHdr_t *pPkthdr)
{
	rtk_igmp_DevConfdb_t *devConf;
	unsigned long current_time=jiffies;
	unsigned long last_cycle_start_time;
	unsigned long diff;
	unsigned long cycle_duration=RATE_LIMIT_CYCLE_DEFINE_SECONDS*CONFIG_HZ;

	int is_igmp=0;

	if(pPkthdr->iph && pPkthdr->ip6h==NULL){
		IGMP_CTRL("Rate Limit: IGMP Packet");
		is_igmp=1;
	}else if (pPkthdr->iph==NULL && pPkthdr->ip6h){
		IGMP_CTRL("Rate Limit: MLD Packet");
		is_igmp=0;
	}else if (pPkthdr->iph && pPkthdr->ip6h){
		IGMP_CTRL("Rate Limit: Not support");
		return RTK_RATE_LIMIT_ACCEPT_PACKET;
	}else{
		IGMP_CTRL("Rate Limit: Not support");
		return RTK_RATE_LIMIT_ACCEPT_PACKET;
	}

	devConf = &pPkthdr->devInfo->devConf;

	if(is_igmp){
		last_cycle_start_time=devConf->igmp.ratelimit.cycle_start_time;
	}else{
		last_cycle_start_time=devConf->igmp6.ratelimit.cycle_start_time;
	}

	//get diff time
	if(current_time < last_cycle_start_time){ //timer overflow case
		diff=(0xffffffff-last_cycle_start_time)+current_time;
	}else{
		diff=current_time-last_cycle_start_time;
	}

	if(diff > cycle_duration){
		//In a new cycle. Calculates the new cycle start time.
		last_cycle_start_time=current_time-(diff%cycle_duration);
		if(is_igmp){
			devConf->igmp.ratelimit.cycle_start_time=last_cycle_start_time;
			devConf->igmp.ratelimit.counter=1;
		}else{
			devConf->igmp6.ratelimit.cycle_start_time=last_cycle_start_time;
			devConf->igmp6.ratelimit.counter=1;
		}
		return RTK_RATE_LIMIT_ACCEPT_PACKET;
	}

	if(is_igmp){
		if(devConf->igmp.ratelimit.counter < devConf->igmp.ratelimit.rate){
			devConf->igmp.ratelimit.counter++;
		}else{
			IGMP_RATE_LIMIT_DROP("Rate Limit: IGMP drop incoming packets");
			return RTK_RATE_LIMIT_DROP_PACKET;
		}
	}else{
		if(devConf->igmp6.ratelimit.counter < devConf->igmp6.ratelimit.rate){
			devConf->igmp6.ratelimit.counter++;
		}else{
			IGMP_RATE_LIMIT_DROP("Rate Limit: MLD drop incoming packets");
			return RTK_RATE_LIMIT_DROP_PACKET;
		}
	}
	return RTK_RATE_LIMIT_ACCEPT_PACKET;
}

rtk_igmp_nf_ret_e_t rtk_igmp_mld_process( rtk_igmp_pktHdr_t *pPkthdr, const struct net_device *SrcDev)
{
	rtk_igmp_nf_ret_e_t ret=RTK_IGMP_NF_ACCEPT;
	rtk_igmp_DevConfdb_t *devConf;

	//eth0.x or nas0_x
	pPkthdr->devInfo=_rtk_igmp_devIfidx_to_devInfo(pPkthdr->devIfindex);
	if(pPkthdr->devInfo==NULL){IGMP_CTRL("pPkthdr->devInfo==NULL %s",SrcDev->name); return RTK_IGMP_NF_ACCEPT;}
	pPkthdr->moduleIndex = rtk_igmp_srcNetDevToModuleIdx(SrcDev,pPkthdr->devInfo);
	if(pPkthdr->moduleIndex ==FAIL) {IGMP_CTRL("moduleIndex not found!"); return RTK_IGMP_NF_ACCEPT;}

	devConf = &pPkthdr->devInfo->devConf;

	if(pPkthdr->igmph)
	{

		if(devConf->igmp.igmpSnoopingDisable)
		{
			return RTK_IGMP_NF_ACCEPT;
		}

		switch(pPkthdr->igmph->type)
		{
			case IGMP_HOST_MEMBERSHIP_REPORT:
				IGMP_CTRL("IGMPv1 Report");

				if(devConf->igmp.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				if((devConf->igmp.igmpVerDrp_v1) ||  (devConf->igmp.igmpReportDrop) )
				{
					IGMP_CTRL("igmpVerDrp_v1 || igmpReportDrop NF_DROP");
					return RTK_IGMP_NF_DROP;
				}
				if(devConf->igmp.igmpVerIgr_v1)
				{
					IGMP_CTRL("igmpVerIgr_v1 NF_ACCEPT");
					return RTK_IGMP_NF_ACCEPT;
				}
				ret=rtk_igmp_processJoin(pPkthdr,SrcDev);
				break;

			case IGMPV2_HOST_MEMBERSHIP_REPORT:
				IGMP_CTRL("IGMPv2 Report");

				if(devConf->igmp.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				if((devConf->igmp.igmpVerDrp_v2) ||  (devConf->igmp.igmpReportDrop) )
				{
					IGMP_CTRL("igmpVerDrp_v2 || igmpReportDrop NF_DROP");
					return RTK_IGMP_NF_DROP;
				}
				if(devConf->igmp.igmpVerIgr_v2)
				{
					IGMP_CTRL("igmpVerIgr_v2 NF_ACCEPT");
					return RTK_IGMP_NF_ACCEPT;
				}
				ret=rtk_igmp_processJoin(pPkthdr,SrcDev);
				break;

			case IGMPV3_HOST_MEMBERSHIP_REPORT:
				IGMP_CTRL("IGMPv3 Report");
				if(devConf->igmp.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				if((devConf->igmp.igmpVerDrp_v3) ||  (devConf->igmp.igmpReportDrop) )
				{
					IGMP_CTRL("igmpVerDrp_v3 || igmpReportDrop NF_DROP");
					return RTK_IGMP_NF_DROP;
				}
				if(devConf->igmp.igmpVerIgr_v3)
				{
					IGMP_CTRL("igmpVerIgr_v3 NF_ACCEPT");
					return RTK_IGMP_NF_ACCEPT;
				}
				ret=rtk_igmp_processIgmpv3Mldv2Reports(pPkthdr,SrcDev);
				break;

			//we don't want to care query packet.
			case IGMP_HOST_MEMBERSHIP_QUERY:
				IGMP_CTRL("IGMP Query");
				if(devConf->igmp.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				//rtk_igmp_processQueries
				break;

			case IGMP_HOST_LEAVE_MESSAGE:
				IGMP_CTRL("IGMPv2 Leave");
				if(devConf->igmp.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				if(devConf->igmp.igmpVerDrp_v2)
				{
					IGMP_CTRL("igmpVerDrp_v2  NF_DROP");
					return RTK_IGMP_NF_DROP;
				}
				if(devConf->igmp.igmpVerIgr_v2)
				{
					IGMP_CTRL("igmpVerIgr_v2  NF_ACCEPT");
					return RTK_IGMP_NF_ACCEPT;
				}
				ret=rtk_igmp_processLeave(pPkthdr,SrcDev);
				break;
			default:
				return RTK_IGMP_NF_ACCEPT;
				break;
		}
	}
	else if(pPkthdr->icmp6h)
	{
		if(devConf->igmp6.igmp6SnoopingDisable)
		{
			return RTK_IGMP_NF_ACCEPT;
		}
		switch(pPkthdr->icmp6h->icmp6_type)
		{
			case ICMPV6_MLD2_REPORT:
				IGMP_CTRL("MLDv2 REPORT");
				if(devConf->igmp6.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				if(devConf->igmp6.igmp6VerDrp_v2 || devConf->igmp6.igmp6ReportDrop)
				{
					IGMP_CTRL("igmp6VerDrp_v2 || igmp6ReportDrop  NF_DROP");
					return RTK_IGMP_NF_DROP;
				}
				if(devConf->igmp6.igmp6VerIgr_v2)
				{
					IGMP_CTRL("igmp6VerIgr_v2  NF_ACCEPT");
					return RTK_IGMP_NF_ACCEPT;
				}
				ret=rtk_igmp_processIgmpv3Mldv2Reports(pPkthdr,SrcDev);
				break;

			//we don't want to care query packet.
			case ICMPV6_MGM_QUERY:
				IGMP_CTRL("MLDv QUERY");
				if(devConf->igmp6.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				break;

			case ICMPV6_MGM_REPORT:
				IGMP_CTRL("MLDv1 REPORT");
				if(devConf->igmp6.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				if(devConf->igmp6.igmp6VerDrp_v1 || devConf->igmp6.igmp6ReportDrop)
				{
					IGMP_CTRL("igmp6VerDrp_v1 || igmp6ReportDrop  NF_DROP");
					return RTK_IGMP_NF_DROP;
				}
				if(devConf->igmp6.igmp6VerIgr_v1)
				{
					IGMP_CTRL("igmp6VerIgr_v1  NF_ACCEPT");
					return RTK_IGMP_NF_ACCEPT;
				}
				ret=rtk_igmp_processJoin(pPkthdr,SrcDev);
				break;

			case ICMPV6_MGM_REDUCTION:
				IGMP_CTRL("MLDv1 Done");
				if(devConf->igmp6.ratelimit.rate)
				{
					if(RTK_RATE_LIMIT_DROP_PACKET==check_rate_limit(pPkthdr)){
						IGMP_RATE_LIMIT_DROP("Drop Packet due to the rate limit");
						return RTK_IGMP_NF_DROP;
					}
				}
				if(devConf->igmp6.igmp6VerDrp_v1)
				{
					IGMP_CTRL("igmp6VerDrp_v1  NF_DROP");
					return RTK_IGMP_NF_DROP;
				}
				if(devConf->igmp6.igmp6VerIgr_v1)
				{
					IGMP_CTRL("igmp6VerIgr_v1  NF_ACCEPT");
					return RTK_IGMP_NF_ACCEPT;
				}
				ret=rtk_igmp_processLeave(pPkthdr,SrcDev);
				break;

			default:
				return RTK_IGMP_NF_ACCEPT;
				break;
		}
	}

	IGMP_CTRL("IGMP/MLD Process ret:%d %s",ret,ret==RTK_IGMP_NF_DROP?"RTK_IGMP_NF_DROP":ret==RTK_IGMP_NF_ACCEPT?"RTK_IGMP_NF_ACCEPT":"RTK_IGMP_NF_CONTINUE");
	return ret;

}


#define MDB_SEARCH

int _rtk_igmp_setEgressDevToFwdInfo(struct rtk_igmp_multicastFwdInfo *multicastFwdInfo ,uint32 egressIfidx)
{

	int i;
	int32 firstInvalid=IGMP_MAX_DEV_NUM;
	for(i=0 ;i<IGMP_MAX_DEV_NUM;i++)
	{
		if(multicastFwdInfo->egressDevInfo[i].valid)
		{
			//forward to same egress dev reuse it
			if(multicastFwdInfo->egressDevInfo[i].devIfIdx==egressIfidx)
				return SUCCESS;
		}
		else
		{
			if(firstInvalid==IGMP_MAX_DEV_NUM)
			{
				firstInvalid =i;
				break;
			}
			
		}
	}
	if(firstInvalid==IGMP_MAX_DEV_NUM) {IGMP_WARNING("IGMP_MAX_DEV_NUM EXCEED!! "); return FAIL;}

	multicastFwdInfo->egressDevInfo[firstInvalid].valid=1;
	multicastFwdInfo->egressDevInfo[firstInvalid].devIfIdx =egressIfidx;

	return SUCCESS;
}



int _rtk_igmp_setClientToEgressPortInfo(struct rtk_igmp_multicastFwdInfo *multicastFwdInfo ,struct rtk_igmp_clientEntry* clientEntry)
{

	int i;
	int32 firstInvalid=IGMP_MAX_DEV_NUM;
	for(i=0 ;i<IGMP_MAX_DEV_NUM;i++)
	{
		if(multicastFwdInfo->egressDevInfo[i].valid)
			continue;
		else
		{
			firstInvalid =i;
			break;
		}
	}
	if(firstInvalid==IGMP_MAX_DEV_NUM) {IGMP_WARNING("IGMP_MAX_DEV_NUM EXCEED!! "); return FAIL;}

	multicastFwdInfo->egressDevInfo[firstInvalid].valid=1;
	multicastFwdInfo->egressDevInfo[firstInvalid].devIfIdx =clientEntry->inIfidx;

#if 0
{
	rtk_igmp_multicastDeviceInfo_t *devInfo;
	devInfo = _rtk_igmp_devIfidx_to_devInfo(clientEntry->inIfidx);

	if(devInfo && devInfo->forceDevCTag)
	{
		multicastFwdInfo->egressDevInfo[firstInvalid].egressCtagif=devInfo->forceDevCTagif;
		multicastFwdInfo->egressDevInfo[firstInvalid].egressVlan = devInfo->forceDevCvid;
	}
	else if (clientEntry->reportCtagif)
	{
		multicastFwdInfo->egressDevInfo[firstInvalid].egressCtagif=1;
		multicastFwdInfo->egressDevInfo[firstInvalid].egressVlan = clientEntry->reportVlanId;
	}
}
#endif
	return SUCCESS;
}



static int _rtk_igmp_getMulticastDataFwdInfo(int32 moduleIndex,uint32 searchCacheFlowFirst, struct rtk_igmp_multicastDataInfo *multicastDataInfo, struct rtk_igmp_multicastFwdInfo *multicastFwdInfo)
{
	struct rtk_igmp_groupEntry * groupEntry=NULL;
	struct net_bridge *br =NULL;
	struct net_bridge_port *p;
	rtk_igmp_multicastDeviceInfo_t* devInfo;


	memset(multicastFwdInfo, 0, sizeof(struct rtk_igmp_multicastFwdInfo));

	if ( (multicastDataInfo->ipVersion==IP_VERSION4 && IN_MULTICAST_RESV_IPV4_ALL(multicastDataInfo->groupAddr[0])) ||
		 (multicastDataInfo->ipVersion==IP_VERSION6 && IN_MULTICAST_RESV_IPV6_ALL(multicastDataInfo->groupAddr[0])) ||
		 _rtk_igmp_checkGroupToPsTbl(multicastDataInfo->ipVersion,multicastDataInfo->groupAddr))
	{
		multicastFwdInfo->reservedMCast=TRUE;
		multicastFwdInfo->cpuFlag=TRUE;
		return FAIL;
	}

	if(searchCacheFlowFirst)
	{
		struct rtk_igmp_mcastFlowEntry *mcastFlowEntry=NULL;
		mcastFlowEntry=rtk_igmp_searchMcastFlowEntry(moduleIndex, multicastDataInfo->ipVersion, multicastDataInfo->sourceIp, multicastDataInfo->groupAddr,multicastDataInfo->sport,multicastDataInfo->dport,multicastDataInfo->vlanTagif?multicastDataInfo->vlanId:FAIL,multicastDataInfo->l4proto);
		if (mcastFlowEntry!=NULL)
		{
			memcpy(multicastFwdInfo, &mcastFlowEntry->multicastFwdInfo, sizeof(struct rtk_igmp_multicastFwdInfo));
			mcastFlowEntry->fwdPktCnt++;
			if(multicastDataInfo->privateField & DUMMY_PKT)
				mcastFlowEntry->defineFwdPktCnt++;

			/* add/update flow*/
			if(multicastDataInfo->privateField & SKIP_LEARNING)
			{
				IGMP_CTRL("Do not flowSetCbEvt to hwcb because FC SKIP_LEARNING");
			}
			else if(hwCb.flowSetCbEvt && mcastFlowEntry->multicastFwdInfo.hwCbAcc==0)
			{
				rtk_igmp_flowCbEvt_t flowCb;
				memset(&flowCb,0,sizeof(flowCb));
				if(mcastFlowEntry->ipVersion == IP_VERSION6)
					flowCb.isIPv6=1;
				memcpy(flowCb.group,mcastFlowEntry->groupAddr,sizeof(flowCb.group));
				memcpy(flowCb.sourceAddr,mcastFlowEntry->serverAddr,sizeof(flowCb.sourceAddr));
				memcpy(flowCb.egressDevInfo,mcastFlowEntry->multicastFwdInfo.egressDevInfo,sizeof(flowCb.egressDevInfo));
				flowCb.l4proto = mcastFlowEntry->l4proto;
				flowCb.sport = mcastFlowEntry->sport;
				flowCb.dport = mcastFlowEntry->dport;
				flowCb.ingressCvlan = mcastFlowEntry->ingressCvlan;
				hwCb.flowSetCbEvt(&flowCb);
				mcastFlowEntry->multicastFwdInfo.hwCbAcc=1;
			}		
			
			return SUCCESS;
		}
	}

	//flooding multicast to Br snooping disable ports
	br = netdev_priv(rtk_igmp_mCastModuleArray[moduleIndex].brDevInfo.dev);
	if(br != NULL)
	{
		uint32 disableSnooping=0;
		list_for_each_entry(p, &br->port_list, list)
		{
			disableSnooping=0;
			devInfo = _rtk_igmp_devIfidx_to_devInfo(p->dev->ifindex);
			if(devInfo)
			{
				if(multicastDataInfo->ipVersion == IP_VERSION4)
					disableSnooping = devInfo->devConf.igmp.igmpSnoopingDisable;
				else if(multicastDataInfo->ipVersion == IP_VERSION6)
					disableSnooping = devInfo->devConf.igmp6.igmp6SnoopingDisable;
				if(disableSnooping)
				{
					_rtk_igmp_setEgressDevToFwdInfo(multicastFwdInfo,p->dev->ifindex);
					IGMP_DATA("Dev[%s] disable Snooping flooding to Dev",devInfo->dev->name);
				}
			}
		}
	}


	groupEntry=rtk_igmp_searchGroupEntry(moduleIndex, multicastDataInfo->ipVersion, multicastDataInfo->groupAddr, multicastDataInfo->vlanId);
	if (groupEntry==NULL)
	{
		if(igmpSysdb.unknownFlood == 0)
			multicastFwdInfo->unknownMCast=TRUE;
		else  // support unknown muticast data flood
		{
			if(br != NULL)
			{
				list_for_each_entry(p, &br->port_list, list)
				{
					devInfo = _rtk_igmp_devIfidx_to_devInfo(p->dev->ifindex);
					if(devInfo)
					{
						
						_rtk_igmp_setEgressDevToFwdInfo(multicastFwdInfo,p->dev->ifindex);
						IGMP_DATA("Dev[%s] disable Snooping flooding to Dev",devInfo->dev->name);
						
					}
				}
			}
		 }
	}
	else
	{

		struct rtk_igmp_clientEntry * clientEntry=NULL;
		struct rtk_igmp_sourceEntry * sourceEntry=NULL;

		for (clientEntry=groupEntry->clientList; clientEntry!=NULL; clientEntry=clientEntry->next)
		{
			uint32 *sourceAddr = multicastDataInfo->sourceIp;
			uint32 careSource;
			sourceEntry=NULL;

			devInfo = _rtk_igmp_devIfidx_to_devInfo(clientEntry->inIfidx);
			if(devInfo == NULL)
			{
				IGMP_DATA("skip invalid inIfidx=%d",clientEntry->inIfidx);
				continue;
			}

			if(clientEntry->igmpVersion == IP_VERSION4)
			{
				careSource = _rtk_igmp_devIfidx_to_devInfo(clientEntry->inIfidx)->devConf.igmp.igmpv3CareSource;
			}
			else
			{
				careSource = _rtk_igmp_devIfidx_to_devInfo(clientEntry->inIfidx)->devConf.igmp6.mldv2CareSource;
			}


			if(careSource)
			{
				if (clientEntry->groupFilterTimer<=rtk_igmp_sysUpSeconds) /*include mode*/
				{
					sourceEntry = rtk_igmp_searchSourceEntry(groupEntry->ipVersion, sourceAddr, clientEntry);
					if (sourceEntry!=NULL)
					{
						if (sourceEntry->portTimer>rtk_igmp_sysUpSeconds)
						{
							IGMP_DATA("include inIfidx=%d",clientEntry->inIfidx);
							_rtk_igmp_setEgressDevToFwdInfo(multicastFwdInfo,clientEntry->inIfidx);
							continue;
						}
					}
				}
				else	/*exclude mode*/
				{
					sourceEntry = rtk_igmp_searchSourceEntry(groupEntry->ipVersion, sourceAddr, clientEntry);
					if (sourceEntry == NULL)
					{
						IGMP_DATA("source not record forwad inIfidx=%d",clientEntry->inIfidx);
						_rtk_igmp_setEgressDevToFwdInfo(multicastFwdInfo,clientEntry->inIfidx);
					}
					else
					{
						if (sourceEntry->portTimer>rtk_igmp_sysUpSeconds)
						{
							IGMP_DATA("source record forwad inIfidx=%d",clientEntry->inIfidx);
							_rtk_igmp_setEgressDevToFwdInfo(multicastFwdInfo,clientEntry->inIfidx);
						}
					}
				}

			}
			else /* don't care source */
			{
				if (clientEntry->groupFilterTimer<=rtk_igmp_sysUpSeconds) /*include mode*/
				{
					sourceEntry = clientEntry->sourceList;
					while (sourceEntry)
					{
						if (sourceEntry->portTimer>rtk_igmp_sysUpSeconds)
						{
							_rtk_igmp_setEgressDevToFwdInfo(multicastFwdInfo,clientEntry->inIfidx);
							IGMP_DATA("add in-mo clt.inIfidx(%d)", clientEntry->inIfidx);
							break;
						}
						sourceEntry = sourceEntry->next;
					}
				}
				else /*exclude mode*/
				{
					IGMP_DATA("add ex-mo clt.inIfidx(%d)", clientEntry->inIfidx);
					_rtk_igmp_setEgressDevToFwdInfo(multicastFwdInfo,clientEntry->inIfidx);
				}
			}

		}

	} //if (group == NULL)

	rtk_igmp_recordMcastFlow(moduleIndex,multicastDataInfo,multicastFwdInfo);

	if(_rtk_igmp_IsflowToDevInfoIsNonValidPort(multicastFwdInfo->egressDevInfo))
	{
		multicastFwdInfo->dropPkt=1;
	}
	return SUCCESS;

}


int rtk_igmp_getMulticastDataFwdInfo(const struct net_device *SrcDev,const struct net_device *DstDev,uint32 searchCacheFlowFirst, struct rtk_igmp_multicastDataInfo *multicastDataInfo, struct rtk_igmp_multicastFwdInfo *multicastFwdInfo, int32 *pkthdrModuleIndex)
{

	int32 moduleIndex= 0;

	if (multicastFwdInfo==NULL || multicastDataInfo==NULL)
	{
		IGMP_DATA("multicastFwdInfo or multicastDataInfo is null, return\n");
		return FAIL;
	}

	if(DstDev)
	{
		//br_x to moduleIndex in postrouting
		moduleIndex = rtk_igmp_dstNetDevToModuleIdx(DstDev);
		if(moduleIndex ==FAIL) {IGMP_DATA("%s ==> moduleIndex not found!",DstDev->name); return FAIL;}
	}
	else
	{
		//nas0_x/eth0.x  to moduleIndex in prerouting
		moduleIndex = rtk_igmp_srcNetDevToModuleIdx(SrcDev,NULL);
		if(moduleIndex ==FAIL) {IGMP_DATA("%s ==> moduleIndex not found!",SrcDev->name); return FAIL;}
	}

	*pkthdrModuleIndex = moduleIndex;

	return _rtk_igmp_getMulticastDataFwdInfo(moduleIndex,searchCacheFlowFirst,multicastDataInfo,multicastFwdInfo);


}

int rtk_igmp_devStatistic_get(rt_igmpHook_devInfo_t devInfo,unsigned int isIpv6,rt_igmpHook_devStatistic_t *statisticInfo)
{
	int32 _devifid=FAIL;
	int getIdx=0;
	int32 activeCnt=0;
	int32 curWeight=0;
	rt_igmpHook_groupStatistic_t groupstatisticInfo;
	
	rtk_igmp_multicastDeviceInfo_t *devcfg=NULL;
	rtk_igmp_multicastModule_t *brcfg=NULL;
	
	_devifid = rtk_igmp_devInfoToIfIdx(devInfo);
	if(_devifid==FAIL)
		return FAIL;

	devcfg =_rtk_igmp_devIfidx_to_devInfo(_devifid);
	brcfg = _rtk_igmp_devIfidx_to_BrDevInfo(_devifid);

	if(devcfg==NULL && brcfg==NULL)
		return FAIL;


	while(1)
	{
		if(rtk_igmp_nextValidGroupStatistic_get(devInfo,&getIdx,isIpv6,&groupstatisticInfo)==SUCCESS && getIdx>=0)
		{
			getIdx++;
			activeCnt++;
			curWeight+=_rtk_igmp_checkGroupWeightTbl(_devifid,isIpv6?IP_VERSION6:IP_VERSION4,groupstatisticInfo.groupAddr);
		}
		else
			break;
	}

	statisticInfo->activeGroupCnt=activeCnt;
	statisticInfo->curWeight =curWeight;
	if(devcfg)
	{
		if(isIpv6)
		{
			statisticInfo->bandwidthExceedCnt = devcfg->devStatistic.igmp6WeightExceedCnt;
			statisticInfo->joinCnt =devcfg->devStatistic.igmp6JoinCnt;
		}
		else
		{
			statisticInfo->bandwidthExceedCnt = devcfg->devStatistic.igmp4WeightExceedCnt;
			statisticInfo->joinCnt =devcfg->devStatistic.igmp4JoinCnt;
		}
	}
	else if(brcfg)
	{
		if(isIpv6)
		{
			statisticInfo->bandwidthExceedCnt = brcfg->brDevInfo.brStatistic.igmp6WeightExceedCnt;
			statisticInfo->joinCnt =  brcfg->brDevInfo.brStatistic.igmp6JoinCnt;

		}
		else
		{
			statisticInfo->bandwidthExceedCnt = brcfg->brDevInfo.brStatistic.igmp4WeightExceedCnt;
			statisticInfo->joinCnt =  brcfg->brDevInfo.brStatistic.igmp4JoinCnt;
		}
	}
	
	IGMP_CTRL("Get activeGroupCnt=%d  curWeight=%d joinCnt=%d bandwidthExceedCnt=%d",statisticInfo->activeGroupCnt,statisticInfo->curWeight,statisticInfo->joinCnt,statisticInfo->bandwidthExceedCnt);
	
	return SUCCESS;
}


int rtk_igmp_nextValidGroupStatistic_get(rt_igmpHook_devInfo_t devInfo,int *getIdx,unsigned int isIpv6,rt_igmpHook_groupStatistic_t *statisticInfo)
{
	int32 _devifid=FAIL;
	rtk_igmp_multicastDeviceInfo_t *devcfg=NULL;
	rtk_igmp_multicastModule_t *brcfg=NULL;
	int32 searchMin,searchIdx;
	uint32 clinetJoinSec=0;

	int i;
	int32 hitGroupAndDevIdx=FAIL;
	int32 hashIndex;
	int32 moduleIndex;
	struct rtk_igmp_groupEntry *groupEntryPtr;
	struct rtk_igmp_clientEntry* clientEntry=NULL;

	_devifid = rtk_igmp_devInfoToIfIdx(devInfo);
	if(_devifid==FAIL)
		return FAIL;

	devcfg =_rtk_igmp_devIfidx_to_devInfo(_devifid);
	brcfg = _rtk_igmp_devIfidx_to_BrDevInfo(_devifid);

	if(devcfg==NULL && brcfg==NULL)
		return FAIL;

	searchMin=*getIdx;
	searchIdx=atomic_read(&groupIncIdx);


	for(moduleIndex=0 ; moduleIndex<IGMP_MAX_BR_MODULE_NUM;moduleIndex++ )
	{
		if(brcfg &&rtk_igmp_mCastModuleArray[moduleIndex].brDevInfo.ifindex!=_devifid)
			continue;			
		if(rtk_igmp_mCastModuleArray[moduleIndex].validBit==0)
			continue;			
		for (hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
		{
			if(isIpv6)
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv6HashTable[hashIndex];
			else
				groupEntryPtr=rtk_igmp_mCastModuleArray[moduleIndex].rtk_igmp_ipv4HashTable[hashIndex];

			while (groupEntryPtr!=NULL)
			{
				//IGMP_API("searchMin=%d groupEntryPtr->groupIdx=%d searchIdx=%d",searchMin,groupEntryPtr->groupIdx,searchIdx);
				if(searchMin <= groupEntryPtr->groupIdx  && groupEntryPtr->groupIdx <= searchIdx)
				{
					statisticInfo->curClientExistTimesSec=-1;
					memset(statisticInfo->curClient,0,sizeof(statisticInfo->curClient));
					
					clientEntry=groupEntryPtr->clientList;
					while (clientEntry!=NULL)
					{
/*						
						if(devcfg)
							IGMP_API("devcfg clientEntry->inIfidx=%d _devifid=%d",clientEntry->inIfidx,_devifid);
						else
							IGMP_API("Br clientEntry->inIfidx=%d _devifid=%d",clientEntry->inIfidx,_devifid);
*/
						if((devcfg && clientEntry->inIfidx ==_devifid) || (brcfg) )
						{
							//hit update group 
							searchIdx = groupEntryPtr->groupIdx;
							hitGroupAndDevIdx=searchIdx;
							memcpy(statisticInfo->groupAddr,groupEntryPtr->groupAddr,sizeof(statisticInfo->groupAddr));
							clinetJoinSec = rtk_igmp_sysUpSeconds -clientEntry->joinSystemSecTime;
							if(clinetJoinSec < statisticInfo->curClientExistTimesSec)
							{
								memcpy(statisticInfo->curClient,clientEntry->clientAddr,sizeof(statisticInfo->curClient));
								statisticInfo->curClientExistTimesSec =clinetJoinSec ;
							}
						}
					
						clientEntry = clientEntry->next;
					}
				}
				groupEntryPtr=groupEntryPtr->next;
			}
			
		}
	}


	if(hitGroupAndDevIdx!=FAIL)
	{
		//we can search sip/vlan from flow input(devIdx,group)
		struct rtk_igmp_mcastFlowEntry *mcastFlowEntry=NULL;
	
		//init to zero
		statisticInfo->vlanId=0;
		memset(statisticInfo->sourceAddr,0,sizeof(statisticInfo->sourceAddr));
	
		for(moduleIndex=0 ; moduleIndex<IGMP_MAX_BR_MODULE_NUM;moduleIndex++ )
		{
			if(brcfg &&rtk_igmp_mCastModuleArray[moduleIndex].brDevInfo.ifindex!=_devifid)
				continue;			
			if(rtk_igmp_mCastModuleArray[moduleIndex].validBit==0)
				continue;				
			for(hashIndex=0;hashIndex<IGMP_HASH_TBL_SIZE;hashIndex++)
			{
				/*to dump multicast flow information*/
				mcastFlowEntry=rtk_igmp_mCastModuleArray[moduleIndex].flowHashTable[hashIndex];
				while(mcastFlowEntry!=NULL)
				{
					if((isIpv6==0 && mcastFlowEntry->ipVersion==IP_VERSION4) ||(isIpv6==1 &&mcastFlowEntry->ipVersion==IP_VERSION6) )
					{
						if(memcmp(mcastFlowEntry->groupAddr,statisticInfo->groupAddr,sizeof(mcastFlowEntry->groupAddr))==0 )
						{
							for(i=0;i<IGMP_MAX_DEV_NUM;i++)
							{
								if(devcfg && mcastFlowEntry->multicastFwdInfo.egressDevInfo[i].devIfIdx!=_devifid)
									continue;

								memcpy(statisticInfo->sourceAddr,mcastFlowEntry->serverAddr,sizeof(statisticInfo->sourceAddr));
								statisticInfo->vlanId = mcastFlowEntry->ingressCvlan==FAIL?0:mcastFlowEntry->ingressCvlan;
								goto GET_SUCCESS;
							}
						}
					}
					mcastFlowEntry=mcastFlowEntry->next;
				}
			}
		}
	}


GET_SUCCESS:

	*getIdx=hitGroupAndDevIdx;

	if(*getIdx !=FAIL)
	{

		if(isIpv6)
		{
			IGMP_API("DIP:%pI6 SIP:%pI6 curClient=%pI6(ExitstSec:%d) Vlan:%d ",
				statisticInfo->groupAddr,statisticInfo->sourceAddr,statisticInfo->curClient,statisticInfo->curClientExistTimesSec,statisticInfo->vlanId);
		}
		else
		{
			IGMP_API("DIP:%pI4h SIP:%pI4h curClient=%pI4h(ExitstSec:%d) Vlan:%d ",
				statisticInfo->groupAddr,statisticInfo->sourceAddr,statisticInfo->curClient,statisticInfo->curClientExistTimesSec,statisticInfo->vlanId);
		}
		return SUCCESS;

	}
	else
	{
		//IGMP_API("no more entry hit here");
		return FAILED;
	}
		
}

