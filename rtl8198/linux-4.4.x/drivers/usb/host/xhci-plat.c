/*
 * xhci-plat.c - xHCI host controller driver platform Bus Glue.
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com
 * Author: Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * A lot of code borrowed from the Linux xHCI driver.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/usb/phy.h>
#include <linux/slab.h>
#include <linux/usb/xhci_pdriver.h>
#include <linux/acpi.h>

#include "xhci.h"
#include "xhci-mvebu.h"
#include "xhci-rcar.h"

static struct hc_driver __read_mostly xhci_plat_hc_driver;

static int xhci_plat_setup(struct usb_hcd *hcd);
static int xhci_plat_start(struct usb_hcd *hcd);

static const struct xhci_driver_overrides xhci_plat_overrides __initconst = {
	.extra_priv_size = sizeof(struct xhci_hcd),
	.reset = xhci_plat_setup,
	.start = xhci_plat_start,
};

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
#include <soc/cortina/ca77xx-atu.h>

/*** Register Offset ***/
#define CORTINA_XHCI_GUSB3PIPECTL0	    0xc2c0	/* data: 0x010c0003 */
#define CORTINA_XHCI_GUSB3PIPECTL1	    0xc2c4	/* data: 0x010c0003 */
#define CORTINA_XHCI_GCTL	            0xc110	/* data: 0x2ee01000 */
#define CORTINA_XHCI_GSBUSCFG0	        0xc100	/* data: 0x0000000f */
#define CORTINA_XHCI_GTXTHRCFG	        0xc108	/* data: 0x23100000 */
#define CORTINA_XHCI_GRXTHRCFG	        0xc10c	/* data: 0x03800000 */
#define CORTINA_XHCI_GUSB2PHYCFG_P0		0xc200	/* data: 0x40102400 */
#define CORTINA_XHCI_GUSB2PHYCFG_P1		0xc204	/* data: 0x40102400 */
#define CORTINA_XHCI_U3S2_PORTSC_P0		0x0440	/* # U3 S2 link status (port3, SS 0) */
#define CORTINA_XHCI_U3S3_PORTSC_P1		0x0450	/* # U3 S3 link status (port4, SS 1) */
#define CORTINA_XHCI_GUSB2PORTSC_P0		0x0420	/* data: 0x000206f3 */
#define CORTINA_XHCI_GUSB2PORTSC_P1		0x0430	/* data: 0x000206f3 */

/* Register configuration values */
#define CORTINA_XHCI_GUSB3GPIPECTL_VAL	0x010c0003
#define CORTINA_XHCI_GCTL_VAL			0x2ee01000
#define CORTINA_XHCI_GSBUSCFG0_VAL		0x0000000f
#define CORTINA_XHCI_GTXTHRCFG_VAL		0x23100000
#define CORTINA_XHCI_GRXTHRCFG_VAL		0x03800000
#define CORTINA_XHCI_GUSB2PHYCFG_VAL	0x40102400
#define CORTINA_XHCI_USB2PORTSC_VAL		0x000206f3

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

static void cortina_xhci_init(struct usb_hcd *hcd)
{
	if (hcd->regs != NULL) {
		writel(CORTINA_XHCI_GUSB3GPIPECTL_VAL, hcd->regs + CORTINA_XHCI_GUSB3PIPECTL0);
		writel(CORTINA_XHCI_GUSB3GPIPECTL_VAL, hcd->regs + CORTINA_XHCI_GUSB3PIPECTL1);
		writel(CORTINA_XHCI_GCTL_VAL, hcd->regs + CORTINA_XHCI_GCTL);
		writel(CORTINA_XHCI_GSBUSCFG0_VAL, hcd->regs + CORTINA_XHCI_GSBUSCFG0);
		writel(CORTINA_XHCI_GTXTHRCFG_VAL, hcd->regs + CORTINA_XHCI_GTXTHRCFG);
		writel(CORTINA_XHCI_GRXTHRCFG_VAL, hcd->regs + CORTINA_XHCI_GRXTHRCFG);
		writel(CORTINA_XHCI_GUSB2PHYCFG_VAL, hcd->regs + CORTINA_XHCI_GUSB2PHYCFG_P0);
		writel(CORTINA_XHCI_GUSB2PHYCFG_VAL, hcd->regs + CORTINA_XHCI_GUSB2PHYCFG_P1);
		writel(CORTINA_XHCI_USB2PORTSC_VAL, hcd->regs + CORTINA_XHCI_GUSB2PORTSC_P0);
		writel(CORTINA_XHCI_USB2PORTSC_VAL, hcd->regs + CORTINA_XHCI_GUSB2PORTSC_P1);
	}

    return;
}
#endif

static void xhci_plat_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	/*
	 * As of now platform drivers don't provide MSI support so we ensure
	 * here that the generic code does not try to make a pci_dev from our
	 * dev struct in order to setup MSI
	 */
	xhci->quirks |= XHCI_PLAT;
}

/* called during probe() after chip reset completes */
static int xhci_plat_setup(struct usb_hcd *hcd)
{
	struct device_node *of_node = hcd->self.controller->of_node;
	int ret;

	if (of_device_is_compatible(of_node, "renesas,xhci-r8a7790") ||
	    of_device_is_compatible(of_node, "renesas,xhci-r8a7791")) {
		ret = xhci_rcar_init_quirk(hcd);
		if (ret)
			return ret;
	}

	return xhci_gen_setup(hcd, xhci_plat_quirks);
}

static int xhci_plat_start(struct usb_hcd *hcd)
{
	struct device_node *of_node = hcd->self.controller->of_node;

	if (of_device_is_compatible(of_node, "renesas,xhci-r8a7790") ||
	    of_device_is_compatible(of_node, "renesas,xhci-r8a7791"))
		xhci_rcar_start(hcd);

	return xhci_run(hcd);
}

static int xhci_plat_probe(struct platform_device *pdev)
{
	struct device_node	*node = pdev->dev.of_node;
	struct usb_xhci_pdata	*pdata = dev_get_platdata(&pdev->dev);
	const struct hc_driver	*driver;
	struct xhci_hcd		*xhci;
	struct resource         *res;
	struct usb_hcd		*hcd;
	struct clk              *clk;
	int			ret;
	int			irq;

	if (usb_disabled())
		return -ENODEV;

	driver = &xhci_plat_hc_driver;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	/* Try to set 64-bit DMA first */
	if (WARN_ON(!pdev->dev.dma_mask))
		/* Platform did not initialize dma_mask */
		ret = dma_coerce_mask_and_coherent(&pdev->dev,
						   DMA_BIT_MASK(64));
	else
		ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));

	/* If seting 64-bit DMA mask fails, fall back to 32-bit DMA mask */
	if (ret) {
		ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
		if (ret)
			return ret;
	}

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
	if(of_dma_is_coherent(node)){
		usb_ace_add();
		printk("%s: xHCI ACE enabled\n", __func__);
	}
#endif

	hcd = usb_create_hcd(driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hcd->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(hcd->regs)) {
		ret = PTR_ERR(hcd->regs);
		goto put_hcd;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);

	/*
	 * Not all platforms have a clk so it is not an error if the
	 * clock does not exists.
	 */
	clk = devm_clk_get(&pdev->dev, NULL);
	if (!IS_ERR(clk)) {
		ret = clk_prepare_enable(clk);
		if (ret)
			goto put_hcd;
	} else if (PTR_ERR(clk) == -EPROBE_DEFER) {
		ret = -EPROBE_DEFER;
		goto put_hcd;
	}

	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-375-xhci") ||
	    of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-380-xhci")) {
		ret = xhci_mvebu_mbus_init_quirk(pdev);
		if (ret)
			goto disable_clk;
	}

	device_wakeup_enable(hcd->self.controller);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
	if(of_device_is_compatible(pdev->dev.of_node, "cortina,ca77xx-xhci"))
		cortina_xhci_init(hcd);
#endif

	xhci = hcd_to_xhci(hcd);
	xhci->clk = clk;
	xhci->main_hcd = hcd;
	xhci->shared_hcd = usb_create_shared_hcd(driver, &pdev->dev,
			dev_name(&pdev->dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto disable_clk;
	}

	if ((node && of_property_read_bool(node, "usb3-lpm-capable")) ||
			(pdata && pdata->usb3_lpm_capable))
		xhci->quirks |= XHCI_LPM_SUPPORT;

	hcd->usb_phy = devm_usb_get_phy_by_phandle(&pdev->dev, "usb-phy", 0);
	if (IS_ERR(hcd->usb_phy)) {
		ret = PTR_ERR(hcd->usb_phy);
		if (ret == -EPROBE_DEFER)
			goto put_usb3_hcd;
		hcd->usb_phy = NULL;
	} else {
		ret = usb_phy_init(hcd->usb_phy);
		if (ret)
			goto put_usb3_hcd;
	}

#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
	if(of_device_is_compatible(pdev->dev.of_node, "cortina,ca77xx-xhci"))
		usb_phy_vbus_on(hcd->usb_phy);
#endif

	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto disable_usb_phy;

	if (HCC_MAX_PSA(xhci->hcc_params) >= 4)
		xhci->shared_hcd->can_do_streams = 1;

	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
		goto dealloc_usb2_hcd;

	return 0;


dealloc_usb2_hcd:
	usb_remove_hcd(hcd);

disable_usb_phy:
	usb_phy_shutdown(hcd->usb_phy);

put_usb3_hcd:
	usb_put_hcd(xhci->shared_hcd);

disable_clk:
	if (!IS_ERR(clk))
		clk_disable_unprepare(clk);

put_hcd:
	usb_put_hcd(hcd);

	return ret;
}

static int xhci_plat_remove(struct platform_device *dev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	struct clk *clk = xhci->clk;

	xhci->xhc_state |= XHCI_STATE_REMOVING;

	usb_remove_hcd(xhci->shared_hcd);
	usb_phy_shutdown(hcd->usb_phy);

	usb_remove_hcd(hcd);
	usb_put_hcd(xhci->shared_hcd);

	if (!IS_ERR(clk))
		clk_disable_unprepare(clk);
	usb_put_hcd(hcd);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int xhci_plat_suspend(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	/*
	 * xhci_suspend() needs `do_wakeup` to know whether host is allowed
	 * to do wakeup during suspend. Since xhci_plat_suspend is currently
	 * only designed for system suspend, device_may_wakeup() is enough
	 * to dertermine whether host is allowed to do wakeup. Need to
	 * reconsider this when xhci_plat_suspend enlarges its scope, e.g.,
	 * also applies to runtime suspend.
	 */
	return xhci_suspend(xhci, device_may_wakeup(dev));
}

static int xhci_plat_resume(struct device *dev)
{
	struct usb_hcd	*hcd = dev_get_drvdata(dev);
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	return xhci_resume(xhci, 0);
}

static const struct dev_pm_ops xhci_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_plat_suspend, xhci_plat_resume)
};
#define DEV_PM_OPS	(&xhci_plat_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_OF
static const struct of_device_id usb_xhci_of_match[] = {
	{ .compatible = "generic-xhci" },
	{ .compatible = "xhci-platform" },
	{ .compatible = "marvell,armada-375-xhci"},
	{ .compatible = "marvell,armada-380-xhci"},
	{ .compatible = "renesas,xhci-r8a7790"},
	{ .compatible = "renesas,xhci-r8a7791"},
	{ .compatible = "cortina,ca77xx-xhci"},
	{ .compatible = "xhci,xhci-rtl8198f"},
	{ },
};
MODULE_DEVICE_TABLE(of, usb_xhci_of_match);
#endif

static const struct acpi_device_id usb_xhci_acpi_match[] = {
	/* XHCI-compliant USB Controller */
	{ "PNP0D10", },
	{ }
};
MODULE_DEVICE_TABLE(acpi, usb_xhci_acpi_match);

static struct platform_driver usb_xhci_driver = {
	.probe	= xhci_plat_probe,
	.remove	= xhci_plat_remove,
	.driver	= {
		.name = "xhci-hcd",
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(usb_xhci_of_match),
		.acpi_match_table = ACPI_PTR(usb_xhci_acpi_match),
	},
};
MODULE_ALIAS("platform:xhci-hcd");

static int __init xhci_plat_init(void)
{
	xhci_init_driver(&xhci_plat_hc_driver, &xhci_plat_overrides);
	return platform_driver_register(&usb_xhci_driver);
}
module_init(xhci_plat_init);

static void __exit xhci_plat_exit(void)
{
	platform_driver_unregister(&usb_xhci_driver);
}
module_exit(xhci_plat_exit);

MODULE_DESCRIPTION("xHCI Platform Host Controller Driver");
MODULE_LICENSE("GPL");
