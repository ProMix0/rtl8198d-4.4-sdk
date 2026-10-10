/*
 * Copyright (C) 2017 Realtek Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/watchdog.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>


#define DRIVER_NAME	"rtl8198f-wdt"

#define PER_WDT_CTRL		0x00
#define   WDT_CTRL_WDTEN	BIT(0)
#define   WDT_CTRL_RSTEN	BIT(1)
#define	  WDT_CTRL_CLKSEL	BIT(2)
#define PER_WDT_PS		0x04
#define PER_WDT_DIV		0x08
#define PER_WDT_LD		0x0C
#define PER_WDT_LOADE		0x10
#define   WDT_LOADE_WDT		BIT(0)
#define	  WDT_LOADE_PRE		BIT(1)
#define PER_WDT_CNT		0x14
#define PER_WDT_IE_0		0x18
#define	  WDT_IE_WDTE		BIT(0)
#define PER_WDT_INT_0		0x1C
#define   WDT_INT_WDTI		BIT(0)
#define PER_WDT_STAT_0		0x20
#define   WDT_STAT_WDTS		BIT(0)


#define WDT_CLK_PS_ONLY		0x4	/* For time unit msec */
#define WDT_CLK_PS_DV		0x0	/* For time unit sec */
#define WDT_START_DEFAULT	0x3

#define WDT_LOADE_LD		0x1
#define WDT_LOADE_PS		0x2
#define WDT_LOADE_ALL		0x3

#define WDT_DEFAULT_TIMEOUT	30	/* seconds */
#define WDT_DEFAULT_RESOLUTION	0
#define WDT_MIN_TIMEOUT		1
#define WDT_TIME_UNIT_SEC	1000
#define WDT_TIME_UNIT_MSEC	1

#define FPGA_K_PCLK		25000	/* FPGA PCLK 25MHz */
#define ASIC_K_PCLK		125000	/* ASIC PCLK 125MHz */

#define WD_RESET_SUBSYS_ENABLE	BIT(4)
#define WD_RESET_ALL_BLOCKS	BIT(6)
#define WD_RESET_REMAP		BIT(7)
#define WD_RESET_EXT_RESET	BIT(8)
#define EXT_RESET		BIT(9)
#define SEC_WDT_ENABLE		BIT(0)

/*
 * Debug macro for 8198F WDT
 * If you want to enable this macro you need to define "DEBUG"
 */
#ifdef DEBUG
#define WDT_DEBUG(fmt, args...) pr_info("%s: " fmt, __func__, ## args)
#else
#define WDT_DEBUG(fmt, args...)
#endif

struct rtl8198f_watchdog {
	void __iomem		*base;
	void __iomem		*glb_cfg;
	void __iomem		*sec_rst;
	struct clk              *clk;
	u32			prescaler;
	u16			ps_div;
	bool			rst;
	bool			ctrl_clksel;
	spinlock_t		io_lock;
	struct watchdog_device	wdd;
	struct notifier_block	wdt_notifier;
};

static bool nowayout = WATCHDOG_NOWAYOUT;
static bool high_resolution = WDT_DEFAULT_RESOLUTION;
static int wdt_timeout = WDT_DEFAULT_TIMEOUT;

static inline void rtk_wdt_w32(struct rtl8198f_watchdog *wdt, u32 offset,
				u32 val)
{
	writel_relaxed(val, wdt->base + offset);
}
static inline u32 rtk_wdt_r32(struct rtl8198f_watchdog *wdt, u32 offset)
{
	return readl(wdt->base + offset);
}

static int rtl8198f_wdt_ping(struct watchdog_device *wdog)
{
	struct rtl8198f_watchdog *wdt = watchdog_get_drvdata(wdog);
	u32 val;

	spin_lock(&wdt->io_lock);
	val = WDT_LOADE_WDT | WDT_LOADE_PRE;
	rtk_wdt_w32(wdt, PER_WDT_LOADE, val);

	spin_unlock(&wdt->io_lock);

	WDT_DEBUG("8198F WDT ping.\n");
	return 0;
}

static int rtl8198f_wdt_start(struct watchdog_device *wdog)
{
	struct rtl8198f_watchdog *wdt = watchdog_get_drvdata(wdog);
	unsigned int val = 0;
	unsigned short count;

	count = high_resolution ? wdog->timeout * 1000 : wdog->timeout;
	spin_lock(&wdt->io_lock);

	/* Set timeout */
	rtk_wdt_w32(wdt, PER_WDT_PS, wdt->prescaler);
	rtk_wdt_w32(wdt, PER_WDT_DIV, wdt->ps_div);
	rtk_wdt_w32(wdt, PER_WDT_LD, count);

	if (wdt->rst)
		val |= WDT_CTRL_RSTEN;
	else
		rtk_wdt_w32(wdt, PER_WDT_IE_0, WDT_IE_WDTE);
	if (wdt->ctrl_clksel)
		val |= WDT_CTRL_CLKSEL;
	val |= WDT_CTRL_WDTEN;
	rtk_wdt_w32(wdt, PER_WDT_CTRL, val);

	spin_unlock(&wdt->io_lock);

	WDT_DEBUG("8198F WDT started.\n");
	return 0;
}

static int rtl8198f_wdt_stop(struct watchdog_device *wdog)
{
	struct rtl8198f_watchdog *wdt = watchdog_get_drvdata(wdog);
	u32 val;

	spin_lock(&wdt->io_lock);

	val = rtk_wdt_r32(wdt, PER_WDT_CTRL);
	val &= ~WDT_CTRL_WDTEN;
	rtk_wdt_w32(wdt, PER_WDT_CTRL, val);

	spin_unlock(&wdt->io_lock);

	WDT_DEBUG("8198F WDT stop.\n");
	return 0;
}

static int rtl8198f_wdt_set_timeout(struct watchdog_device *wdog,
				   unsigned int t)
{
	wdog->timeout = t;

	WDT_DEBUG("8198F WDT set timeout to %u seconds\n", t);
	return rtl8198f_wdt_start(wdog);
}

static unsigned int rtl8198f_wdt_get_timeleft(struct watchdog_device *wdog)
{
	uint32_t t;
	struct rtl8198f_watchdog *wdt = watchdog_get_drvdata(wdog);

	spin_lock(&wdt->io_lock);

	t = rtk_wdt_r32(wdt, PER_WDT_CNT);

	spin_unlock(&wdt->io_lock);

	WDT_DEBUG("8198F WDT timeout in %u seconds\n", t);
	return high_resolution ? t / 1000 : t;
}

static irqreturn_t rtl8198f_wdt_irq_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct rtl8198f_watchdog *wdt = platform_get_drvdata(pdev);

	spin_lock(&wdt->io_lock);

	rtk_wdt_w32(wdt, PER_WDT_INT_0, WDT_INT_WDTI);
	rtk_wdt_w32(wdt, PER_WDT_IE_0, 0);

	spin_unlock(&wdt->io_lock);

	dev_info(&pdev->dev,
		"8198F watchdog time out. Internal reset not enabled\n");
	return IRQ_HANDLED;
}

static int rtl8198f_wdt_notify_sys(struct notifier_block *this,
				unsigned long code, void *unused)
{
	struct rtl8198f_watchdog *wdt = container_of(this,
						     struct rtl8198f_watchdog,
						     wdt_notifier);
	if (code == SYS_DOWN || code == SYS_HALT)
		rtl8198f_wdt_stop(&wdt->wdd);

	return NOTIFY_DONE;
}

static struct watchdog_info rtl8198f_wdt_info = {
	.identity	= "Realtek RTL8198F Watchdog Timer",
	.options	= WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING |
				WDIOF_MAGICCLOSE
};

static struct watchdog_ops rtl8198f_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= rtl8198f_wdt_start,
	.stop		= rtl8198f_wdt_stop,
	.ping		= rtl8198f_wdt_ping,
	.set_timeout	= rtl8198f_wdt_set_timeout,
	.get_timeleft	= rtl8198f_wdt_get_timeleft,
};

static int rtl8198f_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rtl8198f_watchdog *wdt;
	struct resource *res;
	int err, irq;
	unsigned long clock_f;

	wdt = devm_kzalloc(dev, sizeof(*wdt), GFP_KERNEL);
	if (!wdt)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	wdt->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(wdt->base))
		return PTR_ERR(wdt->base);

	wdt->rst = of_property_read_bool(dev->of_node, "reset-on-timeout");
	irq = platform_get_irq(pdev, 0);
	if (!wdt->rst && irq >= 0) {
		err = devm_request_irq(dev, irq, rtl8198f_wdt_irq_handler,
					0, pdev->name, pdev);
		if (err) {
			dev_err(dev, "Fail to register WDT interrupt handler.\n"
				     "err=%d\n", err);
			return err;
		}
	} else if (wdt->rst) {
		u32 val;

		res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		wdt->glb_cfg = devm_ioremap_resource(dev, res);
		if (IS_ERR(wdt->glb_cfg))
			return PTR_ERR(wdt->glb_cfg);

		val = readl(wdt->glb_cfg);
		val |= WD_RESET_SUBSYS_ENABLE | WD_RESET_ALL_BLOCKS |
		       WD_RESET_REMAP | WD_RESET_EXT_RESET;
		writel_relaxed(val, wdt->glb_cfg);

		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		wdt->sec_rst = devm_ioremap_resource(dev, res);
		if (IS_ERR(wdt->sec_rst))
			return PTR_ERR(wdt->sec_rst);

		val = readl(wdt->sec_rst);
		val |= SEC_WDT_ENABLE;
		writel_relaxed(val, wdt->sec_rst);
	}

	wdt->wdd.info		= &rtl8198f_wdt_info;
	wdt->wdd.ops		= &rtl8198f_wdt_ops;
	wdt->wdd.timeout	= WDT_DEFAULT_TIMEOUT;
	wdt->wdd.min_timeout	= WDT_MIN_TIMEOUT;
	wdt->wdd.parent		= dev;
	if (high_resolution)
		wdt->wdd.max_timeout = U32_MAX / 1000;
	else
		wdt->wdd.max_timeout = U32_MAX;

	err = watchdog_init_timeout(&wdt->wdd, wdt_timeout, dev);
	if (err) {
		dev_err(dev, "Failed to init timeout\n");
		return err;
	}
	watchdog_set_nowayout(&wdt->wdd, nowayout);
	watchdog_set_drvdata(&wdt->wdd, wdt);

	wdt->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(wdt->clk)) {
		dev_err(dev, "Failed to get watchdog clock\n");
		err = PTR_ERR(wdt->clk);
		return err;
	}
	err = clk_prepare_enable(wdt->clk);
	if (err) {
		dev_err(dev, "Failed to enable watchdog clock\n");
		return err;
	}
	clock_f = clk_get_rate(wdt->clk);
	wdt->prescaler = clock_f / 1000 - 1;
	if (high_resolution) {
		wdt->ps_div = 0;
		wdt->ctrl_clksel = 1;
	} else {
		wdt->ps_div = 1000;
		wdt->ctrl_clksel = 0;
	}

	spin_lock_init(&wdt->io_lock);

	wdt->wdt_notifier.priority = 128;
	wdt->wdt_notifier.notifier_call = &rtl8198f_wdt_notify_sys;
	err = register_reboot_notifier(&wdt->wdt_notifier);
	if (err != 0) {
		dev_err(dev, "Failed to register notifier\n");
		goto err_clk_disable;
	}

	err = watchdog_register_device(&wdt->wdd);
	if (err) {
		dev_err(dev, "Failed to register watchdog device\n");
		iounmap(wdt->base);
		goto err_clk_disable;
	}
	platform_set_drvdata(pdev, wdt);
	dev_info(dev, "Registered RTL8198F Watchdog\n");

	return 0;

err_clk_disable:
	clk_disable_unprepare(wdt->clk);
	return err;

}

static int rtl8198f_wdt_remove(struct platform_device *pdev)
{
	struct rtl8198f_watchdog *wdt = platform_get_drvdata(pdev);

	if (!nowayout)
		rtl8198f_wdt_stop(&wdt->wdd);

	watchdog_unregister_device(&wdt->wdd);
	unregister_reboot_notifier(&wdt->wdt_notifier);
	clk_disable_unprepare(wdt->clk);

	return 0;
}

static void rtl8198f_wdt_shutdown(struct platform_device *pdev)
{
	struct rtl8198f_watchdog *wdt = platform_get_drvdata(pdev);

	if (watchdog_active(&wdt->wdd))
		rtl8198f_wdt_stop(&wdt->wdd);

	clk_disable_unprepare(wdt->clk);
}

static const struct of_device_id rtl8198f_wdt_match[] = {
	{ .compatible = "rtk,rtl8198f-wdt" },
	{},
};

static struct platform_driver rtl8198f_wdt_driver = {
	.probe		= rtl8198f_wdt_probe,
	.remove		= rtl8198f_wdt_remove,
	.shutdown	= rtl8198f_wdt_shutdown,
	.driver		= {
		.name		= DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= rtl8198f_wdt_match,
	}
};
module_platform_driver(rtl8198f_wdt_driver);

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
	__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");
module_param(wdt_timeout, int, 0);
MODULE_PARM_DESC(wdt_timeout, "Watchdog time in seconds. (default="
	__MODULE_STRING(CORTINA_WDT_DEFAULT_TIMEOUT) ")");

module_param(high_resolution, bool, 0);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Driver for Realtek RTL8198F Watchdog driver");
