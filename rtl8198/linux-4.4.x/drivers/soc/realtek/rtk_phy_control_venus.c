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

#include <soc/cortina/ca8277b_registers.h>
#define CORTINA_SERIAL_PH_REG_BITS  32

#define VENUS_Temperature_Cooling 1

/********VENUS PHY Control *******************/
char global_phy_control_str[CORTINA_SERIAL_PH_REG_BITS][32] = {
"cfg_xfi0_10g",
"cfg_xfi1_10g",
"s0_PCIE2_ISOLATE_0",
"s0_POW_PCIE2_0",
"s0_PCIE2_ISOLATE_1",
"s0_POW_PCIE2_1",
"s1_PCIE2_ISOLATE_0",
"s1_POW_PCIE2_0",
"s1_PCIE2_ISOLATE_1",
"s1_POW_PCIE2_1",
"s2_CPHY_ISOLATE",
"s2_ISO_ANA_B",
"s2_POW_PCIE2",
"s2_POW_USB3",
"reserved0",
"s3_ISO_ANA_B",
"reserved1",
"s3_POW_USB3",
"s3_POW_SGMII",
"USB2_ISOLATE",
"reserved2",
"reserved3",
"reserved4",
"s2_combo_sel",
"s3_combo_sel",
"s3_sata_sgmii_sel[0]",
"s3_sata_sgmii_sel[1]",
"s0_rxaui_mode",
"s0_p_mdio_enable_reg",
"s1_p_mdio_enable_reg",
"s2_p_mdio_enable_reg",
"s3_p_mdio_enable_reg",
};


extern struct proc_dir_entry *realtek_proc;

static void  __maybe_unused rtk_peri_reg_write(uint32_t paddr, uint32_t reg_value)
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


#ifdef VENUS_Temperature_Cooling
#define   CORE_PD                BIT(31)
#define   CRYPTO_PD              BIT(30)
#define   OFFLOAD1_PD            BIT(29)
#define   OFFLOAD0_PD            BIT(28)
#define   L3FE_PD                BIT(27)
#define   PS_PE_EN               BIT(26)
#define   PS_USB3_EN             BIT(25)
#define   PS_PCIE2_EN            BIT(24)
#define   PS_PCIE1_EN            BIT(23)
#define   PS_PCIE0_EN            BIT(22)
#define   REFCLK_SEL_1           BIT(19)
#define   REFCLK_SEL_0           BIT(18)
#define   EXT_ETH_REFCLK         BIT(17)
#define   EPON_PD                BIT(16)
#define   GPON_PD                BIT(15)
#define   CFG_PCIE_2_CLKEN       BIT(14)
#define   CFG_PCIE_1_CLKEN       BIT(12)
#define   CFG_PCIE_0_CLKEN       BIT(10)
u32 GLOBAL_CONFIG_BOOT_VALUE=0;
/* Disable some IP to cool.
 * 1. crypto
 * 2. offload0

*/
static void rtk_GLOBAL_GLOBAL_CONFIG(int action)
{
    GLOBAL_GLOBAL_CONFIG_t reg_v;
    void __iomem *addr;
    addr = ioremap(GLOBAL_GLOBAL_CONFIG, 4);

    if (addr) {
        switch (action){
            case 1:
                 reg_v.wrd = readl_relaxed(addr);
                 if(GLOBAL_CONFIG_BOOT_VALUE==0){
                    GLOBAL_CONFIG_BOOT_VALUE=reg_v.wrd;
                 }
                 /*xxx_PD:  When set power down core clock.,  1-> Power down */
                 reg_v.bf.crypto_pd = 1;
                 reg_v.bf.offload1_pd = 1;

                 /* When set to 1 xxxx totally powered up */
                 reg_v.bf.ps_pcie2_en =0;
                 reg_v.bf.cfg_pcie_2_clken=0;
                 smp_mb();
                 writel_relaxed(reg_v.wrd, addr);

                 break;
            case 0:
                 if(GLOBAL_CONFIG_BOOT_VALUE){
                     reg_v.wrd = GLOBAL_CONFIG_BOOT_VALUE;
                     writel_relaxed(reg_v.wrd, addr);
                 }
                 break;
        }
        iounmap(addr);
    } else {
        printk("GLOBAL_GLOBAL_CONFIG: ioremap failed!\n");
    }
}


static unsigned int REG_ADD_LIST[] =
    { GLOBAL_GLOBAL_CONFIG};

static int rtk_g3_cooling_proc_read(struct seq_file *sf, void *v)
{
    uint32_t tmp = 0;
    int i = 0;
    int array_size;
    array_size = sizeof(REG_ADD_LIST) / sizeof(int);
    printk("8277x:\n");
    for (i = 0; i < array_size; i++) {
        rtk_peri_reg_read(REG_ADD_LIST[i], &tmp);
        printk("Addr(0x%08x) = 0x%08x\n", REG_ADD_LIST[i], tmp);
    }
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
        rtk_GLOBAL_GLOBAL_CONFIG(1);
    } else {
        printk("Revert to Boot Setting.");
        rtk_GLOBAL_GLOBAL_CONFIG(0);
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

#endif  //VENUS_Temperature_Cooling


static int rtk_phy_control_proc_read(struct seq_file *sf, void *v)
{
    GLOBAL_PHY_CONTROL_t reg_v;
    void __iomem *addr;
    int i;

    addr = ioremap(GLOBAL_PHY_CONTROL, 4);
    if (addr) {
        reg_v.wrd = readl_relaxed(addr);
        seq_printf(sf, "GLOBAL_PHY_CONTROL: 0x%x\n", reg_v.wrd);

        for (i = 0; i < CORTINA_SERIAL_PH_REG_BITS; i++) {
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

    for (i = 0; i < CORTINA_SERIAL_PH_REG_BITS; i++) {
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
#if defined(VENUS_Temperature_Cooling)
    pe = proc_create_data("8277x_POWER_DOWN_UNUSED_UNIT",
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
