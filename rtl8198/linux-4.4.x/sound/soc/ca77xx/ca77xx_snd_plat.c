/*
 * File: sound/soc/ca77xx/ca77xx_snd_plat.c
 * Description: ASoC platform driver for CA77XX
 *
 *  Copyright (c) Cortina-Systems Limited 2012. All rights reserved.
 *
 *  Mostly copied from the bf5xx-i2s-pcm.c driver
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/gfp.h>
#include <linux/byteorder/generic.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>

#include "ca77xx_snd_plat.h"
#include "ca77xx_snd_dai.h"
#include "ca77xx_snd_sport.h"

//#define CONFIG_SND_CA77XX_MMAP_SUPPORT

/*
 * For Kernel3.3.8
 * Author: Rudolph
 */
static void ca77xx_snd_dma_irq(void *data)
{
	struct snd_pcm_substream *substream = data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int dir = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? DIR_TX : DIR_RX;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	if (snd_pcm_running(substream)) {
		u16 idx;

		if ((runtime->control->appl_ptr % runtime->period_size) == 0)
			idx = (runtime->control->appl_ptr / runtime->period_size) % runtime->periods;
		else // end of stream 
			idx = (runtime->control->appl_ptr / runtime->period_size + 1) % runtime->periods;

		ca77xx_snd_sport_dma_update(cpu_dai->id, dir, idx);
	}

	snd_pcm_period_elapsed(substream);
}


/*
static void ca77xx_snd_dma_irq(void *data)
{
	struct snd_pcm_substream *substream = data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai_link->cpu_dai;
	int dir = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? DIR_TX : DIR_RX;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	if (snd_pcm_running(substream)) {
		u16 idx;

		if ((runtime->control->appl_ptr % runtime->period_size) == 0)
			idx = (runtime->control->appl_ptr / runtime->period_size) % runtime->periods;
		else // end of stream 
			idx = (runtime->control->appl_ptr / runtime->period_size + 1) % runtime->periods;

		ca77xx_snd_sport_dma_update(cpu_dai->id, dir, idx);
	}

	snd_pcm_period_elapsed(substream);
}
*/

/*
  Definition:
    1 frame represents 1 analog sample from all channels
    1 period is the number of frames in between each hardware interrupt
  Ex: a stereo, 16-bit, 44.1 KHz stream
    1 analog sample is represented with 16 bits = 2 bytes
    1 frame = (num_channels) * (1 sample in bytes)
            = (2 channels) * (2 bytes (16 bits) per sample)
            = 4 bytes (32 bits)
    data transfer rate (Bytes/sec)
            = (num_channels) * (1 sample in bytes) * (analog_rate)
            = (1 frame) * (analog_rate)
            = ( 2 channels ) * (2 bytes/sample) * (44100 samples/sec) = 2*2*44100
            = 176400 Bytes/sec
 */
static const struct snd_pcm_hardware ca77xx_snd_hardware = {
#if defined(CONFIG_SND_CA77XX_MMAP_SUPPORT)
	.info			= SNDRV_PCM_INFO_INTERLEAVED | \
				  SNDRV_PCM_INFO_MMAP |        \
				  SNDRV_PCM_INFO_MMAP_VALID |  \
				  SNDRV_PCM_INFO_BLOCK_TRANSFER,
#else
	.info			= SNDRV_PCM_INFO_INTERLEAVED | \
				  SNDRV_PCM_INFO_BLOCK_TRANSFER,
#endif
	.formats		= CA77XX_SPDIF_FORMATS,
	.rates			= CA77XX_SPDIF_RATES,
//	.rate_min		= 8000,
//	.rate_max		= 48000,
	.channels_min		= CA77XX_I2S_CHANNEL_MIN,
	.channels_max		= CA77XX_I2S_CHANNEL_MAX,
	.buffer_bytes_max	= 4096*16,
	.period_bytes_max	= 1024,
	.period_bytes_min	= 512,
//	.periods_max		= 1024,
//	.periods_min		= 128,
	.periods_max		= 32,
	.periods_min		= 16,
//	.fifo_size
};

static int ca77xx_snd_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	size_t size = ca77xx_snd_hardware.buffer_bytes_max;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	if (snd_pcm_lib_malloc_pages(substream, size) < 0) {
		pr_err("%s: snd_pcm_lib_malloc_pages() fails!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int ca77xx_snd_hw_free(struct snd_pcm_substream *substream)
{
	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	snd_pcm_lib_free_pages(substream);

	return 0;
}


/*
 * For Kernel3.3.8
 * Author:Rudolph
 */
static int ca77xx_snd_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int period_bytes = frames_to_bytes(runtime, runtime->period_size);
	int ret = 0;

	if (sport_debug & (PLAT_TRACE | PLAT_INFO))
		printk("%s: dma_addr 0x%x, dma_area 0x%p, dma_bytes 0x%x, " \
		       "run_periods 0x%x, period_size %d, period_bytes 0x%x, " \
		       "mmap_count %d\n", __func__, runtime->dma_addr, runtime->dma_area, \
		       runtime->dma_bytes, runtime->periods, (int)runtime->period_size, \
		       period_bytes, atomic_read(&substream->mmap_count));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ret = ca77xx_snd_sport_dma_register(cpu_dai->id, DIR_TX, runtime->dma_addr,
			runtime->periods, period_bytes);
		if (ret)
			return -EINVAL;

		ca77xx_snd_sport_dma_callback(cpu_dai->id, DIR_TX, ca77xx_snd_dma_irq, substream);
	} else {
		ret = ca77xx_snd_sport_dma_register(cpu_dai->id, DIR_RX, runtime->dma_addr,
			runtime->periods, period_bytes);
		if (ret)
			return -EINVAL;

		ca77xx_snd_sport_dma_callback(cpu_dai->id, DIR_RX, ca77xx_snd_dma_irq, substream);
	}

	//memset(runtime->dma_area, 0, runtime->periods * period_bytes);	// for debug

	return 0;
}


/*
static int ca77xx_snd_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	int period_bytes = frames_to_bytes(runtime, runtime->period_size);
	int ret = 0;

	if (sport_debug & (PLAT_TRACE | PLAT_INFO))
		printk("%s: dma_addr 0x%x, dma_area 0x%p, dma_bytes 0x%x, " \
		       "run_periods 0x%x, period_size %d, period_bytes 0x%x, " \
		       "mmap_count %d\n", __func__, runtime->dma_addr, runtime->dma_area, \
		       runtime->dma_bytes, runtime->periods, (int)runtime->period_size, \
		       period_bytes, atomic_read(&substream->mmap_count));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ret = ca77xx_snd_sport_dma_register(cpu_dai->id, DIR_TX, runtime->dma_addr,
			runtime->periods, period_bytes);
		if (ret)
			return -EINVAL;

		ca77xx_snd_sport_dma_callback(cpu_dai->id, DIR_TX, ca77xx_snd_dma_irq, substream);
	} else {
		ret = ca77xx_snd_sport_dma_register(cpu_dai->id, DIR_RX, runtime->dma_addr,
			runtime->periods, period_bytes);
		if (ret)
			return -EINVAL;

		ca77xx_snd_sport_dma_callback(cpu_dai->id, DIR_RX, ca77xx_snd_dma_irq, substream);
	}

	//memset(runtime->dma_area, 0, runtime->periods * period_bytes);	// for debug

	return 0;
}
*/

/*
 * For Kernel3.3.6
 * Author:Rudolph
 */
static int ca77xx_snd_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;

	if (sport_debug & (PLAT_TRACE | PLAT_INFO))
		printk("%s: cmd %d stream %d\n", __func__, cmd, substream->stream);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			// HW design: Tx queue can't be empty when Tx trigger ->
			//   write index != 0 
			u16 wt_idx = runtime->control->appl_ptr / runtime->period_size;

			// HW design: queue full --> write index = periods - 1,
			//   can't fill out the full DMA buffer 
			if (wt_idx == runtime->periods)
				wt_idx = runtime->periods - 1;
			ca77xx_snd_sport_dma_update(cpu_dai->id, DIR_TX, wt_idx);

			ca77xx_snd_sport_start(cpu_dai->id, DIR_TX);
		} else {
			ca77xx_snd_sport_start(cpu_dai->id, DIR_RX);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			ca77xx_snd_sport_stop(cpu_dai->id, DIR_TX);
		} else {
			ca77xx_snd_sport_stop(cpu_dai->id, DIR_RX);
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}


/*
static int ca77xx_snd_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	int ret = 0;

	if (sport_debug & (PLAT_TRACE | PLAT_INFO))
		printk("%s: cmd %d stream %d\n", __func__, cmd, substream->stream);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			// HW design: Tx queue can't be empty when Tx trigger ->
			//   write index != 0 
			u16 wt_idx = runtime->control->appl_ptr / runtime->period_size;

			// HW design: queue full --> write index = periods - 1,
			//   can't fill out the full DMA buffer 
			if (wt_idx == runtime->periods)
				wt_idx = runtime->periods - 1;
			ca77xx_snd_sport_dma_update(cpu_dai->id, DIR_TX, wt_idx);

			ca77xx_snd_sport_start(cpu_dai->id, DIR_TX);
		} else {
			ca77xx_snd_sport_start(cpu_dai->id, DIR_RX);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			ca77xx_snd_sport_stop(cpu_dai->id, DIR_TX);
		} else {
			ca77xx_snd_sport_stop(cpu_dai->id, DIR_RX);
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}
*/

/*
 * For Kernel3.3.6 
 * Author:Rudolph
 */
static snd_pcm_uframes_t ca77xx_snd_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int diff;
	snd_pcm_uframes_t frames;

	if (sport_debug & PLAT_TRACE)
		printk("%s, cpu_dai->id=%d\n", __func__, cpu_dai->id);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		diff = ca77xx_snd_sport_dma_curr_offset(cpu_dai->id, DIR_TX);
		frames = bytes_to_frames(substream->runtime, diff);
	} else {
		diff = ca77xx_snd_sport_dma_curr_offset(cpu_dai->id, DIR_RX);
		frames = bytes_to_frames(substream->runtime, diff);
	}

	return frames;
}

/*
static snd_pcm_uframes_t ca77xx_snd_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	unsigned int diff;
	snd_pcm_uframes_t frames;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		diff = ca77xx_snd_sport_dma_curr_offset(cpu_dai->id, DIR_TX);
		frames = bytes_to_frames(substream->runtime, diff);
	} else {
		diff = ca77xx_snd_sport_dma_curr_offset(cpu_dai->id, DIR_RX);
		frames = bytes_to_frames(substream->runtime, diff);
	}

	return frames;
}
*/

static int ca77xx_snd_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	snd_soc_set_runtime_hwparams(substream, &ca77xx_snd_hardware);

	ret = snd_pcm_hw_constraint_integer(runtime, \
			SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto out;

	ret = snd_pcm_hw_constraint_pow2(runtime, 0,
			SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto out;

	return 0;

 out:
 	pr_err("%s: ret %d\n", __func__, ret);
	return ret;
}

/*
 * For Kernel3.3.8
 * Author: Rudolph
 */
static int ca77xx_snd_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ca77xx_snd_sport_dma_callback(cpu_dai->id, DIR_TX, NULL, NULL);
		ca77xx_snd_sport_dma_unregister(cpu_dai->id, DIR_TX);
	} else {
		ca77xx_snd_sport_dma_callback(cpu_dai->id, DIR_RX, NULL, NULL);
		ca77xx_snd_sport_dma_unregister(cpu_dai->id, DIR_RX);
	}

	return 0;
}

/*
static int ca77xx_snd_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ca77xx_snd_sport_dma_callback(cpu_dai->id, DIR_TX, NULL, NULL);
		ca77xx_snd_sport_dma_unregister(cpu_dai->id, DIR_TX);
	} else {
		ca77xx_snd_sport_dma_callback(cpu_dai->id, DIR_RX, NULL, NULL);
		ca77xx_snd_sport_dma_unregister(cpu_dai->id, DIR_RX);
	}

	return 0;
}
*/

static int ca77xx_snd_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	size_t size = vma->vm_end - vma->vm_start;

	if (sport_debug & (PLAT_TRACE | PLAT_INFO))
		printk("%s: addr 0x%x, area 0x%p, size 0x%x\n", __func__, \
		        runtime->dma_addr, runtime->dma_area, size);

	memset(runtime->dma_area, 0, size);
	return dma_mmap_coherent(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr,
				     runtime->dma_bytes);
}

/*
 * For Kernel3.3.8
 * Author:Rudolph
 */
static int ca77xx_snd_copy(struct snd_pcm_substream *substream, int channel,
		    snd_pcm_uframes_t pos,
		    void __user *buf, snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	ca77xx_dai_port_t *dai_port = (ca77xx_dai_port_t *)cpu_dai->private_data;
	unsigned int offset, size;
	u16 wt_idx, rd_idx;

	if (sport_debug & (PLAT_TRACE | PLAT_INFO))
		printk("%s: chan %d, pos 0x%lx, buf 0x%p, count 0x%lx\n", \
		        __func__, channel, pos, buf, count);

	offset = frames_to_bytes(runtime, pos);
	size = frames_to_bytes(runtime, count);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		u32 *dst, *src;
		int i;

		dst = (u32 *)(runtime->dma_area + offset);
		src = (u32 *)buf;

		if (dai_port->type == DAI_TYPE_I2S) {	// big-endian interface
			if (runtime->format == SNDRV_PCM_FORMAT_S24_LE) {
				for (i = 0; i < size/4; i++)
					*(dst + i) = cpu_to_be32(*(src + i)) >> 8;
			} else if (runtime->format == SNDRV_PCM_FORMAT_S24_BE) {
				for (i = 0; i < size/4; i++)
					*(dst + i) = (*(src + i)) >> 8;
			} else {
				for (i = 0; i < size/4; i++)
					*(dst + i) = *(src + i);
			}
		} else if (dai_port->type == DAI_TYPE_SPDIF) {	// little-endian interface
			for (i = 0; i < size/4; i++)
				*(dst + i) = *(src + i);
		}

		return 0;
	} else {
		memcpy(buf, runtime->dma_area + offset, size);

		rd_idx = (offset + size) / frames_to_bytes(runtime, runtime->period_size);
		return ca77xx_snd_sport_dma_update(cpu_dai->id, DIR_RX, rd_idx);
	}

	return 0;
}

/*
static int ca77xx_snd_copy(struct snd_pcm_substream *substream, int channel,
		    snd_pcm_uframes_t pos,
		    void __user *buf, snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	ca77xx_dai_port_t *dai_port = (ca77xx_dai_port_t *)cpu_dai->private_data;
	unsigned int offset, size;
	u16 wt_idx, rd_idx;

	if (sport_debug & (PLAT_TRACE | PLAT_INFO))
		printk("%s: chan %d, pos 0x%lx, buf 0x%p, count 0x%lx\n", \
		        __func__, channel, pos, buf, count);

	offset = frames_to_bytes(runtime, pos);
	size = frames_to_bytes(runtime, count);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		u32 *dst, *src;
		int i;

		dst = (u32 *)(runtime->dma_area + offset);
		src = (u32 *)buf;

		if (dai_port->type == DAI_TYPE_I2S) {	// big-endian interface
			if (runtime->format == SNDRV_PCM_FORMAT_S24_LE) {
				for (i = 0; i < size/4; i++)
					*(dst + i) = cpu_to_be32(*(src + i)) >> 8;
			} else if (runtime->format == SNDRV_PCM_FORMAT_S24_BE) {
				for (i = 0; i < size/4; i++)
					*(dst + i) = (*(src + i)) >> 8;
			} else {
				for (i = 0; i < size/4; i++)
					*(dst + i) = *(src + i);
			}
		} else if (dai_port->type == DAI_TYPE_SPDIF) {	// little-endian interface
			for (i = 0; i < size/4; i++)
				*(dst + i) = *(src + i);
		}

		return 0;
	} else {
		memcpy(buf, runtime->dma_area + offset, size);

		rd_idx = (offset + size) / frames_to_bytes(runtime, runtime->period_size);
		return ca77xx_snd_sport_dma_update(cpu_dai->id, DIR_RX, rd_idx);
	}

	return 0;
}
*/

static struct snd_pcm_ops ca77xx_snd_ops = {
	.open		= ca77xx_snd_open,
	.close		= ca77xx_snd_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= ca77xx_snd_hw_params,
	.hw_free	= ca77xx_snd_hw_free,
	.prepare	= ca77xx_snd_prepare,
	.trigger	= ca77xx_snd_trigger,
	.pointer	= ca77xx_snd_pointer,
	.mmap		= ca77xx_snd_mmap,
	//.copy		= ca77xx_snd_copy,	/* used for no mmap */
	//.silence
	//.page
	//.ack
};

static int ca77xx_snd_preallocate_dma_buffer(struct snd_pcm *pcm,
		struct snd_soc_dai *cpu_dai, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = ca77xx_snd_hardware.buffer_bytes_max;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_coherent(pcm->card->dev, size,
			&buf->addr, GFP_KERNEL|GFP_DMA);
	if (!buf->area) {
		pr_err("Failed to allocate dma memory - Please increase uncached DMA memory region\n");
		return -ENOMEM;
	}
	buf->bytes = size;

	if (sport_debug & PLAT_INFO)
		printk("%s: addr 0x%x, area 0x%p, size 0x%08x\n", __func__, \
		        buf->addr, buf->area, buf->bytes);

	return 0;
}

static void ca77xx_snd_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	/* 2 stream - SNDRV_PCM_STREAM_PLAYBACK & SNDRV_PCM_STREAM_PLAYBACK */
	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_coherent(NULL, buf->bytes, buf->area, 0);
		buf->area = NULL;
	}
}

static u64 ca77xx_snd_dmamask = DMA_BIT_MASK(32);

/* modify for lunxi kernel 3.18 */
//int ca77xx_snd_new(struct snd_card *card, struct snd_soc_dai *cpu_dai, struct snd_pcm *pcm)
int ca77xx_snd_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;
        struct snd_card *card = rtd->card->snd_card;
        struct snd_pcm *pcm = rtd->pcm;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	// move to ca77xx_snd_sport.c
	if (!card->dev->dma_mask)
		card->dev->dma_mask = &ca77xx_snd_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);
	// move to ca77xx_snd_sport.c

	if (cpu_dai->driver->playback.channels_min) {
		ret = ca77xx_snd_preallocate_dma_buffer(pcm, cpu_dai,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (cpu_dai->driver->capture.channels_min) {
		ret = ca77xx_snd_preallocate_dma_buffer(pcm, cpu_dai,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}

 out:
	return ret;
}

/*
int ca77xx_snd_new(struct snd_card *card, struct snd_soc_dai *cpu_dai,
	struct snd_pcm *pcm)
{
	int ret = 0;

	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	// move to ca77xx_snd_sport.c
	if (!card->dev->dma_mask)
		card->dev->dma_mask = &ca77xx_snd_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);
	// move to ca77xx_snd_sport.c

	if (cpu_dai->playback.channels_min) {
		ret = ca77xx_snd_preallocate_dma_buffer(pcm, cpu_dai,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (cpu_dai->capture.channels_min) {
		ret = ca77xx_snd_preallocate_dma_buffer(pcm, cpu_dai,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}

 out:
	return ret;
}
*/

struct snd_soc_platform_driver ca77xx_snd_platform = {
	.ops 	        = &ca77xx_snd_ops,
	.pcm_new	= ca77xx_snd_new,
	.pcm_free	= ca77xx_snd_free_dma_buffers,
};
EXPORT_SYMBOL_GPL(ca77xx_snd_platform);

int ca77xx_soc_platform_register(struct device *dev)
{
        if (sport_debug & PLAT_TRACE)
                printk("%s: ca77xx_snd_platform=%p\n", __func__, &ca77xx_snd_platform);

        return devm_snd_soc_register_platform(dev, &ca77xx_snd_platform);
}
EXPORT_SYMBOL_GPL(ca77xx_soc_platform_register);

int ca77xx_soc_platform_remove(struct platform_device *pdev)
{
        snd_soc_unregister_platform(&pdev->dev);
        return 0;
}
EXPORT_SYMBOL(ca77xx_soc_platform_remove);

int ca77xx_soc_platform_probe(struct platform_device *pdev)
{
	///TODO: for debug
	if (sport_debug & PLAT_TRACE)
		printk("%s: ca77xx_snd_platform=%p\n", __func__, &ca77xx_snd_platform);

	return snd_soc_register_platform(&pdev->dev, &ca77xx_snd_platform);
}
EXPORT_SYMBOL(ca77xx_soc_platform_probe);

#if 0
int ca77xx_soc_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}
EXPORT_SYMBOL(ca77xx_soc_platform_remove);

/* added for linux kernel 3.18 */
static const struct of_device_id ca77xx_pcm_dt_ids[] = {
        {
                .compatible = "cortina,ca77xx-pcm-audio",
        },
        { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ca77xx_pcm_dt_ids);

static struct platform_driver ca77xx_pcm_driver = {
	.driver = {
			.name = "ca77xx-pcm-audio",
			.owner = THIS_MODULE,
			.of_match_table = of_match_ptr(ca77xx_pcm_dt_ids),
	},

	.probe = ca77xx_soc_platform_probe,
	.remove = ca77xx_soc_platform_remove,
};

module_platform_driver(ca77xx_pcm_driver);
#endif

MODULE_DESCRIPTION("Golden Gate PCM driver");
MODULE_LICENSE("GPL");

/*
struct snd_soc_platform ca77xx_snd_platform = {
	.name		= "ca77xx-audio",
	.pcm_ops 	= &ca77xx_snd_ops,
	.pcm_new	= ca77xx_snd_new,
	.pcm_free	= ca77xx_snd_free_dma_buffers,
};
EXPORT_SYMBOL_GPL(ca77xx_snd_platform);
*/


/*
int ca77xx_snd_plat_init(void)
{
	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	return snd_soc_register_platform(&ca77xx_snd_platform);
}
EXPORT_SYMBOL(ca77xx_snd_plat_init);

void ca77xx_snd_plat_exit(void)
{
	if (sport_debug & PLAT_TRACE)
		printk("%s\n", __func__);

	snd_soc_unregister_platform(&ca77xx_snd_platform);
}
EXPORT_SYMBOL(ca77xx_snd_plat_exit);
*/
