#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/skbuff.h>
#include <linux/version.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/inetdevice.h>
#include <net/checksum.h>
#include <net/udp.h>
#include <linux/ctype.h>

#include <soc/cortina/rtk_common_utility.h>
#include "rtk_dns_filter.h"
#include <soc/cortina/rtk_dns_trap.h>

#ifdef CONFIG_IPV6
#include <linux/in6.h>
#include <net/if_inet6.h>
#include <net/addrconf.h>
#endif

int dns_filter_mode = 0;
int rtk_action_for_whitelist = 0;
int rtk_dns_filter_enable = 0;

static LIST_HEAD(rule_list_head);
static DEFINE_RWLOCK(filter_lock);

static int get_dev_ip_mask(const char * name, unsigned int *ip, unsigned int *mask)
{
	struct in_device *in_dev;
	struct net_device *landev;
	struct in_ifaddr *ifap = NULL;
	
	if((name == NULL) || (ip==NULL) || (mask == NULL)) 
	{
		return -1;
	}
	
	if ((landev = __dev_get_by_name(&init_net, name)) != NULL)
	{
		in_dev=(struct in_device *)(landev->ip_ptr);
		if (in_dev != NULL) 
		{
			for (ifap=in_dev->ifa_list; ifap != NULL; ifap=ifap->ifa_next)
			{
				if (strcmp(name, ifap->ifa_label) == 0)
				{
					*ip = ifap->ifa_address;
					*mask = ifap->ifa_mask;
					return 0;
				}
			}

		}
	}

	return -1;
}

static int rtk_calc_filter_num(int  *filter_num)
{
	struct list_head *lh;
	rtk_dns_filter_entry *entry=NULL;
	int num = 0;

	if(filter_num == NULL){
		return -1;
	}
	
	read_lock(&filter_lock);
	list_for_each(lh, &rule_list_head)
	{		
		entry=list_entry(lh, rtk_dns_filter_entry, rule_list);
		if(entry){
			num++;		
		}
	}
	read_unlock(&filter_lock);

	*filter_num = num;

	return 0;
}

static int should_block_domain_name(struct sk_buff *skb,char* domain_name,int *ctcapd_flag)
{	
	struct list_head *lh;
	rtk_dns_filter_entry *entry=NULL;
	int found = 0;
	int rule_mode = 0;
	
	unsigned char *skb_mac = NULL;
	char dmacAddr[3*ETH_ADDR_LEN+1]={'\0'};

	if(domain_name == NULL || skb==NULL || ctcapd_flag==NULL){
		return 0;
	}

	skb_mac = skb_mac_header(skb);

	if(skb_mac==NULL){
		return 0;
	}
	
	snprintf(dmacAddr,sizeof(dmacAddr),"%02X:%02X:%02X:%02X:%02X:%02X", 
		skb_mac[0],skb_mac[1],skb_mac[2],
		skb_mac[3],skb_mac[4],skb_mac[5]);

	read_lock(&filter_lock);
	list_for_each(lh, &rule_list_head)
	{		
		entry=list_entry(lh, rtk_dns_filter_entry, rule_list);	
		
		if(entry){
			if((strstr(entry->macAddr,dmacAddr)!=0)||(!strncmp(entry->macAddr,"all",3)))
			{
				//printk("111 %s %d entryname:%s domain_name:%s\n",__FUNCTION__, __LINE__,entry->data,domain_name);
				if(strstr(entry->data,domain_name)){
					*ctcapd_flag = entry->action;
					rule_mode = entry->listmode;
					found =1;
					break;
				}
			}
		}
	}
	read_unlock(&filter_lock);

	if(found)
	{
		if(!rule_mode)
		{
			//match blacklist rule need modify
			return 1;
		}
		else
		{	
			//match whitelist rule dont need modify
			return 0;
		}
	}
	else{
		if(!dns_filter_mode){
			//blacklist but not match rule dont need modify
			return 0;
		}
		else{
			//whitelist but not match rule need modify
			return 1;
		}
	}
	
}

int modifyDnsReply(struct sk_buff *skb, int ctcapd_flag)
{
    struct iphdr *iph;
    struct udphdr *udph;
    unsigned char *data;
	int record_num = 0;
	unsigned char change_flag = 0;
    int i = 0;
	unsigned int br0_ip;
	unsigned int br0_mask;
	unsigned short dns_hdr_flag;
	int offset = ETH_ADDR_LEN*2+2;
	unsigned short typeT,data_len;
	dnsheader_t *dns_hdr = NULL;

	if(skb == NULL){
		return -1;
	}

	if(skb->protocol==htons(ETH_P_8021Q))
		offset+=4;
	
	iph=(struct iphdr *)(skb->data+offset);

	if(iph==NULL)
		return -1;

    udph=(void *) iph + iph->ihl*4;

	if(udph==NULL)
		return -1;
	
	dns_hdr = (dnsheader_t*)((void*)udph + sizeof(struct udphdr));

    data = (void *)udph + sizeof(struct udphdr) + 12;

	if(dns_hdr==NULL || data==NULL)
		return -1;

    if (ntohs(udph->source) != 53) // DNS response
	{
		return 0;
    }
	if(dns_hdr == NULL)
	{
		return -1;
	}
	if(((dns_hdr->u & 0x8000)>>15) != 1)/*Not an answer*/
	{
		return 0;
	}
	record_num = ntohs(dns_hdr->ancount);
	if(record_num <= 0)
	{
		return -1;
	}
	while(*data){
		data ++;
	}//query domain name end
	data += 5;//(1:to answer,type:2byte ,class:2byte) to answer
	do 
	{
		data += 2;//(domain name pointer,2byte)current:type
		typeT = ntohs((*(unsigned short*)data));
		data += 8;//type:2bytes+class:2bytes+timetolive:4bytes,current:data_len
		data_len = ntohs(*(unsigned short*)data);
		data += 2;//(datalen:2byte),current:data
		//printk("%s %d ctcapd_flag:%d typeT:%d data_len:%d \n",__FUNCTION__, __LINE__, ctcapd_flag,typeT, data_len);
		if(typeT == 0x01 && data_len == 4)//IPV4 address
		{
			if(ctcapd_flag == 1 || (dns_filter_mode == 1 && rtk_action_for_whitelist == 1)){
				//action :1 reply modify ip->dut ip
				if(!get_dev_ip_mask(RTL_PS_BR0_DEV_NAME, &br0_ip, &br0_mask)){
					data[0] = (br0_ip&0xff000000)>>24;
					data[1] = (br0_ip&0x00ff0000)>>16;
					data[2] = (br0_ip&0x0000ff00)>>8;
					data[3] = (br0_ip&0x000000ff)>>0;
				}
				else{
					printk("get ip fail!\n");
					return -1;
				}
			}	
			else if(ctcapd_flag == 2 || (dns_filter_mode == 1 && rtk_action_for_whitelist == 2))
			{
				//action :2 reply dnshdr->rcode=3 name error
				dns_hdr_flag = ntohs(dns_hdr->u);
				dns_hdr_flag |= 0x3;
				dns_hdr->u = htons(dns_hdr_flag);
			}
			else{
				//action :0 drop pkts
				memset(data,0,4);
			}

			#if 0
			printk("dut ip:%x ip:%x.%x.%x.%x flag:%d dns_hdr:%x\n",br0_ip,
									data[0],
									data[1],
									data[2],
									data[3],ctcapd_flag,dns_hdr->u);
			#endif
			
			change_flag = 1;
		}
		else if(typeT == 0x1c && data_len == 16)//IPV6 address
		{
			memset(data,0,16);
			change_flag = 1;
		}
		data += data_len;//to next record
		i ++;
	}while(i < record_num);
	if(change_flag)
	{
		/* ip checksum */
		skb->ip_summed = CHECKSUM_NONE;
		iph->check = 0;
		iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
		
		/* udp checksum */
		udph->check = 0;
		udph->check = csum_tcpudp_magic(iph->saddr, iph->daddr,
						ntohs(udph->len), IPPROTO_UDP,
						csum_partial((char *)udph,
									 ntohs(udph->len), 0));
	}
    return 0;
}

int rtk_filter_dns_query(struct sk_buff **pskb)
{
//	printk("%s------->%d\n",__FUNCTION__,__LINE__);
	struct iphdr *iph;
	struct udphdr *udph;
	unsigned char *data;
	int datalen;
	char name[256];
	int offset = ETH_ADDR_LEN*2+2;
	int ctcapd_filter_flag = 0;
	struct sk_buff *skb = NULL;
	int filter_number = 0;

	if (!pskb || !*pskb){
		return -1;
	}

	rtk_calc_filter_num(&filter_number);
	//dns filter disable and blacklist case don't filter dns packets
	if(rtk_dns_filter_enable!=1 || (filter_number <= 0 && dns_filter_mode == DNS_FILTER_MODE_BLACK)){
		return 0;
	}
	
	skb = *pskb;
	
	if(skb->protocol == htons(ETH_P_8021Q)){
		offset += 4;
	}
	else if(skb->protocol != htons(ETH_P_IP)){
		return 0;
	}
	
	iph=(struct iphdr *)(skb->data+offset);

	if(iph==NULL)
		return -1;

	//only ipv4
	if(iph->protocol==IPPROTO_UDP && iph->version==4){
		udph = (void *)iph + iph->ihl*4;

		if(udph==NULL)
			return -1;
		
		data = (void *)udph + sizeof(struct udphdr) + 12;

		if(data==NULL)
			return -1;

		// DNS Query
		if (ntohs(udph->dest) != 53 && ntohs(udph->source) != 53){
			return 0;
		}
		/*************************************************************************************
 
	    RFC 1035 ( Chap 4.1 , p.25 ) :
	    All communications inside of domain protocol are carried in a single format called a MESSAGE,
	    The top level format of MESSAGE is divided into 5 sections shown below:

	            +---------------------+
	            |             Header              |
	            +---------------------+
	            |            Question             | the question for the name server
	            +---------------------+
	            |             Answer              | RRs answering the question
	            +---------------------+
	            |           Authority              | RRs pointing toward an authority
	            +---------------------+
	            |           Additional             | RRs holding additional information
	            +---------------------+

	    We would parse these sections step by step.
	 
	   	*************************************************************************************/
	 
	 
	    /*************************************************************************************
            [HEADER]

            RFC 1035 ( 4.1.1. , p.26-p.27 ) :

            The header contains the following fields:

                                                                            1    1    1    1    1    1
                   0    1    2    3    4    5    6    7    8    9    0    1    2    3    4    5
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                |                                            ID                                           |
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                |QR|      Opcode     |AA|TC|RD|RA|       Z      |      RCODE     |
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                |                                       QDCOUNT                                     |
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                |                                       ANCOUNT                                      |
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                |                                       NSCOUNT                                      |
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                |                                       ARCOUNT                                      |
                +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

            where:

            ID                      Identifier
            QR                      query (0), response (1).
            OPCODE          Define the kind of this message.
            AA                      Indicate if this message is sent by the AUTHORITY server for first replied query or not.
            TC                      Indicate if this packet is truncated or not.
            RD                      Request DNS server to pursue this query recursively
            RA                      Indicate if the DNS server support recursive process or not.
            Z                       reserved, must be 0.
            RCODE           return value: 0 means NO ERROR, others mean something wrong.
            QDCOUNT # of entries in the question section.
            ANCOUNT # of resource records in the answer section.
            NSCOUNT # of name server resource records in the authority records section.
            ARCOUNT # of resource records in the additional records section.

    	*************************************************************************************/
 
		datalen = ntohs(udph->len) - sizeof(struct udphdr) - 12; // 12 is ID+Flags+QDCOUNT+ANCOUNT+NSCOUNT+ARCOUNT
		memset(name, 0, sizeof(name));

		while (datalen > 0) {
		        if (*data) {
						if((strlen(name) + (*data)) >= (sizeof(name)-1))
							return 0;	
		                strncpy(name+strlen(name), data+1, *data);
		                datalen -= (*data + 1);
		                data += (*data + 1);
		                if (*data) {
		                        strcat(name, ".");
		                }
		        }
		        else
		                break;
		}

		if(should_block_domain_name(skb,name,&ctcapd_filter_flag))
		{
			modifyDnsReply(skb,ctcapd_filter_flag);

			return 0;
		}
	}
	
 	return 0;
}

int rtk_filter_table_head_init(void)
{
	dns_filter_mode = 0;
	
	return 0;
}

int rtk_filter_table_regist(struct list_head * new_item)
{
	if(new_item ==NULL)
		return 0;
	
	write_lock_bh(&filter_lock);
	list_add(new_item, &rule_list_head);
	write_unlock_bh(&filter_lock);
	
	return 0;
}

int rtk_filter_table_del(char *domain,char *macaddr)
{
	struct list_head *lh=NULL;
	struct list_head *lhp=NULL;
	rtk_dns_filter_entry *entry=NULL; 

	if(domain == NULL|| macaddr ==NULL)
		return 0;

	write_lock_bh(&filter_lock);
	list_for_each_safe(lh, lhp,&rule_list_head)
	{	
		entry = list_entry(lh, rtk_dns_filter_entry, rule_list);
		if(entry){
			if((!strncmp(entry->macAddr,macaddr,sizeof(entry->macAddr))) && (!strncmp(entry->data,domain,sizeof(entry->data)))){
				list_del(&entry->rule_list);
				kfree(entry);
			}
		}
	}
	write_unlock_bh(&filter_lock);

	return 0;
	
}

int rtk_filter_table_flush(void)
{
	struct list_head *lh=NULL;
	struct list_head *lhp=NULL;
	rtk_dns_filter_entry *entry=NULL; 

	write_lock_bh(&filter_lock);
	list_for_each_safe(lh, lhp,&rule_list_head)
	{	
		entry = list_entry(lh, rtk_dns_filter_entry, rule_list);
		if(entry){
				list_del(&entry->rule_list);
				kfree(entry);
		}
	}
	write_unlock_bh(&filter_lock);
	
	dns_filter_mode=0;
	
	return 0;
}

unsigned long str2hexnum(unsigned char *p)
{
	unsigned long val=0;
	unsigned long c=0;

	if(p == NULL)
		return -1;
	
	for(val=0;*p!='\0';val=(val<< 4 ) + c, p++)
	{
		c=*p;
		if(c >= '0' && c <='9')
			c=c-'0';
		else if(c >='a' && c <='f')
			c=c-'a'+10;
		else if(c >='A' &&  c<='F')
			c=c-'A'+10;
		else
			return -1;
	}
	
	return val;

}

static int rtk_dns_filter_table_write_proc(struct file *file, const char *buffer,
		      unsigned long count, void *data)
{
  	//char *tokptr=NULL,*tokptr1=NULL;
  	char *tokptr=NULL;
	char *tmpbuf=NULL;
	int ctcapd_filter_flag = 0;
	rtk_dns_filter_entry *entry;
	//int listmode = 0,i = 0;
	int listmode = 0;
	char ctcapd_dns_filter_mac[RTK_DNS_FILTER_MAC_LENTH*RTK_DNS_FILTER_MAC_NUM];
	//char dns_filter_domain_tmp[RTK_DNS_FILTER_CONTENT_LEN];
	unsigned char totalmode[2];

#if 1	// fix -Werror=frame-larger-than=
	unsigned int info_size = RTK_DNS_FILTER_RULE_TOTAL_LEN * sizeof(unsigned char);
	unsigned int domain_size = RTK_DNS_FILTER_CONTENT_LEN * RTK_DNS_FILTER_MAX_NUM * sizeof(char);

	unsigned char *ctcapd_get_info_from_usr_space = kmalloc(info_size, GFP_KERNEL);
	char *ctcapd_dns_filter_domain = kmalloc(domain_size, GFP_KERNEL);
#else
	unsigned char ctcapd_get_info_from_usr_space[RTK_DNS_FILTER_RULE_TOTAL_LEN];
	char ctcapd_dns_filter_domain[RTK_DNS_FILTER_CONTENT_LEN*RTK_DNS_FILTER_MAX_NUM];
#endif

	if(ctcapd_get_info_from_usr_space == NULL || ctcapd_dns_filter_domain == NULL)
		goto _err;
	
	memset(ctcapd_get_info_from_usr_space,0, info_size);
	memset(ctcapd_dns_filter_domain,0, domain_size);


	if(count>RTK_DNS_FILTER_RULE_TOTAL_LEN)
		goto _err;

    if (buffer && !copy_from_user(ctcapd_get_info_from_usr_space, buffer, count))
 	{
	    ctcapd_get_info_from_usr_space[count]='\0';	
		tmpbuf = ctcapd_get_info_from_usr_space;
		if(memcmp(tmpbuf,"init", strlen("init")) == 0)
		{
			rtk_filter_table_head_init();
		}
		else if(memcmp(tmpbuf,"flush", strlen("flush")) == 0)
		{	
			rtk_filter_table_flush();		
		}
		else if(memcmp(tmpbuf,"white", strlen("white")) == 0)
		{
			dns_filter_mode=DNS_FILTER_MODE_WHITE;
		}
		else if(memcmp(tmpbuf,"black", strlen("black")) == 0)
		{
			dns_filter_mode=DNS_FILTER_MODE_BLACK;
		}
		else if(memcmp(tmpbuf,"totalwhitemode:", strlen("totalwhitemode:")) == 0)
		{
			tmpbuf+=strlen("totalwhitemode:");
			if(!tmpbuf) goto _err;

			snprintf(totalmode,sizeof(totalmode),"%s",tmpbuf);
			if((listmode = str2hexnum(totalmode))==-1){
				printk("%s %d totalmode=%s\n",__FUNCTION__,__LINE__,totalmode);
				goto _err;
			}

			rtk_action_for_whitelist = listmode;
			//printk("%s %d rtk_action_for_whitelist=%d listmode:%d\n",__FUNCTION__,__LINE__,rtk_action_for_whitelist,listmode);
		}
		else if(memcmp(tmpbuf,"add:", strlen("add:")) == 0)
		{
			//printk("1:%s %d tmpbuf=%s\n",__FUNCTION__,__LINE__,tmpbuf);
			//add:1 2 www.baidu.com,http://www.douban.com c0:a8:01:65:27:a3
			tmpbuf+=strlen("add:");
			if(!tmpbuf) goto _err;
			
			//printk("2:%s %d tmpbuf=%s\n",__FUNCTION__,__LINE__,tmpbuf);
			//1 2 www.baidu.com,http://www.douban.com c0:a8:01:65:27:a3
			tokptr = strsep(&tmpbuf," ");
			if(!tmpbuf) goto _err;
			if((listmode= str2hexnum(tokptr))==-1)
				goto _err;

			//printk("3:%s %d tmpbuf=%s\n",__FUNCTION__,__LINE__,tmpbuf);
			//2 www.baidu.com,http://www.douban.com c0:a8:01:65:27:a3
			tokptr = strsep(&tmpbuf," ");
			if(!tmpbuf) goto _err;
			if((ctcapd_filter_flag= str2hexnum(tokptr))==-1)
				goto _err;

			//printk("4:%s %d tmpbuf=%s\n",__FUNCTION__,__LINE__,tmpbuf);
			//www.baidu.com,http://www.douban.com c0:a8:01:65:27:a3
			tokptr = strsep(&tmpbuf," ");
			if(!tmpbuf) goto _err;
			if(!snprintf(ctcapd_dns_filter_domain, domain_size,"%s",tokptr))
				goto _err;
			#if 0
			for (tokptr1 = strsep(&tokptr,","); tokptr1 != NULL;
			tokptr1 = strsep(&tokptr,","))
			{
				memset(dns_filter_domain_tmp,0,sizeof(dns_filter_domain_tmp));
				snprintf(dns_filter_domain_tmp,sizeof(dns_filter_domain_tmp),"%s",tokptr1);

				if(!strncmp(dns_filter_domain_tmp,"http://",7))
					for(i=7;i<sizeof(dns_filter_domain_tmp);i++)
						dns_filter_domain_tmp[i-7]=dns_filter_domain_tmp[i];
				if(!strncmp(dns_filter_domain_tmp,"https://",8))
					for(i=8;i<sizeof(dns_filter_domain_tmp);i++)
						dns_filter_domain_tmp[i-8]=dns_filter_domain_tmp[i];
				if(!strncmp(dns_filter_domain_tmp,"www.",4))
					for(i=4;i<sizeof(dns_filter_domain_tmp);i++)
						dns_filter_domain_tmp[i-4]=dns_filter_domain_tmp[i];

				strcat(ctcapd_dns_filter_domain,dns_filter_domain_tmp);
			}
			#endif

			//c0:a8:01:65:27:a3
			//printk("4:%s %d tmpbuf=%s\n",__FUNCTION__,__LINE__,tmpbuf);
			if(!snprintf(ctcapd_dns_filter_mac,sizeof(entry->macAddr),"%s",tmpbuf))
				goto _err;

			entry = kmalloc(sizeof(rtk_dns_filter_entry),GFP_KERNEL);		
			if(entry == NULL)
				goto _err;

			INIT_LIST_HEAD(&entry->rule_list);

			entry->listmode=listmode;
			entry->action	= ctcapd_filter_flag;
			snprintf(entry->data,sizeof(entry->data),"%s",ctcapd_dns_filter_domain);
			snprintf(entry->macAddr,sizeof(entry->macAddr),"%s",ctcapd_dns_filter_mac);

			//printk("=========%s %d data=%s macAddr=%s\n",__FUNCTION__,__LINE__,entry->data,entry->macAddr);
			rtk_filter_table_regist(&entry->rule_list);
		}
		else if(memcmp(tmpbuf,"del:", strlen("del:")) == 0){
			//printk("%s %d tmpbuf:%s\n",__FUNCTION__,__LINE__,tmpbuf);
			tmpbuf+=strlen("del:");
			if(!tmpbuf) goto _err;

			tokptr = strsep(&tmpbuf," ");
			if(!tmpbuf) goto _err;
			if(!snprintf(ctcapd_dns_filter_domain, domain_size,"%s",tokptr))
				goto _err;
			
			if(!snprintf(ctcapd_dns_filter_mac,sizeof(ctcapd_dns_filter_mac),"%s",tmpbuf))
				goto _err;

			rtk_filter_table_del(ctcapd_dns_filter_domain, ctcapd_dns_filter_mac);
		}

		if (ctcapd_get_info_from_usr_space)
			kfree(ctcapd_get_info_from_usr_space);

		if (ctcapd_dns_filter_domain)
			kfree(ctcapd_dns_filter_domain);

		return count;
	}
_err:	

	if (ctcapd_get_info_from_usr_space)
		kfree(ctcapd_get_info_from_usr_space);

	if (ctcapd_dns_filter_domain)
		kfree(ctcapd_dns_filter_domain);

	printk("write filter rule error\n");
	return -1;
}

static int rtk_dns_filter_table_read_proc(struct seq_file *s, void *v)
{
	struct list_head *lh;
	rtk_dns_filter_entry *entry=NULL;
	int filter_number = 0;

	rtk_calc_filter_num(&filter_number);
	
	if(dns_filter_mode){
		seq_printf(s, "total mode=%d total_action:%d rule_num=%d\n",dns_filter_mode, rtk_action_for_whitelist, filter_number);
	}
	else{
		seq_printf(s, "total mode=%d rule_num=%d\n",dns_filter_mode,filter_number);
	}

	read_lock(&filter_lock);
	list_for_each(lh, &rule_list_head)
	{		
		entry=list_entry(lh, rtk_dns_filter_entry, rule_list);
		if(entry){	
			seq_printf(s, "listmode=%d action=%d domain=%s mac=%s\n",entry->listmode,entry->action,entry->data,entry->macAddr);
		}
	}
	read_unlock(&filter_lock);
	
    return 0;
}

static int rtk_dns_filter_enable_write_proc(struct file *file, const char *buffer,
		      unsigned long count, void *data)
{
  	unsigned char ctcapd_get_info_from_usr_space[32];
	unsigned char *tmpbuf=NULL;

	memset(ctcapd_get_info_from_usr_space,0,sizeof(ctcapd_get_info_from_usr_space));
	
    if (buffer && !copy_from_user(&ctcapd_get_info_from_usr_space, buffer, count))
 	{
	    ctcapd_get_info_from_usr_space[count]='\0';	
		tmpbuf = ctcapd_get_info_from_usr_space;
		if(memcmp(tmpbuf,"enable", strlen("enable")) == 0)
		{
			rtk_dns_filter_enable = 1;
		}
		else if(memcmp(tmpbuf,"disable", strlen("disable")) == 0){
			rtk_dns_filter_enable = 0;
		}

		return count;
	}
	
	return -1;
}

static int rtk_dns_filter_enable_read_proc(struct seq_file *s, void *v)
{

	if(rtk_dns_filter_enable == 1)
		seq_printf(s, "dns_filter_enable:%s\n", "enable");
	else
		seq_printf(s, "dns_filter_enable:%s\n", "disable");

    return 0;
}

int rtk_dns_filter_table_single_open(struct inode *inode, struct file *file)
{
        return(single_open(file, rtk_dns_filter_table_read_proc, NULL));
}

int rtk_dns_filter_table_single_write(struct file * file, const char __user * userbuf,
		     size_t count, loff_t * off)
{
	return rtk_dns_filter_table_write_proc(file, userbuf,count, off);
}

struct file_operations rtk_dns_filter_table_proc_fops = {
        .open           = rtk_dns_filter_table_single_open,
	 	.write		= rtk_dns_filter_table_single_write,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

int rtk_dns_filter_enable_single_open(struct inode *inode, struct file *file)
{
        return(single_open(file, rtk_dns_filter_enable_read_proc, NULL));
}

int rtk_dns_filter_enable_single_write(struct file * file, const char __user * userbuf,
		     size_t count, loff_t * off)
{
	return rtk_dns_filter_enable_write_proc(file, userbuf,count, off);
}

struct file_operations rtk_dns_filter_enable_proc_fops = {
        .open           = rtk_dns_filter_enable_single_open,
	 	.write		= rtk_dns_filter_enable_single_write,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static void rtk_dns_filter_create_proc(void)
{
	if(rtk_proc_dir==NULL)
		rtk_proc_dir=proc_mkdir("driver/realtek", NULL);

	rtk_filter_table_head_init();
	if(rtk_proc_dir)
	{
		proc_create(RTK_DNS_FILTER_LIST, 0, rtk_proc_dir, &rtk_dns_filter_table_proc_fops);	
		proc_create(RTK_DNS_FILTER_ENABLE, 0, rtk_proc_dir, &rtk_dns_filter_enable_proc_fops);
	}	
}
static void rtk_dns_filter_destroy_proc(void)
{
	if(rtk_proc_dir)
	{	
		remove_proc_entry(RTK_DNS_FILTER_LIST, rtk_proc_dir);	
		remove_proc_entry(RTK_DNS_FILTER_ENABLE, rtk_proc_dir);
	}
}

int __init rtk_dns_filter_init(void)
{
	rtk_dns_filter_create_proc();

	dns_filter_mode = 0;

	return 0;
}

void __exit rtk_dns_filter_exit(void)
{
	rtk_dns_filter_destroy_proc();
}


