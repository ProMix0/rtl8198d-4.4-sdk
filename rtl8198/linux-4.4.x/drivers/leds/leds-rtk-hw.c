#include <linux/module.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/property.h>
#include <linux/of.h>

#include <rtk/led.h>

#if 0
#define LED_DEBUG
#endif

#ifdef LED_DEBUG
#define DEBUG_PRINT(fmt, args...) printk(fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

enum rtk_hw_led_type {
    HW_LED = 0,
    SW_LED,
};

struct rtk_hw_led_hw_config {
    rtk_enable_t enable;
    rtk_led_operation_t mode;
    rtk_led_type_t type;
    unsigned int cfg_bitmap;
};

struct rtk_hw_led {
    const char *name;
    const char *default_trigger;
    unsigned int id;
    unsigned int type;
    struct rtk_hw_led_hw_config hwcfg;
};

/*
struct rtk_hw_led myleds[] = {
    {
        .name = "swled1",
        .default_trigger = "none",
        .id = 3,
        .type = HW_LED,
        .hwcfg = {
            .enable = ENABLED,
            .mode = LED_OP_PARALLEL,
            .type = LED_TYPE_UTP1,
            .cfg_bitmap = 0xf78,
        },
    },
};
*/

struct rtk_hw_led_platform_data {
    int num_leds;
    const struct rtk_hw_led *leds;
};

struct rtk_hw_led_data {
    struct led_classdev cdev;
    unsigned int id;
    unsigned int type;
    struct rtk_hw_led_hw_config hwcfg;
};

struct rtk_hw_led_priv {
    int num_leds;
    struct rtk_hw_led_data leds[];
};

static char* isConfigSelected(unsigned int bitmap, int id)
{
    return (bitmap & (1 << id))?"*":" ";
}

static void rtk_hw_led_set_type_sw(unsigned int id, struct rtk_hw_led_hw_config hwcfg, rtk_led_force_mode_t force);

static void rtk_hw_led_set_brightness(struct led_classdev *led_cdev,
                                      enum led_brightness value)
{
    struct rtk_hw_led_data *led_dat = container_of(led_cdev, struct rtk_hw_led_data, cdev);

    if (led_dat->type == HW_LED)
        return;

    switch (value) {
    case LED_OFF:
        DEBUG_PRINT("set %s  off\n", led_cdev->name);
        if (led_dat->type != HW_LED)
            rtk_hw_led_set_type_sw(led_dat->id, led_dat->hwcfg, LED_FORCE_OFF);
        break;
    case LED_HALF:
        DEBUG_PRINT("set %s  on, half brightness\n", led_cdev->name);
        break;
    case LED_FULL:
        DEBUG_PRINT("set %s  on, full brightness\n", led_cdev->name);
        if (led_dat->type != HW_LED)
            rtk_hw_led_set_type_sw(led_dat->id, led_dat->hwcfg, LED_FORCE_ON);
        break;
    default:
        DEBUG_PRINT("set %s  on, brightness (%d)\n", led_cdev->name, value);
        if (led_dat->type != HW_LED)
            rtk_hw_led_set_type_sw(led_dat->id, led_dat->hwcfg, LED_FORCE_ON);
        break;
    }

    return;
}

static void rtk_hw_led_set_type_hw(unsigned int id, const struct rtk_hw_led_hw_config hwcfg)
{
    rtk_led_config_t cfg = {.ledEnable = {0}};
    int i;

    DEBUG_PRINT("%s\nled hw id %d\n", __func__, id);
    DEBUG_PRINT("enable: %d | mode: %d | type: %d | cfg_bitmap: 0x%x\n",
                hwcfg.enable, hwcfg.mode, hwcfg.type, hwcfg.cfg_bitmap);

    for (i = 0; i < LED_CONFIG_END; i++) {
        if (hwcfg.cfg_bitmap & (1 << i))
            cfg.ledEnable[i] = ENABLED;
    }
    /*
    for (i = 0; i < LED_CONFIG_END; i++)
    	printk("ledEnable[%d] = %d\n", i, cfg.ledEnable[i]);
    */
    rtk_led_parallelEnable_set(id, hwcfg.enable);
    rtk_led_config_set(id, hwcfg.type, &cfg);
}


static void rtk_hw_led_set_type_sw(unsigned int id, struct rtk_hw_led_hw_config hwcfg, rtk_led_force_mode_t force)
{
    DEBUG_PRINT("%s: led id %d\n", __func__, id);
    rtk_led_config_t cfg = {.ledEnable = {0}};
    int i;

    cfg.ledEnable[LED_CONFIG_FORCE_MODE] = ENABLED;

    rtk_led_parallelEnable_set(id, hwcfg.enable);
    rtk_led_config_set(id, hwcfg.type, &cfg);
    rtk_led_modeForce_set(id, force);
}

static ssize_t rtk_hw_led_port_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_hw_led_data *led_dat = container_of(led_cdev, struct rtk_hw_led_data, cdev);

    sprintf(buf, "led port: %d\n0: UTP0\n1: UTP1\n2: UTP2\n3: UTP3\n4: UTP4\n6: FIBER\n",
            led_dat->hwcfg.type);

    return strlen(buf) + 1;
}

static ssize_t rtk_hw_led_port_store(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf,
                                     size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_hw_led_data *led_dat = container_of(led_cdev, struct rtk_hw_led_data, cdev);
    unsigned long value;
    int ret = -EINVAL;

    ret = kstrtoul(buf, 0, &value);
    if (ret)
        return ret;

    led_dat->hwcfg.type = (unsigned int)value;
    DEBUG_PRINT("set led port to: %d\n", led_dat->hwcfg.type);
    DEBUG_PRINT("0: UTP0\n1: UTP1\n2: UTP2\n3: UTP3\n4: UTP4\n6: FIBER\n");

    if (led_dat->type == HW_LED)
        rtk_hw_led_set_type_hw(led_dat->id, led_dat->hwcfg);

    return size;
}

static DEVICE_ATTR(port, 0644, rtk_hw_led_port_show, rtk_hw_led_port_store);

static ssize_t rtk_hw_led_config_show(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_hw_led_data *led_dat = container_of(led_cdev, struct rtk_hw_led_data, cdev);
    int len;

    len = sprintf(buf, "led config bitmap: 0x%x\n", led_dat->hwcfg.cfg_bitmap);
    len += sprintf(buf + len, "[%s]  0: COL\n[%s]  1: TX ACT\n[%s]  2: RX ACT\n",
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_COL),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_TX_ACT),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_RX_ACT));
    len += sprintf(buf + len, "[%s]  3: SPD10_ACT\n[%s]  4: SPD100_ACT\n[%s]  5: SPD500_ACT\n",
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD10ACT),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD100ACT),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD500ACT));
    len += sprintf(buf + len, "[%s]  6: SPD1000_ACT\n[%s]  7: DUP\n",
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD1000ACT),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_DUP));
    len += sprintf(buf + len, "[%s]  8: SPD10_LNK\n[%s]  9: SPD100_LNK\n",
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD10),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD100));
    len += sprintf(buf + len, "[%s] 10: SPD500_LNK\n[%s] 11: SPD1000_LNK\n",
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD500),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD1000));
    len += sprintf(buf + len, "[%s] 12: FORCE_MODE\n[%s] 13: PON_LINK\n[%s] 14: SOC_LINK_ACK\n",
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_FORCE_MODE),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_PON_LINK),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SOC_LINK_ACK));
    len += sprintf(buf + len, "[%s] 15: PON_ALARM\n[%s] 16: PON_WARNING\n",
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_PON_ALARM),
                   isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_PON_WARNING));

    return strlen(buf) + 1;
}

static ssize_t rtk_hw_led_config_store(struct device *dev,
                                       struct device_attribute *attr,
                                       const char *buf,
                                       size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_hw_led_data *led_dat = container_of(led_cdev, struct rtk_hw_led_data, cdev);
    unsigned long value;
    int ret = -EINVAL;

    ret = kstrtoul(buf, 0, &value);
    if (ret)
        return ret;

    led_dat->hwcfg.cfg_bitmap = (unsigned int)value;

    DEBUG_PRINT("set led config bitmap to 0x%x\n", led_dat->hwcfg.cfg_bitmap);
    DEBUG_PRINT("[%s] 0: COL\n[%s]  1: TX ACT\n[%s]  2: RX ACT\n",
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_COL),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_TX_ACT),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_RX_ACT));

    DEBUG_PRINT("[%s]  3: SPD10_ACT\n[%s]  4: SPD100_ACT\n[%s]  5: SPD500_ACT\n[%s]  6: SPD1000_ACT\n",
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD10ACT),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD100ACT),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD500ACT),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD1000ACT));

    DEBUG_PRINT("[%s]  7: DUP\n",
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_DUP));

    DEBUG_PRINT("[%s]  8: SPD10_LNK\n[%s]  9: SPD100_LNK\n[%s] 10: SPD500_LNK\n[%s] 11: SPD1000_LNK\n",
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD10),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD100),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD500),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SPD1000));

    DEBUG_PRINT("[%s] 12: FORCE_MODE\n[%s] 13: PON_LINK\n[%s] 14: SOC_LINK_ACK\n",
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_FORCE_MODE),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_PON_LINK),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_SOC_LINK_ACK));

    DEBUG_PRINT("[%s] 15: PON_ALARM\n[%s] 16: PON_WARNING\n",
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_PON_ALARM),
                isConfigSelected(led_dat->hwcfg.cfg_bitmap, LED_CONFIG_PON_WARNING));

    if (led_dat->type == HW_LED)
        rtk_hw_led_set_type_hw(led_dat->id, led_dat->hwcfg);

    return size;
}

static DEVICE_ATTR(config, 0644, rtk_hw_led_config_show, rtk_hw_led_config_store);

static ssize_t rtk_hw_led_type_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_hw_led_data *led_dat = container_of(led_cdev, struct rtk_hw_led_data, cdev);

    sprintf(buf, "type: %s\nled id: %d\n",
            led_dat->type?"SW":"HW",
            led_dat->id);

    return strlen(buf) + 1;
}

static ssize_t rtk_hw_led_type_store(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf,
                                     size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_hw_led_data *led_dat = container_of(led_cdev, struct rtk_hw_led_data, cdev);
    unsigned long value;
    static unsigned int last_type;
    int ret = -EINVAL;

    ret = kstrtoul(buf, 0, &value);
    if (ret)
        return ret;

    last_type = led_dat->type;
    led_dat->type = (unsigned int)value;

    if (led_dat->type == HW_LED) {
        rtk_hw_led_set_type_hw(led_dat->id, led_dat->hwcfg);
        if (last_type != HW_LED)  {
            device_create_file(led_cdev->dev, &dev_attr_port);
            device_create_file(led_cdev->dev, &dev_attr_config);
        }
    } else {
        rtk_hw_led_set_type_sw(led_dat->id, led_dat->hwcfg, LED_FORCE_OFF);
        if (last_type == HW_LED) {
            device_remove_file(led_cdev->dev, &dev_attr_port);
            device_remove_file(led_cdev->dev, &dev_attr_config);
        }
    }

    return size;
}

static DEVICE_ATTR(type, 0644, rtk_hw_led_type_show, rtk_hw_led_type_store);

static int create_rtk_hw_led_classdev(const struct rtk_hw_led *template,
                                      struct rtk_hw_led_data *led_dat, struct device *parent)
{
    int ret;

    led_dat->cdev.name = template->name;
    led_dat->cdev.brightness_set = rtk_hw_led_set_brightness;
    led_dat->id = template->id;
    led_dat->type = template->type;
    led_dat->hwcfg = template->hwcfg;

    if (led_dat->type == HW_LED) {
        led_dat->cdev.default_trigger = "none";
        rtk_hw_led_set_type_hw(led_dat->id, led_dat->hwcfg);
    } else {
        led_dat->cdev.default_trigger = template->default_trigger;
        rtk_hw_led_set_type_sw(led_dat->id, led_dat->hwcfg, LED_FORCE_OFF);
    }

    ret = led_classdev_register(parent, &led_dat->cdev);
    if (ret < 0 )
        return ret;

    DEBUG_PRINT("\n\n%s: create attr\n\n", __func__);
    ret = device_create_file(led_dat->cdev.dev, &dev_attr_type);
    if (ret) {
        pr_err("%s: create device attribute failed\n", __func__);
        return -1;
    }

    if (led_dat->type == HW_LED) {
        ret = device_create_file(led_dat->cdev.dev, &dev_attr_port);
        if (ret) {
            pr_err("%s: create device attribute failed\n", __func__);
            return -1;
        }
        ret = device_create_file(led_dat->cdev.dev, &dev_attr_config);
        if (ret) {
            pr_err("%s: create device attribute failed\n", __func__);
            return -1;
        }
    }

    return 0;
}

static void delete_rtk_hw_led_classdev(struct rtk_hw_led_data *led_dat)
{
    led_classdev_unregister(&led_dat->cdev);
}

static inline int sizeof_rtk_hw_leds_priv(int num)
{
    return (sizeof(struct rtk_hw_led_priv) + sizeof(struct rtk_hw_led_data) * num);
}

static struct rtk_hw_led_priv *rtk_hw_led_create(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct fwnode_handle *child;
    struct rtk_hw_led_priv *priv;
    int count, ret;
    struct device_node *np;

    count = device_get_child_node_count(dev);
    if (!count)
        return ERR_PTR(-ENODEV);

    priv = devm_kzalloc(dev, sizeof_rtk_hw_leds_priv(count), GFP_KERNEL);
    if(!priv)
        return ERR_PTR(-ENOMEM);

    device_for_each_child_node(dev, child) {
        struct rtk_hw_led led = {};

        np = to_of_node(child);

        if (fwnode_property_present(child, "label")) {
            fwnode_property_read_string(child, "label", &led.name);
        } else {
            if (IS_ENABLED(CONFIG_OF) && !led.name && np)
                led.name = np->name;
            if (!led.name) {
                ret = -EINVAL;
                goto err;
            }
        }

        fwnode_property_read_string(child, "linux,default-trigger", &led.default_trigger);

        fwnode_property_read_u32(child, "id", &led.id);

        fwnode_property_read_u32(child, "default-type", &led.type);

        fwnode_property_read_u32(child, "enable", &led.hwcfg.enable);

        fwnode_property_read_u32(child, "mode", &led.hwcfg.mode);

        fwnode_property_read_u32(child, "led-type", &led.hwcfg.type);

        fwnode_property_read_u32(child, "config", &led.hwcfg.cfg_bitmap);

        ret = create_rtk_hw_led_classdev(&led, &priv->leds[priv->num_leds], dev);
        if (ret < 0) {
            fwnode_handle_put(child);
            goto err;
        }
        priv->num_leds++;
    }

    return priv;

err:
    for (count = priv->num_leds - 1; count >= 0; count--)
        delete_rtk_hw_led_classdev(&priv->leds[count]);
    return ERR_PTR(ret);
}

static const struct of_device_id of_rtk_hw_leds_match[] = {
    { .compatible = "rtk-hw-leds", },
    {},
};

MODULE_DEVICE_TABLE(of, of_rtk_hw_leds_match);

static int rtk_hw_led_probe(struct platform_device *pdev)
{
    struct rtk_hw_led_platform_data *pdata = dev_get_platdata(&pdev->dev);
    struct rtk_hw_led_priv *priv;
    int i, ret = 0;

    if (pdata && pdata->num_leds) {
        priv = devm_kzalloc(&pdev->dev,
                            sizeof_rtk_hw_leds_priv(pdata->num_leds),
                            GFP_KERNEL);
        if (!priv)
            return -ENOMEM;

        priv->num_leds = pdata->num_leds;
        for (i = 0; i < priv->num_leds; i++) {
            ret = create_rtk_hw_led_classdev(&pdata->leds[i],
                                             &priv->leds[i],
                                             &pdev->dev);
            if (ret < 0) {
                /* On failure: unwind the led creations */
                for (i = i - 1; i >= 0; i--)
                    delete_rtk_hw_led_classdev(&priv->leds[i]);
                return ret;
            }
        }
    } else {
        priv = rtk_hw_led_create(pdev);
        if (IS_ERR(priv))
            return PTR_ERR(priv);
    }

    platform_set_drvdata(pdev, priv);

    return 0;
}

static int rtk_hw_led_remove(struct platform_device *pdev)
{
    struct rtk_hw_led_priv *priv = platform_get_drvdata(pdev);
    int i;

    for (i = 0; i < priv->num_leds; i++)
        delete_rtk_hw_led_classdev(&priv->leds[i]);

    return 0;
}

//static struct platform_device *led_rtk_hw_device;

static struct platform_driver led_rtk_hw_driver = {
    .probe		= rtk_hw_led_probe,
    .remove		= rtk_hw_led_remove,
    .driver		= {
        .name	= "leds-rtk-hw",
        .owner	= THIS_MODULE,
        .of_match_table = of_rtk_hw_leds_match,
    },
};

static int __init led_rtk_hw_init(void)
{
    //struct platform_device *pdev;
    //struct rtk_hw_led_platform_data pdata;
    int err;
#if 0
    pdev = platform_device_alloc("leds-rtk-hw", -1);
    if (!pdev)
        goto err_0;

    led_rtk_hw_device = pdev;

    pdata.num_leds = sizeof(myleds)/sizeof(struct rtk_hw_led);
    pdata.leds = myleds;

    err = platform_device_add_data(pdev, &pdata, sizeof(pdata));
    if (err)
        goto err_1;

    err = platform_device_add(pdev);
    if (err)
        goto err_1;
#endif

    err = platform_driver_register(&led_rtk_hw_driver);
    if (err) {
        pr_err("%s: register platform driver failed\n", __func__);
        goto err_1;
    }

    return 0;

err_1:
    //platform_device_put(pdev);
//err_0:
//   pr_err("%s: error occurs during create platform device\n", __func__);
    return err;
}

static void __exit led_rtk_hw_exit(void)
{
//    platform_device_put(led_rtk_hw_device);
    platform_driver_unregister(&led_rtk_hw_driver);
}

module_init(led_rtk_hw_init);
module_exit(led_rtk_hw_exit);

