#if defined(CONFIG_RX_WIFI_FF_AGG)
#include "re8686_rtl9607c.h"

static rtk_nic_wifiAggQueue_t aggQueue[MAX_GMAC_NUM][RTK_NIC_WIFI_AGGQUEUE_SIZE];
static rtk_nic_wifiAggGdma_t aggGdma[RTK_NIC_GDMA_CHANNEL_NUM];
static aggqueue_rxHook_t aggQueueRxFunc=NULL;
static u32 fdb_salt __read_mostly;

#if (AGGQ_SET_AGGQ_RULE==1)
rwlock_t aggR_lock;
static aggQ_rule_t aggRule[MAX_GMAC_NUM][RTK_NIC_WIFI_AGGQUEUE_SIZE];
#endif /* AGGQ_SET_AGGQ_RULE */


//------------------------------------------------------------------------------
#if defined(CONFIG_RX_WIFI_FF_AGG_DBG)
#include "aggQ_dbg.c"
#endif


//------------------------------------------------------------------------------
static inline int comp_macaddr(const u8 *cs, const u8 *ct)
{
	return (*(u16 *)(cs) == *(u16 *)(ct) && *(u16 *)(cs+2) == *(u16 *)(ct+2) && \
		*(u16 *)(cs+4) == *(u16 *)(ct+4));
}

static inline int mac_hash(const unsigned char *mac, __u16 vid)
{
	/* use 1 byte of OUI and 3 bytes of NIC */
	u32 key = get_unaligned((u32 *)(mac + 2));
	return jhash_2words(key, vid, fdb_salt) & (BR_HASH_SIZE - 1);
}

/* refer to IEEE802.11-2016 Table R-3; Comply with IETF RFC4594 */
static inline
u8 tos_to_up(/*_adapter *padapter,*/ u8 tos /*, char skb_cb0*/)
{
	//struct registry_priv *pregistrypriv = &padapter->registrypriv;
	u8 up = 0;
	u8 dscp = (tos >> 2);
	u8 mode = AGGQ_RTW_UP_MAPPING_RULE;
#ifdef AGGQ_RTW_MULTI_AP_R3
	_adapter * primary_adapter;
#endif
#ifdef AGGQ_FORCE_QOS_SUPPORT
	if (padapter->registrypriv.wifi_mib.force_qos
			&& (skb_cb0 > 0) && (skb_cb0 < 8)) {
		up = skb_cb0;
		goto hw_seq_remap;
	}
#endif

	/* tos precedence mapping */
#ifdef AGGQ_CTC_QOS_DSCP
	if (pregistrypriv->ctc_dscp) {
		if ( dscp >= 0 && dscp <= 7 )
			up = 0;
		else if ( dscp >= 8 && dscp <= 15)
			up = 1;
		else if ( dscp >= 16 && dscp <= 23)
			up = 2;
		else if ( dscp >= 24 && dscp <= 31)
			up = 3;
		else if ( dscp >= 32 && dscp <= 39)
			up = 4;
		else if ( dscp >= 40 && dscp <= 47)
			up = 5;
		else if ( dscp >= 48 && dscp <= 55)
			up = 6;
		else if ( dscp >= 56 && dscp <= 63)
			up = 7;

		/* not use HW SEQ */
		goto tos_to_up_end;
	} else
#endif
	if (mode == 0) {
		up = tos >> 5;
	} else {
		/* refer to IEEE802.11-2016 Table R-3;
		 * DCSP 32(CS4) comply with IETF RFC4594
		 */
		if ( dscp == 0 )
			up = 0;
		else if ( dscp >= 1 && dscp <= 9)
			up = 1;
		else if ( dscp >= 10 && dscp <= 16)
			up = 2;
		else if ( dscp >= 17 && dscp <= 23)
			up = 3;
		else if ( dscp >= 24 && dscp <= 31)
			up = 4;
		else if ( dscp >= 33 && dscp <= 40)
			up = 5;
		else if ((dscp >= 41 && dscp <= 47) || (dscp == 32))
			up = 6;
		else if ( dscp >= 48 && dscp <= 63)
			up = 7;
	}

#ifdef AGGQ_RTW_SSID_PRIORITY
	if(pregistrypriv->wifi_mib.manual_priority != 0xff)
		up = pregistrypriv->wifi_mib.manual_priority;
#endif

#ifdef AGGQ_FORCE_QOS_SUPPORT
hw_seq_remap:
#endif
#ifdef AGGQ_RTW_TXSC_USE_HW_SEQ
	// Freddie ToDo: HW SEQ EN policy. Temporarily map TID to 4 Q and use QSEL as HWSEQ index.
	do {
		static const u32 tid_remap[8] = {0, 1, 1, 0, 4, 4, 6, 6};
		up = tid_remap[up];
	} while (0);
#endif /* CONFIG_RTW_TXSC_USE_HW_SEQ */

#ifdef AGGQ_RTW_MULTI_AP_R3 // if service prioritization is needed and out_value is 8, just set up based on dscp_pcp_table.
	primary_adapter = dvobj_get_primary_adapter(padapter->dvobj);
	if (primary_adapter->service_priority_enabled == 1 && primary_adapter->service_priority_output == 8 && primary_adapter->dscp_pcp_table_enabled == 1) {
		up = *((primary_adapter->dscp_pcp_table) + dscp);
	}
#endif

#ifdef AGGQ_CTC_QOS_DSCP
tos_to_up_end:
#endif
	return up;
}


//------------------------------------------------------------------------------
#if (AGGQ_TIMEOUT_TIMER==1)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
static inline
void timer_hdl(struct timer_list *in_timer)
{
	_timer *ptimer = from_timer(ptimer, in_timer, timer);

	ptimer->function(ptimer->arg);
}
#else
static inline
void timer_hdl(unsigned long cntx)
{
	_timer *ptimer = (_timer *)cntx;

	ptimer->function(ptimer->arg);
}
#endif

static inline
void _init_timer(_timer *ptimer, void *pfunc, void *cntx)
{
	ptimer->function = pfunc;
	ptimer->arg = cntx;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	timer_setup(&ptimer->timer, timer_hdl, 0);
#else
	/* setup_timer(ptimer, pfunc,(u32)cntx);	 */
	ptimer->timer.function = timer_hdl;
	ptimer->timer.data = (unsigned long)ptimer;
	init_timer(&ptimer->timer);
#endif
}

static inline
void _set_timer(_timer *ptimer, u32 delay_time)
{
	mod_timer(&ptimer->timer , (jiffies + (delay_time * HZ / 1000)));
}

static inline
void _cancel_timer(_timer *ptimer, u8 *bcancelled)
{
	*bcancelled = del_timer_sync(&ptimer->timer) == 1 ? 1 : 0;
}

static inline
void _cancel_timer_async(_timer *ptimer)
{
	del_timer(&ptimer->timer);
}

void rtw_init_timer(_timer *ptimer, void *pfunc, void *ctx)
{
	_init_timer(ptimer, pfunc, ctx);
}

static inline
void txsc_amsdu_set_timer(rtk_nic_wifiAggQueue_t *psta, u8 ac)
{
#if 0
	_timer *txsc_amsdu_timer = NULL;

	switch (ac) {
	case 0:
		txsc_amsdu_timer = &psta->txsc_amsdu_vo_timer;
		break;

	case 1:
		txsc_amsdu_timer = &psta->txsc_amsdu_vi_timer;
		break;

	case 2:
		txsc_amsdu_timer = &psta->txsc_amsdu_be_timer;
		break;

	case 3:
		txsc_amsdu_timer = &psta->txsc_amsdu_bk_timer;
		break;

	default:
		txsc_amsdu_timer = &psta->txsc_amsdu_be_timer;
		break;
	}

	_set_timer(txsc_amsdu_timer, RTK_NIC_WIFI_AGG_DEFAULT_AGE);
#else
	_set_timer(&psta->txsc_amsdu_be_timer, RTK_NIC_WIFI_AGG_DEFAULT_AGE);
#endif
}

static inline
u8 txsc_amsdu_get_timer_status(rtk_nic_wifiAggQueue_t *psta, u8 ac)
{
#if 0
	u8 status = TXSC_AMSDU_TIMER_UNSET;

	switch (ac) {
	case 0:
		status = psta->txsc_amsdu_vo_timeout_sts;
		break;

	case 1:
		status = psta->txsc_amsdu_vi_timeout_sts;
		break;

	case 2:
		status = psta->txsc_amsdu_be_timeout_sts;
		break;

	case 3:
		status = psta->txsc_amsdu_bk_timeout_sts;
		break;

	default:
		status = psta->txsc_amsdu_be_timeout_sts;
		break;
	}

	return status;
#else
	return psta->txsc_amsdu_be_timeout_sts;
#endif
}

static inline
void txsc_amsdu_set_timer_status(rtk_nic_wifiAggQueue_t *psta, u8 ac, u8 status)
{
#if 0
	switch (ac) {
	case 0:
		psta->txsc_amsdu_vo_timeout_sts = status;
		break;

	case 1:
		psta->txsc_amsdu_vi_timeout_sts = status;
		break;

	case 2:
		psta->txsc_amsdu_be_timeout_sts = status;
		break;

	case 3:
		psta->txsc_amsdu_bk_timeout_sts = status;
		break;

	default:
		psta->txsc_amsdu_be_timeout_sts = status;
		break;
	}
#else
	psta->txsc_amsdu_be_timeout_sts = status;
#endif
}

/*static inline*/
s32 txsc_amsdu_timeout_tx(rtk_nic_wifiAggQueue_t *aggQ, u8 ac)
{
	txsc_amsdu_set_timer_status(aggQ, ac, TXSC_AMSDU_TIMER_TIMEOUT);
	aggQ_finish(aggQ);
	AGGQ_CNT(aggQ->cp->cp_stats.timeout);
	return 0;
}

static void txsc_amsdu_be_timeout_handler(void *func_ontext)
{
#if 0
	struct sta_info *psta = (struct sta_info *)func_ontext;
	u8 ac = 2;

	//psta->txsc_amsdu_be_timeout_sts = TXSC_AMSDU_TIMER_TIMEOUT;
	#ifdef CONFIG_WFA_OFDMA_Logo_Test_Statistic
	psta->core_txsc_amsdu_timeout++;
	#endif

	/* do amsdu tx */
	txsc_amsdu_timeout_tx(psta, ac);
#else
	rtk_nic_wifiAggQueue_t *aggQ = (rtk_nic_wifiAggQueue_t *)func_ontext;
	unsigned long aggQ_flags;

	_txsc_spinlock_bh(&aggQ->txsc_amsdu_lock);
	if (aggQ && aggQ->num) {
		/* do amsdu tx */
		txsc_amsdu_timeout_tx(aggQ, aggQ->ac);
	}
	_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
#endif
}
#endif /* AGGQ_TIMEOUT_TIMER */


//------------------------------------------------------------------------------
#if (AGGQ_SKB_LIST==0)
static rtk_dma_t rtk_dma[RTK_NIC_GDMA_CHANNEL_NUM] = {
	{2, LX2_BTG_BASED_ADDRESS},
	{3, LX3_BTG_BASED_ADDRESS},

	{0, LX0_BTG_BASED_ADDRESS},
	{1, LX1_BTG_BASED_ADDRESS},
};

static inline
void rtk_dma_reset(int idx)
{
	REG32(aggGdma[idx].base_addr + GDMA_INT_STS) = DMA_COMPLETE;
	REG32(aggGdma[idx].base_addr + GDMA_CTRL) = BURST_32;
	REG32(aggGdma[idx].base_addr + GDMA_INT_STS) = 0;
	REG32(aggGdma[idx].base_addr + GDMA_CTRL) = 0X800000C0;//GDMA_EN | BURST_32;
}

static inline
void rtk_dma_poll(int idx)
{
	while (!(REG32(aggGdma[idx].base_addr + GDMA_INT_STS) & DMA_COMPLETE)) {
		DMA_DBG("Waiting For DMA, GDMA interrupt status = %p",
				REG32(aggGdma[idx].base_addr + GDMA_INT_STS));
	}

	DMA_DBG("GDMA interrupt status = %p\n", REG32(aggGdma[idx].base_addr + GDMA_INT_STS));
}

static inline
void rtk_dma_cpy(void *src, void *dst, int len, int idx)
{
	REG32(aggGdma[idx].base_addr + GDMA_SRC_PTR) = ((unsigned int)src)|0XA0000000;
	REG32(aggGdma[idx].base_addr + GDMA_SRC_LEN) = len;
	REG32(aggGdma[idx].base_addr + GDMA_DST_PTR) = ((unsigned int)dst)|0XA0000000;
	REG32(aggGdma[idx].base_addr + GDMA_DST_LEN) = len;

	REG32(aggGdma[idx].base_addr + GDMA_CTRL) = 0XC00000C0;//GDMA_KICK | GDMA_EN | BURST_32;
}

static void rtk_gmac_gdma_init(int idx, u8 dma_ch, unsigned int addr)
{
	memset(&aggGdma[idx],0,sizeof(rtk_nic_wifiAggGdma_t));
	GDMA_INIT_LOCK(&aggGdma[idx].gdma_lock);
	aggGdma[idx].wait_for_free_skb=re8670_getAlloc(SKB_BUF_SIZE);
	skb_reserve(aggGdma[idx].wait_for_free_skb, RX_OFFSET);

	aggGdma[idx].dma_ch = dma_ch;
	aggGdma[idx].base_addr = addr;

	REG32(addr+0x4)=0x80000000;	//enable interrupt
	REG32(addr+0x8)=0x80000000;	//clear interrput
	REG32(addr+0x8)=0x00000000;	//clear interrput

	REG32(addr+0x20)=0;
	REG32(addr+0x24)=0;
	REG32(addr+0x60)=0;
	REG32(addr+0x64)=0;
	REG32(addr)=0xc00000c0;	//kick-off
}

inline
struct sk_buff* rtk_nic_wifi_aggQueue_copy(
	rtk_nic_wifiAggQueue_t *aggQ, struct sk_buff *skb,
	unsigned char *dst, unsigned char *src, int len)
{
	struct sk_buff *new_skb = NULL;

#if (AGGQ_ENABLE_DMACPY==1)
	unsigned long flags;
	int idx = 0;

	for (idx=0; idx<RTK_NIC_GDMA_CHANNEL_NUM; idx++) {
		rtk_nic_wifiAggGdma_t *gdma = &aggGdma[idx];
		if (gdma->base_addr == 0)
			continue;

		GDMA_WRITE_LOCK(&gdma->gdma_lock, flags);
		if(REG32(gdma->base_addr+0x8) & 0x80000000){
			gdma->pOccupied_queue = aggQ;
			new_skb = gdma->wait_for_free_skb;
			gdma->wait_for_free_skb = skb;

			rtk_dma_reset(idx);
			rtk_dma_cpy(src, dst, len, idx);
			aggQ->cp->cp_stats.gdma_cnt[gdma->dma_ch]++;

			GDMA_WRITE_UNLOCK(&gdma->gdma_lock, flags);
			goto BEFORE_RETURN;
		}
		GDMA_WRITE_UNLOCK(&gdma->gdma_lock, flags);
	}
	aggQ->cp->cp_stats.gdma_busy_cnt++;

	//all GDMA had been used or no mem for new skb, just memcpy and reuse the skb
	memcpy(dst,src,len);
	new_skb=skb;

BEFORE_RETURN:
#else /* !AGGQ_ENABLE_DMACPY */
	memcpy(dst,src,len);
	new_skb=skb;
#endif /* AGGQ_ENABLE_DMACPY */

#if (AGGQ_TIMEOUT_TIMER==1)
	{
		u8 timer_sts;
		timer_sts = txsc_amsdu_get_timer_status(aggQ, aggQ->ac);
		if (aggQ->num >= 0 && timer_sts != TXSC_AMSDU_TIMER_SETTING) {
			txsc_amsdu_set_timer(aggQ, aggQ->ac);
			txsc_amsdu_set_timer_status(aggQ, aggQ->ac, TXSC_AMSDU_TIMER_SETTING);
		}
	}
#endif /* AGGQ_TIMEOUT_TIMER */

	return new_skb;
}

/*inline*/
int rtk_nic_wifi_aggQueue_finish(rtk_nic_wifiAggQueue_t *aggQ)
{
#if (AGGQ_ENABLE_DMACPY==1)
	unsigned long flags;
	int idx = 0;

	for (idx=0; idx<RTK_NIC_GDMA_CHANNEL_NUM; idx++) {
		rtk_nic_wifiAggGdma_t *gdma = &aggGdma[idx];
		GDMA_READ_LOCK(&gdma->gdma_lock, flags);
		if (gdma->pOccupied_queue == aggQ) {
			rtk_dma_poll(idx);
			gdma->pOccupied_queue = NULL;
		}
		GDMA_READ_UNLOCK(&gdma->gdma_lock, flags);
	}
#endif /* AGGQ_ENABLE_DMACPY */

	return 1;
}
#endif /* AGGQ_SKB_LIST */

#if (AGGQ_SPECIAL_PKT_HANDLE==1)
inline
int direct_xmit_pkt(struct sk_buff *skb)
{
	struct iphdr *iph = skb->data+ETH_HLEN;
	struct tcphdr *tcph = (struct tcphdr *) ((void *) iph + iph->ihl*4);

	if (tcph->psh || tcph->fin || tcph->rst || tcph->urg)
		return 1;

	return 0;
}
#endif /* AGGQ_SPECIAL_PKT_HANDLE */

inline
struct sk_buff *aggQ_copy(rtk_nic_wifiAggQueue_t *aggQ, struct sk_buff *skb, int len, int padding)
{
	unsigned char *pData=skb->data;
	struct sk_buff *new_skb = NULL;

#if (AGGQ_SKB_LIST==1)
	struct sk_buff *skb_end = aggQ->skb;

	new_skb = AGG_ALLOC_SKB(SKB_BUF_SIZE);
	if (new_skb) {
		skb_reserve(new_skb, RX_OFFSET);
		aggQ->cp->cp_stats.agg_skb_alloc += 1;
	} else {
		AGGQ_ERR("out of mem..\n");
		return NULL;
	}

	if (skb_tailroom(skb)<len) {
		AGGQ_ERR("no tailroom!!\n");
		return NULL;
	}
	if (skb_headroom(skb)<MSDU_HDRLEN) {
		AGGQ_ERR("no headroom!!\n");
		return NULL;
	}
	skb_put(skb, len);
	skb_push(skb, MSDU_HDRLEN);
	pData = skb->data;
#else /* !AGGQ_SKB_LIST */
	pData -= MSDU_HDRLEN;
#endif /* AGGQ_SKB_LIST */
	memmove(pData, pData+MSDU_HDRLEN, ETH_ALEN<<1);
	*(uint16 *)(pData + 12) = htons(len+MSDU_HDRLEN-ETH_HLEN);	//snap length
	*(uint32 *)(pData + 14) = htonl(0xaaaa0300);				//llc_1
	*(uint16 *)(pData + 18) = htons(0x0000);					//llc_2
#if (AGGQ_SKB_LIST==0)
	dma_cache_wback_inv((unsigned long)pData, 20);
#endif /* !AGGQ_SKB_LIST */

#if (AGGQ_SKB_LIST==1)
	skb->cb[RTK_NIC_WIFI_CB_NUMBER] = AGGQ_PKT_LIST_MODE; //magic
	while (skb_end->next) {
		skb_end = skb_end->next;
	}
	if (padding) {
		if (skb_tailroom(skb_end)<padding) {
			AGGQ_ERR("no tailroom!!\n");
			return NULL;
		}
		skb_put(skb_end, padding);
	}
	if (aggQ->num > 0) {
		skb_end->next = skb;
	}
#else /* !AGGQ_SKB_LIST */
	new_skb = rtk_nic_wifi_aggQueue_copy(
		aggQ, skb,
		aggQ->skb->data+aggQ->size+padding, pData, len+MSDU_HDRLEN);
#endif /* AGGQ_SKB_LIST */

	aggQ->size += len+MSDU_HDRLEN+padding;
	aggQ->num  += 1;
#if (AGGQ_PKT_AGE==1)
	aggQ->age = jiffies + aggQ->age_max;
#endif /* AGGQ_PKT_AGE */

	return new_skb;
}

inline
void aggQ_finish(rtk_nic_wifiAggQueue_t *aggQ)
{
#if (AGGQ_SKB_LIST==1)
	int ret = 1;
#else /* !AGGQ_SKB_LIST */
	int ret = rtk_nic_wifi_aggQueue_finish(aggQ);
#endif /* AGGQ_SKB_LIST */

	if (ret) {
	#if (AGGQ_SKB_LIST==1)
		aggQ->skb->cb[RTK_NIC_WIFI_CB_NUMBER] = AGGQ_PKT_LIST_MODE; //magic
	#else /* !AGGQ_SKB_LIST */
		aggQ->skb->len = aggQ->size;
		aggQ->skb->cb[RTK_NIC_WIFI_CB_NUMBER] = AGGQ_PKT_BUFF_MODE; //magic
	#endif /* AGGQ_SKB_LIST */

		if (aggQ->num==1) {
			aggQ->skb->cb[RTK_NIC_WIFI_CB_NUMBER] |= AGG_SINGLE_PKT_FLAG; //magic

			/* convert to 802.3 */
			memcpy(aggQ->skb->data+ETH_HLEN, aggQ->skb->data+ETH_ALEN, ETH_ALEN);
			memcpy(aggQ->skb->data+MSDU_HDRLEN, aggQ->skb->data, ETH_ALEN);
			skb_pull(aggQ->skb, MSDU_HDRLEN);
		#if (AGGQ_SKB_LIST==0)
			dma_cache_wback_inv(aggQ->skb->data, ETH_ALEN<<1);
		#endif /* !AGGQ_SKB_LIST */
			AGGQ_CNT(aggQ->cp->cp_stats.finish_one);
		} else {
			AGGQ_CNT(aggQ->cp->cp_stats.finish_multi);
		}
		AGGQ_CNT(aggQ->cp->cp_stats.finish);
		aggQueueRxFunc(aggQ->cp, aggQ->skb, &aggQ->rxInfo);
	#if (AGGQ_SKB_LIST==1)
		aggQ->skb = NULL;
	#else /* !AGGQ_SKB_LIST */
		aggQ->skb=AGG_ALLOC_SKB(aggQ->size_max);
	#endif /* AGGQ_SKB_LIST */
	}

	aggQ->num = aggQ->size = 0;
	aggQ->valid = aggQ->skb?1:0;
#if (AGGQ_PKT_AGE==1)
	aggQ->age = jiffies + aggQ->age_max;
#endif /* AGGQ_PKT_AGE */
}

#if 0
inline
rtk_nic_wifiAggQueue_t *aggQ_search(struct sk_buff *skb, unsigned int gmac)
{
	rtk_nic_wifiAggQueue_t *aggQ = NULL;
	int hash;
	u16 wmm_val = 0;

#if (AGGQ_HASH_DSCP==1)
HWPERF_START1(2);
	wmm_val = wwm_value(skb->data);
HWPERF_STOP1(2);
#endif /* AGGQ_HASH_DSCP */

	hash = mac_hash(skb->data, wmm_val);
	aggQ = &aggQueue[gmac][hash];

	return aggQ;
}
#endif

#if 0
inline
u32 aggQ_process(struct re_private *cp,
		struct rx_info *rxInfo, int ring_num, unsigned rx_Mtail,
		struct sk_buff *skb, u32 len, u32 *pkt_age)
{
	//aggQueue mechanism
	if(rxInfo->opts2.bit.reason==64 &&
		hweight16(rxInfo->opts3.bit.fb_hash_or_dst_portmsk)==1 &&
		aggQueueRxFunc){
		unsigned int gmac = cp->gmac;
		int j,first_invalid=-1,padding=0;
		unsigned char *pData=skb->data;
		unsigned short ethType=ntohs(*(unsigned short *)(pData+(ETH_ALEN<<1)));
		rtk_nic_wifiAggQueue_t *aggQ=NULL;
		unsigned long aggQ_flags;
		struct sk_buff *new_skb=NULL;
		int hash;
		int pktlen;
		u16 wmm_val = 0;

	#if (DBG_AGGQ_FORCE_DONT_AGG==1)
		goto OUT_AGG_QUEUE;
	#endif

	#if (AGGQ_DISABLE_2G_AGG==1)
		/* don't AGG for 2G wifi */
		if (gmac==2)
			goto OUT_AGG_QUEUE;
	#endif /* AGGQ_DISABLE_2G_AGG */

	#if (AGGQ_PKT_AGE==1)
		if (*pkt_age) {
			for (j=0;j<RTK_NIC_WIFI_AGG_DEFAULT_AGE;j++) {
				aggQ = &aggQueue[gmac][j];
				_txsc_spinlock_bh(&aggQ->txsc_amsdu_lock);
				if (aggQ->valid && aggQ->num && aggQ->skb) {
					if (time_is_before_eq_jiffies(aggQ->age)) {
						AGGQ_CNT(cp->cp_stats.pkt_age);
						aggQ_finish(aggQ);
					#if (AGGQ_SKB_LIST==0)
						if (aggQ->skb == NULL) {
							AGGQ_ERR("out of mem..\n");
							_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
							goto OUT_AGG_QUEUE;
						}
					#endif /* AGGQ_SKB_LIST */
					}
				}
				_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
			}

			aggQ = NULL;
			*pkt_age = 0;
		}
	#endif /* AGGQ_PKT_AGE */

		if(ethType!=0x0800 && ethType!=0x86dd){
			//printk("the ethType is 0x%x...go out!!\n",ethType);
			goto OUT_AGG_QUEUE;
		}

	#if (AGGQ_5G_MERGE_QUE==1)
		if (gmac!=2)
			gmac=0;
	#endif /* AGGQ_5G_MERGE_QUE */


	#if (AGGQ_HASH_DSCP==1)
		wmm_val = wwm_value(skb->data);
	#endif /* AGGQ_HASH_DSCP */
		hash = mac_hash(skb->data, wmm_val);
		aggQ = &aggQueue[gmac][hash];
		pktlen = len - CRC_LEN;

		_txsc_spinlock_bh(&aggQ->txsc_amsdu_lock);
		if (aggQ->valid) {
			/* same da */
			if (comp_macaddr(skb->data, aggQ->DA)
			#if (AGGQ_HASH_DSCP==1)
				&& aggQ->skb-cb[_SKB_CB_PRIORITY] == wmm_val
			#endif /* AGGQ_HASH_DSCP */
				) {
				int direct_xmit = 0;
				padding = 4 - aggQ->size & 3;

			#if (AGGQ_SPECIAL_PKT_HANDLE==1)
				/* special packet handle */
				direct_xmit = direct_xmit_pkt(skb);
				if (direct_xmit) {
					AGGQ_CNT(aggQ->cp->cp_stats.direct_xmit);
				}
			#endif /* AGGQ_SPECIAL_PKT_HANDLE */

				/* len reach */
				if (aggQ->size+padding+pktlen+MSDU_HDRLEN >= aggQ->size_max) {
					aggQ_finish(aggQ);
					#if (AGGQ_SKB_LIST==1)
					aggQ->valid = 1;
					aggQ->skb = skb;
					#else /* !AGGQ_SKB_LIST */
					if (aggQ->skb == NULL) {
						AGGQ_ERR("out of mem..\n");
						_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
						goto OUT_AGG_QUEUE;
					}
					#endif /* AGGQ_SKB_LIST */
				}

				new_skb = aggQ_copy(aggQ, skb, pktlen, padding);

				/* cnt reach */
				if (aggQ->num >= aggQ->num_max || direct_xmit) {
					aggQ_finish(aggQ);
				}
			#if (AGGQ_SKB_LIST==1)
				if (unlikely(!new_skb)) {
					_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
					goto AGGQ_NOMEM;
				}
			#endif /* AGGQ_SKB_LIST */
			}
		} else {
		#if (AGGQ_HASH_DSCP==1)
			skb->cb[_SKB_CB_PRIORITY] = wmm_val;
		#endif /* AGGQ_HASH_DSCP */
		#if (AGGQ_SKB_LIST==1)
			aggQ->skb=skb;

			aggQ->valid=1;
			aggQ->ac=0;
			aggQ->cp=cp;
			memcpy(aggQ->DA,pData,ETH_ALEN);
			memcpy(&aggQ->rxInfo, &rxInfo, sizeof(struct rx_info));

			new_skb = aggQ_copy(aggQ, skb, pktlen, padding);

			if (unlikely(!new_skb)) {
				_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
				goto AGGQ_NOMEM;
			}
		#else /* !AGGQ_SKB_LIST */
			aggQ->skb=AGG_ALLOC_SKB(aggQ->size_max);
			if(aggQ->skb){
				dma_cache_inv((unsigned long)aggQ->skb->data,(u32)aggQ->skb->end-(u32)aggQ->skb->data);
				aggQ->valid=1;
				aggQ->ac=0;
				aggQ->cp=cp;
				memcpy(aggQ->DA,pData,ETH_ALEN);
				memcpy(&aggQ->rxInfo, &rxInfo, sizeof(struct rx_info));

				new_skb = aggQ_copy(aggQ, skb, pktlen, padding);
			}else{
				AGGQ_ERR("out of mem..\n");
				_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
				goto OUT_AGG_QUEUE;
			}
		#endif /* AGGQ_SKB_LIST */
		}
		_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);

		if(likely(new_skb!=skb)){
			//printk("refill new skb here!!\n");
			skb_reserve(new_skb, -RX_OFFSET); // HW DMA start at 4N+2 only in FS.
			cp->rx_Mring[ring_num][rx_Mtail].addr = CPHYSADDR(new_skb->data);
			cp->rx_skb[ring_num][rx_Mtail].skb = new_skb;
			dma_cache_inv((unsigned long)new_skb->data,(u32)new_skb->end-(u32)new_skb->data);
		}else{
			//printk("refill original skb.\n");
			skb_reserve(new_skb, -RX_OFFSET); // HW DMA start at 4N+2 only in FS.
			dma_cache_inv((unsigned long)new_skb->data,(u32)(new_skb->end-new_skb->data));
		}
		goto rx_next;
	}

rx_next:
	return 1;

OUT_AGG_QUEUE:
	//new_skb = NULL;
#if (AGGQ_5G_MERGE_QUE==1)
	//gmac = cp->gmac;
#endif /* AGGQ_5G_MERGE_QUE */
	return 2;

AGGQ_NOMEM:
	return 3;
}
#endif

//------------------------------------------------------------------------------
int rtk_nic_register_rx_func_aggQueue(aggqueue_rxHook_t pfunc)
{
	if(!pfunc)
		return -EINVAL;
	aggQueueRxFunc=pfunc;
	return 0;
}

static inline
void aggQ_sw_cnt_read(struct re_private *data)
{
	printk("gdma_lx0 count :%14d        gdma_lx1 count :%14d\n",
		data->cp_stats.gdma_cnt[0],data->cp_stats.gdma_cnt[1]);
	printk("gdma_lx2 count :%14d		gdma_lx3 count :%14d\n",
		data->cp_stats.gdma_cnt[2],data->cp_stats.gdma_cnt[3]);
	printk("gdma_busy count :%14d\n",data->cp_stats.gdma_busy_cnt);

	printk("agg_skb_alloc :%14d\n",data->cp_stats.agg_skb_alloc);
}

static inline
void aggQ_sw_cnt_write(struct re_private *data)
{
	data->cp_stats.gdma_cnt[0]=0;
	data->cp_stats.gdma_cnt[1]=0;
	data->cp_stats.gdma_cnt[2]=0;
	data->cp_stats.gdma_cnt[3]=0;
	data->cp_stats.gdma_busy_cnt=0;
}

//------------------------------------------------------------------------------
#if (AGGQ_SET_AGGQ_RULE==1)
void set_aggQ_rule(char *mac, unsigned char num_max, unsigned short size_max)
{
	unsigned long flags;
	aggQ_rule_t *aggR = NULL;
	int gmac = 0;
#ifdef AGGQ_RTW_TXSC_USE_HW_SEQ
#define WMM_NUM 4
#else
#define WMM_NUM 8
#endif
	int _hash = 0, _update = 0;

#if (AGGQ_HASH_DSCP==1)
	int wmm = 0;
	#ifdef AGGQ_RTW_TXSC_USE_HW_SEQ
	static const u32 tid_remap[WMM_NUM] = {0, 1, 4, 6};
	#else
	static const u32 tid_remap[WMM_NUM] = {0, 1, 2, 3, 4, 5, 6, 7};
	#endif
#endif /* AGGQ_HASH_DSCP */

#if (AGGQ_5G_MERGE_QUE==0)
	for (gmac=0; gmac<AGGQ_WIFI5G_GMAC_NUM; gmac++)
#endif
	{
		#if (AGGQ_HASH_DSCP==1)
		for (wmm=0; wmm<WMM_NUM; wmm++)
		#endif /* AGGQ_HASH_DSCP */
		{
		#if (AGGQ_HASH_DSCP==1)
			_hash = mac_hash(mac, tid_remap[wmm]);
		#else /* !AGGQ_HASH_DSCP */
			_hash = mac_hash(mac, 0);
		#endif /* AGGQ_HASH_DSCP */

			aggR = &aggRule[gmac][_hash];
			read_lock_irqsave(&aggR_lock, flags);
			if ((aggR->num_max != num_max) || (aggR->size_max != size_max)) {
				_update = 1;
			} else {
				_update = 0;
			}
			read_unlock_irqrestore(&aggR_lock, flags);

			if (_update) {
				write_lock_irqsave(&aggR_lock, flags);
				memcpy(aggR->DA, mac, ETH_ALEN);
			#if (AGGQ_HASH_DSCP==1)
				aggR->wmm = tid_remap[wmm];
			#endif /* AGGQ_HASH_DSCP */
				aggR->num_max = num_max;
				aggR->size_max = size_max;
				write_unlock_irqrestore(&aggR_lock, flags);
			}
		}
	}
}
EXPORT_SYMBOL(set_aggQ_rule);
#endif /* AGGQ_SET_AGGQ_RULE */

void aggQ_init(void)
{
	int i, j;

#if (AGGQ_SET_AGGQ_RULE==1)
	rwlock_init(&aggR_lock);
	memset(&aggRule[0][0],0,sizeof(aggQ_rule_t)*RTK_NIC_WIFI_AGGQUEUE_SIZE*MAX_GMAC_NUM);
#endif /* AGGQ_SET_AGGQ_RULE */

	memset(&aggQueue[0][0],0,sizeof(rtk_nic_wifiAggQueue_t)*RTK_NIC_WIFI_AGGQUEUE_SIZE*MAX_GMAC_NUM);

	for (i=0; i<MAX_GMAC_NUM; i++) {
		for (j=0;j<RTK_NIC_WIFI_AGGQUEUE_SIZE;j++) {
			rtk_nic_wifiAggQueue_t *aggQ = &aggQueue[i][j];
		#if (AGGQ_SET_AGGQ_RULE==1)
			aggQ_rule_t *aggR = &aggRule[i][j];
		#endif /* AGGQ_SET_AGGQ_RULE */

			aggQ->num_max=RTK_NIC_WIFI_AGG_DEFAULT_NUM;
			aggQ->size_max=RTK_NIC_WIFI_AGG_DEFAULT_SIZE-RTK_NIC_WIFI_AGG_SKB_RESERVED_SIZE;	//preserved header room
		#if (AGGQ_PKT_AGE==1)
			aggQ->age_max=RTK_NIC_WIFI_AGG_DEFAULT_AGE;
		#endif /* AGGQ_PKT_AGE */
		#if (AGGQ_TIMEOUT_TIMER==1)
			spin_lock_init(&aggQ->txsc_amsdu_lock);
			rtw_init_timer(&(aggQ->txsc_amsdu_be_timer), txsc_amsdu_be_timeout_handler, aggQ);
			aggQ->txsc_amsdu_be_timeout_sts = TXSC_AMSDU_TIMER_UNSET;
			aggQ->ac = 0;
		#endif /* AGGQ_TIMEOUT_TIMER */
		#if (AGGQ_SET_AGGQ_RULE==1)
			aggR->num_max=aggQ->num_max;
			aggR->size_max=aggQ->size_max;
		#endif /* AGGQ_SET_AGGQ_RULE */
		}
	}

#if (AGGQ_ENABLE_DMACPY==1)
	//GDMA init
	value=REG32(RTK_SOC_BTG_ENABLE_ADDRESS);
	value|=0x300;
	REG32(RTK_SOC_BTG_ENABLE_ADDRESS)=value;
	value=REG32(RTK_SOC_BTG_ENABLE_ADDRESS2);
#if (AGGQ_ENABLE_DMA2==1)
	value|=0xc0;
#else /* !AGGQ_ENABLE_DMA2 */
	value|=0xb0;//0xc0; 	//lx2 would not be used
#endif /* AGGQ_ENABLE_DMA2 */
	REG32(RTK_SOC_BTG_ENABLE_ADDRESS2)=value;

	for (i=0; i<RTK_NIC_GDMA_CHANNEL_NUM; i++) {
#if (AGGQ_ENABLE_DMA2==0)
		if (rtk_dma[i].dma_ch==2)
			continue;
#endif
		rtk_gmac_gdma_init(i, rtk_dma[i].dma_ch, rtk_dma[i].base_addr);
	}
#endif /* AGGQ_ENABLE_DMACPY */
	get_random_bytes(&fdb_salt, sizeof(fdb_salt));

#if defined(CONFIG_RX_WIFI_FF_AGG_DBG)
	aggQ_dbg_init();
#endif /* CONFIG_RX_WIFI_FF_AGG_DBG */
}

#endif /* CONFIG_RX_WIFI_FF_AGG */
