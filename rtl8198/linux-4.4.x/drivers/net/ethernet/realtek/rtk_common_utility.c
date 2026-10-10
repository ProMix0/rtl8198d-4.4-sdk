#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/inetdevice.h>
#include <soc/cortina/rtk_common_utility.h>
#include <soc/cortina/rtk_dns_trap.h>
#include <soc/cortina/rtk_vlan_passthrough.h>

#ifdef CONFIG_RTK_CTCAPD_FILTER_SUPPORT
#include "rtk_dns_filter.h"
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)&&defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
#include <uapi/linux/igmp.h>
int block_iptv_from_lanWlan_server = 1;
char block_lan_query_ifname[IFNAMSIZ] = {0};
EXPORT_SYMBOL(block_iptv_from_lanWlan_server);

#endif

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#ifdef CONFIG_RTL8686_SWITCH 
#include "rtk/stat.h"
#endif
#ifdef CONFIG_RTL8686NIC
#ifdef CONFIG_RTL9607C_SERIES
#include "re8686_rtl9607c.h"
#endif
#elif defined(CONFIG_LUNA_G3_SERIES)
#include "ca_ne_autoconf.h"
#include "ca_ni.h"
#endif
#ifdef CONFIG_COMMON_RT_API
#include "rtk/rt/rt_stat.h"
#include "rt_stat_ext.h"
#endif
#include <common/error.h>
#endif


struct proc_dir_entry *rtk_proc_dir = NULL;
int rtk_op_mode = GATEWAY_MODE; 
rtl83xx_op_mode_set_hook_t rtl83xx_op_mode_set_hook;


#if defined(CONFIG_RTL_8226_SUPPORT)
#include <linux/if.h>

char rtk_8226_lan_dev_name[IFNAMSIZ] = {0};
EXPORT_SYMBOL(rtk_8226_lan_dev_name);

rtl_get_8226_lan_dev_name_hook_t rtl_get_8226_lan_dev_name_hook;
EXPORT_SYMBOL(rtl_get_8226_lan_dev_name_hook);

int rtk_get_8226_lan_dev_name(void)
{
	memset(rtk_8226_lan_dev_name, 0, IFNAMSIZ*sizeof(char));
	if(rtl_get_8226_lan_dev_name_hook){
		(*rtl_get_8226_lan_dev_name_hook)(rtk_8226_lan_dev_name);
		printk("[%s:%d] rtk_8226_lan_dev_name: %s\n", __FUNCTION__, __LINE__, rtk_8226_lan_dev_name);
		return 0;
	}
	
	return -1;
}
EXPORT_SYMBOL(rtk_get_8226_lan_dev_name);
#endif

#if defined(CONFIG_FC_RTL8198F_SERIES)&&defined(CONFIG_RTK_DEV_AP)
rtl_get_8226_lan_dev_portnum_hook_t rtl_get_8226_lan_dev_portnum_hook;
EXPORT_SYMBOL(rtl_get_8226_lan_dev_portnum_hook);

rtl_get_portnum_in_8367_hook_t rtl_get_portnum_in_8367_hook;
EXPORT_SYMBOL(rtl_get_portnum_in_8367_hook);
#endif

aal_mdio_read_indirect_hook_t aal_mdio_read_indirect_hook=NULL;
aal_mdio_write_indirect_hook_t aal_mdio_write_indirect_hook=NULL;


#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)&&defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
int rtk_drop_igmpquery_from_lan_ports(struct sk_buff **pskb)
{
	struct sk_buff *skb = NULL;
	rtk_fc_ingress_data_t *ingress_data = NULL;
	unsigned char *pdata = NULL;
	struct iphdr *iph;
	struct igmphdr *igmph =NULL; //NULL:not igmp header
	unsigned int offset = 0;

	if (!pskb || !*pskb){
		//printk("%s:%d,parameter error! pskb or *psbk is NULL \n!",__FUNCTION__,__LINE__);
		return -1;
	}

	skb = *pskb;

	if(skb->protocol==htons(ETH_P_8021Q))
	{
		offset+=4;		
		//printk("[%s:%d] vlan packet!!!\n",__FUNCTION__,__LINE__);
	}


	iph=(struct iphdr *)(skb->data+offset);

	
	if(iph!=NULL && iph->version == 4){ // skb is ipv4

		//for igmp type
		if(iph->protocol==IPPROTO_IGMP){
			pdata = skb->data + offset;
			ingress_data = &skb->fcIngressData;

			//unlikely...... do nothing......
			if (!pdata){
				//printk("%s:%d packet is too short! skb->dev->name=%s, skb->len=%u \n",__FUNCTION__,__LINE__,skb->dev->name, skb->len);
				return 0;
			}

			igmph = (void *)iph + iph->ihl*4;

			//Drop query packet from lan port.
			if((igmph->type == 0x11) && (((block_lan_query_ifname[0] == '\0') && (memcmp(skb->dev->name, "nas", 3))) || ((block_lan_query_ifname[0] != '\0') && (!memcmp(skb->dev->name, block_lan_query_ifname, strlen(block_lan_query_ifname))))))
			{	
				//printk("---drop igmph->type =0x%x, skb->dev->name is %s\n",igmph->type,skb->dev->name);
				dev_kfree_skb_any(skb);
				return 1;
			}
			else
			{
				//printk("igmph->type =0x%x,skb->dev->name is %s\n",igmph->type,skb->dev->name);
				return 0;
			}
					
		}
		
	} 	

	return 0;
	
}
#endif

int rtk_netif_receive_skb_hooks(struct sk_buff **pskb)
{
	
#ifdef CONFIG_RTK_VLAN_PASSTHROUGH_SUPPORT
	if(0 != rtk_vlan_passthrough_rx(pskb, 1)){
		//failed skb already free.
		return RTK_PS_HOOKS_DROP;
	}
#endif

#if defined(CONFIG_RTK_DNS_TRAP)
	if (dns_trap_enable)
	{
		if(rtk_dns_trap_enter(pskb)==RTK_PS_HOOKS_RETURN)
			return RTK_PS_HOOKS_RETURN;
	}
#endif

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)&&defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)

	if(block_iptv_from_lanWlan_server == 1)
	{	
		if(1 == rtk_drop_igmpquery_from_lan_ports(pskb))
			return RTK_PS_HOOKS_DROP;
	}
#endif

	return RTK_PS_HOOKS_CONTINUE;
}


int rtk_netif_xmit_skb_hooks(struct sk_buff **pskb)
{
#ifdef CONFIG_RTK_CTCAPD_FILTER_SUPPORT
	rtk_filter_dns_query(pskb);
#endif

	return RTK_PS_HOOKS_CONTINUE;
}



/***************************************************************************
 *  handle for op mode
 ***************************************************************************/

static int rtk_op_mode_read(struct seq_file *s, void *v)
{
	
	seq_printf(s, "rtk_op_mode=%d \n", rtk_op_mode);
	
	return 0;
}

static int rtk_op_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_op_mode_read, (void *) file);
}

static ssize_t rtk_op_mode_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char tmpbuf[64] = {'\0'};
	int mode = 0;
	char *strptr = NULL, *cmd_addr = NULL;
	char *tokptr = NULL;

	if(count >= 64)
		goto errout;

	if (userbuf && (count > 0) && (count < sizeof(tmpbuf)) && !copy_from_user(tmpbuf, userbuf, count)) {

		tmpbuf[count - 1] = '\0';
		strptr = tmpbuf;
		cmd_addr = strsep(&strptr," ");
		
		if (cmd_addr == NULL)
			goto errout;

		if (!memcmp(cmd_addr, "op_mode", 7)) {
			
			tokptr = strsep(&strptr," ");
			if (tokptr == NULL)
				goto errout;

			mode = simple_strtol(tokptr, NULL, 10);

			printk("mode=%d , rtk_op_mode = %d\n", mode, rtk_op_mode);

			if (mode != rtk_op_mode) {
				
				#if defined(CONFIG_RTL_83XX_SUPPORT)
				if (rtl83xx_op_mode_set_hook){
					int ret = (*rtl83xx_op_mode_set_hook)(mode);
					printk("ret=%d mode=%d , rtk_op_mode = %d\n", ret, mode, rtk_op_mode);
				}
				else{
					printk("rtl83xx_op_mode_set_hook is NULL!\n");
				}
				#endif
				
				rtk_op_mode = mode;
			}
		}
		else
		{
			goto errout;
		}
	} 
	else {
errout:

		printk("--------help--------\n");
		printk("echo op_mode [mode] >/proc/driver/realtek/rtk_op_mode\n");
		printk("mode: gateway = 0, bridge = 1, wisp = 2, repeater = 3\n");
	}

	return count;
}

static const struct file_operations rtk_op_mode_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= rtk_op_mode_open,
	.read		= seq_read,
	.write		= rtk_op_mode_write,
	.release	= single_release,
};

#if defined(CONFIG_RTL_BRIDGE_VLAN_SPECIAL_SUPPORT)
unsigned int mc_vid_support = 0;

static int rtk_multicast_vid_read(struct seq_file *s, void *v)
{
	seq_printf(s, "mc_vid_support=%d \n", mc_vid_support);
	
	return 0;
}

static int rtk_multicast_vid_open(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_multicast_vid_read, (void *) file);
}

static ssize_t rtk_multicast_vid_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char tmpbuf[64] = {'\0'};
	char *strptr = NULL, *cmd_addr = NULL;
	char *tokptr = NULL;
	struct net_device *dev;
	char dev_name[IFNAMSIZ + 1] = {0};
	unsigned int mc_vid = -1;

	if(count >= 64)
		goto errout;

	if (userbuf && (count > 0) && (count < sizeof(tmpbuf)) && !copy_from_user(tmpbuf, userbuf, count)) {
		tmpbuf[count - 1] = '\0';
		strptr = tmpbuf;
		cmd_addr = strsep(&strptr," ");
		
		if (cmd_addr == NULL)
			goto errout;

		if(!memcmp(cmd_addr, "enable", 7))
		{
			mc_vid_support = 1;
		}
		else if(!memcmp(cmd_addr, "disable", 7))
		{
			mc_vid_support = 0;
		}
		else
		{
			strncpy(dev_name, cmd_addr, IFNAMSIZ);
		
			tokptr = strsep(&strptr," ");
			if (tokptr == NULL)
				goto errout;

			mc_vid = simple_strtol(tokptr, NULL, 10);

			printk("dev_name=%s ,  mc_vid = %d\n", dev_name, mc_vid);

			dev =  __dev_get_by_name(&init_net, dev_name);
			if(dev)
				dev->mc_vid = mc_vid;
			else
				printk("dev %s not found!\n", dev_name);
		}
	} 
	else {
errout:

		printk("--------help--------\n");
		printk("echo [dev_name] [mc_vid] >/proc/driver/realtek/multicast_vid_setting\n");
		printk("dev_name: eth0/eth1.....\n");
	}

	return count;
}

static const struct file_operations rtk_multicast_vid_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= rtk_multicast_vid_open,
	.read		= seq_read,
	.write		= rtk_multicast_vid_write,
	.release	= single_release,
};
#endif

/***************************************************************************
 *  purpose: read/write Realtek SoC embedded phy (8198F/8198D/...)
 *           read/write external phy (8211F/8226/...)
 ***************************************************************************/

static int rtk_phy_read(struct seq_file *s, void *v)
{
	return 0;
}

static int rtk_phy_open(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_phy_read, (void *) file);
}

static ssize_t rtk_phy_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char tmpbuf[64] = {'\0'};
	char *strptr = NULL, *cmd_addr = NULL;
#ifdef CONFIG_RTL_8226_SUPPORT
	char *tokptr = NULL;
	unsigned short phyid, mmd, reg, data;
#endif

	if(count >= 64)
		goto errout;

	if (userbuf && (count > 0) && (count < sizeof(tmpbuf)) && !copy_from_user(tmpbuf, userbuf, count)) {

		tmpbuf[count - 1] = '\0';
		strptr = tmpbuf;
		cmd_addr = strsep(&strptr," ");
		
		if (cmd_addr == NULL)
			goto errout;

		if (!memcmp(cmd_addr, "help", 4)) {			
			goto errout;
		}
#ifdef CONFIG_RTL_8226_SUPPORT
		else if (!memcmp(cmd_addr, "8226indr", 8)) 
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			phyid=(u16)simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mmd=(u16)simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			reg=(u16)simple_strtol(tokptr, NULL, 16);

			if (aal_mdio_read_indirect_hook) {
				aal_mdio_read_indirect_hook(0, phyid, mmd, reg, &data);
				printk("8226 indirect read: phyid(%d), mmd(%d), reg(0x%x), read back data:0x%x\n", phyid, mmd, reg, data);
			}
		}
		else if (!memcmp(cmd_addr, "8226indw", 8)) 
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			phyid=(u16)simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mmd=(u16)simple_strtol(tokptr, NULL, 0);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			reg=(u16)simple_strtol(tokptr, NULL, 16);

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			data=(u16)simple_strtol(tokptr, NULL, 16);

			if (aal_mdio_write_indirect_hook) {
				aal_mdio_write_indirect_hook(0, phyid, mmd, reg, data);
				printk("8226 indirect write: phyid(%d), mmd(%d), reg(0x%x), write data:0x%x\n", phyid, mmd, reg, data);
			}
		}
#endif		
		else
		{
			goto errout;
		}
	} 
	else {
errout:

		printk("--------help--------\n");
#ifdef CONFIG_RTL_8226_SUPPORT
		printk("echo 8226indr phy_id mmd_id reg_id > phy\n");
		printk("echo 8226indw phy_id mmd_id reg_id data > phy\n");
#endif
	}

	return count;
}

static const struct file_operations rtk_phy_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= rtk_phy_open,
	.read		= seq_read,
	.write		= rtk_phy_write,
	.release	= single_release,
};

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)&&defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
static int rtk_block_iptv_read(struct seq_file *s, void *v)
{
	if(block_iptv_from_lanWlan_server == 1)
		seq_printf(s, "enable\n");
	else
		seq_printf(s, "disable\n");
	seq_printf(s, "ifname: %s\n", block_lan_query_ifname);
	
	return 0;
}

static int rtk_block_iptv_open(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_block_iptv_read, (void *) file);
}

static ssize_t rtk_block_iptv_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char tmpbuf[64] = {'\0'};
	char *strptr = NULL, *cmd_addr = NULL;
	char *tokptr = NULL;

	if(count >= 64)
		goto errout;

	if (userbuf && (count > 0) && (count < sizeof(tmpbuf)) && !copy_from_user(tmpbuf, userbuf, count)) {

		tmpbuf[count - 1] = '\0';
		strptr = tmpbuf;
		cmd_addr = strsep(&strptr," ");
		
		if (cmd_addr == NULL)
			goto errout;

		if (!memcmp(cmd_addr, "enable", 6)) {
			
			block_iptv_from_lanWlan_server =1;
		}
		else if(!memcmp(cmd_addr, "disable", 7))
		{
			block_iptv_from_lanWlan_server = 0;
			memset(block_lan_query_ifname, 0, sizeof(block_lan_query_ifname));
		}
		else if(!memcmp(cmd_addr, "ifname", 6))
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			memcpy(block_lan_query_ifname, tokptr, sizeof(block_lan_query_ifname) - 1);
		}
		else
		{
			goto errout;
		}
	} 
	else {
errout:
		printk( "--------help--------\n");
		printk("echo enable or disable >/proc/driver/realtek/drop_iptv_from_lan_server\n");
		printk("enable: drop igmp query from lan. disable: not drop igmp query from lan.\n");
		printk("echo ifname xxx >/proc/driver/realtek/drop_iptv_from_lan_server\n");
	}

	return count;
}

static const struct file_operations rtk_block_iptv_from_lanWlan_server_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= rtk_block_iptv_open,
	.read		= seq_read,
	.write		= rtk_block_iptv_write,
	.release	= single_release,
};

#endif 

int rtk_get_op_mode(void)
{
	return rtk_op_mode;
}

int rtk_common_utility_proc_init(void)
{
	rtk_proc_dir = proc_mkdir("driver/realtek", NULL);

	if (proc_create("rtk_op_mode", 0, rtk_proc_dir, &rtk_op_mode_proc_fops) == NULL) {
		pr_err("Fail to create rtk_op_mode proc entry.\n");
		return -ENOMEM;
	}
	
	if (proc_create("phy", 0, rtk_proc_dir, &rtk_phy_proc_fops) == NULL) {
		pr_err("Fail to create phy proc entry.\n");
		return -ENOMEM;
	}

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)&&defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
	if (proc_create("drop_iptv_from_lan_server", 0, rtk_proc_dir, &rtk_block_iptv_from_lanWlan_server_proc_fops) == NULL) {
		pr_err("Fail to create phy proc entry.\n");
		return -ENOMEM;
	}
#endif

#if defined(CONFIG_RTL_BRIDGE_VLAN_SPECIAL_SUPPORT)
	if (proc_create("multicast_vid_setting", 0, rtk_proc_dir, &rtk_multicast_vid_proc_fops) == NULL) {
		pr_err("Fail to create multicast_vid_setting proc entry.\n");
		return -ENOMEM;
	}
#endif

	return 0;
}

void rtk_common_utility_proc_exit(void)
{
	remove_proc_entry("rtk_op_mode", rtk_proc_dir);
	remove_proc_entry("phy", rtk_proc_dir);
#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)&&defined(CONFIG_BOLOCK_IPTV_FROM_LAN_SERVER)
	remove_proc_entry("drop_iptv_from_lan_server", rtk_proc_dir);
#endif
}

static int __init rtk_common_utility_init(void)
{
	
	printk("%s %d common utility init! \n", __func__, __LINE__);
	rtk_common_utility_proc_init();

#ifdef CONFIG_RTK_DNS_TRAP
	rtk_dns_trap_init();
#endif

#ifdef CONFIG_RTK_CTCAPD_FILTER_SUPPORT
	rtk_dns_filter_init();
#endif

	return 0;
}

static void __exit rtk_common_utility_exit(void)
{

	printk("%s %d common utility exit! \n", __func__, __LINE__);
	rtk_common_utility_proc_exit();
	
#ifdef CONFIG_RTK_DNS_TRAP
	rtk_dns_trap_exit();
#endif	

#ifdef CONFIG_RTK_CTCAPD_FILTER_SUPPORT
	rtk_dns_filter_exit();
#endif

	return;
}

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
static int is_rtk_nic_dev(struct net_device *dev)
{
	if((dev->priv_flags&IFF_DOMAIN_ELAN) && netdev_priv(dev) 
#if defined(CONFIG_RTL_MULTI_ETH_WAN)
		&& !(dev->priv_flags&IFF_VSMUX)
#endif
	){
		return 1;
	}
#if defined(CONFIG_RTK_DEV_AP)
	else if((dev->priv_flags&IFF_DOMAIN_WAN) && netdev_priv(dev)
#if defined(CONFIG_RTL_MULTI_ETH_WAN)
			&& !(dev->priv_flags&IFF_VSMUX)
#endif
	){
		return 1;
	}
#endif
	return 0;
}
#endif

#if defined(CONFIG_RTL_MULTI_ETH_WAN)
static int is_rtk_wan_dev(struct net_device *dev)
{
	if(((dev->priv_flags&IFF_DOMAIN_WAN) || strstr(dev->name, "ppp"))
		&& (dev->flags & IFF_UP) && netif_carrier_ok(dev)
	){
		return 1;
	}

	return 0;
}
#endif

int rtk_dev_update_stats(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
#ifdef CONFIG_COMMON_RT_API
 	rt_stat_port_cntr_t pPortCntrs;
#endif

#ifdef CONFIG_RTL_MULTI_LAN_DEV
	int phyPortId = -1;

	if(is_rtk_nic_dev(dev))
	{
#ifdef CONFIG_RTL8686NIC
		unsigned int i, txPortMask = 0;
		txPortMask = ((struct re_dev_private*)netdev_priv(dev))->txPortMask;
		for(i=0 ; i<10 ; i++)
		{
			if( txPortMask & (1<<i)){
				phyPortId = i;
				break;
			}
		}
#elif defined(CONFIG_LUNA_G3_SERIES)
		phyPortId = ((struct ca_eth_private*)netdev_priv(dev))->port_cfg.tx_ldpid;
#endif 
		if(phyPortId != -1)
		{
#ifdef CONFIG_COMMON_RT_API
			if (rt_stat_port_getAll(phyPortId, &pPortCntrs) == RT_ERR_OK)
			{
				//memset(stats, 0, sizeof(*stats));
				stats->rx_bytes = (unsigned long long) pPortCntrs.ifInOctets;
				stats->rx_packets = (unsigned long long) (pPortCntrs.ifInUcastPkts+pPortCntrs.ifInMulticastPkts+pPortCntrs.ifInBroadcastPkts);
				stats->rx_errors += (unsigned long long) (pPortCntrs.dot3StatsAligmentErrors+pPortCntrs.dot3StatsFCSErrors+pPortCntrs.dot3StatsSymbolErrors+pPortCntrs.etherStatsCRCAlignErrors);
				stats->rx_dropped += (unsigned long long) (pPortCntrs.ifInDiscards);
				stats->rx_length_errors += (unsigned long long) (pPortCntrs.etherStatsRxUndersizePkts+pPortCntrs.etherStatsRxOversizePkts+pPortCntrs.etherStatsCRCAlignErrors);
				stats->multicast = (unsigned long long) pPortCntrs.ifInMulticastPkts;
				stats->tx_bytes = (unsigned long long) pPortCntrs.ifOutOctets;
				stats->tx_packets = (unsigned long long) (pPortCntrs.ifOutUcastPkts+pPortCntrs.ifOutMulticastPkts+pPortCntrs.ifOutBrocastPkts);
				stats->tx_errors += (unsigned long long) pPortCntrs.etherStatsTxCRCAlignErrors;
				stats->tx_dropped += (unsigned long long) pPortCntrs.ifOutDiscards;
				stats->collisions += (unsigned long long) (pPortCntrs.dot3StatsSingleCollisionFrames+pPortCntrs.dot3StatsMultipleCollisionFrames);
				return 0;
			}
#endif
		}
		return -1;
	}
#endif
#ifdef CONFIG_RTL_MULTI_ETH_WAN
	if(is_rtk_wan_dev(dev))
	{
#ifdef CONFIG_COMMON_RT_API
		rt_stat_netif_mib_t netifMibCnt = {0};
		if(rt_stat_netifMib_get(dev->name, &netifMibCnt) == RT_ERR_OK)
		{
			//memset(stats, 0, sizeof(*stats));
			stats->rx_bytes += (unsigned long long) netifMibCnt.ingress_byte_count;
			stats->rx_packets += (unsigned long long) netifMibCnt.ingress_packet_count;
			stats->tx_bytes += (unsigned long long) netifMibCnt.egress_byte_count;
			stats->tx_packets += (unsigned long long) netifMibCnt.egress_packet_count;
			return 0;
		}
		else
			return -1;
#endif
	}
#endif
	return -1;
}
#endif


module_init(rtk_common_utility_init);
module_exit(rtk_common_utility_exit);

EXPORT_SYMBOL(rtk_proc_dir);
EXPORT_SYMBOL(rtl83xx_op_mode_set_hook);
EXPORT_SYMBOL(rtk_get_op_mode);
EXPORT_SYMBOL(aal_mdio_read_indirect_hook);
EXPORT_SYMBOL(aal_mdio_write_indirect_hook);

