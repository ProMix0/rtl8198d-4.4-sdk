/***************************************************************************
 * File Name    : smux.c
 * Description  : smux mean server mux.
 ***************************************************************************/
#include <asm/uaccess.h>
#include <linux/capability.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/in.h>
#include <linux/init.h>
#include <linux/rtnetlink.h>
#include <linux/notifier.h>
//#include <linux/if_smux.h>
#include "if_smux.h"
#ifdef CONFIG_IP_MROUTE
#include <linux/inetdevice.h>
#endif
#include <linux/if_vlan.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/proc_fs.h>
#if defined(CONFIG_RTK_DEV_AP)
#include <soc/cortina/rtk_multi_wan_vlan.h>
#endif

#ifdef CONFIG_PORT_MIRROR
extern void nic_tx_mirror (struct sk_buff *skb);
static inline void smux_mirror_pkt(struct sk_buff *skb, 
			const struct smux_dev_info *dev_info, const int flag);

#define IN  0x1
#define OUT 0x2
#endif

#ifdef DEBUG
#define DPRINTK(format, args...) printk(KERN_DEBUG "SMUX: " format, ##args)
#else
#define DPRINTK(format, args...)
#endif

//extern unsigned int pvid_per_port[RTL8651_PORT_NUMBER+3];

//#define UNIQUE_MAC_PER_DEV
#undef UNIQUE_MAC_PER_DEV

/***************************************************************************
                         Global variables 
 ***************************************************************************/
#define SET_MODULE_OWNER(dev) do { } while (0)

static DEFINE_RWLOCK(smux_lock);

static LIST_HEAD(smux_grp_devs);


enum  {
	ETH_MODE=0,
	GPON_MODE,
	EPON_MODE,
	FIBER_MODE
} WAN_MODE_T;
static int WAN_MODE = ETH_MODE;

static int smux_device_event(struct notifier_block *, unsigned long, void *);

static struct notifier_block smux_notifier_block = {
	.notifier_call = smux_device_event,
};

#ifdef UNIQUE_MAC_PER_DEV
unsigned char wan_dev_def_vid[9]={[0 ... 8]=0};//index 0 is reserved


/*
 * return value: -1 : FAIL
 */
int allocSmuxDevVid(void)
{
	int i;
	
	for (i=1; i<9; i++)
	{
		if (!wan_dev_def_vid[i])
			break;
	}
	if (i<9)
		return i;

	return -1;
}

int freeSmuxDevVid(int vid)
{
	if ((vid >= 9) || (vid <= 0))
		return -1;
	
	wan_dev_def_vid[vid] = 0;

	return 0;
}
#endif

/***************************************************************************
                         Function Definisions
 ***************************************************************************/

static int smux_ioctl_handler(void __user *);

static inline struct smux_group *list_entry_smuxgrp(const struct list_head *le)
{
	return list_entry(le, struct smux_group, smux_grp_devs);
}

/***************************************************************************
 * Function Name: __find_smux_group
 * Description  : returns the smux group of interfaces/devices from list
 * Returns      : struct smux_group.
 ***************************************************************************/
static struct smux_group *__find_smux_group(const char *ifname)
{
	struct list_head *lh;
	struct smux_group *smux_grp;
	struct smux_group *ret_smux = NULL;

	read_lock(&smux_lock);
	list_for_each(lh, &smux_grp_devs) {
		smux_grp = (struct smux_group *)list_entry_smuxgrp(lh);
		if (!strncmp(smux_grp->real_dev->name, ifname, IFNAMSIZ)) {
			ret_smux = smux_grp;
			break;
		}
	}
	read_unlock(&smux_lock);

	return ret_smux;
} /* __find_smux_group */

static inline struct smux_dev_info *list_entry_smuxdev(const struct list_head *le)
{
  return list_entry(le, struct smux_dev_info, list);
}

/***************************************************************************
 * Function Name: __find_smux_in_smux_group
 * Description  : returns the smux device from smux group of devices 
 * Returns      : struct net_device
 ***************************************************************************/
static struct net_device *__find_smux_in_smux_group(
                                     struct smux_group *smux_grp, 
                                     const char *ifname)
{
	struct list_head *lh;
	struct smux_dev_info * sdev = NULL;
	struct net_device    * ret_dev = NULL;

	read_lock(&smux_lock);
	list_for_each(lh, &smux_grp->virtual_devs) {
		sdev = list_entry_smuxdev(lh);
		if(!strncmp(sdev->vdev->name, ifname, IFNAMSIZ)) {
			ret_dev = sdev->vdev;
			break;
		}
	}
	read_unlock(&smux_lock);

	return ret_dev;
} /* __find_smux_in_smux_group */

/***************************************************************************
 * Function Name: smux_pkt_recv
 * Description  : packet recv routine for all smux devices from real dev.
 * Returns      : 0 on Success
 ***************************************************************************/
int smux_pkt_recv(struct sk_buff *skb, struct net_device *dev)
{
	struct smux_group *grp;
	unsigned char *dstAddr;
	struct sk_buff *skb2;
	struct smux_dev_info *dev_info;
	struct smux_dev_info *dev_info_first;
	struct list_head *lh;
	struct net_device *vdev;
	//struct vlan_hdr *vhdr=NULL;
	unsigned short protocol;
	int isTxDone = 0;
	int isRemoveTagged=0;
	int ret = 0;


	if(!dev) {
		dev_kfree_skb(skb);
//printk("%s %d null dev\n", __func__, __LINE__);
		return 1;
	}

	grp = __find_smux_group(dev->name);
	if(!grp) {
		dev_kfree_skb(skb);
//printk("%s %d null grp dev->name:%s\n", __func__, __LINE__, dev->name);
		return 1;
	}
//printk("%s %d dev->name:%s\n", __func__, __LINE__, dev->name);

	dstAddr = eth_hdr(skb)->h_dest;

	//if (protocol ==  __constant_htons(ETH_P_8021Q)) {
	//	vhdr = (struct vlan_hdr *)(skb->data);
	//	protocol = vhdr->h_vlan_encapsulated_proto;
	//}

	//printk("%s %d enter=================>\n", __func__, __LINE__, skb->protocol);
	read_lock(&smux_lock);

	if (skb->protocol == htons(ETH_P_8021Q)) {
		struct vlan_hdr *vhdr = (struct vlan_hdr *)skb->data;
		u16 vlan_tci = ntohs(vhdr->h_vlan_TCI);
		//printk("%s %d enter=================>smux has CTAG(%d)!!\n", __func__, __LINE__, vlan_tci);
		skb->vlan_tci =(vlan_tci & VLAN_VID_MASK)+1;
		skb->mark =((vlan_tci >> 13)& 0x7)+1;						  
		skb->protocol=vhdr->h_vlan_encapsulated_proto;
		//printk("%s %d after remving ctag=================>the protocol is %x\n", __func__, __LINE__, skb->protocol);
		
		skb_pull_rcsum(skb, VLAN_HLEN);
		skb_reset_network_header(skb);
		isRemoveTagged=1;
		
		#if defined(CONFIG_RTK_DEV_AP)
		if(isRemoveTagged)
		{
			memmove(skb->data - ETH_HLEN, skb->data - VLAN_ETH_HLEN, 12);
			skb->mac_header += VLAN_HLEN;
			dstAddr = eth_hdr(skb)->h_dest;
		}
		#endif
		//printk("%s %d after removing CTAG:skb->data=%x\n", __func__, __LINE__,*(unsigned int *)skb->data);
	}
	protocol=skb->protocol;
	
	/* Multicast Traffic will go on all intf.*/
	if (dstAddr[0] & 1)
	{
//printk("%s %d \n", __func__, __LINE__);

		dev_info_first = NULL;
		/* multicast or broadcast frames */
		list_for_each(lh, &grp->virtual_devs)
		{
			dev_info = list_entry_smuxdev(lh);
			vdev = dev_info->vdev;
			
			if (((skb->vlan_tci&VLAN_VID_MASK) && !(vdev->priv_flags&IFF_VSMUX)) ||
				((vdev->priv_flags&IFF_VSMUX) && 
				 (!(skb->vlan_tci&VLAN_VID_MASK) || ((((skb->vlan_tci&VLAN_VID_MASK)-1)!=dev_info->vid) && (((skb->vlan_tci&VLAN_VID_MASK)-1)!=dev_info->mvid)))))
	  			continue;
#if 0 
			if (((dev_info->proto == SMUX_PROTO_PPPOE) && (protocol != htons(ETH_P_PPP_DISC)) && (protocol != htons(ETH_P_PPP_SES))) ||
				((dev_info->proto == SMUX_PROTO_IPOE) && ((protocol == htons(ETH_P_PPP_DISC)) || (protocol == htons(ETH_P_PPP_SES)))))
			{
				DPRINTK("TRACE %d: packet dropped on RX dev %s\n", __LINE__, vdev->name);
				continue;
			}
#endif		
			if(!dev_info_first) {
				dev_info_first = dev_info;
				continue;
			}
	
#ifdef	CONFIG_PORT_MIRROR
			if(IN_NEED_MIR(dev_info->port_mirror))
			{
				smux_mirror_pkt(skb, dev_info, IN);
			}
#endif
			skb2 = skb_clone(skb, GFP_ATOMIC);
			dev_info->stats.rx_packets++;
			dev_info->stats.rx_bytes += skb2->len;
			skb2->dev = vdev;
			skb2->from_dev = vdev;
			#if !defined(CONFIG_RTK_DEV_AP)
			if ((dev_info->proto == SMUX_PROTO_BRIDGE)&&(isRemoveTagged==1))
			{
				skb2->protocol = htons(ETH_P_8021Q);
				skb_push(skb2, VLAN_HLEN);
				skb_reset_network_header(skb2);
			}
			#else
			skb2->vlan_tci=0;
			#endif
			
			//skb2->pkt_type = PACKET_HOST;			
			netif_rx(skb2);
		}

		if (!dev_info_first) {
			dev_kfree_skb(skb);
			read_unlock(&smux_lock);
			return 1;
		}
		else {
			dev_info_first->stats.rx_packets++;
			dev_info_first->stats.rx_bytes += skb->len; 
			skb->dev = dev_info_first->vdev;
			skb->from_dev = dev_info_first->vdev;
			//skb->pkt_type = PACKET_HOST;
			
#ifdef	CONFIG_PORT_MIRROR
			if(IN_NEED_MIR(dev_info_first->port_mirror))
			{
				smux_mirror_pkt(skb, dev_info_first, IN);
			}
#endif			

			#if !defined(CONFIG_RTK_DEV_AP)
			if ((dev_info_first->proto == SMUX_PROTO_BRIDGE)&&(isRemoveTagged==1))
			{
				skb->protocol = htons(ETH_P_8021Q);
				skb_push(skb, VLAN_HLEN);
				skb_reset_network_header(skb);
			}
			#else
			skb->vlan_tci=0;
			#endif

			ret = netif_rx(skb);
		}	
		isTxDone = 1;		
	}
	else /* route Traffic.*/
	{
//printk("%s %d \n", __func__, __LINE__);	
		#ifndef UNIQUE_MAC_PER_DEV
		dev_info_first = NULL;
		#endif
		/* Routing Interface Traffic : check dst mac */
		list_for_each(lh, &grp->virtual_devs)
		{
			dev_info = list_entry_smuxdev(lh);
			if (dev_info->proto == SMUX_PROTO_BRIDGE)
				continue;
			
			vdev = dev_info->vdev;
			if (((skb->vlan_tci&VLAN_VID_MASK) && !(vdev->priv_flags&IFF_VSMUX)) ||
				((vdev->priv_flags&IFF_VSMUX) && (!(skb->vlan_tci&VLAN_VID_MASK) || (((skb->vlan_tci&VLAN_VID_MASK)-1)!=dev_info->vid))))
				continue;
#if 0 
			if (((dev_info->proto == SMUX_PROTO_PPPOE) && (protocol != htons(ETH_P_PPP_DISC)) && (protocol != htons(ETH_P_PPP_SES))) ||
				((dev_info->proto == SMUX_PROTO_IPOE) && ((protocol == htons(ETH_P_PPP_DISC)) || (protocol == htons(ETH_P_PPP_SES)))))
			{
				DPRINTK("TRACE %d: packet dropped on RX dev %s\n", __LINE__, vdev->name);
				continue;
			}
#endif

			#ifndef UNIQUE_MAC_PER_DEV
			if (!memcmp(dstAddr, vdev->dev_addr, ETH_ALEN))
			{
				if(!dev_info_first) {
					dev_info_first = dev_info;
					continue;
				}
				
				skb2 = skb_copy(skb, GFP_ATOMIC);
				skb2->dev = vdev;
				skb2->from_dev = vdev;
				dev_info->stats.rx_packets++;
				dev_info->stats.rx_bytes += skb2->len;
				skb2->pkt_type = PACKET_HOST;
				//printk("(route) receive from %s port_mirror %x\n", vdev->name, dev_info->port_mirror);
#ifdef	CONFIG_PORT_MIRROR
				if(IN_NEED_MIR(dev_info->port_mirror))
				{
					smux_mirror_pkt(skb, dev_info, IN);
				}
#endif			
				#if defined(CONFIG_RTK_DEV_AP)		
				skb2->vlan_tci=0;
				#endif
				netif_rx(skb2);

				isTxDone = 1;
			}
			#else
			if (!memcmp(dstAddr, vdev->dev_addr, ETH_ALEN))
			{
				skb->dev = vdev;
				skb->from_dev = vdev;
				dev_info->stats.rx_packets++;
				dev_info->stats.rx_bytes += skb->len;
				skb->pkt_type = PACKET_HOST;
				//printk("(route) receive from %s port_mirror %x\n", vdev->name, dev_info->port_mirror);

#ifdef	CONFIG_PORT_MIRROR
				if(IN_NEED_MIR(dev_info->port_mirror))
				{
					smux_mirror_pkt(skb, dev_info, IN);
				}
#endif			
				#if defined(CONFIG_RTK_DEV_AP)	
				skb->vlan_tci=0;
				#endif
				ret = netif_rx(skb);

				isTxDone = 1;
				break;
			}
			#endif
		}

		#ifndef UNIQUE_MAC_PER_DEV
		if (dev_info_first) {
			dev_info_first->stats.rx_packets++;
			dev_info_first->stats.rx_bytes += skb->len; 
			skb->dev = dev_info_first->vdev;
			skb->from_dev = dev_info_first->vdev;
			skb->pkt_type = PACKET_HOST;
			//printk("(route) receive from %s port_mirror %x\n", dev_info_first->vdev->name, dev_info_first->port_mirror);
#ifdef	CONFIG_PORT_MIRROR
			if(IN_NEED_MIR(dev_info_first->port_mirror))
			{
				smux_mirror_pkt(skb, dev_info_first, IN);
			}
#endif	

			//2013/12/16: fixed for bridge WAN to keep original vlan info to PS for vconfig parsing.
			#if !defined(CONFIG_RTK_DEV_AP)	
			if ((dev_info_first->proto == SMUX_PROTO_BRIDGE)&&(isRemoveTagged==1))
			{
				skb->protocol = htons(ETH_P_8021Q);
				skb_push(skb, VLAN_HLEN);
				skb_reset_network_header(skb);
			}
			#else
			skb->vlan_tci=0;
			#endif
			
			ret = netif_rx(skb);
			
			isTxDone = 1;
		}
		#endif
	}

	if(isTxDone != 1) 
	{
		/* Bridging Interface Traffic */
		list_for_each(lh, &grp->virtual_devs)
		{
			dev_info = list_entry_smuxdev(lh);
			if (dev_info->proto != SMUX_PROTO_BRIDGE && !dev_info->brpppoe)
				continue;
			
			vdev = dev_info->vdev;

			if (((skb->vlan_tci&VLAN_VID_MASK) && !(vdev->priv_flags&IFF_VSMUX)) ||
				((vdev->priv_flags&IFF_VSMUX) && (!(skb->vlan_tci&VLAN_VID_MASK) || (((skb->vlan_tci&VLAN_VID_MASK)-1)!=dev_info->vid))))
				continue;
			
			if (vdev->promiscuity)
			{
				skb->dev = vdev;
				skb->from_dev = vdev;
				dev_info->stats.rx_packets++;
				dev_info->stats.rx_bytes += skb->len; 
				skb->pkt_type = PACKET_OTHERHOST;
				//printk("(bridge) receive from %s\n", vdev->name);
#ifdef	CONFIG_PORT_MIRROR
				if(IN_NEED_MIR(dev_info->port_mirror))
				{
					smux_mirror_pkt(skb, dev_info, IN);
				}
#endif

				//2013/12/16: fixed for bridge WAN to keep original vlan info to PS for vconfig parsing.
				#if !defined(CONFIG_RTK_DEV_AP) 
				if(isRemoveTagged==1)
				{
					skb->protocol = htons(ETH_P_8021Q);
					skb_push(skb, VLAN_HLEN);
					skb_reset_network_header(skb);
				}
				#else
				skb->vlan_tci = 0;
				#endif
				ret = netif_rx(skb);
				isTxDone = 1;
				break;
			}
		}
	}
	read_unlock(&smux_lock);

	//printk("=================>%s %d exit.\n", __func__, __LINE__);
	if(!isTxDone) {
		DPRINTK("dropping packet that has wrong dest. on RX dev %s\n", dev->name);
		dev_kfree_skb(skb);
		return 1;
	}

	return ret;
} /* smux_pkt_recv */

/***************************************************************************
 * Function Name: smux_dev_hard_start_xmit
 * Description  : xmit routine for all smux devices on real dev.
 * Returns      : 0 on Success
 ***************************************************************************/
int smux_dev_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct net_device_stats *stats = smux_dev_get_stats(dev);
	struct smux_dev_info *dev_info;

	stats->tx_packets++; 
	stats->tx_bytes += skb->len;

	dev_info = SMUX_DEV_INFO(dev);
	skb->dev = dev_info->smux_grp->real_dev;
	if (-1 == dev_info->vid)
	{
		skb->vlan_tci = 0;
	}
	else
	{	
		#if !defined(CONFIG_RTK_DEV_AP)
		if(dev_info->proto != SMUX_PROTO_BRIDGE)
		#endif
		{
			//PATCH20131216:for fwdEngine, we need smux to remarking ctag directly, not put in skb->vlan_tci
			//if(vlan_put_tag(skb,(dev_info->vid&VLAN_VID_MASK))==NULL)
			if(vlan_insert_tag_set_proto(skb,htons(ETH_P_8021Q), (dev_info->vid&VLAN_VID_MASK)) == NULL)
			{
				printk("[%s]error when add cvlan tag\n",__FUNCTION__);
				return 0;
			}

			if(dev_info->m_1p)
			{
				if (skb->dev->features & NETIF_F_HW_VLAN_CTAG_TX){
					if (skb->dev->features & NETIF_F_HW_VLAN_CTAG_TX_BIT)
						skb->vlan_tci |= ntohs(((dev_info->m_1p-1)<<13));
				}
				else
				{
					struct vlan_ethhdr *veth = (struct vlan_ethhdr *)(skb->data);
					veth->h_vlan_TCI |=ntohs(((dev_info->m_1p-1)<<13));
				}
			}
		}
	}	
	//skb->vlan_member = dev_info->member;
#ifdef	CONFIG_PORT_MIRROR
	if((OUT_NEED_MIR(dev_info->port_mirror)))
	{
		smux_mirror_pkt(skb, dev_info, OUT);
	}
#endif
	//printk("%s,%d::dev_info->member: %x\n",__func__,__LINE__,skb->vlan_member);

#ifdef CONFIG_RTL_SMUX_TXMARK_STREAMID
	if (dev_info->mark > 0 || dev_info->mask > 0)
	{
		skb->mark &= ~dev_info->mask;
		skb->mark |= dev_info->mark;
	}
#endif

	skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev);

	//dev_queue_xmit(skb);

	return 0;
} /* smux_dev_hard_start_xmit */

/***************************************************************************
 * Function Name: smux_dev_open
 * Description  : 
 * Returns      : 0 on Success
 ***************************************************************************/
int smux_dev_open(struct net_device *vdev)
{
	if (!(SMUX_DEV_INFO(vdev)->smux_grp->real_dev->flags & IFF_UP))
		return -ENETDOWN;

	return 0;
} /* smux_dev_open */

/***************************************************************************
 * Function Name: smux_dev_stop
 * Description  : 
 * Returns      : 0 on Success
 ***************************************************************************/
int smux_dev_stop(struct net_device *dev)
{
	return 0;
} /* smux_dev_stop */

/***************************************************************************
 * Function Name: smux_dev_set_mac_address
 * Description  : sets the mac for devs
 * Returns      : 0 on Success
 ***************************************************************************/
int smux_dev_set_mac_address(struct net_device *dev, 
                             void *addr_struct_p)
{
	struct sockaddr *addr = (struct sockaddr *)(addr_struct_p);
	int i, flgs;
#ifdef UNIQUE_MAC_PER_DEV
	struct smux_group *grp = NULL;
	struct smux_dev_info *vdev_info = NULL;
	struct list_head *lh;
#endif
	struct smux_dev_info *sdev_info = NULL;

	if (netif_running(dev))
		return -EBUSY;

	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);

	//copy dev addr to smux info for omci module reading
	sdev_info = SMUX_DEV_INFO(dev);
	memcpy(sdev_info->dev_addr, dev->dev_addr, ETH_ALEN);
	//printk("%s %d: %02x%02x%02x%02x%02x%02x\n", __FUNCTION__, __LINE__, sdev_info->dev_addr[0],sdev_info->dev_addr[1],sdev_info->dev_addr[2],
	//	sdev_info->dev_addr[3],sdev_info->dev_addr[4],sdev_info->dev_addr[5]);
	
	memset(dev->broadcast, 0xff, ETH_ALEN);

	return 0;
	
#ifdef UNIQUE_MAC_PER_DEV

grp = __find_smux_group(ALIASNAME_NAS0);
//grp = __find_smux_group("nas0");
	if (!grp)
		return -EADDRNOTAVAIL;
	if (list_empty(&grp->virtual_devs))
	{
		return -EADDRNOTAVAIL;
	}
	list_for_each(lh, &grp->virtual_devs)
	{
		vdev_info = list_entry_smuxdev(lh);
		if (vdev_info->vdev == dev)
			continue;
		
		if (!memcmp(vdev_info->vdev->dev_addr, addr->sa_data, ETH_ALEN))
		{
        	return -EADDRNOTAVAIL;
		}
	}
#endif

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	printk("%s: Setting MAC address to ", dev->name);
	for (i = 0; i < 6; i++)
		printk(" %2.2x", dev->dev_addr[i]);
	printk(".\n");

	if (memcmp(SMUX_DEV_INFO(dev)->smux_grp->real_dev->dev_addr, dev->dev_addr, dev->addr_len) != 0) {
		if (!(SMUX_DEV_INFO(dev)->smux_grp->real_dev->flags & IFF_PROMISC)) {
			flgs = SMUX_DEV_INFO(dev)->smux_grp->real_dev->flags;

			/* Increment our in-use promiscuity counter */
			dev_set_promiscuity(SMUX_DEV_INFO(dev)->smux_grp->real_dev, 1);

			/* Make PROMISC visible to the user. */
			flgs |= IFF_PROMISC;
			printk("SMUX (%s):  Setting underlying device (%s) to promiscious mode.\n",
				dev->name, SMUX_DEV_INFO(dev)->smux_grp->real_dev->name);
			dev_change_flags(SMUX_DEV_INFO(dev)->smux_grp->real_dev, flgs);
		}
	} else {
		printk("SMUX (%s):  Underlying device (%s) has same MAC, not checking promiscious mode.\n",
			dev->name, SMUX_DEV_INFO(dev)->smux_grp->real_dev->name);
	}


	SMUX_DEV_INFO(dev)->smux_grp->real_dev->netdev_ops->ndo_set_mac_address(dev, addr_struct_p);

	return 0;
} /* smux_dev_set_mac_address */


/***************************************************************************
 * Function Name: smux_dev_ioctl
 * Description  : handles device related ioctls
 * Returns      : 0 on Success
 ***************************************************************************/
int smux_dev_ioctl(struct net_device *vdev, struct ifreq *ifr, int cmd)
{
	struct net_device *real_dev = SMUX_DEV_INFO(vdev)->smux_grp->real_dev;
	struct ifreq ifrr;
	int err = -EOPNOTSUPP;

	strncpy(ifrr.ifr_name, real_dev->name, IFNAMSIZ);
	ifrr.ifr_ifru = ifr->ifr_ifru;

	printk("%s %d cmd 0x%x (dev:%s)\n", __func__, __LINE__, cmd,vdev->name);
	switch(cmd) {
		case SIOCGMIIPHY:
		case SIOCGMIIREG:
		case SIOCSMIIREG:
			if (real_dev->netdev_ops->ndo_do_ioctl && netif_device_present(real_dev))
			err = real_dev->netdev_ops->ndo_do_ioctl(real_dev, &ifrr, cmd);
			break;

		case SIOCETHTOOL:
			err = dev_ethtool(&init_net, &ifrr);

			if (!err)
				ifr->ifr_ifru = ifrr.ifr_ifru;
			break;

#ifdef CONFIG_PORT_MIRROR
		case SIOCPORTMIRROR:
		{
			struct portmir *pmr;
			struct smux_dev_info *dev_info = SMUX_DEV_INFO(vdev);
			struct net_device *dev = NULL;

			pmr = (struct portmir *)ifr->ifr_data;
			//AUG_DBG("the pmr->mir_dev_name is %s\n", pmr->mir_dev_name);

			dev = dev_get_by_name(&init_net, pmr->mir_dev_name);
			if(!dev)
			{
				printk("error lan device!\n");
				break;
			}	
			if((dev->priv_flags & IFF_DOMAIN_ELAN) == 0)
			{
				printk("error lan device!\n");
				break;
			}
			printk("mirror pkt %s/%s %s to dev %s\n", 
					(pmr->port_mirror&0x1)?"to":"", (pmr->port_mirror&0x2)?"from":"",
					vdev->name, pmr->mir_dev_name);
			dev_info->port_mirror = pmr->port_mirror;
			dev_info->mirror_dev  = dev;

			err = 0;
			break;
		}
#endif

#if defined(CONFIG_RTK_DEV_AP)
		case SIOCSITFGROUP:
			{
				struct ifvlan ifvl;
				void __user *useraddr = ifr->ifr_data;
				struct smux_dev_info *dev_info = SMUX_DEV_INFO(vdev);

				if (copy_from_user(&ifvl, useraddr, sizeof(ifvl)))
					return -EFAULT;
								
				if (ifvl.enable){
					dev_info->member = ifvl.member;
				}
				else{
					dev_info->member = 0xFFFFFFFF;
				}

				if (rtl_update_multi_wan_port_mapping_hook){
					err = (*rtl_update_multi_wan_port_mapping_hook)(vdev->name, dev_info->vid, ifvl.member, ifvl.enable, dev_info->proto);
					TRACE("vdev->name=%s, dev_info->vid=%d, dev_info->member=0x%x, ifvl->enable=0x%x dev_info->proto=%d\n", vdev->name, dev_info->vid, dev_info->member, ifvl.enable, dev_info->proto);
				}
				else{
					ERROR("rtl_update_multi_wan_port_mapping_hook is NULL!!! vdev->name=%s, dev_info->vid=%d, dev_info->member=0x%x, ifvl->enable=0x%x, dev_info->proto=%d\n", vdev->name, dev_info->vid, dev_info->member, ifvl.enable, dev_info->proto);
				}
				break;
			}
#endif
				

	}

	return err;
} /* smux_dev_ioctl */

/***************************************************************************
 * Function Name: smux_dev_change_mtu
 * Description  : changes mtu for dev
 * Returns      : 0 on Success
 ***************************************************************************/
int smux_dev_change_mtu(struct net_device *vdev, int new_mtu)
{
	//MTU should be larger than real device.
	if (SMUX_DEV_INFO(vdev)->smux_grp->real_dev->mtu < new_mtu)
		return -ERANGE;

	vdev->mtu = new_mtu;
	//SMUX_DEV_INFO(vdev)->smux_grp->real_dev->netdev_ops->ndo_change_mtu(vdev, new_mtu);

	return 0;
}

/***************************************************************************
 * Function Name: smux_setup
 * Description  : inits device api
 * Returns      : None
 ***************************************************************************/
static void smux_setup(struct net_device *new_dev)
{
	//struct  net_device_ops  dev_ops = {.ndo_get_stats = smux_dev_get_stats};
	SET_MODULE_OWNER(new_dev);
        /* Make this thing known as a SMUX device */
	new_dev->priv_flags |= IFF_OSMUX;
	new_dev->tx_queue_len = 0;
	//new_dev->netdev_ops = &dev_ops;
} /* smux_setup */


/***************************************************************************
 * Function Name: smux_transfer_operstate
 * Description  : updates the operstate of overlay device 
 * Returns      : None.
 ***************************************************************************/
static void smux_transfer_operstate(const struct net_device *rdev, 
                                    struct net_device *vdev)
{

	if (rdev->operstate == IF_OPER_DORMANT)
		netif_dormant_on(vdev);
	else
		netif_dormant_off(vdev);
#if (defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE))
	if (netif_carrier_ok(rdev)) {
		//EPON and GPON mode smux carrier by OAM or OMCI
		if(!(WAN_MODE == EPON_MODE || WAN_MODE == GPON_MODE)) 
		{
			if (!netif_carrier_ok(vdev)){
				printk("[SMUX] %s carrier on !!!\n", vdev->name);
				netif_carrier_on(vdev);
			}
		}
	} else {
		if (netif_carrier_ok(vdev)){
			printk("[SMUX] %s carrier off !!!\n", vdev->name);
			netif_carrier_off(vdev);
		}
	}
#else
	if (netif_carrier_ok(rdev)) {
		if (!netif_carrier_ok(vdev))
			netif_carrier_on(vdev);
	} else {
		if (netif_carrier_ok(vdev))
			netif_carrier_off(vdev);
	}
#endif
} /* smux_transfer_operstate */

static const struct ethtool_ops smux_ethtool_ops = {
	.get_link = ethtool_op_get_link,
};

static const struct net_device_ops smux_netdev_ops = {
	.ndo_open		= smux_dev_open,
	.ndo_stop		= smux_dev_stop,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address 	= smux_dev_set_mac_address,
	.ndo_do_ioctl		= smux_dev_ioctl,
	.ndo_start_xmit		= smux_dev_hard_start_xmit,
	.ndo_change_mtu		= smux_dev_change_mtu,
	.ndo_get_stats = smux_dev_get_stats
};

/***************************************************************************
 * Function Name: smux_register_device
 * Description  : regists new overlay device on real device & registers for 
                  packet handlers depending on the protocol types
 * Returns      : 0 on Success
 ***************************************************************************/
static struct net_device *smux_register_device(const char *rifname,
					       const char *nifname, int smux_proto, int vid, int napt, int brpppoe)
{
	struct net_device *new_dev = NULL;
	struct net_device *real_dev = NULL; 
	struct smux_group *grp = NULL;
	struct smux_dev_info *vdev_info = NULL;
	//int    mac_reused = 0;
	//unsigned char LSB=0;
	//struct list_head *lh;

	//printk("%s %d enter\n", __func__, __LINE__);
	real_dev = dev_get_by_name(&init_net, rifname);
	if (!real_dev) {
		goto real_dev_invalid;
	}

	if (!(real_dev->flags & IFF_UP)) {
		goto real_dev_invalid;
	}

	new_dev = alloc_netdev(sizeof(struct smux_dev_info), nifname, NET_NAME_UNKNOWN, smux_setup);
	if (new_dev == NULL)
	{
		printk("netdev alloc failure\n");
		goto new_dev_invalid;
	}

	//dev->netdev_ops = &rtl819x_netdev_ops;
	ether_setup(new_dev);
	if (vid != -1)
		new_dev->priv_flags |= IFF_VSMUX;

	new_dev->flags &= ~IFF_UP;
	new_dev->flags &= ~IFF_MULTICAST;
	new_dev->priv_flags |= IFF_DOMAIN_WAN;
	//new_dev->priv_flags |= IFF_DOMAIN_ELAN;
	real_dev->priv_flags |= IFF_RSMUX;

	new_dev->state = (real_dev->state & 
                    ((1<<__LINK_STATE_NOCARRIER) |
                     (1<<__LINK_STATE_DORMANT))) |
                     (1<<__LINK_STATE_PRESENT);

	new_dev->mtu = real_dev->mtu;
	new_dev->type = real_dev->type;
	new_dev->hard_header_len = real_dev->hard_header_len;
	new_dev->netdev_ops = &smux_netdev_ops;
	new_dev->ethtool_ops = &smux_ethtool_ops;

	/* find smux group name. if not found create all new smux group */
	grp = __find_smux_group(rifname);
	if (!grp) {
		grp = kzalloc(sizeof(struct smux_group), GFP_KERNEL);

		if(grp) {
			INIT_LIST_HEAD(&grp->virtual_devs);
			INIT_LIST_HEAD(&grp->smux_grp_devs);

			grp->real_dev = real_dev;

			write_lock_irq(&smux_lock);
			list_add_tail(&grp->smux_grp_devs, &smux_grp_devs);
			write_unlock_irq(&smux_lock);
		}
		else {
			free_netdev(new_dev);
			new_dev = NULL;
		}
	}

	if(grp && new_dev) {
		#if 0
		/* Assign default mac to bridge so that we can add it to linux bridge */
		if(smux_proto == SMUX_PROTO_BRIDGE) 
		{
			memcpy( new_dev->dev_addr, "\xFE\xFF\xFF\xFF\xFF\xFF", ETH_ALEN );
		}
		else 
		{
		#ifdef UNIQUE_MAC_PER_DEV
			if (list_empty(&grp->virtual_devs))
			{
				memcpy(new_dev->dev_addr, real_dev->dev_addr, ETH_ALEN);
			}
			else
			{
				list_for_each(lh, &grp->virtual_devs)
				{
					vdev_info = list_entry_smuxdev(lh);
					if (!memcmp(real_dev->dev_addr, vdev_info->vdev->dev_addr, ETH_ALEN))
					{
		            	mac_reused = 1;
					}
					if (LSB < vdev_info->vdev->dev_addr[5])
						LSB = vdev_info->vdev->dev_addr[5];
				}

				memcpy(new_dev->dev_addr, real_dev->dev_addr, ETH_ALEN);
				if (mac_reused)
				{
					//generate new mac address, real_addr mac addr increased by 1.
					new_dev->dev_addr[5] = LSB+1;
				}
			}
		#else
			memcpy(new_dev->dev_addr, real_dev->dev_addr, ETH_ALEN);
		#endif
		}
		#else
		struct net_device *landev;
		/* FIXME: landev name maybe different with lunayueme */
		const char landev_ifname[16]="eth0.2";
		landev = dev_get_by_name(&init_net, landev_ifname);
		if (landev) {
			memcpy(new_dev->dev_addr, landev->dev_addr, ETH_ALEN);
			dev_put(landev);
		}
		else
			printk("%s %d eth0.2 not created.\n", __func__, __LINE__);
		#endif
	}
  
	if(grp && new_dev) {
		struct net_device *ret_dev;
		/*find new smux in smux group if it does not exit create one*/
		if(NULL == (ret_dev=__find_smux_in_smux_group(grp, nifname))) {
			vdev_info = SMUX_DEV_INFO(new_dev);
			memset(vdev_info, 0, sizeof(struct smux_dev_info));
			//m_1p : 0~8, 0 is meaning disable
			if(vid>=0)
				vdev_info->m_1p=vid>>13;
			else
				vdev_info->m_1p=0;			
			vdev_info->smux_grp = grp;
			vdev_info->vdev = new_dev;
			vdev_info->proto = smux_proto;
			#ifdef UNIQUE_MAC_PER_DEV
			if ((vid == -1) && (smux_proto != SMUX_PROTO_BRIDGE)) {			
				if ((vid = allocSmuxDevVid()) == -1)
					printk("fatal error, too many wan interface created.\n");
			}
			#endif
			if(vid!=-1)
				vdev_info->vid = (vid&VLAN_VID_MASK);
			else
				vdev_info->vid = vid;
			vdev_info->mvid = -1; //init to invalid VID
			vdev_info->napt = napt;
			vdev_info->brpppoe = brpppoe;
			vdev_info->member = 0xFFFFFFFF;	//init membership to include all interface.
#ifdef CONFIG_PORT_MIRROR
			vdev_info->port_mirror = 0;
			vdev_info->mirror_dev = NULL;
#endif
			INIT_LIST_HEAD(&vdev_info->list);
			write_lock_irq(&smux_lock);
			list_add_tail(&vdev_info->list, &grp->virtual_devs);
			write_unlock_irq(&smux_lock);
			if(smux_proto == SMUX_PROTO_BRIDGE) {
				new_dev->promiscuity = 1;
			}
			else if(smux_proto == SMUX_PROTO_IPOE) {
				new_dev->flags |= IFF_MULTICAST;
			}

			if (register_netdev(new_dev)) {
				printk("register_netdev failed\n");
				list_del(&vdev_info->list);
				free_netdev(new_dev);
				new_dev = NULL;
			}
			else {
				netif_carrier_off(new_dev);
				smux_transfer_operstate(real_dev, new_dev);
			}
			#if defined(CONFIG_RTK_DEV_AP)
			if (new_dev){
				if (rtl_register_multi_wan_hook){
					TRACE("rifname=%s,nifname=%s,vdev_info->vid=%d, vdev_info->napt=%d, vdev_info->brpppoe=%d, vdev_info->member=0x%x\n", rifname,nifname,vdev_info->vid, vdev_info->napt, vdev_info->brpppoe, vdev_info->member);
					(*rtl_register_multi_wan_hook)(rifname, nifname, smux_proto, vdev_info->vid, (vdev_info->m_1p?(vdev_info->m_1p-1):0), napt, brpppoe);
				}
				else{
					ERROR("rtl_register_multi_wan_hook is NULL!!! rifname=%s,nifname=%s,vdev_info->vid=%d, vdev_info->napt=%d, vdev_info->brpppoe=%d, vdev_info->member=0x%x\n", rifname,nifname,vdev_info->vid, vdev_info->napt, vdev_info->brpppoe, vdev_info->member);
				}
			}
			#endif
		}
		else {
			printk("device %s already exist.\n", nifname);
			free_netdev(new_dev);
			new_dev = ret_dev;
		}
	}


	return new_dev;

real_dev_invalid:
new_dev_invalid:

	return NULL;
} /* smux_register_device */

/***************************************************************************
 * Function Name: smux_unregister_device
 * Description  : unregisters the smux devices along with releasing mem.
 * Returns      : 0 on Success
 ***************************************************************************/
static int smux_unregister_device(const char* vifname)
{
	struct net_device *vdev = NULL;
	struct net_device *real_dev = NULL;
	int ret;
	struct smux_dev_info *dev_info;
	ret = -EINVAL;

	vdev = dev_get_by_name(&init_net, vifname);

	if( vdev && (vdev->priv_flags&IFF_OSMUX)) {
		printk("%s remove smux dev %s\n", __func__, vifname);
		/* remove related acl rule */
		#if 0
		#ifdef CONFIG_RTL8676_Dynamic_ACL
		rtl865x_acl_control_delete_all_by_netif(vdev->name);
		#endif
		rtl865x_delNetif(vdev->name);
		#endif

		dev_info = SMUX_DEV_INFO(vdev);
		
		#if defined(CONFIG_RTK_DEV_AP)
		if (rtl_unregister_multi_wan_hook){
			TRACE("vifname=%s,vdev_info->vid=%d, vdev_info->napt=%d, vdev_info->brpppoe=%d, vdev_info->member=0x%x\n", vifname,dev_info->vid, dev_info->napt, dev_info->brpppoe, dev_info->member);
			(*rtl_unregister_multi_wan_hook)(vifname, dev_info->vid);
		}
		else{
			ERROR("rtl_unregister_multi_wan_hook is NULL!! vifname=%s,vdev_info->vid=%d, vdev_info->napt=%d, vdev_info->brpppoe=%d, vdev_info->member=0x%x\n",vifname,dev_info->vid, dev_info->napt, dev_info->brpppoe, dev_info->member);
		}
		#endif
		
		#ifdef UNIQUE_MAC_PER_DEV
		freeSmuxDevVid(dev_info->vid);
		#endif
		real_dev = dev_info->smux_grp->real_dev;

		write_lock_irq(&smux_lock);
		list_del(&dev_info->list);
		write_unlock_irq(&smux_lock);

		if (list_empty(&dev_info->smux_grp->virtual_devs)) {
			write_lock_irq(&smux_lock);
			list_del(&dev_info->smux_grp->smux_grp_devs);
			write_unlock_irq(&smux_lock);

			kfree(dev_info->smux_grp);
			//if kfree smux_grp, clean real_dev->priv_flags to let netif_rx() does not call smux_pkt_recv() to receive packets
			real_dev->priv_flags &= (~IFF_RSMUX);			
		}

		dev_put(vdev);
		unregister_netdev(vdev);

		synchronize_net();
		dev_put(real_dev); 

		ret = 0;
	}

	return ret;
} /* smux_unregister_device */

/***************************************************************************
 * Function Name: smux_device_event
 * Description  : handles real device events to update overlay devs. status
 * Returns      : 0 on Success
 ***************************************************************************/
static int smux_device_event(struct notifier_block *unused, 
                             unsigned long event, 
                             void *ptr)
{
	struct net_device *rdev = netdev_notifier_info_to_dev(ptr);
	struct smux_group *grp = __find_smux_group(rdev->name);
	int flgs;
	struct list_head *lh;
	struct list_head *lhp;
	struct smux_dev_info *dev_info;


	if (!grp)
		goto out;

	switch (event) {
		case NETDEV_CHANGE:

			/* Propagate real device state to overlay devices */
			read_lock(&smux_lock);
			list_for_each(lh, &grp->virtual_devs) {
				dev_info = list_entry_smuxdev(lh);
				if(dev_info) {
					smux_transfer_operstate(rdev, dev_info->vdev);
				}
			}
			read_unlock(&smux_lock);
			break;

		case NETDEV_DOWN:

			/* Put all Overlay devices for this dev in the down state too.*/
			read_lock(&smux_lock);
			list_for_each(lh, &grp->virtual_devs) {
				dev_info = list_entry_smuxdev(lh);
				if(dev_info) {
					flgs = dev_info->vdev->flags;

					if (!(flgs & IFF_UP))
						continue;

					dev_change_flags(dev_info->vdev, flgs & ~IFF_UP);
				}
			}
			read_unlock(&smux_lock);
			break;

		case NETDEV_UP:

			/* Put all Overlay devices for this dev in the up state too.  */
			read_lock(&smux_lock);
			list_for_each(lh, &grp->virtual_devs) {
				dev_info = list_entry_smuxdev(lh);
				if(dev_info) {
					flgs = dev_info->vdev->flags;

					if (flgs & IFF_UP)
						continue;

					dev_change_flags(dev_info->vdev, flgs & IFF_UP);
				}
			}
			read_unlock(&smux_lock);
			break;

		case NETDEV_UNREGISTER:
			
			/* Delete all Overlay devices for this dev. */
			write_lock_irq(&smux_lock);
			list_for_each_safe(lh, lhp, &grp->virtual_devs) {
				dev_info = list_entry_smuxdev(lh);
				if(dev_info) {
					/* delete by l67530 for cpu0 when reboot system. HG551c.2010/12/07 */
					//list_del(&dev_info->list);
					smux_unregister_device(dev_info->vdev->name);
				}
			}
			write_unlock_irq(&smux_lock);
			break;
		}

out:
  return NOTIFY_DONE;
} /* smux_device_event */

int get_smux_device_info(struct smux_args *parg)
{
	struct list_head *lh;
	struct smux_group *grp = NULL;
	struct smux_dev_info * sdev = NULL;
	//int		idx=0;
	//char	proto[10];

	grp = __find_smux_group(parg->args.rsmux_ifname);
	if (NULL == grp) {
		//printk("%s %s smux group not exist!\n", __func__, parg->args.rsmux_ifname);
		return 0;
	}

	read_lock(&smux_lock);
	parg->valid = 0;
	
	list_for_each(lh, &grp->virtual_devs) {
		sdev = list_entry_smuxdev(lh);

		if (strcmp(sdev->vdev->name, parg->args.osmux_ifname))
			continue;
		
		parg->args.proto = sdev->proto;
		parg->args.vid = sdev->vid;
		parg->args.napt = sdev->napt;
		parg->args.mvid = sdev->mvid;
		
		if (sdev->member != 0xFFFFFFFF)
			parg->member = sdev->member;
		else
			parg->member = 0;

		parg->valid = 1;
#ifdef CONFIG_RTL_SMUX_TXMARK_STREAMID
		parg->args.mark = sdev->mark;
		parg->args.mask = sdev->mask;
#endif
		parg->args.carrier = (netif_carrier_ok(sdev->vdev)) ? 1 : 0;
		break;
	}
	read_unlock(&smux_lock);
	
	return 0;
}
EXPORT_SYMBOL(get_smux_device_info);

int setSmuxMvid(char *rifname, char *nifname, int mvid)
{
	struct list_head *lh;
	struct smux_group *grp = NULL;
	struct smux_dev_info * sdev = NULL;

	grp = __find_smux_group(rifname);
	//printk("%s %d rifname=%s, nifname=%s, mvid=%d\n", __FUNCTION__, __LINE__, rifname, nifname, mvid);

	if (NULL == grp) {
		//printk("%s %s smux group not exist!\n", __func__, parg->args.rsmux_ifname);
		return 0;
	}
	read_lock(&smux_lock);
	list_for_each(lh, &grp->virtual_devs) {
		sdev = list_entry_smuxdev(lh);

		if (strcmp(sdev->vdev->name, nifname))
			continue;

		sdev->mvid = mvid;
		//printk("%s %d set mvid = %d success!\n", __FUNCTION__, __LINE__, sdev->mvid);
		break;
	}
	read_unlock(&smux_lock);
}

/***************************************************************************
 * Function Name: set_smux_device_info
 * Description  : ioctl handler for user apps
 * Returns      : 0 on Success
 ***************************************************************************/
int set_smux_device_info(struct smux_args *parg)
{
	struct list_head *lh;
	struct smux_group *grp = NULL;
	struct smux_dev_info * sdev = NULL;
	int err = -EINVAL, all = 0;
	//int		idx=0;
	//char	proto[10];
	
	grp = __find_smux_group(parg->args.rsmux_ifname);
	if (NULL == grp) {
		//printk("%s %s smux group not exist!\n", __func__, parg->args.rsmux_ifname);
		return -EINVAL;
	}
	
	write_lock_irq(&smux_lock);
	all = (!(strcmp(parg->args.osmux_ifname, "*"))) ? 1 : 0;
	
	list_for_each(lh, &grp->virtual_devs) {
		sdev = list_entry_smuxdev(lh);

		if (!all && strcmp(sdev->vdev->name, parg->args.osmux_ifname))
			continue;

		//sdev->vid = parg->args.vid;
		
#ifdef CONFIG_RTL_SMUX_TXMARK_STREAMID
		if(!all) {
			sdev->mark = parg->args.mark;
			sdev->mask = parg->args.mask;
		}
#endif
		if(parg->args.carrier && !netif_carrier_ok(sdev->vdev)){
			if(netif_carrier_ok(grp->real_dev)){
				printk("[SMUX] %s carrier on !!!\n", sdev->vdev->name);
				netif_carrier_on(sdev->vdev);
			}
		}
		else if(!parg->args.carrier && netif_carrier_ok(sdev->vdev)){
			printk("[SMUX] %s carrier off !!!\n", sdev->vdev->name);
			netif_carrier_off(sdev->vdev);
		}

		err = 0;
		
		if(!all) break;
	}
	write_unlock_irq(&smux_lock);

	return err;
}
EXPORT_SYMBOL(set_smux_device_info);

/***************************************************************************
 * Function Name: smux_ioctl_handler
 * Description  : ioctl handler for user apps
 * Returns      : 0 on Success
 ***************************************************************************/
static int smux_ioctl_handler(void __user *arg)
{
	int err = 0;
	struct smux_ioctl_args *pargs;
	struct smux_args data;

	if (copy_from_user(&data, arg, sizeof(struct smux_ioctl_args)))
		return -EFAULT;

	pargs = (struct smux_ioctl_args *)&data;
	pargs->rsmux_ifname[IFNAMSIZ-1] = 0;
	pargs->osmux_ifname[IFNAMSIZ-1] = 0;


	switch (pargs->cmd) {
		case ADD_SMUX_CMD:
			if (!capable(CAP_NET_ADMIN))
				return -EPERM;
			if(smux_register_device(pargs->rsmux_ifname, pargs->osmux_ifname, pargs->proto, pargs->vid, pargs->napt, pargs->brpppoe)) {
				err = 0;
			} else {
				err = -EINVAL;
			}
			break;

		case REM_SMUX_CMD:
			if (!capable(CAP_NET_ADMIN))
				return -EPERM;
			err = smux_unregister_device(pargs->u.ifname);
			break;

		case GET_SMUX_CMD:
			if (!capable(CAP_NET_ADMIN))
				return -EPERM;

			get_smux_device_info(&data);
			if (copy_to_user(arg, &data, sizeof(struct smux_args)))
				return -EFAULT;
			
			break;
			
		case SET_SMUX_CMD:
			if (!capable(CAP_NET_ADMIN))
				return -EPERM;

			if (set_smux_device_info(&data) == 0){
				err = 0;
			} else {
				err = -EINVAL;
			}
			break;
		
		case ADD_SMUX_MVID_CMD:
			if (!capable(CAP_NET_ADMIN))
				return -EPERM;
			printk("%s %d, CMD: ADD_SMUX_MVID_CMD\n", __FUNCTION__, __LINE__);
			printk("ifname = %s, ifname=%s\n", pargs->rsmux_ifname, pargs->osmux_ifname);
			setSmuxMvid(pargs->rsmux_ifname, pargs->osmux_ifname, pargs->mvid);
			break;
			
		default:
			printk("%s: Unknown SMUX CMD: %x \n",
				__FUNCTION__, pargs->cmd);
			return -EINVAL;
	}

	return err;
} /* smux_ioctl_handler */
static int proc_smux_intf_fops_fops_read(struct seq_file *seq, void *v)
{
	struct list_head *lh, *llh;
	struct smux_group *smux_grp = NULL;
	struct smux_dev_info * sdev = NULL;
	char *proto;
	unsigned char *mac;
	//int		idx=0;
	//char	proto[10];

	read_lock(&smux_lock);

	#if defined(CONFIG_RTK_DEV_AP)
	seq_printf(seq, "= %12s %12s %10s  %13s  %-12s  %-s =\n", "Real-If", "Virtual-If","Protocol","VLAN(PRI/ID) ","MACADDR", "PORTMAPPING");
	#else
	seq_printf(seq, "= %12s %12s %10s  %13s  %-12s =\n", "Real-If", "Virtual-If","Protocol","VLAN(PRI/ID) ","MACADDR");
	#endif
	list_for_each(lh, &smux_grp_devs) {
		smux_grp = (struct smux_group *)list_entry_smuxgrp(lh);
		
		list_for_each(llh, &smux_grp->virtual_devs) {
			sdev = list_entry_smuxdev(llh);
			if(sdev->proto == SMUX_PROTO_PPPOE)
				proto = "pppoe";
			else if(sdev->proto == SMUX_PROTO_BRIDGE)
				proto = "bridge";
			else //if(sdev->proto == SMUX_PROTO_IPOE)
				proto = "ipoe";
			
			mac = sdev->vdev->dev_addr;
			
			#if defined(CONFIG_RTK_DEV_AP)
			seq_printf(seq, "  %12s %12s %10s  %8d/%-4d  %02X%02X%02X%02X%02X%02X  0X%X\n", smux_grp->real_dev->name, sdev->vdev->name, proto, sdev->m_1p, sdev->vid
				,mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], sdev->member);
			#else
			seq_printf(seq, "  %12s %12s %10s  %8d/%-4d  %02X%02X%02X%02X%02X%02X\n", smux_grp->real_dev->name, sdev->vdev->name, proto, sdev->m_1p, sdev->vid
				,mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			#endif
		}
		seq_printf(seq, "\n");
	}
	read_unlock(&smux_lock);
	
	return 0;
}
static int proc_smux_intf_fops_fops_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_smux_intf_fops_fops_read, inode->i_private);
}

static const struct file_operations smux_intf_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_smux_intf_fops_fops_open,
        .read           = seq_read,
        .write          = NULL,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int proc_wanmode_fops_read(struct seq_file *seq, void *v)
{
	int err = 0;
	seq_printf(seq, "WAN Mode: ");
	switch(WAN_MODE){
		case GPON_MODE: seq_printf(seq, "GPON (%d)\n", WAN_MODE); break;
		case EPON_MODE: seq_printf(seq, "EPON (%d)\n", WAN_MODE); break;
		case FIBER_MODE: seq_printf(seq, "Fiber (%d)\n", WAN_MODE); break;
		default: seq_printf(seq, "ETHWAN (%d)\n", WAN_MODE); break;
	}
	return err;
}

static int proc_wanmode_fops_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
	int mode = -1;
	char tmpbuf[24] = {0};
	
	if(buf && count < (sizeof(tmpbuf)-1) && 
		!copy_from_user(tmpbuf, buf, count))
	{
		sscanf(tmpbuf, "%d", &mode);
		switch(mode){
			case ETH_MODE:
			case GPON_MODE: 
			case EPON_MODE: 
			case FIBER_MODE:
				WAN_MODE = mode;
				break;
			default:
				printk("Error WAN Mode !!!!\n"); 
		}
	}
	
	return count;
}

static int proc_wanmode_fops_fops_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_wanmode_fops_read, inode->i_private);
}
static const struct file_operations smux_wanmode_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_wanmode_fops_fops_open,
        .read           = seq_read,
        .write          = proc_wanmode_fops_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};
/***************************************************************************
 * Function Name: smux_init_procfs
 * Description  : Initialization of smux driver
 * Returns      : 0 succexx, other fail
 ***************************************************************************/
static char *smux_dev_name = "rtk_smux";
static struct proc_dir_entry *procfs = NULL;
static int smux_init_procfs(void)
{
	struct proc_dir_entry *entry = NULL;

	/* create a directory */
	procfs = proc_mkdir(smux_dev_name, NULL);
	if(procfs == NULL)
	{
		printk("Register /proc/%s failed\n", smux_dev_name);
		return -ENOMEM;
	}
	
	entry = proc_create("interface", 0644, procfs, &smux_intf_fops);
	if (entry == NULL)
	{
		printk("Register /proc/%s/interface failed\n", smux_dev_name);
		remove_proc_entry(smux_dev_name, NULL);
		return -ENOMEM;
	}
	
	entry = proc_create("wan_mode", 0644, procfs, &smux_wanmode_fops);
	if (entry == NULL)
	{
		printk("Register /proc/%s/wan_mode failed\n", smux_dev_name);
		remove_proc_entry(smux_dev_name, NULL);
		return -ENOMEM;
	}
	return 0;
}
/***************************************************************************
 * Function Name: smux_drv_init
 * Description  : Initialization of smux driver
 * Returns      : struct net_device
 ***************************************************************************/
static int __init smux_drv_init(void)
{
	register_netdevice_notifier(&smux_notifier_block);
	smux_ioctl_set(smux_ioctl_handler);

	smux_init_procfs();

	#if defined(CONFIG_RTK_DEV_AP)
	rtl_multi_wan_init();
	#endif
	
	return 0;
} /* smux_drv_init */

/***************************************************************************
 * Function Name: smux_cleanup_devices
 * Description  : cleans up all the smux devices and releases memory on exit
 * Returns      : None
 ***************************************************************************/
static void __exit smux_cleanup_devices(void)
{
	struct net_device *dev;
	struct list_head *lh;
	struct list_head *lhp;
	struct smux_dev_info *dev_info;
	struct smux_group *grp;


	/* clean up all the smux devices */
	rtnl_lock();
	for_each_netdev(&init_net, dev)
	{
		if (dev->priv_flags & IFF_OSMUX) {
			dev_info = SMUX_DEV_INFO(dev);
			write_lock_irq(&smux_lock);
			list_del(&dev_info->list);
			write_unlock_irq(&smux_lock);
			unregister_netdevice(dev);
		}
	}
	rtnl_unlock();

	/* cleanup all smux groups  */
	write_lock_irq(&smux_lock);
	list_for_each_safe(lh, lhp, &smux_grp_devs) {
		grp = list_entry_smuxgrp(lh);
		if(grp) {
			list_del(&grp->virtual_devs);
		}
	}
	write_unlock_irq(&smux_lock);
} /* smux_cleanup_devices */

/***************************************************************************
 * Function Name: smux_drv_exit
 * Description  : smux module clean routine
 * Returns      : None
 ***************************************************************************/
static void __exit smux_drv_exit(void)
{
	smux_ioctl_set(NULL);

	/* Un-register us from receiving netdevice events */
	unregister_netdevice_notifier(&smux_notifier_block);
	smux_cleanup_devices();
	synchronize_net();

	#if defined(CONFIG_RTK_DEV_AP)
	rtl_multi_wan_exit();
	#endif
	
} /* smux_drv_exit */

#if 0
int smuxDevMacUpdate(unsigned char *pmac)
{
	struct smux_group *grp;
	struct list_head *lh;
	struct smux_dev_info * sdev = NULL;
	//int i;


	grp = __find_smux_group("nas0");
	if (!grp)
		return;

	read_lock(&smux_lock);
	list_for_each(lh, &grp->virtual_devs)
	{
		sdev = list_entry_smuxdev(lh);
		
		if (sdev->proto != SMUX_PROTO_BRIDGE)
		{
			smux_dev_set_mac_address(sdev->vdev, pmac);
		}
	}
	read_unlock(&smux_lock);
}
#endif

#ifdef CONFIG_PORT_MIRROR
static inline void smux_mirror_pkt(struct sk_buff *skb, 
					const struct smux_dev_info *dev_info, const int flag)
{
	struct sk_buff *skb2;
	//AUG_DBG("the dev_info->port_mirror is %d\n", dev_info->port_mirror);
	if ((skb2 = skb_clone(skb, GFP_ATOMIC)) != NULL) 
	{		
		skb2->dev = dev_info->mirror_dev;

		if(IN == flag) {
			skb_push(skb2, ETH_HLEN);
			//printk("IN MIRROR.\n");
		}
			
		//AUG_DBG("the dev_info->mirror_dev is %s\n", dev_info->mirror_dev->name);
		nic_tx_mirror(skb2);
	}
}
#endif

#if defined(CONFIG_RTK_DEV_AP)
int is_smux_vlan_dev(struct net_device *dev)
{
    return dev->priv_flags & IFF_VSMUX;
}

unsigned short smux_vlan_dev_vlan_id(const struct net_device *dev)
{	
	struct smux_dev_info *dev_info = NULL;
	
	dev_info = SMUX_DEV_INFO(dev);
	
	return (dev_info->vid&VLAN_VID_MASK);
}
EXPORT_SYMBOL(is_smux_vlan_dev);
EXPORT_SYMBOL(smux_vlan_dev_vlan_id);

int smux_get_sdev_member(int vid,unsigned int *member)
{
	struct list_head *lh, *llh;
	struct smux_group *smux_grp = NULL;
	struct smux_dev_info * sdev = NULL;
	unsigned int tmp_member = 0;

	read_lock(&smux_lock);

	list_for_each(lh, &smux_grp_devs) {
		smux_grp = (struct smux_group *)list_entry_smuxgrp(lh);
		
		list_for_each(llh, &smux_grp->virtual_devs) {
			sdev = list_entry_smuxdev(llh);

			if(sdev->vid == vid){
				tmp_member |= sdev->member;
			}			
		}
	}

	*member = tmp_member;
	read_unlock(&smux_lock);
	return 0;
}
EXPORT_SYMBOL(smux_get_sdev_member);
#endif

module_init(smux_drv_init);
module_exit(smux_drv_exit);

EXPORT_SYMBOL(smux_pkt_recv);

