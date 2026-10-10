/*
 * File: sound/soc/ca77xx/ca77xx_sound_dai.c
 * Description: ASoC platform driver for CA77XX - I2S interface
 *
 * Copyright (c) Cortina-Systems Limited 2012. All rights reserved.
 *
 * Mostly copied from bf5xx-i2s.c and some from davinci-mcasp.c
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "ca77xx_snd_sport.h"
#include "ca77xx_snd_dai.h"


ca77xx_dai_port_t dai_port[CA77XX_SSP_NUM];


/*
 * DAIs are able to configure to PCM, I2S, SPDIF(only DAI 0). ASoC core doesn't
 * provide the API to do this. So we provide a propreitary API to support this.
 */
int ca77xx_snd_dai_set_port(struct snd_soc_dai *cpu_dai, int port, int type)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);

	if (port >= CA77XX_SSP_NUM)
		return -EINVAL;

	switch (type) {
	case DAI_TYPE_SPDIF:
		if (port != 0)
			return -EINVAL;
	case DAI_TYPE_PCM:
	case DAI_TYPE_I2S:
		break;
	default:
		pr_err("%s: not support type %d\n", __func__, type);
		return -EINVAL;
	}

	dai_port[port].port = port;
	dai_port[port].type = type;

	cpu_dai->private_data = &dai_port[port];

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_dai_set_port);

static int ca77xx_snd_dai_set_sysclk(struct snd_soc_dai *cpu_dai,
		int clk_id, unsigned int freq, int dir)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static int ca77xx_snd_dai_set_pll(struct snd_soc_dai *cpu_dai, int pll_id,
		int source, unsigned int freq_in, unsigned int freq_out)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static int ca77xx_snd_dai_set_clkdiv(struct snd_soc_dai *cpu_dai,
		int div_id, int div)
{
	int ret = 0;

	if (sport_debug & (DAI_TRACE | DAI_INFO))
		printk("%s: id = %d, div_id = %d, div = %d\n", __func__, cpu_dai->id, div_id, div);

	ret = ca77xx_snd_sport_clkdiv(cpu_dai->id, div_id, div);
	if (ret)
		pr_err("%s unsupported dai clkdiv - %d\n", cpu_dai->name, div_id);

	return 0;
}

static int ca77xx_snd_dai_set_dai_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	unsigned int signal_fmt = 0;
	int ret = 0;

	if (sport_debug & (DAI_TRACE | DAI_INFO))
		printk("%s: fmt 0x%x\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		signal_fmt |= SPORT_DATA_I2S;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		signal_fmt |= SPORT_DATA_RIGHT_J;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		signal_fmt |= SPORT_DATA_LEFT_J;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		signal_fmt |= SPORT_DATA_DSP_A;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		signal_fmt |= SPORT_DATA_DSP_B;
		break;
	case SND_SOC_DAIFMT_AC97:
		signal_fmt |= SPORT_DATA_AC97;
		break;
	case SND_SOC_DAIFMT_PDM:
		signal_fmt |= SPORT_DATA_PDM;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_CLOCK_MASK) {
	case SND_SOC_DAIFMT_CONT:
		signal_fmt |= SPORT_CLOCK_CONT;
		break;
	case SND_SOC_DAIFMT_GATED:
		signal_fmt |= SPORT_CLOCK_GATED;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		signal_fmt |= SPORT_SIGNAL_NB_NF;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		signal_fmt |= SPORT_SIGNAL_NB_IF;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		signal_fmt |= SPORT_SIGNAL_IB_NF;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		signal_fmt |= SPORT_SIGNAL_IB_IF;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		signal_fmt |= SPORT_CLOCK_CBM_CFM;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		signal_fmt |= SPORT_CLOCK_CBS_CFS;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		signal_fmt |= SPORT_CLOCK_CBM_CFS;
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		signal_fmt |= SPORT_CLOCK_CBS_CFM;
		break;
	}

	ret = ca77xx_snd_sport_signal_formt(cpu_dai->id, signal_fmt);
	if (ret)
		pr_err("%s: unsupported dai format 0x%x\n", cpu_dai->name, fmt);

	return ret;
}

static int ca77xx_snd_dai_set_tdm_slot(struct snd_soc_dai *cpu_dai,
		unsigned int tx_mask, unsigned int rx_mask,
		int slots, int slot_width)
{
	int ret = 0;

	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);

	ret = ca77xx_snd_sport_slots(cpu_dai->id, tx_mask, rx_mask, slots, slot_width);
	if (ret)
		pr_err("%s not support tdm slots %d, width %d\n", cpu_dai->name, slots, slot_width);

	return ret;
}

static int ca77xx_snd_dai_set_channel_map(struct snd_soc_dai *cpu_dai,
		unsigned int tx_num, unsigned int *tx_slot,
		unsigned int rx_num, unsigned int *rx_slot)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static int ca77xx_snd_dai_set_dai_tristate(struct snd_soc_dai *cpu_dai,
	int tristate)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static int ca77xx_snd_dai_digital_mute(struct snd_soc_dai *cpu_dai, int mute)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static int ca77xx_snd_dai_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *cpu_dai)
{
	int dir, size, endian = 0;
	int ret = 0;

	if (sport_debug & (DAI_TRACE | DAI_INFO))
		printk("%s: params %d\n", __func__, params_format(params));

	size = snd_pcm_format_size(params_format(params), 1);
	endian = snd_pcm_format_big_endian(params_format(params));
	dir = substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? DIR_TX : DIR_RX;

	if (sport_debug & DAI_INFO)
		printk("%s: id %d, dir %d, size %d, endian %d\n", __func__, \
		        cpu_dai->id, dir, size, endian);

	ret = ca77xx_snd_sport_audio_formt(cpu_dai->id, dir, size, endian);
	if (ret)
		pr_err("%s unsupported dai format 0x%x\n", cpu_dai->name,
							params_format(params));

	return ret;
}

static int ca77xx_snd_dai_hw_free(struct snd_pcm_substream *substream,
				struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static int ca77xx_snd_dai_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static int ca77xx_snd_dai_trigger(struct snd_pcm_substream *substream, int cmd,
				struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static snd_pcm_sframes_t ca77xx_snd_dai_delay(
			struct snd_pcm_substream *substream,
			struct snd_soc_dai *cpu_dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_pcm_sframes_t total, frames;
	unsigned int diff;

	if (sport_debug & DAI_TRACE)
		pr_debug("%s\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		diff = ca77xx_snd_sport_dma_curr_offset(cpu_dai->id, DIR_TX);
		frames = bytes_to_frames(substream->runtime, diff);
		total = runtime->periods * runtime->period_size;
		return (runtime->control->appl_ptr + total - frames) % total;
	} else {
		;/* TBD */
	}

	return 0;
}

static int ca77xx_snd_dai_startup(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		pr_debug("%s\n", __func__);
	return 0;
}

static void ca77xx_snd_dai_shutdown(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		pr_debug("%s\n", __func__);
}



/* 
 * For Kernel 3.3.6
 * Author:Rudolph
 */
static int ca77xx_snd_dai_probe(struct snd_soc_dai *cpu_dai)
{
	//ca77xx_dai_port_t *dai_port = (ca77xx_dai_port_t *)cpu_dai->private_data;
	int type, tx, rx;

#if 0
	if (dai_port == NULL) {
		pr_err("%s: dai_port is NULL\n", __func__);
		return -ENODEV;
	}
#endif

	///TODO: for debug
	printk("%s: cpu_dai->id=%d, cpu_dai->driver->id=%d, cpu_dai->name=%s\n", __func__, cpu_dai->id, cpu_dai->driver->id, cpu_dai->name);

        if (!strcmp(cpu_dai->name, ca77xx_snd_dai[DAI_TYPE_PCM].name)) {
		type = CA77XX_SPORT_PCM;
	}
        else if (!strcmp(cpu_dai->name, ca77xx_snd_dai[DAI_TYPE_I2S].name)) {
		type = CA77XX_SPORT_I2S;
	}
        else if (!strcmp(cpu_dai->name, ca77xx_snd_dai[DAI_TYPE_SPDIF].name)) {
		type = CA77XX_SPORT_SPDIF;
	}
	else {
		return -EPERM;
	}

	tx = cpu_dai->driver->playback.channels_max ? 1 : 0;
	rx = cpu_dai->driver->capture.channels_max ? 1 : 0;

	//if (ca77xx_snd_sport_if_register(cpu_dai->driver->id, type, tx, rx))
	if (ca77xx_snd_sport_if_register(cpu_dai->id, type, tx, rx))
		return -EPERM;

	return 0;
}


/*
static int ca77xx_snd_dai_probe(struct platform_device *pdev,
			   struct snd_soc_dai *cpu_dai)
{
	ca77xx_dai_port_t *dai_port = (ca77xx_dai_port_t *)cpu_dai->private_data;
	int type, tx, rx;

	if (dai_port == NULL) {
		pr_err("%s: dai_port is NULL\n", __func__);
		return -ENODEV;
	}

	switch (dai_port->type) {
	case DAI_TYPE_PCM:
		type = CA77XX_SPORT_PCM;
		break;
	case DAI_TYPE_I2S:
		type = CA77XX_SPORT_I2S;
		break;
	case DAI_TYPE_SPDIF:
		type = CA77XX_SPORT_SPDIF;
		break;
	default:
		return -EPERM;
	}

	tx = cpu_dai->playback.channels_max ? 1 : 0;
	rx = cpu_dai->capture.channels_max ? 1 : 0;

	if (ca77xx_snd_sport_if_register(cpu_dai->id, type, tx, rx))
		return -EPERM;

	return 0;
}
*/

/*
 * For Kernel3.3.8
 * Author:Rudolph
 */
static void ca77xx_snd_dai_remove(struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);

	ca77xx_snd_sport_if_unregister(cpu_dai->driver->id);
}

/*
static void ca77xx_snd_dai_remove(struct platform_device *pdev,
			struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);

	ca77xx_snd_sport_if_unregister(cpu_dai->id);
}
*/

#ifdef CONFIG_PM
static int ca77xx_snd_dai_suspend(struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

static int ca77xx_snd_dai_resume(struct snd_soc_dai *cpu_dai)
{
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	return 0;
}

#else
#define ca77xx_snd_dai_suspend	NULL
#define ca77xx_snd_dai_resume	NULL
#endif


static struct snd_soc_dai_ops ca77xx_snd_dai_ops = {
	/* clocking configuration - all optional */
	//.set_sysclk	= ca77xx_snd_dai_set_sysclk,
	//.set_pll	= ca77xx_snd_dai_set_pll,
	.set_clkdiv	= ca77xx_snd_dai_set_clkdiv,

	/* format configuration */
	.set_fmt	= ca77xx_snd_dai_set_dai_fmt,
	.set_tdm_slot	= ca77xx_snd_dai_set_tdm_slot,
	.set_channel_map = ca77xx_snd_dai_set_channel_map,
	.set_tristate	= ca77xx_snd_dai_set_dai_tristate,

	/* digital mute - optional */
	.digital_mute = ca77xx_snd_dai_digital_mute,

	/* audio operations - all optional */
	.startup	= ca77xx_snd_dai_startup,
	.shutdown	= ca77xx_snd_dai_shutdown,
	.hw_params	= ca77xx_snd_dai_hw_params,
	.hw_free	= ca77xx_snd_dai_hw_free,
	.prepare	= ca77xx_snd_dai_prepare,
	.trigger	= ca77xx_snd_dai_trigger,

	/* delay reporting - optional */
	.delay		= ca77xx_snd_dai_delay,
};


/*
 * For 3.18 sound prototype.
 * Author: Aaron 
 */
struct snd_soc_dai_driver ca77xx_snd_dai[] = {
	[DAI_TYPE_PCM] = {
		.name = "ca77xx-pcm",
		.id = 0,
		.probe = ca77xx_snd_dai_probe,
		.remove = ca77xx_snd_dai_remove,
		.suspend = ca77xx_snd_dai_suspend,
		.resume = ca77xx_snd_dai_resume,
		.playback = {
			.channels_min = CA77XX_PCM_CHANNEL_MIN,
			.channels_max = CA77XX_PCM_CHANNEL_MIN,
			.rates = CA77XX_PCM_RATES,
			.formats = CA77XX_PCM_FORMATS,},
		.capture = {
			.channels_min = CA77XX_PCM_CHANNEL_MIN,
			.channels_max = CA77XX_PCM_CHANNEL_MIN,
			.rates = CA77XX_PCM_RATES,
			.formats = CA77XX_PCM_FORMATS},
	},
	[DAI_TYPE_I2S] = {
		.name = "ca77xx-i2s",
		.id = 0,
		.probe = ca77xx_snd_dai_probe,
		.remove = ca77xx_snd_dai_remove,
		.suspend = ca77xx_snd_dai_suspend,
		.resume = ca77xx_snd_dai_resume,
		.playback = {
			.channels_min = CA77XX_I2S_CHANNEL_MIN,
			.channels_max = CA77XX_I2S_CHANNEL_MAX,
			.rates = CA77XX_I2S_RATES,
			.formats = CA77XX_I2S_FORMATS,},
		.ops = &ca77xx_snd_dai_ops,
	},
	[DAI_TYPE_SPDIF] = {
		.name = "ca77xx-spdif",
		.id = 0,
		.probe = ca77xx_snd_dai_probe,
		.remove = ca77xx_snd_dai_remove,
		.suspend = ca77xx_snd_dai_suspend,
		.resume = ca77xx_snd_dai_resume,
		.playback = {
			.channels_min = CA77XX_SPDIF_CHANNEL_MIN,
			.channels_max = CA77XX_SPDIF_CHANNEL_MAX,
			.rates = CA77XX_SPDIF_RATES,
			.formats = CA77XX_SPDIF_FORMATS,},
		.ops = &ca77xx_snd_dai_ops,		
	},
};
EXPORT_SYMBOL_GPL(ca77xx_snd_dai);

/*
struct snd_soc_dai ca77xx_snd_dai[] = {
	[DAI_TYPE_PCM] = {
		.name = "ca77xx-pcm",
		.id = 0,
		.probe = ca77xx_snd_dai_probe,
		.remove = ca77xx_snd_dai_remove,
		.suspend = ca77xx_snd_dai_suspend,
		.resume = ca77xx_snd_dai_resume,
		.playback = {
			.channels_min = CA77XX_PCM_CHANNEL_MIN,
			.channels_max = CA77XX_PCM_CHANNEL_MIN,
			.rates = CA77XX_PCM_RATES,
			.formats = CA77XX_PCM_FORMATS
		},
		.capture = {
			.channels_min = CA77XX_PCM_CHANNEL_MIN,
			.channels_max = CA77XX_PCM_CHANNEL_MIN,
			.rates = CA77XX_PCM_RATES,
			.formats = CA77XX_PCM_FORMATS
		},
	},
	[DAI_TYPE_I2S] = {
		.name = "ca77xx-i2s",
		.id = 0,
		.probe = ca77xx_snd_dai_probe,
		.remove = ca77xx_snd_dai_remove,
		.suspend = ca77xx_snd_dai_suspend,
		.resume = ca77xx_snd_dai_resume,
		.playback = {
			.channels_min = CA77XX_I2S_CHANNEL_MIN,
			.channels_max = CA77XX_I2S_CHANNEL_MAX,
			.rates = CA77XX_I2S_RATES,
			.formats = CA77XX_I2S_FORMATS,
		},
		.ops = &ca77xx_snd_dai_ops,
	},
	[DAI_TYPE_SPDIF] = {
		.name = "ca77xx-spdif",
		.id = 0,
		.probe = ca77xx_snd_dai_probe,
		.remove = ca77xx_snd_dai_remove,
		.suspend = ca77xx_snd_dai_suspend,
		.resume = ca77xx_snd_dai_resume,
		.playback = {
			.channels_min = CA77XX_SPDIF_CHANNEL_MIN,
			.channels_max = CA77XX_SPDIF_CHANNEL_MAX,
			.rates = CA77XX_SPDIF_RATES,
			.formats = CA77XX_SPDIF_FORMATS,
		},
		.ops = &ca77xx_snd_dai_ops,
	},
};
EXPORT_SYMBOL_GPL(ca77xx_snd_dai);
*/

static const struct snd_soc_component_driver ca77xx_snd_component = {
         .name           = "ca77xx-i2s",
};

int ca77xx_snd_dai_init(struct platform_device *pdev)
{
#if 0
	int i, ret;
	struct snd_soc_dai *dev;

	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);
	
	dev = ca77xx_snd_sport_device();
	
	/*
	 * For Kernel 3.18
	 * Author: Aaron 
	 */	
	ret = snd_soc_register_component(&pdev->dev, &ca77xx_snd_component, &ca77xx_snd_dai, 1);
	if (ret != 0) {
		dev_err(dev, "could not register dai\n");
		return ret;
	}
	return ret;
#endif
	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_dai_init);

void ca77xx_snd_dai_exit(struct platform_device *pdev)
{
#if 0
	if (sport_debug & DAI_TRACE)
		printk("%s\n", __func__);

	snd_soc_unregister_dais(&pdev->dev);
#endif
}
EXPORT_SYMBOL(ca77xx_snd_dai_exit);

extern struct snd_soc_codec_driver soc_codec_dev_dae4p;
int ca77xx_soc_platform_register(struct device *dev);
static int ca77xx_i2s_probe(struct platform_device *pdev)
{
        int i, ret;
        struct snd_soc_dai *dev;

        if (sport_debug & DAI_TRACE)
                printk("%s: pdev=%p, device name=%s\n", __func__, pdev, dev_name(&(pdev->dev)));

        dev = ca77xx_snd_sport_device();

        /*
         * For Kernel 3.18
         * Author: Aaron
         */
        ret = snd_soc_register_component(&pdev->dev, &ca77xx_snd_component, &ca77xx_snd_dai, ARRAY_SIZE(ca77xx_snd_dai));
	//ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_dae4p, ca77xx_snd_dai, ARRAY_SIZE(ca77xx_snd_dai));

        if (ret != 0) {
                dev_err(dev, "could not register dai\n");
                return ret;
        }
	printk("%s: register dai OK\n", __func__);

	ret = ca77xx_soc_platform_register(&pdev->dev);
        if (ret) {
                dev_err(&pdev->dev, "register PCM failed: %d\n", ret);
		return ret;
        }
	printk("%s: register I2S OK\n", __func__);

        return ret;
}

static int ca77xx_i2s_remove(struct platform_device *pdev)
{
	if (sport_debug & DAI_TRACE)
                printk("%s\n", __func__);

        snd_soc_unregister_dais(&pdev->dev);

        return 0;
}

static const struct of_device_id ca77xx_i2s_of_match[] = {
        { .compatible = "cortina,ca77xx-i2s", },
        {},
};

static struct platform_driver ca77xx_i2s_driver = {
        .probe          = ca77xx_i2s_probe,
        .remove         = ca77xx_i2s_remove,
        .driver         = {
                .name   = "ca77xx-i2s",
                .owner  = THIS_MODULE,
                .of_match_table = ca77xx_i2s_of_match,
        },
};

int __init ca77xx_i2s_init(void)
{
        int ret = 0;

        if (sport_debug & DAI_TRACE)
                printk("%s\n", __func__);

        ret = platform_driver_register(&ca77xx_i2s_driver);

        return ret;
}

void __exit ca77xx_i2s_exit(void)
{
        if (sport_debug & DAI_TRACE)
                printk("%s\n", __func__);

        platform_driver_unregister(&ca77xx_i2s_driver);
}
module_init(ca77xx_i2s_init);
module_exit(ca77xx_i2s_exit);


