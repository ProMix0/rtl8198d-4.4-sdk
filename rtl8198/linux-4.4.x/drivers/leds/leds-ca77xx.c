/*
 * Driver for CA77XX memory-mapped serial LEDs
 *
 * Copyright (C) 2017 Cortina Access, Inc.
 *		http://www.cortina-access.com
 *
 * based on leds-bcm6328.c
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/io.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/leds-ca77xx.h>

#define CA77XX_LED_CONTROL		0x00
#define CA77XX_LED_CONFIG_0		0x04
#define CA77XX_LED_CONFIG_1		0x08
#define CA77XX_LED_CONFIG_2		0x0c
#define CA77XX_LED_CONFIG_3		0x10
#define CA77XX_LED_CONFIG_4		0x14
#define CA77XX_LED_CONFIG_5		0x18
#define CA77XX_LED_CONFIG_6		0x1c
#define CA77XX_LED_CONFIG_7		0x20
#define CA77XX_LED_CONFIG_8		0x24
#define CA77XX_LED_CONFIG_9		0x28
#define CA77XX_LED_CONFIG_10		0x2c
#define CA77XX_LED_CONFIG_11		0x30
#define CA77XX_LED_CONFIG_12		0x34
#define CA77XX_LED_CONFIG_13		0x38
#define CA77XX_LED_CONFIG_14		0x3c
#define CA77XX_LED_CONFIG_15		0x40

#define CA77XX_LED_MAX_HW_BLINK		127
#define CA77XX_LED_MAX_COUNT		16
#define CA77XX_LED_MAX_PORT		8

/* LED_CONTROL fields */
#define CA77XX_LED_BLINK_RATE1_OFFSET	0
#define CA77XX_LED_BLINK_RATE1_MASK	0xFF
#define CA77XX_LED_BLINK_RATE2_OFFSET	8
#define CA77XX_LED_BLINK_RATE2_MASK	0xFF
#define CA77XX_LED_CLK_TEST		BIT(16)
#define CA77XX_LED_CLK_POLARITY		BIT(17)
#define CA77XX_LED_CLK_TEST_MODE	BIT(16)
#define CA77XX_LED_CLK_TEST_RX_TEST	BIT(30)
#define CA77XX_LED_CLK_TEST_TX_TEST	BIT(31)

/* LED_CONFIG fields */
#define CA77XX_LED_EVENT_ON_OFFSET	0
#define CA77XX_LED_EVENT_ON_MASK	0x7
#define CA77XX_LED_EVENT_BLINK_OFFSET	3
#define CA77XX_LED_EVENT_BLINK_MASK	0x7
#define CA77XX_LED_EVENT_OFF_OFFSET	6
#define CA77XX_LED_EVENT_OFF_MASK	0x7
#define CA77XX_LED_OFF_ON_OFFSET	9
#define CA77XX_LED_OFF_ON_MASK		0x3
#define CA77XX_LED_PORT_OFFSET		11
#define CA77XX_LED_PORT_MASK		0x7
#define CA77XX_LED_OFF_VAL		BIT(14)
#define CA77XX_LED_SW_EVENT		BIT(15)
#define CA77XX_LED_BLINK_SEL		BIT(16)

/**
 * struct ca77xx_led_cfg - configuration for LEDs
 * @cdev: LED class device for this LED
 * @mem: memory resource
 * @lock: memory lock
 * @idx: LED index number
 * @active_low: LED is active low
 * @off_event: off triggered by rx/tx/sw event
 * @blink_event: blinking triggered by rx/tx/sw event
 * @on_event: on triggered by rx/tx/sw event
 * @port: monitor port
 * @blink: haredware blink rate select
 * @enable: LED is enabled/disabled
 */
struct ca77xx_led_cfg {
	struct led_classdev cdev;
	void __iomem *mem;	
	spinlock_t *lock;	/* protect LED resource access */
	unsigned long data;
	int idx;
	bool active_low;

	int off_event;
	int blink_event;
	int on_event;
	int port;
	int blink;
	int enable;
	struct device *dev;
};

/**
 * struct ca77xx_led_cfg - control for LEDs
 * @mem: memory resource
 * @lock: memory lock
 * @test_mode: enter test mode
 * @clk_low: clock polarity
 * @blink_rate1: haredware blink rate 1
 * @blink_rate2: haredware blink rate 2
 * @ca77xx_led: configuration for LEDs
 */
struct ca77xx_led_ctrl {
	void __iomem *mem;
	spinlock_t *lock;	/* protect LED resource access */

	int test_mode;
	int clk_low;
	u16 blink_rate1;
	u16 blink_rate2;

	struct device *dev;

	struct ca77xx_led_cfg *led_cfg[CA77XX_LED_MAX_COUNT];
};

static char *trigger_str[] = {
	[TRIGGER_HW_RX] = "rx",
	[TRIGGER_HW_TX] = "tx",
	[TRIGGER_SW] = "sw",
};

static char *rate_str[] = {
	[BLINK_RATE1] = "blink_rate1",
	[BLINK_RATE2] = "blink_rate2",
};

static struct ca77xx_led_ctrl *glb_led_ctrl;
static int led_control=2;

void ca77xx_led_set_all(int control) {
	unsigned long flags;
	int idx;
	struct ca77xx_led_cfg *led_cfg;
		
	spin_lock_irqsave(glb_led_ctrl->lock, flags);
	if (led_control==control)
		goto out;
	
	if ((led_control > 2) || (led_control < 0))
		goto out;
		
	led_control = control;
	for (idx=0; idx<CA77XX_LED_MAX_COUNT; idx++) {
		unsigned long val;
		led_cfg = glb_led_ctrl->led_cfg[idx];
		val = led_cfg->data & ~(CA77XX_LED_OFF_ON_MASK << CA77XX_LED_OFF_ON_OFFSET) & ~(CA77XX_LED_EVENT_BLINK_MASK << CA77XX_LED_EVENT_BLINK_OFFSET);				
		switch(control) {		
		case 0:			
			iowrite32(val & ~CA77XX_LED_SW_EVENT, led_cfg->mem);
			break;
		case 1:			
			iowrite32(val | CA77XX_LED_SW_EVENT, led_cfg->mem);
			break;
		case 2:
			iowrite32(led_cfg->data, led_cfg->mem);
			break;
		}
	}
out:	
	spin_unlock_irqrestore(glb_led_ctrl->lock, flags);
}
EXPORT_SYMBOL(ca77xx_led_set_all);

int ca77xx_led_get_all(void) {
	return led_control;
}
EXPORT_SYMBOL(ca77xx_led_get_all);

static void ca77xx_led_write(void __iomem *reg, unsigned long data, struct ca77xx_led_cfg *cfg)
{
	if (NULL==cfg)
		iowrite32(data, reg);
	else {
		cfg->data = data;
		if (led_control==2)		
			iowrite32(data, reg);
	}
}

static unsigned long ca77xx_led_read(void __iomem *reg, struct ca77xx_led_cfg *cfg)
{
	if (NULL==cfg)
		return ioread32(reg);
	else 
		return cfg->data;
}

int ca77xx_led_sw_on(int idx, int on)
{
	struct ca77xx_led_cfg *led_cfg;
	unsigned long flags;
	u32 val;

	if (idx >= CA77XX_LED_MAX_COUNT)
		return -EINVAL;

	led_cfg = glb_led_ctrl->led_cfg[idx];

	spin_lock_irqsave(led_cfg->lock, flags);
	val = ca77xx_led_read(led_cfg->mem, led_cfg);
	if (on)
		val |= CA77XX_LED_SW_EVENT;
	else
		val &= ~CA77XX_LED_SW_EVENT;

	ca77xx_led_write(led_cfg->mem, val, led_cfg);
	spin_unlock_irqrestore(led_cfg->lock, flags);

	return 0;
}
EXPORT_SYMBOL(ca77xx_led_sw_on);

/**
 * ca77xx_led_enable - switch LED to off or triggered by event
 */
int ca77xx_led_enable(int idx, int enable)
{
	struct ca77xx_led_cfg *led_cfg;
	unsigned long flags;
	u32 val;

	if (idx >= CA77XX_LED_MAX_COUNT)
		return -EINVAL;

	led_cfg = glb_led_ctrl->led_cfg[idx];

	spin_lock_irqsave(led_cfg->lock, flags);

	val = ca77xx_led_read(led_cfg->mem, led_cfg);

	if (enable) {
		led_cfg->enable = 1;

		/* driven by event */
		val &= ~(CA77XX_LED_OFF_ON_MASK << CA77XX_LED_OFF_ON_OFFSET);
	} else {
		led_cfg->enable = 0;

		/* force off */
		val &= ~(CA77XX_LED_OFF_ON_MASK << CA77XX_LED_OFF_ON_OFFSET);
		val |= 0x3 << CA77XX_LED_OFF_ON_OFFSET;
	}

	ca77xx_led_write(led_cfg->mem, val, led_cfg);

	spin_unlock_irqrestore(led_cfg->lock, flags);

	return 0;
}
EXPORT_SYMBOL(ca77xx_led_enable);

/**
 * ca77xx_led_config - config LED trigger behavior
 */
int ca77xx_led_config(int idx, int active_low, int off_event, int blink_event,
		      int on_event, int port, int blink)
{
	struct ca77xx_led_cfg *led_cfg;
	unsigned long flags;
	u32 val;

	if (idx >= CA77XX_LED_MAX_COUNT)
		return -EINVAL;
	if (off_event > TRIGGER_NONE)
		return -EINVAL;
	if (blink_event > TRIGGER_NONE)
		return -EINVAL;
	if (on_event > TRIGGER_NONE)
		return -EINVAL;
	if (port >= CA77XX_LED_MAX_PORT)
		return -EINVAL;
	if (blink > BLINK_RATE2)
		return -EINVAL;

	led_cfg = glb_led_ctrl->led_cfg[idx];

	spin_lock_irqsave(led_cfg->lock, flags);

	val = ca77xx_led_read(led_cfg->mem, led_cfg);

	if (active_low) {
		led_cfg->active_low = true;
		val |= CA77XX_LED_OFF_VAL;
	} else {
		led_cfg->active_low = false;
		val &= ~CA77XX_LED_OFF_VAL;
	}

	led_cfg->off_event = off_event;
	led_cfg->blink_event = blink_event;
	led_cfg->on_event = on_event;
	led_cfg->blink = blink;

	if (blink == BLINK_RATE1)
		val &= ~CA77XX_LED_BLINK_SEL;
	else if (blink == BLINK_RATE2)
		val |= CA77XX_LED_BLINK_SEL;

	val &= ~(CA77XX_LED_EVENT_OFF_MASK << CA77XX_LED_EVENT_OFF_OFFSET);
	if (off_event != TRIGGER_NONE)
		val |= BIT(off_event) << CA77XX_LED_EVENT_OFF_OFFSET;

	val &= ~(CA77XX_LED_EVENT_BLINK_MASK << CA77XX_LED_EVENT_BLINK_OFFSET);
	if (blink_event != TRIGGER_NONE)
		val |= BIT(blink_event) << CA77XX_LED_EVENT_BLINK_OFFSET;

	val &= ~(CA77XX_LED_EVENT_ON_MASK << CA77XX_LED_EVENT_ON_OFFSET);
	if (on_event != TRIGGER_NONE)
		val |= BIT(on_event) << CA77XX_LED_EVENT_ON_OFFSET;

	led_cfg->port = port;
	val &= ~(CA77XX_LED_PORT_MASK << CA77XX_LED_PORT_OFFSET);
	val |= port << CA77XX_LED_PORT_OFFSET;

	ca77xx_led_write(led_cfg->mem, val, led_cfg);

	spin_unlock_irqrestore(led_cfg->lock, flags);

	return 0;
}
EXPORT_SYMBOL(ca77xx_led_config);

/**
 * ca77xx_led_test_mode - switch all LEDs into/from test mode
 */
int ca77xx_led_test_mode(int enable)
{
	struct ca77xx_led_cfg *led_cfg;
	struct led_classdev *led_cdev;
	unsigned long flags;
	u32 val;
	int i;

	if (enable) {
		spin_lock_irqsave(glb_led_ctrl->lock, flags);

		glb_led_ctrl->test_mode = 1;

		/* hardware generate tx/rx event automatically */
		for (i = 0; i < CA77XX_LED_MAX_COUNT; i++) {
			if (!glb_led_ctrl->led_cfg[i])
				continue;

			led_cfg = glb_led_ctrl->led_cfg[i];
			led_cdev = &led_cfg->cdev;

			/* change to hw rx trigger with blink */
			led_sysfs_disable(led_cdev);
			val = ca77xx_led_read(led_cfg->mem, led_cfg);
			val &= ~(CA77XX_LED_EVENT_ON_MASK <<
				 CA77XX_LED_EVENT_ON_OFFSET);
			val &= ~(CA77XX_LED_EVENT_BLINK_MASK <<
				 CA77XX_LED_EVENT_BLINK_OFFSET);
			val |= BIT(0) << CA77XX_LED_EVENT_BLINK_OFFSET;
			val |= CA77XX_LED_BLINK_SEL;
			ca77xx_led_write(led_cfg->mem, val, led_cfg);
		}

		val = ca77xx_led_read(glb_led_ctrl->mem, NULL);
		val |= CA77XX_LED_CLK_TEST_MODE |
		       CA77XX_LED_CLK_TEST_RX_TEST |
		       CA77XX_LED_CLK_TEST_TX_TEST;
		ca77xx_led_write(glb_led_ctrl->mem, val, NULL);

		spin_unlock_irqrestore(glb_led_ctrl->lock, flags);
	} else {
		spin_lock_irqsave(glb_led_ctrl->lock, flags);

		glb_led_ctrl->test_mode = 0;

		val = ca77xx_led_read(glb_led_ctrl->mem, NULL);
		val &= ~(CA77XX_LED_CLK_TEST_MODE |
			 CA77XX_LED_CLK_TEST_RX_TEST |
			 CA77XX_LED_CLK_TEST_TX_TEST);
		ca77xx_led_write(glb_led_ctrl->mem, val, NULL);

		spin_unlock_irqrestore(glb_led_ctrl->lock, flags);

		for (i = 0; i < CA77XX_LED_MAX_COUNT; i++) {
			led_cfg = glb_led_ctrl->led_cfg[i];

			ca77xx_led_config(i, led_cfg->active_low,
					  led_cfg->off_event,
					  led_cfg->blink_event,
					  led_cfg->on_event, led_cfg->port,
					  led_cfg->blink);
		}
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_led_test_mode);

/**
 * ca77xx_led_set - set on/off software event
 */
static void ca77xx_led_set(struct led_classdev *led_cdev,
			   enum led_brightness value)
{
	struct ca77xx_led_cfg *led_cfg =
		container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(led_cfg->lock, flags);
	val = ca77xx_led_read(led_cfg->mem, led_cfg);
	if (value == LED_OFF)
		val &= ~CA77XX_LED_SW_EVENT;
	else
		val |= CA77XX_LED_SW_EVENT;
	ca77xx_led_write(led_cfg->mem, val, led_cfg);
	spin_unlock_irqrestore(led_cfg->lock, flags);
}

static int ca77xx_blink_set(struct led_classdev *led_cdev,
			    unsigned long *delay_on, unsigned long *delay_off)
{
	struct ca77xx_led_cfg *led_cfg =
		container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	struct ca77xx_led_ctrl *glb_led;
	unsigned long delay, flags;
	u32 val;

	/* limit sw not to use hardware blink */
	if (led_cfg->blink_event == TRIGGER_SW)
		return -EINVAL;

	glb_led = dev_get_drvdata(led_cfg->dev);

	if (*delay_on != *delay_off) {
		dev_dbg(led_cdev->dev,
			"fallback to soft blinking (delay_on != delay_off)\n");
		return -EINVAL;
	}

	if (*delay_on == ((glb_led->blink_rate1 + 1) * 16)) {
		delay = 0;
	} else if (*delay_on == ((glb_led->blink_rate2 + 1) * 16)) {
		delay = 1;
	} else {
		dev_dbg(led_cdev->dev, "fallback to soft blinking\n");
		return -EINVAL;
	}

	spin_lock_irqsave(led_cfg->lock, flags);
	val = ca77xx_led_read(led_cfg->mem, led_cfg);
	if (delay)
		val |= CA77XX_LED_BLINK_SEL;
	else
		val &= ~CA77XX_LED_BLINK_SEL;
	ca77xx_led_write(led_cfg->mem, val, led_cfg);
	spin_unlock_irqrestore(led_cfg->lock, flags);

	return 0;
}

/**
 * ca77xx_led_show_hw_event - show hardware blink event
 */
static ssize_t ca77xx_led_show_hw_event(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags;
	ssize_t len = 0;
	int i;

	spin_lock_irqsave(led_cfg->lock, flags);

	for (i = 0; i <= TRIGGER_SW; i++) {
		if (i == led_cfg->blink_event)
			len += sprintf(buf + len, "[%s] ", trigger_str[i]);
		else
			len += sprintf(buf + len, "%s ", trigger_str[i]);
	}

	spin_unlock_irqrestore(led_cfg->lock, flags);

	len += sprintf(buf + len, "\n");
	return len;
}

/**
 * ca77xx_led_store_hw_event - set hardware blink event
 */
static ssize_t ca77xx_led_store_hw_event(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags;
	u32 val;
	int i;

	for (i = 0; i <= TRIGGER_SW; i++) {
		if (!strncmp(buf, trigger_str[i], strlen(trigger_str[i])))
			break;
	}
	if (i > TRIGGER_SW)
		return size;

	spin_lock_irqsave(led_cfg->lock, flags);

	led_cfg->blink_event = i;

	val = ca77xx_led_read(led_cfg->mem, led_cfg);
	val &= ~(CA77XX_LED_EVENT_ON_MASK << CA77XX_LED_EVENT_ON_OFFSET);
	val &= ~(CA77XX_LED_EVENT_BLINK_MASK << CA77XX_LED_EVENT_BLINK_OFFSET);
	if ((led_cfg->blink_event == TRIGGER_HW_RX) ||
	    (led_cfg->blink_event == TRIGGER_HW_TX)) {
		if (led_cfg->blink_event)
			val |= BIT(led_cfg->blink_event) <<
			       CA77XX_LED_EVENT_BLINK_OFFSET;
		else
			val |= BIT(led_cfg->blink_event) <<
			       CA77XX_LED_EVENT_ON_OFFSET;
	} else if (led_cfg->blink_event == TRIGGER_SW) {
		val |= BIT(led_cfg->blink_event) << CA77XX_LED_EVENT_ON_OFFSET;
	}
	ca77xx_led_write(led_cfg->mem, val, led_cfg);

	spin_unlock_irqrestore(led_cfg->lock, flags);

	if (led_cfg->blink_event == TRIGGER_SW)
		led_sysfs_enable(&led_cfg->cdev);
	else
		led_sysfs_disable(&led_cfg->cdev);

	return size;
}

static DEVICE_ATTR(hw_event, 0644, ca77xx_led_show_hw_event,
		   ca77xx_led_store_hw_event);

static ssize_t ca77xx_led_show_hw_port(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags;
	ssize_t ret = 0;

	spin_lock_irqsave(led_cfg->lock, flags);

	ret += sprintf(buf, "%d\n", led_cfg->port);

	spin_unlock_irqrestore(led_cfg->lock, flags);

	return ret;
}

static ssize_t ca77xx_led_store_hw_port(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags, param;

	u32 val;

	kstrtoul(buf, 10, &param);

	spin_lock_irqsave(led_cfg->lock, flags);

	led_cfg->port = param & CA77XX_LED_PORT_MASK;

	val = ca77xx_led_read(led_cfg->mem, led_cfg);
	val &= ~(CA77XX_LED_PORT_MASK << CA77XX_LED_PORT_OFFSET);
	val |= led_cfg->port << CA77XX_LED_PORT_OFFSET;
	ca77xx_led_write(led_cfg->mem, val, led_cfg);

	spin_unlock_irqrestore(led_cfg->lock, flags);

	return size;
}

static DEVICE_ATTR(hw_port, 0644, ca77xx_led_show_hw_port,
		   ca77xx_led_store_hw_port);

static ssize_t ca77xx_led_show_hw_blink(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags;
	ssize_t len = 0;
	int i;

	spin_lock_irqsave(led_cfg->lock, flags);

	for (i = 0; i <= BLINK_RATE2; i++) {
		if (i == led_cfg->blink)
			len += sprintf(buf + len, "[%s] ", rate_str[i]);
		else
			len += sprintf(buf + len, "%s ", rate_str[i]);
	}

	spin_unlock_irqrestore(led_cfg->lock, flags);

	len += sprintf(buf + len, "\n");
	return len;
}

static ssize_t ca77xx_led_store_hw_blink(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags;
	u32 val;
	int i;

	for (i = 0; i <= BLINK_RATE2; i++) {
		if (!strncmp(buf, rate_str[i], strlen(rate_str[i])))
			break;
	}
	if (i > BLINK_RATE2)
		return size;

	spin_lock_irqsave(led_cfg->lock, flags);

	led_cfg->blink = i;

	val = ca77xx_led_read(led_cfg->mem, led_cfg);
	if (led_cfg->blink == BLINK_RATE1)
		val &= ~CA77XX_LED_BLINK_SEL;
	else if (led_cfg->blink == BLINK_RATE2)
		val |= CA77XX_LED_BLINK_SEL;
	ca77xx_led_write(led_cfg->mem, val, led_cfg);

	spin_unlock_irqrestore(led_cfg->lock, flags);

	return size;
}

static DEVICE_ATTR(hw_blink, 0644, ca77xx_led_show_hw_blink,
		   ca77xx_led_store_hw_blink);

static ssize_t ca77xx_led_show_hw_enable(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags;
	ssize_t ret = 0;

	spin_lock_irqsave(led_cfg->lock, flags);

	ret += sprintf(buf, "%d\n", led_cfg->enable);

	spin_unlock_irqrestore(led_cfg->lock, flags);

	return ret;
}

static ssize_t ca77xx_led_store_hw_enable(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long param;

	kstrtoul(buf, 10, &param);

	ca77xx_led_enable(led_cfg->idx, param);

	return size;
}

static DEVICE_ATTR(hw_enable, 0644, ca77xx_led_show_hw_enable,
		   ca77xx_led_store_hw_enable);

static ssize_t ca77xx_led_show_hw_id(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ca77xx_led_cfg *led_cfg =
			container_of(led_cdev, struct ca77xx_led_cfg, cdev);
	unsigned long flags;
	ssize_t ret = 0;

	spin_lock_irqsave(led_cfg->lock, flags);

	ret += sprintf(buf, "%d\n", led_cfg->idx);

	spin_unlock_irqrestore(led_cfg->lock, flags);

	return ret;
}

static DEVICE_ATTR(hw_id, 0644, ca77xx_led_show_hw_id, NULL);

static struct attribute *ca77xx_led_attrs[] = {
	&dev_attr_hw_id.attr,
	&dev_attr_hw_event.attr,
	&dev_attr_hw_port.attr,
	&dev_attr_hw_blink.attr,
	&dev_attr_hw_enable.attr,
	NULL
};
ATTRIBUTE_GROUPS(ca77xx_led);

static int ca77xx_led_probe(struct ca77xx_led_ctrl *led_ctrl,
			    struct device_node *nc, u32 reg, void __iomem *mem,
			    spinlock_t *lock)
{
	struct ca77xx_led_cfg *led_cfg;
	unsigned long flags;
	const char *state;
	u32 val;
	int rc, active_low, off_event, blink_event, on_event, port, blink;

	led_cfg = devm_kzalloc(led_ctrl->dev, sizeof(*led_cfg), GFP_KERNEL);
	if (!led_cfg)
		return -ENOMEM;
	led_ctrl->led_cfg[reg] = led_cfg;

	led_cfg->idx = reg;
	led_cfg->mem = mem;
	led_cfg->lock = lock;
	led_cfg->dev = led_ctrl->dev;
	led_cfg->data = ioread32(mem);

	led_cfg->cdev.name = of_get_property(nc, "label", NULL) ? : nc->name;
	led_cfg->cdev.default_trigger = of_get_property(nc,
						    "linux,default-trigger",
						    NULL);

	if (of_property_read_bool(nc, "active-low"))
		active_low = 1;
	else
		active_low = 0;

	if (of_property_read_bool(nc, "off,hw-rx"))
		off_event = TRIGGER_HW_RX;
	else if (of_property_read_bool(nc, "off,hw-tx"))
		off_event = TRIGGER_HW_TX;
	else if (of_property_read_bool(nc, "off,sw"))
		off_event = TRIGGER_SW;
	else
		off_event = TRIGGER_NONE;

	if (of_property_read_bool(nc, "blinking,hw-rx"))
		blink_event = TRIGGER_HW_RX;
	else if (of_property_read_bool(nc, "blinking,hw-tx"))
		blink_event = TRIGGER_HW_TX;
	else if (of_property_read_bool(nc, "blinking,sw"))
		blink_event = TRIGGER_SW;
	else
		blink_event = TRIGGER_NONE;

	if (of_property_read_bool(nc, "on,hw-rx"))
		on_event = TRIGGER_HW_RX;
	else if (of_property_read_bool(nc, "on,hw-tx"))
		on_event = TRIGGER_HW_TX;
	else if (of_property_read_bool(nc, "on,sw"))
		on_event = TRIGGER_SW;
	else
		on_event = TRIGGER_NONE;

	if (!of_property_read_u32(nc, "port", &val))
		port = val;
	else
		port = 0;

	if (of_property_read_bool(nc, "blink-rate1"))
		blink = BLINK_RATE1;
	else
		blink = BLINK_RATE2;

	ca77xx_led_config(led_cfg->idx, active_low, off_event, blink_event,
			  on_event, port, blink);

	spin_lock_irqsave(lock, flags);

	if (!of_property_read_string(nc, "default-state", &state)) {
		if (!strcmp(state, "disabled")) {
			led_cfg->enable = 0;
			led_cfg->cdev.brightness = LED_OFF;
		} else {
			led_cfg->enable = 1;

			if (!strcmp(state, "on")) {
				led_cfg->cdev.brightness = LED_FULL;
			} else if (!strcmp(state, "keep")) {
#if 0 /* not support now */
				if (reg & CA77XX_LED_SW_EVENT)
					led_cfg->cdev.brightness = LED_FULL;
				else
					led_cfg->cdev.brightness = LED_OFF;
#endif
			}
		}
	} else {
		led_cfg->enable = 1;
		led_cfg->cdev.brightness = LED_OFF;
	}

	spin_unlock_irqrestore(lock, flags);

	if (led_cfg->enable)
		ca77xx_led_set(&led_cfg->cdev, led_cfg->cdev.brightness);

	ca77xx_led_enable(led_cfg->idx, led_cfg->enable);

	led_cfg->cdev.brightness_set = ca77xx_led_set;
	led_cfg->cdev.blink_set = ca77xx_blink_set;
	led_cfg->cdev.groups = ca77xx_led_groups;

	rc = devm_led_classdev_register(led_ctrl->dev, &led_cfg->cdev);
	if (rc < 0)
		return rc;

	dev_set_drvdata(led_cfg->cdev.dev, led_cfg);

	dev_dbg(led_ctrl->dev, "registered LED %s\n", led_cfg->cdev.name);

	if (led_cfg->on_event == TRIGGER_SW) {
		led_sysfs_enable(&led_cfg->cdev);
	} else {
		led_sysfs_disable(&led_cfg->cdev);
		led_cfg->cdev.default_trigger = "none";
	}

	return 0;
}

static ssize_t ca77xx_leds_test_mode_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct ca77xx_led_ctrl *led_ctrl = dev_get_drvdata(dev);
	unsigned long flags;
	ssize_t ret = 0;

	spin_lock_irqsave(led_ctrl->lock, flags);

	ret += sprintf(buf, "%d\n", led_ctrl->test_mode);

	spin_unlock_irqrestore(led_ctrl->lock, flags);

	return ret;
}

static ssize_t ca77xx_leds_test_mode_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	unsigned long param;

	kstrtoul(buf, 10, &param);

	ca77xx_led_test_mode(param);

	return count;
}
static DEVICE_ATTR(test_mode, 0664, ca77xx_leds_test_mode_show,
			ca77xx_leds_test_mode_store);

static ssize_t ca77xx_leds_clk_low_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct ca77xx_led_ctrl *led_ctrl = dev_get_drvdata(dev);
	unsigned long flags;
	ssize_t ret = 0;

	spin_lock_irqsave(led_ctrl->lock, flags);

	ret += sprintf(buf, "%d\n", led_ctrl->clk_low);

	spin_unlock_irqrestore(led_ctrl->lock, flags);

	return ret;
}

static ssize_t ca77xx_leds_clk_low_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct ca77xx_led_ctrl *led_ctrl = dev_get_drvdata(dev);
	unsigned long flags, param;
	u32 val;

	kstrtoul(buf, 10, &param);

	spin_lock_irqsave(led_ctrl->lock, flags);
	val = ca77xx_led_read(led_ctrl->mem, NULL);
	if (param) {
		led_ctrl->clk_low = 1;
		val |= CA77XX_LED_CLK_POLARITY;
	} else {
		led_ctrl->clk_low = 0;
		val &= ~CA77XX_LED_CLK_POLARITY;
	}
	ca77xx_led_write(led_ctrl->mem, val, NULL);
	spin_unlock_irqrestore(led_ctrl->lock, flags);

	return count;
}
static DEVICE_ATTR(clk_low, 0664, ca77xx_leds_clk_low_show,
			ca77xx_leds_clk_low_store);

static ssize_t ca77xx_leds_blink_rate1_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct ca77xx_led_ctrl *led_ctrl = dev_get_drvdata(dev);
	unsigned long flags;
	ssize_t ret = 0;

	spin_lock_irqsave(led_ctrl->lock, flags);

	ret += sprintf(buf, "%d\n", (led_ctrl->blink_rate1 + 1) * 16);

	spin_unlock_irqrestore(led_ctrl->lock, flags);

	return ret;
}

static ssize_t ca77xx_leds_blink_rate1_store(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf, size_t count)
{
	struct ca77xx_led_ctrl *led_ctrl = dev_get_drvdata(dev);
	unsigned long flags, param;
	u32 val;

	kstrtoul(buf, 10, &param);

	if (param >= 16) {
		param = param / 16 - 1;
		if (param > CA77XX_LED_MAX_HW_BLINK)
			param = CA77XX_LED_MAX_HW_BLINK;
	} else {
		param = 0;
	}

	spin_lock_irqsave(led_ctrl->lock, flags);
	led_ctrl->blink_rate1 = param;
	val = ca77xx_led_read(led_ctrl->mem, NULL);
	val &= ~(CA77XX_LED_BLINK_RATE1_MASK << CA77XX_LED_BLINK_RATE1_OFFSET);
	val |= led_ctrl->blink_rate1 << CA77XX_LED_BLINK_RATE1_OFFSET;
	ca77xx_led_write(led_ctrl->mem, val, NULL);
	spin_unlock_irqrestore(led_ctrl->lock, flags);

	return count;
}
static DEVICE_ATTR(blink_rate1, 0664, ca77xx_leds_blink_rate1_show,
			ca77xx_leds_blink_rate1_store);

static ssize_t ca77xx_leds_blink_rate2_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct ca77xx_led_ctrl *led_ctrl = dev_get_drvdata(dev);
	unsigned long flags;
	ssize_t ret = 0;

	spin_lock_irqsave(led_ctrl->lock, flags);

	ret += sprintf(buf, "%d\n", (led_ctrl->blink_rate2 + 1) * 16);

	spin_unlock_irqrestore(led_ctrl->lock, flags);

	return ret;
}

static ssize_t ca77xx_leds_blink_rate2_store(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf, size_t count)
{
	struct ca77xx_led_ctrl *led_ctrl = dev_get_drvdata(dev);
	unsigned long flags, param;
	u32 val;

	kstrtoul(buf, 10, &param);

	param = param / 16 - 1;
	if (param > CA77XX_LED_MAX_HW_BLINK)
		param = CA77XX_LED_MAX_HW_BLINK;

	spin_lock_irqsave(led_ctrl->lock, flags);
	led_ctrl->blink_rate2 = param;
	val = ca77xx_led_read(led_ctrl->mem, NULL);
	val &= ~(CA77XX_LED_BLINK_RATE2_MASK << CA77XX_LED_BLINK_RATE2_OFFSET);
	val |= led_ctrl->blink_rate2 << CA77XX_LED_BLINK_RATE2_OFFSET;
	ca77xx_led_write(led_ctrl->mem, val, NULL);
	spin_unlock_irqrestore(led_ctrl->lock, flags);

	return count;
}
static DEVICE_ATTR(blink_rate2, 0664, ca77xx_leds_blink_rate2_show,
			ca77xx_leds_blink_rate2_store);

static struct attribute *ca77xx_leds_attributes[] = {
	&dev_attr_test_mode.attr,
	&dev_attr_clk_low.attr,
	&dev_attr_blink_rate1.attr,
	&dev_attr_blink_rate2.attr,
	NULL
};

static const struct attribute_group ca77xx_leds_attr_group = {
	.attrs = ca77xx_leds_attributes,
};

static int ca77xx_leds_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	struct resource *mem_r;
	void __iomem *mem;
	spinlock_t *lock;	/* protect LED resource access */
	u32 val, reg;
	unsigned long flags;

	glb_led_ctrl = devm_kzalloc(dev, sizeof(struct ca77xx_led_ctrl),
				    GFP_KERNEL);
	if (!glb_led_ctrl)
		return -ENOMEM;
	glb_led_ctrl->dev = dev;

	mem_r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_r)
		return -EINVAL;

	mem = devm_ioremap_resource(dev, mem_r);
	if (IS_ERR(mem))
		return PTR_ERR(mem);
	dev_info(dev, "resource - %pr mapped at 0x%pK\n", mem_r, mem);
	glb_led_ctrl->mem = mem;

	lock = devm_kzalloc(dev, sizeof(*lock), GFP_KERNEL);
	if (!lock)
		return -ENOMEM;

	spin_lock_init(lock);
	glb_led_ctrl->lock = lock;

	reg = 0;
	if (!of_property_read_u32(np, "clk-low", &val)) {
		glb_led_ctrl->clk_low = 1;
		reg |= CA77XX_LED_CLK_POLARITY;
	} else {
		reg &= ~CA77XX_LED_CLK_POLARITY;
	}

	if (!of_property_read_u32(np, "hw-blink-rate1", &val)) {
		val = val / 16 - 1;
		glb_led_ctrl->blink_rate1 = val > CA77XX_LED_MAX_HW_BLINK ?
					    CA77XX_LED_MAX_HW_BLINK : val;
	}
	reg |= (glb_led_ctrl->blink_rate1 & CA77XX_LED_BLINK_RATE1_MASK) <<
	       CA77XX_LED_BLINK_RATE1_OFFSET;

	if (!of_property_read_u32(np, "hw-blink-rate2", &val)) {
		val = val / 16 - 1;
		glb_led_ctrl->blink_rate2 = val > CA77XX_LED_MAX_HW_BLINK ?
					    CA77XX_LED_MAX_HW_BLINK : val;
	}
	reg |= (glb_led_ctrl->blink_rate2 & CA77XX_LED_BLINK_RATE2_MASK) <<
	       CA77XX_LED_BLINK_RATE2_OFFSET;

	spin_lock_irqsave(lock, flags);
	ca77xx_led_write(glb_led_ctrl->mem, reg, NULL);
	spin_unlock_irqrestore(lock, flags);

	mem += 4;
	for_each_available_child_of_node(np, child) {
		int rc;
		u32 reg;

		if (of_property_read_u32(child, "reg", &reg))
			continue;

		if (reg >= CA77XX_LED_MAX_COUNT) {
			dev_err(dev, "invalid LED (%u >= %d)\n", reg,
				CA77XX_LED_MAX_COUNT);
			continue;
		}

		rc = ca77xx_led_probe(glb_led_ctrl, child, reg, mem + reg * 4,
				      lock);
		if (rc < 0) {
			of_node_put(child);
			return rc;
		}
	}

	dev_set_drvdata(dev, glb_led_ctrl);

	return sysfs_create_group(&dev->kobj, &ca77xx_leds_attr_group);
}

static int ca77xx_leds_revome(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &ca77xx_leds_attr_group);

	return 0;
}

static const struct of_device_id ca77xx_leds_of_match[] = {
	{ .compatible = "cortina,ca77xx-leds", },
	{ },
};
MODULE_DEVICE_TABLE(of, ca77xx_leds_of_match);

static struct platform_driver ca77xx_leds_driver = {
	.probe = ca77xx_leds_probe,
	.remove = ca77xx_leds_revome,
	.driver = {
		.name = "leds-ca77xx",
		.of_match_table = ca77xx_leds_of_match,
	},
};

module_platform_driver(ca77xx_leds_driver);

MODULE_DESCRIPTION("LED driver for CA77XX controllers");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:leds-ca77xx");
