// SPDX-License-Identifier: GPL-2.0
// Copyright 2017 Ben Whitten <ben.whitten@gmail.com>
// Copyright 2007 Oliver Jowett <oliver@opencloud.com>
//
// LED Kernel Netdev Trigger
//
// Toggles the LED to reflect the link and traffic state of a named net device
//
// Derived from ledtrig-timer.c which is:
//  Copyright 2005-2006 Openedhand Ltd.
//  Author: Richard Purdie <rpurdie@openedhand.com>

#include <linux/atomic.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/netfilter/x_tables.h>
#include "../leds.h"

/*
 * Configurable sysfs attributes:
 *
 * device_name - network device name to monitor
 * interval - duration of LED blink, in milliseconds
 * link -  LED's normal state reflects whether the link is up
 *         (has carrier) or not
 * tx -  LED blinks on transmitted data
 * rx -  LED blinks on receive data
 *
 */

extern unsigned int get_wireless_LED_rx_cnt(struct net_device *dev);
extern unsigned int get_wireless_LED_tx_cnt(struct net_device *dev);
extern unsigned int get_wireless_LED_interval(struct net_device *dev);

#define MAX_DEV_NUM 16
struct led_netdev_data {
	spinlock_t lock;

	struct delayed_work work;
	struct notifier_block notifier;

	struct led_classdev *led_cdev;
	struct net_device *devs[MAX_DEV_NUM];

	char device_name[IFNAMSIZ];
	char device_name_mask[IFNAMSIZ];

	int dev_num;
	int valid;
	atomic_t interval;
	unsigned int last_activity;

	unsigned long mode;

#define NETDEV_LED_LINK	0
#define NETDEV_LED_TX	1
#define NETDEV_LED_RX	2
#define NETDEV_LED_MODE_LINKUP	3
};

enum netdev_led_attr {
	NETDEV_ATTR_LINK,
	NETDEV_ATTR_TX,
	NETDEV_ATTR_RX
};

static void set_baseline_state(struct led_netdev_data *trigger_data)
{
	int current_brightness;
	struct led_classdev *led_cdev = trigger_data->led_cdev;

	current_brightness = led_cdev->brightness;

	if (current_brightness)
		led_cdev->blink_brightness = current_brightness;
	if (!led_cdev->blink_brightness)
		led_cdev->blink_brightness = led_cdev->max_brightness;

	if (!test_bit(NETDEV_LED_MODE_LINKUP, &trigger_data->mode))
		led_set_brightness(led_cdev, LED_OFF);
	else {
		if (test_bit(NETDEV_LED_LINK, &trigger_data->mode)) {
			led_set_brightness(led_cdev,
					   led_cdev->blink_brightness);
		}
		else
			led_set_brightness(led_cdev, LED_OFF);

		/* If we are looking for RX/TX start periodically
		 * checking stats
		 */
		if (test_bit(NETDEV_LED_TX, &trigger_data->mode) ||
		    test_bit(NETDEV_LED_RX, &trigger_data->mode))
			schedule_delayed_work(&trigger_data->work, 0);
	}
}

static void __clear_devs(struct led_netdev_data *trig) {
	int i;

	for (i=0; i<ARRAY_SIZE(trig->devs); i++) {
		if (trig->devs[i])
			dev_put(trig->devs[i]);
		trig->devs[i] = NULL;
	}
	trig->dev_num = 0;
	clear_bit(NETDEV_LED_MODE_LINKUP, &trig->mode);
}

static int __add_dev(struct led_netdev_data *trig, struct net_device *dev) {
	if ((trig->dev_num+1) >= MAX_DEV_NUM)
		return -1;

	dev_hold(dev);
	trig->devs[trig->dev_num] = dev;
	trig->dev_num++;
	set_bit(NETDEV_LED_MODE_LINKUP, &trig->mode);
	return 0;
}

static int __del_dev(struct led_netdev_data *trig, struct net_device *dev) {
	int i;

	if (trig->dev_num == 0) /* nothing to do */
		return 0;

	for (i=0; i<trig->dev_num; i++) {
		if (trig->devs[i] == dev)
			break;
	}

	if (i >= trig->dev_num) /* not found */
		return 0;

	if (i == (trig->dev_num - 1)) {
		trig->devs[i] = NULL;
	} else { /* swap */
		trig->devs[i] = trig->devs[trig->dev_num - 1];
		trig->devs[trig->dev_num - 1] = NULL;
	}
	trig->dev_num--;
	if (0 == trig->dev_num)
		clear_bit(NETDEV_LED_MODE_LINKUP, &trig->mode);

	dev_put(dev);
	return 0;
}

static ssize_t device_name_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct led_netdev_data *trigger_data = led_cdev->trigger_data;
	int len;

	if (size >= IFNAMSIZ)
		return -EINVAL;

	trigger_data->valid = 0;

	cancel_delayed_work_sync(&trigger_data->work);

	spin_lock_bh(&trigger_data->lock);

	__clear_devs(trigger_data);

	memset(trigger_data->device_name, 0, sizeof(trigger_data->device_name));
	memset(trigger_data->device_name_mask, 0, sizeof(trigger_data->device_name_mask));

	strncpy(trigger_data->device_name, buf, size);
	if (size > 0 && trigger_data->device_name[size - 1] == '\n')
		trigger_data->device_name[size - 1] = 0;

	len = strlen(trigger_data->device_name);

	if (len != 0) {
		struct net_device *d;

		trigger_data->valid = 1;

		if (trigger_data->device_name[len - 1] == '+') {
			memset(trigger_data->device_name_mask, 0xff, len - 1);
		} else {
			memset(trigger_data->device_name_mask, 0xff, len + 1);
		}
		rcu_read_lock();
		for_each_netdev_rcu(&init_net, d)  {
			if (ifname_compare_aligned(d->name, trigger_data->device_name, trigger_data->device_name_mask))
				continue;

			if (d->flags & IFF_UP)
				__add_dev(trigger_data, d);
		}
		rcu_read_unlock();
	}

	trigger_data->last_activity = 0;

	set_baseline_state(trigger_data);
	spin_unlock_bh(&trigger_data->lock);

	return size;
}

static ssize_t netdev_led_attr_show(struct device *dev, char *buf,
	enum netdev_led_attr attr)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct led_netdev_data *trigger_data = led_cdev->trigger_data;
	int bit;

	switch (attr) {
	case NETDEV_ATTR_LINK:
		bit = NETDEV_LED_LINK;
		break;
	case NETDEV_ATTR_TX:
		bit = NETDEV_LED_TX;
		break;
	case NETDEV_ATTR_RX:
		bit = NETDEV_LED_RX;
		break;
	default:
		return -EINVAL;
	}

	return sprintf(buf, "%u\n", test_bit(bit, &trigger_data->mode));
}

static ssize_t netdev_led_attr_store(struct device *dev, const char *buf,
	size_t size, enum netdev_led_attr attr)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct led_netdev_data *trigger_data = led_cdev->trigger_data;
	unsigned long state;
	int ret;
	int bit;

	ret = kstrtoul(buf, 0, &state);
	if (ret)
		return ret;

	switch (attr) {
	case NETDEV_ATTR_LINK:
		bit = NETDEV_LED_LINK;
		break;
	case NETDEV_ATTR_TX:
		bit = NETDEV_LED_TX;
		break;
	case NETDEV_ATTR_RX:
		bit = NETDEV_LED_RX;
		break;
	default:
		return -EINVAL;
	}

	cancel_delayed_work_sync(&trigger_data->work);

	if (state)
		set_bit(bit, &trigger_data->mode);
	else
		clear_bit(bit, &trigger_data->mode);

	set_baseline_state(trigger_data);

	return size;
}

static ssize_t tx_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return netdev_led_attr_show(dev, buf, NETDEV_ATTR_TX);
}

static ssize_t tx_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return netdev_led_attr_store(dev, buf, size, NETDEV_ATTR_TX);
}

static DEVICE_ATTR_RW(tx);

static ssize_t rx_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return netdev_led_attr_show(dev, buf, NETDEV_ATTR_RX);
}

static ssize_t rx_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return netdev_led_attr_store(dev, buf, size, NETDEV_ATTR_RX);
}

static DEVICE_ATTR_RW(rx);

static ssize_t interval_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct led_netdev_data *trigger_data = led_cdev->trigger_data;

	return sprintf(buf, "%u\n",
		       jiffies_to_msecs(atomic_read(&trigger_data->interval)));
}

static ssize_t interval_store(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct led_netdev_data *trigger_data = led_cdev->trigger_data;
	unsigned long value;
	int ret;

	ret = kstrtoul(buf, 0, &value);
	if (ret)
		return ret;

	/* impose some basic bounds on the timer interval */
	if (value >= 5 && value <= 10000) {
		cancel_delayed_work_sync(&trigger_data->work);

		atomic_set(&trigger_data->interval, msecs_to_jiffies(value));
		set_baseline_state(trigger_data);	/* resets timer */
	}

	return size;
}

static DEVICE_ATTR_RW(interval);

static int netdev_trig_notify(struct notifier_block *nb,
			      unsigned long evt, void *dv)
{
	struct net_device *dev =
		netdev_notifier_info_to_dev((struct netdev_notifier_info *)dv);
	struct led_netdev_data *trigger_data = container_of(nb,
							    struct
							    led_netdev_data,
							    notifier);

	if (evt != NETDEV_UP && evt != NETDEV_DOWN)
		return NOTIFY_DONE;

	if (!trigger_data->valid || ifname_compare_aligned(dev->name, trigger_data->device_name, trigger_data->device_name_mask))
		return NOTIFY_DONE;

	cancel_delayed_work_sync(&trigger_data->work);

	spin_lock_bh(&trigger_data->lock);

	switch (evt) {
	case NETDEV_UP:
		__add_dev(trigger_data, dev);
		break;

	case NETDEV_DOWN:
		__del_dev(trigger_data, dev);
		break;
	}

	set_baseline_state(trigger_data);

	spin_unlock_bh(&trigger_data->lock);

	return NOTIFY_DONE;
}

/* here's the real work! */
static void netdev_trig_work(struct work_struct *work)
{
	struct led_netdev_data *trigger_data = container_of(work,
							    struct
							    led_netdev_data,
							    work.work);
	unsigned int new_activity = 0;
	unsigned long interval;
	int i, invert;

	if (!test_bit(NETDEV_LED_MODE_LINKUP, &trigger_data->mode)) {
		led_set_brightness(trigger_data->led_cdev, LED_OFF);
		return;
	}

	/* If we are not looking for RX/TX then return  */
	if (!test_bit(NETDEV_LED_TX, &trigger_data->mode) &&
	    !test_bit(NETDEV_LED_RX, &trigger_data->mode))
		return;

	for (i=0; i<trigger_data->dev_num; i++) {
		struct net_device *dev;

		dev = trigger_data->devs[i];
		new_activity += test_bit(NETDEV_LED_RX, &trigger_data->mode) ? get_wireless_LED_rx_cnt(dev) : 0;
		new_activity += test_bit(NETDEV_LED_TX, &trigger_data->mode) ? get_wireless_LED_tx_cnt(dev) : 0;
	}

	if (trigger_data->last_activity != new_activity) {
		led_stop_software_blink(trigger_data->led_cdev);

		interval = jiffies_to_msecs(
				atomic_read(&trigger_data->interval));
		/* base state is ON (link present) */
		led_blink_set_oneshot(trigger_data->led_cdev,
				      &interval,
				      &interval,
				      1);
		trigger_data->last_activity = new_activity;
	}

	schedule_delayed_work(&trigger_data->work,
			(atomic_read(&trigger_data->interval)*2));
}

static void netdev_trig_activate(struct led_classdev *led_cdev)
{
	struct led_netdev_data *trigger_data;
	int rc;

	trigger_data = kzalloc(sizeof(struct led_netdev_data), GFP_KERNEL);
	if (!trigger_data)
		return;

	spin_lock_init(&trigger_data->lock);

	trigger_data->notifier.notifier_call = netdev_trig_notify;
	trigger_data->notifier.priority = 10;

	INIT_DELAYED_WORK(&trigger_data->work, netdev_trig_work);

	trigger_data->led_cdev = led_cdev;

	trigger_data->mode = BIT_MASK(NETDEV_LED_LINK)|BIT_MASK(NETDEV_ATTR_RX)|BIT_MASK(NETDEV_ATTR_TX);
	atomic_set(&trigger_data->interval, msecs_to_jiffies(50));
	trigger_data->last_activity = 0;

	led_cdev->trigger_data = trigger_data;

	rc = device_create_file(led_cdev->dev, &dev_attr_rx);
	if (rc)
		goto err_out_link;
	rc = device_create_file(led_cdev->dev, &dev_attr_tx);
	if (rc)
		goto err_out_rx;
	rc = device_create_file(led_cdev->dev, &dev_attr_interval);
	if (rc)
		goto err_out_tx;
	rc = register_netdevice_notifier(&trigger_data->notifier);
	if (rc)
		goto err_out_interval;

	device_name_store(led_cdev->dev, NULL, "wlan+", 5);

	return;

err_out_interval:
	device_remove_file(led_cdev->dev, &dev_attr_interval);
err_out_tx:
	device_remove_file(led_cdev->dev, &dev_attr_tx);
err_out_rx:
	device_remove_file(led_cdev->dev, &dev_attr_rx);
err_out_link:

	led_cdev->trigger_data = NULL;
	kfree(trigger_data);
}

static void netdev_trig_deactivate(struct led_classdev *led_cdev)
{
	struct led_netdev_data *trigger_data = led_cdev->trigger_data;

	if (trigger_data) {
		unregister_netdevice_notifier(&trigger_data->notifier);

		device_remove_file(led_cdev->dev, &dev_attr_rx);
		device_remove_file(led_cdev->dev, &dev_attr_tx);
		device_remove_file(led_cdev->dev, &dev_attr_interval);

		cancel_delayed_work_sync(&trigger_data->work);

		__clear_devs(trigger_data);

		kfree(trigger_data);
	}
}

static struct led_trigger wlan_led_trigger = {
	.name = "wlanled",
	.activate = netdev_trig_activate,
	.deactivate = netdev_trig_deactivate,
};

static int __init wlanled_trig_init(void)
{
	return led_trigger_register(&wlan_led_trigger);
}

static void __exit netdev_trig_exit(void)
{
	led_trigger_unregister(&wlan_led_trigger);
}

module_init(wlanled_trig_init);
module_exit(netdev_trig_exit);

MODULE_AUTHOR("Ben Whitten <ben.whitten@gmail.com>");
MODULE_AUTHOR("Oliver Jowett <oliver@opencloud.com>");
MODULE_AUTHOR("sd5@realtek.com>");
MODULE_DESCRIPTION("Realtek 8192cd LED trigger");
MODULE_LICENSE("GPL v2");
