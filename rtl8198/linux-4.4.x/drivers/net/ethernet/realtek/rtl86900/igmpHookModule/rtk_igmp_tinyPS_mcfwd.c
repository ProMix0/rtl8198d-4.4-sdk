#include <rtk_igmp_tinyPS_mcfwd.h>
#include <rtk_igmp_snooping.h>
#include <rtk_igmp_debug.h>
#include <rtk_igmp_hook.h>

extern int dev_queue_xmit(struct sk_buff *skb);


#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 1, 0)

int rtk_igmp_br_dev_queue_xmit(struct net *net, struct sock *sk, struct sk_buff *skb)
{
	skb_push(skb, ETH_HLEN);
	br_drop_fake_rtable(skb);
	dev_queue_xmit(skb);
	return 0;
}
int rtk_igmp_br_forward(struct net *net, struct sock *sk, struct sk_buff *skb)
{
	return NF_HOOK(NFPROTO_BRIDGE, NF_BR_POST_ROUTING,net,sk,skb, NULL, skb->dev,rtk_igmp_br_dev_queue_xmit);
}

#else

int rtk_igmp_br_dev_queue_xmit(struct sk_buff *skb)
{
	skb_push(skb, ETH_HLEN);
	br_drop_fake_rtable(skb);
	dev_queue_xmit(skb);
	return 0;
}

int rtk_igmp_br_forward(struct sk_buff *skb)
{
	return NF_HOOK(NFPROTO_BRIDGE, NF_BR_POST_ROUTING, skb, NULL, skb->dev,rtk_igmp_br_dev_queue_xmit);
}

#endif

extern struct rtk_igmp_multicastModule rtk_igmp_mCastModuleArray[IGMP_MAX_BR_MODULE_NUM];

rtk_igmp_nf_ret_e_t rtk_igmp_data_process( struct sk_buff *skb,rtk_igmp_pktHdr_t *pPkthdr, const struct net_device *SrcDev,const struct net_device *DstDev)
{

	int i ;
	int32 ret=RTK_IGMP_NF_DROP;
	struct rtk_igmp_multicastFwdInfo fwdInfo;
	struct rtk_igmp_multicastDataInfo dataInfo;
	int needPassUp = 0;

	//l2 DA/SA buf
	unsigned char *p_eth_hdr=NULL;

	memset(&dataInfo,0,sizeof(dataInfo));
	memset(&fwdInfo,0,sizeof(fwdInfo));


	if(pPkthdr->ingressCtagif)
	{
		dataInfo.vlanId = pPkthdr->ingressCvid;
		dataInfo.vlanTagif=1;
	}
	else
	{
		dataInfo.vlanTagif=0;
	}

	if(pPkthdr->iph)
	{
		p_eth_hdr=((unsigned char *)pPkthdr->iph)-ETH_HLEN;
		dataInfo.ipVersion = IP_VERSION4;
		dataInfo.groupAddr[0] = ntohl(pPkthdr->iph->daddr);
		dataInfo.sourceIp[0] =  ntohl(pPkthdr->iph->saddr);
	}
	else if(pPkthdr->ip6h)
	{
		p_eth_hdr=((unsigned char *)pPkthdr->ip6h)-ETH_HLEN;
		dataInfo.ipVersion = IP_VERSION6;
		memcpy(dataInfo.groupAddr,&pPkthdr->ip6h->daddr.s6_addr[0],sizeof(dataInfo.groupAddr));
		memcpy(dataInfo.sourceIp,&pPkthdr->ip6h->saddr.s6_addr[0],sizeof(dataInfo.sourceIp));
	}

	dataInfo.l4proto = pPkthdr->l4proto;
	if(pPkthdr->udph)
	{
		dataInfo.dport = ntohs(pPkthdr->udph->dest);
		dataInfo.sport = ntohs(pPkthdr->udph->source);
	}
	else
	{
		dataInfo.dport = FAIL;
		dataInfo.sport = FAIL;
	}

	igmp_spin_lock_bh(igmpSysdb.lock_igmp);
	ret = rtk_igmp_getMulticastDataFwdInfo(SrcDev,DstDev,1,&dataInfo,&fwdInfo,&pPkthdr->moduleIndex);
	igmp_spin_unlock_bh(igmpSysdb.lock_igmp);

	if(ret !=SUCCESS)
	{
		if(fwdInfo.reservedMCast)
		{
			IGMP_DATA("Reserved multicast Keeping NF_ACCEPT to Protocol-Stack");
			return RTK_IGMP_NF_ACCEPT;
		}
		else if (fwdInfo.dropPkt)
		{
			IGMP_DATA("fwd non member dropPkt");
			return RTK_IGMP_NF_DROP;
		}
		else if (fwdInfo.unknownMCast)
		{
			IGMP_DATA("unknownMCast pkt");
			//return RTK_IGMP_NF_DROP;  //maybe disable snooping do not dorp
		}
		else
		{
			IGMP_DATA("rtk_igmp_getMulticastDataFwdInfo ret =%d",ret);
		}
		return RTK_IGMP_NF_ACCEPT;
	}

	//change sa to DstDev if routing multicast Data
	if(!SrcDev && DstDev && DstDev->dev_addr &&(DstDev->dev_addr[0]|DstDev->dev_addr[1]|DstDev->dev_addr[2]|DstDev->dev_addr[3]|DstDev->dev_addr[4]|DstDev->dev_addr[5]))
	{

		//change SA
		IGMP_DATA("Multicast Routing Change SA to %pM",DstDev->dev_addr);
		memcpy(p_eth_hdr+ETH_ALEN,DstDev->dev_addr,ETH_ALEN);

		//change DA/ETHTYPE
		if(pPkthdr->ip6h)
		{
			p_eth_hdr[0]=0x33;
			p_eth_hdr[1]=0x33;
			p_eth_hdr[2]=pPkthdr->ip6h->daddr.in6_u.u6_addr8[12];
			p_eth_hdr[3]=pPkthdr->ip6h->daddr.in6_u.u6_addr8[13];
			p_eth_hdr[4]=pPkthdr->ip6h->daddr.in6_u.u6_addr8[14];
			p_eth_hdr[5]=pPkthdr->ip6h->daddr.in6_u.u6_addr8[15];
			p_eth_hdr[12]=0x86;
			p_eth_hdr[13]=0xdd;
		}
		else if(pPkthdr->iph)
		{
			p_eth_hdr[0]=0x01;
			p_eth_hdr[1]=0x00;
			p_eth_hdr[2]=0x5e;
			p_eth_hdr[3]=(ntohl(pPkthdr->iph->daddr) & 0x007f0000)>>16;
			p_eth_hdr[4]=(ntohl(pPkthdr->iph->daddr) & 0xff00)>>8;
			p_eth_hdr[5]=(ntohl(pPkthdr->iph->daddr) & 0xff);
			p_eth_hdr[12]=0x08;
			p_eth_hdr[13]=0x00;
		}
		//skb_push(skb, hh_cacheBufLen);
		//skb_reset_mac_header(skb);
	}
	else
	{
		//if multicast data from port of br-wan , pass up data
		if(memcmp(rtk_igmp_mCastModuleArray[pPkthdr->moduleIndex].brDevInfo.devName,"br-wan",sizeof("br-wan")-1)==0)
		{
			IGMP_DATA("br-wan port get multicast data, also need  passUp packet to protol to handle ");
			needPassUp = 1;
 		}
	}

	for(i=0 ;i< IGMP_MAX_DEV_NUM ;i++)
	{
 		struct sk_buff *send_skb;
 		struct net_device *indev,*outdev;

 		if(fwdInfo.egressDevInfo[i].valid)
 		{
			if(SrcDev  && fwdInfo.egressDevInfo[i].devIfIdx ==SrcDev->ifindex)
			{
				IGMP_DATA("Skip send to Dev:%s[%d] By Source Dev Filter",SrcDev->name,SrcDev->ifindex);
				continue;
			}

	 		indev = skb->dev;
	 		outdev = rtk_igmp_devIfidx_to_dev(fwdInfo.egressDevInfo[i].devIfIdx);

			if(indev == NULL || outdev==NULL)
			{
				IGMP_DATA("NULL indev/outdev send");
				continue;
			}

	 		send_skb = skb_copy(skb, GFP_ATOMIC);
	 		send_skb->dev =outdev;
	 		skb_forward_csum(send_skb);

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 1, 0)
			if(DstDev)
			{
				if(igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_DUMP)
					igmp_dump_packet(skb_mac_header(skb)-ETH_HLEN,skb->len+skb->mac_len+ETH_HLEN,"IGMP Egress");
				IGMP_DATA("Continue to check NF_BR_LOCAL_OUT/NF_BR_POST_ROUTING and Forward to Dev:%s[%d]",outdev->name,outdev->ifindex);
				NF_HOOK(NFPROTO_BRIDGE, NF_BR_LOCAL_OUT,dev_net(indev), NULL, send_skb, NULL, send_skb->dev,rtk_igmp_br_forward);
			}
			else
			{
				if(igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_DUMP)
					igmp_dump_packet(skb_mac_header(skb),skb->len+skb->mac_len,"IGMP Egress");
				IGMP_DATA("Continue to check NF_BR_FORWARD/NF_BR_POST_ROUTING and Forward to Dev:%s[%d]",outdev->name,outdev->ifindex);
				NF_HOOK(NFPROTO_BRIDGE, NF_BR_FORWARD, dev_net(indev), NULL,send_skb, indev, send_skb->dev,rtk_igmp_br_forward);
			}

#else
			if(DstDev)
			{
				if(igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_DUMP)				
					igmp_dump_packet(skb_mac_header(skb)-ETH_HLEN,skb->len+skb->mac_len+ETH_HLEN,"IGMP Egress");
				IGMP_DATA("Continue to check NF_BR_LOCAL_OUT/NF_BR_POST_ROUTING and Forward to Dev:%s[%d]",outdev->name,outdev->ifindex);
				NF_HOOK(NFPROTO_BRIDGE, NF_BR_LOCAL_OUT, send_skb, NULL, send_skb->dev,rtk_igmp_br_forward);
			}
			else
			{
				if(igmpSysdb.igmp_debug_level&RTK_IGMP_DEBUG_LEVEL_DUMP)
					igmp_dump_packet(skb_mac_header(skb),skb->len+skb->mac_len,"IGMP Egress");
				IGMP_DATA("Continue to check NF_BR_FORWARD/NF_BR_POST_ROUTING and Forward to Dev:%s[%d]",outdev->name,outdev->ifindex);
	 			NF_HOOK(NFPROTO_BRIDGE, NF_BR_FORWARD,send_skb, indev, send_skb->dev,rtk_igmp_br_forward);
			}
#endif
 		}
		else
			break;
	}
	if(needPassUp)
	{
		IGMP_DATA("Return accept , Let igmpPorxy handle mulitcast packet too");
		return RTK_IGMP_NF_ACCEPT;
	}
	else
	{
		return RTK_IGMP_NF_DROP;
	}
}



