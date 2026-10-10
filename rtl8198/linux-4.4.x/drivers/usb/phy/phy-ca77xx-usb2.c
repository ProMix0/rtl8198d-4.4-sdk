/*
 * CA77XX USB2.0 PHY Device Driver for dwc3 core controller on
 * Cortina-Access CA77XX SoCs
 *
 * Copyright (C) 2017 Cortina Access, Inc.
 *		 http://www.cortina-access.com
 *
 * Based on phy-rtk-usb2.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/usb/otg.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/reset-controller.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/reset.h>

#define CA77XX_USB2PHY_NAME "ca77xx-usb2phy"

/* USB2(USB High Speed) register offsets */
#define USB2CFG_CNTRL_OFFSET				0x00
#define USB2CFG_PHY_VAUX_RESET					BIT(31)
#define USB2CFG_BUS_CLKEN_GLAVE					BIT(30)
#define USB2CFG_BUS_CLKEN_GMASTER				BIT(29)
#define USB2CFG_BIGENDIAN_GSLAVE				BIT(28)
#define USB2CFG_HOST_PORT_POWER_CTRL_PRESENT	BIT(27)
#define USB2CFG_HOST_MSI_ENABLE					BIT(26)
#define USB2CFG_HOST_LEGACY_SMI_PCI_CMD_REG_WR	BIT(25)
#define USB2CFG_HOST_LEGACY_SMI_BAR_WR			BIT(24)
#define USB2CFG_FLADJ_30MHZ_REG_MASK(x)			((0x3f & (x)) << 16)
#define USB2CFG_PHY_VAUX_RESET_PORT2            BIT(10)
#define USB2CFG_PHY_VAUX_RESET_PORT1            BIT(9)
#define USB2CFG_PHY_VAUX_RESET_PORT0            BIT(8)
#define USB2CFG_XHCI_BUS_MASTER_ENABLE			BIT(4)

#define USB2CFG_STATUS_OFFSET				0x04
#define USB2CFG_HOST_SYSTEM_ERR					BIT(31)
#define USB2CFG_LEGACY_SMI_INTERRUPT			BIT(16)
#define USB2CFG_HOST_CURRENT_BELT_MASK(x)		(0xfff & (x))

#define USB2CFG_PORT_CONFIG_OFFSET			0x08
#define USB2CFG_HOST_DISABLE					BIT(2)
#define USB2CFG_HUB_PORT_OVERCURRENT			BIT(1)
#define USB2CFG_HUB_PORT_PERM_ATTACH			BIT(0)

#define USB2CFG_PORT_STATUS_OFFSET			0x0c
#define USB2CFG_UTMI_FSLS_LOW_POWER				BIT(1)
#define USB2CFG_HUB_VBUS_CTRL					BIT(0)

#define USB2CFG_PHY_PORT0_CONFIG_OFFSET		0x10
#define USB2CFG_PHY_PORT1_CONFIG_OFFSET		0x18
#define USB2CFG_PHY_PORT2_CONFIG_OFFSET		0x20
#define USB2CFG_PHY_PORT_VSTATUS_IN_MASK(x)		((0xff & (x)) << 24)
#define USB2CFG_PHY_PORT_VCNTRL_MASK(x)		((0xf & (x)) << 20)
#define USB2CFG_PHY_PORT_VLOADM					BIT(19)
#define USB2CFG_PHY_PORT_BY_PASS_ON				BIT(18)
#define USB2CFG_PHY_PORT_LF_PD_R_EN				BIT(9)
#define USB2CFG_PHY_PORT_CLKTSTEN				BIT(8)
#define USB2CFG_PHY_PORT_DPPULLDOWN				BIT(6)
#define USB2CFG_PHY_PORT_DMPULLDOWN				BIT(5)
#define USB2CFG_PHY_PORT_TXBITSTUFF_ENABLE		BIT(4)
#define USB2CFG_PHY_PORT_TXBITSTUFF_ENABLE_H	BIT(3)
#define USB2CFG_PHY_PORT_TX_ENABLE_N			BIT(2)
#define USB2CFG_PHY_PORT_TX_DAT					BIT(1)
#define USB2CFG_PHY_PORT_TX_SE0					BIT(0)

#define USB2CFG_PHY_PORT0_STATUS_OFFSET		0x14
#define USB2CFG_PHY_PORT1_STATUS_OFFSET		0x1c
#define USB2CFG_PHY_PORT2_STATUS_OFFSET		0x24
#define USB2CFG_PHY_PORT_VSTATUS_OUT_MASK(x)	((0xff & (x)) << 24)
#define USB2CFG_PHY_PORT_DEBUG_MASK(x)			(0xff & (x))

struct phy_data {
	int size;
	u8 *addr;
	u8 *data;
};

struct ca77xx_usb2_phy {
	struct usb_phy	phy;
	struct device	*dev;
	struct phy_data *phy_data;
	void __iomem	*phy_regbase;
	int port_num;
	struct reset_control	*usbcore_reset; /* reset xhci only once */
#if defined(CONFIG_ARCH_CORTINA_VENUS)
    struct gpio_desc        *u2port0_vbus;
#endif
	spinlock_t		lock;	/* lock access to bank regs */
};

static int debug = 0;

static void ca77xx_usb2_host_reset(struct ca77xx_usb2_phy *ca77xx_phy)
{
	u32 reg_val = 0, reg_ctl = 0;
	unsigned long flags;
	int port_num = ca77xx_phy->port_num;

	if (debug)
		dev_info(ca77xx_phy->dev, "%s()\n", __func__);

	reset_control_assert(ca77xx_phy->usbcore_reset);
	msleep(20);
	reset_control_deassert(ca77xx_phy->usbcore_reset);
	msleep(20);

	/* For USB2.0 PHY PORT RESET */
	spin_lock_irqsave(&ca77xx_phy->lock, flags);
	reg_ctl = readl(ca77xx_phy->phy_regbase + USB2CFG_CNTRL_OFFSET);
	reg_ctl |= USB2CFG_PHY_VAUX_RESET;
	if (port_num > 3) {
		dev_info(ca77xx_phy->dev, "The usb2phy port num is unvalid.\n");
	}
	else {
		if (port_num > 0) {
			reg_val = readl(ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT0_CONFIG_OFFSET);
			reg_val |= 0x00080064;
			writel(reg_val, ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT0_CONFIG_OFFSET);
			if (debug) {
				dev_info(ca77xx_phy->dev, "read USB2CFG_PHY_PORT0_CONFIG reg_val = 0x%08x\n",
					readl(ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT0_CONFIG_OFFSET));
			}
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			reg_ctl |= USB2CFG_PHY_VAUX_RESET_PORT0;
#endif
		}
		if (port_num > 1) {
			reg_val = readl(ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT1_CONFIG_OFFSET);
			reg_val |= 0x00080064;
			writel(reg_val, ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT1_CONFIG_OFFSET);
			if (debug) {
				dev_info(ca77xx_phy->dev, "read USB2CFG_PHY_PORT1_CONFIG reg_val = 0x%08x\n",
					readl(ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT1_CONFIG_OFFSET));
			}
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			reg_ctl |= USB2CFG_PHY_VAUX_RESET_PORT1;
#endif
		}
		if (port_num > 2) {
			reg_val = readl(ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT2_CONFIG_OFFSET);
			reg_val |= 0x00080064;
			writel(reg_val, ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT2_CONFIG_OFFSET);
			if (debug) {
				dev_info(ca77xx_phy->dev, "read USB2CFG_PHY_PORT2_CONFIG reg_val = 0x%08x\n",
					readl(ca77xx_phy->phy_regbase + USB2CFG_PHY_PORT2_CONFIG_OFFSET));
			}
#if defined(CONFIG_ARCH_CORTINA_VENUS)
			reg_ctl |= USB2CFG_PHY_VAUX_RESET_PORT2;
#endif
		}
	}
	writel(reg_ctl, ca77xx_phy->phy_regbase + USB2CFG_CNTRL_OFFSET);
	spin_unlock_irqrestore(&ca77xx_phy->lock, flags);
	if (debug) {
		dev_info(ca77xx_phy->dev, "read USB2CFG_CNTRL reg_val = 0x%08x\n",
			readl(ca77xx_phy->phy_regbase + USB2CFG_CNTRL_OFFSET));
	}
}

static void ca77xx_usb2_phy_port_cfg(struct ca77xx_usb2_phy *ca77xx_phy,
	u8 vstat_in_mask, u8 vctrl_mask, int port_id)
{
	u32 offset = 0;
	u32 reg_val = 0;
	u8 vctrl_mask1, vctrl_mask2;

	vctrl_mask1 = vctrl_mask & 0x0F;
	vctrl_mask2 = (vctrl_mask >> 4) & 0x0F;
	reg_val |= (USB2CFG_PHY_PORT_VSTATUS_IN_MASK(vstat_in_mask)
		| USB2CFG_PHY_PORT_VCNTRL_MASK(vctrl_mask1)
		| USB2CFG_PHY_PORT_DPPULLDOWN
		| USB2CFG_PHY_PORT_DMPULLDOWN
		| USB2CFG_PHY_PORT_VLOADM
		| USB2CFG_PHY_PORT_TX_ENABLE_N);

	if (port_id == 0)
		offset = USB2CFG_PHY_PORT0_CONFIG_OFFSET;
	else if (port_id == 1)
		offset = USB2CFG_PHY_PORT1_CONFIG_OFFSET;
	else if (port_id == 2)
		offset = USB2CFG_PHY_PORT2_CONFIG_OFFSET;
	else
		dev_info(ca77xx_phy->dev, "The usb2phy port num is unvalid.\n");

	writel(reg_val, ca77xx_phy->phy_regbase + offset);
#if 1
	if (debug)
		dev_info(ca77xx_phy->dev, "1read U2PHY_P%d_CFG reg_val = 0x%08x",
			port_id, readl(ca77xx_phy->phy_regbase + offset));
#endif
	reg_val &= ~USB2CFG_PHY_PORT_VLOADM;
	writel(reg_val, ca77xx_phy->phy_regbase + offset);
#if 1
	if (debug)
		dev_info(ca77xx_phy->dev, "2read U2PHY_P%d_CFG reg_val = 0x%08x",
			port_id, readl(ca77xx_phy->phy_regbase + offset));
#endif
	reg_val |= USB2CFG_PHY_PORT_VLOADM;
	writel(reg_val, ca77xx_phy->phy_regbase + offset);
#if 1
	if (debug)
		dev_info(ca77xx_phy->dev, "3read U2PHY_P%d_CFG reg_val = 0x%08x",
			port_id, readl(ca77xx_phy->phy_regbase + offset));
#endif
	/* Clear VCONTROL field before refilling */
	reg_val &= ~USB2CFG_PHY_PORT_VCNTRL_MASK(0xF);
	reg_val |= USB2CFG_PHY_PORT_VCNTRL_MASK(vctrl_mask2);
	writel(reg_val, ca77xx_phy->phy_regbase + offset);
#if 1
	if (debug)
		dev_info(ca77xx_phy->dev, "4read U2PHY_P%d_CFG reg_val = 0x%08x",
			port_id, readl(ca77xx_phy->phy_regbase + offset));
#endif
	reg_val &= ~USB2CFG_PHY_PORT_VLOADM;
	writel(reg_val, ca77xx_phy->phy_regbase + offset);
#if 1
	if (debug)
		dev_info(ca77xx_phy->dev, "5read U2PHY_P%d_CFG reg_val = 0x%08x",
			port_id, readl(ca77xx_phy->phy_regbase + offset));
#endif
	reg_val |= USB2CFG_PHY_PORT_VLOADM;
	writel(reg_val, ca77xx_phy->phy_regbase + offset);
#if 1
	if (debug)
		dev_info(ca77xx_phy->dev, "6read U2PHY_P%d_CFG reg_val = 0x%08x",
			port_id, readl(ca77xx_phy->phy_regbase + offset));
#endif
}

static int ca77xx_usb2_phy_init(struct usb_phy *phy)
{
	struct ca77xx_usb2_phy *ca77xx_phy = (struct ca77xx_usb2_phy *)phy;
	u8 *addr = ca77xx_phy->phy_data->addr;
	u8 *data = ca77xx_phy->phy_data->data;
	int size = ca77xx_phy->phy_data->size;
	int port_num = ca77xx_phy->port_num;
	int index, port_id;

	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	/* USB2 PHY port[0:3] calibration and initialization */
	for (port_id = 0; port_id < port_num; port_id++) {
		for (index = 0; index < size; index++) {
			ca77xx_usb2_phy_port_cfg(ca77xx_phy, *(data + index),
				*(addr + index), port_id);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
			/* Verify bug for rev-D chip for USB2 Port2 init */
			if (index == 11 && port_id == 1) {
				/* 20190917, Wilson/CA suggention use 0x8B
				ca77xx_usb2_phy_port_cfg(ca77xx_phy, 0x8D,
				*/
				ca77xx_usb2_phy_port_cfg(ca77xx_phy, 0x8B,
					*(addr + index), port_id);
			}
#endif
		}
	}
	if (debug) {
		dev_info(phy->dev, "%s Initialized RTK USB 2.0 PHY\n",
			__func__);
	}
	return 0;
}

static void ca77xx_usb2_phy_shutdown(struct usb_phy *phy)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);
}

static int ca77xx_usb2_phy_set_suspend(struct usb_phy *phy,
	int suspend)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int ca77xx_usb2_phy_set_wakeup(struct usb_phy *phy,
	bool enabled)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int ca77xx_usb2_phy_set_vbus(struct usb_phy *phy,
	int on)
{
#if defined(CONFIG_ARCH_CORTINA_VENUS)
	struct ca77xx_usb2_phy *ca77xx_phy = (struct ca77xx_usb2_phy *)phy;

	if (on) { /* on == true */
		gpiod_set_value_cansleep(ca77xx_phy->u2port0_vbus, 0);
	} else { /* on == false */
		gpiod_set_value_cansleep(ca77xx_phy->u2port0_vbus, 1);
	}
	msleep(100);

	if (debug)
		dev_info(phy->dev, "%s() -- u2port0_vbus: %d", __func__,
			gpiod_get_value_cansleep(ca77xx_phy->u2port0_vbus));
#endif
	return 0;
}

static int ca77xx_usb2_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct ca77xx_usb2_phy *ca77xx_usb_phy;
	struct phy_data *phy_data;
	struct resource *res;
	int ret = 0;

	if (debug)
		dev_info(dev, "Starting ca77xx USB 2.0 PHY Probe\n");

	ca77xx_usb_phy = devm_kzalloc(dev, sizeof(*ca77xx_usb_phy), GFP_KERNEL);
	if (IS_ERR(ca77xx_usb_phy))
		return -ENOMEM;
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
				"u2host_reg");
	ca77xx_usb_phy->phy_regbase = devm_ioremap_resource(dev, res);
	if (IS_ERR(ca77xx_usb_phy->phy_regbase))
		return PTR_ERR(ca77xx_usb_phy->phy_regbase);
	dev_info(dev, "usb2_phy resource - %pr mapped at 0x%pK\n", res,
		ca77xx_usb_phy->phy_regbase);

#if defined(CONFIG_ARCH_CORTINA_VENUS)
    ca77xx_usb_phy->u2port0_vbus = devm_gpiod_get_optional(dev, "u2port0-vbus", GPIOD_OUT_HIGH);
    if (IS_ERR(ca77xx_usb_phy->u2port0_vbus))
        dev_info(dev, "USB2-port0 VBUS GPIO Resource Get Failed and Just ignored\n");
	else
        dev_info(dev, "USB2-port0 VBUS GPIO Resource Get O.K and u2port0_vbus:%d\n",
			gpiod_get_value_cansleep(ca77xx_usb_phy->u2port0_vbus));
#endif

	ca77xx_usb_phy->dev				= dev;
	ca77xx_usb_phy->phy.dev			= ca77xx_usb_phy->dev;
	ca77xx_usb_phy->phy.label		= CA77XX_USB2PHY_NAME;
	ca77xx_usb_phy->phy.init		= ca77xx_usb2_phy_init;
	ca77xx_usb_phy->phy.shutdown	= ca77xx_usb2_phy_shutdown;
	ca77xx_usb_phy->phy.set_suspend	= ca77xx_usb2_phy_set_suspend;
	ca77xx_usb_phy->phy.set_wakeup	= ca77xx_usb2_phy_set_wakeup;
	ca77xx_usb_phy->phy.set_vbus	= ca77xx_usb2_phy_set_vbus;
	ca77xx_usb_phy->phy.type        = USB_PHY_TYPE_USB2;

	spin_lock_init(&ca77xx_usb_phy->lock);

	phy_data = devm_kzalloc(dev, sizeof(*phy_data), GFP_KERNEL);
	if (IS_ERR(phy_data))
		return -ENOMEM;

	if (node) {
		ca77xx_usb_phy->usbcore_reset = of_reset_control_get(node,
			"usbcore_reset");
		if (IS_ERR(ca77xx_usb_phy->usbcore_reset)) {
			ret = PTR_ERR(ca77xx_usb_phy->usbcore_reset);
			dev_err(dev, "Failed to get usbcore_reset.\n");
			return ret;
		}

		ret = of_property_read_u32_index(node, "portnum", 0,
			&ca77xx_usb_phy->port_num);
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
		phy_data->data = devm_kzalloc(dev, sizeof(u8)*phy_data->size,
			GFP_KERNEL);
		if (!phy_data->data)
			return -ENOMEM;
		ret = of_property_read_u8_array(node, "phy_data_addr",
			phy_data->addr, phy_data->size);
		if (ret)
			goto err;
		ret = of_property_read_u8_array(node, "phy_data_array",
			phy_data->data, phy_data->size);
		if (ret)
			goto err;
		ca77xx_usb_phy->phy_data = phy_data;
	}
	platform_set_drvdata(pdev, ca77xx_usb_phy);

	ret = usb_add_phy_dev(&ca77xx_usb_phy->phy);
	if (ret)
		goto err;

	/* dphy reset and AUX bus reset for usb core (xhci controller)  */
	ca77xx_usb2_host_reset(ca77xx_usb_phy);

	if (debug)
		dev_info(dev, "Finished ca77xx USB 2.0 PHY Probe\n");
err:
	return ret;
}

static int ca77xx_usb2_phy_remove(struct platform_device *pdev)
{
	struct ca77xx_usb2_phy *ca77xx_usb_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&ca77xx_usb_phy->phy);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id usbphy_ca77xx_dt_match[] = {
	{ .compatible = "cortina,ca77xx-usb2phy", },
	{},
};
MODULE_DEVICE_TABLE(of, usbphy_ca77xx_dt_match);
#endif

static struct platform_driver ca77xx_usb2_phy_driver = {
	.probe		= ca77xx_usb2_phy_probe,
	.remove		= ca77xx_usb2_phy_remove,
	.driver		= {
		.name	= CA77XX_USB2PHY_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(usbphy_ca77xx_dt_match),
	},
};
module_platform_driver(ca77xx_usb2_phy_driver);

MODULE_DESCRIPTION("Cortina-Access CA77XX for USB2port phy driver");
MODULE_ALIAS("platform:" CA77XX_USB2PHY_NAME);
MODULE_LICENSE("GPL v2");
