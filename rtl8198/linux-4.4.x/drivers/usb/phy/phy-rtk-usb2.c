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

#define rtl8198f_USB2PHY_NAME "rtl8198f-usb2phy"

struct rtl8198f_otg_setting {
	void __iomem *otg_ext_cfg;
	void __iomem *glb_usb2phy_cfg;
	struct reset_control *usbotg_en;
	struct reset_control *active_usbotg;
};

static struct phy_data {
	int size;
	u8 *addr;
	u8 *data;
};

struct rtl8198f_usb2_phy {
	struct usb_phy	phy;
	struct device	*dev;
	struct phy_data *phy_data;
	void __iomem	*phy_regbase;
	void __iomem	*phy0_reg;
	int port_num;
	struct reset_control	*usbblock_reset;
	struct reset_control	*s2usb_dphy_reset;
	struct reset_control	*strap_cpu_wdata;
	struct phy              *s2_phy;
	spinlock_t		lock;	/* lock access to bank regs */
};

static struct rtl8198f_usb2_phy *rtl8198f_usb_phy;

static int debug=0;

static u8 rtl8198f_usb_phy_read(struct rtl8198f_usb2_phy *rtl8198f_phy, u8 reg)
{
	u8 val = 0;    
	u32 data = 0;
	
	reg = reg - 0x20;
	data|= (1<<25);		/* NewRegReq */
	data&= ~(1<<22);	/* RegWr */
	data|= (0x2f<<16);	/* RegWr */
	data|= ((reg & 0x0F) << 8) ;
	writel(data, rtl8198f_phy->phy_regbase);
	mdelay(1);

	data|=(1<<25);		/* NewRegReq */
	data&=~(0xff<<8);
	data|= (((reg & 0xF0)>>4) << 8) ;
	writel(data, rtl8198f_phy->phy_regbase);
	mdelay(1);

	val = readl(rtl8198f_phy->phy_regbase) & 0xff;
	//printk("DWC_OTG: phy(0x%02x) = 0x%02x\n", reg + 0x20, val);
        
        return val;
}

static void rtl8198f_usb2_host_reset(struct rtl8198f_usb2_phy *rtl8198f_phy)
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

static void rtl8198f_usb_phy_write(struct rtl8198f_usb2_phy *rtl8198f_phy,
	u8 val, u8 reg, int port_id)
{
	u32 data = 0;
	
	writel(val, rtl8198f_phy->phy0_reg);
	mdelay(1);
	
	data |= (1<<25);	/* NewRegReq */
	data |= (1<<22);	/* RegWr */
	data |= (0x2f<<16);	/* RegWr */
	data |= ((reg & 0x0F) << 8) ;
	writel(data, rtl8198f_phy->phy_regbase);
	mdelay(1);

	data|= (1<<25);		/* NewRegReq */
	data&= ~(0xff<<8);
	data|= (((reg & 0xF0)>>4) << 8) ;
	writel(data, rtl8198f_phy->phy_regbase);
	mdelay(1);

	if (debug)
		dev_info(rtl8198f_phy->dev, "0x%02x = 0x%02x\n",reg, rtl8198f_usb_phy_read(rtl8198f_phy, reg));
	return;
}

static int rtl8198f_usb2_phy_init(struct usb_phy *phy)
{
	struct rtl8198f_usb2_phy *rtl8198f_phy = (struct rtl8198f_usb2_phy *) phy;
	u8 *addr = rtl8198f_phy->phy_data->addr;
	u8 *data = rtl8198f_phy->phy_data->data;
	int size = rtl8198f_phy->phy_data->size;
	int port_num = rtl8198f_phy->port_num;
	int index, port_id;
	rtl8198f_phy->phy_regbase = phy->io_priv;
	dev_info(phy->dev, "%s \n",__func__);

	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	for (port_id = 0; port_id < port_num; port_id++) {
		for (index = 0; index < size; index++) {
			/* USB2 PHY calibration and initialization */
			rtl8198f_usb_phy_write(rtl8198f_phy, *(data + index),
				*(addr + index), port_id);
		}
	}
	if (debug) {
		dev_info(phy->dev, "%s Initialized RTK USB 2.0 PHY\n",
			__func__);
	}
	return 0;
}

static void rtl8198f_usb2_phy_shutdown(struct usb_phy *phy)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);
}

static int rtl8198f_usb2_phy_set_suspend(struct usb_phy *phy,
	int suspend)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int rtl8198f_usb2_phy_set_wakeup(struct usb_phy *phy,
	bool enabled)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int rtl8198f_usb2_phy_set_vbus(struct usb_phy *phy,
	int on)
{
	/* Todo */
	if (debug)
		dev_info(phy->dev, "%s()\n", __func__);

	return 0;
}

static int str_parsing(const char *str,int len, int* result)
{
	int i = 0;
	int idx = 0;
	char c;
	bool is_last_num = 0, is_hex = 0;

	while(i <= len-1) {
		c = str[i++];
		if (is_hex && is_last_num) {
			if (c>='0' && c<='9') {
				result[idx] = 16*result[idx] + c - '0';
			} else if (c>='a' && c<='f') {
				result[idx] = 16*result[idx] + c - 'a' + 10;
			} else if (c>='A' && c<='F') {
				result[idx] = 16*result[idx] + c - 'A' + 10;
			}
			idx++;
			is_hex = 0;
			is_last_num = 0;
		} else if (is_hex) {
			if (c>='0' && c<='9')
				result[idx] = c - '0';
			else if (c>='a' && c<='f')
				result[idx] = c - 'a' + 10;
			else if (c>='A' && c<='F')
				result[idx] = c - 'A' + 10;

			if (c != 'x')
				is_last_num = 1;
		} else if ((c == '0') && (str[i] == 'x')) {
			is_hex = 1;
		} else if (is_last_num) {
			if (c>='0' && c<='9') {
				result[idx] = 10*result[idx] + c - '0';
				if (!(str[i]>='0' && str[i]<='9')) {
					is_last_num = 0;
					idx++;
				}
			} else {
				idx++;
			}
		} else if (c>='0' && c<='9') {
			is_last_num = 1;
			result[idx] = c - '0';
		}
	}
	return idx;	
}


static ssize_t user_set_phy_info(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	printk("usage :\n");
	printk("echo [reg] [data] > /sys/devices/platform/f432b80c.usb2_phy/set_phy\n");
	return 1;
}

static ssize_t user_set_phy(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	int parameter[2] = {0, 0};
	int p_num;
	p_num = str_parsing(buf, count, parameter);

	if (p_num < 2) {
		printk("Invalid parameters\n");
		return count;
	}

	rtl8198f_usb_phy_write(rtl8198f_usb_phy, parameter[1], parameter[0], 0);
	printk("set phy 0x%x = 0x%x, read 0x%x\n", parameter[0], parameter[1], 
		rtl8198f_usb_phy_read(rtl8198f_usb_phy, parameter[0]));
	
	return count;
}

static DEVICE_ATTR(set_phy, S_IRUGO | S_IWUSR, user_set_phy_info, user_set_phy);

static ssize_t user_get_phy_info(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	printk("usage :\n");
	printk("echo [reg] > /sys/devices/platform/f432b80c.usb2_phy/get_phy\n");
	return 1;
}

static ssize_t user_get_phy(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	int parameter[2] = {0, 0};
	str_parsing(buf, count, parameter);
	mdelay(100);
	printk("get phy 0x%x = 0x%x\n", parameter[0], rtl8198f_usb_phy_read(rtl8198f_usb_phy, parameter[0]));
	
	return count;
}

static DEVICE_ATTR(get_phy, S_IRUGO | S_IWUSR, user_get_phy_info, user_get_phy);

static struct attribute *usb_phy_attrs[] = {
	&dev_attr_set_phy.attr,
	&dev_attr_get_phy.attr,
	NULL,
};

static struct attribute_group usb_phy_attr_grp = {
	.attrs = usb_phy_attrs,
};

static int rtl8198f_usb2_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	struct phy_data *phy_data;
	struct resource *res;
	int ret = 0;
	char *phy_data_size, *phy_data_Array, *phy_data_addr;

	if (debug)
		dev_info(dev, "Starting rtl8198f USB 2.0 PHY Probe\n");

	rtl8198f_usb_phy = devm_kzalloc(dev, sizeof(*rtl8198f_usb_phy), GFP_KERNEL);
	if (!rtl8198f_usb_phy)
		return -ENOMEM;
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
				"phy0_reg");
	rtl8198f_usb_phy->phy0_reg = devm_ioremap_resource(dev, res);
	if (IS_ERR(rtl8198f_usb_phy->phy0_reg))
		return PTR_ERR(rtl8198f_usb_phy->phy0_reg);
	dev_info(dev, "usb2_phy resource - %pr mapped at 0x%pK\n", res,
		rtl8198f_usb_phy->phy0_reg);

	rtl8198f_usb_phy->dev				= dev;
	rtl8198f_usb_phy->phy.dev			= rtl8198f_usb_phy->dev;
	rtl8198f_usb_phy->phy.label		= rtl8198f_USB2PHY_NAME;
	rtl8198f_usb_phy->phy.init		= rtl8198f_usb2_phy_init;
	rtl8198f_usb_phy->phy.shutdown	= rtl8198f_usb2_phy_shutdown;
	rtl8198f_usb_phy->phy.set_suspend	= rtl8198f_usb2_phy_set_suspend;
	rtl8198f_usb_phy->phy.set_wakeup	= rtl8198f_usb2_phy_set_wakeup;
	rtl8198f_usb_phy->phy.set_vbus	= rtl8198f_usb2_phy_set_vbus;

	phy_data = devm_kzalloc(dev, sizeof(*phy_data), GFP_KERNEL);
	if (!phy_data)
		return -ENOMEM;

	if (node) {
#if 0
		rtl8198f_usb_phy->usbblock_reset = of_reset_control_get(node,
			"usbblock_reset");
		if (IS_ERR(rtl8198f_usb_phy->usbblock_reset)) {
			ret = PTR_ERR(rtl8198f_usb_phy->usbblock_reset);
			dev_err(dev, "Failed to get usbblack_reset.\n");
			return ret;
		}
		rtl8198f_usb_phy->s2usb_dphy_reset = of_reset_control_get(node,
			"s2usb_dphy_reset");
		if (IS_ERR(rtl8198f_usb_phy->s2usb_dphy_reset)) {
			ret = PTR_ERR(rtl8198f_usb_phy->s2usb_dphy_reset);
			dev_err(dev, "Failed to get s2usb_dphy_reset.\n");
			return ret;
		}
		rtl8198f_usb_phy->s2_phy = devm_phy_get(dev, "s2usb-phy");
		if (IS_ERR(rtl8198f_usb_phy->s2_phy)) {
			ret = PTR_ERR(rtl8198f_usb_phy->s2_phy);
			dev_err(dev, "Failed to get s2usb-phy.\n");
			return ret;
		}
#endif
		ret = of_property_read_u32_index(node, "portN", 0,
			&rtl8198f_usb_phy->port_num);		
		if (ret)
			goto err;

		rtl8198f_usb_phy->strap_cpu_wdata = of_reset_control_get(node,
			"strap_cpu_wdata");
		if (IS_ERR(rtl8198f_usb_phy->strap_cpu_wdata)) {
			ret = PTR_ERR(rtl8198f_usb_phy->strap_cpu_wdata);
			dev_err(dev, "Failed to get strap_cpu_wdata.\n");
			return ret;
		}

		/* apply different phy table */
		if (reset_control_status(rtl8198f_usb_phy->strap_cpu_wdata)) {
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
	rtl8198f_usb_phy->phy_data = phy_data;
	platform_set_drvdata(pdev, rtl8198f_usb_phy);

	if (sysfs_create_group(&pdev->dev.kobj, &usb_phy_attr_grp))
		dev_err(dev, "create fail");
			    
	struct rtl8198f_otg_setting *rtl8198f_otg_setting;
	unsigned int reg;

	rtl8198f_otg_setting = devm_kzalloc(dev, sizeof(*rtl8198f_otg_setting), GFP_KERNEL);
	if (!rtl8198f_otg_setting)
        	return -ENOMEM;

	/* 0xf432b800 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	rtl8198f_otg_setting->otg_ext_cfg = devm_ioremap_resource(dev, res);
	if (IS_ERR(rtl8198f_otg_setting->otg_ext_cfg))
		return PTR_ERR(rtl8198f_otg_setting->otg_ext_cfg);

	/* 0xf43203b8 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	rtl8198f_otg_setting->glb_usb2phy_cfg = devm_ioremap_resource(dev, res);
	if (IS_ERR(rtl8198f_otg_setting->glb_usb2phy_cfg))
		return PTR_ERR(rtl8198f_otg_setting->glb_usb2phy_cfg);

	rtl8198f_otg_setting->usbotg_en = of_reset_control_get(node, "usbotg_en");
    	if (IS_ERR(rtl8198f_otg_setting->usbotg_en)) 
        		return PTR_ERR(rtl8198f_otg_setting->active_usbotg);

	rtl8198f_otg_setting->active_usbotg = of_reset_control_get(node, "active_usbotg");
    	if (IS_ERR(rtl8198f_otg_setting->active_usbotg))
        		return PTR_ERR(rtl8198f_otg_setting->active_usbotg);

	/* b30=por, b28=reset, b26=pass b17=port_one_host, b16=two_port_host */
	reg = readl(rtl8198f_otg_setting->glb_usb2phy_cfg);
	reg |= (1<<30);
	reg &= ~((1<<28) | (1<<26) | (1<<17)  | (1<<16));
	writel(reg, rtl8198f_otg_setting->glb_usb2phy_cfg);

	reset_control_assert(rtl8198f_otg_setting->usbotg_en);
	reset_control_assert(rtl8198f_otg_setting->active_usbotg);

	/* 98f for hang up 50 resistor */
	writel(readl(rtl8198f_otg_setting->otg_ext_cfg) & ~(1<<22) , rtl8198f_otg_setting->otg_ext_cfg);		

	ret = usb_add_phy(&rtl8198f_usb_phy->phy, USB_PHY_TYPE_USB2);
	//ret = usb_add_phy_dev(&rtl8198f_usb_phy->phy);
	if (ret)
		goto err;
	
err:
	return ret;
}

static int rtl8198f_usb2_phy_remove(struct platform_device *pdev)
{
	struct rtl8198f_usb2_phy *rtl8198f_usb_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&rtl8198f_usb_phy->phy);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id usbphy_rtl8198f_dt_match[] = {
	{ .compatible = "rtk,rtl8198f-usb2phy", },
	{},
};
MODULE_DEVICE_TABLE(of, usbphy_rtl8198f_dt_match);
#endif

static struct platform_driver rtl8198f_usb2_phy_driver = {
	.probe		= rtl8198f_usb2_phy_probe,
	.remove		= rtl8198f_usb2_phy_remove,
	.driver		= {
		.name	= rtl8198f_USB2PHY_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(usbphy_rtl8198f_dt_match),
	},
};
module_platform_driver(rtl8198f_usb2_phy_driver);

MODULE_DESCRIPTION("RTK rtl8198f for dwc3core controller usb2phy driver");
MODULE_ALIAS("platform:" rtl8198f_USB2PHY_NAME);
MODULE_LICENSE("GPL v2");

