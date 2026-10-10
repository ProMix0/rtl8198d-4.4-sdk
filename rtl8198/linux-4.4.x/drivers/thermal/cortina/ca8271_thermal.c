/*
 * Cortina-Access thermal driver
 *
 * Copyright (C) 2018 Cortina Access, Inc.
 *		http://www.cortina-access.com
 *
 * Based on kirwood_thermal.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/delay.h>

#define TMS_REG_A		0x0
#define TMS_REG_B		0x4
#define TMS_REG_C		0x8
#define TMS_REG_DATA	0xC
#define TMS_REG_RANGE	0x10

#define CORTINA_TEMPERATURE_NEGATIVE_FLAG	(1<<18)
#define CORTINA_TEMPERATURE_MASK	0x0003FC00
#define CORTINA_TEMPERATURE_FRACT_MASK	0x000003FF
#define CORTINA_TEMPERATURE_SHIFT	10

/* Cortina Thermal Sensor Dev Structure */
struct cortina_thermal_priv {
	void __iomem *sensor;
};

static int cortina_get_temp(void *p, int *temp)
{
	unsigned long reg;
	struct cortina_thermal_priv *priv = p;
	int negative = 0;

	reg = readl_relaxed(priv->sensor + TMS_REG_DATA);

	/*
	 * Calculate temperature.
	 * The value is Celsius with lower 10 bits as fraction and
	 * [17:10] is integer and [18] is sign.
	 */
	negative = (reg & CORTINA_TEMPERATURE_NEGATIVE_FLAG) ? 1 : 0;
	*temp = (reg & CORTINA_TEMPERATURE_MASK) >> CORTINA_TEMPERATURE_SHIFT;
	*temp *= 1000;
	*temp += (reg & CORTINA_TEMPERATURE_FRACT_MASK);
	if (negative == 1)
		*temp = 0 - *temp;

	return 0;
}

static struct thermal_zone_of_device_ops ops = {
	.get_temp = cortina_get_temp,
};

static const struct of_device_id cortina_thermal_id_table[] = {
	{.compatible = "cortina,thermal-sensor"},
	{}
};

static int cortina_thermal_probe(struct platform_device *pdev)
{
	struct thermal_zone_device *thermal = NULL;
	struct cortina_thermal_priv *priv;
	struct resource *res;
	unsigned int reg_v;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->sensor = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->sensor))
		return PTR_ERR(priv->sensor);

	writel_relaxed(0x0A110000, priv->sensor + TMS_REG_A);
	writel_relaxed(0x003497FF, priv->sensor + TMS_REG_B);
	writel_relaxed(0x0101011d, priv->sensor + TMS_REG_C);
	mdelay(300);
	reg_v = readl_relaxed(priv->sensor + TMS_REG_C);
	reg_v = readl_relaxed(priv->sensor + TMS_REG_DATA);
	reg_v = readl_relaxed(priv->sensor + TMS_REG_DATA);
	dev_info(&pdev->dev, "SoC Temperature=%s%d\n",
	       reg_v & CORTINA_TEMPERATURE_NEGATIVE_FLAG ? "-" : " ",
	       (reg_v & CORTINA_TEMPERATURE_MASK) >> CORTINA_TEMPERATURE_SHIFT);

	thermal = thermal_zone_of_sensor_register(&pdev->dev, 0, priv,
						     &ops);

	if (IS_ERR(thermal)) {
		dev_err(&pdev->dev, "Failed to register thermal zone device\n");
		return PTR_ERR(thermal);
	}

	platform_set_drvdata(pdev, thermal);

	return 0;
}

static int cortina_thermal_exit(struct platform_device *pdev)
{
	struct thermal_zone_device *cortina_thermal =
	    platform_get_drvdata(pdev);

	thermal_zone_device_unregister(cortina_thermal);

	return 0;
}

MODULE_DEVICE_TABLE(of, cortina_thermal_id_table);

static struct platform_driver cortina_thermal_driver = {
	.probe = cortina_thermal_probe,
	.remove = cortina_thermal_exit,
	.driver = {
		   .name = "cortina_thermal",
		   .of_match_table = cortina_thermal_id_table,
		   },
};

module_platform_driver(cortina_thermal_driver);

MODULE_DESCRIPTION("Cortina-Access thermal driver");
MODULE_LICENSE("GPL");
