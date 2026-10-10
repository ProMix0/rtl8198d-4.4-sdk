#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h> 
#include <net/sock.h> 
#include <net/netlink.h> 
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>

static struct sock *pppoe_monitor_sock;
int monitor_pid = -1;

#define NETLINK_PPPOE_MONITOR	29

enum{
	PPPOE_MONITOR_PID=0,
	PPPOE_MONITOR_ADD_PACKET_FEATURE,
	PPPOE_MONITOR_DEL_PACKET_FEATURE,
	PPPOE_MONITOR_FLUSH_PACKET_FEATURE,
};

typedef struct {
	char ifname[16];
	int pktLen;
	unsigned char pktContent[0];
} pppoeMsg_t;

struct PPPoEMonitorPacket {
    unsigned char srcip[4];
	unsigned short srcport;
	unsigned char protocol;
	unsigned char indev[32];	
};

struct PPPoEMonitorPacket_LIST{
	struct PPPoEMonitorPacket packet_feature;
	unsigned int match_cnt;
	struct PPPoEMonitorPacket_LIST*next;
};
DEFINE_RWLOCK(pppMonitorList_lock);

struct PPPoEMonitorPacket_LIST *pppMonitorListHead=NULL;
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
unsigned int forward_by_ps_mark=0;
#endif

static void dump_packet(unsigned char *ptitle, unsigned char *pkt, unsigned int size)
{
	char tmpbuf[100];
	int i, n=0;

	if (ptitle)
		snprintf(tmpbuf, sizeof(tmpbuf), "%s", ptitle);
	else
		tmpbuf[0] = '\0';
	
	for (i=0; i<size; i++)
	{
		if (!(i & 0xf))
		{
			printk("%s\n", tmpbuf);
			n = sprintf(tmpbuf, "%03X:\t", i);
		}
		n += snprintf((tmpbuf+n), sizeof(tmpbuf)-n, " %02X", pkt[i]);
	}
	printk("%s\n", tmpbuf);
}

int match_pppoe_monitor_packet(char* indev, unsigned int srcip, unsigned char proto, unsigned short srcport)
{
	struct PPPoEMonitorPacket_LIST *pListNode;
	unsigned int match_srcip;
	read_lock_bh(&pppMonitorList_lock);
	pListNode = pppMonitorListHead;
	while(pListNode)
	{
		match_srcip = *((unsigned int*)(pListNode->packet_feature.srcip));
		if((strlen(pListNode->packet_feature.indev) == 0 || strcmp(pListNode->packet_feature.indev, indev) == 0) &&
			(pListNode->packet_feature.protocol == 0 || pListNode->packet_feature.protocol == proto) &&
			(match_srcip == 0 || match_srcip == srcip) &&
			(pListNode->packet_feature.srcport == 0 || pListNode->packet_feature.srcport == srcport))
		{
			pListNode->match_cnt++;
			read_unlock_bh(&pppMonitorList_lock);
			return 1;
		}
		pListNode = pListNode->next;
	}
	read_unlock_bh(&pppMonitorList_lock);
	return 0;
}

int pppoe_dial_monitor_process(struct sk_buff *skb)
{
	struct sk_buff *deliverSkb;
    struct nlmsghdr *nlh;
	pppoeMsg_t *pMsgData;
	unsigned char *pData;
	int len;
	int ret;

	if (-1 == monitor_pid)
		return -1;
	
	//if (skb->dev->priv_flags & IFF_RSMUX)
	//	return -1;
	
	/* L2 header has been removed */
	pData = skb->data-2;
	if (ntohs(*(unsigned short *)pData) == ETH_P_8021Q)
		pData += 4;
	if ((ntohs(*(unsigned short *)pData) == ETH_P_PPP_DISC) ||
		((ntohs(*(unsigned short *)pData) == ETH_P_PPP_SES) && (ntohs(*(unsigned short *)(pData+8)) != 0x0021) && (ntohs(*(unsigned short *)(pData+8)) != 0x0057)))
	{
		len = sizeof(pppoeMsg_t) + skb->len + ETH_HLEN;
		deliverSkb = alloc_skb(NLMSG_SPACE(len), GFP_ATOMIC);
		if (!deliverSkb)
		{
			return -1;
		}

#if defined(CONFIG_KERNEL_2_6_30)
		nlh = NLMSG_PUT(deliverSkb, monitor_pid, 0, 0, len);
#else
		nlh = nlmsg_put(deliverSkb, monitor_pid, 0, 0, len, 0);
#endif
		nlh->nlmsg_flags = 0;
		pMsgData = NLMSG_DATA(nlh);
		
		snprintf(pMsgData->ifname, 16, "%s", skb->dev->name);
		pMsgData->pktLen = skb->len + ETH_HLEN;

		//dump_packet("kernel:", skb->data-14, skb->len+14);
		if (ntohs(*(unsigned short *)(skb->data-2)) == ETH_P_8021Q)
		{
			/* remove vlan tag */
			memcpy(pMsgData->pktContent, skb->data-ETH_HLEN, 12);
			memcpy(pMsgData->pktContent+12, skb->data+2, skb->len-2);
		}
		else if ((skb->vlan_tci) && (eth_hdr(skb)->h_proto == ETH_P_8021Q))
		{
			memcpy(pMsgData->pktContent, skb->data-ETH_HLEN-4, 12);
			memcpy(pMsgData->pktContent+12, skb->data-2, skb->len+2);
		}
		else
			memcpy(pMsgData->pktContent, skb->data-ETH_HLEN, pMsgData->pktLen);
		
#if defined(CONFIG_KERNEL_2_6_30)
		NETLINK_CB(deliverSkb).pid = 0; /* from kernel */
#else
		NETLINK_CB(deliverSkb).portid = 0; /* from kernel */
#endif
		NETLINK_CB(deliverSkb).dst_group = 0; /* unicast */
		
		ret = netlink_unicast(pppoe_monitor_sock, deliverSkb, monitor_pid, MSG_DONTWAIT);
		if (ret < 0)
		{
			printk("%s send failed %d, monitor_pid=%d\n", __func__, ret, monitor_pid);
		}
		
		return 0;
	}
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	else if((ntohs(*(unsigned short *)pData) == ETH_P_PPP_SES) && (ntohs(*(unsigned short *)(pData+8)) == 0x0021) )
	{
		struct iphdr* iph;
		unsigned int  srcip;
		unsigned short srcport;
		unsigned char proto;	
		struct PPPoEMonitorPacket packet_feature={0};

		if((pppMonitorListHead == NULL) || (forward_by_ps_mark == 0))
			return 0;
		pData += 10;
		iph = pData;
		srcip = ntohl(iph->saddr);
		proto = iph->protocol;
		if(proto == 6)
		{
			struct tcphdr* tcph = (char*)iph + iph->ihl*4;
			srcport = tcph->source;
		}
		else if(proto == 17)
		{
			struct udphdr* udph = (char*)iph + iph->ihl*4;
			srcport = udph->source;
		}
		
		if(match_pppoe_monitor_packet(skb->dev->name, srcip, proto, srcport))
		{		
#ifdef CONFIG_RTK_SKB_MARK2
			skb->mark2 |= forward_by_ps_mark;
#endif
		}
		return 0;
	}
#endif
}

int delete_pppoe_monitor_packet(struct PPPoEMonitorPacket*packet_feature)
{
	struct PPPoEMonitorPacket_LIST *pListNode,*pPreListNode=NULL;
	write_lock_bh(&pppMonitorList_lock);
	pListNode = pppMonitorListHead;
	while(pListNode)
	{
		if((strcmp(pListNode->packet_feature.indev, packet_feature->indev)==0) &&
			(pListNode->packet_feature.protocol == packet_feature->protocol) &&
			(memcmp(pListNode->packet_feature.srcip, packet_feature->srcip, 4)==0) &&
			(pListNode->packet_feature.srcport == packet_feature->srcport))
		{
			if(pPreListNode)
				pPreListNode->next = pListNode->next;
			else
				pppMonitorListHead = pListNode->next;
			printk("pppoe_monitor delete monitor packet feature\nindev:%s srcip:%pI4 proto:%u srcport:%u\n", pListNode->packet_feature.indev,
				pListNode->packet_feature.srcip, pListNode->packet_feature.protocol, 
				pListNode->packet_feature.srcport);
			kfree(pListNode);
			write_unlock_bh(&pppMonitorList_lock);;
			return 1;
		}
		pPreListNode = pListNode;
		pListNode = pListNode->next;
	}
	write_unlock_bh(&pppMonitorList_lock);
	return 0;
}

int flush_pppoe_monitor_packet(void)
{
	struct PPPoEMonitorPacket_LIST *pListNode,*pPreListNode=NULL;
	write_lock_bh(&pppMonitorList_lock);
	pListNode = pppMonitorListHead;
	while(pppMonitorListHead)
	{
		pListNode = pppMonitorListHead;
		pppMonitorListHead = pppMonitorListHead->next;
		kfree(pListNode);
	}
	write_unlock_bh(&pppMonitorList_lock);
	return 0;
}

struct PPPoEMonitorPacket_LIST *find_pppoe_monitor_packet(struct PPPoEMonitorPacket*packet_feature)
{
	struct PPPoEMonitorPacket_LIST *pListNode;
	if(packet_feature==NULL)
		return NULL;
	read_lock_bh(&pppMonitorList_lock);
	pListNode = pppMonitorListHead;
	while(pListNode)
	{
		if((strcmp(pListNode->packet_feature.indev, packet_feature->indev)==0) &&
			(pListNode->packet_feature.protocol == packet_feature->protocol) &&
			(memcmp(pListNode->packet_feature.srcip, packet_feature->srcip, 4)==0) &&
			(pListNode->packet_feature.srcport == packet_feature->srcport))
		{
			read_unlock_bh(&pppMonitorList_lock);
			return pListNode;
		}
		pListNode = pListNode->next;
	}
	read_unlock_bh(&pppMonitorList_lock);
	return NULL;
}

int dump_pppoe_monitor_packet_list(void)
{
	struct PPPoEMonitorPacket_LIST *pListNode;
	printk("PPPoE Monitor Packet List:\n");
	read_lock_bh(&pppMonitorList_lock);
	pListNode = pppMonitorListHead;
	while(pListNode)
	{
		printk("indev:%s srcip:%pI4 proto:%u srcport:%u match_cnt=%u\n", pListNode->packet_feature.indev,
			pListNode->packet_feature.srcip, pListNode->packet_feature.protocol, 
			pListNode->packet_feature.srcport, pListNode->match_cnt);
		pListNode = pListNode->next;
	}
	read_unlock_bh(&pppMonitorList_lock);
}

static void pppoe_monitor_netlink_recv(struct sk_buff *skb)
{
    u_int pid;
    void *data;
    struct nlmsghdr *nlh;

    nlh = (struct nlmsghdr *)skb->data;
    pid = NETLINK_CREDS(skb)->pid;
    data = NLMSG_DATA(nlh);
	if(nlh->nlmsg_type == PPPOE_MONITOR_PID)
	{
		monitor_pid = *(unsigned int *)data;
    	printk("pppoe_monitor_netlink_recv from pid=%d set monitor_pid=%d\n", pid, monitor_pid);
	}
	else if(nlh->nlmsg_type == PPPOE_MONITOR_ADD_PACKET_FEATURE)
	{
		struct PPPoEMonitorPacket *packetFeature = (struct PPPoEMonitorPacket *)data;
		struct PPPoEMonitorPacket_LIST *pListNode;
		if(find_pppoe_monitor_packet(packetFeature))
		{
			printk("pppoe_monitor get duplicated monitor packet feature\n");
			return;
		}
		pListNode = kmalloc(sizeof(struct PPPoEMonitorPacket_LIST),GFP_KERNEL);
		pListNode->match_cnt = 0;
		memcpy(&(pListNode->packet_feature), packetFeature, sizeof(struct PPPoEMonitorPacket));
		printk("pppoe_monitor get monitor packet feature\nindev:%s srcip:%pI4 proto:%u srcport:%u\n", pListNode->packet_feature.indev,
			pListNode->packet_feature.srcip, pListNode->packet_feature.protocol, 
			pListNode->packet_feature.srcport);
		write_lock_bh(&pppMonitorList_lock);
		pListNode->next = pppMonitorListHead;
		pppMonitorListHead = pListNode;
		write_unlock_bh(&pppMonitorList_lock);
		//dump_pppoe_monitor_packet_list();
	}
	else if(nlh->nlmsg_type == PPPOE_MONITOR_DEL_PACKET_FEATURE)
	{
		struct PPPoEMonitorPacket *packetFeature = (struct PPPoEMonitorPacket *)data;
		delete_pppoe_monitor_packet(packetFeature);
	}
	else if(nlh->nlmsg_type == PPPOE_MONITOR_FLUSH_PACKET_FEATURE)
	{
		flush_pppoe_monitor_packet();
	}
}

static int proc_ppp_monitor_fops_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
     char tmpbuf[16] = {0};
     int len = (count > 15) ? 15 : count;
    
	 if (buf && !copy_from_user(tmpbuf, buf, len))
     {
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
		forward_by_ps_mark = simple_strtoul(tmpbuf, NULL, 0);
#endif
		return count;
     }

	 return -EFAULT;
}   

static int proc_ppp_monitor_fops_read(struct seq_file *seq, void *v)
{
	printk("forward_by_ps_mark=0x%x;\n", forward_by_ps_mark);
	dump_pppoe_monitor_packet_list();
    return 0;
}

static int proc_ppp_monitor_fops_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_ppp_monitor_fops_read, inode->i_private);
}

static const struct file_operations ppp_monitor_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_ppp_monitor_fops_open,
        .write          = proc_ppp_monitor_fops_write,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

int __init pppoe_monitor_netlink_init(void)
{
	struct proc_dir_entry *entry = NULL;
	entry = proc_create("ppp-monitor", 0644, NULL, &ppp_monitor_fops);
	if (entry == NULL)
	{
		printk("Register /proc/ppp-monitor failed\n");		
		return -ENOMEM;
	}
    /* Create netlink for communicate between kernel and user protocol */
#if defined(CONFIG_KERNEL_2_6_30)
    pppoe_monitor_sock = netlink_kernel_create(&init_net, NETLINK_PPPOE_MONITOR, 0, pppoe_monitor_netlink_recv, NULL, THIS_MODULE);
#else	
	struct netlink_kernel_cfg cfg = {
	    .input = pppoe_monitor_netlink_recv,
	};

	pppoe_monitor_sock = netlink_kernel_create(&init_net, NETLINK_PPPOE_MONITOR, &cfg);
#endif
}

void __exit pppoe_monitor_netlink_exit(void)
{
    sock_release(pppoe_monitor_sock->sk_socket);
}

module_init(pppoe_monitor_netlink_init);
module_exit(pppoe_monitor_netlink_exit);

