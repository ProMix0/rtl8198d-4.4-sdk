/*
 * Copyright 2017-2019 Cortina Access, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <dt-bindings/clock/ca77xx-glb-cfg.h>

#define CFG_PCIE0_CLKEN			10
#define CFG_SATA_CLKEN			11
#define CFG_PCIE1_CLKEN			12
#define CFG_PCIE2_CLKEN			14

#define CFG_L3FE_PD			27
#define CFG_OFFLOAD0_PD			28
#define CFG_OFFLOAD1_PD			29
#define CFG_CRYPTO_PD			30
#define	CFG_CORE_PD			31

struct ca77xx_glb_cfg {
	struct device *dev;
	void __iomem *regs;
	spinlock_t regs_lock; /* register access lock */
	const char *osc_name;

	struct clk_onecell_data onecell;
	struct clk *clks[CA77XX_CLOCK_COUNT];
};

static int ca77xx_clk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct clk **clks;
	struct ca77xx_glb_cfg *glb_cfg;
	struct resource *res;
	void *ret = NULL;
	int rc;

	glb_cfg = devm_kzalloc(dev, sizeof(*glb_cfg), GFP_KERNEL);
	if (!glb_cfg)
		return -ENOMEM;

	spin_lock_init(&glb_cfg->regs_lock);
	glb_cfg->dev = dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	glb_cfg->regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(glb_cfg->regs))
		return PTR_ERR(glb_cfg->regs);

	platform_set_drvdata(pdev, glb_cfg);

	glb_cfg->onecell.clk_num = CA77XX_CLOCK_COUNT;
	glb_cfg->onecell.clks = glb_cfg->clks;
	clks = glb_cfg->clks;

	clks[CA77XX_CLOCK_PCIE0] =
		clk_register_gate(dev, "pcie0", NULL, 0, glb_cfg->regs,
				  CFG_PCIE0_CLKEN, 0, &glb_cfg->regs_lock);
	clks[CA77XX_CLOCK_SATA] =
		clk_register_gate(dev, "sata", NULL, 0, glb_cfg->regs,
				  CFG_SATA_CLKEN, 0, &glb_cfg->regs_lock);
	clks[CA77XX_CLOCK_PCIE1] =
		clk_register_gate(dev, "pcie1", NULL, 0, glb_cfg->regs,
				  CFG_PCIE1_CLKEN, 0, &glb_cfg->regs_lock);
	clks[CA77XX_CLOCK_PCIE2] =
		clk_register_gate(dev, "pcie2", NULL, 0, glb_cfg->regs,
				  CFG_PCIE2_CLKEN, 0, &glb_cfg->regs_lock);
#if 0
	clks[CA77XX_CLOCK_L3FE] =
		clk_register_gate(dev, "l3fe", NULL, 0, glb_cfg->regs,
				  CFG_L3FE_PD, CLK_GATE_SET_TO_DISABLE,
				  &glb_cfg->regs_lock);
	clks[CA77XX_CLOCK_OFFLOAD0] =
		clk_register_gate(dev, "pe0", NULL, 0, glb_cfg->regs,
				  CFG_OFFLOAD0_PD, CLK_GATE_SET_TO_DISABLE,
				  &glb_cfg->regs_lock);
	clks[CA77XX_CLOCK_OFFLOAD1] =
		clk_register_gate(dev, "pe1", NULL, 0, glb_cfg->regs,
				  CFG_OFFLOAD1_PD, CLK_GATE_SET_TO_DISABLE,
				  &glb_cfg->regs_lock);
	clks[CA77XX_CLOCK_CRYPTO] =
		clk_register_gate(dev, "crypto", NULL, 0, glb_cfg->regs,
				  CFG_CRYPTO_PD, CLK_GATE_SET_TO_DISABLE,
				  &glb_cfg->regs_lock);
	clks[CA77XX_CLOCK_CORE] =
		clk_register_gate(dev, "core", NULL, 0, glb_cfg->regs,
				  CFG_CORE_PD, CLK_GATE_SET_TO_DISABLE,
				  &glb_cfg->regs_lock);
#endif

	return of_clk_add_provider(dev->of_node, of_clk_src_onecell_get,
				   &glb_cfg->onecell);
}

static const struct of_device_id ca77xx_clk_of_match[] = {
	{ .compatible = "cortina,ca77xx-glb-cfg", },
	{}
};
MODULE_DEVICE_TABLE(of, ca77xx_clk_of_match);

static struct platform_driver ca77xx_clk_driver = {
	.driver = {
		.name = "ca77xx-glb-cfg",
		.of_match_table = ca77xx_clk_of_match,
	},
	.probe          = ca77xx_clk_probe,
};

builtin_platform_driver(ca77xx_clk_driver);

MODULE_DESCRIPTION("CA7774 clock driver");
MODULE_LICENSE("GPL v2");

