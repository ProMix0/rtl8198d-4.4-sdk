#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <uapi/linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/types.h>
#include <linux/inet.h>
#include <net/rtl/rtl_types.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/spinlock_types.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>

//#define __DEBUG 1

#define DRIVER_AUTHOR 	"IulianWu"
#define DRIVER_DESC 	"Traffic Monitor"

static int g_traffic_monitor_debug = 0;

struct sk_buff *sock_buff;
static struct nf_hook_ops ipv4_monitor_nfho;
static struct nf_hook_ops ipv6_monitor_nfho;
static struct proc_dir_entry *proc_dir = NULL;

#define PROCFS_TRAFFIC_MONITOR_ADD		"traffic_monitor_add"
#define PROCFS_TRAFFIC_MONITOR_MOD		"traffic_monitor_mod"
#define PROCFS_TRAFFIC_MONITOR_DEL		"traffic_monitor_del"
#define PROCFS_TRAFFIC_MONITOR_COUNTER	"traffic_monitor_counter"
#define PROCFS_TRAFFIC_MONITOR_DEBUG	"traffic_monitor_debug"
#define PROCFS_DBUS_DIR 			"dbus"
#define TRAFFIC_MONITOR_DISAPPEAR_TIMEOUT	10 //if counter didn't increase in 10secs, adjust the traffic is disappear.

static DEFINE_SPINLOCK(traffic_monitor_unres_lock); //protect the list 

typedef struct traffic_montior_key_entry_s
{
	//int bundleID;
	int instNum;
	int isIpv4;
	int netmask;
	union{
		__be32 daddr;
		struct{
			struct in6_addr	ip6Daddr;
			struct in6_addr ip6Dmask;
		}ip6;
	} ipInfo;
	char url[128];
} traffic_montior_key_entry_t;

typedef struct traffic_montior_entry_s
{
	traffic_montior_key_entry_t key;
	int counter;
	long timer;
	bool detect_flag;
	bool dispear_flag;
    struct list_head list;
} traffic_montior_entry_t;

traffic_montior_entry_t traffic_montior_entry_list;


#define traffic_monitor_printk(f,args...) \
do{\
  printk("\033[1;36;40m[%s:%d]\n",__FUNCTION__,__LINE__);\
  printk(f,##args);\
  printk("\033[0m");\
}while(0);

static bool ipAddress_match(__be32 addr, __be32 net, uint8 bits) {
	if (bits == 0) {
		return true;
	}
	
	return !((addr ^ net) & htonl(0xFFFFFFFFu << (32 - bits)));
}


/* Older kernel has different funcion definition. */
static unsigned int ipv4_traffic_monitor_hook(
		const struct nf_hook_ops *ops,
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	char *ifname = (char *)(in->name);
	traffic_montior_entry_t *node = NULL;
	struct ethhdr *mac_header = (struct ethhdr *)skb_mac_header(skb);
	struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);
	sock_buff = skb;
	struct timespec now;
	static int debug_ip4_cnt = 0;

	if (!sock_buff) {
		return NF_ACCEPT;
	}

	//check the entry is empty or not
	if(!list_empty(&traffic_montior_entry_list.list)){ 
		list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		{
			if(node->key.isIpv4 == 0)
				continue;	

			if(g_traffic_monitor_debug){
				if(debug_ip4_cnt < g_traffic_monitor_debug){
					traffic_monitor_printk("======ipv4_traffic_monitor_hook=====\n"\
											"node->key.d_addr is %pI4\n"\
											"node->key.netmask is %d\n"\
											"got packet dip is %pI4\n"\
											"debug_ip4_cnt is %d\n",  
											&(node->key.ipInfo.daddr),
											node->key.netmask,
											&(ip_header->daddr),
											debug_ip4_cnt);
					debug_ip4_cnt++;
				}
			}
		
			if (ipAddress_match(ip_header->daddr, node->key.ipInfo.daddr, node->key.netmask )) {
				if(node->counter == 0) {
					node->detect_flag = TRUE;
				}
				node->counter++;
				now = current_kernel_time();
				node->timer = now.tv_sec;
				if(g_traffic_monitor_debug){		
					traffic_monitor_printk("======Found the monitor destination ipaddress in dev: %s=====\n"\
											"src_mac: %pM\n"\
											"dst_mac: %pM\n"\
											"src_ip: %pI4\n"\
											"dst_ip: %pI4\n"\
											"node->counter: %d\n"\
											"timer: %ld\n", 
											ifname, mac_header->h_source, mac_header->h_dest,
											&ip_header->saddr, &ip_header->daddr, node->counter, now.tv_sec);
				}			
			}

		}	
	}
	else if(g_traffic_monitor_debug){
		traffic_monitor_printk("======ipv4_traffic_monitor_hook list is empty ======! \n");
	}

	return NF_ACCEPT;
}

/* Older kernel has different funcion definition. */
static unsigned int ipv6_traffic_monitor_hook(
		const struct nf_hook_ops *ops,
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	char *ifname = (char *)(in->name);
	traffic_montior_entry_t *node = NULL;
	struct ethhdr *mac_header = eth_hdr(skb);
	struct ipv6hdr *ipv6_header = ipv6_hdr(skb);;
	sock_buff = skb;
	struct timespec now;
	static int debug_ip6_cnt = 0;

	if (!sock_buff) {
		return NF_ACCEPT;
	}

	//check the entry is empty or not
	if(!list_empty(&traffic_montior_entry_list.list)){ 
		list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		{
			if(node->key.isIpv4 == 1)
				continue;	

			if(g_traffic_monitor_debug){
				if(debug_ip6_cnt < g_traffic_monitor_debug){
					traffic_monitor_printk("======ipv6_traffic_monitor_hook=====\n"\
											"node->key.d_addr is %pI6\n"\
											"node->key.netmask is %d\n"\
											"got packet dip is %pI6\n"\
											"debug_ip6_cnt is %d\n ", 
											&(node->key.ipInfo.ip6.ip6Daddr), 
											node->key.netmask,
											&(ipv6_header->daddr),
											debug_ip6_cnt);

					debug_ip6_cnt++;
				}
			}

			if (0 == ipv6_masked_addr_cmp(&(node->key.ipInfo.ip6.ip6Daddr), &(node->key.ipInfo.ip6.ip6Dmask),&(ipv6_header->daddr)))
			{
				if(node->counter == 0) {
					node->detect_flag = TRUE;
				}
				node->counter++;
				now = current_kernel_time();
				node->timer = now.tv_sec;
				
				if(g_traffic_monitor_debug){	
					traffic_monitor_printk("======Found the monitor destination ipaddress in dev: %s=====\n"\
											"src_mac: %pM\n"\
											"dst_mac: %pM\n"\
											"src_ip6: %pI6\n"\
											"dst_ip6: %pI6\n"\
											"timer: %ld\n",
											ifname,  mac_header->h_source, mac_header->h_dest,
											&(ipv6_header->saddr), &(ipv6_header->daddr), now.tv_sec);
				}			
			}

		}	
	}
	else if(g_traffic_monitor_debug){
		traffic_monitor_printk("======ipv6_traffic_monitor_hook list is empty ======! \n");
	}

	return NF_ACCEPT;
}

static int netfilterIpv6Net2Mask(unsigned int netlen, struct in6_addr *mask)
{
	char *p = (char *)mask;

	if(netlen > 128)
	{
		traffic_monitor_printk("%s:%d, mask len (%d) is not valid\n", __FUNCTION__, __LINE__, netlen);
		return -1;
	}
	
	memset(p, 0xff, netlen / 8);
	memset(p + (netlen / 8) + 1, 0, (128 - netlen) / 8);
	p[netlen/8] = 0xff << (8 - (netlen & 7));
	
	return 0;
}

static int proc_traffic_monitor_add_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[512] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	int instNum, netmask;
	traffic_montior_entry_t *pTrafficMonitorEntry;
	traffic_montior_entry_t *node = NULL; 
	
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		if(g_traffic_monitor_debug){
			traffic_monitor_printk("%s:%d, input parameters: \"%s\"\n", __FUNCTION__, __LINE__, tmpbuf);
		}

		pTrafficMonitorEntry = kmalloc(sizeof(traffic_montior_entry_t), GFP_ATOMIC);
		if (!pTrafficMonitorEntry)
			return -1;
		memset(pTrafficMonitorEntry, 0, sizeof(traffic_montior_entry_t));

		strptr = tmpbuf;
		/* instNum */
		tokptr = strsep(&strptr, " ");
		if (!tokptr){ kfree(pTrafficMonitorEntry); return -EFAULT;}
		instNum = simple_strtol(tokptr, NULL, 0);
		pTrafficMonitorEntry->key.instNum = instNum;

		/* IP version */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {kfree(pTrafficMonitorEntry); return -EFAULT;}
		pTrafficMonitorEntry->key.isIpv4 = simple_strtol(tokptr, NULL, 0);

		/* IP addr */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {kfree(pTrafficMonitorEntry); return -EFAULT;}
		if(pTrafficMonitorEntry->key.isIpv4)//IPv4 address
		{
			pTrafficMonitorEntry->key.ipInfo.daddr= in_aton(tokptr);
		}
		else//IPv6 address
		{
			in6_pton(tokptr, -1, (void *)&(pTrafficMonitorEntry->key.ipInfo.ip6.ip6Daddr), -1, NULL);			
		}

		/* netmask */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {kfree(pTrafficMonitorEntry); return -EFAULT;}
		netmask = simple_strtol(tokptr, NULL, 0);
		pTrafficMonitorEntry->key.netmask = netmask;

		if(0 == pTrafficMonitorEntry->key.isIpv4)
		{
			netfilterIpv6Net2Mask(pTrafficMonitorEntry->key.netmask, &(pTrafficMonitorEntry->key.ipInfo.ip6.ip6Dmask));
		}

		/* url */
		tokptr = strsep(&strptr, "\n");
		if (!tokptr) {
			strcpy(pTrafficMonitorEntry->key.url, "na");
		} else {
			strcpy(pTrafficMonitorEntry->key.url, tokptr);
		}

		if(g_traffic_monitor_debug){
			if(pTrafficMonitorEntry->key.isIpv4){
				traffic_monitor_printk("The url of  instNum=%d isIpv4=%d pTrafficMonitorEntry->key.url=%s addr=%pI4/%d\n", 
										instNum, pTrafficMonitorEntry->key.isIpv4, pTrafficMonitorEntry->key.url,
										&pTrafficMonitorEntry->key.ipInfo.daddr, pTrafficMonitorEntry->key.netmask);
			}
			else{
				traffic_monitor_printk("The url of  instNum=%d isIpv4=%d pTrafficMonitorEntry->key.url=%s addr=%pI6/%d\n", 
										instNum, pTrafficMonitorEntry->key.isIpv4, pTrafficMonitorEntry->key.url,
										&pTrafficMonitorEntry->key.ipInfo.ip6.ip6Daddr, pTrafficMonitorEntry->key.netmask);				
			}
		}
		
        //Found the entry exist or not
		list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		{
			if(node->key.isIpv4 != pTrafficMonitorEntry->key.isIpv4)
				continue;
				
			if(!memcmp (&pTrafficMonitorEntry->key, &node->key, sizeof(node->key))){

				if(g_traffic_monitor_debug){
					if(pTrafficMonitorEntry->key.isIpv4){
						traffic_monitor_printk("The Entry already exist of instNum=%d url=%s addr=%pI4/%d\n", 
								instNum, pTrafficMonitorEntry->key.url, &pTrafficMonitorEntry->key.ipInfo.daddr, 
								pTrafficMonitorEntry->key.netmask);						
					}
					else{			
						traffic_monitor_printk("The Entry already exist of instNum=%d url=%s addr=%pI6/%d\n", 
								instNum, pTrafficMonitorEntry->key.url, &pTrafficMonitorEntry->key.ipInfo.ip6.ip6Daddr, 
								pTrafficMonitorEntry->key.netmask);
					}
				}

				kfree(pTrafficMonitorEntry);
				return count;
			}
		}
		spin_lock(&traffic_monitor_unres_lock);
		list_add_tail(&(pTrafficMonitorEntry->list),&(traffic_montior_entry_list.list));
		spin_unlock(&traffic_monitor_unres_lock);
	}
	return count;
}

static int proc_traffic_monitor_mod_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[512] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	int instNum, netmask;
	traffic_montior_entry_t *pTrafficMonitorEntry;
	traffic_montior_entry_t *node = NULL; 
	
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		if(g_traffic_monitor_debug){
			traffic_monitor_printk("%s:%d, input parameters: \"%s\"\n", __FUNCTION__, __LINE__, tmpbuf);
		}

		pTrafficMonitorEntry = kmalloc(sizeof(traffic_montior_entry_t), GFP_ATOMIC);
		if (!pTrafficMonitorEntry)
			return -1;
		memset(pTrafficMonitorEntry, 0, sizeof(traffic_montior_entry_t));

		strptr = tmpbuf;
		/* instNum */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {kfree(pTrafficMonitorEntry); return -EFAULT;}
		instNum = simple_strtol(tokptr, NULL, 0);
		pTrafficMonitorEntry->key.instNum = instNum;

		/* IP version */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {kfree(pTrafficMonitorEntry); return -EFAULT;}
		pTrafficMonitorEntry->key.isIpv4 = simple_strtol(tokptr, NULL, 0);

		/* IP addr */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {kfree(pTrafficMonitorEntry); return -EFAULT;}
		if(pTrafficMonitorEntry->key.isIpv4)//IPv4 address
		{
			pTrafficMonitorEntry->key.ipInfo.daddr = in_aton(tokptr);
		}
		else//IPv6 address
		{
			in6_pton(tokptr, -1, (void *)&(pTrafficMonitorEntry->key.ipInfo.ip6.ip6Daddr), -1, NULL);		
		}

		/* netmask */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {kfree(pTrafficMonitorEntry); return -EFAULT;}
		netmask = simple_strtol(tokptr, NULL, 0);
		pTrafficMonitorEntry->key.netmask = netmask;
		
		if(0 == pTrafficMonitorEntry->key.isIpv4)
		{
			netfilterIpv6Net2Mask(pTrafficMonitorEntry->key.netmask, &(pTrafficMonitorEntry->key.ipInfo.ip6.ip6Dmask));
		}

		/* url */
		tokptr = strsep(&strptr, "\n");
		if (!tokptr)
			strcpy(pTrafficMonitorEntry->key.url, "na");
		else
			strcpy(pTrafficMonitorEntry->key.url, tokptr);

		if(g_traffic_monitor_debug){
			if(pTrafficMonitorEntry->key.isIpv4){
				traffic_monitor_printk("The url of  instNum=%d isIpv4=%d pTrafficMonitorEntry->key.url=%s addr=%pI4/%d\n", 
										instNum, pTrafficMonitorEntry->key.isIpv4, pTrafficMonitorEntry->key.url,
										&pTrafficMonitorEntry->key.ipInfo.daddr, pTrafficMonitorEntry->key.netmask);
			}
			else{
				traffic_monitor_printk("The url of  instNum=%d isIpv4=%d pTrafficMonitorEntry->key.url=%s addr=%pI6/%d\n", 
										instNum, pTrafficMonitorEntry->key.isIpv4, pTrafficMonitorEntry->key.url,
										&pTrafficMonitorEntry->key.ipInfo.ip6.ip6Daddr, pTrafficMonitorEntry->key.netmask);				
			}
		}
			
        //Found the entry exist or not
		list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		{
			if(pTrafficMonitorEntry->key.instNum == node->key.instNum ){
				if(strcmp(pTrafficMonitorEntry->key.url, "na") &&
					!strcmp(pTrafficMonitorEntry->key.url, node->key.url))
				{
					node->key.isIpv4 = pTrafficMonitorEntry->key.isIpv4;
					if(pTrafficMonitorEntry->key.isIpv4)
					{
						node->key.ipInfo.daddr = pTrafficMonitorEntry->key.ipInfo.daddr;
					}
					else //ipv6
					{
						memcpy(&(node->key.ipInfo.ip6), &(pTrafficMonitorEntry->key.ipInfo.ip6), 
								sizeof(node->key.ipInfo.ip6));
					}
					
					node->key.netmask = pTrafficMonitorEntry->key.netmask;			
	
					if(g_traffic_monitor_debug){
						if(node->key.isIpv4){
							traffic_monitor_printk("Modify instNum=%d ip_addr=%pI4 netmask=%d url=%s\n", 
													instNum, &pTrafficMonitorEntry->key.ipInfo.daddr, 
													pTrafficMonitorEntry->key.netmask, pTrafficMonitorEntry->key.url);
						}
						else{
							traffic_monitor_printk("Modify instNum=%d ip_addr=%pI6 netmask=%d url=%s\n", 
													instNum, &pTrafficMonitorEntry->key.ipInfo.ip6.ip6Daddr, 
													pTrafficMonitorEntry->key.netmask, pTrafficMonitorEntry->key.url);
						}
					}
					
					kfree(pTrafficMonitorEntry);
					return count;
				}
			}
		}
	}
	return count;
}

static int proc_traffic_monitor_read(struct seq_file *seq, void *v)
{
	traffic_montior_entry_t *node = NULL; 

	printk("SYNOPSIS:\n");
	printk("         echo \"[instNum] [isIPv4] [IP] [netmask] [url]\" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_ADD);
	printk("example  echo \"1 1 8.8.8.8 24\" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_ADD);
	printk("example  echo \"1 0 2001:db9::100 128 \" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_ADD);
	printk("\n");
	printk("DESCRIPTION:\n");
	printk("         Add/Modify the instNum and IP address to traffic monitor\n");
		
	list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
	{
		if(node->key.isIpv4){
	 		printk("instNum:%d isIPv4:%d daddr = %pI4/%d url:%s counter:%d timer:%d detect_flag:%d dispear_flag:%d \n",
					node->key.instNum, node->key.isIpv4, &(node->key.ipInfo.daddr), node->key.netmask, 
					node->key.url, node->counter, node->timer, node->detect_flag, node->dispear_flag); 
		}
		else{
	 		printk("instNum:%d isIPv4:%d daddr = %pI6/%d url:%s counter:%d timer:%d detect_flag:%d dispear_flag:%d \n",
					node->key.instNum, node->key.isIpv4, &(node->key.ipInfo.ip6.ip6Daddr), node->key.netmask, 
					node->key.url, node->counter, node->timer, node->detect_flag, node->dispear_flag); 			
		}		
	}
	return 0;
}

static int proc_traffic_monitor_del(struct seq_file *seq, void *v)
{
	traffic_montior_entry_t *node = NULL;

	printk("SYNOPSIS:\n");
	printk("         echo \"[instNum]\" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);
	printk("example  echo \"1\" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);
	printk("         echo \"[instNum] [isIPv4] [IP] [netmask] | [url]\" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);
	printk("example  echo \"1 1 8.8.8.8 24 \" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);	
	printk("example  echo \"1 0 2001:db9::100 128 \" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);	
	printk("example  echo \"1 www.google.com \" > /proc/dbus/%s\n", PROCFS_TRAFFIC_MONITOR_DEL);	
	printk("\n");
	printk("DESCRIPTION:\n");
	printk("         1.Delete the all IP address belong to the instNum from traffic monitor\n");
	printk("         2.Delete the instNum and IP address from traffic monitor\n");
	list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
	{
		if(node->key.isIpv4){
			printk("instNum:%d isIpv4:%d daddr = %pI4 netmask=%d url:%s\n", node->key.instNum, node->key.isIpv4, 
					&(node->key.ipInfo.daddr), node->key.netmask,  node->key.url);
		}
		else{
			printk("instNum:%d isIpv4:%d daddr = %pI6 netmask=%d url:%s\n", node->key.instNum, node->key.isIpv4, 
					&(node->key.ipInfo.ip6.ip6Daddr), node->key.netmask,	node->key.url);
		}
	} 
	return 0;
}


static int proc_traffic_monitor_debug(struct seq_file *seq, void *v)
{
	traffic_montior_entry_t *node = NULL;

	printk("traffic monitor debug: %d\n", g_traffic_monitor_debug);

	return 0;
}


static int proc_traffic_monitor_add_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_traffic_monitor_read, inode->i_private);
}

static int proc_traffic_monitor_mod_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_traffic_monitor_read, inode->i_private);
}

static int proc_traffic_monitor_del_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_traffic_monitor_del, inode->i_private);
}

static int proc_traffic_monitor_debug_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_traffic_monitor_debug, inode->i_private);
}

static int proc_traffic_monitor_debug_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[512] = "\0";
	
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		g_traffic_monitor_debug = simple_strtol(tmpbuf, NULL, 0);
	}
	return count;
}


static int proc_traffic_monitor_del_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[512] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	__be32 ip_addr;
	int no_ip_info = 0;
	struct in6_addr	ip6Daddr;
	int is_ip4 = -1;
	int instNum, netmask;
	int del_success = 0;
	traffic_montior_entry_t *node = NULL, *tmp = NULL;
	char url[128]={0};
	
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {	
		if(g_traffic_monitor_debug){
			traffic_monitor_printk("%s:%d, input parameters: \"%s\"\n", __FUNCTION__, __LINE__, tmpbuf);
		}

		strptr = tmpbuf;
		/* instNum */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) return -EFAULT;
		instNum = simple_strtol(tokptr, NULL, 0);

		/* isIPv4 */
		tokptr = strsep(&strptr, " ");
		if (!tokptr){
			is_ip4=-1;
		}
		else {
			is_ip4 = simple_strtol(tokptr, NULL, 0);
		}

		/* IP address */
		tokptr = strsep(&strptr, " ");
		if (!tokptr){
			no_ip_info=1;
		}
		else {
			no_ip_info=0;
			if(is_ip4 == 1)
				ip_addr=in_aton(tokptr);
			else if(is_ip4 == 0)
				in6_pton(tokptr, -1, (void *)&(ip6Daddr), -1, NULL);
		}

		/* netmask */
		tokptr = strsep(&strptr, " ");
		if (!tokptr) {
			netmask=0;
		} else {
			netmask = simple_strtol(tokptr, NULL, 0);
		}
		
		/* url */
		tokptr = strsep(&strptr, "\n");
		if (!tokptr) {
			strcpy(url, "");
		} else {
			strcpy(url, tokptr);
		}

		if(g_traffic_monitor_debug)
		{
			if(is_ip4){
				traffic_monitor_printk("Try to Delete instNum=%d isIpv4=%d ip_addr=%pI4 netmask=%d url=%s\n", 
						instNum, is_ip4, &ip_addr, netmask, url);
			}
			else{
				traffic_monitor_printk("Try to Delete instNum=%d isIpv4=%d ip_addr=%pI6 netmask=%d url=%s\n", 
						instNum, is_ip4, &ip6Daddr, netmask, url);
			}
		}

		list_for_each_entry_safe(node, tmp, &(traffic_montior_entry_list.list), list)
		{
			del_success = 0;
			
			if (node->key.instNum == instNum) {			
				if (no_ip_info == 1 && (!strcmp(url, "") || !strcmp(url, "na"))){ //Delete all list in instNum		
					if(g_traffic_monitor_debug){
						traffic_monitor_printk("Delete by instNum, ID = %d\n", instNum);
					}
					del_success = 1;

					list_del(&node->list);
	        		kfree(node);
				}
				else {
					if(node->key.isIpv4 != is_ip4){
						continue;
					}
					if(strcmp(url, "")&&strcmp(url, "na")){ //if url exist, check url only
						if(!strcmp(url, node->key.url)) {
							if(g_traffic_monitor_debug){
								traffic_monitor_printk("Delete by URL, instNum = %d, URL = %s\n", instNum, url);
							}
							del_success = 1;

							list_del(&node->list);
		        			kfree(node);
						}
					}
					else{
						if (((is_ip4 == 1)&&(ip_addr == node->key.ipInfo.daddr) && (netmask == node->key.netmask)) ||
							((is_ip4 == 0)&&(!memcmp(&ip6Daddr, &node->key.ipInfo.ip6.ip6Daddr, sizeof(struct in6_addr)))&&(netmask == node->key.netmask))){ //check ip address					
							if(g_traffic_monitor_debug){
								traffic_monitor_printk("Delete by IP, instNum = %d, URL = %s, isIpv4 = %d\n", instNum, url, is_ip4);	
							}
							del_success = 1;
							
							list_del(&node->list);
		        			kfree(node);
						}
					}
				}
			}
			if(g_traffic_monitor_debug&del_success){
				if(node->key.isIpv4){
					traffic_monitor_printk("Success Delete Entry: instNum=%d isIpv4=%d ip_addr=%pI4 netmask=%d url=%s\n",
							node->key.instNum, node->key.isIpv4, &(node->key.ipInfo.daddr), 
							node->key.netmask, node->key.url);
				}
				else{
					traffic_monitor_printk("Success Delete Entry: instNum=%d isIpv4=%d ip_addr=%pI6 netmask=%d url=%s\n",
							node->key.instNum, node->key.isIpv4, &(node->key.ipInfo.ip6.ip6Daddr), 
							node->key.netmask, node->key.url);
				}			
			}	
		}	
	}
	return count;
}

static struct mfc_cache *traffic_monitor_seq_idx(loff_t pos)
{
	traffic_montior_entry_t *node = NULL;
	list_for_each_entry(node, &(traffic_montior_entry_list.list), list)
		if (pos-- == 0)
                	return node;
	return NULL;
}

static void *traffic_monitor_seq_start(struct seq_file *s, loff_t *pos) 
{ 
#if __DEBUG
	traffic_monitor_printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos);		 
#endif
	return *pos ? traffic_monitor_seq_idx(*pos - 1)	: SEQ_START_TOKEN;
}
static void *traffic_monitor_seq_next(struct seq_file *s, void *v, loff_t *pos) 
{ 
	++*pos;
#if __DEBUG
	traffic_monitor_printk("[%s:%d] pos=%Ld \n", __FUNCTION__, __LINE__, *pos); 	 
#endif
	return traffic_monitor_seq_start(s, pos);
} 

static void traffic_monitor_seq_stop(struct seq_file *s, void *v) 
{ 
#if __DEBUG
	traffic_monitor_printk("[%s:%d] \n", __FUNCTION__, __LINE__); 	 
#endif	
} 

static int traffic_monitor_seq_show(struct seq_file *seq, void *v) 
{ 
	int n;
	
	if (v == SEQ_START_TOKEN) {
		seq_puts(seq,  "id IP netmask detect dispear url\n");
	} 
	else {
		struct timespec now;
		long curr_time;		
		traffic_montior_entry_t *node = v;
		now = current_kernel_time();
		curr_time = now.tv_sec;
		
		/* Traffic dispear , reset the counter*/
		if ( node->counter !=0 && ((curr_time - node->timer) > TRAFFIC_MONITOR_DISAPPEAR_TIMEOUT) ) {
			node->counter = 0;
			node->timer = 0;
			node->dispear_flag = TRUE; 
		}

		if(node->key.isIpv4){
			seq_printf(seq, "%d %d %pI4 %d %d %d %s\n",
					node->key.instNum, node->key.isIpv4, &(node->key.ipInfo.daddr), node->key.netmask, 
					node->detect_flag, node->dispear_flag, node->key.url);
		}
		else{
			seq_printf(seq, "%d %d %pI6 %d %d %d %s\n",
					node->key.instNum, node->key.isIpv4, &(node->key.ipInfo.ip6.ip6Daddr), node->key.netmask, 
					node->detect_flag, node->dispear_flag, node->key.url);
		}
		
		//traffic_monitor_printk("node->detect_flag=%d node->dispear_flag=%d\n", node->detect_flag, node->dispear_flag);
		/* This two flag used for sending OSGI event ,it should be clean after read */
		if (node->detect_flag) node->detect_flag = FALSE;
		if (node->dispear_flag) node->dispear_flag = FALSE;		
		
	}
	return 0;
}

static struct seq_operations traffic_monitor_counter_ops = { 
	.start			= traffic_monitor_seq_start,
	.next			= traffic_monitor_seq_next,
	.stop			= traffic_monitor_seq_stop,
	.show			= traffic_monitor_seq_show 
};

static int traffic_monitor_counter_open(struct inode *inode, struct file *file) 
{ 
	return seq_open(file, &traffic_monitor_counter_ops);
};

static struct file_operations traffic_monitor_counter_fops = { 
	.owner 			= THIS_MODULE,
	.open			= traffic_monitor_counter_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= seq_release 
};

static const struct file_operations traffic_monitor_add_fops = {
    .owner          = THIS_MODULE,
    .open           = proc_traffic_monitor_add_open,
    .read           = seq_read,
    .write          = proc_traffic_monitor_add_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static const struct file_operations traffic_monitor_mod_fops = {
    .owner          = THIS_MODULE,
    .open           = proc_traffic_monitor_mod_open,
    .read           = seq_read,
    .write          = proc_traffic_monitor_mod_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};


static const struct file_operations traffic_monitor_del_fops = {
    .owner          = THIS_MODULE,
    .open           = proc_traffic_monitor_del_open,
    .read           = seq_read,
    .write          = proc_traffic_monitor_del_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static const struct file_operations traffic_monitor_debug_fops = {
    .owner          = THIS_MODULE,
    .open           = proc_traffic_monitor_debug_open,
    .read           = seq_read,
    .write          = proc_traffic_monitor_debug_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static void traffic_montior_entry_list_init(void)
{
	memset(&traffic_montior_entry_list, 0, sizeof(traffic_montior_entry_list));
    traffic_montior_entry_list.counter = -1;
    INIT_LIST_HEAD(&traffic_montior_entry_list.list);
}


static int __init init_main(void)
{
	struct proc_dir_entry *entry = NULL;
	
	proc_dir = proc_mkdir(PROCFS_DBUS_DIR, NULL);
	if ( proc_dir == NULL){
		printk("%s: Register to /proc/%s failed \n", __FUNCTION__, PROCFS_DBUS_DIR);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_ADD, 0644, proc_dir, &traffic_monitor_add_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_DBUS_DIR, PROCFS_TRAFFIC_MONITOR_ADD);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_MOD, 0644, proc_dir, &traffic_monitor_mod_fops, NULL);
	if ( entry == NULL){
		traffic_monitor_printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_DBUS_DIR, PROCFS_TRAFFIC_MONITOR_MOD);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_DEL, 0644, proc_dir, &traffic_monitor_del_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_DBUS_DIR, PROCFS_TRAFFIC_MONITOR_DEL);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_DEBUG, 0644, proc_dir, &traffic_monitor_debug_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_DBUS_DIR, PROCFS_TRAFFIC_MONITOR_DEBUG);
	}

	entry = proc_create_data(PROCFS_TRAFFIC_MONITOR_COUNTER, 0444, proc_dir, &traffic_monitor_counter_fops, NULL);
	if ( entry == NULL){
		printk("%s: Register to /proc/%s/%s failed \n", __FUNCTION__, PROCFS_DBUS_DIR, PROCFS_TRAFFIC_MONITOR_COUNTER);
	}

	traffic_montior_entry_list_init();
	ipv4_monitor_nfho.hook = 	ipv4_traffic_monitor_hook;
	ipv4_monitor_nfho.hooknum = 	NF_INET_FORWARD;
	ipv4_monitor_nfho.pf = 		NFPROTO_IPV4;
	ipv4_monitor_nfho.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&ipv4_monitor_nfho);

	ipv6_monitor_nfho.hook = ipv6_traffic_monitor_hook;
	ipv6_monitor_nfho.hooknum = 	NF_INET_FORWARD;
	ipv6_monitor_nfho.pf = 		NFPROTO_IPV6;
	ipv6_monitor_nfho.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&ipv6_monitor_nfho);

	
	printk("%s: Successfully inserted traffic monitor hook into kernel\n", __FUNCTION__);

	return 0;
}

static void __exit cleanup_main(void)
{
	traffic_montior_entry_t *node = NULL, *tmp = NULL;
	list_for_each_entry_safe(node, tmp, &(traffic_montior_entry_list.list), list)
	{
        list_del(&node->list);
        kfree(node);		
	}

    remove_proc_entry(PROCFS_TRAFFIC_MONITOR_ADD, proc_dir);
	remove_proc_entry(PROCFS_TRAFFIC_MONITOR_MOD, proc_dir);
	remove_proc_entry(PROCFS_TRAFFIC_MONITOR_DEL, proc_dir);
	remove_proc_entry(PROCFS_TRAFFIC_MONITOR_COUNTER, proc_dir);
	remove_proc_entry(PROCFS_TRAFFIC_MONITOR_DEBUG, proc_dir);
	remove_proc_entry(PROCFS_DBUS_DIR, NULL);
	
	nf_unregister_hook(&ipv4_monitor_nfho);
	nf_unregister_hook(&ipv6_monitor_nfho);
	
	printk("Successfully unloaded traffic monitor hook\n");
}

module_init(init_main);
module_exit(cleanup_main);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

