/* re8670.c: A Linux Ethernet driver for the RealTek 8670 chips. */

#define DRV_NAME		"8686"
#define DRV_VERSION		"0.0.1"
#define DRV_RELDATE		"Aug 17, 2016"
#define DRV_DUALBAND	"Dual Band Disable"


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
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <net/xfrm.h>
#include <linux/proc_fs.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/etherdevice.h>

#ifdef CONFIG_RTK_WFOAX
#define WFO_GMAC_NO 2
#endif
/* to be removed */
#define BSP_EN_GMAC  (1<<1)
#define BSP_EN_GMAC2 (1<<20)
#define BSP_EN_GMAC1 (1<<19)
#define WRITE_MEM32(addr, val)   (*(volatile unsigned int *)   (addr)) = (val)
#define READ_MEM32(addr)         (*(volatile unsigned int *)   (addr))

#include <bspchip.h>


#ifdef CONFIG_RTL865X_ETH_PRIV_SKB
#include "re_privskb.h"
#include "re_privskb_rg.h"
#endif
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
#include "re_privskb_adv.h"
#include "brg_shortcut.h"
#endif

#include "../../ethctl_implement.h"
#include "re8686_rtl9607c.h"

#undef __IRAM_NIC
#define __IRAM_NIC __attribute__ ((__section__(".iram-fwd")))

#ifdef CONFIG_RTK_HOST_SPEEDUP
#include "../../net/ipv4/ip_speedup.h"
#include <net/tcp.h>
#endif

#define SMBSHORTCUT
#ifdef SMBSHORTCUT
unsigned int smbserverip = 0xc0a80101;
unsigned char smbscflag = 0x0;
#endif

#ifdef CONFIG_RTL8686_SWITCH
//#include "../sdk/include/dal/apollo/raw/apollo_raw_port.h"
#include <rtk/l2.h>
#include <dal/apollomp/dal_apollomp_l2.h>
#include <rtk/switch.h>
#include <rtk/acl.h>
#include <rtk/trap.h>
#include <rtk/gponv2.h> 
#include <common/type.h>
#include <rtk/intr.h>
#include <module/intr_bcaster/intr_bcaster.h>
#endif
#if defined(CONFIG_RTK_SOC_RTL8198D)
#include <rtk/rate.h>
#include <rtk/stat.h>
#include <rtk/port.h>
#include "hal/common/halctrl.h"
#endif

#ifdef CONFIG_RG_DEBUG
#include <rtk_rg_profile.h>
#else
#include "../../../arch/mips/rtl8198d/bspchip.h"

#define MIPS_PERF_NIC_RX 0
inline void mips_perf_measure_entrance(int index)
{
}

inline void mips_perf_measure_exit(int index)
{
}
#endif

void re8686_direct_tx_to_cpu_port(struct tx_info *tx_info_ptr);
static inline void kick_tx(unsigned int gmac, int ring_num);
#if defined(CONFIG_RTW_MEMPOOL)
extern int rtw_remove_mem_pool_ex(char *name);
extern struct sk_buff *rtw_mem_pool_alloc_skb(int size);
extern void rtw_create_mem_pool_ex(char *name, int max, int min, int size);
#endif

#if defined(CONFIG_APOLLO_ROMEDRIVER)
int re8670_start_xmit_check(struct sk_buff *skb, struct net_device *dev);
#elif defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
extern int rtk_fc_skb_tx(struct sk_buff *skb, struct net_device *dev);
int re8670_start_xmit_fc(struct sk_buff *skb, struct net_device *dev);
extern int rtk_dev_update_stats(struct net_device *dev, struct rtnl_link_stats64 *stats);
#endif
/*static*/ int re8670_start_xmit (struct sk_buff *skb, struct net_device *dev);

/* Jonah + for FASTROUTE */
struct net_device *eth_net_dev[MAX_GMAC_NUM];
struct tasklet_struct *eth_rx_tasklets[MAX_GMAC_NUM]={0};

#define WITH_NAPI		""

/* These identify the driver base version and may not be removed. */
static char version[] = KERN_INFO DRV_NAME " Ethernet driver v" DRV_VERSION " (" DRV_RELDATE ")["DRV_DUALBAND"]" WITH_NAPI "\n";

MODULE_AUTHOR("Sam Hsu <sam_hsu@realtek.com>");
MODULE_DESCRIPTION("RealTek RTL-8686 series 10/100/1000 Ethernet driver");
MODULE_LICENSE("GPL");

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
   The RTL chips use a 64 element hash table based on the Ethernet CRC.  */
static int multicast_filter_limit = 32;
#if 1//LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)	//shlee 2.6
module_param(multicast_filter_limit, int, S_IRUGO|S_IWUSR);
#else
MODULE_PARM (multicast_filter_limit, "i");
#endif
MODULE_PARM_DESC (multicast_filter_limit, "8686 maximum number of filtered multicast addresses");

#ifndef CONFIG_RTK_L34_ENABLE
static unsigned int	SWITCH_MODE	= RTL8686_Switch_Mode_Normal;
#endif


#define PFX			DRV_NAME ": "
#define CP_DEF_MSG_ENABLE	(NETIF_MSG_DRV		| \
		NETIF_MSG_PROBE 	| \
		NETIF_MSG_LINK)
#define CP_REGS_SIZE		(0xff + 1)

#define DESC_ALIGN		0x100
#define UNCACHE_MASK		0xa0000000
/*add 1 desc for dummy desc : we will allocate one more desc even if the RING_SIZE is 0, 
this can let the ring which we disable has a dummy desc in its FDP, RD says our NIC need this...*/
#define RE8670_RXRING_BYTES(RING_SIZE)	( (sizeof(struct dma_rx_desc) * (RING_SIZE+1)) + DESC_ALIGN)
#define RE8670_TXRING_BYTES(RING_SIZE)	( (sizeof(struct dma_tx_desc) * (RING_SIZE+1)) + DESC_ALIGN)

#define NEXT_TX(N,RING_SIZE)		(((N) + 1) & (RING_SIZE - 1))
#define NEXT_RX(N,RING_SIZE)		(((N) + 1) & (RING_SIZE - 1))

#define NOT_IN_BITMAP(map, num) (!((map)&(1<<(num))))

static inline int idx_sw2hw(int ring_num) {
	return (MAX_TXRING_NUM-1)-ring_num;
}

//#define PKT_BUF_SZ		1536	/* Size of each temporary Rx buffer.*/
#ifndef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
#define RX_OFFSET		2	//move to re_privskb_adv.h
#endif

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024, 7==end of packet. */
/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT		(10*HZ)
/* hardware minimum and maximum for a single frame's data payload */
#define CP_MIN_MTU		60	/* TODO: allow lower, but pad */
#define CP_MAX_MTU		4096

#define TOTAL_RTL8686_DEV_NUM	((sizeof(rtl8686_dev_table))/(sizeof(struct rtl8686_dev_table_entry)))

#define RLE0787_W32(idx, reg, value)			(*(volatile u32*)((u32)re_private_data[idx].base+reg)) = (u32)value
#define RLE0787_W16(idx, reg, value)			(*(volatile u16*)((u32)re_private_data[idx].base+reg)) = (u16)value
#define RLE0787_W8(idx, reg, value)			(*(volatile u8*)((u32)re_private_data[idx].base+reg)) = (u8)value
#define RLE0787_R32(idx, reg)				(*(volatile u32*)((u32)re_private_data[idx].base+reg))
#define RLE0787_R16(idx, reg)				(*(volatile u16*)((u32)re_private_data[idx].base+reg))
#define RLE0787_R8(idx, reg)				(*(volatile u8*)((u32)re_private_data[idx].base+reg))
#define RLE0787_SET_BIT32(idx, reg, bits)		(RLE0787_W32(idx, reg, (RLE0787_R32(idx, reg) | (bits))))
#define RLE0787_CLR_BIT32(idx, reg, bits)		(RLE0787_W32(idx, reg, (RLE0787_R32(idx, reg) & ~(bits))))

#define TX_HQBUFFS_AVAIL(CP,ring_num)					\
		(((CP)->tx_Mhqtail[ring_num] - (CP)->tx_Mhqhead[ring_num] + (CP)->re8670_tx_ring_size[ring_num] - 1)&((CP)->re8670_tx_ring_size[ring_num] - 1))		

#define RX_HQBUFFS_EMPTY(CP,ring_num)					\
		(!ring_num ? ((CP)->rx_Mtail[ring_num] == RLE0787_R16((CP)->gmac, RxCDO)) : ((CP)->rx_Mtail[ring_num] == RLE0787_R16((CP)->gmac, RxCDO2+(ADDR_OFFSET*(ring_num-1)))))
		
#ifdef RX_MRING_INT_SPLIT/*plz add into both define and not define area*/
#define en_rx_mring_int_split(idx) RLE0787_W32(idx, CONFIG_REG, (RLE0787_R32(idx, CONFIG_REG) | En_int_split))
#define set_rring_route(idx) {RLE0787_W32(idx, RRING_ROUTING1, 0x65432111);RLE0787_W32(idx, RRING_ROUTING2, 0x65432111);RLE0787_W32(idx, RRING_ROUTING3, 0x65432111);RLE0787_W32(idx, RRING_ROUTING4, 0x65432111);RLE0787_W32(idx, RRING_ROUTING5, 0x65432111);RLE0787_W32(idx, RRING_ROUTING6, 0x65432111);RLE0787_W32(idx, RRING_ROUTING7, 0x65432111);}
#define retrive_isr1_status(idx) RLE0787_R32(idx, ISR1)
#define assigne_cpisr_status(x, y) (x) = (y)
#define assigne_cpisr1_status(x, y) (x) = (y)
#define gather_rx_isr(x,y) (x) = (x)|(!!(y & 0x3f))
#define gather_tx_isr(x,y) (x) = (x)|(((!!y) << 16))
#define CLEAR_ISR1(idx, x) RLE0787_W32(idx, ISR1, (x))
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
#define MASK_IMR0_TXALL(idx) RLE0787_R32(idx, IMR0)&=~(re_private_data[idx].rx_multiring_bitmap<<16)
#define UNMASK_IMR0_TXALL(idx) RLE0787_R32(idx, IMR0)|=(re_private_data[idx].rx_multiring_bitmap<<16)
#endif
#define MASK_IMR0_RXALL(idx) RLE0787_R32(idx, IMR0)&=~(re_private_data[idx].rx_multiring_bitmap)
#define UNMASK_IMR0_RXALL(idx) RLE0787_R32(idx, IMR0)|=(re_private_data[idx].rx_multiring_bitmap)
#else
#define en_rx_mring_int_split(idx)
#define set_rring_route(idx)
#define retrive_isr1_status(idx) 0
#define assigne_cpisr_status(x, y)
#define assigne_cpisr1_status(x, y)
#define gather_rx_isr(x, y)
#define CLEAR_ISR1(idx, x) 
#define MASK_IMR0_RXALL(idx)
#define UNMASK_IMR0_RXALL(idx)
#endif

static struct re_private_root re_private_data_root = {0};
static struct re_private re_private_data[MAX_GMAC_NUM] = {{0}};

void memDump(void *start, u32 size, char *strHeader, char format)
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

		if(format & RTL8686_MEM_DUMP_FORMAT_ADDR)
			printk("\n%08x ", (u32) line);
		else
			printk("\n ");
		
		//Hex
		if(format & RTL8686_MEM_DUMP_FORMAT_HEX)
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
		if(format & RTL8686_MEM_DUMP_FORMAT_TEXT)
			printk("  %s", ascii);
	}
	printk("\n");
	return;
}

#undef ETH_DBG
#define ETH_DBG
#ifdef ETH_DBG
static void skb_debug(const struct sk_buff *skb, int enable, int times, int flag)
{
#define NUM2PRINT 1518
	if (unlikely(times && (enable&flag))) {
		if (flag == RTL8686_SKB_RX)
			printk("\nI: ");
		else
			printk("\nO: ");
		printk("eth len = %d eth name %s", skb->len,skb->dev?skb->dev->name:"");
		memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "", RTL8686_MEM_DUMP_FORMAT_ALL);
		if(skb->len > NUM2PRINT){
			printk("........");
		}
		printk("\n");
	}
}
//int32 dump_hs (void);
//int32 dump_lut_table (void);

void rxinfo_debug(struct re_private *cp, struct rx_info *pRxInfo, int ring_num)
{
	if (unlikely(cp->debug_times && (cp->debug_enable&RTL8686_RXINFO))) {
		printk("rxInfo[ring%d]:\n", ring_num);		
		printk("opts1\t= 0x%08x Own=%d EOR=%d FS=%d LS=%d CRCErr=%d IP4CSErr=%d L4CSErr=%d RCDF=%d IPFrag=%d PPPoETag=%d RWT=%d Len=%d\n", pRxInfo->opts1.dw,
			pRxInfo->opts1.bit.own,
			pRxInfo->opts1.bit.eor,
			pRxInfo->opts1.bit.fs,
			pRxInfo->opts1.bit.ls,
			pRxInfo->opts1.bit.crcerr,
			pRxInfo->opts1.bit.ipv4csf,
			pRxInfo->opts1.bit.l4csf,
			pRxInfo->opts1.bit.rcdf,
			pRxInfo->opts1.bit.ipfrag,			
			pRxInfo->opts1.bit.pppoetag,
			pRxInfo->opts1.bit.rwt,
			pRxInfo->opts1.bit.data_length
		);
		printk("addr\t= 0x%08x\n", pRxInfo->addr);
		printk("opts2\t= 0x%08x CpuTag=%d PTPCpuTagEst=%d SVLAN_Est=%d Reason=%d CtagVa=%d CvlanTag=%d\n", pRxInfo->opts2.dw,
			pRxInfo->opts2.bit.cputag,	
			pRxInfo->opts2.bit.ptp_in_cpu_tag_exist,
			pRxInfo->opts2.bit.svlan_tag_exist,
			pRxInfo->opts2.bit.reason,
			pRxInfo->opts2.bit.ctagva,
			pRxInfo->opts2.bit.cvlan_tag
		);		
		printk("opts3\t= 0x%08x IntPri=%d PONSID_or_EXTSPA=%d L3Rout=%d ORG=%d SrcPortNum=%d FBI=%d FBHSH_or_DstPortmsk=0x%x\n", pRxInfo->opts3.dw,
			pRxInfo->opts3.bit.internal_priority,	
			pRxInfo->opts3.bit.pon_sid_or_extspa,	
			pRxInfo->opts3.bit.l3routing,	
			pRxInfo->opts3.bit.origformat,	
			pRxInfo->opts3.bit.src_port_num,
			pRxInfo->opts3.bit.fbi,
			pRxInfo->opts3.bit.fb_hash_or_dst_portmsk
		);
	}
}

void txinfo_debug(struct re_private *cp, struct tx_info *pTxInfo)
{
	if (unlikely(cp->debug_times && (cp->debug_enable&RTL8686_TXINFO))) {
		printk("txInfo:\n");
		printk("opts1\t= 0x%08x Own=%d EOR=%d FS=%d LS=%d IPCS=%d L4CS=%d TPID_Sel=%d STAG_Aware=%d CRC=%d Len=%d\n", pTxInfo->opts1.dw,
			pTxInfo->opts1.bit.own,
			pTxInfo->opts1.bit.eor,
			pTxInfo->opts1.bit.fs,
			pTxInfo->opts1.bit.ls,
			pTxInfo->opts1.bit.ipcs,
			pTxInfo->opts1.bit.l4cs,
			pTxInfo->opts1.bit.tpid_sel,
			pTxInfo->opts1.bit.stag_aware,
			pTxInfo->opts1.bit.crc,
			pTxInfo->opts1.bit.data_length
		);
		printk("addr\t= 0x%08x \n", pTxInfo->addr);
		printk("opts2\t= 0x%08x CPUTag=%d SVLANAct=%d CVLANAct=%d TxPmsk=0x%x CVLAN_VID=%d CVLAN_Pri=%d\n", pTxInfo->opts2.dw,
			pTxInfo->opts2.bit.cputag,			
			pTxInfo->opts2.bit.tx_svlan_action,
			pTxInfo->opts2.bit.tx_cvlan_action,
			pTxInfo->opts2.bit.tx_portmask,
			pTxInfo->opts2.bit.cvlan_vidh<<8|pTxInfo->opts2.bit.cvlan_vidl,
			pTxInfo->opts2.bit.cvlan_prio
		);
		printk("opts3\t= 0x%08x AsPri=%d CpuPri=%d Keep=%d DisLrn=%d Psel=%d L34Keep=%d extspa=%d Pppoe=%d SidIdx=%d PON_SID=%d\n", pTxInfo->opts3.dw,
			pTxInfo->opts3.bit.aspri,
			pTxInfo->opts3.bit.cputag_pri,
			pTxInfo->opts3.bit.keep,
			pTxInfo->opts3.bit.dislrn,
			pTxInfo->opts3.bit.cputag_psel,
			pTxInfo->opts3.bit.l34_keep,
			pTxInfo->opts3.bit.extspa,
			pTxInfo->opts3.bit.tx_pppoe_action,
			pTxInfo->opts3.bit.tx_pppoe_idx,
			pTxInfo->opts3.bit.tx_dst_stream_id
		);
		printk("opts4\t= 0x%08x LgsEn=%d LgMtu=%d SVLAN_VID=%d SVLAN_Pri=%d\n", pTxInfo->opts4.dw,
			pTxInfo->opts4.bit.lgsen,
			pTxInfo->opts4.bit.lgmtu,
			pTxInfo->opts4.bit.svlan_vidh<<8|pTxInfo->opts4.bit.svlan_vidl,
			pTxInfo->opts4.bit.svlan_prio
		);	
	}
}

static char mt_watch_tmp[512];

#define OTHERS_TRACE(gmac, enable, times, comment, arg...) \
do {\
		if(unlikely(times && (enable&RTL8686_OTHERS)))\
		{\
			u8 mt_trace_head_str[]="[NIC OTHERS]\033[1;36m"; \
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s %s \033[1;30m@%s:%d\033[0m\n",mt_trace_head_str,mt_watch_tmp,__FILE__,__LINE__);\
		}\
} while(0)

#define RX_TRACE(gmac, enable, times, comment, arg...) \
do {\
		if(unlikely(times && (enable&RTL8686_RX_TRACE)))\
		{\
			u8 mt_trace_head_str[]="[NIC RX]\033[1;36m"; \
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s %s \033[1;30m@%s:%d\033[0m\n",mt_trace_head_str,mt_watch_tmp,__FILE__,__LINE__);\
		}\
} while(0)

#define TX_TRACE(gmac, enable, times, comment, arg...) \
do {\
		if(unlikely(times && (enable&RTL8686_TX_TRACE)))\
		{\
			u8 mt_trace_head_str[]="[NIC TX]\033[1;36m"; \
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s %s \033[1;30m@%s:%d\033[0m\n",mt_trace_head_str,mt_watch_tmp,__FILE__,__LINE__);\
		}\
} while(0)

#define RX_WARN(gmac, enable, times, comment, arg...) \
do {\
		if(unlikely(times && (enable&RTL8686_RX_WARN)))\
		{\
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s\n",mt_watch_tmp);\
		}\
} while(0)

#define TX_WARN(gmac, enable, times, comment, arg...) \
do {\
		if(unlikely(times && (enable&RTL8686_TX_WARN)))\
		{\
			int mt_trace_i;\
			sprintf( mt_watch_tmp, comment,## arg);\
			for(mt_trace_i=0;mt_trace_i<512;mt_trace_i++) \
			{\
				if(mt_watch_tmp[mt_trace_i]==0) break; \
			}\
			if(mt_watch_tmp[mt_trace_i-1]=='\n') mt_watch_tmp[mt_trace_i-1]=' ';\
			printk("%s\n",mt_watch_tmp);\
		}\
} while(0)

#define ETHDBG_PRINT(gmac, enable, times, flag, fmt, args...)  if(unlikely(times && (enable&flag))) printk(fmt, ##args)
#define SKB_DBG(args...) skb_debug(args)
#define RXINFO_DBG(CP, args...) rxinfo_debug(CP, args)
#define TXINFO_DBG(CP, args...) txinfo_debug(CP, args)
#define RXDBG_TIMES_UPDATE(CP) \
do { \
		if(unlikely((CP->debug_times>0) && (CP->debug_enable&RTL8686_RX_ALL))) \
		{ \
			CP->debug_times--; \
		} \
} while(0)
#define TXDBG_TIMES_UPDATE(CP) \
do { \
		if(unlikely((CP->debug_times>0) && (CP->debug_enable&RTL8686_TX_ALL))) \
		{ \
			CP->debug_times--; \
		} \
} while(0)
#else
#define RX_TRACE(gmac, enable, times, args...)
#define TX_TRACE(gmac, enable, times, args...)
#define RX_WARN(gmac, enable, times, comment, arg...)
#define TX_WARN(gmac, enable, times, comment, arg...)
#define ETHDBG_PRINT(gmac, enable, times, flag, fmt, args...)
#define SKB_DBG(args...) 
#define RXINFO_DBG(CP, args...)
#define TXINFO_DBG(CP, args...)
#define RXDBG_TIMES_UPDATE(CP)
#define TXDBG_TIMES_UPDATE(CP)
#endif

struct ring_info {
	struct sk_buff		*skb;
	dma_addr_t		mapping;
	unsigned		frag;
};

static int dev_num=0;

#define ROOTDEV	(re_private_data_root.dev)
#define ROOTPRIV2PRIV(idx)  ((struct re_private*)re_private_data_root.re_private_data_ptr[idx])
#define PRIV2DEV(priv)  (((struct re_private_root*)priv)->dev)
#define DEV2CP(dev)  (((struct re_dev_private*)dev->priv)->pCp)
#define DEVPRIV(dev)  ((struct re_dev_private*)dev->priv)
#define VTAG2DESC(d) (((((d) & 0x00ff)<<8) | (((d) & 0xff00)>>8)) & 0x0000ffff)
/*warning! +1 for smux.................................................*/
#define VTAG2VLANTCI(v) (( (((v) & 0xff00)>>8) | (((v) & 0x00ff)<<8) ) + 1) 

static void __re8670_set_rx_mode(unsigned int gmac);
static void re8670_tx(struct re_private *cp, int ring_num, int from_hw);

static void re8670_clean_rings(struct re_private *cp);
static void re8670_tx_timeout(struct net_device *dev);
static int change_dev_port_mapping(int port_num ,char* name);
static int change_tx_force_gmac(char* name);
int re8670_reset(void);

/*================================================
			GMAC used Global Variable
================================================*/ 

#if defined(CONFIG_RX_WIFI_FF_AGG)
#include "aggQ.c"
#endif /* CONFIG_RX_WIFI_FF_AGG */

#if defined(CONFIG_RTL865X_ETH_PRIV_SKB)
extern int eth_skb_free_num;
extern int eth_skb_alloc_num;
extern int lowest_eth_skb_free_num;
extern int critical_eth_skb_free_num;
extern int dynamic_alloc_skb_num;
#endif

static struct rtl8686_dev_table_entry rtl8686_dev_table[] = {	
	//ifname, ifflag, vid, phyPort, dev_instant
	{"eth0",		RTL8686_ELAN, 0, CPU_PORT0, 0, NULL}, // root dev eth0 must be first
	{"lan0",		RTL8686_ELAN, 0, LAN_PORT1, 1, NULL},
	{"lan1",		RTL8686_ELAN, 0, LAN_PORT2, 1, NULL},
	{"lan2",		RTL8686_ELAN, 0, LAN_PORT3, 1, NULL},
#if defined(CONFIG_RTL_ETH_R1004)
	{"lan3",		RTL8686_WAN, 0, LAN_PORT4, 1, NULL},
#else
	{"lan3",		RTL8686_ELAN, 0, LAN_PORT4, 1, NULL},
#endif
	{"lan4",		RTL8686_ELAN, 0, LAN_PORT5, 1, NULL},
	{"lan5",		RTL8686_ELAN, 0, LAN_PORT6, 1, NULL},
/*#if !defined(CONFIG_ETHWAN_USE_USB_SGMII) && !defined(CONFIG_ETHWAN_USE_PCIE1_SGMII)
	{"eth0.8",		RTL8686_ELAN, 0, APOLLOPRO_SGMII0_PORT, 1, NULL},
	{"eth0.9",		RTL8686_ELAN, 0, APOLLOPRO_SGMII1_PORT, 1, NULL},
#elif !defined(CONFIG_ETHWAN_USE_USB_SGMII)
	{"eth0.8",		RTL8686_ELAN, 0, APOLLOPRO_SGMII0_PORT, 1, NULL},
#elif !defined(CONFIG_ETHWAN_USE_PCIE1_SGMII)
	{"eth0.8",		RTL8686_ELAN, 0, APOLLOPRO_SGMII1_PORT, 1, NULL},
#endif*/
#if defined(CONFIG_RTL_ETH_R1004)
	{"eth1",			RTL8686_ELAN, 0, WAN_PORT, 0, NULL},
#else
	{"eth1",			RTL8686_WAN, 0, WAN_PORT, 0, NULL},
#endif
#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
	{"ifprobe",		RTL8686_WAN, 0, LAN_PORT6, 0, NULL}, 
#endif
	/*{"pon",			RTL8686_WAN, 0, WAN_PORT, 0, NULL},*/
};

#ifdef CONFIG_RTL8686_SWITCH
struct netif_carrier_dev_mapping
{
	unsigned char 	ifname[IFNAMSIZ];
	struct net_device *phy_dev;
	unsigned char status; // 0 : disable , 1 : enable
	int linkchangetimes;
};

// global variable for link change interrupt device mapping

int wan_first_link_up=0;
struct netif_carrier_dev_mapping LCDev_mapping[SW_PORT_NUM];

static void gmacintr_notifier_link_change(intrBcasterMsg_t	*pMsgData)
{
#if 0
    printk("intrType	= %d\n", pMsgData->intrType);
    printk("intrSubType	= %u\n", pMsgData->intrSubType);
    printk("intrBitMask	= %u\n", pMsgData->intrBitMask);
    printk("intrStatus	= %d\n", pMsgData->intrStatus);
#endif

	if(!IS_LAN_PORT(pMsgData->intrBitMask))
		return;

	if(pMsgData->intrType == MSG_TYPE_LINK_CHANGE) // Link Change Interrupt
	{
		if(pMsgData->intrBitMask < SW_PORT_NUM)
		{
			if(pMsgData->intrSubType == 1 && LCDev_mapping[pMsgData->intrBitMask].status == 0) // Link Up
			{
				//nas0 default status is running, do carrier off to notifier userspace when first link up
				if(strncmp(LCDev_mapping[pMsgData->intrBitMask].phy_dev->name,"nas0",4)==0){
					if(wan_first_link_up <= 1){
						if (netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
							netif_carrier_off(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
						wan_first_link_up++;
					}
				}
				if (!netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_on(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 1;
			}
			else if (pMsgData->intrSubType == 2 && LCDev_mapping[pMsgData->intrBitMask].status == 1) //Link Down 
			{
				if (netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_off(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 0;
			}
			else printk("%s,something error,dev:%s\n",__func__,LCDev_mapping[pMsgData->intrBitMask].phy_dev->name);
			
		}
	}
}
// Handle Link Change Interrupt for Netlink 
static intrBcasterNotifier_t GMAClinkChangeNotifier = {
    .notifyType = MSG_TYPE_LINK_CHANGE,
    .notifierCb = gmacintr_notifier_link_change,
};

#if defined(CONFIG_ETHWAN_USE_USB_SGMII) || defined(CONFIG_ETHWAN_USE_PCIE1_SGMII)
static void gmacintr_notifier_ethwan_link_change(intrBcasterMsg_t	*pMsgData)
{
#if 0
    printk("intrType	= %d\n", pMsgData->intrType);
    printk("intrSubType	= %u\n", pMsgData->intrSubType);
    printk("intrBitMask	= %u\n", pMsgData->intrBitMask);
    printk("intrStatus	= %d\n", pMsgData->intrStatus);
#endif

	if(!IS_WAN_PORT(pMsgData->intrBitMask))
		return;

	if(pMsgData->intrType == MSG_TYPE_LINK_CHANGE) // Link Change Interrupt
	{
		if(pMsgData->intrBitMask < SW_PORT_NUM && pMsgData->intrBitMask >= 0)
		{
			if(pMsgData->intrSubType == 1 && LCDev_mapping[pMsgData->intrBitMask].status == 0) // Link Up
			{
				if (!netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_on(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 1;
			}
			else if (pMsgData->intrSubType == 2 && LCDev_mapping[pMsgData->intrBitMask].status == 1) //Link Down 
			{
				if (netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_off(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 0;
			}
			else printk("something error ?!\n");
			
		}
	}
}
// Handle Link Change Interrupt for Netlink 
static intrBcasterNotifier_t GMACethwanStateChangeNotifier = {
    .notifyType = MSG_TYPE_LINK_CHANGE,
    .notifierCb = gmacintr_notifier_ethwan_link_change,
};
#else
static void gmacintr_notifier_onu_state_change(intrBcasterMsg_t	*pMsgData)
{
#if 0
    printk("intrType	= %d\n", pMsgData->intrType);
    printk("intrSubType	= %u\n", pMsgData->intrSubType);
    printk("intrBitMask	= %u\n", pMsgData->intrBitMask);
    printk("intrStatus	= %d\n", pMsgData->intrStatus);
#endif

	if(!IS_PON_PORT(pMsgData->intrBitMask))
		return;
	
	if(pMsgData->intrType == MSG_TYPE_ONU_STATE) // ONU state Change event
	{
		if(pMsgData->intrBitMask < SW_PORT_NUM)
		{
			if(pMsgData->intrSubType == GPON_STATE_O5 && LCDev_mapping[pMsgData->intrBitMask].status == 0) // Link Up
			{
				if (!netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_on(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 1;
			}
			else if (pMsgData->intrSubType != GPON_STATE_O5 && LCDev_mapping[pMsgData->intrBitMask].status == 1) //Link Down 
			{
				if (netif_carrier_ok(LCDev_mapping[pMsgData->intrBitMask].phy_dev))
				{
					netif_carrier_off(LCDev_mapping[pMsgData->intrBitMask].phy_dev);
				}
				LCDev_mapping[pMsgData->intrBitMask].status = 0;
			}			
		}
	}
}
// Handle Link Change Interrupt for Netlink 
static intrBcasterNotifier_t GMAConuStateChangeNotifier = {
    .notifyType = MSG_TYPE_ONU_STATE,
    .notifierCb = gmacintr_notifier_onu_state_change,
};
#endif
#endif
static int re8670_set_mac_addr(struct net_device *dev, void *addr_p)
{
	struct re_private_root *root_cp = DEV2CP(dev);
	struct re_private *cp;
	unsigned int gmac;
	struct sockaddr *addr = addr_p;

	if (netif_running(dev))
        return -EBUSY;

    if (!is_valid_ether_addr(addr->sa_data))
    	return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
	
	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		cp = root_cp->re_private_data_ptr[gmac];
		
		if(cp->gmac_enabled != GMAC_TRUE)
			continue;
	
		RLE0787_W32(cp->gmac, IDR0 , (dev->dev_addr[0] << 24) | (dev->dev_addr[1] << 16) | 
				(dev->dev_addr[2] << 8) | (dev->dev_addr[3] << 0));
		RLE0787_W32(cp->gmac, IDR4 , (dev->dev_addr[4] << 24) | (dev->dev_addr[5] << 16));
	}
    return 0;
}

static int config_rx_sideband(u32 gmac, u8 enabled)
{
	if(enabled)
		RLE0787_SET_BIT32(gmac, CONFIG_REG, (1<<22)|(1<<23));
	else
		RLE0787_CLR_BIT32(gmac, CONFIG_REG, (1<<22)|(1<<23));

	return 0;
}

static int config_tx_jumbo(u32 gmac, u8 enabled)
{
	if(enabled)
	{
		RLE0787_SET_BIT32(gmac, TCR, (1<<15));
		RLE0787_SET_BIT32(gmac, IO_CMD, (1<<28));
		RLE0787_SET_BIT32(gmac, CONFIG_REG, (1<<16));
	}
	else
	{
		RLE0787_CLR_BIT32(gmac, TCR, (1<<15));
		RLE0787_CLR_BIT32(gmac, IO_CMD, (1<<28));
		RLE0787_CLR_BIT32(gmac, CONFIG_REG, (1<<16));
	}

	return 0;
}

#ifdef CONFIG_REALTEK_HW_LSO
static int re8670_set_mtu(struct net_device *dev, int new_mtu)
{
	//printk("%s-%d: dev=%s new_mtu=%d\n", __func__, __LINE__, dev->name, new_mtu);
	
	if (new_mtu < 68 || new_mtu > RE8686_ETH_DATA_LEN)
	{
		return -EINVAL;
	}

	dev->mtu = new_mtu;
	return 0;
}

static int re8670_init_mtu(void)
{
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	int i;
	
	for(i=0;i<totalDev;i++)
	{
		re8670_set_mtu(rtl8686_dev_table[i].dev_instant, CP_LS_MTU);
	}

	return 0;
}

static int re8670_get_mtu(struct sk_buff *skb, struct re_private *cp)
{
	int mtu;

	if (skb->protocol == htons(ETH_P_IPV6))
		return 0;

	if(skb_is_gso(skb))
	{
		if(skb_gso_network_seglen(skb) <= skb->dev->mtu)
		{
			mtu = skb_gso_network_seglen(skb);
		}
		else
		{
			mtu = skb->dev->mtu;
		}

		if (mtu > RE8670_MAX_ETH_FRAME_SIZE)
		{
			mtu = RE8670_MAX_ETH_FRAME_SIZE;
		}
		
		TX_TRACE(cp->gmac, cp->debug_enable, cp->debug_times, "mtu=%d\n", mtu);
		return mtu;
	}
	else if (skb_shinfo(skb)->nr_frags)
	{
		mtu = skb->dev->mtu;
		
		if (mtu > RE8670_MAX_ETH_FRAME_SIZE)
		{
			mtu = RE8670_MAX_ETH_FRAME_SIZE;
		}

		TX_TRACE(cp->gmac, cp->debug_enable, cp->debug_times, "mtu=%d\n", mtu);
		return mtu;
	}
	else
		return 0;
}
#endif

static int re8670_set_features(struct net_device *dev, netdev_features_t features)
{
	printk("%s %d: enter\n", __func__, __LINE__);
	return 0;
}

#ifdef TX_RECYCLE_SKB_USING_TOK_INT
static inline u32 read_isr_status(struct re_private *cp){
	unsigned int gmac = cp->gmac;
	u32 isr_status = (RLE0787_R16(gmac, ISR) & (RX_ALL(gmac) | TOK));

	/*rx mring split int*/
	assigne_cpisr_status(cp->isr_status, isr_status);
	assigne_cpisr1_status(cp->isr1_status, (retrive_isr1_status(gmac) & cp->rx_multiring_bitmap));
	gather_rx_isr(isr_status, cp->isr1_status);
	assigne_cpisr1_status(cp->isr1_status_tx, (retrive_isr1_status(gmac) & (cp->tx_multiring_bitmap<<16)));
	gather_tx_isr(isr_status, cp->isr1_status_tx);
	return isr_status;
}
#else
static inline u16 read_isr_status(struct re_private *cp){
	unsigned int gmac = cp->gmac;
	u16 isr_status = (RLE0787_R16(gmac, ISR) & RX_ALL(gmac));

	/*rx mring split int*/
	assigne_cpisr_status(cp->isr_status, isr_status);
	assigne_cpisr1_status(cp->isr1_status, (retrive_isr1_status(gmac) & cp->rx_multiring_bitmap));
	gather_rx_isr(isr_status, cp->isr1_status);
	return isr_status;
}
#endif

static inline void mask_rx_int(unsigned int gmac){
	RLE0787_R16(gmac, IMR)&=(u16)(~RX_ALL(gmac));
	MASK_IMR0_RXALL(gmac);
}

static inline void unmask_rx_int(unsigned int gmac){
	RLE0787_R16(gmac, IMR)|=(u16)(RX_ALL(gmac));//we still open imr when rx_work==0 for a quickly schedule
	UNMASK_IMR0_RXALL(gmac);
}

#ifdef TX_RECYCLE_SKB_USING_TOK_INT
static inline void mask_tx_int(unsigned int gmac){
	RLE0787_R16(gmac, IMR)&=(u16)(~(TOK|TOK_TI));
	MASK_IMR0_TXALL(gmac);
}

static inline void unmask_tx_int(unsigned int gmac){
	RLE0787_R16(gmac, IMR)|=(u16)(TOK|TOK_TI);//we still open imr when rx_work==0 for a quickly schedule
	UNMASK_IMR0_TXALL(gmac);
}
#endif

static inline void clear_isr(struct re_private *cp, u16 status){
	unsigned int gmac = cp->gmac;
	
	RLE0787_W16(gmac, ISR, status);
#ifdef RX_MRING_INT_SPLIT
	CLEAR_ISR1(gmac, (cp->isr1_status&~(cp->rx_multiring_bitmap)));
#else
	CLEAR_ISR1(gmac, cp->isr1_status);
#endif
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
	CLEAR_ISR1(gmac, cp->isr1_status_tx);
#endif
}

__IRAM_NIC
struct sk_buff *re8670_getAlloc(unsigned int size)
{	
	struct sk_buff *skb=NULL;
	
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
	skb = dev_alloc_skb_priv_eth(size);
#elif defined(CONFIG_RTL865X_ETH_PRIV_SKB)
	if ( eth_skb_alloc_num < RE8670_MAX_ALLOC_RXSKB_NUM ) 
	{
		skb = dev_alloc_skb_priv_eth(size);
#if RE8670_DYNAMIC_ALLOC_RXSKB_NUM
		if(skb == NULL)
		{
			dynamic_alloc_skb_num++;
			skb = dev_alloc_skb(size);
			if(skb && (unsigned int)skb->data&0xf) {
				skb->data = (((unsigned int)skb->data)&0xfffffff0)+0x10;
			}
		}
#endif
	}
#else
#ifdef CONFIG_RTW_MEMPOOL
	skb = rtw_mem_pool_alloc_skb(size);
#else
	skb = dev_alloc_skb(size);
#endif
#endif

	return skb;
}

__IRAM_NIC
struct sk_buff *re8670_getMcAlloc(unsigned int size)
{	
	struct sk_buff *skb=NULL;
	
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB
	if ( eth_skb_alloc_num < RE8670_MAX_ALLOC_MC_NUM ) 
	{
		skb = dev_alloc_skb_priv_eth(size);
#if RE8670_DYNAMIC_ALLOC_RXSKB_NUM
		if(skb == NULL)
		{
			dynamic_alloc_skb_num++;
			skb = dev_alloc_skb(size);
			if(skb && (unsigned int)skb->data&0xf) {
				skb->data = (((unsigned int)skb->data)&0xfffffff0)+0x10;
			}
		}
#endif
	}
#else
#ifdef CONFIG_RTW_MEMPOOL
	skb = rtw_mem_pool_alloc_skb(size);
#else
	skb = dev_alloc_skb(size);
#endif
#endif

	return skb;
}

__IRAM_NIC
struct sk_buff *re8670_getBcAlloc(unsigned int size)
{	
	struct sk_buff *skb=NULL;
	
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB 
	if ( eth_skb_alloc_num < RE8670_MAX_ALLOC_BC_NUM )
	{
		skb = dev_alloc_skb_priv_eth(size);
#if RE8670_DYNAMIC_ALLOC_RXSKB_NUM
		if(skb == NULL)
		{
			dynamic_alloc_skb_num++;
			skb = dev_alloc_skb(size);
			if(skb && (unsigned int)skb->data&0xf) {
				skb->data = (((unsigned int)skb->data)&0xfffffff0)+0x10;
			}
		}
#endif
	}
#else
#ifdef CONFIG_RTW_MEMPOOL
		skb = rtw_mem_pool_alloc_skb(size);
#else
		skb = dev_alloc_skb(size);
#endif
#endif

	return skb;
}

__IRAM_NIC
struct sk_buff *re8670_getCriticalAlloc(unsigned int size)
{
	struct sk_buff *skb=NULL;

#ifdef CONFIG_RTL865X_ETH_PRIV_SKB  //For RTL8192CE
	skb = dev_alloc_critical_skb_priv_eth(CROSS_LAN_MBUF_LEN);
#else
#ifdef CONFIG_RTW_MEMPOOL
	skb = rtw_mem_pool_alloc_skb(size);
#else
	skb = dev_alloc_skb(size);
#endif
#endif

	return skb;
}

__IRAM_NIC
struct sk_buff *re8670_getJumboAlloc(unsigned int size)
{
	struct sk_buff *skb=NULL;

#ifdef CONFIG_RTL865X_ETH_PRIV_SKB  //For RTL8192CE
	skb = dev_alloc_skb_priv_eth_rg(CROSS_LAN_MBUF_LEN_RG);
#else
#ifdef CONFIG_RTW_MEMPOOL
	skb = rtw_mem_pool_alloc_skb(size);
#else
	skb = dev_alloc_skb(size);
#endif
#endif

	return skb;
}

static inline void rtk_gmac_set_rxbufsize (struct re_private_root *root_cp)
{
	unsigned int mtu = root_cp->dev->mtu;
	struct re_private *cp;	
	unsigned int gmac;

	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		cp = root_cp->re_private_data_ptr[gmac];

		if(cp->gmac_enabled != GMAC_TRUE)
			continue;
	
		if (mtu > ETH_DATA_LEN)
			/* MTU + ethernet header + FCS + optional VLAN tag */
			cp->rx_buf_sz = mtu + ETH_HLEN + 8;
		else
			cp->rx_buf_sz = SKB_BUF_SIZE;
	}
}

atomic_t lock_tx_tail = ATOMIC_INIT(0);

int re8686_register_rxfunc_by_port(unsigned int gmac, unsigned int port, p2rfunc_t pfunc){
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];	

	if(port >= SW_PORT_NUM || !pfunc)
		return -EINVAL;
	cp->port2rxfunc[port] = pfunc;
	return 0;
}

int re8686_register_rxfunc_all_port(p2rfunc_t pfunc){
	unsigned int gmac;
	unsigned int i;
	int ret;
	
	if(!pfunc)
		return -EINVAL;

	for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {
		for(i=0;i<SW_PORT_NUM;i++) {
			ret = re8686_register_rxfunc_by_port(gmac, i, pfunc);
			if(ret){
				return ret;
			}
		}
	}

	return 0;
}

void re8686_reset_rxfunc_to_default(unsigned int gmac) {
	unsigned int i;
	for(i=0;i<SW_PORT_NUM;i++){
		re8686_register_rxfunc_by_port(gmac, i, re8670_rx_skb);
	}
}

static inline void _tx_additional_setting(struct sk_buff *skb, struct net_device *dev, struct tx_info *pTxInfo){
	pTxInfo->opts2.bit.cputag = 1;

#if defined(CONFIG_SFU)
	//the txPmask and VLAN information should be decided in fwdEngine_xmit and recorded in txInfo,
	//so we do not need these check code here
#else
	if(pTxInfo->opts2.bit.tx_portmask == 0){
		pTxInfo->opts2.bit.tx_portmask = DEVPRIV(dev)->txPortMask;
	}
	
	//luke:20130411, patch for protocol stack broadcast hardware lookup packet to LAN will fail 
	//this patch should be remove when fwdEngine Tx module is ready.
	if((pTxInfo->opts2.bit.tx_portmask == 0) && ((skb->data[0]&1)==1))
	{

			//luke:20130506, patch for broadcast packet from WAN port will return to WAN port, cause spanning tree protocol treats it as loop
			if(skb->from_dev && (skb->from_dev->priv_flags & (IFF_DOMAIN_ELAN|IFF_DOMAIN_WAN)))
			{
				//printk("skb is from %s %x, did not tx to wan port\n",skb->from_dev->name,DEVPRIV(skb->from_dev)->txPortMask);
			}
			else
			{
				//printk("broadcast!! the dev name is %s, txportmask is %x\n",dev->name,DEVPRIV(dev)->txPortMask);
				//force do DirectTx to all port except CPU port
				pTxInfo->opts2.bit.tx_portmask=0x2f;
			}
			//printk("the txpmsk is %x\n",pTxInfo->opts3.bit.tx_portmask);

	}
#endif

	if(skb->from_dev){
		if(skb->from_dev->priv_flags & (IFF_DOMAIN_ELAN|IFF_DOMAIN_WAN)){
			pTxInfo->opts3.bit.dislrn = 1; //disable cpu port learning
		}
	}
#if defined(CONFIG_RTK_L34_ENABLE) && defined(CONFIG_RG_API_RLE0371)
	//tysu patch for 0371
	pTxInfo->opts2.bit.tx_portmask=0;
#endif
	
	//20130724: If NIC TX send by HWLOOKUP, L34 Keep will cause un-except problem.
	if(pTxInfo->opts2.bit.tx_portmask!=0) pTxInfo->opts3.bit.l34_keep = 1;	//ensure switch won't modify or filter packet
	//20141104LUKE: when L34Keep is on, Keep is also needed for gpon.
	if(pTxInfo->opts3.bit.l34_keep)pTxInfo->opts3.bit.keep = 1;

	//20131028: If ipv4 packet length <=60, then recaculate data_length in txInfo. Otherwise GMAC will cause L4chksum error
	//20131029: OMCI packet may cause data_length too big, so check streamID to filter them!!
	if(skb->len==60
#ifdef CONFIG_GPON_FEATURE
		&& pTxInfo->opts3.bit.tx_dst_stream_id!=0x40
#endif
		)
	{
		u16 off=12;
		u32 xlen=0;
	
		if((*(u16*)(skb->data+off))==htons(0x8100))//CTAG
	        off+=4;
		if(((*(u16*)(skb->data+off))==htons(0x8863))||((*(u16*)(skb->data+off))==htons(0x8864)))//PPPoE
			off+=8;
		if(((*(u16*)(skb->data+off))==htons(0x0800))||((*(u16*)(skb->data+off))==htons(0x0021)))//IPv4 or IPv4 with PPPoE
		{
			xlen=(off+2+((skb->data[off+4]<<8)|skb->data[off+5]))&0x1ffff;	//l3offset + IPv4 total length
			if(xlen<=60)
				pTxInfo->opts1.bit.data_length=xlen;
		}
			
	}
}

static 
__IRAM_NIC
void	tx_additional_setting(struct sk_buff *skb, struct net_device *dev, struct tx_info *pTxInfo)
{
#ifndef RE8686_VERIFY
	_tx_additional_setting(skb, dev, pTxInfo);
#else
	tx_additional_setting_verify_version(skb, dev, pTxInfo);
#endif
}

//#define RE8686_VERIFY
#ifdef RE8686_VERIFY
#include "./re8686_verify.c"
#else
#define INVERIFYMODE 0
#endif


__IRAM_NIC
#if !defined(CONFIG_RG_WMUX_SUPPORT) && !defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
static
#endif
struct net_device* decideRxDevice(struct re_private *cp, struct rx_info *pRxInfo){
	unsigned int num = (pRxInfo->opts3.bit.src_port_num >= SW_PORT_NUM) ? 
		(0) : pRxInfo->opts3.bit.src_port_num ;

	return cp->port2dev[num];
}

static inline void updateRxStatic(struct re_private *cp, struct sk_buff *skb){
	skb->dev->last_rx = jiffies;
	cp->cp_stats.rx_sw_num++;
	DEVPRIV(skb->dev)->net_stats.rx_packets++;
	DEVPRIV(skb->dev)->net_stats.rx_bytes += skb->len;
}


__IRAM_NIC
int re8670_rx_skb(struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo)
{
#ifdef RX_NAPI_MODE
	struct net_device *dev;
#endif
	unsigned int gmac = cp->gmac;
	struct net_device *net_device_record = NULL;
	int ret;

	skb->dev = decideRxDevice(cp, pRxInfo);
#ifdef RX_NAPI_MODE
	dev = skb->dev;
#endif
	net_device_record = skb->dev;

	skb->vlan_tci = (pRxInfo->opts2.bit.ctagva) ? VTAG2VLANTCI(pRxInfo->opts2.bit.cvlan_tag) : 0;
	ETHDBG_PRINT(gmac, cp->debug_enable, cp->debug_times, RTL8686_SKB_RX, "This packet comes from %s(vlan %u)\n", skb->dev->name, skb->vlan_tci);

	skb->from_dev=skb->dev;

	/* switch_port is patched for iptables and ebtables rule matching */
	//skb->switch_port = NULL;
#if 0 //remove it, it will influence FC to assign GPON SID to mark 
	skb->mark = (skb->vlan_tci & 0xFFF);
#endif
	//printk("%s %d switch_port: %s vlan_tci=0x%x mark=0x%x\n", 
	//		__func__, __LINE__, skb->switch_port, skb->vlan_tci, skb->mark);

	skb->protocol = eth_type_trans (skb, skb->dev);
	
	//do we need any wan dev rx hacking here?(before pass to netif_rx)
	
	updateRxStatic(cp, skb);
	SKB_DBG(skb, cp->debug_enable, cp->debug_times, RTL8686_SKB_RX);
#ifdef RX_NAPI_MODE
	if(dev->features & NETIF_F_GRO)
	{
		ret = napi_gro_receive(&cp->napi, skb);
		if(ret == GRO_DROP)
			DEVPRIV(net_device_record)->net_stats.rx_dropped++;
#ifdef RX_NAPI_MODE_DEBUG
		switch(ret)
		{
			case GRO_NORMAL:
				cp->cp_stats.rx_napi_gro_normal++;
				break;
			case GRO_DROP:
				cp->cp_stats.rx_napi_gro_drop++;
				break;
			case GRO_MERGED_FREE:
				cp->cp_stats.rx_napi_gro_merged_free++;
				break;
			case GRO_HELD:
				cp->cp_stats.rx_napi_gro_held++;
				break;
			case GRO_MERGED:
				cp->cp_stats.rx_napi_gro_merged++;
				break;
			default:
				break;
		}
#endif
	}
	else
	{
		if(netif_rx(skb) == NET_RX_DROP)
			DEVPRIV(net_device_record)->net_stats.rx_dropped++;
	}
#else
	if (netif_rx(skb) == NET_RX_DROP)
		DEVPRIV(net_device_record)->net_stats.rx_dropped++;
#endif

	return RE8670_RX_STOP_SKBNOFREE;
}

#ifdef SMBSHORTCUT
int checkLocalIPPort(u32 srcip, u16 dport){
//      printk("src 0x%x, dport %d\n", srcip, dport);
        if ( srcip == smbserverip && ( dport == 20 || dport == 21 || dport == 137 || dport == 138 || dport == 139 || dport == 445))
                return 1;
        else
                return 0;
}

int checkSMBIPPort(struct sk_buff *skb, int hook){
        struct iphdr *iph = ip_hdr(skb);
        //const struct iphdr *iph = ((struct sk_buff *)skb)->nh.iph;
        struct tcphdr *th;

        if ( !smbscflag )
                return 0;

        if (unlikely( iph->frag_off & 0x3fff )) /* Ignore fragment */
        {
                //printk(KERN_ERR "[%s][%d]\n", __FUNCTION__, __LINE__);
                return 0;
        }

        th=(struct tcphdr *) ((void *) iph + iph->ihl*4);
        if (th->syn)
                return 0;

        switch (hook) {
                case NF_INET_LOCAL_IN:
                case NF_INET_PRE_ROUTING:
                        if ( checkLocalIPPort(iph->daddr, th->dest) ){
                                //printk("Incoming serverip = %d.%d.%d.%d\n",  (iph->daddr & 0xff000000) >> 24,         (iph->daddr & 0x00ff0000) >> 16, (iph->daddr & 0x0000ff00) >> 8,        (iph->daddr & 0xff));
                                return 1;
                        }
                        break;
                case NF_INET_LOCAL_OUT:
                case NF_INET_POST_ROUTING:
                        if ( checkLocalIPPort(iph->saddr, th->source) ){
                                //printk("Outgoing serverip = %d.%d.%d.%d\n", (iph->saddr & 0xff000000) >> 24,  (iph->saddr & 0x00ff0000) >> 16, (iph->saddr & 0x0000ff00) >> 8,        (iph->saddr & 0xff));
                                return 1;
                        }
                        break;
                default:
                        return 0;
        }
        return 0;
}

static void smbSetServerIP(u32 sip){
        smbserverip = sip;
}
static void smbSetflag(u8 flag){
        smbscflag = flag;
}
#endif


#ifdef CONFIG_RTK_HOST_SPEEDUP
int re8670_start_xmit_txInfo (struct sk_buff *skb, struct net_device *dev, struct tx_info* ptxInfo, struct tx_info* ptxInfoMask);
static inline void speedtest_packet_parser(struct sk_buff *skb, speedup_pktHdr_t *pPktHdr)
{
	struct tcphdr *tcph;
	unsigned char *pData = skb->data;

	pPktHdr->flags.all = 0;
	
	pPktHdr->skb = skb;
	pPktHdr->pL2Hdr = pData;

	pData += 12;
	if (*(unsigned short *)pData == htons(0x8100))
		pData += 4;
	if (*(unsigned short *)pData == htons(0x8864))
	{
		pPktHdr->f_ispppoe = 1;
		pData += 2;
		pPktHdr->pPPPoEHdr = pData;
	}

	if ((pPktHdr->f_ispppoe && (*(unsigned short *)(pData+6) == htons(0x0021))) ||
		(*(unsigned short *)pData == htons(0x0800)))
	{
		struct iphdr *iph;
		pPktHdr->f_isipv4 = 1;
		
		if (pPktHdr->f_ispppoe)
			pPktHdr->pL3Hdr = pData + 8;
		else
			pPktHdr->pL3Hdr = pData + 2;
		
		iph = (struct iphdr *)pPktHdr->pL3Hdr;
		memcpy(pPktHdr->saddr, &(iph->saddr), IP_ADDR_LEN);
		memcpy(pPktHdr->daddr, &(iph->daddr), IP_ADDR_LEN);

		if (iph->protocol == IPPROTO_TCP)
		{
			pPktHdr->f_istcp = 1;		

			tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);
			pPktHdr->pL4Hdr = (unsigned char *)tcph;		
			pPktHdr->sport = tcph->source;
			pPktHdr->dport = tcph->dest;
			pPktHdr->seqNo = ntohl(tcph->seq);
			pPktHdr->ackNo = ntohl(tcph->ack_seq);
			pPktHdr->dataLen = ntohs(iph->tot_len) - (iph->ihl<<2) - (tcph->doff<<2);
		}
	}
	else if((pPktHdr->f_ispppoe && (*(unsigned short *)(pData+6) == htons(0x0057))) ||
		(*(unsigned short *)pData == htons(0x86dd)))
	{
		unsigned char nexthead,*p_nexthead;
		struct ipv6hdr *iph;
		int ext_opt_len=0;
		pPktHdr->f_isipv6 = 1;
		
		if (pPktHdr->f_ispppoe)
			pData += 8;
		else
			pData += 2;
		pPktHdr->pL3Hdr = pData;
		iph = (struct ipv6hdr *)pPktHdr->pL3Hdr;
		memcpy(pPktHdr->saddr, &(iph->saddr), IP6_ADDR_LEN);
		memcpy(pPktHdr->daddr, &(iph->daddr), IP6_ADDR_LEN);
		nexthead=iph->nexthdr;
		pData += 40;
		while ( (nexthead == IPPROTO_HOPOPTS) || (nexthead == IPPROTO_ROUTING) || (nexthead == IPPROTO_FRAGMENT)
			 || (nexthead == IPPROTO_DSTOPTS) || (nexthead == IPPROTO_MH))
		{
			nexthead = *pData;			
			pData += (*(pData+1)+1)<<3;
			ext_opt_len += (*(pData+1)+1)<<3;
		}
		if(nexthead == IPPROTO_TCP)
		{
			pPktHdr->f_istcp = 1;		

			tcph = (struct tcphdr*)pData;
			pPktHdr->pL4Hdr = (unsigned char *)tcph;		
			pPktHdr->sport = tcph->source;
			pPktHdr->dport = tcph->dest;
			pPktHdr->seqNo = ntohl(tcph->seq);
			pPktHdr->ackNo = ntohl(tcph->ack_seq);
			pPktHdr->dataLen = ntohs(iph->payload_len) - ext_opt_len - (tcph->doff<<2);
		}
	}
}
				
__IRAM_NIC
int re8670_host_speedup (struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo)
{
	speedup_pktHdr_t pktHdr={0};
    struct tcphdr *tcph;
#ifdef SMBSHORTCUT	
	struct udphdr *udph;
#endif	
	struct iphdr *iph;
    unsigned char *pData = skb->data;
	unsigned int pktlen=0, l3Len, l4Len;
	void *pHostTrack;
	int ret;
	int ipver=0;

    if (isHostSpeedUpEnable())
    {
    	speedtest_packet_parser(skb, &pktHdr);
		
		if ((pktHdr.f_isipv4||pktHdr.f_isipv6) && pktHdr.f_istcp)
        {
        	if(pktHdr.f_isipv6)
				ipver = 1;
        	read_lock(&speedup_lock);
            if ((pHostTrack = shouldSpeedUp(0, pktHdr.daddr, pktHdr.saddr, pktHdr.dport, pktHdr.sport, ipver)) != NULL)
            {
				tcph = (struct tcphdr*)pktHdr.pL4Hdr;
				
            	if (tcph == NULL || (tcph->syn) || (tcph->fin))
            	{
            		read_unlock(&speedup_lock);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
					return RE8670_RX_CONTINUE;
#else
					return re8670_rx_skb(cp, skb, pRxInfo);
#endif
            	}
				
				if ((0 == pktHdr.dataLen) && tcph->ack)
				{
					if (isUploadSpeedTestStream(pHostTrack))
					{
						skb->dev = decideRxDevice(cp, pRxInfo);
						updateRxStatic(cp, skb);

						ret = speedtest_upload_shortcut_enter(pHostTrack, &pktHdr);
						if (SPEEDUP_RET_MATCH_ACK == ret)
						{
							send_upload_data_now(&pktHdr);
							read_unlock(&speedup_lock);

							if (getUploadAckStat(&pktHdr) > 5)
								return RE8670_RX_STOP;
							else
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
								return RE8670_RX_CONTINUE;
#else
								return re8670_rx_skb(cp, skb, pRxInfo);
#endif
						}
						else if (SPEEDUP_RET_MATCH_NO_ACK == ret)
						{
							read_unlock(&speedup_lock);
							
							if (getUploadAckStat(&pktHdr) > 5)
								return RE8670_RX_STOP;
							else
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
								return RE8670_RX_CONTINUE;
#else
								return re8670_rx_skb(cp, skb, pRxInfo);
#endif
						}
						else
						{
							read_unlock(&speedup_lock);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
							return RE8670_RX_CONTINUE;
#else
							return re8670_rx_skb(cp, skb, pRxInfo);
#endif
						}
					}
					else
					{
						read_unlock(&speedup_lock);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
						return RE8670_RX_CONTINUE;
#else						
						return re8670_rx_skb(cp, skb, pRxInfo);
#endif
					}
				}
				
				fast_ack_calibration_enter(skb->len);

            	/* calc rx rate */
				speedtest_sample_enter(skb->len);
				speedtest_payload_statistic(pktHdr.dataLen);
				
				if (shouldBypassSpeedUP(pHostTrack))
				{
					speedtest_stream_shortcut_rx_learn(pHostTrack, &pktHdr, pktHdr.dataLen);
					read_unlock(&speedup_lock);
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
					return RE8670_RX_CONTINUE;
#else
					return re8670_rx_skb(cp, skb, pRxInfo);
#endif
				}
				
				skb->dev = decideRxDevice(cp, pRxInfo);
				updateRxStatic(cp, skb);

            	ret = speedtest_stream_shortcut_check(pHostTrack, &pktHdr, pktHdr.dataLen);
				read_unlock(&speedup_lock);
				if (SPEEDUP_RET_MATCH_ACK == ret)
        		{
        			/* ACK right now! */
					delay_ack_xmit(&pktHdr);
					
        			return RE8670_RX_STOP_SKBNOFREE;
        		}
				else if (SPEEDUP_RET_MATCH_NO_ACK == ret)
				{
					return RE8670_RX_STOP;
				}
				else if (SPEEDUP_RET_MATCH_QUEUED == ret)
				{
					return RE8670_RX_STOP_SKBNOFREE;
				}
				else
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
					return RE8670_RX_CONTINUE;
#else
					return re8670_rx_skb(cp, skb, pRxInfo);
#endif
            }
			read_unlock(&speedup_lock);
        }
    }

#ifdef SMBSHORTCUT
	if ( smbscflag  && (0x3f&(1 << pRxInfo->opts3.bit.src_port_num)) ){
                pData += 12;
		if (*(unsigned short *)pData == htons(0x8100))
                        pData += 4;

		if (((*(unsigned short *)pData == htons(0x8864)) && (*(unsigned short *)(pData+8) == htons(0x0021))) ||
			(*(unsigned short *)pData == htons(0x0800)))
                {
			if (*(unsigned short *)pData == htons(0x0800))
                                iph = (struct iphdr *)(pData + 2);
                        else
                                iph = (struct iphdr *)(pData + 10);

                        if (iph->protocol == IPPROTO_TCP){
                                tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);
                                if ( checkLocalIPPort(iph->daddr, tcph->dest) ){
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
										return RE8670_RX_CONTINUE;
#else
                                        return re8670_rx_skb(cp, skb, pRxInfo);
#endif
                                }
                        }else if (iph->protocol == IPPROTO_UDP){
                                udph = (struct udphdr*)((__u32 *)iph + iph->ihl);
                                if ( checkLocalIPPort(iph->daddr, udph->dest) ){
#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
										return RE8670_RX_CONTINUE;
#else
                                        return re8670_rx_skb(cp, skb, pRxInfo);
#endif
                                }
                        }
                }
        }
#endif

    return RE8670_RX_CONTINUE;
}
#endif


__IRAM_NIC
static void re8670_rx_software (struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo)
{
	unsigned int num = (pRxInfo->opts3.bit.src_port_num >= SW_PORT_NUM) ? (0) : pRxInfo->opts3.bit.src_port_num ;
	unsigned int gmac = cp->gmac;

#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
	if(cp->rx_save_int_prio_to_skb == GMAC_ON)
	{
		skb->internal_priority = pRxInfo->opts3.bit.internal_priority;
		RX_TRACE(gmac, cp->debug_enable, cp->debug_times, "priority=%d\n", skb->internal_priority);
	}
#endif
#ifdef CONFIG_RTL8192CD
	if(unlikely(cp->wifi_tx_qos_enable == GMAC_ON))
	{
		skb->cb[0] = cp->wifi_tx_qos_mapping[pRxInfo->opts3.bit.internal_priority];
	}
#ifdef LOCAL_SERVICE_ACCELERATE
	else
#endif
#endif
#ifdef LOCAL_SERVICE_ACCELERATE
	if(cp->local_service_acc_enable && pRxInfo->opts3.bit.internal_priority == GMAC_PRIORITY_LOCAL_SERVICE_ACC)
	{
		SOCK_MARK2_LOCAL_SERVICE_ACC_SET(skb->mark2, 1ULL);
	}
#endif

	RX_TRACE(gmac, cp->debug_enable, cp->debug_times, "port2rxfunc=0x%p, internal_priority=%d, skb->cb[0]=%d\n", cp->port2rxfunc[num], pRxInfo->opts3.bit.internal_priority, skb->cb[0]);
	cp->port2rxfunc[num](cp, skb, pRxInfo);
}


static 
__IRAM_NIC
unsigned int re8670_rx_csum_ok (struct rx_info *rxInfo)
{
#if 0
	unsigned int protocol = rxInfo->opts1.bit.pkttype;

	switch(protocol){
		case RxProtoTCP:
		case RxProtoUDP:
		case RxProtoICMP:
		case RxProtoIGMP:/*we check both l4cs and ipv4cs when protocol is ipv4 l4*/
			if(likely((!rxInfo->opts1.bit.l4csf) && (!rxInfo->opts1.bit.ipv4csf))){
				return 1;
			}
			break;
		case RxProtoTCPv6:
		case RxProtoUDPv6:
		case RxProtoICMPv6:/*when protocol is ipv6, we only check l4cs*/
			if(likely(!rxInfo->opts1.bit.l4csf)){
				return 1;
			}
			break;
		default:/*no guarantee when the protocol is only ipv4*/
			break;
	}
#endif
	return 0;
}

// Kaohj -- for polling mode
int eth_poll=0; // poll mode flag
void eth_poll_schedule(void)
{
	tasklet_hi_schedule(eth_rx_tasklets);
}

static inline void retriveRxInfo(DMA_RX_DESC *desc, struct rx_info *pRxInfo){
	pRxInfo->opts1.dw = desc->opts1;
	pRxInfo->addr= desc->addr;
	pRxInfo->opts2.dw = desc->opts2;
	pRxInfo->opts3.dw = desc->opts3;
}

static inline void updateGmacFlowControl(unsigned int gmac,unsigned rx_tail,int ring_num)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];	
	unsigned int new_cpu_desc_num;		

	if(cp->re8670_rx_flow_control_status[ring_num] != GMAC_TRUE)
		return;
	
	if(ring_num==0)
	{    
		new_cpu_desc_num = RLE0787_R32(gmac, EthrntRxCPU_Des_Num);
		new_cpu_desc_num &= 0x00FFFF0F; // clear
		new_cpu_desc_num |= (((rx_tail&0xFF)<<24)|(((rx_tail>>8)&0xF)<<4)); // update
		RLE0787_R32(gmac, EthrntRxCPU_Des_Num) = new_cpu_desc_num;
	}
	else
	{
		new_cpu_desc_num = RLE0787_R32(gmac, EthrntRxCPU_Des_Num2+ADDR_OFFSET*(ring_num-1));
		new_cpu_desc_num &= 0xfffff000; // clear
		new_cpu_desc_num |= (((rx_tail&0xFF)|(((rx_tail>>8)&0xF)<<4))); // update
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+ADDR_OFFSET*(ring_num-1), new_cpu_desc_num);
	}
}

void vlan_detag(unsigned int gmac, int onoff){
	switch(onoff){
		case GMAC_ON:
			RLE0787_W8(gmac, CMD, RLE0787_R8(gmac, CMD)|RxVLAN_Detag);
			break;
		case GMAC_OFF:
			RLE0787_W8(gmac, CMD, RLE0787_R8(gmac, CMD)&~RxVLAN_Detag);
			break;
		default:
			printk("%s %d: wrong arg %d\n", __func__, __LINE__, onoff);
			break;
	}
}

void SetTxRingPrioInRR(unsigned int gmac)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];	
	
	cp->iocmd1_reg |= TX_RR_scheduler;
	RLE0787_W32(gmac, IO_CMD1, cp->iocmd1_reg);
}

static inline unsigned char getRxRingBitMap(struct re_private *cp)
{
//	unsigned int gmac = cp->gmac;
	unsigned char result = (unsigned char)((cp->isr1_status)|(RX_RDU_CHECK(cp->isr_status))|((cp->isr_status&SW_INT)?cp->rx_multiring_bitmap:0));
		
	cp->isr_status=0;	
	cp->isr1_status=0;
	return result;
}

#ifdef CONFIG_RTL8686_SWITCH
void re8686_set_pauseBySw(unsigned char enable)
{
	re_private_data_root.rx_pause_by_software_enable = (enable) ? GMAC_TRUE : GMAC_FALSE;
	return;
}

void setSoftwareInterrupt(void)
{
#if defined (CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	struct softnet_data *sd;
	unsigned long flag;
	unsigned int qlen;

	sd = &per_cpu(softnet_data, smp_processor_id());
	local_irq_save(flag);
	spin_lock(&sd->input_pkt_queue.lock);

	qlen = skb_queue_len(&sd->input_pkt_queue);
#endif
	//linux input queue is empty and skb is enough=>trigger NIC to receive packet 
	if (
#ifdef CONFIG_RTL865X_ETH_PRIV_SKB 
		(eth_skb_alloc_num < RE8670_MAX_ALLOC_RXSKB_NUM)
#else
		GMAC_TRUE
#endif
#if defined (CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)		
		&&(qlen <= netdev_max_backlog)
#endif		
		)
	{
		//Gmac 0
		if(re_private_data_root.rx_pause_by_software_bitmap & (1<<0))
			RLE0787_W32(0, SWINT_REG, 1<<24);
		//Gmac 1
		if(re_private_data_root.rx_pause_by_software_bitmap & (1<<1))
			RLE0787_W32(1, SWINT_REG, 1<<24);
	}	
	else
	{
		mod_timer(&re_private_data_root.rx_pause_by_software_interrupt_timer, jiffies+1);
	}
#if defined (CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)		
	spin_unlock(&sd->input_pkt_queue.lock);
	local_irq_restore(flag);
#endif	
	return ;
}
#endif

#ifdef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
/*
 * RETURN VAL: 	1 fast forwarding successfully
 *                    	0 should go normal path
 */
static int re8670_ff_enter(struct re_private *cp, struct sk_buff *skb, struct rx_info *pRxInfo)
{
	extern void reinit_skbhdr(struct sk_buff *skb, 
						void (*prealloc_cb)(struct sk_buff *, unsigned));
	extern int rteFastForwarding(struct sk_buff *skb);

	skb->dev = decideRxDevice(cp, pRxInfo);
	if ( !ether_addr_equal(skb->data, skb->dev->dev_addr))
	{
		int dir=DIR_LAN;
		
		if (skb->dev->priv_flags & IFF_DOMAIN_WAN) // wan
			dir = DIR_WAN;
		
		if (brgFastForwarding((struct sk_buff *)skb, dir))
		{
			return 1;
		}
	}

	skb_reset_mac_header(skb);
	skb->protocol = ((unsigned short *)(skb->data))[6];
		
	skb_pull(skb, ETH_HLEN);
	skb->dst = NULL;
	
	if (NET_RX_SUCCESS == rteFastForwarding(skb))
	{
		return 1;
	}
	skb_push(skb, ETH_HLEN);

	reinit_skbhdr(skb, rtl865x_free_eth_priv_buf);
	
	return 0;
}
#endif //end of CONFIG_RTL865X_ETH_PRIV_SKB_ADV

#if defined (CONFIG_RTK_VOIP_QOS)
extern int ( *check_voip_channel_loading )( void );
#endif

/*
	we can use this map to decide what sequence we want, that means, queue's priority.
	also, we can use this to decrease some iterations when we split rx interrupt. 
	if u only open rx ring1 and ring6, u can set pri_map = {0,5,1,2,3,4}, and 
	rx_ring_bitmap_current will only has these two bits as well, so we don't need to run "for"
	6 times.
	*/
#ifdef RX_MRING_INT_SPLIT
	unsigned char pri_map[MAX_GMAC_NUM][MAX_RXRING_NUM] = {{5,0,1,2,3,4}, {5,0,1,2,3,4}, {5,0,1,2,3,4}};
	unsigned char higherq[MAX_GMAC_NUM][MAX_RXRING_NUM] = {{0x3e,0x3c,0x38,0x30,0x20,0x00}, {0x3e,0x3c,0x38,0x30,0x20,0x00}, {0x3e,0x3c,0x38,0x30,0x20,0x00}};
#endif
static int loop_wdt=0;

__IRAM_NIC
#ifdef RX_NAPI_MODE
static unsigned int re8670_rx(struct re_private *cp)
#else
static void re8670_rx(struct re_private *cp)
#endif
{
#if defined(CONFIG_RX_WIFI_FF_AGG)
	#if (AGGQ_PKT_AGE==1)
	u32 pkt_age = 1;
	#endif /* AGGQ_PKT_AGE */
#endif /* CONFIG_RX_WIFI_FF_AGG */

	unsigned int gmac = cp->gmac;
	int rx_work;
	int ring_num=0;
	unsigned rx_Mtail;
	unsigned long expiry = jiffies + (2 * HZ);

#ifdef RX_NAPI_MODE
	unsigned int rx_packets_cnt = 0;
#endif

#if defined (CONFIG_RTK_VOIP_QOS)
	unsigned long start_time = jiffies;
	int pkt_rcv_cnt = 0;
#endif
#ifdef CONFIG_RTK_HOST_SPEEDUP
	unsigned int unAckSegmentExist = 0;
#endif

#ifdef RX_MRING_INT_SPLIT
	static unsigned char rx_ring_backup = 0;
	unsigned char rx_ring_bitmap_current;
	int i = 0, max_rx_ring_num;
#endif

	GMAC_SPIN_LOCK(&cp->rx_lock);

	//protect eth rx while reboot
	if(unlikely(cp->eth_close == GMAC_TRUE)){
#ifndef RX_NAPI_MODE
		unmask_rx_int(gmac);
#endif
		GMAC_SPIN_UNLOCK(&cp->rx_lock);
#ifdef RX_NAPI_MODE
		return rx_packets_cnt;
#else
		return;
#endif
	}
	
#ifdef RX_MRING_INT_SPLIT
	rx_ring_bitmap_current=getRxRingBitMap(cp);	
	rx_ring_bitmap_current|=rx_ring_backup;
	rx_ring_backup=0;
	max_rx_ring_num = cp->rx_not_only_ring1?MAX_RXRING_NUM:1;
	for(i=0 ; rx_ring_bitmap_current && i<max_rx_ring_num ; i++){
		if(cp->rx_not_only_ring1) {
			ring_num = pri_map[gmac][i];
			if(NOT_IN_BITMAP(rx_ring_bitmap_current, ring_num)){
				continue;
			}
			rx_ring_bitmap_current&=~(1<<ring_num);
			if(RX_HQBUFFS_EMPTY(cp, ring_num))
			{
				CLEAR_ISR1(gmac, (1<<ring_num));
			}
		}
#endif		
		rx_Mtail = cp->rx_Mtail[ring_num];

		// Kaohj
		if (unlikely(eth_poll)) // in poll mode
			rx_work = 2; // rx rate is rx_work * 100 pps (timer tick is 10ms)
		else
			rx_work = cp->re8670_rx_ring_size[ring_num];
#if defined(CONFIG_RTK_VOIP_QOS)
		start_time = jiffies;
		pkt_rcv_cnt = 0;
#endif

		while (rx_work--)
		{
			u32 len;
			struct sk_buff *skb, *new_skb=NULL;
#ifdef CONFIG_RG_JUMBO_FRAME
			struct sk_buff *orig_skb;
#endif
			DMA_RX_DESC *desc;
			unsigned buflen;
			struct rx_info rxInfo;
			loop_wdt++;	
			if (loop_wdt > 1024) {
#ifdef CONFIG_LUNA_WDT_KTHREAD
				extern void luna_watchdog_kick(void);
				luna_watchdog_kick();
				loop_wdt = 0;
#endif 
			}

			if (unlikely(time_is_before_jiffies(expiry))) {
				printk_once("nic-rx: rx time exceed %lu/%lu\n",expiry, jiffies);
				break;
			}

#ifdef CONFIG_RTL8686_SWITCH
			if(re_private_data_root.rx_pause_by_software_enable)
			{
#if defined (CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)			
				struct softnet_data *sd;
				unsigned long flag;
				unsigned int qlen;
				//The packet will be dropped by Linux PS when it is sent to PS and PS input queue is full, so pause receiving packet to SW.
			
				sd = &per_cpu(softnet_data, smp_processor_id());
				local_irq_save(flag);
				spin_lock(&sd->input_pkt_queue.lock);

				qlen = skb_queue_len(&sd->input_pkt_queue);
				
				if (qlen > netdev_max_backlog)
				{
					cp->pauseBySwRingBitmap |= (1<<ring_num);
					re_private_data_root.rx_pause_by_software_bitmap |= (1<<gmac);
					if(!timer_pending(&re_private_data_root.rx_pause_by_software_interrupt_timer))
						mod_timer(&re_private_data_root.rx_pause_by_software_interrupt_timer, jiffies+1);
					spin_unlock(&sd->input_pkt_queue.lock);
					local_irq_restore(flag);
					break;
				}
				spin_unlock(&sd->input_pkt_queue.lock);
				local_irq_restore(flag);
#endif			
				cp->pauseBySwRingBitmap &= ~(1<<ring_num);
				re_private_data_root.rx_pause_by_software_bitmap &= ~(1<<gmac);
			}
#endif
#if defined(CONFIG_RTK_VOIP_QOS)
			if( (pkt_rcv_cnt++ > 100 || (jiffies - start_time) >= 1)&& check_voip_channel_loading && check_voip_channel_loading() > 0)
			{
				break; 
			}
#endif

#ifdef RX_MRING_INT_SPLIT
#ifdef CONFIG_RTK_HOST_SPEEDUP
			if (!isHostSpeedUpEnable())
			{
#endif
				/* if higher queue has packet, quit and restart */
				if(cp->rx_not_only_ring1) {					
					if (retrive_isr1_status(gmac)&higherq[gmac][ring_num]) {
						rx_ring_backup=(1<<ring_num)|rx_ring_bitmap_current; /* restore */
						rx_ring_bitmap_current|=rx_ring_backup;
						//i = MAX_RXRING_NUM;
						break;				
					}
				}
#ifdef CONFIG_RTK_HOST_SPEEDUP
			}
#endif
#endif 
			desc = &cp->rx_Mring[ring_num][rx_Mtail];	
			retriveRxInfo(desc, &rxInfo);

			if (rxInfo.opts1.bit.own)
			{
				break;
			}

			mips_perf_measure_entrance(MIPS_PERF_NIC_RX);

            cp->cp_stats.rx_hw_num++;
            skb = cp->rx_skb[ring_num][rx_Mtail].skb;
            if (unlikely(!skb))
            	BUG();

			RXINFO_DBG(cp, &rxInfo, ring_num);
			
			len = rxInfo.opts1.bit.data_length & 0x0fff;		//minus CRC 4 bytes later
	
			if (unlikely(rxInfo.opts1.bit.rcdf)){		//DMA error
#ifdef CONFIG_RG_JUMBO_FRAME
				if(cp->jumboLength>0)		//flush jumbo skb
				{
					if(cp->jumboFrame) 
						dev_kfree_skb_any(cp->jumboFrame);
					cp->jumboLength=0;
					cp->jumboFrame=NULL;
				}
#endif
				cp->cp_stats.rcdf++;
				goto rx_next;
			}
			if (unlikely(rxInfo.opts1.bit.crcerr)){		//CRC error
#ifdef CONFIG_RG_JUMBO_FRAME
				if(cp->jumboLength>0)		//flush jumbo skb
				{
					if(cp->jumboFrame) 
						dev_kfree_skb_any(cp->jumboFrame);
					cp->jumboLength=0;
					cp->jumboFrame=NULL;
				}
#endif
				cp->cp_stats.crcerr++;
				goto rx_next;
			}

			buflen = cp->rx_buf_sz + RX_OFFSET;
#ifdef CONFIG_RG_JUMBO_FRAME
			if(rxInfo.opts1.bit.fs==1)	//FS=1
			{
				if(rxInfo.opts1.bit.ls==1) ////FS=1, LS=1
				{
					orig_skb=NULL;
					goto rx_to_software;
				}
				else //FS=1, LS=0
				{				
					orig_skb=skb;
					//printk("FS=1, LS=0, first frag skb is %d\n",len);
					//memDump(skb->data+RX_OFFSET,64,"skb_start_64", RTL8686_MEM_DUMP_FORMAT_ALL);
					//memDump(skb->data+RX_OFFSET+len-16,16,"skb_last_16", RTL8686_MEM_DUMP_FORMAT_ALL);

					if(cp->jumboLength>0)		//flush jumbo skb
					{
						if(cp->jumboFrame) 
							dev_kfree_skb_any(cp->jumboFrame);
						cp->jumboLength=0;
						cp->jumboFrame=NULL;
					}
					
					//Init global variables, copy the skb into jumboFrame and free the frag one
					cp->jumboFrame = dev_alloc_skb(JUMBO_SKB_BUF_SIZE);
					if (unlikely(!cp->jumboFrame)) {
						printk("there is no memory for jumbo frame\n");
						cp->cp_stats.rx_no_mem++;
						dma_cache_wback_inv((unsigned long)skb->data, SKB_BUF_SIZE);
						cp->jumboLength=0;
						goto rx_next;
					}
					cp->jumboLength=len;
					
					//Copy skb into jumboFrame
					memcpy(cp->jumboFrame->data+RX_OFFSET,skb->data+RX_OFFSET,len);
				}

			}
			else 	//FS=0
			{
				orig_skb=skb; //keep
				//if the length is zero, JumboFrame is not allocated...
				if(unlikely(cp->jumboLength==0))
				{
					printk("the first skb is not entered or error...\n");
					cp->cp_stats.frag++;
				}
				else
				{
					if(unlikely(cp->jumboLength+len>=JUMBO_SKB_BUF_SIZE))
					{
						if(cp->jumboFrame) 
							dev_kfree_skb_any(cp->jumboFrame);
						cp->jumboLength=0;
						cp->jumboFrame=NULL;
						cp->cp_stats.toobig++;
					}
					else
					{
						//Copy into jumboFrame, free the skb						
						memcpy(cp->jumboFrame->data+RX_OFFSET+cp->jumboLength,skb->data,len);
						cp->jumboLength+=len;
						
						if(rxInfo.opts1.bit.ls==1)		//FS=0, LS=1
						{
							//printk("FS=0, LS=1, last frag skb is %d\n",len);
							//memDump(skb->data,64,"skb_start_64", RTL8686_MEM_DUMP_FORMAT_ALL);
							//memDump(skb->data+len-16,16,"skb_last_16", RTL8686_MEM_DUMP_FORMAT_ALL);
							//dev_kfree_skb_any(skb);							
							skb=cp->jumboFrame;
							len=cp->jumboLength;							
							rxInfo.opts1.bit.data_length=cp->jumboLength & 0x3fff;		//at most 14 bits
							cp->jumboLength=0;
							cp->jumboFrame=NULL;
							rxInfo.opts1.bit.fs=1;
							dma_cache_wback_inv((unsigned long)skb->data,len);
							goto rx_to_software;
						}
						else
						{
							//printk("FS=0, LS=0 frag skb is %d\n",len);
							//memDump(skb->data,64,"skb_start_64", RTL8686_MEM_DUMP_FORMAT_ALL);
							//memDump(skb->data+len-16,16,"skb_last_16", RTL8686_MEM_DUMP_FORMAT_ALL);							
						}
						dma_cache_wback_inv((unsigned long)skb->data,SKB_BUF_SIZE);
					}
				}
			}
			goto rx_next;
rx_to_software:
#else
			if (unlikely((rxInfo.opts1.dw & (FirstFrag | LastFrag)) != (FirstFrag | LastFrag))) {
				cp->cp_stats.frag++;
				goto rx_next;
			}
#endif

			if(rxInfo.opts1.bit.fs==1)
			{
				skb_reserve(skb, RX_OFFSET); // HW DMA start at 4N+2 only in FS.
			}
#if defined(CONFIG_RX_WIFI_FF_AGG)
			//aggQueue mechanism
			if(rxInfo.opts2.bit.reason==64 &&
				hweight16(rxInfo.opts3.bit.fb_hash_or_dst_portmsk)==1 &&
				aggQueueRxFunc){
				unsigned char *pData=skb->data;
				unsigned short ethType=ntohs(*(unsigned short *)(pData+(ETH_ALEN<<1)));
				rtk_nic_wifiAggQueue_t *aggQ=NULL;
				unsigned long aggQ_flags;
				int padding = 0, hash = 0, pktlen = 0;
				u16 wmm_val = 0;

			#if (DBG_AGGQ_FORCE_DONT_AGG==1)
				goto OUT_AGG_QUEUE;
			#endif

			#if (AGGQ_DISABLE_2G_AGG==1) && (AGGQ_WIFI5G_GMAC_NUM==2)
				/* don't AGG for 2G wifi */
				if (gmac==2)
					goto OUT_AGG_QUEUE;
			#endif /* AGGQ_DISABLE_2G_AGG && AGGQ_WIFI5G_GMAC_NUM */

			#if (AGGQ_5G_MERGE_QUE==1)
				#if (AGGQ_WIFI5G_GMAC_NUM==2)
				if (gmac!=2)
					gmac=0;
				#elif (AGGQ_WIFI5G_GMAC_NUM==3)
					gmac=0;
				#else
					#error "not handle!!"
				#endif
			#endif /* AGGQ_5G_MERGE_QUE */

			#if (AGGQ_PKT_AGE==1)
				if (pkt_age) {
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
					pkt_age = 0;
				}
			#endif /* AGGQ_PKT_AGE */

				if(ethType!=0x0800 && ethType!=0x86dd){
					//printk("the ethType is 0x%x...go out!!\n",ethType);
					goto OUT_AGG_QUEUE;
				}

			#if (AGGQ_HASH_DSCP==1)
				if (ethType==0x0800) {
					wmm_val = tos_to_up(*(skb->data + ETH_HLEN + 1));
				}
			#endif /* AGGQ_HASH_DSCP */
				hash = mac_hash(skb->data, wmm_val);
				aggQ = &aggQueue[gmac][hash];
				pktlen = len - CRC_LEN;

				_txsc_spinlock_bh(&aggQ->txsc_amsdu_lock);
				if (aggQ->valid) {
					/* same da */
					if (
					#if (AGGQ_HASH_DSCP==1)
						aggQ->skb->cb[_SKB_CB_PRIORITY] == wmm_val &&
					#endif /* AGGQ_HASH_DSCP */
						comp_macaddr(skb->data, aggQ->DA)
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
					} else {
						//AGGQ_ERR("hash collision..\n");
						_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
						goto OUT_AGG_QUEUE;
					}
				} else {
				#if (AGGQ_SET_AGGQ_RULE==1)
					unsigned long flags;
					aggQ_rule_t *aggR = &aggRule[gmac][hash];
					read_lock_irqsave(&aggR_lock, flags);
					if (
					#if (AGGQ_HASH_DSCP==1)
						aggR->wmm == wmm_val &&
					#endif /* AGGQ_HASH_DSCP */
						comp_macaddr(skb->data, aggR->DA)) {
						if ((aggQ->num_max != aggR->num_max) ||
							(aggQ->size_max != aggR->size_max)) {
							aggQ->num_max = aggR->num_max;
							aggQ->size_max = aggR->size_max;
						}
					}
					read_unlock_irqrestore(&aggR_lock, flags);

					if (aggQ->num_max<2) {
						_txsc_spinunlock_bh(&aggQ->txsc_amsdu_lock);
						goto OUT_AGG_QUEUE;
					}
				#endif /* AGGQ_SET_AGGQ_RULE */

				#if 1//(AGGQ_HASH_DSCP==1)
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
OUT_AGG_QUEUE:
			new_skb = NULL;
		#if (AGGQ_5G_MERGE_QUE==1)
			gmac = cp->gmac;
		#endif /* AGGQ_5G_MERGE_QUE */
#endif /* CONFIG_RX_WIFI_FF_AGG */

#ifdef CONFIG_RG_JUMBO_FRAME
			if(orig_skb==NULL) {
				if(rxInfo.opts3.bit.internal_priority!=GMAC_PRIORITY_CRITICAL) {
					new_skb=re8670_getAlloc(SKB_BUF_SIZE);
				} else {
					cp->cp_stats.rx_critical_num++;
					new_skb=re8670_getCriticalAlloc(SKB_BUF_SIZE);
					if (!new_skb) {
						cp->cp_stats.rx_critical_drop_num++;
					}
				}
			}
			else
				new_skb=orig_skb;
#else				
			if(rxInfo.opts3.bit.internal_priority!=GMAC_PRIORITY_CRITICAL) {
				new_skb=re8670_getAlloc(SKB_BUF_SIZE);
			} else {
				cp->cp_stats.rx_critical_num++;
				new_skb=re8670_getCriticalAlloc(SKB_BUF_SIZE);
				if (!new_skb) {
					cp->cp_stats.rx_critical_drop_num++;
				}
			}
#endif
			if (unlikely(!new_skb)) {
#if defined(CONFIG_RX_WIFI_FF_AGG)
AGGQ_NOMEM:
#endif /* CONFIG_RX_WIFI_FF_AGG */
				if(rxInfo.opts1.bit.fs==1)
				{
					skb_reserve(skb, -RX_OFFSET); // HW DMA start at 4N+2 only in FS.
				}
				cp->cp_stats.rx_no_mem++;
				//printk("nic rx new_skb alloc failed no mem %d\n",eth_skb_alloc_num);
				//dma_cache_wback_inv((unsigned long)skb->data,buflen);
				dma_cache_wback_inv((unsigned long)skb->data,SKB_BUF_SIZE);
				//goto rx_next;
				//The packet will be dropped when run out of skb, so pause receiving packet to SW.
#ifdef CONFIG_RTL8686_SWITCH
				if(re_private_data_root.rx_pause_by_software_enable)
				{		
					cp->pauseBySwRingBitmap |= (1<<ring_num);
					re_private_data_root.rx_pause_by_software_bitmap |= (1<<gmac);
					if(!timer_pending(&re_private_data_root.rx_pause_by_software_interrupt_timer))
						mod_timer(&re_private_data_root.rx_pause_by_software_interrupt_timer, jiffies+1);
					break;
				}
				else
#endif
				{
					goto rx_next;
				}
			}

			/* Handle checksum offloading for incoming packets. */
			if (re8670_rx_csum_ok(&rxInfo))
				skb->ip_summed = CHECKSUM_UNNECESSARY;
			else
				skb->ip_summed = CHECKSUM_NONE;

			len-=4;	//minus CRC 4 bytes here			
			skb_put(skb, len);
			
			SKB_DBG(skb, cp->debug_enable, cp->debug_times, RTL8686_SKB_RX);
			RX_TRACE(gmac, cp->debug_enable, cp->debug_times, "SKB[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d\n",(u32)skb&0xffff
			,skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5]
			,skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11]			
			,(skb->data[12]<<8)|skb->data[13],len);
			RXDBG_TIMES_UPDATE(cp);

			cp->rx_Mring[ring_num][rx_Mtail].addr = CPHYSADDR(new_skb->data);
			cp->rx_skb[ring_num][rx_Mtail].skb = new_skb;
			
			dma_cache_inv((unsigned long)new_skb->data,(u32)new_skb->end-(u32)new_skb->data);
			//dma_cache_inv((unsigned long)skb->data,skb->len);

			/*fastpath enter here.*/
#ifndef CONFIG_RTK_L34_ENABLE
	#ifdef CONFIG_RTL865X_ETH_PRIV_SKB_ADV
			if (re8670_ff_enter(cp, skb, &rxInfo))
			{
				goto rx_next;
			}
			//printk("go normal path\n");
	#endif
#endif//end of CONFIG_RTK_L34_ENABLE
			re8670_rx_software(cp, skb, &rxInfo);
rx_next:
			desc->opts1 = (DescOwn | cp->rx_buf_sz) | ((rx_Mtail == (cp->re8670_rx_ring_size[ring_num] - 1))?RingEnd:0);
			updateGmacFlowControl(gmac, rx_Mtail, ring_num);
			rx_Mtail = NEXT_RX(rx_Mtail, cp->re8670_rx_ring_size[ring_num]);
#ifdef RX_NAPI_MODE
			if(++rx_packets_cnt >= cp->napi_budget)
			{
				cp->rx_Mtail[ring_num] = rx_Mtail;
				GMAC_SPIN_UNLOCK(&cp->rx_lock);
				return rx_packets_cnt;
			}
#endif
			mips_perf_measure_exit(MIPS_PERF_NIC_RX);
		}
		cp->rx_Mtail[ring_num] = rx_Mtail;

#ifndef RX_NAPI_MODE
#ifdef CONFIG_RTK_HOST_SPEEDUP
		unAckSegmentExist = 0;
		if (isDownloadHostSpeedUpEnable())
		{
			/* if unacked segment exist, we should always enable Rx tasklet to waiting for new segment or send ack
			 * when timer expired.
			 */
			unAckSegmentExist = send_delay_ack_timer();
		}
		if ((rx_work <= 0) || unAckSegmentExist)
		{
			tasklet_hi_schedule(&cp->rx_tasklets);
		}
#else
		if(rx_work <= 0)
		{
			tasklet_hi_schedule(&cp->rx_tasklets);
		}
#endif
#endif

#ifdef RX_MRING_INT_SPLIT
	}
#endif
#ifndef RX_NAPI_MODE
	unmask_rx_int(gmac);
#endif
	GMAC_SPIN_UNLOCK(&cp->rx_lock); 
#ifdef RX_NAPI_MODE
	return rx_packets_cnt;
#endif
}

#ifdef RX_NAPI_MODE
#ifdef RX_NAPI_MODE_DEBUG
#define MAX_NAPI_STATISTIC_NUM	2000
unsigned int napi_statistic[MAX_NAPI_STATISTIC_NUM] = {0};
#endif

unsigned int re8670_rx_napi_gmac_index_get(struct napi_struct *napi)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp;
	int i;

	for(i=0 ; i<MAX_GMAC_NUM ; i++)
	{
		cp = root_cp->re_private_data_ptr[i];
		if(cp->gmac_enabled != GMAC_TRUE)
			continue;
		
		if(&cp->napi == napi)
			return i;
	}
	return NULL;
}

static int re8670_poll(struct napi_struct *napi, int budget)
{
	struct re_private_root *root_cp = &re_private_data_root;	
	struct re_private *cp;
	unsigned int gmac;
	int total_received_pkts = 0;
	u16 status;

	gmac = re8670_rx_napi_gmac_index_get(napi);
	
	cp = root_cp->re_private_data_ptr[gmac];
	total_received_pkts = re8670_rx(cp);
#ifdef RX_NAPI_MODE_DEBUG
	napi_statistic[total_received_pkts]++;
#endif
	if(total_received_pkts < budget) 
	{
		napi_complete_done(napi, total_received_pkts);
		/* do not diable interrupt if we got bad packet */
		unmask_rx_int(gmac);
		cp->cp_stats.rx_napi_interrupt++;
		return 0;
	}
	else
	{
		cp->cp_stats.rx_napi_poll++;
		status = read_isr_status(cp);
	}

	return total_received_pkts;
}
#endif

__IRAM_NIC
static irqreturn_t re8670_interrupt(int irq, void *re_private_ptr, struct pt_regs *regs)
{
	struct re_private *cp = re_private_ptr;
	unsigned int gmac = cp->gmac;
	u32 status = read_isr_status(cp);
	
	if (!status)  
	{
		//printk("%s: no status indicated in interrupt, weird!\n", __func__);	//shlee 2.6
		return IRQ_RETVAL(IRQ_NONE);
	}
	
	if (status & RX_ALL(gmac)) 
	{
		if(status & RER_RUNT)
		{
			cp->cp_stats.rer_runt++;
		}
		if(status & RER_OVF)
		{
			cp->cp_stats.rer_ovf++;
		}
		if(status & RDU_ALL)
		{
			mask_rx_int(gmac);
			cp->cp_stats.rdu++;
#ifdef RX_NAPI_MODE
			napi_schedule(&cp->napi);
#else
			tasklet_hi_schedule(&cp->rx_tasklets);
#endif
		}
		if(status & (RX_OK | SW_INT))
		{
			mask_rx_int(gmac);
#ifdef RX_NAPI_MODE
			napi_schedule(&cp->napi);
#else
			tasklet_hi_schedule(&cp->rx_tasklets);
#endif
		}
	}
	
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
	if(status & TX_ALL(gmac))
	{
		mask_tx_int(gmac);
		///--printk("TX OK...recycle\n");
		cp->cp_stats.tok++;
		tasklet_hi_schedule(&cp->tx_tasklets);
	}	
#endif
	clear_isr(cp, status);

	return IRQ_RETVAL(IRQ_HANDLED);
}

static inline void updateTxStatic(struct sk_buff *skb){
	DEVPRIV(skb->dev)->net_stats.tx_packets++;
	DEVPRIV(skb->dev)->net_stats.tx_bytes += skb->len;
}

#ifdef TX_RECYCLE_SKB_USING_TOK_INT
atomic_t re8670_tx_lock=ATOMIC_INIT(0);
#endif

void nic_tx_ring_dump(unsigned int gmac);
void nic_rx_ring_dump(unsigned int gmac);

__IRAM_NIC void re8670_tx(struct re_private *cp, int ring_num, int from_hw)
{
	unsigned tx_tail = cp->tx_Mhqtail[ring_num];
	u32 status;
	struct sk_buff *skb;

	if(unlikely(cp->eth_close == GMAC_TRUE))
		return;
	
	while (!((status = (cp->tx_Mhqring[ring_num][tx_tail].opts1))& DescOwn)) {
		if (tx_tail == cp->tx_Mhqhead[ring_num])
			break;

		skb = cp->tx_skb[ring_num][tx_tail].skb;

#ifdef CONFIG_REALTEK_HW_LSO
		//printk("skb=%x tx_tail=%d\n",(u32)skb,tx_tail);
		while((u32)skb==0xffffffff) //skb is last_frag in mfrag_skb.
		{
			cp->tx_skb[ring_num][tx_tail].skb=NULL;
			tx_tail = NEXT_TX(tx_tail,cp->re8670_tx_ring_size[ring_num]); //tysu: this skb is many frags skb, just free once.
			skb = cp->tx_skb[ring_num][tx_tail].skb;
			//printk("skb=%x tx_tail=%d\n",(u32)skb,tx_tail);			
			status = cp->tx_Mhqring[ring_num][tx_tail].opts1;			
			if(status & DescOwn) break;
		}

		if(status & DescOwn) break;
		if(skb==NULL) break;
#else
		if (unlikely(!skb))   
			break;
#endif
		updateTxStatic(skb);	

		dev_kfree_skb_any(skb);
#if defined(TX_RECYCLE_SKB_USING_TOK_INT) || defined(TX_RECYCLE_SKB_USING_POLLING)
		if(from_hw) {
			cp->cp_stats.tok_free_skb++;
		}
#endif
		cp->tx_skb[ring_num][tx_tail].skb = NULL;		
		
		tx_tail = NEXT_TX(tx_tail, cp->re8670_tx_ring_size[ring_num]);
	}
	cp->tx_Mhqtail[ring_num]=tx_tail;

	if (netif_queue_stopped(ROOTDEV) && (TX_HQBUFFS_AVAIL(cp, ring_num) > (MAX_SKB_FRAGS + 1)))
		netif_wake_queue(ROOTDEV);
}

#ifdef TX_RECYCLE_SKB_USING_TOK_INT
__IRAM_NIC void re8670_tx_all(struct re_private *cp)
{
	GMAC_SPIN_LOCK(&cp->tx_lock);
	re8670_tx(cp, 0, 1);
	unmask_tx_int(cp->gmac);
	GMAC_SPIN_UNLOCK(&cp->tx_lock);
}
#elif defined(TX_RECYCLE_SKB_USING_POLLING)
__IRAM_NIC void re8670_tx_all(struct re_private *cp)
{
	if(cp->gmac_enabled == GMAC_FALSE)
	{
		goto tx_all_exit;
	}

	if(cp->eth_close == GMAC_TRUE)
	{
		goto tx_all_exit;
	}
	
	GMAC_SPIN_LOCK(&cp->tx_lock);	
	re8670_tx(cp, 0, 1);
#ifdef TX_CREATE_TEST_PACKET_DEBUG
	if(!cp->test_packet_start)
#endif
	{
	kick_tx(cp->gmac, 0);
	}	
	GMAC_SPIN_UNLOCK(&cp->tx_lock);

tx_all_exit:
	mod_timer(&cp->tok_polling_timer, jiffies+TX_RECYCLE_SKB_POLLING_SECONDS);
	return;
}
#endif

__IRAM_NIC void checkTXDesc(int ring_num, unsigned int gmac)
{
#if !defined(TX_RECYCLE_SKB_USING_TOK_INT) && !defined(TX_RECYCLE_SKB_USING_POLLING)
	struct re_private_root *root_cp = &re_private_data_root;	
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	
	if (likely(!atomic_read(&lock_tx_tail)) && cp!=NULL) {
		atomic_set(&lock_tx_tail, 1);
        re8670_tx(cp, ring_num, 0);
		atomic_set(&lock_tx_tail, 0);
	}
#else
	return;
#endif
}

#ifdef CONFIG_REALTEK_HW_LSO
void re8670_mFrag_xmit(struct sk_buff *skb, struct re_private *cp, unsigned *entry, struct tx_info* ptxInfo, int ring_num)
{
	struct re_private_root *root_cp = &re_private_data_root;
	int addi_l2_len = 0;
	unsigned first_entry;
	u32 first_len;
	struct sk_buff *new_skb;
	unsigned int gmac = cp->gmac;
	struct tx_info* first_txd;
	struct tx_info* txd;
	int frag;
#ifdef HW_LSO_ENABLE
	int mtu;
#endif

	struct ethhdr *eth = eth_hdr(skb);
	if (eth && (eth->h_proto == htons(0x8100))) {
		addi_l2_len += VLAN_HLEN;
	}

	if (skb->protocol == htons(ETH_P_PPP_SES)) {
		addi_l2_len += 8;
	}

	if (likely((skb->len - skb->data_len) != (RE8670_HW_LSO_FS_TX_STUCK_LEN+addi_l2_len) 
		&& (skb->len - skb->data_len) != RE8670_HW_LSO_FS_TX_STUCK_LEN_1))
	{
#ifdef CONFIG_REALTEK_HW_LSO			
		cp->tx_skb[ring_num][*entry].skb = (struct sk_buff *)0xffffffff;
#else
		cp->tx_skb[ring_num][*entry].skb = skb;
#endif
		cp->tx_skb[ring_num][*entry].mapping = (dma_addr_t)skb->data;
		cp->tx_skb[ring_num][*entry].frag = 1;

		first_txd = &cp->tx_Mhqring[ring_num][*entry];
		if(ptxInfo)
		{
			memcpy(first_txd, ptxInfo, sizeof(struct tx_info));
		}

		first_txd->addr = (skb->data);
#ifdef HW_LSO_ENABLE
		mtu = re8670_get_mtu(skb, cp);
		if(mtu && (skb->len-14) > mtu)
		{
			first_txd->opts4.bit.lgsen = 1;
			first_txd->opts4.bit.lgmtu = mtu; //real MTU
		}
		else
		{
			first_txd->opts4.bit.lgsen = 0;
			first_txd->opts4.bit.lgmtu = 0;
		}
#endif
		dma_cache_wback_inv((unsigned long)skb->data, (skb->len - skb->data_len));
		first_txd->opts1.bit.fs = 1;
		first_txd->opts1.bit.ls = 0;
		first_txd->opts1.bit.crc = 1;
		first_txd->opts1.bit.ipcs = 1;
		first_len = (skb->len - skb->data_len);
		first_txd->opts1.bit.data_length = first_len;	
		first_txd->opts1.bit.eor = (*entry == (cp->re8670_tx_ring_size[ring_num] - 1)) ? 1 : 0;
		re8686_direct_tx_to_cpu_port(first_txd);
		first_txd->opts1.bit.own = 0;
		first_entry = *entry;
#ifdef TX_RING_DEBUG
		if(cp->tx_ring_backup_debug)
		{
			DMA_TX_DESC  *dbg_txd = &cp->rtl8686_tx_ring_debug[ring_num].txDescriptor[*entry];
			memset(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry], 0 , TX_RING_DEBUG_BUFFER_SIZE);
			memcpy(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry], skb->data, (skb->len - skb->data_len));
			apply_to_txdesc(dbg_txd, first_txd);
			dbg_txd->addr = &cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry];
		}
#endif
		dma_cache_wback_inv(first_txd, sizeof(struct tx_info)); 

		*entry = NEXT_TX(*entry, cp->re8670_tx_ring_size[ring_num]);
		for (frag = 0; frag < skb_shinfo(skb)->nr_frags; frag++) {
			skb_frag_t *this_frag = &skb_shinfo(skb)->frags[frag];
			u32 len, mapping, new_skb_len;

#if defined(LINUX_SG_ENABLE) && defined(NIC_DESC_ACCELERATE_FOR_SG)
			u32 first_data_mapping;
			u32 next_mapping;
			skb_frag_t *next_frag = &skb_shinfo(skb)->frags[frag+1];
			if((frag+1) == skb_shinfo(skb)->nr_frags) next_frag=NULL;

			len=0;
			mapping = (u32)page_address(skb_frag_page(this_frag))+(u32)this_frag->page_offset;
			first_data_mapping=mapping;

			while(next_frag)
			{
				next_mapping=(u32)page_address(skb_frag_page(next_frag))+(u32)next_frag->page_offset;
				if(next_mapping==this_frag->size+mapping)
				{
					len += this_frag->size;
					this_frag=next_frag;
					mapping=next_mapping;
					++frag;
					next_frag=&skb_shinfo(skb)->frags[frag+1];
					if((frag+1) == skb_shinfo(skb)->nr_frags) next_frag=NULL;
				}
				else //the data is not continuously
				{
					break;
				}
			}
			len+=this_frag->size;
			//first_data_mapping |= UNCACHE_MASK; //tysu: disable uncache
			mapping=first_data_mapping;
#else
			len = this_frag->size;
			//mapping = (u32)page_address(this_frag->page)+(u32)this_frag->page_offset|UNCACHE_MASK;
			mapping = (u32)page_address(this_frag->page.p)+(u32)this_frag->page_offset; //tysu: disable uncache
#endif
			txd = &cp->tx_Mhqring[ring_num][*entry];
			if(ptxInfo)
			{
				memcpy(txd, ptxInfo, sizeof(struct tx_info));
			}
			txd->addr = (mapping);

#ifdef HW_LSO_ENABLE
			if(mtu && (skb->len-14) > mtu)
			{
				txd->opts4.bit.lgsen = 1;
				txd->opts4.bit.lgmtu = mtu; //real MTU
			}
			else
			{
				txd->opts4.bit.lgsen = 0;
				txd->opts4.bit.lgmtu = 0;
			}
#endif
			dma_cache_wback_inv((unsigned long)mapping, len);
			txd->opts1.bit.fs = 0;
			if(frag != skb_shinfo(skb)->nr_frags-1)
			{
				if (!frag && (mtu > first_len) && (len <= (mtu - first_len)))
				{
					new_skb = dev_alloc_skb(skb->len);
					if(new_skb)
					{
						new_skb->dev = skb->dev;
						cp->tx_skb[ring_num][first_entry].skb = new_skb;
						//first_data_mapping = (new_skb->data);
						txd->addr = (new_skb->data);
						new_skb_len = len;
						this_frag = &skb_shinfo(skb)->frags[frag];
						memcpy((new_skb->data), (u32)page_address(skb_frag_page(this_frag))+(u32)this_frag->page_offset, this_frag->size);
						while(new_skb_len <= (mtu - first_len))
						{
							frag++;
							this_frag = &skb_shinfo(skb)->frags[frag];
							memcpy((new_skb->data + new_skb_len), (u32)page_address(skb_frag_page(this_frag))+(u32)this_frag->page_offset, this_frag->size);
							new_skb_len += this_frag->size;
							if (frag != skb_shinfo(skb)->nr_frags-1)
							{
								txd->opts1.bit.ls = 0;
								cp->tx_skb[ring_num][*entry].skb = (struct sk_buff *)0xffffffff;
							}
							else
							{
								txd->opts1.bit.ls = 1;
								cp->tx_skb[ring_num][*entry].skb = skb;
								break;
							}
						}
						skb_put(new_skb, new_skb_len);
						len = new_skb_len;
						dma_cache_wback_inv(new_skb->data, new_skb->len);
					}
					else
					{
						txd->opts1.bit.ls = 0;
						cp->tx_skb[ring_num][*entry].skb = (struct sk_buff *)0xffffffff;
					}
				}
				else
				{
					txd->opts1.bit.ls = 0;
					cp->tx_skb[ring_num][*entry].skb = (struct sk_buff *)0xffffffff;
				}
			}
			else
			{
				txd->opts1.bit.ls = 1;
				cp->tx_skb[ring_num][*entry].skb = skb;
			}
			txd->opts1.bit.crc = 1;
			txd->opts1.bit.ipcs = 1;
			txd->opts1.bit.data_length = len;
			txd->opts1.bit.eor = (*entry == (cp->re8670_tx_ring_size[ring_num] - 1)) ? 1 : 0;
			re8686_direct_tx_to_cpu_port(txd);
			txd->opts1.bit.own = 1;
#ifdef TX_RING_DEBUG
			if(cp->tx_ring_backup_debug)
			{
				DMA_TX_DESC  *dbg_txd = &cp->rtl8686_tx_ring_debug[ring_num].txDescriptor[*entry];
				memset(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry], 0 , TX_RING_DEBUG_BUFFER_SIZE);
				memcpy(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry], mapping, len);
				apply_to_txdesc(dbg_txd, txd);
				dbg_txd->addr = &cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry];
			}
#endif
			dma_cache_wback_inv(txd, sizeof(struct tx_info));

			//wmb();
			//msg_queue_print("i=%x %x s=%x %x l=%d\n",*(u8*)(txd->addr+0x12),*(u8*)(txd->addr+0x13),*(u8*)(txd->addr+0x32),*(u8*)(txd->addr+0x33),len);

#if defined(LINUX_SG_ENABLE) && defined(NIC_DESC_ACCELERATE_FOR_SG)			
			cp->tx_skb[ring_num][*entry].mapping = (dma_addr_t)first_data_mapping;
#else
			cp->tx_skb[ring_num][*entry].mapping = (dma_addr_t)mapping;
#endif
			cp->tx_skb[ring_num][*entry].frag = frag + 1;
			*entry = NEXT_TX(*entry, cp->re8670_tx_ring_size[ring_num]);
		}

		first_txd->opts1.bit.own = 1;
		dma_cache_wback_inv(&(first_txd->opts1), sizeof(u32));
	}
	else
	{
		new_skb = dev_alloc_skb(skb->len);
		if(new_skb)
		{
			new_skb->dev = skb->dev;
			cp->tx_skb[ring_num][*entry].skb = new_skb;
			first_txd = &cp->tx_Mhqring[ring_num][*entry];
			if(ptxInfo)
			{
				memcpy(first_txd, ptxInfo, sizeof(struct tx_info));
			}

			first_txd->addr = (new_skb->data);
			memcpy(new_skb->data, skb->data, (skb->len - skb->data_len));
#ifdef HW_LSO_ENABLE
			mtu = re8670_get_mtu(skb, cp);
			if(mtu && (skb->len-14) > mtu)
			{
				first_txd->opts4.bit.lgsen = 1;
				first_txd->opts4.bit.lgmtu = mtu; //real MTU
			}
			else
			{
				first_txd->opts4.bit.lgsen = 0;
				first_txd->opts4.bit.lgmtu = 0;
			}
#endif
			first_txd->opts1.bit.fs = 1;
			first_txd->opts1.bit.ls = 1;
			first_txd->opts1.bit.crc = 1;
			first_txd->opts1.bit.ipcs = 1;
			first_len = (skb->len - skb->data_len);
			first_txd->opts1.bit.data_length = first_len;
			first_txd->opts1.bit.eor = (*entry == (cp->re8670_tx_ring_size[ring_num] - 1)) ? 1 : 0;
			re8686_direct_tx_to_cpu_port(first_txd);

			for (frag = 0; frag < skb_shinfo(skb)->nr_frags; frag++) {
				skb_frag_t *this_frag = &skb_shinfo(skb)->frags[frag];

				memcpy(new_skb->data + first_txd->opts1.bit.data_length, (u32)page_address(skb_frag_page(this_frag))+(u32)this_frag->page_offset, this_frag->size);
				first_txd->opts1.bit.data_length += this_frag->size;
			}
			dma_cache_wback_inv((unsigned long)new_skb->data, first_txd->opts1.bit.data_length);
			first_txd->opts1.bit.own = 1;

			if(!first_txd->opts4.bit.lgsen && first_txd->opts1.bit.data_length > RE8670_MAX_ETH_FRAME_SIZE)
			{
				if(!cp->tx_jumbo_frame_enabled || (first_txd->opts1.bit.data_length > RE8670_MAX_ETH_JUMBO_FRAME_SIZE))
				{
					dev_kfree_skb_any(new_skb);
					cp->tx_skb[ring_num][*entry].skb = skb;
					*entry = NEXT_TX(*entry, cp->re8670_tx_ring_size[ring_num]);
					return -1;
				}
				else
				{
					first_txd->opts1.dw &= ~(IPCS|L4CS);
				}
			}

#ifdef TX_RING_DEBUG
			if(cp->tx_ring_backup_debug)
			{
				DMA_TX_DESC  *dbg_txd = &cp->rtl8686_tx_ring_debug[ring_num].txDescriptor[*entry];
				memset(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry], 0 , TX_RING_DEBUG_BUFFER_SIZE);
				memcpy(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry], skb->data, (skb->len - skb->data_len));
				apply_to_txdesc(dbg_txd, first_txd);
				dbg_txd->addr = &cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[*entry];
			}
#endif
			dma_cache_wback_inv(first_txd, sizeof(struct tx_info));
			dev_kfree_skb_any(skb);
		}
		else
		{
			cp->tx_skb[ring_num][*entry].skb = skb;
		}
		*entry = NEXT_TX(*entry, cp->re8670_tx_ring_size[ring_num]);
	}
}
#endif

static inline void kick_tx(unsigned int gmac, int ring_num){	
	ring_num = idx_sw2hw(ring_num);
	
	switch(ring_num) {
		case 0:
		case 1:
		case 2:
		case 3:
			RLE0787_W32(gmac, IO_CMD, RLE0787_R32(gmac, IO_CMD) | (1 << ring_num));
			break;
		case 4:
			RLE0787_W32(gmac, IO_CMD1, RLE0787_R32(gmac, IO_CMD1) | TX_POLL5);
			break;
		default:
			printk("%s %d: wrong ring num %d\n", __func__, __LINE__, ring_num);
			break;
	}
}

static inline void apply_to_txdesc(DMA_TX_DESC  *txd, struct tx_info *pTxInfo){
	txd->addr = pTxInfo->addr;
	txd->opts2 = pTxInfo->opts2.dw;
	txd->opts3 = pTxInfo->opts3.dw;
	txd->opts4 = pTxInfo->opts4.dw;
	//must be last write....
	wmb();
	txd->opts1 = pTxInfo->opts1.dw;
}

__IRAM_NIC
void do_txInfoMask(struct tx_info* pTxInfo, struct tx_info* ptx, struct tx_info* ptxMask){
	if(ptxMask && ptx){
		if(ptxMask->opts1.dw)
		{
			pTxInfo->opts1.dw &= (~ptxMask->opts1.dw);
			pTxInfo->opts1.dw |= (ptx->opts1.dw & ptxMask->opts1.dw);
		}
		if(ptxMask->opts2.dw)
		{
			pTxInfo->opts2.dw &= (~ptxMask->opts2.dw);
			pTxInfo->opts2.dw |= (ptx->opts2.dw & ptxMask->opts2.dw);
		}
		if(ptxMask->opts3.dw)
		{
			pTxInfo->opts3.dw &= (~ptxMask->opts3.dw);
			pTxInfo->opts3.dw |= (ptx->opts3.dw & ptxMask->opts3.dw);
		}
		if(ptxMask->opts4.dw)
		{
			pTxInfo->opts4.dw &= (~ptxMask->opts4.dw);
			pTxInfo->opts4.dw |= (ptx->opts4.dw & ptxMask->opts4.dw);
		}
	}
}

void re8686_direct_tx_to_cpu_port(struct tx_info *tx_info_ptr)
{
	if((tx_info_ptr->opts2.bit.tx_portmask & CPU_PORT_MASK_ALL)
		&& !(tx_info_ptr->opts3.bit.tx_dst_stream_id & 0x7F)
	)
	{
		tx_info_ptr->opts3.bit.tx_dst_stream_id |= 0x1;
	}
}

int re8686_send_with_txInfo_and_mask(struct sk_buff *skb, struct tx_info* ptxInfo, int ring_num, struct tx_info* ptxInfoMask)
{
	struct net_device *dev = ROOTDEV;
	struct re_private_root *root_cp = DEV2CP(dev);
	unsigned int gmac = ptxInfo->opts3.bit.gmac_id;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	unsigned entry;
	u32 eor;
	struct tx_info local_txInfo={{{0}},0,{{0}},{{0}}};
#ifdef HW_LSO_ENABLE
	int mtu;
#endif

	if(unlikely(gmac >= MAX_GMAC_NUM) || unlikely(cp->gmac_enabled != GMAC_TRUE)) {
		dev_kfree_skb_any(skb);
		TX_TRACE(0, cp->debug_enable, cp->debug_times, "GMAC%d disabled, free skb.\n", gmac);
		return -1;
	}

	GMAC_SPIN_LOCK(&cp->tx_lock);

	if((skb->dev==NULL) || !(skb->dev->priv_flags&(IFF_DOMAIN_ELAN|IFF_DOMAIN_WAN)))
		skb->dev=dev;
	
	//save ptxInfo, now we only need to save opts1 and opts2
	memcpy(&local_txInfo, ptxInfo, sizeof(struct tx_info));
	
	ETHDBG_PRINT(gmac, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX, "Tx dev=%s nr_frags=%d\n", dev->name, skb_shinfo(skb)->nr_frags);
	SKB_DBG(skb, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX);
	cp->cp_stats.tx_sw_num++;
	checkTXDesc(ring_num, gmac);

	if(unlikely(cp->eth_close == GMAC_TRUE)) {
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		dev_kfree_skb_any(skb);
		GMAC_SPIN_UNLOCK(&cp->tx_lock);
		return -1;
	}

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= (skb_shinfo(skb)->nr_frags + 1)))
	{
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		dev_kfree_skb_any(skb);
		cp->cp_stats.tx_no_desc++;
		netif_stop_queue(dev);
		kick_tx(gmac, ring_num);
		GMAC_SPIN_UNLOCK(&cp->tx_lock);
		return -1;
	}
			
	entry = cp->tx_Mhqhead[ring_num];    
	eor = (entry == (cp->re8670_tx_ring_size[ring_num] - 1)) ? RingEnd : 0;
	if (skb_shinfo(skb)->nr_frags == 0) {
		u32 len;
		DMA_TX_DESC  *txd;
		txd = &cp->tx_Mhqring[ring_num][entry];    

		len = skb->len;

		if(len < RE8686_HW_SMALLEST_DATA_LEN)
		{
			DEVPRIV(skb->dev)->net_stats.tx_dropped++;
			dev_kfree_skb_any(skb);
			cp->cp_stats.tooshort++;
			TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "skb len too short dropped (len=%d).\n", len);
			GMAC_SPIN_UNLOCK(&cp->tx_lock);
			return -1;
		}

		if(len > 0)		
		// Kaohj --- invalidate DCache before NIC DMA
			dma_cache_wback_inv((unsigned long)skb->data, len);

		//default setting, always need this
		local_txInfo.addr = (u32)(skb->data);

		//[step1] opts1 clear & set init value
		local_txInfo.opts1.dw &= ~(RingEnd|0x1ffff|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);
		local_txInfo.opts1.dw |= (eor|len|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);

		//[step2] opts1 might be changed by some additional setting or parameter ptxInfo
		//plz put tx additional setting into this function
		tx_additional_setting(skb, dev, &local_txInfo);
		do_txInfoMask(&local_txInfo, ptxInfo, ptxInfoMask);

		//[step3] HW LSO or jumbo frame decision
#ifdef HW_LSO_ENABLE
		local_txInfo.opts4.bit.lgsen = 0;
		mtu = re8670_get_mtu(skb, cp);
		if(skb_is_gso(skb) && mtu && (skb->len-14) > mtu)
		{
			local_txInfo.opts4.bit.lgsen = 1;
			local_txInfo.opts4.bit.lgmtu = mtu; //real MTU
		}
		else
#endif
		if(skb->len > RE8670_MAX_ETH_FRAME_SIZE)
		{
			if(!cp->tx_jumbo_frame_enabled || (skb->len >= RE8670_MAX_ETH_JUMBO_FRAME_SIZE))
			{
				//memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "TX SKB LEN > 1522", RTL8686_MEM_DUMP_FORMAT_ALL);
				DEVPRIV(skb->dev)->net_stats.tx_dropped++;
				dev_kfree_skb_any(skb);
				GMAC_SPIN_UNLOCK(&cp->tx_lock);
				return -1;
			}
			else
			{
				local_txInfo.opts1.dw &= ~(IPCS|L4CS);
			}
		}

		//[step4] support CPU port direct Tx 
		if(local_txInfo.opts2.bit.tx_portmask&CPU_PORT_MASK_ALL)
		{
			re8686_direct_tx_to_cpu_port(&local_txInfo);
		}

		TXINFO_DBG(cp, &local_txInfo);
		
		//apply to txdesc
		apply_to_txdesc(txd, &local_txInfo);
#ifdef TX_RING_DEBUG
		if(cp->tx_ring_backup_debug)
		{
			DMA_TX_DESC  *dbg_txd = &cp->rtl8686_tx_ring_debug[ring_num].txDescriptor[entry];
			memset(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry], 0 , TX_RING_DEBUG_BUFFER_SIZE);
			memcpy(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry], skb->data, skb->len);
			apply_to_txdesc(dbg_txd, &local_txInfo);
			dbg_txd->addr = &cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry];
		}
#endif
		cp->tx_skb[ring_num][entry].skb = skb;
		cp->tx_skb[ring_num][entry].mapping = (dma_addr_t)(skb->data);
		cp->tx_skb[ring_num][entry].frag = 0;

		TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "FROM_FWD[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d VlanAct=%d Vlan=%d Pri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x PON_SID=%d\n",
		(u32)skb&0xffff,
		skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],
		skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11],
		(skb->data[12]<<8)|skb->data[13],skb->len,
		local_txInfo.opts2.bit.tx_cvlan_action,
		local_txInfo.opts2.bit.cvlan_vidh<<8|local_txInfo.opts2.bit.cvlan_vidl,
		local_txInfo.opts2.bit.cvlan_prio,
		local_txInfo.opts3.bit.extspa,
		local_txInfo.opts2.bit.tx_portmask,
		local_txInfo.opts3.bit.l34_keep,
		local_txInfo.opts3.bit.tx_dst_stream_id);

		entry = NEXT_TX(entry, cp->re8670_tx_ring_size[ring_num]);
	} else {
#ifdef CONFIG_REALTEK_HW_LSO	
		re8670_mFrag_xmit(skb, cp, &entry, NULL, ring_num);
#else
		printk("%s %d: not support frag xmit now\n", __func__, __LINE__);
		dev_kfree_skb_any(skb);
#endif
	}
	cp->tx_Mhqhead[ring_num] = entry;

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= 1))
		netif_stop_queue(dev);

	//for memory controller's write buffer
	//write_buffer_flush();
	mb();
	cp->cp_stats.tx_hw_num++;
	TXDBG_TIMES_UPDATE(cp);
	dev->trans_start = jiffies;
	netdev_pick_tx(dev, skb, NULL)->trans_start = jiffies;
	sk_pacing_shift_update(skb->sk, 8);
	GMAC_SPIN_UNLOCK(&cp->tx_lock);
#ifdef TX_CREATE_TEST_PACKET_DEBUG
	if(!cp->test_packet_start)
#endif
	{
		kick_tx(gmac, ring_num);
	}

	return 0;
}

__IRAM_NIC
int re8686_send_with_txInfo(struct sk_buff *skb, struct tx_info* ptxInfo, int ring_num) 
{
	struct net_device *dev = ROOTDEV;
	struct re_private_root *root_cp = DEV2CP(dev);	
	unsigned int gmac = ptxInfo->opts3.bit.gmac_id;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	unsigned entry;
	u32 eor;
#ifdef HW_LSO_ENABLE
	int mtu;
#endif

	if(unlikely(gmac >= MAX_GMAC_NUM) || unlikely(cp->gmac_enabled != GMAC_TRUE)) {
		dev_kfree_skb_any(skb);
		TX_TRACE(0, cp->debug_enable, cp->debug_times, "GMAC%d disabled, free skb.\n", gmac);
		return -1;
	}
	
	GMAC_SPIN_LOCK(&cp->tx_lock);

	if((skb->dev==NULL) || !(skb->dev->priv_flags&(IFF_DOMAIN_ELAN|IFF_DOMAIN_WAN)))
		skb->dev=dev;
	
	ETHDBG_PRINT(gmac, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX, "Tx dev=%s nr_frags=%d\n", skb->dev->name, skb_shinfo(skb)->nr_frags);
	SKB_DBG(skb, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX);
	cp->cp_stats.tx_sw_num++;
	checkTXDesc(ring_num, gmac);

	if(unlikely(cp->eth_close == GMAC_TRUE)
#ifdef TX_CREATE_TEST_PACKET_DEBUG
		|| cp->test_packet_start
#endif
		) 
	{
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		dev_kfree_skb_any(skb);
		GMAC_SPIN_UNLOCK(&cp->tx_lock);
		return -1;
	}

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= (skb_shinfo(skb)->nr_frags + 1)))
	{
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		dev_kfree_skb_any(skb);
		cp->cp_stats.tx_no_desc++;
		netif_stop_queue(dev);
		kick_tx(gmac, ring_num);
		GMAC_SPIN_UNLOCK(&cp->tx_lock);
		return -1;
	}

	entry = cp->tx_Mhqhead[ring_num];	 
	eor = (entry == (cp->re8670_tx_ring_size[ring_num] - 1)) ? RingEnd : 0;
	if (skb_shinfo(skb)->nr_frags == 0) {
		u32 len;
		DMA_TX_DESC  *txd;
		txd = &cp->tx_Mhqring[ring_num][entry];    

		len = skb->len;

		if(len < RE8686_HW_SMALLEST_DATA_LEN)
		{
			DEVPRIV(skb->dev)->net_stats.tx_dropped++;
			dev_kfree_skb_any(skb);
			cp->cp_stats.tooshort++;
			TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "skb len too short dropped (len=%d).\n", len);
			GMAC_SPIN_UNLOCK(&cp->tx_lock);
			return -1;
		}
		
		if(len > 0) 	
		// Kaohj --- invalidate DCache before NIC DMA
			dma_cache_wback_inv((unsigned long)skb->data, len);

		//default setting, always need this
		ptxInfo->addr = (u32)(skb->data);

		//[step1] opts1 clear & set init value
		ptxInfo->opts1.dw &= ~(RingEnd|0x1ffff|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);
		ptxInfo->opts1.dw |= (eor|len|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);

		//[step2] HW LSO or jumbo frame decision
#ifdef HW_LSO_ENABLE
		ptxInfo->opts4.bit.lgsen = 0;
		mtu = re8670_get_mtu(skb, cp);
		if(skb_is_gso(skb) && mtu && (skb->len-14) > mtu)
		{
			ptxInfo->opts4.bit.lgsen = 1;
			ptxInfo->opts4.bit.lgmtu = mtu; //real MTU
		}
		else
#endif
		if(skb->len > RE8670_MAX_ETH_FRAME_SIZE)
		{
			if(!cp->tx_jumbo_frame_enabled || (skb->len >= RE8670_MAX_ETH_JUMBO_FRAME_SIZE))
			{
				//memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "TX SKB LEN > 1522", RTL8686_MEM_DUMP_FORMAT_ALL);
				DEVPRIV(skb->dev)->net_stats.tx_dropped++;
				dev_kfree_skb_any(skb);
				GMAC_SPIN_UNLOCK(&cp->tx_lock);
				return -1;
			}
			else
			{
				ptxInfo->opts1.dw &= ~(IPCS|L4CS);
			}
		}

		//[step3] support CPU port direct Tx 
		if(ptxInfo->opts2.bit.tx_portmask&CPU_PORT_MASK_ALL)
		{
			re8686_direct_tx_to_cpu_port(ptxInfo);
		}
		
		ptxInfo->opts2.bit.cputag = 1;

		TXINFO_DBG(cp, ptxInfo);
		
		//apply to txdesc
		apply_to_txdesc(txd, ptxInfo);
#ifdef TX_RING_DEBUG
		if(cp->tx_ring_backup_debug)
		{
			DMA_TX_DESC  *dbg_txd = &cp->rtl8686_tx_ring_debug[ring_num].txDescriptor[entry];
			memset(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry], 0 , TX_RING_DEBUG_BUFFER_SIZE);
			memcpy(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry], skb->data, skb->len);
			apply_to_txdesc(dbg_txd, ptxInfo);
			dbg_txd->addr = &cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry];
		}
#endif
		cp->tx_skb[ring_num][entry].skb = skb;
		cp->tx_skb[ring_num][entry].mapping = (dma_addr_t)(skb->data);
		cp->tx_skb[ring_num][entry].frag = 0;

		TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "FROM_FC[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d VlanAct=%d Vlan=%d Pri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x PON_SID=%d\n",
		(u32)skb&0xffff,
		skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],
		skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11],
		(skb->data[12]<<8)|skb->data[13],skb->len,
		ptxInfo->opts2.bit.tx_cvlan_action,
		ptxInfo->opts2.bit.cvlan_vidh<<8|ptxInfo->opts2.bit.cvlan_vidl,
		ptxInfo->opts2.bit.cvlan_prio,
		ptxInfo->opts3.bit.extspa,
		ptxInfo->opts2.bit.tx_portmask,
		ptxInfo->opts3.bit.l34_keep,
		ptxInfo->opts3.bit.tx_dst_stream_id);

		entry = NEXT_TX(entry, cp->re8670_tx_ring_size[ring_num]);
	} else {
#ifdef CONFIG_REALTEK_HW_LSO	
		re8670_mFrag_xmit(skb, cp, &entry, ptxInfo, ring_num);
#else
		printk("%s %d: not support frag xmit now\n", __func__, __LINE__);
		dev_kfree_skb_any(skb);
#endif
	}
	cp->tx_Mhqhead[ring_num] = entry;

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= 1))
		netif_stop_queue(dev);

	//for memory controller's write buffer
	//write_buffer_flush();
	mb();
	cp->cp_stats.tx_hw_num++;
	TXDBG_TIMES_UPDATE(cp);
	dev->trans_start = jiffies;
	netdev_pick_tx(dev, skb, NULL)->trans_start = jiffies;
	sk_pacing_shift_update(skb->sk, 8);
	GMAC_SPIN_UNLOCK(&cp->tx_lock);
#ifdef TX_CREATE_TEST_PACKET_DEBUG
	if(!cp->test_packet_start)
#endif
	{
		kick_tx(gmac, ring_num);
	}

	return 0;
}

#if defined(CONFIG_RTK_NIC_HWLOOKUP_DEAMSDU_OFFLOAD)
unsigned long amsdu_counter=0;
int rtk_nic_amsdu_cnt_get(unsigned long *pAmsduCnt)
{
	*pAmsduCnt=amsdu_counter;
	return 0;
}
int rtk_nic_amsdu_cnt_clear(void)
{
	amsdu_counter=0;
	return 0;
}
EXPORT_SYMBOL(rtk_nic_amsdu_cnt_get);
EXPORT_SYMBOL(rtk_nic_amsdu_cnt_clear);

#ifdef GET_GMAC_BY_LEN
static int tx_len_cmp = 0;
#ifdef GMAC_LEN_STATISTIC
static uint64 tx_acc_len[2] = {0};
static uint32 tx_acc_cnt[2] = {0};
#endif
int _get_gmac_by_len(int tx_len)
{
	unsigned int gmac;
	if (tx_len_cmp > 0) {
		tx_len_cmp = tx_len_cmp - tx_len;
		gmac = 1;
	} else {
		tx_len_cmp = tx_len_cmp + tx_len;
		gmac = 0;
	}
#ifdef GMAC_LEN_STATISTIC
	tx_acc_len[gmac] += tx_len;
	tx_acc_cnt[gmac]++;
#endif
	return gmac;
}
#endif /* GET_GMAC_BY_LEN */

extern uint32 _rtk_fc_wlan_rx_lookup_gmac(struct sk_buff *skb);
__IRAM_NIC
int re8686_wifi_hwlookup_deamsdu(struct sk_buff *skb, struct tx_info* ptxInfo, int ring_num)
{
	unsigned char *pAmsdu_data=skb->data;
	int snap_len=ntohs(*(unsigned short *)(pAmsdu_data+ETH_ALEN+ETH_ALEN));

	//normal single skb, not amsdu packet.
	if(snap_len >= 0x0800){
		if (ptxInfo->opts3.bit.gmac_id != 2) {
#ifdef GET_GMAC_BY_LEN
			ptxInfo->opts3.bit.gmac_id = _get_gmac_by_len(snap_len);
#else
			ptxInfo->opts3.bit.gmac_id = _rtk_fc_wlan_rx_lookup_gmac(skb);
#endif /* GET_GMAC_BY_LEN */
		}
		return re8686_send_with_txInfo(skb, ptxInfo, ring_num);
	}else{
		struct net_device *dev = ROOTDEV;
		struct re_private_root *root_cp = DEV2CP(dev);
		unsigned int gmac = ptxInfo->opts3.bit.gmac_id;
		struct re_private *cp;// = root_cp->re_private_data_ptr[gmac];
		unsigned int entry,last_entry;
		u32 eor;
#ifdef HW_LSO_ENABLE
		int mtu;
#endif
		int ringNum=ring_num;

		DMA_TX_DESC  *txd;
		int DESC_NEED_NUM=1;
		struct sk_buff *txq_skb;
		unsigned char padding;
		int amsdu_len,offset,keepLoop=1;
		unsigned char *pSnap_data;

		amsdu_len=skb->len;
		
		while(keepLoop){
			if (ptxInfo->opts3.bit.gmac_id != 2) {
#ifdef GET_GMAC_BY_LEN
				gmac = _get_gmac_by_len(snap_len);
#else
				gmac = _rtk_fc_wlan_rx_lookup_gmac(skb);
#endif /* GET_GMAC_BY_LEN */
			}
			cp=root_cp->re_private_data_ptr[gmac];

			if(unlikely(gmac >= MAX_GMAC_NUM) || unlikely(cp->gmac_enabled != GMAC_TRUE)
#ifdef TX_BURST_PACKET_SEND
				|| unlikely(cp->burst_tx_info.burst_buffer)
#endif
				) {
				dev_kfree_skb_any(skb);
				TX_TRACE(0, cp->debug_enable, cp->debug_times, "GMAC%d disabled, free skb.\n", gmac);
				return -1;
			}

			GMAC_SPIN_LOCK(&cp->tx_lock);
#ifdef HWNAT_CUSTOMIZE
			if(re8686_tx_ring_customized[gmac][ring_num])ringNum=MAX_TXRING_NUM-1;
#endif

			if((skb->dev==NULL) || !(skb->dev->priv_flags&(IFF_DOMAIN_ELAN|IFF_DOMAIN_WAN)))
				skb->dev=dev;

			ETHDBG_PRINT(gmac, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX, "Tx dev=%s\n", skb->dev->name);
			SKB_DBG(skb, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX);

			cp->cp_stats.tx_sw_num++;

			checkTXDesc(ringNum, gmac);

			if(unlikely(cp->eth_close == GMAC_TRUE)
#ifdef TX_CREATE_TEST_PACKET_DEBUG
				|| cp->test_packet_start
#endif
				)
			{
				DEVPRIV(skb->dev)->net_stats.tx_dropped++;
				dev_kfree_skb_any(skb);
				GMAC_SPIN_UNLOCK(&cp->tx_lock);
				return -1;
			}

			if (unlikely(TX_HQBUFFS_AVAIL(cp,ringNum) <= DESC_NEED_NUM))
			{
				DEVPRIV(skb->dev)->net_stats.tx_dropped++;
				dev_kfree_skb_any(skb);
				cp->cp_stats.tx_no_desc++;
				netif_stop_queue(dev);
				kick_tx(gmac, ringNum);
				GMAC_SPIN_UNLOCK(&cp->tx_lock);
				return -1;
			}

			
			last_entry = cp->re8670_tx_ring_size[ringNum];	//init the last entry
		//while(1){
			amsdu_counter++;
			pSnap_data=pAmsdu_data+8;							//DA(6B)+SA(6B)+LEN(2B)+LLC(3B)+SNAP(5B)-DA(6B)-SA(6B)-ETHTYPE(2B)=8
			memmove(pSnap_data,pAmsdu_data,ETH_ALEN<<1);		//move DA and SA to ahead of ETHTYPE of SNAP header

			padding=(snap_len+ETH_HLEN)&0x3;
			if(padding){
				offset=4-padding;
			}else
				offset=0;
			offset+=snap_len;
			offset+=ETH_HLEN;	//DA(6B)+SA(6B)+LEN(2B)
			amsdu_len-=offset;

			//packet len = snap_len - LLC(3B) - SNAP(5B) + DA(6B) + SA(6B) + ETHERTYPE(2B)
			snap_len+=6;

			//the last snap packet should call hw_lookup with real SKB, otherwise NULL.
			if(amsdu_len<=ETH_HLEN)
				txq_skb=skb;
			else
				txq_skb=0xFFFFFFFF;

			if(snap_len < RE8686_HW_SMALLEST_DATA_LEN)
			{
				DEVPRIV(skb->dev)->net_stats.tx_dropped++;
				if(last_entry != cp->re8670_tx_ring_size[ringNum])
					cp->tx_skb[ringNum][last_entry].skb=skb;
				else
					dev_kfree_skb_any(skb);
				cp->cp_stats.tooshort++;
				TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "skb len too short dropped (len=%d).\n", snap_len);
				GMAC_SPIN_UNLOCK(&cp->tx_lock);
				return -1;
			}
			if(snap_len > RE8670_MAX_ETH_FRAME_SIZE)
			{
				if(!cp->tx_jumbo_frame_enabled || (snap_len >= RE8670_MAX_ETH_JUMBO_FRAME_SIZE))
				{
					//memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "TX SKB LEN > 1522", RTL8686_MEM_DUMP_FORMAT_ALL);
					DEVPRIV(skb->dev)->net_stats.tx_dropped++;
					if(last_entry != cp->re8670_tx_ring_size[ringNum])
						cp->tx_skb[ringNum][last_entry].skb=skb;
					else
						dev_kfree_skb_any(skb);
					GMAC_SPIN_UNLOCK(&cp->tx_lock);
					return -1;
				}
				else
				{
					ptxInfo->opts1.dw &= ~(IPCS|L4CS);
				}
			}
			entry = cp->tx_Mhqhead[ringNum];
			eor = (entry == (cp->re8670_tx_ring_size[ringNum] - 1)) ? RingEnd : 0;
			txd = &cp->tx_Mhqring[ringNum][entry];

			// Kaohj --- invalidate DCache before NIC DMA
			dma_cache_wback_inv((unsigned long)pSnap_data, snap_len);

			//default setting, always need this
			ptxInfo->addr = (u32)(pSnap_data);

			//[step1] opts1 clear & set init value
			ptxInfo->opts1.dw &= ~(RingEnd|0x1ffff|FirstFrag);
			ptxInfo->opts1.dw |= (eor|snap_len|DescOwn|LastFrag|TxCRC|IPCS);

			//[step2] HW LSO or jumbo frame decision
#ifdef CONFIG_REALTEK_HW_LSO
			ptxInfo->opts4.bit.lgsen = 0;
#endif
			//[step3] support CPU port direct Tx
			if(ptxInfo->opts2.bit.tx_portmask&CPU_PORT_MASK_ALL)
			{
				re8686_direct_tx_to_cpu_port(ptxInfo);
			}

			ptxInfo->opts2.bit.cputag = 1;

			TXINFO_DBG(cp, ptxInfo);

			//apply to txdesc
			apply_to_txdesc(txd, ptxInfo);
#ifdef TX_RING_DEBUG
			if(cp->tx_ring_backup_debug)
			{
				DMA_TX_DESC  *dbg_txd = &cp->rtl8686_tx_ring_debug[ringNum].txDescriptor[entry];
				memset(&cp->rtl8686_tx_ring_debug[ringNum].dataBuffer[entry], 0 , TX_RING_DEBUG_BUFFER_SIZE);
				memcpy(&cp->rtl8686_tx_ring_debug[ringNum].dataBuffer[entry], pSnap_data, snap_len);
				apply_to_txdesc(dbg_txd, ptxInfo);
				dbg_txd->addr = &cp->rtl8686_tx_ring_debug[ringNum].dataBuffer[entry];
			}
#endif
			cp->tx_skb[ringNum][entry].skb = txq_skb;
			cp->tx_skb[ringNum][entry].mapping = 0;
			cp->tx_skb[ringNum][entry].frag = 0;

			TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "FROM_FC[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d VlanAct=%d Vlan=%d Pri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x PON_SID=%d\n",
				(u32)skb&0xffff,
				pSnap_data[0],pSnap_data[1],pSnap_data[2],pSnap_data[3],pSnap_data[4],pSnap_data[5],
				pSnap_data[6],pSnap_data[7],pSnap_data[8],pSnap_data[9],pSnap_data[10],pSnap_data[11],
				(pSnap_data[12]<<8)|pSnap_data[13],snap_len,
				ptxInfo->opts2.bit.tx_cvlan_action,
				ptxInfo->opts2.bit.cvlan_vidh<<8|ptxInfo->opts2.bit.cvlan_vidl,
				ptxInfo->opts2.bit.cvlan_prio,
				ptxInfo->opts3.bit.extspa,
				ptxInfo->opts2.bit.tx_portmask,
				ptxInfo->opts3.bit.l34_keep,
				ptxInfo->opts3.bit.tx_dst_stream_id);

			last_entry = entry;
			cp->tx_Mhqhead[ringNum] = NEXT_TX(entry, cp->re8670_tx_ring_size[ringNum]);
			cp->cp_stats.tx_hw_num++;

			if(txq_skb==0xFFFFFFFF){
				//move to next snap packet
				pAmsdu_data+=offset;
				//len for next one
				snap_len=ntohs(*(unsigned short *)(pAmsdu_data+ETH_ALEN+ETH_ALEN));

				if (unlikely(TX_HQBUFFS_AVAIL(cp,ringNum) <= DESC_NEED_NUM)){
					DEVPRIV(skb->dev)->net_stats.tx_dropped++;
					cp->cp_stats.tx_no_desc++;
					//make sure the whole skb will be released in the end
					cp->tx_skb[ringNum][last_entry].skb=skb;
					netif_stop_queue(dev);
					keepLoop=0;
				}
			}else{
				//finish all snap packets, just leave now.
				if (unlikely(TX_HQBUFFS_AVAIL(cp,ringNum) <= 1))
					netif_stop_queue(dev);
				keepLoop=0;
			}
		//}

			//for memory controller's write buffer
			//write_buffer_flush();
			mb();
			TXDBG_TIMES_UPDATE(cp);
			dev->trans_start = jiffies;
			netdev_pick_tx(dev, skb, NULL)->trans_start = jiffies;
			sk_pacing_shift_update(skb->sk, 8);
			GMAC_SPIN_UNLOCK(&cp->tx_lock);
#ifdef TX_CREATE_TEST_PACKET_DEBUG
			if(!cp->test_packet_start)
#endif
			{
				kick_tx(gmac, ringNum);
			}

		}

		return 0;
	}
}

__IRAM_NIC
int re8686_wifi_hwlookup_deamsdu_orig(struct sk_buff *skb, struct tx_info* ptxInfo, int ring_num)
{
	unsigned char *pAmsdu_data=skb->data;
	int snap_len=ntohs(*(unsigned short *)(pAmsdu_data+ETH_ALEN+ETH_ALEN));

	//normal single skb, not amsdu packet.
	if(snap_len >= 0x0800){
		return re8686_send_with_txInfo(skb, ptxInfo, ring_num);
	}else{
		struct net_device *dev = ROOTDEV;
		struct re_private_root *root_cp = DEV2CP(dev);
		unsigned int gmac = ptxInfo->opts3.bit.gmac_id;
		struct re_private *cp = root_cp->re_private_data_ptr[gmac];
		unsigned int entry,last_entry;
		u32 eor;
#ifdef HW_LSO_ENABLE
		int mtu;
#endif
		int ringNum=ring_num;

		DMA_TX_DESC  *txd;
#if defined(CONFIG_RTK_NIC_HWLOOKUP_SNAP_TRANSFORM_BY_TX_DESC)
		DMA_TX_DESC fs_txd;
		int DESC_NEED_NUM=2;
#else
		int DESC_NEED_NUM=1;
#endif
		struct sk_buff *txq_skb;
		unsigned char padding;
		int amsdu_len,offset;
		unsigned char *pSnap_data;

		if(unlikely(gmac >= MAX_GMAC_NUM) || unlikely(cp->gmac_enabled != GMAC_TRUE)
#ifdef TX_BURST_PACKET_SEND
			|| unlikely(cp->burst_tx_info.burst_buffer)
#endif
			) {
			dev_kfree_skb_any(skb);
			TX_TRACE(0, cp->debug_enable, cp->debug_times, "GMAC%d disabled, free skb.\n", gmac);
			return -1;
		}

		GMAC_SPIN_LOCK(&cp->tx_lock);
#ifdef HWNAT_CUSTOMIZE
		if(re8686_tx_ring_customized[gmac][ring_num])ringNum=MAX_TXRING_NUM-1;
#endif

		if((skb->dev==NULL) || !(skb->dev->priv_flags&(IFF_DOMAIN_ELAN|IFF_DOMAIN_WAN)))
			skb->dev=dev;

		ETHDBG_PRINT(gmac, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX, "Tx dev=%s\n", skb->dev->name);
		SKB_DBG(skb, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX);

		cp->cp_stats.tx_sw_num++;

		checkTXDesc(ringNum, gmac);

		if(unlikely(cp->eth_close == GMAC_TRUE)
#ifdef TX_CREATE_TEST_PACKET_DEBUG
			|| cp->test_packet_start
#endif
			)
		{
			DEVPRIV(skb->dev)->net_stats.tx_dropped++;
			dev_kfree_skb_any(skb);
			GMAC_SPIN_UNLOCK(&cp->tx_lock);
			return -1;
		}

		if (unlikely(TX_HQBUFFS_AVAIL(cp,ringNum) <= DESC_NEED_NUM))
		{
			DEVPRIV(skb->dev)->net_stats.tx_dropped++;
			dev_kfree_skb_any(skb);
			cp->cp_stats.tx_no_desc++;
			netif_stop_queue(dev);
			kick_tx(gmac, ringNum);
			GMAC_SPIN_UNLOCK(&cp->tx_lock);
			return -1;
		}
#if defined(CONFIG_RTK_NIC_HWLOOKUP_SNAP_TRANSFORM_BY_TX_DESC)
		//prepare first txd for DA and SA
		// Kaohj --- invalidate DCache before NIC DMA
		dma_cache_wback_inv((unsigned long)pAmsdu_data, ETH_ALEN<<1);

		//default setting, always need this
		ptxInfo->addr = (u32)(pAmsdu_data);

		//[step1] opts1 clear & set init value
		ptxInfo->opts1.dw &= ~(RingEnd|0x1ffff|LastFrag);
		ptxInfo->opts1.dw |= (ETH_ALEN<<1|DescOwn|FirstFrag|TxCRC|IPCS);

		//[step2] HW LSO or jumbo frame decision
#ifdef CONFIG_REALTEK_HW_LSO
		ptxInfo->opts4.bit.lgsen = 0;
#endif
		//[step3] support CPU port direct Tx
		if(ptxInfo->opts2.bit.tx_portmask&CPU_PORT_MASK_ALL)
		{
			re8686_direct_tx_to_cpu_port(ptxInfo);
		}

		ptxInfo->opts2.bit.cputag = 1;

		//apply to first txdesc
		apply_to_txdesc(&fs_txd, ptxInfo);
#endif

		amsdu_len=skb->len;
		last_entry = cp->re8670_tx_ring_size[ringNum];	//init the last entry
		while(1){
			amsdu_counter++;
#if defined(CONFIG_RTK_NIC_HWLOOKUP_SNAP_TRANSFORM_BY_TX_DESC)
			pSnap_data=pAmsdu_data+ETH_HLEN+6;					//pointer to ahead of ETHTYPE of SNAP header
#else
			pSnap_data=pAmsdu_data+8;							//DA(6B)+SA(6B)+LEN(2B)+LLC(3B)+SNAP(5B)-DA(6B)-SA(6B)-ETHTYPE(2B)=8
			memmove(pSnap_data,pAmsdu_data,ETH_ALEN<<1);		//move DA and SA to ahead of ETHTYPE of SNAP header
#endif

			padding=(snap_len+ETH_HLEN)&0x3;
			if(padding){
				offset=4-padding;
			}else
				offset=0;
			offset+=snap_len;
			offset+=ETH_HLEN;	//DA(6B)+SA(6B)+LEN(2B)
			amsdu_len-=offset;

#if defined(CONFIG_RTK_NIC_HWLOOKUP_SNAP_TRANSFORM_BY_TX_DESC)
			//second tx desc len = snap_len - LLC(3B) - SNAP(5B) + ETHERTYPE(2B)
			snap_len-=6;
#else
			//packet len = snap_len - LLC(3B) - SNAP(5B) + DA(6B) + SA(6B) + ETHERTYPE(2B)
			snap_len+=6;
#endif

			//the last snap packet should call hw_lookup with real SKB, otherwise NULL.
			if(amsdu_len<=0)
				txq_skb=skb;
			else
				txq_skb=0xFFFFFFFF;

			if(snap_len < RE8686_HW_SMALLEST_DATA_LEN)
			{
				DEVPRIV(skb->dev)->net_stats.tx_dropped++;
				if(last_entry != cp->re8670_tx_ring_size[ringNum])
					cp->tx_skb[ringNum][last_entry].skb=skb;
				else
					dev_kfree_skb_any(skb);
				cp->cp_stats.tooshort++;
				TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "skb len too short dropped (len=%d).\n", snap_len);
				GMAC_SPIN_UNLOCK(&cp->tx_lock);
				return -1;
			}
			if(snap_len > RE8670_MAX_ETH_FRAME_SIZE)
			{
				if(!cp->tx_jumbo_frame_enabled || (snap_len >= RE8670_MAX_ETH_JUMBO_FRAME_SIZE))
				{
					//memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "TX SKB LEN > 1522", RTL8686_MEM_DUMP_FORMAT_ALL);
					DEVPRIV(skb->dev)->net_stats.tx_dropped++;
					if(last_entry != cp->re8670_tx_ring_size[ringNum])
						cp->tx_skb[ringNum][last_entry].skb=skb;
					else
						dev_kfree_skb_any(skb);
					GMAC_SPIN_UNLOCK(&cp->tx_lock);
					return -1;
				}
				else
				{
					ptxInfo->opts1.dw &= ~(IPCS|L4CS);
				}
			}
			entry = cp->tx_Mhqhead[ringNum];
#if defined(CONFIG_RTK_NIC_HWLOOKUP_SNAP_TRANSFORM_BY_TX_DESC)
			if(entry == (cp->re8670_tx_ring_size[ringNum] - 1))
				fs_txd.opts1 |= RingEnd;
			else
				fs_txd.opts1 &= ~RingEnd;
			cp->tx_Mhqring[ringNum][entry]=fs_txd;
			//txd->opts1=fs_txd.opts1;
			//txd->addr=fs_txd.addr;
			//txd->opts2=fs_txd.opts2;
			//txd->opts3=fs_txd.opts3;
			//txd->opts4=fs_txd.opts4;
			TXINFO_DBG(cp, (struct tx_info *)&fs_txd);

			cp->tx_skb[ringNum][entry].skb = 0xFFFFFFFF;
			cp->tx_skb[ringNum][entry].mapping = 0;
			cp->tx_skb[ringNum][entry].frag = 0;

			//get next txd for second tx desc
			entry = NEXT_TX(entry, cp->re8670_tx_ring_size[ringNum]);
#endif
			eor = (entry == (cp->re8670_tx_ring_size[ringNum] - 1)) ? RingEnd : 0;
			txd = &cp->tx_Mhqring[ringNum][entry];

			// Kaohj --- invalidate DCache before NIC DMA
			dma_cache_wback_inv((unsigned long)pSnap_data, snap_len);

			//default setting, always need this
			ptxInfo->addr = (u32)(pSnap_data);

			//[step1] opts1 clear & set init value
			ptxInfo->opts1.dw &= ~(RingEnd|0x1ffff|FirstFrag);
			if(snap_len > RE8670_MAX_ETH_FRAME_SIZE)
				ptxInfo->opts1.dw |= (eor|snap_len|DescOwn|FirstFrag|LastFrag|TxCRC);
			else
				ptxInfo->opts1.dw |= (eor|snap_len|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);

			//[step2] HW LSO or jumbo frame decision
#ifdef CONFIG_REALTEK_HW_LSO
			ptxInfo->opts4.bit.lgsen = 0;
#endif
			//[step3] support CPU port direct Tx
			if(ptxInfo->opts2.bit.tx_portmask&CPU_PORT_MASK_ALL)
			{
				re8686_direct_tx_to_cpu_port(ptxInfo);
			}

			ptxInfo->opts2.bit.cputag = 1;

			TXINFO_DBG(cp, ptxInfo);

			//apply to txdesc
			apply_to_txdesc(txd, ptxInfo);
#ifdef TX_RING_DEBUG
			if(cp->tx_ring_backup_debug)
			{
				DMA_TX_DESC  *dbg_txd = &cp->rtl8686_tx_ring_debug[ringNum].txDescriptor[entry];
				memset(&cp->rtl8686_tx_ring_debug[ringNum].dataBuffer[entry], 0 , TX_RING_DEBUG_BUFFER_SIZE);
				memcpy(&cp->rtl8686_tx_ring_debug[ringNum].dataBuffer[entry], pSnap_data, snap_len);
				apply_to_txdesc(dbg_txd, ptxInfo);
				dbg_txd->addr = &cp->rtl8686_tx_ring_debug[ringNum].dataBuffer[entry];
			}
#endif
			cp->tx_skb[ringNum][entry].skb = txq_skb;
			cp->tx_skb[ringNum][entry].mapping = 0;
			cp->tx_skb[ringNum][entry].frag = 0;

			TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "FROM_FC[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d VlanAct=%d Vlan=%d Pri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x PON_SID=%d\n",
				(u32)skb&0xffff,
				pSnap_data[0],pSnap_data[1],pSnap_data[2],pSnap_data[3],pSnap_data[4],pSnap_data[5],
				pSnap_data[6],pSnap_data[7],pSnap_data[8],pSnap_data[9],pSnap_data[10],pSnap_data[11],
				(pSnap_data[12]<<8)|pSnap_data[13],snap_len,
				ptxInfo->opts2.bit.tx_cvlan_action,
				ptxInfo->opts2.bit.cvlan_vidh<<8|ptxInfo->opts2.bit.cvlan_vidl,
				ptxInfo->opts2.bit.cvlan_prio,
				ptxInfo->opts3.bit.extspa,
				ptxInfo->opts2.bit.tx_portmask,
				ptxInfo->opts3.bit.l34_keep,
				ptxInfo->opts3.bit.tx_dst_stream_id);

			last_entry = entry;
			cp->tx_Mhqhead[ringNum] = NEXT_TX(entry, cp->re8670_tx_ring_size[ringNum]);
			cp->cp_stats.tx_hw_num++;

			if(txq_skb==0xFFFFFFFF){
				//move to next snap packet
				pAmsdu_data+=offset;
				//len for next one
				snap_len=ntohs(*(unsigned short *)(pAmsdu_data+ETH_ALEN+ETH_ALEN));

				if (unlikely(TX_HQBUFFS_AVAIL(cp,ringNum) <= DESC_NEED_NUM)){
					DEVPRIV(skb->dev)->net_stats.tx_dropped++;
					cp->cp_stats.tx_no_desc++;
					//make sure the whole skb will be released in the end
					cp->tx_skb[ringNum][last_entry].skb=skb;
					netif_stop_queue(dev);
					break;
				}
			}else{
				//finish all snap packets, just leave now.
				if (unlikely(TX_HQBUFFS_AVAIL(cp,ringNum) <= 1))
					netif_stop_queue(dev);
				break;
			}
		}

		//for memory controller's write buffer
		//write_buffer_flush();
		mb();
		TXDBG_TIMES_UPDATE(cp);
		dev->trans_start = jiffies;
		netdev_pick_tx(dev, skb, NULL)->trans_start = jiffies;
		sk_pacing_shift_update(skb->sk, 8);
		GMAC_SPIN_UNLOCK(&cp->tx_lock);
#ifdef TX_CREATE_TEST_PACKET_DEBUG
		if(!cp->test_packet_start)
#endif
		{
			kick_tx(gmac, ringNum);
		}

		return 0;
	}
}
#endif

/*
*	return 0 -> set OK, return -1 -> set FAIL
*	ptxInfo: pointer of tx_info(ptxInfo->opts3.bit.gmac_id would be used)
*	reg_num: 0 to set VLAN_REG, 1 to set VLAN1_REG
*	value: the value to set VLAN_REG/ VLAN1_REG
*/
__IRAM_NIC
int re8686_set_vlan_register(struct tx_info* ptxInfo, unsigned char reg_num, unsigned int value) 
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[ptxInfo->opts3.bit.gmac_id];
	unsigned int gmac = cp->gmac;
	
	if(!value)
	{
		printk(" %s %d: wrong VLAN register value=%d\n", __func__, __LINE__, value);
		return -1;
	}

	switch(reg_num)
	{
		case 0:
			cp->stag_pid = value;
			RLE0787_W32(gmac, VLAN_REG, value);
			break;
		case 1:
			cp->stag_pid1 = value;
			RLE0787_W32(gmac, VLAN1_REG, value);
			break;
		default:
			printk(" %s %d: wrong reg_num=%d\n", __func__, __LINE__, reg_num);
			return -1;
	}

	return 0;
}

/*
*	return 0 -> get OK, return -1 -> get FAIL
*	ptxInfo: pointer of tx_info(ptxInfo->opts3.bit.gmac_id would be used)
*	reg_num: 0 to get VLAN_REG, 1 to get VLAN1_REG
*	value_p: the pointer to get content of VLAN_REG/ VLAN1_REG
*/
__IRAM_NIC
int re8686_get_vlan_register(struct tx_info* ptxInfo, unsigned char reg_num, unsigned int *value_p) 
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[ptxInfo->opts3.bit.gmac_id];
	unsigned int gmac = cp->gmac;

	if(value_p==NULL)
	{
		printk(" %s %d: wrong VLAN register value pointer=NULL\n", __func__, __LINE__);
		return -1;
	}

	switch(reg_num)
	{
		case 0:
			*value_p = RLE0787_R32(gmac, VLAN_REG);
			break;
		case 1:
			*value_p = RLE0787_R32(gmac, VLAN1_REG);
			break;
		default:
			printk(" %s %d: wrong reg_num=%d\n", __func__, __LINE__, reg_num);
			return -1;
	}

	return 0;
}

#if 0//def CONFIG_APOLLO_ROMEDRIVER
int re8670_start_xmit_common(struct sk_buff *skb, struct net_device *dev, struct tx_info *ptx, struct tx_info *ptxMask);

__IRAM_NIC int rtk_rg_fwdEngine_xmit (struct sk_buff *skb, void *void_ptx, void *void_ptxMask) // (void *) using (rtk_rg_txdesc_t *) casting in romeDriver, using (struct tx_info*) casting in re8686.c.
{
	struct tx_info *ptx=(struct tx_info *)void_ptx;
	struct tx_info *ptxMask=(struct tx_info *)void_ptxMask;			
	skb->cb[0]=1;
	skb->dev = rtl8686_dev_table[0].dev_instant;
	re8670_start_xmit_common(skb,skb->dev,ptx,ptxMask);	
	return 0;
}

__IRAM_NIC int re8670_start_xmit (struct sk_buff *skb, struct net_device *dev)	//shlee temp, fix this later
{
	skb->cb[0]=0;
	re8670_start_xmit_common(skb,skb->dev,NULL,NULL);
	return 0;
}

__IRAM_NIC int re8670_start_xmit_common(struct sk_buff *skb, struct net_device *dev, struct tx_info *ptx, struct tx_info *ptxMask)
#else
#if defined(CONFIG_RTK_L34_ENABLE) && defined(CONFIG_APOLLO_GPON_FPGATEST) 
extern int _rtk_rg_virtualMAC_with_PON_get(void);
#endif
__IRAM_NIC int re8670_start_xmit_txInfo (struct sk_buff *skb, struct net_device *dev, struct tx_info* ptxInfo, struct tx_info* ptxInfoMask)	//luke
#endif
{
	struct re_private *cp = re_private_data_root.re_private_data_ptr[re_private_data_root.tx_force_gmac_index];
	unsigned int gmac = cp->gmac;
	unsigned entry;
	u32 eor;
	struct tx_info txInfo;
	int ring_num=0;
#ifdef HW_LSO_ENABLE
	int mtu;
#endif

	if(unlikely(gmac >= MAX_GMAC_NUM) || unlikely(cp->gmac_enabled != GMAC_TRUE)) {
		dev_kfree_skb_any(skb);
		TX_TRACE(0, cp->debug_enable, cp->debug_times, "GMAC%d disabled, free skb.\n", gmac);
		return 0;
	}

	GMAC_SPIN_LOCK(&cp->tx_lock);

	memset(&txInfo, 0, sizeof(struct tx_info));
	ETHDBG_PRINT(gmac, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX, "Tx dev=%s nr_frags=%d\n", dev->name, skb_shinfo(skb)->nr_frags);
	SKB_DBG(skb, cp->debug_enable, cp->debug_times, RTL8686_SKB_TX);
	cp->cp_stats.tx_sw_num++;
	checkTXDesc(ring_num, gmac);

	if(unlikely(cp->eth_close == GMAC_TRUE || INVERIFYMODE)
#ifdef TX_CREATE_TEST_PACKET_DEBUG
		|| cp->test_packet_start
#endif
		) 
	{
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;	
		dev_kfree_skb_any(skb);
		GMAC_SPIN_UNLOCK(&cp->tx_lock);
		return 0;
	}

	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= (skb_shinfo(skb)->nr_frags + 1)))
	{
		DEVPRIV(skb->dev)->net_stats.tx_dropped++;
		dev_kfree_skb_any(skb);
		cp->cp_stats.tx_no_desc++;		
		netif_stop_queue(dev);
		kick_tx(gmac, ring_num);
		GMAC_SPIN_UNLOCK(&cp->tx_lock);
		return -1;
	}	

	entry = cp->tx_Mhqhead[ring_num];
	eor = (entry == (cp->re8670_tx_ring_size[ring_num] - 1)) ? RingEnd : 0;
	if (skb_shinfo(skb)->nr_frags == 0) {
		DMA_TX_DESC  *txd = &cp->tx_Mhqring[ring_num][entry];
		u32 len;

		len = skb->len;	

		if(len < RE8686_HW_SMALLEST_DATA_LEN)
		{
			DEVPRIV(skb->dev)->net_stats.tx_dropped++;
			dev_kfree_skb_any(skb);
			cp->cp_stats.tooshort++;
			TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "skb len too short dropped (len=%d).\n", len);
			GMAC_SPIN_UNLOCK(&cp->tx_lock);
			return -1;
		}
		
		// Kaohj --- invalidate DCache before NIC DMA
		dma_cache_wback_inv((unsigned long)skb->data, len);

		//default setting, always need this
		txInfo.addr = CPHYSADDR(skb->data);
		
		//[step1] opts1 set init value
		txInfo.opts1.dw = (eor|len|DescOwn|FirstFrag|LastFrag|TxCRC|IPCS);

		//[step2] opts1 might be changed by some additional setting or parameter ptxInfo
#if 0//def CONFIG_APOLLO_ROMEDRIVER
		if(skb->cb[0]) //from RomeDriver
			_rtk_rg_fwdEngineTxDescSetting((void*)&txInfo,(void*)ptx,(void*)ptxMask);
#endif
		//plz put tx additional setting into this function
		tx_additional_setting(skb, dev, &txInfo);		
		do_txInfoMask(&txInfo, ptxInfo, ptxInfoMask);

		//[step3] HW LSO or jumbo frame decision
#ifdef HW_LSO_ENABLE
		txInfo.opts4.bit.lgsen = 0;
		mtu = re8670_get_mtu(skb, cp);
		if(skb_is_gso(skb) && mtu && (skb->len-14) > mtu)
		{
			txInfo.opts4.bit.lgsen = 1;
			txInfo.opts4.bit.lgmtu = mtu; //real MTU
		}
		else
#endif
		if(skb->len > RE8670_MAX_ETH_FRAME_SIZE)
		{
			if(!cp->tx_jumbo_frame_enabled || (skb->len >= RE8670_MAX_ETH_JUMBO_FRAME_SIZE))
			{
				//memDump(skb->data, (skb->len > NUM2PRINT)?NUM2PRINT : skb->len, "TX SKB LEN > 1522", RTL8686_MEM_DUMP_FORMAT_ALL);
				DEVPRIV(skb->dev)->net_stats.tx_dropped++;
				dev_kfree_skb_any(skb);
				GMAC_SPIN_UNLOCK(&cp->tx_lock);
				return -1;
			}
			else
			{
				txInfo.opts1.dw &= ~(IPCS|L4CS);
			}
		}

		//[step4] support CPU port direct Tx 
		if(txInfo.opts2.bit.tx_portmask&CPU_PORT_MASK_ALL)
		{
			re8686_direct_tx_to_cpu_port(&txInfo);			
		}

#if defined(CONFIG_RTK_L34_ENABLE) && defined(CONFIG_APOLLO_GPON_FPGATEST) 
		//20150703LUKE: filter packet from protocol stack send to virtualmac mapping portmask!
		txInfo.opts3.bit.tx_portmask&=~(_rtk_rg_virtualMAC_with_PON_get());
#endif

		TXINFO_DBG(cp, &txInfo);
		//apply to txdesc
		apply_to_txdesc(txd, &txInfo);
#ifdef TX_RING_DEBUG
		if(cp->tx_ring_backup_debug)
		{
			DMA_TX_DESC  *dbg_txd = &cp->rtl8686_tx_ring_debug[ring_num].txDescriptor[entry];
			memset(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry], 0 , TX_RING_DEBUG_BUFFER_SIZE);
			memcpy(&cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry], skb->data, skb->len);
			apply_to_txdesc(dbg_txd, &txInfo);
			dbg_txd->addr = &cp->rtl8686_tx_ring_debug[ring_num].dataBuffer[entry];
		}
#endif
		cp->tx_skb[ring_num][entry].skb = skb;
		cp->tx_skb[ring_num][entry].mapping = CPHYSADDR(skb->data);
		cp->tx_skb[ring_num][entry].frag = 0;
		entry = NEXT_TX(entry, cp->re8670_tx_ring_size[ring_num]);
	} else {
#ifdef CONFIG_REALTEK_HW_LSO	
		re8670_mFrag_xmit(skb, cp, &entry, ptxInfo, ring_num);
#else
		printk("%s %d: not support frag xmit now\n", __func__, __LINE__);
		dev_kfree_skb_any(skb);
#endif		
	}
	cp->tx_Mhqhead[ring_num] = entry;

#if 0//krammer close this
	if (unlikely(TX_HQBUFFS_AVAIL(cp,ring_num) <= 1))
		netif_stop_queue(dev);
#endif

	//for memory controller's write buffer
	//write_buffer_flush();
	mb();
	cp->cp_stats.tx_hw_num++;

	TX_TRACE(gmac, cp->debug_enable, cp->debug_times, "FROM_PS[%x] DA=%02x:%02x:%02x:%02x:%02x:%02x SA=%02x:%02x:%02x:%02x:%02x:%02x ethtype=%04x len=%d Vlan=%d Pri=%d ExtSpa=%d TxPmsdk=0x%x L34Keep=%x PON_SID=%d\n",
		(u32)skb&0xffff,
		skb->data[0],skb->data[1],skb->data[2],skb->data[3],skb->data[4],skb->data[5],
		skb->data[6],skb->data[7],skb->data[8],skb->data[9],skb->data[10],skb->data[11],
		(skb->data[12]<<8)|skb->data[13],skb->len,
		txInfo.opts2.bit.cvlan_vidh<<8|txInfo.opts2.bit.cvlan_vidl,
		txInfo.opts2.bit.cvlan_prio,
		txInfo.opts3.bit.extspa,
		txInfo.opts2.bit.tx_portmask,
		txInfo.opts3.bit.l34_keep,
		txInfo.opts3.bit.tx_dst_stream_id);
	TXDBG_TIMES_UPDATE(cp);
	dev->trans_start = jiffies;
	netdev_pick_tx(dev, skb, NULL)->trans_start = jiffies;
	sk_pacing_shift_update(skb->sk, 8);
	GMAC_SPIN_UNLOCK(&cp->tx_lock);
#ifdef TX_CREATE_TEST_PACKET_DEBUG
	if(!cp->test_packet_start)
#endif
	{
		kick_tx(gmac, ring_num);
	}

	return 0;
}

__IRAM_NIC int re8670_start_xmit (struct sk_buff *skb, struct net_device *dev)	//shlee temp, fix this later
{
	return re8670_start_xmit_txInfo(skb,dev,NULL,NULL);
}

/* Set or clear the multicast filter for this adaptor.
   This routine is not state sensitive and need not be SMP locked. */
static void __re8670_set_rx_mode (unsigned int gmac)
{
	int rx_mode;

	rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys | AcceptAllPhys;

	/* We can safely update without stopping the chip. */
	// Kao, 2004/01/07
	RLE0787_W32(gmac, MAR0, 0xFFFFFFFF);
	RLE0787_W32(gmac, MAR4, 0xFFFFFFFF);
	RLE0787_W32(gmac, RCR, rx_mode);
}

static void re8670_set_rx_mode (struct net_device *dev)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp;
	unsigned int gmac;

	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		cp = root_cp->re_private_data_ptr[gmac];

		if(cp->gmac_enabled != GMAC_TRUE)
			continue;
		
		//GMAC_SPIN_LOCK(&cp->rx_lock);
		__re8670_set_rx_mode(cp->gmac);
		//GMAC_SPIN_UNLOCK(&cp->rx_lock);
	}
}

static struct rtnl_link_stats64 *re8670_get_stats(struct net_device *dev, struct rtnl_link_stats64 *stats64)
{
	if (unlikely(stats64 == NULL))
		return NULL;

	stats64->rx_packets 	= DEVPRIV(dev)->net_stats.rx_packets;
	stats64->rx_bytes   	= DEVPRIV(dev)->net_stats.rx_bytes;
	stats64->rx_dropped   	= DEVPRIV(dev)->net_stats.rx_dropped;
	stats64->tx_packets 	= DEVPRIV(dev)->net_stats.tx_packets;
	stats64->tx_bytes   	= DEVPRIV(dev)->net_stats.tx_bytes;
	stats64->tx_dropped   	= DEVPRIV(dev)->net_stats.tx_dropped;
	stats64->multicast  	= DEVPRIV(dev)->net_stats.multicast;

#ifdef CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
	rtk_dev_update_stats(dev, stats64);
#endif

	return stats64;
}

static void re8670_init_trx_cdo(struct re_private *cp)
{
	int j;
	for(j=0;j<MAX_TXRING_NUM;j++){
		cp->tx_Mhqhead[j] = cp->tx_Mhqtail[j] = 0;
	}

	for(j=0;j<MAX_RXRING_NUM;j++){
		cp->rx_Mtail[j] = 0;
	}
}

static void re8670_stop_hw (struct re_private *cp)
{
	unsigned int gmac=cp->gmac;
	int j;
	RLE0787_W32(gmac, IO_CMD, 0); /* timer  rx int 1 packets*/
	RLE0787_W32(gmac, IO_CMD1, 0); //czyao
	RLE0787_W16(gmac, IMR, 0);
	RLE0787_W32(gmac, IMR0, 0);
	RLE0787_W16(gmac, ISR, 0xffff);
	RLE0787_W32(gmac, ISR1, 0xffffffff);
	//synchronize_irq();
	synchronize_irq(cp->irq);/*linux-2.6.19*/
	udelay(10);

	for(j=0;j<MAX_RXRING_NUM;j++)
		cp->rx_Mtail[j] = 0;		
	for(j=0;j<MAX_TXRING_NUM;j++)
		cp->tx_Mhqhead[j] = cp->tx_Mhqtail[j] = 0;
}

//static unsigned int re8670_get_irq_number (struct re_private *cp)
//{
//	unsigned int gmac=cp->gmac;	
//	return (gmac<MAX_GMAC_NUM) ? cp->irq : 0;		
//}

static void re8670_ip_enable_all(void)
{
	REG32(BSP_IP_SEL) |= BSP_EN_GMAC;
#ifdef CONFIG_GMAC1_USABLE
	REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC1;
#endif
#ifdef CONFIG_GMAC2_USABLE
	REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC2;
#endif
}

static void re8670_ip_enable(unsigned int gmac)
{
	if(gmac == 0)
	{
		REG32(BSP_IP_SEL) |= BSP_EN_GMAC;
	}
#ifdef CONFIG_GMAC1_USABLE
	else if(gmac == 1)
	{
		REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC1;
	}
#endif
#ifdef CONFIG_GMAC2_USABLE
	else if(gmac == 2)
	{
		REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC2;
	}
#endif
}

static void re8670_reset_hw (struct re_private *cp)
	{
	unsigned int gmac = cp->gmac;
	
	/* After apollo use this for totaly gmac reset
	, in old method, mring can't receive packet at first time packet coming */
	disable_irq(cp->irq);
	switch(gmac) {
		case 0:
			REG32(BSP_IP_SEL) &= ~BSP_EN_GMAC;
			mdelay(10);
			REG32(BSP_IP_SEL) |= BSP_EN_GMAC;
			break;
		case 1:
			REG32(NEW_BSP_IP_SEL) &= ~BSP_EN_GMAC1;
			mdelay(10);
			REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC1;
			break;
		case 2:
			REG32(NEW_BSP_IP_SEL) &= ~BSP_EN_GMAC2;
			mdelay(10);
			REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC2;
			break;
	}
	enable_irq(cp->irq);
}

static inline void re8670_start_hw (struct re_private *cp)
{
	unsigned int gmac=cp->gmac;

	RLE0787_W32(gmac, IO_CMD1, cp->iocmd1_reg);
	RLE0787_W32(gmac, IO_CMD, cp->iocmd_reg); /* timer  rx int 1 packets*/
	//printk("Start NIC in Pkt Processor disabled mode.. IO_CMD %x\n", RLE0787_R32(gmac, IO_CMD));	
}

int re8686_set_flow_control(unsigned int gmac, unsigned int ring, unsigned char enable)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	u32 reg32_val, ring_size, ring0_size_msk;
	u16 desc_l, desc_h;
	u16 reg16_val;
	#ifndef CONFIG_GMAC1_USABLE
	if (gmac==1)
		goto error;
	#endif 
	#ifndef CONFIG_GMAC2_USABLE
	if (gmac==2)
		goto error;
	#endif 
	#ifdef CONFIG_RTK_SINGLE_RX_RING
	if (ring >= MAX_RXRING_NUM)
		goto error;
	#endif
	
	if(enable)
	{
		ring_size = (cp->re8670_rx_ring_size[ring]);
	}
	else
	{
		ring_size = ((cp->re8670_rx_ring_size[ring])<<1);
	}
#ifndef CONFIG_RTK_SINGLE_RX_RING
	if(ring==0)
#endif
	{
		ring0_size_msk = ring_size-1;
		desc_l = ring0_size_msk&0xff;
		desc_h = (ring0_size_msk&0xf00)>>8;
		reg32_val = (desc_l<<24)|((desc_h)<<4)|(RLE0787_R32(gmac, EthrntRxCPU_Des_Num)&0x00ffff0f);
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num, reg32_val);
		reg32_val = (desc_l<<8)|desc_h|(RLE0787_R32(gmac, RxCDO)&0xffff00f0);
		RLE0787_W32(gmac, RxCDO, reg32_val);		
	}
#ifndef CONFIG_RTK_SINGLE_RX_RING
	else
	{
		reg32_val = ((ring_size&0xfff)-1)|(RLE0787_R32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(ring-1)))&0xfffff000);
		RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(ring-1)), reg32_val);
		reg16_val = ((ring_size&0xfff)-1);
		RLE0787_W16(gmac, RxRingSize2+(ADDR_OFFSET*(ring-1)), reg16_val);
	}
#endif
	cp->re8670_rx_flow_control_status[ring] = enable?GMAC_ON:GMAC_OFF;
	return 0;
#if !defined(CONFIG_GMAC1_USABLE) || !defined(CONFIG_GMAC2_USABLE) || defined(CONFIG_RTK_SINGLE_RX_RING)
error:
	//printk("%s(%d): ignored gmac=%d ring=%u\n",__func__,__LINE__,gmac,ring);
	return -1;
#endif
}

static void multi_rtx_ring_init(struct re_private *cp)
{
	unsigned int gmac=cp->gmac;
	u32 reg32_val, ring0_size_msk;
	u16 desc_l, desc_h;
	int i;
	
	for(i=0;i<MAX_TXRING_NUM;i++)
	{		
		RLE0787_W32(gmac, TxFDP1+(ADDR_OFFSET*i), CPHYSADDR(cp->tx_Mhqring[idx_sw2hw(i)]));
		RLE0787_W16(gmac, TxCDO1+(ADDR_OFFSET*i), 0);
	}
	for(i=0;i<MAX_RXRING_NUM;i++)
	{			
		/*we set flow control even if we don't enable this queue.........
		this is because we want to prevent triggering flow control of the queue we disable.....*/
		if(i==0)
		{
			ring0_size_msk = (cp->re8670_rx_ring_size[i])-1;
			desc_l = ring0_size_msk&0xff;
			desc_h = (ring0_size_msk&0xf00)>>8;
			RLE0787_W32(gmac, RxFDP, CPHYSADDR(cp->rx_Mring[0]));
			reg32_val = (desc_l<<24)|((TH_ON_VAL&0xff)<<16)|((TH_OFF_VAL&0xff)<<8)|((desc_h)<<4)|((TH_ON_VAL&0xf00)>>8);
			RLE0787_W32(gmac, EthrntRxCPU_Des_Num, reg32_val);
			reg32_val = (desc_l<<8)|desc_h;
			RLE0787_W32(gmac, RxCDO, reg32_val);
			reg32_val = ((TH_OFF_VAL&0xf00)<<16)|(RLE0787_R32(gmac, Rx_Pse_Des_Thres_h)&0xf0ffffff);
			RLE0787_W32(gmac, Rx_Pse_Des_Thres_h, reg32_val);
		}
		else
		{
			RLE0787_W32(gmac, RxFDP2+(ADDR_OFFSET*(i-1)), CPHYSADDR(cp->rx_Mring[i]));
			RLE0787_W32(gmac, EthrntRxCPU_Des_Wrap2+(ADDR_OFFSET*(i-1)), 0);  //initialize the register content
			RLE0787_W32(gmac, EthrntRxCPU_Des_Wrap2+(ADDR_OFFSET*(i-1)), ((TH_ON_VAL&0xfff)<<16)|(TH_OFF_VAL&0xfff));
			RLE0787_W32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(i-1)), (cp->re8670_rx_ring_size[i]&0xfff)-1);
			RLE0787_W16(gmac, RxRingSize2+(ADDR_OFFSET*(i-1)), (cp->re8670_rx_ring_size[i]&0xfff)-1);
		}
	}
	// set queue 456 to default setting , otherwise the eth0 can't rx packet (due to gmac flow control)
	re8686_set_flow_control(gmac, 0, cp->re8670_rx_flow_control_status[0]);
#ifndef CONFIG_RTK_SINGLE_RX_RING
	re8686_set_flow_control(gmac, 1, cp->re8670_rx_flow_control_status[1]);
	re8686_set_flow_control(gmac, 2, cp->re8670_rx_flow_control_status[2]);
	re8686_set_flow_control(gmac, 3, cp->re8670_rx_flow_control_status[3]);
	re8686_set_flow_control(gmac, 4, cp->re8670_rx_flow_control_status[4]);
	re8686_set_flow_control(gmac, 5, cp->re8670_rx_flow_control_status[5]);
#endif

}

static void re8670_init_hw (struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	u8 status;
	// Kao
	u32 *hwaddr, cputag_info;
	//unsigned int value;

	re8670_reset_hw(cp);

	RLE0787_W8(gmac, CMD, RxChkSum|RxJumboSupport);	 /* checksum */		//shlee 8672
//#ifdef CONFIG_ETHWAN
//	vlan_detag(GMAC_ON);//krammer: default de-tag from skb into desc
//#endif
	vlan_detag(gmac, GMAC_OFF);
	RLE0787_W16(gmac, ISR, 0xffff);/*clear all interrupt*/
	RLE0787_W32(gmac, ISR1, 0xffffffff);/*clear all interrupt*/
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
	RLE0787_W16(gmac, IMR,RX_ALL(gmac) | TOK); 
#else
	RLE0787_W16(gmac, IMR,RX_ALL(gmac)); 
#endif
	UNMASK_IMR0_RXALL(gmac);
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
	UNMASK_IMR0_TXALL(gmac);
#endif
	// Kao
	//20170502: disable gmac padding by default
	RLE0787_W32(gmac, TCR,(u32)(0x0C01));	
	RLE0787_W32(gmac, CPUtagCR,(u32)(0x0000));  /* Turn off CPU tag function */
	//cpu tag function
	cputag_info = (CTEN_RX | 2<<CT_RSIZE_L | 2<<CT_TSIZE | CT_APPLO_PRO | CTPM_8370 | CTPV_8370);
	RLE0787_W32(gmac, CPUtagCR,cputag_info); /* Turn on the cpu tag adding function */  //czyao 8672c
	RLE0787_W32(gmac, CPUtag1CR, (CT1_SID));	

	multi_rtx_ring_init(cp);

	status = RLE0787_R8(gmac, MSR);
	status = status | (TXFCE|FORCE_TX);	// enable tx flowctrl
	status = status | RXFCE;
	RLE0787_W8(gmac, MSR, status);	
	// Kao, set hw ID for physical match
	hwaddr = (u32 *)ROOTDEV->dev_addr;
	RLE0787_W32(gmac, IDR0, *hwaddr);	
	hwaddr = (u32 *)(ROOTDEV->dev_addr+4);
	RLE0787_W32(gmac, IDR4, *hwaddr);	
	
	RLE0787_W32(gmac, CONFIG_REG, Rff_size_sel_2k);	
	en_rx_mring_int_split(gmac);
	config_rx_sideband(gmac, GMAC_ON);
	set_rring_route(gmac);
	
	re8670_start_hw(cp);
	__re8670_set_rx_mode(cp->gmac);
}

static int re8670_refill_rx (struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	unsigned int i, j;	

	for(j=0;j<MAX_RXRING_NUM;j++)
	{
		for (i = 0; i < cp->re8670_rx_ring_size[j]; i++) {
			struct sk_buff *skb;
			skb = re8670_getAlloc(SKB_BUF_SIZE);
			if (!skb)
				goto err_out;
			// Kaohj --- invalidate DCache for uncachable usage
			//ql_xu
			dma_cache_wback_inv((unsigned long)skb->data, SKB_BUF_SIZE);
			skb->dev = ROOTDEV;
			cp->rx_skb[j][i].skb = skb;
			cp->rx_skb[j][i].frag = 0;
			if ((u32)skb->data &0x3)
				printk(KERN_DEBUG "skb->data unaligment %8x\n",(u32)skb->data);

			cp->rx_Mring[j][i].addr = (u32)skb->data|UNCACHE_MASK;      

			if (i == (cp->re8670_rx_ring_size[j] - 1))          
				cp->rx_Mring[j][i].opts1 = (DescOwn | RingEnd | SKB_BUF_SIZE);
			else
				cp->rx_Mring[j][i].opts1 =(DescOwn | SKB_BUF_SIZE);
			cp->rx_Mring[j][i].opts2 = 0;

		}    
	}

	return 0;
err_out:
	re8670_clean_rings(cp);
	return -ENOMEM;
}

static void re8670_tx_timeout (struct net_device *dev)
{
	struct re_private_root *root_cp = DEV2CP(dev);
	struct re_private *cp;
	unsigned int gmac;
#ifdef TX_WATCHDOG_TIMEOUT_RESET
	int32   ret;
	extern  int32 rtk_switch_core_reset(void);
#endif
	
	printk("%s %d enter dev=%s\n", __func__, __LINE__, dev->name);
	
#ifdef TX_WATCHDOG_TIMEOUT_RESET
	if (RT_ERR_OK != (ret = rtk_switch_core_reset()))
		printk("%s:%d rtk_switch_core_reset fail!\n",__FUNCTION__,__LINE__);
	else
		printk("%s:%d rtk_switch_core_reset ok!\n",__FUNCTION__,__LINE__);

	re8670_reset();
#endif
	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		cp = root_cp->re_private_data_ptr[gmac];
		cp->cp_stats.tx_timeouts++;		
	}
	if (netif_queue_stopped(dev))
		netif_wake_queue(dev);

	printk("%s %d exit\n", __func__, __LINE__);
}

static int re8670_init_rings (struct re_private *cp)
{
	int j;
	for(j=0;j<MAX_TXRING_NUM;j++){
		cp->tx_Mhqhead[j] = cp->tx_Mhqtail[j] = 0;
	}	

	for(j=0;j<MAX_RXRING_NUM;j++){
		cp->rx_Mtail[j] = 0;		
	}	
	return re8670_refill_rx (cp);
}

static int re8670_alloc_rings (struct re_private *cp)
{
	unsigned int gmac;
	void*	pBuf;
	int j;
	
	gmac = cp->gmac;
	for(j=0;j<MAX_RXRING_NUM;j++)
	{    
		pBuf = kzalloc(RE8670_RXRING_BYTES(cp->re8670_rx_ring_size[j]), GFP_KERNEL);	
		if (!pBuf)
			goto ErrMem;

		dma_cache_wback_inv((unsigned long)pBuf, RE8670_RXRING_BYTES(cp->re8670_rx_ring_size[j]));
		cp->rxdesc_Mbuf[j] = pBuf;

		pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ;
		cp->rx_Mring[j] = (DMA_RX_DESC*)((u32)(pBuf) | UNCACHE_MASK);
		if(cp->re8670_rx_ring_size[j]) {
			pBuf=(struct ring_info*)kzalloc(sizeof(struct ring_info)*cp->re8670_rx_ring_size[j],GFP_KERNEL);			
			if (!pBuf)
				goto ErrMem;

			cp->rx_skb[j]= pBuf;
		}
	}

	for(j=0;j<MAX_TXRING_NUM;j++)
	{
		if(!cp->re8670_tx_ring_size[j])
			continue;

		pBuf = kzalloc(RE8670_TXRING_BYTES(cp->re8670_tx_ring_size[j]), GFP_KERNEL);		
		if (!pBuf)
			goto ErrMem;

		dma_cache_wback_inv((unsigned long)pBuf, RE8670_TXRING_BYTES(cp->re8670_tx_ring_size[j]));
		cp->txdesc_Mbuf[j] = pBuf;
		pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ;
		cp->tx_Mhqring[j] = (DMA_TX_DESC*)((u32)(pBuf) | UNCACHE_MASK);
		pBuf =(struct ring_info*)kzalloc(sizeof(struct ring_info)*cp->re8670_tx_ring_size[j],GFP_KERNEL);		
		if (!pBuf)
			goto ErrMem;
		cp->tx_skb[j] = pBuf;
	}

	return re8670_init_rings(cp);

ErrMem:

	for(j=0;j<MAX_RXRING_NUM;j++)
	{
		if (cp->rx_skb[j])    
			kfree(cp->rx_skb[j]);
	}        

	for(j=0;j<MAX_TXRING_NUM;j++)
	{        
		if (cp->tx_skb[j])    
			kfree(cp->tx_skb[j]);	
	}
	
	return -ENOMEM;

}

static void re8670_clean_rings (struct re_private *cp)
{
	unsigned int gmac = cp->gmac;
	unsigned i,j;
	
	for (j = 0; j < MAX_RXRING_NUM; j++) {
		if(cp->rx_skb[j]){
			for (i = 0; i < cp->re8670_rx_ring_size[j]; i++) {
				if (cp->rx_skb[j][i].skb) {
					dev_kfree_skb(cp->rx_skb[j][i].skb);
				}
			}
			memset(cp->rx_skb[j], 0, sizeof(struct ring_info) * cp->re8670_rx_ring_size[j]);		
		}
	}
	for (j = 0; j < MAX_TXRING_NUM; j++) {
		if(cp->tx_skb[j]){
			for (i = 0; i < cp->re8670_tx_ring_size[j]; i++) {
				if (cp->tx_skb[j][i].skb) {
					struct sk_buff *skb = cp->tx_skb[j][i].skb;
					if((u32)skb!=0xffffffff){
						DEVPRIV(skb->dev)->net_stats.tx_dropped++;
						dev_kfree_skb(skb);
					}
				}
			}
			memset(cp->tx_skb[j], 0, sizeof(struct ring_info) * cp->re8670_tx_ring_size[j]);	
		}
	}
}

static void re8670_free_rings (struct re_private *cp)
{
	int j;
	re8670_clean_rings(cp);
	/*pci_free_consistent(cp->pdev, CP_RING_BYTES, cp->rx_ring, cp->ring_dma);*/

	for(j=0;j<MAX_RXRING_NUM;j++)
	{
		if (cp->rxdesc_Mbuf[j]) {
			kfree(cp->rxdesc_Mbuf[j]);
			cp->rxdesc_Mbuf[j] = NULL;
		}
		
		cp->rx_Mring[j] = NULL;   

		if (cp->rx_skb[j]) {   			
			kfree(cp->rx_skb[j]);
			cp->rx_skb[j] = NULL;
		}
		cp->rx_skb[j]=NULL;
	}     

	for(j=0;j<MAX_TXRING_NUM;j++)        
	{
		if (cp->txdesc_Mbuf[j]) {
			kfree(cp->txdesc_Mbuf[j]);
			cp->txdesc_Mbuf[j] = NULL;
		}
		
		cp->tx_Mhqring[j] = NULL;

		if (cp->tx_skb[j])   {			
			kfree(cp->tx_skb[j]);
			cp->tx_skb[j] = NULL;
		}
		cp->tx_skb[j]=NULL;	
	}    

}

static int re8670_open (struct net_device *dev)
{
	struct re_private_root *root_cp = DEV2CP(dev);
	struct re_private *cp;
	unsigned int gmac;
	int rc=0;

	if (netif_msg_ifup(root_cp))
		printk(KERN_DEBUG "%s: enabling interface\n", dev->name);

	if(dev_num == 0) {
		for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
		{
			cp = root_cp->re_private_data_ptr[gmac];
			printk("%s %d\n", __func__, __LINE__);
			if(cp->gmac_enabled == GMAC_TRUE) 
			{
				rtk_gmac_set_rxbufsize(root_cp);	/* set new rx buf size */
#ifdef CONFIG_RTK_WFOAX
				if (gmac != WFO_GMAC_NO)
#endif
				{
				re8670_init_trx_cdo(cp);
				//rc = request_irq(dev->irq, (irq_handler_t)re8670_interrupt, IRQF_DISABLED, dev->name, dev);
				rc = request_irq(cp->irq, (irq_handler_t)re8670_interrupt, 0, root_cp->dev->name, cp);
				if (rc)
					goto err_out_hw;
				}
#ifdef RX_NAPI_MODE
				//netif_napi_add(root_cp->dev, &cp->napi, re8670_poll, cp->napi_budget);
				napi_enable(&cp->napi);
#endif
#ifdef CONFIG_RTK_WFOAX
				if (gmac != WFO_GMAC_NO)
#endif
				re8670_init_hw(cp);
				if(cp->stag_pid)
					RLE0787_R32(gmac, VLAN_REG) = cp->stag_pid;
				if(cp->stag_pid1)
					RLE0787_R32(gmac, VLAN1_REG) = cp->stag_pid1;

#ifdef TX_RECYCLE_SKB_USING_POLLING
				if(timer_pending(&cp->tok_polling_timer))
					del_timer(&cp->tok_polling_timer);	
				init_timer(&cp->tok_polling_timer);
				cp->tok_polling_timer.function = re8670_tx_all;
				cp->tok_polling_timer.data = cp;
				mod_timer(&cp->tok_polling_timer, jiffies+1);
#endif
			}
		}
	}

	dev_num++;
	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		cp = root_cp->re_private_data_ptr[gmac];
		
		if(cp->gmac_enabled != GMAC_TRUE)
			continue;
		
		cp->eth_close = GMAC_FALSE;
	}
#ifdef CONFIG_AUTO_DHCP_CHECK
	cp = root_cp->re_private_data_ptr[0];
	if(cp->gmac_enabled == GMAC_TRUE) {
	    int num = 0;
        struct net_device *dev_tmp = NULL;
	    for (num=0; num<SW_PORT_NUM; num++)
	    {
			dev_tmp = cp->port2dev[num];
            if (dev_tmp && (dev==dev_tmp)) {
            	//rtk_port_adminEnable_set(num, 1);
            	rtk_port_phyPowerDown_set(num, 0);
            	printk("%s[%d], num=%d\n", __FUNCTION__, __LINE__, num);
            }
	    }
	}
#endif
	netif_start_queue(dev);

	return 0;

err_out_hw:	
	return rc;
}

static int re8670_close (struct net_device *dev)
{
	struct re_private_root *root_cp = DEV2CP(dev);
	struct re_private 	   *cp;
	unsigned int gmac;

	dev_num--;	

	if(dev_num == 0)
	{
		netif_stop_queue(dev);
		for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
		{
			cp = root_cp->re_private_data_ptr[gmac];
			if(cp->gmac_enabled == GMAC_TRUE) 
			{			
				cp->eth_close = GMAC_TRUE;
				re8670_stop_hw(cp);

				printk("%s %d irq=%d name=%s\n", __func__, __LINE__, cp->irq, root_cp->dev->name);
				free_irq(cp->irq, cp);	
#ifdef RX_NAPI_MODE
				//netif_napi_add(root_cp->dev, &cp->napi, re8670_poll, cp->napi_budget);
				napi_disable(&cp->napi);
#endif
				printk("%s %d\n", __func__, __LINE__);
				cp->stag_pid = RLE0787_R32(cp->gmac, VLAN_REG);
				cp->stag_pid1 = RLE0787_R32(cp->gmac, VLAN1_REG);
			}
		}
	}
#ifdef CONFIG_AUTO_DHCP_CHECK	
	cp = root_cp->re_private_data_ptr[0];
	if(cp->gmac_enabled == GMAC_TRUE) 
	{		
		int num = 0;
		struct net_device *dev_tmp = NULL;

		for (num=0; num<SW_PORT_NUM; num++)
		{
			dev_tmp = cp->port2dev[num];
			if (dev_tmp && (dev==dev_tmp)) {
				//rtk_port_adminEnable_set(num, 0);
				rtk_port_phyPowerDown_set(num, 1);
				printk("%s[%d], num=%d\n", __FUNCTION__, __LINE__, num);
			}
		}
	}
#endif	
	if (netif_msg_ifdown(root_cp))
		printk(KERN_DEBUG "%s: disabling interface\n", dev->name);

	return 0;
}

int re8670_ioctl (struct net_device *dev, struct ifreq *rq, int cmd)
{
	int rc = 0;

	if (!netif_running(dev) && cmd!=SIOCETHTEST)
		return -EINVAL;

	switch (cmd) {
#if 1
		case SIOCETHTEST:
			eth_ctl((struct eth_arg *)rq->ifr_data);
			break;
#endif
#ifdef CONFIG_RTK_HOST_SPEEDUP
        case SIOCSPEEDTEST:                                        
            speedtest_ctl(rq->ifr_data);
            break;
#endif
		default:
			rc = -EOPNOTSUPP;
			break;
	}

	return rc;
}

static int dbg_level_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;	
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	if (buf && !copy_from_user(tmpBuf, buf, len))
	{
		data->debug_enable=simple_strtoul(tmpBuf, NULL, 16);
		printk("write debug_enable to 0x%08x\n", data->debug_enable);
		return count;
	}
	return -EFAULT;
}

static int dbg_times_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	unsigned char tmpBuf[16] = {0};
	int len = (count > 15) ? 15 : count;	
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	if (buf && !copy_from_user(tmpBuf, buf, len))
	{
		data->debug_times=simple_strtoul(tmpBuf, NULL, 16);
		printk("write debug_times to %d times\n", data->debug_times);
		return count;
	}
	return -EFAULT;
}

static int memrw_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	return 0;
}

static int memrw_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 		tmpbuf[64];
	unsigned int	*mem_addr, mem_data, mem_len;
	char		*strptr, *cmd_addr;
	char		*tokptr;
	unsigned int ring_num, i;
	unsigned int start_idx, end_idx;
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
		{
			goto errout;
		}
		printk("cmd %s\n", cmd_addr);
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}

#ifdef TX_RING_DEBUG
		if (!memcmp(cmd_addr, "txr", 3))
		{
			if(!data->tx_ring_backup_debug)
			{
				printk("%s %d invalid tx_ring_backup_debug !\n", __func__, __LINE__);
				goto errout;
			}
			ring_num=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			start_idx=simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			end_idx=simple_strtol(tokptr, NULL, 0);
			if(ring_num >= MAX_TXRING_NUM)
			{
				printk("%s %d invalid ring_num !\n", __func__, __LINE__);
				goto errout;
			}
			if(start_idx >= re_private_data[gmac].re8670_tx_ring_size[ring_num])
			{
				printk("%s %d invalid start_idx !\n", __func__, __LINE__);
				goto errout;
			}
			if(end_idx >= re_private_data[gmac].re8670_tx_ring_size[ring_num])
			{
				printk("%s %d invalid end_idx !\n", __func__, __LINE__);
				goto errout;
			}
			for(i=start_idx ; i<=end_idx ; i++)
			{
				printk("########################ring[%d] [i=%d]########################", ring_num, i);
				if(((DMA_TX_DESC *)&re_private_data[gmac].rtl8686_tx_ring_debug[ring_num].txDescriptor[i])->opts1&0x80000000)

				memDump(&re_private_data[gmac].rtl8686_tx_ring_debug[ring_num].dataBuffer[i], ((DMA_TX_DESC *)&re_private_data[gmac].rtl8686_tx_ring_debug[ring_num].txDescriptor[i])->opts1&0x1ffff, "", RTL8686_MEM_DUMP_FORMAT_ALL);
				printk("#################################################################\n");
			}
		}
		else
#endif
		if(!memcmp(cmd_addr, "r_addr_hex_text", 15))
		{
			mem_addr=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_len=simple_strtol(tokptr, NULL, 0);
			memDump(mem_addr, mem_len, "", RTL8686_MEM_DUMP_FORMAT_ALL);
		}
		else if (!memcmp(cmd_addr, "r_addr_hex", 10))
		{
			mem_addr=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_len=simple_strtol(tokptr, NULL, 0);
			memDump(mem_addr, mem_len, "", RTL8686_MEM_DUMP_FORMAT_ADDR | RTL8686_MEM_DUMP_FORMAT_HEX);
		}
		else if (!memcmp(cmd_addr, "r_hex", 5))
		{
			mem_addr=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_len=simple_strtol(tokptr, NULL, 0);
			memDump(mem_addr, mem_len, "", RTL8686_MEM_DUMP_FORMAT_HEX);
		}
		else if (!memcmp(cmd_addr, "r_text", 5))
		{
			mem_addr=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_len=simple_strtol(tokptr, NULL, 0);
			memDump(mem_addr, mem_len, "", RTL8686_MEM_DUMP_FORMAT_TEXT);
		}
		else if (!memcmp(cmd_addr, "r", 1))
		{
			mem_addr=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_len=simple_strtol(tokptr, NULL, 0);
			memDump(mem_addr, mem_len, "", RTL8686_MEM_DUMP_FORMAT_ALL);
		}
		else if (!memcmp(cmd_addr, "w", 1))
		{
			mem_addr=(unsigned int*)simple_strtol(tokptr, NULL, 0);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			mem_data=simple_strtol(tokptr, NULL, 0);
			WRITE_MEM32(mem_addr, mem_data);
			printk("Write memory 0x%p dat 0x%x: 0x%x\n", mem_addr, mem_data, READ_MEM32(mem_addr));
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("Memory operation only support \"r\" and \"w\" as the first parameter\n");
		printk("Read format:	\"r mem_addr length\"\n");
		printk("Write format:	\"w mem_addr mem_data\"\n");
#ifdef TX_RING_DEBUG
		printk("Tx ring debug data buffer format:	\"txr ring_num start_idx end_idx\"\n");
#endif
	}
	return count;
}
#ifdef CONFIG_RTL8686_SWITCH
#ifndef CONFIG_RTK_L34_ENABLE
static int switch_mode_read(struct seq_file *seq, void *v)
{	  
	  switch(SWITCH_MODE)
	  {
		case RTL8686_Switch_Mode_Trap2Cpu:
			printk("Asic switch mode : trap2cpu\n");
			break;
		case RTL8686_Switch_Mode_Normal:
			printk("Asic switch mode : normal\n");
			break;
		default:
			printk("Asic switch mode : Unknown\n");
	  }      
      return 0;
}
static int switch_control_set_mode(int mode)
{
	if( mode!=RTL8686_Switch_Mode_Trap2Cpu
		&& mode!=RTL8686_Switch_Mode_Normal)
		return -1;	
	SWITCH_MODE = mode;
	return 0;
}
#define SWITCH_PORT_NUM 5
static int switch_normal(void)
{
	int ret = RT_ERR_FAILED;
	rtk_acl_ingress_entry_t aclRule;
	memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t)); 
    aclRule.index = 0;	
	if((ret = rtk_acl_igrRuleEntry_del(aclRule.index))!= RT_ERR_OK)
	{
		printk("%s-%d error rtk_acl_igrRuleEntry_del index %d\n",__func__,__LINE__,aclRule.index);
		return ret; 	
	}
	return 0;
}
static int switch_trap2cpu(void)
{
	int ret = RT_ERR_FAILED;
	rtk_acl_ingress_entry_t aclRule;	
	unsigned int port;
	memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t)); 
	//default set to acl index 0, if set trap2cpu
    aclRule.index = 0;	
    aclRule.templateIdx = 0;
	//aclRule.activePorts.bits[0] = 1 << 0; /* port 0*/
   	for(port=0;port < SWITCH_PORT_NUM ; port ++)
		aclRule.activePorts.bits[0] |= (1 << port);
	aclRule.valid = PADDING_ENABLED;
	aclRule.act.enableAct[ACL_IGR_FORWARD_ACT]= PADDING_ENABLED;
	aclRule.act.forwardAct.act = ACL_IGR_FORWARD_TRAP_ACT;
	if((ret = rtk_acl_igrRuleEntry_add(&aclRule))!= RT_ERR_OK) 
	{
		printk("%s-%d error rtk_acl_igrRuleEntry_add index %d\n",__func__,__LINE__,aclRule.index);
		return ret; 
	}
   	for(port=0;port < SWITCH_PORT_NUM ; port ++)
   	{ 
		rtk_acl_igrState_set(port,PADDING_ENABLED);
	}
	return 0;
}
static int switch_mode_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 	tmpbuf[512];
	char		*strptr;	
	int retval = -1;
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;	
	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		if(strlen(strptr)==0)
		{
			goto errout;
		}
		if(strncmp(strptr, "trap2cpu",8) == 0)
		{
			retval = switch_control_set_mode(RTL8686_Switch_Mode_Trap2Cpu);
			vlan_detag(gmac, GMAC_ON);
			retval = switch_trap2cpu();
#ifdef CONFIG_RTL_MULTI_LAN_DEV			
			change_dev_port_mapping(LAN_PORT1,"lan0");
			change_dev_port_mapping(LAN_PORT2,"lan1");
			change_dev_port_mapping(LAN_PORT3,"lan2");
			change_dev_port_mapping(LAN_PORT4,"lan3");
			change_dev_port_mapping(LAN_PORT5,"lan4");
			change_dev_port_mapping(LAN_PORT6,"lan5");
			change_dev_port_mapping(WAN_PORT,"eth1");
			#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
			change_dev_port_mapping(LAN_PORT6,"ifprobe");
			#endif
#endif						
		}
		else if(strncmp(strptr, "normal",6) == 0)
		{
			retval = switch_control_set_mode(RTL8686_Switch_Mode_Normal);
			vlan_detag(gmac, GMAC_OFF);
			retval = switch_normal();
#ifdef CONFIG_RTL_MULTI_LAN_DEV			
			change_dev_port_mapping(LAN_PORT1,"lan0");
			change_dev_port_mapping(LAN_PORT2,"lan1");
			change_dev_port_mapping(LAN_PORT3,"lan2");
			change_dev_port_mapping(LAN_PORT4,"lan3");
			change_dev_port_mapping(LAN_PORT5,"lan4");
			change_dev_port_mapping(LAN_PORT6,"lan5");
			change_dev_port_mapping(WAN_PORT,"eth1");
			#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
			change_dev_port_mapping(LAN_PORT6,"ifprobe");
			#endif
#endif	
		}
		else
		{
			goto errout;
		}
		if(retval==0)
			printk("write success ! \n");
		else
			printk("error : change mode fail ! \n");
	}
	else
	{
errout:
		printk("error input  (trap/normal)\n");
	}
	return count;
}
#endif //CONFIG_RTK_L34_ENABLE

static u32 rtl_ethtool_get_port_link(struct net_device *dev)
{
	//unsigned int i;
	u32 txportmask = DEVPRIV(dev)->txPortMask;
	rtk_port_linkStatus_t LinkStatus;
	int portnum;

	if(txportmask == 0){//eth0, always link, have problem?
		return 1;
	}
#if 0
	for(i=0;i<SW_PORT_NUM;i++){
		if(txportmask & (1<<i)){
			//return get_port_link(i);//first match bit
			return 1;
		}
	}
#endif
	//krammer change to default link up
	//return 0;//default link down
	for(portnum = 0; portnum < SW_PORT_NUM; ++portnum)
	{
		if(txportmask & (1 << portnum))
			break;
	}
		
	if(rtk_port_link_get(portnum, &LinkStatus) != RT_ERR_OK){
		printk("\n %s %d\n", __FUNCTION__, __LINE__);
	}
#if 0
	if (0 == LinkStatus)
		printk("port %d is down.\n", portnum);
	else if (1 == LinkStatus)
		printk("port %d is up.\n", portnum);
	else
		printk("port %d is chaos\n", portnum);
#endif
	return LinkStatus;//default link down
}



static int rtl_ethtool_get_settings(struct net_device *dev, struct ethtool_cmd *ecmd)
{
#if 0
	unsigned int  portnum;
	unsigned int  tmp_spd;

    apollo_raw_port_ability_t Ability;

	u32 txportmask = DEVPRIV(dev)->txPortMask;

	if(0 == txportmask)
		return 0;

	//we only treat the first match bit;
	for(portnum = 0; portnum < SW_PORT_NUM; ++portnum)
	{
		if(txportmask & (1 << portnum))
			break;
	}

	if(portnum > SW_PORT_NUM)
		return 0;

	apollo_raw_port_ability_get(portnum, &Ability);

	ecmd->supported = SUPPORTED_10baseT_Half 		
							|	SUPPORTED_10baseT_Full 	
							|	SUPPORTED_100baseT_Half 	
							|	SUPPORTED_100baseT_Full
							|	SUPPORTED_1000baseT_Full
							|	SUPPORTED_Autoneg 	
							|	SUPPORTED_Pause
							|	((PON_PORT == portnum) ? SUPPORTED_FIBRE : SUPPORTED_TP );
		                   
	ecmd->advertising = ADVERTISED_10baseT_Half 
							|	ADVERTISED_10baseT_Full
							|	ADVERTISED_100baseT_Full
							|	ADVERTISED_100baseT_Half
							|	ADVERTISED_Autoneg
							|	ADVERTISED_Pause
							|	((PON_PORT == portnum) ? SUPPORTED_FIBRE : SUPPORTED_TP );

	if(PON_PORT == portnum)
		ecmd->port = PORT_FIBRE;
	else
		ecmd->port = PORT_TP;

	ecmd->autoneg = (Ability.nwayAbility) ? AUTONEG_ENABLE : AUTONEG_DISABLE;

	ecmd->duplex = Ability.duplex ? DUPLEX_FULL : DUPLEX_HALF;

	ecmd->transceiver = XCVR_INTERNAL;

	ecmd->phy_address = portnum;

	if(PON_PORT == portnum)
		tmp_spd = Ability.linkFib1g ? SPEED_1000 : SPEED_100;
	else
		tmp_spd = (PORT_SPEED_100M == Ability.speed) ? SPEED_100 : 
						((PORT_SPEED_10M == Ability.speed) ? SPEED_10 : SPEED_1000);

	//if the port is not linked, we set the speed as "UNKOWN!".
	if(rtl_ethtool_get_port_link(dev))
		ecmd->speed = tmp_spd;
	else
		ecmd->speed = 0;
#endif
	return 0;	
}


#else
static u32 rtl_ethtool_get_port_link(struct net_device *dev)
{
	return 1;//always link, have problem?
}
#endif

static const struct ethtool_ops rtl_ethtool_ops = {
	.get_link 			= rtl_ethtool_get_port_link,
#ifdef CONFIG_RTL8686_SWITCH
	.get_settings 		= rtl_ethtool_get_settings,
#endif
};

void rtl_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &rtl_ethtool_ops;
}

#ifdef TX_RECYCLE_SKB_USING_TOK_INT
static void tx_int_mitigation_set(unsigned int gmac, unsigned int pkts)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	unsigned int value, mask;
	
	if(!pkts) {
		printk("%s %d ERROR ! pkts is NULL\n", __func__, __LINE__);
		return;
	}

	printk("%s %d pkts=%d\n", __func__, __LINE__, pkts);
	mask = TX_MIT_MASK;
	if(pkts==1) {
		value = TX_MIT_1PKT;
	}else if(pkts==4) {
		value = TX_MIT_4PKT;
	}else if(pkts==8) {
		value = TX_MIT_8PKT;
	}else if(pkts==12) {
		value = TX_MIT_12PKT;
	}else if(pkts==16) {
		value = TX_MIT_16PKT;
	}else if(pkts==20) {
		value = TX_MIT_20PKT;
	}else if(pkts==24) {
		value = TX_MIT_24PKT;
	}else if(pkts==28) {
		value = TX_MIT_28PKT;
	}else {
		printk("%s %d ERROR ! No such case\n", __func__, __LINE__);
		return;
	}

	cp->iocmd_reg &= ~(mask);
	cp->iocmd_reg |= value;
	RLE0787_W32(gmac, IO_CMD, cp->iocmd_reg);
	printk("%s %d set OK\n", __func__, __LINE__);
	return;
}

static void tx_int_mitigation_timer_set(unsigned int gmac, unsigned int tus)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	unsigned int value, mask;
	
	if(tus<0x0 || tus>0xf) {
		printk("%s %d ERROR ! No such case tus=0x%x\n", __func__, __LINE__, tus);
		return;
	}

	mask = TX_PKT_TIMER_MASK;
	value = (tus << 24);

	cp->iocmd_reg &= ~(mask);
	cp->iocmd_reg |= value;
	RLE0787_W32(gmac, IO_CMD, cp->iocmd_reg);
	printk("%s %d set OK\n", __func__, __LINE__);
	return;
}
#endif

static int misc_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	printk("spin lock type: ");
#if (GMAC_SPIN_LOCK_TYPE==GMAC_SPIN_LOCK_TYPE_BH)
	printk("BH\n");
#else
	printk("IRQ_SAVE\n");
#endif
	printk("Rx multiple ring interrpt split: ");
#ifdef RX_MRING_INT_SPLIT
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("Rx NAPI mode: ");
#ifdef RX_NAPI_MODE
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("Rx NAPI mode debug: ");
#ifdef RX_NAPI_MODE_DEBUG
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("Tx ring debug: ");
#ifdef TX_RING_DEBUG
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("Tx create test packet debug: ");
#ifdef TX_CREATE_TEST_PACKET_DEBUG
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("Tx watchdog timeout reset: ");
#ifdef TX_WATCHDOG_TIMEOUT_RESET
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("Tx HW LSO: ");
#ifdef CONFIG_REALTEK_HW_LSO
	printk("Enabled\n");
	printk("	-GMAC mode: ");
#if	GMAC_MODE==MODE_PURE_SW
	printk("MODE_PURE_SW\n");
#elif GMAC_MODE==MODE_SG_GSO
	printk("MODE_SG_GSO\n");
#elif GMAC_MODE==MODE_HW_LSO
	printk("MODE_HW_LSO\n");
#elif GMAC_MODE==MODE_HW_LSO_SG
	printk("MODE_HW_LSO_SG\n");
#endif
	printk("	-HW_LSO_ENABLE: ");
#ifdef HW_LSO_ENABLE
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("	-HW_CHECKSUM_OFFLOAD: ");
#ifdef HW_CHECKSUM_OFFLOAD
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("	-LINUX_LSO_ENABLE: ");
#ifdef LINUX_LSO_ENABLE
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("	-LINUX_SG_ENABLE: ");
#ifdef LINUX_SG_ENABLE
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
#else
	printk("Disabled\n");
#endif
	printk("Local service accelerate: ");
#ifdef LOCAL_SERVICE_ACCELERATE
	printk("%s\n", data->local_service_acc_enable?"Enabled":"Disabled");
#else
	printk("Not active\n");
#endif
	printk("Tx recycle skb using TOK interrpt: ");
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
	printk("Enabled\n");
#else
	printk("Disabled\n");
#endif
	printk("Tx recycle skb using timer polling: ");
#ifdef TX_RECYCLE_SKB_USING_POLLING
	printk("Enabled (%d sec)\n", TX_RECYCLE_SKB_POLLING_SECONDS);
#else
	printk("Disabled\n");
#endif
	printk("iocmd_reg 0x%x\n", data->iocmd_reg);
	printk("iocmd1_reg 0x%x\n", data->iocmd1_reg);
	printk("	tx_jumbo_frame_enabled: %s\n", data->tx_jumbo_frame_enabled?"Enabled":"Disabled");
	printk("	rx_save_int_prio_to_skb: ");
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
	printk("%s\n", data->rx_save_int_prio_to_skb?"Enabled":"Disabled");
#else
	printk("Not active\n");
#endif
#ifdef TX_RING_DEBUG
	printk("	tx_ring_backup_debug: %s\n", data->tx_ring_backup_debug?"Enabled":"Disabled");
#endif
#ifdef TX_CREATE_TEST_PACKET_DEBUG
	printk("tx_packet_test: %d\n", data->test_packet_start);
#endif
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
	printk("Tx recycle using interrupt mode\n");
	printk("	-tx_int_mitigation: 0x%x\n", data->tx_int_mitigation);
	printk("	-tx_int_mitigation_timer: 0x%x\n", data->tx_int_mitigation_timer);
#elif defined(TX_RECYCLE_SKB_USING_POLLING)
	printk("Tx recycle using polling mode\n");
	printk("	-polling per seconds: %d/%d\n", TX_RECYCLE_SKB_POLLING_SECONDS, CONFIG_HZ);
#endif
	printk("==============================================================\n");
	printk("Usage:\n");
	printk("	-iocmd_reg [0x********]\n");
	printk("	-iocmd1_reg [0x********]\n");
	printk("	-tx_jumbo_frame_enabled [0/1]\n");
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
	printk("	-rx_save_int_prio_to_skb [0/1]\n");
#endif
#ifdef TX_RING_DEBUG
	printk("	-tx_ring_backup_debug [0/1]\n");
#endif
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
	printk("	-tx_mit [1/4/8/12/16/20/24/28]\n");
	printk("	-tx_mit_timer [0/1/.../15]\n");
#endif
#ifdef TX_CREATE_TEST_PACKET_DEBUG
	printk("	-tx_packet_test [1/2/0] --> [start/kick/end]\n");
#endif
#ifdef LOCAL_SERVICE_ACCELERATE
	printk("	-local_service_acc_enable [0/1]\n");
#endif
	printk("==============================================================\n");
	return 0;
}

#ifdef TX_CREATE_TEST_PACKET_DEBUG
void send_test_packet_out_1(struct re_private *cp)
{
	struct sk_buff *test_skb = NULL;
	struct tx_info txInfo, txInfoMask;
	u32 len = 66;
	struct net_device *dev;	
	int i, j;

	cp->test_packet[0] = 0xe8;cp->test_packet[1] = 0x9d;cp->test_packet[2] = 0x87;cp->test_packet[3] = 0x9b;
	cp->test_packet[4] = 0xe8;cp->test_packet[5] = 0x2e;
	cp->test_packet[6] = 0x00;cp->test_packet[7] = 0xe0;cp->test_packet[8] = 0x4c;cp->test_packet[9] = 0x86;
	cp->test_packet[10] = 0x70;cp->test_packet[11] = 0x01;
	cp->test_packet[12] = 0x08;
	cp->test_packet[13] = 0x00;
	cp->test_packet[14] = 0x45;
	cp->test_packet[15] = 0x00;
	cp->test_packet[16] = 0x05;
	cp->test_packet[17] = 0xdc;
	cp->test_packet[18] = 0x39;
	cp->test_packet[19] = 0x50;

	cp->test_packet[20] = 0x40;
	cp->test_packet[21] = 0x00;
	cp->test_packet[22] = 0x40;
	cp->test_packet[23] = 0x08;

	for(j=24 ; j<300 ; j++)
	{
		cp->test_packet[j] = 0xaa;
	}

	dev = dev_get_by_name(&init_net,"eth0.4");
	if(dev==NULL)
	{
		printk("can't find eth0.4 device\n");
		return;
	}
	test_skb = dev_alloc_skb(JUMBO_SKB_BUF_SIZE);
    if(!test_skb) {
		//spin_unlock_irqrestore(&test_lock, flags);
        printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
		return;			
    }
	test_skb->dev = dev;
    skb_put(test_skb, len);
	memcpy(test_skb->data, &cp->test_packet[0], len);
	txInfo.opts1.dw = 0x20000042;
	txInfo.opts2.dw = 0x80040000;
	txInfo.opts3.dw = 0x08220000;
	txInfo.opts4.dw = 0xddc00000;
	txInfoMask.opts1.dw = 0xffffffff;
	txInfoMask.opts2.dw = 0xffffffff;
	txInfoMask.opts3.dw = 0xffffffff;
	txInfoMask.opts4.dw = 0xffffffff;
	re8686_send_with_txInfo_and_mask(test_skb, &txInfo, 0, &txInfoMask);
}

void send_test_packet_out_2(struct re_private *cp)
{
	struct sk_buff *test_skb = NULL;
	struct tx_info txInfo, txInfoMask;
	u32 len = 3376;
	struct net_device *dev;
	int i, j;

	for(j=0 ; j<5000 ; j++)
	{
		cp->test_packet[j] = 0xbb;
	}
	
	dev = dev_get_by_name(&init_net,"eth0.4");
	if(dev==NULL)
	{
		printk("can't find eth0.4 device\n");
		return;
	}
	test_skb = dev_alloc_skb(JUMBO_SKB_BUF_SIZE);
    if(!test_skb) {
        printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
		return;			
    }
	test_skb->dev = dev;
    skb_put(test_skb, len);
	memcpy(test_skb->data, &cp->test_packet[0], len);
	txInfo.opts1.dw = 0x00000d30;
	txInfo.opts2.dw = 0x80040000;
	txInfo.opts3.dw = 0x08220000;
	txInfo.opts4.dw = 0xddc00000;
	txInfoMask.opts1.dw = 0xffffffff;
	txInfoMask.opts2.dw = 0xffffffff;
	txInfoMask.opts3.dw = 0xffffffff;
	txInfoMask.opts4.dw = 0xffffffff;
	re8686_send_with_txInfo_and_mask(test_skb, &txInfo, 0, &txInfoMask);
}

void send_test_packet_out_3(struct re_private *cp)
{
	struct sk_buff *test_skb = NULL;
	struct tx_info txInfo, txInfoMask;
	u32 len = 11104;
	struct net_device *dev;
	int i, j;

	for(j=0 ; j<12000 ; j++)
	{
		cp->test_packet[j] = 0xcc;
	}
	
	dev = dev_get_by_name(&init_net,"eth0.4");
	if(dev==NULL)
	{
		printk("can't find eth0.4 device\n");
		return;
	}
	test_skb = dev_alloc_skb(JUMBO_SKB_BUF_SIZE);
    if(!test_skb) {
        printk("%s:%d allocte skb for jumbo frame fail\n", __FILE__, __LINE__);
		return;			
    }
	test_skb->dev = dev;
    skb_put(test_skb, len);
	memcpy(test_skb->data, &cp->test_packet[0], len);
	txInfo.opts1.dw = 0x10002b60;
	txInfo.opts2.dw = 0x80040000;
	txInfo.opts3.dw = 0x08220000;
	txInfo.opts4.dw = 0xddc00000;
	txInfoMask.opts1.dw = 0xffffffff;
	txInfoMask.opts2.dw = 0xffffffff;
	txInfoMask.opts3.dw = 0xffffffff;
	txInfoMask.opts4.dw = 0xffffffff;
	re8686_send_with_txInfo_and_mask(test_skb, &txInfo, 0, &txInfoMask);
}
#endif

static int misc_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int value;
	unsigned int gmac;
	char	tmpbuf[64];
	char	*strptr, *var;
	char	*tokptr;
	void*	pBuf;
	int i, j;

#define VAR_NUM 16
	char var_name[VAR_NUM][64] = {"iocmd_reg", "iocmd1_reg", "tx_jumbo_frame_enabled", "rx_save_int_prio_to_skb", "tx_ring_backup_debug", "tx_mit", "tx_mit_timer", "tx_packet_test", "local_service_acc_enable", ""};

	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	unsigned int* pVar[VAR_NUM] = {&(data->iocmd_reg), &(data->iocmd1_reg), &(data->tx_jumbo_frame_enabled), &(data->rx_save_int_prio_to_skb), &(data->tx_ring_backup_debug), &(data->tx_int_mitigation), &(data->tx_int_mitigation_timer), &(data->test_packet_start), &(data->local_service_acc_enable), NULL};
	
	gmac = data->gmac;
	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		var = strsep(&strptr," ");
		if (var==NULL)
		{
			goto errout;
		}
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		
		for(i=0;strlen(var_name[i])&&pVar[i];i++){
			if(!memcmp(var, var_name[i], strlen(var_name[i]))){
				if(!strcmp(var, "tx_jumbo_frame_enabled"))
				{
					value = simple_strtol(tokptr, NULL, 0);
					*pVar[i] = value;
					config_tx_jumbo(gmac, value);
				}
#ifdef TX_RING_DEBUG
				else if(!strcmp(var, "tx_ring_backup_debug"))
				{
					memset(data->rtl8686_tx_ring_debug, 0, sizeof(struct rtl8686_tx_ring_debug_entry)*MAX_GMAC_NUM*MAX_TXRING_NUM);
					value = simple_strtol(tokptr, NULL, 0);
					*pVar[i] = value;
					if(value)
					{
						for(j=0;j<MAX_TXRING_NUM;j++)
						{
							if(!data->re8670_tx_ring_size[j])
								continue;

							pBuf = kzalloc(RE8670_TXRING_BYTES(data->re8670_tx_ring_size[j]), GFP_KERNEL);
							if (!pBuf)
								printk("%s %d FAIL !\n\n", __func__, __LINE__);

							dma_cache_wback_inv((unsigned long)pBuf, RE8670_TXRING_BYTES(data->re8670_tx_ring_size[j]));
							pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ;
							data->rtl8686_tx_ring_debug[j].txDescriptor = (DMA_TX_DESC*)((u32)(pBuf) | UNCACHE_MASK);

							pBuf = kzalloc(data->re8670_tx_ring_size[j]*sizeof(struct tx_data_buffter), GFP_KERNEL);
							if (!pBuf)
								printk("%s %d FAIL !\n\n", __func__, __LINE__);

							data->rtl8686_tx_ring_debug[j].dataBuffer = pBuf;
						}
					}
					else
					{
						for(j=0;j<MAX_TXRING_NUM;j++)
						{
							kfree(data->rtl8686_tx_ring_debug[j].txDescriptor);
							kfree(data->rtl8686_tx_ring_debug[j].dataBuffer);
						}
					}
				}
#endif
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
				else if(!strcmp(var, "tx_mit")) 
				{
					value = simple_strtol(tokptr, NULL, 0);
					*pVar[i] = value;
					tx_int_mitigation_set(gmac, value);
				} else if(!strcmp(var, "tx_mit_timer")) 
				{
					value = simple_strtol(tokptr, NULL, 0);
					*pVar[i] = value;
					tx_int_mitigation_timer_set(gmac, value);
				}
#endif
#ifdef TX_CREATE_TEST_PACKET_DEBUG
				else if(!strcmp(var, "tx_packet_test")) 
				{
					value = simple_strtol(tokptr, NULL, 0);					
					if(value == 1)
					{
						*pVar[i] = value;
						send_test_packet_out_1(data);
						send_test_packet_out_2(data);
						send_test_packet_out_3(data);
					}
					else if(value == 2)
					{
						*pVar[i] = value;
						kick_tx(0, 0);
					}
				}
#endif
				else
				{
					value = simple_strtol(tokptr, NULL, 0);
					*pVar[i] = value;
				}
				break;
			}
		}

		if(!pVar[i]){
			goto errout;
		}

	}
	else
	{
errout:
		printk("iocmd_reg 0x%x\n", data->iocmd_reg);
		printk("iocmd1_reg 0x%x\n", data->iocmd1_reg);
		printk("tx_jumbo_frame_enabled: %s\n", data->tx_jumbo_frame_enabled?"Enabled":"Disabled");
		printk("rx_save_int_prio_to_skb: ");
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
		printk("%s\n", data->rx_save_int_prio_to_skb?"Enabled":"Disabled");
#else
		printk("Not active\n");
#endif
#ifdef TX_RING_DEBUG
		printk("tx_ring_backup_debug: %s\n", data->tx_ring_backup_debug?"Enabled":"Disabled");
#endif
#ifdef TX_CREATE_TEST_PACKET_DEBUG
		printk("tx_packet_test: %d\n", data->test_packet_start);
#endif
		printk("Local service accelerate: ");
#ifdef LOCAL_SERVICE_ACCELERATE
		printk("%s\n", data->local_service_acc_enable?"Enabled":"Disabled");
#else
		printk("Not active\n");
#endif
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
		printk("Tx recycle using interrupt mode\n");
		printk("	-tx_int_mitigation: 0x%x\n", data->tx_int_mitigation);
		printk("	-tx_int_mitigation_timer: 0x%x\n", data->tx_int_mitigation_timer);
#elif defined(TX_RECYCLE_SKB_USING_POLLING)
		printk("Tx recycle using polling mode\n");
		printk("	-polling per seconds: %d/%d\n", TX_RECYCLE_SKB_POLLING_SECONDS, CONFIG_HZ);
#endif
		printk("==============================================================\n");
		printk("Usage:\n");
		printk("	-iocmd_reg [0x********]\n");
		printk("	-iocmd1_reg [0x********]\n");
		printk("	-tx_jumbo_frame_enabled [0/1]\n");
#ifdef CONFIG_RTK_CONTROL_PACKET_PROTECTION
		printk("	-rx_save_int_prio_to_skb [0/1]\n");
#endif
#ifdef TX_RING_DEBUG
		printk("	-tx_ring_backup_debug [0/1]\n");
#endif
#ifdef TX_RECYCLE_SKB_USING_TOK_INT
		printk("	-tx_mit [1/4/8/12/16/20/24/28]\n");
		printk("	-tx_mit_timer [0/1/.../15]\n");
#endif
#ifdef TX_CREATE_TEST_PACKET_DEBUG
		printk("	-tx_packet_test [1/2/0] --> [start/kick/end]\n");
#endif
#ifdef LOCAL_SERVICE_ACCELERATE
		printk("	-local_service_acc_enable [0/1]\n");
#endif
		printk("==============================================================\n");

	}
	return count;
}

static int dev_port_mapping_read(struct seq_file *m, void *v)
{
	static struct re_private *data;
	unsigned int i;
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	struct file *file = m->private;
	
	data=PDE_DATA(file_inode(file));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	seq_printf(m, "WAN PORT %d, CPU PORT %d\n", WAN_PORT, data->gmac_cpu_port);
	seq_printf(m, "DEV ability: ");
	for(i=0;i<totalDev;i++){
		if(rtl8686_dev_table[i].dev_instant)
			seq_printf(m, "%s ", rtl8686_dev_table[i].dev_instant->name);
	}
	seq_printf(m, "\n");

	seq_printf(m, "rx: phyPort -> dev[the packet rx from phyPort will send to kernel using dev]\n");
	for(i=0;i<SW_PORT_NUM;i++){
		if(data->port2dev[i])
			seq_printf(m, "port%d -> %s\n", i, data->port2dev[i]->name);
	}

	seq_printf(m, "tx:dev -> txPortMask[when tx from dev, we will use this txPortMask]\n");
	for(i=0;i<totalDev;i++){
		if(rtl8686_dev_table[i].dev_instant)
			#if defined(CONFIG_RTL_BRIDGE_VLAN_SPECIAL_SUPPORT)
			seq_printf(m, "%s -> 0x%x   mc_vid: %d\n",
				rtl8686_dev_table[i].dev_instant->name, DEVPRIV(rtl8686_dev_table[i].dev_instant)->txPortMask , rtl8686_dev_table[i].dev_instant->mc_vid);
			#else
			seq_printf(m, "%s -> 0x%x\n",
				rtl8686_dev_table[i].dev_instant->name, DEVPRIV(rtl8686_dev_table[i].dev_instant)->txPortMask);
			#endif
	}

	seq_printf(m, "TX force using: GMAC%d\n", re_private_data_root.tx_force_gmac_index);
#ifdef CONFIG_RTL8686_SWITCH
	seq_printf(m, "netdev carrier mapping\n");
	for(i=0;i<SW_PORT_NUM;i++)
	{
		seq_printf(m, "Port%d => ifname:%s , dev:%s(0x%p) , status:%d \n",
			i , LCDev_mapping[i].ifname , LCDev_mapping[i].phy_dev->name, LCDev_mapping[i].phy_dev, LCDev_mapping[i].status);
	}
#endif
	return 0;
}

static int dev_port_mapping_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 		tmpbuf[64];
	char		*strptr;
	unsigned int port_num;//, i;
	char		*tokptr;
	unsigned long buf_size;
	
	buf_size = min(count, (sizeof(tmpbuf)-1));
	if (buf && !copy_from_user(tmpbuf, buf, buf_size)) {
	
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		port_num = simple_strtol(tokptr, NULL, 0);
		if(strcmp(tokptr, "tx_force_gmac")) {
			printk("port %d ", port_num);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			if(tokptr[strlen(tokptr) - 1] == 0x0a)
				tokptr[strlen(tokptr) - 1] = 0x00;
			printk("assign to %s\n", tokptr);
			change_dev_port_mapping(port_num, tokptr);	
		} else {
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			if(tokptr[strlen(tokptr) - 1] == 0x0a)
				tokptr[strlen(tokptr) - 1] = 0x00;			
			if(!change_tx_force_gmac(tokptr))
				printk("TX force using %s \n", tokptr);
			else
				goto errout;
		}
	}
	else
	{
errout:
		printk("num_of_port dev_name\n");
		printk("tx_force_gmac GMAC0/GMAC1/GMAC2 \n");
		return -EFAULT;
	}
		
	return count;
}

static int change_dev_tx_port_mask(int port_num, char* name, int index)
{
	if(strcmp(name, "eth0")){
		DEVPRIV(rtl8686_dev_table[index].dev_instant)->txPortMask = 1 << port_num;
		//printk("%s -> 0x%x\n", 
			//rtl8686_dev_table[index].dev_instant->name, DEVPRIV(rtl8686_dev_table[index].dev_instant)->txPortMask);
	}
	return 0;
}

/*ccwei: add dev port mapping api*/
static int change_dev_port_mapping(int port_num, char* name)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp;
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	struct net_device *dev = NULL;
	unsigned int gmac;
	char* dev_name;	
	int i;
	
	for(i=0;i<totalDev;i++){

		if(i == 0)
		{
			continue;
		}
		
		if(rtl8686_dev_table[i].dev_instant)
		{
			dev_name = rtl8686_dev_table[i].dev_instant->name;
			dev = rtl8686_dev_table[i].dev_instant;
		}
		else
		{
			printk("no dev_instant, strange.......\n");
			dev_name = rtl8686_dev_table[i].ifname;
		}		
		if(!strcmp(dev_name, name)){
			for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {

				cp = root_cp->re_private_data_ptr[gmac];
				
				if(cp->gmac_enabled != GMAC_TRUE)
					continue;	
				
				cp->port2dev[port_num] = rtl8686_dev_table[i].dev_instant;
			}
			change_dev_tx_port_mask(port_num, name, i);
			break;
		}
	}
	
	if(i == totalDev){
		printk("can't find dev %s\n", name);
		return -1;
	}
	
#ifdef CONFIG_RTL8686_SWITCH 
	if(port_num < SW_PORT_NUM && port_num >= 0 && dev )
	{
		LCDev_mapping[port_num].phy_dev = dev;
		strcpy(LCDev_mapping[port_num].ifname,dev->name);
	}
#endif
	
	return 0;
}

static int change_tx_force_gmac(char* name)
{
	unsigned int gmac;
	int ret = -1;
	int i;

	sscanf(name, "GMAC%d", &gmac);
	if(gmac<MAX_GMAC_NUM)
	{
 		re_private_data_root.tx_force_gmac_index = gmac;
		ret = 0;
	} 
	
	return ret;
}

#if defined(CONFIG_APOLLO_ROMEDRIVER)
extern int fwdEngine_rx_skb(struct re_private *cp, struct sk_buff *skb,struct rx_info *pRxInfo);
#endif
static int port_to_rxfunc_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int totalPortTable = SW_PORT_NUM;
	unsigned int i;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	//default set to eth0
	for(i=0;i<totalPortTable;i++){
		printk("port%d -> 0x%p\n", i, data->port2rxfunc[i]);
	}
	printk("================[port2rxfunc func pointer table]================\n");
#if defined(CONFIG_APOLLO_ROMEDRIVER)
	printk("fwdEngine_rx_skb: 0x%p\n", fwdEngine_rx_skb);
#endif
	printk("re8670_rx_skb: 0x%p\n", re8670_rx_skb);
	printk("================================================================\n");
	return 0;
}

static int port_to_rxfunc_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 	tmpbuf[512];
	char		*strptr;	
	//int retval = -1, i;
	static struct re_private *data;
	
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		if( strlen(strptr)==0 )
		{
			goto errout;
		}
#if defined(CONFIG_APOLLO_ROMEDRIVER)
		if(strncmp(strptr, "force2rg", 8) == 0)
		{
			re8686_register_rxfunc_all_port(&fwdEngine_rx_skb);
			printk("force NIC Rx hook to RG only!\n");
		}
		else 
#endif
		if(strncmp(strptr, "force2nf", 8) == 0)
		{
			re8686_register_rxfunc_all_port(&re8670_rx_skb);
			printk("force NIC Rx hook to PS only!\n");
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
#if defined(CONFIG_APOLLO_ROMEDRIVER)
		printk("error input (force2rg/force2nf)\n");
#else
		printk("error input (force2nf)\n");
#endif
	}
	return count;
}

int dbg_level_read(struct file *filp, char *buf, size_t count, loff_t *offp ) 
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	printk("[debug_enable = 0x%08x]\n", data->debug_enable);
	printk("RTL8686_PRINT_NOTHING\t0x%08x\n", RTL8686_PRINT_NOTHING);
	printk("RTL8686_SKB_RX\t\t0x%08x\n", RTL8686_SKB_RX);
	printk("RTL8686_SKB_TX\t\t0x%08x\n", RTL8686_SKB_TX);
	printk("RTL8686_RXINFO\t\t0x%08x\n", RTL8686_RXINFO);
	printk("RTL8686_TXINFO\t\t0x%08x\n", RTL8686_TXINFO);
	printk("RTL8686_RX_TRACE\t\t0x%08x\n", RTL8686_RX_TRACE);
	printk("RTL8686_TX_TRACE\t\t0x%08x\n", RTL8686_TX_TRACE);
	printk("RTL8686_RX_WARN\t\t0x%08x\n", RTL8686_RX_WARN);
	printk("RTL8686_TX_WARN\t\t0x%08x\n", RTL8686_TX_WARN);
	printk("RTL8686_OTHERS\t\t0x%08x\n", RTL8686_OTHERS);
	
	return 0;
}

int dbg_times_read(struct file *filp, char *buf, size_t count, loff_t *offp ) 
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	gmac = data->gmac;
	printk("[debug_times = %d times]\n", data->debug_times);
	
	return 0;
}

static int hw_reg_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	int len = 0, i;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	printk("ETHBASE		=0x%08x\n", data->base);
	printk("IDR		=%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\n", 
		RLE0787_R8(gmac, IDR0), RLE0787_R8(gmac, IDR1), RLE0787_R8(gmac, IDR2), RLE0787_R8(gmac, IDR3), RLE0787_R8(gmac, IDR4), RLE0787_R8(gmac, IDR5));
	printk("MAR		=%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\n", 
		RLE0787_R8(gmac, MAR0), RLE0787_R8(gmac, MAR1), RLE0787_R8(gmac, MAR2), RLE0787_R8(gmac, MAR3), 
		RLE0787_R8(gmac, MAR4), RLE0787_R8(gmac, MAR5), RLE0787_R8(gmac, MAR6), RLE0787_R8(gmac, MAR7));
	printk("TXOKCNT		=0x%04x		RXOKCNT		=0x%04x\n", RLE0787_R16(gmac, TXOKCNT), RLE0787_R16(gmac, RXOKCNT));
	printk("TXERR		=0x%04x		RXERRR		=0x%04x\n", RLE0787_R16(gmac, TXERR), RLE0787_R16(gmac, RXERRR));
	printk("MISSPKT		=0x%04x		FAE		=0x%04x\n", RLE0787_R16(gmac, MISSPKT), RLE0787_R16(gmac, FAE));
	printk("TX1COL		=0x%04x		TXMCOL		=0x%04x\n", RLE0787_R16(gmac, TX1COL), RLE0787_R16(gmac, TXMCOL));
	printk("RXOKPHY		=0x%04x		RXOKBRD		=0x%04x\n", RLE0787_R16(gmac, RXOKPHY), RLE0787_R16(gmac, RXOKBRD));
	printk("RXOKMUL		=0x%04x		TXABT		=0x%04x\n", RLE0787_R16(gmac, RXOKMUL), RLE0787_R16(gmac, TXABT));
	printk("TXUNDRN		=0x%04x		RDUMISSPKT	=0x%04x\n", RLE0787_R16(gmac, TXUNDRN), RLE0787_R16(gmac, RDUMISSPKT));
	printk("TRSR		=0x%08x\n", RLE0787_R32(gmac, TRSR));
	printk("CMD		=0x%02x		IMR		=0x%04x\n", RLE0787_R8(gmac, CMD), RLE0787_R16(gmac, IMR));
	printk("ISR		=0x%04x		TCR		=0x%08x\n", RLE0787_R16(gmac, ISR), RLE0787_R32(gmac, TCR));
	printk("ISR1		=0x%08x	IMR0		=0x%08x\n", RLE0787_R32(gmac, ISR1), RLE0787_R32(gmac, IMR0));
	printk("RCR		=0x%08x	CPUtagCR	=0x%08x\n", RLE0787_R32(gmac, RCR), RLE0787_R32(gmac, CPUtagCR));
	printk("CONFIG_REG	=0x%08x	CPUtag1CR	=0x%08x\n", RLE0787_R32(gmac, CONFIG_REG), RLE0787_R32(gmac, CPUtag1CR));
	printk("MSR		=0x%08x	VLAN_REG	=0x%08x\n", RLE0787_R32(gmac, MSR), RLE0787_R32(gmac, VLAN_REG));
	printk("VLAN1_REG	=0x%08x		LEDCR		=0x%08x\n", RLE0787_R32(gmac, VLAN1_REG), RLE0787_R32(gmac, LEDCR));

	for(i=0;i<MAX_TXRING_NUM;i++)
	{
		printk("TxFDP%d		=0x%08x	TxCDO%d		=0x%04x\n",i,RLE0787_R32(gmac, TxFDP1+(ADDR_OFFSET*i)),i,RLE0787_R16(gmac, TxCDO1+(ADDR_OFFSET*i)));
	}
	for(i=0;i<MAX_RXRING_NUM;i++){
#ifndef CONFIG_RTK_SINGLE_RX_RING
		if(i==0)
#endif
		{
			printk("RxRingSize%d	=0x%04x\n", i, RLE0787_R16(gmac, RxRingSize));
			printk("RxFDP%d		=0x%08x	RxCDO%d		=0x%04x\n",i,RLE0787_R32(gmac, RxFDP), i, RLE0787_R16(gmac, RxCDO));
			printk("EthrntRxCPU_Des_Num	=0x%02x	EthrntRxCPU_Des_Wrap	=0x%02x\n", 
				RLE0787_R8(gmac, EthrntRxCPU_Des_Num), RLE0787_R8(gmac, EthrntRxCPU_Des_Wrap));
			printk("Rx_Pse_Des_Thres	=0x%02x	EthrntRxCPU_Des_Num_h	=0x%02x\n", 
				RLE0787_R8(gmac, Rx_Pse_Des_Thres), RLE0787_R8(gmac, EthrntRxCPU_Des_Num_h));
			printk("Rx_Pse_Des_Thres_h	=0x%02x\n", RLE0787_R8(gmac, Rx_Pse_Des_Thres_h));
		}
#ifndef CONFIG_RTK_SINGLE_RX_RING
		else
		{
			printk("RxRingSize%d	=0x%04x\n", i, RLE0787_R16(gmac, RxRingSize2+(ADDR_OFFSET*(i-1))));
			printk("RxFDP%d		=0x%08x	RxCDO%d		=0x%04x\n",i,RLE0787_R32(gmac, RxFDP2+(ADDR_OFFSET*(i-1))),i,RLE0787_R16(gmac, RxCDO2+(ADDR_OFFSET*(i-1))));
			printk("RxCPU_Des_Num%d	=0x%08x	RxCPU_Des_Thres%d=0x%08x\n", 
				i, RLE0787_R32(gmac, EthrntRxCPU_Des_Num2+(ADDR_OFFSET*(i-1))), 
				i, RLE0787_R32(gmac, EthrntRxCPU_Des_Wrap2+(ADDR_OFFSET*(i-1))));
		}
#endif
	}
	printk("RRING_ROUTING1	=0x%08x	RRING_ROUTING2	=0x%08x\n", RLE0787_R32(gmac, RRING_ROUTING1), RLE0787_R32(gmac, RRING_ROUTING2));
	printk("RRING_ROUTING3	=0x%08x	RRING_ROUTING4	=0x%08x\n", RLE0787_R32(gmac, RRING_ROUTING3), RLE0787_R32(gmac, RRING_ROUTING4));
	printk("RRING_ROUTING5	=0x%08x	RRING_ROUTING6	=0x%08x\n", RLE0787_R32(gmac, RRING_ROUTING5), RLE0787_R32(gmac, RRING_ROUTING6));
	printk("RRING_ROUTING7	=0x%08x\n", RLE0787_R32(gmac, RRING_ROUTING7));
	printk("IO_CMD		=0x%08x	IO_CMD1		=0x%08x\n", RLE0787_R32(gmac, IO_CMD), RLE0787_R32(gmac, IO_CMD1));
	printk("DIAG1_REG       =0x%08x\n",RLE0787_R32(gmac, DIAG1_REG));

	return len;
}

static int hw_reg_write(struct file *file, const char __user *buff, size_t len, loff_t *ppos)
{
	char 	tmpbuf[512];
	char		*strptr;
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(file));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	if (buff && !copy_from_user(tmpbuf, buff, len))
	{
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		if( strlen(strptr)==0 )
		{
			goto errout;
		}
		if(strncmp(strptr, "reset", 5) == 0)
		{
			if(re8670_reset())
				goto errout;
		}
		else if(strncmp(strptr, "swint", 5) == 0)
		{
			RLE0787_W32(gmac, SWINT_REG, (1<<24));
			printk("SWINT triggered !\n");
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("error input !\n");
	}
	return len;
}

static int sw_cnt_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	int len = 0, i;
	
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	printk("%-24s:%14d          %-24s:%14d\n", 
		"rx_hw_num", data->cp_stats.rx_hw_num, "rx_sw_num", data->cp_stats.rx_sw_num);
	printk("%-24s:%14d          %-24s:%14d\n", 
		"rer_runt", data->cp_stats.rer_runt, "rer_ovf", data->cp_stats.rer_ovf);
	printk("%-24s:%14d          %-24s:%14d\n", 
		"rdu", data->cp_stats.rdu, "frag", data->cp_stats.frag);
#ifdef CONFIG_RG_JUMBO_FRAME
	printk("%-24s:%14d\n", "toobig", data->cp_stats.toobig);
#endif
	printk("%-24s:%14d          %-24s:%14d\n", 
		"crcerr", data->cp_stats.crcerr, "rcdf", data->cp_stats.rcdf);
	printk("%-24s:%14d          %-24s:%14d\n", 
		"rx_no_mem", data->cp_stats.rx_no_mem, "tx_sw_num", data->cp_stats.tx_sw_num);
	printk("%-24s:%14d          %-24s:%14d\n", 
		"tx_hw_num", data->cp_stats.tx_hw_num, "tx_no_desc", data->cp_stats.tx_no_desc);
	printk("%-24s:%14d          %-24s:%14d\n", 
		"rx_critical_num", data->cp_stats.rx_critical_num, "rx_critical_drop_num", data->cp_stats.rx_critical_drop_num);
	printk("%-24s:%14d          %-24s:%14d\n", 
		"tok", data->cp_stats.tok, "tok_free_skb", data->cp_stats.tok_free_skb);
	printk("%-24s:%14d          %-24s:%14d\n", 
		"tx_timeouts", data->cp_stats.tx_timeouts, "tooshort", data->cp_stats.tooshort);
#if defined(CONFIG_RTL865X_ETH_PRIV_SKB)
	printk("%-24s:%14d          %-24s:%14d\n", "nic_max_alloc_threshold", RE8670_MAX_ALLOC_RXSKB_NUM, "prealloc_pool_size", MAX_ETH_SKB_NUM);
	printk("%-24s:%14d          %-24s:%14d\n", "dynamic_alloc_threshold", RE8670_DYNAMIC_ALLOC_RXSKB_NUM, "dynamic_alloc_skb_num", dynamic_alloc_skb_num);
	printk("%-24s:%14d          %-24s:%14d\n", "eth_skb_free_num", eth_skb_free_num, "eth_skb_alloc_num", eth_skb_alloc_num);
	printk("%-24s:%14d          %-24s:%14d\n", "top_eth_skb_alloc_num", (MAX_ETH_SKB_NUM-lowest_eth_skb_free_num), "cri_eth_skb_free_num", critical_eth_skb_free_num);
#else
	printk("%-24s:%14d			%-24s:%14d\n", "nic_prealloc_threshold", RE8670_MAX_ALLOC_RXSKB_NUM, "prealloc_pool_size", 0);
#endif
#ifdef RX_NAPI_MODE
	printk("%-24s:%14d          %-24s:%14d\n", 
		"rx_napi_poll", data->cp_stats.rx_napi_poll, "rx_napi_interrupt", data->cp_stats.rx_napi_interrupt);
#ifdef RX_NAPI_MODE_DEBUG
	unsigned long long total_packet_cnt = 0;
	for(i=0 ; i<MAX_NAPI_STATISTIC_NUM ; i++)
	{
		if(napi_statistic[i])
		{
			printk("	napi_statistic[%d]=%d\n", i, napi_statistic[i]);
			total_packet_cnt = total_packet_cnt + napi_statistic[i];
		}
	}	
	printk("	total_poll_and_interrupt=%llu\n", total_packet_cnt);
	printk("%-24s:%14d          %-24s:%14d\n", "rx_napi_gro_normal", data->cp_stats.rx_napi_gro_normal, "rx_napi_gro_drop", data->cp_stats.rx_napi_gro_drop);
	printk("%-24s:%14d          %-24s:%14d\n", "rx_napi_gro_merged_free", data->cp_stats.rx_napi_gro_merged_free, "rx_napi_gro_held", data->cp_stats.rx_napi_gro_held);
	printk("%-24s:%14d\n", "rx_napi_gro_merged", data->cp_stats.rx_napi_gro_merged);
#endif
#endif

	aggQ_sw_cnt_read(data);

	return len;
}

static int sw_cnt_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	char 	tmpbuf[512];
	char		*strptr;	
	int i;
	//int retval = -1, i;
	static struct re_private *data;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	if (buf && !copy_from_user(tmpbuf, buf, count))
	{
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		if( strlen(strptr)==0 )
		{
			goto errout;
		}
		if(strncmp(strptr, "all", 8) == 0 || strncmp(strptr, "1", 1) == 0)
		{
			data->cp_stats.rx_hw_num=0;
			data->cp_stats.rx_sw_num=0;
			data->cp_stats.rer_runt=0;
			data->cp_stats.rer_ovf=0;
			data->cp_stats.rdu=0;
			data->cp_stats.frag=0;
			data->cp_stats.toobig=0;
			data->cp_stats.crcerr=0;
			data->cp_stats.rcdf=0;
			data->cp_stats.rx_no_mem=0;
			data->cp_stats.tx_sw_num=0;
			data->cp_stats.tx_hw_num=0;
			data->cp_stats.tx_no_desc=0;
			data->cp_stats.rx_critical_num=0;
			data->cp_stats.rx_critical_drop_num=0;
			data->cp_stats.tooshort=0;
#ifdef RX_NAPI_MODE
			data->cp_stats.rx_napi_poll=0;
			data->cp_stats.rx_napi_interrupt=0;
#ifdef RX_NAPI_MODE_DEBUG
			for(i=0 ; i<MAX_NAPI_STATISTIC_NUM ; i++)
			{
				if(napi_statistic[i])
				{
					napi_statistic[i] = 0;
				}
			}
			data->cp_stats.rx_napi_gro_normal=0;
			data->cp_stats.rx_napi_gro_drop=0;
			data->cp_stats.rx_napi_gro_merged_free=0;
			data->cp_stats.rx_napi_gro_held=0;
			data->cp_stats.rx_napi_gro_merged=0;
#endif
#endif
#if defined(CONFIG_RTL865X_ETH_PRIV_SKB)
			lowest_eth_skb_free_num = MAX_ETH_SKB_NUM;
			dynamic_alloc_skb_num = 0;
#endif
			aggQ_sw_cnt_write(data);
			printk("all software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_hw_num", 9) == 0 || strncmp(strptr, "2", 1) == 0)
		{
			data->cp_stats.rx_hw_num=0;
			printk("rx_hw_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_sw_num", 9) == 0 || strncmp(strptr, "3", 1) == 0)
		{
			data->cp_stats.rx_sw_num=0;
			printk("rx_sw_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "rer_runt", 8) == 0 || strncmp(strptr, "4", 1) == 0)
		{
			data->cp_stats.rer_runt=0;
			printk("rer_runt software counter cleared !\n");
		}
		else if(strncmp(strptr, "rer_ovf", 7) == 0 || strncmp(strptr, "5", 1) == 0)
		{
			data->cp_stats.rer_ovf=0;
			printk("rer_ovf software counter cleared !\n");
		}
		else if(strncmp(strptr, "rdu", 3) == 0 || strncmp(strptr, "6", 1) == 0)
		{
			data->cp_stats.rdu=0;
			printk("rdu software counter cleared !\n");
		}
		else if(strncmp(strptr, "frag", 4) == 0 || strncmp(strptr, "7", 1) == 0)
		{
			data->cp_stats.frag=0;
			printk("frag software counter cleared !\n");
		}
		else if(strncmp(strptr, "toobig", 6) == 0 || strncmp(strptr, "8", 1) == 0)
		{
			data->cp_stats.toobig=0;
			printk("toobig software counter cleared !\n");
		}
		else if(strncmp(strptr, "crcerr", 6) == 0 || strncmp(strptr, "9", 1) == 0)
		{
			data->cp_stats.crcerr=0;
			printk("crcerr software counter cleared !\n");
		}
		else if(strncmp(strptr, "rcdf", 4) == 0 || strncmp(strptr, "10", 2) == 0)
		{
			data->cp_stats.rcdf=0;
			printk("rcdf software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_no_mem", 9) == 0 || strncmp(strptr, "11", 2) == 0)
		{
			data->cp_stats.rx_no_mem=0;
			printk("rx_no_mem software counter cleared !\n");
		}
		else if(strncmp(strptr, "tx_sw_num", 9) == 0 || strncmp(strptr, "12", 2) == 0)
		{
			data->cp_stats.tx_sw_num=0;
			printk("tx_sw_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "tx_hw_num", 9) == 0 || strncmp(strptr, "13", 2) == 0)
		{
			data->cp_stats.tx_hw_num=0;
			printk("tx_hw_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "tx_no_desc", 10) == 0 || strncmp(strptr, "14", 2) == 0)
		{
			data->cp_stats.tx_no_desc=0;
			printk("tx_no_desc software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_critical_num", 15) == 0 || strncmp(strptr, "15", 2) == 0)
		{
			data->cp_stats.rx_critical_num=0;
			printk("rx_critical_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_cri_no_desc", 14) == 0 || strncmp(strptr, "16", 2) == 0)
		{
			data->cp_stats.rx_critical_drop_num=0;
			printk("rx_cri_no_desc software counter cleared !\n");
		}
#ifdef RX_NAPI_MODE
		else if(strncmp(strptr, "rx_napi_poll", 12) == 0 || strncmp(strptr, "17", 2) == 0)
		{
			data->cp_stats.rx_napi_poll=0;
			printk("rx_napi_poll software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_napi_interrupt", 17) == 0 || strncmp(strptr, "18", 2) == 0)
		{
			data->cp_stats.rx_napi_interrupt=0;
			printk("rx_napi_interrupt software counter cleared !\n");
		}
#endif
#if defined(CONFIG_RTL865X_ETH_PRIV_SKB)
		else if(strncmp(strptr, "top_eth_skb_alloc_num", 21) == 0 || strncmp(strptr, "19", 2) == 0)
		{
			lowest_eth_skb_free_num = MAX_ETH_SKB_NUM;
			printk("top_eth_skb_alloc_num software counter cleared !\n");
		}
		else if(strncmp(strptr, "dynamic_alloc_skb_num", 21) == 0 || strncmp(strptr, "20", 2) == 0)
		{
			dynamic_alloc_skb_num = 0;
			printk("dynamic_alloc_skb_num software counter cleared !\n");
		}
#endif
	else if(strncmp(strptr, "tooshort", 10) == 0 || strncmp(strptr, "21", 2) == 0)
	{
		data->cp_stats.tooshort=0;
		printk("tooshort software counter cleared !\n");
	}
#ifdef RX_NAPI_MODE_DEBUG
		else if(strncmp(strptr, "napi_stat_reset", 15) == 0 || strncmp(strptr, "22", 2) == 0)
		{
			for(i=0 ; i<MAX_NAPI_STATISTIC_NUM ; i++)
			{
				if(napi_statistic[i])
				{
					napi_statistic[i] = 0;
				}
			}
			printk("napi_statistic software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_napi_gro_normal", 18) == 0 || strncmp(strptr, "23", 2) == 0)
		{
			data->cp_stats.rx_napi_gro_normal=0;
			printk("rx_napi_gro_normal software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_napi_gro_drop", 16) == 0 || strncmp(strptr, "24", 2) == 0)
		{
			data->cp_stats.rx_napi_gro_drop=0;
			printk("rx_napi_gro_drop software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_napi_gro_merged_free", 23) == 0 || strncmp(strptr, "25", 2) == 0)
		{
			data->cp_stats.rx_napi_gro_merged_free=0;
			printk("rx_napi_gro_merged_free software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_napi_gro_held", 16) == 0 || strncmp(strptr, "26", 2) == 0)
		{
			data->cp_stats.rx_napi_gro_held=0;
			printk("rx_napi_gro_held software counter cleared !\n");
		}
		else if(strncmp(strptr, "rx_napi_gro_merged", 18) == 0 || strncmp(strptr, "27", 2) == 0)
		{
			data->cp_stats.rx_napi_gro_merged=0;
			printk("rx_napi_gro_merged software counter cleared !\n");
		}
#endif
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("error input: \n");
		printk("1/ all - clear all\n");
		printk("2/ rx_hw_num - clear rx_hw_num\n");
		printk("3/ rx_sw_num - clear rx_sw_num\n");
		printk("4/ rer_runt - clear rer_runt\n");
		printk("5/ rer_ovf - clear rer_ovf\n");
		printk("6/ rdu - clear rdu\n");
		printk("7/ frag - clear frag\n");
		printk("8/ toobig - clear toobig\n");
		printk("9/ crcerr - clear crcerr\n");
		printk("10/ rcdf - clear rcdf\n");
		printk("11/ rx_no_mem - clear rx_no_mem\n");
		printk("12/ tx_sw_num - clear tx_sw_num\n");
		printk("13/ tx_hw_num - clear tx_hw_num\n");
		printk("14/ tx_no_desc - clear tx_no_desc\n");
		printk("15/ rx_critical_num - clear rx_critical_num\n");
		printk("16/ rx_cri_no_desc - clear rx_cri_no_desc\n");
#ifdef RX_NAPI_MODE
		printk("17/ rx_napi_poll - clear rx_napi_poll\n");
		printk("18/ rx_napi_interrupt - clear rx_napi_interrupt\n");
#endif
#if defined(CONFIG_RTL865X_ETH_PRIV_SKB)
		printk("19/ top_eth_skb_alloc_num - clear top_eth_skb_alloc_num\n");
		printk("20/ dynamic_alloc_skb_num - clear dynamic_alloc_skb_num\n");
#endif
		printk("21/	tooshort - clear tooshort\n");
	}
	return count;
}

static int rx_ring_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	unsigned int	ring_num, fc_enabled;
	static struct re_private *data;
	unsigned int gmac;
	char 	tmpbuf[64];
	char	*strptr;
	char	*tokptr;
	int 	i=0;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count-1] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		if(!strcmp(tokptr, "free"))
		{
			printk("\r\nFree 20 Rx skb to GMAC ring 0 !! \n\n");
			if(data->rx_Mring[0] == 0 || data->rx_skb[0] == 0)
			{
				printk("no rx_ring info!\n");
			}

			for(i=0;i<20;i++)
			{
				printk("[idx%3d]:desc[0x%p] \n", data->rx_Mtail[0], &data->rx_Mring[0][data->rx_Mtail[0]]);
				data->rx_Mring[0][data->rx_Mtail[0]].opts1 = (DescOwn | data->rx_buf_sz) | ((data->rx_Mtail[0] == (data->re8670_rx_ring_size[0] - 1))?RingEnd:0);
				updateGmacFlowControl(gmac, data->rx_Mtail[0], 0);
				data->rx_Mtail[0] = NEXT_RX(data->rx_Mtail[0],data->re8670_rx_ring_size[0]);
			}
		}
		else if(!strcmp(tokptr, "fc"))
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			ring_num=simple_strtol(tokptr, NULL, 0);
			if(ring_num>=MAX_RXRING_NUM)
			{
				goto errout;	
			}
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;	
			}
			fc_enabled=simple_strtol(tokptr, NULL, 0);
			if(fc_enabled != 0 && fc_enabled != 1)
			{
				goto errout;
			}
			re8686_set_flow_control(gmac, ring_num, fc_enabled);
			printk("\r\nGMAC%d ring%d flow control is %s \n", gmac, ring_num, fc_enabled?"Enabled":"Disabled");
			goto show_fc_result;
		}
		else if(!strcmp(tokptr, "?") || !strcmp(tokptr, "help"))
		{
			goto errout;
		}
		else 
		{
			data->rx_ring_show_bitmap = simple_strtol(tokptr, NULL, 0);
			printk("\r\nrx_ring_show_bitmap 0x%08x \n", data->rx_ring_show_bitmap);
		}
	}
	else
	{
errout:
		printk("\r\nRx ring operation only support \"fc\", \"?\" or \"help\" as the first parameter\n");
		printk("Ring flow control:	\"fc ring_num(0~%d) switch(0/1)\"\n", (MAX_RXRING_NUM-1));
		printk("rx_ring bitmap: 0x%08x\n", data->rx_ring_show_bitmap);
show_fc_result:
		printk("flow control of GMAC%d of each Rx ring:\n", gmac);
		for(i=0;i<MAX_RXRING_NUM;i++)
		{
			printk("	Rx ring %d is %s\n", i, (data->re8670_rx_flow_control_status[i]==GMAC_ON)?"ON":"OFF");
		}
	}
	return count;
}

static int rx_ring_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	int len = 0;
	int i = 0,j = 0;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	for(j=0;j<MAX_RXRING_NUM;j++)
	{
		if(data->rx_ring_show_bitmap&(1<<j))
		{
			printk("==================rx_ring [%d] info==================\n\n",j);
			if(data->rx_Mring[j] == 0 || data->rx_skb[j] == 0){
				printk("no rx_ring info!\n");
				continue;
			}

			for(i=0;i<data->re8670_rx_ring_size[j];i++){
				printk("[idx%3d]:desc[0x%p]->skb[0x%p]->buf[0x%08x]:%s", 
					i, &data->rx_Mring[j][i], data->rx_skb[j][i].skb, 
					data->rx_Mring[j][i].addr, 
					(data->rx_Mring[j][i].opts1 & DescOwn)? "NIC" : "CPU");
				if(i == data->rx_Mtail[j]){
					printk("<=rx_tail");
				}
				printk("\n");
			}
		}
	}
	return len;
}

static int tx_ring_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	char 	tmpbuf[64];
	char	*strptr;
	char	*tokptr;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count-1] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		data->tx_ring_show_bitmap = simple_strtol(tokptr, NULL, 0);
		printk("tx_ring_show_bitmap 0x%08x\n", data->tx_ring_show_bitmap);		
	}
	else
	{
errout:
		printk("tx_ring_show_bitmap: 0x%08x\n", data->tx_ring_show_bitmap);
	}
	return count;
}
static int tx_ring_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	int len = 0;
	int i = 0,j = 0;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	for(j=0;j<MAX_TXRING_NUM;j++)
	{
		if(data->tx_ring_show_bitmap&(1<<j))
		{
			printk("=============FDP%d:tx_ring [%d] info==================\n\n",idx_sw2hw(j)+1,j);
			if(data->tx_Mhqring[j] == 0 || data->tx_skb[j] == 0){
				printk("no tx_ring info\n");
				continue;
			}

			for(i=0;i<data->re8670_tx_ring_size[j];i++) {
				printk("[idx%3d]:desc[0x%p]->skb[0x%p]->buf[0x%08x]:%s", 
					i, &data->tx_Mhqring[j][i], data->tx_skb[j][i].skb, 
					data->tx_Mhqring[j][i].addr, 
					(data->tx_Mhqring[j][i].opts1 & DescOwn)? "NIC" : "CPU");
				if(i == data->tx_Mhqtail[j]) {
					printk("<=tx_hqtail");
				}
				if(i == data->tx_Mhqhead[j]) {
					printk("<=tx_hqhead");
				}
				printk("\n");
			}
		}
	}

	return len;
}


void nic_tx_ring_dump(unsigned int gmac)
{
	struct re_private *cp;
	DMA_TX_DESC *txd;
	int ring_index=0;
	int i;
	
	cp=re_private_data_root.re_private_data_ptr[gmac];	
	for(ring_index=0;ring_index<MAX_TXRING_NUM;ring_index++)
	{
		if(cp->tx_ring_show_bitmap&(1<<ring_index))
		{
	for(i=0;i<cp->re8670_tx_ring_size[ring_index];i++)
	{
		txd = (DMA_TX_DESC *)((u32)&cp->tx_Mhqring[ring_index][i]|0xa0000000);
				printk("%08x[%03d] %08x %08x %08x %08x %08x OWN=%d E=%d F=%d L=%d LEN=%05d LSO=%d MTU=%d",(u32)&txd->opts1,i,txd->opts1,txd->addr,txd->opts2,txd->opts3,txd->opts4
					,(txd->opts1&0x80000000)?1:0
					,(txd->opts1&0x40000000)?1:0
					,(txd->opts1&0x20000000)?1:0
					,(txd->opts1&0x10000000)?1:0
					,(txd->opts1&0x1ffff)
					,(txd->opts4&0x80000000)?1:0
					,(txd->opts4&0x7ff00000)>>20
				);
				if(i == cp->tx_Mhqtail[ring_index]) {
					printk("<=tx_hqtail");
				}
				if(i == cp->tx_Mhqhead[ring_index]) {
					printk("<=tx_hqhead");
				}
				printk("\n");
			}
			printk("\n");
		}
	}
	
#ifdef TX_RING_DEBUG
	if(cp->tx_ring_backup_debug)
	{
		printk("###########################Additional Tx ring debug info start###########################\n");
		for(ring_index=0;ring_index<MAX_TXRING_NUM;ring_index++)
		{
			if(cp->tx_ring_show_bitmap&(1<<ring_index))
			{
				for(i=0;i<cp->re8670_tx_ring_size[ring_index];i++)
				{
					txd = (DMA_TX_DESC *)((u32)&cp->rtl8686_tx_ring_debug[ring_index].txDescriptor[i]|0xa0000000);
					
					printk("%08x[%03d] %08x %08x %08x %08x %08x OWN=%d E=%d F=%d L=%d LEN=%05d LSO=%d MTU=%d\n",(u32)&txd->opts1,i,txd->opts1,txd->addr,txd->opts2,txd->opts3,txd->opts4
						,(txd->opts1&0x80000000)?1:0
						,(txd->opts1&0x40000000)?1:0
						,(txd->opts1&0x20000000)?1:0
						,(txd->opts1&0x10000000)?1:0
						,(txd->opts1&0x1ffff)
						,(txd->opts4&0x80000000)?1:0
						,(txd->opts4&0x7ff00000)>>20
					);
				}
				printk("\n");
			}
		}
		printk("###########################Additional Tx ring debug info end###########################\n");
	}
#endif	
}

static int nic_tx_ring_dump_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	nic_tx_ring_dump(gmac);
	return 0;
}

static int nic_tx_ring_dump_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{	
	return	tx_ring_write(filp, buf, count, offp);
}


void nic_rx_ring_dump(unsigned int gmac)
{
	struct re_private *cp;
	DMA_RX_DESC *rxd;
	int i;
	int ring_index=0;

	cp=re_private_data_root.re_private_data_ptr[gmac];	
	for(ring_index=0;ring_index<MAX_RXRING_NUM;ring_index++)
	{
		if(cp->rx_ring_show_bitmap&(1<<ring_index))
		{
			for(i=0;i<cp->re8670_rx_ring_size[ring_index];i++)
			{
				rxd = (DMA_RX_DESC *)((u32)&cp->rx_Mring[ring_index][i]|0xa0000000);
				printk("%08x[%03d] %08x %08x %08x %08x OWN=%d E=%d LEN=%05d",(u32)&rxd->opts1,i,rxd->opts1,rxd->addr,rxd->opts2,rxd->opts3
					,(rxd->opts1&0x80000000)?1:0
					,(rxd->opts1&0x40000000)?1:0
					,(rxd->opts1&0xfff)
				);
				if(ring_index < MAX_TXRING_NUM && i == cp->tx_Mhqtail[ring_index]) {
					printk("<=tx_hqtail");
				}
				if(ring_index < MAX_TXRING_NUM && i == cp->tx_Mhqhead[ring_index]) {
					printk("<=tx_hqhead");
				}
				printk("\n");
			}
			printk("\n");
		}
	}
}

static int nic_rx_ring_dump_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	nic_rx_ring_dump(gmac);
	return 0;
}

static int nic_rx_ring_dump_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	return	rx_ring_write(filp, buf, count, offp);
}

static int padding_enable_read(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	//printk("[padding_enable = %d]\n", data->padding_enable);
	switch (data->padding_enable) {
	case 0:
		printk("0, (padding is disable)\n");
		break;
	case 1:
		printk("1, (padding is enable)\n");
		break;
	default:
		printk("Error, (padding_enable is invalid)\n");
	}
	return 0;
}

void gmac_padding_enable(unsigned int gmac, char enable)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	
	if (gmac>2)
		goto error;
#ifndef CONFIG_GMAC1_USABLE
	if (gmac==1)
		goto error;
#endif
#ifndef CONFIG_GMAC2_USABLE
	if (gmac==2)
		goto error;
#endif
	if(enable==PADDING_ENABLED)
	{
		cp->padding_enable=1;
		RLE0787_W32(gmac, TCR, RLE0787_R32(gmac, TCR) & (~0x1));
		printk("Enable gmac[%d] padding\n", gmac);
	}
	else
	{
		cp->padding_enable=0;
		RLE0787_W32(gmac, TCR, RLE0787_R32(gmac, TCR) | 0x1);
		printk("Disable gmac[%d] padding\n", gmac);
	}
	
	return;
error:
	printk("%s: ignored gmac %d\n",__func__,gmac);
}

static int padding_enable_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int gmac;
	unsigned char tmpBuf[16] = {0};
	int i=0;	
	data=PDE_DATA(file_inode(filp));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}

	gmac = data->gmac;
	if (buf && count==2 && !copy_from_user(tmpBuf, buf, count))
	{
		tmpBuf[count] = '\0';
		i=simple_strtoul(tmpBuf, NULL, 10);
		switch (i) {
		case 0:
			gmac_padding_enable(gmac, PADDING_DISABLED);
			break;
		case 1:
			gmac_padding_enable(gmac, PADDING_ENABLED);
			break;
		default:
			printk("Unknown Setting\nDisable: echo 0 > /proc/rtl8686gmac/padding_enable\nEnable: echo 1 > /proc/rtl8686gmac/padding_enable\n");
			break;
		}
	}

	return count;
}

#ifdef CONFIG_REALTEK_HW_LSO
static int mtu_read(struct seq_file *m, void *v)
{
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	int i;
	
	for(i=0;i<totalDev;i++)
	{
		seq_printf(m, "%s: mtu=%d\n", rtl8686_dev_table[i].dev_instant->name, rtl8686_dev_table[i].dev_instant->mtu);
	}
	
	return 0;
}

static int mtu_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	unsigned char tmpBuf[64] = {0};
	char	*strptr;
	char	*tokptr;
	int mtu=0, i;
	
	if (buf && !copy_from_user(tmpBuf, buf, count))
	{
		tmpBuf[count] = '\0';
		strptr=tmpBuf;
		
		tokptr = strsep(&strptr," ");		
		if (tokptr==NULL)
		{
			goto errout;
		}

		for(i=0;i<totalDev;i++)
		{
			if(!strcmp(rtl8686_dev_table[i].dev_instant->name, tokptr))
			{
				tokptr = strsep(&strptr," ");
				if (tokptr==NULL)
				{
					goto errout;
				}
				
				if(tokptr[strlen(tokptr)-1] == 0x0a)
					tokptr[strlen(tokptr)-1] = 0x00;
				
				mtu=simple_strtoul(tokptr, NULL, 10);
				if(mtu != rtl8686_dev_table[i].dev_instant->mtu)
				{
					re8670_set_mtu(rtl8686_dev_table[i].dev_instant, mtu);
					return count;
				}
			}
		}
	}
	
errout:
	for(i=0;i<totalDev;i++)
	{
		printk("%s: mtu=%d\n", rtl8686_dev_table[i].dev_instant->name, rtl8686_dev_table[i].dev_instant->mtu);
	}

	return count;	
}
#endif

#ifdef CONFIG_RTL8192CD
static int wifi_tx_qos_mapping_read(struct seq_file *m, void *v)
{
	static struct re_private *data;
	struct file *file = m->private;
	unsigned int i;
	
	data=PDE_DATA(file_inode(file));
	if(!(data)){
		printk(KERN_INFO "Null data");
		return 0;
	}
	
	seq_printf(m, "Wifi Tx QoS mapping: \n");
	seq_printf(m, "  1,2: BK low\n");
	seq_printf(m, "  0,3: BE\n");
	seq_printf(m, "  4,5: VI\n");
	seq_printf(m, "  6,7: VO high\n");
	seq_printf(m, "  wifi_tx_qos_enable: %s\n", data->wifi_tx_qos_enable ? "ON":"OFF");
	if(data->wifi_tx_qos_enable)
	{
		for(i=0 ; i<GMAC_MAX_QUEUE_NUM ; i++)
		{
			seq_printf(m, "  internal priority %d mapping to wifi priority %d ", i, data->wifi_tx_qos_mapping[i]);
			switch(data->wifi_tx_qos_mapping[i])
			{
				case 1:
				case 2:
					seq_printf(m, "BK, queue 0.\n");
					break;
				case 0:
				case 3:
					seq_printf(m, "BE, queue 1.\n");
					break;
				case 4:
				case 5:
					seq_printf(m, "VI, queue 2.\n");
					break;
				case 6:
				case 7:
					seq_printf(m, "VO, queue 3.\n");
					break;
				default:
					seq_printf(m, "[WARNING] mapping FAIL !\n");
					break;
			}
		}
	}
	
	return 0;
}

static int wifi_tx_qos_mapping_write(struct file *filp, char *buf, size_t count, loff_t *offp )
{
	static struct re_private *data;
	unsigned int wifi_queue_index;
	unsigned int internal_priority;
	char 		tmpbuf[64];
	char		*strptr;
	char		*tokptr;
	unsigned long buf_size;

	/*
		1,2: BK low
		0,3: BE
		4,5: VI
		6,7: VO high
	*/

	data=PDE_DATA(file_inode(filp));
	buf_size = min(count, (sizeof(tmpbuf)-1));
	if (buf && !copy_from_user(tmpbuf, buf, buf_size)) {
		tmpbuf[count-1] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		internal_priority = simple_strtol(tokptr, NULL, 0);
		if(!strcmp(tokptr, "enable")) 
		{
			data->wifi_tx_qos_enable = GMAC_ON;
			printk("wifi_tx_qos_enable=%d ", data->wifi_tx_qos_enable);
		}
		else if(!strcmp(tokptr, "disable"))
		{
			data->wifi_tx_qos_enable = GMAC_OFF;
			printk("wifi_tx_qos_enable=%d ", data->wifi_tx_qos_enable);
		}
		else
		{
			if(internal_priority > 7)
			{
				printk("invalid internal_priority=%d ", internal_priority);
				goto errout;
			}
			
			printk("internal priority %d ", internal_priority);
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			wifi_queue_index = simple_strtol(tokptr, NULL, 0);

			if(wifi_queue_index > 3)
			{
				printk("invalid wifi_queue_index=%d ", wifi_queue_index);
				goto errout;
			}
			
			printk("mapping to wifi queue %d\n", wifi_queue_index);
			if(wifi_queue_index == 0)
				data->wifi_tx_qos_mapping[internal_priority] = 1;
			else if(wifi_queue_index == 1)
				data->wifi_tx_qos_mapping[internal_priority] = 3;
			else if(wifi_queue_index == 2)
				data->wifi_tx_qos_mapping[internal_priority] = 5;
			else if(wifi_queue_index == 3)
				data->wifi_tx_qos_mapping[internal_priority] = 7;
		}
	}
	else
	{
errout:
		printk("internal_priority wifi_queue_index\n");
		return -EFAULT;
	}
		
	return count;
}
#endif
#if defined(CONFIG_RTK_SOC_RTL8198D)
static int qos_dump_info(rtk_port_t port){
	rtk_qid_t queue;
	rtk_enable_t enable, IfgInclude;
	uint32 meterIdx, q_rate, p_rate;

	if(port>=SW_PORT_NUM)
		return 0;
	
	// get port BW
	if(rtk_rate_portEgrBandwidthCtrlRate_get(port,&p_rate) < 0){
		printk("%s:%d fail to get port rate for port%d!\n",__FUNCTION__,__LINE__,port);
		return 0;
	}
	printk("port[%d] | status:%d | BW:%d=======================\n", port,LCDev_mapping[port].status,p_rate);
	for(queue=0;queue<HAL_MAX_NUM_OF_QUEUE();queue++){
		meterIdx = 0;
		// get queue enable
		if(rtk_rate_egrQueueBwCtrlEnable_get(port,queue,&enable) < 0){
			printk("%s:%d fail to get enable for port%d queue%d!\n",__FUNCTION__,__LINE__,port,queue);
			return 0;
		}
		// get queue mapping to meterIdx
		if(rtk_rate_egrQueueBwCtrlMeterIdx_get(port,queue,&meterIdx) < 0){
			printk("%s:%d fail to get meterIdx for port%d queue%d!\n",__FUNCTION__,__LINE__,port,meterIdx);
			return 0;
		}else{
			meterIdx += (port%6)*8;
		}
		// get rate of meterIdx
		if(rtk_rate_shareMeter_get(meterIdx,&q_rate,&IfgInclude) < 0){
			printk("%s:%d fail to get rate or IfgInclude for meterIdx%d!\n",__FUNCTION__,__LINE__,meterIdx);
			return 0;
		}
		printk("queue%d -> enable=%d | meterIdx=%d | rate=%d | IfgInclude=%d\n", queue, enable, meterIdx, q_rate, IfgInclude);
	}
	
	return 1;
}

static void qos_help(void){
	int i;
	printk("port:");
	for(i=0;i<SW_PORT_NUM;i++){
		if(HAL_IS_PORT_EXIST(i))
			printk("%d,",i);
	}
	printk("queue:0~%d,enable:0/1,meterIdx:0~%d,rate:0~4194296\n",HAL_MAX_NUM_OF_QUEUE()-1,HAL_MAX_NUM_OF_METERING()-1);
	printk("Set queue enable:   echo set_en [port] [queue] [enable] > proc/rtl8686gmac/qos_cfg\n");
	printk("Map queue to Meter: echo q2meter [port] [queue] [meterIdx] > proc/rtl8686gmac/qos_cfg\n");
	printk("Set Meter rate:     echo set_rate [MeterIdx] [rate] > proc/rtl8686gmac/qos_cfg\n");
	printk("Get port info :     echo r [port] > proc/rtl8686gmac/qos_cfg\n");
	return;
}

static int qos_cfg_read(struct seq_file *m, void *v){
	rtk_port_t port;
	for(port=0;port<SW_PORT_NUM;port++){
		if(!HAL_IS_PORT_EXIST(port))
			continue;
		if(!qos_dump_info(port)){
			printk("%s:%d dump qos_cfg fail for port %d!\n",__FUNCTION__,__LINE__,port);
			return 0;
		}
	}
	return 0;
}

static int qos_cfg_write(struct file *filp, char *buf, size_t count, loff_t *offp ){
	char tmpbuf[64];
	char  *strptr,*tokptr;
	uint32 cmd_buff[2] = {0};
	int i=0;
	rtk_port_t port;
	rtk_qid_t queue;
	uint32 meterIdx, meterRate;
	rtk_enable_t enable;
	int len = (count > 63) ? 63 : count;
	
	if (buf && !copy_from_user(tmpbuf, buf, len)) {
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
			goto err;
		
		if(!memcmp(tokptr, "q2meter", 7)){// queue map to meter
			if(sscanf(buf,"q2meter %d %d %d",&port,&queue,&meterIdx)!=3){
				printk("%s:%d Invalid input!\n",__FUNCTION__,__LINE__);
				goto err;
			}
			if(!HAL_IS_PORT_EXIST(port) || queue>=HAL_MAX_NUM_OF_QUEUE() || meterIdx>=HAL_MAX_NUM_OF_METERING()){ 
				printk("%s:%d variables %d,%d,%d out of range!\n",__FUNCTION__,__LINE__,port,queue,meterIdx);
				goto err;
			}
			else{
				if(rtk_rate_egrQueueBwCtrlMeterIdx_set(port, queue, meterIdx)<0){
					printk("%s:%d fail in rtk_rate_shareMeter_set!\n",__FUNCTION__,__LINE__);
					goto err;
				}
				else
					printk("%s:%d rtk_rate_shareMeter_set success!\n",__FUNCTION__,__LINE__);
			}
			
		}
		else if(!memcmp(tokptr, "set_en", 6)){// queue enable
			if(sscanf(buf,"set_en %d %d %d",&port,&queue,&enable)!=3){
				printk("%s:%d Invalid input!\n",__FUNCTION__,__LINE__);
				goto err;
			}
			if(!HAL_IS_PORT_EXIST(port) || queue>=HAL_MAX_NUM_OF_QUEUE() || enable>1){
				printk("%s:%d variables %d,%d,%d out of range!\n",__FUNCTION__,__LINE__,port,queue,enable);
				goto err;
			}
			else{
				if(rtk_rate_egrQueueBwCtrlEnable_set(port, queue, enable)<0){
					printk("%s:%d fail in rtk_rate_egrQueueBwCtrlEnable_set!\n",__FUNCTION__,__LINE__);
					goto err;
				}
				else
					printk("%s:%d rtk_rate_egrQueueBwCtrlEnable_set success!\n",__FUNCTION__,__LINE__);
			}
		}
		else if(!memcmp(tokptr, "set_rate", 8)){// meter rate
			if(sscanf(buf,"set_rate %d %d",&meterIdx,&meterRate)!=2){
				printk("%s:%d Invalid input!\n",__FUNCTION__,__LINE__);
				goto err;
			}
			if(meterIdx>=HAL_MAX_NUM_OF_METERING() || meterRate> (0x7ffff<<3) ){
				printk("%s:%d invalid variables!\n",__FUNCTION__,__LINE__);
				goto err;
			}
			else{
				if(rtk_rate_shareMeter_set(meterIdx, meterRate, 1)<0){
					printk("%s:%d fail in rtk_rate_shareMeter_set!\n",__FUNCTION__,__LINE__);
					goto err;
				}
				else
					printk("%s:%d rtk_rate_shareMeter_set success!\n",__FUNCTION__,__LINE__);
			}
		}
		else if(!memcmp(tokptr, "r", 1)){// read info
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto err;
			port = (rtk_port_t)simple_strtol(tokptr, NULL, 0);
			if(!HAL_IS_PORT_EXIST(port)){
				printk("%s:%d Invalid port %d!\n",__FUNCTION__,__LINE__,port);
				goto err;
			}
			else if(!qos_dump_info(port)){
				printk("%s:%d Fail to dump qos info!\n",__FUNCTION__,__LINE__);
				goto err;
			}
		}
		else
			goto err;
	}
	else{
err:
		qos_help();
	}

	return count;

}

static void asic_counter_help(void){
	int i;
	printk("port:");
	for(i=0;i<SW_PORT_NUM;i++){
		if(HAL_IS_PORT_EXIST(i))
			printk("%d,",i);
	}
	printk("\n");
	printk("Dump port counter info             : echo r [port] > proc/rtl8686gmac/asic_counter\n");
	printk("Reset mib counter for single port  : echo clear [port] > proc/rtl8686gmac/asic_counter\n");
	printk("Reset mib counter for all ports    : echo clear all > proc/rtl8686gmac/asic_counter\n");
	return;
}

const char *counterStr[] ={
	MIB_COUNTERSTR_IF_IN_OCTETS,
    MIB_COUNTERSTR_IF_IN_UCAST_PKTS,
    MIB_COUNTERSTR_F_IN_MULTICAST_PKTS,
    MIB_COUNTERSTR_IF_IN_BROADCAST_PKTS,
    MIB_COUNTERSTR_IF_IN_DISCARDS,
    MIB_COUNTERSTR_IF_OUT_OCTETS,
    MIB_COUNTERSTR_IF_OUT_DISCARDS,
    MIB_COUNTERSTR_IF_OUT_UCAST_PKTS_CNT,
    MIB_COUNTERSTR_IF_OUT_MULTICAST_PKTS_CNT,
    MIB_COUNTERSTR_IF_OUT_BROADCAST_PKTS_CNT,
    MIB_COUNTERSTR_DOT1D_PORT_DELAY_EXCEEDED_DISCARDS,
    MIB_COUNTERSTR_DOT1D_TP_PORT_IN_DISCARDS,
    MIB_COUNTERSTR_DOT1D_TP_HC_PORT_IN_DISCARDS,
    MIB_COUNTERSTR_DOT3_IN_PAUSE_FRAMES,
    MIB_COUNTERSTR_DOT3_OUT_PAUSE_FRAMES,
    MIB_COUNTERSTR_DOT3_OUT_PAUSE_ON_FRAMES,
    MIB_COUNTERSTR_DOT3_STATS_ALIGNMENT_ERRORS,
    MIB_COUNTERSTR_DOT3_STATS_FCS_ERRORS,
    MIB_COUNTERSTR_DOT3_STATS_SINGLE_COLLISION_FRAMES,
    MIB_COUNTERSTR_DOT3_STATS_MULTIPLE_COLLISION_FRAMES,
    MIB_COUNTERSTR_DOT3_STATS_DEFERRED_TRANSMISSIONS,
    MIB_COUNTERSTR_DOT3_STATS_LATE_COLLISIONS,
    MIB_COUNTERSTR_DOT3_STATS_EXCESSIVE_COLLISIONS,
    MIB_COUNTERSTR_DOT3_STATS_FRAME_TOO_LONGS,
    MIB_COUNTERSTR_DOT3_STATS_SYMBOL_ERRORS,
    MIB_COUNTERSTR_DOT3_CONTROL_IN_UNKNOWN_OPCODES,
    MIB_COUNTERSTR_ETHER_STATS_DROP_EVENTS,
    MIB_COUNTERSTR_ETHER_STATS_OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_BROADCAST_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_MULTICAST_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_UNDER_SIZE_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_OVERSIZE_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_FRAGMENTS,
    MIB_COUNTERSTR_ETHER_STATS_JABBERS,
    MIB_COUNTERSTR_ETHER_STATS_COLLISIONS,
    MIB_COUNTERSTR_ETHER_STATS_CRC_ALIGN_ERRORS,
    MIB_COUNTERSTR_ETHER_STATS_PKTS_64OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_PKTS_65TO127OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_PKTS_128TO255OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_PKTS_256TO511OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_PKTS_512TO1023OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_PKTS_1024TO1518OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_UNDER_SIZE_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_TX_OVERSIZE_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_TX_PKTS_64OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_PKTS_65TO127OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_PKTS_128TO255OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_PKTS_256TO511OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_PKTS_512TO1023OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_PKTS_1024TO1518OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_PKTS_1519TOMAXOCTETS,
    MIB_COUNTERSTR_ETHER_STATS_TX_BROADCAST_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_TX_MULTICAST_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_TX_FRAGMENTS,
    MIB_COUNTERSTR_ETHER_STATS_TX_JABBERS,
    MIB_COUNTERSTR_ETHER_STATS_TX_CRC_ALIGN_ERRORS,
    MIB_COUNTERSTR_ETHER_STATS_RX_UNDER_SIZE_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_RX_UNDER_SIZE_DROP_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_RX_OVERSIZE_PKTS,
    MIB_COUNTERSTR_ETHER_STATS_RX_PKTS_64OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_RX_PKTS_65TO127OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_RX_PKTS_128TO255OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_RX_PKTS_256TO511OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_RX_PKTS_512TO1023OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_RX_PKTS_1024TO1518OCTETS,
    MIB_COUNTERSTR_ETHER_STATS_RX_PKTS_1519TOMAXOCTETS,
    MIB_COUNTERSTR_IN_OAM_PDU_PKTS,
    MIB_COUNTERSTR_OUT_OAM_PDU_PKTS
};

static int asic_counter_dump_info(rtk_port_t port){
	uint64 cntr;
	rtk_stat_port_type_t type;
	
	if(!HAL_IS_PORT_EXIST(port))
		return 0;

	printk("Port: %d\n", port);
	for (type = 0; type < MIB_PORT_CNTR_END; type++){
		if(rtk_stat_port_get(port, type, &cntr) == RT_ERR_OK)
			printk("%-35s: %25llu\n",counterStr[type],cntr);
	}
	return 1;
}

static int asic_counter_read(struct seq_file *m, void *v){
	rtk_port_t port;
	for(port=0;port<SW_PORT_NUM;port++){
		if(!HAL_IS_PORT_EXIST(port))
			continue;
		if(!asic_counter_dump_info(port)){
			printk("%s:%d dump asic_counter fail for port %d!\n",__FUNCTION__,__LINE__,port);
			return 0;
		}
	}
	return 0;
}

static int asic_counter_write(struct file *filp, char *buf, size_t count, loff_t *offp ){
	char tmpbuf[64];
	char *strptr, *tokptr;
	rtk_port_t port;
	int len = (count > 63) ? 63 : count;
	
	if (buf && !copy_from_user(tmpbuf, buf, len)) {
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
			goto err;
		
		if(!memcmp(tokptr, "r", 1)){// read mib counter
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto err;
			port = (rtk_port_t)simple_strtol(tokptr, NULL, 0);
			if(!HAL_IS_PORT_EXIST(port) || !asic_counter_dump_info(port)){
				printk("%s:%d error in dumping port info!\n",__FUNCTION__,__LINE__);
				goto err;
			}
		}
		else if(!memcmp(tokptr, "clear", 5)){
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto err;
			if(!memcmp(tokptr, "all", 3)){// reset all ports
				for(port=0;port<SW_PORT_NUM;port++){
					if(!HAL_IS_PORT_EXIST(port))
						continue;
					if(rtk_stat_port_reset(port) != RT_ERR_OK){
						printk("%s:%d fail to reset mib counter for port %d!\n",__FUNCTION__,__LINE__,port);
						goto err;
					}
				}
			}
			else{// reset single port
				port = (rtk_port_t)simple_strtol(tokptr, NULL, 0);
				if(!HAL_IS_PORT_EXIST(port)){
					printk("%s:%d Invalid port %d!\n",__FUNCTION__,__LINE__,port);
					goto err;
				}
				if(rtk_stat_port_reset(port) != RT_ERR_OK){
					printk("%s:%d reset port %d error!\n",__FUNCTION__,__LINE__,port);
					goto err;
				}
				else{
					printk("%s:%d reset port %d successfully!\n",__FUNCTION__,__LINE__,port);
					asic_counter_dump_info(port);
				}
			}
		}
		else{
			goto err;
		}
	}
	else{
err:
		printk("%s:%d error!\n",__FUNCTION__,__LINE__);
		asic_counter_help();
	}
	
	return count;
}

const char *Str_portSpeed[] = {
    PORT_STR_SPEED_10M,
    PORT_STR_SPEED_100M,
    PORT_STR_SPEED_GIGA,
    PORT_STR_SPEED_500M,
    PORT_STR_SPEED_10G,
    PORT_STR_SPEED_2G5,    
    PORT_STR_SPEED_5G,
    PORT_STR_SPEED_2G5LITE,
    PORT_STR_SPEED_5GLITE,
    
};
const char *Str_portDuplex[] = {
	PORT_STR_HALF_DUPLEX,
	PORT_STR_FULL_DUPLEX
};
	
const char *Str_portLinkStatus[] = {
	PORT_STR_LINK_DOWN,
	PORT_STR_LINK_UP
};
		
const char *Str_enDisplay[] = {
	PORT_STR_X,
	PORT_STR_V
};

static void port_status_help(void){
	int i;
	printk("port:");
	for(i=0;i<SW_PORT_NUM;i++){
		if(HAL_IS_PORT_EXIST(i))
			printk("%d,",i);
	}
	printk("\n");
	printk("Dump port status             : echo get [port] > proc/rtl8686gmac/port_status\n");
	return;
}

static int port_dump_info(rtk_port_t port){
	rtk_port_linkStatus_t linkStatus;
	rtk_port_speed_t speed;
	rtk_port_duplex_t duplex;
	uint32 txfc,rxfc;
	
	if(!HAL_IS_PORT_EXIST(port))
		return 0;
	
	if(rtk_port_link_get(port, &linkStatus)!= RT_ERR_OK){
		printk("%s:%d rt_port_link_get fail!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	if(rtk_port_speedDuplex_get(port, &speed, &duplex)!=RT_ERR_OK){
		printk("%s:%d rt_port_speedDuplex_get fail!\n",__FUNCTION__,__LINE__);
		return 0;
	}
	if(rtk_port_flowctrl_get(port, &txfc, &rxfc)!=RT_ERR_OK){
		printk("%s:%d rt_port_flowctrl_get fail!\n",__FUNCTION__,__LINE__);
		return 0;
	}

	printk("Port:%d Status:%s Speed:%s Duplex:%s TX_FC:%s RX_FC:%s\n",port,Str_portLinkStatus[linkStatus],
																			Str_portSpeed[speed],
																			Str_portDuplex[duplex],
																			Str_enDisplay[txfc],
																			Str_enDisplay[rxfc]);
	return 1;
}

static int port_status_write(struct file *filp, char *buf, size_t count, loff_t *offp ){
	char tmpbuf[64];
	char *strptr,*tokptr;
	rtk_port_t port;
	int len = (count > 63) ? 63 : count;
		
	if (buf && !copy_from_user(tmpbuf, buf, len)) {
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
			goto err;
		if(!memcmp(tokptr, "get", 3)){
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto err;
			port = (rtk_port_t)simple_strtol(tokptr, NULL, 0);
			if(!HAL_IS_PORT_EXIST(port)){
				printk("%s:%d Invalid port %d!\n",__FUNCTION__,__LINE__,port);
				goto err;
			}
			if(!port_dump_info(port)){
				printk("%s:%d fail to dump port %d status!\n",__FUNCTION__,__LINE__,port);
				goto err;
			}
		}
		else
			goto err;
	}
	else{
err:
		printk("%s:%d error!\n",__FUNCTION__,__LINE__);
		port_status_help();
	}

	return count;

}
static int port_status_read(struct seq_file *m, void *v){
	rtk_port_t port;
	for(port=0;port<SW_PORT_NUM;port++){
		if(!HAL_IS_PORT_EXIST(port))
			continue;
		if(!port_dump_info(port)){
			printk("%s:%d dump port status fail for port %d!\n",__FUNCTION__,__LINE__,port);
			return 0;
		}
	}
	return 0;
}

int meminfo_write(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	return count;
}

int meminfo_read(struct seq_file *m, void *v)
{
	int i, j, k;
	int ring_size, rx_buff_size, rx_buff_size_pad, rx_desc_size, tx_desc_size;
	int total_rx_skb = 0, total_rx_desc = 0, total_tx_desc = 0;
	int total_rx_skb_r = 0, total_rx_desc_r = 0, total_tx_desc_r = 0;	// for kmalloc roundup

	seq_printf(m, "MAX_GMAC_NUM: %d\n", MAX_GMAC_NUM);
	seq_printf(m, "MAX_RXRING_NUM: %d\n", MAX_RXRING_NUM);
	seq_printf(m, "MAX_TXRING_NUM: %d\n", MAX_TXRING_NUM);
	seq_printf(m, "SKB_BUF_SIZE: %d\n", SKB_BUF_SIZE);
	seq_printf(m, "NET_SKB_PAD: %d\n\n", NET_SKB_PAD);

	rx_buff_size = SKB_BUF_SIZE;
	rx_buff_size_pad = SKB_BUF_SIZE + NET_SKB_PAD;

	for (i = 0; i < MAX_GMAC_NUM; i++) {
		seq_printf(m, "[GMAC%d]\n", i);
		for (j = 0; j < MAX_RXRING_NUM; j++) {
			ring_size = re_private_data_root.re_private_data_ptr[i]->re8670_rx_ring_size[j];
			rx_desc_size = RE8670_RXRING_BYTES(ring_size);
			rx_buff_size_pad = rx_buff_size + NET_SKB_PAD;

			seq_printf(m, "\tring%d_size: %5d \t mem_rx_desc: %10d \t mem_rx_skb: %10d\n", 
				j, ring_size, rx_desc_size, (ring_size * rx_buff_size_pad));

			total_rx_skb += (ring_size * rx_buff_size_pad);
			total_rx_desc += rx_desc_size;

			total_rx_skb_r += (ring_size * (1 << fls(rx_buff_size_pad - 1)));
			total_rx_desc_r += (1 << fls(rx_desc_size - 1));
		}

		seq_printf(m, "\n%s\n", "    [TX]");
		for (j = 0; j < MAX_TXRING_NUM; j++) {
			ring_size = re_private_data_root.re_private_data_ptr[i]->re8670_tx_ring_size[j];
			tx_desc_size = RE8670_TXRING_BYTES(ring_size);

			seq_printf(m, "\tring%d_size: %5d \t mem_tx_desc: %10d\n", j, ring_size, tx_desc_size);
			total_tx_desc += tx_desc_size;
			total_tx_desc_r += (1 << fls(tx_desc_size - 1));
		}
	}

	seq_printf(m, "\n[malloc] %d = %d + %d + %d = total_rx_skb + total_rx_desc + total_tx_desc\n", 
		(total_rx_skb + total_rx_desc + total_tx_desc), total_rx_skb, total_rx_desc, total_tx_desc);

	seq_printf(m, "\n[malloc(R)] %d = %d + %d + %d = total_rx_skb_r + total_rx_desc_r + total_tx_desc_r\n", 
		(total_rx_skb_r + total_rx_desc_r + total_tx_desc_r), total_rx_skb_r, total_rx_desc_r, total_tx_desc_r);

	return 0;
}
#endif
static int dbg_level_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}

static const struct file_operations dbglv_fops = {
        .owner          = THIS_MODULE,
        .open           = dbg_level_open,
        .read           = dbg_level_read,
        .write          = dbg_level_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int dbg_times_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}

static const struct file_operations dbgtimes_fops = {
        .owner          = THIS_MODULE,
        .open           = dbg_times_open,
        .read           = dbg_times_read,
        .write          = dbg_times_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int hwreg_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations hwreg_fops = {
        .owner          = THIS_MODULE,
        .open           = hwreg_open,
        .read           = hw_reg_read,
        .write          = hw_reg_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int sw_cnt_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations swcnt_fops = {
        .owner          = THIS_MODULE,
        .open           = sw_cnt_open,
        .read           = sw_cnt_read,
        .write           = sw_cnt_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int rxring_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations rxring_fops = {
        .owner          = THIS_MODULE,
        .open           = rxring_open,
        .read           = rx_ring_read,
        .llseek         = seq_lseek,
		.write          = rx_ring_write,
        .release        = single_release,
};

static int txring_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations txring_fops = {
        .owner          = THIS_MODULE,
        .open           = txring_open,
        .read           = tx_ring_read,
        .llseek         = seq_lseek,
		.write          = tx_ring_write,
        .release        = single_release,
};

static int memrw_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations mem_fops = {
        .owner          = THIS_MODULE,
        .open           = memrw_open,
        .read           = memrw_read,
        .write          = memrw_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

int dev_port_mapping_open(struct inode *inode, struct file *file)
{
	return single_open(file, dev_port_mapping_read, (void *) file);
}

static const struct file_operations devport_fops = {
	.owner		= THIS_MODULE,
	.open		= dev_port_mapping_open,
	.read		= seq_read,
	.write		= dev_port_mapping_write,
	.llseek     = seq_lseek,
	.release	= single_release,
};

static int misc_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations misc_fops = {
        .owner          = THIS_MODULE,
        .open           = misc_open,
        .read           = misc_read,
        .write          = misc_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int port_to_rxfunc_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations port2rx_fops = {
        .owner          = THIS_MODULE,
        .open           = port_to_rxfunc_open,
        .read           = port_to_rxfunc_read,
        .write          = port_to_rxfunc_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

#ifdef CONFIG_RTL8686_SWITCH
#ifndef CONFIG_RTK_L34_ENABLE
static int swmode_open(struct inode *inode, struct file *file)
{
        return single_open(file, switch_mode_read, inode->i_private);
}
static const struct file_operations swmode_fops = {
        .owner          = THIS_MODULE,
        .open           = swmode_open,
        .read           = seq_read,
        .write          = switch_mode_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#endif // CONFIG_RTK_L34_ENABLE
#endif // CONFIG_RTL8686_SWITCH

#ifdef RE8686_VERIFY
static int verify_open(struct inode *inode, struct file *file)
{
        return single_open(file, verify_read, inode->i_private);
}
static const struct file_operations verify_fops = {
        .owner          = THIS_MODULE,
        .open           = verify_open,
        .read           = seq_read,
        .write          = verify_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#endif //RE8686_VERIFY

static int txdetail_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations txdetail_fops = {
        .owner          = THIS_MODULE,
        .open           = txdetail_open,
        .read           = nic_tx_ring_dump_read,
        .write          = nic_tx_ring_dump_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int rxdetail_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations rxdetail_fops = {
        .owner          = THIS_MODULE,
        .open           = rxdetail_open,
        .read           = nic_rx_ring_dump_read,
        .write          = nic_rx_ring_dump_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int padding_open(struct inode *inode, struct file *file)
{
        return single_open(file, NULL, inode->i_private);
}
static const struct file_operations padding_fops = {
        .owner          = THIS_MODULE,
        .open           = padding_open,
        .read           = padding_enable_read,
        .write          = padding_enable_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};

#ifdef CONFIG_REALTEK_HW_LSO
static int mtu_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtu_read, (void *) file);
}
static const struct file_operations mtu_fops = {
	.owner          = THIS_MODULE,
	.open           = mtu_open,
	.read           = seq_read,
	.write          = mtu_write,
	.llseek         = seq_lseek,
	.release        = single_release,
};
#endif

#ifdef CONFIG_RTL8192CD
int wifi_tx_qos_mapping_open(struct inode *inode, struct file *file)
{
	return single_open(file, wifi_tx_qos_mapping_read, (void *) file);
}

static const struct file_operations wifitxqos_fops = {
	.owner		= THIS_MODULE,
	.open		= wifi_tx_qos_mapping_open,
	.read		= seq_read,
	.write		= wifi_tx_qos_mapping_write,
	.llseek     = seq_lseek,
	.release	= single_release,
};
#endif

#if defined(CONFIG_RTK_SOC_RTL8198D)
int qos_cfg_open(struct inode *inode, struct file *file)
{
	return single_open(file, qos_cfg_read, (void *) file);
}

static const struct file_operations qos_fops = {
	.owner		= THIS_MODULE,
	.open		= qos_cfg_open,
	.read		= seq_read,
	.write		= qos_cfg_write,
	.llseek     = seq_lseek,
	.release	= single_release,
};

int asic_counter_open(struct inode *inode, struct file *file)
{
	return single_open(file, asic_counter_read, (void *) file);
}

static const struct file_operations asicounter_fops = {
	.owner		= THIS_MODULE,
	.open		= asic_counter_open,
	.read		= seq_read,
	.write		= asic_counter_write,
	.llseek 	= seq_lseek,
	.release	= single_release,
};

int port_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, port_status_read, (void *) file);
}

static const struct file_operations portstatus_fops = {
	.owner		= THIS_MODULE,
	.open		= port_status_open,
	.read		= seq_read,
	.write		= port_status_write,
	.llseek     = seq_lseek,
	.release	= single_release,
};

int meminfo_open(struct inode *inode, struct file *file)
{
	return single_open(file, meminfo_read, (void *) file);
}

static const struct file_operations meminfo_fops = {
	.owner		= THIS_MODULE,
	.open		= meminfo_open,
	.read		= seq_read,
	.write		= meminfo_write,
	.llseek     = seq_lseek,
	.release	= single_release,
};
#endif

#define RTL8686_GMAC0_PROC_DIR_NAME "rtl8686gmac"
#define RTL8686_GMAC1_PROC_DIR_NAME "rtl8686gmac1"
#define RTL8686_GMAC2_PROC_DIR_NAME "rtl8686gmac2"
unsigned char *rtl8686_proc_dir_name[MAX_GMAC_NUM] =
{
	RTL8686_GMAC0_PROC_DIR_NAME,
	RTL8686_GMAC1_PROC_DIR_NAME,
	RTL8686_GMAC2_PROC_DIR_NAME
};
struct proc_dir_entry *rtl8686_proc_dir[MAX_GMAC_NUM]={0};

struct gmac_proc_entry{
	struct proc_dir_entry* p_dir_entry;
	char* entry_name;
	struct file_operations *fops;
};

static void rtl8686_proc_debug_init(unsigned int gmac)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	
	if(cp->gmac_enabled != GMAC_TRUE)
		return -1;
	
	if(rtl8686_proc_dir[gmac]==NULL)
		rtl8686_proc_dir[gmac] = proc_mkdir(rtl8686_proc_dir_name[gmac], NULL);
	
	if(rtl8686_proc_dir[gmac])
	{
		proc_create_data("dbg_level", 0, rtl8686_proc_dir[gmac], &dbglv_fops, cp);
		proc_create_data("dbg_times", 0, rtl8686_proc_dir[gmac], &dbgtimes_fops, cp);
		proc_create_data("hw_reg", 0, rtl8686_proc_dir[gmac], &hwreg_fops, cp);
		proc_create_data("sw_cnt", 0, rtl8686_proc_dir[gmac], &swcnt_fops, cp);
		proc_create_data("rx_ring", 0, rtl8686_proc_dir[gmac], &rxring_fops, cp);
		proc_create_data("tx_ring", 0, rtl8686_proc_dir[gmac], &txring_fops, cp);
		proc_create_data("mem", 0, rtl8686_proc_dir[gmac], &mem_fops, cp);
		proc_create_data("dev_port_mapping", 0, rtl8686_proc_dir[gmac], &devport_fops, cp);
		proc_create_data("misc", 0, rtl8686_proc_dir[gmac], &misc_fops, cp);
		proc_create_data("port2rxfunc", 0, rtl8686_proc_dir[gmac], &port2rx_fops, cp);
		#ifdef CONFIG_RTL8686_SWITCH
		#ifndef CONFIG_RTK_L34_ENABLE
		proc_create_data("switch_mode", 0, rtl8686_proc_dir[gmac], &swmode_fops, cp);
		#endif
		#endif
		#ifdef RE8686_VERIFY
		proc_create_data("verify", 0, rtl8686_proc_dir[gmac], &verify_fops, cp);
		#endif
		proc_create_data("rx_ring_detail", 0, rtl8686_proc_dir[gmac], &rxdetail_fops, cp);
		proc_create_data("tx_ring_detail", 0, rtl8686_proc_dir[gmac], &txdetail_fops, cp);
		proc_create_data("padding_enable", 0, rtl8686_proc_dir[gmac], &padding_fops, cp);
#ifdef CONFIG_REALTEK_HW_LSO
		proc_create_data("mtu", 0, rtl8686_proc_dir[gmac], &mtu_fops, cp);
#endif
#ifdef CONFIG_RTL8192CD
		proc_create_data("wifi_tx_qos_mapping", 0, rtl8686_proc_dir[gmac], &wifitxqos_fops, cp);
#endif
#if defined(CONFIG_RTK_SOC_RTL8198D)
		proc_create_data("qos_cfg", 0, rtl8686_proc_dir[gmac], &qos_fops, cp);
		proc_create_data("asic_counter", 0, rtl8686_proc_dir[gmac], &asicounter_fops, cp);
		proc_create_data("port_status", 0, rtl8686_proc_dir[gmac], &portstatus_fops, cp);
		proc_create_data("meminfo", 0, rtl8686_proc_dir[gmac], &meminfo_fops, cp);
#endif

	}
}

void port_relate_setting(unsigned int gmac) 
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	int i, j;

	re8686_reset_rxfunc_to_default(gmac);
	// port2dev: default set to eth0, eth1 and eth3 respectly
	for(j=0;j<SW_PORT_NUM;j++) {
		cp->port2dev[j] = eth_net_dev[gmac];
	}
	
	for(i=0;i<totalDev;i++) {
		if(rtl8686_dev_table[i].dev_instant==NULL)
			continue;
		DEVPRIV(rtl8686_dev_table[i].dev_instant)->txPortMask = 
			IS_CPU_PORT(rtl8686_dev_table[i].phyPort) ? 0 : (1<<rtl8686_dev_table[i].phyPort);
	}
}

#if defined(CONFIG_APOLLO_ROMEDRIVER) //&& defined(CONFIG_GPON_FEATURE)
extern int rtk_rg_fwdEngine_xmit (struct sk_buff *skb, struct net_device *dev);
#endif

#ifdef CONFIG_RTK_HOST_SPEEDUP
extern struct tx_info speedup_txDesc,speedup_txDescMask;
#endif
#if defined(CONFIG_APOLLO_ROMEDRIVER)
__IRAM_NIC int re8670_start_xmit_check(struct sk_buff *skb, struct net_device *dev)
{
        // direct tx
        if(memcmp(skb->dev->name,"wlan1",5)==0 || memcmp(skb->dev->name,"vwlan", 5)==0)
        {
                return re8670_start_xmit_txInfo(skb,dev,NULL,NULL);
        }
        else
        {       // send to fwdEngine
			struct tcphdr *tcph=NULL;
			unsigned char *pData = skb->data;
			struct iphdr *iph=NULL;	
			struct ipv6hdr *ip6h=NULL;	
			void*srcaddr,*dstaddr;
			void*l3hdr;
			void *pHostTrack;
			int ret;
			int ipver=0;

#ifdef CONFIG_RTK_HOST_SPEEDUP
            if (isHostSpeedUpEnable())
            {
                pData += 12;
                if (*(unsigned short *)pData == htons(0x8100))
                        pData += 4;
				if(*(unsigned short *)pData == htons(0x8864))
				{
					if(*(unsigned short *)(pData+8) == htons(0x0021))
						iph = (struct iphdr *)(pData + 10);
					else if(*(unsigned short *)(pData+8) == htons(0x0057))
					{
						ip6h = (struct iphdr *)(pData + 10);
						pData+=10;
					}
				}
				else if(*(unsigned short *)pData == htons(0x0800))
				{
					iph = (struct iphdr *)(pData + 2);
				}
				else if(*(unsigned short *)pData == htons(0x86dd))
				{
					ip6h = (struct iphdr *)(pData + 2);
					pData+=2;
				}
				if(iph)
				{
					ipver = 0;
					srcaddr = &(iph->saddr);
					dstaddr = &(iph->daddr);
					l3hdr = iph;
					if(IPPROTO_TCP == iph->protocol)
						tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);
				}
				else if(ip6h)
				{
					u8 nexthead = ip6h->nexthdr;
					ipver = 1;
					srcaddr = &(ip6h->saddr);
					dstaddr = &(ip6h->daddr);
					l3hdr = ip6h;
					pData += 40; 
					while ( (nexthead == IPPROTO_HOPOPTS) || (nexthead == IPPROTO_ROUTING) || (nexthead == IPPROTO_FRAGMENT)
						 || (nexthead == IPPROTO_DSTOPTS) || (nexthead == IPPROTO_MH))
					{
						nexthead = *pData;			
						pData += (*(pData+1)+1)<<3;
					}
					if(nexthead == IPPROTO_TCP)
						tcph = pData;
				}
				if(tcph)
				{
					read_lock_bh(&speedup_lock);
                    if ((pHostTrack = shouldSpeedUp(1, dstaddr, srcaddr, tcph->dest, tcph->source, ipver)) != NULL)
                    {
						ret = speedtest_stream_shortcut_tx_learn(pHostTrack, l3hdr, tcph, skb);
						read_unlock_bh(&speedup_lock);
						re8670_start_xmit_txInfo(skb, dev, &speedup_txDesc, &speedup_txDescMask);
						if (ret)
						{
							start_xmit_upload_data();
						}
						return 0;
                    }
					read_unlock_bh(&speedup_lock);
				}
            }
#endif

#ifdef	SMBSHORTCUT
			if ( smbscflag ){
				pData += 12;
				if (*(unsigned short *)pData == htons(0x8100))
						pData += 4;
				if (((*(unsigned short *)pData == htons(0x8864)) && (*(unsigned short *)(pData+8) == htons(0x0021))) ||
						(*(unsigned short *)pData == htons(0x0800)))
				{
					
					if (*(unsigned short *)pData == htons(0x0800))
						iph = (struct iphdr *)(pData + 2);
					else
						iph = (struct iphdr *)(pData + 10);		
					tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);				
					if (checkLocalIPPort(iph->saddr, tcph->source))
					{
							return re8670_start_xmit_txInfo(skb,dev,NULL,NULL);
					}					
				}
			}
#endif					
                return rtk_rg_fwdEngine_xmit (skb,dev);
        }
}
#elif defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
int re8670_start_xmit_fc(struct sk_buff *skb, struct net_device *dev)
{
#ifdef CONFIG_RTK_HOST_SPEEDUP
	struct tcphdr *tcph=NULL;
	unsigned char *pData = skb->data;
	struct iphdr *iph=NULL;	
	struct ipv6hdr *ip6h=NULL;	
	void*srcaddr,*dstaddr;
	void*l3hdr;
	void *pHostTrack;
	int ret;
	int ipver=0;

	if (isHostSpeedUpEnable())
	{
	    pData += 12;
	    if (*(unsigned short *)pData == htons(0x8100))
	            pData += 4;
		if(*(unsigned short *)pData == htons(0x8864))
		{
			if(*(unsigned short *)(pData+8) == htons(0x0021))
				iph = (struct iphdr *)(pData + 10);
			else if(*(unsigned short *)(pData+8) == htons(0x0057))
			{
				ip6h = (struct iphdr *)(pData + 10);
				pData+=10;
			}
		}
		else if(*(unsigned short *)pData == htons(0x0800))
		{
			iph = (struct iphdr *)(pData + 2);
		}
		else if(*(unsigned short *)pData == htons(0x86dd))
		{
			ip6h = (struct iphdr *)(pData + 2);
			pData+=2;
		}
		if(iph)
		{
			ipver = 0;
			srcaddr = &(iph->saddr);
			dstaddr = &(iph->daddr);
			l3hdr = iph;
			if(IPPROTO_TCP == iph->protocol)
				tcph = (struct tcphdr*)((__u32 *)iph + iph->ihl);
		}
		else if(ip6h)
		{
			u8 nexthead = ip6h->nexthdr;
			ipver = 1;
			srcaddr = &(ip6h->saddr);
			dstaddr = &(ip6h->daddr);
			l3hdr = ip6h;
			pData += 40; 
			while ( (nexthead == IPPROTO_HOPOPTS) || (nexthead == IPPROTO_ROUTING) || (nexthead == IPPROTO_FRAGMENT)
				 || (nexthead == IPPROTO_DSTOPTS) || (nexthead == IPPROTO_MH))
			{
				nexthead = *pData;			
				pData += (*(pData+1)+1)<<3;
			}
			if(nexthead == IPPROTO_TCP)
				tcph = pData;
		}
		if(tcph)
		{
			read_lock_bh(&speedup_lock);
	        if ((pHostTrack = shouldSpeedUp(1, dstaddr, srcaddr, tcph->dest, tcph->source, ipver)) != NULL)
	        {
				ret = speedtest_stream_shortcut_tx_learn(pHostTrack, l3hdr, tcph, skb);
				read_unlock_bh(&speedup_lock);
				re8670_start_xmit_txInfo(skb, dev, &speedup_txDesc, &speedup_txDescMask);
				if (ret)
				{
					start_xmit_upload_data();
				}
				return 0;
	        }
			read_unlock_bh(&speedup_lock);
		}
	}
#endif
	 return rtk_fc_skb_tx(skb, dev);
}
#endif


struct net_device_ops rtl_netdevops = {
    .ndo_open       = re8670_open,
    .ndo_stop       = re8670_close,
#if defined(CONFIG_APOLLO_ROMEDRIVER)
    .ndo_start_xmit = re8670_start_xmit_check,
#elif defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	.ndo_start_xmit = re8670_start_xmit_fc,
#else
	.ndo_start_xmit = re8670_start_xmit,
#endif
    .ndo_do_ioctl   = re8670_ioctl,
    .ndo_tx_timeout = re8670_tx_timeout,
	.ndo_set_rx_mode = re8670_set_rx_mode,
	.ndo_set_mac_address = re8670_set_mac_addr,
	.ndo_set_features = re8670_set_features,
	.ndo_get_stats64 = re8670_get_stats,
#ifdef CONFIG_REALTEK_HW_LSO
	.ndo_change_mtu = re8670_set_mtu,
#endif
};

//int __init re8670_probe (void)
//{
//	printk("%s: do nothing here !!\n", __func__);
//}

//void __exit re8670_exit (void)
//{
//	struct re_private_root *root_cp = &re_private_data_root;
//	struct re_private *cp;
//	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
//    extern int drv_nic_rxhook_exit(void);
//	unsigned int gmac;
//	unsigned i;
//
//	//desc and hw setting & proc		
//	if(dev_num){			
//		for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {
//
//			cp = root_cp->re_private_data_ptr[gmac];
//			
//			if(cp->gmac_enabled != GMAC_TRUE)
//				continue;
//		
//			re8670_close(eth_net_dev[gmac]);			
//		}			
//	}
//	
//	for(gmac=0;gmac<MAX_GMAC_NUM;gmac++) {
//
//		cp = root_cp->re_private_data_ptr[gmac];
//		
//		if(cp->gmac_enabled != GMAC_TRUE)
//			continue;
//		
//		re8670_free_rings(cp);
//		proc_remove(rtl8686_proc_dir[gmac]);
//	}
//	
//	//dev
//	for(i=0; i < totalDev; i++){		
//		if(rtl8686_dev_table[i].dev_instant){
//			unregister_netdev(rtl8686_dev_table[i].dev_instant);
//			free_netdev(rtl8686_dev_table[i].dev_instant);
//		}
//	}
//#ifdef CONFIG_RTL8686_SWITCH
//	if(timer_pending(&re_private_data_root.rx_pause_by_software_interrupt_timer))
//		del_timer(&re_private_data_root.rx_pause_by_software_interrupt_timer);
//	
//	//for gpon driver
//    drv_nic_rxhook_exit();
//#endif
//}


#if 1 // CONFIG_OF
static const struct of_device_id of_platform_rtk_gmac_table[] = {
	{ .compatible = "rtk,gmac" },
};
MODULE_DEVICE_TABLE(of, of_platform_rtk_gmac_table);

static int rtk_gmac_re_private_data_init(void)
{
	struct re_private_root *root_cp = &re_private_data_root;
	unsigned int gmac, rx_ring_idx;

	///--init re_private_data_root
	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		root_cp->re_private_data_ptr[gmac] = &re_private_data[gmac];
#ifdef CONFIG_RTL8686_SWITCH
		root_cp->tx_force_gmac_index = 0;
		root_cp->rx_pause_by_software_bitmap = 0;
		root_cp->rx_pause_by_software_enable = GMAC_FALSE;
#endif
	}

	///--init re_private_data
	root_cp->re_private_data_ptr[0]->gmac_enabled = GMAC_TRUE;
#ifdef CONFIG_GMAC1_USABLE	
	root_cp->re_private_data_ptr[1]->gmac_enabled = GMAC_TRUE; 
#else
	root_cp->re_private_data_ptr[1]->gmac_enabled = GMAC_FALSE; 
#endif 
#ifdef CONFIG_GMAC2_USABLE
	root_cp->re_private_data_ptr[2]->gmac_enabled = GMAC_TRUE; 
#else
	root_cp->re_private_data_ptr[2]->gmac_enabled = GMAC_FALSE;
#endif
#ifdef TX_RING_DEBUG
	root_cp->re_private_data_ptr[0]->tx_ring_backup_debug = GMAC_TRUE;
	root_cp->re_private_data_ptr[1]->tx_ring_backup_debug = GMAC_FALSE;
	root_cp->re_private_data_ptr[2]->tx_ring_backup_debug = GMAC_FALSE;
#endif

#ifdef RX_NAPI_MODE
	root_cp->re_private_data_ptr[0]->napi_budget = GMAC0_RX_NAPI_BUDGET;
	root_cp->re_private_data_ptr[1]->napi_budget = GMAC1_RX_NAPI_BUDGET;
	root_cp->re_private_data_ptr[2]->napi_budget = GMAC2_RX_NAPI_BUDGET;
#endif

	root_cp->re_private_data_ptr[0]->gmac_cpu_port = CPU_PORT0;
#ifndef CONFIG_RTL9603CVD_SERIES
	root_cp->re_private_data_ptr[1]->gmac_cpu_port = CPU_PORT1;
	root_cp->re_private_data_ptr[2]->gmac_cpu_port = CPU_PORT2;
#endif
	root_cp->re_private_data_ptr[0]->re8670_rx_ring_size[0] = GMAC0_RX1_SIZE;
#ifndef CONFIG_RTK_SINGLE_RX_RING
	root_cp->re_private_data_ptr[0]->re8670_rx_ring_size[1] = GMAC0_RX2_SIZE;
	root_cp->re_private_data_ptr[0]->re8670_rx_ring_size[2] = GMAC0_RX3_SIZE;
	root_cp->re_private_data_ptr[0]->re8670_rx_ring_size[3] = GMAC0_RX4_SIZE;
	root_cp->re_private_data_ptr[0]->re8670_rx_ring_size[4] = GMAC0_RX5_SIZE;
	root_cp->re_private_data_ptr[0]->re8670_rx_ring_size[5] = GMAC0_RX6_SIZE;
#endif
	root_cp->re_private_data_ptr[0]->re8670_tx_ring_size[0] = GMAC0_TX1_SIZE;
	root_cp->re_private_data_ptr[0]->re8670_tx_ring_size[1] = GMAC0_TX2_SIZE;
	root_cp->re_private_data_ptr[0]->re8670_tx_ring_size[2] = GMAC0_TX3_SIZE;
	root_cp->re_private_data_ptr[0]->re8670_tx_ring_size[3] = GMAC0_TX4_SIZE;
	root_cp->re_private_data_ptr[0]->re8670_tx_ring_size[4] = GMAC0_TX5_SIZE;

	root_cp->re_private_data_ptr[1]->re8670_rx_ring_size[0] = GMAC1_RX1_SIZE;
#ifndef CONFIG_RTK_SINGLE_RX_RING
	root_cp->re_private_data_ptr[1]->re8670_rx_ring_size[1] = GMAC1_RX2_SIZE;
	root_cp->re_private_data_ptr[1]->re8670_rx_ring_size[2] = GMAC1_RX3_SIZE;
	root_cp->re_private_data_ptr[1]->re8670_rx_ring_size[3] = GMAC1_RX4_SIZE;
	root_cp->re_private_data_ptr[1]->re8670_rx_ring_size[4] = GMAC1_RX5_SIZE;
	root_cp->re_private_data_ptr[1]->re8670_rx_ring_size[5] = GMAC1_RX6_SIZE;
#endif
	root_cp->re_private_data_ptr[1]->re8670_tx_ring_size[0] = GMAC1_TX1_SIZE;
	root_cp->re_private_data_ptr[1]->re8670_tx_ring_size[1] = GMAC1_TX2_SIZE;
	root_cp->re_private_data_ptr[1]->re8670_tx_ring_size[2] = GMAC1_TX3_SIZE;
	root_cp->re_private_data_ptr[1]->re8670_tx_ring_size[3] = GMAC1_TX4_SIZE;
	root_cp->re_private_data_ptr[1]->re8670_tx_ring_size[4] = GMAC1_TX5_SIZE;

	root_cp->re_private_data_ptr[2]->re8670_rx_ring_size[0] = GMAC2_RX1_SIZE;
#ifndef CONFIG_RTK_SINGLE_RX_RING
	root_cp->re_private_data_ptr[2]->re8670_rx_ring_size[1] = GMAC2_RX2_SIZE;
	root_cp->re_private_data_ptr[2]->re8670_rx_ring_size[2] = GMAC2_RX3_SIZE;
	root_cp->re_private_data_ptr[2]->re8670_rx_ring_size[3] = GMAC2_RX4_SIZE;
	root_cp->re_private_data_ptr[2]->re8670_rx_ring_size[4] = GMAC2_RX5_SIZE;
	root_cp->re_private_data_ptr[2]->re8670_rx_ring_size[5] = GMAC2_RX6_SIZE;
#endif
	root_cp->re_private_data_ptr[2]->re8670_tx_ring_size[0] = GMAC2_TX1_SIZE;
	root_cp->re_private_data_ptr[2]->re8670_tx_ring_size[1] = GMAC2_TX2_SIZE;
	root_cp->re_private_data_ptr[2]->re8670_tx_ring_size[2] = GMAC2_TX3_SIZE;
	root_cp->re_private_data_ptr[2]->re8670_tx_ring_size[3] = GMAC2_TX4_SIZE;
	root_cp->re_private_data_ptr[2]->re8670_tx_ring_size[4] = GMAC2_TX5_SIZE;

	root_cp->re_private_data_ptr[0]->rx_multiring_bitmap = GMAC0_RX_MULTIRING_BITMAP;
	root_cp->re_private_data_ptr[1]->rx_multiring_bitmap = GMAC1_RX_MULTIRING_BITMAP;
	root_cp->re_private_data_ptr[2]->rx_multiring_bitmap = GMAC2_RX_MULTIRING_BITMAP;

	root_cp->re_private_data_ptr[0]->tx_multiring_bitmap = GMAC0_TX_MULTIRING_BITMAP;
	root_cp->re_private_data_ptr[1]->tx_multiring_bitmap = GMAC1_TX_MULTIRING_BITMAP;
	root_cp->re_private_data_ptr[2]->tx_multiring_bitmap = GMAC2_TX_MULTIRING_BITMAP;

	root_cp->re_private_data_ptr[0]->rx_only_ring1 = GMAC0_RX_ONLY_RING1;
	root_cp->re_private_data_ptr[1]->rx_only_ring1 = GMAC1_RX_ONLY_RING1;
	root_cp->re_private_data_ptr[2]->rx_only_ring1 = GMAC2_RX_ONLY_RING1;

	root_cp->re_private_data_ptr[0]->rx_not_only_ring1 = GMAC0_RX_NOT_ONLY_RING1;
	root_cp->re_private_data_ptr[1]->rx_not_only_ring1 = GMAC1_RX_NOT_ONLY_RING1;
	root_cp->re_private_data_ptr[2]->rx_not_only_ring1 = GMAC2_RX_NOT_ONLY_RING1;
	
	root_cp->re_private_data_ptr[0]->iocmd_reg = CMD_CONFIG;
	root_cp->re_private_data_ptr[1]->iocmd_reg = CMD_CONFIG;
	root_cp->re_private_data_ptr[2]->iocmd_reg = CMD_CONFIG;

	root_cp->re_private_data_ptr[0]->iocmd1_reg = (CMD1_CONFIG | (GMAC0_RX_NOT_ONLY_RING1<<25) | GMAC0_RX_MULTIRING_BITMAP << 16 | txq1_h);
	root_cp->re_private_data_ptr[1]->iocmd1_reg = (CMD1_CONFIG | (GMAC1_RX_NOT_ONLY_RING1<<25) | GMAC1_RX_MULTIRING_BITMAP << 16 | txq1_h);
	root_cp->re_private_data_ptr[2]->iocmd1_reg = (CMD1_CONFIG | (GMAC2_RX_NOT_ONLY_RING1<<25) | GMAC2_RX_MULTIRING_BITMAP << 16 | txq1_h);

	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		for(rx_ring_idx=0 ; rx_ring_idx<MAX_RXRING_NUM ; rx_ring_idx++)
		{
			root_cp->re_private_data_ptr[gmac]->re8670_rx_flow_control_status[rx_ring_idx] = GMAC_ON;
		}
		root_cp->re_private_data_ptr[gmac]->rx_ring_show_bitmap = 0x1;
		root_cp->re_private_data_ptr[gmac]->tx_ring_show_bitmap = 0x1;
#ifdef HW_LSO_ENABLE
		root_cp->re_private_data_ptr[gmac]->tx_jumbo_frame_enabled = GMAC_OFF;
#else
		root_cp->re_private_data_ptr[gmac]->tx_jumbo_frame_enabled = GMAC_ON;
#endif
		root_cp->re_private_data_ptr[gmac]->debug_enable = 0;
		root_cp->re_private_data_ptr[gmac]->debug_times = 0;
		root_cp->re_private_data_ptr[gmac]->padding_enable = 0;
		root_cp->re_private_data_ptr[gmac]->pauseBySwRingBitmap = 0;
		root_cp->re_private_data_ptr[gmac]->tx_int_mitigation = 0xd;
		root_cp->re_private_data_ptr[gmac]->tx_int_mitigation_timer = 1;
	}

	return 0;
}

static int rtk_gmac_register_root_netdev(unsigned int base, int irq)
{
	struct net_device *dev_temp;
	int i, j, rc;
	
	//printk("\n%s %d: allocate new netdev name=%s\n", __func__, __LINE__, rtl8686_dev_table[0].ifname);
	dev_temp = alloc_etherdev(sizeof(struct re_dev_private));
	if (!dev_temp) {
		printk("%s %d: alloc_etherdev fail!\n", __func__, __LINE__);
		return NULL;
	}

	sprintf(dev_temp->name, rtl8686_dev_table[0].ifname);
	dev_temp->netdev_ops = &rtl_netdevops;
	dev_temp->watchdog_timeo = TX_TIMEOUT;
	
#ifdef CONFIG_REALTEK_HW_LSO
#ifdef HW_CHECKSUM_OFFLOAD
	dev_temp->features |= NETIF_F_HW_CSUM;
	dev_temp->hw_features |= NETIF_F_HW_CSUM;
#endif
#ifdef LINUX_SG_ENABLE
	dev_temp->features |= NETIF_F_SG;
	dev_temp->hw_features |= NETIF_F_SG;
#endif
#ifdef LINUX_LSO_ENABLE
#ifdef HW_LSO_ENABLE
	dev_temp->features |= NETIF_F_GSO | NETIF_F_TSO; //: test
	dev_temp->hw_features |= NETIF_F_GSO | NETIF_F_TSO;
#else
//	dev->features |= NETIF_F_SG |  NETIF_F_GSO; //: this configuration will crash in skb_segment
	dev_temp->features |= NETIF_F_GSO; //:test
	dev_temp->hw_features |= NETIF_F_GSO;
#endif
	netif_set_gso_max_size(dev_temp, 65535); 	
#endif
#endif
#ifdef RX_NAPI_MODE
	dev_temp->features |= NETIF_F_GRO;
	dev_temp->hw_features |= NETIF_F_GRO;
#endif

	dev_temp->irq = irq;	// internal phy
	//priv data setting
	dev_temp->priv_flags = IFF_DOMAIN_ELAN;
	
	dev_temp->base_addr = (unsigned long) base;

	/* read MAC address from EEPROM */
	for (i = 0; i < 3; i++)
		((u16 *) (dev_temp->dev_addr))[i] = i;

	DEV2CP(dev_temp) = &re_private_data_root;
	rtl8686_dev_table[0].dev_instant = dev_temp;
	rtl_set_ethtool_ops(dev_temp);
	rc = register_netdev(dev_temp);
	if (rc) {
		printk("%s %d rc = %d\n", __func__, __LINE__, rc);
		return NULL;
	}

	/*
	printk (KERN_INFO "%s: %s at 0x%lx, "
			"%02x:%02x:%02x:%02x:%02x:%02x, "
			"IRQ %d\n",
			dev_temp->name,
			"RTL-8686",
			dev_temp->base_addr,
			dev_temp->dev_addr[0], dev_temp->dev_addr[1],
			dev_temp->dev_addr[2], dev_temp->dev_addr[3],
			dev_temp->dev_addr[4], dev_temp->dev_addr[5],
			dev_temp->irq);
			*/
	
	return dev_temp;
}

static int rtk_gmac_register_other_netdev(void)
{
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	struct net_device *dev = NULL;
	int i, j, rc;
	
	for(i=1;i<totalDev;i++) {
		dev = alloc_etherdev(sizeof(struct re_dev_private));
		if (!dev) {
			printk("%s %d alloc_etherdev fail!\n", __func__, __LINE__);
			goto err_out_iomap;
		}
		/* read MAC address from EEPROM */
		for (j = 0; j < 3; j++)
			((u16 *) (dev->dev_addr))[j] = j;

		DEV2CP(dev) = &re_private_data_root;
		dev->netdev_ops = &rtl_netdevops;
		dev->watchdog_timeo = TX_TIMEOUT;
		// priv data setting
		switch(rtl8686_dev_table[i].ifflag)
		{
			case RTL8686_ELAN:
				dev->priv_flags = IFF_DOMAIN_ELAN;
				break;
			case RTL8686_WAN:
				dev->priv_flags = IFF_DOMAIN_WAN;
				break;
			case RTL8686_WLAN:
				dev->priv_flags = IFF_DOMAIN_WLAN;
				break;
			case RTL8686_SVAP:		//for slave VAP interfaces, bridge should not send query for each, just need one for slave's br0
				dev->priv_flags = IFF_DOMAIN_WLAN | IFF_DOMAIN_WAN;
				break;
			default:
				printk("Error! Should not go here!\n");
		}
		
#ifdef CONFIG_REALTEK_HW_LSO
#ifdef HW_CHECKSUM_OFFLOAD
		dev->features |= NETIF_F_HW_CSUM;
		dev->hw_features |= NETIF_F_HW_CSUM;
#endif
#ifdef LINUX_SG_ENABLE
		dev->features |= NETIF_F_SG;
		dev->hw_features |= NETIF_F_SG;
#endif
#ifdef LINUX_LSO_ENABLE
#ifdef HW_LSO_ENABLE
		dev->features |= NETIF_F_GSO | NETIF_F_TSO; //: test
		dev->hw_features |= NETIF_F_GSO | NETIF_F_TSO;
#else
	//	dev->features |= NETIF_F_SG |  NETIF_F_GSO; //: this configuration will crash in skb_segment
		dev->features |= NETIF_F_GSO; //:test
		dev->hw_features |= NETIF_F_GSO;
#endif
		netif_set_gso_max_size(dev, 65535); 	
#endif
#endif
#ifdef RX_NAPI_MODE
		dev->features |= NETIF_F_GRO;
		dev->hw_features |= NETIF_F_GRO;
#endif

		dev->base_addr = PRIV2DEV(&re_private_data_root)->base_addr;
		dev->irq = PRIV2DEV(&re_private_data_root)->irq; // internal phy		
		rtl8686_dev_table[i].dev_instant = dev;

		if (strncmp(rtl8686_dev_table[i].ifname, "lan", 3) == 0)
			strncpy(dev->name, rtl8686_dev_table[i].ifname, IFNAMSIZ);
		if (strncmp(rtl8686_dev_table[i].ifname, "eth", 3) == 0)
			strncpy(dev->name, rtl8686_dev_table[i].ifname, IFNAMSIZ);
		else
			memcpy(dev->name, rtl8686_dev_table[i].ifname, strlen(rtl8686_dev_table[i].ifname));

		rtl_set_ethtool_ops(dev);
		rc = register_netdev(dev);
		if (rc) {
			printk("%s %d rc = %d\n", __func__, __LINE__, rc);
			goto err_out_iomap;
		}
#ifdef CONFIG_RTL8686_SWITCH 
		if(i < (1+MAX_LAN_PORT+MAX_PON_PORT) && i >= 1 )
		{
			// copy interface name
			memcpy(&LCDev_mapping[rtl8686_dev_table[i].phyPort].ifname,dev->name , strlen(dev->name));
			// assign net_dev
			LCDev_mapping[rtl8686_dev_table[i].phyPort].phy_dev = dev;
			/* sfu requires port link change notifier */
			if ( rtl8686_dev_table[i].isPanelPort || !strncmp(dev->name, "eth1", 4))
				netif_carrier_off(LCDev_mapping[rtl8686_dev_table[i].phyPort].phy_dev);
		}
#endif
		/*
		printk (KERN_INFO "%s: %s at 0x%lx, "
				"%02x:%02x:%02x:%02x:%02x:%02x, "
				"IRQ %d\n",
				dev->name,
				"RTL-8686",
				dev->base_addr,
				dev->dev_addr[0], dev->dev_addr[1],
				dev->dev_addr[2], dev->dev_addr[3],
				dev->dev_addr[4], dev->dev_addr[5],
				dev->irq);
				*/
#if defined(CONFIG_GPON_FEATURE) || defined(CONFIG_EPON_FEATURE)
		if(!strncmp(dev->name, "nas", 3) && dev->priv_flags == IFF_DOMAIN_WAN)
		{
			netif_carrier_off(dev);
			printk(">>>> Set %s carrier off !!! \n",dev->name);
		}
#endif
	}

	return 0;

err_out_iomap:
	printk("%s %d: here is a error when probe! \n", __func__, __LINE__);
	return -1 ;
}

static int rtk_gmac_multi_lan_device_init(void)
{
#ifdef CONFIG_RTL8686_SWITCH 
	// port 5 is rgmii port
#ifdef CONFIG_RGMII_RESET_PROCESS
	strcpy(LCDev_mapping[RGMII_PORT].ifname, "eth0");
    LCDev_mapping[RGMII_PORT].phy_dev = eth_net_dev[0];
#else
	strcpy(LCDev_mapping[RGMII_PORT].ifname, "nas0");
	LCDev_mapping[RGMII_PORT].phy_dev = LCDev_mapping[WAN_PORT].phy_dev;
#endif
	/* sfu requires port link change notifier */
	if(intr_bcaster_notifier_cb_register(&GMAClinkChangeNotifier) != 0)
	{
		printk("Interrupt Broadcaster for Link Change Error !! \n");
	}
#if defined(CONFIG_ETHWAN_USE_USB_SGMII) || defined(CONFIG_ETHWAN_USE_PCIE1_SGMII)
	if(intr_bcaster_notifier_cb_register(&GMACethwanStateChangeNotifier) != 0)
	{
		printk("Interrupt Broadcaster for ether WAN Link Change Error !! \n");
	}
#else
	if(intr_bcaster_notifier_cb_register(&GMAConuStateChangeNotifier) != 0)
	{
		printk("Interrupt Broadcaster for onu state Change Error !! \n");
	}
#endif
#endif

#ifdef CONFIG_RTL_MULTI_LAN_DEV
	// Setup default port mapping
	change_dev_port_mapping(LAN_PORT1,"lan0");
	change_dev_port_mapping(LAN_PORT2,"lan1");
	change_dev_port_mapping(LAN_PORT3,"lan2");
	change_dev_port_mapping(LAN_PORT4,"lan3");
	change_dev_port_mapping(LAN_PORT5,"lan4");
	change_dev_port_mapping(LAN_PORT6,"lan5");
	change_dev_port_mapping(WAN_PORT,"eth1");
	#if defined(CONFIG_RTL_MULTI_PHY_ETH_WAN)
	change_dev_port_mapping(LAN_PORT6,"ifprobe");
	#endif
#endif

#if defined(CONFIG_EXTERNAL_PHY_POLLING) && defined(CONFIG_MDC_MDIO_EXT_PORT)
	#if (CONFIG_MDC_MDIO_EXT_PORT==APOLLOPRO_SGMII0_PORT) || (CONFIG_MDC_MDIO_EXT_PORT==APOLLOPRO_SGMII1_PORT)
	rtk_port_serdesMode_set((CONFIG_MDC_MDIO_EXT_PORT-APOLLOPRO_SGMII0_PORT), LAN_SDS_MODE_SGMII_MAC);
	rtk_port_serdesNWay_set((CONFIG_MDC_MDIO_EXT_PORT-APOLLOPRO_SGMII0_PORT), LAN_SDS_NWAY_AUTO);
	#endif
#endif

	return 0;
}

static int rtk_gmac_netdev_init(unsigned int gmac, unsigned int base, int irq)
{
	unsigned int current_gmac=0;
	unsigned int ring_index;
	struct net_device *dev = NULL;
	int netdev_registered;
	int i, rc=0;
	void*	pBuf;

	if(gmac >= MAX_GMAC_NUM)
	{
		printk("%s %d: invalid gmac=%d\n", __func__, __LINE__, gmac);
		return -1;
	}
	
	if(re_private_data[gmac].gmac_enabled != GMAC_TRUE)
		return -1;

	/*
	printk("GMAC%d Tx Ring Size:\n", gmac);
	for(ring_index=0;ring_index<MAX_TXRING_NUM;ring_index++) {
		printk("[#%d]:%d ", ring_index, re_private_data[gmac].re8670_tx_ring_size[ring_index]);
	}
	printk("\nGMAC%d Rx Ring Size:\n", gmac);
	for(ring_index=0;ring_index<MAX_RXRING_NUM;ring_index++) {
		printk("[#%d]:%d ", ring_index, re_private_data[gmac].re8670_rx_ring_size[ring_index]);
	}
	*/

	//allocate and register root dev first
	dev = dev_get_by_name(&init_net, rtl8686_dev_table[0].ifname);
	if(dev==NULL)
	{
		dev = rtk_gmac_register_root_netdev(base, irq); 
		if(dev == NULL) {
			printk("%s %d rtk_gmac_register_root_netdev return FAIL\n", __func__, __LINE__);
			return -1;
		}

		netdev_registered = 0;
	}
	else
	{
		printk("\n%s %d: netdev name=%s exist\n", __func__, __LINE__, rtl8686_dev_table[0].ifname);
		netdev_registered = 1;
	}

	//set remain re_private_data
	re_private_data[gmac].gmac = gmac;
	re_private_data_root.dev = dev;
	spin_lock_init (&(re_private_data[gmac].tx_lock));
	spin_lock_init (&(re_private_data[gmac].rx_lock));
	re_private_data[gmac].base = base;
	re_private_data[gmac].irq = irq;
	memset(&re_private_data[gmac].rx_tasklets, 0, sizeof(struct tasklet_struct));
	re_private_data[gmac].rx_tasklets.func=(void (*)(unsigned long))re8670_rx;	
	re_private_data[gmac].rx_tasklets.data=(unsigned long)&re_private_data[gmac];

#ifdef TX_RECYCLE_SKB_USING_TOK_INT
	memset(&re_private_data[gmac].tx_tasklets, 0, sizeof(struct tasklet_struct));
	re_private_data[gmac].tx_tasklets.func=(void (*)(unsigned long))re8670_tx_all;
	re_private_data[gmac].tx_tasklets.data=(unsigned long)&re_private_data[gmac];	
#endif
	eth_rx_tasklets[gmac] = &(re_private_data[gmac].rx_tasklets);
	eth_net_dev[gmac] = dev;
	rtl8686_proc_debug_init(gmac);
	port_relate_setting(gmac);

#ifdef RX_NAPI_MODE
	re_private_data[gmac].napi.dev = dev;
	netif_napi_add(dev, &re_private_data[gmac].napi, re8670_poll, re_private_data[gmac].napi_budget);
#endif

	if(!netdev_registered)
	{
		// allocate and register other device
		if(rtk_gmac_register_other_netdev()) {
			printk("%s %d rtk_gmac_register_other_netdev return FAIL\n", __func__, __LINE__);
			return -1;
		}
		rtk_gmac_multi_lan_device_init();
		
		//sw stuff
#if defined(CONFIG_RTL865X_ETH_PRIV_SKB) || defined(CONFIG_RTL865X_ETH_PRIV_SKB_ADV)
		init_priv_eth_skb_buf();
		init_priv_eth_skb_buf_rg();
#endif

#if defined(CONFIG_RTW_MEMPOOL)
		rtw_create_mem_pool_ex("8670_buffer",4*(RE8670_RX_RING_SIZE),(RE8670_RX_RING_SIZE),SKB_BUF_SIZE);
#endif

#ifdef CONFIG_RTL8686_SWITCH
		if(timer_pending(&re_private_data_root.rx_pause_by_software_interrupt_timer))
			del_timer(&re_private_data_root.rx_pause_by_software_interrupt_timer);	
		init_timer(&re_private_data_root.rx_pause_by_software_interrupt_timer);
		re_private_data_root.rx_pause_by_software_interrupt_timer.function = setSoftwareInterrupt;
		re_private_data_root.rx_pause_by_software_interrupt_timer.data = (unsigned long)NULL;
#endif
	}
	
#ifdef TX_RING_DEBUG
	if(re_private_data[gmac].tx_ring_backup_debug)
	{
		memset(re_private_data[gmac].rtl8686_tx_ring_debug, 0, sizeof(struct rtl8686_tx_ring_debug_entry)*MAX_GMAC_NUM*MAX_TXRING_NUM);
		for(i=0;i<MAX_TXRING_NUM;i++)
		{
			if(!re_private_data[gmac].re8670_tx_ring_size[i])
				continue;

			pBuf = kzalloc(RE8670_TXRING_BYTES(re_private_data[gmac].re8670_tx_ring_size[i]), GFP_KERNEL);
			if (!pBuf)
				printk("%s %d FAIL !\n\n", __func__, __LINE__);

			pBuf = (void*)( (u32)(pBuf + DESC_ALIGN-1) &  ~(DESC_ALIGN -1) ) ;
			re_private_data[gmac].rtl8686_tx_ring_debug[i].txDescriptor = (DMA_TX_DESC*)((u32)(pBuf) | UNCACHE_MASK);

			pBuf = kzalloc(re_private_data[gmac].re8670_tx_ring_size[i]*sizeof(struct tx_data_buffter), GFP_KERNEL);
			if (!pBuf)
				printk("%s %d FAIL !\n\n", __func__, __LINE__);

			re_private_data[gmac].rtl8686_tx_ring_debug[i].dataBuffer = pBuf;
		}
	}
#endif

#ifdef CONFIG_RTK_WFOAX
	if(gmac != WFO_GMAC_NO)
#endif
	rc = re8670_alloc_rings(&re_private_data[gmac]);
	if (rc)
		return -1;

	return 0;
}

static int rtk_gmac_hw_init(unsigned int gmac)
{
	re8670_ip_enable(gmac);
	
	if(re_private_data[gmac].gmac_enabled != GMAC_TRUE)
		return -1;

	//stop hw
	re8670_stop_hw(&re_private_data[gmac]);		
	re8670_reset_hw(&re_private_data[gmac]);

	config_tx_jumbo(gmac, re_private_data[gmac].tx_jumbo_frame_enabled);

	//20170502: disable gmac padding by default
	gmac_padding_enable(gmac, DISABLED);

	return 0;
}

static int of_platform_rtk_gmac_probe(struct platform_device *ofdev)
{
	const struct of_device_id *match;
	struct device_node *np;
	int irq, i;
	u32 *regs;

#ifdef CONFIG_RTL8686_SWITCH
	extern int drv_nic_rxhook_init(void);
#endif
	
	match = of_match_device(of_platform_rtk_gmac_table, &ofdev->dev);
	if (!match)
		return -EINVAL;
	
	np = ofdev->dev.of_node;
		
	rtk_gmac_re_private_data_init();
	for (i=0; i<MAX_GMAC_NUM; i++) {
		char name[8];
		struct device_node *child;
		snprintf(name, sizeof(name)-1, "gmac%d", i);
		child = of_find_node_by_name(np, name);		
		if (child) {
			regs  = of_get_property(child, "reg", NULL);
			if (regs) {
				irq = irq_of_parse_and_map(child, 0);
				//printk("%s(%d): reg %x %x irq=%d\n", __func__,__LINE__,regs[0],regs[1],irq);				
				rtk_gmac_netdev_init(i, ioremap(regs[0], regs[1]), irq);
				rtk_gmac_hw_init(i);			
			}
		} else {
			struct re_private_root *root_cp = &re_private_data_root;
			root_cp->re_private_data_ptr[i]->gmac_enabled = GMAC_FALSE;
		}
	}
#ifdef CONFIG_RTL8686_SWITCH
	drv_nic_rxhook_init();	
#endif
#ifdef CONFIG_REALTEK_HW_LSO
	re8670_init_mtu();
#endif

#if defined(CONFIG_RX_WIFI_FF_AGG)
	aggQ_init();
#endif /* CONFIG_RX_WIFI_FF_AGG */

	return 0;
}

static int of_platform_rtk_gmac_remove(struct platform_device *ofdev) {
	return 0;
}

static struct platform_driver of_platform_rtk_gmac_driver = {
	.driver = {
		.name = "rtk_gmac",
		.of_match_table = of_platform_rtk_gmac_table,
	},
	.probe = of_platform_rtk_gmac_probe,
	.remove = of_platform_rtk_gmac_remove,
};
module_platform_driver(of_platform_rtk_gmac_driver);
#endif // CONFIG_OF

void re8670_hardware_reset(unsigned int gmac)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp = root_cp->re_private_data_ptr[gmac];
	unsigned int waitingTimes = 0;
	
	printk("%s %d enter\n", __func__, __LINE__);
	
	//MII Tx Disable
	RLE0787_W32(gmac, IO_CMD, RLE0787_R32(gmac, IO_CMD)&~(1<<4));
	//MII Rx Disable
	RLE0787_W32(gmac, IO_CMD, RLE0787_R32(gmac, IO_CMD)&~(1<<5));
	
	re8670_stop_hw(cp);

	switch(gmac) {
		case 0:
			REG32(BSP_IP_SEL) &= ~BSP_EN_GMAC;
			mdelay(10);
			REG32(BSP_IP_SEL) |= BSP_EN_GMAC;
			break;
		case 1:
			REG32(NEW_BSP_IP_SEL) &= ~BSP_EN_GMAC1;
			mdelay(10);
			REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC1;
			break;
		case 2:
			REG32(NEW_BSP_IP_SEL) &= ~BSP_EN_GMAC2;
			mdelay(10);
			REG32(NEW_BSP_IP_SEL) |= BSP_EN_GMAC2;
			break;
	}
	
	//Setting to 1 forces the Ethernet module to a software reset state which disables the transmitter and receiver
	RLE0787_W32(gmac, COM_REG, RLE0787_R32(gmac, COM_REG) | 0x1);
	while(RLE0787_R32(gmac, COM_REG) & 0x1 && (waitingTimes < 1000))
	{
		printk("%s %d waiting ... (waitingTimes=%d)\n", __func__, __LINE__, waitingTimes);
		waitingTimes++;
	}

	//MII Tx Enable
	RLE0787_W32(gmac, IO_CMD, RLE0787_R32(gmac, IO_CMD)|(1<<4));
	//MII Rx Enable
	RLE0787_W32(gmac, IO_CMD, RLE0787_R32(gmac, IO_CMD)|(1<<5));
	
	config_tx_jumbo(gmac, cp->tx_jumbo_frame_enabled);
	gmac_padding_enable(gmac, DISABLED);

	printk("%s %d exit\n", __func__, __LINE__);
}

int re8670_reset(void)
{
	struct re_private_root *root_cp = &re_private_data_root;
	struct re_private *cp;
	unsigned int totalDev = TOTAL_RTL8686_DEV_NUM;
	unsigned int tempLxcbus0MasterReg;
	unsigned int tempLxcbus0SlaveReg;
	unsigned int tempLxcbus1MasterReg;
	unsigned int tempLxcbus1SlaveReg;
	unsigned int upMask = 0;
	unsigned int gmac;
	int i;

	printk("%s %d enter\n", __func__, __LINE__);

	//printk("%s %d LXCBUS0_MASTER_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS0_MASTER_REG)));
	//printk("%s %d LXCBUS0_SLAVE_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS0_SLAVE_REG)));
	//printk("%s %d LXCBUS1_MASTER_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS1_MASTER_REG)));
	//printk("%s %d LXCBUS1_SLAVE_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS1_SLAVE_REG)));

	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		cp = root_cp->re_private_data_ptr[gmac];
		if(cp->gmac_enabled == GMAC_FALSE)
			continue;
		
		disable_irq(cp->irq);
		cp->eth_close=GMAC_TRUE;
	}

	for(i=0 ; i<totalDev ; i++)
	{
		if(rtl8686_dev_table[i].dev_instant)
		{
			if(netif_running(rtl8686_dev_table[i].dev_instant))
			{
				//printk("%s %d dev=%s is UP\n", __func__, __LINE__, rtl8686_dev_table[i].dev_instant->name);
				upMask |= (1<<i);
				if(!netif_queue_stopped(rtl8686_dev_table[i].dev_instant))
					netif_stop_queue(rtl8686_dev_table[i].dev_instant);
			}
		}
	}

	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		cp = root_cp->re_private_data_ptr[gmac];
		if(cp->gmac_enabled == GMAC_FALSE)
			continue;

		GMAC_SPIN_LOCK(&cp->tx_lock);
		printk("%s %d gmac=%d\n", __func__, __LINE__, gmac);
		
		//Enable LX bus timeout monitor
		if(gmac==0 || gmac==2)
		{
			printk("%s %d Enable LX bus 0 timeout monitor\n", __func__, __LINE__);
			tempLxcbus0MasterReg = (*(volatile u32*)((u32) LXCBUS0_MASTER_REG));
			tempLxcbus0SlaveReg = (*(volatile u32*)((u32) LXCBUS0_SLAVE_REG));
			(*(volatile u32*)((u32)LXCBUS0_MASTER_REG)) = tempLxcbus0MasterReg | (1<<31);
			(*(volatile u32*)((u32)LXCBUS0_SLAVE_REG)) = tempLxcbus0SlaveReg | (1<<31);
			//printk("%s %d LXCBUS0_MASTER_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS0_MASTER_REG)));
			//printk("%s %d LXCBUS0_SLAVE_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS0_SLAVE_REG)));
		}
		if(gmac==1)
		{
			printk("%s %d Enable LX bus 1 timeout monitor\n", __func__, __LINE__);
			tempLxcbus1MasterReg = (*(volatile u32*)((u32) LXCBUS1_MASTER_REG));
			tempLxcbus1SlaveReg = (*(volatile u32*)((u32) LXCBUS1_SLAVE_REG));
			(*(volatile u32*)((u32)LXCBUS1_MASTER_REG)) = tempLxcbus1MasterReg | (1<<31);
			(*(volatile u32*)((u32)LXCBUS1_SLAVE_REG)) = tempLxcbus1SlaveReg | (1<<31);
			//printk("%s %d LXCBUS1_MASTER_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS1_MASTER_REG)));
			//printk("%s %d LXCBUS1_SLAVE_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS1_SLAVE_REG)));
		}
		
		cp->stag_pid = RLE0787_R32(gmac, VLAN_REG);
		cp->stag_pid1 = RLE0787_R32(gmac, VLAN1_REG);
		
		re8670_hardware_reset(gmac);

		re8670_free_rings(cp);
		re8670_alloc_rings(cp);
		re8670_init_hw(cp);
		re8670_init_trx_cdo(cp);

		if(cp->stag_pid)
			RLE0787_R32(gmac, VLAN_REG) = cp->stag_pid;
		if(cp->stag_pid1)
			RLE0787_R32(gmac, VLAN1_REG) = cp->stag_pid1;

		mdelay(500);
		//Disable LX bus timeout monitor
		if(gmac==0 || gmac==2)
		{
			printk("%s %d Disable LX bus 0 timeout monitor\n", __func__, __LINE__);
			(*(volatile u32*)((u32)LXCBUS0_MASTER_REG)) = tempLxcbus0MasterReg;
			(*(volatile u32*)((u32)LXCBUS0_SLAVE_REG)) = tempLxcbus0SlaveReg;
			//printk("%s %d LXCBUS0_MASTER_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS0_MASTER_REG)));
			//printk("%s %d LXCBUS0_SLAVE_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS0_SLAVE_REG)));
		}
		if(gmac==1)
		{
			printk("%s %d Disable LX bus 1 timeout monitor\n", __func__, __LINE__);
			(*(volatile u32*)((u32)LXCBUS1_MASTER_REG)) = tempLxcbus1MasterReg;
			(*(volatile u32*)((u32)LXCBUS1_SLAVE_REG)) = tempLxcbus1SlaveReg;
			//printk("%s %d LXCBUS1_MASTER_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS1_MASTER_REG)));
			//printk("%s %d LXCBUS1_SLAVE_REG=0x%x\n", __func__, __LINE__, (*(volatile u32*)((u32)LXCBUS1_SLAVE_REG)));
		}

		GMAC_SPIN_UNLOCK(&cp->tx_lock);
	}

	for(i=0 ; i<totalDev ; i++)
	{
		if(rtl8686_dev_table[i].dev_instant)
		{
			if(upMask & (1<<i))
			{
				//printk("%s %d dev=%s was UP\n", __func__, __LINE__, rtl8686_dev_table[i].dev_instant->name);
				if(netif_queue_stopped(rtl8686_dev_table[i].dev_instant))
					netif_wake_queue(rtl8686_dev_table[i].dev_instant);
			}
		}
	}

	for(gmac=0 ; gmac<MAX_GMAC_NUM ; gmac++)
	{
		cp = root_cp->re_private_data_ptr[gmac];
		if(cp->gmac_enabled == GMAC_FALSE)
			continue;

		cp->eth_close=GMAC_FALSE;
		enable_irq(cp->irq);
	}

	printk("%s %d exit\n", __func__, __LINE__);
	return 0;
}

#ifdef SMBSHORTCUT
static int smbsc_read_proc(struct seq_file *seq, void *v)
{
	seq_printf(seq, "SMB serverip = %d.%d.%d.%d\n", 
		(smbserverip & 0xff000000) >> 24, 
		(smbserverip & 0x00ff0000) >> 16, 
		(smbserverip & 0x0000ff00) >> 8, 
		(smbserverip & 0xff));
	if ( smbscflag )
		seq_printf(seq, "SMB shortcut enable\n");
	else
		seq_printf(seq, "SMB shortcut disable\n");
	return 0;
}

static int proc_smbsc_open(struct inode *inode, struct file *file)
{
	return single_open(file, smbsc_read_proc, inode->i_private);
}

//# echo "sip 192.168.1.2" > /proc/smbshortcut
//# echo "set 1" > /proc/smbshortcut
int smbsc_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char tmpbuf[512]={0};
	u32 sip=0;
	unsigned char flag = 0;

	if (buffer && !copy_from_user(tmpbuf, buffer, count))
	{
		char *strptr,*split_str;

		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		split_str=strsep(&strptr," ");
		/*parse command*/

		//printk("split_str [%s]\n", split_str);
		while(1)
		{
			//split_str=strsep(&strptr," ");
			//printk("split_str [%s]\n", split_str);
			if(strcasecmp(split_str, "sip") == 0)
			{
				char *ip_token,*split_ip_token,j;
				split_str=strsep(&strptr," ");
				ip_token=split_str;
				//printk("%s-%d sip=%s\n",__func__,__LINE__,ip_token);
				for(j=0;j<4;j++)
				{
					split_ip_token=strsep(&ip_token,".");
					//printk("ip_token [%s], split_ip_token [%s]\n", ip_token, split_ip_token);
					sip|=(simple_strtol(split_ip_token, NULL, 0)<<((3-j)<<3));
					if(ip_token==NULL) {
						smbSetServerIP(sip);
						break;
					}
				}
			}else if(strcasecmp(split_str, "set") == 0)
			{
				split_str=strsep(&strptr," ");
				flag=simple_strtol(split_str, NULL, 0);
				smbSetflag(flag);
				//printk("%s-%d sport=%d\n",__func__,__LINE__,sport);
			}else
				split_str=strsep(&strptr," ");
			if (strptr==NULL) break;
		}
		//printk("sip 0x%x, flag 0x%x\n", sip, flag);

	}
	
	return count;
}

static const struct file_operations smbsc_fops = {
        .owner          = THIS_MODULE,
        .open           = proc_smbsc_open,
        .read           = seq_read,
        .write          = smbsc_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

int __init SMBShortCut_proc_init(void)
{
	struct proc_dir_entry *entry=NULL;

	entry = proc_create_data("smbshortcut", 0644, NULL, &smbsc_fops,NULL);
	if (entry == NULL)
	{
		printk("create proc smbsc fail!\n");		
		remove_proc_entry("smbsc", NULL);
		return -ENOMEM;
	}

	return 0;
}

int SMBShortCut_proc_exit(void)
{

	return 0;
}
module_init(SMBShortCut_proc_init);
#endif

#if defined(CONFIG_RTL_ETH_DEVICE_NAME_REFINE)
int rtl_change_wan_dev_flags(int wan_portmask)
{
	int port_id;
	struct net_device *dev = NULL;

	for(port_id=0; port_id<SW_PORT_NUM; port_id++)
	{
		if(LCDev_mapping[port_id].phy_dev == NULL)
			continue;
		dev = LCDev_mapping[port_id].phy_dev;

		if((1<<port_id) & wan_portmask)
		{
			dev->priv_flags &= ~IFF_DOMAIN_ELAN;
			dev->priv_flags |= IFF_DOMAIN_WAN;
		}else{
			dev->priv_flags &= ~(IFF_DOMAIN_WAN);
			dev->priv_flags |= IFF_DOMAIN_ELAN;
		}
	}

	return 0;
}
#endif

//module_init(re8670_probe);
//module_exit(re8670_exit);
#ifdef CONFIG_WIRELESS_LAN_MODULE
int (*wirelessnet_hook)(void) = NULL;
EXPORT_SYMBOL(wirelessnet_hook);
#endif
EXPORT_SYMBOL(re8686_send_with_txInfo_and_mask);
EXPORT_SYMBOL(re8686_set_flow_control);
#ifdef CONFIG_RTL8686_SWITCH
EXPORT_SYMBOL(re8686_set_pauseBySw);
#endif
EXPORT_SYMBOL(re8686_set_vlan_register);
EXPORT_SYMBOL(re8686_get_vlan_register);

