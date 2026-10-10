#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <soc/cortina/g3_registers.h>


extern struct proc_dir_entry *realtek_proc;


#define G3_Temperature_HIGH_WORKAROUND 1

#define G3HGU_POWER_SAVING_EXTRA   1

#ifdef G3_Temperature_HIGH_WORKAROUND
/***********************************************************/
static void rtk_GLOBAL_GLOBAL_CONFIG(void)
{
    GLOBAL_GLOBAL_CONFIG_t reg_v;
    void __iomem *addr;
    addr = ioremap(GLOBAL_GLOBAL_CONFIG, 4);

    if (addr) {
        reg_v.wrd = readl_relaxed(addr);
        /* ca_uint32_t crypto_pd            :  1 ;  bits 30:30 */
        reg_v.bf.crypto_pd = 1;
        /* ca_uint32_t offload0_pd          :  1 ;  bits 28:28  */
        reg_v.bf.offload0_pd = 1;
        writel_relaxed(reg_v.wrd, addr);
        printk
            ("GLOBAL_GLOBAL_CONFIG: crypto_clock(%s), offload0_clock(%s).\n",
             reg_v.bf.crypto_pd ? "PWR_DOWN" : "PWR_UP",
             reg_v.bf.offload0_pd ? "PWR_DOWN" : "PWR_UP");
        smp_mb();
        iounmap(addr);
    } else {
        printk("GLOBAL_GLOBAL_CONFIG: ioremap failed!\n");
    }
}

/********* Temperature too high issue**********************/
static void rtk_chane_XFI_RG46(void)
{

    XFI_RG46_t reg_v;
    void __iomem *addr;
    addr = ioremap(XFI_RG46, 4);

    if (addr) {
        reg_v.wrd = readl_relaxed(addr);
        reg_v.bf.POW_PCIX = 0;
        reg_v.bf.PDOWN = 1;
        smp_mb();
        writel_relaxed(reg_v.wrd, addr);
#if 0
        smp_mb();
        reg_v.wrd = readl_relaxed(addr);
        printk("XFI_RG46(0x%08x) = 0x%08x\n", XFI_RG46, reg_v.wrd);
#endif
        iounmap(addr);
    } else {
        printk("XFI_RG46: ioremap failed!\n");
    }

}

/***********************************************
Saturn:
    All Saturn registers can be access by G3(CA8277/CA879):
	G3    <-----> Saturn
    0xf30xxxxx => 0xd00xxxxx, APB0/APB1
    0xf31xxxxx => 0x441xxxxx, APB2
    0xf32xxxxx => 0x522xxxxx, APB3
    0xf38xxxxx => 0x400xxxxx, SRAM

**************************************************/
#define SATURN_CLKGEN_CPLLDIV               0xf3109010
#define SATURN_GLOBAL_PIN_MUX               0xf3100050
#define SATURN_GLOBAL_GPIO_MUX_2            0xf3100060
#define SATURN_GLOBAL_GPIO_MUX_3            0xf3100064
#define SATURN_GLOBAL_GIGE_PHY              0xf3100068
#define SATURN_CLKGEN_TAROKO_AP_FREQ_CONF   0xf3109028
#define SATURN_CLKGEN_TAROKO_AP_FREQ_CHG    0xf310902c

static void rtk_peri_reg_write(uint32_t paddr, uint32_t reg_value)
{
    void __iomem *addr;
    addr = ioremap(paddr, 4);
    if (addr) {
        writel_relaxed(reg_value, addr);
        iounmap(addr);
    } else {
        printk("ioremap(0x%08x) fail!\n", paddr);
    }
}


static int rtk_peri_reg_read(uint32_t paddr, uint32_t * reg_v)
{
    void __iomem *addr;

    addr = ioremap(paddr, 4);
    if (addr) {
        *reg_v = readl_relaxed(addr);
        iounmap(addr);
        return 0;
    } else {
        return -1;
    }
}




static void rtk_change_saturn_setting(void)
{

/************************************************
	# core-clk: 333->200M
        devmem 0xf3109010 32 0x088a0000
*****************************************************/
    rtk_peri_reg_write(SATURN_CLKGEN_CPLLDIV, 0x088a0000);
    //This settings will cause Satrun can't interrupt G3
/******************************************************
 # GPIO-3// reference clock output(CPLL) disable(bit1)
        devmem 0xf3100064 32 0x0000000e
*******************************************************/
    rtk_peri_reg_write(SATURN_GLOBAL_GPIO_MUX_3, 0x0000000e);
/*****************************************************
 # turn off PCM output sclk
        devmem 0xf3100050 32 0x02081004
************************************************************/
//    rtk_peri_reg_write(SATURN_GLOBAL_PIN_MUX, 0x02881004);
/***********************************************************
# turn off SD output clock --- didn't see obvious changes
        devmem 0xf3100060 32 0x20
*********************************************************/
    rtk_peri_reg_write(SATURN_GLOBAL_GPIO_MUX_2, 0x20);
/*********************************************************
# Kernel software should not open port 3/4/5
# reset GE PHY(port 3)
        devmem 0xf3100068 32 0
***********************************************************/
    rtk_peri_reg_write(SATURN_GLOBAL_GIGE_PHY, 0);
/***********************************************************
# Overwrite CPU speed to 125MHz
        devmem 0xf3109028 32 0x41ff0410
        devmem 0xf310902c 32 0x1
************************************************************/
    rtk_peri_reg_write(SATURN_CLKGEN_TAROKO_AP_FREQ_CONF, 0x41ff0410);
    rtk_peri_reg_write(SATURN_CLKGEN_TAROKO_AP_FREQ_CHG, 0x1);
/*****************************************************************
devmem 0xf3100058 32 0X00001BC7
****************************************************************/
    rtk_peri_reg_write(0xf3100058, 0x00001BC7);
}

static void rtk_change_g3_setting(void)
{
/*******************2018/12/12 *******************************
devmem 0xf432013c w 0x20101A1A
devmem 0xf432005c 32 0x000000FF
devmem 0xf4320060 32 0X0000000D
devmem 0xf4320054 32 0X00018000
************************************************************/
#if 0
//This setting cause PON no traffic.
    rtk_peri_reg_write(GLOBAL_EPLLDIV, 0x20101A1A);
#endif
#if 0                           //Move to gpio dervier : gpio-ca77xx.c
#ifdef CONFIG_LEDS_CA77XX
    rtk_peri_reg_write(PER_GPIO0_CFG, 0xFFFF87F7);
#endif
#endif
    rtk_peri_reg_write(GLOBAL_GPIO_MUX_3, 0x000000FF);
    rtk_peri_reg_write(GLOBAL_GPIO_MUX_4, 0X0000000D);
    rtk_peri_reg_write(GLOBAL_GPIO_MUX_1, 0X00018000);

}

#if G3HGU_POWER_SAVING_EXTRA
/*****************************************
* 2019/02/13
* Discussion: CH/CA, Jerry/HW
*
* 1.	Enable GHPY EEE feature
	1.1 NI_HV_PT_EEE_LPI_CFG0, offset=0xA0
* 2.   	turn off G3 RGMII
* 3. .	turn off saturn RGMII
*
*******************************************/
#define NI_HV_PT_EEE_LPI_CFG0_OFFSET  0xA0
#define NI_HV_PT_EEE_LPI_CFG0_COUNT   5

static unsigned int GPHY_REGS_ARY[] =
    { 0xf433827c, 0xf4338264, 0xf43381fc, 0xf43381e4, 0xf433817c,
0xf4338164, 0xf43380fc, 0xf43380e4 };
static unsigned int GPHY_REGS_VALUE[] =
    { 0xa43, 0x0060, 0xa43, 0x0060, 0xa43, 0x0060, 0xa43, 0x0060 };

static void rtk_enable_GHPY_EEE_feature(void)
{
    NI_HV_PT_EEE_LPI_CFG0_t reg_v;
    int i;
    int array_size = 0;
    void __iomem *addr;

    for (i = 0; i < NI_HV_PT_EEE_LPI_CFG0_COUNT; i++) {
        addr =
            ioremap((NI_HV_PT_EEE_LPI_CFG0 +
                     i * NI_HV_PT_EEE_LPI_CFG0_OFFSET), 4);
        if (addr) {
            reg_v.wrd = readl_relaxed(addr);
            reg_v.bf.tx_eee_enable = 1;
            smp_mb();
            writel_relaxed(reg_v.wrd, addr);
            iounmap(addr);
            addr = NULL;
        }
    }

    array_size = sizeof(GPHY_REGS_ARY) / sizeof(int);

    for (i = 0; i < array_size; i++) {
        rtk_peri_reg_write(GPHY_REGS_ARY[i], GPHY_REGS_VALUE[i]);
        printk("[%s]Addr(0x%08x) = 0x%08x\n", __func__, GPHY_REGS_ARY[i],
               GPHY_REGS_VALUE[i]);
    }

}

static void rtk_GHPY_EEE_feature_dump(void)
{
    NI_HV_PT_EEE_LPI_CFG0_t reg_v;
    uint32_t tmp = 0;
    uint32_t reg = 0;
    int i;
    int array_size = 0;
    void __iomem *addr;

    printk("Dump GHPY_EEE_feature Setting:\n");

    reg = NI_HV_PT_EEE_LPI_CFG0;
    for (i = 0; i < NI_HV_PT_EEE_LPI_CFG0_COUNT; i++) {
        addr =
            ioremap((NI_HV_PT_EEE_LPI_CFG0 +
                     i * NI_HV_PT_EEE_LPI_CFG0_OFFSET), 4);
        if (addr) {
            reg_v.wrd = readl_relaxed(addr);
            printk("Addr(0x%08x) = 0x%08x\n",
                   (NI_HV_PT_EEE_LPI_CFG0 +
                    i * NI_HV_PT_EEE_LPI_CFG0_OFFSET), reg_v.wrd);
            iounmap(addr);
            addr = NULL;
        }
    }

    array_size = sizeof(GPHY_REGS_ARY) / sizeof(int);

    for (i = 0; i < array_size; i++) {
        rtk_peri_reg_read(GPHY_REGS_ARY[i], &tmp);
        printk("Addr(0x%08x) = 0x%08x\n", GPHY_REGS_ARY[i], tmp);
    }

}

/**********************************************
4.turn off RGMII
G3 :
devmem 0xf4304684 w 0x00001804
Saturn :
devmem 0xf3004624 w 0x00001804
**********************************************/
#define G3_RGMII_CONTROL_REG       0xf4304684
#define G3_RGMII_VALUE             0x00001804
#define SATURN_RGMII_CONTROL_REG   0xf3004624
#define SATURN_RGMII_VALUE         0x00001804

static void rtk_trun_off_RGMII(void)
{
    rtk_peri_reg_write(G3_RGMII_CONTROL_REG, G3_RGMII_VALUE);
    rtk_peri_reg_write(SATURN_RGMII_CONTROL_REG, SATURN_RGMII_VALUE);
}

static void rtk_RGMII_dump(void)
{
    uint32_t tmp = 0;
    printk("---- RGMII ------\n");
    rtk_peri_reg_read(G3_RGMII_CONTROL_REG, &tmp);
    printk("Addr(0x%08x) = 0x%08x\n", G3_RGMII_CONTROL_REG, tmp);
    rtk_peri_reg_read(SATURN_RGMII_CONTROL_REG, &tmp);
    printk("Addr(0x%08x) = 0x%08x\n", SATURN_RGMII_CONTROL_REG, tmp);
}

#endif
static void rtk_extra_dump(void)
{
#if G3HGU_POWER_SAVING_EXTRA
    rtk_GHPY_EEE_feature_dump();
    rtk_RGMII_dump();
#endif
}

static void rtk_power_saving_extra(void)
{
#if G3HGU_POWER_SAVING_EXTRA
    rtk_enable_GHPY_EEE_feature();
    rtk_trun_off_RGMII();
#endif
}

/************************************************************************************************/

static unsigned int G3_REG_ADD_LIST[] =
    { 0xF4331104, 0XF4320024, 0XF4320020, 0xf432013c, 0xf4329280,
0xf432005c, 0xf4320060, 0xf4320054 };

static unsigned int SATURN_REG_ADD_LIST[] =
    { 0xf3109010, 0xf3100064, 0xf3100050, 0xf3100060, 0xf3100068,
0xf3109028, 0xf3100058 };

static int rtk_g3_cooling_proc_read(struct seq_file *sf, void *v)
{
    uint32_t tmp = 0;
    int i = 0;
    int array_size;
    array_size = sizeof(G3_REG_ADD_LIST) / sizeof(int);
    printk("G3:\n");
    for (i = 0; i < array_size; i++) {
        rtk_peri_reg_read(G3_REG_ADD_LIST[i], &tmp);
        printk("Addr(0x%08x) = 0x%08x\n", G3_REG_ADD_LIST[i], tmp);
    }

    array_size = sizeof(SATURN_REG_ADD_LIST) / sizeof(int);
    printk("Satrun:\n");
    for (i = 0; i < array_size; i++) {
        rtk_peri_reg_read(SATURN_REG_ADD_LIST[i], &tmp);
        printk("Addr(0x%08x) = 0x%08x\n", G3_REG_ADD_LIST[i], tmp);
    }

    rtk_extra_dump();

    return 0;
}




static int rtk_g3_cooling_single_open(struct inode *inode,
                                      struct file *file)
{
    return (single_open(file, rtk_g3_cooling_proc_read, NULL));
}


static ssize_t rtk_g3_cooling_proc_write(struct file *file,
                                         const char __user * buffer,
                                         size_t count, loff_t * off)
{


    int tmp;
    if (sscanf(buffer, "%d", &tmp) != 1) {
        return -1;
    }


    if (tmp) {
        rtk_chane_XFI_RG46();
        rtk_change_saturn_setting();
        rtk_GLOBAL_GLOBAL_CONFIG();
        rtk_change_g3_setting();
        rtk_power_saving_extra();
    } else {
        printk("Can't revert previous setting.");
    }

    return count;
}




static const struct file_operations fops_g3_cooling = {
    .open = rtk_g3_cooling_single_open,
    .write = rtk_g3_cooling_proc_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

#endif

/********G3 PHY Control *******************/
/*******************************************
31 RW 0 s3_p_mdio_enable_reg
30 RW 0 s2_p_mdio_enable_reg
29 RW 0 s1_p_mdio_enable_reg
28 RW 0 s0_p_mdio_enable_reg
27 RW 0 sata_sel_phy
26 RW 0 s0_rxaui_mode
25 RW 0 s3_sata_sgmii_sel
24 RW 0 s3_combo_sel
23 RW 0 s2_combo_sel
22 RW 0 s1_combo_sel
21 R 0
20 R 0
19 R 0
18 R 0
17 RW 0 s3_POW_USB3
16 RW 0 s3_POW_SATA
15 RW 0 s3_SATA_SGMII_ISOLATE
14 RW 0 s3_USB3_ISOLATE
13 RW 0 s2_POW_USB3
12 RW 0 s2_POW_PCIE2
11 RW 0 s2_USB3_ISOLATE
10 RW 0 s2_PCIE2_ISOLATE
 9 RW 0 s1_POW_PCIE2
 8 RW 0 s1_PCIE2_ISOLATE
 7 RW 0 s1_POW_SATA
 6 RW 0 s1_SATA_ISOLATE
 5 RW 0 s0_POW_PCIE2_1
 4 RW 0 s0_PCIE2_ISOLATE_1
 3 RW 0 s0_POW_PCIE2_0
 2 RW 0 s0_PCIE2_ISOLATE_0
 1 RW 1 cfg_xfi1_10g
 0 RW 1 cfg_xfi0_10g
 *************************************************/
#define G3_PH_REG_BITS  32
char global_phy_control_str[G3_PH_REG_BITS][32] = {
    "cfg_xfi0_10g",
    "cfg_xfi1_10g",
    "s0_PCIE2_ISOLATE_0",
    "s0_POW_PCIE2_0",
    "s0_PCIE2_ISOLATE_1",
    "s0_POW_PCIE2_1",
    "s1_SATA_ISOLATE",
    "s1_POW_SATA",
    "s1_PCIE2_ISOLATE",
    "s1_POW_PCIE2",
    "s2_PCIE2_ISOLATE",
    "s2_USB3_ISOLATE",
    "s2_POW_PCIE2",
    "s2_POW_USB3",
    "s3_USB3_ISOLATE",
    "s3_SATA_SGMII_ISOLATE",
    "s3_POW_SATA",
    "s3_POW_USB3",
    "reserved1",
    "reserved2",
    "reserved3",
    "reserved4",
    "s1_combo_sel",
    "s2_combo_sel",
    "s3_combo_sel",
    "s3_sata_sgmii_sel",
    "s0_rxaui_mode",
    "sata_sel_phy",
    "s0_p_mdio_enable_reg",
    "s1_p_mdio_enable_reg",
    "s2_p_mdio_enable_reg",
    "s3_p_mdio_enable_reg"
};


static int rtk_phy_control_proc_read(struct seq_file *sf, void *v)
{
    GLOBAL_PHY_CONTROL_t reg_v;
    void __iomem *addr;
    int i;

    addr = ioremap(GLOBAL_PHY_CONTROL, 4);
    if (addr) {
        reg_v.wrd = readl_relaxed(addr);
        //printk("GLOBAL_PHY_CONTROL: addr(%p) = 0x%x\n", addr, reg_v.wrd);
        seq_printf(sf, "GLOBAL_PHY_CONTROL: 0x%x\n", reg_v.wrd);

        for (i = 0; i < G3_PH_REG_BITS; i++) {
            //printk("  --- %s : %d\n", &global_phy_control_str[i][0], reg_v.wrd & (1 << i));
            seq_printf(sf, "   %s, bits(%d): %d\n",
                       &global_phy_control_str[i][0], i,
                       (reg_v.wrd & (1 << i)) >> i);
        }

        iounmap(addr);
    } else {
        printk("GLOBAL_PHY_CONTROL: ioremap failed!\n");
    }


    return 0;
}

static int rtk_phy_control_single_open(struct inode *inode,
                                       struct file *file)
{
    return (single_open(file, rtk_phy_control_proc_read, NULL));
}

static void rtk_phy_control_bit_write(int bit)
{

    GLOBAL_PHY_CONTROL_t reg_v;
    void __iomem *addr;
    addr = ioremap(GLOBAL_PHY_CONTROL, 4);

    if (addr) {
        reg_v.wrd = readl_relaxed(addr);
        //printk("GLOBAL_PHY_CONTROL: addr(0x%x) = 0x%p\n", addr, reg_v.wrd);
        reg_v.wrd &= ~(1 << bit);
        writel_relaxed(reg_v.wrd, addr);
        smp_mb();
        reg_v.wrd = readl_relaxed(addr);
        //printk("GLOBAL_PHY_CONTROL: 0x%x\n", reg_v.wrd);
        iounmap(addr);
    } else {
        printk("GLOBAL_PHY_CONTROL: ioremap failed!\n");
    }

}

static ssize_t rtk_phy_control_proc_write(struct file *file,
                                          const char __user * buffer,
                                          size_t count, loff_t * off)
{

    char buf[128];
    int i;
    if (copy_from_user(buf, buffer, count)) {
        return -EFAULT;
    }

    buf[count - 1] = '\0';

    for (i = 0; i < G3_PH_REG_BITS; i++) {
        if (strncmp(buf, &global_phy_control_str[i][0], 32) == 0) {
            rtk_phy_control_bit_write(i);
            break;
        }
    }

    return count;
}


static const struct file_operations fops_phy_control = {
    .open = rtk_phy_control_single_open,
    .write = rtk_phy_control_proc_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init rtk_g3phy_clr_init(void)
{
    struct proc_dir_entry *pe;

    pe = proc_create_data("g3_phy_ctrl_bit_clr",
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
                          realtek_proc, &fops_phy_control, NULL);
    if (!pe) {
        return -EINVAL;
    }
#ifdef G3_Temperature_HIGH_WORKAROUND
    pe = proc_create_data("g3_cooling_temperature",
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
                          realtek_proc, &fops_g3_cooling, NULL);
    if (!pe) {
        return -EINVAL;
    }
#endif

    return 0;
}

static void __exit rtk_g3phy_clr_exit(void)
{
    return;
}

module_init(rtk_g3phy_clr_init);
module_exit(rtk_g3phy_clr_exit);
