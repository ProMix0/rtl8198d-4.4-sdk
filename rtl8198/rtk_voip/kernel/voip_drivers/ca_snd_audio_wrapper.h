#ifndef _CA_SND_AUDIO_WRAPPER
#define _CA_SND_AUDIO_WRAPPER

int audio_driver_load(void);
int audio_if_init(int chan_num, void *pTxBuf[], void *pRxBuf[]);
int audio_if_exit(void);
int audio_if_start(void);
int audio_if_stop(void);
int audio_tx_data(void);
int audio_rx_data(void);
int audio_isr_handler(void);
int audio_channel_wideband(int chan, int enable);

int audio_dma_tx_update(void);
int audio_dma_rx_update(void);
u32 audio_rd_dma_reg(unsigned int reg);
u32 audio_get_dma_rxbuf_size(u32 rx_ptr, u32 wt_ptr);
#endif /* _CA_SND_AUDIO_WRAPPER */
