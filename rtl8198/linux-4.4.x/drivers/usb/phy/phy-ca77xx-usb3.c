/*
 * CA77XX USB3.0 PHY Device Driver for dwc3 core controller on
 * Cortina-Access CA77XX SoCs
 *
 * Copyright (C) 2017 Cortina Access, Inc.
 *		 http://www.cortina-access.com
 *
 * Based on phy-rtk-usb3.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/usb/otg.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/usb/ch11.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/reset-controller.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/reset.h>

#define CA77XX_USB3PHY_NAME "ca77xx-usb3phy"

/* USB3(USB Super Speed) register offsets */
#define USB3CFG_CNTRL_OFFSET				0x00
#define USB3CFG_PHY_VAUX_RESET					BIT(31)
#define USB3CFG_BUS_CLKEN_GSLAVE				BIT(30)
#define USB3CFG_BUS_CLKEN_GMASTER				BIT(29)
#define USB3CFG_BIGENDIAN_GSLAVE				BIT(28)
#define USB3CFG_HOST_PORT_POWER_CTRL_PRESENT	BIT(27)
#define USB3CFG_HOST_MSI_ENABLE					BIT(26)
#define USB3CFG_HOST_LEGACY_SMI_PCI_CMD_REG_WR	BIT(25)
#define USB3CFG_HOST_LEGACY_SMI_BAR_WR			BIT(24)
#define USB3CFG_FLADJ_30MHZ_REG_MASK(x)			((0x3f & (x)) << 16)
#define USB3CFG_XHCI_BUS_MASTER_ENABLE			BIT(4)
#define USB3CFG_BUS_FILTER_BYPASS_MASK(x)		(0xf & (x))

#define USB3CFG_STATUS_OFFSET				0x04
#define USB3CFG_HOST_SYSTEM_ERR					BIT(31)
#define USB3CFG_LEGACY_SMI_INTERRUPT			BIT(16)
#define USB3CFG_HOST_CURRENT_BELT_MASK(x)		(0xfff & (x))

#define USB3CFG_PORT_CONFIG_OFFSET			0x08
#define USB3CFG_U3P1_HOST_DISABLE				BIT(26)
#define USB3CFG_U3P1_HUB_PORT_OVERCURRENT		BIT(25)
#define USB3CFG_U3P1_HUB_PORT_PERM_ATTACH		BIT(24)
#define USB3CFG_U3P0_HOST_DISABLE				BIT(18)
#define USB3CFG_U3P0_HUB_PORT_OVERCURRENT		BIT(17)
#define USB3CFG_U3P0_HUB_PORT_PERM_ATTACH		BIT(16)
#define USB3CFG_U2P1_HOST_DISABLE				BIT(10)
#define USB3CFG_U2P1_HUB_PORT_OVERCURRENT		BIT(9)
#define USB3CFG_U2P1_HUB_PORT_PERM_ATTACH		BIT(8)
#define USB3CFG_U2P0_HOST_DISABLE				BIT(2)
#define USB3CFG_U2P0_HUB_PORT_OVERCURRENT		BIT(1)
#define USB3CFG_U2P0_HUB_PORT_PERM_ATTACH		BIT(0)

#define USB3CFG_PORT_STATUS_OFFSET			0x0c
#define USB3CFG_U3P1_PIPE3_PHY_MODE_MASK(x)		((0x3 & (x)) << 28)
#define USB3CFG_U3P1_HUB_VBUS_CTRL				BIT(24)
#define USB3CFG_U3P0_PIPE3_PHY_MODE_MASK(x)		((0x3 & (x)) << 20)
#define USB3CFG_U3P0_HUB_VBUS_CTRL				BIT(16)
#define USB3CFG_U2P1_UTMI_FSLS_LOW_POWER		BIT(9)
#define USB3CFG_U2P1_HUB_VBUS_CTRL				BIT(8)
#define USB3CFG_U2P0_UTMI_FSLS_LOW_POWER		BIT(1)
#define USB3CFG_U2P0_HUB_VBUS_CTRL				BIT(0)

#define USB3CFG_PHY_PORT0_CONFIG0_OFFSET	0x10
#define USB3CFG_PHY_PORT1_CONFIG0_OFFSET	0x1c
#define USB3CFG_PHY_PORT_CKBUF_EN					BIT(31)
#define USB3CFG_PHY_PORT_MAC_PHY_PLL_EN_REG			BIT(30)
#define USB3CFG_PHY_PORT_MAC_PHY_RECV_DET_EN_REG	BIT(29)
#define USB3CFG_PHY_PORT_MAC_PHY_RECV_DET_REQ_REG	BIT(28)
#define USB3CFG_PHY_PORT_CLK_MODE_SEL_MASK(x)		(0x3 & (x))

#define USB3CFG_PHY_PORT0_STATUS0_OFFSET	0x14
#define USB3CFG_PHY_PORT1_STATUS0_OFFSET	0x20
#define USB3CFG_PHY_PORT_PHY_MAC_RECV_DET_END		BIT(21)
#define USB3CFG_PHY_PORT_PHY_MAC_RECV_DET_ACK		BIT(20)
#define USB3CFG_PHY_PORT_CLK_RDY					BIT(19)
#define USB3CFG_PHY_PORT_COUNT_NUM_VAL(x)			(0x7ffff & (x))

#define USB3CFG_PHY_PORT0_STATUS1_OFFSET	0x18
#define USB3CFG_PHY_PORT1_STATUS1_OFFSET	0x24
#define USB3CFG_PHY_PORT_DISPARITY_ERR_MASK(x)		((0xff & (x)) << 24)
#define USB3CFG_PHY_PORT_ELASTIC_BUF_UDF_MASK(x)	((0xff & (x)) << 16)
#define USB3CFG_PHY_PORT_ELASTIC_BUF_OVF_MASK(x)	((0xff & (x)) << 8)
#define USB3CFG_PHY_PORT_DECODE_ERR_MASK(x)			(0xff & (x))

struct phy_data {
	int size;
	u8 *addr;
	u16 *data;
};

struct ca77xx_usb3_phy {
	struct usb_phy	phy;
	struct device	*dev;
	void __iomem	*phy_regbase;
	int				 port_mask;
#if defined(CONFIG_ARCH_CORTINA_VENUS)
	struct gpio_desc		*u3port0_vbus;
	struct gpio_desc		*u3port1_vbus;
#endif
	struct phy_data *phy_data;
	void __iomem	*p0_regbase;
	void __iomem	*p1_regbase;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	struct gpio_desc		*phy_vbus;
#endif
	struct reset_control	*s2usb_dphy_reset;
	struct reset_control	*s3usb_dphy_reset;
	struct phy				*s2_phy;
	struct phy				*s3_phy;
	spinlock_t		lock;	/* lock access to bank regs */
};

static int debug = 0;

static inline u32 ca77xx_usb3_phy_read(struct ca77xx_usb3_phy *ca77xx_phy,
	u8 addr, int port_id)
{
	u32 data;
	u32 offset = addr << 2;

	if (port_id == 0)
		data = readl(ca77xx_phy->p0_regbase + offset);
	if (port_id == 1)
		data = readl(ca77xx_phy->p1_regbase + offset);
	if (debug) {
		dev_info(ca77xx_phy->dev, "Read port<%d>, offset=0x%04x, data=0x%08x\n",
			port_id, offset, data);
	}

	return data;
}

static inline void ca77xx_usb3_phy_write(struct ca77xx_usb3_phy *ca77xx_phy,
	u8 addr, u16 data, int port_id)
{
	u32 offset = addr << 2;

	if (port_id == 0)
		writel(data, ca77xx_phy->p0_regbase + offset);
	if (port_id == 1)
		writel(data, ca77xx_phy->p1_regbase + offset);
}

static void ca77xx_usb3_host_reset(struct ca77xx_usb3_phy *ca77xx_phy)
{
	int port_mask = ca77xx_phy->port_mask;
	u32 reg_val = 0;
	unsigned long flags;

	if (debug)
		dev_info(ca77xx_phy->dev, "%s()\n", __func__);
	/* DPhy reset */
	reset_control_assert(ca77xx_phy->s2usb_dphy_reset);
	msleep(20);
	reset_control_assert(ca77xx_phy->s3usb_dphy_reset);
	msleep(20);
	/* NOTE:
		On Venus platform, it should release S3 reset first,
		and then release S2 reset later.
		But G3/G3HGU don't have this requirement.
	*/
	if (port_mask & 0x2) {
		phy_power_on(ca77xx_phy->s3_phy);
		msleep(20);
	}
	if (port_mask & 0x1) {
		phy_power_on(ca77xx_phy->s2_phy);
		msleep(20);
	}
	reset_control_deassert(ca77xx_phy->s2usb_dphy_reset);
	msleep(20);
	reset_control_deassert(ca77xx_phy->s3usb_dphy_reset);
	msleep(20);

	/* For USB3.0 PHY RESET */
	spin_lock_irqsave(&ca77xx_phy->lock, flags);
	reg_val = readl(ca77xx_phy->phy_regbase + USB3CFG_CNTRL_OFFSET);
	reg_val |= USB3CFG_PHY_VAUX_RESET;
	writel(reg_val, ca77xx_phy->phy_regbase + USB3CFG_CNTRL_OFFSET);
	spin_unlock_irqrestore(&ca77xx_phy->lock, flags);
	if (debug) {
		dev_info(ca77xx_phy->dev, "read USB3CFG_CNTRL reg_val = 0x%08x",
			readl(ca77xx_phy->phy_regbase + USB3CFG_CNTRL_OFFSET));
	}
}

static int ca77xx_usb3_phy_init(struct usb_phy *phy)
{
	struct ca77xx_usb3_phy *ca77xx_phy = (struct ca77xx_usb3_phy *)phy;
	u8 *addr = ca77xx_phy->phy_data->addr;
	u16 *data = ca77xx_phy->phy_data->data;
	int size = ca77xx_phy->phy_data->size;
	int port_mask = ca77xx_phy->port_mask;
	int index;

	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	if (ca77xx_phy->phy_vbus != NULL) {
		gpiod_set_value(ca77xx_phy->phy_vbus, 0);
		msleep(100);
		gpiod_set_value(ca77xx_phy->phy_vbus, 1);
		msleep(100);
	}
#endif
	if (port_mask & 0x1) {
		for (index = 0; index < size; index++) {
			ca77xx_usb3_phy_write(ca77xx_phy, *(addr + index),
				*(data + index), 0);
			if (debug)
				(void)ca77xx_usb3_phy_read(ca77xx_phy, *(addr+index), 0);
		}
	}

	if (port_mask & 0x2) {
		for (index = 0; index < size; index++) {
			ca77xx_usb3_phy_write(ca77xx_phy, *(addr + index),
				*(data + index), 1);
			if (debug)
				(void)ca77xx_usb3_phy_read(ca77xx_phy, *(addr+index), 1);
		}
	}


	if (debug) {
		dev_info(phy->dev, "%s Initialized RTK USB 3.0 PHY\n",
			__func__);
	}
	return 0;
}

static void ca77xx_usb3_phy_shutdown(struct usb_phy *phy)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);
}

static int ca77xx_usb3_phy_set_suspend(struct usb_phy *phy,
	int suspend)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int ca77xx_usb3_phy_set_wakeup(struct usb_phy *phy,
	bool enabled)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int ca77xx_usb3_phy_set_vbus(struct usb_phy *phy, int on)
{
#if defined(CONFIG_ARCH_CORTINA_VENUS)
	struct ca77xx_usb3_phy *ca77xx_phy = (struct ca77xx_usb3_phy *)phy;
	int port_mask = ca77xx_phy->port_mask;

	if (on) { /* on == true */
		if (port_mask & 0x1)
			gpiod_set_value_cansleep(ca77xx_phy->u3port0_vbus, 0);
		if (port_mask & 0x2)
			gpiod_set_value_cansleep(ca77xx_phy->u3port1_vbus, 0);
	} else { /* on == false */
		if (port_mask & 0x1)
			gpiod_set_value_cansleep(ca77xx_phy->u3port0_vbus, 1);
		if (port_mask & 0x2)
			gpiod_set_value_cansleep(ca77xx_phy->u3port1_vbus, 1);
	}
	msleep(100);

	if (debug)
		dev_info(phy->dev, "%s() -- u3port0_vbus: %d, u3port1_vbus: %d\n", __func__,
			gpiod_get_value_cansleep(ca77xx_phy->u3port0_vbus),
			gpiod_get_value_cansleep(ca77xx_phy->u3port1_vbus));
#endif
	return 0;
}

static int ca77xx_usb3_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct ca77xx_usb3_phy *ca77xx_usb_phy;
	struct phy_data *phy_data;

	struct resource *res;
	int	ret = 0;

	if (debug)
		dev_info(dev, "Starting ca77xx USB 3.0 PHY Probe\n");

	ca77xx_usb_phy = devm_kzalloc(dev, sizeof(*ca77xx_usb_phy), GFP_KERNEL);
	if (IS_ERR(ca77xx_usb_phy))
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
				"u3host_reg");
	ca77xx_usb_phy->phy_regbase = devm_ioremap_resource(dev, res);
	if (IS_ERR(ca77xx_usb_phy->phy_regbase))
		return PTR_ERR(ca77xx_usb_phy->phy_regbase);
	dev_info(dev, "usb3_phy resource - %pr mapped at 0x%pK\n", res,
		ca77xx_usb_phy->phy_regbase);

#if defined(CONFIG_ARCH_CORTINA_VENUS)
	ca77xx_usb_phy->u3port0_vbus = devm_gpiod_get_optional(dev, "u3port0-vbus", GPIOD_OUT_HIGH);
	if (IS_ERR(ca77xx_usb_phy->u3port0_vbus))
		dev_info(dev, "USB3-port0(S2) VBUS GPIO Resource Get Failed and Just ignored\n");
	else
		dev_info(dev, "USB3-port0(S2) VBUS GPIO Resource Get O.K and u3port0_vbus:%d\n",
			gpiod_get_value_cansleep(ca77xx_usb_phy->u3port0_vbus));

	ca77xx_usb_phy->u3port1_vbus = devm_gpiod_get_optional(dev, "u3port1-vbus", GPIOD_OUT_HIGH);
	if (IS_ERR(ca77xx_usb_phy->u3port1_vbus))
		dev_info(dev, "USB3-port1(S3) VBUS GPIO Resource Get Failed and Just ignored\n");
	else
		dev_info(dev, "USB3-port1(S3) VBUS GPIO Resource Get O.K and u3port1_vbus:%d\n",
			gpiod_get_value_cansleep(ca77xx_usb_phy->u3port1_vbus));
#endif
#if defined(CONFIG_ARCH_CORITNA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	ca77xx_usb_phy->phy_vbus = devm_gpiod_get_optional(dev, "phy-vbus", GPIOD_OUT_LOW);
	if (IS_ERR(ca77xx_usb_phy->phy_vbus))
		dev_info(dev, "USB3 PHY VBUS GPIO Resource Get Failed and Just ignored\n");
	else
		dev_info(dev, "USB3 PHY VBUS GPIO Resource Get O.K and phy_vbus:%d\n",
			gpiod_get_value(ca77xx_usb_phy->phy_vbus));
#endif

	ca77xx_usb_phy->dev				= dev;
	ca77xx_usb_phy->phy.dev			= ca77xx_usb_phy->dev;
	ca77xx_usb_phy->phy.label		= CA77XX_USB3PHY_NAME;
	ca77xx_usb_phy->phy.init		= ca77xx_usb3_phy_init;
	ca77xx_usb_phy->phy.shutdown	= ca77xx_usb3_phy_shutdown;
	ca77xx_usb_phy->phy.set_suspend	= ca77xx_usb3_phy_set_suspend;
	ca77xx_usb_phy->phy.set_wakeup	= ca77xx_usb3_phy_set_wakeup;
	ca77xx_usb_phy->phy.set_vbus	= ca77xx_usb3_phy_set_vbus;
	ca77xx_usb_phy->phy.type		= USB_PHY_TYPE_USB3;

	spin_lock_init(&ca77xx_usb_phy->lock);

	phy_data = devm_kzalloc(dev, sizeof(*phy_data), GFP_KERNEL);
	if (IS_ERR(phy_data))
		return -ENOMEM;

	if (node) {
		ca77xx_usb_phy->s2usb_dphy_reset = of_reset_control_get(node,
			"s2usb3_dphy_reset");
		if (IS_ERR(ca77xx_usb_phy->s2usb_dphy_reset)) {
			ret = PTR_ERR(ca77xx_usb_phy->s2usb_dphy_reset);
			dev_err(dev, "Failed to get s2usb_dphy_reset.\n");
			return ret;
		}
		ca77xx_usb_phy->s2_phy = devm_phy_get(dev, "s2usb3-phy");
		if (IS_ERR(ca77xx_usb_phy->s2_phy)) {
			ret = PTR_ERR(ca77xx_usb_phy->s2_phy);
			dev_err(dev, "Failed to get s2usb-phy.\n");
			return ret;
		}

		ca77xx_usb_phy->s3usb_dphy_reset = of_reset_control_get(node,
			"s3usb3_dphy_reset");
		if (IS_ERR(ca77xx_usb_phy->s3usb_dphy_reset)) {
			ret = PTR_ERR(ca77xx_usb_phy->s3usb_dphy_reset);
			dev_err(dev, "Failed to get s3usb_dphy_reset.\n");
			return ret;
		}
		ca77xx_usb_phy->s3_phy = devm_phy_get(dev, "s3usb3-phy");
		if (IS_ERR(ca77xx_usb_phy->s3_phy)) {
			ret = PTR_ERR(ca77xx_usb_phy->s3_phy);
			dev_err(dev, "Failed to get s3usb-phy.\n");
			return ret;
		}

		ret = of_property_read_u32_index(node, "portmask", 0,
			&ca77xx_usb_phy->port_mask);
		if (ret)
			goto err;

		ret = of_property_read_u32_index(node, "phy_data_size", 0,
			&phy_data->size);
		if (ret)
			goto err;
		phy_data->addr = devm_kzalloc(dev, sizeof(u8)*phy_data->size,
			GFP_KERNEL);
		if (!phy_data->addr)
			return -ENOMEM;
		phy_data->data = devm_kzalloc(dev, sizeof(u16)*phy_data->size,
			GFP_KERNEL);
		if (!phy_data->data)
			return -ENOMEM;
		ret = of_property_read_u8_array(node, "phy_data_addr",
			phy_data->addr, phy_data->size);
		if (ret)
			goto err;
		ret = of_property_read_u16_array(node, "phy_data_array",
			phy_data->data, phy_data->size);
		if (ret)
			goto err;
		ca77xx_usb_phy->phy_data = phy_data;
	}

	/* Enable S2 for USB3.0 port0 */
	if (ca77xx_usb_phy->port_mask & 0x1) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					"s2usbphy_u3port0");
		ca77xx_usb_phy->p0_regbase = devm_ioremap_resource(dev, res);
		if (IS_ERR(ca77xx_usb_phy->p0_regbase))
			return PTR_ERR(ca77xx_usb_phy->p0_regbase);
		dev_info(dev, "Enabled S2/port0 resource - %pr mapped at 0x%pK\n", res,
			ca77xx_usb_phy->p0_regbase);
	}

	/* Enable S3 for USB3.0 port1 */
	if (ca77xx_usb_phy->port_mask & 0x2) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					"s3usbphy_u3port1");
		ca77xx_usb_phy->p1_regbase = devm_ioremap_resource(dev, res);
		if (IS_ERR(ca77xx_usb_phy->p1_regbase))
			return PTR_ERR(ca77xx_usb_phy->p1_regbase);
		dev_info(dev, "Enabled S3/port1 resource - %pr mapped at 0x%pK\n", res,
			ca77xx_usb_phy->p1_regbase);
	}

	platform_set_drvdata(pdev, ca77xx_usb_phy);
	ret = usb_add_phy_dev(&ca77xx_usb_phy->phy);
	if (ret)
		goto err;

	/* dphy reset and AUX bus reset */
	ca77xx_usb3_host_reset(ca77xx_usb_phy);

	if (debug)
		dev_info(dev, "Finished ca77xx USB 3.0 PHY Probe\n");
err:
	return ret;
}

static int ca77xx_usb3_phy_remove(struct platform_device *pdev)
{
	struct ca77xx_usb3_phy *ca77xx_usb_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&ca77xx_usb_phy->phy);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id usbphy_ca77xx_dt_match[] = {
	{ .compatible = "cortina,ca77xx-usb3phy", },
	{},
};
MODULE_DEVICE_TABLE(of, usbphy_ca77xx_dt_match);
#endif

static struct platform_driver ca77xx_usb3_phy_driver = {
	.probe		= ca77xx_usb3_phy_probe,
	.remove		= ca77xx_usb3_phy_remove,
	.driver		= {
		.name	= CA77XX_USB3PHY_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(usbphy_ca77xx_dt_match),
	},
};
module_platform_driver(ca77xx_usb3_phy_driver);

MODULE_DESCRIPTION("Cortina-Access CA77XX for USB3port phy driver");
MODULE_ALIAS("platform:" CA77XX_USB3PHY_NAME);
MODULE_LICENSE("GPL v2");
