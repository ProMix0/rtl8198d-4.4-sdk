
#include <rtk_igmp_snooping.h>
#include <rt_igmp_ext.h>
#include <rtk_igmp_debug.h>
#include <rtk_igmp_hook.h>


#include <common/rt_type.h>
#include <rtk/l2.h>

#include <rtk_fc_helper_wlan.h>
#include <rtk_fc_helper.h>
#include <linux/workqueue.h>


#define UNCTAG -1
#define IF_SOURCECARE 1
#define IF_VLANCARE 1


int32 _rtk_igmp_hwRegInit(void)
{
	// set to auto mode
	rt_igmp_multicastMode_set(RT_IGMP_MULTICAST_SYNC_AUTO);

	return SUCCESS;
}
int32 _rtk_igmp_hwUnRegClear(void)
{
	// set to kernel mode
	rt_igmp_multicastMode_set(RT_IGMP_MULTICAST_SYNC_MODE_KERNEL);
	return SUCCESS;
}

int32 _rtk_igmp_unknownIpMc_Action(rtk_igmp_unknownMcCbEvt_t *unknownCtrl)
{

	if(unknownCtrl->unknownAct==UNKNOWN_MC_DROP)
	{
		if(unknownCtrl->isIpv6)
			rtk_l2_lookupMissAction_set(DLF_TYPE_IP6MC,ACTION_DROP);
		else
			rtk_l2_lookupMissAction_set(DLF_TYPE_IPMC, ACTION_DROP);
	}
	else if(unknownCtrl->unknownAct==UNKNOWN_MC_TRAP)
	{
		if(unknownCtrl->isIpv6)
			rtk_l2_lookupMissAction_set(DLF_TYPE_IP6MC,ACTION_TRAP2CPU);
		else
			rtk_l2_lookupMissAction_set(DLF_TYPE_IPMC, ACTION_TRAP2CPU);
	}
	return SUCCESS;
}

int32 _rtk_igmp_flowDelCbEvt(rtk_igmp_flowCbEvt_t *flowCb)
{
	int ret=0;
	rt_igmp_multicast_group_cfg_t mcConfig;

	if(flowCb->l4proto != IPPROTO_UDP)
		return FAIL;

	memset(&mcConfig,0,sizeof(mcConfig));
	mcConfig.groupBehavior=RT_MC_BEHAVIOR_FLOW_FORWAED;
	mcConfig.is_ipv6=flowCb->isIPv6;

	mcConfig.careIngressCvid=IF_VLANCARE;
	mcConfig.careSourceAddress=IF_SOURCECARE;

	if(flowCb->ingressCvlan==FAIL)
		mcConfig.ingress_ctagif=0;
	else
	{
		mcConfig.ingress_ctagif=1;
		mcConfig.ingress_cvid = flowCb->ingressCvlan;
	}

	if(flowCb->isIPv6)
		memcpy(&mcConfig.source_addr.ipv6[0],&flowCb->sourceAddr[0],sizeof(mcConfig.source_addr));
	else
		mcConfig.source_addr.ipv4=flowCb->sourceAddr[0];


	if(flowCb->isIPv6)
		memcpy(&mcConfig.group_addr.ipv6[0],&flowCb->group[0],sizeof(mcConfig.group_addr));
	else
		mcConfig.group_addr.ipv4=flowCb->group[0];

	ret = rt_igmp_multicastGroup_set (mcConfig);

//may delete same sip/dip but  dport/sport different not a error
//	if(ret)
//		IGMP_WARNING("ret=%d Error",ret);

	return ret;

}

/*
l4proto/dport/sport  will miss here
*/
int32 _rtk_igmp_flowSetCbEvt(rtk_igmp_flowCbEvt_t *flowCb)
{

	int ret=0;
	// int action0Active=0,action1Active=0;
	rt_igmp_multicast_group_cfg_t mcConfig;
	int i,j;
	rtk_fc_wlan_devidx_t wlanDevIdx;
	rtk_fc_wlan_devmask_t wlanDevIdMask;
	rt_wlan_mbssid_mask_t wlanMbssidMask[RT_WLAN_DEVICE_MAX];

	if(flowCb->l4proto != IPPROTO_UDP)
		return FAIL;

	memset(&mcConfig,0,sizeof(mcConfig));
	mcConfig.groupBehavior=RT_MC_BEHAVIOR_FLOW_FORWAED;
	mcConfig.is_ipv6=flowCb->isIPv6;

	mcConfig.careIngressCvid=IF_VLANCARE;
	mcConfig.careSourceAddress=IF_SOURCECARE;

	if(flowCb->ingressCvlan==FAIL)
		mcConfig.ingress_ctagif=0;
	else
	{
		mcConfig.ingress_ctagif=1;
		mcConfig.ingress_cvid = flowCb->ingressCvlan;
	}

	if(flowCb->isIPv6)
		memcpy(&mcConfig.source_addr.ipv6[0],&flowCb->sourceAddr[0],sizeof(mcConfig.source_addr));
	else
		mcConfig.source_addr.ipv4=flowCb->sourceAddr[0];


	if(flowCb->isIPv6)
		memcpy(&mcConfig.group_addr.ipv6[0],&flowCb->group[0],sizeof(mcConfig.group_addr));
	else
		mcConfig.group_addr.ipv4=flowCb->group[0];


	//we only used first_act_portmask only ( first_act_portmask == (first_act_portmask|second_act_portmask)
	for(i=0;i<IGMP_MAX_DEV_NUM;i++)
	{
		if(flowCb->egressDevInfo[i].valid)
		{
			struct net_device *dev;
			rtk_fc_realdev_t rdev;
			dev = rtk_igmp_devIfidx_to_dev(flowCb->egressDevInfo[i].devIfIdx);

			if(dev==NULL)
				continue;

			if(rtk_fc_dev_get_realdev_phyport(dev,&rdev)!=FAIL)
			{
				if(mcConfig.first_act_portmask & (1 << rdev.physicalPid) )
				{
					//multi-packet send to same physical port not support for now
					IGMP_DATA("multi-packet send to same physical port not support for now");
					return FAIL;
				}
				else
				{
					mcConfig.first_act_portmask |= (1 << rdev.physicalPid);
				}
			}
			else if(rtk_fc_dev2wlanDevIdx(dev, &wlanDevIdx)!=FAIL)
			{
				wlanDevIdMask = 1<< wlanDevIdx;
				rtk_fc_wlan_devMask2RtmbssidMask(&wlanDevIdMask,wlanMbssidMask);
				for(j=0 ; j<RT_WLAN_DEVICE_MAX ; j++)
					mcConfig.first_act_wlanMbssidMask[j] |= wlanMbssidMask[j];
			}
			
		}
	}
	IGMP_HWCB("set first_action_portmask to %x first_act_wlanMbssidMask[ %x %x %x ]",mcConfig.first_act_portmask,mcConfig.first_act_wlanMbssidMask[0],mcConfig.first_act_wlanMbssidMask[1],mcConfig.first_act_wlanMbssidMask[2]);

	ret = rt_igmp_multicastGroup_set (mcConfig);

	if(ret)
		IGMP_WARNING("ret=%d Error",ret);


	return ret;
}


int32 _rtk_igmp_groupAddCbEvt(rtk_igmp_groupCbEvt_t *groupCb)
{
	int ret=0;
	rt_igmp_multicast_group_cfg_t mcConfig;
	memset(&mcConfig,0,sizeof(mcConfig));

	mcConfig.careIngressCvid=IF_VLANCARE;
	mcConfig.careSourceAddress=IF_SOURCECARE;

	mcConfig.groupBehavior=RT_MC_BEHAVIOR_GROUP_AS_KNOW;
	mcConfig.is_ipv6=groupCb->isIPv6;

	if(groupCb->isIPv6)
		memcpy(&mcConfig.group_addr.ipv6[0],&groupCb->group[0],sizeof(mcConfig.group_addr));
	else
		mcConfig.group_addr.ipv4=groupCb->group[0];

	ret = rt_igmp_multicastGroup_set (mcConfig);

	if(ret)
		IGMP_WARNING("ret=%d Error",ret);

	return SUCCESS;

}

int32 _rtk_igmp_groupDelCbEvt(rtk_igmp_groupCbEvt_t *groupCb)
{
	int ret=0;
	rt_igmp_multicast_group_cfg_t mcConfig;
	memset(&mcConfig,0,sizeof(mcConfig));

	mcConfig.careIngressCvid=IF_VLANCARE;
	mcConfig.careSourceAddress=IF_SOURCECARE;;

	mcConfig.groupBehavior=RT_MC_BEHAVIOR_GROUP_AS_UNKNOW;
	mcConfig.is_ipv6=groupCb->isIPv6;

	if(groupCb->isIPv6)
		memcpy(&mcConfig.group_addr.ipv6[0],&groupCb->group[0],sizeof(mcConfig.group_addr));
	else
		mcConfig.group_addr.ipv4=groupCb->group[0];

	mcConfig.minusGroupRefCntOnly = groupCb->minusGroupRefCntOnly;

	ret = rt_igmp_multicastGroup_set (mcConfig);

	if(ret)
		IGMP_WARNING("ret=%d Error",ret);

	return SUCCESS;


}


typedef struct rtk_igmp_RegWK_s
{
	struct work_struct work;
}rtk_igmp_RegWK_t;

typedef struct rtk_igmp_groupCbWK_s
{
	struct work_struct work;
	rtk_igmp_groupCbEvt_t groupCb;
}rtk_igmp_groupCbWK_t;

typedef struct rtk_igmp_flowCbWK_s
{
	struct work_struct work;
	rtk_igmp_flowCbEvt_t flowCb;
}rtk_igmp_flowCbWK_t;

typedef struct rtk_igmp_unknownMcCbWK_s
{
	struct work_struct work;
	rtk_igmp_unknownMcCbEvt_t unknownCtrl;
}rtk_igmp_unknownMcCbWK_t;


atomic_t wqAllocCnt=ATOMIC_INIT(0);
atomic_t wqFreeCnt=ATOMIC_INIT(0);


void w_rtk_igmp_hwRegInit(struct work_struct *p_work)
{
	rtk_igmp_RegWK_t *p_callback_work = container_of(p_work, rtk_igmp_RegWK_t, work);
	_rtk_igmp_hwRegInit();
	atomic_inc(&wqFreeCnt);
	kfree(p_callback_work);
}
void w_rtk_igmp_hwUnRegClear(struct work_struct *p_work)
{
	rtk_igmp_RegWK_t *p_callback_work = container_of(p_work, rtk_igmp_RegWK_t, work);
	_rtk_igmp_hwUnRegClear();
	atomic_inc(&wqFreeCnt);
	kfree(p_callback_work);
}
void w_rtk_igmp_unknownIpMc_Action(struct work_struct *p_work)
{
	rtk_igmp_unknownMcCbWK_t *p_callback_work = container_of(p_work, rtk_igmp_unknownMcCbWK_t, work);
	_rtk_igmp_unknownIpMc_Action(&p_callback_work->unknownCtrl);
	atomic_inc(&wqFreeCnt);
	kfree(p_callback_work);
}
void w_rtk_igmp_flowDelCbEvt(struct work_struct *p_work)
{
	rtk_igmp_flowCbWK_t *p_callback_work = container_of(p_work, rtk_igmp_flowCbWK_t, work);
	_rtk_igmp_flowDelCbEvt(&p_callback_work->flowCb);
	atomic_inc(&wqFreeCnt);
	kfree(p_callback_work);
}
void w_rtk_igmp_flowSetCbEvt(struct work_struct *p_work)
{
	rtk_igmp_flowCbWK_t *p_callback_work = container_of(p_work, rtk_igmp_flowCbWK_t, work);
	_rtk_igmp_flowSetCbEvt(&p_callback_work->flowCb);
	atomic_inc(&wqFreeCnt);
	kfree(p_callback_work);
}
void w_rtk_igmp_groupAddCbEvt(struct work_struct *p_work)
{
	rtk_igmp_groupCbWK_t *p_callback_work = container_of(p_work, rtk_igmp_groupCbWK_t, work);
	_rtk_igmp_groupAddCbEvt(&p_callback_work->groupCb);
	atomic_inc(&wqFreeCnt);
	kfree(p_callback_work);
}
void w_rtk_igmp_groupDelCbEvt(struct work_struct *p_work)
{
	rtk_igmp_groupCbWK_t *p_callback_work = container_of(p_work, rtk_igmp_groupCbWK_t, work);
	_rtk_igmp_groupDelCbEvt(&p_callback_work->groupCb);
	atomic_inc(&wqFreeCnt);
	kfree(p_callback_work);
}


int32 rtk_igmp_hwRegInit(void)
{
	int32 ret;
	rtk_igmp_RegWK_t *work = kzalloc(sizeof(typeof(*work)), GFP_ATOMIC);
	atomic_inc(&wqAllocCnt);
	INIT_WORK(&work->work, w_rtk_igmp_hwRegInit);
	ret=schedule_work(&work->work);
	if(ret==false)
		printk("Wq Error\n");	
	return SUCCESS;
}
int32 rtk_igmp_hwUnRegClear(void)
{
	int32 ret;
	rtk_igmp_RegWK_t *work = kzalloc(sizeof(typeof(*work)), GFP_ATOMIC);
	atomic_inc(&wqAllocCnt);
	INIT_WORK(&work->work, w_rtk_igmp_hwUnRegClear);
	ret=schedule_work(&work->work);
	if(ret==false)
		printk("Wq Error\n");	
	return SUCCESS;
}
int32 rtk_igmp_unknownIpMc_Action(rtk_igmp_unknownMcCbEvt_t *unknownCtrl)
{
	int32 ret;
	rtk_igmp_unknownMcCbWK_t *work = kzalloc(sizeof(typeof(*work)), GFP_ATOMIC);
	atomic_inc(&wqAllocCnt);
	INIT_WORK(&work->work, w_rtk_igmp_unknownIpMc_Action);
	memcpy(&work->unknownCtrl,unknownCtrl,sizeof(work->unknownCtrl));
	ret=schedule_work(&work->work);
	if(ret==false)
		printk("Wq Error\n");	
	return SUCCESS;
}
int32 rtk_igmp_flowDelCbEvt(rtk_igmp_flowCbEvt_t *flowCb)
{
	int32 ret;
	rtk_igmp_flowCbWK_t *work = kzalloc(sizeof(typeof(*work)), GFP_ATOMIC);
	atomic_inc(&wqAllocCnt);
	INIT_WORK(&work->work, w_rtk_igmp_flowDelCbEvt);
	memcpy(&work->flowCb,flowCb,sizeof(work->flowCb));
	ret=schedule_work(&work->work);
	if(ret==false)
		printk("Wq Error\n");	
	return SUCCESS;
}
int32 rtk_igmp_flowSetCbEvt(rtk_igmp_flowCbEvt_t *flowCb)
{
	int32 ret;
	rtk_igmp_flowCbWK_t *work = kzalloc(sizeof(typeof(*work)), GFP_ATOMIC);
	atomic_inc(&wqAllocCnt);
	INIT_WORK(&work->work, w_rtk_igmp_flowSetCbEvt);
	memcpy(&work->flowCb,flowCb,sizeof(work->flowCb));
	ret=schedule_work(&work->work);
	if(ret==false)
		printk("Wq Error\n");	
	return SUCCESS;
}
int32 rtk_igmp_groupAddCbEvt(rtk_igmp_groupCbEvt_t *groupCb)
{
	int32 ret;
	rtk_igmp_groupCbWK_t *work = kzalloc(sizeof(typeof(*work)), GFP_ATOMIC);
	atomic_inc(&wqAllocCnt);
	INIT_WORK(&work->work, w_rtk_igmp_groupAddCbEvt);
	memcpy(&work->groupCb,groupCb,sizeof(work->groupCb));	
	ret=schedule_work(&work->work);
	if(ret==false)
		printk("Wq Error\n");	
	return SUCCESS;
}
int32 rtk_igmp_groupDelCbEvt(rtk_igmp_groupCbEvt_t *groupCb)
{
	int32 ret;
	rtk_igmp_groupCbWK_t *work = kzalloc(sizeof(typeof(*work)), GFP_ATOMIC);
	atomic_inc(&wqAllocCnt);
	INIT_WORK(&work->work, w_rtk_igmp_groupDelCbEvt);
	memcpy(&work->groupCb,groupCb,sizeof(work->groupCb));	
	ret=schedule_work(&work->work);
	if(ret==false)
		printk("Wq Error\n");
	return SUCCESS;
}

