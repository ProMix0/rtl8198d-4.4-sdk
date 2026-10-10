/*
 * Cortina Specific Extensions for Synopsys DW Multimedia Card Interface driver
 *
 * Copyright (C) 2018, Cortina Accss Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"
#include "dw_mmc-cortina.h"

void inline sd_phase_override(struct dw_mci *host, u32 regs_val)
{
	struct dw_mci_cortina_priv_data *priv = host->priv;

	u32 regs = regs_val;
	regs |= (1 << PHASE_RESET_OVERRIDE);
	writel(regs, (void *)priv->sd_dll_ctrl);
	usleep_range(1000, 1500);
	regs &= ~(1 << PHASE_RESET_OVERRIDE);
	writel(regs, (void *)priv->sd_dll_ctrl);
	usleep_range(1000, 1500);
	wmb();
}

static int dw_mci_cortina_set_clock(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_cortina_priv_data *priv = host->priv;

	u32 regs = readl((void *)priv->sd_dll_ctrl);
	u32 io_drv_val = readl((void *)priv->g_io_drv_ctrl);

	if (ios->clock >= 100000000) {
		regs |= 1 << 1;  // 200MHz
		regs &= ~(CLK_DRV_PHASE_MASK << 16);
		regs &= ~(CLK_SMPL_PHASE_MASK << 24);
		host->bus_hz = 200000000;
		if (ios->timing == MMC_TIMING_MMC_HS200) {
			io_drv_val &= ~(SD_DS_MASK << SD_DS_SHIFT);
			io_drv_val |= 0xbb << SD_DS_SHIFT;
		}
	}
	else {
		regs = 0x143000; // use default value
		host->bus_hz = 50000000;
		io_drv_val &= ~(SD_DS_MASK << SD_DS_SHIFT);
		io_drv_val |= 0x33 << SD_DS_SHIFT; // default is 0x33
	}

	writel(io_drv_val, (void *)priv->g_io_drv_ctrl);
	sd_phase_override(host, regs);

	dev_dbg(host->dev, "%s(%d): SD_DLL_CTRL:0x%08x, host bus_hz %uHz "
		"timing = %u, ios clock %u, io_drv %x\n",
		__func__, __LINE__, regs, host->bus_hz, ios->timing, ios->clock, io_drv_val);

	return 0;
}

static int dw_mci_cortina_set_ios(struct dw_mci *host, struct mmc_ios *ios)
{

	struct dw_mci_cortina_priv_data *priv = host->priv;

	if (ios->timing == MMC_TIMING_UHS_SDR50 ||
			ios->timing == MMC_TIMING_UHS_SDR104) {
		set_bit(DW_MMC_CARD_NO_USE_HOLD, &host->slot->flags);
	} else {
		clear_bit(DW_MMC_CARD_NO_USE_HOLD, &host->slot->flags);
	}

	priv->ios = ios;

	return dw_mci_cortina_set_clock(host, ios);
}

static int dw_mci_cortina_clk_drv_phase_tuning(struct dw_mci_slot *slot, u32 opcode)
{
	struct dw_mci *host = slot->host;
	struct mmc_host *mmc = slot->mmc;
	struct dw_mci_cortina_priv_data *priv = host->priv;
	u32 regs;
	u8 phase_level;
	u8 i, j;
	u8 *good_phase;
	int tmp_range = 0, max_range = 0, begin_phase = 0, mid_phase = 0, err = 0;

	if (priv->ios->clock == 200000000)
		phase_level = 20;
	else if (priv->ios->clock == 100000000)
		phase_level = 40;
	else {
		dev_err(host->dev, "%s can't support frequency %u\n", __func__,  priv->ios->clock);
		return -EINVAL;
	}

	good_phase = kzalloc(phase_level * sizeof(u8), GFP_KERNEL);
	if (IS_ERR_OR_NULL(good_phase))
		return -ENOMEM;

	regs = readl((void*)priv->sd_dll_ctrl);
	for (i=0; i<phase_level; i++) {
		regs &= ~(0x3f << CLK_DRV_PHASE_CRTL);
		regs |= ((i & 0x3f) << CLK_DRV_PHASE_CRTL);
		sd_phase_override(host, regs);

		if (!mmc_send_tuning(mmc, opcode, NULL)) {
			good_phase[i] = 1;
		}
	}

	// find longest shift length
	for (i = 0; i < phase_level; i++) {
		tmp_range = 0; // clear

		for (j = 0; j < phase_level; j++) {
			u8 tmp = i+j;
			if (tmp >= phase_level)
				tmp -= phase_level;
			if (good_phase[tmp] == 1)
				tmp_range++;
			else
				break;
		}

		if (tmp_range > max_range) {
			max_range = tmp_range;
			begin_phase = i;
		}
	}

	if (max_range < 2) {
		err = -EIO;
		goto OUT;
	}

	// find middle phase
	mid_phase = begin_phase  + max_range / 2;
	if (mid_phase >= phase_level)
		mid_phase -= phase_level;

	// write register
	regs &= ~(0x3f << CLK_DRV_PHASE_CRTL);
	regs |= (mid_phase & 0x3f) <<  CLK_DRV_PHASE_CRTL;
	sd_phase_override(host, regs);
	regs = readl((void*) priv->sd_dll_ctrl);

	priv->cur_phase_drv = mid_phase;

	dev_dbg(host->dev, "drv phase %d, regs 0x%08x, begin %d, range %d\n",
			priv->cur_phase_drv, regs, begin_phase, max_range);

OUT:
	if (good_phase)
		kfree(good_phase);
	return err;
}

static int dw_mci_cortina_clk_smpl_phase_tuning(struct dw_mci_slot *slot, u32 opcode)
{
	struct dw_mci *host = slot->host;
	struct mmc_host *mmc = slot->mmc;
	struct dw_mci_cortina_priv_data *priv = host->priv;

	u32 regs;
	u8 phase_level;
	u8 i, j;
	u8 *good_phase;
	int tmp_range = 0, max_range = 0, begin_phase = 0, mid_phase = 0, err = 0;

	if (priv->ios->clock == 200000000)
		phase_level = 20;
	else if (priv->ios->clock == 100000000)
		phase_level = 40;
	else {
		dev_err(host->dev, "%s can't support frequency %u\n", __func__, priv->ios->clock);
		return -EINVAL;
	}

	good_phase = kzalloc(phase_level * sizeof(u8), GFP_KERNEL);
	if (IS_ERR_OR_NULL(good_phase))
		return -ENOMEM;

	regs = readl((void*)priv->sd_dll_ctrl);
	for (i=0; i<phase_level; i++) {
		regs &= ~(0x3f << CLK_SMPL_PHASE_CRTL);
		regs |= ((i & 0x3f) << CLK_SMPL_PHASE_CRTL);
		sd_phase_override(host, regs);

		if (!mmc_send_tuning(mmc, opcode, NULL)) {
			good_phase[i] = 1;
		}
	}

	// find longest shift length
	for (i = 0; i < phase_level; i++) {
		tmp_range = 0; // clear

		for (j = 0; j < phase_level; j++) {
			u8 tmp = i+j;
			if (tmp >= phase_level)
				tmp -= phase_level;
			if (good_phase[tmp] == 1)
				tmp_range++;
			else
				break;
		}

		if (tmp_range > max_range) {
			max_range = tmp_range;
			begin_phase = i;
		}
	}

	if (max_range < 2) {
		err = -EIO;
		goto OUT;
	}

	// find middle phase
	mid_phase = begin_phase  + max_range / 2;
	if (mid_phase >= phase_level)
		mid_phase -= phase_level;

	// write register
	regs &= ~(0x3f << CLK_SMPL_PHASE_CRTL);
	regs |= (mid_phase & 0x3f) <<  CLK_SMPL_PHASE_CRTL;
	sd_phase_override(host, regs);
	regs = readl((void*) priv->sd_dll_ctrl);

	priv->cur_phase_smpl = mid_phase;

	dev_dbg(host->dev, "smpl phase %d, regs 0x%08x, begin %d, range %d\n",
			priv->cur_phase_smpl, regs, begin_phase, max_range);

OUT:
	if (good_phase)
		kfree(good_phase);
	return err;
}

static int dw_mci_cortina_execute_tuning(struct dw_mci_slot *slot, u32 opcode)
{

	int drv_err = 0;
	int smpl_err = 0;
#if 1
	drv_err = dw_mci_cortina_clk_drv_phase_tuning(slot, opcode);
	if (drv_err) {
		dev_err(slot->host->dev, "clk_drv_phase_tuning fail\n");
	}
#endif
#if 1
	smpl_err = dw_mci_cortina_clk_smpl_phase_tuning(slot, opcode);
	if (smpl_err) {
		dev_err(slot->host->dev, "clk_smpl_phase_tuning fail\n");
	}
#endif

		return (drv_err || smpl_err);
}


static int dw_mci_cortina_init(struct dw_mci *host)
{
	struct dw_mci_cortina_priv_data *priv;
	struct device_node *node = host->dev->of_node;
	u32 regs = 0;
	int ret;

	priv  = devm_kzalloc(host->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(host->dev, "%s(%d) private data alloc fail\n", __func__, __LINE__);
		return -ENOMEM;
	}

	of_property_read_u32(node, "sd_dll_ctrl_reg", &regs);
	priv->sd_dll_ctrl = ioremap_nocache(regs, 4);
	if (IS_ERR(priv->sd_dll_ctrl)) {
		dev_err(host->dev, "%s(%d) ioremap 0x%08x fail\n", __func__, __LINE__, regs);
		goto ERR;
	}

	of_property_read_u32(node, "g_io_drv_ctrl", &regs);
	priv->g_io_drv_ctrl = ioremap_nocache(regs, 4);
	if (IS_ERR(priv->g_io_drv_ctrl)) {
		dev_err(host->dev, "%s(%d) ioremap 0x%08x fail\n", __func__, __LINE__, regs);
		goto ERR;
	}

	host->priv = priv;

	return 0;
ERR:
	iounmap(priv->sd_dll_ctrl);
	iounmap(priv->g_io_drv_ctrl);

	return -EINVAL;
}

#ifdef CONFIG_PM_SLEEP
static int dw_mci_cortina_suspend(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	return dw_mci_runtime_suspend(dev);
}

static int dw_mci_cortina_resume(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	return dw_mci_runtime_resume(dev);
}

#else
#define dw_mci_cortina_suspend		NULL
#define dw_mci_cortina_resume		NULL
#endif /* CONFIG_PM_SLEEP */

static const struct dw_mci_drv_data ca_drv_data = {
	.init = dw_mci_cortina_init,
	.execute_tuning = dw_mci_cortina_execute_tuning,
	.set_ios = dw_mci_cortina_set_ios,
};

static const struct of_device_id dw_mci_cortina_match[] = {
	{
		.compatible = "snps,dw-cortina",
		.data = &ca_drv_data,
	},

	{},
};
MODULE_DEVICE_TABLE(of, dw_mci_cortina_match);

static ssize_t
dw_mci_show_cmd_info(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_cortina_priv_data *priv = host->priv;

	priv->debug_flag = ~priv->debug_flag & 0x1;
	printk("%s debug %s\n", __func__, (priv->debug_flag & 0x1)?"on":"off");
	return 0;
}

static DEVICE_ATTR(show_cmd_info, S_IRUGO, dw_mci_show_cmd_info, NULL);

static int dw_mci_cortina_probe(struct platform_device *pdev)
{
	const struct dw_mci_drv_data *drv_data;
	const struct of_device_id *match;
	int ret;

	if (device_create_file(&pdev->dev, &dev_attr_show_cmd_info))
		pr_warn("%s: unable to show cmd info\n", __func__);

	match = of_match_node(dw_mci_cortina_match, pdev->dev.of_node);
	drv_data = match->data;

	ret = dw_mci_pltfm_register(pdev, drv_data);
	if (ret)
		pr_err("%s: can't register dw_mmc-cortina\n", __func__);

	return ret;
}

static const struct dev_pm_ops dw_mci_cortina_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_mci_cortina_suspend, dw_mci_cortina_resume)
};

static struct platform_driver dw_mci_cortina_pltfm_driver = {
	.probe		= dw_mci_cortina_probe,
	.remove		= dw_mci_pltfm_remove,
	.driver		= {
		.name		= "dwmmc_cortina",
		.of_match_table	= dw_mci_cortina_match,
		.pm		= &dw_mci_cortina_pmops,
	},
};

module_platform_driver(dw_mci_cortina_pltfm_driver);

MODULE_DESCRIPTION("Cortina Specific DW-MSHC Driver Extension");
MODULE_AUTHOR("Arthur Li <arthur.li@cortina-access.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:dwmmc_cortina");
