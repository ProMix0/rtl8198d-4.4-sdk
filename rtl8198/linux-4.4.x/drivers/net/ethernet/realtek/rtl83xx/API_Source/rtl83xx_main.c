/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2010 by Cortina Systems Incorporated.                 */
/***********************************************************************/
/*
 *
 * rtl83xx_main.c
 *
 * $Id: rtl83xx_main.c,v 1.3 2013/02/04 06:41:38 ewang Exp $
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>	/* copy_*_user */
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/delay.h>	/* mdelay() */
#include <linux/skbuff.h>
#include <linux/phy.h>
#include <linux/stat.h>
#include <linux/netdevice.h>
#include <linux/miscdevice.h>
//#include <linux/notifier.h>
//#include <linux/reboot.h>
#include <asm/io.h>		/* mem r/w */
#include <linux/of_gpio.h>
#include <linux/of_mdio.h>

#include <soc/cortina/rtk_vlan_common.h>
#include "rtl8367c_asicdrv_port.h"	/* speed/duplex/mode */
#include "rtl8367c_asicdrv.h"	/* rtl8367c_setAsicReg() */
#include "rtl8367c_asicdrv_cputag.h"	/* rtl8367c_setAsicCputagPosition() */
#include "rtl8367c_asicdrv_vlan.h"	/* rtl8367c_setAsicVlanFilter() */
#include "smi.h"			/* smi_read()/smi_write() */
#include "rtl83xx_ioctl.h"	/* ioctl command IDs */
#include "rtl8367m_vb.h"	/* RTK_CMD_T */
#include "rtk_api_ext.h"
#include "acl.h"
#include "rtk_acl.h"
#include "cpu.h"
#include "dot1x.h"
#include "eee.h"
#include "igmp.h"
#include "interrupt.h"
#include "l2.h"
#include "oam.h"
#include "port.h"
#include "qos.h"
#include "rate.h"
#include "rtk_switch.h"
#include "vlan.h"
#include "svlan.h"
#include "ptp.h"
#include "stat.h"

#if defined(CONFIG_GPIO_CA77XX)
#include <linux/gpio.h>
#endif
#if defined(CONFIG_RTK_DEV_AP)
#include <soc/cortina/rtk_multi_wan_vlan.h>
#endif

#include <soc/cortina/rtk_common_utility.h>

#if defined(CONFIG_RTK_VLAN_PASSTHROUGH_SUPPORT)
#include <soc/cortina/rtk_vlan_passthrough.h>
#endif

#ifdef RTL83XX_DBG
#define PFX     "RTK"
#define MESSAGE(format, args...) printk(KERN_WARNING PFX \
	":%s:%d: " format, __func__, __LINE__ , ## args)
#else
#define MESSAGE(format, args...)
#endif

#define INIT(x) do { \
	if ((ret = x) != 0) { \
		MESSAGE("Initialize %s fail, ret = %d\n", #x, ret); \
		goto fail; \
	} \
} while(0)

/* 83XX RST PIN*/
int rtl83xx_reset_gpio;
int rtl8226_reset_gpio0;
int rtl8226_reset_gpio1;

/* GPIO simulate I2C */
extern rtk_uint32  smi_SCK;
extern rtk_uint32  smi_SDA;;

/* global variables */
spinlock_t sw_lock;
int *ni_driver_state; /* 1: ready, 0: initializing */

rtk_uint32 vportmask;
rtk_uint32 vutpportmask;

#ifdef CONFIG_RTL83XX_MDIO_GPIO
struct mii_bus *rtk_smibus;
#endif

/* external variables */
extern struct net_device *ni_get_device(unsigned char port_id);

#include <soc/cortina/ca_types.h>
typedef ca_uint16_t ca_dev_id_t;
ca_status_t cap_mdio_write(CA_IN ca_dev_id_t device_id,
CA_IN      ca_uint8_t              st_code,
CA_IN      ca_uint8_t              phy_port_addr,
CA_IN      ca_uint8_t              reg_dev_addr,
CA_IN      ca_uint16_t             addr,
CA_IN      ca_uint16_t            data);

ca_status_t cap_mdio_read(CA_IN ca_dev_id_t device_id,
CA_IN      ca_uint8_t              st_code,
CA_IN      ca_uint8_t              phy_port_addr,
CA_IN      ca_uint8_t              reg_dev_addr,
CA_IN      ca_uint16_t             addr,
CA_OUT   ca_uint16_t         *data);

int ni_mdio_write(int phy_addr, int reg_addr, u16 value)
{
	return cap_mdio_write(0, 1, (ca_uint8_t)phy_addr, (ca_uint8_t)reg_addr, 0, value);
}

int ni_mdio_read(int phy_addr, int reg_addr)
{
	int value;
	cap_mdio_read(0, 1, (ca_uint8_t)phy_addr, (ca_uint8_t)reg_addr, 0, (ca_uint16_t *)&value);
	return value;
}

unsigned char eth_mac[6] = {0x00, 0x13, 0x25, 0x00, 0x00, 0x02};

static int rtl83xx_sw_init(void)
{
	/* To be done
	 * 
	 */

	return 0;
}
static int rtl83xx_init_cpu_port(void)
{
	rtk_port_mac_ability_t mac_cfg;
	rtk_mode_ext_t mode;
	rtk_api_ret_t ret = 0;

#ifdef RTK_EXT_RGMII
	/* note: rtk_port_rgmiiDelayExt_set should be called before rtk_port_macForceLinkExt_set
	         to avoid the rxc glitch */
	INIT(rtk_port_rgmiiDelayExt_set(SWITCH_CPU_PORT, 1, 3));
	mode = MODE_EXT_RGMII;
	mac_cfg.forcemode = MAC_FORCE;
#if defined(CONFIG_CORTINA_BOARD_FPGA)
	mac_cfg.speed = SPD_100M;
#elif defined(CONFIG_CORTINA_BOARD_8198F_ENG)
	mac_cfg.speed = SPD_1000M;
#endif
	mac_cfg.duplex = FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = CPORT_TX_PAUSE;
	mac_cfg.rxpause = CPORT_RX_PAUSE;

      /*
         * Ref/eng board connects ext0 to G2 in RGMII mode.
         * valid txDelay for ref board : 0~1, and we choose 1.
         * valid rxDelay for ref board : 2~7, and we choose 7.
         * valid txDelay for eng board : 0~1, and we choose 1.
         * valid rxDelay for eng board : 2~7, and we choose 7.
         */
        INIT(rtk_port_macForceLinkExt_set(SWITCH_CPU_PORT, mode, &mac_cfg));
	//INIT(rtk_port_rgmiiDelayExt_set(SWITCH_CPU_PORT, 1, 3));
#else
	printk("%s: Initialze HSGMII...\n", __func__);

	mode = MODE_EXT_HSGMII;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = PORT_SPEED_2500M;
	mac_cfg.duplex = PORT_FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = CPORT_TX_PAUSE;
	mac_cfg.rxpause = CPORT_RX_PAUSE;
	INIT(rtk_port_macForceLinkExt_set(EXT_PORT0, mode, &mac_cfg));
	msleep(1000);
	//rtk_port_sds_reset(EXT_PORT0);
#endif

	return 0;
fail:
	MESSAGE("Initializaing switch CPU port fail\n");
	return -EPERM;
}

#if 0
/*
 * disable_phy_addr0_response
 * workaround to disable response of PHY address 0.
 * It is only valid for Realtek RTL8211 series PHY.
 */
static void disable_phy_addr0_response(unsigned phy_addr)
{
	unsigned int val;

	//spin_lock(&sw_lock);

	/* REG31 write 0x0007, set to extension page */
	ni_mdio_write(phy_addr, 31, 0x0007);

	/* REG30 write 0x002C, set to extension page 44 */
	ni_mdio_write(phy_addr, 30, 0x002C);

	/*
	 * REG27 write bit[2] =0
	 * disable response PHYAD=0  function.
	 * we should read REG27 and clear bit[2], and write back.
	 */
	val = ni_mdio_read(phy_addr, 27);
	val &= ~(1 << 2);
	ni_mdio_write(phy_addr, 27, val);

	/* REG31 write 0X0000, back to page0 */
	ni_mdio_write(phy_addr, 31, 0x0000);

	//spin_unlock(&sw_lock);

}
#endif

static int rtl83xx_add_cpu_mac(void)
{
	rtk_api_ret_t ret = 0;
	rtk_l2_ucastAddr_t l2_data;
	rtk_mac_t mac;

	memset(&l2_data, 0, sizeof(rtk_l2_ucastAddr_t));

	///TODO: for G3Lite
	//memcpy(mac.octet, eth_mac[CPU_PORT_ID], ETHER_ADDR_LEN);
	memcpy(mac.octet, &(eth_mac[0]), ETHER_ADDR_LEN);

	l2_data.port = SWITCH_CPU_PORT;
	l2_data.is_static = 1;

	INIT(rtk_l2_addr_add(&mac, &l2_data));

	return 0;
fail:
	MESSAGE("Add CPU MAC to switch fail\n");
	return -EPERM;
}

extern unsigned int ca_ni_get_wan_port_id(void);

int rtl83xx_lan_wan_isolation_init(int mode)
{
	rtk_portmask_t pm;
	unsigned int wan_port;
	int i;

	if (mode > REPEATER_MODE)
		return 0;
	
	wan_port = ca_ni_get_wan_port_id();

	for (i=0; i<RTL8367C_PHYNO; i++) {
		rtk_port_isolation_get(i, &pm);

		if (mode != GATEWAY_MODE) {
			pm.bits[0] |= 0x1f;
		}
		else {
			if (i == wan_port) {
				pm.bits[0] = (pm.bits[0] & ~0x1f) | BIT(wan_port);
			}
			else {
				pm.bits[0] = (pm.bits[0] | 0x1f) & ~BIT(wan_port);
			}
		}
		rtk_port_isolation_set(i, &pm);		
	}

	return 0;
}

int rtl83xx_disable_lan_wan_isolation(void)
{
	rtk_portmask_t pm;
	int i, retVal;


	for (i=0; i<=4; i++) {
		//all portmask
		pm.bits[0] = vportmask;
		retVal = rtk_port_isolation_set(i, &pm);		
		printk("[%s:%d] i: %d, retVal: %d, pm.bits[0]: 0x%x\n", __FUNCTION__, __LINE__, i, retVal, pm.bits[0]);
	}

	return 0;
}

/* membermsk and untagmsk are physical port */
int rtl83xx_vlan_set(unsigned int vid, unsigned int membermsk, unsigned int untagmsk, unsigned short pri,unsigned short fid)
{
	int ret = 0;
	rtk_vlan_cfg_t vlan_cfg;
	rtk_portmask_t mbr_msk, untag_msk;

	
	TRACE("vid=%u, membermsk=0x%x, untagmsk=0x%x, pri=%u fid=%u\n", vid, membermsk, untagmsk, pri,fid);
	memset(&vlan_cfg, 0x00, sizeof(rtk_vlan_cfg_t));

	/* the mbr/untag paramter of rtk_vlan_set is logicalmask */	
	if (rtk_switch_portmask_P2L_get(membermsk, &mbr_msk) != RT_ERR_OK)
		return RT_ERR_FAILED;
	
	if (rtk_switch_portmask_P2L_get(untagmsk, &untag_msk) != RT_ERR_OK)
		return RT_ERR_FAILED;
	
	vlan_cfg.mbr.bits[0] = mbr_msk.bits[0];
	vlan_cfg.untag.bits[0] = untag_msk.bits[0];
	vlan_cfg.fid_msti = fid;
	vlan_cfg.vbpri = pri;

	#if 0 //include cpu port or not, decide by caller.
	if (!membermsk&&!untagmsk){
		//do nothing
		TRACE("vid=%u, vlan_cfg.mbr.bits[0]=0x%x, vlan_cfg.untag.bits[0]=0x%x vlan_cfg.fid_msti=%u vlan_cfg.vbpri=%u ret=%d\n", vid, vlan_cfg.mbr.bits[0], vlan_cfg.untag.bits[0],vlan_cfg.fid_msti,vlan_cfg.vbpri, ret);
	}
	else{
		vlan_cfg.mbr.bits[0] |= (1<<SWITCH_CPU_PORT);
		TRACE("vid=%u, vlan_cfg.mbr.bits[0]=0x%x, vlan_cfg.untag.bits[0]=0x%x vlan_cfg.fid_msti=%u SWITCH_CPU_PORT=%u ret=%d\n", vid, vlan_cfg.mbr.bits[0], vlan_cfg.untag.bits[0],vlan_cfg.fid_msti, SWITCH_CPU_PORT,ret);
	}
	#endif

	ret = rtk_vlan_set(vid, &vlan_cfg);
	TRACE("vid=%u, vlan_cfg.mbr.bits[0]=0x%x, vlan_cfg.untag.bits[0]=0x%x vlan_cfg.vbpri=%u vlan_cfg.fid_msti=%u ret=%d\n", vid, vlan_cfg.mbr.bits[0], vlan_cfg.untag.bits[0],vlan_cfg.vbpri,vlan_cfg.fid_msti, ret);
	
	return ret;
}
EXPORT_SYMBOL(rtl83xx_vlan_set);

/* membermsk and untagmsk are physical ports */
int rtl83xx_vlan_get(unsigned int vid, unsigned int *membermsk, unsigned int *untagmsk, unsigned short *pri,unsigned short *fid)
{
	rtk_vlan_cfg_t vlan;
	int ret = 0;

	//vlan.mbr and vlan.untag are logical ports
	ret = rtk_vlan_get(vid, &vlan);
	if (ret != RT_ERR_OK){
		ERROR("vid=%u ret=%d\n", vid,ret);
		return ret;
	}
	
	TRACE("%s %d vid=%u, vlan.mbr=0x%x, vlan.untag=0x%x, vlan.vbpri=%u, vlan.fid_msti=%u\n", __func__, __LINE__, vid, vlan.mbr.bits[0], vlan.untag.bits[0], vlan.vbpri, vlan.fid_msti);
	
	if (membermsk){
		//change to physical ports
		if ((ret = rtk_switch_portmask_L2P_get(&(vlan.mbr), membermsk)) != RT_ERR_OK)
			return ret;
		TRACE("%s %d vid=%u, membermsk=0x%x\n", __func__, __LINE__, vid, *membermsk);
	}

	if (untagmsk){
		if ((ret = rtk_switch_portmask_L2P_get(&(vlan.untag), untagmsk)) != RT_ERR_OK)
				return ret;
		TRACE("%s %d vid=%u, untagmsk=0x%x\n", __func__, __LINE__, vid, *untagmsk);
	}

	if (pri)
		*pri = vlan.vbpri;

	if (fid)
		*fid = vlan.fid_msti;
	
	return ret;
}

int rtl83xx_pvid_set(unsigned int port, unsigned int pvid, unsigned int priority)
{
	int ret = 0;
	unsigned int logic_port = rtk_switch_port_P2L_get(port);
	
	if(logic_port == UNDEFINE_PORT){
		return FAIL;
	}
		
	ret = rtk_vlan_portPvid_set(logic_port, pvid, priority);
	TRACE("port=%u, logic_port=%u pvid=%u, priority=%u ret=%d \n", port, logic_port, pvid, priority, ret);
	
	return ret;
}

int rtl83xx_pvid_get(unsigned int port, unsigned int *pvid, unsigned int *priority)
{
	int ret = 0;
	unsigned int logic_port = rtk_switch_port_P2L_get(port);
	
	if(logic_port == UNDEFINE_PORT){
		return FAIL;
	}
			
	ret = rtk_vlan_portPvid_get(logic_port, pvid, priority);
	TRACE("port=%u, logic_port=%u pvid=%u, priority=%u ret=%d \n", port,logic_port, *pvid, *priority, ret);

	return ret;
}

/* physical port mask  */
int rtl83xx_get_cpu_portmask(void)
{
	unsigned int portmask = 0;

	portmask = (1<<UTP_PORT6) | (1<<UTP_PORT7);
	TRACE("vportmask=0x%x, vutpportmask=0x%x, portmask=0x%x\n", vportmask, vutpportmask, portmask);
	
	return portmask;
}

/* call this function after set OP_MODE  
 * vlan passthrough only consider pure bridge mode
*/
int rtl83xx_vlan_passthrough_set(int enable)
{
	rtk_vlan_cfg_t vlanCfg;
	rtk_api_ret_t retVal = 0;
	int i = 0;
	rtk_uint32 port_mask = 0;
	
	for (i = 0; i <= RTL8367C_VIDMAX; i++)
	{
		//skip reserved vlan group
		if ((i == RTL_DEFAULT_VLANID)
			|| (i == RTL_LANVLANID))
			continue;

		memset(&vlanCfg, 0x00, sizeof(rtk_vlan_cfg_t));
		if (enable){
			//vlan passthrough enabled, set other vlan groups tagged.
			port_mask = vportmask ;
			vlanCfg.mbr.bits[0] = port_mask;	
			vlanCfg.fid_msti = RTL_LAN_FID;
		}
		else{
			//vlan passthrough disabled, clear other vlan groups.
		}
		
		retVal = rtk_vlan_set(i, &vlanCfg); 
	}
	

	return retVal;
}
int rtl83xx_clear_vlan(void)
{
	int i = 0, retVal = 0;
	rtk_vlan_cfg_t vlanCfg;
	
	for (i = 0; i <= RTL8367C_VIDMAX; i++)
	{
		//clear all vlan group
		memset(&vlanCfg, 0x00, sizeof(rtk_vlan_cfg_t));
		retVal=rtk_vlan_set(i, &vlanCfg); 
	}

	return retVal;
}

/*
   when enable vlan
   1. disable lan wan isolation, for lan wan bridge can reach wire speed.
   2. try use fid......
   3. try not set wan port in lan vlan group......
*/
int rtl83xx_default_vlan_set(int mode)
{
	rtk_vlan_cfg_t vlanCfg;
	rtk_api_ret_t retVal = 0;
	int i = 0;
	unsigned int wan_port = 0;
	rtk_uint32 port_mask = 0;
	

	wan_port = ca_ni_get_wan_port_id();

	//first clear all vlan
	rtl83xx_clear_vlan();

	//set default vlan group 1
	memset(&vlanCfg, 0x00, sizeof(rtk_vlan_cfg_t));
	port_mask = vportmask ;
	vlanCfg.mbr.bits[0] = vlanCfg.untag.bits[0] = port_mask;	
	vlanCfg.fid_msti = RTL_DEFAULT_FID;

	retVal=rtk_vlan_set(RTL_DEFAULT_VLANID, &vlanCfg); //all vlan's fid is 0 
	//printk("%s %d retVal=%d vid=%u portmask=0x%x \n", __func__, __LINE__,retVal, RTL_DEFAULT_VLANID,port_mask);

	//set lan vlan group
	memset(&vlanCfg, 0x00, sizeof(rtk_vlan_cfg_t));
	if (mode != GATEWAY_MODE)
		port_mask = vportmask;
	else
		port_mask = (vportmask & (~BIT(wan_port)));
	vlanCfg.mbr.bits[0] = vlanCfg.untag.bits[0] = port_mask;	
	vlanCfg.fid_msti = RTL_LAN_FID;
	retVal=rtk_vlan_set(RTL_LANVLANID, &vlanCfg); //all vlan's fid is 0 
	//printk("%s %d retVal=%d vid=%u portmask=0x%x \n", __func__, __LINE__,retVal, RTL_LANVLANID,port_mask);

	//set wan vlan group
	if (mode == GATEWAY_MODE)
	{
		memset(&vlanCfg, 0x00, sizeof(rtk_vlan_cfg_t));
		port_mask = ((vportmask & (~vutpportmask)) | BIT(wan_port));
		vlanCfg.mbr.bits[0] = port_mask;	
		vlanCfg.untag.bits[0] = port_mask;
		vlanCfg.fid_msti = RTL_WAN_FID;
		retVal=rtk_vlan_set(RTL_WANVLANID, &vlanCfg); //all vlan's fid is 0 
		//printk("%s %d retVal=%d vid=%u portmask=0x%x \n", __func__, __LINE__,retVal, RTL_WANVLANID,port_mask);
	}
	/* set pvid */	
	for(i=0;i<8;i++)
	{
		int logic_port = rtk_switch_port_P2L_get(i);
		if(logic_port == UNDEFINE_PORT){
			continue;
		}
		
		if (mode != GATEWAY_MODE)
		{
			if ((logic_port == SWITCH_CPU_PORT) || (logic_port ==EXT_PORT1)){
				retVal=rtk_vlan_portPvid_set(logic_port, RTL_DEFAULT_VLANID,0);
				//printk("%s %d retVal=%d port=%u pvid=%u \n", __func__, __LINE__, retVal, i, RTL_DEFAULT_VLANID);
			}
			else{
			retVal=rtk_vlan_portPvid_set(logic_port, RTL_LANVLANID,0);
			//printk("%s %d retVal=%d port=%u pvid=%u \n", __func__, __LINE__, retVal, i, RTL_LANVLANID);
		}
		}
		else
		{
			if (logic_port == wan_port){
				retVal=rtk_vlan_portPvid_set(logic_port, RTL_WANVLANID,0);
				//printk("%s %d retVal=%d port=%u pvid=%u \n", __func__, __LINE__, retVal, i, RTL_WANVLANID);
			}
			else if (logic_port == SWITCH_CPU_PORT){
				retVal=rtk_vlan_portPvid_set(logic_port, RTL_DEFAULT_VLANID,0);
				//printk("%s %d retVal=%d port=%u pvid=%u \n", __func__, __LINE__, retVal, i, RTL_DEFAULT_VLANID);
			}
			else {
				retVal=rtk_vlan_portPvid_set(logic_port, RTL_LANVLANID,0);
				//printk("%s %d retVal=%d port=%u pvid=%u \n", __func__, __LINE__, retVal, i, RTL_LANVLANID);
			}
		}
	} 

	return 0;
}

int rtl83xx_op_mode_set(int mode)
{
	int ret = 0;

	/* v5xx/v6xx,  8226 as wan, 83xx always as lan */
	#ifdef CONFIG_RTL_8226_SUPPORT
	mode = BRIDGE_MODE;
	#endif
	
	#if !defined(RTK_DISABLE_LANWAN_ISOLATION)
	ret = rtl83xx_lan_wan_isolation_init(mode);
	#else
	ret = rtl83xx_default_vlan_set(mode);
	#endif

	return ret;
}

void rtl83xx_hook_init(void)
{
	TRACE("========83XX HOOK INIT========\n");
	#ifdef CONFIG_RTL_MULTI_ETH_WAN
	rtl83xx_vlan_set_hook = &rtl83xx_vlan_set;
	rtl83xx_vlan_get_hook = &rtl83xx_vlan_get;
	rtl83xx_pvid_set_hook = &rtl83xx_pvid_set;
	rtl83xx_pvid_get_hook = &rtl83xx_pvid_get;
	rtl83xx_get_cpu_portmask_hook = &rtl83xx_get_cpu_portmask;
	#endif

	#if defined(CONFIG_RTK_VLAN_PASSTHROUGH_SUPPORT)
	rtl83xx_vlan_passthrough_set_hook = &rtl83xx_vlan_passthrough_set;
	#endif

	rtl83xx_op_mode_set_hook = &rtl83xx_op_mode_set;
	
	return;
}

void rtl83xx_hook_exit(void)
{
	TRACE("========83XX HOOK EXIT========\n");
	#ifdef CONFIG_RTL_MULTI_ETH_WAN	
	rtl83xx_vlan_set_hook = NULL;
	rtl83xx_vlan_get_hook = NULL;
	rtl83xx_pvid_set_hook = NULL;
	rtl83xx_pvid_get_hook = NULL;
	rtl83xx_get_cpu_portmask_hook = NULL;
	#endif
	
	#if defined(CONFIG_RTK_VLAN_PASSTHROUGH_SUPPORT)
	rtl83xx_vlan_passthrough_set_hook = NULL;
	#endif
	
	rtl83xx_op_mode_set_hook = NULL;

	return;
}
#if 0
static int rtl83xx_notify_sys(struct notifier_block *this,
				unsigned long code, void *unused)
{
	if (code == SYS_DOWN || code == SYS_HALT) {
		rtl8367c_setAsicReg(RTL8367C_REG_CHIP_RESET, (1<<RTL8367C_CHIP_RST_OFFSET));
	}
	return NOTIFY_DONE;
}

static struct notifier_block rtl83xx_notifier = {
	.notifier_call = rtl83xx_notify_sys
};
#endif

#if defined(CONFIG_ARCH_RTL8198F) && (defined(CONFIG_RTL_8367S_SUPPORT) || defined(CONFIG_RTL_8226_SUPPORT))

static rtk_uint32 rtl83xx_sds_rd(rtk_uint32 page, rtk_uint32 reg)
{
	rtk_uint32 data;
	rtk_uint32 sds_addr = ((page & 0x3f) << 5) + (reg & 0x1f);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_ADR, sds_addr);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_CMD, 0x0080);
	rtl8367c_getAsicReg(RTL8367C_REG_SDS_INDACS_DATA, &data);
	return data;
}

static void rtl83xx_sds_wr(rtk_uint32 page, rtk_uint32 reg, rtk_uint32 data)
{
	rtk_uint32 sds_addr = ((page & 0x3f) << 5) + (reg & 0x1f);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_ADR, sds_addr);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_DATA, data);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_CMD, 0x00C0);
	msleep(100);
	/* printk("rtl83xx_sds_wr 0x%04x 0x%04x\n", data, rtl83xx_sds_rd(page, reg)); */
	return;
}

static void rtl83xx_sdsMode_hgmii_cfg(void)
{
	u32 data;
	/* HSGMII { set mac6_sel_hsgmii 0x1  ;  set mac6_sel_sgmii 0x0 } */
	/* 1000M { set mac_speed 0x2  } */
	/* 1 - enable   { set sds_rxfc 0x1 ;  set sds_txfc 0x1 } */
	/* 1  - up   { set sds_link 0x1  } */
	data = 0x1400 | (1 << 11) | (0x2 << 7) | (1 << 13) | (1 <<14)
		| (1 << 9);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_MISC, data);
}

static void rtl83xx_sds_patch_hsgmii_mode(void)
{
	/* purpose :  to patch RTL8367SB/SC SERDES PHY parameter for HSGMII mode 
	 * patch table :  (patch data is get from SD2 switch team - Arshian 2017/08)  
	 * ---------------
	 * APGE REG DATA :
	 * ---------------
	 * 0x28  0x0 0x82F0
	 * 0x28  0x1 0xF195
	 * 0x28  0x2 0x31A2
	 * 0x28  0x3 0x7960
	 * 0x28  0x4 0x9728
	 * 0x21  0x3 0x9D85
	 * 0x21  0x4 0xD810
	 * 0x1   0xE bit[15]=0## 0x0   0x1 0x0FA0
	 * Reg 0x00D0=0x7
	 * Reg 0x0399=0x7
	 * Reg 0x03FA=0x7
	 */
	 u32 buf;
	/* update parameter : */
	rtl83xx_sds_wr(0x28, 0x0, 0x82f0);
	rtl83xx_sds_wr(0x28, 0x1, 0xf195);
	rtl83xx_sds_wr(0x28, 0x2, 0x31a2);
	rtl83xx_sds_wr(0x28, 0x3, 0x7960);
	rtl83xx_sds_wr(0x28, 0x4, 0x9728);
	rtl83xx_sds_wr(0x21, 0x3, 0x9d85);
	rtl83xx_sds_wr(0x21, 0x4, 0xd810);
	buf = rtl83xx_sds_rd(0x1, 0xe);
	rtl83xx_sds_wr(0x1, 0xe, buf & ~(1 << 15) & 0xffff);
	rtl83xx_sds_wr(0x0, 0x1, 0x0FA0);
	rtl8367c_setAsicReg(0x00d0, 0x7);
	rtl8367c_setAsicReg(RTL8367C_REG_PORT6_EGRESSBW_CTRL1, 0x7);
	rtl8367c_setAsicReg(0x03FA, 0x7);
	/* SDS RXCDR reset : */
	rtl83xx_sds_wr(0x0, 0x0, 0x1401);
	rtl83xx_sds_wr(0x0, 0x0, 0x1403);
	/* BitError reset : */
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_DATA, 0x0004);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_ADR, 0x0400);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_CMD, 0x00C0);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_DATA, 0x0000);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_ADR, 0x0400);
	rtl8367c_setAsicReg(RTL8367C_REG_SDS_INDACS_CMD, 0x00C0);
}

void rtl83xx_hsgmii_init(void)
{
	printk("----------rtl83xx_hsgmii_init-----------\n");
	rtl83xx_sds_patch_hsgmii_mode();
	/* ##<4> cfg RTL8367: RGMII ext2  (MAC7) disable  */
	rtl8367c_setAsicReg(RTL8367C_REG_DIGITAL_INTERFACE_SELECT_1, 0); 
	rtl83xx_sdsMode_hgmii_cfg();
 	/* ##<5> 8367 SDS RXCDR reset : */
 	rtl83xx_sds_wr(0x0, 0x0, 0x1401);
 	rtl83xx_sds_wr(0x0, 0x0, 0x1403);
	/* set LED */
	rtl8367c_setAsicReg(RTL8367C_REG_LED_CONFIGURATION, 0x96);
}

extern void ca_ni_init_98f_hsgmii(void);
extern void ca_ni_reset_98f_cdr(void);

extern int rtl8226_2g5_setup(int phy_id);
extern int rtl8226_sds_patch(int phy_id, int linkst);
extern void rtl8226_init_timer(void);

int setup_hsgmii(void)
{
	ca_ni_init_98f_hsgmii();
#ifdef CONFIG_RTL_8367S_SUPPORT
	rtl83xx_hsgmii_init();
#endif
#ifdef CONFIG_RTL_8226_SUPPORT /* rtl8226 */
#ifdef CONFIG_RTL_8226_USE_PORT1
	rtl8226_2g5_setup(0x06);
#endif
#ifdef CONFIG_RTL_8226_USE_PORT0
	rtl8226_2g5_setup(0x07);
#endif
	rtl8226_init_timer();
#endif
	ca_ni_reset_98f_cdr();
	return RT_ERR_OK;
}

#endif

static int rtl83xx_hw_init(void)
{
	rtk_api_ret_t ret = 0;
	/* unsigned int vendor_id, chip_id, phy_id; */
	rtk_portmask_t portmask;//	,portmask2;
	rtk_port_phy_ability_t phy_ability;
	//int i, cnt = 0;
	int port;

#ifdef CONFIG_CS752X_VIRTUAL_NETWORK_INTERFACE
#ifdef CONFIG_CS752X_VIRTUAL_NI_DBLTAG
	rtk_svlan_memberCfg_t svlan_cfg;
#endif

#ifndef RTL83XX_L2_ISOLATION
	unsigned int acl_rule_num;
	rtk_filter_field_t acl_field;
	rtk_filter_cfg_t acl_cfg;
	rtk_filter_action_t acl_act;
#endif
#endif

#if defined(CONFIG_CORTINA_REFERENCE) || \
	defined(CONFIG_CORTINA_REFERENCE_B) || \
	defined(CONFIG_CORTINA_REFERENCE_S)
	rtk_port_phy_ability_t phy_ability;
#endif


#ifdef CONFIG_CORTINA_FPGA
	printk("rtl83xx: Skip to init switch chip\n");
	return 0;
#endif
#if 0
	//To be implementation.
	/* Disable response PHYAD=0 function of RTL8211 series PHY */
	if (CPU_PORT_ID != 0) {
		/* check PHY of Golden Gate port 0 */
		spin_lock(&sw_lock);
		vendor_id = ni_mdio_read(GE_PORT0_PHY_ADDR, 2);
		chip_id = ni_mdio_read(GE_PORT0_PHY_ADDR, 3);
		spin_unlock(&sw_lock);
		phy_id = vendor_id << 16 | chip_id;
		if ((phy_id & PHY_ID_MASK) == PHY_ID_RTL8211)
			disable_phy_addr0_response(GE_PORT0_PHY_ADDR);
	}
	if (CPU_PORT_ID != 1) {
		/* check PHY of Golden Gate port 1 */
		spin_lock(&sw_lock);
		vendor_id = ni_mdio_read(GE_PORT1_PHY_ADDR, 2);
		chip_id = ni_mdio_read(GE_PORT1_PHY_ADDR, 3);
		spin_unlock(&sw_lock);
		phy_id = vendor_id << 16 | chip_id;
		if ((phy_id & PHY_ID_MASK) == PHY_ID_RTL8211)
			disable_phy_addr0_response(GE_PORT1_PHY_ADDR);
	}
	if (CPU_PORT_ID != 2) {
		/* check PHY of Golden Gate port 2 */
		spin_lock(&sw_lock);
		vendor_id = ni_mdio_read(GE_PORT2_PHY_ADDR, 2);
		chip_id = ni_mdio_read(GE_PORT2_PHY_ADDR, 3);
		spin_unlock(&sw_lock);
		phy_id = vendor_id << 16 | chip_id;
		if ((phy_id & PHY_ID_MASK) == PHY_ID_RTL8211)
			disable_phy_addr0_response(GE_PORT2_PHY_ADDR);
	}
#endif

	/*
	 * Reset RTL83XX switch IC
	 */
#if defined(CONFIG_GPIO_CA77XX)	 
	gpio_set_value(rtl83xx_reset_gpio, 0);
#if defined(CONFIG_RTL_8226_USE_PORT0)
	gpio_set_value(rtl8226_reset_gpio0, 0);
#endif
#if defined(CONFIG_RTL_8226_USE_PORT1)
	gpio_set_value(rtl8226_reset_gpio1, 0);
#endif
	mdelay(10);
	gpio_set_value(rtl83xx_reset_gpio, 1);
#if defined(CONFIG_RTL_8226_USE_PORT0)
	gpio_set_value(rtl8226_reset_gpio0, 1);
#endif
#if defined(CONFIG_RTL_8226_USE_PORT1)
	gpio_set_value(rtl8226_reset_gpio1, 1);
#endif
	mdelay(100);
#endif

	/*
	 * Init switch chip
	 */
	INIT(rtk_switch_init());
	rtk_get_halctrl_member(&vportmask, &vutpportmask);
	INIT(rtk_port_phyEnableAll_set(DISABLED));
	INIT(rtk_qos_init(8));
	INIT(rtk_vlan_init());
	INIT(rtk_svlan_init());
	INIT(rtk_oam_init());
	INIT(rtk_ptp_init());
	INIT(rtk_l2_init());
	INIT(rtk_filter_igrAcl_init());
	rtl83xx_init_acl();
	INIT(rtk_eee_init());
//	INIT(rtk_igmp_init());
	portmask.bits[0] = (1 << SWITCH_CPU_PORT);
	//INIT(rtk_igmp_static_router_port_set(portmask));
	INIT(rtk_stat_global_reset());
#if defined(CONFIG_ARCH_RTL8198F) && (defined(CONFIG_RTL_8367S_SUPPORT) || defined(CONFIG_RTL_8226_SUPPORT))
	INIT(setup_hsgmii());
#endif
	INIT(rtl83xx_init_cpu_port());
	INIT(rtl83xx_add_cpu_mac());

#if defined(CONFIG_CORTINA_CPU_TAG)
	/* config rtk-tag */
	INIT(rtk_cpu_tagPort_set(SWITCH_CPU_PORT, CPU_INSERT_TO_ALL));
	INIT(rtk_cpu_tagLength_set(CPU_LEN_4BYTES));
	INIT(rtk_cpu_enable_set(1));
	
	#if !defined(RTK_DISABLE_LANWAN_ISOLATION)
	INIT(rtl83xx_lan_wan_isolation_init(GATEWAY_MODE));
	#else
		#ifdef CONFIG_RTL_8226_SUPPORT
		// wan is not in 8367S/8367RB
		rtl83xx_default_vlan_set(BRIDGE_MODE);
		#else
		rtl83xx_default_vlan_set(rtk_get_op_mode());
		#endif
	#endif

#endif
	memset(&phy_ability, 0, sizeof(rtk_port_phy_ability_t));

	/* only disable Full_1000 capability but enable others */
	phy_ability.AutoNegotiation = 1;
	phy_ability.Half_10 = 1;
	phy_ability.Full_10 = 1;
	phy_ability.Half_100 = 1;
	phy_ability.Full_100 = 1;
#if defined(CONFIG_CORTINA_BOARD_8198F_ENG)
	phy_ability.Full_1000 = 1;
#endif
	phy_ability.FC = 1;

	for (port = 0; port < RTL8367C_PHYNO; port++) {
		INIT(rtk_83xx_port_phyAutoNegoAbility_set(port, &phy_ability));
	}

        INIT(rtk_port_phyEnableAll_set(ENABLED));

#if defined(CONFIG_CORTINA_CPU_TAG)
	/* disable cpu port's mac addr learning ability */
	rtk_l2_limitLearningCnt_set(SWITCH_CPU_PORT, 0);

	/* disable unknown unicast/mcast/bcast flooding between LAN ports */
	rtl8367c_setAsicReg(RTL8367C_REG_UNDA_FLOODING_PMSK, BIT(rtk_switch_port_L2P_get(SWITCH_CPU_PORT)));
	rtl8367c_setAsicReg(RTL8367C_REG_UNMCAST_FLOADING_PMSK, BIT(rtk_switch_port_L2P_get(SWITCH_CPU_PORT)));
	rtl8367c_setAsicReg(RTL8367C_REG_BCAST_FLOADING_PMSK, BIT(rtk_switch_port_L2P_get(SWITCH_CPU_PORT)));
#endif

#if defined(CONFIG_CORTINA_SHORT_IPG) && !defined(CONFIG_RTL_8367S_SUPPORT)
	ret = rtl8367c_setAsicPortSmallIpg(rtk_switch_port_L2P_get(SWITCH_CPU_PORT), ENABLED);
#endif

#if defined(CONFIG_ARCH_RTL8198F) && defined(CONFIG_RTK_DEV_AP)
	{
	extern unsigned int rtl_get_board_type(void);
	unsigned int board_info = rtl_get_board_type();
	rtk_uint32 data;
	if ((board_info & 0xFFF) == 0x312) {
		/* invert port 0 LED for V312 board */
		rtl8367c_getAsicRegBits(RTL8367C_REG_LED_ACTIVE_LOW_CFG0, RTL8367C_PORT0_LED_ACTIVE_LOW_MASK, &data);
		data ^= 0x7;
		rtl8367c_setAsicRegBits(RTL8367C_REG_LED_ACTIVE_LOW_CFG0, RTL8367C_PORT0_LED_ACTIVE_LOW_MASK, data);				
	}				
	}
#endif

	//register_reboot_notifier(&rtl83xx_notifier);

	return 0;
fail:
	MESSAGE("Initialization fail\n");
	return -EPERM;
}

static long rtl83xx_ioctl(struct file *file,
			  unsigned int cmd, unsigned long arg)
{

	void __user *argp = (void __user *)arg;
	RTK_CMD_T rtk_cmd;
	rtk_api_ret_t ret = 0;
	int i;
	/* rtk_portmask_t portMask; */

	if (cmd != SIOCDEVPRIVATE) {
	    MESSAGE("It is not private command (0x%X). cmd = (0x%X)\n",
		    SIOCDEVPRIVATE, cmd);
	    return -EOPNOTSUPP;
	}

	if (copy_from_user((void *)&rtk_cmd, argp, sizeof(rtk_cmd))) {
	    MESSAGE("Copy from user space fail\n");
	    return -EFAULT;
	}

#ifdef CONFIG_CORTINA_FPGA
	if (rtk_cmd.cmd < RTK_MAX) {
		printk("Receive IOCTL command %d\n", rtk_cmd.cmd);
		rtk_cmd.ret = RT_ERR_OK;
		return 0;
	}
#endif

	switch (rtk_cmd.cmd) {
	case RTK_SWITCH_INIT:
		ret = rtk_switch_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_SWITCH_MAX_PKTLEN_SET:
		ret = rtk_switch_portMaxPktLen_set(	
			rtk_cmd.para.switch_max_pktlen.port,
			rtk_cmd.para.switch_max_pktlen.speed,
			rtk_cmd.para.switch_max_pktlen.cfgId);
		rtk_cmd.ret = ret;
		break;
	case RTK_SWITCH_MAX_PKTLEN_GET:
		ret = rtk_switch_portMaxPktLen_get(
			rtk_cmd.para.switch_max_pktlen.port,
			rtk_cmd.para.switch_max_pktlen.speed,
			&rtk_cmd.para.switch_max_pktlen.cfgId);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_IGR_BW_SET:
		ret = rtk_rate_igrBandwidthCtrlRate_set(
			rtk_cmd.para.rate_igr_bw.port,
			rtk_cmd.para.rate_igr_bw.rate,
			rtk_cmd.para.rate_igr_bw.ifg_include,
			rtk_cmd.para.rate_igr_bw.fc_enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_IGR_BW_GET:
		ret = rtk_rate_igrBandwidthCtrlRate_get(
			rtk_cmd.para.rate_igr_bw.port,
			&rtk_cmd.para.rate_igr_bw.rate,
			&rtk_cmd.para.rate_igr_bw.ifg_include,
			&rtk_cmd.para.rate_igr_bw.fc_enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_BW_SET:
		ret = rtk_rate_egrBandwidthCtrlRate_set(
			rtk_cmd.para.rate_egr_bw.port,
			rtk_cmd.para.rate_egr_bw.rate,
			rtk_cmd.para.rate_egr_bw.ifg_include
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_BW_GET:
		ret = rtk_rate_egrBandwidthCtrlRate_get(
			rtk_cmd.para.rate_egr_bw.port,
			&rtk_cmd.para.rate_egr_bw.rate,
			&rtk_cmd.para.rate_egr_bw.ifg_include
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_QBW_EN_SET:
		ret = rtk_rate_egrQueueBwCtrlEnable_set(
			rtk_cmd.para.rate_egr_qbw_en.port,
			rtk_cmd.para.rate_egr_qbw_en.queue,
			rtk_cmd.para.rate_egr_qbw_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_QBW_EN_GET:
		ret = rtk_rate_egrQueueBwCtrlEnable_get(
			rtk_cmd.para.rate_egr_qbw_en.port,
			rtk_cmd.para.rate_egr_qbw_en.queue,
			&rtk_cmd.para.rate_egr_qbw_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_QBW_SET:
		ret = rtk_rate_egrQueueBwCtrlRate_set(
			rtk_cmd.para.rate_egr_qbw.port,
			rtk_cmd.para.rate_egr_qbw.queue,
			rtk_cmd.para.rate_egr_qbw.index
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_QBW_GET:
		ret = rtk_rate_egrQueueBwCtrlRate_get(
			rtk_cmd.para.rate_egr_qbw.port,
			rtk_cmd.para.rate_egr_qbw.queue,
			&rtk_cmd.para.rate_egr_qbw.index
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STORM_CTRL_RATE_SET:
//		ret = rtk_storm_controlRate_set(		//removed by leo, new driver not support 2015-07-06
//			rtk_cmd.para.storm_ctrl_rate.port,
//			rtk_cmd.para.storm_ctrl_rate.storm_type,
//			rtk_cmd.para.storm_ctrl_rate.rate,
//			rtk_cmd.para.storm_ctrl_rate.ifg_include,
//			rtk_cmd.para.storm_ctrl_rate.mode
//			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STORM_CTRL_RATE_GET:
//		ret = rtk_storm_controlRate_get(		//removed by leo, new driver not support 2015-07-06
//			rtk_cmd.para.storm_ctrl_rate.port,
//			rtk_cmd.para.storm_ctrl_rate.storm_type,
//			&rtk_cmd.para.storm_ctrl_rate.rate,
//			&rtk_cmd.para.storm_ctrl_rate.ifg_include,
//			rtk_cmd.para.storm_ctrl_rate.mode
//			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STORM_BYPASS_SET:
		ret = rtk_storm_bypass_set(
			rtk_cmd.para.storm_bypass.type,
			rtk_cmd.para.storm_bypass.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STORM_BYPASS_GET:
		ret = rtk_storm_bypass_get(
			rtk_cmd.para.storm_bypass.type,
			&rtk_cmd.para.storm_bypass.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_INIT:
		ret = rtk_qos_init(
			rtk_cmd.para.qos.queue_num
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PRI_SEL_SET:
		ret = rtk_qos_priSel_set(
			rtk_cmd.para.qos_pri_sel.index,
			&rtk_cmd.para.qos_pri_sel.priDec
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PRI_SEL_GET:
		ret = rtk_qos_priSel_get(
			rtk_cmd.para.qos_pri_sel.index,
			&rtk_cmd.para.qos_pri_sel.priDec
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_PRI_REMAP_SET:
		ret = rtk_qos_1pPriRemap_set(
			rtk_cmd.para.qos_dot1p_pri_remap.dot1p_pri,
			rtk_cmd.para.qos_dot1p_pri_remap.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_PRI_REMAP_GET:
		ret = rtk_qos_1pPriRemap_get(
			rtk_cmd.para.qos_dot1p_pri_remap.dot1p_pri,
			&rtk_cmd.para.qos_dot1p_pri_remap.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_PRI_REMAP_SET:
		ret = rtk_qos_dscpPriRemap_set(
			rtk_cmd.para.qos_dscp_pri_remap.dscp,
			rtk_cmd.para.qos_dscp_pri_remap.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_PRI_REMAP_GET:
		ret = rtk_qos_dscpPriRemap_get(
			rtk_cmd.para.qos_dscp_pri_remap.dscp,
			&rtk_cmd.para.qos_dscp_pri_remap.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PORT_PRI_SET:
		ret = rtk_qos_portPri_set(
			rtk_cmd.para.qos_port_pri.port,
			rtk_cmd.para.qos_port_pri.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PORT_PRI_GET:
		ret = rtk_qos_portPri_get(
			rtk_cmd.para.qos_port_pri.port,
			&rtk_cmd.para.qos_port_pri.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_QUE_NUM_SET:
		ret = rtk_qos_queueNum_set(
			rtk_cmd.para.qos_que_num.port,
			rtk_cmd.para.qos_que_num.queue_num
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_QUE_NUM_GET:
		ret = rtk_qos_queueNum_get(
			rtk_cmd.para.qos_que_num.port,
			&rtk_cmd.para.qos_que_num.queue_num
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PRI_MAP_SET:
		ret = rtk_qos_priMap_set(
			rtk_cmd.para.qos_pri_map.queue_num,
			&rtk_cmd.para.qos_pri_map.pri2qid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PRI_MAP_GET:
		ret = rtk_qos_priMap_get(
			rtk_cmd.para.qos_pri_map.queue_num,
			&rtk_cmd.para.qos_pri_map.pri2qid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_SCHE_QUE_SET:
		ret = rtk_qos_schedulingQueue_set(
			rtk_cmd.para.qos_sche_que.port,
			&rtk_cmd.para.qos_sche_que.qweights
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_SCHE_QUE_GET:
		ret = rtk_qos_schedulingQueue_get(
			rtk_cmd.para.qos_sche_que.port,
			&rtk_cmd.para.qos_sche_que.qweights
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_REMARK_EN_SET:
		ret = rtk_qos_1pRemarkEnable_set(
			rtk_cmd.para.qos_dot1p_remark_en.port,
			rtk_cmd.para.qos_dot1p_remark_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_REMARK_EN_GET:
		ret = rtk_qos_1pRemarkEnable_get(
			rtk_cmd.para.qos_dot1p_remark_en.port,
			&rtk_cmd.para.qos_dot1p_remark_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_REMARK_SET:
		ret = rtk_qos_1pRemark_set(
			rtk_cmd.para.qos_dot1p_remark.int_pri,
			rtk_cmd.para.qos_dot1p_remark.dot1p_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_REMARK_GET:
		ret = rtk_qos_1pRemark_get(
			rtk_cmd.para.qos_dot1p_remark.int_pri,
			&rtk_cmd.para.qos_dot1p_remark.dot1p_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_REMARK_EN_SET:
		ret = rtk_qos_dscpRemarkEnable_set(
			rtk_cmd.para.qos_dscp_remark_en.port,
			rtk_cmd.para.qos_dscp_remark_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_REMARK_EN_GET:
		ret = rtk_qos_dscpRemarkEnable_get(
			rtk_cmd.para.qos_dscp_remark_en.port,
			&rtk_cmd.para.qos_dscp_remark_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_REMARK_SET:
		ret = rtk_qos_dscpRemark_set(
			rtk_cmd.para.qos_dscp_remark.int_pri,
			rtk_cmd.para.qos_dscp_remark.dscp
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_REMARK_GET:
		ret = rtk_qos_dscpRemark_get(
			rtk_cmd.para.qos_dscp_remark.int_pri,
			&rtk_cmd.para.qos_dscp_remark.dscp
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_AN_ABILITY_SET:
		ret = rtk_83xx_port_phyAutoNegoAbility_set(
			rtk_cmd.para.port_phy_an_ability.port,
			&rtk_cmd.para.port_phy_an_ability.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_AN_ABILITY_GET:
		ret = rtk_83xx_port_phyAutoNegoAbility_get(
			rtk_cmd.para.port_phy_an_ability.port,
			&rtk_cmd.para.port_phy_an_ability.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_FORCE_ABILITY_SET:
		ret = rtk_port_phyForceModeAbility_set(
			rtk_cmd.para.port_phy_force_ability.port,
			&rtk_cmd.para.port_phy_force_ability.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_FORCE_ABILITY_GET:
		ret = rtk_port_phyForceModeAbility_get(
			rtk_cmd.para.port_phy_force_ability.port,
			&rtk_cmd.para.port_phy_force_ability.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_STATUS_GET:
		ret = rtk_port_phyStatus_get(
			rtk_cmd.para.port_phy_status.port,
			&rtk_cmd.para.port_phy_status.linkStatus,
			&rtk_cmd.para.port_phy_status.speed,
			&rtk_cmd.para.port_phy_status.duplex
			);
#if 0
		printk("%s: port=%d, linkStatus=%d, speed=%d, duplex=%d\n", __func__,
			rtk_cmd.para.port_phy_status.port, rtk_cmd.para.port_phy_status.linkStatus, rtk_cmd.para.port_phy_status.speed, rtk_cmd.para.port_phy_status.duplex);
		printk("%s: ret=%d\n", __func__, ret);
#endif
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_MACFORCELINKEXT_SET:
		ret = rtk_port_macForceLink_set(
			rtk_cmd.para.port_mac_force_link_ext.port,
			&rtk_cmd.para.port_mac_force_link_ext.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_MACFORCELINKEXT_GET:
		ret = rtk_port_macForceLink_get(
			rtk_cmd.para.port_mac_force_link_ext.port,
			&rtk_cmd.para.port_mac_force_link_ext.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_MACSTATUS_GET:
		ret = rtk_port_macStatus_get(
			rtk_cmd.para.port_mac_status.port,
			&rtk_cmd.para.port_mac_status.status
			);
		rtk_cmd.ret = ret;
		break;

	case RTK_PORT_PHYREG_SET:
		ret = rtk_83xx_port_phyReg_set(
			rtk_cmd.para.port_phy_reg.port,
			rtk_cmd.para.port_phy_reg.reg,
			rtk_cmd.para.port_phy_reg.data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHYREG_GET:
		ret = rtk_83xx_port_phyReg_get(
			rtk_cmd.para.port_phy_reg.port,
			rtk_cmd.para.port_phy_reg.reg,
			&rtk_cmd.para.port_phy_reg.data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ADMIN_STATE_SET:
		ret = rtk_port_adminEnable_set(
			rtk_cmd.para.port_admin_state.port,
			rtk_cmd.para.port_admin_state.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ADMIN_STATE_GET:
		ret = rtk_port_adminEnable_get(
			rtk_cmd.para.port_admin_state.port,
			&rtk_cmd.para.port_admin_state.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ISOLATION_SET:
		ret = rtk_port_isolation_set(
			rtk_cmd.para.port_isolation.port,
			&rtk_cmd.para.port_isolation.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ISOLATION_GET:
		ret = rtk_port_isolation_get(
			rtk_cmd.para.port_isolation.port,
			&rtk_cmd.para.port_isolation.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT0_SET:
		ret = rtk_port_rgmiiDelayExt_set(
			rtk_cmd.para.port_rgmii_delay.port,
			rtk_cmd.para.port_rgmii_delay.txDelay,
			rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT0_GET:
		ret = rtk_port_rgmiiDelayExt_get(
			rtk_cmd.para.port_rgmii_delay.port,
			&rtk_cmd.para.port_rgmii_delay.txDelay,
			&rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT1_SET:
//		ret = rtk_port_rgmiiDelayExt1_set(	//removed by leo, new driver not support 2015-07-06
//			rtk_cmd.para.port_rgmii_delay.txDelay,
//			rtk_cmd.para.port_rgmii_delay.rxDelay
//			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT1_GET:
//		ret = rtk_port_rgmiiDelayExt1_get(	//removed by leo, new driver not support 2015-07-06
//			&rtk_cmd.para.port_rgmii_delay.txDelay,
//			&rtk_cmd.para.port_rgmii_delay.rxDelay
//			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT_SET:
		ret = rtk_port_rgmiiDelayExt_set(
			rtk_cmd.para.port_rgmii_delay.port,
			rtk_cmd.para.port_rgmii_delay.txDelay,
			rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT_GET:
		ret = rtk_port_rgmiiDelayExt_get(
			rtk_cmd.para.port_rgmii_delay.port,
			&rtk_cmd.para.port_rgmii_delay.txDelay,
			&rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ENABLE_ALL_SET:
		ret = rtk_port_phyEnableAll_set(
			rtk_cmd.para.port_enable_all.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ENABLE_ALL_GET:
		ret = rtk_port_phyEnableAll_get(
			&rtk_cmd.para.port_enable_all.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_INIT:
		ret = rtk_vlan_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_SET:
		ret = rtk_vlan_set(
			rtk_cmd.para.vlan.vid,
			&rtk_cmd.para.vlan.pVlanCfg
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_GET:
		ret = rtk_vlan_get(
			rtk_cmd.para.vlan.vid,
			&rtk_cmd.para.vlan.pVlanCfg
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PVID_SET:
		ret = rtk_vlan_portPvid_set(
			rtk_cmd.para.vlan_pvid.port,
			rtk_cmd.para.vlan_pvid.pvid,
			rtk_cmd.para.vlan_pvid.priority
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PVID_GET:
		ret = rtk_vlan_portPvid_get(
			rtk_cmd.para.vlan_pvid.port,
			&rtk_cmd.para.vlan_pvid.pvid,
			&rtk_cmd.para.vlan_pvid.priority
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PORT_IGRFILTER_EN_SET:
		ret = rtk_vlan_portIgrFilterEnable_set(
			rtk_cmd.para.vlan_port_igrfilter_en.port,
			rtk_cmd.para.vlan_port_igrfilter_en.igr_filter
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PORT_IGRFILTER_EN_GET:
		ret = rtk_vlan_portIgrFilterEnable_get(
			rtk_cmd.para.vlan_port_igrfilter_en.port,
			&rtk_cmd.para.vlan_port_igrfilter_en.igr_filter
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PORT_AFT_SET:
		ret = rtk_vlan_portAcceptFrameType_set(
			rtk_cmd.para.vlan_port_aft.port,
			rtk_cmd.para.vlan_port_aft.accept_frame_type
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PORT_AFT_GET:
		ret = rtk_vlan_portAcceptFrameType_get(
			rtk_cmd.para.vlan_port_aft.port,
			&rtk_cmd.para.vlan_port_aft.accept_frame_type
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_BASED_PRI_SET:
//		ret = rtk_vlan_vlanBasedPriority_set(		//removed by leo, new driver not support 2015-07-06
//			rtk_cmd.para.vlan_based_pri.vid,
//			rtk_cmd.para.vlan_based_pri.priority
//			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_BASED_PRI_GET:
//		ret = rtk_vlan_vlanBasedPriority_get(		//removed by leo, new driver not support 2015-07-06
//			rtk_cmd.para.vlan_based_pri.vid,
//			&rtk_cmd.para.vlan_based_pri.priority
//			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_TAGMODE_SET:
		ret = rtk_vlan_tagMode_set(
			rtk_cmd.para.vlan_tagmode.port,
			rtk_cmd.para.vlan_tagmode.tag_mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_TAGMODE_GET:
		ret = rtk_vlan_tagMode_get(
			rtk_cmd.para.vlan_tagmode.port,
			&rtk_cmd.para.vlan_tagmode.tag_mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_MBR_FILTER_SET:
		ret = rtl8367c_setAsicVlanFilter(
			rtk_cmd.para.vlan_mbr_filter.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_MBR_FILTER_GET:
		ret = rtl8367c_getAsicVlanFilter(
			&rtk_cmd.para.vlan_mbr_filter.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STP_INIT:
		//ret = rtk_stp_init();		//removed by leo, new driver not support 2015-07-06
		rtk_cmd.ret = ret;
		break;
	case RTK_STP_MSTP_STATE_SET:
		ret = rtk_stp_mstpState_set(
			rtk_cmd.para.stp_mstp_state.msti,
			rtk_cmd.para.stp_mstp_state.port,
			rtk_cmd.para.stp_mstp_state.stp_state
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STP_MSTP_STATE_GET:
		ret = rtk_stp_mstpState_get(
			rtk_cmd.para.stp_mstp_state.msti,
			rtk_cmd.para.stp_mstp_state.port,
			&rtk_cmd.para.stp_mstp_state.stp_state
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_INIT:
		ret = rtk_l2_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_ADDR_ADD:
		ret = rtk_l2_addr_add(
			&rtk_cmd.para.l2_addr.mac,
			&rtk_cmd.para.l2_addr.l2_data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_ADDR_GET:
		ret = rtk_l2_addr_get(
			&rtk_cmd.para.l2_addr.mac,
			&rtk_cmd.para.l2_addr.l2_data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_ADDR_NEXT_GET:
		ret = rtk_l2_addr_next_get(
			rtk_cmd.para.l2_addr.read_method,
			rtk_cmd.para.l2_addr.port,
			&rtk_cmd.para.l2_addr.address,
			&rtk_cmd.para.l2_addr.l2_data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_ADDR_DEL:
		ret = rtk_l2_addr_del(
			&rtk_cmd.para.l2_addr.mac,
			&rtk_cmd.para.l2_addr.l2_data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_MCADDR_ADD:
		ret = rtk_l2_mcastAddr_add(
			&rtk_cmd.para.l2_mcaddr.pMcastAddr
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_MCADDR_GET:
		ret = rtk_l2_mcastAddr_get(
			&rtk_cmd.para.l2_mcaddr.pMcastAddr
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_MCADDR_NEXT_GET:
		ret = rtk_l2_mcastAddr_next_get(
			&rtk_cmd.para.l2_mcaddr.address,
			&rtk_cmd.para.l2_mcaddr.pMcastAddr
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_MCADDR_DEL:
		ret = rtk_l2_mcastAddr_del(
			&rtk_cmd.para.l2_mcaddr.pMcastAddr
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_AGING_ENABLE_SET:
		ret = rtk_l2_agingEnable_set(
			rtk_cmd.para.l2_aging_en.port,
			rtk_cmd.para.l2_aging_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_AGING_ENABLE_GET:
		ret = rtk_l2_agingEnable_get(
			rtk_cmd.para.l2_aging_en.port,
			&rtk_cmd.para.l2_aging_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_LMT_SET:
		ret = rtk_l2_limitLearningCnt_set(
			rtk_cmd.para.l2_lrn_lmt.port,
			rtk_cmd.para.l2_lrn_lmt.mac_cnt
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_LMT_GET:
		ret = rtk_l2_limitLearningCnt_get(
			rtk_cmd.para.l2_lrn_lmt.port,
			&rtk_cmd.para.l2_lrn_lmt.mac_cnt
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_ACTION_SET:
		ret = rtk_l2_limitLearningCntAction_set(
			rtk_cmd.para.l2_lrn_action.port,
			rtk_cmd.para.l2_lrn_action.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_ACTION_GET:
		ret = rtk_l2_limitLearningCntAction_get(
			rtk_cmd.para.l2_lrn_action.port,
			&rtk_cmd.para.l2_lrn_action.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_CNT_GET:
		ret = rtk_l2_learningCnt_get(
			rtk_cmd.para.l2_lrn_cnt.port,
			&rtk_cmd.para.l2_lrn_cnt.mac_cnt
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_FLOOD_PORTS_SET:
		ret = rtk_l2_floodPortMask_set(
			rtk_cmd.para.l2_flood_ports.flood_type,
			&rtk_cmd.para.l2_flood_ports.flood_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_FLOOD_PORTS_GET:
		ret = rtk_l2_floodPortMask_get(
			rtk_cmd.para.l2_flood_ports.flood_type,
			&rtk_cmd.para.l2_flood_ports.flood_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LOCALPKT_PMT_SET:
		ret = rtk_l2_localPktPermit_set(
			rtk_cmd.para.l2_localpkt_pmt.port,
			rtk_cmd.para.l2_localpkt_pmt.permit
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LOCALPKT_PMT_GET:
		ret = rtk_l2_localPktPermit_get(
			rtk_cmd.para.l2_localpkt_pmt.port,
			&rtk_cmd.para.l2_localpkt_pmt.permit
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_AGING_SET:
		ret = rtk_l2_aging_set(
			rtk_cmd.para.l2_aging.aging_time
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_AGING_GET:
		ret = rtk_l2_aging_get(
			&rtk_cmd.para.l2_aging.aging_time
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_INIT:
		ret = rtk_svlan_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_SVC_PORT_ADD:
		ret = rtk_svlan_servicePort_add(
			rtk_cmd.para.svlan_svc_port.port
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_SVC_PORT_GET:
		ret = rtk_svlan_servicePort_get(
			&rtk_cmd.para.svlan_svc_port.svlan_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_SVC_PORT_DEL:
		ret = rtk_svlan_servicePort_del(
			rtk_cmd.para.svlan_svc_port.port
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_MBRPORT_ENTRY_SET:
		ret = rtk_svlan_memberPortEntry_set(
			rtk_cmd.para.svlan_mbrport_entry.svid_idx,
			&rtk_cmd.para.svlan_mbrport_entry.svlan_cfg
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_MBRPORT_ENTRY_GET:
		ret = rtk_svlan_memberPortEntry_get(
			rtk_cmd.para.svlan_mbrport_entry.svid_idx,
			&rtk_cmd.para.svlan_mbrport_entry.svlan_cfg
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_DEF_SVID_SET:
		ret = rtk_svlan_defaultSvlan_set(
			rtk_cmd.para.svlan_def_svid.port,
			rtk_cmd.para.svlan_def_svid.svid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_DEF_SVID_GET:
		ret = rtk_svlan_defaultSvlan_get(
			rtk_cmd.para.svlan_def_svid.port,
			&rtk_cmd.para.svlan_def_svid.svid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_UNMATCH_ACTION_SET:
		ret = rtk_svlan_unmatch_action_set(
			rtk_cmd.para.svlan_unmatch_act.action,
			rtk_cmd.para.svlan_unmatch_act.svid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_UNMATCH_ACTION_GET:
		ret = rtk_svlan_unmatch_action_get(
			&rtk_cmd.para.svlan_unmatch_act.action,
			&rtk_cmd.para.svlan_unmatch_act.svid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_ENABLE_SET:
		ret = rtk_cpu_enable_set(
			rtk_cmd.para.cpu_tag.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_ENABLE_GET:
		ret = rtk_cpu_enable_get(
			&rtk_cmd.para.cpu_tag.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_TAG_PORT_SET:
		ret = rtk_cpu_tagPort_set(
			rtk_cmd.para.cpu_tag_port.port,
			rtk_cmd.para.cpu_tag_port.mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_TAG_PORT_GET:
		ret = rtk_cpu_tagPort_get(
			&rtk_cmd.para.cpu_tag_port.port,
			&rtk_cmd.para.cpu_tag_port.mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_TAG_POSITION_SET:
		ret = rtl8367c_setAsicCputagPosition(
			rtk_cmd.para.cpu_tag_position.position
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_TAG_POSITION_GET:
		ret = rtl8367c_getAsicCputagPosition(
			&rtk_cmd.para.cpu_tag_position.position
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_MIRROR_PORT_BASED_SET:
		ret = rtk_83xx_mirror_portBased_set(
			rtk_cmd.para.mirror_portbased.mirroring_port,
			&rtk_cmd.para.mirror_portbased.mirrored_rx_portmask,
			&rtk_cmd.para.mirror_portbased.mirrored_tx_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_MIRROR_PORT_BASED_GET:
		ret = rtk_mirror_portBased_get(
			&rtk_cmd.para.mirror_portbased.mirroring_port,
			&rtk_cmd.para.mirror_portbased.mirrored_rx_portmask,
			&rtk_cmd.para.mirror_portbased.mirrored_tx_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_MIRROR_PORT_ISO_SET:
		ret = rtk_83xx_mirror_portIso_set(
			rtk_cmd.para.mirror_portiso.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_MIRROR_PORT_ISO_GET:
		ret = rtk_mirror_portIso_get(
			&rtk_cmd.para.mirror_portiso.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_GLOBAL_RESET:
		ret = rtk_stat_global_reset();
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_PORT_RESET:
		ret = rtk_stat_port_reset(
			rtk_cmd.para.stat_port.port
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_GLOBAL_GET:
		ret = rtk_stat_global_get(
			rtk_cmd.para.stat_global.cntr_idx,
			&rtk_cmd.para.stat_global.cntr
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_GLOBAL_GETALL:
		ret = rtk_stat_global_getAll(
			&rtk_cmd.para.stat_global.global_cntrs
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_PORT_GET:
		ret = rtk_stat_port_get(
			rtk_cmd.para.stat_port.port,
			rtk_cmd.para.stat_port.cntr_idx,
			&rtk_cmd.para.stat_port.cntr
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_PORT_GETALL:
		ret = rtk_83xx_stat_port_getAll(
			rtk_cmd.para.stat_port.port,
			&rtk_cmd.para.stat_port.port_cntrs
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_INIT:
		ret = rtk_filter_igrAcl_init();
		rtk_cmd.ret = ret;
		break;
	/* We don't support to add acl filter field from user space.
	 * User should prepare filter fields as an array and use RTK_ACL_CFG_ADD
	 */
/*
	case RTK_ACL_FIELD_ADD:
		ret = rtk_filter_igrAcl_field_add(
			&rtk_cmd.para.acl_field.filter_cfg,
			&rtk_cmd.para.acl_field.filter_field
			);
		rtk_cmd.ret = ret;
		break;
*/
	/*
	 * User should prepare acl filter fields in array and assign field_num.
	 */
	case RTK_ACL_CFG_ADD:
		for (i = 0; i < rtk_cmd.para.acl_cfg.field_num; i++) {
			ret = rtk_filter_igrAcl_field_add(
				&rtk_cmd.para.acl_cfg.filter_cfg,
				&rtk_cmd.para.acl_cfg.filter_field[i]
				);
			if (ret) {
				rtk_cmd.ret = ret;
				break;
			}

		}
		ret = rtk_filter_igrAcl_cfg_add(
			rtk_cmd.para.acl_cfg.filter_id,
			&rtk_cmd.para.acl_cfg.filter_cfg,
			&rtk_cmd.para.acl_cfg.action,
			&rtk_cmd.para.acl_cfg.ruleNum
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_CFG_DEL:
		ret = rtk_filter_igrAcl_cfg_del(
			rtk_cmd.para.acl_cfg.filter_id
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_CFG_DELALL:
		ret = rtk_filter_igrAcl_cfg_delAll();
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_CFG_GET:
		ret = rtk_filter_igrAcl_cfg_get(
			rtk_cmd.para.acl_cfg.filter_id,
			&rtk_cmd.para.acl_cfg.filter_cfg_raw,
			&rtk_cmd.para.acl_cfg.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_UMACTION_SET:
		ret = rtk_filter_igrAcl_unmatchAction_set(
			rtk_cmd.para.acl_umaction.port,
			rtk_cmd.para.acl_umaction.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_UMACTION_GET:
		ret = rtk_filter_igrAcl_unmatchAction_get(
			rtk_cmd.para.acl_umaction.port,
			&rtk_cmd.para.acl_umaction.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_STATE_SET:
		ret = rtk_filter_igrAcl_state_set(
			rtk_cmd.para.acl_state.port,
			rtk_cmd.para.acl_state.state
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_STATE_GET:
		ret = rtk_filter_igrAcl_state_get(
			rtk_cmd.para.acl_state.port,
			&rtk_cmd.para.acl_state.state
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_EEE_INIT:
		ret = rtk_eee_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_INIT:
		ret = rtk_igmp_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_STATE_SET:
		ret = rtk_igmp_state_set(
			rtk_cmd.para.igmp_state.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_STATE_GET:
		ret = rtk_igmp_state_get(
			&rtk_cmd.para.igmp_state.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_STATIC_ROUTER_PORT_SET:
		ret = rtk_igmp_static_router_port_set(
			&rtk_cmd.para.igmp_router_port.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_STATIC_ROUTER_PORT_GET:
		ret = rtk_igmp_static_router_port_get(
			&rtk_cmd.para.igmp_router_port.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_PROTOCOL_SET:
		ret = rtk_igmp_protocol_set(
			rtk_cmd.para.igmp_protocol.port,
			rtk_cmd.para.igmp_protocol.protocol,
			rtk_cmd.para.igmp_protocol.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_PROTOCOL_GET:
		ret = rtk_igmp_protocol_get(
			rtk_cmd.para.igmp_protocol.port,
			rtk_cmd.para.igmp_protocol.protocol,
			&rtk_cmd.para.igmp_protocol.action
			);
		rtk_cmd.ret = ret;
		break;

	/******* MDIO READ/WRITE CMD **************/
	case PHY_REG_READ:
		spin_lock(&sw_lock);
		rtk_cmd.para.mdio.data = ni_mdio_read(
			rtk_cmd.para.mdio.phy_addr,
			rtk_cmd.para.mdio.reg_addr
			);

		///TODO: debug
		printk("%s: rtk_cmd.para.mdio.phy_addr=%d, rtk_cmd.para.mdio.reg_addr=%d, rtk_cmd.para.mdio.data=%d\n", __func__,
			rtk_cmd.para.mdio.phy_addr, rtk_cmd.para.mdio.reg_addr, rtk_cmd.para.mdio.data);
		spin_unlock(&sw_lock);
		rtk_cmd.ret = 0;
		break;
	case PHY_REG_WRITE:
		spin_lock(&sw_lock);
		ret = ni_mdio_write(
			rtk_cmd.para.mdio.phy_addr,
			rtk_cmd.para.mdio.reg_addr,
			rtk_cmd.para.mdio.data
			);
		spin_unlock(&sw_lock);
		rtk_cmd.ret = ret;
		break;
	case SWITCH_REG_READ:
		ret = smi_read(
			rtk_cmd.para.mdio.reg_addr,
			&rtk_cmd.para.mdio.data
			);
		rtk_cmd.ret = ret;
		break;
	case SWITCH_REG_WRITE:
		ret = smi_write(
			rtk_cmd.para.mdio.reg_addr,
			rtk_cmd.para.mdio.data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_PORT_SDS_RESET:
		ret = rtk_port_sds_reset(rtk_cmd.para.sds_reset.port);
		rtk_cmd.ret = ret;
		break;
        case RTK_STAT_PORT_SGMII_LINK_STATUS_GET:
		ret = rtk_port_sgmiiLinkStatus_get(rtk_cmd.para.sgmii_link_status.port, &rtk_cmd.para.sgmii_link_status.SignalDetect,
				&rtk_cmd.para.sgmii_link_status.Sync, &rtk_cmd.para.sgmii_link_status.Link);
		rtk_cmd.ret = ret;
		break;
	default:
		rtk_cmd.ret = -EPERM;
	}

	if (copy_to_user(argp, (void *)&rtk_cmd, sizeof(rtk_cmd))) {
	    MESSAGE("Copy to user space fail\n");
	    return -EFAULT;
	}

	return 0;
} /* rtl83xx_ioctl */

static int rtl83xx_open(struct inode *inode, struct file *file)
{
	MESSAGE("RTL83xx switch open success\n\n");
	return 0;
}

static int rtl83xx_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations rtl83xx_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = rtl83xx_ioctl,
	.open = rtl83xx_open,
	.release = rtl83xx_release,
};

static struct miscdevice rtl83xx_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SWITCH_DEVICE_NAME,
	.fops = &rtl83xx_fops,
};


static int rtl83xx_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int ret = 0;
#ifdef CONFIG_RTL83XX_MDIO_GPIO
	struct device_node *bus_node;
#endif

	rtl83xx_reset_gpio = of_get_named_gpio_flags(np, "reset-gpio", 0, &flags);
	if (rtl83xx_reset_gpio == -EPROBE_DEFER) {
		ret = rtl83xx_reset_gpio;
		goto fail;
	}

	if (gpio_is_valid(rtl83xx_reset_gpio)) {
		//printk("[%s - %d]rtl83xx_reset-gpio[%d] \n", __func__, __LINE__, rtl83xx_reset_gpio);
		ret = devm_gpio_request_one(dev, rtl83xx_reset_gpio, (GPIOF_DIR_OUT | GPIOF_EXPORT_DIR_FIXED), "rtl83xx_reset-gpio");
		if (ret)
			goto fail;
	}
#ifdef CONFIG_RTL_8226_USE_PORT0
	rtl8226_reset_gpio0 = of_get_named_gpio_flags(np, "reset-gpio", 1, &flags);
	if (rtl8226_reset_gpio0 == -EPROBE_DEFER) {
		ret = rtl8226_reset_gpio0;
		goto fail;
	}

	if (gpio_is_valid(rtl8226_reset_gpio0)) {
		printk("[%s - %d]rtl83xx_reset-gpio[%d] \n", __func__, __LINE__, rtl8226_reset_gpio0);
		ret = devm_gpio_request_one(dev, rtl8226_reset_gpio0, (GPIOF_DIR_OUT | GPIOF_EXPORT_DIR_FIXED), "rtl8226_port0_reset-gpio");
		if (ret)
			goto fail;
	}
#endif
#ifdef CONFIG_RTL_8226_USE_PORT1
	rtl8226_reset_gpio1 = of_get_named_gpio_flags(np, "reset-gpio", 2, &flags);
	if (rtl8226_reset_gpio1 == -EPROBE_DEFER) {
		ret = rtl8226_reset_gpio1;
		goto fail;
	}

	if (gpio_is_valid(rtl8226_reset_gpio1)) {
		printk("[%s - %d]rtl83xx_reset-gpio[%d] \n", __func__, __LINE__, rtl8226_reset_gpio1);
		ret = devm_gpio_request_one(dev, rtl8226_reset_gpio1, (GPIOF_DIR_OUT | GPIOF_EXPORT_DIR_FIXED), "rtl8226_port1_reset-gpio");
		if (ret)
			goto fail;
	}
#endif

#ifdef CONFIG_RTL83XX_I2C
	/* GPIO simulate I2C */
	smi_SCK = of_get_named_gpio_flags(np, "smi-gpio", 0, &flags);
	if (smi_SCK == -EPROBE_DEFER) {
		ret = smi_SCK;
		goto fail;
	}
	if (gpio_is_valid(smi_SCK)) {
		printk("[%s - %d] gpio_as_smi_sck [%d] \n", __func__, __LINE__, smi_SCK);
		ret  = devm_gpio_request_one(dev, smi_SCK, (GPIOF_DIR_OUT | GPIOF_EXPORT_DIR_FIXED), "rtl83xx-smi-sck-gpio");
		if (ret)
			goto fail;
	}
	smi_SDA = of_get_named_gpio_flags(np, "smi-gpio", 1, &flags);
	if (smi_SDA == -EPROBE_DEFER) {
		ret = smi_SDA;
		goto fail;
	}
	if (gpio_is_valid(smi_SDA)) {
		printk("[%s - %d] gpio_as_smi_sda [%d] \n", __func__, __LINE__, smi_SDA);
		ret  = devm_gpio_request_one(dev, smi_SDA, (GPIOF_DIR_OUT), "rtl83xx-smi-sda-gpio");
		if (ret)
			goto fail;
	}
#endif /* CONFIG_RTL83XX_I2C */
	
#ifdef CONFIG_RTL83XX_MDIO_GPIO
	bus_node = of_parse_phandle(dev->of_node, "rtk-smibus", 0);
	if (!bus_node) {
		ret = -ENODEV;
		goto fail;
	}

	rtk_smibus = of_mdio_find_bus(bus_node);
	if (rtk_smibus == NULL) {
		ret = -EPROBE_DEFER;
		goto fail;
	}
	printk("%s: rtk_smibus->name=%s rtk_smibus->id=%s\n", __func__, rtk_smibus->name, rtk_smibus->id);
#endif
	return 0;

fail:
	printk("%s fail error: %x\n", __func__, ret);
	return ret;
}

static int rtl83xx_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	devm_gpio_free(dev, rtl83xx_reset_gpio);
#ifdef CONFIG_RTL_8226_USE_PORT0
	devm_gpio_free(dev, rtl8226_reset_gpio0);
#endif
#ifdef CONFIG_RTL_8226_USE_PORT1
	devm_gpio_free(dev, rtl8226_reset_gpio1);
#endif
	return 0;
}

static const struct of_device_id rtl83xx_of_match[] = {
	{ .compatible = "realtek,rtl83xx" },
	{ },
};

MODULE_DEVICE_TABLE(of, rtl83xx_of_match);

static struct platform_driver rtl83xx_driver = {
	.probe = rtl83xx_probe,
	.remove	= rtl83xx_remove,
	.driver = {
		.name		= "rtl83xx",
		.owner		= THIS_MODULE,
		.of_match_table	= rtl83xx_of_match,
	},
};

static int __init rtl83xx_module_init(void)
{
	int ret = 0;

	spin_lock_init(&sw_lock);

	ret = platform_driver_register(&rtl83xx_driver);
	if (ret) {
		MESSAGE("register rtl83xx error: %x\n", ret);
		goto out;
	}

	ret = rtl83xx_sw_init();
	if (ret) {
		MESSAGE("Switch software init error: %x\n", ret);
		goto out;
	}

	ret = rtl83xx_hw_init();
	if (ret) {
		MESSAGE("Switch hardware init error: %x\n", ret);
		goto out;
	}

	ret = misc_register(&rtl83xx_miscdev);
	if (ret) {
		MESSAGE("Switch misc device register error: %x\n", ret);
		goto out;
	}


	ret = rtl83xx_proc_init();
	if (ret) {
		MESSAGE("Switch proc init error: %x\n", ret);
		goto out;
	}

	//init hook functions
	rtl83xx_hook_init();

	return 0;
out:
	return ret;
}

module_init(rtl83xx_module_init);

static void __exit rtl83xx_module_exit(void)
{
	misc_deregister(&rtl83xx_miscdev);
	platform_driver_unregister(&rtl83xx_driver);
	rtl83xx_proc_exit();
	rtl83xx_hook_exit();
}

module_exit(rtl83xx_module_exit);

MODULE_AUTHOR("Eric Wang <eric.wang@cortina-systems.com>");
MODULE_LICENSE("GPL");
