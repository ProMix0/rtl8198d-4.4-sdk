#ifndef __RTK_DNS_FILTER_H__
#define __RTK_DNS_FILTER_H__

#define DNS_KEY_TABLE	1
#define RTK_DNS_FILTER_CONTENT_LEN 32
#define RTK_DNS_FILTER_MAX_NUM 32
#define RTK_DNS_FILTER_MAC_LENTH 3*6+1
#define RTK_DNS_FILTER_MAC_NUM 3
#define RTK_DNS_FILTER_RULE_TOTAL_LEN 60+RTK_DNS_FILTER_MAC_LENTH*RTK_DNS_FILTER_MAC_NUM+RTK_DNS_FILTER_CONTENT_LEN*RTK_DNS_FILTER_MAX_NUM

#define DNS_FILTER_MODE_BLACK	0
#define DNS_FILTER_MODE_WHITE	1
#define ETH_ADDR_LEN 6

#define RTK_DNS_FILTER_LIST "rtk_dns_filter_list"
#define RTK_DNS_FILTER_ENABLE "rtk_dns_filter_enable"

#define RTL_PS_BR0_DEV_NAME "br0"
typedef struct _rtk_dns_filter
{
	struct list_head rule_list;
	unsigned int listmode;//0:blacklist 1:blacklist
	unsigned int action; //0:drop ;1:response br0 ip; 2:rcode:no server name
	char macAddr[RTK_DNS_FILTER_MAC_LENTH*RTK_DNS_FILTER_MAC_NUM];
	char data[RTK_DNS_FILTER_CONTENT_LEN*RTK_DNS_FILTER_MAX_NUM];
}rtk_dns_filter_entry;

int rtk_dns_filter_init(void);
void rtk_dns_filter_exit(void);
int rtk_filter_dns_query(struct sk_buff **pskb);

#endif

