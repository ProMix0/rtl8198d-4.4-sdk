//#include <linux/config.h>
//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <linux/compiler.h>
#include <generated/ca_ne_autoconf.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/list.h>
#include <linux/igmp.h>
#include <net/addrconf.h>

#include "ca_ni.h"
#include "ca_ext.h"
#include "ca_ni_proc.h"
#include "ca_ni_dump.h"
#include "ca_mut.h"
#include "aal_hash.h"
#include "nat.h"
#include "aal_common.h"
#include "flow.h"

#ifdef CONFIG_RTL867X_IPTABLES_FAST_PATH
int FastPath_Check(struct sk_buff *skb);
#endif

#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
rtk_sw_mib_t cpuUsMib;
int rtk_nic_cpu_us_mib_counting(unsigned char *dmac, unsigned int pktlen)
{
	if(dmac[0] == 0xff) {
		cpuUsMib.bc_pkt += 1;
		cpuUsMib.bc_bytes += pktlen;
	}else if((dmac[0]&0x1) == 0x01) {
		cpuUsMib.mc_pkt += 1;
		cpuUsMib.mc_bytes += pktlen;
	}else {
		cpuUsMib.uc_pkt += 1;
		cpuUsMib.uc_bytes += pktlen;
	}

	return 0;
}
int rtk_nic_cpu_us_mib_get(rtk_sw_mib_t *pCpuUsMib)
{
	memcpy(pCpuUsMib, &cpuUsMib, sizeof(cpuUsMib));
	return 0;
}
int rtk_nic_cpu_us_mib_clear(void)
{
	memset(&cpuUsMib, 0, sizeof(cpuUsMib));
	return 0;
}

EXPORT_SYMBOL(rtk_nic_cpu_us_mib_counting);
EXPORT_SYMBOL(rtk_nic_cpu_us_mib_get);
EXPORT_SYMBOL(rtk_nic_cpu_us_mib_clear);

#if defined(CONFIG_ARCH_REALTEK_TAURUS)
rtk_tx_dma_lso_mode_t rtk_dma_lso_mode_check(struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config)
{
	rtk_tx_dma_lso_mode_t dma_lso_mode = RTK_TX_MODE_DEFAULT;

	if((tx_config->core_config.bf.sw_id != 0) || (skb && dev && (skb->len > dev->mtu))){	//hdr_cpu, segment
		dma_lso_mode = RTK_TX_MODE_DEFAULT;
	}else if((tx_config->core_config.bf.ldpid == AAL_LPORT_ETH_NI7) && (skb && (skb->len < ((0x1<<11) - 1)))){	//buf_len only 11 bit
		dma_lso_mode = RTK_TX_MODE_DIRECT_TX_WAN;
	}
	else if((tx_config->core_config.bf.ldpid < AAL_LPORT_ETH_NI7) && (skb && (skb->len < ((0x1<<11) - 1)))){	//buf_len only 11 bit
		dma_lso_mode = RTK_TX_MODE_DIRECT_TX_LAN;
	}
	else if(((tx_config->core_config.bf.ldpid == AAL_LPORT_L3_WAN) || (tx_config->core_config.bf.ldpid == AAL_LPORT_L3_LAN)) && 
			tx_config->bypass_lso && skb && (skb->len < ((0x1<<14) - 1))){	//buf_len only 14 bit
		dma_lso_mode = RTK_TX_MODE_WIFI_HW_LOOKUP;
	}

	return dma_lso_mode;
}
#endif
#endif

#ifdef CONFIG_RTK_NIC_RX_HOOK
extern int ca_ni_num_intf;
/*
typedef struct drv_nic_hook_entry
{
	int portmask;
	int priority;
	p2rfunc_t do_rx;
	struct list_head list;

}drv_nic_hook_entry_t ;
*/
/* for rx handler mantian*/
static struct list_head rxHookHead;

#ifdef CONFIG_RTK_HOST_SPEEDUP
extern int drv_nic_register_rxhook(int portmask,int priority,p2rfunc_t rx);
extern int ca_ni_host_speedup(struct napi_struct *napi,struct net_device *dev, struct sk_buff *skb, nic_hook_private_t *nh_priv);
#endif
void ca_ni_netlink_igmpsnooping_reply(int pid, int seq, ca_uint8_t *skb_data, ca_uint32_t data_len);
void ca_ni_netlink_send_packet(ca_uint32_t lspid, ca_uint8_t *data, ca_uint32_t len, ca_uint32_t mirror, HEADER_CPU_T *hdr_cpu);
int mac_debug_txCb_register(netdev_tx_t (*pCb)(char *buf, ca_uint32_t len, dma_txbuf_t *pTx_headers, dma_txbuf_t *pHeader_mask));


static int nic_insert_entry(drv_nic_hook_entry_t *entry)
{

	struct list_head *next = NULL, *tmp=NULL;
	drv_nic_hook_entry_t *cur = NULL;
	//printk(KERN_INFO "[%s-%d]\n", __func__,__LINE__);
	int notEmpty=0;
	list_for_each_safe(next,tmp,&rxHookHead){

		cur = list_entry(next,drv_nic_hook_entry_t,list);

		if(cur == NULL){
			return -1;
		}
		//printk("[%s-%d]priority: %d, portmask %d, rx: %p\n",__func__,__LINE__,cur->priority,cur->portmask,cur->do_rx);

	    if(cur->priority <= entry->priority){
			cur->list.prev->next = &entry->list;
			entry->list.next = &cur->list;
			entry->list.prev = cur->list.prev;
			cur->list.prev = &entry->list;
			printk("insert here, priority: %d, portmask %d, rx: %p\n",entry->priority,entry->portmask,entry->do_rx);
			return 0;
		}
		notEmpty = 1;
	}
	/*can't search means it's the first entry*/
	if(notEmpty)
	{
printk("not first entry: %d, portmask %d, rx: %p\n",entry->priority,entry->portmask,entry->do_rx);	
		list_add_tail(&entry->list,&rxHookHead);
	}
	else
	{/*first entry*/
printk("first entry: %d, portmask %d, rx: %p\n",entry->priority,entry->portmask,entry->do_rx);	
		list_add(&entry->list,&rxHookHead);
	}


	return 0;
}


/*	Purpose: Used for hook rx callback function
*	Parameters:
*  		portmask - this callback function want to receive from which ports
*		priority	- the priority for callback fun, 0~100, 0: lowest, 100: highest
*		rx		- callback function
*		Note-
*			Please update the priority of your callback in here and the doc file in
*			http://cadinfo.realtek.com.tw/svn/CN/Switch/trunk/dumb/document/Apollo/Internal Spec/NIC/[Application Note] GMAC_v1.0.0_20120806.doc
*
*/
int nic_register_rxhook(int portmask,int priority,p2rfunc_t rx)
{

	drv_nic_hook_entry_t *entry;
	entry = (drv_nic_hook_entry_t*)kmalloc(sizeof(drv_nic_hook_entry_t),GFP_KERNEL);
	if(!entry){
		return -1;
	}
	printk(KERN_INFO "[%s-%d] portmask=%x, pri:%d, rx=%p\n", __func__,__LINE__, portmask,priority,rx);
	/*assigned value to entry*/
	entry->do_rx = rx;
	entry->portmask = portmask;
	entry->priority = priority;
	/*insert entry depend on priority*/
	nic_insert_entry(entry);
	return NIC_DRIVER_IS_READY;
}


int nic_unregister_rxhook(int portmask,int priority,p2rfunc_t rx)
{

	struct list_head *next = NULL, *tmp=NULL;
	drv_nic_hook_entry_t *cur = NULL;
	printk(KERN_INFO "[%s-%d]\n", __func__,__LINE__);

	list_for_each_safe(next,tmp,&rxHookHead){

		cur = list_entry(next,drv_nic_hook_entry_t,list);

		if(cur == NULL){
			return -1;
		}

	    	if(cur->do_rx == rx && cur->portmask == portmask && cur->priority==priority){
			list_del(&cur->list);
			kfree(cur);
			return 0;
		}
	}
	return -1;
}


static void nic_rxhook_clear(void)
{

	struct list_head *next = NULL, *tmp=NULL;
	drv_nic_hook_entry_t *cur = NULL;
	printk(KERN_INFO "[%s-%d]\n", __func__,__LINE__);

	list_for_each_safe(next,tmp,&rxHookHead){

		cur = list_entry(next,drv_nic_hook_entry_t,list);

	    	if(cur!=NULL){
			list_del(&cur->list);
			kfree(cur);
		}
	}
	return;
}


/*__IRAM*/
static int nic_rx_list(struct napi_struct *napi,struct net_device *dev, struct sk_buff *skb, nic_hook_private_t *nh_priv)//, struct rx_info *pRxInfo)
{

	struct list_head *next = NULL, *tmp=NULL;
	drv_nic_hook_entry_t *cur = NULL;
	int ret;
	int portNum;
#ifdef CONFIG_RTL867X_IPTABLES_FAST_PATH
	int skip_rg=0;
#endif

	ca_eth_private_t *cep = netdev_priv(napi->dev);
    //portNum = pRxInfo->opts3.bit.src_port_num;
	if(cep == NULL){
		printk(KERN_INFO "[%s-%d] cep == NULL\n", __func__,__LINE__);
		return 0;
	}

#ifdef CONFIG_RTL867X_IPTABLES_FAST_PATH
	skip_rg = FastPath_Check(skb);
#endif

	list_for_each_safe(next,tmp,&rxHookHead){

		cur = list_entry(next,drv_nic_hook_entry_t,list);
		if(cur == NULL){
			printk(KERN_INFO "[%s-%d] cur == NULL\n", __func__,__LINE__);
			return -1;
		}
#if defined(CONFIG_RTL_MULTI_LAN_DEV)		
		if(dev==NULL){
			if(skb){
				__kfree_skb(skb);
			}
			printk(KERN_INFO "[%s-%d] dev_port_mapping fail\n", __func__,__LINE__);
			return -1;
		}
#endif		
		//if(cur->portmask & 1 << portNum){
#ifdef CONFIG_RTL867X_IPTABLES_FAST_PATH
			if(skip_rg && cur->priority == RENIC_RXPRI_RG)
				continue;
#endif

			ret = cur->do_rx(napi,dev,skb,nh_priv);//,pRxInfo);
			
			if(ret==RE8670_RX_STOP){
				if(skb){
					__kfree_skb(skb);
				}
				return 0;
		    }
			else if(ret==RE8670_RX_STOP_SKBNOFREE)	//return without free skb
				return 0;
		//}
	}
	return 0;
}
extern int ca_ni_num_intf;
/*
int renic_rxhook_test (struct napi_struct *napi,struct net_device *dev, struct sk_buff *skb, HEADER_A_T *hdr_a, HEADER_CPU_T *hdr_cpu)
{
	int i;
	ca_eth_private_t *cep = netdev_priv(napi->dev);
	//cep->port_cfg.rx_ldpid;
	printk("[%s-%d]\n",__func__,__LINE__);

	//printk("[%s-%d] rx_ldpid=%d, tx_ldpit=%d\n",__func__,__LINE__,cep->port_cfg.rx_ldpid,cep->port_cfg.tx_ldpid);
	//printk("[%s-%d] status=%x, num_phy_port=%d\n",__func__,__LINE__,cep->status,cep->port_cfg.num_phy_port);
#if 0
	for ( i = 0; i < ca_ni_num_intf; i++) 
	{
	
	}
#endif
	return RE8670_RX_CONTINUE;
}
*/
void dump_port_cfg(ca_port_cfg_info_t *port_cfg)
{
	if (ca_ni_debug & NI_DBG_DUMP_RX) {
		int i;
		printk("[%s-%d] link=%d\n",__func__,__LINE__,port_cfg->link);
		printk("[%s-%d] autoneg=%d\n",__func__,__LINE__,port_cfg->autoneg);
		printk("[%s-%d] speed=%d\n",__func__,__LINE__,port_cfg->speed);
		printk("[%s-%d] duplex=%d\n",__func__,__LINE__,port_cfg->duplex);
		printk("[%s-%d] flowctrl=%d\n",__func__,__LINE__,port_cfg->flowctrl);
		printk("[%s-%d] num_phy_port=%d\n",__func__,__LINE__,port_cfg->num_phy_port);
		for(i=0;i<CA_NI_NUM_OF_PORT;i++)
		{
			printk("[%s-%d] port_id[%d]=%d\n",__func__,__LINE__,i,port_cfg->port_id[i]);
		}
		printk("[%s-%d] rx_ldpid=%d\n",__func__,__LINE__,port_cfg->rx_ldpid);
		printk("[%s-%d] tx_ldpid=%d\n",__func__,__LINE__,port_cfg->tx_ldpid);	
		printk("[%s-%d] irq=%d\n",__func__,__LINE__,port_cfg->irq);
		if(port_cfg->mac_addr != NULL)
			printk("[%s-%d] mac=%02X:%02X:%02X:%02X:%02X:%02X\n",__func__,__LINE__,port_cfg->mac_addr[0],port_cfg->mac_addr[1],port_cfg->mac_addr[2],port_cfg->mac_addr[3],port_cfg->mac_addr[4],port_cfg->mac_addr[5]);
	}
}

int nic_rxhook_default(struct napi_struct *napi,struct net_device *dev, struct sk_buff *skb, nic_hook_private_t *nh_priv)
{

	ca_eth_private_t *cep = netdev_priv(napi->dev);
	//ca_eth_private_t *cep = netdev_priv(skb->dev);
	struct net_device_stats *stats = &cep->dev->stats;
    u16 vid;
	int ret;
	ca_uint16_t ethertype;
	int vlan_offset,pkt_len;
	ca_uint8_t *tmp_ptr;


	skb->protocol = eth_type_trans(skb, skb->dev);
	skb->pkt_type = PACKET_HOST;
	/* end of fill the sturcture of skb */
	//dump_port_cfg(&(cep->port_cfg));
#if 0 //maybe let RG forwarding engine to handle it.

#if defined(CONFIG_HWNAT_RG) || defined(CONFIG_HWNAT_RG_MODULE) || defined(CONFIG_HWNAT_FLEETCONNTRACK)
	// L2 will be learned in HWNAT driver
#else
	/* call ca_l2_addr_add() to add a FDB entry */
	/* if hdr type = 2 (CPU bound for l2 learning then call l2 addr add */
	if (nh_priv->hdr_a->bits.hdr_type == 0x2) {
		//vid = ca_ni_rx_get_vid(nh_priv->save_skb_data);
		vid = ca_ni_rx_get_vid(skb->data);
		ca_ni_l2_addr_add(vid, &(nh_priv->sa_mac), nh_priv->hdr_a->bits.lspid, nh_priv->hdr_a->bits.bits_32_63.pkt_info.mcgid & 0x7);

		/* should check if UUC packet flooding to other ports in the same flooding domain */
		ca_ni_l2_flooding(vid, nh_priv->da_mac, skb, napi, nh_priv->hdr_a);
	}
#endif	

	/*memo: just User space application can use ca_rx() to monitor these packets, duplicate the packet to user space by this API*/
	/* call netlink to send packet to user space packet size should decrement by 4 (CRC) */
	//ca_ni_netlink_send_packet(nh_priv->hdr_a->bits.lspid, nh_priv->save_skb_data, nh_priv->hdr_a->bits.pkt_size - 4, nh_priv->hdr_a->bits.bits_32_63.pkt_info.mirror, nh_priv->hdr_cpu);

	/* call netlink igmpsnooping reply if we got a IGMP or MLD control packet */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
	//ethertype = ca_ni_parse_l2(nh_priv->save_skb_data, NULL, NULL, &tmp_ptr, NULL, false);
	ethertype = ca_ni_parse_l2(skb->data, NULL, NULL, &tmp_ptr, NULL, false);
	//vlan_offset = tmp_ptr - (nh_priv->save_skb_data + 12); /* offset from MAC_DA to EtherType within packet */
	vlan_offset = tmp_ptr - (skb->data + 12); /* offset from MAC_DA to EtherType within packet */
	
	if (ethertype == ntohs(ETH_P_IP)) {
		skb_set_network_header(skb, vlan_offset);
		skb_set_transport_header(skb, vlan_offset);
		ret = ip_mc_check_igmp(skb, NULL);
		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			if (!ret) {
				printk("%s: It's a IPv4 IGMP packet\n", __func__);
			}
		}
	}
#if defined(CONFIG_IPV6)
	else if (ethertype == ntohs(ETH_P_IPV6)) {
		skb_set_network_header(skb, vlan_offset);
		skb_set_transport_header(skb, vlan_offset);
		ret = ipv6_mc_check_mld(skb, NULL);
		if (ca_ni_debug & NI_DBG_DUMP_RX) {
			if (!ret) {
				printk("%s: It's a IPv6 IGMP packet\n", __func__);
			}
		}
	}
#endif
	if (!ret)
#endif
	//ca_ni_netlink_igmpsnooping_reply(0, 1, nh_priv->save_skb_data, nh_priv->hdr_a->bits.pkt_size - 4);

	//record status befroe RTK rx hook
	/* modify the statistics count of stats */
	if (cep != NULL) {
		int extra_padding = 0;
		if (pkt_len > (dev->mtu + ETH_HLEN + extra_padding)) {
			stats->rx_errors++;
			stats->rx_bytes += pkt_len;
			pr_err("%s:: RX [ldpid %d] pkt_len %d > %d\n", __func__, cep->port_cfg.rx_ldpid, pkt_len, dev->mtu + ETH_HLEN + extra_padding);
		} else {
			stats->rx_packets++;
			stats->rx_bytes += skb->len;
		}
	}
#endif	
	ca_skb_out(skb);

#ifdef CONFIG_RTK_APP_PORT_BINDING_SUPPORT
	skb->from_dev = skb->dev;
#endif

	if (unlikely(napi && ((dev->features) & NETIF_F_GRO))) {
		napi_gro_receive(napi, skb);
	}
	else{
		netif_receive_skb(skb);
	}
	
	return RE8670_RX_STOP_SKBNOFREE;
}


netdev_tx_t nic_egress_start_xmit(struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config)
{
	netdev_tx_t ret = NETDEV_TX_OK;
	bool use_two_des_tx = FALSE;

#if defined(CONFIG_ARCH_REALTEK_TAURUS)

	if(unlikely(((rtk_dma_lso_mode_check(skb, dev, tx_config) == 0) && (skb_headroom(skb) < sizeof(dma_txbuf_t))) 
		|| skb_header_cloned(skb) || (skb_shinfo(skb)->nr_frags != 0)))
		use_two_des_tx = TRUE;

#else
#if defined(CONFIG_HWNAT_FLEETCONNTRACK)

	if(unlikely((skb_headroom(skb) < sizeof(dma_txbuf_t)) || skb_header_cloned(skb) || (skb_shinfo(skb)->nr_frags != 0)))
		use_two_des_tx = TRUE;

#endif	//CONFIG_HWNAT_FLEETCONNTRACK
#endif

	if(use_two_des_tx) {
		
		ret = __ca_ni_start_xmit(skb, NULL, tx_config);
		
	}else{

#if defined(CONFIG_HWNAT_FLEETCONNTRACK)
		if((tx_config->bypass_lso) && (tx_config->core_config.bf.flow_id_set == FALSE)) {
			// no necessary to insert headers before skb data, call xmit directly.
			
		}
		else
#endif
		{
			// be safe
			if(skb_cow_head(skb, sizeof(dma_txbuf_t))<0)
			{
				printk("skb head room is not enough\n");
				return 0xff;		// netdev_tx_t
			}
		}
			
		ret = __ca_ni_start_xmit_buf(skb, NULL, tx_config);
		
	}
	
	if(unlikely(ret != NETDEV_TX_OK))
		ca_dev_kfree_skb_any(skb);

	return NETDEV_TX_OK;
}

#if defined(CONFIG_ARCH_CORTINA_VENUS)
netdev_tx_t nic_egress_debugStart_xmit(char *buf, ca_uint32_t len, dma_txbuf_t *pTx_headers, dma_txbuf_t *pHeader_mask)
{
	netdev_tx_t ret = NETDEV_TX_OK;
    char *pBuf;
	
    /* Allocate new buffer for head room */
    pBuf = (char *) kmalloc(sizeof(char) * (sizeof(dma_txbuf_t) + len), GFP_KERNEL);
    if(!pBuf)
    {
        return NETDEV_TX_BUSY;
    }

    memcpy(pBuf + sizeof(dma_txbuf_t), buf, len);

	ret = ca_ni_debugStart_xmit_buf(pBuf + sizeof(dma_txbuf_t), len, pTx_headers, pHeader_mask);
    kfree(pBuf);

	
	return ret;
}
#endif

void nic_ca_skb_in(struct sk_buff *skb, const char *fname, int line, bool trace)
{
	__ca_skb_in(skb, fname, line, trace);
}
void nic_ca_skb_out(struct sk_buff *skb, const char *fname, int line)
{
	__ca_skb_out(skb, fname, line);
}



extern void nic_hook_rx_register(p2NicRxHook_t nicRxHook);
extern void nic_unhook_rx_register(p2NicRxUnHook_t nicRxUnHook);
extern void nic_reset_rxfunc_to_default(void);
extern int nic_register_rxfunc_all_port(p2rfunc_t pfunc);




int nic_rxhook_init(void)
{
#if 0
    volatile unsigned int data1;
    unsigned int data2;
#endif
	printk(KERN_INFO "%s: init nic RX Hook functions.\n", __func__);
	printk(KERN_INFO "%s: nic_register_rxhook=%p\n", __func__,&nic_register_rxhook);
	printk(KERN_INFO "%s: nic_unregister_rxhook=%p\n", __func__,&nic_unregister_rxhook);
	INIT_LIST_HEAD(&rxHookHead);

#if defined(CONFIG_ARCH_REALTEK_TAURUS)
	printk(KERN_INFO "%s: FIXME - nic_hook_rx_register and nic_unhook_rx_register\n", __func__);
#else
	nic_hook_rx_register(&nic_register_rxhook);
	nic_unhook_rx_register(&nic_unregister_rxhook);
#endif

	/*add for default re8686 rx handler*/
	//drv_nic_register_rxhook(0x7f,RENIC_RXPRI_DEFAULT,re8670_rx_skb);
	nic_register_rxhook(0x7f,RENIC_RXPRI_DEFAULT,nic_rxhook_default);

	//drv_nic_register_rxhook(0x7f,RENIC_RXPRI_DUMP,renic_rxhook_test);
#ifdef CONFIG_RTK_HOST_SPEEDUP
	drv_nic_register_rxhook(0x7f,RENIC_RXPRI_PATCH, ca_ni_host_speedup);
#endif

	/*register to renic netdev*/
	nic_register_rxfunc_all_port(&nic_rx_list);

#if defined(CONFIG_ARCH_CORTINA_VENUS) && !defined(CONFIG_ARCH_REALTEK_TAURUS)
	/* register to MAC debug module */
	mac_debug_txCb_register(&nic_egress_debugStart_xmit);
#endif

	return 0;
}

void nic_rxhook_exit(void)
{
	printk(KERN_INFO "%s: exit nic RX Hook functions.\n", __func__);

	/*register to re8686 netdev*/
	nic_reset_rxfunc_to_default();
	/*clear all hook*/
	nic_rxhook_clear();
}

EXPORT_SYMBOL(nic_egress_start_xmit);
#if defined(CONFIG_ARCH_CORTINA_VENUS) && !defined(CONFIG_ARCH_REALTEK_TAURUS)
EXPORT_SYMBOL(nic_egress_debugStart_xmit);
#endif
EXPORT_SYMBOL(nic_ca_skb_in);
EXPORT_SYMBOL(nic_ca_skb_out);
EXPORT_SYMBOL(nic_register_rxhook);
EXPORT_SYMBOL(nic_unregister_rxhook);
EXPORT_SYMBOL(nic_rxhook_init);
EXPORT_SYMBOL(nic_rxhook_exit);
EXPORT_SYMBOL(nic_rxhook_default);


int aal_l3fe_keep_lspid_unchange_get(ca_device_id_t device_id, ca_uint8_t *config)
{
	int ret;
	L3FE_STG0_CTRL_t stg0_ctrl;

	if ((ret = aal_l3fe_stg0_ctrl_get(device_id, &stg0_ctrl)) != AAL_E_OK) {
		ca_printf("%s: Fail to get STG0 ctrl. (ret=%d)\n", __func__, ret);
		return ret;
	}
	printk("=== device_id %u ===\n", device_id);
	printk("=== L3FE_STG0_CTRL : 0x%08x ===\n", stg0_ctrl.wrd);
	printk("keep_lspid_unchange=%u\n", stg0_ctrl.bf.keep_lspid_unchange);

	*config=stg0_ctrl.bf.keep_lspid_unchange;

	return CA_E_OK;
}

int aal_l3fe_keep_lspid_unchange_set(ca_device_id_t device_id, ca_uint8_t *config)
{
	int ret;
	L3FE_STG0_CTRL_t stg0_ctrl;

	if ((ret = aal_l3fe_stg0_ctrl_get(device_id, &stg0_ctrl)) != AAL_E_OK) {
		ca_printf("%s: Fail to get STG0 ctrl. (ret=%d)\n", __func__, ret);
		return ret;
	}
	printk("=== device_id %u ===\n", device_id);
	printk("=== L3FE_STG0_CTRL : 0x%08x ===\n", stg0_ctrl.wrd);
	printk("keep_lspid_unchange=%u\n", stg0_ctrl.bf.keep_lspid_unchange);

	stg0_ctrl.bf.keep_lspid_unchange=*config;

	if ((ret = aal_l3fe_stg0_ctrl_set(0, &stg0_ctrl)) != AAL_E_OK) {
		ca_printf("%s: Fail to set STG0 ctrl. (ret=%d)\n", __func__, ret);
		return ret;
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3fe_keep_lspid_unchange_get);
EXPORT_SYMBOL(aal_l3fe_keep_lspid_unchange_set);

int ca_flow_age_set(ca_device_id_t device_id, ca_uint32_t hash_idx, ca_uint32_t age)
{
	int ret = 0, time = 0;

	if (hash_idx & CA_L3_NAT_ID_OVERFLOW_FLAG)
		ret = aal_hash_age_set(AAL_TABLE_HASH_OVERFLOW_TBL, hash_idx & CA_L3_NAT_HASH_IDX_MASK, age);
	else
		ret = aal_hash_age_set(AAL_TABLE_HASH_HASH_TBL, hash_idx, age);

	
	return ret;

}
int ca_flow_age_get(ca_device_id_t device_id, ca_uint32_t hash_idx, ca_uint32_t *age)
{
	int ret = 0, time = 0;

	if(hash_idx & CA_L3_NAT_ID_OVERFLOW_FLAG)
		ret = aal_hash_age_get(AAL_TABLE_HASH_OVERFLOW_TBL, hash_idx, age, &time);
	else
		ret = aal_hash_age_get(AAL_TABLE_HASH_HASH_TBL, hash_idx, age, &time);


	return ret;

}
EXPORT_SYMBOL(ca_flow_age_set);
EXPORT_SYMBOL(ca_flow_age_get);

/* get  traffic bits per 32 flow */
int ca_flow_traffic_status_get(ca_device_id_t device_id, ca_uint32_t hash_idx, ca_uint32_t *trfStatus)
{
	int ret = 0, time = 0;

	if(hash_idx & CA_L3_NAT_ID_OVERFLOW_FLAG)
		ret = aal_hash_traffic_status_get(AAL_TABLE_HASH_OVERFLOW_TBL, hash_idx & CA_L3_NAT_HASH_IDX_MASK, trfStatus);
	else
		ret = aal_hash_traffic_status_get(AAL_TABLE_HASH_HASH_TBL, hash_idx, trfStatus);


	return ret;

}
EXPORT_SYMBOL(ca_flow_traffic_status_get);
#endif

#ifdef CONFIG_RTK_NIC_TX_HOOK
extern int nic_register_txfunc(p2tfunc_t pfunc);
extern int nic_register_txfunc_default(void);

int nic_register_txhook(p2tfunc_t tx)
{
	nic_register_txfunc(tx);
	return NIC_DRIVER_IS_READY;
}

int nic_txhook_init(void)
{
	printk(KERN_INFO "%s: init nic TX Hook functions.\n", __func__);
	nic_register_txfunc_default();
	return 0;
}

int nic_txhook_exit(void)
{
	printk(KERN_INFO "%s: init nic TX Hook functions.\n", __func__);
	nic_register_txfunc_default();
	return 0;
}
EXPORT_SYMBOL(nic_register_txhook);
EXPORT_SYMBOL(nic_txhook_init);
EXPORT_SYMBOL(nic_txhook_exit);
#endif


#ifdef CONFIG_LUNA_G3_SERIES

uint32_t rtk_ne_reg_read(uint32_t addr)
{
	return ca_ne_reg_read(0, addr);
}

void rtk_ne_reg_write(uint32_t data, uint32_t addr)
{
	ca_ne_reg_write(0, data, addr);
}
EXPORT_SYMBOL(rtk_ne_reg_read);
EXPORT_SYMBOL(rtk_ne_reg_write);

uint32_t rtk_dma_lso_reg_read(uint32_t addr)
{
	return CA_DMA_LSO_REG_READ(addr);
}

void rtk_dma_lso_reg_write(uint32_t data, uint32_t addr)
{
	CA_DMA_LSO_REG_WRITE(data, addr);
}
EXPORT_SYMBOL(rtk_dma_lso_reg_read);
EXPORT_SYMBOL(rtk_dma_lso_reg_write);

struct net_device * rtk_ni_virtual_cpuport_alloc(int cpuPort, char *name)
{
	if(cpuPort<AAL_LPORT_CPU_2 || cpuPort>AAL_LPORT_CPU_7)
		return NULL;
	else
		return ca_ni_virtual_instance_alloc(cpuPort, name);
}
uint32_t rtk_ni_virtual_cpuport_free(int cpuPort)
{
	if(cpuPort<AAL_LPORT_CPU_2 || cpuPort>AAL_LPORT_CPU_7)
		return CA_E_PORT;
	else
		return ca_ni_virtual_instance_free(cpuPort-AAL_LPORT_CPU_0);
}
uint32_t rtk_ni_virtual_cpuport_open(int cpuPort)
{
	if(cpuPort<AAL_LPORT_CPU_2 || cpuPort>AAL_LPORT_CPU_7)
		return CA_E_PORT;
	else{
		
		nic_register_rxfunc_by_port(cpuPort-AAL_LPORT_CPU_0, &nic_rx_list);
			
		return ca_ni_virtual_instance_open(cpuPort-AAL_LPORT_CPU_0);
	}
}
uint32_t rtk_ni_virtual_cpuport_close(int cpuPort)
{
	if(cpuPort<AAL_LPORT_CPU_2 || cpuPort>AAL_LPORT_CPU_7)
		return CA_E_PORT;
	else{
		
		return ca_ni_virtual_instance_close(cpuPort-AAL_LPORT_CPU_0);
	}
}

EXPORT_SYMBOL(rtk_ni_virtual_cpuport_alloc);
EXPORT_SYMBOL(rtk_ni_virtual_cpuport_free);
EXPORT_SYMBOL(rtk_ni_virtual_cpuport_open);
EXPORT_SYMBOL(rtk_ni_virtual_cpuport_close);

#if !defined(CONFIG_ARCH_REALTEK_TAURUS)
extern void flow_table_dump(void);
EXPORT_SYMBOL(flow_table_dump);
extern void print_flow(ca_flow_t *flow, bool is_static, bool get_age_from_hw);
EXPORT_SYMBOL(print_flow);
#endif

#endif

