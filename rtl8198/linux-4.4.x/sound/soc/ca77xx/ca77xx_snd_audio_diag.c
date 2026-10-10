#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/bitops.h>
#include <sound/ca77xx_snd_audio.h>

struct audio_diag_dev {
	void *hw_dev;

	spinlock_t lock;

	void (*tx_handler)(void *data);
	void (*rx_handler)(void *data);

	int dma_chan;
	unsigned int valid_chan_mask;
	int oneshot_symbol_num;
	int symbol_size;

	int ssp_en;
};

struct audio_diag_dev audio_diag;

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

#define MAX_BUFFER_DURATION 1280	/* ms */

static unsigned int pclk = 2048000;
static unsigned int ifmode = IF_PCM;	/* 0: PCM, 1: ISI, 2: ZSI */
static unsigned int cfrole = CF_MASTER;	/* 0: master, 1: slave */
static unsigned int loopback = LB_NO;
static unsigned int opmode = OP_ALONE;
static unsigned int rslots;

static unsigned int dev_chan_num = 2;
static unsigned int dev_chan_size = 2;
static unsigned int dev_chan_wb = 0;

#define DBG_10MS_INTR	0x00000001
#define DBG_TX_NOTIFY	0x00000002
#define DBG_RX_NOTIFY	0x00000004

static unsigned int dbg_flag = 0;

void pcm_tx_notify(void *data)
{
	int i, j, ch;
	int16_t *tx_buf16 = NULL;
	int8_t *tx_buf8 = NULL;
	int total_chan = audio_diag.dma_chan;
	unsigned int chan_mask = audio_diag.valid_chan_mask;
	int symbol_10ms = audio_diag.oneshot_symbol_num;
	int symbol_size = audio_diag.symbol_size;

	if (opmode != OP_DSP)
		goto IGNORE_TX;

	tx_buf16 = (int16_t *)ca77xx_snd_audio_buf_req(audio_diag.hw_dev, DIR_TX, 0);
	tx_buf8 = (int8_t *)tx_buf16;

	if (symbol_size == 2) {
		for (i = 0; i < symbol_10ms; i++) {
			ch = 0;
			for (j = 0; j < total_chan; j++) {
				if (chan_mask & BIT(j)) {
					; /* *tx_buf = *app_audio_buf */;
				}
				ch++;
				tx_buf16++;
			}
		}
	} else {
		for (i = 0; i < symbol_10ms; i++) {
			ch = 0;
			for (j = 0; j < total_chan; j++) {
				if (chan_mask & BIT(j)) {
					; /* *tx_buf = *app_audio_buf */;
				}
				ch++;
				tx_buf8++;
			}
		}
	}

	return;

IGNORE_TX:
	if (dbg_flag & DBG_TX_NOTIFY)
		printk("Tx\n");
}

void pcm_rx_notify(void *data)
{
	int i, j, ch;
	int16_t *rx_buf16 = NULL;
	int16_t *tx_buf16 = NULL;
	int8_t *rx_buf8 = NULL;
	int8_t *tx_buf8 = NULL;
	int total_chan = audio_diag.dma_chan;
	unsigned int chan_mask = audio_diag.valid_chan_mask;
	int symbol_10ms = audio_diag.oneshot_symbol_num;
	int symbol_size = audio_diag.symbol_size;

	if (opmode == OP_ALONE)
		goto IGNORE_RX;

	if (opmode == OP_LOOPBACK) {
		tx_buf16 = (int16_t *)ca77xx_snd_audio_buf_req(audio_diag.hw_dev, DIR_TX, 200); /* 200 shot later */
	} else if (opmode == OP_TALK) {
		tx_buf16 = (int16_t *)ca77xx_snd_audio_buf_req(audio_diag.hw_dev, DIR_TX, 0); /* current */
	}
	tx_buf8 = (int8_t *)tx_buf16;

	rx_buf16 = (int16_t *)ca77xx_snd_audio_buf_req(audio_diag.hw_dev, DIR_RX, 0);
	rx_buf8 = (int8_t *)rx_buf16;

	if (symbol_size == 2) {
		for (i = 0; i < symbol_10ms; i++) {
			ch = 0;

			for (j = 0; j < total_chan; j++) {

				if (chan_mask & BIT(j)) {
					if (opmode == OP_LOOPBACK) {
						*(tx_buf16) = *(rx_buf16);
					} else if (opmode == OP_TALK) {
						if (ch == 0)
							*(tx_buf16 + 1) = *(rx_buf16);
						else if(ch == 1)
							*(tx_buf16 - 1) = *(rx_buf16);
					} else {
						;/* *(app_audio_buf + ch) = *(rx_buf16 + ch) */
					}

				ch++;
				}
				rx_buf16++;
				tx_buf16++;
			}
		}
	}else {
		for (i = 0; i < symbol_10ms; i++) {
			ch = 0;

			for (j = 0; j < total_chan; j++) {

				if (chan_mask & BIT(j)) {
					if (opmode == OP_LOOPBACK) {
						*(tx_buf8) = *(rx_buf8);
					} else if (opmode == OP_TALK) {
						if (ch == 0)
							*(tx_buf8 + 1) = *(rx_buf8);
						else if(ch == 1)
							*(tx_buf8 - 1) = *(rx_buf8);
					} else {
						;/* *(app_audio_buf) = *(rx_buf8) */
					}

				ch++;
				}
				rx_buf8++;
				tx_buf8++;
			}
		}
	}

	return;

IGNORE_RX:
	if (dbg_flag & DBG_RX_NOTIFY)
		printk("Rx\n");
}

/******************************************************************************/
static void audio_wrapper_tx_handler(void *data)
{
	static unsigned int tx_cnt;

	tx_cnt++;

	if ((dbg_flag & DBG_10MS_INTR) && ((tx_cnt % 200) == 0))
		printk("tx_cnt(%d)\n", tx_cnt);

	audio_diag.tx_handler(data);
}

static void audio_wrapper_rx_handler(void *data)
{
	static unsigned int rx_cnt;

	rx_cnt++;

	if ((dbg_flag & DBG_10MS_INTR) && ((rx_cnt % 200) == 100))
		printk("rx_cnt(%d)\n", rx_cnt);

	audio_diag.rx_handler(data);
}

int audio_if_init(int chan_num, int chan_size, int op, int spi_cs)
{
	struct ca77xx_audio_cfg cfg;
	int max_chan, inner_chan_num;
	int oneshot_num, oneshot_size;
	int buf_size, endian, slots, width;
	unsigned int signal_fmt, tx_vld_mask[4], rx_vld_mask[4];
	int reserved_slot = 0;
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
	audio_diag.symbol_size = chan_size;
	audio_diag.oneshot_symbol_num = (8000 / 100) * (dev_chan_wb ? 2 : 1);
	buf_size = audio_diag.symbol_size * audio_diag.oneshot_symbol_num;

	max_chan = slots / audio_diag.symbol_size;

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

	audio_diag.dma_chan = inner_chan_num;

	oneshot_num = MAX_BUFFER_DURATION / 10;
	oneshot_size = audio_diag.dma_chan * buf_size;

	for (i = 0; i < chan_num; i++)
		audio_diag.valid_chan_mask |= BIT(i);

	for (i = 0; i < 4; i++) {
		tx_vld_mask[i] = 0;
		rx_vld_mask[i] = 0;
	}

	if (ifmode == IF_ZSI)
		reserved_slot = 2;
	if (rslots)
		reserved_slot = rslots;

	for (i = 0; i < audio_diag.dma_chan * audio_diag.symbol_size; i++) {
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

	audio_diag.hw_dev = ca77xx_snd_audio_if_register(&cfg);
	if (!audio_diag.hw_dev)
		return -ENODEV;

	if (ca77xx_snd_audio_clkcfg(audio_diag.hw_dev, pclk))
		return -EINVAL;

	if (ca77xx_snd_audio_slots(audio_diag.hw_dev, tx_vld_mask, rx_vld_mask,
				   slots, width))
		return -EINVAL;

	if (ca77xx_snd_audio_signal_formt(audio_diag.hw_dev, signal_fmt))
		return -EINVAL;

	ca77xx_snd_audio_data_formt(audio_diag.hw_dev, DIR_TX,
				    audio_diag.symbol_size, endian);
	ca77xx_snd_audio_data_formt(audio_diag.hw_dev, DIR_RX,
				    audio_diag.symbol_size, endian);

	ca77xx_snd_audio_dma_register(audio_diag.hw_dev, DIR_TX, oneshot_num,
				      oneshot_size);
	ca77xx_snd_audio_dma_register(audio_diag.hw_dev, DIR_RX, oneshot_num,
				      oneshot_size);

	ca77xx_snd_audio_dma_callback(audio_diag.hw_dev, DIR_TX,
				      audio_wrapper_tx_handler,
				      (void *)0x76543210);
	ca77xx_snd_audio_dma_callback(audio_diag.hw_dev, DIR_RX,
				      audio_wrapper_rx_handler,
				      (void *)0x89ABCDEF);

	ca77xx_snd_audio_start(audio_diag.hw_dev, DIR_RX);
	ca77xx_snd_audio_start(audio_diag.hw_dev, DIR_TX);

	return 0;
}

int audio_if_exit(void)
{
	ca77xx_snd_audio_stop(audio_diag.hw_dev, DIR_TX);
	ca77xx_snd_audio_stop(audio_diag.hw_dev, DIR_RX);

	ca77xx_snd_audio_dma_unregister(audio_diag.hw_dev, DIR_TX);
	ca77xx_snd_audio_dma_unregister(audio_diag.hw_dev, DIR_RX);

	ca77xx_snd_audio_if_unregister(audio_diag.hw_dev);

	return 0;
}

int init_module(void)
{
	memset(&audio_diag, 0, sizeof(struct audio_diag_dev));

	audio_diag.tx_handler = pcm_tx_notify;
	audio_diag.rx_handler = pcm_rx_notify;

	spin_lock_init(&audio_diag.lock);

	audio_if_init(dev_chan_num, dev_chan_size, IF_PCM, 0);

	return 0;
}

void cleanup_module(void)
{
	audio_if_exit();

	memset(&audio_diag, 0, sizeof(struct audio_diag_dev));
}

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

module_param_named(debug, dbg_flag, uint, 0644);
MODULE_PARM_DESC(debug, "debug flag\n");

MODULE_LICENSE("GPL");
