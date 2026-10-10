#ifndef __CA77XX_SND_AUDIO_H__
#define __CA77XX_SND_AUDIO_H__

#define CA77XX_AUDIO_UNDEF	0
#define CA77XX_AUDIO_PCM	1
#define CA77XX_AUDIO_I2S	2

#define DIR_TX			BIT(0)
#define DIR_RX			BIT(1)
#define DIR_BOTH		(DIR_TX | DIR_RX)

#define	FLAG_ISI		BIT(1)
#define	FLAG_ZSI		BIT(2)
#define	FLAG_INT_LB		BIT(3)
#define FLAG_MASK		(FLAG_ISI | FLAG_ZSI | FLAG_INT_LB)

#define MAX_SPI_CS		4

struct ca77xx_audio_cfg {
	unsigned int	idx;
	unsigned int	type;
	unsigned int	dir;
	unsigned short	flag;
	unsigned short	spi_cs;
};

/* audio formats */
#define AUDIO_DATA_I2S			0 /* I2S mode */
#define AUDIO_DATA_RIGHT_J		1 /* Right Justified mode */
#define AUDIO_DATA_LEFT_J		2 /* Left Justified mode */
#define AUDIO_DATA_DSP_A		3 /* L data MSB after FRM LRC */
#define AUDIO_DATA_DSP_B		4 /* L data MSB during FRM LRC */
#define AUDIO_DATA_AC97			5 /* AC97 */
#define AUDIO_DATA_PDM			6 /* Pulse density modulation */

/* clock gating */
#define AUDIO_CLOCK_CONT		(0 << 4) /* continuous clock */
#define AUDIO_CLOCK_GATED		(1 << 4) /* clock is gated */

/* signal inversions */
#define AUDIO_SIGNAL_NB_NF		(0 << 8) /* normal bit clock + frame */
#define AUDIO_SIGNAL_NB_IF		(1 << 8) /* normal BCLK + inv FRM */
#define AUDIO_SIGNAL_IB_NF		(2 << 8) /* invert BCLK + nor FRM */
#define AUDIO_SIGNAL_IB_IF		(3 << 8) /* invert BCLK + FRM */

/* clock masters */
#define AUDIO_CLOCK_CBM_CFM		(0 << 12) /* codec clk & FRM master */
#define AUDIO_CLOCK_CBS_CFM		(1 << 12) /* codec clk slave & FRM master */
#define AUDIO_CLOCK_CBM_CFS		(2 << 12) /* codec clk master & frame slave */
#define AUDIO_CLOCK_CBS_CFS		(3 << 12) /* codec clk & FRM slave */

#define AUDIO_DATA_FORMAT_MASK		0x000f
#define AUDIO_CLOCK_GATE_MASK		0x00f0
#define AUDIO_SIGNAL_INV_MASK		0x0f00
#define AUDIO_CLOCK_MASTER_MASK		0xf000

/* clock division id */
#define AUDIO_INT_SYS_CLK_DIV		0
#define AUDIO_EXT_ISI_CLK_DIV		1
#define AUDIO_EXT_ZSI_CLK_DIV		2

void *ca77xx_snd_audio_if_register(struct ca77xx_audio_cfg *cfg);
int ca77xx_snd_audio_if_unregister(void *dev_instance);
int ca77xx_snd_audio_clkdiv(void *dev_instance, int clk_src, int data_rate);
int ca77xx_snd_audio_clkcfg(void *dev_instance, int data_rate);
int ca77xx_snd_audio_slots(void *dev_instance, unsigned int tx_mask,
			   unsigned int rx_mask, int slots, int width);
int ca77xx_snd_audio_signal_formt(void *dev_instance, unsigned int format);
int ca77xx_snd_audio_data_formt(void *dev_instance, int dir, int wide, int endian);
int ca77xx_snd_audio_dma_register(void *dev_instance, int dir,
				  unsigned int buf_num, unsigned int buf_size);
int ca77xx_snd_audio_dma_unregister(void *dev_instance, int dir);
int ca77xx_snd_audio_dma_callback(void *dev_instance, int dir,
				  void (*callback)(void *), void *data);
int ca77xx_snd_audio_start(void *dev_instance, int dir);
int ca77xx_snd_audio_stop(void *dev_instance, int dir);
void *ca77xx_snd_audio_buf_req(void *dev_instance, int dir, int block_offset);

#endif /* __CA77XX_SND_AUDIO_H__ */
