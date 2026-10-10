#ifndef CA_NI_TEST_H
#define CA_NI_TEST_H

#define RTK_NI_TEST_MAX_TX_API_NUM				5
#define RTK_NI_TEST_MAX_TXINFO_NUM				5
#define RTK_NI_TEST_MAX_IPV6_EXTENTION_HDR_NUM	8

typedef struct {
	unsigned int count;
	unsigned int flag;
	unsigned int frame_len;
	unsigned char sa[6];
	unsigned int reserve0;
	unsigned char da[6];
	unsigned int reserve1;
	unsigned short ethertype;
	unsigned char ipver; //0: none, 1:v4, 2:v6
	unsigned int ipv4_sip;
	unsigned int ipv4_dip;
	unsigned short ipv4_flags_offset;
	unsigned char ipv4_option_type;
	unsigned char ipv4_option_length;
	unsigned char ipv6_sip[16];
	unsigned char ipv6_dip[16];
	unsigned int ipv6_extension_header_num;
	unsigned int ipv6_extension_header_index;
	unsigned char ipv6_extension_header_type[RTK_NI_TEST_MAX_IPV6_EXTENTION_HDR_NUM];
	unsigned char ipv6_extension_header_length[RTK_NI_TEST_MAX_IPV6_EXTENTION_HDR_NUM];
	unsigned int cvlan_vid;
	unsigned char cvlan_pri;
	unsigned int svlan_vid;
	unsigned char svlan_pri;
	unsigned char l4proto;
	unsigned char l4proto_tcp_data_offset;
	unsigned char l4proto_tcp_flag;
	struct pppoe_hdr *pppoehdr;
	struct rtk_ni_test_ppp_header_t *ppphdr;
	struct iphdr *iphdr;
	struct rtk_ni_test_ip_option_t *ipopt; 
	struct ipv6hdr *ipv6hdr;
	struct tcphdr *tcphdr;
	struct udphdr *udphdr;
	struct icmphdr *icmphdr;
	struct icmp6hdr *icmp6hdr;
	struct igmphdr *igmphdr;
	unsigned short l3cs_answer;
	unsigned short l4cs_answer;
	unsigned int crc_answer;
} rtk_ni_test_local2tx_t;

typedef struct rtk_ni_test_txinfo {
	ca_ni_tx_config_t tx_info;
	unsigned int tx_info_flag;
} rtk_ni_test_txinfo_t;

struct rtk_ni_test_private {
	unsigned int debug_level;
	unsigned int debug_times;
	unsigned int rx2tx;
	unsigned int rx2txl34csreset;
	spinlock_t	rtk_ni_test_lock;
	nic_hook_private_t rx_info;
	rtk_ni_test_txinfo_t tx_info[RTK_NI_TEST_MAX_TXINFO_NUM];
	rtk_ni_test_local2tx_t local2tx;
	netdev_tx_t (*tx_api[RTK_NI_TEST_MAX_TX_API_NUM]) (struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config);
	unsigned int tx_api_idx;
};

struct rtk_ni_test_ppp_header_t {
	__be16 ppp_proto;
	char payload[0];
};

struct rtk_ni_test_ip_option_t {
	unsigned char type;
	unsigned char length;
	unsigned char data[0];
};

struct rtk_ni_test_ipv6_extension_header_t {
	unsigned char type;
	unsigned char length;
	unsigned char padding[6];
	unsigned char data[0];
};

enum RTK_NI_TEST_DEBUG_LEVEL{
	RTK_NI_TEST_PRINT_NOTHING = 0,
	RTK_NI_TEST_SKB_RX = (1<<0),
	RTK_NI_TEST_SKB_TX = (1<<1),
	RTK_NI_TEST_RXINFO = (1<<2),
	RTK_NI_TEST_TXINFO = (1<<3),
	RTK_NI_TEST_RX_TRACE = (1<<4),
	RTK_NI_TEST_TX_TRACE = (1<<5),	
	RTK_NI_TEST_RX_WARN = (1<<6),	
	RTK_NI_TEST_TX_WARN = (1<<7),
	RTK_NI_TEST_OTHERS = (1<<8),
	RTK_NI_TEST_RX_ALL = (RTK_NI_TEST_SKB_RX|RTK_NI_TEST_RXINFO|RTK_NI_TEST_RX_TRACE),
	RTK_NI_TEST_TX_ALL = (RTK_NI_TEST_SKB_TX|RTK_NI_TEST_TXINFO|RTK_NI_TEST_TX_TRACE),
};

enum RTK_NI_TEST_MEM_DUMP_FORMAT{
	RTK_NI_TEST_MEM_DUMP_FORMAT_ADDR = (1<<0),
	RTK_NI_TEST_MEM_DUMP_FORMAT_HEX = (1<<1),
	RTK_NI_TEST_MEM_DUMP_FORMAT_TEXT = (1<<2),
	RTK_NI_TEST_MEM_DUMP_FORMAT_ALL = (RTK_NI_TEST_MEM_DUMP_FORMAT_ADDR | RTK_NI_TEST_MEM_DUMP_FORMAT_HEX | RTK_NI_TEST_MEM_DUMP_FORMAT_TEXT),
};

enum RTK_NI_TEST_L4PROTO{
	RTK_NI_TEST_L4PROTO_TCP = 6,
	RTK_NI_TEST_L4PROTO_UDP = 17,
	RTK_NI_TEST_L4PROTO_ICMP = 1,
	RTK_NI_TEST_L4PROTO_ICMPV6 = 58,
	RTK_NI_TEST_L4PROTO_IGMP = 2,
};

enum RTK_NI_TEST_L4PROTO_TCP_FLAG{
	RTK_NI_TEST_L4PROTO_TCP_FLAG_CWR = 0,
	RTK_NI_TEST_L4PROTO_TCP_FLAG_ECE,
	RTK_NI_TEST_L4PROTO_TCP_FLAG_URG,
	RTK_NI_TEST_L4PROTO_TCP_FLAG_ACK,
	RTK_NI_TEST_L4PROTO_TCP_FLAG_PSH,
	RTK_NI_TEST_L4PROTO_TCP_FLAG_RST,
	RTK_NI_TEST_L4PROTO_TCP_FLAG_SYN,
	RTK_NI_TEST_L4PROTO_TCP_FLAG_FIN,
};

enum RTK_NI_TEST_LOCAL2TX_FLAG{
	RTK_NI_TEST_LOCAL2TX_FLAG_FRAME_LEN = (1<<0),
	RTK_NI_TEST_LOCAL2TX_FLAG_SA = (1<<1),
	RTK_NI_TEST_LOCAL2TX_FLAG_DA = (1<<2),
	RTK_NI_TEST_LOCAL2TX_FLAG_ETHERTYPE = (1<<3),
	RTK_NI_TEST_LOCAL2TX_FLAG_IPVER = (1<<4),	
	RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_SIP = (1<<5),
	RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_DIP = (1<<6),
	RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_FLAGS_OFFSET = (1<<7),
	RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_TYPE = (1<<8),
	RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_LENGTH = (1<<9),
	RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_SIP = (1<<10),	
	RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_DIP = (1<<11),
	RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_EXTENSION_HDR_NUM = (1<<12),
	RTK_NI_TEST_LOCAL2TX_FLAG_PPPOE = (1<<13),
	RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN = (1<<14),
	RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN = (1<<15),
	RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_VID = (1<<16),
	RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_PRI = (1<<17),
	RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_VID = (1<<18),
	RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_PRI = (1<<19),
	RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO = (1<<20),
	RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_DATA_OFFSET = (1<<21),
	RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG = (1<<22),
	RTK_NI_TEST_LOCAL2TX_FLAG_L3CS_HW_OFFLOAD_DISABLE = (1<<23),
	RTK_NI_TEST_LOCAL2TX_FLAG_L4CS_HW_OFFLOAD_DISABLE = (1<<24),
};

enum RTK_NI_TEST_TXINFO_INDEX{
	RTK_NI_TEST_TXINFO_INDEX_NORMAL = 0,
	RTK_NI_TEST_TXINFO_INDEX_WIFI,
};

struct rtk_ni_test_txinfo_mapping_entry {
	unsigned char	name[64];
	unsigned char	txinfo_index;
};

struct rtk_ni_test_txinfo_mapping_entry rtk_ni_test_txinfo_mapping_table[] = {
	{"Normal", RTK_NI_TEST_TXINFO_INDEX_NORMAL},
	{"Wifi", RTK_NI_TEST_TXINFO_INDEX_WIFI},
};

#define RTK_NI_TEST_TOTAL_TXINFO_NUM	((sizeof(rtk_ni_test_txinfo_mapping_table))/(sizeof(struct rtk_ni_test_txinfo_mapping_entry)))

#define RTK_NI_TEST_PROC_DIR_NAME "rtk_ni_test"

#define RX_TRACE(comment, ...) \
do {\
		if(unlikely(rtk_ni_test_priv.debug_times && (rtk_ni_test_priv.debug_level&RTK_NI_TEST_RX_TRACE)))\
		{\
			printk("[RX TRACE]\033[1;36m");\
			printk(comment"\033[0m\n", ##__VA_ARGS__);\
		}\
} while(0)

#define TX_TRACE(comment, ...) \
do {\
		if(unlikely(rtk_ni_test_priv.debug_times && (rtk_ni_test_priv.debug_level&RTK_NI_TEST_TX_TRACE)))\
		{\
			printk("[TX TRACE]\033[1;36m");\
			printk(comment"\033[0m\n", ##__VA_ARGS__);\
		}\
} while(0)

static void memDump(void *start, u32 size, char *strHeader, char format)
{
	int row, column, index, index2, max;
//	uint32 buffer[5];
	u8 *buf, *line, ascii[17];
	char empty = '.';

	if(!start ||(size==0))
		return;
	line = (u8*)start;

	/*
	16 bytes per line
	*/
	if (strHeader)
		printk("%s", strHeader);
	column = size % 16;
	row = (size / 16) + 1;
	for (index = 0; index < row; index++, line += 16) 
	{
		buf = line;

		memset (ascii, 0, 17);

		max = (index == row - 1) ? column : 16;
		if ( max==0 ) break; /* If we need not dump this line, break it. */

		if(format & RTK_NI_TEST_MEM_DUMP_FORMAT_ADDR)
			printk("\n%08x ", (u32) line);
		else
			printk("\n ");
		
		//Hex
		if(format & RTK_NI_TEST_MEM_DUMP_FORMAT_HEX)
		{
			for (index2 = 0; index2 < max; index2++)
			{
				if (index2 == 8)
					printk("  ");
				printk("%02x ", (u8) buf[index2]);
				ascii[index2] = (((u8) buf[index2] < 32) || ((u8) buf[index2] >= 128)) ? empty : buf[index2];
			}

			if (max != 16)
			{
				if (max < 8)
					printk("  ");
				for (index2 = 16 - max; index2 > 0; index2--)
					printk("   ");
			}
		}
		else
		{
			for (index2 = 0; index2 < max; index2++)
			{
				ascii[index2] = (((u8) buf[index2] < 32) || ((u8) buf[index2] >= 128)) ? empty : buf[index2];
			}
		}

		//ASCII
		if(format & RTK_NI_TEST_MEM_DUMP_FORMAT_TEXT)
			printk("  %s", ascii);
	}
	printk("\n");
	return;
}

static void skb_debug(const struct sk_buff *skb, int flag)
{
#define NUM2PRINT 1518
	if (flag == RTK_NI_TEST_SKB_RX)
		printk("\nI: ");
	else
		printk("\nO: ");
	printk("eth len = %d eth name %s", skb->len,skb->dev?skb->dev->name:"");
	memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "", RTK_NI_TEST_MEM_DUMP_FORMAT_ALL);
	if(skb->len > NUM2PRINT){
		printk("........");
	}
	printk("\n");
}

#define RX_SKB_DBG(args...) \
do {\
	if(unlikely(rtk_ni_test_priv.debug_times && (rtk_ni_test_priv.debug_level&RTK_NI_TEST_SKB_RX)))\
	{\
		skb_debug(args);\
	}\
} while(0)

#define TX_SKB_DBG(args...) \
do {\
	if(unlikely(rtk_ni_test_priv.debug_times && (rtk_ni_test_priv.debug_level&RTK_NI_TEST_SKB_TX)))\
	{\
		skb_debug(args);\
	}\
} while(0)

#endif
