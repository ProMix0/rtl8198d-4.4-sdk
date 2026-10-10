#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/bitops.h>
#include "ca_snd_audio.h"
#define MAX_SW_CH_NUM	8

struct audio_wrapper_dev {
	void *hw_dev;
	int chan_num;

	spinlock_t lock;

	int dma_chan;
	unsigned int valid_chan_mask;
	int oneshot_symbol_num;
	int symbol_size;

	int ssp_en;

	/* sw DSP resource */
	void **sw_chan_txbuf;
	void **sw_chan_rxbuf;

	int sw_chan_wb[MAX_SW_CH_NUM];
	int wideband;
};

static struct audio_wrapper_dev audio_if;

#define IF_PCM		0
#define IF_ISI		1
#define IF_ZSI		2

#define CF_MASTER	0
#define CF_SLAVE	1

#define LB_NO		0
#define LB_INTERNAL	1
#define LB_EXTERNAL	2

#define OP_DSP		0
#define	OP_LOOPBACK	1
#define OP_TALK		2
#define OP_ALONE	3

#define PCM_SLOT_SIZE	8
#define NB_SAMPLE_RATE	8000
#define WB_SAMPLE_RATE	16000

#if defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_ISI)
	unsigned int ifmode = 1;
#elif defined(CONFIG_RTK_VOIP_DRIVERS_SLIC_ZSI)
	unsigned int ifmode = 2;
#else
	unsigned int ifmode = 0;	/* PCM */
#endif
#if defined(CONFIG_RTK_VOIP_SLIC_PCLK_4096)
        unsigned int pclk = 4096000;
#elif defined(CONFIG_RTK_VOIP_SLIC_PCLK_3072)
        unsigned int pclk = 3072000;
#else
        unsigned int pclk = 2048000;
#endif

unsigned int dev_spi_cs = 0;
static unsigned int cfrole = CF_MASTER;	/* 0: master, 1: slave */
static unsigned int loopback = LB_NO;
static unsigned int opmode = OP_DSP;
static unsigned int rslots;

static unsigned int dev_chan_num = 2;
static unsigned int dev_chan_size = 2;
static unsigned int dev_chan_wb = 1;

#define DBG_10MS_INTR	0x00000001
#define DBG_TX_NOTIFY	0x00000002
#define DBG_RX_NOTIFY	0x00000004

static unsigned int dbg_flag = 0; 

void pcm_tx_notify(void *data)
{
	int i, j, ch;
	int16_t *tx_buf16 = NULL;
	int8_t *tx_buf8 = NULL;
	int hw_chan_num = audio_if.dma_chan;
	int sw_chan_num = audio_if.chan_num;
	unsigned int chan_mask = audio_if.valid_chan_mask;
	int symbol_10ms = audio_if.oneshot_symbol_num;
	int symbol_size = audio_if.symbol_size;
	int16_t *pcmTxBuf[sw_chan_num];

	if (opmode != OP_DSP)	/* tx data handled by pcm_rx_notify() */
		goto IGNORE_TX;

	if (audio_if.wideband)
		hw_chan_num *= 2;

	tx_buf16 = (int16_t *)ca77xx_snd_audio_buf_req(audio_if.hw_dev, DIR_TX, 0);
	tx_buf8 = (int8_t *)tx_buf16;
	for (i = 0; i < sw_chan_num; i++) {
		pcmTxBuf[i] = (int16_t *)audio_if.sw_chan_txbuf[i];
		//printk("pcmTxBuf[%d] %p \n", i, pcmTxBuf[i]);
	}

	if (symbol_size == 2) {
		for (i = 0; i < symbol_10ms; i++) {
			ch = 0;
			for (j = 0; j < hw_chan_num; j++) {
				if (chan_mask & BIT(j)) {
					if ((ch / sw_chan_num) == 0) {
						*tx_buf16 = *(pcmTxBuf[ch % sw_chan_num]++);
					} else {
						if (audio_if.sw_chan_wb[ch % sw_chan_num]) {
							*tx_buf16 = *(pcmTxBuf[ch % sw_chan_num]++);
						}
					}
					ch++;
				}
				tx_buf16++;
			}
		}
	} else {
		/* CA_FIXME */
 	}

IGNORE_TX:
	return;
}

void pcm_rx_notify(void *data)
{
	int i, j, ch;
	int16_t *rx_buf16 = NULL;
	int16_t *tx_buf16 = NULL;
	int8_t *rx_buf8 = NULL;
	int8_t *tx_buf8 = NULL;
	int hw_chan_num = audio_if.dma_chan;
	int sw_chan_num = audio_if.chan_num;
	unsigned int chan_mask = audio_if.valid_chan_mask;
	int symbol_10ms = audio_if.oneshot_symbol_num; /*160*/
	int symbol_size = audio_if.symbol_size;
	int16_t *pcmRxBuf[sw_chan_num];

	if (opmode == OP_ALONE)
		goto IGNORE_RX;

	if (opmode == OP_LOOPBACK) {
		tx_buf16 = (int16_t *)ca77xx_snd_audio_buf_req(audio_if.hw_dev, DIR_TX, 200); /* 200 shot later */
	} else if (opmode == OP_TALK) {
		tx_buf16 = (int16_t *)ca77xx_snd_audio_buf_req(audio_if.hw_dev, DIR_TX, 0); /* current */
	}

	if (audio_if.wideband)
		hw_chan_num *= 2;

	tx_buf8 = (int8_t *)tx_buf16;

	rx_buf16 = (int16_t *)ca77xx_snd_audio_buf_req(audio_if.hw_dev, DIR_RX, 0);
	rx_buf8 = (int8_t *)rx_buf16;
	for (i = 0; i < sw_chan_num; i++) 
		pcmRxBuf[i] = (int16_t *)audio_if.sw_chan_rxbuf[i];

	if (symbol_size == 2) {
		for (i = 0; i < symbol_10ms; i++) {
			ch = 0;

			for (j = 0; j < hw_chan_num; j++) {

				if (chan_mask & BIT(j)) {
					if (opmode == OP_LOOPBACK) {
						*(tx_buf16) = *(rx_buf16);
					} else if (opmode == OP_TALK) {
						if (ch == 0)
							*(tx_buf16 + 1) = *(rx_buf16);
						else if(ch == 1)
							*(tx_buf16 - 1) = *(rx_buf16);
					} else {
						if ((ch / sw_chan_num) == 0) {
							*(pcmRxBuf[ch % sw_chan_num]++) = *(rx_buf16);
						} else {
							if (audio_if.sw_chan_wb[ch % sw_chan_num])
								*(pcmRxBuf[ch % sw_chan_num]++) = *(rx_buf16);
						}
					}
					ch++;
				}
				rx_buf16++;
				tx_buf16++;
			}
		}
	} else {
		/* CA_FIXME */
	}

IGNORE_RX:
	return;
}

/******************************************************************************/
int audio_channel_wideband(int chan, int enable)
{
	if ((!audio_if.wideband) && enable)
		return -1;

	audio_if.sw_chan_wb[chan] = enable ? 1 : 0;

	return 0;
}

int audio_isr_handler(void)
{
	return ca77xx_audio_dma_int_dispatch(audio_if.hw_dev);
}

int audio_tx_data(void)
{
	return ca77xx_audio_tx_data(audio_if.hw_dev);
}

int audio_rx_data(void)
{
	return ca77xx_audio_rx_data(audio_if.hw_dev);
}
int audio_dma_tx_update(void)
{
  return ca77xx_audio_dma_tx_update_wt_idx(audio_if.hw_dev);
}
int audio_dma_rx_update(void)
{
  return ca77xx_audio_dma_rx_update_rd_idx(audio_if.hw_dev);
}
u32 audio_rd_dma_reg(unsigned int reg)
{
   return audio_rd_dma_ssp_reg(audio_if.hw_dev, reg);
}
u32 audio_get_dma_rxbuf_size(u32 rx_ptr, u32 wt_ptr)
{
   return ca77xx_audio_get_dma_rxbuf_size(audio_if.hw_dev,rx_ptr,wt_ptr);
}
int audio_if_start(void)
{
	ca77xx_snd_audio_start(audio_if.hw_dev, DIR_RX);
	ca77xx_snd_audio_start(audio_if.hw_dev, DIR_TX);

	return 0;
}

int audio_if_stop(void)
{
	ca77xx_snd_audio_stop(audio_if.hw_dev, DIR_TX);
	ca77xx_snd_audio_stop(audio_if.hw_dev, DIR_RX);

	return 0;
}

int audio_if_init(int chan_num, void *pTxBuf[], void *pRxBuf[])
{
	int chan_size = dev_chan_size;
	int spi_cs = dev_spi_cs;
	int op = OP_DSP;

	struct ca77xx_audio_cfg cfg;
	int max_chan, inner_chan_num;
	int oneshot_num, oneshot_size;
	int buf_size, endian, slots, width;
	unsigned int signal_fmt, tx_vld_mask[4], rx_vld_mask[4];
	int reserved_slot = 2;
	int i, j;

	slots = pclk / (8000 * 8);
	/* FSYNC must be provided to Proslic before it can be accessed -
	 * SPORT_CLOCK_CONT
	 */
	signal_fmt = AUDIO_DATA_DSP_A | AUDIO_CLOCK_GATED |
		     AUDIO_SIGNAL_IB_IF;
	if ((cfrole == CF_MASTER) || (ifmode == IF_ISI) || (ifmode == IF_ZSI))
		signal_fmt |= AUDIO_CLOCK_CBM_CFM;
	else
		signal_fmt |= AUDIO_CLOCK_CBS_CFS;
	width = PCM_SLOT_SIZE;
	endian = 0;

	/* 10ms */
	audio_if.symbol_size = chan_size;
	audio_if.oneshot_symbol_num = (8000 / 100);  //10ms
	buf_size = audio_if.symbol_size * audio_if.oneshot_symbol_num * (dev_chan_wb ? 2 : 1);

	max_chan = slots / audio_if.symbol_size;

	for (inner_chan_num = chan_num; inner_chan_num < max_chan;
	     inner_chan_num++) {
		/* Trigger DMA interrupt every 10ms to provide each channel's
		 * whole 10ms sample data. DMA data block is 128 byte alignment.
		 */
		if (((inner_chan_num * buf_size) % 128) == 0)
			break;
	}

	if (inner_chan_num >= max_chan)
		return -EPERM;

	audio_if.dma_chan = inner_chan_num;

	oneshot_num = MAX_BUFFER_DURATION / 10;
	oneshot_size = audio_if.dma_chan * buf_size;

	for (i = 0; i < chan_num; i++) {
		audio_if.valid_chan_mask |= BIT(i);

		if (dev_chan_wb)
			audio_if.valid_chan_mask |= BIT(i + inner_chan_num);
	}
	audio_if.wideband = dev_chan_wb;

	for (i = 0; i < 4; i++) {
		tx_vld_mask[i] = 0;
		rx_vld_mask[i] = 0;
	}

	if (ifmode == IF_ZSI)
		reserved_slot = 2;
	if (rslots)
		reserved_slot = rslots;

	for (i = 0; i < audio_if.dma_chan * audio_if.symbol_size; i++) {
		j = i + reserved_slot;
		tx_vld_mask[j/32] |= BIT(j%32);
		rx_vld_mask[j/32] |= BIT(j%32);

		if (dev_chan_wb) {
			tx_vld_mask[(j + slots/2)/32] |= BIT((j + slots/2)%32);
			rx_vld_mask[(j + slots/2)/32] |= BIT((j + slots/2)%32);
		}
	}

	/**********************************************************************/
	cfg.idx = 0;
	cfg.type = CA77XX_AUDIO_PCM;
	cfg.dir = DIR_BOTH;

	if (ifmode == IF_PCM)
		cfg.flag = 0;
	else if (ifmode == IF_ISI)
		cfg.flag = FLAG_ISI;
	else if (ifmode == IF_ZSI)
		cfg.flag = FLAG_ZSI;
	if (loopback == LB_INTERNAL)
		cfg.flag |= FLAG_INT_LB;
	cfg.spi_cs = spi_cs;

	audio_if.hw_dev = ca77xx_snd_audio_if_register(&cfg);
	if (!audio_if.hw_dev)
		return -ENODEV;

	if (ca77xx_snd_audio_clkcfg(audio_if.hw_dev, pclk))
		return -EINVAL;

	if (ca77xx_snd_audio_slots(audio_if.hw_dev, tx_vld_mask, rx_vld_mask,
				   slots, width))
		return -EINVAL;

	if (ca77xx_snd_audio_signal_formt(audio_if.hw_dev, signal_fmt))
		return -EINVAL;

	ca77xx_snd_audio_data_formt(audio_if.hw_dev, DIR_TX,
				    audio_if.symbol_size, endian);
	ca77xx_snd_audio_data_formt(audio_if.hw_dev, DIR_RX,
				    audio_if.symbol_size, endian);

	ca77xx_snd_audio_dma_register(audio_if.hw_dev, DIR_TX, oneshot_num,
				      oneshot_size);
	ca77xx_snd_audio_dma_register(audio_if.hw_dev, DIR_RX, oneshot_num,
				      oneshot_size);

	ca77xx_snd_audio_dma_callback(audio_if.hw_dev, DIR_TX,
				      pcm_tx_notify,
				      (void *)0x76543210);
	ca77xx_snd_audio_dma_callback(audio_if.hw_dev, DIR_RX,
				      pcm_rx_notify,
				      (void *)0x89ABCDEF);

	audio_if.chan_num = chan_num;
	audio_if.sw_chan_txbuf = pTxBuf;
	audio_if.sw_chan_rxbuf = pRxBuf;

	return 0;
}

int audio_if_exit(void)
{
	ca77xx_snd_audio_dma_unregister(audio_if.hw_dev, DIR_TX);
	ca77xx_snd_audio_dma_unregister(audio_if.hw_dev, DIR_RX);

	ca77xx_snd_audio_if_unregister(audio_if.hw_dev);

	return 0;
}

#ifndef HOST_LINUX
int audio_driver_load(void)
{
	return ca77xx_audio_probe();
}
#endif

#ifdef HOST_LINUX
#if 0
int init_module(void)
{
	memset(&audio_if, 0, sizeof(struct audio_wrapper_dev));

	spin_lock_init(&audio_if.lock);

	audio_if_init(dev_chan_num, dev_chan_size, IF_PCM, 0);

	return 0;
}

void cleanup_module(void)
{
	audio_if_exit();

	memset(&audio_if, 0, sizeof(struct audio_wrapper_dev));
}
#endif

module_param_named(pclk, pclk, uint, 0644);
module_param_named(ifmode, ifmode, uint, 0644);
MODULE_PARM_DESC(ifmode, "mode selection(0:PCM, 1:ISI, 2:ZSI)\n");
module_param_named(cfrole, cfrole, uint, 0644);
MODULE_PARM_DESC(cfrole, "clock frame role(0:master, 1:slave\n");
module_param_named(loopback, loopback, uint, 0644);
MODULE_PARM_DESC(loopback, "loopback(0:no, 1:internal, 2:external\n");
module_param_named(opmode, opmode, uint, 0644);
module_param_named(rslots, rslots, uint, 0644);
MODULE_PARM_DESC(rslots, "reserved slot number (reserved 0 ~ rslots-1 slots\n");

module_param_named(chan_num, dev_chan_num, uint, 0644);
MODULE_PARM_DESC(chan_num, "device channel number\n");
module_param_named(chan_size, dev_chan_size, uint, 0644);
MODULE_PARM_DESC(chan_size, "device channel size\n");
module_param_named(chan_wb, dev_chan_wb, uint, 0644);
MODULE_PARM_DESC(chan_wb, "device channel wideband\n");

#if 0
module_param_named(debug, dbg_flag, uint, 0644);
MODULE_PARM_DESC(debug, "debug flag\n");
#endif

MODULE_LICENSE("GPL");
#endif
