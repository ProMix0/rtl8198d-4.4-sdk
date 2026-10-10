/*
 * rtc-ca77xx.c -- CA77XX Real Time Clock driver.
 *
 * Copyright (C) 2018 Cortina Access, Inc.
 *              http://www.cortina-access.com
 *
 *  Based on rtc-spear.c & rtc-palmas.c
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/bcd.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/spinlock.h>

/* #define SECURE_MODE	1 */

/* RTC registers */
#define CA77XX_RTC_ALARM_SEC		0x00
#define CA77XX_RTC_ALARM_MIN		0x04
#define CA77XX_RTC_ALARM_HOUR		0x08
#define CA77XX_RTC_ALARM_DAY		0x0C
#define CA77XX_RTC_ALARM_WEEK		0x10
#define CA77XX_RTC_ALARM_MONTH		0x14
#define CA77XX_RTC_ALARM_YEAR		0x18
#define CA77XX_RTC_USER_SRAM0		0x1C	/* secure access only */
#define CA77XX_RTC_USER_SRAM1		0x20	/* secure access only */
#define CA77XX_RTC_INT_EN		0x24
#define CA77XX_RTC_CNTL			0x28
#define CA77XX_RTC_COMP			0x2C
#define CA77XX_RTC_TIME_CNTR		0x30
#define CA77XX_RTC_CHANGING		0x34

#define CA77XX_RTC_INT_STATUS		0x3C
#define CA77XX_RTC_BCD_SEC		0x40
#define CA77XX_RTC_BCD_MIN		0x44
#define CA77XX_RTC_BCD_HOUR		0x48
#define CA77XX_RTC_BCD_DAY		0x4C
#define CA77XX_RTC_BCD_WEEK		0x50
#define CA77XX_RTC_BCD_MONTH		0x54
#define CA77XX_RTC_BCD_YEAR		0x58
#define CA77XX_RTC_BATTERY_OK		0x5C
#define CA77XX_RTC_LREG_BCD_SEC		0x60
#define CA77XX_RTC_LREG_BCD_MIN		0x64
#define CA77XX_RTC_LREG_BCD_HOUR	0x68
#define CA77XX_RTC_LREG_BCD_DAY		0x6C
#define CA77XX_RTC_LREG_BCD_WEEK	0x70
#define CA77XX_RTC_LREG_BCD_MONTH	0x74
#define CA77XX_RTC_LREG_BCD_YEAR	0x78

/* CNTL reg */
#define RTC_CNTL_WRITE_EN	0x0001
#define RTC_CNTL_SEL_WAKEUP	0x0002
#define RTC_CNTL_WRITE_ENABLED	0x0100
#define RTC_CNTL_TIME_LOADED	0x0200

/* INT_EN & INT_STATUS reg */
#define RTC_INT_SECONDLY	0x01
#define RTC_INT_MINUTELY	0x02
#define RTC_INT_HOURLY		0x04
#define RTC_INT_WEEKLY		0x08
#define RTC_INT_MONTHLY		0x10
#define RTC_INT_YEARLY		0x20
#define RTC_INT_DATE		0x40
#define RTC_INT_WAKEUP		0x80
#define RTC_INT_MASK		0xFF

struct ca77xx_rtc_config {
	struct rtc_device *rtc;
	spinlock_t lock;
	void __iomem *ioaddr;
	unsigned int irq_wake;
};

static inline void ca77xx_rtc_clear_interrupt(struct ca77xx_rtc_config *config)
{
#if (SECURE_MODE == 1)
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&config->lock, flags);
	val = readl(config->ioaddr + CA77XX_RTC_INT_STATUS);
	val |= RTC_INT_MASK;
	writel(val, config->ioaddr + CA77XX_RTC_INT_STATUS);
	spin_unlock_irqrestore(&config->lock, flags);
#else
	/* TBD */
	dev_warn(&config->rtc->dev, "%s() in Non Secure Mode - TBD!\n",
		 __func__);
#endif
}

static inline void ca77xx_rtc_enable_interrupt(struct ca77xx_rtc_config *config)
{
#if (SECURE_MODE == 1)
	unsigned long flags;

	spin_lock_irqsave(&config->lock, flags);
	writel(RTC_INT_DATE, config->ioaddr + CA77XX_RTC_INT_EN);
	spin_unlock_irqrestore(&config->lock, flags);
#else
	/* TBD */
	dev_warn(&config->rtc->dev, "%s() in Non Secure Mode - TBD!\n",
		 __func__);
#endif
}

static inline void ca77xx_rtc_disable_interrupt(
	struct ca77xx_rtc_config *config)
{
#if (SECURE_MODE == 1)
	unsigned long flags;

	spin_lock_irqsave(&config->lock, flags);
	writel(0, config->ioaddr + CA77XX_RTC_INT_STATUS);
	spin_unlock_irqrestore(&config->lock, flags);
#else
	/* TBD */
	dev_warn(&config->rtc->dev, "%s() in Non Secure Mode - TBD!\n",
		 __func__);
#endif
}

static irqreturn_t ca77xx_rtc_irq(int irq, void *dev_id)
{
	struct ca77xx_rtc_config *config = dev_id;
	unsigned long flags, events = 0;
	unsigned int irq_data;

	spin_lock_irqsave(&config->lock, flags);
	irq_data = readl(config->ioaddr + CA77XX_RTC_INT_STATUS);
	spin_unlock_irqrestore(&config->lock, flags);

	if ((irq_data & RTC_INT_MASK)) {
		ca77xx_rtc_clear_interrupt(config);
		events = RTC_IRQF | RTC_AF;
		rtc_update_irq(config->rtc, 1, events);
		return IRQ_HANDLED;
	} else {
		return IRQ_NONE;
	}
}

static int tm2bcd(struct rtc_time *tm)
{
	if (rtc_valid_tm(tm) != 0)
		return -EINVAL;

	tm->tm_sec = bin2bcd(tm->tm_sec);
	tm->tm_min = bin2bcd(tm->tm_min);
	tm->tm_hour = bin2bcd(tm->tm_hour);
	tm->tm_mday = bin2bcd(tm->tm_mday - 1);
	tm->tm_mon = bin2bcd(tm->tm_mon);
	/* epoch == 1900 */
	tm->tm_year = bin2bcd(tm->tm_year - 100);

	return 0;
}

static void bcd2tm(struct rtc_time *tm)
{
	tm->tm_sec = bcd2bin(tm->tm_sec);
	tm->tm_min = bcd2bin(tm->tm_min);
	tm->tm_hour = bcd2bin(tm->tm_hour);
	tm->tm_mday = bcd2bin(tm->tm_mday) + 1;
	tm->tm_mon = bcd2bin(tm->tm_mon);
	/* epoch == 1900 */
	tm->tm_year = bcd2bin(tm->tm_year) + 100;
}

/*
 * ca77xx_rtc_read_time - set the time
 * @dev: rtc device in use
 * @tm: holds date and time
 *
 * This function read time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int ca77xx_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct ca77xx_rtc_config *config = dev_get_drvdata(dev);

	tm->tm_sec = readl(config->ioaddr + CA77XX_RTC_BCD_SEC);
	tm->tm_min = readl(config->ioaddr + CA77XX_RTC_BCD_MIN);
	tm->tm_hour = readl(config->ioaddr + CA77XX_RTC_BCD_HOUR);
	tm->tm_mday = readl(config->ioaddr + CA77XX_RTC_BCD_DAY);
	tm->tm_mon = readl(config->ioaddr + CA77XX_RTC_BCD_MONTH);
	tm->tm_year = readl(config->ioaddr + CA77XX_RTC_BCD_YEAR);
	tm->tm_wday = readl(config->ioaddr + CA77XX_RTC_BCD_WEEK);

	bcd2tm(tm);

	return 0;
}

/*
 * ca77xx_rtc_set_time - set the time
 * @dev: rtc device in use
 * @tm: holds date and time
 *
 * This function set time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int ca77xx_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct ca77xx_rtc_config *config = dev_get_drvdata(dev);

	if (tm2bcd(tm) < 0)
		return -EINVAL;

#if (SECURE_MODE == 1)
	writel(tm->tm_sec, config->ioaddr + CA77XX_RTC_LREG_BCD_SEC);
	writel(tm->tm_min, config->ioaddr + CA77XX_RTC_LREG_BCD_MIN);
	writel(tm->tm_hour, config->ioaddr + CA77XX_RTC_LREG_BCD_HOUR);
	writel(tm->tm_mday, config->ioaddr + CA77XX_RTC_LREG_BCD_DAY);
	writel(tm->tm_mon, config->ioaddr + CA77XX_RTC_LREG_BCD_MONTH);
	writel(tm->tm_year, config->ioaddr + CA77XX_RTC_LREG_BCD_YEAR);
	writel(tm->tm_wday, config->ioaddr + CA77XX_RTC_LREG_BCD_WEEK);
#else
	/* TBD */
	dev_warn(&config->rtc->dev, "%s() in Non Secure Mode - TBD!\n",
		 __func__);
#endif

	return 0;
}

/*
 * ca77xx_rtc_read_alarm - read the alarm time
 * @dev: rtc device in use
 * @alm: holds alarm date and time
 *
 * This function read alarm time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int ca77xx_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	struct ca77xx_rtc_config *config = dev_get_drvdata(dev);

	alm->time.tm_sec = readl(config->ioaddr + CA77XX_RTC_ALARM_SEC);
	alm->time.tm_min = readl(config->ioaddr + CA77XX_RTC_ALARM_MIN);
	alm->time.tm_hour = readl(config->ioaddr + CA77XX_RTC_ALARM_HOUR);
	alm->time.tm_mday = readl(config->ioaddr + CA77XX_RTC_ALARM_DAY);
	alm->time.tm_mon = readl(config->ioaddr + CA77XX_RTC_ALARM_MONTH);
	alm->time.tm_year = readl(config->ioaddr + CA77XX_RTC_ALARM_YEAR);
	alm->time.tm_wday = readl(config->ioaddr + CA77XX_RTC_ALARM_WEEK);

	alm->enabled = !!(readl(config->ioaddr + CA77XX_RTC_INT_EN) &
			  RTC_INT_DATE);

	bcd2tm(&alm->time);

	return 0;
}

/*
 * ca77xx_rtc_set_alarm - set the alarm time
 * @dev: rtc device in use
 * @alm: holds alarm date and time
 *
 * This function set alarm time and date. On success it will return 0
 * otherwise -ve error is returned.
 */
static int ca77xx_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	struct ca77xx_rtc_config *config = dev_get_drvdata(dev);
	unsigned int time, date;
	int err;

	if (tm2bcd(&alm->time) < 0)
		return -EINVAL;

#if (SECURE_MODE == 1)
	writel(alm->time.tm_sec, config->ioaddr + CA77XX_RTC_ALARM_SEC);
	writel(alm->time.tm_min, config->ioaddr + CA77XX_RTC_ALARM_MIN);
	writel(alm->time.tm_hour, config->ioaddr + CA77XX_RTC_ALARM_HOUR);
	writel(alm->time.tm_mday, config->ioaddr + CA77XX_RTC_ALARM_DAY);
	writel(alm->time.tm_mon, config->ioaddr + CA77XX_RTC_ALARM_MONTH);
	writel(alm->time.tm_year, config->ioaddr + CA77XX_RTC_ALARM_YEAR);
	writel(alm->time.tm_wday, config->ioaddr + CA77XX_RTC_ALARM_WEEK);

	if (alm->enabled)
		ca77xx_rtc_enable_interrupt(config);
	else
		ca77xx_rtc_disable_interrupt(config);
#else
	/* TBD */
	dev_warn(&config->rtc->dev, "%s() in Non Secure Mode - TBD!\n",
		 __func__);
#endif

	return 0;
}

static int ca77xx_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct ca77xx_rtc_config *config = dev_get_drvdata(dev);
	int ret = 0;

	ca77xx_rtc_clear_interrupt(config);

	switch (enabled) {
	case 0:
		/* alarm off */
		ca77xx_rtc_disable_interrupt(config);
		break;
	case 1:
		/* alarm on */
		ca77xx_rtc_enable_interrupt(config);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct rtc_class_ops ca77xx_rtc_ops = {
	.read_time = ca77xx_rtc_read_time,
	.set_time = ca77xx_rtc_set_time,
	.read_alarm = ca77xx_rtc_read_alarm,
	.set_alarm = ca77xx_rtc_set_alarm,
	.alarm_irq_enable = ca77xx_alarm_irq_enable,
};

static int ca77xx_rtc_init(struct device *dev)
{
	struct ca77xx_rtc_config *config = dev_get_drvdata(dev);

#if (SECURE_MODE == 1)
	u32 val;
	int i = 0;

	writel(RTC_CNTL_WRITE_EN, config->ioaddr + CA77XX_RTC_CNTL);
	do {
		usleep_range(100, 200);
		val = readl(config->ioaddr + CA77XX_RTC_CNTL);
	} while (val & RTC_CNTL_WRITE_ENABLED & (i++ < 200));

	return (i < 200) ? 0 : -EPERM;
#else
	/* TBD */
	dev_warn(&config->rtc->dev, "%s() in Non Secure Mode - TBD!\n",
		 __func__);

	return 0;
#endif
}

static void ca77xx_rtc_exit(struct device *dev)
{
	struct ca77xx_rtc_config *config = dev_get_drvdata(dev);

#if (SECURE_MODE == 1)
	writel(0, config->ioaddr + CA77XX_RTC_CNTL);
#else
	/* TBD */
	dev_warn(&config->rtc->dev, "%s() in Non Secure Mode - TBD!\n",
		 __func__);
#endif
}

static int ca77xx_rtc_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct ca77xx_rtc_config *config;
	int status = 0;
	int irq;

	config = devm_kzalloc(&pdev->dev, sizeof(*config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	/* alarm irqs */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no update irq?\n");
		return irq;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	config->ioaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(config->ioaddr))
		return PTR_ERR(config->ioaddr);

	spin_lock_init(&config->lock);
	platform_set_drvdata(pdev, config);

	if (ca77xx_rtc_init(&pdev->dev))
		return -EIO;

	status = devm_request_irq(&pdev->dev, irq, ca77xx_rtc_irq, 0,
				  pdev->name, config);
	if (status) {
		dev_err(&pdev->dev, "Alarm interrupt IRQ%d already claimed\n",
			irq);
		return status;
	}

	if (!device_can_wakeup(&pdev->dev))
		device_init_wakeup(&pdev->dev, 1);

	config->rtc = devm_rtc_device_register(&pdev->dev, pdev->name,
					&ca77xx_rtc_ops, THIS_MODULE);
	if (IS_ERR(config->rtc)) {
		dev_err(&pdev->dev, "can't register RTC device, err %ld\n",
			PTR_ERR(config->rtc));
		status = PTR_ERR(config->rtc);
		return status;
	}

	return 0;
}

static int ca77xx_rtc_remove(struct platform_device *pdev)
{
	struct ca77xx_rtc_config *config = platform_get_drvdata(pdev);

	ca77xx_rtc_disable_interrupt(config);
	device_init_wakeup(&pdev->dev, 0);
	ca77xx_rtc_exit(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ca77xx_rtc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ca77xx_rtc_config *config = platform_get_drvdata(pdev);
	int irq;

	irq = platform_get_irq(pdev, 0);
	if (device_may_wakeup(&pdev->dev)) {
		if (!enable_irq_wake(irq))
			config->irq_wake = 1;
	} else {
		ca77xx_rtc_disable_interrupt(config);
	}

	return 0;
}

static int ca77xx_rtc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ca77xx_rtc_config *config = platform_get_drvdata(pdev);
	int irq;

	irq = platform_get_irq(pdev, 0);

	if (device_may_wakeup(&pdev->dev)) {
		if (config->irq_wake) {
			disable_irq_wake(irq);
			config->irq_wake = 0;
		}
	} else {
		ca77xx_rtc_enable_interrupt(config);
	}

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(ca77xx_rtc_pm_ops, ca77xx_rtc_suspend,
			 ca77xx_rtc_resume);

static void ca77xx_rtc_shutdown(struct platform_device *pdev)
{
	struct ca77xx_rtc_config *config = platform_get_drvdata(pdev);

	ca77xx_rtc_disable_interrupt(config);
}

#ifdef CONFIG_OF
static const struct of_device_id ca77xx_rtc_id_table[] = {
	{ .compatible = "cortina,ca77xx-rtc" },
	{ }
};
MODULE_DEVICE_TABLE(of, ca77xx_rtc_id_table);
#endif

static struct platform_driver ca77xx_rtc_driver = {
	.probe = ca77xx_rtc_probe,
	.remove = ca77xx_rtc_remove,
	.shutdown = ca77xx_rtc_shutdown,
	.driver = {
		.name = "rtc-ca77xx",
		.pm = &ca77xx_rtc_pm_ops,
		.of_match_table = of_match_ptr(ca77xx_rtc_id_table),
	},
};

module_platform_driver(ca77xx_rtc_driver);

MODULE_ALIAS("platform:rtc-ca77xx");
MODULE_DESCRIPTION("Cortina Access CA77XX series RTC driver");
MODULE_LICENSE("GPL v2");
