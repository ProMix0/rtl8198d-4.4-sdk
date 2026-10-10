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


#include "rtl8367c_asicdrv_port.h"	/* speed/duplex/mode */
#include "rtl8367c_asicdrv.h"	/* rtl8367c_setAsicReg() */
#include "rtl8367c_asicdrv_cputag.h"	/* rtl8367c_setAsicCputagPosition() */
#include "rtl8367c_asicdrv_vlan.h"	/* rtl8367c_setAsicVlanFilter() */
#include "smi.h"			/* smi_read()/smi_write() */
#include "rtl83xx_ioctl.h"	/* ioctl command IDs */
#include "rtl8367m_vb.h"	/* RTK_CMD_T */
#include "rtk_api_ext.h"
#include "acl.h"
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
#include "rtk_acl.h"
#include <soc/cortina/ca_types.h>
#include <soc/cortina/rtk_common_utility.h>

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

/* external variables */
extern struct net_device *ni_get_device(unsigned char port_id);

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

/* 
 * return type definition
 */

#define	PROC_READ_RETURN_VALUE		0
#define SUCCESS 0

/***************************************************************************
 * phyreg
 ***************************************************************************/

void rtl_get_83xx_snr(void)
{
	int i, phy_id;
	unsigned long flags;
	rtk_port_mac_ability_t sts;
	unsigned int sum_snr_a = 0, sum_snr_b = 0, sum_snr_c = 0, sum_snr_d = 0, snr_tmp=0;

	local_irq_save(flags);
	for (phy_id=0; phy_id <=RTK_PHY_ID_MAX; phy_id++)
	{
		memset(&sts, 0, sizeof(rtk_port_mac_ability_t));
		if ((rtk_port_macStatus_get(phy_id, &sts)==0) && (sts.link==1))
		{
			for (i=0; i<10; i++)
			{
				if (rtl8367c_getAsicPHYOCPReg(phy_id, 0xA8C0, &snr_tmp) == RT_ERR_OK) {
					sum_snr_a += snr_tmp;
				} else {
					printk("%s[%d], rtl8367c_getAsicPHYOCPReg() Failed\n", __FUNCTION__, __LINE__);
					local_irq_restore(flags);
					return;
				}

				if (rtl8367c_getAsicPHYOCPReg(phy_id, 0xA9C0, &snr_tmp) == RT_ERR_OK) {
					sum_snr_b += snr_tmp;
				} else {
					printk("%s[%d], rtl8367c_getAsicPHYOCPReg() Failed\n", __FUNCTION__, __LINE__);
					local_irq_restore(flags);
					return;
				}

				if (rtl8367c_getAsicPHYOCPReg(phy_id, 0xAAC0, &snr_tmp) == RT_ERR_OK) {
					sum_snr_c += snr_tmp;
				} else {
					printk("%s[%d], rtl8367c_getAsicPHYOCPReg() Failed\n", __FUNCTION__, __LINE__);
					local_irq_restore(flags);
					return;
				}

				if (rtl8367c_getAsicPHYOCPReg(phy_id, 0xABC0, &snr_tmp) == RT_ERR_OK) {
					sum_snr_d += snr_tmp;
				} else {
					printk("%s[%d], rtl8367c_getAsicPHYOCPReg() Failed\n", __FUNCTION__, __LINE__);
					local_irq_restore(flags);
					return;
				}
			}

			sum_snr_a = sum_snr_a/10;
			sum_snr_b = sum_snr_b/10;
			sum_snr_c = sum_snr_c/10;
			sum_snr_d = sum_snr_d/10;

			printk("Port[%d] link speed is %s, CH_A_SNR is %d, CH_B_SNR is %d, CH_C_SNR is %d, CH_D_SNR is %d\n", 
				    phy_id, (sts.speed)==0x1?"100M":((sts.speed)==0x2?"1G": ((sts.speed) ==0x0?"10M":"Unkown")), 
				    sum_snr_a, sum_snr_b, sum_snr_c, sum_snr_d);
		}
	}
	printk("\n## NOTE: snr value translating to dB value is by: -(10 * log10(snr/pow(2,18)))\n");

	local_irq_restore(flags);
	return;
}

int rtl_8367r_vlan_read(void)
{
	int  i = 0, ret = 0;
	unsigned int pvid = 0, priority = 0;
	//rtk_l2_addr_table_t l2_entry;
	rtk_uint32 address = 0; 
	rtk_l2_ucastAddr_t l2_api_data; 
	int logic_port = 0;
	rtk_data_t efid = 0;
	rtk_portmask_t portmsk;
	unsigned int membermsk = 0, untagmsk = 0, phy_port_msk = 0;
	unsigned short pri = 0, fid = 0;
	
	//unsigned long flags;

	//local_irq_save(flags);
#if 1
	printk("%s\n", "vlan:");
	for (i=1; i <= 1000; i++)
	{
		if(i!=1 && i!=8 && i!=9 && i%100!=0 && i!=110 && i!=210 && i!=310 && i!=410)
			continue;

		/* mbr or untagmbr show physical port */		
		ret = rtl83xx_vlan_get(i, &membermsk, &untagmsk, &pri, &fid);
		if ((ret == 0) && (membermsk != 0))
			printk("vid %d Mbrmsk 0x%x Untagmsk 0x%x priority %u fid %u\n", i, membermsk, untagmsk, pri, fid);           
	}
#endif
	printk("\n%s\n", "pvid:");
	for(i=0;i<8;i++)
	{
		
		 logic_port = rtk_switch_port_P2L_get(i);
		if(logic_port == UNDEFINE_PORT){
			continue;
		}
		
		pvid = priority = 0;        
		if (rtk_vlan_portPvid_get(logic_port, &pvid, &priority) == 0)
			printk("port %d pvid %u pri %u\n", i, pvid, priority);           
	}

	printk("\n%s\n", "efid:");
    for(i=0;i<8;i++)
    {
        efid = 0;
		logic_port = rtk_switch_port_P2L_get(i);
		if(logic_port == UNDEFINE_PORT){
			continue;
		}
        if (rtk_port_efid_get(logic_port, &efid)==0)
            printk("port %d efid %u\n", i, efid);           
    }

	printk("\n%s\n", "port isolation:");
    for(i=0;i<8;i++)
    {
    	logic_port = rtk_switch_port_P2L_get(i);
    	if(logic_port == UNDEFINE_PORT){
			continue;
		}
        memset(&portmsk, 0x00, sizeof(portmsk));         
        if (rtk_port_isolation_get(logic_port, &portmsk)==0){	
			phy_port_msk = 0;
		    rtk_switch_portmask_L2P_get(&portmsk, &phy_port_msk);
            printk("port %d portmask 0x%x \n", i, phy_port_msk);
        }
    }

	
	printk("\n%s\n", "l2:");

#if 1
	/* Get all unicast entry at whole system*/ 
	while (1) 
	{ 
		if((ret=rtk_l2_addr_next_get(READMETHOD_NEXT_L2UC, UTP_PORT0, &address, &l2_api_data))!=RT_ERR_OK) 
		{ 
			break; 
		}

		phy_port_msk = rtk_switch_port_L2P_get(l2_api_data.port);
		
		printk("[%4d] %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X %-3d %-3d %-4d %-3d %-4d %-4s %-7s %-7s %-6s %-3d %-2s %-2s\n",
			address,
			l2_api_data.mac.octet[0],l2_api_data.mac.octet[1],l2_api_data.mac.octet[2],\
			l2_api_data.mac.octet[3],l2_api_data.mac.octet[4],l2_api_data.mac.octet[5],\
			phy_port_msk, l2_api_data.ivl, l2_api_data.cvid,\
			l2_api_data.fid, l2_api_data.efid,
			(l2_api_data.auth ? "Auth" : "x"),\
			(l2_api_data.sa_block? "Block" : "x"),\
			(l2_api_data.da_block? "Block" : "x"),\
			(l2_api_data.is_static? "Static" : "Auto"),\
			l2_api_data.priority,\
			(l2_api_data.sa_pri_en? "En" : "x"),
			(l2_api_data.fwd_pri_en? "En" : "x"));
		address++; 
	} 
#else
	/*Get All Lookup Table and Print the valid entry*/ 
	for (i=1;i<=2112;i++)
	{
		memset(&l2_entry,0,sizeof(rtk_l2_addr_table_t));
		l2_entry.index = i;
		ret = rtk_l2_entry_get(&l2_entry);
		if (ret==RT_ERR_OK)
		{
			if(l2_entry.is_ipmul)
			{				
				printk("\r\nIndex SourceIP DestinationIP MemberPort State\n");           
				printk("%4d ", l2_entry.index);
				printk("%0x ",(l2_entry.sip));
				printk("%0x ",(l2_entry.dip));
				printk("%-8x ",l2_entry.portmask.bits[0]);
				printk("%s \n",(l2_entry.is_static? "Static" : "Auto"));
			}
			else if(l2_entry.mac.octet[0]&0x01)
			{
				printk("%4d %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X %-8x %-4d %-4s %-5s %s %d\n",
				l2_entry.index,
				l2_entry.mac.octet[0],l2_entry.mac.octet[1],l2_entry.mac.octet[2],
				l2_entry.mac.octet[3],l2_entry.mac.octet[4],l2_entry.mac.octet[5],
				l2_entry.portmask.bits[0], l2_entry.fid, (l2_entry.auth ? "Auth" : "x"),
				(l2_entry.sa_block? "Block" : "x"), (l2_entry.is_static? "Static" : "Auto"),
				l2_entry.age);
			}
			else if((l2_entry.age!=0)||(l2_entry.is_static==1))
			{
				printk("%4d %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X %-8x %-4d %-4s %-5s %s %d\n",
				l2_entry.index,l2_entry.mac.octet[0],l2_entry.mac.octet[1],l2_entry.mac.octet[2],
				l2_entry.mac.octet[3],l2_entry.mac.octet[4],l2_entry.mac.octet[5],
				l2_entry.portmask.bits[0], l2_entry.fid, (l2_entry.auth ? "Auth" : "x"),
				(l2_entry.sa_block? "Block" : "x"), (l2_entry.is_static? "Static" : "Auto"),
				l2_entry.age);
			}
		}
	}
#endif
	//local_irq_restore(flags);
	return 0;
}

extern rtk_api_ret_t rtk_83xx_setFlowControl(rtk_enable_t qosEnable);
rtk_api_ret_t rtk_get_83xx_cpu_flow_control(void)
{
	rtk_api_ret_t ret;
	rtk_port_mac_ability_t mac_cfg;
    rtk_mode_ext_t mode;
#ifdef RTK_EXT_RGMII
	/* keep insistent with rtl83xx_init_cpu_port() */
	rtk_port_t cpu_port = SWITCH_CPU_PORT;
#else
	rtk_port_t cpu_port = EXT_PORT0;
#endif

	if((ret = rtk_port_macForceLinkExt_get(cpu_port,&mode,&mac_cfg)) != 0){
		printk("%s, 0x%x\n", "rtk_port_macForceLinkExt_get error", ret);
		return RT_ERR_FAILED;
	}

	printk("rtl83xx cpu flow control\n");
	printk("\ttx pause:%d\n",  mac_cfg.txpause);
	printk("\trx pause:%d\n",  mac_cfg.rxpause);
			
	return RT_ERR_OK;
}

static int phyreg_show_help(struct seq_file *m, void *v)
{
	return PROC_READ_RETURN_VALUE;
}

static int phyreg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, phyreg_show_help, (void *) file);
}

static ssize_t phyreg_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char 		tmpbuf[64];
	unsigned int	phyId, regId, regData;
	char		*strptr, *cmd_addr;
	char		*tokptr;
	int 	ret=0;

	if(count > 64)
		goto errout;


	if (userbuf && !copy_from_user(tmpbuf, userbuf, count)) {
		if(count == 64)
			tmpbuf[count-1] = '\0';
		else
			tmpbuf[count] = '\0';

		strptr=tmpbuf;
		cmd_addr = strsep(&strptr," ");
		
		if (cmd_addr==NULL)
			goto errout;

		if (!memcmp(cmd_addr, "8367read", 8) || !memcmp(cmd_addr, "83xxread", 8)) {
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regId=simple_strtol(tokptr, NULL, 16);
			ret = rtl83xx_getAsicReg(regId, &regData);
			if(ret==0)
				rtlglue_printf("rtl83xx_getAsicReg: reg= %x, data= %x\n", regId, regData);
			else
				rtlglue_printf("get fail %d\n", ret);
		} else if (!memcmp(cmd_addr, "8367write", 9) || !memcmp(cmd_addr, "83xxwrite", 9)) {
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regId=simple_strtol(tokptr, NULL, 16);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regData=simple_strtol(tokptr, NULL, 16);

			ret = rtl83xx_setAsicReg(regId, regData);
			if(ret==0)
				rtlglue_printf("rtl83xx_setAsicReg: reg= %x, data= %x\n", regId, regData);
			else
				rtlglue_printf("set fail %d\n", ret);
		} else if (!memcmp(cmd_addr, "8367phyr", 8) || !memcmp(cmd_addr, "83xxphyr", 8)) {
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			phyId=simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regId=simple_strtol(tokptr, NULL, 0);

			ret=rtl83xx_getAsicPHYReg(phyId, regId, &regData);
			if(ret==SUCCESS)
				rtlglue_printf("read 83XX phyId(%d), regId(%d), regData:0x%x\n", phyId, regId, regData);
			else
				rtlglue_printf("error input!\n");
		} else if (!memcmp(cmd_addr, "8367phyw", 8) || !memcmp(cmd_addr, "83xxphyw", 8)) {
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			phyId=simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regId=simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regData=simple_strtol(tokptr, NULL, 16);

			ret=rtl83xx_setAsicPHYReg(phyId, regId, regData);
			if(ret==SUCCESS)
				rtlglue_printf("Write 83XX phyId(%d), regId(%d), regData:0x%x\n", phyId, regId, regData);
			else
				rtlglue_printf("error input!\n");
		} else if (!memcmp(cmd_addr, "8367ocpr", 8) || !memcmp(cmd_addr, "83xxocpr", 8)) {
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			phyId=simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regId=simple_strtol(tokptr, NULL, 0);

			ret=rtl83xx_getAsicPHYOCPReg(phyId, regId, &regData);
			if(ret==SUCCESS)
				rtlglue_printf("read 83XX phyId(%d), ocpAddr(0x%x), regData:0x%x\n", phyId, regId, regData);
			else
				rtlglue_printf("error input!\n");
		} else if (!memcmp(cmd_addr, "8367ocpw", 8) || !memcmp(cmd_addr, "83xxocpw", 8)) {
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			phyId=simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regId=simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			regData=simple_strtol(tokptr, NULL, 16);

			ret=rtl83xx_setAsicPHYOCPReg(phyId, regId, regData);
			if(ret==SUCCESS)
				rtlglue_printf("Write 83XX phyId(%d), ocpAddr(0x%x), regData:0x%x\n", phyId, regId, regData);
			else
				rtlglue_printf("error input!\n");
		} else if (!memcmp(cmd_addr, "8367test", 8) || !memcmp(cmd_addr, "83xxtest", 8)) {
			extern int rtk_port_phyTestMode_set(unsigned int port, int mode);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			phyId=simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			if (tokptr[0] == 'm') {
				extern int rtk_port_phyMdx_set(unsigned int port, int mode);
				if(strncmp(tokptr, "mdi_auto",8) == 0)
					regId = 0; // PHY_AUTO_CROSSOVER_MODE
				else if(strncmp(tokptr, "mdix",4) == 0)
					regId = 2; // PHY_FORCE_MDIX_MODE
				else if(strncmp(tokptr, "mdi",3) == 0)
					regId = 1; // PHY_FORCE_MDI_MODE
				else
					regId = 3;
				ret=rtk_port_phyMdx_set(phyId, regId);
				if(ret==SUCCESS)
					rtlglue_printf("set 83XX phyId(%d) to mdi mode: %d\n", phyId, regId);
				else
					rtlglue_printf("rtk_port_phyMdx_set return error\n");
			} else {
				regId=simple_strtol(tokptr, NULL, 0);

				ret=rtk_port_phyTestMode_set(phyId, regId);
				if(ret==SUCCESS)
					rtlglue_printf("set 83XX phyId(%d) to mode: %d\n", phyId, regId);
				else
					rtlglue_printf("rtk_port_phyTestMode_set return error\n");
			}		
		} else if (!memcmp(cmd_addr, "8367snr", 7) || !memcmp(cmd_addr, "83xxsnr", 7)) {
			/* Show 83xx link up ports' SNR value */
			rtl_get_83xx_snr();
		} else if (!memcmp(cmd_addr, "8367l2", 6) || !memcmp(cmd_addr, "83xxl2", 6)) {
			rtl_8367r_vlan_read();
		} else if (!memcmp(cmd_addr, "83xxFC", 6)) {
			cmd_addr = strsep(&strptr," ");
			if (cmd_addr==NULL)
				goto errout;

			if (!memcmp(cmd_addr, "show", 4)){
				rtk_get_83xx_cpu_flow_control();
			}
			else if (!memcmp(cmd_addr, "0", 1)){
				// disable cpu flow control <-> qos enable
				rtk_83xx_setFlowControl(1);
			}
			else if (!memcmp(cmd_addr, "1", 1)){
				// enable cpu flow control <-> qos disable
				rtk_83xx_setFlowControl(0);
			}
			else{
				goto errout;
			}
		}
	} else
	{
errout:
		rtlglue_printf("error input!\n");
	}

	return count;
}

static const struct file_operations fops_phyreg = {
	.owner		= THIS_MODULE,
	.open		= phyreg_proc_open,
	.read		= seq_read,
	.write		= phyreg_proc_write,
	.release	= single_release,
};

/***************************************************************************
 * asiccounter
 ***************************************************************************/

static int asiccounter_show_help(struct seq_file *m, void *v)
{
	return PROC_READ_RETURN_VALUE;
}

static int asiccounter_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, asiccounter_show_help, (void *) file);
}

void display_8367r_port_stat(unsigned int port, rtk_stat_port_cntr_t *pPort_cntrs)
{
	if ( port > 4 )
		rtlglue_printf("\n<CPU port>\n");
	else
		rtlglue_printf("\n<Port: %d>\n", port);

	rtlglue_printf("Rx counters\n");

	rtlglue_printf("   ifInOctets  %llu\n", pPort_cntrs->ifInOctets);
	rtlglue_printf("   etherStatsOctets  %llu\n", pPort_cntrs->etherStatsOctets);
	rtlglue_printf("   ifInUcastPkts  %u\n", pPort_cntrs->ifInUcastPkts);
	rtlglue_printf("   etherStatsMcastPkts  %u\n", pPort_cntrs->etherStatsMcastPkts);
	rtlglue_printf("   etherStatsBcastPkts  %u\n", pPort_cntrs->etherStatsBcastPkts);
	
	rtlglue_printf("   StatsFCSErrors  %u\n", pPort_cntrs->dot3StatsFCSErrors);
	rtlglue_printf("   StatsSymbolErrors  %u\n", pPort_cntrs->dot3StatsSymbolErrors);
	rtlglue_printf("   InPauseFrames  %u\n", pPort_cntrs->dot3InPauseFrames);
	rtlglue_printf("   ControlInUnknownOpcodes  %u\n", pPort_cntrs->dot3ControlInUnknownOpcodes);
	rtlglue_printf("   etherStatsFragments  %u\n", pPort_cntrs->etherStatsFragments);
	rtlglue_printf("   etherStatsJabbers  %u\n", pPort_cntrs->etherStatsJabbers);
	rtlglue_printf("   etherStatsDropEvents  %u\n", pPort_cntrs->etherStatsDropEvents);

	rtlglue_printf("   etherStatsUndersizePkts  %u\n", pPort_cntrs->etherStatsUndersizePkts);
	rtlglue_printf("   etherStatsOversizePkts  %u\n", pPort_cntrs->etherStatsOversizePkts);

	rtlglue_printf("   dot1dTpPortInDiscards  %u\n", pPort_cntrs->dot1dTpPortInDiscards);
	rtlglue_printf("   inOampduPkts  %u\n", pPort_cntrs->inOampduPkts);
	
	rtlglue_printf("   Len= 64: %u pkts, 65 - 127: %u pkts, 128 - 255: %u pkts\n",
		pPort_cntrs->etherStatsPkts64Octets,
		pPort_cntrs->etherStatsPkts65to127Octets,
		pPort_cntrs->etherStatsPkts128to255Octets);
	rtlglue_printf("       256 - 511: %u pkts, 512 - 1023: %u pkts, 1024 - 1518: %u pkts\n",
		pPort_cntrs->etherStatsPkts256to511Octets,
		pPort_cntrs->etherStatsPkts512to1023Octets,
		pPort_cntrs->etherStatsPkts1024toMaxOctets);

	rtlglue_printf("\nOutput counters\n");

	rtlglue_printf("   ifOutOctets  %llu\n", pPort_cntrs->ifOutOctets);
	rtlglue_printf("   ifOutUcastPkts  %u\n", pPort_cntrs->ifOutUcastPkts);
	rtlglue_printf("   ifOutMulticastPkts  %u\n", pPort_cntrs->ifOutMulticastPkts);
	rtlglue_printf("   ifOutBrocastPkts  %u\n", pPort_cntrs->ifOutBrocastPkts);
	rtlglue_printf("   ifOutDiscards  %u\n", pPort_cntrs->ifOutDiscards);

	rtlglue_printf("   StatsSingleCollisionFrames  %u\n", pPort_cntrs->dot3StatsSingleCollisionFrames);
	rtlglue_printf("   StatsMultipleCollisionFrames  %u\n", pPort_cntrs->dot3StatsMultipleCollisionFrames);
	rtlglue_printf("   StatsDeferredTransmissions  %u\n", pPort_cntrs->dot3StatsDeferredTransmissions);
	rtlglue_printf("   StatsLateCollisions  %u\n", pPort_cntrs->dot3StatsLateCollisions);
	rtlglue_printf("   etherStatsCollisions  %u\n", pPort_cntrs->etherStatsCollisions);
	rtlglue_printf("   StatsExcessiveCollisions  %u\n", pPort_cntrs->dot3StatsExcessiveCollisions);
	rtlglue_printf("   OutPauseFrames  %u\n", pPort_cntrs->dot3OutPauseFrames);
	rtlglue_printf("   dot1dBasePortDelayExceededDiscards  %u\n", pPort_cntrs->dot1dBasePortDelayExceededDiscards);

	rtlglue_printf("   outOampduPkts  %u\n", pPort_cntrs->outOampduPkts);
	rtlglue_printf("   pktgenPkts  %u\n", pPort_cntrs->pktgenPkts);

}

void display_8367r_port_stat_simple(unsigned int port, rtk_stat_port_cntr_t *pPort_cntrs)
{
	if ( port > 4 )
		rtlglue_printf("\n<CPU port>\n");
	else
		rtlglue_printf("\n<Port: %d>\n", port);

	rtlglue_printf("Rx counters\n");
	rtlglue_printf("   ifInOctets  %llu\n", pPort_cntrs->ifInOctets);
	rtlglue_printf("   ifInUcastPkts  %u\n", pPort_cntrs->ifInUcastPkts);
	rtlglue_printf("   etherStatsMcastPkts  %u\n", pPort_cntrs->etherStatsMcastPkts);
	rtlglue_printf("   etherStatsBcastPkts  %u\n", pPort_cntrs->etherStatsBcastPkts);	
	rtlglue_printf("   StatsFCSErrors  %u\n", pPort_cntrs->dot3StatsFCSErrors);
	rtlglue_printf("   StatsSymbolErrors  %u\n", pPort_cntrs->dot3StatsSymbolErrors);
	rtlglue_printf("   InPauseFrames  %u\n", pPort_cntrs->dot3InPauseFrames);
	rtlglue_printf("   ControlInUnknownOpcodes  %u\n", pPort_cntrs->dot3ControlInUnknownOpcodes);
	rtlglue_printf("   etherStatsFragments  %u\n", pPort_cntrs->etherStatsFragments);
	rtlglue_printf("   etherStatsJabbers  %u\n", pPort_cntrs->etherStatsJabbers);
	rtlglue_printf("   etherStatsDropEvents  %u\n", pPort_cntrs->etherStatsDropEvents);
	rtlglue_printf("   etherStatsUndersizePkts  %u\n", pPort_cntrs->etherStatsUndersizePkts);
	rtlglue_printf("   etherStatsOversizePkts  %u\n", pPort_cntrs->etherStatsOversizePkts);
	rtlglue_printf("   dot1dTpPortInDiscards  %u\n", pPort_cntrs->dot1dTpPortInDiscards);
	rtlglue_printf("   Len= [64B]: %u pkts, [65~127B]: %u pkts, [128~255B]: %u pkts\n",
		pPort_cntrs->etherStatsPkts64Octets,
		pPort_cntrs->etherStatsPkts65to127Octets,
		pPort_cntrs->etherStatsPkts128to255Octets);
	rtlglue_printf("       [256~511B]: %u pkts, [512~1023B]: %u pkts, [1024~1518B]: %u pkts\n",
		pPort_cntrs->etherStatsPkts256to511Octets,
		pPort_cntrs->etherStatsPkts512to1023Octets,
		pPort_cntrs->etherStatsPkts1024toMaxOctets);
	
	rtlglue_printf("\nOutput counters\n");
	rtlglue_printf("   ifOutOctets  %llu\n", pPort_cntrs->ifOutOctets);
	rtlglue_printf("   ifOutUcastPkts  %u\n", pPort_cntrs->ifOutUcastPkts);
	rtlglue_printf("   ifOutMulticastPkts  %u\n", pPort_cntrs->ifOutMulticastPkts);
	rtlglue_printf("   ifOutBrocastPkts  %u\n", pPort_cntrs->ifOutBrocastPkts);
	rtlglue_printf("   ifOutDiscards  %u\n", pPort_cntrs->ifOutDiscards);
	rtlglue_printf("   StatsSingleCollisionFrames  %u\n", pPort_cntrs->dot3StatsSingleCollisionFrames);
	rtlglue_printf("   StatsMultipleCollisionFrames  %u\n", pPort_cntrs->dot3StatsMultipleCollisionFrames);
	rtlglue_printf("   StatsDeferredTransmissions  %u\n", pPort_cntrs->dot3StatsDeferredTransmissions);
	rtlglue_printf("   StatsLateCollisions  %u\n", pPort_cntrs->dot3StatsLateCollisions);
	rtlglue_printf("   etherStatsCollisions  %u\n", pPort_cntrs->etherStatsCollisions);
	rtlglue_printf("   StatsExcessiveCollisions  %u\n", pPort_cntrs->dot3StatsExcessiveCollisions);
	rtlglue_printf("   OutPauseFrames  %u\n", pPort_cntrs->dot3OutPauseFrames);
	rtlglue_printf("   dot1dBasePortDelayExceededDiscards  %u\n", pPort_cntrs->dot1dBasePortDelayExceededDiscards);
}

static ssize_t asiccounter_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{

	char 		tmpbuf[512];
	char		*strptr;
	char		*cmdptr;
	unsigned int	portNum=0xFFFFFFFF;
	rtk_stat_port_cntr_t  port_cntrs;

	if(count > 512)
		goto errout;

	if (userbuf && !copy_from_user(tmpbuf, userbuf, count))	{
		if(count == 512)
			tmpbuf[count-1] = '\0';
		else
			tmpbuf[count] = '\0';

		strptr=tmpbuf;

		if(strlen(strptr)==0)
			goto errout;

		cmdptr = strsep(&strptr," ");

		if (cmdptr==NULL)
			goto errout;

		/*parse command*/
		if(strncmp(cmdptr, "clear",5) == 0) {
			rtk_stat_global_reset();
		} else if(strncmp(cmdptr, "dump",4) == 0) {
			cmdptr = strsep(&strptr," ");

			if (cmdptr==NULL)
				goto errout;

			if(strncmp(cmdptr, "8367",4) == 0 || strncmp(cmdptr, "83xx",4) == 0) {
				for (portNum=0; portNum<19; portNum++) {

					if ((portNum > 4) && (portNum != CPU_PORT_ID))
						continue;

					memset(&port_cntrs, 0, sizeof(rtk_stat_port_cntr_t));
					rtk_83xx_stat_port_getAll(portNum, &port_cntrs);
					display_8367r_port_stat(portNum, &port_cntrs);
				}
				return count;
			}
			else if(strncmp(cmdptr, "83mib",5) == 0) {

				if ((cmdptr[5] >= '0') && (cmdptr[5] <= '7')) {
					unsigned int offset = 100;
					rtk_stat_counter_t cntr;
					portNum = cmdptr[5] - '0';
					if (portNum >= 5)
						portNum += 11;
					if ((cmdptr[6] >= '0') && (cmdptr[6] <= '6')) {
						offset = (cmdptr[6] - '0') * 10 + (cmdptr[7] - '0');
					}

					if (offset <= 61) {
						rtk_stat_port_get(portNum, offset, &cntr);
						rtlglue_printf("\nGet 83xx mib: port: %d, mib offset: %d, mib value: %llu\n", 
							portNum, offset, cntr);
					}
					else {
						memset(&port_cntrs, 0, sizeof(rtk_stat_port_cntr_t));
						rtl_stat_port_getserial(portNum, &port_cntrs);
						display_8367r_port_stat_simple(portNum, &port_cntrs);
					}
				}
				else
				for (portNum=0; portNum<19; portNum++) {

					if ((portNum > 4) && (portNum != CPU_PORT_ID))
						continue;

					memset(&port_cntrs, 0, sizeof(rtk_stat_port_cntr_t));
					rtl_stat_port_getserial(portNum, &port_cntrs);
					display_8367r_port_stat_simple(portNum, &port_cntrs);
				}
				return count;
			} else {
				goto errout;
			}
		} else {
			goto errout;
		}
	} else {
errout:
		rtlglue_printf("error input\n");
	}
	return count;
}


static const struct file_operations fops_asiccounter = {
	.owner		= THIS_MODULE,
	.open		= asiccounter_proc_open,
	.read		= seq_read,
	.write		= asiccounter_proc_write,
	.release	= single_release,
};

#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
/***************************************************************************
 * qos_83xx
 ***************************************************************************/
static int qos_83xx_show_help(struct seq_file *s, void *v)
{
	int port, logic_port, queueNum, i, queue, ret;	
	rtk_qos_queue_weights_t qWeights;
	rtk_meter_id_t meterId;
	rtk_rate_t qRate, pRate;
	rtk_enable_t qIfg_include, pIfg_include, qEnable;
	rtk_uint32 Bucket_size;
    rtk_port_mac_ability_t mac_cfg;
    rtk_mode_ext_t mode ;
	rtk_rate_t inRate;
	rtk_enable_t inIfg_include, inFc_enable;
	rtk_meter_type_t type;
#ifdef RTK_EXT_RGMII
	/* keep insistent with rtl83xx_init_cpu_port() */
	rtk_port_t cpu_port = SWITCH_CPU_PORT;
#else
	rtk_port_t cpu_port = EXT_PORT0;
#endif

	seq_printf(s, "rtl83xx qos is %s\n", (g83xxQosEnabled>0 ? "enabled" : "disabled"));
	
	if((ret = rtk_port_macForceLinkExt_get(cpu_port,&mode,&mac_cfg)) != 0){
		printk("%s, 0x%x\n", "rtk_port_macForceLinkExt_get error", ret);
		goto done;
	}
	 
	seq_printf(s, "%s\n", "rtl83xx cpu flow control");
	seq_printf(s, "\ttx pause:%d\n", mac_cfg.txpause);
	seq_printf(s, "\trx pause:%d\n", mac_cfg.rxpause);

	seq_printf(s, "\n%s\n", "rtl83xx qos related parameters");
	for(port = 0; port < 8; port++){
		logic_port = rtk_switch_port_P2L_get(port);
		if(logic_port == UNDEFINE_PORT){
			continue;
		}
		
		if((ret = rtk_qos_queueNum_get(logic_port,&queueNum)) != 0){
			printk("%s, 0x%x\n", "rtk_qos_queueNum_get error", ret);
			goto done;
		}

		if((ret = rtk_rate_egrBandwidthCtrlRate_get(logic_port, &pRate, &pIfg_include)) != 0){
			printk("%s, 0x%x\n", "rtk_rate_egrBandwidthCtrlRate_get error", ret);
			goto done;
		}

		if((ret = rtk_qos_schedulingQueue_get(logic_port, &qWeights)) != 0){
			printk("%s, 0x%x\n", "rtk_qos_schedulingQueue_get error", ret);
			goto done;
		}
		
		if((ret = rtk_rate_egrQueueBwCtrlEnable_get(logic_port, RTK_WHOLE_SYSTEM, &qEnable)) != 0){
			printk("%s, 0x%x\n", "rtk_rate_egrQueueBwCtrlEnable_get error", ret);
			goto done;
		}

		seq_printf(s,"<%d> queueNum:%d portRate:%d Ifg include:%d queueBwCtrl:%s\n", 
			logic_port, queueNum, pRate, pIfg_include, (qEnable==ENABLED)?"enabled":"disable");

	
		if((ret = rtk_rate_igrBandwidthCtrlRate_get(logic_port, &inRate, &inIfg_include, &inFc_enable)) != 0){
			printk("%s, 0x%x\n", "rtk_rate_igrBandwidthCtrlRate_get error", ret);
			goto done;
		}
		seq_printf(s,"\tingressBw:%d Ifg include:%d flowControl:%d\n", inRate, inIfg_include, inFc_enable);

		seq_printf(s,"\tQueue Parameters:\n ");
		for(queue=0; queue<RTK_MAX_NUM_OF_QUEUE; queue++)
		{
			if((ret = rtk_rate_egrQueueBwCtrlRate_get(logic_port, queue, &meterId)) != 0){
				printk("%s, 0x%x\n", "rtk_rate_egrQueueBwCtrlRate_get error", ret);
				goto done;
			}
			 
			seq_printf(s,"\t[%d] type:%s, weight:%d, MeterId:%d\n", 
				queue, (qWeights.weights[queue]==0)?"SP":"WFQ", qWeights.weights[queue], meterId);
		}

		seq_printf(s,"\tMeter Parameters:\n ");
		if(port < 4)
		{
			for(i = port*8; i <= port*8+7; i++)
			{			
				if((ret = rtk_rate_shareMeter_get(i, &type, &qRate, &qIfg_include)) != 0){
					printk("%s, 0x%x\n", "rtk_rate_shareMeter_get error", ret);
					goto done;
				}
				
				if((ret = rtk_rate_shareMeterBucket_get(i,&Bucket_size)) != 0){
					printk("%s, 0x%x\n", "rtk_rate_shareMeterBucket_get error", ret);
					goto done;
				}
				
				if(i==port*8)
					seq_printf(s,"\t");
				
				if(i==port*8+4)
					seq_printf(s,"\n\t");
				
				seq_printf(s,"[%d]%d%s,%d,%d ", i, qRate, (type == METER_TYPE_KBPS?"kbps":"pps"), qIfg_include,Bucket_size);
			}
			seq_printf(s,"\n");
		}
		else
		{
			for(i = (port-4)*8; i <= (port-4)*8+7; i++)
			{	
				if(i==(port-4)*8)
					seq_printf(s,"\t");
				
				if(i==(port-4)*8+4)
					seq_printf(s,"\n\t");
				
				if((ret = rtk_rate_shareMeter_get(i, &type, &qRate, &qIfg_include)) != 0){
					printk("%s, 0x%x\n", "rtk_rate_shareMeter_get error", ret);
					goto done;
				}
				
				if((ret = rtk_rate_shareMeterBucket_get(i,&Bucket_size)) != 0){
					printk("%s, 0x%x\n", "rtk_rate_shareMeterBucket_get error", ret);
					goto done;
				}
				seq_printf(s,"[%d]%d%s,%d,%d ", i, qRate, (type == METER_TYPE_KBPS?"kbps":"pps"), qIfg_include, Bucket_size);
			}
			seq_printf(s,"\n");
		}
	}
done:
	return 0;
}

static int qos_83xx_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, qos_83xx_show_help, (void *) file);
}

extern int rtl83xx_default_vlan_set(int mode);
static ssize_t qos_83xx_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char tmpbuf[128];
	char *strptr, *cmdptr;
	int ret, port, logic_port, page, qid, prio;
	int queue_num, rate;
	int i, j;
	unsigned int queue_page[RTK_MAX_NUM_OF_QUEUE];
	rtk_qos_queue_weights_t qweights;

	if(count > 127)
		goto fail;

	if(userbuf && !copy_from_user(tmpbuf, userbuf, count)){
		tmpbuf[count] = '\0';
		
		strptr=tmpbuf;
		if(strlen(strptr)==0)
			goto fail;

		cmdptr = strsep(&strptr," ");
		if (cmdptr==NULL)
			goto fail;

		/*parse command*/
		if(strncmp(cmdptr, "1", 1) == 0){
			if((ret = rtk_83xx_qos_init(8)) != 0){
				rtlglue_printf("[%s:%d] rtk_83xx_qos_init failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
				goto fail;
			}
			
			if((ret = rtk_83xx_qos_enable(1)) != 0){
				rtlglue_printf("[%s:%d] rtk_83xx_qos_enable(1) failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
				goto fail;
			}
			
			#if !defined(RTK_DISABLE_LANWAN_ISOLATION)
			INIT(rtl83xx_default_vlan_set(GATEWAY_MODE));
			#endif

			goto done;
		}
		else if(strncmp(cmdptr, "0", 1) == 0){
			if((ret = rtk_83xx_qos_init(8)) != 0){
				rtlglue_printf("[%s:%d] rtk_83xx_qos_init failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
				goto fail;
			}
			
			if((ret = rtk_83xx_qos_enable(0)) != 0){
				rtlglue_printf("[%s:%d] rtk_83xx_qos_enable(0) failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
				goto fail;
			}
			
			#if !defined(RTK_DISABLE_LANWAN_ISOLATION)
			INIT(rtk_vlan_init());
			#endif
			
			goto done;
		}
		else if(strncmp(cmdptr, "portpri", 7) == 0){
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL){
				goto fail;
			}

			if(strncmp(cmdptr, "show", 4) == 0){
				rtlglue_printf("port remap:\n");
				for(port = 0; port <= RTK_PHY_ID_MAX; port++){
					logic_port = rtk_switch_port_P2L_get(port);
					if(logic_port == UNDEFINE_PORT){
						continue;
					}
				
					if((ret = rtk_qos_portPri_get(logic_port,&prio)) != 0){
						rtlglue_printf("%s, 0x%x\n", "rtk_qos_portPri_get error", ret);
						goto done;
					}
					rtlglue_printf("\tport%d: %d\n", logic_port, prio);
				}
				goto done;
			}
			else if(strncmp(cmdptr, "port", 4) == 0){
				if(((i = sscanf(strptr, "%d pri %d", &port, &prio)) != 2) || (port > RTK_PHY_ID_MAX)){
					goto fail;
				}

				logic_port = rtk_switch_port_P2L_get(port);
				if(logic_port == UNDEFINE_PORT){
					goto fail;
				}
			
				if((ret = rtk_qos_portPri_set(logic_port, prio)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_portPri_set error", ret);
				}
				goto done;
			}
		}
		else if(strncmp(cmdptr, "port", 4) == 0){
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL)
				goto fail;
			port = simple_strtol(cmdptr, NULL, 10);
			logic_port = rtk_switch_port_P2L_get(port);
			if(logic_port == UNDEFINE_PORT){
				goto fail;
			}

			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL)
				goto fail;

			if(strncmp(cmdptr, "rate", 4) == 0){
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL)
					goto fail;
				rate = simple_strtol(cmdptr, NULL, 10);

				if((ret = rtk_rate_egrBandwidthCtrlRate_set(logic_port, rate, ENABLED)) != 0){
					rtlglue_printf("[%s:%d] rtk_rate_egrBandwidthCtrlRate_set failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
					goto fail;
				}

				if(rate == RTL8367C_QOS_RATE_INPUT_MAX){
					if((ret = rtk_rate_egrQueueBwCtrlEnable_set(logic_port, 0xFF, 0)) != 0){
						rtlglue_printf("[%s:%d] rtk_rate_egrQueueBwCtrlEnable_set failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
						goto fail;
					}
				}
				
				goto done;
			}
			else if(strncmp(cmdptr, "queue", 5) == 0){
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL)
					goto fail;
				qid = simple_strtol(cmdptr, NULL, 10);

				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL)
					goto fail;
				if(strncmp(cmdptr, "meter", 5) == 0){
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL)
						goto fail;
					page = simple_strtol(cmdptr, NULL, 10);

					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL)
						goto fail;
					if(strncmp(cmdptr, "rate", 4) == 0){
						cmdptr = strsep(&strptr," ");
						if (cmdptr==NULL)
							goto fail;
						rate = simple_strtol(cmdptr, NULL, 10);

						if((ret = rtk_rate_egrQueueBwCtrlRate_set(logic_port, qid, page)) != 0){
							rtlglue_printf("[%s:%d] rtk_rate_egrQueueBwCtrlRate_set failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
							goto fail;
						}
						
						if((ret = rtk_rate_egrQueueBwCtrlEnable_set(logic_port, 0xFF, 1)) != 0){
							rtlglue_printf("[%s:%d] rtk_rate_egrQueueBwCtrlEnable_set failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
							goto fail;
						}

						if((ret = rtk_rate_shareMeter_set(page, METER_TYPE_KBPS, rate, 1)) != 0){
							rtlglue_printf("[%s:%d] rtk_rate_shareMeter_set failure, 0x%x!\n", __FUNCTION__, __LINE__, ret);
							goto fail;
						}
						
						goto done;
					}
				}
			}
			else if(strncmp(cmdptr, "qnum", 4) == 0){
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL){
					goto fail;
				}

				queue_num = simple_strtol(cmdptr, NULL, 10);
				if((ret = rtk_qos_queueNum_set(logic_port, queue_num)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_queueNum_set error", ret);
				}
				goto done;
			}
			else if(strncmp(cmdptr, "qtype", 5) == 0){
				memset(&qweights, 0, sizeof(qweights));
				if(RTK_MAX_NUM_OF_QUEUE == 8){
					if((i = sscanf(strptr, "q0 %d q1 %d q2 %d q3 %d q4 %d q5 %d q6 %d q7 %d", 
					&(qweights.weights[0]), &(qweights.weights[1]), &(qweights.weights[2]), 
					&(qweights.weights[3]), &(qweights.weights[4]), &(qweights.weights[5]), 
					&(qweights.weights[6]), &(qweights.weights[7]))) != 8){
						goto fail;
					}

					if((ret =rtk_qos_schedulingQueue_set(logic_port, &qweights)) != 0){
						rtlglue_printf("%s, 0x%x\n", "rtk_qos_queueNum_set error", ret);
					}
					goto done;
				}
			}
		}
		else if(strncmp(cmdptr, "current", 7) == 0){				
			rtlglue_printf( "Current Page for Egress Port and Queues\n");
			rtlglue_printf( "PortNo.\tPort\tQ0\tQ1\tQ2\tQ3\tQ4\tQ5\tQ6\tQ7\n");
		
			for(port = 0; port < RTK_MAX_NUM_OF_PORT; port++){
				/*Egress Port page number*/
				rtl8367c_setAsicReg(RTL8367C_REG_FLOWCTRL_DEBUG_CTRL0, port);
				rtl8367c_getAsicReg(RTL8367C_REG_FLOWCTRL_PORT_PAGE_COUNT, &page);
				
				for(qid = 0; qid <= 7; qid++){						 
					rtl8367c_getAsicReg(RTL8367C_REG_FLOWCTRL_QUEUE0_PAGE_COUNT+qid, &queue_page[qid]);
				}
		
				rtlglue_printf("%2d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", port,page,
				queue_page[0],queue_page[1],queue_page[2],queue_page[3],queue_page[4],queue_page[5],queue_page[6],queue_page[7]);
			}

			goto done;
		}
		else if(strncmp(cmdptr, "redot1p", 7) == 0){
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL)
				goto fail;

			if(strncmp(cmdptr, "show", 4) == 0){
			rtk_enable_t qEnable;
			
			rtlglue_printf("\nrtl83xx dot1p remark:\n");
				for(port = 0; port <= RTK_PORT_ID_MAX; port++){
				logic_port = rtk_switch_port_P2L_get(port);
				if(logic_port == UNDEFINE_PORT){
					continue;
				}

				if((ret = rtk_qos_1pRemarkEnable_get(logic_port, &qEnable)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_1pRemarkEnable_get error", ret);
							goto done;
				}
				rtlglue_printf("<%d> dot1p remark: %s\n", logic_port, (qEnable > 0 ? "enable" : "disable"));

				for(i = 0; i <= RTK_PRIMAX; i++){
					if((ret = rtk_qos_1pRemark_get(i, &j)) < 0){
						rtlglue_printf("[%s:%d] rtk_qos_1pRemark_set fails, %d\n", __FUNCTION__, __LINE__, ret);
								goto done;
					}
					rtlglue_printf("\t pri[%d]	redot1p[%d]\n", i, j);
				}
			}
				goto done;
			}
			else if(strncmp(cmdptr, "port", 4) == 0){
				if((i = sscanf(strptr, "%d pri %d dot1p %d", &port, &prio, &j)) != 3){
					goto fail;
				}

				logic_port = rtk_switch_port_P2L_get(port);
				if(logic_port == UNDEFINE_PORT){
					goto fail;
				}
				
				if((ret = rtk_qos_1pRemarkEnable_set(logic_port, 1)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_83xx_qos_1pRemarkEnable_set error", ret);
					goto done;
				}

				if((ret = rtk_qos_1pRemark_set(prio, j)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_83xx_qos_1pRemark_set error", ret);
				}
				goto done;
			}
			else if(strncmp(cmdptr, "disable", 7) == 0){
				for(prio = 0; prio <= RTK_PRIMAX; prio++){
					if((ret = rtk_qos_1pRemark_set(prio, 0)) != 0){
						rtlglue_printf("%s, 0x%x\n", "rtk_qos_1pRemark_set error", ret);
						continue;
					}
				}

				for(port = 0; port < RTK_MAX_NUM_OF_PORT; port++){
					logic_port = rtk_switch_port_P2L_get(port);
					if(logic_port == UNDEFINE_PORT){
						continue;
					}
					
					if((ret = rtk_qos_1pRemarkEnable_set(logic_port, 0)) != 0){
						rtlglue_printf("%s, 0x%x\n", "rtk_qos_1pRemarkEnable_set error", ret);
						continue;
					}
				}
				goto done;
			}
		}
		else if(strncmp(cmdptr, "redscp", 6) == 0){
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL)
				goto fail;

			if(strncmp(cmdptr, "show", 4) == 0){
				rtk_enable_t qEnable;
				if((ret = rtk_qos_dscpRemarkEnable_get(RTK_WHOLE_SYSTEM, &qEnable)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_dscpRemarkEnable_set error", ret);
					goto done;
				}

				rtlglue_printf("\nrtl83xx whole system dscp remark: %s\n", qEnable==DISABLED ? "disable" : "enable");
				for(i = 0; i <= RTK_PRIMAX; i++){
					if((ret = rtk_qos_dscpRemark_get(i, &j)) < 0){
						rtlglue_printf("[%s:%d] rtk_qos_dscpRemark_get fails, %d\n", __FUNCTION__, __LINE__, ret);
						continue;
					}
					rtlglue_printf("\t pri[%d]	redscp[%d]\n", i, j);
				}
				goto done;
			}
			else if(strncmp(cmdptr, "pri", 3) == 0){
				if((i = sscanf(strptr, "%d dscp %d", &prio, &j)) != 2){
					goto fail;
				}

				if((ret = rtk_qos_dscpRemarkEnable_set(RTK_WHOLE_SYSTEM, 1)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_dscpRemarkEnable_set error", ret);
					goto done;
				}

				if((ret = rtk_qos_dscpRemark_set(prio, j)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_83xx_qos_dscpRemark_set error", ret);
				}
				goto done;
			}
			else if(strncmp(cmdptr, "disable", 7) == 0){
				for(prio = 0; prio <= RTK_PRIMAX; prio++){
					if((ret = rtk_qos_dscpRemark_set(prio, 0)) != 0){
						rtlglue_printf("%s, 0x%x\n", "rtk_qos_dscpRemark_set error", ret);
						continue;
					}
				}
				
				if((ret = rtk_qos_dscpRemarkEnable_set(RTK_WHOLE_SYSTEM, 0)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_dscpRemarkEnable_set error", ret);
				}
				goto done;
			}
		}
		else if(strncmp(cmdptr, "prio2qid", 8) == 0){
			rtk_qos_pri2queue_t pPri2qid;
			
			rtlglue_printf("num\tprio0\tprio1\tprio2\tprio3\tprio4\tprio5\tprio6\tprio7\n");
			for(queue_num=1; queue_num<=RTL8367C_QUEUENO; queue_num++){
				rtlglue_printf("[%d]\t", queue_num);

				if((ret = rtk_qos_priMap_get(queue_num, &pPri2qid)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_priMap_get error", ret);
					goto done;
				}

				for(prio=0; prio<RTK_MAX_NUM_OF_PRIORITY; prio++){
					rtlglue_printf("%d\t", pPri2qid.pri2queue[prio]);
				}
				rtlglue_printf("\n");
			}
			goto done;
		}
		else if(strncmp(cmdptr, "decision", 8) == 0){
			rtk_priority_select_t pPriDec;
			
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL){
				goto fail;
			}

			if(strncmp(cmdptr, "show", 4) == 0){
				if((ret = rtk_83xx_qos_priSelGroup_get(0, &pPriDec)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_priSel_get error", ret);
					goto done;
				}

				rtlglue_printf("priority selection tbl 0:\n");
				rtlglue_printf("\tport:%d, 1p:%d, acl:%d, dscp:%d, cvlan:%d, svlan:%d, smac:%d, dmac:%d\n",
					pPriDec.port_pri, pPriDec.dot1q_pri, pPriDec.acl_pri, pPriDec.dscp_pri, 
					pPriDec.cvlan_pri, pPriDec.svlan_pri, pPriDec.smac_pri, pPriDec.dmac_pri);

				rtlglue_printf("priority selection tbl 1:\n");
				if((ret = rtk_83xx_qos_priSelGroup_get(1, &pPriDec)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_priSel_get error", ret);
					goto done;
				}
				rtlglue_printf("\tport:%d, 1p:%d, acl:%d, dscp:%d, cvlan:%d, svlan:%d, smac:%d, dmac:%d\n",
					pPriDec.port_pri, pPriDec.dot1q_pri, pPriDec.acl_pri, pPriDec.dscp_pri, 
					pPriDec.cvlan_pri, pPriDec.svlan_pri, pPriDec.smac_pri, pPriDec.dmac_pri);

				rtlglue_printf("\nport\tpriority selection tbl\n");
				for(port = 0; port < 8; port++){
					logic_port = rtk_switch_port_P2L_get(port);
					if(logic_port == UNDEFINE_PORT){
						continue;
					}

					if((ret = rtk_qos_portPriSelIndex_get(logic_port, &i)) != 0){
						rtlglue_printf("%s, 0x%x\n", "rtk_qos_portPriSelIndex_get error", ret);
						goto done;
					}

					rtlglue_printf("%d\t%d\n", logic_port, i);
				}
				goto done;
			}
			else if(strncmp(cmdptr, "set", 3) == 0){
				if((i = sscanf(strptr, "port %d 1p %d acl %d dscp %d cvlan %d svlan %d dmac %d smac %d", 
				&pPriDec.port_pri, &pPriDec.dot1q_pri, &pPriDec.acl_pri, &pPriDec.dscp_pri, 
				&pPriDec.cvlan_pri, &pPriDec.svlan_pri, &pPriDec.dmac_pri, &pPriDec.smac_pri)) != 8){
					rtlglue_printf("i: %d, port: %d, 802.1p: %d\n", i, pPriDec.port_pri, pPriDec.dot1q_pri);
					goto fail;
				}
				
				if((ret = rtk_83xx_qos_priSelGroup_set(0, &pPriDec)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_priSel_set tbl 0 error", ret);
					goto done;
				}

				if((ret = rtk_83xx_qos_priSelGroup_set(1, &pPriDec)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_priSel_set tbl 1 error", ret);
				}
				
				goto done;
			}
		}
		else if(strncmp(cmdptr, "dot1qpri", 8) == 0){
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL){
				goto fail;
			}

			if(strncmp(cmdptr, "show", 4) == 0){
				rtlglue_printf("dot1p remap:\n");
				for(i = 0; i <= RTL8367C_PRIMAX; i++){
					if((ret = rtk_qos_1pPriRemap_get(i,&prio)) != 0){
						rtlglue_printf("%s, 0x%x\n", "rtk_qos_1pPriRemap_get error", ret);
						goto done;
					}
					rtlglue_printf("\tdot1p %d: %d\n", i, prio);
				}
				goto done;
			}
			else if(strncmp(cmdptr, "dot1q", 5) == 0){
				if((i = sscanf(strptr, "%d pri %d", &j, &prio)) != 2){
					goto fail;
				}

				if((ret = rtk_qos_1pPriRemap_set(j, prio)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_1pPriRemap_set error", ret);
				}
				goto done;
			}
		}
		else if(strncmp(cmdptr, "dscppri", 7) == 0)
		{
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL){
				goto fail;
			}

			if(strncmp(cmdptr, "show", 4) == 0){
				rtlglue_printf("dscp remap:\n");
				for(i = 0; i <= RTL8367C_DSCPMAX; i++){

					if((ret = rtk_qos_dscpPriRemap_get(i, &prio)) != 0){
						rtlglue_printf("%s, 0x%x\n", "rtk_qos_dscpPriRemap_get error", ret);
						goto done;
					}

					rtlglue_printf("\tdscp %d: %d", i, prio);
					if(i && (i % 4) == 3)
						rtlglue_printf("\n");
				}
				goto done;
			}
			else if(strncmp(cmdptr, "dscp", 4) == 0){
				if((i = sscanf(strptr, "%d pri %d", &j, &prio)) != 2){
					goto fail;
				}

				if((ret = rtk_qos_dscpPriRemap_set(j, prio)) != 0){
					rtlglue_printf("%s, 0x%x\n", "rtk_qos_dscpPriRemap_set error", ret);
				}
				goto done;
			}
		}
		else if(strncmp(cmdptr, "cvlanpri", 8) == 0){
			// TODO
		}
		else if(strncmp(cmdptr, "svlanpri", 8) == 0){
			// TODO
		}
		else if(strncmp(cmdptr, "dmacpri", 7) == 0){
			// TODO
		}
		else if(strncmp(cmdptr, "smacpri", 7) == 0){
			// TODO
		}
	} 
	
fail:
	rtlglue_printf("error input\n");

done:
	return count;
}

static const struct file_operations fops_qos_83xx = {
	.owner		= THIS_MODULE,
	.open		= qos_83xx_proc_open,
	.read		= seq_read,
	.write		= qos_83xx_proc_write,
	.release	= single_release,
};
#endif


/***************************************************************************
 * sw_acl_83xx
 ***************************************************************************/
extern rtl83xx_acl_rule_header_t rtl83xx_acl_tbl;
/*for default acl filter selector*/
const unsigned char *type_string[FILTER_FIELD_RAW_END+1] = {
	"unused", 
	"dmac(0-15)",
	"dmac(16-31)",
	"dmac(32-47)",
	"smac(0-15)",
	"smac(16-31)",
	"smac(32-47)",
	"eth type",
	"stag",
	"ctag",
	"","","","","","",
	"v4 sip(0-15)",
	"v4 sip(16-31)",
	"v4 dip(0-15)",
	"v4 dip(16-31)",
 	"","","","","","","","","","","","",
	"v6 sip(0-15)",
	"v6 sip(16-31)",
	"","","","","","",
	"v6 dip(0-15)",
	"v6 dip(16-31)",
	"","","","","","",
	"vid range",
	"ip range",
	"port range",
	"vaild",
	"","","","","","","","","","","","",
	"select00",
	"select01", 
	"select02", 
	"select03", 
	"select04", 
	"select05", 
	"select06", 
	"select07", 
	"select08",
	"select09",
	"select10", 
	"select11", 
	"select12", 
	"select13", 
	"select14",
	"select15",
	"end"
};

const unsigned char *action_string[FILTER_ENACT_END+1] = {
	"cvlan ingress",
	"cvlan egress",
	"cvlan svid",
	"policing1",
	"svlan ingress",
	"svlan egress",
	"svlan cvid",
	"policing2",
	"policing0",
	"copy cpu",
	"drop",
	"add dst port",
	"redirect",
	"mirror",
	"trap to cpu",
	"isolation",
	"priority",
	"dscp remark",
	"1p remark",
	"policing3",
	"interrupt",
	"gpo",
	"egress ctag untag",
	"egress ctag tag",
	"egress ctag keep",
	"egress ctag keep and 1p remark",
	"end"	
};

const unsigned char *iprange_string[IPRANGE_END+1] = {
	"unused",
	"ipv4 sip",
	"ipv4 dip",
	"ipv6 sip",
	"ipv6 dip",
	"end"	
};
const unsigned char *portrange_string[PORTRANGE_END+1] = {
	"unused",
	"sport",
	"dport",
	"end"	
};
const unsigned char *vidrange_string[VIDRANGE_END+1] = {
	"unused",
	"cvid",
	"svid",
	"end"	
};

static int sw_acl_83xx_show_help(struct seq_file *s, void *v)
{
	rtl83xx_acl_rule_t *curAcl;
	rtk_int32 cnt=0;
	
	unsigned char *actionT[RTL83XX_ACL_END+1] = 
		{"copy cpu",
		 "drop",
		 "redirect",
		 "mirror",
		 "trap to cpu",
		 "priority",
		 "dscp remark",
		 "1p remark",
		 "policing0",
		 "invalid"
		};
	unsigned char *filterT[FILTER_END+1]=
		{"none",
		 "mask",
		 "range",
		 "invalid"
		};
	
	seq_printf(s, "rtl83xx sw acl rule:\n");	
	CLIST_FOREACH(curAcl,&rtl83xx_acl_tbl,next_rule)
	{
		seq_printf(s, "[%d]\tprio:%d, member:%x, invert:%d, size:%d, in_asic:%d, index:%d\n", 
			cnt, curAcl->prio, curAcl->member, curAcl->invert, 
			curAcl->size, curAcl->in_asic, curAcl->index);

		seq_printf(s,"\tether type: %x	 ether type mask: %x\n", curAcl->filter.ethertype, curAcl->filter.ethertype_mask);
		seq_printf(s,"\tSMAC: %x:%x:%x:%x:%x:%x  SMACM: %x:%x:%x:%x:%x:%x\n",
			curAcl->filter.smac.octet[0],curAcl->filter.smac.octet[1],curAcl->filter.smac.octet[2],
			curAcl->filter.smac.octet[3],curAcl->filter.smac.octet[4],curAcl->filter.smac.octet[5],
			curAcl->filter.smac_mask.octet[0],curAcl->filter.smac_mask.octet[1],curAcl->filter.smac_mask.octet[2],
			curAcl->filter.smac_mask.octet[3],curAcl->filter.smac_mask.octet[4],curAcl->filter.smac_mask.octet[5]);
		seq_printf(s,"\tDMAC: %x:%x:%x:%x:%x:%x  DMACM: %x:%x:%x:%x:%x:%x\n",
			curAcl->filter.dmac.octet[0],curAcl->filter.dmac.octet[1],curAcl->filter.dmac.octet[2],
			curAcl->filter.dmac.octet[3],curAcl->filter.dmac.octet[4],curAcl->filter.dmac.octet[5],
			curAcl->filter.dmac_mask.octet[0],curAcl->filter.dmac_mask.octet[1],curAcl->filter.dmac_mask.octet[2],
			curAcl->filter.dmac_mask.octet[3],curAcl->filter.dmac_mask.octet[4],curAcl->filter.dmac_mask.octet[5]);
		seq_printf(s,"\tip filter type:%s sip: %d.%d.%d.%d sipM: %d.%d.%d.%d\n", 
			filterT[curAcl->filter.sip_type],
			(curAcl->filter.sip>>24),((curAcl->filter.sip&0x00ff0000)>>16), 
			((curAcl->filter.sip&0x0000ff00)>>8),(curAcl->filter.sip&0xff), 
			(curAcl->filter.sip_mask>>24), ((curAcl->filter.sip_mask&0x00ff0000)>>16),
			((curAcl->filter.sip_mask&0x0000ff00)>>8), (curAcl->filter.sip_mask&0xff));
		seq_printf(s,"\tip filter type:%s dip: %d.%d.%d.%d dipM: %d.%d.%d.%d\n", 
			filterT[curAcl->filter.dip_type],
			(curAcl->filter.dip>>24),((curAcl->filter.dip&0x00ff0000)>>16), 
			((curAcl->filter.dip&0x0000ff00)>>8),(curAcl->filter.dip&0xff), 
			(curAcl->filter.dip_mask>>24), ((curAcl->filter.dip_mask&0x00ff0000)>>16),
			((curAcl->filter.dip_mask&0x0000ff00)>>8), (curAcl->filter.dip_mask&0xff));
		seq_printf(s,"\tTos: %x   TosM: %x   ipProto: %x   ipProtoM: %x\n",
			curAcl->filter.ip_tos, curAcl->filter.ip_tos_mask, curAcl->filter.protocal, curAcl->filter.protocal_mask);
		seq_printf(s,"\tSport filter type:%s Sport:%x   SportM:%x\n",
			filterT[curAcl->filter.sport_type],
			curAcl->filter.sport, curAcl->filter.sport_mask);
		seq_printf(s,"\tDport filter type:%s Dport:%x   DportM:%x\n",
			filterT[curAcl->filter.dport_type],
			curAcl->filter.dport, curAcl->filter.dport_mask);
		seq_printf(s,"\tAction:%s",actionT[curAcl->action.act_type]);
		if(curAcl->action.act_type==RTL83XX_ACL_PRIORITY)
			seq_printf(s,"\t%d\n",curAcl->action.priority);
		else if(curAcl->action.act_type == RTL83XX_ACL_POLICING0)
			seq_printf(s,"\tmeter: %d rate: %d\n", curAcl->action.meter_index, curAcl->action.policing);
		else
			seq_printf(s, "\n");
		cnt++;
	}

	return 0;
}

static int sw_acl_83xx_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sw_acl_83xx_show_help, (void *) file);
}

/*1.add
	echo "add $member $rule_prio $invert $match1 $match2 ... $matchN action $action_para" > /proc/driver/realtek/sw_acl_83xx
	echo "add 1f 5 0 smac 01:00:5e:01:02:03 ff:ff:ff:ff:ff:ff action 5 2" > /proc/driver/realtek/sw_acl_83xx
	$matchN:
	smac $smac $smask  	--  smac 08:ef:4c:00:00:01 ff:ff:ff:ff:ff:ff:ff
	dmac $dmac $dmask	--  dmac 01:00:5e:01:02:03 ff:ff:ff:ff:ff:ff:ff
	sip $filter_type $sip $sipmask	-- sip 1 C0A80164 ffffffff  //mask
								-- sip 2 C0A80164 C0A80174 //ip range
	dip $filter_type $dip $dipmask 	-- dip 1 C0A802A0 ffffffff 
								-- dip 2 C0A802A0 C0A802B0
	tos $tos(hex) $mask(hex)		-- tos C FC 	//dscp 3 
	proto $proto(hex) $mask(hex) 	-- proto 6 ff 	//tcp 6
								-- proto 11 ff	//udp 17
	sport $filter_type $sport(dec) $mask(dec)	-- sport 1 2000 65535 	//mask
										-- sport 2 2000 3000  	//port range
	dport $filter_type $dport(dec) $mask(dec) 	-- dport 1 3000 65535 	//mask
										-- dport 2 3000 4000   	//port range
										
	$action_para
	1		//drop
	4		//trap to cpu
	5 $prio 	//qos prio
	8 $meter_index $rate			// policing0: ratelimit, unit kbps
	
   2.delete   	
  	echo "delete $member $rule_prio $invert $match1 $match2 ... $matchN action $action_para" > /proc/driver/realtek/sw_acl_83xx

   3.sync to asic
   	echo "to_asic" > /proc/driver/realtek/sw_acl_83xx

   4.flush
   	echo "flush all" > /proc/driver/realtek/sw_acl_83xx
   	echo "flush prio 0" > /proc/driver/realtek/sw_acl_83xx
*/
static ssize_t sw_acl_83xx_proc_write(struct file * file, const char __user * userbuf, 
	size_t count, loff_t * off)
{
	char tmpbuf[512] = {0};
	char *strptr = NULL, *tokptr = NULL;
	rtk_int32 add_flag = 0, cnt = 0, rulenum=0, flush_prio = -1;
	rtk_uint32 mac[6]={0}, mac_mask[6]={0};

	rtl83xx_acl_rule_t rule;
	memset(&rule, 0, sizeof(rtl83xx_acl_rule_t));

	if(count < 2 || count > 511)
		goto errout;

	if(userbuf && !copy_from_user(tmpbuf, userbuf, count)){
		tmpbuf[count] = '\0';
		
		strptr = tmpbuf;
		tokptr = strsep(&strptr," ");

		if(tokptr != NULL)
		{
			if(!memcmp(tokptr, "add", 3)||!memcmp(tokptr, "del", 3))
			{
				if(!memcmp(tokptr, "add", 3))
					add_flag = 1;
				else
					add_flag = 0;

				//member
				tokptr = strsep(&strptr, " ");
				if(tokptr==NULL)
					goto errout;
				cnt = sscanf(tokptr, "%x", &rule.member);

				//prio
				tokptr = strsep(&strptr, " ");
				if(tokptr==NULL)
					goto errout;
				cnt = sscanf(tokptr, "%d", &rule.prio);

				//invert
				tokptr = strsep(&strptr, " ");
				if(tokptr==NULL)
					goto errout;
				cnt = sscanf(tokptr, "%c", &rule.invert);
				rule.invert -= '0';
				
				tokptr = strsep(&strptr, " ");
				while(tokptr && memcmp(tokptr, "action", 6))
				{
					if(!memcmp(tokptr, "smac", 4))
					{	
						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%2x:%2x:%2x:%2x:%2x:%2x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
						rule.filter.smac.octet[0] = mac[0];
						rule.filter.smac.octet[1] = mac[1];
						rule.filter.smac.octet[2] = mac[2];
						rule.filter.smac.octet[3] = mac[3];
						rule.filter.smac.octet[4] = mac[4];
						rule.filter.smac.octet[5] = mac[5];

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;					
						cnt = sscanf(tokptr, "%2x:%2x:%2x:%2x:%2x:%2x", &mac_mask[0], &mac_mask[1], &mac_mask[2], &mac_mask[3], &mac_mask[4], &mac_mask[5]);
						rule.filter.smac_mask.octet[0] = mac_mask[0];
						rule.filter.smac_mask.octet[1] = mac_mask[1];
						rule.filter.smac_mask.octet[2] = mac_mask[2];
						rule.filter.smac_mask.octet[3] = mac_mask[3];
						rule.filter.smac_mask.octet[4] = mac_mask[4];
						rule.filter.smac_mask.octet[5] = mac_mask[5];

						rulenum ++;
					}
					else if(!memcmp(tokptr, "dmac", 4))
					{	
						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%2x:%2x:%2x:%2x:%2x:%2x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
						rule.filter.dmac.octet[0] = mac[0];
						rule.filter.dmac.octet[1] = mac[1];
						rule.filter.dmac.octet[2] = mac[2];
						rule.filter.dmac.octet[3] = mac[3];
						rule.filter.dmac.octet[4] = mac[4];
						rule.filter.dmac.octet[5] = mac[5];

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;					
						cnt = sscanf(tokptr, "%2x:%2x:%2x:%2x:%2x:%2x", &mac_mask[0], &mac_mask[1], &mac_mask[2], &mac_mask[3], &mac_mask[4], &mac_mask[5]);
						rule.filter.dmac_mask.octet[0] = mac_mask[0];
						rule.filter.dmac_mask.octet[1] = mac_mask[1];
						rule.filter.dmac_mask.octet[2] = mac_mask[2];
						rule.filter.dmac_mask.octet[3] = mac_mask[3];
						rule.filter.dmac_mask.octet[4] = mac_mask[4];
						rule.filter.dmac_mask.octet[5] = mac_mask[5];

						rulenum ++;
					}
					else if(!memcmp(tokptr, "sip", 3))
					{
					
						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%d", &rule.filter.sip_type);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%x", &rule.filter.sip);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%x", &rule.filter.sip_mask);

						rulenum ++;
					}
					else if(!memcmp(tokptr, "dip", 3))
					{
					
						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%d", &rule.filter.dip_type);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%x", &rule.filter.dip);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%x", &rule.filter.dip_mask);

						rulenum ++;
					}
					else if(!memcmp(tokptr, "tos", 3))
					{
					
						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%x", &rule.filter.ip_tos);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%x", &rule.filter.ip_tos_mask);

						rulenum ++;
					}
					else if(!memcmp(tokptr, "proto", 5))
					{
					
						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%x", &rule.filter.protocal);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%x", &rule.filter.protocal_mask);

						rulenum ++;
					}
					else if(!memcmp(tokptr, "sport", 5))
					{
					
						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%d", &rule.filter.sport_type);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%d", &rule.filter.sport);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%d", &rule.filter.sport_mask);

						rulenum ++;
					}
					else if(!memcmp(tokptr, "dport", 5))
					{
					
						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%d", &rule.filter.dport_type);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%d", &rule.filter.dport);

						tokptr = strsep(&strptr, " ");
						if(tokptr == NULL)
							goto errout;
						cnt = sscanf(tokptr, "%d", &rule.filter.dport_mask);

						rulenum ++;
					}
					else
					{
						goto errout;
					}
					
					tokptr = strsep(&strptr, " ");

				}
				if(rulenum==0 || memcmp(tokptr, "action", 6))
					goto errout;

				tokptr = strsep(&strptr, " ");
				if(tokptr == NULL)
					goto errout;

				cnt = sscanf(tokptr, "%d", &rule.action.act_type);
				if(rule.action.act_type >= RTL83XX_ACL_END)
					goto errout;
					
				if(rule.action.act_type==RTL83XX_ACL_PRIORITY)
				{
					tokptr = strsep(&strptr, " ");
					if(tokptr == NULL)
						goto errout;
					
					cnt = sscanf(tokptr, "%d", &rule.action.priority);
				}
				else if(rule.action.act_type == RTL83XX_ACL_POLICING0)
				{
					tokptr = strsep(&strptr, " ");
					if(tokptr == NULL)
						goto errout;

					cnt = sscanf(tokptr, "%d", &rule.action.meter_index);
					if(rule.action.meter_index > RTL83XX_METERMAX)
						goto errout;
					
					tokptr = strsep(&strptr, " ");
					if(tokptr == NULL)
						goto errout;
					
					cnt = sscanf(tokptr, "%d", &rule.action.policing);
					if(rule.action.policing < 8 || rule.action.policing > (0x1FFFF*8))
						goto errout;
				}

				if(add_flag)
					rtl83xx_addAclRule(&rule);
				else
					rtl83xx_deleteAclRule(&rule);

			}
			else if(!memcmp(tokptr, "to_asic", 7))
			{
				rtl83xx_syncAclTblToAsic();
			}
			else if(!memcmp(tokptr, "flush", 5))
			{
				tokptr = strsep(&strptr, " ");
				if(tokptr==NULL)
					goto errout;

				if(!memcmp(tokptr, "all", 3))
				{
					rtl83xx_flushAclRule();
					rtl83xx_syncAclTblToAsic();
				}
				else if(!memcmp(tokptr, "prio", 4))
				{
					tokptr = strsep(&strptr, " ");
					if(tokptr==NULL)
						goto errout;
					cnt = sscanf(tokptr, "%d", &flush_prio);
					rtl83xx_flushAclRulebyPrio(flush_prio);
					rtl83xx_syncAclTblToAsic();
				}
				else
					goto errout;
			}
			else{
				goto errout;
			}
		}
		else{
			goto errout;
		}
	}
	else{
errout:
		rtlglue_printf("error command!\n");
	}
	
	return count;
}

static const struct file_operations fops_sw_acl_83xx = {
	.owner		= THIS_MODULE,
	.open		= sw_acl_83xx_proc_open,
	.read		= seq_read,
	.write		= sw_acl_83xx_proc_write,
	.release	= single_release,
};


/***************************************************************************
 * hw_acl_83xx
 ***************************************************************************/
static int hw_acl_83xx_show_help(struct seq_file *s, void *v)
{
	rtk_filter_id_t filter_id;
	rtk_int32 range_id;
	rtk_filter_act_enable_t i;
	for(filter_id=0; filter_id<=RTL83XX_ACLRULEMAX; filter_id++)
	{
		rtk_filter_cfg_raw_t pFilter_cfg;
		rtk_filter_action_t pAction;
		
		rtk_filter_igrAcl_cfg_get(filter_id, &pFilter_cfg, &pAction);
		{
			if(pFilter_cfg.valid)
			{	
				seq_printf(s,"[%d]", filter_id);
				seq_printf(s,"\tActivePort:%x, PMask:%x, Invert:%d\n", pFilter_cfg.activeport.value, pFilter_cfg.activeport.mask, pFilter_cfg.invert);
				seq_printf(s,"\tData:\t%4x\t%4x\t%4x\t%4x\t%4x\t%4x\t%4x\t%4x\n", 
					pFilter_cfg.dataFieldRaw[0],
					pFilter_cfg.dataFieldRaw[1],
					pFilter_cfg.dataFieldRaw[2],
					pFilter_cfg.dataFieldRaw[3],
					pFilter_cfg.dataFieldRaw[4],
					pFilter_cfg.dataFieldRaw[5],
					pFilter_cfg.dataFieldRaw[6],
					pFilter_cfg.dataFieldRaw[7]);
				seq_printf(s,"\tDMask:\t%4x\t%4x\t%4x\t%4x\t%4x\t%4x\t%4x\t%4x\n", 
					pFilter_cfg.careFieldRaw[0],
					pFilter_cfg.careFieldRaw[1],
					pFilter_cfg.careFieldRaw[2],
					pFilter_cfg.careFieldRaw[3],
					pFilter_cfg.careFieldRaw[4],
					pFilter_cfg.careFieldRaw[5],
					pFilter_cfg.careFieldRaw[6],
					pFilter_cfg.careFieldRaw[7]);
				seq_printf(s,"\ttype:\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
					type_string[pFilter_cfg.fieldRawType[0]],
					type_string[pFilter_cfg.fieldRawType[1]],
					type_string[pFilter_cfg.fieldRawType[2]],
					type_string[pFilter_cfg.fieldRawType[3]],
					type_string[pFilter_cfg.fieldRawType[4]],
					type_string[pFilter_cfg.fieldRawType[5]],
					type_string[pFilter_cfg.fieldRawType[6]],
					type_string[pFilter_cfg.fieldRawType[7]]);
				seq_printf(s, "\tAction:");
				for(i=0; i<FILTER_ENACT_END; i++)
				{
					if(pAction.actEnable[i])
					{
						seq_printf(s, "%s ", action_string[i]);
						if(i==FILTER_ENACT_PRIORITY)
							seq_printf(s, "%d ", pAction.filterPriority);
						else if(i == FILTER_ENACT_POLICING_0)
							seq_printf(s, "meter %d ", pAction.filterPolicingIdx[0]);
						/*to do:other type paramater*/
					}
				}
				seq_printf(s, "\n");
			}
		}
	}
	
	seq_printf(s, "vid range list:\n");
	for(range_id=0; range_id<=RTL83XX_ACLRANGEMAX; range_id++)
	{
		rtk_filter_vidrange_t pType;
		rtk_uint32 pUpperVid, pLowerVid;

		rtk_filter_vidrange_get(range_id, &pType, &pUpperVid, &pLowerVid);
		
		if(pType>VIDRANGE_UNUSED && pType<VIDRANGE_END)
			seq_printf(s,"\tvid range:%d, type:%s, upper:%d, lower:%d\n", 1<<range_id, vidrange_string[pType], pUpperVid, pLowerVid);

	}

	seq_printf(s, "port range list:\n");
	for(range_id=0; range_id<=RTL83XX_ACLRANGEMAX; range_id++)
	{
		rtk_filter_portrange_t pType;
		rtk_uint32 pUpperPort, pLowerPort;

		rtk_filter_portrange_get(range_id, &pType, &pUpperPort, &pLowerPort);
		
		if(pType>PORTRANGE_UNUSED && pType<PORTRANGE_END)
			seq_printf(s,"\tport range:%d, type:%s, upper:%x, lower:%x\n", 1<<range_id, portrange_string[pType], pUpperPort, pLowerPort);
	}

	seq_printf(s, "ip range list:\n");
	for(range_id=0; range_id<=RTL83XX_ACLRANGEMAX; range_id++)
	{
		rtk_filter_iprange_t pType;
		ipaddr_t pUpperIp[4], pLowerIp[4];

		rtk_filter_iprange_get(range_id, &pType, pUpperIp, pLowerIp);
		if(pType>IPRANGE_UNUSED && pType<=IPRANGE_IPV4_DIP)
		{
			seq_printf(s, "\tip range:%d, type:%s, upper:%x, lower:%x\n", 1<<range_id, iprange_string[pType], pUpperIp[0], pLowerIp[0]);
		}
		else if(pType>IPRANGE_IPV4_DIP && pType<IPRANGE_END)
		{
			seq_printf(s, "\tip range:%d, type:%s, upper:%x-%x-%x-%x, lower:%x-%x-%x-%x",
				1<<range_id, iprange_string[pType], 
				pUpperIp[0], pUpperIp[1], pUpperIp[2], pUpperIp[3],
				pLowerIp[0], pLowerIp[1], pLowerIp[2], pLowerIp[3]);
		}
		else
		{
		}
	}
	return 0;
}

static int hw_acl_83xx_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hw_acl_83xx_show_help, (void *) file);
}

static ssize_t hw_acl_83xx_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	return count;	
}

static const struct file_operations fops_hw_acl_83xx = {
	.owner		= THIS_MODULE,
	.open		= hw_acl_83xx_proc_open,
	.read		= seq_read,
	.write		= hw_acl_83xx_proc_write,
	.release	= single_release,
};			  


/* 83xx vlan */
static int vlan_83xx_show_help(struct seq_file *s, void *v)
{
	rtl_8367r_vlan_read();
	
	return 0;
}

static int vlan_83xx_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vlan_83xx_show_help, (void *) file);
}

static ssize_t vlan_83xx_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char tmpbuf[128] = {'\0'};
	char *strptr = NULL, *cmd_addr = NULL;
	char *tokptr = NULL;
	int ret = 0, i = 0;
	rtk_uint32 vid =0, mbrmsk = 0, untagmsk = 0, tagmsk = 0, fid = 0, priority = 0;

	if(count > 127)
		goto errout;

	if(userbuf && !copy_from_user(tmpbuf, userbuf, count))
	{
		tmpbuf[count] = '\0';
		strptr = tmpbuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
		{
			goto errout;
		}
		
		rtlglue_printf("cmd %s\n", cmd_addr);
		if (!memcmp(cmd_addr, "dump", 4))
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			rtlglue_printf("\n83xx vlan info:\n");
			if ((!memcmp(tokptr, "all", 3))){
					//only dump special vid
					for (i = 0; i <= 4096; i++) 
					{	
						if(i==1 || i==8 || i==9 || i==10 || i%100==0){
							mbrmsk = untagmsk = priority = fid = 0;
							ret = rtl83xx_vlan_get(i, &mbrmsk, &untagmsk, &priority,&fid);
							if (ret != 0)
								continue;	
							rtlglue_printf("vid %d Mbrmsk 0x%x Untagmsk 0x%x fid %u priority %u\n", i, mbrmsk, untagmsk, fid, priority);	 
						}
					}
			}
			else{
				//dump single vlan info
				vid = simple_strtol(tokptr, NULL, 0);	
				
				ret = rtl83xx_vlan_get(vid, &mbrmsk, &untagmsk, &priority,&fid);
				if (ret == 0){
					rtlglue_printf("vid %d Mbrmsk 0x%x Untagmsk 0x%x fid %u priority %u\n", vid, mbrmsk, untagmsk, fid, priority);
				}
				else{
					rtlglue_printf("get vlan vid %d error ! \n", vid);
				}
			}
		}
		else if ((!memcmp(cmd_addr, "add", 3)) || 
			(!memcmp(cmd_addr, "Add", 3)) ||
			(!memcmp(cmd_addr, "ADD", 3)))
		{
			
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			vid=simple_strtol(tokptr, NULL, 0);
			
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mbrmsk = simple_strtol(tokptr, NULL, 16);
			
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			untagmsk = simple_strtol(tokptr, NULL, 16);
			
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			fid = simple_strtol(tokptr, NULL, 0);

			//priority is optional 
			tokptr = strsep(&strptr," ");
			if (tokptr)
			{
				priority = simple_strtol(tokptr, NULL, 0);
			}
			
			rtlglue_printf("vid = %d mbrmsk = 0x%x  untagmsk = 0x%x fid = %u priority=%u\n", vid, mbrmsk, untagmsk, fid, priority);
			ret = rtl83xx_vlan_set(vid, mbrmsk, untagmsk, priority,fid);
			
			if (ret == 0){
				rtlglue_printf("vid %d success!\n", vid);
			}
			else{
				rtlglue_printf("vid %d failed!\n", vid);
			}

		}
		else if ((!memcmp(cmd_addr, "del", 3)) || 
			(!memcmp(cmd_addr, "Del", 3)) ||
			(!memcmp(cmd_addr, "DEL", 3)))
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			
			tagmsk = untagmsk = mbrmsk = fid = priority = 0;
			if (!memcmp(tokptr, "all", 3)){ 
				for (i = 0; i <= 4095; i++)
				{
					ret = rtl83xx_vlan_set(i, mbrmsk, untagmsk, priority,fid);
				}
			}
			else{
				vid = simple_strtol(tokptr, NULL, 0);
				ret = rtl83xx_vlan_set(vid, mbrmsk, untagmsk, priority,fid);
				if (ret == 0){
					rtlglue_printf("vid %d success!\n", vid);
				}
				else{
					rtlglue_printf("vid %d failed!\n", vid);
				}
			}
		}
		else{
			goto errout;
		}


		return count;
errout:
		rtlglue_printf("echo \"add $vid $mbrmsk $untagmsk $fid [$priority]\" > /proc/driver/realtek/vlan_83xx\n");
		rtlglue_printf("echo \"del $vid \" > /proc/driver/realtek/vlan_83xx\n");
		rtlglue_printf("echo \"dump $vid \" > /proc/driver/realtek/vlan_83xx\n");
	}
	
	return count;
}

static const struct file_operations fops_vlan_83xx = {
	.owner		= THIS_MODULE,
	.open		= vlan_83xx_proc_open,
	.read		= seq_read,
	.write		= vlan_83xx_proc_write,
	.release	= single_release,
};

static int pvid_83xx_show_help(struct seq_file *s, void *v)
{
	rtk_int32  i;	
	rtk_uint32 pvid = 0, priority = 0;
	int ret = 0;

	seq_printf(s,"\nport pvid pri\n");
	for (i = 0; i < 8; i++){
		pvid = priority = 0;		
		ret = rtl83xx_pvid_get(i, &pvid, &priority);
		seq_printf(s,"port%d %u %u\n", i, (ret==0)? pvid : 0, (ret==0)? priority : 0);
	}
	seq_printf(s,"\n");


	return 0;
}


static int pvid_83xx_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, pvid_83xx_show_help, (void *) file);
}

static ssize_t pvid_83xx_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char tmpbuf[128] = {'\0'};	
	char *strptr = NULL;
	char *tokptr = NULL;
	rtk_uint32 port = 0, pvid = 0, priority = 0;
	rtk_int32 ret = 0;
	
	if(count > 127)
		goto errout;
	
	if (userbuf && !copy_from_user(tmpbuf, userbuf, count))
	{
		
		tmpbuf[count] = '\0';
		strptr = tmpbuf;

		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		port = simple_strtol(tokptr, NULL, 0);

		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		pvid = simple_strtol(tokptr, NULL, 0);
		if(pvid > 4096)
		{
			goto errout;
		}

		tokptr = strsep(&strptr," ");
		if (tokptr)
		{
			priority = simple_strtol(tokptr, NULL, 0);
		}
		
		ret = rtl83xx_pvid_set(port, pvid, priority);

		return count;
errout:
		rtlglue_printf("echo $port $pvid [$priority] > /proc/driver/realtek/pvid_83xx\n");

	}

	return count;
}


static const struct file_operations fops_pvid_83xx = {
	.owner		= THIS_MODULE,
	.open		= pvid_83xx_proc_open,
	.read		= seq_read,
	.write		= pvid_83xx_proc_write,
	.release	= single_release,
};



#if defined(CONFIG_FC_RTL8198F_SERIES)&&defined(CONFIG_RTK_DEV_AP)
extern int rtk_l2_8367_ipMcastAddr_get(u32 dip, u32 sip ,rtk_portmask_t* portmask);
extern int rtk_l2_8367_McastAddr_get(char *Dmac,rtk_portmask_t* portmask);

static ssize_t mc_l2table_83xx_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char tmpbuf[128] = {'\0'};
	char *strptr = NULL, *cmd_addr = NULL;
	char *tokptr = NULL;
	int ret = 0, i = 0;
	unsigned int SipAddr[4] = {0};
	unsigned int DipAddr[4] = {0};
	unsigned int groupAddr = 0;
	unsigned int srcAddr = 0;
	char octet[ETHER_ADDR_LEN]= {0};
	rtk_portmask_t ipMcastmask;
	u32 address = 0 ;
	rtk_l2_ipMcastAddr_t ipMcastAddr;
	rtk_l2_mcastAddr_t  mcastAddr;


	if(count > 127)
		goto errout;

	if(userbuf && !copy_from_user(tmpbuf, userbuf, count))
	{
		tmpbuf[count] = '\0';
		strptr = tmpbuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
		{
			goto errout;
		}
		
		if ((!memcmp(cmd_addr, "get", 3)) ||
			(!memcmp(cmd_addr, "Get", 3)) ||
			(!memcmp(cmd_addr, "GET", 3)))
		{
			rtlglue_printf("Get info : \n");

			cmd_addr = strsep(&strptr," ");
			if (cmd_addr==NULL)
			{
				goto errout;
			}
			
			if ((!memcmp(cmd_addr, "dip", 3)) ||
			(!memcmp(cmd_addr, "DIP", 3)) ||
			(!memcmp(cmd_addr, "Dip", 3)))
			{
				cmd_addr = strsep(&strptr," ");
				if (cmd_addr==NULL)
				{
					goto errout;
				}
				
				sscanf(cmd_addr, "%d.%d.%d.%d", &DipAddr[0], &DipAddr[1], &DipAddr[2], &DipAddr[3]);
				groupAddr=(DipAddr[0]<<24)|(DipAddr[1]<<16)|(DipAddr[2]<<8)|(DipAddr[3]);
					
				cmd_addr = strsep(&strptr," ");
				if (cmd_addr==NULL)
				{
					goto errout;
				}

				sscanf(cmd_addr, "%d.%d.%d.%d", &SipAddr[0], &SipAddr[1], &SipAddr[2], &SipAddr[3]);
				srcAddr=(SipAddr[0]<<24)|(SipAddr[1]<<16)|(SipAddr[2]<<8)|(SipAddr[3]);

				//printk("%s:%d:DIp =%d.%d.%d.%d\n", __FUNCTION__,__LINE__,DipAddr[0],DipAddr[1],DipAddr[2],DipAddr[3]);
				//printk("%s:%d:SIp =%d.%d.%d.%d\n", __FUNCTION__,__LINE__,SipAddr[0],SipAddr[1],SipAddr[2],SipAddr[3]);

				if(0 == rtk_l2_8367_ipMcastAddr_get(groupAddr, srcAddr ,&ipMcastmask))
				{
					rtlglue_printf("GIP:%d.%d.%d.%d  SIP:%d.%d.%d.%d 8367_PortMask 0x%x \n",DipAddr[0],DipAddr[1],DipAddr[2],DipAddr[3],SipAddr[0],SipAddr[1],SipAddr[2],SipAddr[3],ipMcastmask.bits[0]);
				}
				else
					rtlglue_printf("Can't find l2 MC entry :GIP:%d.%d.%d.%d  SIP:%d.%d.%d.%d \n",DipAddr[0],DipAddr[1],DipAddr[2],DipAddr[3],SipAddr[0],SipAddr[1],SipAddr[2],SipAddr[3]);
			}
			else if((!memcmp(cmd_addr, "dmac", 4)) ||
			(!memcmp(cmd_addr, "Dmac", 4)) ||
			(!memcmp(cmd_addr, "DMAC", 4)))
			{
				cmd_addr = strsep(&strptr," ");
				if (cmd_addr==NULL)
				{
					goto errout;
				}
				
				sscanf(cmd_addr, "%2x-%2x-%2x-%2x-%2x-%2x", &octet[0], &octet[1], &octet[2], &octet[3], &octet[4], &octet[5]);
				if(0 ==rtk_l2_8367_McastAddr_get(octet,&ipMcastmask))
				{
					rtlglue_printf("find l2 MC entry. MAC:%2x-%2x-%2x-%2x-%2x-%2x ,8367_PortMask 0x%x \n",&octet[0], &octet[1], &octet[2], &octet[3], &octet[4], &octet[5],ipMcastmask.bits[0]);
				}
				else
					rtlglue_printf("Can't find l2 MC entry . MAC:%2x-%2x-%2x-%2x-%2x-%2x  \n",&octet[0], &octet[1], &octet[2], &octet[3], &octet[4], &octet[5]);
					
			}
			else
			{
				goto errout;
			}
		}
		else if ((!memcmp(cmd_addr, "show", 4)) || 
			(!memcmp(cmd_addr, "Show", 4)) ||
			(!memcmp(cmd_addr, "SHOW", 4)))
		{
			cmd_addr = strsep(&strptr," ");
			if (cmd_addr==NULL)
			{
				goto errout;
			}

			if((!memcmp(cmd_addr, "IPTABLE", 7))|| 
			(!memcmp(cmd_addr, "iptable", 7)) ||
			(!memcmp(cmd_addr, "IpTable", 7)))
			{	
				rtlglue_printf("Show all L2 IP MC table : \n");
				address = 0;
				while(1)
				{
					if(rtk_l2_ipMcastAddr_next_get(&address, &ipMcastAddr)!= RT_ERR_OK)
					{
						break;
					}
					address = address+1;
					rtlglue_printf("dip:0x%x sip:0x%x portmask:0x%x priority:[%d] fwd_pri_en:[%d] igmp_asic:[%d] igmp_index:[%d] address:[%d]\n",
					ipMcastAddr.dip, ipMcastAddr.sip, ipMcastAddr.portmask,ipMcastAddr.priority,ipMcastAddr.fwd_pri_en,ipMcastAddr.igmp_asic,ipMcastAddr.igmp_index,ipMcastAddr.address);
				}

			}
			else if((!memcmp(cmd_addr, "MacTable", 8))|| 
			(!memcmp(cmd_addr, "MACTABLE", 8)) ||
			(!memcmp(cmd_addr, "mactable", 8)))
			{
				address = 0;
				while(1)
				{
					if(rtk_l2_mcastAddr_next_get(&address, &mcastAddr)!= RT_ERR_OK) 
					{
						break;
					}
					address = address+1;
					rtlglue_printf("mac:0x%2x-%2x-%2x-%2x-%2x-%2x portmask:0x%x vid:[%d] fid:[%d] priority:[%d] ivl:[%d] fwd_pri_en:[%d] igmp_asic:[%d] igmp_index:[%d] address:[%d]\n",
					mcastAddr.mac.octet[0],mcastAddr.mac.octet[1],mcastAddr.mac.octet[2],mcastAddr.mac.octet[3],mcastAddr.mac.octet[4],mcastAddr.mac.octet[5],\
					mcastAddr.portmask, mcastAddr.vid, mcastAddr.fid,mcastAddr.priority,mcastAddr.ivl, mcastAddr.fwd_pri_en,mcastAddr.igmp_asic,mcastAddr.igmp_index,mcastAddr.address);
				}
			}
			else{
				goto errout;
			}

		}
		/*else if ((!memcmp(cmd_addr, "add", 3)) || 
			(!memcmp(cmd_addr, "Add", 3)) ||
			(!memcmp(cmd_addr, "ADD", 3)))
		{
			

		}
		else if ((!memcmp(cmd_addr, "del", 3)) || 
			(!memcmp(cmd_addr, "Del", 3)) ||
			(!memcmp(cmd_addr, "DEL", 3)))
		{
		    
		}*/
		else{
			goto errout;
		}


		return count;
errout:
		
		rtlglue_printf("Invalid input parameters:\n");
		
	}
	
	return count;
}



static int mc_l2table_83xx_show_help(struct seq_file *s, void *v)
{
	return PROC_READ_RETURN_VALUE;
}

static int mc_l2table_83xx_open(struct inode *inode, struct file *file)
{
	return single_open(file, mc_l2table_83xx_show_help, (void *) file);
}


static const struct file_operations fops_mc_l2table_83xx = {
	.owner		= THIS_MODULE,
	.open		= mc_l2table_83xx_open,
	.read		= seq_read,
	.write		= mc_l2table_83xx_write,
	.release	= single_release,
};
#endif

/***************************************************************************
 * init/exit functions
 ***************************************************************************/

int rtl83xx_proc_init(void)
{
	if (proc_create("asiccounter", 0, rtk_proc_dir, &fops_asiccounter) == NULL) {
		pr_err("Fail to create proc entry asiccounter.\n");
		return -ENOMEM;
	}

	if (proc_create("phyreg", 0, rtk_proc_dir, &fops_phyreg) == NULL) {
		pr_err("Fail to create proc entry phyreg.\n");
		return -ENOMEM;
	}

#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	if (proc_create("qos_83xx", 0, rtk_proc_dir, &fops_qos_83xx) == NULL) {
		pr_err("Fail to create proc entry qos_83xx.\n");
		return -ENOMEM;
	}
#endif

	if (proc_create("sw_acl_83xx", 0, rtk_proc_dir, &fops_sw_acl_83xx) == NULL) {
		pr_err("Fail to create proc entry sw_acl_83xx.\n");
		return -ENOMEM;
	}

	if (proc_create("hw_acl_83xx", 0, rtk_proc_dir, &fops_hw_acl_83xx) == NULL) {
		pr_err("Fail to create proc entry hw_acl_83xx.\n");
		return -ENOMEM;
	}

	if (proc_create("vlan_83xx", 0, rtk_proc_dir, &fops_vlan_83xx) == NULL) {
		pr_err("Fail to create proc entry vlan_83xx.\n");
		return -ENOMEM;
	}
	
	if (proc_create("pvid_83xx", 0, rtk_proc_dir, &fops_pvid_83xx) == NULL) {
		pr_err("Fail to create proc entry pvid_83xx.\n");
		return -ENOMEM;
	}

	#if defined(CONFIG_FC_RTL8198F_SERIES)&&defined(CONFIG_RTK_DEV_AP)
	if (proc_create("mc_l2table_83xx", 0, rtk_proc_dir, &fops_mc_l2table_83xx) == NULL) {
		pr_err("Fail to create proc entry mc_l2table_83xx.\n");
		return -ENOMEM;
	}
#endif

	return 0;
}

void rtl83xx_proc_exit(void)
{
	remove_proc_entry("asiccounter", rtk_proc_dir);
	remove_proc_entry("phyreg", rtk_proc_dir);
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	remove_proc_entry("qos_83xx", rtk_proc_dir);
#endif
	remove_proc_entry("sw_acl_83xx", rtk_proc_dir);
	remove_proc_entry("hw_acl_83xx", rtk_proc_dir);
	remove_proc_entry("vlan_83xx", rtk_proc_dir);
	remove_proc_entry("pvid_83xx", rtk_proc_dir);
#if defined(CONFIG_FC_RTL8198F_SERIES)&&defined(CONFIG_RTK_DEV_AP)
	remove_proc_entry("mc_l2table_83xx", rtk_proc_dir);
#endif
}


MODULE_LICENSE("GPL");
