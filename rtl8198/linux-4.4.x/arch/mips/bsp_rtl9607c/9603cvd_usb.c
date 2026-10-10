/*
 * Copyright 2011, Realtek Semiconductor Corp.
 *
 * USB PHY access and USB MAC identification
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include "bspchip.h"
#include "rtl_usb_phy.h"

struct phy_regs {
    u32 ehci_utmi_reg;
    u32 usb2_ext_reg;
};
#define PADDR(addr)  ((addr) & 0x1FFFFFFF)
static u64 bsp_usb_dmamask = 0xFFFFFFFFUL;
/* USB Host Controller */
#ifdef CONFIG_USB_OHCI_HCD
static struct resource bsp_usb_ohci_resource[] = {
    [0] = {
        .start = PADDR(BSP_OHCI_BASE),
        .end   = PADDR(BSP_OHCI_BASE) + BSP_OHCI_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = BSP_USBHOSTP2_IRQ,
        .end   = BSP_USBHOSTP2_IRQ,
        .flags = IORESOURCE_IRQ,
    }
};

static struct platform_device bsp_usb_ohci_device = {
    .name          = "rtl86xx-ohci",
    .id            = 0,
    .num_resources = ARRAY_SIZE(bsp_usb_ohci_resource),
    .resource      = bsp_usb_ohci_resource,
    .dev           = {
        .dma_mask = &bsp_usb_dmamask,
        .coherent_dma_mask = 0xFFFFFFFFUL,
    }
};

#endif
#ifdef CONFIG_USB_EHCI_HCD
static struct resource bsp_usb_ehci_resource[] = {
    [0] = {
        .start = PADDR(BSP_EHCI_BASE),
        .end   = PADDR(BSP_EHCI_BASE) + BSP_EHCI_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = BSP_USBHOSTP2_IRQ,
        .end   = BSP_USBHOSTP2_IRQ,
        .flags = IORESOURCE_IRQ,
    }
};

static struct platform_device bsp_usb_ehci_device = {
    .name          = "rtl86xx-ehci",
    .id            = 0,
    .num_resources = ARRAY_SIZE(bsp_usb_ehci_resource),
    .resource      = bsp_usb_ehci_resource,
    .dev           = {
        .dma_mask = &bsp_usb_dmamask,
        .coherent_dma_mask = 0xFFFFFFFFUL,
    }
};
#endif

static struct platform_device *bsp_usb_devs0[] __initdata = {
#ifdef CONFIG_USB_OHCI_HCD
    &bsp_usb_ohci_device,
#endif
#ifdef CONFIG_USB_EHCI_HCD
    &bsp_usb_ehci_device,
#endif
};

/* ============== EHCI ============ */
#define LOOP_LIMIT 100000
#define UTMI_NOP   (1<<12)
#define UTMI_LOAD  (0<<12)
#define BUILD_CMD(vp,vctrl) (((vp)&0xf)<<13)|(((vctrl)&0xf)<<8)
#define UTMI_BUSY(x) (x&(1<<17))
static u32 utmi_wait(struct rtk_usb_phy *phy)
{
    u32 __reg, __c = 0;
    struct phy_regs *p = phy->priv;
    u32 utmi_reg = p->ehci_utmi_reg;
    mdelay(1);
    do {
        __c++;
        __reg = le32_to_cpu(REG32(utmi_reg));
        if (unlikely(__c>LOOP_LIMIT)) {
            printk("utmi_wait timeout\n");
            return 0;
        }
    } while (UTMI_BUSY(__reg));
    return __reg;
}

static void utmi_set(struct rtk_usb_phy *phy, u32 v)
{
    struct phy_regs *p = phy->priv;
    u32 utmi_reg = p->ehci_utmi_reg;
    utmi_wait(phy);
    REG32(utmi_reg) = cpu_to_le32(v);
}

static u32 utmi_get(struct rtk_usb_phy *phy)
{
    u32 reg;
    reg = utmi_wait(phy);
    return reg;
}

static int ehci_phy_get(struct rtk_usb_phy *phy, u8 port, u8 reg, u8 *data)
{

    // send [3:0]
    utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
    utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_LOAD);
    utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);

    // send [7:4]
    utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
    utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_LOAD);
    utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);

    *data = utmi_get(phy) & 0xff;

    return 0;
}


static int ehci_is_valid_param(u8 port, u8 reg)
{

    if (port > 2)
        return 0;

    if (((reg >= 0xe0) && (reg <= 0xe7)) ||
            ((reg >= 0xf0) && (reg <= 0xf7)) ||
            ((reg >= 0xb0) && (reg <= 0xb7)))
        return 1;

    return 0;
}

static int __ehci_phy_read(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 *val)
{
    int ret = 0;
    u8 data;

    //printk("%s(%d): %02x %02x %d\n",__func__,__LINE__,port,reg,ehci_is_valid_param(port, reg));
    if (!ehci_is_valid_param(port, reg))
        return -EINVAL;

    if (!((reg >= 0xb0) && (reg <= 0xb7)))
        reg -= 0x20;

    ehci_phy_get(phy, port, reg, &data);

    *val = data;

    return ret;
}

static int __ehci_phy_write(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val)
{
    struct phy_regs *p = phy->priv;
    u32 usb2_aux_reg = p->usb2_ext_reg+0xc;

    if (!ehci_is_valid_param(port, reg))
        return -EINVAL;

    //if (port)
    //	REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff0000) | ((val & 0xff) << 16);
    //else
    //	REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff) | (val & 0xff);
    REG32(usb2_aux_reg) = (val)&0xff;

    // send [3:0]
    utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
    utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_LOAD);
    utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);

    // send [7:4]
    utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
    utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_LOAD);
    utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);

    return 0;

}

static int __ehci_phy_get_next(struct rtk_usb_phy *phy, u8 port, int *reg)
{
    if (port > 2)
        return -EINVAL;

    if (*reg < 0)
        *reg = 0xe0;
    else if ((*reg >= 0xe0) && (*reg <= 0xe6))
        *reg += 1;
    else if (*reg == 0xe7)
        *reg = 0xf0;
    else if ((*reg >= 0xf0) && (*reg <= 0xf6))
        *reg += 1;
    else
        return -EINVAL;

    return 0;
}

static void __ehci_phy_init(struct rtk_usb_phy *phy)
{

    struct phy_regs *p = phy->priv;

    REG32(p->usb2_ext_reg + 0x10) &= ~(1<<5); /* disable force-host-disconnect */

    mdelay(10); /* allow IP to startup */

    __ehci_phy_write(phy, phy->port, 0xF4, 0x9B);
    __ehci_phy_write(phy, phy->port, 0xE4, 0x6C);
    __ehci_phy_write(phy, phy->port, 0xE7, 0x61);
    __ehci_phy_write(phy, phy->port, 0xF4, 0xBB);
    __ehci_phy_write(phy, phy->port, 0xE0, 0x21);
    __ehci_phy_write(phy, phy->port, 0xE0, 0x25);

    __ehci_phy_write(phy, phy->port, 0xF4, 0x9B);
    __ehci_phy_write(phy, phy->port, 0xE0, 0xE3);
    __ehci_phy_write(phy, phy->port, 0xE0, 0x63);
    __ehci_phy_write(phy, phy->port, 0xE0, 0xE3);

    __ehci_phy_write(phy, phy->port, 0xE0, 0xE3);
    __ehci_phy_write(phy, phy->port, 0xE1, 0x30);
    __ehci_phy_write(phy, phy->port, 0xE2, 0x0B);
    __ehci_phy_write(phy, phy->port, 0xE3, 0x8D);
    __ehci_phy_write(phy, phy->port, 0xE4, 0xE3);
    __ehci_phy_write(phy, phy->port, 0xE5, 0x19);
    __ehci_phy_write(phy, phy->port, 0xE6, 0xC2);
    __ehci_phy_write(phy, phy->port, 0xE7, 0x91);
    __ehci_phy_write(phy, phy->port, 0xF0, 0xFC);
    __ehci_phy_write(phy, phy->port, 0xF1, 0x8C);
    __ehci_phy_write(phy, phy->port, 0xF2, 0x00);
    __ehci_phy_write(phy, phy->port, 0xF3, 0x11);
    __ehci_phy_write(phy, phy->port, 0xF4, 0x9B);
    __ehci_phy_write(phy, phy->port, 0xF5, 0x04);
    __ehci_phy_write(phy, phy->port, 0xF6, 0x00);
    __ehci_phy_write(phy, phy->port, 0xF7, 0x0A);

    __ehci_phy_write(phy, phy->port, 0xF4, 0xbb); /* page0 = 0x9b, page1 = 0xbb */
    __ehci_phy_write(phy, phy->port, 0xE0, 0x25);
    __ehci_phy_write(phy, phy->port, 0xE1, 0xcf);
    __ehci_phy_write(phy, phy->port, 0xE2, 0x00);
    __ehci_phy_write(phy, phy->port, 0xE3, 0x00);
    __ehci_phy_write(phy, phy->port, 0xE4, 0x00);
    __ehci_phy_write(phy, phy->port, 0xE5, 0x11);
    __ehci_phy_write(phy, phy->port, 0xE6, 0x06);
    __ehci_phy_write(phy, phy->port, 0xE7, 0x66);
    __ehci_phy_write(phy, phy->port, 0xF4, 0x9b);

}

static struct phy_regs ehci_regs = {
    .ehci_utmi_reg = BSP_EHCI_UTMI_CTRL,
    .usb2_ext_reg = 0xB8140200,
};
static struct rtk_usb_phy ehci_phy = {
    .name = "ehci-phy",
    .port = 0,
    .vport = 1,
    .priv = &ehci_regs,
    .phy_init = __ehci_phy_init,
    .phy_read = __ehci_phy_read,
    .phy_write = __ehci_phy_write,
    .phy_get_next = __ehci_phy_get_next,
};

extern int rtk_usb_phy_register(struct rtk_usb_phy *phy);

int __init bsp_usb_init_9603cvd(void)
{
    int ret = 0;
#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_OHCI_HCD)
    REG32(BSP_IP_SEL) |= (1<<3) | (1<<31);
    mdelay(5);
    ehci_phy.port = 1;
    rtk_usb_phy_register(&ehci_phy);

    ret = platform_add_devices(bsp_usb_devs0, ARRAY_SIZE(bsp_usb_devs0));
    if (ret < 0) {
        printk("ERROR: unable to add EHCI\n");
        return ret;
    }
#endif
    return ret;
}


