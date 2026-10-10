/*
 * sound/soc/ca77xx/ca77xx_snd_dai.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _CA77XX_SND_DAI_H
#define _CA77XX_SND_DAI_H

#define DAI_TYPE_PCM	0
#define DAI_TYPE_I2S	1
#define DAI_TYPE_SPDIF	2

#define CA77XX_PCM_CHANNEL_MIN		2
#define CA77XX_PCM_CHANNEL_MAX		2
#define CA77XX_I2S_CHANNEL_MIN		2
#define CA77XX_I2S_CHANNEL_MAX		2
#define CA77XX_SPDIF_CHANNEL_MIN	2
#define CA77XX_SPDIF_CHANNEL_MAX	2

#define CA77XX_PCM_RATES SNDRV_PCM_RATE_8000
#define CA77XX_I2S_RATES SNDRV_PCM_RATE_8000_192000
#define CA77XX_SPDIF_RATES SNDRV_PCM_RATE_8000_192000

#define CA77XX_PCM_FORMATS (SNDRV_PCM_FMTBIT_MU_LAW | SNDRV_PCM_FMTBIT_A_LAW | \
			    SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE)
#define CA77XX_I2S_FORMATS (SNDRV_PCM_FMTBIT_S8 | \
                            SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
                            SNDRV_PCM_FMTBIT_S24_LE)
#define CA77XX_SPDIF_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | \
                              SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE | SNDRV_PCM_FMTBIT_MPEG)

typedef struct {
	u8 port;
	u8 type;
} ca77xx_dai_port_t;

int ca77xx_snd_dai_set_port(struct snd_soc_dai *cpu_dai, int port, int type);

extern struct snd_soc_dai_driver ca77xx_snd_dai[];
//extern struct snd_soc_dai ca77xx_snd_dai[];

#endif
