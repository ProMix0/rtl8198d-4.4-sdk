/*
 * Cortina-Access OTP Driver
 *
 * Copyright (C) 2020 Cortina Access, Inc.
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

#define OP_READ            0
#define OP_WRITE       1

struct cortina_otp_context {
	struct device *dev;
	void __iomem *base;
	unsigned int nregs;
};

static int cortina_otp_write(void *context, const void *data, size_t count)
{
	/* Nothing TBD, Read-Only */

	return 0;
}

static int cortina_otp_read(void *context,
			    const void *reg, size_t reg_size,
			    void *val, size_t val_size)
{
	struct cortina_otp_context *priv = context;
	int i, index, count;

	index = (*(u32 *) reg) >> 0;
	count = val_size;

	if (count > (priv->nregs - index))
		count = priv->nregs - index;

	for (i = index; i < (index + count); i++) {
		*(u8 *) val = readb(priv->base + i);
		val += 1;
	}

	return (i - index);
}

static struct regmap_bus cortina_otp_bus = {
	.read = cortina_otp_read,
	.write = cortina_otp_write,
	.reg_format_endian_default = REGMAP_ENDIAN_NATIVE,
	.val_format_endian_default = REGMAP_ENDIAN_NATIVE,
};

static struct regmap_config cortina_otp_regmap_config = {
	.reg_bits = 8,
	.reg_stride = 1,
	.val_bits = 8,
};

static struct nvmem_config econfig = {
	.name = "cortina-otp",
	.owner = THIS_MODULE,
	.read_only = true,
};

static const struct of_device_id cortina_otp_match[] = {
	{.compatible = "cortina,cortina-otp",},
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, cortina_otp_match);

static int cortina_otp_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct nvmem_device *nvmem;
	struct regmap *regmap;
	void __iomem *base;
	struct cortina_otp_context *context;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	context = devm_kzalloc(dev, sizeof(struct cortina_otp_context),
			       GFP_KERNEL);
	if (IS_ERR(context))
		return PTR_ERR(context);

	context->dev = dev;
	context->base = base;
	if (of_property_read_u32(np, "ngroups", &context->nregs)) {
		dev_warn(dev, "Missing ngroups OF property\n");
		context->nregs = 1024;
	}

	cortina_otp_regmap_config.max_register = (context->nregs - 1);

	regmap = devm_regmap_init(dev, &cortina_otp_bus,
				  context, &cortina_otp_regmap_config);
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

static int cortina_otp_remove(struct platform_device *pdev)
{
	struct nvmem_device *nvmem = platform_get_drvdata(pdev);

	return nvmem_unregister(nvmem);
}

static struct platform_driver cortina_otp_driver = {
	.probe = cortina_otp_probe,
	.remove = cortina_otp_remove,
	.driver = {
		   .name = "cortina-otp",
		   .of_match_table = cortina_otp_match,
		   },
};

module_platform_driver(cortina_otp_driver);
MODULE_DESCRIPTION("cortina_otp driver");
MODULE_LICENSE("GPL v2");
