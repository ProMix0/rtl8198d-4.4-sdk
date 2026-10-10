/*
 * Cortina-Access eFuse Driver
 *
 * Copyright (C) 2017 Cortina Access, Inc.
 *              http://www.cortina-access.com
 *
 * Based on rockchip-efuse.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */

#include <linux/platform_device.h>
#include <linux/nvmem-provider.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/of.h>

#define FUSE_CONTROL		0x00
#define	  FUSE_GROUP_OFFSET	  0
#define	  FUSE_GROUP_MASK	  0x1F
#define	  FUSE_GROUP_MAX	  31
#define	  FUSE_NUMBER_OFFSET	  5
#define	  FUSE_NUMBER_MASK	  0x1F
#define	  FUSE_NUMBER_MAX	  31
#define	  FUSE_MR		  BIT(10)
#define	  FUSE_PROGRAM		  BIT(11)
#define	  FUSE_READ		  BIT(12)
#define FUSE_RESULT		0x04
#define	  FUSE_DONE		  BIT(0)
#define FUSE_VALUE		0x08
#define FUSE_PGM_TIMING		0x0C
#define FUSE_READ_TIMING	0x10

#define OP_READ			0
#define OP_WRITE		1

#define POLL_COUNT		100

struct cortina_efuse_context {
	struct device *dev;
	void __iomem *base;
	unsigned int nregs;
	bool redundant;
};

static int cortina_efuse_access(void *context, int group, u32 *val, int op)
{
	struct cortina_efuse_context *priv = context;
	u32 reg;
	int i;

	if (op == OP_READ) {
		*val = 0;

		reg = (group & FUSE_GROUP_MASK) << FUSE_GROUP_OFFSET |
		      FUSE_READ;
		writel(reg, priv->base + FUSE_CONTROL);

		i = POLL_COUNT;
		do {
			reg = readl(priv->base + FUSE_RESULT);
			if (reg & FUSE_DONE)
				break;
			usleep_range(10, 20);
		} while (--i);

		if (i)
			*val = readl(priv->base + FUSE_VALUE);
		else
			dev_err(priv->dev, "FUSE: read group %d TIMEOUT!\n",
				group);

		writel(0, priv->base + FUSE_CONTROL);

		return i ? EIO : 0;
	}

	return 0;
}

static int cortina_efuse_write(void *context, const void *data, size_t count)
{
	/* Nothing TBD, Read-Only */

	return 0;
}

static int cortina_efuse_read(void *context,
			      const void *reg, size_t reg_size,
			      void *val, size_t val_size)
{
	struct cortina_efuse_context *priv = context;
	u32 redundant;
	int i, index, count;

	index = (*(u32 *)reg) >> 2;
	count = val_size >> 2;

	if (count > (priv->nregs - index))
		count = priv->nregs - index;

	for (i = index; i < (index + count); i++) {
		cortina_efuse_access(context, i, val, OP_READ);
		if (priv->redundant) {
			cortina_efuse_access(context, i + priv->nregs,
					     &redundant, OP_READ);
			*(u32 *)val = *(u32 *)val | redundant;
		}
		val += 4;
	}

	return (i - index) * 4;
}

static struct regmap_bus cortina_efuse_bus = {
	.read = cortina_efuse_read,
	.write = cortina_efuse_write,
	.reg_format_endian_default = REGMAP_ENDIAN_NATIVE,
	.val_format_endian_default = REGMAP_ENDIAN_NATIVE,
};

static struct regmap_config cortina_efuse_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
};

static struct nvmem_config econfig = {
	.name = "cortina-efuse",
	.owner = THIS_MODULE,
	.read_only = true,
};

static const struct of_device_id cortina_efuse_match[] = {
	{ .compatible = "cortina,cortina-efuse",},
	{ /* sentinel */},
};
MODULE_DEVICE_TABLE(of, cortina_efuse_match);

static int cortina_efuse_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct nvmem_device *nvmem;
	struct regmap *regmap;
	void __iomem *base;
	struct cortina_efuse_context *context;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	context = devm_kzalloc(dev, sizeof(struct cortina_efuse_context),
			       GFP_KERNEL);
	if (IS_ERR(context))
		return PTR_ERR(context);

	context->dev = dev;
	context->base = base;
	if (of_property_read_u32(np, "ngroups", &context->nregs)) {
		dev_warn(dev, "Missing ngroups OF property\n");
		context->nregs = 32;
	}
	if (of_property_read_bool(np, "redundant"))
		context->redundant = true;
	else
		context->redundant = false;
	if (context->redundant)
		context->nregs /= 2;

	cortina_efuse_regmap_config.max_register = (context->nregs - 1) * 4;

	regmap = devm_regmap_init(dev, &cortina_efuse_bus,
				  context, &cortina_efuse_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(dev, "regmap init failed\n");
		return PTR_ERR(regmap);
	}
	econfig.dev = dev;
	nvmem = nvmem_register(&econfig);
	if (IS_ERR(nvmem))
		return PTR_ERR(nvmem);

	platform_set_drvdata(pdev, nvmem);

	return 0;
}

static int cortina_efuse_remove(struct platform_device *pdev)
{
	struct nvmem_device *nvmem = platform_get_drvdata(pdev);

	return nvmem_unregister(nvmem);
}

static struct platform_driver cortina_efuse_driver = {
	.probe = cortina_efuse_probe,
	.remove = cortina_efuse_remove,
	.driver = {
		.name = "cortina-efuse",
		.of_match_table = cortina_efuse_match,
	},
};

module_platform_driver(cortina_efuse_driver);
MODULE_DESCRIPTION("cortina_efuse driver");
MODULE_LICENSE("GPL v2");
