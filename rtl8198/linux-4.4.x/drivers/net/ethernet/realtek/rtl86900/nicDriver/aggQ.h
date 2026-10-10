#ifndef _AGG_Q_H_
#define _AGG_Q_H_

#if defined(CONFIG_RX_WIFI_FF_AGG)
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/spinlock.h>

#include <linux/jhash.h>

#define BR_HASH_BITS 8
#define BR_HASH_SIZE (1 << BR_HASH_BITS)


#define AGGQ_TIMEOUT_TIMER	1
#define AGGQ_5G_MERGE_QUE	0
#define AGGQ_DISABLE_2G_AGG	1
#define AGGQ_ENABLE_DMACPY	1
#define AGGQ_ENABLE_DMA2	1
#define AGGQ_SPECIAL_PKT_HANDLE	0
#define AGGQ_PKT_AGE		0
#define AGGQ_HASH_DSCP		1
#define AGGQ_SKB_LIST		1
#define AGGQ_SET_AGGQ_RULE	1

#if defined(CONFIG_FC_WIFI_TRAP_HASH_SUPPORT)
#define AGGQ_WIFI5G_GMAC_NUM 2
#elif defined(CONFIG_FC_WIFI_TX_GMAC_TRUNKING_SUPPORT)
#define AGGQ_WIFI5G_GMAC_NUM 3
#else
#error "not handle!!"
#endif


#if 1 /* sync with g6 */
#define AGGQ_RTW_UP_MAPPING_RULE 0 //0:tos, 1:dscp
//#define AGGQ_FORCE_QOS_SUPPORT
#define AGGQ_RTW_TXSC_USE_HW_SEQ

//#define AGGQ_RTW_MULTI_AP_R3
//#define AGGQ_CTC_QOS_DSCP
//#define AGGQ_RTW_SSID_PRIORITY
#endif /* sync with g6 */


#define DBG_AGGQ_FORCE_DONT_AGG 0
#if defined(CONFIG_RX_WIFI_FF_AGG_DBG)
#define AGGQ_DBG_CNT 1//struct cp_extra_stats
#else
#define AGGQ_DBG_CNT 0
#endif /* CONFIG_RX_WIFI_FF_AGG_DBG */

#define AGG_ALLOC_SKB(LEN) dev_alloc_skb(LEN)

#define RTK_NIC_WIFI_AGG_DEFAULT_AGE	1

#define RTK_NIC_WIFI_AGG_DEFAULT_NUM	5
#define RTK_NIC_WIFI_AGG_DEFAULT_SIZE	7817
#define RTK_NIC_WIFI_AGG_SKB_RESERVED_SIZE	0
#define RTK_NIC_WIFI_AGGQUEUE_SIZE BR_HASH_SIZE
#define RTK_NIC_WIFI_MAGIC_NUMBER 0x5A
	#define AGGQ_PKT_LIST_MODE 0x5A
	#define AGGQ_PKT_BUFF_MODE 0x6A
	#define AGG_SINGLE_PKT_FLAG (1)
	#if !defined(__DRV_TYPES_H__)
	#define _SKB_CB_PRIORITY	1 /* refer g6_wifi_driver/include/xmit_osdep.h */
	#endif /* !__DRV_TYPES_H__ */
#define RTK_NIC_WIFI_CB_NUMBER	32 /* equal to g6_wifi_driver/include/xmit_osdep.h, _SKB_CB_ETH_AGG */
#define RTK_NIC_GDMA_CHANNEL_NUM	4
#define RTK_SOC_BTG_ENABLE_ADDRESS 0xb8000600
#define RTK_SOC_BTG_ENABLE_ADDRESS2 0xb800063c
#define LX0_BTG_BASED_ADDRESS 0xB8144000
#define LX1_BTG_BASED_ADDRESS 0xB800A000
#define LX2_BTG_BASED_ADDRESS 0xB814C000
#define LX3_BTG_BASED_ADDRESS 0xB8018000


#if !defined(__DRV_TYPES_H__)
enum rtk_nic_gdmaIdx
{
	RTK_NIC_GDMA_LX0=0,
	RTK_NIC_GDMA_LX1=1,
	RTK_NIC_GDMA_LX2=2,
	RTK_NIC_GDMA_LX3=3,
	RTK_NIC_GDMA_MAX,
};

enum rtk_nic_gdmaMask
{
	RTK_NIC_GDMA_LX0_MSK=0x1,
	RTK_NIC_GDMA_LX1_MSK=0x2,
	RTK_NIC_GDMA_LX2_MSK=0x4,
	RTK_NIC_GDMA_LX3_MSK=0x8,
};

#if (AGGQ_TIMEOUT_TIMER==1)
enum txsc_amsdu_timer_type {
	TXSC_AMSDU_TIMER_UNSET,
	TXSC_AMSDU_TIMER_SETTING,
	TXSC_AMSDU_TIMER_TIMEOUT,
};

#define _txsc_spinlock_bh(lock)		spin_lock_irqsave(lock, aggQ_flags)
#define _txsc_spinunlock_bh(lock)	spin_unlock_irqrestore(lock, aggQ_flags)

typedef struct rtw_timer_list {
	struct timer_list timer;
	void (*function)(void *);
	void *arg;
} _timer;

typedef	spinlock_t _lock;

#else /* !AGGQ_TIMEOUT_TIMER */
#define _txsc_spinlock_bh(lock)
#define _txsc_spinunlock_bh(lock)
#endif /* AGGQ_TIMEOUT_TIMER */

typedef struct rtk_nic_wifiAggQueue_s
{
	struct sk_buff *skb;
	unsigned char valid;
	unsigned char num;
	unsigned char num_max;
	unsigned short size;
	unsigned short size_max;
	unsigned long age;
	unsigned int age_max;

	struct re_private *cp;
	struct rx_info rxInfo;

#if (AGGQ_TIMEOUT_TIMER==1)
	_timer txsc_amsdu_be_timer;
	u8 txsc_amsdu_be_timeout_sts;
	_lock	txsc_amsdu_lock;
	u8 ac;
#endif /* AGGQ_TIMEOUT_TIMER */

	unsigned char DA[ETH_ALEN];
}rtk_nic_wifiAggQueue_t;

typedef struct aggQ_rule_s
{
	unsigned char num_max;
	unsigned short size_max;
	int wmm;
	unsigned char DA[ETH_ALEN];
}aggQ_rule_t;

typedef struct rtk_nic_wifiAggGdma_s
{
	rtk_nic_wifiAggQueue_t *pOccupied_queue;
	struct sk_buff *wait_for_free_skb;
	u8 dma_ch;
	unsigned int base_addr;
	rwlock_t gdma_lock;
}rtk_nic_wifiAggGdma_t;
#endif /* __DRV_TYPES_H__ */

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(x) ((u8 *)(x))[0], ((u8 *)(x))[1], ((u8 *)(x))[2], ((u8 *)(x))[3], ((u8 *)(x))[4], ((u8 *)(x))[5]

#define GDMA_INIT_LOCK(_LOCK)				rwlock_init(_LOCK)
#define GDMA_WRITE_LOCK(_LOCK, _FLAGS)		write_lock_irqsave(_LOCK, _FLAGS)
#define GDMA_WRITE_UNLOCK(_LOCK, _FLAGS)	write_unlock_irqrestore(_LOCK, _FLAGS)
#define GDMA_READ_LOCK(_LOCK, _FLAGS)		read_lock_irqsave(_LOCK, _FLAGS)
#define GDMA_READ_UNLOCK(_LOCK, _FLAGS)		read_unlock_irqrestore(_LOCK, _FLAGS)

#if 0//def GDMA_DEBUG
#define DMA_DBG(fmt, args...) printk("GDMA: "fmt"\n", ##args)
#else
#define DMA_DBG(fmt, args...)
#endif
#define DMA_INFO(fmt, args...) printk("GDMA: "fmt"\n", ##args)

#define BSP_IP_EN		0xB8000600UL
  #define BSP_EN_GDMA0	(0x1 << 8)
  #define BSP_EN_GDMA1	(0x1 << 9)
#define BSP_NEW_IP_EN	0xB800063CUL
  #define BSP_EN_GDMA2	(0x1 << 6)
  #define BSP_EN_GDMA3	(0x1 << 7)

#define GDMA_CTRL       0x0
  #define BURST_32		0x000000C0//(0x3 << 6)
  #define GDMA_KICK		(0x1 << 30)
  #define GDMA_EN		(0x1 << 31)
#define GDMA_INT_STS	0x8
  #define DMA_COMPLETE	0x80000000//(0x1 << 31)
#define GDMA_SRC_PTR	0x20
#define GDMA_SRC_LEN	0x24
#define GDMA_DST_PTR	0x60
#define GDMA_DST_LEN	0x64

#define GDMA_LEN_MAX	0x1FFF
#define GDMA_IDX_MAX	0x1FFF

typedef struct rtk_dma_s
{
	u8 dma_ch;
	unsigned int base_addr;
}rtk_dma_t;

//------------------------------------------------------------------------------
enum {
	PRT_NONE,
	PRT_TAB,
	PRT_NEWL,
};

#define agg_print(mode, num, fmt, args...) \
	do { \
		int iiiidx; \
		for (iiiidx=0; iiiidx<num; iiiidx++) { \
			if (mode==1) printk("\t");	\
			else if (mode==2) printk("\n"); \
		}	\
		printk(fmt, ## args); \
	} while (0)

#define EN_AGGQ_TO 0
#if (EN_AGGQ_TO==1)
#define AGGQ_TO(fmt, args...) \
	do { \
		agg_print(PRT_TAB, 0, fmt, ## args); \
	} while(0)
#else
#define AGGQ_TO(fmt, args...) \
	do {} while(0)
#endif

#if 1
#define AGGQ_ERR(fmt, args...) \
	printk("==> [%s:%d], "fmt, __func__, __LINE__, ## args)
#else
#define AGGQ_ERR(fmt, args...)
#endif


#if (AGGQ_DBG_CNT==1)
#define AGGQ_CNT(X) do{X=X+1;}while(0)
#else
#define AGGQ_CNT(X)
#endif /* AGGQ_DBG_CNT */

//------------------------------------------------------------------------------
#if !defined(__DRV_TYPES_H__)
typedef int (*aggqueue_rxHook_t)(struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo);
int rtk_nic_register_rx_func_aggQueue(aggqueue_rxHook_t pfunc);

extern inline struct sk_buff* rtk_nic_wifi_aggQueue_copy(rtk_nic_wifiAggQueue_t *aggQ, struct sk_buff *skb, unsigned char *dst, unsigned char *src, int len);
extern inline int rtk_nic_wifi_aggQueue_finish(rtk_nic_wifiAggQueue_t *aggQ);
extern void aggQ_finish(rtk_nic_wifiAggQueue_t *aggQ);
#endif /* !__DRV_TYPES_H__ */

#if (AGGQ_SET_AGGQ_RULE==1)
extern void set_aggQ_rule(char *mac, unsigned char num_max, unsigned short size_max);
#endif /* AGGQ_SET_AGGQ_RULE */

extern void hwperf_start(int index);
extern void hwperf_stop(int index);

#define HWPERF_START1(x)	hwperf_start(x)
#define HWPERF_STOP1(x)		hwperf_stop(x)

#define CRC_LEN 4
#define MSDU_HDRLEN 8

#if (AGGQ_ENABLE_DMACPY==1)
#undef AGGQ_ENABLE_DMA2
#define AGGQ_ENABLE_DMA2 1
#endif /* AGGQ_ENABLE_DMACPY */
#if (AGGQ_SKB_LIST==1)
#undef AGGQ_ENABLE_DMACPY
#define AGGQ_ENABLE_DMACPY 0
#endif /* AGGQ_SKB_LIST */

#endif /* CONFIG_RX_WIFI_FF_AGG */


//------------------------------------------------------------------------------
#if !defined(CONFIG_RX_WIFI_FF_AGG)
#define AGGQ_CNT(X)
#define aggQ_sw_cnt_read(data)
#define aggQ_sw_cnt_write(data)
#endif /* !CONFIG_RX_WIFI_FF_AGG */

#endif /* _AGG_Q_H_ */

