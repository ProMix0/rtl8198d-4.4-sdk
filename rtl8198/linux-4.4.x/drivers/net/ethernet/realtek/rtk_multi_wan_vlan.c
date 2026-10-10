#include <linux/module.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <soc/cortina/rtk_vlan_common.h>
#include <soc/cortina/rtk_multi_wan_vlan.h>
#if defined(CONFIG_RTK_VLAN_PASSTHROUGH_SUPPORT)
#include <soc/cortina/rtk_vlan_passthrough.h>
#endif
#include <soc/cortina/rtk_lan_vlan.h>
#include <uapi/linux/if_ether.h>

#include "if_smux.h"

#if defined(CONFIG_RTK_DEV_AP)
rtl_register_multi_wan_hook_t rtl_register_multi_wan_hook;
rtl_unregister_multi_wan_hook_t rtl_unregister_multi_wan_hook;
rtl_update_multi_wan_port_mapping_hook_t rtl_update_multi_wan_port_mapping_hook;
EXPORT_SYMBOL(rtl_register_multi_wan_hook);
EXPORT_SYMBOL(rtl_unregister_multi_wan_hook);
EXPORT_SYMBOL(rtl_update_multi_wan_port_mapping_hook);
#endif

rtl83xx_vlan_set_hook_t rtl83xx_vlan_set_hook;
EXPORT_SYMBOL(rtl83xx_vlan_set_hook);
rtl83xx_vlan_get_hook_t rtl83xx_vlan_get_hook;
EXPORT_SYMBOL(rtl83xx_vlan_get_hook);
rtl83xx_pvid_get_hook_t rtl83xx_pvid_get_hook;
EXPORT_SYMBOL(rtl83xx_pvid_get_hook);
rtl83xx_pvid_set_hook_t rtl83xx_pvid_set_hook;
EXPORT_SYMBOL(rtl83xx_pvid_set_hook);
rtl83xx_get_cpu_portmask_hook_t rtl83xx_get_cpu_portmask_hook;
EXPORT_SYMBOL(rtl83xx_get_cpu_portmask_hook);


ca_ni_get_wan_port_id_hook_t ca_ni_get_wan_port_id_hook;
EXPORT_SYMBOL(ca_ni_get_wan_port_id_hook);
rtl_get_board_type_hook_t rtl_get_board_type_hook;
EXPORT_SYMBOL(rtl_get_board_type_hook);

rtl_get_phy_portmask_by_dev_name_hook_t rtl_get_phy_portmask_by_dev_name_hook; 
EXPORT_SYMBOL(rtl_get_phy_portmask_by_dev_name_hook);

unsigned int rtk_multi_wan_debug = 0x2;
unsigned char rtk_multi_wan_dbg_buff[RTK_MAX_DBG_MSG_LEN];
EXPORT_SYMBOL(rtk_multi_wan_debug);
EXPORT_SYMBOL(rtk_multi_wan_dbg_buff);

struct lan_dev_bind_mask_drv lan_dev_bind_mask_mapping_drv[] = 
{
	{RTL_DRV_LAN1_NAME, 		RTL_ETH_LAN1_BIND_MASK},
	{RTL_DRV_LAN2_NAME, 		RTL_ETH_LAN2_BIND_MASK},
	{RTL_DRV_LAN3_NAME, 		RTL_ETH_LAN3_BIND_MASK},
	{RTL_DRV_LAN4_NAME, 		RTL_ETH_LAN4_BIND_MASK},
	{RTL_DRV_LAN5_NAME, 		RTL_ETH_LAN5_BIND_MASK},
};

struct lan_dev_bind_mask_drv wlan_dev_bind_mask_mapping_drv[] =
{
	{RTL_DRV_WLAN0_NAME,		RTL_WLAN0_BIND_MASK},
#ifdef CONFIG_RTL_VAP_SUPPORT
	{RTL_DRV_WLAN0_VAP0_NAME,	RTL_WLAN0_VAP0_BIND_MASK},
	{RTL_DRV_WLAN0_VAP1_NAME,	RTL_WLAN0_VAP1_BIND_MASK},
	{RTL_DRV_WLAN0_VAP2_NAME,	RTL_WLAN0_VAP2_BIND_MASK},
	{RTL_DRV_WLAN0_VAP3_NAME,	RTL_WLAN0_VAP3_BIND_MASK},
#endif
#ifdef CONFIG_USE_PCIE_SLOT_0
	{RTL_DRV_WLAN1_NAME,		RTL_WLAN1_BIND_MASK},
#ifdef CONFIG_RTL_VAP_SUPPORT
	{RTL_DRV_WLAN1_VAP0_NAME,	RTL_WLAN0_VAP0_BIND_MASK},
	{RTL_DRV_WLAN1_VAP0_NAME,	RTL_WLAN0_VAP1_BIND_MASK},
	{RTL_DRV_WLAN1_VAP0_NAME,	RTL_WLAN0_VAP2_BIND_MASK},
	{RTL_DRV_WLAN1_VAP0_NAME,	RTL_WLAN0_VAP3_BIND_MASK},
#endif
#endif
};

int rtl_get_eth_dev_num(void)
{
#if 0
	int i = 0;
	unsigned int board_type = 0;
	
	if (rtl_get_board_type_hook){
		board_type = (*rtl_get_board_type_hook)();
	}
	else{
		/* if not get board type ,default 4*/
		return 4;
	}

	if ((board_type & 0xF00) == 0x500){
		return 5;
	}else
		return 4;
#else
	/* all board only support 4 lan, may be modify */
	return 4;
#endif
}

char *rtl_get_dev_name_by_bind_port_mask(unsigned int portmask)
{
	int i = 0;
	int lan_dev_num = rtl_get_eth_dev_num();

	for (i=0; i<(sizeof(lan_dev_bind_mask_mapping_drv)/sizeof(lan_dev_bind_mask_mapping_drv[0])) && i < lan_dev_num; i++)
	{
		if (portmask & lan_dev_bind_mask_mapping_drv[i].bind_mask) {
			return lan_dev_bind_mask_mapping_drv[i].ifname;
		}
	}
	
	for (i=0; i<(sizeof(wlan_dev_bind_mask_mapping_drv)/sizeof(wlan_dev_bind_mask_mapping_drv[0])); i++)
	{
		wlan_dev_bind_mask_mapping_drv[i].bind_mask = (1<<(lan_dev_num+i));
	
		if (portmask & wlan_dev_bind_mask_mapping_drv[i].bind_mask) {
			return wlan_dev_bind_mask_mapping_drv[i].ifname;
		}
	}
	
	return NULL;
}
EXPORT_SYMBOL(rtl_get_dev_name_by_bind_port_mask);

unsigned int rtl_get_bind_port_mask_by_dev_name(unsigned char *dev_name)
{
	int i = 0;
	int lan_dev_num = rtl_get_eth_dev_num();

	for (i=0; i<(sizeof(lan_dev_bind_mask_mapping_drv)/sizeof(lan_dev_bind_mask_mapping_drv[0])) && i < lan_dev_num; i++)
	{
		if (!strncmp(dev_name,lan_dev_bind_mask_mapping_drv[i].ifname,IFNAMSIZ))
		{
			return lan_dev_bind_mask_mapping_drv[i].bind_mask;
		}
	}
	
	for (i=0; i<(sizeof(wlan_dev_bind_mask_mapping_drv)/sizeof(wlan_dev_bind_mask_mapping_drv[0])); i++)
	{
		wlan_dev_bind_mask_mapping_drv[i].bind_mask = (1<<(lan_dev_num+i));
	
		if (strncmp(dev_name,lan_dev_bind_mask_mapping_drv[i].ifname,IFNAMSIZ))
		{
			return lan_dev_bind_mask_mapping_drv[i].bind_mask;
		}
	}
	
	return 0;
}
EXPORT_SYMBOL(rtl_get_bind_port_mask_by_dev_name);


/* port_mask and tag_mask may changed */
int rtl_change_dev_mask_to_port_mask(unsigned int *port_mask, unsigned int *tag_mask)
{
	int i = 0;
	unsigned int new_portmask = 0, new_tagmask = 0, old_portmask= 0, old_tagmask = 0;
	unsigned int tmp_mask = 0;
	char *dev_name = NULL;
	
	if (!port_mask || !tag_mask || (0 == *port_mask))
		return FAIL;

	old_portmask = *port_mask;
	old_tagmask = *tag_mask;
	for (i = 0; i < 8*sizeof(unsigned int); i++){
		if ((1<<i) & old_portmask){
			tmp_mask = 0;
			dev_name = rtl_get_dev_name_by_bind_port_mask((1<<i));		
			//printk("%s %d dev_name=%s \n", __func__, __LINE__, dev_name?dev_name:"NULL");
			if (dev_name && !strncmp(dev_name, RTL_DRV_LAN_NAME_PREFIX, 5)){
				if (rtl_get_phy_portmask_by_dev_name_hook){
					tmp_mask = (*rtl_get_phy_portmask_by_dev_name_hook)(dev_name);
					//if not multi-wan,disable multi-lan,dont do mapping,just use org mask 
					if (!tmp_mask)
						tmp_mask = (1<<i);
					TRACE("tmp_mask=0x%x dev_name=%s\n",tmp_mask, dev_name);
				}
				else{
					TRACE("rtl_get_phy_portmask_by_dev_name_hook is NULL, tmp_mask=0x%x dev_name=%s\n", tmp_mask, dev_name);
					tmp_mask = (1<<i);
				}
			}
			else{
				tmp_mask = (1<<i);
			}
			
			new_portmask |= tmp_mask;
			if ((1<<i) & old_tagmask){
				new_tagmask |= tmp_mask;
			}
		}
	}

	TRACE("new_portmask=0x%x new_tagmask=0x%x old_portmask=0x%x old_tagmask=0x%x\n", new_portmask, new_tagmask,old_portmask, old_tagmask);

	*port_mask = new_portmask;
	*tag_mask = new_tagmask;

	return SUCCESS;
}

int rtl_get_port_mask_by_dev_mask(unsigned int *port_mask, struct net_device *dev)
{
	unsigned int tmp_mask = 0;
	char *dev_name = NULL;

	dev_name = dev->name;
	
	if (dev_name && !strncmp(dev_name, RTL_DRV_LAN_NAME_PREFIX, 5)){
		if (rtl_get_phy_portmask_by_dev_name_hook){
			tmp_mask = (*rtl_get_phy_portmask_by_dev_name_hook)(dev_name);
			//if not multi-wan,disable multi-lan,dont do mapping,just use org mask 
			if (!tmp_mask)
				tmp_mask = 0;
			TRACE("tmp_mask=0x%x dev_name=%s\n",tmp_mask, dev_name);
		}
		else{
			TRACE("rtl_get_phy_portmask_by_dev_name_hook is NULL, tmp_mask=0x%x dev_name=%s\n", tmp_mask, dev_name);
			tmp_mask = 0;
		}
	}
	else{
		tmp_mask = 0;
	}

	*port_mask = tmp_mask;

	return SUCCESS;
}


/* 
 *	Function Name:	rtl_register_multi_wan_dev
 *	Description:	set hardware information
 *
 *	Return:		SUCCESS	:set successfully 
 * 				FAILED	:set failed 
 */

int rtl_register_multi_wan_dev(const char *rifname, const char *nifname, int smux_proto,
										int vid, unsigned int priority, int napt, int brpppoe)
{
	int ret = 0;
	unsigned int wan_port = 0;
#if defined(CONFIG_RTL_83XX_SUPPORT)
	unsigned int mbr_msk = 0, untag_msk = 0, membermsk = 0, untagmsk= 0;
	unsigned int tmp_vid = ((vid==-1)?RTL_WANVLANID:vid);
	unsigned int ext_cpu_mask = 0;
	unsigned short pri = 0, fid = 0;
#endif

	#if 0
	if (vid < 1 || vid >=VLAN_VID_MASK)
		return FAIL;
	#endif

	if (!rifname || !nifname)
		return FAIL;

	if (ca_ni_get_wan_port_id_hook){
		wan_port = (*ca_ni_get_wan_port_id_hook)();
		TRACE("wan_port=%u \n", wan_port);
	}
	else{
		ERROR("ca_ni_get_wan_port_id_hook is NULL!\n");
		return FAIL;
	}

	//set vlan for 83xx
	#if defined(CONFIG_RTL_83XX_SUPPORT)
	if (rtl83xx_get_cpu_portmask_hook){
		ext_cpu_mask = (*rtl83xx_get_cpu_portmask_hook)();
		TRACE("ext_cpu_mask=%x \n", ext_cpu_mask);
	}
	else{
		ERROR("rtl83xx_get_cpu_portmask_hook is NULL!\n");
		return FAIL;
	}
	mbr_msk = ext_cpu_mask;
	#if !defined(CONFIG_RTL_8226_SUPPORT)
	mbr_msk |= 1 << wan_port;
	#endif
	
	if (tmp_vid == RTL_WANVLANID)
		untag_msk = mbr_msk;

	//for nat wan and bridge wan both untag or with same tag, first get group then set.
	if (rtl83xx_vlan_get_hook){
		ret = (*rtl83xx_vlan_get_hook)(tmp_vid, &membermsk, &untagmsk, &pri, &fid);
		if (ret == SUCCESS){
			/* vlan group already existed 
			 * priority ? priority should be same, fid ? should be RTL_WAN_FID? fix me?
			 */
			mbr_msk |= membermsk;
			untag_msk |= untagmsk;
		}
	}
	
	if (rtl83xx_vlan_set_hook){
		ret = (*rtl83xx_vlan_set_hook)(tmp_vid, mbr_msk, untag_msk, priority, RTL_WAN_FID);
		TRACE("rifname=%s, nifname=%s, smux_proto=%d, vid=%d, priority=%d, napt=%d, brpppoe=%d\n", rifname, nifname, smux_proto, vid, priority, napt, brpppoe);
		TRACE("tmp_vid=%u mbr_msk=0x%x untag_msk=0x%x RTL_WAN_FID=0x%x\n", tmp_vid, mbr_msk, untag_msk, RTL_WAN_FID);
	}
	else{
		ERROR("rtl83xx_vlan_set_hook is NULL!\n");
	}
	#endif

	return ret;
}

EXPORT_SYMBOL(rtl_register_multi_wan_dev);

/* 
 *	Function Name:	rtl_unregister_multi_wan_dev
 *	Description:	delete hardware information
 *
 *	Return:		SUCCESS	:	set 8676hwnat successfully 
 *				FAILED		:	set 8676hwnat failed 
 */

int rtl_unregister_multi_wan_dev(const char *nifname, int vid)
{
	int ret = 0;
#if defined(CONFIG_RTL_83XX_SUPPORT)
	unsigned int mbr_msk = 0, untag_msk = 0;
#endif

	if (vid < 1 || vid >=VLAN_VID_MASK)
		return FAIL;

	if (!nifname)
		return FAIL;
	
	#if defined(CONFIG_RTL_83XX_SUPPORT)
	if (rtl83xx_vlan_set_hook){
		ret = (*rtl83xx_vlan_set_hook)(vid, mbr_msk, untag_msk,0, 0);
		TRACE("nifname=%s, vid=%d\n", nifname, vid);
	}
	else{
		ERROR("rtl83xx_vlan_set_hook is NULL!\n");
	}
	#endif

	return ret;
}

EXPORT_SYMBOL(rtl_unregister_multi_wan_dev);


/* currently only update for bridge wan, only for external switch */
int rtl_update_multi_wan_port_mapping(char *vifname, int vid, unsigned int member, char enable, unsigned int proto)
{
	int ret = 0, i = 0;
	unsigned int port_mask = 0, tag_mask = 0;
	//unsigned int tmp_vid = 0, tmp_pvid = 0, pvid = 0;
	unsigned int tmp_vid = 0, tmp_pvid = 0;
	unsigned int membermsk = 0, untagmsk = 0;
#if defined(CONFIG_RTL_83XX_SUPPORT)
	unsigned short pri = 0, port_pri = 0, fid = 0;
#endif
	unsigned int board_type = 0;
#if defined(CONFIG_RTK_VLAN_BINDING_SUPPORT)
	struct rtk_lan_vlan_info bridge_lan_info = {0};
#endif

	TRACE("vifname=%s, vid=%d, member=0x%x, enable=%d, proto=%d\n", vifname, vid, member, enable, proto);
	
	if (proto != SMUX_PROTO_BRIDGE)
		return SUCCESS;

	port_mask = member;
	tag_mask = 0; //currently lanside always untag.
	if (rtl_get_board_type_hook){
		board_type = (*rtl_get_board_type_hook)();
		TRACE("board_type=0x%x \n", board_type);
	}
	else{
		ERROR("rtl_get_board_type_hook is NULL!\n");
		return FAIL;
	}
	//v600, exclude lan1(bit0), because lan1 is not external switch port
	if ((board_type & 0xF00) == 0x600){
		port_mask &= ~RTL_ETH_LAN1_BIND_MASK;
	}
	TRACE("port_mask=0x%x, tag_mask=0x%x\n",port_mask, tag_mask);
	//change for external switch
	rtl_change_dev_mask_to_port_mask(&port_mask, &tag_mask);
	TRACE("port_mask=0x%x, tag_mask=0x%x\n",port_mask, tag_mask);

	tmp_vid = (vid == -1)?RTL_WANVLANID:vid;

	#if defined(CONFIG_RTL_83XX_SUPPORT)
	if (rtl83xx_vlan_get_hook){
		ret = (*rtl83xx_vlan_get_hook)(tmp_vid, &membermsk, &untagmsk, &pri, &fid);
		if (ret != SUCCESS){
			ERROR("get tmp_vid=%u, vid=%d failed! \n", tmp_vid, vid);
			return ret;
		}
	}
	else{
		ERROR("rtl83xx_vlan_get_hook is NULL!");
		return FAIL;
	}
	#endif
	
	if (enable){
		membermsk |= port_mask;
		untagmsk |= port_mask;
	}
	else{
		membermsk &= ~port_mask;
		untagmsk &= ~port_mask;
	}

	//configure rtk_lan_vlan_info for bridge wan<->lan vlan binding's pvid
#if defined(CONFIG_RTK_VLAN_BINDING_SUPPORT)
	VLAN_TRACE("=========%s:%d tmp_vid:%d membermsk:%d untagmsk:%d =======\n",__FUNCTION__,__LINE__,tmp_vid, membermsk, untagmsk);
	bridge_lan_info.lan_vid   = tmp_vid;
	bridge_lan_info.wan_vid   = tmp_vid;
	bridge_lan_info.portmask  = member;
	bridge_lan_info.tagmask   = 0;
	
	if(enable)
	{
		bridge_lan_info.br_wan   = 1;
		rtk_add_lan_vlan_info(bridge_lan_info);
	}
	else
	{
		rtk_remove_lan_vlan_info(bridge_lan_info.lan_vid,bridge_lan_info.portmask, bridge_lan_info.tagmask);
	}
#endif
	
	//configure 8367r bridge vlan
	#if defined(CONFIG_RTL_83XX_SUPPORT)
	if (rtl83xx_vlan_set_hook){
		ret = (*rtl83xx_vlan_set_hook)(tmp_vid, membermsk, untagmsk, pri, fid);
		TRACE("tmp_vid=%u, membermsk=0x%x, untagmsk=0x%x, pri=%u, fid=%u ret=%d\n",tmp_vid, membermsk, untagmsk, pri, fid, ret);
		if (ret != SUCCESS)
			return ret;
	}
	else{
		ERROR("rtl83xx_vlan_set_hook is NULL!\n");
		return FAIL;
	}
	#endif

	//update pvid;
	tmp_pvid = (enable?tmp_vid:RTL_LANVLANID);
	for (i = 0; i < MAX_ETH_NUM; i++)
	{
		if ((1<<i) & port_mask){
			#if defined(CONFIG_RTL_83XX_SUPPORT)
			#if 0
			if (rtl83xx_pvid_get_hook){
				ret = (*rtl83xx_pvid_get_hook)(i, &pvid, &port_pri);
				TRACE("i=%d, pvid=%u, port_pri=%u, ret=%d\n",i, pvid, port_pri, ret);
				if (ret != SUCCESS)
					return ret;
			}
			else{
				ERROR("rtl83xx_pvid_get_hook is NULL!\n");
				return FAIL;
			}
			#endif

			if (rtl83xx_pvid_set_hook){
				//fix me ? if enable port-mapping, use vlan group's priority
				ret = (*rtl83xx_pvid_set_hook)(i, tmp_pvid, (enable?pri:0));
				TRACE("i=%d, tmp_pvid=%u, pri=%u, ret=%d\n",i, tmp_pvid, (enable?pri:0), ret);
				if (ret != SUCCESS)
					return ret;
			}
			else{
				ERROR("rtl83xx_pvid_set_hook is NULL!\n");
				return FAIL;
			}
			#endif
		}
	}

	//configure nat lan vid
	#if defined(CONFIG_RTL_83XX_SUPPORT)
	if (rtl83xx_vlan_get_hook){
		ret = (*rtl83xx_vlan_get_hook)(RTL_LANVLANID, &membermsk, &untagmsk, &pri, &fid);
		if (ret != SUCCESS){
			ERROR("get RTL_LANVLANID=%u, vid=%d failed! \n", RTL_LANVLANID, vid);
			return ret;
		}
	}
	else{
		ERROR("rtl83xx_vlan_get_hook is NULL!");
		return FAIL;
	}
	#endif
	
	if (!enable){
		membermsk |= port_mask;
		untagmsk |= port_mask;
	}
	else{
		membermsk &= ~port_mask;
		untagmsk &= ~port_mask;
	}
	#if defined(CONFIG_RTL_83XX_SUPPORT)
	if (rtl83xx_vlan_set_hook){
		ret = (*rtl83xx_vlan_set_hook)(RTL_LANVLANID, membermsk, untagmsk, pri, fid);
		TRACE("RTL_LANVLANID=%u, membermsk=0x%x, untagmsk=0x%x, pri=%u, fid=%u ret=%d\n",RTL_LANVLANID, membermsk, untagmsk, pri, fid, ret);
		if (ret != SUCCESS)
			return ret;
	}
	else{
		ERROR("rtl83xx_vlan_set_hook is NULL!\n");
		return FAIL;
	}
	#endif
	
	return 	ret;
}

int rtl_multi_wan_rx_process(struct sk_buff *skb, unsigned int phy_src_port)
{
	int ret = SUCCESS;
	
	#if defined(CONFIG_RTK_VLAN_PASSTHROUGH_SUPPORT)
	/* if vlan passthrough enabled, donot remove vlan tag
	 * when enabled vlan passthrough, other vlan function should disabled.
	*/
	if (rtk_get_vlan_passthrough_enable())
		return ret;
	#endif
	
	/* for 98F +8367r , portbinding bridge lan port rx may with tag, should remove it, 
	   otherwise tx packets maybe with double tag */
	if ((phy_src_port < CA_MAX_ETHERNET_PORT_NUMBER) && skb && skb->dev && !(skb->dev->priv_flags & IFF_RSMUX))
		{
#if defined(CONFIG_RTK_VLAN_BINDING_SUPPORT)
			if(rtk_lan_vlan_enable)
			{
				ret = rtk_lan_vlan_process_rx(&skb);
			}
			else
#endif
			{
				ret = remove_vlan_tag(&skb);
				if(ret != FAIL)
				{
					ret = SUCCESS;
				}
			}
		}
	
	return ret;
}
EXPORT_SYMBOL(rtl_multi_wan_rx_process);


int rtl_multi_wan_init(void)
{
	TRACE("===========MULTI WAN INIT===================\n");
	rtl_register_multi_wan_hook = &rtl_register_multi_wan_dev;
	rtl_unregister_multi_wan_hook = &rtl_unregister_multi_wan_dev;
	rtl_update_multi_wan_port_mapping_hook = &rtl_update_multi_wan_port_mapping;
	
	
	return 0;
} 

void rtl_multi_wan_exit(void)
{
	
	TRACE("===========MULTI WAN EXIT===================\n");
	rtl_register_multi_wan_hook = NULL;
	rtl_unregister_multi_wan_hook = NULL;
	rtl_update_multi_wan_port_mapping_hook = NULL;

	return;
} 

EXPORT_SYMBOL(rtl_multi_wan_init);
EXPORT_SYMBOL(rtl_multi_wan_exit);


