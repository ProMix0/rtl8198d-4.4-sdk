#include "rtk_acl.h"
#include "rtk_api.h"
#include "rtl8367c_asicdrv_meter.h"
//#include "rtk_api_ext.h"

rtl83xx_acl_rule_header_t rtl83xx_acl_tbl;
static rtl83xx_acl_rule_header_t rtl83xx_free_acl_entry_header;
static rtl83xx_acl_rule_t *rtl83xx_acl_entry_pool=NULL;
rtk_uint8 free_ip_range_index_pool[RTL83XX_ACLRANGEMAX] = {0};
rtk_uint8 free_vid_range_index_pool[RTL83XX_ACLRANGEMAX] = {0};
rtk_uint8 free_port_range_index_pool[RTL83XX_ACLRANGEMAX] = {0};
rtk_filter_field_t	filter_field[MAX_FILTER_NUM];

extern rtk_uint32 vportmask;

//check ok
static rtk_int32 _rtl83xx_initAclRulePool(void)
{
	rtk_int32 i;

	CLIST_INIT(&rtl83xx_free_acl_entry_header);

	TBL_MEM_ALLOC(rtl83xx_acl_entry_pool, rtl83xx_acl_rule_t, RTL83XX_ACLRULENO);
	
	if(rtl83xx_acl_entry_pool!=NULL)
	{
		memset(rtl83xx_acl_entry_pool, 0, RTL83XX_ACLRULENO * sizeof(rtl83xx_acl_rule_t));	
	}
	else
	{
		return RT_ERR_FAILED;
	}
	
	for(i = 0; i<RTL83XX_ACLRULENO;i++)
	{
		CLIST_INSERT_HEAD(&rtl83xx_free_acl_entry_header, &rtl83xx_acl_entry_pool[i], next_rule);
	}
	
	return RT_ERR_OK;
}

//check ok
static rtk_int32 _rtl83xx_isSameAcl(rtl83xx_acl_rule_t *rule1, rtl83xx_acl_rule_t *rule2, rtk_uint32 flag)
{
	rtk_int32 ret=1, checked=0;
	
	if(flag&COMPARE_FLAG_PRIO)
	{
		if(rule1->prio != rule2->prio)
			ret = 0;
		checked = 1;
	}
	if(flag&COMPARE_FLAG_MEMBER)
	{
		if(rule1->member != rule2->member)
			ret = 0;
		checked = 1;
	}

	if(flag&COMPARE_FLAG_INVERT)
	{
		if(rule1->invert != rule2->invert)
			ret = 0;
		checked = 1;
	}
	
	if(flag&COMPARE_FLAG_FILTER)
	{
		if(memcmp(&rule1->filter, &rule2->filter, sizeof(rtl83xx_acl_filter_t)))
			ret = 0;
		checked = 1;
	}
	
	if(flag&COMPARE_FLAG_ACTION)
	{
		if(memcmp(&rule1->action, &rule2->action, sizeof(rtl83xx_acl_action_t)))
			ret = 0;
		checked = 1;
	}

	if(checked)
		return ret;
	else
		return 0;
}

//check ok
static rtl83xx_acl_rule_t *_rtl83xx_allocAclRuleEntry(void)
{
	rtl83xx_acl_rule_t *newRule=NULL;
	newRule = CLIST_FIRST(&rtl83xx_free_acl_entry_header);
	if(newRule!=NULL)
	{
		CLIST_REMOVE(&rtl83xx_free_acl_entry_header, newRule, next_rule);
		memset(newRule,0,sizeof(rtl83xx_acl_rule_t));
	}
	return newRule;
}

//check ok
static rtk_int32 _rtl83xx_freeAclRuleEntry(rtl83xx_acl_rule_t *aclPtr)
{
	if(aclPtr==NULL)
	{
		return RT_ERR_FAILED;
	}
	memset(aclPtr,0,sizeof(rtl83xx_acl_rule_t));
	CLIST_INSERT_HEAD(&rtl83xx_free_acl_entry_header, aclPtr, next_rule);
	
	return RT_ERR_OK;
}

//check ok
static rtk_int32 _rtl83xx_flushAclRuleEntry(rtl83xx_acl_rule_header_t * aclChainHead)
{
	rtl83xx_acl_rule_t *curAcl,*nextAcl;
	
	if(aclChainHead==NULL)
	{
		return RT_ERR_FAILED;
	}
	
	curAcl=CLIST_FIRST(aclChainHead);
	while(curAcl)
	{
		nextAcl=CLIST_NEXT(curAcl, next_rule);
		/*remove from the old descriptor chain*/
		CLIST_REMOVE(aclChainHead, curAcl, next_rule);
		/*return to the free descriptor chain*/
		_rtl83xx_freeAclRuleEntry(curAcl);
		curAcl = nextAcl;
	}

	return RT_ERR_OK;
}

//check ok
static rtk_int32 _rtl83xx_flushAclRuleEntrybyPrio(rtl83xx_acl_rule_header_t *aclChainHead, rtk_int32 prio)
{
	rtl83xx_acl_rule_t *curAcl,*nextAcl;
	
	if(aclChainHead==NULL)
	{
		return RT_ERR_FAILED;
	}
	
	curAcl=CLIST_FIRST(aclChainHead);
	while(curAcl)
	{
		nextAcl=CLIST_NEXT(curAcl, next_rule);
		if(curAcl->prio == prio)
		{
			CLIST_REMOVE(aclChainHead, curAcl, next_rule);
			_rtl83xx_freeAclRuleEntry(curAcl);
		}
		curAcl = nextAcl;
	}

	return RT_ERR_OK;
}

static rtk_int32 _rtl83xx_flushAclRuleEntrybyFlag(rtl83xx_acl_rule_header_t *aclChainHead, rtk_uint32 flag, rtl83xx_acl_rule_t *compareAcl)
{
	rtl83xx_acl_rule_t *curAcl,*nextAcl;
	
	if(aclChainHead==NULL)
	{
		return RT_ERR_FAILED;
	}
	
	curAcl=CLIST_FIRST(aclChainHead);
	while(curAcl)
	{
		nextAcl=CLIST_NEXT(curAcl, next_rule);
		if(_rtl83xx_isSameAcl(curAcl, compareAcl, flag))
		{
			CLIST_REMOVE(aclChainHead, curAcl, next_rule);
			_rtl83xx_freeAclRuleEntry(curAcl);
		}
		curAcl = nextAcl;
	}

	return RT_ERR_OK;
}

//check ok
static rtk_int32 _rtl83xx_aclRuleEntryEnqueue(rtl83xx_acl_rule_header_t *aclChainHead,
												 rtl83xx_acl_rule_t *enqueueAcl)
{

	rtl83xx_acl_rule_t *newAcl;
	rtl83xx_acl_rule_t *curAcl,*nextAcl;
	rtl83xx_acl_rule_t *lastAcl;

	if(aclChainHead==NULL)
	{
		return RT_ERR_FAILED;
	}
	
	if(enqueueAcl==NULL)
	{
		return RT_ERR_FAILED;
	}

	for(curAcl=CLIST_FIRST(aclChainHead); curAcl!=NULL; curAcl=nextAcl)
	{

		nextAcl=CLIST_NEXT(curAcl, next_rule);

		if(_rtl83xx_isSameAcl(curAcl, enqueueAcl, COMPARE_FLAG_ALL))			
			break;
	}
	if(curAcl)
	{
		//already exist
		return RT_ERR_OK;
	}
	
	newAcl=_rtl83xx_allocAclRuleEntry();
	
	if(newAcl!=NULL)
	{
		memcpy(newAcl, enqueueAcl,sizeof(rtl83xx_acl_rule_t ));
		newAcl->next_rule.le_next=NULL;
		newAcl->next_rule.le_prev=NULL;
	}
	else
	{
		/*no enough memory*/
		printk("not enough 83xx acl entry!\n");
		return RT_ERR_FAILED;
	}
	
	for(curAcl=CLIST_FIRST(aclChainHead); curAcl!=NULL; curAcl=nextAcl)
	{

		nextAcl=CLIST_NEXT(curAcl, next_rule);
		lastAcl=curAcl;
		
		if(curAcl->prio > newAcl->prio)
		{	
			break;
		}
	}

	if(curAcl)
	{
		//find a entry has larger prio value(lower priority)
		CLIST_INSERT_BEFORE(aclChainHead,curAcl,newAcl,next_rule);
	}
	else
	{
		
		if(CLIST_EMPTY(aclChainHead))
		{
			CLIST_INSERT_HEAD(aclChainHead, newAcl, next_rule);
		}
		else
		{
			CLIST_INSERT_AFTER(aclChainHead,lastAcl,newAcl,next_rule);
		}
	}

	return RT_ERR_OK;
	
}
												 
//check ok
static rtk_int32 _rtl83xx_aclRuleEntryDequeue(rtl83xx_acl_rule_header_t *aclChainHead,
												 rtl83xx_acl_rule_t *dequeueAcl)
{
	rtl83xx_acl_rule_t *curAcl,*nextAcl;

	if(aclChainHead==NULL)
	{
		return RT_ERR_FAILED;
	}
	
	if(dequeueAcl==NULL)
	{
		return RT_ERR_FAILED;
	}	

	for(curAcl=CLIST_FIRST(aclChainHead); curAcl!=NULL; curAcl=nextAcl)
	{

		nextAcl=CLIST_NEXT(curAcl, next_rule);

		if(_rtl83xx_isSameAcl(curAcl, dequeueAcl, COMPARE_FLAG_ALL))			
			break;
	}

	if(curAcl)
	{
		//find the delete entry in aclChainHeadList
		CLIST_REMOVE(aclChainHead,curAcl,next_rule);
		_rtl83xx_freeAclRuleEntry(curAcl);
	}

	return RT_ERR_OK;
	
}
												 
//check ok
static rtk_int32 _rtl83xx_allocIpRangeIndex(void)
{
	rtk_int32 i;
	for(i=0; i<RTL83XX_ACLRANGEMAX; i++)
	{
		if(free_ip_range_index_pool[i]==0)
		{
			free_ip_range_index_pool[i]=1;
			return i;
		}
	}
	return -1;
}

//check ok
static rtk_int32 _rtl83xx_freeIpRangeIndex(rtk_uint8 ip_range_index)
{
	if(ip_range_index>=RTL83XX_ACLRANGEMAX)
		return RT_ERR_FAILED;

	free_ip_range_index_pool[ip_range_index] = 0;
	rtk_filter_iprange_set(ip_range_index, IPRANGE_UNUSED, 0, 0);
	return RT_ERR_OK;
}

//check ok
static rtk_int32 _rtl83xx_resetIpRangeIndex(void)
{
	rtk_uint32 index;
	memset(free_ip_range_index_pool, 0, RTL83XX_ACLRANGEMAX*sizeof(rtk_uint8));
	for(index=0; index<=RTL83XX_ACLRANGEMAX; index++)
	{
		rtk_filter_iprange_set(index, IPRANGE_UNUSED, 0, 0);
	}
	return RT_ERR_OK;
}

//check ok
#if 0//complier error:defined but not used
static rtk_int32 _rtl83xx_allocVidRangeIndex(void)
{
	rtk_int32 i;
	for(i=0; i<RTL8367B_ACLRANGEMAX; i++)
	{
		if(free_vid_range_index_pool[i]==0)
		{
			free_vid_range_index_pool[i]=1;
			return i;
		}
	}
	return -1;
}
//check ok
static rtk_int32 _rtl83xx_freeVidRangeIndex(rtk_uint8 vid_range_index)
{
	if(vid_range_index>=RTL8367B_ACLRANGEMAX)
		return RT_ERR_FAILED;

	free_vid_range_index_pool[vid_range_index] = 0;
	rtk_filter_vidrange_set(vid_range_index, VIDRANGE_UNUSED, 0, 0);
	return RT_ERR_OK;
}
#endif

//check ok
static rtk_int32 _rtl83xx_resetVidRangeIndex(void)
{
	rtk_uint32 index=0;

	memset(free_vid_range_index_pool, 0, RTL83XX_ACLRANGEMAX*sizeof(rtk_uint8));
	for(index=0; index<=RTL83XX_ACLRANGEMAX; index++)
	{
		rtk_filter_vidrange_set(index, VIDRANGE_UNUSED, 0, 0);
	}
	return RT_ERR_OK;
}

//check ok
static rtk_int32 _rtl83xx_allocPortRangeIndex(void)
{
	rtk_int32 i;
	for(i=0; i<RTL83XX_ACLRANGEMAX; i++)
	{
		if(free_port_range_index_pool[i]==0)
		{
			free_port_range_index_pool[i]=1;
			return i;
		}
	}
	return -1;
}

//check ok
static rtk_int32 _rtl83xx_freePortRangeIndex(rtk_uint8 port_range_index)
{
	if(port_range_index>=RTL83XX_ACLRANGEMAX)
		return RT_ERR_FAILED;

	free_port_range_index_pool[port_range_index] = 0;
	rtk_filter_portrange_set(port_range_index, PORTRANGE_UNUSED, 0, 0);
	return RT_ERR_OK;
}

//check ok
static rtk_int32 _rtl83xx_resetPortRangeIndex(void)
{
	rtk_uint32 index;
	memset(free_port_range_index_pool, 0, RTL83XX_ACLRANGEMAX*sizeof(rtk_uint8));
	for(index=0; index<=RTL83XX_ACLRANGEMAX; index++)
	{
		rtk_filter_portrange_set(index, PORTRANGE_UNUSED, 0, 0);
	}
	return RT_ERR_OK;
}

//check ok
static rtk_filter_act_enable_t _rtl83xx_getActionType(rtk_uint32 type)
{
	rtk_filter_act_enable_t rtl83xx_type;
	if(type >= RTL83XX_ACL_END)
		return FILTER_ENACT_END;
	
	switch(type)
	{
		case RTL83XX_ACL_COPY_CPU:
			rtl83xx_type =  FILTER_ENACT_COPY_CPU;
			break;
		case RTL83XX_ACL_DROP:
			rtl83xx_type =  FILTER_ENACT_DROP;
			break;
		case RTL83XX_ACL_REDIRECT:
			rtl83xx_type =  FILTER_ENACT_REDIRECT;
			break;
		case RTL83XX_ACL_MIRROR:
			rtl83xx_type =  FILTER_ENACT_MIRROR;
			break;
		case RTL83XX_ACL_TRAP_CPU:
			rtl83xx_type =  FILTER_ENACT_TRAP_CPU;
			break;
		case RTL83XX_ACL_PRIORITY:
			rtl83xx_type =  FILTER_ENACT_PRIORITY;
			break;
		case RTL83XX_ACL_DSCP_REMARK:
			rtl83xx_type =  FILTER_ENACT_DSCP_REMARK;
			break;		
		case RTL83XX_ACL_1P_REMARK:
			rtl83xx_type =  FILTER_ENACT_1P_REMARK;
			break;
		case RTL83XX_ACL_POLICING0:
			rtl83xx_type =  FILTER_ENACT_POLICING_0;
			break;
		default:
			rtl83xx_type =  FILTER_ENACT_END;
			break;
	}

	return rtl83xx_type; 
			
}

//check ok
static rtk_int32 _rtl83xx_isValidFilterIndex(rtk_filter_id_t filter_id)
{
	if(filter_id<RTL83XX_ACLRULENO)
		return 1;
	else
		return 0;
}

/*	sync acl rule to asic
	input: aclRule->index...
	output: 	aclRule->size, aclRule->in_asic
*/
//check ok
static rtk_int32 _rtl83xx_syncAclRuleToAsic(rtl83xx_acl_rule_t *aclRule)
{
	rtk_api_ret_t retVal;
	//rtk_filter_field_t	filter_field[10];
	rtk_filter_cfg_t	cfg;
	rtk_filter_action_t act;
	rtk_filter_number_t filterNum=0;
	rtk_filter_act_enable_t act_enable;
	
	memset(filter_field, 0, MAX_FILTER_NUM*sizeof(rtk_filter_field_t));
	memset(&cfg, 0, sizeof(rtk_filter_cfg_t));
	memset(&act, 0, sizeof(rtk_filter_action_t));

	filterNum = 0;
		//all packets
	if((INVALID_MAC(aclRule->filter.smac_mask.octet))&&
	    (INVALID_MAC(aclRule->filter.dmac_mask.octet))&&
	    (aclRule->filter.ethertype_mask==0)&&
	    (aclRule->filter.dip_type==FILTER_NONE)&&
	    (aclRule->filter.sip_type==FILTER_NONE)&&
	    (aclRule->filter.ip_tos_mask==0)&&
	    (aclRule->filter.protocal_mask==0)&&
	    (aclRule->filter.sport_type==FILTER_NONE)&&
	    (aclRule->filter.dport_type==FILTER_NONE)&&
	    (aclRule->filter.vlanpri_mask==0))
	{
		filter_field[0].fieldType=FILTER_FIELD_DMAC;
		if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[0])) != RT_ERR_OK)
			return retVal;
	}
	else
	{
		//1.src mac
		if(!INVALID_MAC(aclRule->filter.smac_mask.octet))
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			
			filter_field[filterNum].fieldType = FILTER_FIELD_SMAC;
			filter_field[filterNum].filter_pattern_union.smac.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.smac.value.octet[0] = aclRule->filter.smac.octet[0];
			filter_field[filterNum].filter_pattern_union.smac.value.octet[1] = aclRule->filter.smac.octet[1];
			filter_field[filterNum].filter_pattern_union.smac.value.octet[2] = aclRule->filter.smac.octet[2];
			filter_field[filterNum].filter_pattern_union.smac.value.octet[3] = aclRule->filter.smac.octet[3];
			filter_field[filterNum].filter_pattern_union.smac.value.octet[4] = aclRule->filter.smac.octet[4];
			filter_field[filterNum].filter_pattern_union.smac.value.octet[5] = aclRule->filter.smac.octet[5];
		
			filter_field[filterNum].filter_pattern_union.smac.mask.octet[0] = aclRule->filter.smac_mask.octet[0];
			filter_field[filterNum].filter_pattern_union.smac.mask.octet[1] = aclRule->filter.smac_mask.octet[1];
			filter_field[filterNum].filter_pattern_union.smac.mask.octet[2] = aclRule->filter.smac_mask.octet[2];
			filter_field[filterNum].filter_pattern_union.smac.mask.octet[3] = aclRule->filter.smac_mask.octet[3];
			filter_field[filterNum].filter_pattern_union.smac.mask.octet[4] = aclRule->filter.smac_mask.octet[4];
			filter_field[filterNum].filter_pattern_union.smac.mask.octet[5] = aclRule->filter.smac_mask.octet[5];
			filter_field[filterNum].next = NULL;	

			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;

			filterNum++;
		}

		//2.dest mac
		if(!INVALID_MAC(aclRule->filter.dmac_mask.octet))
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			
			filter_field[filterNum].fieldType = FILTER_FIELD_DMAC;
			filter_field[filterNum].filter_pattern_union.dmac.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.dmac.value.octet[0] = aclRule->filter.dmac.octet[0];
			filter_field[filterNum].filter_pattern_union.dmac.value.octet[1] = aclRule->filter.dmac.octet[1];
			filter_field[filterNum].filter_pattern_union.dmac.value.octet[2] = aclRule->filter.dmac.octet[2];
			filter_field[filterNum].filter_pattern_union.dmac.value.octet[3] = aclRule->filter.dmac.octet[3];
			filter_field[filterNum].filter_pattern_union.dmac.value.octet[4] = aclRule->filter.dmac.octet[4];
			filter_field[filterNum].filter_pattern_union.dmac.value.octet[5] = aclRule->filter.dmac.octet[5];
		
			filter_field[filterNum].filter_pattern_union.dmac.mask.octet[0] = aclRule->filter.dmac_mask.octet[0];
			filter_field[filterNum].filter_pattern_union.dmac.mask.octet[1] = aclRule->filter.dmac_mask.octet[1];
			filter_field[filterNum].filter_pattern_union.dmac.mask.octet[2] = aclRule->filter.dmac_mask.octet[2];
			filter_field[filterNum].filter_pattern_union.dmac.mask.octet[3] = aclRule->filter.dmac_mask.octet[3];
			filter_field[filterNum].filter_pattern_union.dmac.mask.octet[4] = aclRule->filter.dmac_mask.octet[4];
			filter_field[filterNum].filter_pattern_union.dmac.mask.octet[5] = aclRule->filter.dmac_mask.octet[5];
			filter_field[filterNum].next = NULL;	

			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;

			filterNum++;
		}

		//3.ethernet type
		if(aclRule->filter.ethertype_mask)
		{		
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			
			filter_field[filterNum].fieldType = FILTER_FIELD_ETHERTYPE;
			filter_field[filterNum].filter_pattern_union.etherType.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.etherType.value = aclRule->filter.ethertype;
			filter_field[filterNum].filter_pattern_union.etherType.mask = aclRule->filter.ethertype_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;
			filterNum++;
		}
		
		//4.sip
		if(aclRule->filter.sip_type==FILTER_MASK && aclRule->filter.sip_mask)
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			filter_field[filterNum].fieldType = FILTER_FIELD_IPV4_SIP;
			filter_field[filterNum].filter_pattern_union.sip.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.sip.value = aclRule->filter.sip;
			filter_field[filterNum].filter_pattern_union.sip.mask = aclRule->filter.sip_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;
			filterNum++;
		}
		
		//5.dip
		if(aclRule->filter.dip_type==FILTER_MASK && aclRule->filter.dip_mask)
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			filter_field[filterNum].fieldType = FILTER_FIELD_IPV4_DIP;
			filter_field[filterNum].filter_pattern_union.dip.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.dip.value = aclRule->filter.dip;
			filter_field[filterNum].filter_pattern_union.dip.mask = aclRule->filter.dip_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;
			filterNum++;
		}

		//6.ip range
		if((aclRule->filter.sip_type==FILTER_RANGE)||(aclRule->filter.dip_type==FILTER_RANGE))
		{
			rtk_int32 range_sindex=-1, range_dindex=-1, range_mask=0;

			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}

			if(aclRule->filter.sip_type==FILTER_RANGE && aclRule->filter.sip<=aclRule->filter.sip_mask)
			{
				range_sindex = _rtl83xx_allocIpRangeIndex();
				if(range_sindex == -1)
					return RT_ERR_OUT_OF_RANGE;

				if ((retVal = rtk_filter_iprange_set(range_sindex, IPRANGE_IPV4_SIP, aclRule->filter.sip_mask, aclRule->filter.sip)) != RT_ERR_OK)
				{
					_rtl83xx_freeIpRangeIndex(range_sindex);
					return retVal;
				}
				range_mask |= 1<<range_sindex;
			}

			if(aclRule->filter.dip_type==FILTER_RANGE && aclRule->filter.dip<=aclRule->filter.dip_mask)
			{
				range_dindex = _rtl83xx_allocIpRangeIndex();

				if(range_dindex == -1)
					return RT_ERR_OUT_OF_RANGE;
			
				if ((retVal = rtk_filter_iprange_set(range_dindex, IPRANGE_IPV4_DIP, aclRule->filter.dip_mask, aclRule->filter.dip)) != RT_ERR_OK)
				{
					_rtl83xx_freeIpRangeIndex(range_dindex);
					return retVal;
				}
				range_mask |= 1<<range_dindex;
			}		

			if(range_mask==0)
			{
				printk("wrong ip range parameters!\n");
				return RT_ERR_INPUT;
			}

			filter_field[filterNum].fieldType = FILTER_FIELD_IP_RANGE;
			filter_field[filterNum].filter_pattern_union.inData.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.inData.value = range_mask; //bit mask
			filter_field[filterNum].filter_pattern_union.inData.mask = range_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
			{	
				if(range_sindex!=-1)
					_rtl83xx_freeIpRangeIndex(range_sindex);
				if(range_dindex!=-1)
					_rtl83xx_freeIpRangeIndex(range_dindex);
				return retVal;
			}
			filterNum++;
		}

		//7.ip tos(dhcp)
		if(aclRule->filter.ip_tos_mask)
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			filter_field[filterNum].fieldType = FILTER_FIELD_IPV4_TOS;
			filter_field[filterNum].filter_pattern_union.ipTos.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.ipTos.value = aclRule->filter.ip_tos;
			filter_field[filterNum].filter_pattern_union.ipTos.mask = aclRule->filter.ip_tos_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;
			filterNum++;
		}

		//8.protocol
		if(aclRule->filter.protocal_mask)
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			filter_field[filterNum].fieldType = FILTER_FIELD_IPV4_PROTOCOL;
			filter_field[filterNum].filter_pattern_union.protocol.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.protocol.value = aclRule->filter.protocal;
			filter_field[filterNum].filter_pattern_union.protocol.mask = aclRule->filter.protocal_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;
			filterNum++;
		}

		//9.src port
		if(aclRule->filter.sport_type==FILTER_MASK && aclRule->filter.sport_mask)
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			filter_field[filterNum].fieldType = FILTER_FIELD_UDP_SPORT;
			filter_field[filterNum].filter_pattern_union.udpSrcPort.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.udpSrcPort.value = aclRule->filter.sport;
			filter_field[filterNum].filter_pattern_union.udpSrcPort.mask = aclRule->filter.sport_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;
			filterNum++;
		}

		//10.dest port
		if(aclRule->filter.dport_type==FILTER_MASK && aclRule->filter.dport_mask)
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			filter_field[filterNum].fieldType = FILTER_FIELD_UDP_DPORT;
			filter_field[filterNum].filter_pattern_union.udpDstPort.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.udpDstPort.value = aclRule->filter.dport;
			filter_field[filterNum].filter_pattern_union.udpDstPort.mask = aclRule->filter.dport_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;
			filterNum++;
		}
		
		//11.port range
		if((aclRule->filter.sport_type==FILTER_RANGE) || (aclRule->filter.dport_type==FILTER_RANGE))
		{
			rtk_int32 range_sindex=-1, range_dindex=-1, range_mask=0;
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			if((aclRule->filter.sport_type==FILTER_RANGE) && (aclRule->filter.sport<=aclRule->filter.sport_mask))
			{
				range_sindex = _rtl83xx_allocPortRangeIndex();
				if(range_sindex==-1)
					return RT_ERR_OUT_OF_RANGE;

				if ((retVal = rtk_filter_portrange_set(range_sindex, PORTRANGE_SPORT, aclRule->filter.sport_mask, aclRule->filter.sport)) != RT_ERR_OK)
				{
					_rtl83xx_freePortRangeIndex(range_sindex);
					return retVal;
				}
				range_mask |= 1<<range_sindex;
			}

			if((aclRule->filter.dport_type==FILTER_RANGE) && (aclRule->filter.dport<=aclRule->filter.dport_mask))
			{
				range_dindex = _rtl83xx_allocPortRangeIndex();
				if(range_dindex==-1)
					return RT_ERR_OUT_OF_RANGE;

				if ((retVal = rtk_filter_portrange_set(range_dindex, PORTRANGE_DPORT, aclRule->filter.dport_mask, aclRule->filter.dport)) != RT_ERR_OK)
				{
					_rtl83xx_freePortRangeIndex(range_dindex);
					return retVal;
				}
				range_mask |= 1<<range_dindex;
			}

			if(range_mask==0)
			{
				printk("wrong port range parameters!\n");
				return RT_ERR_INPUT;
			}

			filter_field[filterNum].fieldType = FILTER_FIELD_PORT_RANGE;
			filter_field[filterNum].filter_pattern_union.inData.dataType = FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.inData.value = range_mask; //bit mask
			filter_field[filterNum].filter_pattern_union.inData.mask = range_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
			{	
				if(range_sindex!=-1)
					_rtl83xx_freePortRangeIndex(range_sindex);
				if(range_dindex!=-1)
					_rtl83xx_freePortRangeIndex(range_dindex);
					
				return retVal;
			}
			filterNum++;

		}

		//12.1p
		if(aclRule->filter.vlanpri_mask)
		{
			if(filterNum>=MAX_FILTER_NUM)
			{
				printk("too much filter!\n");
				return RT_ERR_OUT_OF_RANGE;
			}
			filter_field[filterNum].fieldType = FILTER_FIELD_CTAG;
			filter_field[filterNum].filter_pattern_union.ctag.pri.dataType=FILTER_FIELD_DATA_MASK;
			filter_field[filterNum].filter_pattern_union.ctag.pri.value=aclRule->filter.vlanpri;
			filter_field[filterNum].filter_pattern_union.ctag.pri.mask=aclRule->filter.vlanpri_mask;
			filter_field[filterNum].next = NULL;
			if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[filterNum])) != RT_ERR_OK)
				return retVal;
			filterNum++;
		}
	}

	cfg.activeport.value.bits[0] = aclRule->member;
	cfg.activeport.mask.bits[0] = vportmask;
	cfg.invert = aclRule->invert;

	act_enable = _rtl83xx_getActionType(aclRule->action.act_type);
	if(act_enable==FILTER_ENACT_END)
	{
		//printk("not support the acl type yet!")
		return RT_ERR_INPUT;
	}
	act.actEnable[act_enable] = TRUE;
	if(act_enable==FILTER_ENACT_PRIORITY)
		act.filterPriority = aclRule->action.priority;
	else if(act_enable==FILTER_ENACT_POLICING_0){
		if ((retVal = rtl8367c_setAsicShareMeter(aclRule->action.meter_index, aclRule->action.policing >> 3, 1)) != RT_ERR_OK)
			return retVal;
		act.filterPolicingIdx[0] = aclRule->action.meter_index;
	}

//	printk("index:%d, [%s:%d]\n", aclRule->index, __FUNCTION__, __LINE__);
	if ((retVal = rtk_filter_igrAcl_cfg_add(aclRule->index, &cfg, &act, &aclRule->size)) != RT_ERR_OK)
	{
		//printk("retVal:%d, [%s:%d]\n" , retVal, __FUNCTION__, __LINE__);
		return retVal;
	}

	aclRule->in_asic = 1;
	
	return RT_ERR_OK;

}

rtk_int32 rtl83xx_init_acl(void)
{
	CLIST_INIT(&rtl83xx_acl_tbl);
	_rtl83xx_initAclRulePool();
	_rtl83xx_resetIpRangeIndex();
	_rtl83xx_resetPortRangeIndex();
	_rtl83xx_resetVidRangeIndex();
	return 1;
}

rtk_int32 rtl83xx_addAclRule(rtl83xx_acl_rule_t *aclRule)
{
	return _rtl83xx_aclRuleEntryEnqueue(&rtl83xx_acl_tbl, aclRule);
}

rtk_int32 rtl83xx_deleteAclRule(rtl83xx_acl_rule_t *aclRule)
{
	return _rtl83xx_aclRuleEntryDequeue(&rtl83xx_acl_tbl, aclRule);
}

rtk_int32 rtl83xx_flushAclRule(void)
{
	return _rtl83xx_flushAclRuleEntry(&rtl83xx_acl_tbl);
}

rtk_int32 rtl83xx_flushAclRulebyPrio(rtk_int32 prio)
{
	return _rtl83xx_flushAclRuleEntrybyPrio(&rtl83xx_acl_tbl, prio);
}
rtk_int32 rtl83xx_flushAclRulebyFlag(rtk_uint32 flag, rtl83xx_acl_rule_t *compareAcl)
{
	return _rtl83xx_flushAclRuleEntrybyFlag(&rtl83xx_acl_tbl, flag, compareAcl);
}

rtk_int32 rtl83xx_syncAclTblToAsic(void)
{
	rtl83xx_acl_rule_t *curAcl;
	rtk_uint32 curfilter = 0;

	rtk_filter_igrAcl_cfg_delAll();
	_rtl83xx_resetPortRangeIndex();
	_rtl83xx_resetVidRangeIndex();
	_rtl83xx_resetIpRangeIndex();
	
	CLIST_FOREACH(curAcl,&rtl83xx_acl_tbl,next_rule)
	{
		if(_rtl83xx_isValidFilterIndex(curfilter)==0)
			return RT_ERR_FAILED;
		
		curAcl->index = curfilter;
		curAcl->in_asic = 0;
		curAcl->size = 0;
		
		_rtl83xx_syncAclRuleToAsic(curAcl);
		if(curAcl->in_asic)
		{
			curfilter += curAcl->size;
		}
		else
			return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}
int rtl83xx_acl_test(void)
{
	int retVal;
	rtk_filter_field_t	filter_field[2];
	rtk_filter_cfg_t	cfg;
	rtk_filter_action_t act;
	rtk_filter_number_t ruleNum = 0;
	
	memset(filter_field, 0, 2*sizeof(rtk_filter_field_t));
	memset(&cfg, 0, sizeof(rtk_filter_cfg_t));
	memset(&act, 0, sizeof(rtk_filter_action_t));

#if 0
	//mac test
	filter_field[0].fieldType = FILTER_FIELD_DMAC;
	filter_field[0].filter_pattern_union.dmac.dataType = FILTER_FIELD_DATA_MASK;
	filter_field[0].filter_pattern_union.dmac.value.octet[0] = 0xF0;
	filter_field[0].filter_pattern_union.dmac.value.octet[1] = 0x1F;
	filter_field[0].filter_pattern_union.dmac.value.octet[2] = 0xAF;
	filter_field[0].filter_pattern_union.dmac.value.octet[3] = 0x67;
	filter_field[0].filter_pattern_union.dmac.value.octet[4] = 0x71;
	filter_field[0].filter_pattern_union.dmac.value.octet[5] = 0x99;
	
	filter_field[0].filter_pattern_union.dmac.mask.octet[0] = 0xFF;
	filter_field[0].filter_pattern_union.dmac.mask.octet[1] = 0xFF;
	filter_field[0].filter_pattern_union.dmac.mask.octet[2] = 0xFF;
	filter_field[0].filter_pattern_union.dmac.mask.octet[3] = 0xFF;
	filter_field[0].filter_pattern_union.dmac.mask.octet[4] = 0xFF;
	filter_field[0].filter_pattern_union.dmac.mask.octet[5] = 0xFF;
	filter_field[0].next = NULL;	

	if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[0])) != RT_ERR_OK)
		return retVal;

	//sip
	filter_field[0].fieldType = FILTER_FIELD_IPV4_SIP;
	filter_field[0].filter_pattern_union.sip.dataType = FILTER_FIELD_DATA_MASK;
	filter_field[0].filter_pattern_union.sip.value = 0xc0a80196;
	filter_field[0].filter_pattern_union.sip.mask = 0xFFFFFFFF;
	filter_field[0].next = NULL;
	if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[1])) != RT_ERR_OK)
		return retVal;

	//dhcp
	filter_field[0].fieldType = FILTER_FIELD_IPV4_TOS;
	filter_field[0].filter_pattern_union.ipTos.dataType = FILTER_FIELD_DATA_MASK;
	filter_field[0].filter_pattern_union.ipTos.value = 0xF0;
	filter_field[0].filter_pattern_union.ipTos.mask = 0xFC;
	filter_field[0].next = NULL;
	if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[2])) != RT_ERR_OK)
		return retVal;

	//protocol
	filter_field[0].fieldType = FILTER_FIELD_IPV4_PROTOCOL;
	filter_field[0].filter_pattern_union.protocol.dataType = FILTER_FIELD_DATA_MASK;
	filter_field[0].filter_pattern_union.protocol.value = 0x11;
	filter_field[0].filter_pattern_union.protocol.mask = 0xFF;
	filter_field[0].next = NULL;
	if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[3])) != RT_ERR_OK)
		return retVal;

	//port	
	/*if you want to identify tcp port or udp port ,the protocol must be used at the same time.
	  or use cared tag*/
	filter_field[0].fieldType = FILTER_FIELD_UDP_SPORT;
	filter_field[0].filter_pattern_union.udpSrcPort.dataType = FILTER_FIELD_DATA_MASK;
	filter_field[0].filter_pattern_union.udpSrcPort.value = 2000;
	filter_field[0].filter_pattern_union.udpSrcPort.mask = 0xFFFF;
	filter_field[0].next = NULL;
	if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[4])) != RT_ERR_OK)
		return retVal;
#endif

#if 0
	//dip range
	/*inData.mask indicates the bits cared.
	   if the bit is 1, the packets within the ip range are matched, 
	   otherwise the packets beyond the ip range are matched*/

	ipaddr_t lowerip, upperip;
	lowerip = 0xc0a801a0;
	upperip = 0xc0a801b0;
	if ((retVal = rtk_filter_iprange_set(1, IPRANGE_IPV4_DIP, upperip, lowerip)) != RT_ERR_OK)
	{
		return retVal;
	}
	filter_field[0].fieldType = FILTER_FIELD_IP_RANGE;
	filter_field[0].filter_pattern_union.inData.dataType = FILTER_FIELD_DATA_MASK;
	filter_field[0].filter_pattern_union.inData.value = 0x0002; //bit mask
	filter_field[0].filter_pattern_union.inData.mask = 0xFFFF;
	filter_field[0].next = NULL;
	if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[5])) != RT_ERR_OK)
	{	
		return retVal;
	}
#endif

#if 1
	filter_field[0].fieldType = FILTER_FIELD_CTAG;
	filter_field[0].filter_pattern_union.ctag.pri.dataType=FILTER_FIELD_DATA_MASK;
	filter_field[0].filter_pattern_union.ctag.pri.value=7;
	filter_field[0].filter_pattern_union.ctag.pri.mask=0xF;
	
	filter_field[0].next = NULL;
	if ((retVal = rtk_filter_igrAcl_field_add(&cfg, &filter_field[0])) != RT_ERR_OK)
		return retVal;
#endif
	
	cfg.activeport.value.bits[0] = 0x1F;
	cfg.activeport.mask.bits[0] = vportmask;
	cfg.invert = FALSE;

	act.actEnable[FILTER_ENACT_DROP] = TRUE;

	if ((retVal = rtk_filter_igrAcl_cfg_add(2, &cfg, &act, &ruleNum)) != RT_ERR_OK)
	{
		printk("retVal:%x, [%s:%d]\n", retVal, __FUNCTION__, __LINE__);
		return retVal;
	}

	printk("ruleNum:%d, [%s %d]\n", ruleNum, __func__, __LINE__);
	return RT_ERR_OK;

}
