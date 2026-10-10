/*
 * RTL8198F USB2.0 PHY Device Driver for dwc2 core controller on
 * Cortina-Access rtl8198f SoCs
 *
 * Copyright (C) 2017 Realtek
 *
 * Based on phy-rtk-usb2.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/usb/otg.h>
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
#include <soc/cortina/g3lite_registers.h>
#include <linux/string.h>

static int debug = 0;

#define RTL8198F_USB3PHY_NAME "rtl8198f-usb3phy"

#define U2PHY_PORT0_CONFIG_OFFSET	0x10
#define U2PHY_PORT0_STATUS_OFFSET	0x14
#define U2PHY_PORT1_CONFIG_OFFSET	0x18
#define U2PHY_PORT1_STATUS_OFFSET	0x1c

static struct phy_data {
	int size;
	u8 *addr;
	u8 *data;
};

struct rtl8198f_usb3_phy {
	struct usb_phy	phy;
	struct device	*dev;
	struct phy_data *phy_data;
	void __iomem	*phy_regbase;
	void __iomem	*u2_phy_rebase;
	void __iomem	*u3_phy_rebase;
	int port_num;
	struct reset_control	*usbblock_reset;
	struct reset_control	*s2usb_dphy_reset;
	struct reset_control	*strap_cpu_wdata;
	struct phy              *s2_phy;
	spinlock_t		lock;	/* lock access to bank regs */
};

static u32 rtl8198f_u3_phy_get(struct rtl8198f_usb3_phy *rtl8198f_phy, u32 reg,
	u8 page)
{
	u32 phy_reg;
	if (page == 0)
		phy_reg = rtl8198f_phy->u3_phy_rebase;
	else if (page ==1)
		phy_reg = rtl8198f_phy->u3_phy_rebase + 0x80;

	phy_reg += (reg << 2);
	
	return readl(phy_reg);
}

static void rtl8198f_u3_phy_set(struct rtl8198f_usb3_phy *rtl8198f_phy, u32 reg,
	u32 data, u8 page)
{
	u32 phy_reg;
	if (page == 0)
		phy_reg = rtl8198f_phy->u3_phy_rebase;
	else if (page ==1)
		phy_reg = rtl8198f_phy->u3_phy_rebase + 0x80;

	phy_reg += (reg << 2);

	writel(data, phy_reg);

	/*printk("rtl8198f_u3_phy_set 0x%x, 0x%x\n",
		data, rtl8198f_u3_phy_get(rtl8198f_phy, reg, page));*/
	return;
}

static u8 rtl8198f_usb2_phy_read(struct rtl8198f_usb3_phy *rtl8198f_phy, u8 reg,
	int port_id)
{
	u32 u2_port_control;
	u32 u2_port_status;
	u32 u2_port_readdata;
	u32 reg_low;
	u32 reg_high;
	u8 val;

	if (port_id == 0) {
		/* 0xf432b038 */
		u2_port_control = rtl8198f_phy->u2_phy_rebase + U2PHY_PORT0_CONFIG_OFFSET;
		/* 0xf432b03c */
		u2_port_status= rtl8198f_phy->u2_phy_rebase + U2PHY_PORT0_STATUS_OFFSET;
	} else if (port_id == 1) {
		/* 0xf432b040 */
		u2_port_control = rtl8198f_phy->u2_phy_rebase + U2PHY_PORT1_CONFIG_OFFSET;
		/* 0xf432b044 */
		u2_port_status= rtl8198f_phy->u2_phy_rebase + U2PHY_PORT1_STATUS_OFFSET;
	}

	reg &= ~(0x20);

	u2_port_readdata = readl(u2_port_control);
	u2_port_readdata &= 0xff0fffff;
	reg_low = reg;
	reg_low &= 0x0f;
	u2_port_readdata &= 0xff0fffff;
	u2_port_readdata |= reg_low << 20;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata &= 0xfff7ffff;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata |= 0x00080000;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	reg_high = reg;
	reg_high &= 0xf0;
	reg_high = reg_high >> 4;
	u2_port_readdata &= 0xff0fffff;
	u2_port_readdata |= reg_high << 20;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata &= 0xfff7ffff;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata |= 0x00080000;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata = readl(u2_port_status);
	mdelay(1);
	u2_port_readdata = u2_port_readdata >> 24;
	val = (u8) u2_port_readdata;
	
	return val;
}

static void rtl8198f_usb3_host_reset(struct rtl8198f_usb3_phy *rtl8198f_phy)
{
	dev_info(rtl8198f_phy->dev, "%s \n",__func__);
#if 0
	u32 reg_val;
	unsigned long flags;

	if (debug)
		dev_info(rtl8198f_phy->dev, "%s()\n", __func__);

	reset_control_assert(rtl8198f_phy->s2usb_dphy_reset);
	mdelay(1);
	reset_control_assert(rtl8198f_phy->usbblock_reset);
	mdelay(1);
	phy_power_on(rtl8198f_phy->s2_phy);
	mdelay(1);
	reset_control_deassert(rtl8198f_phy->s2usb_dphy_reset);
	mdelay(1);
	reset_control_deassert(rtl8198f_phy->usbblock_reset);
	mdelay(1);

	/* For USB2.0 PHY RESET */
	spin_lock_irqsave(&rtl8198f_phy->lock, flags);

	reg_val = readl(rtl8198f_phy->phy_regbase + USB2CFG_CONTROL_OFFSET);
	reg_val |= USB2CFG_PHY_VAUX_RESET;
	writel(reg_val, rtl8198f_phy->phy_regbase + USB2CFG_CONTROL_OFFSET);

	spin_unlock_irqrestore(&rtl8198f_phy->lock, flags);
	if (debug) {
		dev_info(rtl8198f_phy->dev, "read USB2CFG_CONTROL reg_val = 0x%08x",
			readl(rtl8198f_phy->phy_regbase + USB2CFG_CONTROL_OFFSET));
	}
#endif
}

static void rtl8198f_usb2_phy_write(struct rtl8198f_usb3_phy *rtl8198f_phy,
	u8 val, u8 reg, int port_id)
{
	u32 u2_port_control;
	u32 u2_port_readdata;
	u32 reg_low;
	u32 reg_high;

	if (port_id == 0) {
		/* 0xf432b038 */
		u2_port_control = rtl8198f_phy->u2_phy_rebase + U2PHY_PORT0_CONFIG_OFFSET;
	} else if (port_id == 1) {
		/* 0xf432b040 */
		u2_port_control = rtl8198f_phy->u2_phy_rebase + U2PHY_PORT1_CONFIG_OFFSET;
	}

	u2_port_readdata = readl(u2_port_control);
	u2_port_readdata &= 0x000fffff;
	reg_low = reg;
	reg_low &= 0x0f;
	u2_port_readdata |= reg_low << 20;

	u2_port_readdata |= val<<24;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata &= 0xfff7ffff;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata |= 0x00080000;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);

	u2_port_readdata &= 0xff0fffff;
	reg_high = reg;
	reg_high &= 0xf0;
	reg_high = reg_high >> 4;
	u2_port_readdata |= reg_high << 20;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata &= 0xfff7ffff;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);
	u2_port_readdata |= 0x00080000;
	writel(u2_port_readdata, u2_port_control);
	mdelay(1);

	if (debug)
		dev_info(rtl8198f_phy->dev,"set 0x%x = 0x%x ; read 0x%x\n", reg, val, 
			rtl8198f_usb2_phy_read(rtl8198f_phy, reg, port_id));
}

static int rtl8198f_usb3_phy_init(struct usb_phy *phy)
{
	struct rtl8198f_usb3_phy *rtl8198f_phy = (struct rtl8198f_usb3_phy *) phy;
	u8 *addr = rtl8198f_phy->phy_data->addr;
	u8 *data = rtl8198f_phy->phy_data->data;
	int size = rtl8198f_phy->phy_data->size;
	int port_num = rtl8198f_phy->port_num;
	int index, port_id;
	rtl8198f_phy->phy_regbase = phy->io_priv;
	
	dev_info(phy->dev, "%s()\n",__func__);

	for (port_id = 0; port_id < port_num; port_id++) {
		if (debug)
			dev_info(phy->dev, "port %d\n",port_id);
		for (index = 0; index < size; index++) {
			/* USB2 PHY calibration and initialization */
			rtl8198f_usb2_phy_write(rtl8198f_phy, *(data + index),
				*(addr + index), port_id);
		}
	}

	/* for rtl8198fb type-c */
	if ((readl(RTL8198F_GLB_REG_ADDR(GLOBAL_BONDING_OPTION)) & 0x3) == 2) {
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x0e, 0x3000, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x01, 0xe059, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x08, 0x4f51, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x1b, 0xff04, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x23, 0xad60, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x20, 0x54aa, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x0c, 0x0000, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x07, 0x00f3, 1);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x09, 0x525c, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x09, 0x505c, 0);
		rtl8198f_u3_phy_set(rtl8198f_phy, 0x09, 0x525c, 0);
	}
	
	if (debug) {
		dev_info(phy->dev, "%s Initialized RTK USB 3.0 PHY\n",
			__func__);
	}
	return 0;
}

static void rtl8198f_usb3_phy_shutdown(struct usb_phy *phy)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);
}

static int rtl8198f_usb3_phy_set_suspend(struct usb_phy *phy,
	int suspend)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int rtl8198f_usb3_phy_set_wakeup(struct usb_phy *phy,
	bool enabled)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int rtl8198f_usb3_phy_set_vbus(struct usb_phy *phy,
	int on)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

struct rtl8198f_usb3 {
		void __iomem            *global_usb2phy_cfg;
		void __iomem            *usbcfg_u3_control;
		void __iomem			*usbcfg_typec_ctl;
		void __iomem			*hcd;

		struct reset_control    *core_reset;
		struct reset_control    *usb3_isolate;
		struct reset_control    *pow_usb3;
		struct reset_control    *u3_bg_en;
		struct reset_control    *u3_mbias_en;
		struct reset_control    *u3_suspend_clk;
};

#define ID_RTL8198FH	3
#define	ID_RTL8198FB	2
#define OFFSET_USB_TYPEC_CTRL_CC1_0	0
#define OFFSET_USB_TYPEC_CTRL_CC2_0	0x8
#define OFFSET_USB_TYPEC_STS		0x10
#define OFFSET_USB_TYPEC_CTRL		0x14
#define OFFSET_CC_DET_INT		0x18
#define OFFSET_CC_DET_INT_EN		0x1c

/* for USB3 type-c support */
irqreturn_t xhci_handle_typec_intr(int irq, void *dev)
{
	struct rtl8198f_usb3 *rtl8198f_usb3 = dev;
	unsigned int cc_status,cc_int;
	unsigned int typec_control;
	//printk("xhci_handle_typec_intr\n");
	/* disable interrupt */
	writel(0, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_CC_DET_INT_EN);
	cc_int = readl(rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_CC_DET_INT);
	cc_status = readl(rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_USB_TYPEC_STS);

	if ((cc_status & 0x4) == 0) {
		typec_control = readl(rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_USB_TYPEC_CTRL_CC1_0);
		/* clear tx0 and rx0 */
		typec_control &= 0xcfffffff; 
		/* enable switch select tx0 and rx0 */
		writel((typec_control | (1<<29)) & ~(3<<27) 
			, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_USB_TYPEC_CTRL_CC1_0);
	}

	if ((cc_status & 0x20) == 0) {
		typec_control = readl(rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_USB_TYPEC_CTRL_CC1_0);
		/* enable switch select tx0 and rx0 */
		writel((typec_control | (1<<29)) | (3<<27), 
			rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_USB_TYPEC_CTRL_CC1_0);
	}

	/* clear interrupt */
	writel(cc_int, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_CC_DET_INT);
	/* enable imr */
	writel(0x3, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_CC_DET_INT_EN);

	return 0;
}

struct rtl8198f_usb3_phy *rtl8198f_u3_phy;

static char* next_arg(const char *argv)
{
	while (*argv != 32) {
		argv ++;
	} 
	argv ++;
	return argv;
}

static ssize_t user_set_u3_phy_info(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	printk("usage :\n");
	printk("echo [reg] [data] [page] > /sys/devices/platform/f432b028.usb3_phy/set_u3_phy\n");
	return 1;
}

static ssize_t user_set_u3_phy(struct device *dev, struct device_attribute *attr,
		     const char * argv, size_t count)
{
	unsigned int reg = simple_strtoul(argv, NULL, 16);
	argv = next_arg(argv);
	unsigned int data = simple_strtoul(argv, NULL, 16);
	argv = next_arg(argv);
	unsigned int page = simple_strtoul(argv, NULL, 16);

	rtl8198f_u3_phy_set(rtl8198f_u3_phy, reg, data, page);
	printk("set phy page %d 0x%x = 0x%x, read 0x%x\n", page, reg, data, 
		rtl8198f_u3_phy_get(rtl8198f_u3_phy, reg, page));
	
	return count;
}

static DEVICE_ATTR(set_u3_phy, S_IRUGO | S_IWUSR, user_set_u3_phy_info, user_set_u3_phy);

static ssize_t user_get_u3_phy_info(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	printk("usage :\n");
	printk("echo [reg] [page] > /sys/devices/platform/f432b028.usb3_phy/set_u3_phy\n");
	return 1;
}

static ssize_t user_get_u3_phy(struct device *dev, struct device_attribute *attr,
		     const char *argv, size_t count)
{
	unsigned int reg = simple_strtoul(argv, NULL, 16);
	argv = next_arg(argv);
	unsigned int page = simple_strtoul(argv, NULL, 16);
	printk("get phy page %d 0x%x = 0x%x\n", 
		page, reg, rtl8198f_u3_phy_get(rtl8198f_u3_phy, reg, page));
	
	return count;
}

static DEVICE_ATTR(get_u3_phy, S_IRUGO | S_IWUSR, user_get_u3_phy_info, user_get_u3_phy);

static struct attribute *usb_phy_attrs[] = {
	&dev_attr_set_u3_phy.attr,
	&dev_attr_get_u3_phy.attr,
	NULL,
};

static struct attribute_group usb_phy_attr_grp = {
	.attrs = usb_phy_attrs,
};

static int rtl8198f_usb3_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	
	struct phy_data *phy_data;
	struct resource *res;
	int ret = 0;
	char *phy_data_size, *phy_data_Array, *phy_data_addr;

	dev_info(dev, "Starting rtl8198f USB 3.0 PHY Probe\n");

	rtl8198f_u3_phy = devm_kzalloc(dev, sizeof(*rtl8198f_u3_phy), GFP_KERNEL);
	if (!rtl8198f_u3_phy)
		return -ENOMEM;
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
				"usbcfg_u2_base");
	rtl8198f_u3_phy->u2_phy_rebase = devm_ioremap_resource(dev, res);
	if (IS_ERR(rtl8198f_u3_phy->u2_phy_rebase))
		return PTR_ERR(rtl8198f_u3_phy->u2_phy_rebase);
	dev_info(dev, "usb2_phy resource - %pr mapped at 0x%pK\n", res,
		rtl8198f_u3_phy->u2_phy_rebase);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
				"u3_phy_base");
	rtl8198f_u3_phy->u3_phy_rebase = devm_ioremap_resource(dev, res);
	if (IS_ERR(rtl8198f_u3_phy->u3_phy_rebase))
		return PTR_ERR(rtl8198f_u3_phy->u3_phy_rebase);
	dev_info(dev, "usb3_phy resource - %pr mapped at 0x%pK\n", res,
		rtl8198f_u3_phy->u3_phy_rebase);

	rtl8198f_u3_phy->dev				= dev;
	rtl8198f_u3_phy->phy.dev			= rtl8198f_u3_phy->dev;
	rtl8198f_u3_phy->phy.label		= RTL8198F_USB3PHY_NAME;
	rtl8198f_u3_phy->phy.init		= rtl8198f_usb3_phy_init;
	rtl8198f_u3_phy->phy.shutdown	= rtl8198f_usb3_phy_shutdown;
	rtl8198f_u3_phy->phy.set_suspend	= rtl8198f_usb3_phy_set_suspend;
	rtl8198f_u3_phy->phy.set_wakeup	= rtl8198f_usb3_phy_set_wakeup;
	rtl8198f_u3_phy->phy.set_vbus	= rtl8198f_usb3_phy_set_vbus;

	phy_data = devm_kzalloc(dev, sizeof(*phy_data), GFP_KERNEL);
	if (!phy_data)
		return -ENOMEM;

	if (node) {
		ret = of_property_read_u32_index(node, "portN", 0,
			&rtl8198f_u3_phy->port_num);		
		if (ret)
			goto err;

		rtl8198f_u3_phy->strap_cpu_wdata = of_reset_control_get(node,
			"strap_cpu_wdata");
		if (IS_ERR(rtl8198f_u3_phy->strap_cpu_wdata)) {
			ret = PTR_ERR(rtl8198f_u3_phy->strap_cpu_wdata);
			dev_err(dev, "Failed to get strap_cpu_wdata.\n");
			return ret;
		}

		/* apply different phy table */
		if(reset_control_status(rtl8198f_u3_phy->strap_cpu_wdata)) {
			dev_info(dev, "Load 40M PHY table\n");
			phy_data_size = "phy_data_size_40M";
			phy_data_addr = "phy_data_addr_40M";
			phy_data_Array = "phy_data_Array_40M";
		} else {
			dev_info(dev, "Load 25M PHY table\n");
			phy_data_size = "phy_data_size_25M";
			phy_data_addr = "phy_data_addr_25M";
			phy_data_Array = "phy_data_Array_25M";
		}
		
		ret = of_property_read_u32_index(node, phy_data_size, 0,
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
		ret = of_property_read_u8_array(node, phy_data_addr,
			phy_data->addr, phy_data->size);
		if (ret)
			goto err;
		ret = of_property_read_u8_array(node, phy_data_Array,
			phy_data->data, phy_data->size);
		if (ret)
			goto err;
	}
	rtl8198f_u3_phy->phy_data = phy_data;
	platform_set_drvdata(pdev, rtl8198f_u3_phy);

	ret = usb_add_phy(&rtl8198f_u3_phy->phy, USB_PHY_TYPE_USB3);
	if (ret)
		goto err;

	struct rtl8198f_usb3 *rtl8198f_usb3;
	u32 reg;
	u32 id = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_BONDING_OPTION)) & 0x3;
	
    rtl8198f_usb3 = devm_kzalloc(&pdev->dev, sizeof(*rtl8198f_usb3), GFP_KERNEL);
    if (!rtl8198f_usb3)
            goto err;
			
	dev_info(&pdev->dev, "rtl8198f usb3 setting\n");

    rtl8198f_usb3->core_reset = of_reset_control_get(node, "core_reset");
    if (IS_ERR(rtl8198f_usb3->core_reset)) {
    	ret = PTR_ERR(rtl8198f_usb3->core_reset);
        goto err;
    }

	rtl8198f_usb3->usb3_isolate= of_reset_control_get(node, "usb3_isolate");
    if (IS_ERR(rtl8198f_usb3->usb3_isolate)) {
    	ret = PTR_ERR(rtl8198f_usb3->usb3_isolate);
        goto err;
    }

    rtl8198f_usb3->pow_usb3 = of_reset_control_get(node, "pow_usb3");
    if (IS_ERR(rtl8198f_usb3->pow_usb3)) {
    	ret = PTR_ERR(rtl8198f_usb3->pow_usb3);
        goto err;
    }

    rtl8198f_usb3->u3_bg_en = of_reset_control_get(node, "u3_bg_en");
    if (IS_ERR(rtl8198f_usb3->u3_bg_en)) {
    	ret = PTR_ERR(rtl8198f_usb3->u3_bg_en);
        goto err;
    }

    rtl8198f_usb3->u3_mbias_en = of_reset_control_get(node, "u3_mbias_en");
    if (IS_ERR(rtl8198f_usb3->u3_mbias_en)) {
    	ret = PTR_ERR(rtl8198f_usb3->u3_mbias_en);
        goto err;
    }

    rtl8198f_usb3->u3_suspend_clk = of_reset_control_get(node, "u3_suspend_clk");
    if (IS_ERR(rtl8198f_usb3->u3_suspend_clk)) {
    	ret = PTR_ERR(rtl8198f_usb3->u3_suspend_clk);
        goto err;
	}

	/* for 0xf43203b8 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	dev_info(&pdev->dev, "addr start=0x%x, end=0x%x\n", res->start, res->end);
	rtl8198f_usb3->global_usb2phy_cfg = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(rtl8198f_usb3->global_usb2phy_cfg)) {
		ret = PTR_ERR(rtl8198f_usb3->global_usb2phy_cfg);
		goto err;
	}
	
	/* for 0xf432b000 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	dev_info(&pdev->dev, "addr start=0x%x, end=0x%x\n", res->start, res->end);
	rtl8198f_usb3->usbcfg_u3_control = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rtl8198f_usb3->usbcfg_u3_control)) {
		ret = PTR_ERR(rtl8198f_usb3->usbcfg_u3_control);
		goto err;
	}
	
	/* for 0xf420c2c0 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 5);
	dev_info(&pdev->dev, "addr start=0x%x, end=0x%x\n", res->start, res->end);
	rtl8198f_usb3->hcd = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(rtl8198f_usb3->hcd)) {
		ret = PTR_ERR(rtl8198f_usb3->hcd);
		goto err;
	}

	if (id == ID_RTL8198FB) {
		/* for 0xf432b050 - 0xf432b06c */
		res = platform_get_resource(pdev, IORESOURCE_MEM, 4);
		dev_info(&pdev->dev, "addr start=0x%x, end=0x%x\n", res->start, res->end);
		rtl8198f_usb3->usbcfg_typec_ctl= devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(rtl8198f_usb3->usbcfg_typec_ctl)) {
			ret = PTR_ERR(rtl8198f_usb3->usbcfg_typec_ctl);
			goto err;
		}
		
		/* REG32(0xf432b068)=0x03;//clear interrupt */
		writel(0x3, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_CC_DET_INT);
		/* REG32(0xf432b06c)=0x00;//disable interrupt */
		writel(0, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_CC_DET_INT_EN);

		/* USB3 type-c */
		int irq_typec;
		irq_typec = platform_get_irq(pdev, 0);
		if (irq_typec < 0)
			goto err;
		else
			dev_info(&pdev->dev, "irq type c num %d\n", irq_typec);

		ret = devm_request_irq(&pdev->dev, irq_typec,
					  xhci_handle_typec_intr, IRQF_SHARED,
					  dev_name(&pdev->dev), rtl8198f_usb3);
		if (ret)
			goto err;

	}

	if (id == ID_RTL8198FB) {
		/* REG32(0xf4320020) = 0x2900; */
		reset_control_assert(rtl8198f_usb3->pow_usb3);
	}
	/* REG32(0xf43203b0) = 0x34000000; */
	reset_control_assert(rtl8198f_usb3->u3_suspend_clk);
	/* REG32(0xf432000c) = 0x00100080; */
	reset_control_deassert(rtl8198f_usb3->core_reset);
	/* REG32(0xf432b000) = 0xe8200010; */
	reg = readl(rtl8198f_usb3->usbcfg_u3_control);
	writel(reg | (0x1<<31), rtl8198f_usb3->usbcfg_u3_control);
	/* REG32(0xf420c2c0) = 0x10c5001; */
	writel(0x10c5001, rtl8198f_usb3->hcd + 0xc2c0);
	/* REG32(0xf420c2c4) = 0x01; */
	writel(0x01, rtl8198f_usb3->hcd + 0xc2c4);
	/* REG32(0xf43203b0) = 0x30000000; */
	reset_control_deassert(rtl8198f_usb3->u3_suspend_clk);
	if (id == ID_RTL8198FB) {
		/* REG32(0xf43203b8) = 0xc0030000; */
		reg = readl(rtl8198f_usb3->global_usb2phy_cfg);
		if (reg & (1<<17)) {
			writel(0xc0030000, rtl8198f_usb3->global_usb2phy_cfg);
		} else {
			/* otg existed */
			writel(0xc0010000, rtl8198f_usb3->global_usb2phy_cfg);
		}
		/* REG32(0xf4320020) = 0x2100; */
		reset_control_deassert(rtl8198f_usb3->usb3_isolate);
		/* REG32(0xf4320024) = 0x4000300; */
		reset_control_assert(rtl8198f_usb3->u3_bg_en);
		reset_control_assert(rtl8198f_usb3->u3_mbias_en);

		/* REG32(0xf432b050)=0x23def7a9;  //enable CC1 detect ,CC MODE=1 DFP USB pull  36k,high 4.7k(3A/5V),12K (1.5A/5V),36K */
		writel(0x23def7a9, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_USB_TYPEC_CTRL_CC1_0);
		/* REG32(0xf432b058)=0x03def7a9;  //enable CC2 detect,CC MODE=1 DFP USB pull high 36kk */
		writel(0x03def7a9, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_USB_TYPEC_CTRL_CC2_0);
		/* REG32(0xf432b064)=0x31; //init type c control */
		writel(0x31, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_USB_TYPEC_CTRL);
		/* REG32(0xf432b068)=0x03;//clear interrupt */
		writel(0x3, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_CC_DET_INT);
		/* REG32(0xf432b06c)=0x03;//enable interrupt */
		writel(0x3, rtl8198f_usb3->usbcfg_typec_ctl + OFFSET_CC_DET_INT_EN);
	} else {
		dev_info(&pdev->dev, "It is 98FH port 1 usb 2.0\n");
		reg = readl(rtl8198f_usb3->hcd + 0xc11c);
		writel(reg | 0x04000000, rtl8198f_usb3->hcd + 0xc11c);
		reg = readl(rtl8198f_usb3->hcd + 0xc2c0);
		writel(reg | 0x80, rtl8198f_usb3->hcd + 0xc2c0);
		reset_control_deassert(rtl8198f_usb3->u3_bg_en);
		reset_control_deassert(rtl8198f_usb3->u3_mbias_en);
		/* REG32(0xf43203b8)=0x40020000 port 1 host ,one port only ;gated port0 */
		writel(0x40020000, rtl8198f_usb3->global_usb2phy_cfg);
	}

	if (sysfs_create_group(&pdev->dev.kobj, &usb_phy_attr_grp))
		dev_err(dev, "create fail");

err:
	return ret;
}

static int rtl8198f_usb3_phy_remove(struct platform_device *pdev)
{
	struct rtl8198f_usb3_phy *rtl8198f_usb_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&rtl8198f_usb_phy->phy);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id usbphy_rtl8198f_dt_match[] = {
	{ .compatible = "rtk,rtl8198f-usb3phy", },
	{},
};
MODULE_DEVICE_TABLE(of, usbphy_rtl8198f_dt_match);
#endif

static struct platform_driver rtl8198f_usb3_phy_driver = {
	.probe		= rtl8198f_usb3_phy_probe,
	.remove		= rtl8198f_usb3_phy_remove,
	.driver		= {
		.name	= RTL8198F_USB3PHY_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(usbphy_rtl8198f_dt_match),
	},
};
module_platform_driver(rtl8198f_usb3_phy_driver);

MODULE_DESCRIPTION("RTK rtl8198f for dwc3core controller usb2phy driver");
MODULE_ALIAS("platform:" rtl8198f_USB2PHY_NAME);
MODULE_LICENSE("GPL v2");

