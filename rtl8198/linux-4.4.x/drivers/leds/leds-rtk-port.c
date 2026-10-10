#include <linux/io.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

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
#define CA77XX_LED_BLINK_OR		BIT(17)

/**
 * struct rtk_port_led_cfg - configuration for LEDs
 * @cdev: LED class device for this LED
 * @mem: memory resource
 * @lock: memory lock
 * @idx: LED index number
 * @mode: LED mode
 * @active_low: LED is active low
 * @enable: LED is enabled/disabled for hardware blink
 * @off_event: off triggered by rx/tx/sw event
 * @blink_event: blinking triggered by rx/tx/sw event
 * @on_event: on triggered by rx/tx/sw event
 * @port: monitor port
 * @blink: hardware blink rate select
 */
struct rtk_port_led_cfg {
    struct led_classdev cdev;
    void __iomem *mem;
    spinlock_t *lock;	/* protect LED resource access */
    unsigned long data;
    int idx;
    int mode;
    bool active_low;

    /* used by hardware blink AND/OR mode */
    bool enable;
    int off_event;
    int blink_event;
    int on_event;
    int port;
    int blink;

    /* controller device */
    struct device *dev;
};

/**
 * struct rtk_port_led_cfg - control for LEDs
 * @mem: memory resource
 * @lock: memory lock
 * @test_mode: enter test mode
 * @clk_low: clock polarity
 * @blink_rate1: haredware blink rate 1
 * @blink_rate2: haredware blink rate 2
 * @rtk_port_led: configuration for LEDs
 */
struct rtk_port_led_ctrl {
    void __iomem *mem;
    void __iomem *spare;
    spinlock_t *lock;	/* protect LED resource access */

    int test_mode;
    int clk_low;
    u16 blink_rate1;
    u16 blink_rate2;
    int port_cnt;
    int *port2led;

    struct device *dev;

    struct rtk_port_led_cfg *led_cfg[CA77XX_LED_MAX_COUNT];
};

enum mode {
    HW_BLINK_AND = 0,
    HW_BLINK_OR,
    SW_MODE,
};

enum blink_rate {
    BLINK_RATE1 = 0,
    BLINK_RATE2,
};

enum event_type {
    EVENT_HW_RX = 0,
    EVENT_HW_TX,
    EVENT_SW,
    EVENT_NONE,
    EVENT_CNT,
};

#define EVENT_STR_LEN 4
static char *event_str[] = {
    [EVENT_HW_RX] = "rx",
    [EVENT_HW_TX] = "tx",
    [EVENT_SW] = "sw",
    [EVENT_NONE] = "none",
};

static char *rate_str[] = {
    [BLINK_RATE1] = "blink_rate1",
    [BLINK_RATE2] = "blink_rate2",
};

static char *mode_str[] = {
    [HW_BLINK_AND] = "hw blink AND",
    [HW_BLINK_OR] = "hw blink OR",
    [SW_MODE] = "sw",
};

#define RTK_PORT_LED_PORT_CNT 4

static struct rtk_port_led_ctrl *glb_led_ctrl;
static int led_control=2;

static void rtk_port_led_write(void __iomem *reg, unsigned long data, struct rtk_port_led_cfg *cfg)
{
    if (NULL==cfg)
        iowrite32(data, reg);
    else {
        cfg->data = data;
        if (led_control==2)
            iowrite32(data, reg);
    }
}

static unsigned long rtk_port_led_read(void __iomem *reg, struct rtk_port_led_cfg *cfg)
{
    if (NULL==cfg)
        return ioread32(reg);
    else
        return cfg->data;
}

static void rtk_port_led_reg_set_bit(void __iomem *reg, int bit)
{
    u32 val;

    val = ioread32(reg);
    val |= 1 << bit;

    iowrite32(val, reg);
}

static void rtk_port_led_reg_clear_bit(void __iomem *reg, int bit)
{
    u32 val;

    val = ioread32(reg);
    val &= ~(1 << bit);

    iowrite32(val, reg);
}

/**
 * rtk_port_led_hw_enable - switch LED to off or triggered by event
 */
static int rtk_port_led_hw_enable(int idx, bool enable)
{
    struct rtk_port_led_cfg *led_cfg;
    unsigned long flags;
    u32 val;
    int rc;

    if (idx >= CA77XX_LED_MAX_COUNT)
        return -EINVAL;

    if (!glb_led_ctrl) {
        printk("ERROR!! glb_led_ctrl is not set\n");
        return -ENODEV;
    }

    led_cfg = glb_led_ctrl->led_cfg[idx];

    if (!led_cfg) {
        dev_err(glb_led_ctrl->dev, "Led %d has not been probed!\n", idx);
        return -ENODEV;
    }

    if (led_cfg->mode == SW_MODE)
        return -EINVAL;

    spin_lock_irqsave(led_cfg->lock, flags);

    val = rtk_port_led_read(led_cfg->mem, led_cfg);

    switch (led_cfg->mode) {
    case HW_BLINK_AND:
        if (enable) {
            led_cfg->enable = 1;

            /* driven by event */
            val &= ~(CA77XX_LED_OFF_ON_MASK << CA77XX_LED_OFF_ON_OFFSET);
        } else {
            led_cfg->enable = 0;

            /* force off */
            val &= ~(CA77XX_LED_OFF_ON_MASK << CA77XX_LED_OFF_ON_OFFSET);
            val |= 0x2 << CA77XX_LED_OFF_ON_OFFSET;
        }
        break;
    case HW_BLINK_OR:
        if (enable) {
            led_cfg->enable = 1;

            /* driven by event */
            val &= ~(CA77XX_LED_OFF_ON_MASK << CA77XX_LED_OFF_ON_OFFSET);
        } else {
            led_cfg->enable = 0;

            /* force off, a workaroud approach for or mode bug */
            val &= ~(CA77XX_LED_OFF_ON_MASK << CA77XX_LED_OFF_ON_OFFSET);
            val |= 0x1 << CA77XX_LED_OFF_ON_OFFSET;
        }
        break;
    default:
        /*do nothing*/
        break;
    }

    rtk_port_led_write(led_cfg->mem, val, led_cfg);

    spin_unlock_irqrestore(led_cfg->lock, flags);

    return 0;
}

int rtk_port_led_hw_blink_on(int port)
{
    if (port >= glb_led_ctrl->port_cnt)
        return -EINVAL;

    rtk_port_led_hw_enable(glb_led_ctrl->port2led[port], 1);
    return 0;
}
EXPORT_SYMBOL(rtk_port_led_hw_blink_on);

int rtk_port_led_hw_blink_off(int port)
{
    if (port >= glb_led_ctrl->port_cnt)
        return -EINVAL;

    rtk_port_led_hw_enable(glb_led_ctrl->port2led[port], 0);
    return 0;
}
EXPORT_SYMBOL(rtk_port_led_hw_blink_off);

static int rtk_port_led_gpio_request(int idx)
{
    struct rtk_port_led_cfg *led_cfg;
    unsigned long flags;
    int rc;

    if (idx >= CA77XX_LED_MAX_COUNT)
        return -EINVAL;

    if (!glb_led_ctrl) {
        printk("ERROR!! glb_led_ctrl is not set\n");
        return -ENODEV;
    }

    led_cfg = glb_led_ctrl->led_cfg[idx];

    if (!led_cfg) {
        dev_err(glb_led_ctrl->dev, "Led %d has not been probed!\n", idx);
        return -ENODEV;
    }

    dev_info(led_cfg->dev, "LED %d request gpio\n", led_cfg->idx);

    /* requeset the muxed gpio pin as output */
    rc = devm_gpio_request_one(led_cfg->dev, led_cfg->idx, GPIOF_OUT_INIT_HIGH, led_cfg->cdev.name);
    if (rc < 0) {
        dev_err(led_cfg->dev, "Can not request muxed gpio pin %d!!\n", led_cfg->idx);
        return rc;
    }

    return 0;
}

int rtk_port_led_hw_blink_request(int port)
{
    int rc;

    if (port >= glb_led_ctrl->port_cnt)
        return -EINVAL;

    rc = rtk_port_led_gpio_request(glb_led_ctrl->port2led[port]);
    return rc;
}
EXPORT_SYMBOL(rtk_port_led_hw_blink_request);

/**
 * rtk_port_led_config - config LED trigger behavior
 */
static int rtk_port_led_config(int idx, int active_low, int off_event, int blink_event,
                               int on_event, int port, int blink, int mode)
{
    struct rtk_port_led_cfg *led_cfg;
    unsigned long flags;
    u32 val;

    if (idx >= CA77XX_LED_MAX_COUNT)
        return -EINVAL;
    if (off_event > EVENT_NONE)
        return -EINVAL;
    if (blink_event > BIT(EVENT_NONE))
        return -EINVAL;
    if (on_event > EVENT_NONE)
        return -EINVAL;
    if (port >= CA77XX_LED_MAX_PORT)
        return -EINVAL;
    if (blink > BLINK_RATE2)
        return -EINVAL;

    led_cfg = glb_led_ctrl->led_cfg[idx];

    spin_lock_irqsave(led_cfg->lock, flags);

    val = rtk_port_led_read(led_cfg->mem, led_cfg);

    dev_info(led_cfg->cdev.dev, "change to mode: %s\n", mode_str[mode]);
    switch (mode) {
    case HW_BLINK_AND:
        led_cfg->mode = HW_BLINK_AND;
        val &= ~CA77XX_LED_BLINK_OR;

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

        val &= ~(CA77XX_LED_EVENT_OFF_MASK << CA77XX_LED_EVENT_OFF_OFFSET);
        if (off_event < EVENT_NONE)
            val |= BIT(off_event) << CA77XX_LED_EVENT_OFF_OFFSET;
        val &= ~(CA77XX_LED_EVENT_BLINK_MASK << CA77XX_LED_EVENT_BLINK_OFFSET);
        val |= blink_event << CA77XX_LED_EVENT_BLINK_OFFSET;
        val &= ~(CA77XX_LED_EVENT_ON_MASK << CA77XX_LED_EVENT_ON_OFFSET);
        if (on_event < EVENT_NONE)
            val |= BIT(on_event) << CA77XX_LED_EVENT_ON_OFFSET;

        break;

    case HW_BLINK_OR:
        led_cfg->mode = HW_BLINK_OR;
        val |= CA77XX_LED_BLINK_OR;

        /*
         * LED polarity should be inverted as a workaroud
         * for blink or mode bug.
         */
        if (active_low) {
            led_cfg->active_low = true;
            val &= ~CA77XX_LED_OFF_VAL;
        } else {
            led_cfg->active_low = false;
            val |= CA77XX_LED_OFF_VAL;
        }

        /*
         * All the events should be ORed as a workaroud
         * for blink or mode bug.
         */
        led_cfg->off_event = EVENT_NONE;
        led_cfg->blink_event = BIT(EVENT_HW_RX) | BIT(EVENT_HW_TX) | BIT(EVENT_SW);
        led_cfg->on_event = EVENT_NONE;

        val &= ~CA77XX_LED_SW_EVENT;
        val &= ~(CA77XX_LED_EVENT_OFF_MASK << CA77XX_LED_EVENT_OFF_OFFSET);
        val &= ~(CA77XX_LED_EVENT_BLINK_MASK << CA77XX_LED_EVENT_BLINK_OFFSET);
        val |= led_cfg->blink_event << CA77XX_LED_EVENT_BLINK_OFFSET;
        val &= ~(CA77XX_LED_EVENT_ON_MASK << CA77XX_LED_EVENT_ON_OFFSET);
        break;

    case SW_MODE:
        led_cfg->mode = SW_MODE;
        val &= ~CA77XX_LED_BLINK_OR;
        if (active_low) {
            led_cfg->active_low = true;
            val |= CA77XX_LED_OFF_VAL;
        } else {
            led_cfg->active_low = false;
            val &= ~CA77XX_LED_OFF_VAL;
        }

        /*
         * We only concern about on event in SW_MODE,
         * and only sw event should be set.
         */
        val &= ~(CA77XX_LED_EVENT_OFF_MASK << CA77XX_LED_EVENT_OFF_OFFSET);
        val &= ~(CA77XX_LED_EVENT_BLINK_MASK << CA77XX_LED_EVENT_BLINK_OFFSET);
        val &= ~(CA77XX_LED_EVENT_ON_MASK << CA77XX_LED_EVENT_ON_OFFSET);
        val |= BIT(EVENT_SW) << CA77XX_LED_EVENT_ON_OFFSET;
        break;

    default:
        return -EINVAL;
    }


    led_cfg->blink = blink;
    if (blink == BLINK_RATE1)
        val &= ~CA77XX_LED_BLINK_SEL;
    else if (blink == BLINK_RATE2)
        val |= CA77XX_LED_BLINK_SEL;

    led_cfg->port = port;
    val &= ~(CA77XX_LED_PORT_MASK << CA77XX_LED_PORT_OFFSET);
    val |= port << CA77XX_LED_PORT_OFFSET;

    rtk_port_led_write(led_cfg->mem, val, led_cfg);

    spin_unlock_irqrestore(led_cfg->lock, flags);

    return 0;
}

/**
 * rtk_port_led_set - set on/off software event
 */
static void rtk_port_led_set(struct led_classdev *led_cdev,
                             enum led_brightness value)
{
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long flags;
    u32 val;

    spin_lock_irqsave(led_cfg->lock, flags);
    val = rtk_port_led_read(led_cfg->mem, led_cfg);
    if (value == LED_OFF)
        val &= ~CA77XX_LED_SW_EVENT;
    else
        val |= CA77XX_LED_SW_EVENT;
    rtk_port_led_write(led_cfg->mem, val, led_cfg);
    spin_unlock_irqrestore(led_cfg->lock, flags);
}

static int rtk_port_blink_set(struct led_classdev *led_cdev,
                              unsigned long *delay_on, unsigned long *delay_off)
{
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    struct rtk_port_led_ctrl *glb_led;
    unsigned long delay, flags;
    u32 val;

    /* limit sw not to use hardware blink */
    if (led_cfg->mode == SW_MODE)
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
    val = rtk_port_led_read(led_cfg->mem, led_cfg);
    if (delay)
        val |= CA77XX_LED_BLINK_SEL;
    else
        val &= ~CA77XX_LED_BLINK_SEL;
    rtk_port_led_write(led_cfg->mem, val, led_cfg);
    spin_unlock_irqrestore(led_cfg->lock, flags);

    return 0;
}

/**
 * rtk_port_led_show_blink_event - show hardware blink event
 */
static ssize_t rtk_port_led_show_blink_event(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long flags;
    ssize_t len = 0;
    int i;

    spin_lock_irqsave(led_cfg->lock, flags);

    for (i = 0; i < EVENT_CNT; i++) {
        if (BIT(i) & led_cfg->blink_event || (i == EVENT_NONE && !led_cfg->blink_event))
            len += sprintf(buf + len, "[%s] ", event_str[i]);
        else
            len += sprintf(buf + len, "%s ", event_str[i]);
    }

    spin_unlock_irqrestore(led_cfg->lock, flags);

    len += sprintf(buf + len, "\n");
    return len;
}

/**
 * rtk_port_led_store_blink_event - set hardware blink event
 */
static ssize_t rtk_port_led_store_blink_event(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long flags;
    u32 val;
    char event[EVENT_CNT][EVENT_STR_LEN];
    int events;
    int i, j;

    events = sscanf(buf, "%s %s %s ", event[0], event[1], event[2]);
    if (events < 1)
        return -EINVAL;
    //else
    //printk("events: %d\n", events);

    led_cfg->blink_event = 0;
    for (i = 0; i < events; i++) {
        //printk("event[%d] = %s\n", i, event[i]);
        for (j = 0; j < EVENT_CNT; j++) {
            if (!strncmp(event[i], event_str[j], strlen(event_str[j]))) {
                if (j < EVENT_NONE)
                    led_cfg->blink_event |= BIT(j);
                break;
            }
        }
    }

    spin_lock_irqsave(led_cfg->lock, flags);

    val = rtk_port_led_read(led_cfg->mem, led_cfg);
    val &= ~(CA77XX_LED_EVENT_BLINK_MASK << CA77XX_LED_EVENT_BLINK_OFFSET);
    val |= led_cfg->blink_event << CA77XX_LED_EVENT_BLINK_OFFSET;
    rtk_port_led_write(led_cfg->mem, val, led_cfg);

    spin_unlock_irqrestore(led_cfg->lock, flags);

    return size;
}

static DEVICE_ATTR(blink_event, 0644, rtk_port_led_show_blink_event,
                   rtk_port_led_store_blink_event);

static ssize_t rtk_port_led_show_hw_port(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long flags;
    ssize_t ret = 0;

    spin_lock_irqsave(led_cfg->lock, flags);

    ret += sprintf(buf, "%d\n", led_cfg->port);

    spin_unlock_irqrestore(led_cfg->lock, flags);

    return ret;
}

static ssize_t rtk_port_led_store_hw_port(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long flags, param;

    u32 val;

    kstrtoul(buf, 10, &param);

    spin_lock_irqsave(led_cfg->lock, flags);

    led_cfg->port = param & CA77XX_LED_PORT_MASK;

    val = rtk_port_led_read(led_cfg->mem, led_cfg);
    val &= ~(CA77XX_LED_PORT_MASK << CA77XX_LED_PORT_OFFSET);
    val |= led_cfg->port << CA77XX_LED_PORT_OFFSET;
    rtk_port_led_write(led_cfg->mem, val, led_cfg);

    glb_led_ctrl->port2led[led_cfg->port] = led_cfg->idx;
    spin_unlock_irqrestore(led_cfg->lock, flags);

    return size;
}

static DEVICE_ATTR(hw_port, 0644, rtk_port_led_show_hw_port,
                   rtk_port_led_store_hw_port);

static ssize_t rtk_port_led_show_hw_blink(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
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

static ssize_t rtk_port_led_store_hw_blink(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
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

    val = rtk_port_led_read(led_cfg->mem, led_cfg);
    if (led_cfg->blink == BLINK_RATE1)
        val &= ~CA77XX_LED_BLINK_SEL;
    else if (led_cfg->blink == BLINK_RATE2)
        val |= CA77XX_LED_BLINK_SEL;
    rtk_port_led_write(led_cfg->mem, val, led_cfg);

    spin_unlock_irqrestore(led_cfg->lock, flags);

    return size;
}

static DEVICE_ATTR(hw_blink, 0644, rtk_port_led_show_hw_blink,
                   rtk_port_led_store_hw_blink);

static ssize_t rtk_port_led_show_hw_enable(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long flags;
    ssize_t ret = 0;

    spin_lock_irqsave(led_cfg->lock, flags);

    ret += sprintf(buf, "%d\n", led_cfg->enable);

    spin_unlock_irqrestore(led_cfg->lock, flags);

    return ret;
}

static ssize_t rtk_port_led_store_hw_enable(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long param;

    kstrtoul(buf, 10, &param);

    rtk_port_led_hw_enable(led_cfg->idx, param);

    return size;
}

static DEVICE_ATTR(hw_enable, 0644, rtk_port_led_show_hw_enable,
                   rtk_port_led_store_hw_enable);

static struct attribute *rtk_port_led_hw_blink_attrs[] = {
    &dev_attr_blink_event.attr,
    &dev_attr_hw_port.attr,
    &dev_attr_hw_blink.attr,
    &dev_attr_hw_enable.attr,
    NULL
};
ATTRIBUTE_GROUPS(rtk_port_led_hw_blink);

static ssize_t rtk_port_led_show_mode(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long flags;
    ssize_t ret = 0;

    spin_lock_irqsave(led_cfg->lock, flags);

    ret += sprintf(buf, "current mode: [%d] %s\n\n%d: %s\n%d: %s\n%d: %s\n",
                   led_cfg->mode, mode_str[led_cfg->mode],
                   HW_BLINK_AND, mode_str[HW_BLINK_AND],
                   HW_BLINK_OR, mode_str[HW_BLINK_OR],
                   SW_MODE, mode_str[SW_MODE]);

    spin_unlock_irqrestore(led_cfg->lock, flags);

    return ret;
}

static ssize_t rtk_port_led_store_mode(struct device *dev,
                                       struct device_attribute *attr,
                                       const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    int mode;

    if (sscanf(buf, "%d", &mode) != 1 || mode < 0 || mode > SW_MODE)
        return -EINVAL;

    if (mode == led_cfg->mode)
        return size;

    led_cfg->mode = mode;
    rtk_port_led_config(led_cfg->idx, led_cfg->active_low, led_cfg->off_event, led_cfg->blink_event,
                        led_cfg->on_event, led_cfg->port, led_cfg->blink, led_cfg->mode);

    if (led_cfg->mode == SW_MODE) {
        led_sysfs_enable(&led_cfg->cdev);
        sysfs_remove_groups(&led_cfg->cdev.dev->kobj, &rtk_port_led_hw_blink_groups);
        rtk_port_led_set(&led_cfg->cdev, led_cfg->cdev.brightness);
    } else {
        led_sysfs_disable(&led_cfg->cdev);
        led_cfg->cdev.default_trigger = "none";
        sysfs_create_groups(&led_cfg->cdev.dev->kobj, &rtk_port_led_hw_blink_groups);
    }
    return size;
}

static DEVICE_ATTR(mode, 0644, rtk_port_led_show_mode,
                   rtk_port_led_store_mode);


static ssize_t rtk_port_led_show_hw_id(struct device *dev,
                                       struct device_attribute *attr,
                                       const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct rtk_port_led_cfg *led_cfg =
        container_of(led_cdev, struct rtk_port_led_cfg, cdev);
    unsigned long flags;
    ssize_t ret = 0;

    spin_lock_irqsave(led_cfg->lock, flags);

    ret += sprintf(buf, "%d\n", led_cfg->idx);

    spin_unlock_irqrestore(led_cfg->lock, flags);

    return ret;
}

static DEVICE_ATTR(hw_id, 0444, rtk_port_led_show_hw_id, NULL);

static struct attribute *rtk_port_led_default_attrs[] = {
    &dev_attr_hw_id.attr,
    &dev_attr_mode.attr,
    NULL
};
ATTRIBUTE_GROUPS(rtk_port_led_default);

static int rtk_port_led_probe(struct rtk_port_led_ctrl *led_ctrl,
                              struct device_node *nc, u32 reg, void __iomem *mem,
                              spinlock_t *lock)
{
    struct rtk_port_led_cfg *led_cfg;
    unsigned long flags;
    const char *state;
    u32 val;
    int rc, active_low, off_event, blink_event, on_event, port, blink, mode;

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
        off_event = EVENT_HW_RX;
    else if (of_property_read_bool(nc, "off,hw-tx"))
        off_event = EVENT_HW_TX;
    else if (of_property_read_bool(nc, "off,sw"))
        off_event = EVENT_SW;
    else
        off_event = EVENT_NONE;

    blink_event = EVENT_NONE;
    if (of_property_read_bool(nc, "blinking,hw-rx"))
        blink_event |= BIT(EVENT_HW_RX);
    if (of_property_read_bool(nc, "blinking,hw-tx"))
        blink_event |= BIT(EVENT_HW_TX);
    if (of_property_read_bool(nc, "blinking,sw"))
        blink_event |= BIT(EVENT_SW);

    if (of_property_read_bool(nc, "on,hw-rx"))
        on_event = EVENT_HW_RX;
    else if (of_property_read_bool(nc, "on,hw-tx"))
        on_event = EVENT_HW_TX;
    else if (of_property_read_bool(nc, "on,sw"))
        on_event = EVENT_SW;
    else
        on_event = EVENT_NONE;

    if (!of_property_read_u32(nc, "port", &val))
        port = val;
    else
        port = 0;

    led_ctrl->port2led[port] = led_cfg->idx;

    if (of_property_read_bool(nc, "blink-rate1"))
        blink = BLINK_RATE1;
    else
        blink = BLINK_RATE2;

    if (!of_property_read_u32(nc, "mode", &val))
        mode = val;
    else
        mode = SW_MODE;

    rtk_port_led_config(led_cfg->idx, active_low, off_event, blink_event,
                        on_event, port, blink, mode);

    spin_lock_irqsave(lock, flags);
    spin_unlock_irqrestore(lock, flags);

    rtk_port_led_hw_enable(led_cfg->idx, led_cfg->enable);

    /* setup PER_SPARE */
    rtk_port_led_reg_set_bit(glb_led_ctrl->spare, led_cfg->idx);

    led_cfg->cdev.brightness_set = rtk_port_led_set;
    led_cfg->cdev.blink_set = rtk_port_blink_set;
    led_cfg->cdev.groups = rtk_port_led_default_groups;

    rc = devm_led_classdev_register(led_ctrl->dev, &led_cfg->cdev);
    if (rc < 0)
        return rc;

    dev_set_drvdata(led_cfg->cdev.dev, led_cfg);

    dev_dbg(led_ctrl->dev, "registered LED %s\n", led_cfg->cdev.name);

    if (led_cfg->mode == SW_MODE) {
        led_sysfs_enable(&led_cfg->cdev);
        if (!of_property_read_string(nc, "default-state", &state)) {
            if (!strcmp(state, "on"))
                led_cfg->cdev.brightness = LED_FULL;
            else
                led_cfg->cdev.brightness = LED_OFF;
        }
        rtk_port_led_gpio_request(led_cfg->idx);
        rtk_port_led_set(&led_cfg->cdev, led_cfg->cdev.brightness);
    } else {
        led_sysfs_disable(&led_cfg->cdev);
        led_cfg->cdev.default_trigger = "none";
        sysfs_create_groups(&led_cfg->cdev.dev->kobj, &rtk_port_led_hw_blink_groups);
        /*
         * PHY port driver will call rtk_port_led_hw_blink_request to request the
         * corresponding gpio pin after the port initialization.
         */
    }

    return 0;
}

static ssize_t rtk_port_leds_blink_rate1_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct rtk_port_led_ctrl *led_ctrl = dev_get_drvdata(dev);
    unsigned long flags;
    ssize_t ret = 0;

    spin_lock_irqsave(led_ctrl->lock, flags);

    ret += sprintf(buf, "%d\n", (led_ctrl->blink_rate1 + 1) * 16);

    spin_unlock_irqrestore(led_ctrl->lock, flags);

    return ret;
}

static ssize_t rtk_port_leds_blink_rate1_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct rtk_port_led_ctrl *led_ctrl = dev_get_drvdata(dev);
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
    val = rtk_port_led_read(led_ctrl->mem, NULL);
    val &= ~(CA77XX_LED_BLINK_RATE1_MASK << CA77XX_LED_BLINK_RATE1_OFFSET);
    val |= led_ctrl->blink_rate1 << CA77XX_LED_BLINK_RATE1_OFFSET;
    rtk_port_led_write(led_ctrl->mem, val, NULL);
    spin_unlock_irqrestore(led_ctrl->lock, flags);

    return count;
}
static DEVICE_ATTR(blink_rate1, 0664, rtk_port_leds_blink_rate1_show,
                   rtk_port_leds_blink_rate1_store);

static ssize_t rtk_port_leds_blink_rate2_show(struct device *dev,
        struct device_attribute *attr,
        char *buf)
{
    struct rtk_port_led_ctrl *led_ctrl = dev_get_drvdata(dev);
    unsigned long flags;
    ssize_t ret = 0;

    spin_lock_irqsave(led_ctrl->lock, flags);

    ret += sprintf(buf, "%d\n", (led_ctrl->blink_rate2 + 1) * 16);

    spin_unlock_irqrestore(led_ctrl->lock, flags);

    return ret;
}

static ssize_t rtk_port_leds_blink_rate2_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct rtk_port_led_ctrl *led_ctrl = dev_get_drvdata(dev);
    unsigned long flags, param;
    u32 val;

    kstrtoul(buf, 10, &param);

    param = param / 16 - 1;
    if (param > CA77XX_LED_MAX_HW_BLINK)
        param = CA77XX_LED_MAX_HW_BLINK;

    spin_lock_irqsave(led_ctrl->lock, flags);
    led_ctrl->blink_rate2 = param;
    val = rtk_port_led_read(led_ctrl->mem, NULL);
    val &= ~(CA77XX_LED_BLINK_RATE2_MASK << CA77XX_LED_BLINK_RATE2_OFFSET);
    val |= led_ctrl->blink_rate2 << CA77XX_LED_BLINK_RATE2_OFFSET;
    rtk_port_led_write(led_ctrl->mem, val, NULL);
    spin_unlock_irqrestore(led_ctrl->lock, flags);

    return count;
}
static DEVICE_ATTR(blink_rate2, 0664, rtk_port_leds_blink_rate2_show,
                   rtk_port_leds_blink_rate2_store);

static struct attribute *rtk_port_leds_attributes[] = {
    &dev_attr_blink_rate1.attr,
    &dev_attr_blink_rate2.attr,
    NULL
};

static const struct attribute_group rtk_port_leds_attr_group = {
    .attrs = rtk_port_leds_attributes,
};

static int rtk_port_leds_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *np = pdev->dev.of_node;
    struct device_node *child;
    struct resource *reg_res;
    void __iomem *mem;
    spinlock_t *lock;	/* protect LED resource access */
    u32 val, reg;
    unsigned long flags;

    glb_led_ctrl = devm_kzalloc(dev, sizeof(struct rtk_port_led_ctrl),
                                GFP_KERNEL);
    if (!glb_led_ctrl)
        return -ENOMEM;
    glb_led_ctrl->dev = dev;

    /* get led ctrl base addr */
    reg_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "led_ctrl");
    if (!reg_res)
        return -EINVAL;

    glb_led_ctrl->mem = devm_ioremap_resource(dev, reg_res);
    if (IS_ERR(glb_led_ctrl->mem))
        return PTR_ERR(glb_led_ctrl->mem);
    dev_info(dev, "resource - %pr mapped at 0x%pK\n", reg_res, glb_led_ctrl->mem);

    /* get led spare addr */
    reg_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "spare");
    if (!reg_res)
        return -EINVAL;

    glb_led_ctrl->spare = devm_ioremap_resource(dev, reg_res);
    if (IS_ERR(glb_led_ctrl->spare))
        return PTR_ERR(glb_led_ctrl->spare);
    dev_info(dev, "resource - %pr mapped at 0x%pK\n", reg_res, glb_led_ctrl->spare);

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
    rtk_port_led_write(glb_led_ctrl->mem, reg, NULL);
    spin_unlock_irqrestore(lock, flags);

    //get supported port count
    if (!of_property_read_u32(np, "num-ports", &val)) {
        glb_led_ctrl->port_cnt = val;
        glb_led_ctrl->port2led = devm_kzalloc(dev, sizeof(int)*val, GFP_KERNEL);
        dev_info(dev, "supported port count: %d\n", glb_led_ctrl->port_cnt);
    } else {
        glb_led_ctrl->port_cnt = RTK_PORT_LED_PORT_CNT;
        glb_led_ctrl->port2led = devm_kzalloc(dev, sizeof(int)*RTK_PORT_LED_PORT_CNT, GFP_KERNEL);
        dev_info(dev, "default supported port count: %d\n", glb_led_ctrl->port_cnt);
    }

    mem = glb_led_ctrl->mem;
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

        rc = rtk_port_led_probe(glb_led_ctrl, child, reg, mem + reg * 4,
                                lock);
        if (rc < 0) {
            of_node_put(child);
            return rc;
        }
    }

    dev_set_drvdata(dev, glb_led_ctrl);

    return sysfs_create_group(&dev->kobj, &rtk_port_leds_attr_group);
}

static int rtk_port_leds_revome(struct platform_device *pdev)
{
    int i;

    sysfs_remove_group(&pdev->dev.kobj, &rtk_port_leds_attr_group);

    /* clear all PER_SPARE */
    for (i = 0; i < CA77XX_LED_MAX_COUNT; i++)
        rtk_port_led_reg_clear_bit(glb_led_ctrl->spare, i);

    return 0;
}

static const struct of_device_id rtk_port_leds_of_match[] = {
    { .compatible = "realtek,rtk_port-leds", },
    { },
};
MODULE_DEVICE_TABLE(of, rtk_port_leds_of_match);

static struct platform_driver rtk_port_leds_driver = {
    .probe = rtk_port_leds_probe,
    .remove = rtk_port_leds_revome,
    .driver = {
        .name = "leds-rtk_port",
        .of_match_table = rtk_port_leds_of_match,
    },
};

module_platform_driver(rtk_port_leds_driver);

MODULE_DESCRIPTION("LED driver for CA8277B controllers");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:leds-rtk_port");
