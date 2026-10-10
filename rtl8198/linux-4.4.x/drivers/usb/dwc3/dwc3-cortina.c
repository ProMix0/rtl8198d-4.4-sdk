/*
 * Copyright (c) 2016, Cortina Access. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/usb/otg.h>
#include <linux/usb/usb_phy_generic.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
#include <soc/cortina/ca77xx-atu.h>
#endif

struct cortina_dwc3 {
	struct platform_device	*usb2_phy;
	struct platform_device	*usb3_phy;
	struct device	*dev;
	unsigned int	cache_coherency;
};

static int debug = 1;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
static void usb_ace_add(void)
{
	struct atu_attr atu;
	struct ace_attr ace;
	char name[32];

	sprintf(name, "usb0_atu");

	atu.in = 0;
	atu.out = atu.in + ACE_S1_ADDR;
	atu.size = 0x80000000; /* 2G */

	ace.cache_en = 1;
	ace.arcache = AXCACHE_BUFFERABLE | AXCACHE_CACHEABLE |
		      AXCACHE_READ_ALLOCATE | AXCACHE_WRITE_ALLOCATE;
	ace.awcache = AXCACHE_BUFFERABLE | AXCACHE_CACHEABLE |
		      AXCACHE_READ_ALLOCATE | AXCACHE_WRITE_ALLOCATE;

	ace.qos_en = 0;
	ace.arqos = 0;
	ace.awqos = 0;

	ace.prot_en = 1;
	ace.arprot = AXPROT_NONSECURE_ACCESS;
	ace.awprot = AXPROT_NONSECURE_ACCESS;

	ace.user_en = 1;
	ace.aruser.wrd = 0;
	ace.aruser.bf.axuser = 0;
	ace.aruser.bf.axdomain = AXDOMAIN_OUTER_SHAREABLE;
	ace.aruser.bf.axbar = AXBAR_NORMAL_ACCESS_S;
	ace.aruser.bf.axsnoop = ARSNOOP_READONCE;
	ace.awuser.wrd = 0;
	ace.awuser.bf.axuser = 0;
	ace.awuser.bf.axdomain = AXDOMAIN_OUTER_SHAREABLE;
	ace.awuser.bf.axbar = AXBAR_NORMAL_ACCESS_S;
	ace.awuser.bf.axsnoop = AWSNOOP_WRITEUNIQUE;

	ca77xx_ace_set(name, 0, &atu, &ace, 1);
}
#endif

static int cortina_dwc3_register_phys(struct cortina_dwc3 *dwc3)
{
	struct usb_phy_generic_platform_data pdata;
	struct platform_device	*pdev;
	int			ret;

	memset(&pdata, 0x00, sizeof(pdata));

	pdev = platform_device_alloc("usb_phy_generic", PLATFORM_DEVID_AUTO);
	if (!pdev)
		return -ENOMEM;

	dwc3->usb2_phy = pdev;
	pdata.type = USB_PHY_TYPE_USB2;
	pdata.gpio_reset = -1;

	ret = platform_device_add_data(dwc3->usb2_phy, &pdata, sizeof(pdata));
	if (ret)
		goto err1;

	pdev = platform_device_alloc("usb_phy_generic", PLATFORM_DEVID_AUTO);
	if (!pdev) {
		ret = -ENOMEM;
		goto err1;
	}

	dwc3->usb3_phy = pdev;
	pdata.type = USB_PHY_TYPE_USB3;

	ret = platform_device_add_data(dwc3->usb3_phy, &pdata, sizeof(pdata));
	if (ret)
		goto err2;

	ret = platform_device_add(dwc3->usb2_phy);
	if (ret)
		goto err2;

	ret = platform_device_add(dwc3->usb3_phy);
	if (ret)
		goto err3;

	return 0;

err3:
	platform_device_del(dwc3->usb2_phy);

err2:
	platform_device_put(dwc3->usb3_phy);

err1:
	platform_device_put(dwc3->usb2_phy);

	return ret;
}

static int cortina_dwc3_remove_child(struct device *dev, void *unused)
{
	struct platform_device *pdev = to_platform_device(dev);

	platform_device_unregister(pdev);

	return 0;
}

static int cortina_dwc3_probe(struct platform_device *pdev)
{
	struct cortina_dwc3	*dwc3;
	struct device		*dev = &pdev->dev;
	struct device_node	*node = dev->of_node;
	int			ret;

	if (debug)
		dev_info(dev, "%s() start.\n", __func__);

	dwc3 = devm_kzalloc(dev, sizeof(*dwc3), GFP_KERNEL);
	if (!dwc3)
		return -ENOMEM;

	/*
	 * Right now device-tree probed devices don't get dma_mask set.
	 * Since shared usb code relies on it, set it here for now.
	 * Once we move to full device tree support this will vanish off.
	 */
	ret = dma_coerce_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	platform_set_drvdata(pdev, dwc3);
	ret = cortina_dwc3_register_phys(dwc3);
	if (ret) {
		dev_err(dev, "couldn't register PHYs\n");
		return ret;
	}
	dwc3->dev = dev;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
	if (of_property_read_bool(node, "ace-enable"))
		dwc3->cache_coherency = true;
	else
		dwc3->cache_coherency = false;
	dev_info(dev, "xHCI ACE %s\n", dwc3->cache_coherency ?
		 "enabled" : "disabled");

	if (dwc3->cache_coherency)
		usb_ace_add();
#endif

	if (node) {
		ret = of_platform_populate(node, NULL, NULL, dev);
		if (ret) {
			dev_err(dev, "failed to add dwc3 core\n");
			goto err4;
		}
	} else {
		dev_err(dev, "no device node, failed to add dwc3 core\n");
		ret = -ENODEV;
		goto err4;
	}

	if (debug)
		dev_info(dev, "%s() End.\n", __func__);

	return 0;
err4:
	return ret;
}

static int cortina_dwc3_remove(struct platform_device *pdev)
{
	struct cortina_dwc3	*dwc3 = platform_get_drvdata(pdev);

	device_for_each_child(&pdev->dev, NULL, cortina_dwc3_remove_child);
	platform_device_unregister(dwc3->usb2_phy);
	platform_device_unregister(dwc3->usb3_phy);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cortina_dwc3_match[] = {
	{ .compatible = "cortina,dwusb3" },
	{},
};
MODULE_DEVICE_TABLE(of, cortina_dwc3_match);
#endif

#ifdef CONFIG_PM_SLEEP
static int cortina_dwc3_suspend(struct device *dev)
{
     /* struct cortina_dwc3 *dwc3 = dev_get_drvdata(dev); */

	/* clk_disable(dwc3->clk); */

	return 0;
}

static int cortina_dwc3_resume(struct device *dev)
{
	/* struct cortina_dwc3 *dwc3 = dev_get_drvdata(dev); */

	/* runtime set active to reflect active state. */
	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return 0;
}

static const struct dev_pm_ops cortina_dwc3_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cortina_dwc3_suspend, cortina_dwc3_resume)
};

#define DEV_PM_OPS	(&cortina_dwc3_dev_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static struct platform_driver cortina_dwc3_driver = {
	.probe		= cortina_dwc3_probe,
	.remove		= cortina_dwc3_remove,
	.driver		= {
		.name	= "cortina-dwc3",
		.of_match_table = of_match_ptr(cortina_dwc3_match),
		.pm	= DEV_PM_OPS,
	},
};

module_platform_driver(cortina_dwc3_driver);

MODULE_ALIAS("platform:cortina-dwc3");
MODULE_AUTHOR("Jason Li<jason.li@cortina-access.com>");
MODULE_DESCRIPTION("DesignWare CORTINA-ACCESS Ca77xx Soc USB3 Glue Layer");
MODULE_LICENSE("GPL v2");
