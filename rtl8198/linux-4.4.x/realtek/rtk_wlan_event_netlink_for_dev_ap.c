#include <uapi/linux/if.h>
#include <net/sock.h>
#include <linux/netlink.h>

static int rtk_eventd_pid=0;
static struct sock * nl_eventd_sk=NULL;
#define MAX_PAYLOAD 2048

typedef struct __rtkEventHdr 
{
	int eventID;	
	char name[16];
	unsigned char data[0];
}rtkEventHdr;

#define RTK_EVENTD_HDR_LEN sizeof(rtkEventHdr)

int rtk_nlrecvmsg(struct sk_buff *skb,int _len, void *_recv_data)
{
	int pid;
//	struct sk_buff *skb;
  	struct nlmsghdr *nlh;
	
	pid=0;
//  	printk("net_link: data is ready to read.\n");
  	//skb = skb_get(_skb);
	
	if (skb->len >= NLMSG_SPACE(0)) {
    	nlh = nlmsg_hdr(skb);
		memcpy(_recv_data,NLMSG_DATA(nlh),_len);
    		pid = nlh->nlmsg_pid; /*pid of sending process */
//    		printk("net_link: pid is %d\n", pid);    		
		//kfree_skb(skb);	
		return pid;
  	}
	else
	{
		//kfree_skb(skb);	
		return -1;
	}
}
EXPORT_SYMBOL(rtk_nlrecvmsg);

int rtk_nlsendmsg (int _pid,struct sock *_nl_sk,int _len,void *_send_info) 
{
        struct nlmsghdr *nlh;
        struct sk_buff *skb;
        int rc;
        int len;

	 len = NLMSG_SPACE(_len+sizeof(struct nlmsghdr)+32);
 
        skb = alloc_skb(len, GFP_ATOMIC);
        if (!skb){
                printk(KERN_ERR "net_link: allocate failed.\n");
                return -1;
        }
        nlh = nlmsg_put(skb,0,0,0,len,0);
        NETLINK_CB(skb).portid = 0; /* from kernel */
 
        memcpy(NLMSG_DATA(nlh), _send_info, _len);
//        printk("net_link: going to send.\n");
        rc = netlink_unicast(_nl_sk, skb, _pid, MSG_DONTWAIT);
        if (rc < 0) {
                printk(KERN_ERR "net_link: can not unicast skb (%d)\n", rc);
        }
//        printk("net_link: send to %d is ok.\n",_pid);
        return 0;
}
EXPORT_SYMBOL(rtk_nlsendmsg);

void rtk_eventd_netlink_receive(struct sk_buff *puskb)
{
	//int		pid;
	rtkEventHdr * pEventdHdr=NULL;
	char msgBuf[1024];
	
	rtk_eventd_pid = rtk_nlrecvmsg(puskb, sizeof(msgBuf), msgBuf);
	if(rtk_eventd_pid<1)
	{
		return;
	}
	
	pEventdHdr=(rtkEventHdr *)msgBuf;
	
	//panic_printk("%s:%d##pid=%d\n",__FUNCTION__,__LINE__,rtk_eventd_pid);
	//panic_printk("%s:%d##eventID=%d\n",__FUNCTION__,__LINE__,pEventdHdr->eventID);
	//panic_printk("%s:%d##msg=%s\n",__FUNCTION__,__LINE__,pEventdHdr->data);
	
  	return;
}
EXPORT_SYMBOL(rtk_eventd_netlink_receive);

int get_nl_eventd_pid(void)
{
	return rtk_eventd_pid;
}
EXPORT_SYMBOL(get_nl_eventd_pid);

struct sock *get_nl_eventd_sk(void)
{
	return nl_eventd_sk;
}
EXPORT_SYMBOL(get_nl_eventd_sk);



void rtk_eventd_netlink_send(int pid, struct sock *nl_sk, int eventID, char *ifname, char *data, int data_len)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	int rc, len;
	
	rtkEventHdr *pEventdHdr=NULL;
	char msgBuf[2048];
	memset(msgBuf, 0, sizeof(msgBuf));
	pEventdHdr=(rtkEventHdr *)msgBuf;
	pEventdHdr->eventID=eventID;
	
	if(ifname!=NULL)
	{
		strcpy(pEventdHdr->name, ifname);
	}
	
	if(data_len > MAX_PAYLOAD - RTK_EVENTD_HDR_LEN || (data_len + sizeof(rtkEventHdr) > sizeof(msgBuf)))
	{
		//panic_printk("%s:%d##date len is too long!\n",__FUNCTION__,__LINE__);
		printk("%s:%d##date len[%d] is too long!\n",__FUNCTION__,__LINE__, data_len);
		return;
	}
	
	memcpy(pEventdHdr->data, data,data_len);	
	
	//if(data_len>MAX_OPENVPN_PAYLOAD)
	//{
	//	printk("%s:%d##date len is too long!\n",__FUNCTION__,__LINE__);
	//	return;
	//}
	
	len = NLMSG_SPACE(MAX_PAYLOAD-NLMSG_HDRLEN);
	//len = NLMSG_SPACE(MAX_PAYLOAD-RTK_EVENTD_HDR_LEN);

	skb = alloc_skb(len, GFP_ATOMIC);
	if(!skb)
	{
		printk(KERN_ERR "net_link: allocate failed.\n");
		return;
	}
	
	nlh = nlmsg_put(skb,0,0,0,len,0);

	if(nlh==NULL)
	{
		printk("data_len=%d len=%d nlh is NULL!\n\n", data_len, len);
	}
	
	NETLINK_CB(skb).portid = 0; /* from kernel */

	//memcpy(NLMSG_DATA(nlh), data, data_len);
	memcpy(NLMSG_DATA(nlh), pEventdHdr, data_len+sizeof(rtkEventHdr));
	
	nlh->nlmsg_len=data_len+sizeof(rtkEventHdr)+NLMSG_HDRLEN;
	
	rc = netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
	
	if (rc < 0)
	{
		printk("%s net_link: can not unicast skb (%d)\n", __FUNCTION__, rc);
	}

	return;
}
EXPORT_SYMBOL(rtk_eventd_netlink_send);

void rtk_eventd_netlink_send_multicast(int pid, struct sock *nl_sk, int eventID, char *ifname, char *data, int data_len)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	int rc, len;
	#define Rtk_Event_Netlink_Group (1<<0)
	
	rtkEventHdr *pEventdHdr=NULL;
	char msgBuf[128];
	memset(msgBuf, 0, sizeof(msgBuf));
	pEventdHdr=(rtkEventHdr *)msgBuf;
	pEventdHdr->eventID=eventID;

	//#define Rtk_Event_Netlink_Group (1<<8)
	
	if(ifname!=NULL)
	{
		strcpy(pEventdHdr->name, ifname);
	}
	
	if((data_len > MAX_PAYLOAD - RTK_EVENTD_HDR_LEN) || (data_len + sizeof(rtkEventHdr) > sizeof(msgBuf)))
	{
		//panic_printk("%s:%d##date len is too long!\n",__FUNCTION__,__LINE__);
		printk("%s:%d##date len[%d] is too long!\n",__FUNCTION__,__LINE__, data_len);
		return;
	}

	//strcpy(pEventdHdr->data, data);
	memcpy(pEventdHdr->data, data,data_len);
	
	//if(data_len>MAX_OPENVPN_PAYLOAD)
	//{
	//	printk("%s:%d##date len is too long!\n",__FUNCTION__,__LINE__);
	//	return;
	//}
	
	len = NLMSG_SPACE(MAX_PAYLOAD-NLMSG_HDRLEN);
	//len = NLMSG_SPACE(MAX_PAYLOAD-RTK_EVENTD_HDR_LEN);

	skb = alloc_skb(len, GFP_ATOMIC);
	if(!skb)
	{
		printk(KERN_ERR "net_link: allocate failed.\n");
		return;
	}
	
	nlh = nlmsg_put(skb,0,0,0,len,0);

	if(nlh==NULL)
	{
		printk("data_len=%d len=%d nlh is NULL!\n\n", data_len, len);
	}
	
	NETLINK_CB(skb).portid = 0; /* from kernel */
       /* to mcast group 1<<0 */  
       NETLINK_CB(skb).dst_group = Rtk_Event_Netlink_Group;

	//memcpy(NLMSG_DATA(nlh), data, data_len);
	memcpy(NLMSG_DATA(nlh), pEventdHdr, data_len+sizeof(rtkEventHdr));
	
	nlh->nlmsg_len=data_len+sizeof(rtkEventHdr)+NLMSG_HDRLEN;
	
	rc = netlink_broadcast(nl_sk, skb, 0, 1, GFP_KERNEL);
	if (rc < 0)
	{
		printk("%s net_link: can not multicast skb (%d)\n", __FUNCTION__, rc);
	}

	return;
}
EXPORT_SYMBOL(rtk_eventd_netlink_send_multicast);

void rtk_smart_roaming_netlink_send_multicast(int pid, struct sock *nl_sk, char *data, int data_len)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	int rc, len;
	#define Rtk_Smart_Roaming_Netlink_Group (1<<1)
		
	//if(data_len>MAX_OPENVPN_PAYLOAD)
	//{
	//	printk("%s:%d##date len is too long!\n",__FUNCTION__,__LINE__);
	//	return;
	//}
	if(data_len > MAX_PAYLOAD - RTK_EVENTD_HDR_LEN)
	{
		//panic_printk("%s:%d##date len is too long!\n",__FUNCTION__,__LINE__);
		printk("%s:%d##date len is too long!\n",__FUNCTION__,__LINE__);
		return;
	}
	
	len = NLMSG_SPACE(MAX_PAYLOAD-NLMSG_HDRLEN);
	//len = NLMSG_SPACE(MAX_PAYLOAD-RTK_EVENTD_HDR_LEN);

	skb = alloc_skb(len, GFP_ATOMIC);
	if(!skb)
	{
		printk(KERN_ERR "net_link: allocate failed.\n");
		return;
	}
	
	nlh = nlmsg_put(skb,0,0,0,len,0);

	if(nlh==NULL)
	{
		printk("data_len=%d len=%d nlh is NULL!\n\n", data_len, len);
	}
	
	NETLINK_CB(skb).portid = 0; /* from kernel */
       /* to mcast group 1<<0 */  
       NETLINK_CB(skb).dst_group = Rtk_Smart_Roaming_Netlink_Group;

	memcpy(NLMSG_DATA(nlh), data, data_len);
	//memcpy(NLMSG_DATA(nlh), pEventdHdr, data_len+sizeof(rtkEventHdr));
	
	nlh->nlmsg_len=data_len+sizeof(rtkEventHdr)+NLMSG_HDRLEN;

	rc = netlink_broadcast(nl_sk, skb, 0, Rtk_Smart_Roaming_Netlink_Group, GFP_KERNEL);
	
	if (rc < 0)
	{
		printk(KERN_ERR "net_link: can not multicast skb (%d)\n", rc);
	}

	return;
}
EXPORT_SYMBOL(rtk_smart_roaming_netlink_send_multicast);

struct sock * rtk_eventd_netlink_init(void) 
{
	struct netlink_kernel_cfg cfg={
		.input=rtk_eventd_netlink_receive,
	};

	nl_eventd_sk = netlink_kernel_create(&init_net, NETLINK_RTK_EVENTD, &cfg);
  	//nl_eventd_sk = netlink_kernel_create(&init_net, NETLINK_RTK_EVENTD, 0, rtk_eventd_netlink_receive, NULL, THIS_MODULE);

  	if (!nl_eventd_sk) 
	{
    		//panic_printk(KERN_ERR "kernel create eventd netlink socket fail!\n");
    		printk(KERN_ERR "kernel create eventd netlink socket fail!\n");
  	}
  	return nl_eventd_sk;
}
EXPORT_SYMBOL(rtk_eventd_netlink_init);

/*  below is added for userspace error based on sd9-/linux-4.4.x/realtek/rtk_wlan_event.c:
 *   [queryWlanEventCouter:3239] Can't open file /proc/rtk_wlan_event/wlan0/wlan_event_counter !
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/spinlock_types.h>
#include <linux/seq_file.h>

#define DRIVER_AUTHOR 	"IulianWu"
#define DRIVER_DESC 	"RealTek WLAN Event Counters"

static struct proc_dir_entry *entry = NULL;
static struct proc_dir_entry *parent0, *parent1;

#define PROCFS_WLAN_EVENT_COUNTER			"wlan_event_counter"
#define PROCFS_OSGI_DIR0 					"rtk_wlan_event"
#define PROCFS_OSGI_DIR1 					"wlan1_event"
#define WLAN_EVENT_DISAPPEAR_TIMEOUT		10 //if counter didn't increase in 10secs, adjust the traffic is disappear.
#define MACADDRLEN	6


typedef struct wlan_event_entry_s
{
	int eventID;
	unsigned char mac[MACADDRLEN];
	char ifname[IFNAMSIZ];
	char reason;
	long timer;
    struct list_head list;
} wlan_event_entry_t;

wlan_event_entry_t wlan_event_entry_list;

static struct mfc_cache *wlan_event_seq_idx(loff_t pos)
{
	wlan_event_entry_t *node = NULL;
	list_for_each_entry(node, &(wlan_event_entry_list.list), list)
	if (pos-- == 0)
		return node;
	return NULL;
}

static void *wlan_event_seq_start(struct seq_file *s, loff_t *pos)
{
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos);
#endif
	return *pos ? wlan_event_seq_idx(*pos - 1)	: SEQ_START_TOKEN;
}
static void *wlan_event_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	++*pos;
#if __DEBUG
	printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos);
#endif
	return wlan_event_seq_start(s, pos);
}

static void wlan_event_seq_stop(struct seq_file *s, void *v)
{
#if __DEBUG
	printk("[%s:%d] \n", __FUNCTION__, __LINE__);
#endif
	wlan_event_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(wlan_event_entry_list.list), list)
	{
		list_del(&node->list);
		kfree(node);
	}
}

static int wlan_event_seq_show(struct seq_file *seq, void *v)
{
	int n;

	if (v == SEQ_START_TOKEN) {
		seq_puts(seq,  "eventID ifname mac reason\n");
	}
	else {
		struct timespec now;
		long curr_time;
		wlan_event_entry_t *node = v;
		now = current_kernel_time();
		curr_time = now.tv_sec;
#if __DEBUG
		printk("[%s:%d] %d %s %02x:%02x:%02x:%02x:%02x:%02x %d\n", __FUNCTION__, __LINE__,
				node->eventID, node->ifname, node->mac[0], node->mac[1], node->mac[2],
				node->mac[3], node->mac[4], node->mac[5], node->reason);
#endif
		seq_printf(seq, "%d %s %02x:%02x:%02x:%02x:%02x:%02x %d\n",
				node->eventID, node->ifname, node->mac[0], node->mac[1], node->mac[2],
				node->mac[3], node->mac[4], node->mac[5], node->reason);

	}
	return 0;
}

static struct seq_operations wlan_event_counter_ops = {
	.start			= wlan_event_seq_start,
	.next			= wlan_event_seq_next,
	.stop			= wlan_event_seq_stop,
	.show			= wlan_event_seq_show
};

static int wlan_event_counter_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &wlan_event_counter_ops);
};

static struct file_operations wlan_event_counter_fops = {
	.owner 			= THIS_MODULE,
	.open			= wlan_event_counter_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= seq_release
};


static void wlan_event_entry_list_init(void)
{
    INIT_LIST_HEAD(&wlan_event_entry_list.list);
}

static int __init wlan_event_init_main(void)
{

	parent0 = proc_mkdir(PROCFS_OSGI_DIR0, NULL);
	parent1 = proc_mkdir("wlan0", parent0);
	entry = proc_create_data(PROCFS_WLAN_EVENT_COUNTER, 0444, parent1, &wlan_event_counter_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_OSGI_DIR0, PROCFS_WLAN_EVENT_COUNTER);
	}

	wlan_event_entry_list_init();

	printk("%s: Successfully inserted a hook into kernel\n", __FUNCTION__);
	return 0;
}

static void __exit wlan_event_cleanup_main(void)
{
	wlan_event_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(wlan_event_entry_list.list), list)
	{
		list_del(&node->list);
		kfree(node);
	}

	remove_proc_entry(PROCFS_WLAN_EVENT_COUNTER, parent1);
	remove_proc_entry("wlan0", parent0);
	proc_remove(parent0);

	printk("Successfully unloaded the hook\n");
}


late_initcall(wlan_event_init_main);
module_exit(wlan_event_cleanup_main);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);




