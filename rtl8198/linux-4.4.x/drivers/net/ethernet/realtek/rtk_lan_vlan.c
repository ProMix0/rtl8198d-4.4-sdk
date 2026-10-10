#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <soc/cortina/rtk_common_utility.h>
#include <soc/cortina/rtk_vlan_common.h>
#include <soc/cortina/rtk_lan_vlan.h>
#include <soc/cortina/rtk_multi_wan_vlan.h>
#include <soc/cortina/rtk_vlan_common.h>

int rtk_vlan_mode = RTK_WAN_VLAN_MODE;
int rtk_lan_vlan_enable = 0;
int rtk_lan_vlan_dev_mode = RTK_NORMAL_VLAN_DEV_MODE;

#ifndef RTK_USE_MEMPOOL_FOR_VLAN_INFO
static LIST_HEAD(rtk_lan_vlan_info_list);
spinlock_t rtk_lan_vlan_info_list_lock;
#else
struct rtk_lan_vlan_info lan_vlan_info_pool[LAN_VLAN_POOL_SIZE] = {0};
#endif

inline void rtk_lan_vlan_info_lock(int lock)
{
	#ifndef RTK_USE_MEMPOOL_FOR_VLAN_INFO
	if(lock)
		spin_lock_bh(&rtk_lan_vlan_info_list_lock);
	else
		spin_unlock_bh(&rtk_lan_vlan_info_list_lock);
	#endif
}



#ifndef RTK_USE_MEMPOOL_FOR_VLAN_INFO
/*
not safe, should be called under rtk_lan_vlan_info_read_lock(1),
and call rtk_lan_vlan_info_read_lock(0) after finish using the rtk_lan_vlan_info struct.

usage:
	struct rtk_lan_vlan_info *info = NULL;

	rtk_lan_vlan_info_read_lock(1);
	while((info = __rtk_lan_vlan_info_iterate(info)))
	{
		...
	}

	...
	rtk_lan_vlan_info_read_lock(0);
*/
inline struct rtk_lan_vlan_info *__rtk_lan_vlan_info_iterate(struct rtk_lan_vlan_info *info)
{
	struct rtk_lan_vlan_info *info_ret = NULL;
	struct list_head *next = NULL;

	next = (info == NULL) ? rtk_lan_vlan_info_list.next : info->list.next;
	
	if(next == &rtk_lan_vlan_info_list)
		info_ret = NULL;
	else
		info_ret = list_entry(next, struct rtk_lan_vlan_info, list);

	return info_ret;
}

inline struct rtk_lan_vlan_info *__rtk_alloc_lan_vlan_info(void)
{
	struct rtk_lan_vlan_info *info = NULL;
	
	info = kzalloc(sizeof(struct rtk_lan_vlan_info), GFP_KERNEL);

	if(info != NULL)
	{
		INIT_LIST_HEAD(&info->list);
		list_add_tail(&info->list, &rtk_lan_vlan_info_list);
	}

	return info;
}

inline void __rtk_free_lan_vlan_info(struct rtk_lan_vlan_info *info)
{
	if(info == NULL)
		return;
	
	list_del(&info->list);
	kfree(info);
}
#else
inline struct rtk_lan_vlan_info *__rtk_lan_vlan_info_iterate(struct rtk_lan_vlan_info *info)
{
	if(info == NULL)
		info = &lan_vlan_info_pool[0];
	else
		info += 1;
	
	while(info < lan_vlan_info_pool + LAN_VLAN_POOL_SIZE)
	{
		if(info->valid) return info;
		info += 1;
	}
	
	return NULL;
}

inline struct rtk_lan_vlan_info *__rtk_alloc_lan_vlan_info(void)
{
	struct rtk_lan_vlan_info *info = NULL;
	
	info = &lan_vlan_info_pool[0];
	while(info < lan_vlan_info_pool + LAN_VLAN_POOL_SIZE)
	{
		if(info->valid == 0)
		{
			info->valid = 1;
			return info;
		}
		info += 1;
	}

	return NULL;
}

inline void __rtk_free_lan_vlan_info(struct rtk_lan_vlan_info *info)
{
	if(info == NULL)
		return;
	
	memset((void *)info, 0, sizeof(struct rtk_lan_vlan_info));
}
#endif

/*
not safe, should be called under __lan_vlan_info_read_lock(1),
*/
struct rtk_lan_vlan_info *__rtk_find_lan_vlan_info(struct rtk_lan_vlan_info_find *info_find)
{
	struct rtk_lan_vlan_info *l_info = NULL;
	int match_flag = 0;

	if((info_find->find_mask & 
		(RTK_LAN_VLAN_INFO_VID | RTK_LAN_VLAN_INFO_PORTMASK | RTK_LAN_VLAN_INFO_TAGMASK | RTK_LAN_VLAN_INFO_BR_WAN | RTK_LAN_VLAN_INFO_HW_NAT)) == 0)
		return NULL;
 
	while((l_info = __rtk_lan_vlan_info_iterate(l_info)))	
	{
		//printk("%s:%d l_info.vid:%d l_info.portmask:%d l_info.tagmask:%llx\n", __FUNCTION__, __LINE__,l_info->lan_vid,l_info->portmask,l_info->tagmask);
		if(info_find->find_mask & RTK_LAN_VLAN_INFO_VID)
		{
			match_flag = (l_info->lan_vid == info_find->vid);
			if((info_find->inverse_mask & RTK_LAN_VLAN_INFO_VID) ? match_flag : !match_flag)
				continue;
		}
			
		if(info_find->find_mask & RTK_LAN_VLAN_INFO_PORTMASK)
		{
			match_flag = (l_info->portmask & info_find->portmask);
			if((info_find->inverse_mask & RTK_LAN_VLAN_INFO_PORTMASK) ? match_flag : !match_flag)
				continue;
		}

		if(info_find->find_mask & RTK_LAN_VLAN_INFO_TAGMASK)
		{
			match_flag = (l_info->tagmask & info_find->tagmask);
			if((info_find->inverse_mask & RTK_LAN_VLAN_INFO_TAGMASK) ? match_flag : !match_flag)
				continue;
		}

		if(info_find->find_mask & RTK_LAN_VLAN_INFO_BR_WAN)
		{
			match_flag = (l_info->br_wan == info_find->br_wan);
			if((info_find->inverse_mask & RTK_LAN_VLAN_INFO_BR_WAN) ? match_flag : !match_flag)
				continue;
		}

		if(info_find->find_mask & RTK_LAN_VLAN_INFO_HW_NAT)
		{
			match_flag = (l_info->hw_nat == info_find->hw_nat);
			if((info_find->inverse_mask & RTK_LAN_VLAN_INFO_HW_NAT) ? match_flag : !match_flag)
				continue;
		}

		return l_info;
	}

	return NULL;
}

/*
A safe version of __rtk_find_lan_vlan_info().
*/
int rtk_find_lan_vlan_info(struct rtk_lan_vlan_info *info, struct rtk_lan_vlan_info_find *info_find)
{
	struct rtk_lan_vlan_info *info_found = NULL;

	if(info == NULL || info_find == NULL)
		return FAIL;

	rtk_lan_vlan_info_lock(1);
 
	info_found = __rtk_find_lan_vlan_info(info_find);
	if(info_found != NULL)
		memcpy((void *)info, (void *)info_found, sizeof(struct rtk_lan_vlan_info));
		
	rtk_lan_vlan_info_lock(0);

	return (info_found == NULL) ? FAIL : SUCCESS;
}

static int rtk_set_lan_pvid(unsigned int portmask,int pvid,unsigned int pri)
{
	int i = 0, ret = 0;
	unsigned int pvid_tmp = 0, pri_tmp = 0;

	//port0~port4
	portmask &= 0x1F;
	
	VLAN_TRACE("%s:%d portmask:%d i:%d vid:%d\n", __FUNCTION__, __LINE__,portmask,i,pvid);
	for (i = 0; i < MAX_ETH_NUM; i++)
	{
	
		if (rtl83xx_pvid_get_hook){
			ret = (*rtl83xx_pvid_get_hook)(i, &pvid_tmp, &pri_tmp);
			VLAN_TRACE("i=%d, pvid=%u, port_pri=%u, ret=%d\n",i, pvid_tmp, pri_tmp, ret);
			if (ret != SUCCESS)
				return ret;
		}
		else{
			VLAN_ERROR("rtl83xx_pvid_get_hook is NULL!\n");
			return FAIL;
		}
	
		if ((1<<i) & portmask){
			if(pri_tmp!= 0){
				pri = pri_tmp;
			}
		
			if (rtl83xx_pvid_set_hook){
				ret = (*rtl83xx_pvid_set_hook)(i, pvid, pri);
				if (ret != SUCCESS)
					return ret;
			}
			else{
				return FAIL;
			}
		}
	}

	return SUCCESS;
}

void rtk_update_lan_pvid(void)
{
	int idx = 0, ret = 0;
	struct rtk_lan_vlan_info info = {0};
	struct rtk_lan_vlan_info_find info_find = {0};
	unsigned int portmask = 0, tagmask = 0;

	for(idx = 0; idx < RTK_LAN_ETH_NUM; idx++)
	{
		
		//dev mask
		portmask = tagmask = 1 << idx;
		
		//use device bit mask to check
		if (rtk_lan_vlan_dev_mode == RTK_ALL_VLAN_DEV_MODE)
		{
			info_find.vid = RTL_LANVLANID;
			info_find.portmask = portmask;
			info_find.find_mask = RTK_LAN_VLAN_INFO_VID | RTK_LAN_VLAN_INFO_PORTMASK;
			info_find.inverse_mask = RTK_LAN_VLAN_INFO_VID;
		}
		else
		{
			info_find.vid = RTL_LANVLANID;
			info_find.portmask = portmask;
			info_find.tagmask = tagmask;
			info_find.find_mask = RTK_LAN_VLAN_INFO_VID | RTK_LAN_VLAN_INFO_PORTMASK | RTK_LAN_VLAN_INFO_TAGMASK;
			info_find.inverse_mask = RTK_LAN_VLAN_INFO_VID | RTK_LAN_VLAN_INFO_TAGMASK;
		}
		
		ret = rtk_find_lan_vlan_info(&info, &info_find);
		VLAN_TRACE("%s:%d portmask:%d vid:%d ret:%d\n", __FUNCTION__, __LINE__,portmask,info.lan_vid,ret);

		rtl_change_dev_mask_to_port_mask(&portmask, &tagmask);
		VLAN_TRACE("%s:%d portmask:%d tagmask:%d\n", __FUNCTION__, __LINE__,portmask, tagmask);

		if(ret == SUCCESS)
		{
			VLAN_TRACE("%s:%d portmask:%d vid:%d ret:%d\n", __FUNCTION__, __LINE__,portmask,info.lan_vid,ret);
			rtk_set_lan_pvid(portmask,info.lan_vid,0);
		}
		else
		{
			VLAN_TRACE("%s:%d portmask:%d vid:%d ret:%d\n", __FUNCTION__, __LINE__,portmask,info.lan_vid,ret);
			rtk_set_lan_pvid(portmask,RTL_LANVLANID,0);
		}

	}

	//vlan_9_portmask is physical portmask, change to dev bit mask
	//tagmask = 0;
	//rtl_change_dev_mask_to_port_mask(&vlan_9_portmask, &tagmask);
	//rtk_add_lan_vlan_info(RTL_LANVLANID, vlan_9_portmask, (unsigned int)0, 0, 0);
}


int rtk_set_lan_vlan(int vid, unsigned int port_mask, unsigned int tag_mask, int pri, int fid)
{
	unsigned int board_type = 0;
	unsigned int membermsk = 0, untagmsk = 0, tmp_pvid = 0, tmp_vid = 0;
	unsigned int ext_cpu_mask = 0;
	int ret = 0;

	VLAN_TRACE("%s:%d vid:%d portmask:%x tagmask:%x\n", __FUNCTION__, __LINE__,vid,port_mask,tag_mask);
	tmp_vid = vid;
	tmp_pvid = tmp_vid;
	port_mask &= RTL_ETH_LAN1_BIND_MASK | RTL_ETH_LAN2_BIND_MASK | RTL_ETH_LAN3_BIND_MASK | RTL_ETH_LAN4_BIND_MASK | RTL_ETH_LAN5_BIND_MASK;

	if (rtl_get_board_type_hook){
		board_type = (*rtl_get_board_type_hook)();
		VLAN_TRACE("board_type=0x%x \n", board_type);
	}
	else{
		VLAN_ERROR("rtl_get_board_type_hook is NULL!\n");
		return FAIL;
	}
	//v600, exclude lan1(bit0), because lan1 is not external switch port
	if ((board_type & 0xF00) == 0x600){
		port_mask &= ~RTL_ETH_LAN1_BIND_MASK;
	}

	VLAN_TRACE("port_mask=0x%x, tag_mask=0x%x\n",port_mask, tag_mask);
	//change for external switch
	rtl_change_dev_mask_to_port_mask(&port_mask, &tag_mask);
	VLAN_TRACE("port_mask=0x%x, tag_mask=0x%x\n",port_mask, tag_mask);
	
#if defined(CONFIG_RTL_83XX_SUPPORT)
	if (rtl83xx_vlan_get_hook){
		ret = (*rtl83xx_vlan_get_hook)(tmp_vid, &membermsk, &untagmsk, &pri, &fid);
		if (ret != SUCCESS){
			VLAN_ERROR("get tmp_vid=%u\n", tmp_vid);
			return ret;
		}
	}
	else{
		VLAN_ERROR("rtl83xx_vlan_get_hook is NULL!");
		return FAIL;
	}

	untagmsk |= port_mask & (~tag_mask);
	membermsk |= port_mask;

	//add cpu port when lan vlan ports have tagged ports
	if(tag_mask!= 0)
	{
		if (rtl83xx_get_cpu_portmask_hook){
		ext_cpu_mask = (*rtl83xx_get_cpu_portmask_hook)();
		VLAN_TRACE("ext_cpu_mask=%x \n", ext_cpu_mask);
		}
		else{
		VLAN_ERROR("rtl83xx_get_cpu_portmask_hook is NULL!\n");
		return FAIL;
		}
		membermsk |= ext_cpu_mask;
	}
	
	if (rtl83xx_vlan_set_hook){
		ret = (*rtl83xx_vlan_set_hook)(tmp_vid, membermsk, untagmsk, pri, fid);
		VLAN_TRACE("tmp_vid=%u, membermsk=0x%x, untagmsk=0x%x, pri=%u, fid=%u ret=%d\n",tmp_vid, membermsk, untagmsk, pri, fid, ret);
		if (ret != SUCCESS)
			return ret;
	}
	else{
		VLAN_ERROR("rtl83xx_vlan_set_hook is NULL!\n");
		return FAIL;
	}

	//update pvid
	if(vid != RTL_LANVLANID)
	rtk_update_lan_pvid();
#endif

	return SUCCESS;

}

extern unsigned int rtl_get_bind_port_mask_by_dev_name(unsigned char *dev_name);

int rtk_lan_vlan_process_rx(struct sk_buff **pskb)
{
	struct net_device *dev = NULL;
	struct rtk_lan_vlan_info info = {0};
	struct rtk_lan_vlan_info_find info_find = {0}; 
	unsigned short vid = 0, pvid = 0;
	struct sk_buff *skb = *pskb;
	unsigned int dev_member_mask = 0;
	unsigned int port_mask = 0, tag_mask = 0;

	dev = skb->dev;
	vid = rtk_get_skb_vid(skb);
	pvid = RTL_LANVLANID;
	
	if(dev == NULL)
	{
		VLAN_TRACE("%s:%d dev_name:%s dev_member_mask:%x vid:%d\n", __FUNCTION__, __LINE__, dev->name, dev_member_mask, vid);
		dev_kfree_skb_any(skb);
		return FAIL;
	}

	dev_member_mask = rtl_get_bind_port_mask_by_dev_name(dev->name);
	if(dev_member_mask == 0)
	{
		VLAN_TRACE("%s:%d dev_name:%s dev_member_mask:%x vid:%d\n", __FUNCTION__, __LINE__, dev->name, dev_member_mask, vid);
		dev_kfree_skb_any(skb);
		return FAIL;
	}

	//unknown lan vlan check
	info_find.vid = vid? vid : pvid;//wan dev vid
	info_find.find_mask = RTK_LAN_VLAN_INFO_VID;
	if(rtk_find_lan_vlan_info(&info, &info_find) == FAIL)
	{
		VLAN_TRACE("%s:%d dev_name:%s dev_member_mask:%x info_find.vid:%d info.lan_vid:%d\n", __FUNCTION__, __LINE__, dev->name, dev_member_mask,info_find.vid,info.lan_vid);
		dev_kfree_skb_any(skb);
		return FAIL;
	}

	port_mask = (unsigned int)info.portmask;
	tag_mask = (unsigned int)info.tagmask;

	//lan vlan ingress membership check
	if(!(port_mask & dev_member_mask))
	{
		VLAN_TRACE("%s:%d dev_name:%s dev_member_mask:%x info.portmask:%llx port_mask:%x\n", __FUNCTION__, __LINE__, dev->name, dev_member_mask, info.portmask, port_mask);
		dev_kfree_skb_any(skb);
		return FAIL;
	}

	//tagging process
	switch(rtk_lan_vlan_dev_mode)
	{
		case RTK_NORMAL_VLAN_DEV_MODE:

			if((tag_mask & dev_member_mask) && vid == 0)
			{
				if(rtk_insert_vlan_tag_and_priority(pvid, 0, (struct sk_buff **)(&skb))==FAIL)
					return FAIL;
			}
			if(!(tag_mask & dev_member_mask) && vid != 0)
			{
				if(remove_vlan_tag((struct sk_buff **)(&skb))==FAIL)
					return FAIL;
			}
			break;
		case RTK_ALL_VLAN_DEV_MODE:
			if(vid == 0)
			{
				if(rtk_insert_vlan_tag_and_priority(vid, 0, (struct sk_buff **)(&skb))==FAIL)
					return FAIL;
			}
			break;
		case RTK_NO_VLAN_DEV_MODE:
		default:
			if(vid != 0)
			{
				if(remove_vlan_tag((struct sk_buff **)(&skb))==FAIL)
					return FAIL;
			}
			break;
	}

	skb->srcVlanId = vid;
	*pskb = skb;
		
	return SUCCESS;
}


int rtk_remove_lan_vlan(int vid)
{
	unsigned int board_type = 0;
	unsigned int membermsk = 0, untagmsk = 0, tmp_pvid = 0, tmp_vid = 0;
	unsigned short pri = 0, fid = 0;
	unsigned int port_mask = 0, tag_mask = 0,ext_cpu_mask = 0;
	int ret = 0;

	tmp_vid = vid;
	tmp_pvid = RTL_LANVLANID;

#if defined(CONFIG_RTL_83XX_SUPPORT)
	if (rtl83xx_vlan_get_hook){
		ret = (*rtl83xx_vlan_get_hook)(tmp_vid, &membermsk, &untagmsk, &pri, &fid);
		if (ret != SUCCESS){
			VLAN_ERROR("get tmp_vid=%u failed! \n", tmp_vid);
			return ret;
		}
	}
	else{
		VLAN_ERROR("rtl83xx_vlan_get_hook is NULL!");
		return FAIL;
	}
		
	pri = membermsk = fid = untagmsk = 0;
	
	//remove selected vlan 
	if (rtl83xx_vlan_set_hook){
		ret = (*rtl83xx_vlan_set_hook)(tmp_vid, membermsk, untagmsk, pri, fid);
		VLAN_TRACE("tmp_vid=%u, membermsk=0x%x, untagmsk=0x%x, pri=%u, fid=%u ret=%d\n",tmp_vid, membermsk, untagmsk, pri, fid, ret);
		if (ret != SUCCESS)
			return ret;
	}
	else{
		VLAN_ERROR("rtl83xx_vlan_set_hook is NULL!\n");
		return FAIL;
	}

	//update pvid
	if(vid != RTL_LANVLANID)
	rtk_update_lan_pvid();
#endif

	return SUCCESS;

}


/* portmask and tagmask means dev bits mask */
int rtk_add_lan_vlan_info(struct rtk_lan_vlan_info lan_info)
{
	struct rtk_lan_vlan_info *info = NULL;
	struct rtk_lan_vlan_info_find info_find = {0};
	int exist_flag = 0, ret = 0;
	unsigned short pri = 0, fid = 0;

	rtk_lan_vlan_info_lock(1);

	info_find.vid = lan_info.lan_vid;
	info_find.find_mask = RTK_LAN_VLAN_INFO_VID;
	info = __rtk_find_lan_vlan_info(&info_find);
	if(info == NULL)
	{
		info = __rtk_alloc_lan_vlan_info();
		if(info == NULL){
			rtk_lan_vlan_info_lock(0);
			return FAIL;
		}
		exist_flag = 0;
	}
	else{
		exist_flag = 1;
	}
	
	if (!exist_flag || ((exist_flag)&&!info->br_wan)){
		info->lan_vid = lan_info.lan_vid;
		info->portmask = lan_info.portmask;
		info->tagmask = lan_info.tagmask;
		info->br_wan = lan_info.br_wan;
		info->hw_nat = lan_info.hw_nat;
		//printk("%s %d vid=%d portmask=0x%llx, tagmask=0x%llx info->portmask=0x%llx, info->tagmask=0x%llx\n", __func__, __LINE__,vid, portmask, tagmask, info->portmask, info->tagmask);
	}
	else{
		info->portmask |= lan_info.portmask;
		info->tagmask |= lan_info.tagmask;
		//printk("\n %s %d  vid=%d info->portmask=0x%llx, info->tagmask=0x%llx, portmask=0x%llx, tagmask=0x%llx br_wan=%d , hw_nat=%d\n", __func__, __LINE__, vid, info->portmask, info->tagmask, portmask, tagmask, br_wan, hw_nat);
	}
	
	rtk_lan_vlan_info_lock(0);
	//printk("%s:%d l_info.vid:%d l_info.portmask:%llx l_info.tagmask:%llx\n", __FUNCTION__, __LINE__,info->lan_vid,info->portmask,info->tagmask);
	//set lan vlan info
	ret = rtk_set_lan_vlan(lan_info.lan_vid,(unsigned int)lan_info.portmask,(unsigned int)lan_info.tagmask,pri,fid);
	if(ret != SUCCESS){
		VLAN_ERROR("set 83xx hw lan vlan is fail!");
		return FAIL;
	}
	
	return SUCCESS;
} 

int rtk_remove_lan_vlan_info(int vid, uint64_t portmask, uint64_t tagmask)
{
	struct rtk_lan_vlan_info *info = NULL;
	struct rtk_lan_vlan_info_find info_find = {0};
	int hw_nat = 0 , ret = 0;

	rtk_lan_vlan_info_lock(1);

	info_find.vid = vid;
	info_find.find_mask = RTK_LAN_VLAN_INFO_VID;
	info = __rtk_find_lan_vlan_info(&info_find);
	if(info == NULL){
		rtk_lan_vlan_info_lock(0);
		return FAIL;
	}

	hw_nat = info->hw_nat;
	/* 
	 * if it's brdige vlan group, when info->portmask==0, free the lan vlan entry 
	*/
	if (info->br_wan){
		info->portmask &= (~portmask);
		info->tagmask &= (~tagmask);
		//printk("%s %d vid=%d portmask=0x%llx, tagmask=0x%llx info->portmask=0x%llx, info->tagmask=0x%llx\n", __func__, __LINE__,vid, portmask, tagmask, info->portmask, info->tagmask);
	}
	
	if (!info->br_wan || !info->portmask)
	__rtk_free_lan_vlan_info(info);
	
	rtk_lan_vlan_info_lock(0);

	//remove lan vlan info
	ret = rtk_remove_lan_vlan(vid);
	if(ret != SUCCESS){
		VLAN_ERROR("remove 83xx hw lan vlan is fail!");
		return FAIL;
	}

	return SUCCESS;
}


int rtk_lan_vlan_cmd_parse(unsigned char *cmd[], int num)
{	  
	int ret = 0;

	if (!strncmp(cmd[0], "add",3))
	{
		struct rtk_lan_vlan_info lan_info;
		int idx = 0;

		if (num < 4)
			return FAIL;

		memset(&lan_info, 0x00, sizeof(lan_info));
		
		sscanf(cmd[1], "%d", &lan_info.lan_vid);

		//only for test to add pvid:9  lan vlan info
		#if 0
		if (lan_info.lan_vid == RTL_LANVLANID || lan_info.lan_vid == RTL_WANVLANID)
		{
			printk("vid %d is reserved.\n", lan_info.lan_vid);
			return FAIL;
		}
		#endif

		sscanf(cmd[2], "%llx", &lan_info.portmask);
		sscanf(cmd[3], "%llx", &lan_info.tagmask);

		idx = 4;
		while(idx < num)
		{
			if(!strncmp(cmd[idx], "br_wan",6))
				lan_info.br_wan = 1;
			else if(!strncmp(cmd[idx],"hw_nat",6))
				lan_info.hw_nat = 1;
			else if (!strncmp(cmd[idx],"wan_vid",7)){
				idx++;
				sscanf(cmd[idx], "%d", &lan_info.wan_vid);
			}
			else if (!strncmp(cmd[idx], "wan_name",8)){
				idx++;
				sscanf(cmd[idx], "%s", &lan_info.wan_dev_name[0]);
			}
			
			idx++;
		}
		
		ret = rtk_add_lan_vlan_info(lan_info);
		if(ret < 0)
			printk("Add lan vlan failed!\n");
	}
	else if (!strncmp(cmd[0], "remove",6))
	{
		int vid = -1;
		uint64_t portmask = 0, tagmask = 0;
		
		if (num < 2)
			return FAIL;

		sscanf(cmd[1], "%d", &vid);
		if (num >= 4){
			/* backward compatible with old remove command 
			 * echo "vid" > /proc/lan_vlan
			 * so if not set portmask and tagmask in command line, it means to delete whole vlan group.
			*/
			sscanf(cmd[2], "%llx", &portmask);
			sscanf(cmd[3], "%llx", &tagmask);
		}
		else{
			//portmask = tagmask = RTL_LANPORT_MASK & (~RTL_CPUPORT_MASK);
			//rtl_change_port_mask_to_dev_mask(&portmask, &tagmask);
			//printk("%s %d portmask=0x%llx, tagmask=0x%llx\n", __FUNCTION__, __LINE__, portmask, tagmask);
		}
		ret = rtk_remove_lan_vlan_info(vid, portmask, tagmask);
		if(ret < 0)
			printk("Remove lan vlan failed!\n");
	}
	else if (!strncmp(cmd[0], "enable",6))
	{		
		int enable = 0;
		
		if (num < 2)
			return FAIL;

		sscanf(cmd[1], "%d", &enable);

		if(enable != 0 && enable != 1)
			return FAIL;

		rtk_lan_vlan_enable = enable;
	}
	else if (!strncmp(cmd[0], "dev_mode",8))
	{		
		int dev_mode = 0;
		
		if (num < 2)
			return FAIL;

		if(!strncmp(cmd[1], "normal",6))
			dev_mode = RTK_NORMAL_VLAN_DEV_MODE;
		else if(!strncmp(cmd[1], "no_vlan_dev",11))
			dev_mode = RTK_NO_VLAN_DEV_MODE;
		else if(!strncmp(cmd[1], "all_vlan_dev",12))
			dev_mode = RTK_ALL_VLAN_DEV_MODE;
		else
			return FAIL;

		rtk_lan_vlan_dev_mode = dev_mode;
	}
	else if (!strncmp(cmd[0], "vlan_debug_level",16))
	{
		unsigned int debug_level = 0;
		
		if (num < 2)
			return FAIL;

		sscanf(cmd[1], "%x", &debug_level);

		printk("%s %d debug_level=0x%x\n", __func__, __LINE__, debug_level);
		
		rtk_vlan_debug = debug_level;
		
	}	
	else
		return FAIL;
		
	return SUCCESS;
}

int rtk_lan_vlan_show(
	#ifdef __ECOS
	void
	#else
	struct seq_file *s
	#endif
	)
{
	struct rtk_lan_vlan_info *info = NULL;

	SEQ_PRINT("\n");
	SEQ_PRINT("enable        :  %d\n", rtk_lan_vlan_enable);
	SEQ_PRINT("vlan_dev_mode :  %d\n", rtk_lan_vlan_dev_mode);
	SEQ_PRINT("rtk_vlan_debug:  0x%x\n", rtk_vlan_debug);
	SEQ_PRINT("\n");

	rtk_lan_vlan_info_lock(1);
 
	while((info = __rtk_lan_vlan_info_iterate(info)))		
	{	
		SEQ_PRINT("lan_vid           :  %d\n", info->lan_vid);
		SEQ_PRINT("wan_vid           :  %d\n", info->wan_vid);
		SEQ_PRINT("wan_dev_name      :  %s\n", info->wan_dev_name);
		SEQ_PRINT("portmask          :  0x%016llx\n", info->portmask);
		SEQ_PRINT("tagmask           :  0x%016llx\n", info->tagmask);
		SEQ_PRINT("hw_nat            :  %d\n", info->hw_nat);
		SEQ_PRINT("br_wan            :  %d\n", info->br_wan);
		SEQ_PRINT("\n");	
	}
	
	rtk_lan_vlan_info_lock(0);

    return 0;
}

static int rtk_lan_vlan_write(struct file *file, const char *buffer, unsigned long count, void *data)
{	  
	static unsigned char cmd[RTK_PARAMETER_SIZE][32];
	unsigned char  *arg[RTK_PARAMETER_SIZE];
	unsigned char  strBuffer[256] = {0};
	int num = 0, i = 0;

	if(count > 96)
		goto cmd_invalid;

	if (!buffer || copy_from_user(strBuffer, buffer, count) != 0)
		return 0;

	for (i = 0; i < RTK_PARAMETER_SIZE; i++)
	{
		arg[i] = cmd[i];
	}
	
	memset(cmd, 0, sizeof(cmd));
	
	num = sscanf(strBuffer, "%s %s %s %s %s %s %s %s %s %s %s %s", 
	cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8], cmd[9], cmd[10], cmd[11]);
	
	if(FAIL == rtk_lan_vlan_cmd_parse(arg, num))
		goto cmd_invalid;
		
	return count;

cmd_invalid:
	printk("Command format invalid!\n");
	return -EFAULT; 
}

static int rtk_lan_vlan_read(struct seq_file *s, void *v)
{
	rtk_lan_vlan_show(s);
	
	return 0;
}

static ssize_t rtk_lan_vlan_single_write(struct file * file, const char __user * userbuf,
			 size_t count, loff_t * off)
{
	return rtk_lan_vlan_write(file, userbuf, count, off);
}

int rtk_lan_vlan_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, rtk_lan_vlan_read,NULL));
}

struct file_operations rtk_lan_vlan_proc_fops = {
		.open				= rtk_lan_vlan_single_open,
		.write				= rtk_lan_vlan_single_write,
		.read				= seq_read,
		.llseek 			= seq_lseek,
		.release			= single_release,
};

void rtk_lan_vlan_proc_init(void)
{	
	if (rtk_proc_dir)	
		printk("%s %d create proc file in /proc/driver/realtek/ \n", __func__, __LINE__);
	else
		printk("%s %d create proc file in /proc/ \n", __func__, __LINE__);

	proc_create_data("rtk_lan_vlan", 0, rtk_proc_dir, &rtk_lan_vlan_proc_fops, NULL);

	return ;
}

void rtk_lan_vlan_proc_exit(void)
{
	remove_proc_entry("rtk_lan_vlan", rtk_proc_dir);
}

static int __init rtk_lan_vlan_init(void)
{
	#ifdef RTK_USE_MEMPOOL_FOR_VLAN_INFO
	memset(lan_vlan_info_pool, 0, sizeof(lan_vlan_info_pool));
	#else
	spin_lock_init(&rtk_lan_vlan_info_list_lock);
	#endif
	
	//proc init
	rtk_lan_vlan_proc_init();
	
	return 0;
}

static void __exit rtk_lan_vlan_exit(void)
{
	rtk_lan_vlan_proc_exit();
}

module_init(rtk_lan_vlan_init);
module_exit(rtk_lan_vlan_exit);


