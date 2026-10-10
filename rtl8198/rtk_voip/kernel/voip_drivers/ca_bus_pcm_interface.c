/*
 *	Realtek RTL8186 PCM Controller Driver
 *
 *	Author : thlin@realtek.com.tw
 *
 *	2005.09.05
 *
 *	Copyright 2005 Realtek Semiconductor Corp.
 */
//#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/version.h>
//#if  defined(CONFIG_DEFAULTS_KERNEL_3_10)
//#include <asm/system.h>
//#endif
#include <asm/irq.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif
#include <linux/timekeeping.h>
#define SUPPRESSION_PCM_INT_ORDER	2

#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM8651
#elif defined CONFIG_RTK_VOIP_DRIVERS_PCM8186
#include <asm/rtl8186.h>
#define EXTERNEL_CLK
#elif defined (CONFIG_RTK_VOIP_DRIVERS_PCM8671) || defined (CONFIG_RTK_VOIP_DRIVERS_PCM8672) || defined (CONFIG_RTK_VOIP_DRIVERS_PCM8676)
#include "../include/rtl8671.h"
#elif defined CONFIG_RTK_VOIP_DRIVERS_PCM865xC
#elif defined CONFIG_RTK_VOIP_DRIVERS_PCM8972B_FAMILY
#elif defined CONFIG_RTK_VOIP_DRIVERS_PCM89xxC
#endif

#include "rtk_voip.h"
#include "voip_addrspace.h"
#include "voip_types.h"
#include "voip_init.h"
#include "voip_proc.h"
#include "voip_dev.h"

#include "mem.h"
#include "dmem_stack.h"
#ifndef CONFIG_CPU_HAS_RADIAX
#include "radiax_save.h"
#endif

#include "cp3_profile.h"

#include "ca_bus_pcm_interface.h"
#include "snd_define.h"

#include "ca_snd_audio.h"
#include "ca_snd_audio_wrapper.h"
#include "ca77xx_snd_audio_reg.h"
//#include "ca77xx_registers.h"
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM8651
#include "voip_support.h"
#include "dsp_main.h"
voip_callback_t toRegister[BUS_PCM_CH_NUM];
#endif


#if !defined( BUS_PCM_CH_NUM ) || \
	( BUS_PCM_CH_NUM < 1 ) || ( BUS_PCM_CH_NUM > 8 )
#error "BUS_PCM_CH_NUM value is invalid!!!"
#endif

#include "con_register.h"
#include "con_bus_handler.h"
#include "con_mux.h"
#ifdef SUPPORT_SILAB_FXO_TUNE
#include "fxo_tune.h"
#endif

#define G3_TEST

#ifdef G3_TEST

#include "ca_snd_audio_wrapper.h"
#endif

#ifndef SUPPORT_CUT_DSP_PROCESS
extern Word16 nPcmIntPeriod[BUS_PCM_CH_NUM];            // pcm interrupt period (ms), set in INITIAL Cmd
#endif

//#define DISABLE_PCI	// kenny: temporary add. Shall be moved to somewhere, bootcode!?

extern int law;	// pkshih: move to bus_pcm_law.c

#ifdef CONFIG_VOIP_COP3_PROFILE
#include "voip_debug.h"
extern st_CP3_VoIP_param cp3_voip_param;
#endif

#ifdef SUPPORT_VOIP_DBG_COUNTER
extern uint32 gVoipCounterEnable;
extern void PCM_tx_count(uint32 chid);
extern void PCM_rx_count(uint32 chid);
#endif

#include "section_def.h"

#if defined (AUDIOCODES_VOIP)

#ifdef AUDIOCODES_VOTING_MECHANISM
#define AC_PCM_ISR_CTLWRD_RESET_CH(ch) (0xFF000000 >> ((ch)*8))
#define AC_PCM_ISR_WRITE(b) pcm_outl(PCMISR, (b));

#define AC_PCM_IMR_READ() pcm_inl(PCMIMR)
#define AC_PCM_IMR_WRITE(b) pcm_outl(PCMIMR,(b))
#define AC_PCM_IMR_CTLWRD_P0OK(ch) BIT((4*(MAXCH-(ch)-1)+3))
#define AC_PCM_IMR_CTLWRD_P1OK(ch) BIT((4*(MAXCH-(ch)-1)+2))
#define AC_PCM_IMR_CTLWRD_TBUA(ch) BIT((4*(MAXCH-(ch)-1)+1))
#define AC_PCM_IMR_CTLWRD_RBUA(ch) BIT(4*(MAXCH-(ch)-1))

#define AC_PCM_CHCNR_READ() pcm_inl(PCMCHCNR)
#define AC_PCM_CHCNR_WRITE(b) pcm_outl(PCMCHCNR,(b))
#define AC_PCM_CHCNR_CTLWRD_RX_TX(ch) (BIT((8*(MAXCH-(ch)-1)+1)) | BIT(8*(MAXCH-(ch)-1)))
#define AC_PCM_CHCNR_CTLWRD_TX(ch) BIT((8*(MAXCH-(ch)-1)+1))
#define AC_PCM_CHCNR_CTLWRD_RX(ch) BIT(8*(MAXCH-(ch)-1))
#endif

#endif

#ifdef SUPPRESSION_PCM_INT_ORDER
static uint32 IMR_enable = 0;	// each bit indicates channel is enabled
static uint32 IMR_fold = 0;		// which IMR fold is enabled
#if SUPPRESSION_PCM_INT_ORDER == 1		// half
#define IMR_FOLD_BASE		0x55555555UL
#define IMR_FOLD_NUM		2
#define IMR_FOLD_GET( bch )	( bch & 0x01 )
#elif SUPPRESSION_PCM_INT_ORDER == 2	// quarter
#define IMR_FOLD_BASE		0x11111111UL
#define IMR_FOLD_NUM		4
#define IMR_FOLD_GET( bch )	( bch & 0x03 )
#endif
#endif

// --------------------------------------------------------
// channel mapping architecture
// --------------------------------------------------------

#include "rtk_voip.h"
#include "voip_init.h"
#include "con_register.h"

static voip_bus_t bus_pcm[ BUS_PCM_CH_NUM ];
static uint32 bus_pcm_wideband_hybrid_bits = 0;	//8k+8k wideband

/************************************************************************/
/*********************************************/
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM8186
#define DEV_NAME	"pcmctrl"
#define PCM_MAJOR	244	//use 0 for assigning major number dynamicly.
int pcmctrl_major = PCM_MAJOR;		//device major number
#endif
static struct pcm_priv pcmctrl_devices[ BUS_PCM_CH_NUM ];

static struct {
	uint8 tx[ BUFFER_SIZE ] __attribute__((aligned(32)));
	uint8 rx[ BUFFER_SIZE ] __attribute__((aligned(32)));
} pcm_dma[ BUS_PCM_CH_NUM ];

__pcmifdata10 static int txpage[BUS_PCM_CH_NUM];
__pcmifdata09 static int rxpage[BUS_PCM_CH_NUM];
__pcmifdata16 static int tr_cnt[BUS_PCM_CH_NUM];
__pcmifdata17 static char chanEnabled[BUS_PCM_CH_NUM];

#if defined (AUDIOCODES_VOIP)
__pcmifdata05 unsigned long _chVoting = 0;
#define AC_PCM_VOTING_CH_ON(ch) _chVoting |= (1<<(ch))
#define AC_PCM_VOTING_CH_OFF(ch) _chVoting &= ~(1<<(ch))
#define AC_PCM_VOTING_GET() _chVoting
#endif
#if BUS_PCM_CH_NUM > 4
__pcmifdata06 static unsigned long ChanTxPage[16] = {CH0P0TOK, CH0P1TOK, CH1P0TOK, CH1P1TOK, CH2P0TOK, CH2P1TOK, CH3P0TOK, CH3P1TOK, CH4P0TOK, CH4P1TOK, CH5P0TOK, CH5P1TOK, CH6P0TOK, CH6P1TOK, CH7P0TOK, CH7P1TOK};
__pcmifdata07 static unsigned long ChanRxPage[16] = {CH0P0ROK, CH0P1ROK, CH1P0ROK, CH1P1ROK, CH2P0ROK, CH2P1ROK, CH3P0ROK, CH3P1ROK, CH4P0ROK, CH4P1ROK, CH5P0ROK, CH5P1ROK, CH6P0ROK, CH6P1ROK, CH7P0ROK, CH7P1ROK};
#else
__pcmifdata06 static unsigned long ChanTxPage[8] = {CH0P0TOK, CH0P1TOK, CH1P0TOK, CH1P1TOK, CH2P0TOK, CH2P1TOK, CH3P0TOK, CH3P1TOK};
__pcmifdata07 static unsigned long ChanRxPage[8] = {CH0P0ROK, CH0P1ROK, CH1P0ROK, CH1P1ROK, CH2P0ROK, CH2P1ROK, CH3P0ROK, CH3P1ROK};
#endif

//__pcmifdata08 uint32 **pRxBuf, **pTxBuf;
__pcmifdata08 uint32 *pRxBuf[ BUS_PCM_CH_NUM ], *pTxBuf[ BUS_PCM_CH_NUM ];


//__pcmifdata18 uint32 rxlayerbuf[ BUS_PCM_CH_NUM ];
//__pcmifdata19 uint32 txlayerbuf[ BUS_PCM_CH_NUM ];


//#include "voip_types.h"
//#include "../voip_dsp/dsp_r1/include/typedef.h"
//#include "voip_control.h"

//uint32 g_txVolumneGain_DAA = 7; // init value: 0dB
//uint32 g_rxVolumneGain_DAA = 7; // init value: 0dB

//#if defined (AUDIOCODES_VOIP)
/*** Play Hold Tone for DAA Tx Path ***/
//unsigned int ToneBuf_DAA[TX_BUF_SIZE/4]={0};
//#else
//extern unsigned int ToneBuf_DAA[];
//#endif

#define PCM_DBG_SUPPRESSION
#define PCM_DBG_PROC	// put T0 R0 in /proc/voip/pcm_dbg

#ifdef PCM_DBG_SUPPRESSION
	#define DBG_NUM_START	1200
	static int dbg_num = 0;
	static uint32 T[BUS_PCM_CH_NUM] = {0};
	static uint32 R[BUS_PCM_CH_NUM] = {0};
	static uint32 TE[BUS_PCM_CH_NUM] = {0};
	static uint32 RF[BUS_PCM_CH_NUM] = {0};

	static uint32 T_[BUS_PCM_CH_NUM] = {0};
	static uint32 R_[BUS_PCM_CH_NUM] = {0};

#ifdef PCM_DBG_PROC
	static uint32 pcm_dbg_sum = 0;
#endif
#endif

//#ifdef FXO_BUSY_TONE_DET
//#include "voip_types.h"
//#include "voip_control.h"
//#include "tone_det_i.h"
//#endif

#ifdef RTK_VOICE_RECORD
#include "voip_control.h"
char txdatabuf[DATAGETBUFSIZE];
char rxdatabuf[DATAGETBUFSIZE];
//char txdatabuf2[DATAGETBUFSIZE];
char rxdatabuf2[DATAGETBUFSIZE];
TstVoipdataget stVoipdataget[BUS_PCM_CH_NUM] = {[0 ... BUS_PCM_CH_NUM-1]={0,0,0,0,0,0,0,txdatabuf, rxdatabuf, rxdatabuf2}};
#endif //#ifdef RTK_VOICE_RECORD

#ifdef RTK_VOICE_PLAY
char txplaydatabuf[DATAPUTBUFSIZE];
TstVoipdataput stVoipdataput[BUS_PCM_CH_NUM] = {[0 ... BUS_PCM_CH_NUM-1]={0,0,0,txplaydatabuf}};
#endif //#ifdef RTK_VOICE_PLAY

static void pcm_disableChan(unsigned int chid);
static void PCM_init(void);
static void pcm_channel_slot(int pcm_mode);
static void print_pcm(void);

static void bus_pcm_set_format( voip_bus_t *this, uint32 _format );

//========================================================//

static int pcm_set_page_size(unsigned int chid, unsigned int size)
{
	printk("\n\n%s: chid %d, size %d\n\n", __func__, chid, size);
	/* Write the reg PCMBSIZE to set pagesize. */

#ifndef G3_TEST
	unsigned int n_size;
	n_size = (size/4 - 1);
#if BUS_PCM_CH_NUM > 4
	if (chid & 4) {/* chid 4~7 */
		pcm_outl(PCMBSIZE47, pcm_inl(PCMBSIZE47) & (~(0xFF << (8 * (MAXCH-(chid&3)-1)))));//set to zero
		pcm_outl(PCMBSIZE47, pcm_inl(PCMBSIZE47)|(n_size << (8 * (MAXCH-(chid&3)-1))));	//set pagesize
	} else
#endif
	{
		pcm_outl(PCMBSIZE, pcm_inl(PCMBSIZE) & (~(0xFF << (8 * (MAXCH-chid-1)))));//set to zero
		pcm_outl(PCMBSIZE, pcm_inl(PCMBSIZE)|(n_size << (8 * (MAXCH-chid-1))));	//set pagesize
	}
	//PDBUG("set channel %d page size = %d\n", chid, size);
	// too many console message will cause R0, T0
	//printk("set channel %d page size = %d\n", chid, size);
#else
#endif
	pcmctrl_devices[ chid ].page_size = size;

	return 0;
}

#ifndef OPTIMIZATION
static unsigned int pcm_get_page_size(unsigned int chid)
{
	/* Read the reg PCMBSIZE to get pagesize*/
	unsigned int pagesize, n_size;	/* Actual pagesize which can get from "pcm_get_page_size()".
 					It's different from the PCMPAGE_SIZE define in header file. */
#ifndef G3_TEST
#if BUS_PCM_CH_NUM > 4
	if (chid & 4) {/* chid 4~7 */
		n_size = ((pcm_inl(PCMBSIZE47) >>( 8 * (MAXCH-(chid&3)-1)))) & 0xFF;
	} else
#endif
	{
		n_size = ((pcm_inl(PCMBSIZE) >>( 8 * (MAXCH-chid-1)))) & 0xFF;
	}
	pagesize = 4*(n_size + 1);

//	PDBUG("get channel %d page size = %d\n", chid, pagesize);
#else
#endif
	printk("\n\n%s: chid %d, pagesize %d\n\n", __func__, chid, pcmctrl_devices[ chid ].page_size);
	return pagesize;
}
#else
//#define pcm_get_page_size(x) (PCM_PERIOD*PCM_PERIOD_10MS_SIZE)
#define pcm_get_page_size( bch ) ( pcmctrl_devices[ bch ].page_size )
#endif

/* Set Tx, Rx own bit to PCM Controller. */
#if defined (AUDIOCODES_VOIP)
//static inline
#endif
static void pcm_set_tx_own_bit(unsigned int chid, unsigned int pageindex)
{
}

#if defined (AUDIOCODES_VOIP)
//static inline
#endif
static void pcm_set_rx_own_bit(unsigned int chid, unsigned int pageindex)
{
}

/* Get the Tx, Rx base address */
static unsigned int pcm_get_tx_base_addr(unsigned int chid)
{
	unsigned int txbaseaddr;
#ifndef G3_TEST
#if BUS_PCM_CH_NUM > 4
	if (chid & 4) {/* chid 4~7 */
	 	//txbaseaddr = (pcm_inl(CH47TX_BSA(chid)) & 0xFFFFFFFC)|0xA0000000; // to virtual non-cached address
	 	txbaseaddr = Physical2NonCache(pcm_inl(CH47TX_BSA(chid)) & 0xFFFFFFFC); // to virtual non-cached address
	} else
#endif
	{
	 	//txbaseaddr = (pcm_inl(TX_BSA(chid)) & 0xFFFFFFFC)|0xA0000000; // to virtual non-cached address
	 	txbaseaddr = Physical2NonCache(pcm_inl(TX_BSA(chid)) & 0xFFFFFFFC); // to virtual non-cached address
	}
	PDBUG(" get Tx base addresss = 0x%x, ch=%d\n", txbaseaddr, chid);
#else
//	printk("get Tx base addresss = 0x%x, ch=%d\n", txBuf[chid], chid);
//	return txBuf[chid];
#endif

	return txbaseaddr;

}


static unsigned int pcm_get_rx_base_addr(unsigned int chid)
{
	unsigned int rxbaseaddr;
#ifndef G3_TEST
#if BUS_PCM_CH_NUM > 4
	if (chid & 4) {/* chid 4~7 */
	 	//rxbaseaddr = (pcm_inl(CH47RX_BSA(chid)) & 0xFFFFFFFC)|0xA0000000; // to virtual non-cached address
	 	rxbaseaddr = Physical2NonCache(pcm_inl(CH47RX_BSA(chid)) & 0xFFFFFFFC); // to virtual non-cached address
	} else
#endif
	{
 		//rxbaseaddr = (pcm_inl(RX_BSA(chid)) & 0xFFFFFFFC)|0xA0000000; // to virtual non-cached address
 		rxbaseaddr = Physical2NonCache(pcm_inl(RX_BSA(chid)) & 0xFFFFFFFC); // to virtual non-cached address
 	}

	PDBUG(" get Rx base addresss = 0x%x, ch=%d\n", rxbaseaddr, chid);
#else
//	printk("get Rx base addresss = 0x%x, ch=%d\n", rxBuf[chid], chid);
//	return rxBuf[chid];
#endif

	return rxbaseaddr;
}


static void pcm_enable(void)
{
}

static void pcm_disable(void)
{
}


void pcm_tx_rx_enable( uint32 bch1, uint32 bch2 )
{
}

static void pcm_tx_disable(unsigned int chid)
{
}

static void pcm_rx_disable(unsigned int chid)
{
}

static void pcm_isr_reset(unsigned int chid)
{
}

static void pcm_isr_reset_tx_rx(unsigned int chid, int tx, int rx)
{
}

static void pcm_imr_enable(unsigned int chid, unsigned char type)
{
}

static void pcm_imr_disable(unsigned int chid, unsigned char type)
{
}

static void EnaPcmIntr(uint32 chid)
{
}

static void DisPcmIntr(uint32 chid)
{
}

//static void init_var(void)
static void init_var(struct pcm_priv *pcm_dev)
{
	int i;

	for(i=0; i<BUS_PCM_CH_NUM; i++)
	{
		chanEnabled[i] = FALSE;

		//FXS_FXO_functionality_init(i);

//#ifdef SUPPORT_PCM_FIFO
//		PCM_FIFO_Init(i);
//#endif

		//rxlayerbuf[i] = (uint32)((uint32)pcm_get_rx_base_addr(i));  // Get virtual non-cached address
		//txlayerbuf[i] = (uint32)((uint32)pcm_get_tx_base_addr(i));  // Get virtual non-cached address
		//pRxBuf[i] = (uint32 *)pcm_get_rx_base_addr(i);
		//pTxBuf[i] = (uint32 *)pcm_get_tx_base_addr(i);
		pRxBuf[i] = (uint32 *)pcm_dev[i].rx_buffer;
		pTxBuf[i] = (uint32 *)pcm_dev[i].tx_buffer;

		txpage[i] = 0;
		rxpage[i] = 0;
		tr_cnt[i] = 0;

		//InitializeIVR( i );


		//printk("----> sync_sample_offset_daa[%d] = %d\n", i, sync_sample_offset_daa[i]);

		//if (pcm_ch_for_DAA[i] == 1)
		//	AES_ON(i);



	}


	//pRxBuf = (uint32**)((uint32)&rxlayerbuf[0]);
	//pTxBuf = (uint32**)((uint32)&txlayerbuf[0]);

	for (i=0; i<BUS_PCM_CH_NUM; i++)
	{
		BOOT_MSG("pTxBuf[%d]=%p\n", i, pTxBuf[i]);
		BOOT_MSG("pRxBuf[%d]=%p\n", i, pRxBuf[i]);
	}

	//ProfileInit();
}





static int test_cnt = 0;
static int pcm_tx_count2 = 0;


#if   1 //def DEBUG_PCM_INTERVAL
long Last_pcm_tx_count=0;
unsigned long  max_pcm_tx_count=0;
unsigned long min_pcm_tx_count=0x7fffffffffffff;
long Last_pcm_rx_count=0;
unsigned long  max_pcm_rx_count=0;
unsigned long min_pcm_rx_count=0x7fffffffffffff;
long tv_sec[5000][2];
long tv_sec2[5000][2];;
int DMA_TX_IDX[5000][2];
unsigned int DMA_TX_count=0;
int DMA_RX_IDX[5000][2];
unsigned int DMA_RX_count=0;
unsigned int tv_rx_index=0;
struct timespec last_time_pcm_tx;
struct timespec last_time_pcm_rx;
#endif
typedef unsigned int ca_uint32_t;
typedef volatile union {
  struct {
    ca_uint32_t index                : 13 ; /* bits 12:0 */
    ca_uint32_t rsrvd1               : 19 ;
  } bf ;
  ca_uint32_t     wrd ;
} DMA_SEC_DMA_SSP_Q_TXQ_WPTR_t;

typedef volatile union {
  struct {
    ca_uint32_t index                : 13 ; /* bits 12:0 */
    ca_uint32_t rsrvd1               : 19 ;
  } bf ;
  ca_uint32_t     wrd ;
} DMA_SEC_DMA_SSP_Q_TXQ_RPTR_t;

typedef volatile union {
  struct {
    ca_uint32_t index                : 13 ; /* bits 12:0 */
    ca_uint32_t rsrvd1               : 19 ;
  } bf ;
  ca_uint32_t     wrd ;
} DMA_SEC_DMA_SSP_Q_RXQ_WPTR_t;


#define DMA_SEC_DMA_SSP_Q_RXQ_RPTR               0xf7001a10
#define DMA_SEC_DMA_SSP_Q_RXQ_RPTR_dft           0x00000000

typedef volatile union {
  struct {
    ca_uint32_t index                : 13 ; /* bits 12:0 */
    ca_uint32_t rsrvd1               : 19 ;
  } bf ;
  ca_uint32_t     wrd ;
} DMA_SEC_DMA_SSP_Q_RXQ_RPTR_t;
int debug_record_flag=1;
int debug_record_flag_count=0;
static int32 pcm_ISR2(int DIR)
{
	uint32 bch = 0, i, j;
	static uint32 last_bch = 0;

	extern void isr_bus_reset_need_rx_vars( void );
	isr_bus_reset_need_rx_vars();

	DMA_SEC_DMA_SSP_Q_TXQ_WPTR_t reg_txq_wptr;
	DMA_SEC_DMA_SSP_Q_TXQ_RPTR_t reg_txq_rptr;
	u16 tx_wt_idx, tx_rd_idx;

	reg_txq_wptr.wrd = audio_rd_dma_reg(CA_DMA_SSP_TX_WPTR);
	tx_wt_idx = reg_txq_wptr.bf.index;

	reg_txq_rptr.wrd = audio_rd_dma_reg( CA_DMA_SSP_TX_RPTR);
	tx_rd_idx=reg_txq_rptr.bf.index;

	DMA_SEC_DMA_SSP_Q_RXQ_WPTR_t reg_rxq_wptr;
	DMA_SEC_DMA_SSP_Q_RXQ_RPTR_t reg_rxq_rptr;
	u16 rx_wt_idx;
	u16 rx_rd_idx;
#ifdef DEBUG_PCM_INTERVAL
	if(DIR==DIR_TX)
	{
	struct timespec time_pcm;
	getnstimeofday(&time_pcm);

	if(DMA_TX_count<1995)
	{
		tv_sec[DMA_TX_count][0]=time_pcm.tv_sec;
		tv_sec[DMA_TX_count][1]=time_pcm.tv_nsec;

	}
	if(DMA_TX_count<2000)
	{
		DMA_TX_IDX[DMA_TX_count][0]=tx_rd_idx;
		DMA_TX_IDX[DMA_TX_count][1]=tx_wt_idx;
	}
	unsigned long current_count=time_pcm.tv_sec*1000000000+time_pcm.tv_nsec;
	if(Last_pcm_tx_count!=0)
	{
	long diff_count=((time_pcm.tv_sec-last_time_pcm_tx.tv_sec)*1000000000+(time_pcm.tv_nsec-last_time_pcm_tx.tv_nsec));
	if((diff_count>max_pcm_tx_count)&&((time_pcm.tv_sec-last_time_pcm_tx.tv_sec)<=10))
	{
		tv_sec[1995][0]=last_time_pcm_tx.tv_sec;
		tv_sec[1995][1]=last_time_pcm_tx.tv_nsec;

		tv_sec[1996][0]=time_pcm.tv_sec;
		tv_sec[1996][1]=time_pcm.tv_nsec;
		max_pcm_tx_count=diff_count;
		tv_sec[1999][0]=diff_count;
	}
	if((diff_count<min_pcm_tx_count)&&(diff_count>=0))
	{

		tv_sec[1997][0]=last_time_pcm_tx.tv_sec;
		tv_sec[1997][1]=last_time_pcm_tx.tv_nsec;

		tv_sec[1998][0]=time_pcm.tv_sec;
		tv_sec[1998][1]=time_pcm.tv_nsec;
		min_pcm_tx_count=diff_count;
		tv_sec[1999][1]=diff_count;
	}
	}

	Last_pcm_tx_count=current_count;
	last_time_pcm_tx=time_pcm;

		DMA_TX_count++;
	}
#endif
	if((DIR==DIR_TX)||((DIR==DIR_RX)&&(tx_wt_idx==tx_rd_idx)))
	{
	//for (j = 0; j < BUS_PCM_CH_NUM; j++, bch = (bch+1)%BUS_PCM_CH_NUM)
		for (j = 0; j < BUS_PCM_CH_NUM; j++, bch = ( bch+1 >= BUS_PCM_CH_NUM ? 0 : bch+1 ) ) {
		    	voip_bus_t * const p_bus = &bus_pcm[ bch ];
		    	voip_con_t * const p_con = p_bus ->con_ptr;

			uint32* txbuf = pTxBuf[bch];

			if (p_bus ->enabled && p_con ->con_ops ->isr_bus_read_tx(p_con,	p_bus, (uint16 *)txbuf) == NULL) {
				// TX FIFO is empty!
				//memset(txbuf, 0, pcm_get_page_size(bch));
				memset((void *)txbuf, 0, 640);
#if 0 //def PCM_DBG_SUPPRESSION
				TE[bch]++;
#else
				printk("TE(%d) ", bch);
#endif
			} else {
				tr_cnt[bch]++;
		   }
		}
		audio_tx_data();
		//if((DIR==DIR_RX)&&(tx_wt_idx==tx_rd_idx))
		audio_dma_tx_update();
	}

	//ca77xx_audio_dma_tx_irq_handler(0, 0);

	reg_rxq_wptr.wrd = audio_rd_dma_reg( CA_DMA_SSP_RX_WPTR);
	rx_wt_idx= reg_rxq_wptr.bf.index;

	reg_rxq_rptr.wrd = audio_rd_dma_reg(CA_DMA_SSP_RX_RPTR);
	rx_rd_idx= reg_rxq_rptr.bf.index;
	int DMA_RX_Buffer_count=audio_get_dma_rxbuf_size(rx_rd_idx,rx_wt_idx);
	#ifdef DEBUG_PCM_INTERVAL
	struct timespec time_pcm;

	if((DIR==DIR_RX)&&(debug_record_flag&1))
	{
		getnstimeofday(&time_pcm);
		if (0) //(tv_rx_index<5000)
		{
			tv_sec2[DMA_RX_count][0]=time_pcm.tv_sec;
			tv_sec2[DMA_RX_count][1]=time_pcm.tv_nsec;
		}
	//	tv_rx_index=(tv_rx_index+1)%5000;
		unsigned long current_count=time_pcm.tv_sec*1000000000+time_pcm.tv_nsec;
		if(Last_pcm_rx_count!=0)
		{
			long diff_count=((time_pcm.tv_sec-last_time_pcm_rx.tv_sec)*1000000000+(time_pcm.tv_nsec-last_time_pcm_rx.tv_nsec));
			if((diff_count>max_pcm_rx_count)&&((time_pcm.tv_sec-last_time_pcm_rx.tv_sec)<=10))
			{
			//	tv_sec2[4995][0]=last_time_pcm_rx.tv_sec;
			//	tv_sec2[4995][1]=last_time_pcm_rx.tv_nsec;

			//	tv_sec2[4996][0]=time_pcm.tv_sec;
			//	tv_sec2[4996][1]=time_pcm.tv_nsec;
				max_pcm_rx_count=diff_count;
			//	tv_sec2[4999][0]=diff_count;
			}
			if((diff_count<min_pcm_rx_count)&&(diff_count>=0))
			{

			//	tv_sec2[4997][0]=last_time_pcm_rx.tv_sec;
			//	tv_sec2[4997][1]=last_time_pcm_rx.tv_nsec;

			//	tv_sec2[4998][0]=time_pcm.tv_sec;
			//	tv_sec2[4998][1]=time_pcm.tv_nsec;
				min_pcm_rx_count=diff_count;
			//	tv_sec2[4999][1]=diff_count;
			}
		}
		Last_pcm_rx_count=current_count;
		last_time_pcm_rx=time_pcm;
		//DMA_RX_count++;
	}
#endif

	if((DIR==DIR_RX)||((DIR==DIR_TX)&&(DMA_RX_Buffer_count>=2)))
	{
		while(audio_get_dma_rxbuf_size(rx_rd_idx,rx_wt_idx)>=2)
		//if((DIR==DIR_RX)||((DIR==DIR_TX)&&(DMA_RX_Buffer_count>=2)))
	 	{
#ifdef DEBUG_PCM_INTERVAL
	 		DMA_RX_Buffer_count=audio_get_dma_rxbuf_size(rx_rd_idx,rx_wt_idx);
			if((DMA_RX_count<5000)&&(debug_record_flag&1))
			{
				DMA_RX_IDX[DMA_RX_count][0]=rx_rd_idx;
				DMA_RX_IDX[DMA_RX_count][1]=rx_wt_idx;
				tv_sec2[DMA_RX_count][0]=time_pcm.tv_sec;
				tv_sec2[DMA_RX_count][1]=time_pcm.tv_nsec;
				DMA_RX_count=(DMA_RX_count+1)%5000;
				if(DMA_RX_Buffer_count>=62)
					debug_record_flag=0;

			}
#endif
			//ca77xx_audio_dma_rx_irq_handler(0, 0);
			audio_rx_data();  //audio->dma_ctrl->rx_buf -----> pRxBuf
		//	if((DIR==DIR_TX)&&(DMA_RX_Buffer_count>=2))
			audio_dma_rx_update();
			reg_rxq_wptr.wrd = audio_rd_dma_reg( CA_DMA_SSP_RX_WPTR);
			rx_wt_idx= reg_rxq_wptr.bf.index;
			reg_rxq_rptr.wrd = audio_rd_dma_reg(CA_DMA_SSP_RX_RPTR);
			rx_rd_idx= reg_rxq_rptr.bf.index;
			for (j = 0; j < BUS_PCM_CH_NUM; j++) {
				const voip_bus_t * const p_bus = &bus_pcm[ j ];
				voip_con_t * const p_con = p_bus->con_ptr;
				voip_snd_t * const p_snd = p_con->snd_ptr;


				if (!p_bus->enabled)
					continue;

				if (p_con->con_ops->isr_bus_write_rx_TH(p_con, p_bus, (uint16 *)pRxBuf[j]) == NULL ) { // pRxBuf ---> rx_fifo
#if 0 //def PCM_DBG_SUPPRESSION
					RF[ j ] ++;
#else
					printk("RF(%d) ", j);
#endif
				} else  {
					if (gVoipCounterEnable == 1)
						PCM_rx_count(j);
				}
			}
			for (j = 0; j < BUS_PCM_CH_NUM; j++) {
				const voip_bus_t * const p_bus = &bus_pcm[ j ];
				voip_con_t * const p_con = p_bus ->con_ptr;

				if (!p_bus ->enabled)
					continue;

				p_con->con_ops->isr_bus_write_rx_BH(p_con);
			    }
			}
	}
#ifdef DEBUG_PCM_INTERVAL
	if(debug_record_flag==0)
	{
		int k;
		int kk=0;
		for ( k=DMA_RX_count;kk<5000;kk++,k=(k+1)%5000)
	 		printk("%5d %5d (%3d)  :  (%12d  ,  %12d)\n",DMA_RX_IDX[k][0],DMA_RX_IDX[k][1],(DMA_RX_IDX[k][1]-DMA_RX_IDX[k][0]+64)%64,tv_sec2[k][0],tv_sec2[k][1]);
		debug_record_flag=2;
	}
#endif
	isr_schedule_bus_handler();

	return 0;
}

#ifdef SUPPORT_PCM_FIFO
__pcmIsr01
//#define CHECK_TR_CNT // Let TX and RX in order
#ifndef G3_TEST
#if BUS_PCM_CH_NUM > 4
static int32 pcm_ISR(uint32 pcm_isr03, uint32 pcm_isr47)
#else
static int32 pcm_ISR(uint32 pcm_isr)
#endif
#else
static int32 pcm_ISR(void)
#endif
{

#if 0  //loopback test without RTK codes
		ca77xx_audio_dma_tx_irq_handler(0,0);
		ca77xx_audio_dma_rx_irq_handler(0,0);

		return 0;
#endif

	uint32 bch = 0, i, j;
	static uint32 last_bch = 0;
	uint32 tx_isrpage, rx_isrpage;
#if BUS_PCM_CH_NUM > 4
	uint32* pcm_isr = NULL;
#endif

	//bch = (++last_bch) % BUS_PCM_CH_NUM;
	bch = ( ++last_bch >= BUS_PCM_CH_NUM ? 0 : last_bch );
#ifndef G3_TEST
	for (i=0; i < 2; i++) // page0/page1
	{
#endif
	    //int need_PCM_RX[ BUS_PCM_CH_NUM ];
#ifndef SUPPORT_SYS_DMEM_STACK
	//static
#endif
	    //__attribute__((aligned(8)))
		//uint32 rxBuf[ BUS_PCM_CH_NUM ][PCM_PERIOD*PCM_PERIOD_10MS_SIZE/sizeof(uint32)];
	    //memset(need_PCM_RX, 0, sizeof(need_PCM_RX));
	    extern void isr_bus_reset_need_rx_vars( void );
    	isr_bus_reset_need_rx_vars();

	    //for (j = 0; j < BUS_PCM_CH_NUM; j++, bch = (bch+1)%BUS_PCM_CH_NUM)
	    for (j = 0; j < BUS_PCM_CH_NUM; j++, bch = ( bch+1 >= BUS_PCM_CH_NUM ? 0 : bch+1 ) )
	    {
	    	voip_bus_t * const p_bus = &bus_pcm[ bch ];
	    	//voip_bus_t * const p_bus = &bus_pcm[ j];
	    	voip_con_t * const p_con = p_bus ->con_ptr;

#if !defined (AUDIOCODES_VOTING_MECHANISM)
	    	//if (chanEnabled[bch] == FALSE)
			//	continue;
#if 0  //HO, g3 test
			if( !p_bus ->enabled ){
				//printk("HOHOHO NA\n");
				continue;
			}
#endif
#endif

#ifndef G3_TEST
#if BUS_PCM_CH_NUM > 4
			if (bch < 4)
		    	pcm_isr = &pcm_isr03;
			else
		    	pcm_isr = &pcm_isr47;
#endif
			tx_isrpage = 2*bch + txpage[bch];
			rx_isrpage = 2*bch + rxpage[bch];

#if 0 // debug only
			if ( 	tr_cnt[bch] == 1
				&& (pcm_isr & ChanTxPage[tx_isrpage])
				&& (pcm_isr & ChanRxPage[rx_isrpage]) == 0
			) {
				printk("+");
				int pcm_isr2 = pcm_inl(PCMISR) & ChanRxPage[rx_isrpage];
				if (pcm_isr2 != 0) {
					pcm_clean_isr(pcm_isr2);
					pcm_isr |= pcm_isr2;
					printk("-");
				}
			}
#endif

#ifdef CHECK_TR_CNT
	#if BUS_PCM_CH_NUM > 4
			if( ((*pcm_isr) & ChanTxPage[tx_isrpage]) && tr_cnt[bch] == 0 )
	#else
			// check if PCM TX and RX is in order, too.
			if( (pcm_isr & ChanTxPage[tx_isrpage]) && tr_cnt[bch] == 0 )
	#endif
#else
	#if BUS_PCM_CH_NUM > 4
			if( ((*pcm_isr) & ChanTxPage[tx_isrpage]) )
	#else
			if( (pcm_isr & ChanTxPage[tx_isrpage]) )
	#endif
#endif
			{
#endif
				voip_snd_t * const p_snd = p_con ->snd_ptr;

#if defined (AUDIOCODES_VOTING_MECHANISM)
				//if (chanEnabled[chid] == TRUE)	{
				if(1){
#endif
#ifndef G3_TEST
					uint32* txbuf = &pTxBuf[bch][txpage[bch]*(pcm_get_page_size(bch)>>2)];
#else
					uint32* txbuf = pTxBuf[bch];
#endif
					//if(test_cnt==0) printk(" p_con 0x%x, p_bus 0x%x\n", p_con, p_bus);
					//if(test_cnt==1) printk(" p_con 0x%x, p_bus 0x%x\n", p_con, p_bus);


					//HO, g3 test
					//if( p_con ->con_ops ->isr_bus_read_tx( p_con,
					if( p_bus ->enabled && p_con ->con_ops ->isr_bus_read_tx( p_con,
								p_bus, ( uint16 * )txbuf ) == NULL )
					{
						// TX FIFO is empty!
						//memset(txbuf, 0, pcm_get_page_size(bch));
						memset((void *)txbuf, 0, 640);
					#ifdef PCM_DBG_SUPPRESSION
						TE[bch]++;
					#else
						printk("TE(%d) ", bch);
					#endif
					} else {
/*						if(test_cnt++%500==0){
							printk("j=%d, bch=%d  pTxBuf[0]=0x%x pTxBuf[1]=0x%x\n", j, bch, pTxBuf[0], pTxBuf[1]);
							printk("0x%x,  0x%x 0x%x 0x%x 0x%x   txcount=%d, TE=%d\n", txbuf, txbuf[0], txbuf[1], txbuf[2], txbuf[3], pcm_tx_count2, TE[0]);
						} */
#ifdef SUPPORT_SILAB_FXO_TUNE
						int tmp;

						if (FXO_tune_test_flag_get(p_con->cch))
						{
							//if (p_con->cch != 2)
							//	printk(">%d ", p_con->cch);

							memset(txbuf, 0, pcm_get_page_size(bch));
							test_tone_gen( p_con->cch, ( Word16* )txbuf, 80);
							FXO_tune_test_tone_cntr_add(p_con->cch);
							tmp = tone_analysis(p_con->cch, (unsigned short*)txbuf, 80 );
							//printk(">%d\n", tmp);
							FXO_tune_test_tone_pwr_update(p_con->cch, tmp);
						}

						// Debug
						//if( p_con->cch == 2 )
						//	ddinst_rw_auto( VOIP_DEV_PCM1_TX, ( char * )txbuf, 160 );
#endif

#ifdef SUPPORT_VOIP_DBG_COUNTER
						if (gVoipCounterEnable == 1)
							PCM_tx_count(bch);
#endif
						//HO, g3 test, test
						pcm_tx_count2++;
					}

#if 0	// TX play tone test
					{
						const short vpat[16]={32767,30272,23170,12539,0,-12539,-23170,-30272,-32767,-30272,-23170,-12539, 0,12539,23170,30272 };
						short *tx_x = txbuf;
						int x;

						if( p_bus ->enabled == 1 ) {
							// narrowband
							for( x = 0; x < 80; x ++ ) {
								*( tx_x + x ) = vpat[ x & 0xF ];	// '& 0xF' is '% 16'
							}
						} else {
							// wideband
							for( x = 0; x < 160; x ++ ) {
								*( tx_x + x ) = vpat[ ( x >> 1 ) & 0xF ];	// '& 0xF' is '% 16'
							}
						}
					}
#endif

#if defined (AUDIOCODES_VOTING_MECHANISM)
				}	// chanEnabled
#endif
#if 0//defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_LE89116) && defined(CONFIG_RTK_VOIP_DRIVERS_DAA_SUPPORT)
				if( p_snd ->snd_flags.b.attenuateTx_6dB ) {
					int jj, tmp, pcm_sample;
					short *tmp_ptr;
					pcm_sample = pcm_get_page_size(bch)>>1;
					tmp_ptr = (short*)txbuf;
					for (jj=0; jj<(pcm_sample); jj++) {
						tmp = (*tmp_ptr)>>1;
						*tmp_ptr++=tmp;
					}
				}
#endif
#ifndef G3_TEST
				pcm_set_tx_own_bit(bch, txpage[bch]);
#if BUS_PCM_CH_NUM > 4
				(*pcm_isr) &= ~ChanTxPage[tx_isrpage];
#else
				pcm_isr &= ~ChanTxPage[tx_isrpage];
#endif
				txpage[bch] ^= 1;
#endif
				if(j==BUS_PCM_CH_NUM-1)
					ca77xx_audio_dma_tx_irq_handler(0,0);


				tr_cnt[bch]++;
#ifndef G3_TEST
			} // end of tx
#endif
/*
			ca77xx_audio_dma_rx_irq_handler(0,0);
			isr_schedule_bus_handler();

			return 0;
			*/

#ifndef G3_TEST
#if BUS_PCM_CH_NUM > 4
			if( (*pcm_isr) & ChanRxPage[rx_isrpage] ) {
#else
			if( pcm_isr & ChanRxPage[rx_isrpage] ) {
#endif
#endif
#if 0	// RX play tone test
					{
						const short vpat[16]={32767,30272,23170,12539,0,-12539,-23170,-30272,-32767,-30272,-23170,-12539, 0,12539,23170,30272 };
						short *rx_x = &pRxBuf[bch][rxpage[bch]*(pcm_get_page_size(bch)>>2)];
						int x;

						if( p_bus ->enabled == 1 ) {
							// narrowband
							for( x = 0; x < 80; x ++ ) {
								*( rx_x + x ) = vpat[ x & 0xF ];	// '& 0xF' is '% 16'
							}
						} else {
							// wideband
							for( x = 0; x < 160; x ++ ) {
								*( rx_x + x ) = vpat[ ( x >> 1 ) & 0xF ];	// '& 0xF' is '% 16'
							}
						}
					}
#endif
				// pcm_set_rx_own_bit ASAP helps a lot!

#ifdef G3_TEST
				if(j==BUS_PCM_CH_NUM-1){
					ca77xx_audio_dma_rx_irq_handler(0,0);
				}
#endif

#if 0  //HO, g3 test, for test only
#ifndef G3_TEST
				if( p_con ->con_ops ->isr_bus_write_rx_TH( p_con, p_bus,
						( uint16 * )&pRxBuf[bch][rxpage[bch]*(pcm_get_page_size(bch)>>2)] )
						== NULL )
#else
				if( p_con ->con_ops ->isr_bus_write_rx_TH( p_con, p_bus,
						( uint16 * )pRxBuf[bch])
						== NULL )
#endif
				{
					#ifdef PCM_DBG_SUPPRESSION
					RF[ bch ] ++;
					#else
					printk("RF(%d) ", bch);
					#endif
				}
#ifdef SUPPORT_VOIP_DBG_COUNTER
				else
				{
					if (gVoipCounterEnable == 1)
						PCM_rx_count(bch);
				}
#endif

#endif

#ifndef G3_TEST
				pcm_set_rx_own_bit(bch, rxpage[bch]);

#if BUS_PCM_CH_NUM > 4
				(*pcm_isr) &= ~ChanRxPage[rx_isrpage];
#else
				pcm_isr &= ~ChanRxPage[rx_isrpage];
#endif
				rxpage[bch] ^= 1;
#ifdef CHECK_TR_CNT
				if (tr_cnt[bch] != 1) {
					//printk("tr_cnt[%d]=%d ", bch, tr_cnt[bch]);
					printk("tr%d-%d ", bch, tr_cnt[bch]);
					tr_cnt[bch] = 1;
				}
#endif
#endif

				tr_cnt[bch]--;
				last_bch = bch;
#ifndef G3_TEST
			}
#endif
	    } // end of for j

	    for (j = 0; j < BUS_PCM_CH_NUM; j++)
	    {
	    	const voip_bus_t * const p_bus = &bus_pcm[ j ];
	    	voip_con_t * const p_con = p_bus ->con_ptr;
		#if 1
			if( !p_bus ->enabled ){
				//printk("HOHOHO NA2\n");
				continue;
			}
		#endif
			if( p_con ->con_ops ->isr_bus_write_rx_TH( p_con, p_bus,
					( uint16 * )pRxBuf[j]) == NULL )
			{
					#ifdef PCM_DBG_SUPPRESSION
								RF[ j ] ++;
					#else
								printk("RF(%d) ", j);
					#endif
			}
			else
			{
					if (gVoipCounterEnable == 1)
							PCM_rx_count(j);
			}

	    }

	    // Do PCM_RX()
	    for (j = 0; j < BUS_PCM_CH_NUM; j++)
	    {
	    	const voip_bus_t * const p_bus = &bus_pcm[ j ];
	    	voip_con_t * const p_con = p_bus ->con_ptr;
		#if 1
			if( !p_bus ->enabled ){
				//printk("HOHOHO NA2\n");
				continue;
			}
		#endif
			p_con ->con_ops ->isr_bus_write_rx_BH( p_con );
	    }
#ifndef G3_TEST
	} // end of for i
#endif

#ifndef G3_TEST
#if 1
#if BUS_PCM_CH_NUM > 4
	if (pcm_isr03 != 0 || pcm_isr47 != 0) {
		printk(" pcm_isr03 = %X \n", pcm_isr03);
		printk(" pcm_isr47 = %X \n", pcm_isr47);
	}
#else
	if (pcm_isr != 0)
		printk(" pcm_isr = %X ", pcm_isr);
#endif
#endif
#endif
	isr_schedule_bus_handler();

	return 0;
}
#else // SUPPORT_PCM_FIFO

#if BUS_PCM_CH_NUM > 4
static int32 pcm_ISR(uint32 pcm_isr03, uint32 pcm_isr47)
#else
static int32 pcm_ISR(uint32 pcm_isr)
#endif
{
#ifdef G3_TEST
#if 0  //loopback test without RTK codes
		ca77xx_audio_dma_tx_irq_handler(0,0);
		ca77xx_audio_dma_rx_irq_handler(0,0);

		return 0;
#endif
#endif

	uint32 chid, tx_isrpage, rx_isrpage;
#if BUS_PCM_CH_NUM > 4
	uint32* pcm_isr = NULL;
	static uint32 last_isr03=0, last_isr47=0;
	uint32 orig_isr03 = pcm_isr03, orig_isr47 = pcm_isr47;
#else
	static uint32 last_isr=0;
	uint32 orig_isr = pcm_isr;
#endif


#ifndef G3_TEST
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM8651

#if 1
	if( pcm_isr & 0x0F0F0F0F )
	{
		if(pcm_isr & 0x0F000000)
			printk("BU0 ");
		if(pcm_isr & 0x000F0000)
			printk("BU1 ");
	}
#endif
	pcm_isr &= 0xF0F0F0F0;
#endif
#endif

	for(chid=0; chid<BUS_PCM_CH_NUM; chid++)
	{
		if (chanEnabled[chid] == FALSE)
			continue;

#ifndef G3_TEST
#if BUS_PCM_CH_NUM > 4
	    	if (chid < 4)
	    		pcm_isr = &pcm_isr03;
		else
	    		pcm_isr = &pcm_isr47;
#endif

#if BUS_PCM_CH_NUM > 4
	    while ( (*pcm_isr) & (0xF0000000 >> ((chid&3)*8))) {
#else
	    while ( pcm_isr & (0xF0000000 >> (chid*8))) {
#endif
// Handle Tx First
		tx_isrpage = 2*chid + txpage[chid];
#endif

#if BUS_PCM_CH_NUM > 4
		if( ((*pcm_isr) & ChanTxPage[tx_isrpage]) && (tr_cnt[chid] == 0))
#else
		if( (pcm_isr & ChanTxPage[tx_isrpage]) && (tr_cnt[chid] == 0))
#endif
		{
			if (tr_cnt[chid] > 0) {
#if BUS_PCM_CH_NUM > 4
				printk("TX: tr_cnt[chid]=%d, isr03=%X(%X), isr47=%X(%X),page(%d,%d)\n", tr_cnt[chid], orig_isr03, last_isr03, orig_isr47, last_isr47, txpage[chid], rxpage[chid]);
#else
				printk("TX: tr_cnt[chid]=%d, isr=%X(%X), page(%d,%d)\n", tr_cnt[chid], orig_isr, last_isr, txpage[chid], rxpage[chid]);
#endif
				while(1) ;
			}
			tr_cnt[chid]++;
			memcpy( &pTxBuf[chid][txpage[chid]*pcm_get_page_size(chid)>>2], &TxBufTmp[chid][0] , pcm_get_page_size(chid) );
			pcm_set_tx_own_bit(chid, txpage[chid]);
#if BUS_PCM_CH_NUM > 4
			(*pcm_isr) &= ~ChanTxPage[tx_isrpage];
#else
			pcm_isr &= ~ChanTxPage[tx_isrpage];
#endif
			txpage[chid] ^= 1;
			//printk(" t%d ", frame_count%10);
			//printk("t");
		}

		// Check ROK to avoid system down!
		rx_isrpage = 2*chid + rxpage[chid];
#if BUS_PCM_CH_NUM > 4
		if ( ((*pcm_isr) & ChanRxPage[rx_isrpage]) == 0 ) {
#else
		if ( (pcm_isr & ChanRxPage[rx_isrpage]) == 0 ) {
#endif
#if BUS_PCM_CH_NUM > 4
			int pcm_isr2;
			if (chid & 4) {
				pcm_isr2 = pcm_inl(PCMISR47) & ChanRxPage[rx_isrpage]; // ROK only
				pcm_clean_isr47(pcm_isr2);
			} else {
				pcm_isr2  = pcm_inl(PCMISR) & ChanRxPage[rx_isrpage]; // ROK only
				pcm_clean_isr(pcm_isr2);
			}
			pcm_isr |= pcm_isr2;
#else
			int pcm_isr2 = pcm_inl(PCMISR) & ChanRxPage[rx_isrpage]; // ROK only
			pcm_clean_isr(pcm_isr2);
			pcm_isr |= pcm_isr2;
			//printk("R\n");
#endif
		}

#if BUS_PCM_CH_NUM > 4
		if( (*pcm_isr) & ChanRxPage[rx_isrpage] ) {
#else
		if( pcm_isr & ChanRxPage[rx_isrpage] ) {
#endif
			//printk("r");
			if (tr_cnt[chid] == 0) { // wait for TOK: for db-121 un-buffed board only?
				//PCM_recovery(chid, &pcm_isr, &tr_cnt[chid], &txpage[chid], &rxpage[chid]);
				//continue;
				printk(".");
				tr_cnt[chid]++;
			}

			tr_cnt[chid]--;

			PCM_RX(chid);
			pcm_set_rx_own_bit(chid, rxpage[chid]);

			//pcm_set_rx_own_bit(chid, rxpage[chid]);
#if BUS_PCM_CH_NUM > 4
			(*pcm_isr) &= ~ChanRxPage[rx_isrpage];
#else
			pcm_isr &= ~ChanRxPage[rx_isrpage];
#endif
			rxpage[chid] ^= 1;
		#ifdef FEATURE_COP3_PCM_HANDLER
			ProfileEnterPoint(PROFILE_INDEX_PCM_HANDLER);
		#endif
			PCM_handler(chid);
			//printk(" r2%d ", frame_count%10);
		#ifdef FEATURE_COP3_PCM_HANDLER
			ProfileExitPoint(PROFILE_INDEX_PCM_HANDLER);
			ProfilePerDump(PROFILE_INDEX_PCM_HANDLER, cp3_voip_param.cp3_dump_period);
		#endif
			// Check TOK to avoid TU
			tx_isrpage = 2*chid + txpage[chid];
#if BUS_PCM_CH_NUM > 4
			if ( ((*pcm_isr) & ChanTxPage[tx_isrpage]) == 0 ) {
#else
			if ( (pcm_isr & ChanTxPage[tx_isrpage]) == 0 ) {
#endif

#if BUS_PCM_CH_NUM > 4

#endif
				int pcm_isr2;
#if BUS_PCM_CH_NUM > 4
				if (chid & 4)
					pcm_isr2 = pcm_inl(PCMISR47) & ChanTxPage[tx_isrpage];
				else
#endif
					pcm_isr2 = pcm_inl(PCMISR) & ChanTxPage[tx_isrpage];
				if (pcm_isr2 != 0) {

#if BUS_PCM_CH_NUM > 4
					if (chid & 4)
						pcm_clean_isr47(pcm_isr2);
					else
#endif
						pcm_clean_isr(pcm_isr2);

#if BUS_PCM_CH_NUM > 4
					(*pcm_isr) |= pcm_isr2;
#else
					pcm_isr |= pcm_isr2;
#endif

					tr_cnt[chid]++;

					memcpy( &pTxBuf[chid][txpage[chid]*pcm_get_page_size(chid)>>2], &TxBufTmp[chid][0] , pcm_get_page_size(chid) );

					pcm_set_tx_own_bit(chid, txpage[chid]);
#if BUS_PCM_CH_NUM > 4
					(*pcm_isr) &= ~ChanTxPage[tx_isrpage];
#else
					pcm_isr &= ~ChanTxPage[tx_isrpage];
#endif
					txpage[chid] ^= 1;
					//printk("T");
				}
				//printk("R\n");
			}

		}

	    } // end of while





	} // end of for

#if BUS_PCM_CH_NUM > 4
	last_isr03 = orig_isr03;
	last_isr47 = orig_isr47;
	if ((*pcm_isr) != 0)
		printk(" %X ", (*pcm_isr));
#else
	last_isr = orig_isr;
	if (pcm_isr != 0)
		printk(" %X ", pcm_isr);
#endif

	return 0;
}
#endif // SUPPORT_PCM_FIFO

#ifndef CONFIG_RTK_VOIP_PLATFORM_7742
#ifdef CONFIG_RTK_VOIP_MODULE
#undef SUPPORT_SYS_DMEM_STACK
#else
#if ! defined (AUDIOCODES_VOIP)
#ifdef PCM_HANDLER_USE_TASKLET
#ifdef VOCODER_INT
#if PCM_PERIOD != 1
#define SUPPORT_SYS_DMEM_STACK
#endif
#else // !VOCODER_INT
#define SUPPORT_SYS_DMEM_STACK
#endif // VOCODER_INT
#endif
#endif
#endif

#ifdef SUPPORT_SYS_DMEM_STACK
#include "../../voip_dsp/dsp_r1/common/util/codec_mem.h"
#endif
#endif


#if !defined(CONFIG_RTK_VOIP_DRIVERS_PCM8972B_FAMILY) && !defined(CONFIG_RTK_VOIP_DRIVERS_PCM89xxC) &&	!defined(CONFIG_RTK_VOIP_DRIVERS_PCM89xxD) && !defined(CONFIG_RTK_VOIP_DRIVERS_PCM8672) && !defined (CONFIG_RTK_VOIP_DRIVERS_PCM8676) &&	\
	!defined(CONFIG_RTK_VOIP_PLATFORM_8686) && !defined(CONFIG_RTK_VOIP_PLATFORM_7742)
//#define CHECK_PCM_ISR_AGAIN
#endif

#define CHECK_PCM_ISR_REENTRY
#ifdef CHECK_PCM_ISR_REENTRY
static int in_pcm_isr = 0;
#endif


__pcmIsr00
#if defined(CONFIG_DEFAULTS_KERNEL_2_6) || defined(CONFIG_DEFAULTS_KERNEL_3_4) || defined(CONFIG_DEFAULTS_KERNEL_3_10)
/*static */irqreturn_t pcm_interrupt(int32 irq, void *dev_instance,int DIR)
#elif defined CONFIG_RTK_VOIP_DRIVERS_PCM8672 || defined CONFIG_RTK_VOIP_DRIVERS_PCM8676
__IRAM static irq_handler_t pcm_interrupt(int32 irq, void *dev_instance)
#else
static void pcm_interrupt(int32 irq, void *dev_instance, struct pt_regs *regs)
#endif
{
#ifndef CONFIG_CPU_HAS_RADIAX
#ifndef AUDIOCODES_VOIP
	extern int save_radiax_reg(void);
	extern int load_radiax_reg(void);
#endif
#endif
#ifdef SUPPORT_SYS_DMEM_STACK
	extern int dmem_size;
#endif

	unsigned int status_val;
#if BUS_PCM_CH_NUM > 4
	unsigned int status_val47;
#endif

#if ! defined (AUDIOCODES_VOTING_MECHANISM)
	unsigned int maskval;
	int channel=0;
#endif

#ifdef CHECK_PCM_ISR_REENTRY
	if (in_pcm_isr == 1) {
		printk("PCM ISR re-entry\n");
		while (1) ;
	}
	in_pcm_isr = 1;
#endif

#ifndef CONFIG_CPU_HAS_RADIAX
#ifndef AUDIOCODES_VOIP
#ifndef VC6
	save_radiax_reg(); /* save radiax register value */
#endif
#else
	volatile struct pt_radiax radiax_regs;
	save_radiax_reg(&radiax_regs); /* save radiax register value */
#endif
#endif

#ifdef SUPPORT_SYS_DMEM_STACK
	set_DMEM((unsigned long)&__sys_dmem_start, (unsigned long)(dmem_size-1));

	sys_dmem_sp = (unsigned long)&(sys_dmem_stack[SYS_DMEM_SSIZE]);
	entry_dmem_stack(&sys_orig_sp, &sys_dmem_sp);
#endif

#ifdef FEATURE_COP3_PCMISR
	if (cp3_voip_param.bCp3Count_PCM_ISR == 1)
		ProfileEnterPoint(PROFILE_INDEX_PCMISR);
#endif

	//printk("1 PCMISR= 0x%x\n", pcm_inl(PCMISR));

#ifdef G3_TEST
	pcm_ISR2(DIR);
#else
#ifdef CHECK_PCM_ISR_AGAIN
	//int pcm_isr_cnt = 0;
    #if BUS_PCM_CH_NUM > 4
	while ((status_val = pcm_inl(PCMISR)) || (status_val47 = pcm_inl(PCMISR47)))
    #else
	while((status_val = pcm_inl(PCMISR)))
    #endif
#else
    #if BUS_PCM_CH_NUM > 4
	status_val = pcm_inl(PCMISR);
	status_val47 = pcm_inl(PCMISR47);
	if ((status_val) || (status_val47))
    #else
	if((status_val = pcm_inl(PCMISR)))
    #endif
#endif
	{
#ifdef CHECK_PCM_ISR_AGAIN
    #if BUS_PCM_CH_NUM > 4
		status_val47 = pcm_inl(PCMISR47);/* status_val47 may not update when PCMISR true */
    #endif
#if 0
		pcm_isr_cnt++;
#endif
#endif
#if defined (AUDIOCODES_VOTING_MECHANISM)
		AC_PCM_ISR_WRITE(status_val);
#else
    #if BUS_PCM_CH_NUM > 4
		pcm_outl(PCMISR47, status_val47);
    #endif
		pcm_outl(PCMISR, status_val);
#endif
		//if ((status_val& pcm_inl(PCMIMR)) & 0xF0F0F0F0) { // TOK and ROK only
#if BUS_PCM_CH_NUM > 4
		if ( ((status_val) & 0xF0F0F0F0) || ((status_val47) & 0xF0F0F0F0) ) { // TOK and ROK only
			pcm_ISR(status_val & 0xF0F0F0F0, status_val47 & 0xF0F0F0F0);
		}
#else
		if ((status_val) & 0xF0F0F0F0) { // TOK and ROK only
			pcm_ISR(status_val & 0xF0F0F0F0);
		}
#endif


#if ! defined (AUDIOCODES_VOTING_MECHANISM)

#if 1
		//if((maskval = (status_val & ISR_MASK(channel))))
		if((maskval = (status_val & 0x0F0F0F0F))) // Buffer Unavailable only
		{
#if 0
			if(maskval & ROK_MASK(channel))
				printk("ROK.....");
			if(maskval & TOK_MASK(channel))
				printk("TOK.....");
#endif

#ifdef PCM_DBG_SUPPRESSION
			for( channel = 0; channel < BUS_PCM_CH_NUM; channel ++ ) {
				if(maskval & TBU_MASK(channel))	{
					T[channel]++;
					T_[channel]++;
				}
				if(maskval & RBU_MASK(channel)) {
					R[channel]++;
					R_[channel]++;
				}
			}
#else
			channel=0;
			if(maskval & TBU_MASK(channel))
				printk("T0 ");
			if(maskval & RBU_MASK(channel))
				printk("R0 ");
			channel=1;
			if(maskval & TBU_MASK(channel))
				printk("T1 ");
			if(maskval & RBU_MASK(channel))
				printk("R1 ");
			channel=2;
			if(maskval & TBU_MASK(channel))
				printk("T2 ");
			if(maskval & RBU_MASK(channel))
				printk("R2 ");
			channel=3;
			if(maskval & TBU_MASK(channel))
				printk("T3 ");
			if(maskval & RBU_MASK(channel))
				printk("R3 ");

			printk("\n");
#endif
		}
    #if BUS_PCM_CH_NUM > 4
		if((maskval = (status_val47 & 0x0F0F0F0F))) // Buffer Unavailable only
		{
#if 0
			if(maskval & ROK_MASK(channel))
				printk("ROK.....");
			if(maskval & TOK_MASK(channel))
				printk("TOK.....");
#endif

	#ifdef PCM_DBG_SUPPRESSION
			for( channel = 4; channel < 8; channel ++ ) {
				if(maskval & TBU_MASK(channel))	{
					T[channel]++;
					T_[channel]++;
				}
				if(maskval & RBU_MASK(channel)) {
					R[channel]++;
					R_[channel]++;
				}
			}
	#else
			channel=4;
			if(maskval & TBU_MASK(channel))
				printk("T4 ");
			if(maskval & RBU_MASK(channel))
				printk("R4 ");
			channel=5;
			if(maskval & TBU_MASK(channel))
				printk("T5 ");
			if(maskval & RBU_MASK(channel))
				printk("R5 ");
			channel=6;
			if(maskval & TBU_MASK(channel))
				printk("T6 ");
			if(maskval & RBU_MASK(channel))
				printk("R6 ");
			channel=7;
			if(maskval & TBU_MASK(channel))
				printk("T7 ");
			if(maskval & RBU_MASK(channel))
				printk("R7 ");

			printk("\n");
	#endif
		}
    #endif
#endif
#endif /*AUDIOCODES_VOTING_MECHANISM*/

	}

#endif  //G3_TEST

#if 0
//#ifdef PCM_DBG_SUPPRESSION
	dbg_num++;
	int i;
	if(dbg_num == 1200)
	{
		for(i = 0; i <BUS_PCM_CH_NUM; i++)
		{
			if(T[i])
				printk("T%d[%d]", i,T[i]);
			if(R[i])
				printk("R%d[%d]", i,R[i]);
			if(TE[i])
				printk("TE%d[%d]", i,TE[i]);
			if(RF[i])
				printk("RF%d[%d]", i,RF[i]);
			T[i] = R[i] = TE[i] = RF[i] = 0;
		}
		dbg_num = 0;
	}
#endif

#ifdef CHECK_PCM_ISR_AGAIN
#if 0
	if ( pcm_isr_cnt > 1)
		printk(" (%d) ", pcm_isr_cnt);
#endif
#endif

#ifdef FEATURE_COP3_PCMISR
	if (cp3_voip_param.bCp3Count_PCM_ISR == 1)
		ProfileExitPoint(PROFILE_INDEX_PCMISR);
#endif

#ifdef SUPPORT_SYS_DMEM_STACK
	leave_dmem_stack(&sys_orig_sp);
	sys_orig_sp = 0;
#endif

#ifndef CONFIG_CPU_HAS_RADIAX
#ifndef AUDIOCODES_VOIP
#ifndef VC6
	load_radiax_reg(); /* load saved radiax register value */
#endif
#else
	restore_radiax_reg(&radiax_regs);
#endif
#endif

#ifdef FEATURE_COP3_PCMISR
	if (cp3_voip_param.bCp3Count_PCM_ISR == 1)
		ProfilePerDump(PROFILE_INDEX_PCMISR, cp3_voip_param.cp3_dump_period);
#endif

#ifdef CHECK_PCM_ISR_REENTRY
	in_pcm_isr = 0;
#endif

#if defined(CONFIG_DEFAULTS_KERNEL_2_6) || defined(CONFIG_DEFAULTS_KERNEL_3_4) || defined(CONFIG_DEFAULTS_KERNEL_3_10)
	return IRQ_HANDLED;
#endif
}

#ifdef PCM_DBG_SUPPRESSION
static void printk_pcm_dbg_in_tasklet( void )
{
	int i;
#ifdef PCM_DBG_PROC
	uint32 sum = 0;
#else
	int type;
#endif

	dbg_num++;
	if(dbg_num < DBG_NUM_START)
		return;

#ifdef PCM_DBG_PROC
	dbg_num = 0;

	for( i = 0; i < BUS_PCM_CH_NUM; i ++ ) {
		sum += T[ i ];
		sum += R[ i ];
		sum += TE[ i ];
		sum += RF[ i ];
	}

	if( pcm_dbg_sum != sum ) {
		pcm_dbg_sum = sum;
		printk( "T0" );
	}
#else
	i = ( dbg_num - DBG_NUM_START ) >> 2;
	type = ( dbg_num - DBG_NUM_START ) & 0x03;

	if( i >= BUS_PCM_CH_NUM ) {
		dbg_num = 0;
		goto label_pcm_dbg_done;
	}

	if( T[i] && type == 0 )
	{
		printk("T%d[%d]", i,T[i]);
		T[i] = 0;
	}
	else if( R[i] && type == 1 )
	{
		printk("R%d[%d]", i,R[i]);
		R[i] = 0;
	}
	else if( TE[i] && type == 2 )
	{
		printk("TE%d[%d]", i,TE[i]);
		TE[i] = 0;
	}
	else if( RF[i] && type == 3 )
	{
		printk("RF%d[%d]", i,RF[i]);
		RF[i] = 0;
	}

label_pcm_dbg_done:
	;
#endif
}
#endif

#if 1
static uint32 reseting_debouncing = 0;
//Done
static inline void reset_pcm_BH_in_timer( void )
{
	uint32 reseting;
	uint32 bch;

	if( reseting_debouncing == 0 )
		return;

	if( -- reseting_debouncing > 0 )
		return;

	reseting_debouncing = 0;	// be more safe!

	for( bch = 0; bch < BUS_PCM_CH_NUM; bch ++ ) {
		voip_bus_t *p_bus = &bus_pcm[ bch ];

		if( !p_bus ->reseting )
			continue;

		reseting = p_bus ->reseting;
		p_bus ->reseting = 0;

		if( p_bus ->enabled != 2 )
			continue;

		p_bus ->bus_ops ->reset_BH( p_bus, reseting & 0x02, reseting & 0x01 );
	}
}
//Done
void bus_pcm_checking_timer( void )
{
	if( bus_pcm_wideband_hybrid_bits )
		reset_pcm_BH_in_timer();
}
//TODO:
static void reset_pcm_in_tasklet( void )
{
	static uint32 reset_wait = 0;
	uint32 bch;
	int count = 0;

	printk("\n\n%s\n\n", __func__);

	if( reset_wait ++ < 1200 )
		goto label_bus_pcm_reset_done;

	reset_wait = 0;

	// ok. parse every pcm
	for( bch = 0; bch < BUS_PCM_CH_NUM; bch ++ ) {
		voip_bus_t *p_bus = &bus_pcm[ bch ];
		voip_con_t *p_con;
		uint32 bch2;

		if( p_bus ->enabled != 2 )
			continue;

		if( T_[ bch ] == 0 && R_[ bch ] == 0 )
			continue;

		printk( "reset:%d,e:%d,%d%d\n", bch, p_bus ->enabled, !!T_[ bch ], !!R_[ bch ] );

		// reset pcm ch
		bch2 = p_bus ->bus_partner_ptr ->bch;

		p_con = p_bus ->con_ptr;

		p_bus ->bus_ops ->reset_TH( p_bus, T_[ bch ], R_[ bch ] );

		p_con ->con_ops ->bus_fifo_flush( p_con, T_[ bch ], R_[ bch ] );

#if 1
		// do BH in reset_pcm_BH_in_timer()
		p_bus ->reseting = 	( T_[ bch ] ? 0x02 : 0 ) |	// assign on first channel only!
							( R_[ bch ] ? 0x01 : 0 );
#else
		mdelay( 1 );

		p_bus ->bus_ops ->reset_BH( p_bus, T_[ bch ], R_[ bch ] );
#endif
		//printk( "reset:%d,e:%d ok\n", bch, p_bus ->enabled );

		// reset counter
		bch2 = p_bus ->bus_partner_ptr ->bch;

		T_[ bch ] = R_[ bch ] = T_[ bch2 ] = R_[ bch2 ] = 0;
		count ++;
	}

	// if any
	if( count ) {
		reseting_debouncing = 2;
		//isr_schedule_bus_handler();
	}

label_bus_pcm_reset_done:
	;
}
#endif
//Done
void bus_pcm_processor_in_tasklet( void )
{
#ifdef PCM_DBG_SUPPRESSION
	printk_pcm_dbg_in_tasklet();
#endif

#if 1
	if( bus_pcm_wideband_hybrid_bits )
		reset_pcm_in_tasklet();
#endif
}

static void pcmdev_malloc(struct pcm_priv* pcm_dev, unsigned char CH)
{
		/*  BUFFER_SIZE : define the allocated memory size for PCM tx, rx buffer(2 page) */
		/*  pcmdev_malloc() transfer allocated tx, rx buffer to noncache region and map to physical address */

		//pcm_dev->tx_allocated = kmalloc(BUFFER_SIZE, GFP_KERNEL);
		pcm_dev->tx_allocated = pcm_dma[ CH ].tx;

		if(pcm_dev->tx_allocated)
		{
			//pcm_dev->rx_allocated = kmalloc(BUFFER_SIZE, GFP_KERNEL);
			pcm_dev->rx_allocated = pcm_dma[ CH ].rx;

			if(pcm_dev->rx_allocated)
			{
				//assign non-cached address.
				pcm_dev->tx_buffer = (unsigned char *)(pcm_dev->tx_allocated);
				pcm_dev->rx_buffer = (unsigned char *)(pcm_dev->rx_allocated);

				PDBUG("Tx get noncache addr at 0x%X\n", (unsigned int)pcm_dev->tx_buffer);
				PDBUG("Rx get noncache addr at 0x%X\n", (unsigned int)pcm_dev->rx_buffer);
#ifndef G3_TEST
#if BUS_PCM_CH_NUM > 4
				if (CH & 4) // ch 4 ~ 7
				{
					//set TX, Rx buffer
					pcm_outl(CH47TX_BSA(CH), (unsigned int)(Virtual2Physical(pcm_dev->tx_buffer)));
					pcm_outl(CH47RX_BSA(CH), (unsigned int)(Virtual2Physical(pcm_dev->rx_buffer)));
					PDBUG("Virtula2Physical OK!\n");

				}
				else

#endif
				{
					//set TX, Rx buffer
					pcm_outl(TX_BSA(CH), (unsigned int)(Virtual2Physical(pcm_dev->tx_buffer)));
					pcm_outl(RX_BSA(CH), (unsigned int)(Virtual2Physical(pcm_dev->rx_buffer)));
					PDBUG("Virtula2Physical OK!\n");
				}
#endif
				//set all allocated buffer to 0
				memset(pcm_dev->tx_buffer, 0, BUFFER_SIZE);
				memset(pcm_dev->rx_buffer, 0, BUFFER_SIZE);
				PDBUG("Set all allocated buffer to 0 OK!\n");
			}
		}
}

#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM8186

static int pcm_fasync_fun(int fd, struct file *filp, int mode)
{
	struct pcm_priv *pcm_dev;
	pcm_dev = &(pcmctrl_devices[0]);
	return fasync_helper(fd, filp, mode, &(pcm_dev->pcm_fasync));
}

struct file_operations pcmctrl_fops = {
	fasync:         pcm_fasync_fun,
};
#endif

int __init pcmctrl_init(void)
{
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM8651
	int i;
	int ret;
#endif
	int result;
	unsigned char ch = 0;
	char strbuf[NAME_SIZE];
	struct pcm_priv* pcm_dev;

	printk("\n====== RTK PCM Controller Initialization =======\n");

#ifndef HOST_LINUX
	result = audio_driver_load();
#endif


//extern int SLIC_init(int pcm_mode);

//#if defined (AUDIOCODES_VOIP)
	PCM_init();
//#endif

	//for (chid=0; chid<BUS_PCM_CH_NUM; chid++)
	//{
	//	pcm_set_tx_mute(chid, FALSE);
	//	pcm_set_rx_mute(chid, FALSE);
	//}
#ifndef G3_TEST
#ifdef EXTERNEL_CLK
	unsigned long tmp;
	tmp = pcm_inl(0xbd01003c);
	BOOT_MSG("before 0xbd01003c content = %x\n", tmp);
	tmp |= 0x40000;
	pcm_outl(0xbd01003c, tmp);
	tmp = pcm_inl(0xbd01003c);
	BOOT_MSG("after 0xbd01003c content = %x\n", tmp);
	//rtl_outl(0xbd01003c, rtl_inl(0xbd01003c) | 0x40000 );

	//tmp = rtl_inl(0xbd01003c);
#endif

#ifdef DISABLE_PCI
	tmp = pcm_inl(0xbd01003c);
	BOOT_MSG("before PCI disabled, 0xbd01003c content = %x\n", tmp);
	tmp |= 0xC0000000;
	pcm_outl(0xbd01003c, tmp);
	tmp = pcm_inl(0xbd01003c);
	BOOT_MSG("after PCI disabled, 0xbd01003c content = %x\n", tmp);
#endif

#endif

	//pcmctrl_devices = kmalloc((BUS_PCM_CH_NUM) * sizeof(struct pcm_priv), GFP_KERNEL);
#ifdef CONFIG_RTK_VOIP_DRIVERS_PCM8186
	int res = register_chrdev(pcmctrl_major, DEV_NAME, &pcmctrl_fops);

	if(res < 0){
		PERR("Can't register PCM controller devices.");
		return res;
	}
#endif

//========================== Memory Allocated and Request for IRQ =======================//
	if(1)//pcmctrl_devices)
	{
		memset(pcmctrl_devices, 0, (BUS_PCM_CH_NUM) * sizeof(struct pcm_priv));

		pcm_enable();	// pcm must be enable before init SLIC, or it will fail during initialization.

		for (ch = 0; ch <BUS_PCM_CH_NUM; ch++)
		{
			sprintf(strbuf, "pcm%d", ch);
			memcpy(pcmctrl_devices[ch].name, strbuf, NAME_SIZE);
			pcm_dev = &pcmctrl_devices[ch];
			pcmdev_malloc(pcm_dev, ch);


			//====================== Set Compander =======================//
			bus_pcm_set_format( &bus_pcm[ ch ], law );

		}

		//init_var();
		init_var(pcmctrl_devices);

		result = 0;
	}
	else
	{
		printk("Allocate Memory failed. \n");
		result = -ENOMEM;
	}

//******************PCM time slot setup***********//
	pcm_channel_slot(law);  //chiminer modify

	pcm_dev = pcmctrl_devices;
#ifndef CONFIG_RTK_VOIP_PLATFORM_7742
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))
		result = request_irq(PCM_IRQ, pcm_interrupt, IRQF_PERCPU | IRQF_TIMER,
			pcm_dev->name, pcm_dev);// NULL OK
	#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
		result = request_irq(PCM_IRQ, pcm_interrupt, IRQF_DISABLED | IRQF_PERCPU | IRQF_TIMER,
			pcm_dev->name, pcm_dev);// NULL OK
	#else
		result = request_irq(PCM_IRQ, pcm_interrupt, SA_INTERRUPT, pcm_dev->name, pcm_dev);// NULL OK
	#endif
#endif
		if(result)
		{
			printk("Can't request IRQ for channel %d.\n", ch);
			//PERR(KERN_ERR "Can't request IRQ for channel %d.\n", ch);
			return result;

		}
		printk("Request IRQ for PCM Controller OK!.\n");

#if defined(CONFIG_RTK_VOIP_DRIVERS_PCM8651) && !defined(CONFIG_RTK_VOIP_RX)
	for( i = 0 ; i < BUS_PCM_CH_NUM ; i++ )
	{
		toRegister[i].msTick = 10000; /* tick service is triggered every 100ms. */
		toRegister[i].msRemain = 0;
		toRegister[i].chid = i;

		toRegister[i].fpTick  = NULL; /*tick*/
		toRegister[i].fpPktRx = DSP_pktRx; /* packet recv */
		toRegister[i].fpPcmInt= pcm_ISR; /* pcm interrupt */
		//toRegister[i].fpPcmInt = NULL;
		ret = voip_register_callback( i, &toRegister[i] );
		printk("ROME driver register result ch[%d]: %d\n",i,ret);
	}
	printk("Registered into ROME voip_support\n");
#endif

	print_pcm();

// For OC 8186 EVB, switch Relay to SLIC
#if defined (CONFIG_RTK_VOIP_DRIVERS_PCM8186V_OC) || defined (CONFIG_RTK_VOIP_DRIVERS_PCM8651_2S_OC)
	Legerity_GPIO_dir_set(0, 1, 1);
	Legerity_GPIO_dir_set(1, 1, 1);
	Legerity_GPIO_data(0, 1, 1);
	Legerity_GPIO_data(1, 1, 1);
#endif

	printk("================= FISISH ===============\n");

#ifdef PCM_ISR_PERIOD_TEST

#if 0	//Let PCM CLK at wrong state
	FIBER_MODE;
	mdelay(50);
	UTP_MODE;
	mdelay(50);
	FIBER_MODE;
	mdelay(50);
	printk("-->0x%x\n", FIBER_MODE_R);
#endif

#if 1	//Check PCM CLK
	int chk_ret;
	chk_ret = PcmClkCheck();
	if (chk_ret == 1)
		PRINT_Y("PCM CLK Pass!\n");
	else if (chk_ret == -1)
	{
		PRINT_R("PCM CLK Fail!\n");

#if 0	//Fix PCM CLK, and check again
		PRINT_G("Recovery it...");
		UTP_MODE;
		mdelay(50);
		FIBER_MODE;
		PRINT_G("Done!\n");

		mdelay(50);
		PcmClkCheck();
#endif
	}
	else if (chk_ret == 0)
		PRINT_R("PCM CLK Unknow!\n");
#endif
#endif	//PCM_ISR_PERIOD_TEST

	int init_ret;
	init_ret = audio_if_init(BUS_PCM_CH_NUM, (void *)pTxBuf, (void *)pRxBuf);
	if (init_ret)
		printk("Audio if init failed: %d\n", init_ret);

	return result;

}

static void __exit pcmctrl_cleanup(void)
{
	pcm_disable();
	//kfree(pcmctrl_devices);
}

static void print_pcm(void)	// thlin+ for debug
{

}



static void PCM_init(void)
{
	unsigned long flags;
	uint32 chid;

#if defined (AUDIOCODES_VOTING_MECHANISM)
	int nPeriod, reg_pbsize, i;
#endif

	save_flags(flags); cli();

#ifndef AUDIOCODES_VOIP
#ifdef SUPPORT_AES_ISR
	Init_AES();
#endif
#endif


        restore_flags(flags);

        //print_pcm();
        //return;
}

static void __exit PCM_shutdown(void)
{
	uint32 chid;

	for (chid=0; chid<BUS_PCM_CH_NUM; chid++)
	{
		pcm_disableChan(chid);
	}

	kill_bus_handler();

}

static void pcm_band_select( uint32 bch1, uint32 bch2, int bandfactor )
{
	// narrowband: bandfactor == 1, wideband: bandfactor == 2
#ifndef G3_TEST
#if PCM_VERSION == 0x0400
	uint32 bits;

#if BUS_PCM_CH_NUM > 4
	if (bch1 & 4) {/* chid 4~7 */
		bits =	BIT((8*(MAXCH-(bch1&3)-1)+3)) |
				BIT((8*(MAXCH-(bch2&3)-1)+3));

		if( bandfactor == 2 )
			pcm_outl(PCMCHCNR47, pcm_inl(PCMCHCNR47)|bits);
		else
			pcm_outl(PCMCHCNR47, pcm_inl(PCMCHCNR47)&~bits);
	} else
#endif
	{
		bits =	BIT((8*(MAXCH-(bch1&3)-1)+3)) |
				BIT((8*(MAXCH-(bch2&3)-1)+3));

		if( bandfactor == 2 )
			pcm_outl(PCMCHCNR, pcm_inl(PCMCHCNR)|bits);
		else
			pcm_outl(PCMCHCNR, pcm_inl(PCMCHCNR)&~bits);
	}

#endif // PCM_VERSION == 0x0400
#endif
}

#if !defined (AUDIOCODES_VOTING_MECHANISM)
//Done
static void pcm_enableChan( uint32 bch1, uint32 bch2, int bandfactor )
{
	int reg_pbsize;
	Word16 nPeriod;
	int		i;
	//voip_bus_t * const p_bus = &bus_pcm[ chid ];
	//voip_con_t * const p_con = p_bus ->con_ptr;

	// chan id from 0 ~ (VOIP_CH_NUM - 1)
	if( bch1 >= BUS_PCM_CH_NUM || bch2 >= BUS_PCM_CH_NUM )
		return;
	if( chanEnabled[ bch1 ] == TRUE && chanEnabled[ bch2 ] == TRUE )
		return;

	// only real enable
	T_[ bch1 ] = T_[ bch2 ] = R_[ bch1 ] = R_[ bch2 ] = 0;

#if 0//def CONFIG_RTK_VOIP_DRIVERS_IIS
	if (chid == 0) {
		extern void iis_enableChan(unsigned int chid);
		iis_enableChan(chid);
		return;
	}
#endif
	txpage[ bch1 ] = txpage[ bch2 ] =0;
	rxpage[ bch1 ] = rxpage[ bch2 ] = 0;

	// move to dsp_ops ->enable
	//p_con ->con_ops ->handle_event_bus_enable( p_con, 1 /* enable */ );

#ifdef PCM_PERIOD
	nPeriod = 10*PCM_PERIOD; // 10ms * PCM_PERIOD
#else
#ifdef SUPPORT_CUT_DSP_PROCESS
	nPeriod = 10; //10ms
#else
        extern Word16 nPcmIntPeriod[];
	nPeriod = nPcmIntPeriod[chid];
#endif
#endif
	reg_pbsize = nPeriod * 8 * 2 * bandfactor;		// 8 samples/ms * 2 bytes/sample * bandfactor
	pcm_set_page_size( bch1, reg_pbsize);
	pcm_set_page_size( bch2, reg_pbsize);
	//pcm_get_page_size(chid);
#if 1
	for(i=0; i<2; i++)
	{
		pcm_set_tx_own_bit( bch1, i);
		pcm_set_rx_own_bit( bch1, i);
		pcm_set_tx_own_bit( bch2, i);
		pcm_set_rx_own_bit( bch2, i);
	}
#endif

	pcm_band_select( bch1, bch2, bandfactor );

	if( reg_pbsize * 2 > BUFFER_SIZE ) {
		PRINT_R( "PCM(%d,%d) page size is too large %d > %d.\n",
					bch1, bch2, reg_pbsize * 2, BUFFER_SIZE );
	}

#if 1	// clean TX data
	memset( pcm_dma[ bch1 ].tx , 0, BUFFER_SIZE );
	if( bch1 != bch2 )
		memset( pcm_dma[ bch2 ].tx , 0, BUFFER_SIZE );
#endif

	chanEnabled[ bch1 ] = chanEnabled[ bch2 ] = TRUE;
	EnaPcmIntr( bch1 );
	EnaPcmIntr( bch2 );
	pcm_tx_rx_enable( bch1, bch2 );

	PRINT_MSG("PCM ch%d,%d enable\n", bch1, bch2 );

}
#else //defined (AUDIOCODES_VOTING_MECHANISM)



static void pcm_enableChan(unsigned int chid)
{
	unsigned long ac_isr = 0,ac_imr = 0,ac_chnr=0,j,voting;
	int reg_pbsize;
	Word16 nPeriod;
	int		i;

	// chan id from 0 ~ (VOIP_CH_NUM - 1)
	if(chid >= BUS_PCM_CH_NUM)
		return;

	if(chanEnabled[chid] == TRUE)
		return;

#ifdef SUPPORT_PCM_FIFO
	PCM_FIFO_Init(chid);
#endif


	chanEnabled[chid] = TRUE;
	voting = AC_PCM_VOTING_GET();
	AC_PCM_VOTING_CH_ON(chid);
	if (voting != 0) return;

	nPeriod = 10*PCM_PERIOD; // 10ms * PCM_PERIOD
	reg_pbsize = nPeriod * 8 * 2;		// 8 samples/ms * 2 bytes/sample
	for(j=0; j<BUS_PCM_CH_NUM; j++)
	{
		pcm_set_page_size(j, reg_pbsize);
		for(i=0; i<2; i++)
		{
			pcm_set_tx_own_bit(j, i);
			pcm_set_rx_own_bit(j, i);
		}
		txpage[j] = 0;
		rxpage[j] = 0;
	}

#ifndef G3_TEST
	for(i=0; i<BUS_PCM_CH_NUM; i++)
	{
		ac_chnr |= AC_PCM_CHCNR_CTLWRD_RX_TX(i);
		ac_imr |=(	AC_PCM_IMR_CTLWRD_P0OK(i)|
				AC_PCM_IMR_CTLWRD_P1OK(i)|
				AC_PCM_IMR_CTLWRD_TBUA(i)|
				AC_PCM_IMR_CTLWRD_RBUA(i));
		ac_isr |=AC_PCM_ISR_CTLWRD_RESET_CH(i);
	}
	//pcm_tx_rx_enable
	AC_PCM_CHCNR_WRITE(AC_PCM_CHCNR_READ()|(ac_chnr));

	//EnaPcmIntr
	AC_PCM_ISR_WRITE(ac_isr);
	AC_PCM_IMR_WRITE(AC_PCM_IMR_READ()|(ac_imr));
#endif
}
#endif

#if !defined (AUDIOCODES_VOTING_MECHANISM)
static void pcm_disableChan(unsigned int chid)
{
	if(chanEnabled[chid] == FALSE)
		return;

	// chan id from 0 ~ (VOIP_CH_NUM - 1)
	if(chid >= BUS_PCM_CH_NUM)
		return;

#if 0//def CONFIG_RTK_VOIP_DRIVERS_IIS
	if (chid == 0) {
		extern void iis_disableChan(unsigned int chid);
		iis_disableChan(chid);
		chanEnabled[chid] = FALSE;
		return;
	}
#endif

	pcm_rx_disable(chid);
	pcm_tx_disable(chid);

	DisPcmIntr(chid);

	chanEnabled[chid] = FALSE;
	PRINT_MSG("PCM ch%d disable\n", chid);
}
#else
static void pcm_disableChan(unsigned int chid)
{
	unsigned long ac_isr = 0,ac_imr = 0,ac_chnr=0,i,j;

	if (AC_PCM_VOTING_GET() == 0)
	{
		return; //already all off;
	}

	AC_PCM_VOTING_CH_OFF(chid);
#ifndef G3_TEST
	if (AC_PCM_VOTING_GET() == 0)
	{
		ac_imr = AC_PCM_IMR_READ();
		for(i=0; i<BUS_PCM_CH_NUM; i++)
		{
			ac_imr |=(	AC_PCM_IMR_CTLWRD_P0OK(i)|
						AC_PCM_IMR_CTLWRD_P1OK(i)|
						AC_PCM_IMR_CTLWRD_TBUA(i)|
						AC_PCM_IMR_CTLWRD_RBUA(i));
			ac_isr |=AC_PCM_ISR_CTLWRD_RESET_CH(i);
		}
		AC_PCM_IMR_WRITE(AC_PCM_IMR_READ()&(~(ac_imr)));
		AC_PCM_ISR_WRITE(ac_isr);
	}

	if (AC_PCM_VOTING_GET() == 0)
	{
		for(i=0; i<BUS_PCM_CH_NUM; i++) ac_chnr |= AC_PCM_CHCNR_CTLWRD_RX(i);
		AC_PCM_CHCNR_WRITE(AC_PCM_CHCNR_READ()&(~(ac_chnr)));
		ac_chnr = 0;
		for(i=0; i<BUS_PCM_CH_NUM; i++) ac_chnr |= AC_PCM_CHCNR_CTLWRD_TX(i);
		AC_PCM_CHCNR_WRITE(AC_PCM_CHCNR_READ()&(~(ac_chnr)));
	}
#endif
	chanEnabled[chid] = FALSE;
}
#endif
//Done
static void PCM_restart( uint32 bch1, uint32 bch2, int bandfactor )
{
#if ! defined (AUDIOCODES_VOTING_MECHANISM)
	txpage[ bch1 ] = txpage[ bch2 ] = 0;
	rxpage[ bch1 ] = rxpage[ bch2 ] = 0;
#endif
	tr_cnt[ bch1 ] = tr_cnt[ bch2 ] = 0;
	pcm_enableChan( bch1, bch2, bandfactor );

}



static void pcm_channel_slot(int pcm_mode)
{
#ifndef G3_TEST
	switch(pcm_mode)
	{
		case BUSDATFMT_PCM_LINEAR:
		case BUSDATFMT_PCM_ALAW:
		case BUSDATFMT_PCM_ULAW:
		case BUSDATFMT_PCM_WIDEBAND_LINEAR:
		case BUSDATFMT_PCM_WIDEBAND_ALAW:
		case BUSDATFMT_PCM_WIDEBAND_ULAW:
			pcm_outl(PCMTSR,0x00020406); //ch0:slot0 ,ch1:slot2 ,ch2:slot4 ,ch3:slot6
#if BUS_PCM_CH_NUM > 4
			pcm_outl(PCMTSR47,0x080a0c0e); //ch4:slot8 ,ch5:slot10 ,ch6:slot12 ,ch7:slot14
#endif
			break;

#if 0
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_SILAB
			pcm_outl(PCMTSR,0x00021012); //ch0:slot0 + ch2:slot16, ch1:slot2 + ch3:slot18
#if BUS_PCM_CH_NUM > 4
			pcm_outl(PCMTSR47,0x04061416); //ch4:slot4 + ch6:slot20, ch5:slot6 + ch7:slot22
#endif
#endif
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_ZARLINK
			pcm_outl(PCMTSR,0x00020406); //ch0:slot0 + ch2:slot2, ch1:slot4 + ch3:slot6
#if BUS_PCM_CH_NUM > 4
			pcm_outl(PCMTSR47,0x080a0c0e); //ch4:slot8 + ch6:slot10, ch5:slot12 + ch7:slot14
#endif
#endif
#endif
			break;

		default:
			pcm_outl(PCMTSR,0x00020406);
#if BUS_PCM_CH_NUM > 4
			pcm_outl(PCMTSR47,0x080a0c0e);
#endif
			PRINT_R("Error pcm mode in %s, line%d\n", __FUNCTION__, __LINE__);
			break;

	}
#endif

	return;
}

static int bus_pcm_enable( voip_bus_t *this, int enable )
{
	//printk("\n\n%s: Enter(bch %d, enable %d)\n\n", __func__, this->bch, enable);
	if (enable == 2)
		audio_channel_wideband(this->bch, 1);
	else
		audio_channel_wideband(this->bch, 0);

	return 0;

	voip_bus_t * const p_bus_partner =
		( enable == 2 || enable == 0 ? this ->bus_partner_ptr : NULL );
	const uint32 bch2 =
		( p_bus_partner ? p_bus_partner ->bch : this ->bch );
	const int bandfactor = ( p_bus_partner ? 1 : enable );

	// wideband bits mask
	if( enable == 2 && p_bus_partner ) {
		bus_pcm_wideband_hybrid_bits |=
					( ( 1 << this ->bch  ) | ( 1 << bch2 ) );
	} else if( enable == 0 ) {
		bus_pcm_wideband_hybrid_bits &=
					~( ( 1 << this ->bch  ) | ( 1 << bch2 ) );
	}

	// assign role
	this ->role_var = ( enable == 2 ? this ->role_bind : BUS_ROLE_SINGLE );
	//if( p_bus_partner )	// binding decide it!
	//	p_bus_partner ->role_var = p_bus_partner ->role_bind;

	if( enable ) {
		PCM_restart( this ->bch, bch2, bandfactor );
	} else {
		pcm_disableChan( this ->bch );
		if( p_bus_partner )
			pcm_disableChan( bch2 );
	}

	return 0;
}

static int bus_pcm_reset_TH( voip_bus_t *this, int tx, int rx )
{
	// turn off interrupt
	const uint32 bch1 = this ->bch;
	const uint32 bch2 = ( this ->enabled == 2 ? this ->bus_partner_ptr ->bch : this ->bch );
	int i;

	printk("\n\n%s: Enter(%d, %d)\n\n", __func__, tx, rx);	return 0;

	if( rx && tx ) {
		//DisPcmIntr( bch1 );
		//DisPcmIntr( bch2 );
	}

	if( rx ) {
		pcm_rx_disable( bch1 );
		pcm_rx_disable( bch2 );
	}

	if( tx ) {
		pcm_tx_disable( bch1 );
		pcm_tx_disable( bch2 );
	}

	//mdelay( 10 );

	// clean PCM_ISR
	pcm_isr_reset_tx_rx( bch1, tx, rx );
	pcm_isr_reset_tx_rx( bch2, tx, rx );

	// reset software variables (reference pcm_enableChan())
	if( tx )
		txpage[ bch1 ] = txpage[ bch2 ] = 0;

	if( rx )
		rxpage[ bch1 ] = rxpage[ bch2 ] = 0;

	for(i=0; i<2; i++)
	{
		if( tx ) {
			pcm_set_tx_own_bit( bch1, i);
			pcm_set_tx_own_bit( bch2, i);
		}

		if( rx ) {
			pcm_set_rx_own_bit( bch1, i);
			pcm_set_rx_own_bit( bch2, i);
		}
	}

	return 0;
}

static int bus_pcm_reset_BH( voip_bus_t *this, int tx, int rx )
{
	const uint32 bch1 = this ->bch;
	const uint32 bch2 = ( this ->enabled == 2 ? this ->bus_partner_ptr ->bch : this ->bch );

	printk("\n\n%s: Enter(%d, %d)\n\n", __func__, tx, rx);	return 0;

	//int i;

	// IMR
	//EnaPcmIntr( bch1 );
	//EnaPcmIntr( bch2 );

	// turn on tx/rx
	pcm_tx_rx_enable( bch1, bch2 );

	return 0;
}

static void bus_pcm_set_timeslot( voip_bus_t *this, uint32 ts1, uint32 ts2 )
{
	printk("\n\n%s: Enter\n\n", __func__);

	this ->TS1_var = ts1;
	this ->TS2_var = ts2;
}

static void bus_pcm_set_format( voip_bus_t *this, uint32 _format )
{
	printk("\n\n%s: Enter\n\n", __func__);
}

const bus_ops_t bus_pcm_ops = {
	// common operation
	.enable = bus_pcm_enable,
	.reset_TH = bus_pcm_reset_TH,
	.reset_BH = bus_pcm_reset_BH,

	.set_timeslot = bus_pcm_set_timeslot,
	.set_format = bus_pcm_set_format,
};


int __init voip_bus_pcm_init( void )
{
	//extern int __init pcmctrl_init( void );
	//extern const bus_ops_t bus_pcm_ops;
	int i;

	for (i = 0; i < BUS_PCM_CH_NUM; i ++) {
		bus_pcm[ i ].bch = i;
		bus_pcm[ i ].name = "pcm";
		bus_pcm[ i ].bus_type = BUS_TYPE_PCM;
		bus_pcm[ i ].band_mode_sup = BAND_MODE_8K | BAND_MODE_16K;
		bus_pcm[ i ].bus_group = (i < 4 ? BUS_GROUP_PCM03 : BUS_GROUP_PCM47);	/* CA_FIXME */
		bus_pcm[ i ].bus_ops = &bus_pcm_ops;
		bus_pcm[ i ].priv = &pcmctrl_devices[i];
	}

	register_voip_bus(bus_pcm, BUS_PCM_CH_NUM);

	pcmctrl_init();

	return 0;
}

static void __exit voip_bus_pcm_exit( void )
{
	PCM_shutdown();
	pcmctrl_cleanup();
}


voip_initcall_bus( voip_bus_pcm_init );
voip_exitcall( voip_bus_pcm_exit );

// --------------------------------------------------------
// proc
// --------------------------------------------------------
int __init voip_pcm_proc_init( void )
{
	return 0;
}

void __exit voip_pcm_proc_exit( void )
{

}

voip_initcall_proc( voip_pcm_proc_init );
voip_exitcall( voip_pcm_proc_exit );

