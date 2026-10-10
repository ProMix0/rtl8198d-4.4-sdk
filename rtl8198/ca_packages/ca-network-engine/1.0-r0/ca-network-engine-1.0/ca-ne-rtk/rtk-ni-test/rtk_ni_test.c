#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/in.h>
#include <linux/if_vlan.h>
#include <linux/crc32.h>
#include <linux/inet.h>
#include <linux/if_pppox.h>
#include <linux/ppp_defs.h>
#include <linux/icmp.h>
#include <linux/igmp.h>
#include <net/ipv6.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/ip6_checksum.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <net/xfrm.h>
#include <linux/proc_fs.h>
#include <bspchip.h>
#include "ca_ni.h"
#include "rtk_ni_test.h"

#undef __IRAM_NIC
#define __IRAM_NIC __attribute__ ((__section__(".iram-fwd")))

extern struct rtk_ni_test_private rtk_ni_test_priv;
extern int __rtk_ni_test_init(void);
extern int __rtk_ni_test_exit(void);
extern int __rtk_ni_test_start_xmit(struct sk_buff *skb);
extern int __tx_info_n_read(struct seq_file *seq, void *v);
extern int __tx_info_n_write(struct file *filp, char *buf, size_t count, loff_t *offp);
extern int __rx_info_read(struct seq_file *seq, void *v);	

static int rtk_ni_test_start_xmit(struct sk_buff *skb)
{
	return __rtk_ni_test_start_xmit(skb);
}

static int tx_info_read(struct seq_file *seq, void *v)
{
	int i;

	for(i=0 ; i<RTK_NI_TEST_TOTAL_TXINFO_NUM ; i++)
	{
		seq_printf(seq, "[%d] %s -----> txinfo_%d\n", i, rtk_ni_test_txinfo_mapping_table[i].name, rtk_ni_test_txinfo_mapping_table[i].txinfo_index);
	}

	return (0);
}

static int tx_info_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	printk("%s-%d: enter\n", __func__, __LINE__);
	// TODO: implement later if neccesary
	return count;
}

static int tx_info_n_read(struct seq_file *seq, void *v)
{
	return __tx_info_n_read(seq, v);
}

static int tx_info_n_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	return __tx_info_n_write(filp, buf, count, offp);
}

static int rx_info_read(struct seq_file *seq, void *v)
{
	return __rx_info_read(seq, v);
}

static int rx_info_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	printk("%s-%d: enter\n", __func__, __LINE__);
	// TODO: implement later if neccesary
	return count;
}

static int dbg_level_read(struct seq_file *seq, void *v) 
{
	seq_printf(seq, "[debug_enable = 0x%08x]\n", rtk_ni_test_priv.debug_level);
	seq_printf(seq, "RTK_NI_TEST_PRINT_NOTHING\t0x%08x\n", RTK_NI_TEST_PRINT_NOTHING);
	seq_printf(seq, "RTK_NI_TEST_SKB_RX\t\t0x%08x\n", RTK_NI_TEST_SKB_RX);
	seq_printf(seq, "RTK_NI_TEST_SKB_TX\t\t0x%08x\n", RTK_NI_TEST_SKB_TX);
	seq_printf(seq, "RTK_NI_TEST_RXINFO\t\t0x%08x\n", RTK_NI_TEST_RXINFO);
	seq_printf(seq, "RTK_NI_TEST_TXINFO\t\t0x%08x\n", RTK_NI_TEST_TXINFO);
	seq_printf(seq, "RTK_NI_TEST_RX_TRACE\t\t0x%08x\n", RTK_NI_TEST_RX_TRACE);
	seq_printf(seq, "RTK_NI_TEST_TX_TRACE\t\t0x%08x\n", RTK_NI_TEST_TX_TRACE);
	seq_printf(seq, "RTK_NI_TEST_RX_WARN\t\t0x%08x\n", RTK_NI_TEST_RX_WARN);
	seq_printf(seq, "RTK_NI_TEST_TX_WARN\t\t0x%08x\n", RTK_NI_TEST_TX_WARN);
	seq_printf(seq, "RTK_NI_TEST_OTHERS\t\t0x%08x\n", RTK_NI_TEST_OTHERS);
	
	return (0);
}

static int dbg_level_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	static struct rtk_ni_test_private *data;
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return (0);
	}
	
	if (buf && !copy_from_user(tmpBuf, buf, len))
	{
		data->debug_level = simple_strtoul(tmpBuf, NULL, 16);
		printk("write debug_enable to 0x%08x\n", data->debug_level);
		return count;
	}
	return -EFAULT;
}

static int dbg_times_read(struct seq_file *seq, void *v) 
{
	seq_printf(seq, "[debug_times = %d times]\n", rtk_ni_test_priv.debug_times);
	return (0);
}

static int dbg_times_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	static struct rtk_ni_test_private *data;
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return (0);
	}
	
	if (buf && !copy_from_user(tmpBuf, buf, len))
	{
		data->debug_times = simple_strtoul(tmpBuf, NULL, 10);
		printk("write debug_times to %d times\n", data->debug_times);
		return count;
	}
	return -EFAULT;
}

static int rx2tx_read(struct seq_file *seq, void *v) 
{
	seq_printf(seq, "[rx2tx = %d]\n", rtk_ni_test_priv.rx2tx);
	return (0);
}

static int rx2tx_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	static struct rtk_ni_test_private *data;
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return (0);
	}
	
	if (buf && !copy_from_user(tmpBuf, buf, len))
	{
		data->rx2tx = simple_strtoul(tmpBuf, NULL, 10);
		printk("write rx2tx to %d\n", data->rx2tx);
		return count;
	}
	return -EFAULT;
}

static int rx2txl34csreset_read(struct seq_file *seq, void *v) 
{
	seq_printf(seq, "[rx2txl34csreset = %d]\n", rtk_ni_test_priv.rx2txl34csreset);
	return (0);
}

static int rx2txl34csreset_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	static struct rtk_ni_test_private *data;
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return (0);
	}
	
	if (buf && !copy_from_user(tmpBuf, buf, len))
	{
		data->rx2txl34csreset = simple_strtoul(tmpBuf, NULL, 10);
		printk("write rx2txl34csreset to %d\n", data->rx2txl34csreset);
		return count;
	}
	return -EFAULT;
}

static int local2tx_build_ethernet(rtk_ni_test_local2tx_t *local2tx_p, struct sk_buff *skb, int *current_ptr)
{
	//DA
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_DA)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_DA\n");
		memcpy(skb->data+(*current_ptr), &local2tx_p->da[0], sizeof(local2tx_p->da));
		(*current_ptr) += sizeof(local2tx_p->da);
	}
	else
	{
		skb->data[(*current_ptr)++] = 0x00;
		skb->data[(*current_ptr)++] = 0x11;
		skb->data[(*current_ptr)++] = 0x11;
		skb->data[(*current_ptr)++] = 0x11;
		skb->data[(*current_ptr)++] = 0x11;
		skb->data[(*current_ptr)++] = 0x11;
	}
	//SA
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SA)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_SA\n");
		memcpy(skb->data+(*current_ptr), &local2tx_p->sa[0], sizeof(local2tx_p->sa));
		(*current_ptr) += sizeof(local2tx_p->da);
	}
	else
	{
		skb->data[(*current_ptr)++] = 0x00;
		skb->data[(*current_ptr)++] = 0x22;
		skb->data[(*current_ptr)++] = 0x22;
		skb->data[(*current_ptr)++] = 0x22;
		skb->data[(*current_ptr)++] = 0x22;
		skb->data[(*current_ptr)++] = 0x22;
	}
	//SVLAN
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN\n");
		*((unsigned short *) (skb->data+(*current_ptr))) = htons(0x88a8);
		(*current_ptr) += 2;
		if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_PRI)
		{
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_VID)
			{
				skb->data[(*current_ptr)++] = (local2tx_p->svlan_pri<<5)|((local2tx_p->svlan_vid&0xf00)>>8);
			}
			else
				skb->data[(*current_ptr)++] = (local2tx_p->svlan_pri<<5);
		}
		else
			(*current_ptr)++;
		if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_VID)
		{
			skb->data[(*current_ptr)++] = (local2tx_p->svlan_vid&0xff);
		}
		else
			(*current_ptr)++;
	}
	//CVLAN
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN\n");
		*((unsigned short *) (skb->data+(*current_ptr))) = htons(0x8100);
		(*current_ptr) += 2;
		if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_PRI)
		{
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_VID)
			{
				skb->data[(*current_ptr)++] = (local2tx_p->cvlan_pri<<5)|((local2tx_p->cvlan_vid&0xf00)>>8);
			}
			else
				skb->data[(*current_ptr)++] = (local2tx_p->cvlan_pri<<5);
		}
		else
			(*current_ptr)++;
		if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_VID)
		{
			skb->data[(*current_ptr)++] = (local2tx_p->cvlan_vid&0xff);
		}
		else
			(*current_ptr)++;
	}
	//Ethertype
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_PPPOE)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_PPPOE\n");
		*((unsigned short *) (skb->data+(*current_ptr))) = htons(ETH_P_PPP_SES);
	}
	else if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPVER
		&& (local2tx_p->ipver == 4 || local2tx_p->ipver == 6))
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPVER\n");
		if(local2tx_p->ipver == 4)
			*((unsigned short *) (skb->data+(*current_ptr))) = htons(ETH_P_IP);
		else
			*((unsigned short *) (skb->data+(*current_ptr))) = htons(ETH_P_IPV6);
	}
	else if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_ETHERTYPE)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_ETHERTYPE\n");
		*((unsigned short *) (skb->data+(*current_ptr))) = htons(local2tx_p->ethertype);
	}
	else
	{
		printk("DEFAULT_ETHERTYPE_0x0800\n");
		*((unsigned short *) (skb->data+(*current_ptr))) = htons(ETH_P_IP);
	}
	(*current_ptr) += sizeof(local2tx_p->ethertype);
	
	return (0);
}

static int local2tx_build_pppoe(rtk_ni_test_local2tx_t *local2tx_p, struct sk_buff *skb, int *current_ptr)
{
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_PPPOE)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_PPPOE\n");
		local2tx_p->pppoehdr = (skb->data+(*current_ptr));
		local2tx_p->ppphdr = &(rtk_ni_test_priv.local2tx.pppoehdr->tag);
		local2tx_p->pppoehdr->ver = 0x1;
		local2tx_p->pppoehdr->type = 0x1;
		local2tx_p->pppoehdr->code = 0x0;
		local2tx_p->pppoehdr->sid = htons(0x22);
		local2tx_p->pppoehdr->length = htons(local2tx_p->frame_len - ((*current_ptr) + sizeof(struct pppoe_hdr)));
		if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPVER)
		{
			if(local2tx_p->ipver == 4)
				local2tx_p->ppphdr->ppp_proto = htons(PPP_IP);
			else if(local2tx_p->ipver == 6)
				local2tx_p->ppphdr->ppp_proto = htons(PPP_IPV6);
		}
		(*current_ptr) += (sizeof(struct pppoe_hdr) + sizeof(struct rtk_ni_test_ppp_header_t));
	}

	return (0);
}

static int local2tx_build_network_layer(rtk_ni_test_local2tx_t *local2tx_p, struct sk_buff *skb, int *current_ptr)
{
	int i;
	
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPVER)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPVER\n");
		if(!(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L3CS_HW_OFFLOAD_DISABLE))
		{
			printk("RTK_NI_TEST_LOCAL2TX_FLAG_L3CS_HW_OFFLOAD_DISABLE\n");
			//skb->network_header = (skb->data+(*current_ptr)) - skb->head;
			skb_set_network_header(skb, (*current_ptr));
		}
		if(local2tx_p->ipver == 4)
		{
			local2tx_p->iphdr = (skb->data+(*current_ptr));
			local2tx_p->iphdr->version = 4;
			local2tx_p->iphdr->ihl = 5;
			local2tx_p->iphdr->tot_len = htons(local2tx_p->frame_len - (*current_ptr));
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_FLAGS_OFFSET)
			{
				printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_FLAGS_OFFSET\n");
				local2tx_p->iphdr->frag_off = htons(local2tx_p->ipv4_flags_offset);
			}
			local2tx_p->iphdr->ttl = 64;
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO)
				local2tx_p->iphdr->protocol = local2tx_p->l4proto;
			local2tx_p->iphdr->check = 0;
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_SIP)
			{
				printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_SIP\n");
				local2tx_p->iphdr->saddr = local2tx_p->ipv4_sip;
			}
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_DIP)
			{
				printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_DIP\n");
				local2tx_p->iphdr->daddr = local2tx_p->ipv4_dip;
			}
			(*current_ptr) += sizeof(struct iphdr);
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_TYPE
				&& local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_LENGTH)
			{
				printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_TYPE\n");
				printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_LENGTH\n");
				local2tx_p->ipopt = (skb->data+(*current_ptr));
				local2tx_p->ipopt->type = local2tx_p->ipv4_option_type;
				local2tx_p->ipopt->length = local2tx_p->ipv4_option_length;
				local2tx_p->iphdr->ihl += (local2tx_p->ipv4_option_length/4);
				(*current_ptr) += local2tx_p->ipv4_option_length;
			}
		}
		else if(local2tx_p->ipver == 6)
		{
			local2tx_p->ipv6hdr = (skb->data+(*current_ptr));
			local2tx_p->ipv6hdr->version = 6;
			local2tx_p->ipv6hdr->payload_len = htons(local2tx_p->frame_len - ((*current_ptr) + sizeof(struct ipv6hdr)));
			local2tx_p->ipv6hdr->hop_limit = 255;
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_SIP)
			{
				printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_SIP\n");
				memcpy(&(local2tx_p->ipv6hdr->saddr), &local2tx_p->ipv6_sip[0], 16);
			}
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_DIP)
			{
				printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_DIP\n");
				memcpy(&(local2tx_p->ipv6hdr->daddr), &local2tx_p->ipv6_dip[0], 16);
			}
			(*current_ptr) += sizeof(struct ipv6hdr);
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_EXTENSION_HDR_NUM)
			{
				printk("RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_EXTENSION_HDR_NUM\n");
				struct rtk_ni_test_ipv6_extension_header_t *ip6_ext_hdr_p;
				local2tx_p->ipv6hdr->nexthdr=local2tx_p->ipv6_extension_header_type[0];
				for(i=0 ; i<local2tx_p->ipv6_extension_header_num ; i++)
				{	
					ip6_ext_hdr_p = (skb->data+(*current_ptr));
					if(i != local2tx_p->ipv6_extension_header_num-1)
						ip6_ext_hdr_p->type = local2tx_p->ipv6_extension_header_type[i+1];
					else if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO && local2tx_p->l4proto)
						ip6_ext_hdr_p->type = local2tx_p->l4proto;
					ip6_ext_hdr_p->length = local2tx_p->ipv6_extension_header_length[i];
					(*current_ptr) += sizeof(struct rtk_ni_test_ipv6_extension_header_t)+(local2tx_p->ipv6_extension_header_length[i]*8);
				}
			}
			else if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO && local2tx_p->l4proto)
			{
				local2tx_p->ipv6hdr->nexthdr = local2tx_p->l4proto;
			}

		}

		// Calculate & save L3CS
		if(local2tx_p->iphdr)
			local2tx_p->l3cs_answer = ip_fast_csum((char *) local2tx_p->iphdr, local2tx_p->iphdr->ihl);
	}

	return (0);
}

static int local2tx_build_transport_layer(rtk_ni_test_local2tx_t *local2tx_p, struct sk_buff *skb, int *current_ptr)
{
	unsigned char	*icmpv6_option;
	
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO)
	{
		printk("RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO\n");
		//skb->transport_header = (skb->data+(*current_ptr)) - skb->head;
		skb_set_transport_header(skb, (*current_ptr));
		switch(local2tx_p->l4proto)
		{
			case RTK_NI_TEST_L4PROTO_TCP:				
				printk("RTK_NI_TEST_L4PROTO_TCP\n");
				local2tx_p->tcphdr = (skb->data+(*current_ptr));
				local2tx_p->tcphdr->source = htons(1000);
				local2tx_p->tcphdr->dest = htons(2000);
				local2tx_p->tcphdr->seq = htonl(6666);
				local2tx_p->tcphdr->ack_seq = htonl(7777);
				if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_DATA_OFFSET)
					local2tx_p->tcphdr->doff = local2tx_p->l4proto_tcp_data_offset;
				else
					local2tx_p->tcphdr->doff = 5;
				if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG)
				{
					if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_CWR))
						local2tx_p->tcphdr->cwr = 1;
					if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_ECE))
						local2tx_p->tcphdr->ece = 1;
					if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_URG))
						local2tx_p->tcphdr->urg = 1;
					if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_ACK))
						local2tx_p->tcphdr->ack = 1;
					if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_PSH))
						local2tx_p->tcphdr->psh = 1;
					if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_RST))
						local2tx_p->tcphdr->rst = 1;
					if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_SYN))
						local2tx_p->tcphdr->syn = 1;
					if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_FIN))
						local2tx_p->tcphdr->fin = 1;
				}
				else
				{
					local2tx_p->tcphdr->fin = 0;
					local2tx_p->tcphdr->syn = 0;
					local2tx_p->tcphdr->rst = 0;
					local2tx_p->tcphdr->psh = 0;
					local2tx_p->tcphdr->ack = 1;
					local2tx_p->tcphdr->urg = 0;
					local2tx_p->tcphdr->ece = 0;
					local2tx_p->tcphdr->cwr = 0;
				}
				local2tx_p->tcphdr->window = htons(3592);
				local2tx_p->tcphdr->check = htons(0);
				local2tx_p->tcphdr->urg_ptr = htons(0);
				(*current_ptr) += sizeof(struct tcphdr);
				break;
			case RTK_NI_TEST_L4PROTO_UDP:
				printk("RTK_NI_TEST_L4PROTO_UDP\n");
				local2tx_p->udphdr = (skb->data+(*current_ptr));
				local2tx_p->udphdr->source = htons(0);
				local2tx_p->udphdr->dest = htons(0);
				local2tx_p->udphdr->len = htons(local2tx_p->frame_len - (*current_ptr));
				
				if(!(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4CS_HW_OFFLOAD_DISABLE))
				{
					printk("RTK_NI_TEST_LOCAL2TX_FLAG_L4CS_HW_OFFLOAD_DISABLE\n");
					local2tx_p->udphdr->check = htons(0x1111); // NI driver tuggle udp_en only when check != 0
				}
				else
				{
					local2tx_p->udphdr->check = htons(0);
				}
				
				(*current_ptr) += sizeof(struct udphdr);
				break;
			case RTK_NI_TEST_L4PROTO_ICMP:
				printk("RTK_NI_TEST_L4PROTO_ICMP\n");
				local2tx_p->icmphdr = (skb->data+(*current_ptr));
				local2tx_p->icmphdr->type = 8;
				local2tx_p->icmphdr->code = 0;
				local2tx_p->icmphdr->checksum = htons(0);
				local2tx_p->icmphdr->un.echo.id = htons(0xa);
				local2tx_p->icmphdr->un.echo.sequence = htons(0xb);
				(*current_ptr) += sizeof(struct icmphdr);
				break;
			case RTK_NI_TEST_L4PROTO_ICMPV6:				
				printk("RTK_NI_TEST_L4PROTO_ICMPV6\n");
				local2tx_p->icmp6hdr = (skb->data+(*current_ptr));
				local2tx_p->icmp6hdr->icmp6_type = 133;
				local2tx_p->icmp6hdr->icmp6_code = 0;
				local2tx_p->icmp6hdr->icmp6_cksum = htons(0);
				local2tx_p->icmp6hdr->icmp6_dataun.un_data32[0] = 0;
				(*current_ptr) += sizeof(struct icmp6hdr);
				icmpv6_option = (skb->data+(*current_ptr));
				*(icmpv6_option+0) = 0x1;*(icmpv6_option+1) = 0x1;
				*(icmpv6_option+2) = 0x0;*(icmpv6_option+3) = 0x1;
				*(icmpv6_option+4) = 0x1;*(icmpv6_option+5) = 0x1;
				*(icmpv6_option+6) = 0x1;*(icmpv6_option+7) = 0x1;
				(*current_ptr) += 8;
				/*if((*current_ptr)+8 <= local2tx_p->frame_len)
					(*current_ptr) += 8;
				local2tx_p->frame_len = (*current_ptr);*/		
				break;
			case RTK_NI_TEST_L4PROTO_IGMP:
				printk("RTK_NI_TEST_L4PROTO_IGMP\n");
				local2tx_p->igmphdr = (skb->data+(*current_ptr));
				local2tx_p->igmphdr->type = 0x16;
				local2tx_p->igmphdr->code = 0x0;
				local2tx_p->igmphdr->csum = htons(0);
				local2tx_p->igmphdr->group = in_aton("234.2.3.4");
				(*current_ptr) += sizeof(struct igmphdr);		
				break;
			default:
				break;
		}
	}
	return (0);
}

static int local2tx_calculate_and_save_l4cs(rtk_ni_test_local2tx_t *local2tx_p, struct sk_buff *skb)
{
	__wsum csum = 0;
	if(local2tx_p->tcphdr || local2tx_p->udphdr || local2tx_p->icmphdr || local2tx_p->icmp6hdr || local2tx_p->igmphdr)
	{
		int offset = skb_transport_offset(skb);	
		unsigned short temp_l4cs;
		int hlen = skb->len - offset;
		struct in6_addr *saddr6;
		struct in6_addr *daddr6;
		__be32	saddr;
		__be32	daddr;

		if(local2tx_p->tcphdr)
		{
			if(local2tx_p->ipver == 4)
			{
				saddr = local2tx_p->iphdr->saddr;
				daddr = local2tx_p->iphdr->daddr;
				temp_l4cs = ntohs(local2tx_p->tcphdr->check);
				local2tx_p->tcphdr->check = htons(0);
				csum = skb_checksum(skb, offset, hlen, 0);
				local2tx_p->l4cs_answer = csum_tcpudp_magic(saddr, daddr,
								hlen, IPPROTO_TCP, csum);
				local2tx_p->tcphdr->check = htons(temp_l4cs);
			}
			else if(local2tx_p->ipver == 6)
			{
				saddr6 = &(local2tx_p->ipv6hdr->saddr);
				daddr6 = &(local2tx_p->ipv6hdr->daddr);
				temp_l4cs = ntohs(local2tx_p->tcphdr->check);
				local2tx_p->tcphdr->check = htons(0);
				csum = skb_checksum(skb, offset, hlen, 0);
				local2tx_p->l4cs_answer = csum_ipv6_magic(saddr6, daddr6,
						  hlen, IPPROTO_TCP, csum);
				local2tx_p->tcphdr->check = htons(temp_l4cs);
			}
		}
		else if(local2tx_p->udphdr)
		{
			int len;
			len = ntohs(local2tx_p->udphdr->len);
			
			if(local2tx_p->ipver == 4)
			{
				struct sk_buff *frags;
				skb_walk_frags(skb, frags) {
					csum = csum_add(csum, frags->csum);
					hlen -= frags->len;
				}
				saddr = local2tx_p->iphdr->saddr;
				daddr = local2tx_p->iphdr->daddr;
				temp_l4cs = ntohs(local2tx_p->udphdr->check);
				local2tx_p->udphdr->check = htons(0);
				csum = skb_checksum(skb, offset, hlen, 0);
				local2tx_p->l4cs_answer = udp_v4_check(len, saddr, daddr, csum);
				local2tx_p->udphdr->check = htons(temp_l4cs);
			}
			else if(local2tx_p->ipver == 6)
			{
				saddr6 = &(local2tx_p->ipv6hdr->saddr);
				daddr6 = &(local2tx_p->ipv6hdr->daddr);
				temp_l4cs = ntohs(local2tx_p->udphdr->check);
				local2tx_p->udphdr->check = htons(0);
				csum = skb_checksum(skb, offset, hlen, 0);
				local2tx_p->l4cs_answer = udp_v6_check(len, saddr6, daddr6, csum);
				local2tx_p->udphdr->check = htons(temp_l4cs);
			}
		}
		else if(local2tx_p->icmphdr)
		{
			csum = csum_partial((void *)local2tx_p->icmphdr, (skb->len-offset), csum);
			local2tx_p->l4cs_answer = csum_fold(csum);
		}
		else if(local2tx_p->icmp6hdr && local2tx_p->ipver == 6)
		{
			// TODO: still have some issue here
			saddr6 = &(local2tx_p->ipv6hdr->saddr);
			daddr6 = &(local2tx_p->ipv6hdr->daddr);
			temp_l4cs = ntohs(local2tx_p->icmp6hdr->icmp6_cksum);
			local2tx_p->icmp6hdr->icmp6_cksum = 0;
			csum = csum_partial(local2tx_p->icmp6hdr, sizeof(struct icmp6hdr), csum);
			local2tx_p->l4cs_answer = csum_ipv6_magic(saddr6, daddr6,
							      hlen, IPPROTO_ICMPV6, csum);
			local2tx_p->icmp6hdr->icmp6_cksum = htons(temp_l4cs);
		}
		else if(local2tx_p->igmphdr)
		{
			local2tx_p->l4cs_answer = ip_compute_csum((void *)local2tx_p->igmphdr, sizeof(struct igmphdr));
		}
	}

	return (0);
}

static struct sk_buff * local2tx_build_packet(rtk_ni_test_local2tx_t *local2tx_p)
{
	struct sk_buff *skb = NULL;	
	unsigned int payload_data;
	int current_ptr;
	int i;
	
	skb = dev_alloc_skb(local2tx_p->frame_len);
	if(skb == NULL)
		return NULL;
	
	local2tx_p->pppoehdr = NULL;
	local2tx_p->ppphdr = NULL;
	local2tx_p->iphdr = NULL;
	local2tx_p->ipv6hdr = NULL;
	local2tx_p->l3cs_answer = 0;
	local2tx_p->l4cs_answer = 0;
	local2tx_p->crc_answer = 0;
	current_ptr = 0;
	memset(skb->data, 0, local2tx_p->frame_len);

	/*Ethernet*/
	local2tx_build_ethernet(local2tx_p, skb, &current_ptr);
	/*PPPoE*/
	local2tx_build_pppoe(local2tx_p, skb, &current_ptr);
	/*Network layer*/
	local2tx_build_network_layer(local2tx_p, skb, &current_ptr);
	/*Transport layer*/
	local2tx_build_transport_layer(local2tx_p, skb, &current_ptr);
	//Payload
	payload_data = 0;
	for(i=current_ptr ; i<local2tx_p->frame_len ; i++)
	{
		skb->data[current_ptr++] = (unsigned char)(payload_data&0xff);
		payload_data++;
	}

	skb_put(skb, local2tx_p->frame_len);

	// Calculate & save L4CS
	local2tx_calculate_and_save_l4cs(local2tx_p, skb);
	
	return skb;
}

static int local2tx_read(struct seq_file *seq, void *v) 
{
	rtk_ni_test_local2tx_t *local2tx_p = &rtk_ni_test_priv.local2tx;
	int i;
	
	seq_printf(seq, "[count = %d]\n", local2tx_p->count);
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_FRAME_LEN)
	{
		seq_printf(seq, "frame_len=%d\n", local2tx_p->frame_len);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SA)
	{
		seq_printf(seq, "sa=%02X:%02X:%02X:%02X:%02X:%02X\n", local2tx_p->sa[0], local2tx_p->sa[1], local2tx_p->sa[2], local2tx_p->sa[3], local2tx_p->sa[4], local2tx_p->sa[5]);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_DA)
	{
		seq_printf(seq, "da=%02X:%02X:%02X:%02X:%02X:%02X\n", local2tx_p->da[0], local2tx_p->da[1], local2tx_p->da[2], local2tx_p->da[3], local2tx_p->da[4], local2tx_p->da[5]);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN)
	{
		seq_printf(seq, "cvlan\n");
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_PRI)
	{
		seq_printf(seq, "cvlan_pri=%d\n", local2tx_p->cvlan_pri);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_VID)
	{
		seq_printf(seq, "cvlan_vid=%d\n", local2tx_p->cvlan_vid);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN)
	{
		seq_printf(seq, "svlan\n");
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_PRI)
	{
		seq_printf(seq, "svlan_pri=%d\n", local2tx_p->svlan_pri);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_VID)
	{
		seq_printf(seq, "svlan_vid=%d\n", local2tx_p->svlan_vid);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_ETHERTYPE)
	{
		seq_printf(seq, "ethertype=0x%04X\n", local2tx_p->ethertype);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_PPPOE)
	{
		seq_printf(seq, "pppoe\n");
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPVER)
	{
		seq_printf(seq, "ipver=%d\n", local2tx_p->ipver);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_SIP)
	{
		seq_printf(seq, "ipv4_sip=%pI4\n", &(local2tx_p->ipv4_sip));
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_DIP)
	{
		seq_printf(seq, "ipv4_dip=%pI4\n", &(local2tx_p->ipv4_dip));
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_FLAGS_OFFSET)
	{
		seq_printf(seq, "ipv4_flags_offset=0x%04X (DF=%d MF=%d offset=%d)\n", local2tx_p->ipv4_flags_offset
			, rtk_ni_test_priv.local2tx.ipv4_flags_offset&(1<<14) ? 1:0, local2tx_p->ipv4_flags_offset&(1<<13) ? 1:0, local2tx_p->ipv4_flags_offset&0x1fff);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_TYPE)
	{
		seq_printf(seq, "ipv4_option_type=0x%02X\n", local2tx_p->ipv4_option_type);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_LENGTH)
	{
		seq_printf(seq, "ipv4_option_length=%d\n", local2tx_p->ipv4_option_length);
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_SIP)
	{
		seq_printf(seq, "ipv6_sip=%pI6\n", &(local2tx_p->ipv6_sip));
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_DIP)
	{
		seq_printf(seq, "ipv6_dip=%pI6\n", &(local2tx_p->ipv6_dip));
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_EXTENSION_HDR_NUM)
	{
		seq_printf(seq, "ipv6_extension_header_num=%d\n", local2tx_p->ipv6_extension_header_num);
		seq_printf(seq, "ipv6_extension_header_index=%d\n", local2tx_p->ipv6_extension_header_index);
		for(i=0 ; i<local2tx_p->ipv6_extension_header_num ; i++)
		{
			seq_printf(seq, "ipv6_extension_header_type[%d]=%d\n", i, local2tx_p->ipv6_extension_header_type[i]);
			seq_printf(seq, "ipv6_extension_header_length[%d]=%d\n", i, local2tx_p->ipv6_extension_header_length[i]);
		}
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO)
	{
		if(local2tx_p->l4proto == RTK_NI_TEST_L4PROTO_TCP)
		{
			seq_printf(seq, "TCP\n");
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_DATA_OFFSET)
			{
				seq_printf(seq, "l4proto_tcp_data_offset=%d \n", local2tx_p->l4proto_tcp_data_offset);
			}
			if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG)
			{
				seq_printf(seq, "l4proto_tcp_flag=");
				if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_CWR))
					seq_printf(seq, "CWR.");
				if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_ECE))
					seq_printf(seq, "ECE.");
				if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_URG))
					seq_printf(seq, "URG.");
				if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_ACK))
					seq_printf(seq, "ACK.");
				if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_PSH))
					seq_printf(seq, "PSH.");
				if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_RST))
					seq_printf(seq, "RST.");
				if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_SYN))
					seq_printf(seq, "SYN.");
				if(local2tx_p->l4proto_tcp_flag&(1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_FIN))
					seq_printf(seq, "FIN.");
				seq_printf(seq, "\n");
			}
		}
		else if(local2tx_p->l4proto == RTK_NI_TEST_L4PROTO_UDP)
			seq_printf(seq, "UDP\n");
		else if(local2tx_p->l4proto == RTK_NI_TEST_L4PROTO_ICMP)
			seq_printf(seq, "ICMP\n");
		else if(local2tx_p->l4proto == RTK_NI_TEST_L4PROTO_ICMPV6)
			seq_printf(seq, "ICMPV6\n");
		else if(local2tx_p->l4proto == RTK_NI_TEST_L4PROTO_IGMP)
			seq_printf(seq, "IGMP\n");
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L3CS_HW_OFFLOAD_DISABLE)
	{
		seq_printf(seq, "l3cs_hw_offload_disable\n");
	}
	if(local2tx_p->flag&RTK_NI_TEST_LOCAL2TX_FLAG_L4CS_HW_OFFLOAD_DISABLE)
	{
		seq_printf(seq, "l4cs_hw_offload_disable\n");
	}
	if(local2tx_p->iphdr)
	{
		seq_printf(seq, "L3CS=%02X %02X\n", (ntohs(local2tx_p->l3cs_answer)&0xff00)>>8
			, (ntohs(local2tx_p->l3cs_answer)&0xff));
	}
	if(local2tx_p->tcphdr || local2tx_p->udphdr || local2tx_p->icmphdr || local2tx_p->icmp6hdr || local2tx_p->igmphdr)
	{
		seq_printf(seq, "L4CS=%02X %02X\n", (ntohs(local2tx_p->l4cs_answer)&0xff00)>>8
			, (ntohs(local2tx_p->l4cs_answer)&0xff));
	}
	
	return (0);
}

static int local2tx_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	static struct rtk_ni_test_private *data;
	struct sk_buff *skb = NULL;
	char tmpbuf[64];
	char *strptr;
	char *fieldptr,*valueptr;
	unsigned long long value64;
	unsigned int value, i;

	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return count;
	}

	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count-1] = '\0';
		strptr=tmpbuf;
		fieldptr = strsep(&strptr," ");
		if(fieldptr == NULL)
		{
			goto errout;
		}

		if(!strcmp(fieldptr,"start"))
		{
			printk("start transmit packet %d times with frame_len %d.\n", data->local2tx.count, data->local2tx.frame_len);
			for(i=0 ; i<data->local2tx.count ; i++)
			{
				if((skb = local2tx_build_packet(&data->local2tx)) == NULL)
				{
					printk("local2tx_build_packet FAIL.\n");
					goto errout;
				}				
				rtk_ni_test_start_xmit(skb);
			}
		}
		else if(!strcmp(fieldptr,"default"))
		{
			printk("set local2tx to default.\n");
			memset(&data->local2tx, 0, sizeof(rtk_ni_test_local2tx_t));
			data->local2tx.flag = 0;
		}
		else
		{
			valueptr = strsep(&strptr," ");
			if(valueptr == NULL)
			{
				goto errout;
			}

			if(!strcmp(fieldptr,"sw_id"))
			{
				value64 = simple_strtoll(valueptr, NULL, 0);
			}
			else
			{
				value = simple_strtol(valueptr, NULL, 0);
			}
			
			printk("set field: %s, value %s\n",fieldptr, valueptr);

			if(!strcmp(fieldptr,"count"))
			{
				data->local2tx.count = value;
			}
			else if(!strcmp(fieldptr,"frame_len"))
			{
				if(value < 64)
				{
					printk("ERROR: should be >= 64\n");
					goto errout;
				}
				data->local2tx.frame_len = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_FRAME_LEN;
			}
			else if(!strcmp(fieldptr,"sa"))
			{
				if(sscanf(valueptr, "%02x:%02x:%02x:%02x:%02x:%02x", &data->local2tx.sa[0], &data->local2tx.sa[1], &data->local2tx.sa[2], &data->local2tx.sa[3], &data->local2tx.sa[4], &data->local2tx.sa[5]) != 6 ||
					sscanf(valueptr, "%02X:%02X:%02X:%02X:%02X:%02X", &data->local2tx.sa[0], &data->local2tx.sa[1], &data->local2tx.sa[2], &data->local2tx.sa[3], &data->local2tx.sa[4], &data->local2tx.sa[5]) != 6)
				{
					printk("ERROR: should be xx:xx:xx:xx:xx:xx format\n");
					goto errout;
				}
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_SA;
			}
			else if(!strcmp(fieldptr,"da"))
			{
				if(sscanf(valueptr, "%02x:%02x:%02x:%02x:%02x:%02x", &data->local2tx.da[0], &data->local2tx.da[1], &data->local2tx.da[2], &data->local2tx.da[3], &data->local2tx.da[4], &data->local2tx.da[5]) != 6 ||
					sscanf(valueptr, "%02X:%02X:%02X:%02X:%02X:%02X", &data->local2tx.da[0], &data->local2tx.da[1], &data->local2tx.da[2], &data->local2tx.da[3], &data->local2tx.da[4], &data->local2tx.da[5]) != 6)
				{
					printk("ERROR: should be xx:xx:xx:xx:xx:xx format\n");
					goto errout;
				}
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_DA;
			}
			else if(!strcmp(fieldptr,"cvlan"))
			{
				if(value)
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN;
				else
					data->local2tx.flag &= ~RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN;
			}
			else if(!strcmp(fieldptr,"svlan"))
			{
				if(value)
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN;
				else
					data->local2tx.flag &= ~RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN;
			}
			else if(!strcmp(fieldptr,"cvlan_pri"))
			{
				if(value > 7)
				{
					printk("ERROR: should be 0~7\n");
					goto errout;
				}
				data->local2tx.cvlan_pri = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_PRI;
			}
			else if(!strcmp(fieldptr,"svlan_pri"))
			{				
				if(value > 7)
				{
					printk("ERROR: should be 0~7\n");
					goto errout;
				}
				data->local2tx.svlan_pri = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_PRI;
			}
			else if(!strcmp(fieldptr,"cvlan_vid"))
			{
				if(value > 4095)
				{
					printk("ERROR: should be 0~4095\n");
					goto errout;
				}
				data->local2tx.cvlan_vid = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_CVLAN_VID;
			}
			else if(!strcmp(fieldptr,"svlan_vid"))
			{
				if(value > 4095)
				{
					printk("ERROR: should be 0~4095\n");
					goto errout;
				}
				data->local2tx.svlan_vid = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_SVLAN_VID;
			}
			else if(!strcmp(fieldptr,"pppoe"))
			{
				if(value)
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_PPPOE;
				else
					data->local2tx.flag &= ~RTK_NI_TEST_LOCAL2TX_FLAG_PPPOE;
			}
			else if(!strcmp(fieldptr,"ethertype"))
			{
				if(value >= (1<<16))
				{
					printk("ERROR: over range\n");
					goto errout;
				}
				data->local2tx.ethertype = value&0xffff;				
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_ETHERTYPE;
			}
			else if(!strcmp(fieldptr,"ipver"))
			{
				if(value != 4 && value != 6)
				{
					printk("ERROR: should be 4 or 6\n");
					goto errout;
				}
				data->local2tx.ipver = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPVER;
			}
			else if(!strcmp(fieldptr,"ipv4_sip"))
			{
				data->local2tx.ipv4_sip = in_aton(valueptr);
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_SIP;
			}
			else if(!strcmp(fieldptr,"ipv4_dip"))
			{
				data->local2tx.ipv4_dip = in_aton(valueptr);
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_DIP;
			}
			else if(!strcmp(fieldptr,"ipv4_flags_offset"))
			{
				data->local2tx.ipv4_flags_offset = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_FLAGS_OFFSET;
			}
			else if(!strcmp(fieldptr,"ipv4_option_type"))
			{
				data->local2tx.ipv4_option_type = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_TYPE;
			}
			else if(!strcmp(fieldptr,"ipv4_option_length"))
			{
				if(value%4)
				{
					printk("ERROR: should be multiple of 4\n");
					goto errout;
				}
				data->local2tx.ipv4_option_length = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPV4_OPTION_LENGTH;
			}
			else if(!strcmp(fieldptr,"ipv6_sip"))
			{
				in6_pton(valueptr,-1,&(data->local2tx.ipv6_sip),-1,NULL);
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_SIP;
			}
			else if(!strcmp(fieldptr,"ipv6_dip"))
			{
				in6_pton(valueptr,-1,&(data->local2tx.ipv6_dip),-1,NULL);
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_DIP;
			}
			else if(!strcmp(fieldptr,"ipv6_extension_header_num"))
			{
				if(value > RTK_NI_TEST_MAX_IPV6_EXTENTION_HDR_NUM)
				{
					printk("ERROR: over range\n");
					goto errout;
				}
				data->local2tx.ipv6_extension_header_num = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_IPV6_EXTENSION_HDR_NUM;
			}
			else if(!strcmp(fieldptr,"ipv6_extension_header_index"))
			{
				if(value >= data->local2tx.ipv6_extension_header_num || !data->local2tx.ipv6_extension_header_num)
				{
					printk("ERROR: over range\n");
					goto errout;
				}
				data->local2tx.ipv6_extension_header_index = value;
			}
			else if(!strcmp(fieldptr,"ipv6_extension_header_type"))
			{
				if(value&(1<<8))
				{
					printk("ERROR: over range\n");
					goto errout;
				}
				data->local2tx.ipv6_extension_header_type[data->local2tx.ipv6_extension_header_index] = value;
			}
			else if(!strcmp(fieldptr,"ipv6_extension_header_length"))
			{
				if(value&(1<<8))
				{
					printk("ERROR: over range\n");
					goto errout;
				}
				data->local2tx.ipv6_extension_header_length[data->local2tx.ipv6_extension_header_index] = value;
			}
			else if(!strcmp(fieldptr,"l4proto"))
			{
				if(!strcmp(valueptr, "TCP"))
				{
					data->local2tx.l4proto = RTK_NI_TEST_L4PROTO_TCP;
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO;
				}
				else if(!strcmp(valueptr, "UDP"))
				{
					data->local2tx.l4proto = RTK_NI_TEST_L4PROTO_UDP;
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO;
				}
				else if(!strcmp(valueptr, "ICMP"))
				{
					data->local2tx.l4proto = RTK_NI_TEST_L4PROTO_ICMP;
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO;
				}
				else if(!strcmp(valueptr, "ICMPV6"))
				{
					data->local2tx.l4proto = RTK_NI_TEST_L4PROTO_ICMPV6;
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO;
				}
				else if(!strcmp(valueptr, "IGMP"))
				{
					data->local2tx.l4proto = RTK_NI_TEST_L4PROTO_IGMP;
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO;
				}
				else if(!strcmp(valueptr, "NONE"))
				{
					data->local2tx.l4proto = 0;
					data->local2tx.flag &= ~RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO;
				}
				else
				{
					printk("ERROR: l4proto not support\n");
					goto errout;
				}
			}
			else if(!strcmp(fieldptr,"l4proto_tcp_data_offset"))
			{
				if(value&(1<<4))
				{
					printk("ERROR: over range\n");
					goto errout;
				}
				data->local2tx.l4proto_tcp_data_offset = value;
				data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_DATA_OFFSET;
			}
			else if(!strcmp(fieldptr,"l4proto_tcp_flag"))
			{
				if(!strcmp(valueptr, "CWR"))
				{
					data->local2tx.l4proto_tcp_flag |= (1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_CWR);
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else if(!strcmp(valueptr, "ECE"))
				{
					data->local2tx.l4proto_tcp_flag |= (1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_ECE);
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else if(!strcmp(valueptr, "URG"))
				{
					data->local2tx.l4proto_tcp_flag |= (1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_URG);
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else if(!strcmp(valueptr, "ACK"))
				{
					data->local2tx.l4proto_tcp_flag |= (1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_ACK);
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else if(!strcmp(valueptr, "PSH"))
				{
					data->local2tx.l4proto_tcp_flag |= (1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_PSH);
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else if(!strcmp(valueptr, "RST"))
				{
					data->local2tx.l4proto_tcp_flag |= (1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_RST);
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else if(!strcmp(valueptr, "SYN"))
				{
					data->local2tx.l4proto_tcp_flag |= (1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_SYN);
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else if(!strcmp(valueptr, "FIN"))
				{
					data->local2tx.l4proto_tcp_flag |= (1<<RTK_NI_TEST_L4PROTO_TCP_FLAG_FIN);
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else if(!strcmp(valueptr, "NONE"))
				{
					data->local2tx.l4proto_tcp_flag = 0;
					data->local2tx.flag &= ~RTK_NI_TEST_LOCAL2TX_FLAG_L4PROTO_TCP_FLAG;
				}
				else
				{
					printk("ERROR: l4proto_tcp_flag not support\n");
					goto errout;
				}
			}
			else if(!strcmp(fieldptr,"l3cs_hw_offload_disable"))
			{
				if(value)
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L3CS_HW_OFFLOAD_DISABLE;
				else
					data->local2tx.flag &= ~RTK_NI_TEST_LOCAL2TX_FLAG_L3CS_HW_OFFLOAD_DISABLE;
			}
			else if(!strcmp(fieldptr,"l4cs_hw_offload_disable"))
			{
				if(value)
					data->local2tx.flag |= RTK_NI_TEST_LOCAL2TX_FLAG_L4CS_HW_OFFLOAD_DISABLE;
				else
					data->local2tx.flag &= ~RTK_NI_TEST_LOCAL2TX_FLAG_L4CS_HW_OFFLOAD_DISABLE;
			}
			else
				goto errout;
		}
		
		return count;
	}
	else
	{
errout:
		printk("%s error\n", __func__);
		printk("count\n");
		printk("frame_len\n");
		printk("sa\n");
		printk("da\n");
		printk("cvlan\n");
		printk("svlan\n");
		printk("cvlan_pri\n");
		printk("svlan_pri\n");
		printk("cvlan_vid\n");
		printk("svlan_vid\n");
		printk("pppoe\n");
		printk("ipver\n");
		printk("ethertype\n");
		printk("ipv4_sip\n");
		printk("ipv4_dip\n");
		printk("ipv4_flags_offset\n");
		printk("ipv4_option_type\n");
		printk("ipv4_option_length\n");
		printk("ipv6_sip\n");
		printk("ipv6_dip\n");
		printk("ipv6_extension_header_num\n");
		printk("ipv6_extension_header_index\n");
		printk("ipv6_extension_header_type\n");
		printk("ipv6_extension_header_length\n");
		printk("l4proto\n");
		printk("l4proto_tcp_data_offset\n");
		printk("l4proto_tcp_flag\n");
		printk("l3cs_hw_offload_disable\n");
		printk("l4cs_hw_offload_disable\n");
	}
	
	return count;
}

static int txapi_read(struct seq_file *seq, void *v) 
{
	int i;
	
	seq_printf(seq, "[txapi = %pF]\n", rtk_ni_test_priv.tx_api[rtk_ni_test_priv.tx_api_idx]);
	for(i=0 ; i<RTK_NI_TEST_MAX_TX_API_NUM ; i++)
	{
		if(rtk_ni_test_priv.tx_api[i])
			seq_printf(seq, "[%d] %pF\n", i, rtk_ni_test_priv.tx_api[i]);
	}
	return (0);
}

static int txapi_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	static struct rtk_ni_test_private *data;
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;
	unsigned int tx_api_index;
	
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return (0);
	}
	
	if (buf && !copy_from_user(tmpBuf, buf, len))
	{
		tx_api_index = simple_strtoul(tmpBuf, NULL, 10);
		if(tx_api_index>=0 && tx_api_index<RTK_NI_TEST_MAX_TX_API_NUM)
		{
			data->tx_api_idx = tx_api_index;
			if(data->tx_api[data->tx_api_idx])
				printk("write txapi to %pF\n", data->tx_api[data->tx_api_idx]);
			else
				printk("not support APIs in list.\n");
		}
		return count;
	}
	return -EFAULT;
}

struct proc_dir_entry *rtk_ni_test_proc_dir=NULL;

static int rx_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_info_read, inode->i_private);
}
static const struct file_operations rxinfo_fops = {
	.owner = THIS_MODULE,
	.open = rx_info_open,
	.read = seq_read,
	.write = rx_info_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int tx_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, tx_info_read, inode->i_private);
}
static const struct file_operations txinfo_fops = {
	.owner = THIS_MODULE,
	.open = tx_info_open,
	.read = seq_read,
	.write = tx_info_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int tx_info_n_open(struct inode *inode, struct file *file)
{
	return single_open(file, tx_info_n_read, (void *) file);
}
static const struct file_operations txinfo_n_fops = {
	.owner = THIS_MODULE,
	.open = tx_info_n_open,
	.read = seq_read,
	.write = tx_info_n_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int dbg_level_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_level_read, inode->i_private);
}
static const struct file_operations dbglevel_fops = {
	.owner = THIS_MODULE,
	.open = dbg_level_open,
	.read = seq_read,
	.write = dbg_level_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int dbg_times_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_times_read, inode->i_private);
}

static const struct file_operations dbgtimes_fops = {
	.owner = THIS_MODULE,
	.open = dbg_times_open,
	.read = seq_read,
	.write = dbg_times_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int rx2tx_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx2tx_read, inode->i_private);
}
static const struct file_operations rx2tx_fops = {
	.owner = THIS_MODULE,
	.open = rx2tx_open,
	.read = seq_read,
	.write = rx2tx_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int rx2txl34csreset_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx2txl34csreset_read, inode->i_private);
}
static const struct file_operations rx2txl34csreset_fops = {
	.owner = THIS_MODULE,
	.open = rx2txl34csreset_open,
	.read = seq_read,
	.write = rx2txl34csreset_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int local2tx_open(struct inode *inode, struct file *file)
{
	return single_open(file, local2tx_read, inode->i_private);
}
static const struct file_operations local2tx_fops = {
	.owner = THIS_MODULE,
	.open = local2tx_open,
	.read = seq_read,
	.write = local2tx_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int txapi_open(struct inode *inode, struct file *file)
{
	return single_open(file, txapi_read, inode->i_private);
}
static const struct file_operations txapi_fops = {
	.owner = THIS_MODULE,
	.open = txapi_open,
	.read = seq_read,
	.write = txapi_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void rtk_ni_test_proc_init(void){

	unsigned char procName[128];
	int i;
	
	if(rtk_ni_test_proc_dir==NULL)
		rtk_ni_test_proc_dir = proc_mkdir(RTK_NI_TEST_PROC_DIR_NAME, NULL);
	if(rtk_ni_test_proc_dir)
	{
		proc_create_data("rxinfo", 0, rtk_ni_test_proc_dir, &rxinfo_fops, &rtk_ni_test_priv);
		proc_create_data("txinfo", 0, rtk_ni_test_proc_dir, &txinfo_fops, &rtk_ni_test_priv);
		for(i=0 ; i<RTK_NI_TEST_TOTAL_TXINFO_NUM ; i++)
		{
			sprintf(procName, "txinfo_%d", i);
			proc_create_data(procName, 0, rtk_ni_test_proc_dir, &txinfo_n_fops, &rtk_ni_test_priv.tx_info[i]);
		}
		proc_create_data("dbglevel", 0, rtk_ni_test_proc_dir, &dbglevel_fops, &rtk_ni_test_priv);
		proc_create_data("dbgtimes", 0, rtk_ni_test_proc_dir, &dbgtimes_fops, &rtk_ni_test_priv);
		proc_create_data("rx2tx", 0, rtk_ni_test_proc_dir, &rx2tx_fops, &rtk_ni_test_priv);
		proc_create_data("rx2txl34csreset", 0, rtk_ni_test_proc_dir, &rx2txl34csreset_fops, &rtk_ni_test_priv);
		proc_create_data("local2tx", 0, rtk_ni_test_proc_dir, &local2tx_fops, &rtk_ni_test_priv);
		proc_create_data("txapi", 0, rtk_ni_test_proc_dir, &txapi_fops, &rtk_ni_test_priv);
	}
}

static int __init rtk_ni_test_init(void)
{
	__rtk_ni_test_init();
	rtk_ni_test_proc_init();	
	return (0);
}

static void __exit rtk_ni_test_exit(void)
{
	printk("exit RTK NI test module\n");
	__rtk_ni_test_exit();
	if(rtk_ni_test_proc_dir!=NULL)
		proc_remove(rtk_ni_test_proc_dir);
}

module_init(rtk_ni_test_init);
module_exit(rtk_ni_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sam Hsu <sam_hsu@realtek.com>");

