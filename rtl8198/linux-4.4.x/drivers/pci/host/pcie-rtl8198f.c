/*
 * PCIe host controller driver for Cortina-Access CA77XX SoCs
 *
 * Copyright (C) 2015 Cortina Access, Inc.
 *		http://www.cortina-access.com
 *
 * Based on pci-exynos.c, pci-dra7xx.c, pci-mvebu.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/reset.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/gpio/consumer.h>
#include <linux/phy/phy.h>
#include <soc/cortina/ca77xx-atu.h>
#include <soc/cortina/g3lite_registers.h>  
#include "pcie-rtl8198f-dw.h"

#define to_ca77xx_pcie(x)	container_of(x, struct ca77xx_pcie, pp)

struct ca77xx_pcie {
	void __iomem		*reg_base; /* elbi */
	void __iomem		*serdes_base; /* serdes phy */
	phys_addr_t		dbi_start;
	phys_addr_t		dbi_end;

	struct gpio_desc	*dev_reset_gpio;

	struct clk		*clk;
	struct clk		*bus_clk;
	struct pcie_port	pp;

	struct irq_domain	*intx_irq_domain;
	struct device		*dev;

	struct reset_control	*core_reset;
	struct reset_control	*phy_reset;

	struct phy              *phy;

	u32			idx;
	bool			coherent;
	bool			auto_calibration;
};

#define PCIE_GLBL_INTERRUPT_0					0x00
#define   INT_RADM_INTA_ASSERTED				  0x00000001
#define   INT_RADM_INTA_DEASSERTED				  0x00000002
#define   INT_RADM_INTB_ASSERTED				  0x00000004
#define   INT_RADM_INTB_DEASSERTED				  0x00000008
#define   INT_RADM_INTC_ASSERTED				  0x00000010
#define   INT_RADM_INTC_DEASSERTED				  0x00000020
#define   INT_RADM_INTD_ASSERTED				  0x00000040
#define   INT_RADM_INTD_DEASSERTED				  0x00000080
#define   INT_MSI_CTR_INT					  0x00000100
#define   INT_SMLH_LINK_UP					  0x00000200
#define   INT_HP_INT						  0x00000400
#define   INT_RADM_CORRECTABLE_ERR				  0x00000800
#define   INT_RADM_NONFATAL_ERR					  0x00001000
#define   INT_RADM_FATAL_ERR					  0x00002000
#define   INT_RADM_PM_TO_ACK					  0x00004000
#define   INT_RADM_PM_PME					  0x00008000
#define   INT_RADM_QOVERFLOW					  0x00010000
#define   INT_LINK_DOWN						  0x00400000
#define   INT_CFG_AER_RC_ERR_MSI				  0x00800000
#define	  INT_CFG_PME_MSI					  0x01000000
#define	  INT_HP_PME						  0x02000000
#define   INT_HP_MSI						  0x04000000
#define   INT_CFG_UR_RESP					  0x08000000
#define PCIE_GLBL_INTERRUPT_ENABLE_0				0x04
#define PCIE_GLBL_INTERRUPT_1					0x08
#define PCIE_GLBL_INTERRUPT_ENABLE_1				0x0C
#define PCIE_GLBL_AXI_MASTER_RESP_MISC_INFO			0x10
#define PCIE_GLBL_AXI_MSTR_SLV_RESP_ERR_LOW_PW_MAP		0x14
#define PCIE_GLBL_CORE_CONFIG_REG				0x18
#define   PCIE_LTSSM_ENABLE					  BIT(0)
#define PCIE_GLBL_PM_INFO_RESET_VOLT_LOW_PWR_STATUS		0x1C
#define   PWR_STATUS_SMLH_LTSSM_STATE_OFFSET			  0x4
#define   PWR_STATUS_SMLH_LTSSM_STATE_MASK			  0x000003F0
#define   PWR_STATUS_RDLH_LINK_UP				  BIT(18)
#define PCIE_GLBL_RTLH_INFO					0x20
#define PCIE_GLBL_AXI_MASTER_WR_MISC_INFO			0x24
#define PCIE_GLBL_AXI_MASTER_RD_MISC_INFO			0x28
#define PCIE_GLBL_AXI_SLAVE_BRESP_MISC_INFO			0x2C
#define PCIE_GLBL_AXI_SLAVE_RD_RESP_MISC_INFO_COMP_TIMEOUT	0x30
#define PCIE_GLBL_CORE_DEBUG_0					0x34
#define PCIE_GLBL_CORE_DEBUG_1					0x38
#define PCIE_GLBL_CORE_DEBUG_E1					0x3C
#define PCIE_GLBL_PCIE_CONTR_CFG_START_ADDR			0x40
#define PCIE_GLBL_PCIE_CONTR_CFG_END_ADDR			0x44
#define PCIE_X1_GLBL_AXI_ATU_BYPASS				0x48
/*...*/
#define PCIE_GLBL_MSI_TARGET_ADDRESS_31_0_LSB			0x98
#define PCIE_GLBL_MSI_TARGET_ADDRESS_63_32_MSB			0x9C
/*...*/

/* ltssm definition */
#define LTSSM_DETECT_QUIET	0x00
#define LTSSM_DETECT_ACT	0x01
#define LTSSM_POLL_ACTIVE	0x02
#define LTSSM_POLL_COMPLIANCE	0x03
#define LTSSM_POLL_CONFIG	0x04
#define LTSSM_PRE_DETECT_QUIET	0x05
#define LTSSM_DETECT_WAIT	0x06
#define LTSSM_CFG_LINKWD_START	0x07
#define LTSSM_CFG_LINKWD_ACEPT	0x08
#define LTSSM_CFG_LANENUM_WAI	0x09
#define LTSSM_CFG_LANENUM_ACEPT	0x0A
#define LTSSM_CFG_COMPLETE	0x0B
#define LTSSM_CFG_IDLE		0x0C
#define LTSSM_RCVRY_LOCK	0x0D
#define LTSSM_RCVRY_SPEED	0x0E
#define LTSSM_RCVRY_RCVRCFG	0x0F
#define LTSSM_RCVRY_IDLE	0x10
#define LTSSM_L0		0x11
#define LTSSM_L0S		0x12
#define LTSSM_L123_SEND_EIDLE	0x13
#define LTSSM_L1_IDLE		0x14
#define LTSSM_L2_IDLE		0x15
#define LTSSM_L2_WAKE		0x16
#define LTSSM_DISABLED_ENTRY	0x17
#define LTSSM_DISABLED_IDLE	0x18
#define LTSSM_DISABLED		0x19
#define LTSSM_ENTRY		0x1A
#define LTSSM_LPBK_ACTIVE	0x1B
#define LTSSM_LPBK_EXIT		0x1C
#define LTSSM_LPBK_EXIT_TIMEOUT	0x1D
#define LTSSM_HOT_RESET_ENTRY	0X1E
#define LTSSM_HOT_RESET		0x1F
#define LTSSM_RCVRY_EQ0		0x20
#define LTSSM_RCVRY_EQ1		0x21
#define LTSSM_RCVRY_EQ2		0x22
#define LTSSM_RCVRY_EQ3		0x23

const char *ltssm_str[] = {
	"DETECT_QUIET",
	"DETECT_ACT",
	"POLL_ACTIVE",
	"POLL_COMPLIANCE",
	"POLL_CONFIG",
	"PRE_DETECT_QUIET",
	"DETECT_WAIT",
	"CFG_LINKWD_START",
	"CFG_LINKWD_ACEPT",
	"CFG_LANENUM_WAI",
	"CFG_LANENUM_ACEPT",
	"CFG_COMPLETE",
	"CFG_IDLE",
	"RCVRY_LOCK",
	"RCVRY_SPEED",
	"RCVRY_RCVRCFG",
	"RCVRY_IDLE",
	"L0",
	"L0S",
	"L123_SEND_EIDLE",
	"L1_IDLE",
	"L2_IDLE",
	"L2_WAKE",
	"DISABLED_ENTRY",
	"DISABLED_IDLE",
	"DISABLED",
	"LPBK_ENTRY",
	"LPBK_ACTIVE",
	"LPBK_EXIT",
	"LPBK_EXIT_TIMEOUT",
	"HOT_RESET_ENTRY",
	"HOT_RESET",
	"RCVRY_EQ0",
	"RCVRY_EQ1",
	"RCVRY_EQ2",
	"RCVRY_EQ3"
};

#define SERDES_PHY_INIT		0
#define SERDES_PHY_AUTO_CAL	1

/* pci-keystone.c - driver specific constants */
#define MAX_INTX_HOST_IRQS		4
#define MAX_MSI_HOST_IRQS		8

#define MAX_LINKUP_POLL_COUNT		500

static inline u32 ca77xx_pcie_readl(struct ca77xx_pcie *pcie, u32 reg);
static inline void ca77xx_pcie_writel(struct ca77xx_pcie *pcie, u32 val,
				      u32 reg);
static int ca77xx_pcie_ltssm(struct pcie_port *pp);

static void pcie_ace_add(int idx)
{
	struct atu_attr atu;
	struct ace_attr ace;
	char name[32];

	sprintf(name, "pcie%d_atu", idx);

	atu.in = 0;
	atu.out = atu.in + ACE_S2_ADDR;
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

static void pcie_ace_del(int idx)
{
	char name[32];

	sprintf(name, "pcie%d_atu", idx);

	ca77xx_ace_del(name, 0);
}

static ssize_t ca77xx_pcie_ace_enable_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct ca77xx_pcie *ca77xx_pcie = dev_get_drvdata(dev);
	struct atu_attr atu;
	struct ace_attr ace;
	char name[32];
	int enable;
	ssize_t ret = 0;

	if (ca77xx_pcie->coherent) {
		dev_info(dev, "ACE enable\n");

		sprintf(name, "pcie%d_atu", ca77xx_pcie->idx);

		ca77xx_ace_get(name, 0, &atu, &ace, &enable);
		dev_info(dev, "atu - in 0x%llx, out 0x%llx, size 0x%x\n",
			 atu.in, atu.out, atu.size);
		dev_info(dev, "axi - cache_en %d, arcache 0x%x, awcache 0x%x\n",
			 ace.cache_en, ace.arcache, ace.awcache);
		dev_info(dev, "axi - qos_en %d, arqos 0x%x, awqos 0x%x\n",
			 ace.qos_en, ace.arqos, ace.awqos);
		dev_info(dev, "axi - prot_en %d, arprot 0x%x, awprot 0x%x\n",
			 ace.prot_en, ace.arprot, ace.awprot);
		dev_info(dev, "axi - user_en %d\n", ace.user_en);
		dev_info(dev, "axi aruser - axuser %d, axdomain %d, axbar %d, axsnoop %d\n",
			 ace.aruser.bf.axuser, ace.aruser.bf.axdomain,
			 ace.aruser.bf.axbar, ace.aruser.bf.axsnoop);
		dev_info(dev, "axi awuser - axuser %d, axdomain %d, axbar %d, axsnoop %d\n",
			 ace.awuser.bf.axuser, ace.awuser.bf.axdomain,
			 ace.awuser.bf.axbar, ace.awuser.bf.axsnoop);
	} else {
		dev_info(dev, "ACE disable\n");
	}

	ret += sprintf(buf, "%d\n", ca77xx_pcie->coherent);

	return ret;
}
static DEVICE_ATTR(ace_enable, 0664, ca77xx_pcie_ace_enable_show, NULL);

static struct attribute *ca77xx_pcie_attributes[] = {
	&dev_attr_ace_enable.attr,
	NULL
};

static const struct attribute_group ca77xx_pcie_attr_group = {
	.attrs = ca77xx_pcie_attributes,
};

static int ca77xx_pcie_serdes_phy_init(struct ca77xx_pcie *ca77xx_pcie)
{
	if (1) {
		
		/* Gen1 */
		/* AN01 */
		
		writel(0xa812, ca77xx_pcie->serdes_base + 0x0004);
		/* AN04 (default 0x4000), bit[13] ssc_enable */
		writel(0x5000, ca77xx_pcie->serdes_base + 0x0010);
		/* AN06 */
		writel(0x001f, ca77xx_pcie->serdes_base + 0x0018);
		/* AN0a - EQ manual mode */
		writel(0x9670, ca77xx_pcie->serdes_base + 0x0028);
		/* AN0d */
		writel(0xef1a, ca77xx_pcie->serdes_base + 0x0034);
		/* AN1b */
		writel(0xaea1, ca77xx_pcie->serdes_base + 0x1b*4);
		
		/* AN20 */
		writel(0x94ac, ca77xx_pcie->serdes_base + 0x20*4);
		/* AN21 */
		writel(0x66aa, ca77xx_pcie->serdes_base + 0x21*4);
		/* AN22 */
		writel(0x53, ca77xx_pcie->serdes_base+ 0x22*4);
		/* AN2a */
		writel(0x1040, ca77xx_pcie->serdes_base + 0x2a*4);
		/* AN28 */
		writel(0xf802, ca77xx_pcie->serdes_base + 0x28*4);
		/* AN29 */
		writel(0xff04,ca77xx_pcie->serdes_base + 0x29*4);
		/* AN2f */
		writel(0x69bd, ca77xx_pcie->serdes_base + 0x2f*4);
		/* AN0b */
		writel(0x7905, ca77xx_pcie->serdes_base + 0x0b*4);
		/* AN09 */
		writel(0x520c, ca77xx_pcie->serdes_base+ 0x09*4);
		/* AN09 */
		writel(0x520c, ca77xx_pcie->serdes_base + 0x09*4);
		/* AN09 */
		writel(0x520c, ca77xx_pcie->serdes_base+ 0x09*4);
		/////////////////////////////////////////////
		//PHY GEN 2
		/* AN28 */
		writel(0xa819, ca77xx_pcie->serdes_base + 0x41*4);
		

		/* Gen2 */
		/* AN41 */
		writel(0xa819, ca77xx_pcie->serdes_base + 0x41*4);
		/* AN42 (default 0x4000), bit[13] ssc_enable */
		writel(0x6042, ca77xx_pcie->serdes_base+ 0x42*4);
		/* AN44 */
		writel(0x5000, ca77xx_pcie->serdes_base + 0x44*4);
		
		/* AN45 - EQ manual mode */
		writel(0x23da, ca77xx_pcie->serdes_base + 0x45*4);
		/* AN46 - lcp */
		writel(0x1f, ca77xx_pcie->serdes_base + 0x46*4);
		/* AN47 - cmu */
		writel(0x2e40, ca77xx_pcie->serdes_base + 0x47*4);
	

		/* AN4a */
		writel(0x9670, ca77xx_pcie->serdes_base + 0x4a*4);

		/* AN4b */
		writel(0x7905, ca77xx_pcie->serdes_base + 0x4b*4);

		/* AN4c */
		writel(0xc008, ca77xx_pcie->serdes_base + 0x4c*4);
		/* AN4e */
		writel(0x1000, ca77xx_pcie->serdes_base + 0x4e*4);

		/* AN4f */
		writel(0x0009, ca77xx_pcie->serdes_base + 0x4f*4);
		/* AN5b */
		writel(0xaea1, ca77xx_pcie->serdes_base + 0x5b*4);

		/* AN5e */
		writel(0x24eb, ca77xx_pcie->serdes_base + 0x5e*4);
		/* AN60 */
		writel(0x9488, ca77xx_pcie->serdes_base + 0x60*4);
		/* AN61 */
		writel(0x8877, ca77xx_pcie->serdes_base + 0x61*4);
		/* AN62 */
		writel(0x53, ca77xx_pcie->serdes_base + 0x62*4);
		/* AN63 */
		writel(0xaa26, ca77xx_pcie->serdes_base + 0x63*4);

		/* AN68 */
		writel(0xf802, ca77xx_pcie->serdes_base + 0x68*4);
		/* AN69 */
		writel(0xff04, ca77xx_pcie->serdes_base + 0x69*4);
		/* AN6a */
		writel(0x1040, ca77xx_pcie->serdes_base + 0x6a*4);
		/* AN6f */
		writel(0x65bd, ca77xx_pcie->serdes_base + 0x6f*4);
	
		/* AN49 - oobs */
		writel(0x520c, ca77xx_pcie->serdes_base + 0x49*4);
		/* AN49 - oobs */
		writel(0x500c, ca77xx_pcie->serdes_base + 0x49*4);
		/* AN49 - oobs */
		writel(0x520c, ca77xx_pcie->serdes_base + 0x49*4);
		/* AN61 */
		writel(0x8877, ca77xx_pcie->serdes_base + 0x61*4);
		/* AN62 */
		writel(0x53, ca77xx_pcie->serdes_base   + 0x62*4);
		/* AN63 */
		writel(0xaa26, ca77xx_pcie->serdes_base + 0x63*4);
		
		
	} 	
		return 0;
}

static int ca77xx_pcie_serdes_phy_power_on(struct ca77xx_pcie *ca77xx_pcie)
{
	u32 reg;

	dev_info(ca77xx_pcie->dev, "auto calibration ...\n");

	reg = readl(ca77xx_pcie->serdes_base + 0x01a0);
	reg &= ~GENMASK(10, 6);
	writel(reg, ca77xx_pcie->serdes_base + 0x01a0);

	reg = readl(ca77xx_pcie->serdes_base + 0x010c);
	reg &= ~GENMASK(15, 8);
	reg |= BIT(7);
	writel(reg, ca77xx_pcie->serdes_base + 0x010c);

	reg = readl(ca77xx_pcie->serdes_base + 0x0180);
	reg &= ~BIT(14);
	writel(reg, ca77xx_pcie->serdes_base + 0x0180);

	reg = readl(ca77xx_pcie->serdes_base + 0x0128);
	reg &= ~BIT(14);
	writel(reg, ca77xx_pcie->serdes_base + 0x0128);

	reg = readl(ca77xx_pcie->serdes_base + 0x0128);
	reg &= ~BIT(6);
	writel(reg, ca77xx_pcie->serdes_base + 0x0128);

	mdelay(1);

	reg = readl(ca77xx_pcie->serdes_base + 0x0128);
	reg |= BIT(6);
	writel(reg, ca77xx_pcie->serdes_base + 0x0128);

	if (!ca77xx_pcie->idx) {
		reg = readl(ca77xx_pcie->serdes_base + 0x11a0);
		reg &= ~GENMASK(10, 6);
		writel(reg, ca77xx_pcie->serdes_base + 0x11a0);

		reg = readl(ca77xx_pcie->serdes_base + 0x110c);
		reg &= ~GENMASK(15, 8);
		reg |= BIT(7);
		writel(reg, ca77xx_pcie->serdes_base + 0x110c);

		reg = readl(ca77xx_pcie->serdes_base + 0x1180);
		reg &= ~BIT(14);
		writel(reg, ca77xx_pcie->serdes_base + 0x1180);

		reg = readl(ca77xx_pcie->serdes_base + 0x1128);
		reg &= ~BIT(14);
		writel(reg, ca77xx_pcie->serdes_base + 0x1128);

		reg = readl(ca77xx_pcie->serdes_base + 0x1128);
		reg &= ~BIT(6);
		writel(reg, ca77xx_pcie->serdes_base + 0x1128);

		mdelay(1);

		reg = readl(ca77xx_pcie->serdes_base + 0x1128);
		reg |= BIT(6);
		writel(reg, ca77xx_pcie->serdes_base + 0x1128);
	}

	return 0;
}
static void setup_host_reg(struct ca77xx_pcie *pcie)
{
	u32 reg;
	reg = readl(RTL8198F_GLB_REG_ADDR(0xf4320020));
 	writel(reg | (1 << 9) | (1 << 7), RTL8198F_GLB_REG_ADDR(0xf4320020));
	writel(0x00000002, RTL8198F_GLB_REG_ADDR(0xf4320010));
	mdelay(1);
	writel(0x00000000, RTL8198F_GLB_REG_ADDR(0xf4320010));
	mdelay(1);

        reg = readl(RTL8198F_GLB_REG_ADDR(0xf432000c));	

	writel( (reg|(1<<7)), RTL8198F_GLB_REG_ADDR(0xf432000c));
	mdelay(1);
	writel(reg & (~(1<<7)), RTL8198F_GLB_REG_ADDR(0xf432000c));
	mdelay(1);
	reg = readl(RTL8198F_GLB_REG_ADDR(0xf43203ac));
	writel(reg | (1 << 9), RTL8198F_GLB_REG_ADDR(0xf43203ac));
	mdelay(1);
	reg = readl(RTL8198F_GLB_REG_ADDR(0xf4320014));
	writel(reg | (1 << 10), RTL8198F_GLB_REG_ADDR(0xf4320014));
	mdelay(1);
	writel(reg & ~(1 << 10), RTL8198F_GLB_REG_ADDR(0xf4320014));

	pr_info("\nset_reg().....\n");
	pr_info("\n");
}
static void dump_reg(struct ca77xx_pcie *pcie)
{
	u32 reg;
 	pr_info("\ndump_reg().....\n");

	reg = PCIE_GLBL_INTERRUPT_0;
	pr_info("\t0x%02X: PCIE_GLBL_INTERRUPT_0 = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_INTERRUPT_ENABLE_0;
	pr_info("\t0x%02X: PCIE_GLBL_INTERRUPT_ENABLE_0 = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_AXI_MASTER_RESP_MISC_INFO;
	pr_info("\t0x%02X: PCIE_GLBL_AXI_MASTER_RESP_MISC_INFO = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_AXI_MSTR_SLV_RESP_ERR_LOW_PW_MAP;
	pr_info("\t0x%02X: PCIE_GLBL_AXI_MSTR_SLV_RESP_ERR_LOW_PW_MAP = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_CORE_CONFIG_REG;
	pr_info("\t0x%02X: PCIE_GLBL_CORE_CONFIG_REG = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_PM_INFO_RESET_VOLT_LOW_PWR_STATUS;
	pr_info("\t0x%02X: PCIE_GLBL_PM_INFO_RESET_VOLT_LOW_PWR_STATUS = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_RTLH_INFO;
	pr_info("\t0x%02X: PCIE_GLBL_RTLH_INFO = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_AXI_MASTER_WR_MISC_INFO;
	pr_info("\t0x%02X: PCIE_GLBL_AXI_MASTER_WR_MISC_INFO = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_AXI_MASTER_RD_MISC_INFO;
	pr_info("\t0x%02X: PCIE_GLBL_AXI_MASTER_RD_MISC_INFO = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_AXI_SLAVE_BRESP_MISC_INFO;
	pr_info("\t0x%02X: PCIE_GLBL_AXI_SLAVE_BRESP_MISC_INFO = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_AXI_SLAVE_RD_RESP_MISC_INFO_COMP_TIMEOUT;
	pr_info("\t0x%02X: PCIE_GLBL_AXI_SLAVE_RD_RESP_MISC_INFO_COMP_TIMEOUT = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_PCIE_CONTR_CFG_START_ADDR;
	pr_info("\t0x%02X: PCIE_GLBL_PCIE_CONTR_CFG_START_ADDR = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_PCIE_CONTR_CFG_END_ADDR;
	pr_info("\t0x%02X: PCIE_GLBL_PCIE_CONTR_CFG_END_ADDR = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_X1_GLBL_AXI_ATU_BYPASS;
	pr_info("\t0x%02X: PCIE_X1_GLBL_AXI_ATU_BYPASS = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_MSI_TARGET_ADDRESS_31_0_LSB;
	pr_info("\t0x%02X: PCIE_GLBL_MSI_TARGET_ADDRESS_31_0_LSB = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));
	reg = PCIE_GLBL_AXI_MASTER_RESP_MISC_INFO;
	pr_info("\t0x%02X: PCIE_GLBL_MSI_TARGET_ADDRESS_63_32_MSB = 0x%x\n",
		reg, ca77xx_pcie_readl(pcie, reg));

	pr_info("\n");
}

static inline u32 ca77xx_pcie_readl(struct ca77xx_pcie *pcie, u32 reg)
{
	//printk("R:0x%x : %x" ,pcie->reg_base + reg,readl(pcie->reg_base + reg));
	return readl(pcie->reg_base + reg);
}

static inline void ca77xx_pcie_writel(struct ca77xx_pcie *pcie, u32 val,
				      u32 reg)
{
	//printk("reg: 0x%x\r\n",(pcie->reg_base + reg));
	writel(val, pcie->reg_base + reg);
}

#define GLOBAL_PHY_CTL 0xf4320020
#define GLOBAL_DPHY_RESET 0xf4320010
#define GLOBAL_BLOCK_RESET 0xf432000c
#define GLOBAL_IP_1_ENABLE 0xf43203ac
#define GLOBAL_PCIE_RST 0xf4320014
#define HV_GLOBAL_MASTER_OVERRIDE_SECURITY 0xf4320100
#define HV_GLOBAL_SECURITY_OVERRIDE 0xf43200fc
#define GLOBAL_ARM_DEBUG                         0xf43200ac
#define GLOBAL_SECURITY_OVERRIDE                 0xf43200fc
#define GLOBAL_MASTER_OVERRIDE_SECURITY          0xf4320100

static int ca77xx_pcie_host_reset(struct ca77xx_pcie *ca77xx_pcie,
				  int serdes_phase)
{

 
       struct pcie_port *pp = &ca77xx_pcie->pp;


	if (serdes_phase == SERDES_PHY_INIT)
	{
	
		ca77xx_pcie_serdes_phy_init(ca77xx_pcie);
		{
			 u32 reg;
			 
				reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_PCIE_RST));
				 writel( (reg | BIT(10)), RTL8198F_GLB_REG_ADDR(GLOBAL_PCIE_RST));
				 mdelay(10);
				 writel((reg & ~ BIT(10)) , RTL8198F_GLB_REG_ADDR(GLOBAL_PCIE_RST));
				 mdelay(10);
		}
		
	}
	else
	{
		ca77xx_pcie_serdes_phy_power_on(ca77xx_pcie);
	}
	phy_power_on(ca77xx_pcie->phy);
	msleep(1);
	return 0;
}

static int ca77xx_pcie_host_setup(struct ca77xx_pcie *ca77xx_pcie)
{
	struct pcie_port *pp = &ca77xx_pcie->pp;
	
	setup_host_reg(ca77xx_pcie);

	ca77xx_pcie_writel(ca77xx_pcie, ca77xx_pcie->dbi_start,
			   PCIE_GLBL_PCIE_CONTR_CFG_START_ADDR);
	ca77xx_pcie_writel(ca77xx_pcie, ca77xx_pcie->dbi_end,
			   PCIE_GLBL_PCIE_CONTR_CFG_END_ADDR);

	dw_pcie_setup_rc(pp);

	return 0;
}

static int ca77xx_pcie_device_reset(struct ca77xx_pcie *ca77xx_pcie)
{
	gpiod_set_value(ca77xx_pcie->dev_reset_gpio, 1);

	mdelay(1);

	gpiod_set_value(ca77xx_pcie->dev_reset_gpio, 0);

	return 0;
}

static int ca77xx_pcie_establish_link(struct ca77xx_pcie *ca77xx_pcie)
{
	struct pcie_port *pp = &ca77xx_pcie->pp;
	int count = 0;
	int ltssm;

	if (dw_pcie_link_up(pp)) {
		dev_err(pp->dev, "Link already up\n");
		return 0;
	}

	/* assert LTSSM enable */
	ca77xx_pcie_writel(ca77xx_pcie, PCIE_LTSSM_ENABLE,
			   PCIE_GLBL_CORE_CONFIG_REG);

	/* check if the link is up or not */
	while (!dw_pcie_link_up(pp)) {
		if (count == 0)
			dev_info(ca77xx_pcie->dev, "Linking ...\n");

		usleep_range(1000, 2000);
		count++;

		if (count > MAX_LINKUP_POLL_COUNT) {
			ltssm = ca77xx_pcie_ltssm(pp);
			dev_err(ca77xx_pcie->dev,
				"Link Fail(ltssm = 0x%x - %s)\n",
				ltssm, ltssm_str[ltssm]);
			return (ltssm == LTSSM_DETECT_QUIET ?
				-ENODEV : -EINVAL);
		}
	}
	if (pp->device_ready_time)
		msleep(pp->device_ready_time);
	if (dw_pcie_link_up(pp)) {
		dev_info(ca77xx_pcie->dev, "Link Up\n");
	} else {
		ltssm = ca77xx_pcie_ltssm(pp);
		dev_info(ca77xx_pcie->dev,
			 "Link Not Stable...Down(ltssm = 0x%x - %s)\n",
			 ltssm, ltssm_str[ltssm]);
		return -EAGAIN;
	}

	return 0;
}

static void ca77xx_pcie_misc_enable(struct ca77xx_pcie *ca77xx_pcie)
{
	u32 val;

	val = ca77xx_pcie_readl(ca77xx_pcie, PCIE_GLBL_INTERRUPT_ENABLE_0);

	val |= (
		INT_HP_INT |
		INT_RADM_CORRECTABLE_ERR |
		INT_RADM_NONFATAL_ERR |
		INT_RADM_FATAL_ERR |
		INT_RADM_PM_TO_ACK |
		INT_RADM_PM_PME |
		INT_RADM_QOVERFLOW |
		INT_LINK_DOWN |
		INT_CFG_AER_RC_ERR_MSI |
		INT_CFG_PME_MSI	|
		INT_HP_PME |
		INT_HP_MSI |
		INT_CFG_UR_RESP
		);

	ca77xx_pcie_writel(ca77xx_pcie, val, PCIE_GLBL_INTERRUPT_ENABLE_0);
}

static void ca77xx_pcie_intx_enable(struct ca77xx_pcie *ca77xx_pcie)
{
	u32 val;

	val = ca77xx_pcie_readl(ca77xx_pcie, PCIE_GLBL_INTERRUPT_ENABLE_0);

	val |= (INT_RADM_INTA_ASSERTED | INT_RADM_INTB_ASSERTED |
		INT_RADM_INTC_ASSERTED | INT_RADM_INTD_ASSERTED);

	ca77xx_pcie_writel(ca77xx_pcie, val, PCIE_GLBL_INTERRUPT_ENABLE_0);
}

/* g2-pcie.c */
static void ca77xx_pcie_intx_nop(struct irq_data *irqd)
{
}

static void ca77xx_pcie_mask_intx_irq(struct irq_data *irqd)
{
	struct ca77xx_pcie *ca77xx_pcie = irq_data_get_irq_chip_data(irqd);
	u32 val;

	val = ca77xx_pcie_readl(ca77xx_pcie, PCIE_GLBL_INTERRUPT_ENABLE_0);

	val &= ~(INT_RADM_INTA_ASSERTED | INT_RADM_INTB_ASSERTED |
		 INT_RADM_INTC_ASSERTED | INT_RADM_INTD_ASSERTED);

	ca77xx_pcie_writel(ca77xx_pcie, val, PCIE_GLBL_INTERRUPT_ENABLE_0);
}

static void ca77xx_pcie_unmask_intx_irq(struct irq_data *irqd)
{
	struct ca77xx_pcie *ca77xx_pcie = irq_data_get_irq_chip_data(irqd);

	ca77xx_pcie_intx_enable(ca77xx_pcie);
}

static struct irq_chip ca77xx_pcie_intx_irq_chip = {
	.name = "PCI-INTx",
	.irq_ack = ca77xx_pcie_intx_nop,
	.irq_enable = ca77xx_pcie_unmask_intx_irq,
	.irq_disable = ca77xx_pcie_mask_intx_irq,
	.irq_mask = ca77xx_pcie_mask_intx_irq,
	.irq_unmask = ca77xx_pcie_unmask_intx_irq,
};

/* g2-pcie.c */

/* pci-dra7xx.c */
static int ca77xx_pcie_intx_map(struct irq_domain *domain, unsigned int irq,
				irq_hw_number_t hwirq)
{
	irq_set_chip_and_handler(irq, &ca77xx_pcie_intx_irq_chip,
				 handle_simple_irq);
	irq_set_chip_data(irq, domain->host_data);

	return 0;
}

static const struct irq_domain_ops intx_domain_ops = {
	.map = ca77xx_pcie_intx_map,
};

static int ca77xx_pcie_intx_host_init(struct ca77xx_pcie *ca77xx_pcie,
				      struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct device_node *pcie_intc_node =  of_get_next_child(node, NULL);
	int ret;

	if (IS_ERR(pcie_intc_node)) {
		ret = PTR_ERR(pcie_intc_node);
		dev_err(dev, "No PCIe Intc node found(%d)\n", ret);
		return ret;
	}

	ca77xx_pcie->intx_irq_domain =
		irq_domain_add_linear(pcie_intc_node, MAX_INTX_HOST_IRQS,
				      &intx_domain_ops, ca77xx_pcie);
	if (IS_ERR(ca77xx_pcie->intx_irq_domain)) {
		ret = PTR_ERR(ca77xx_pcie->intx_irq_domain);
		dev_err(dev, "Failed to get a INTx IRQ domain(%d)\n", ret);
		return ret;
	}

	return 0;
}

/* pci-dra7xx.c */

static void ca77xx_pcie_msi_enable(struct ca77xx_pcie *ca77xx_pcie)
{
	u32 val;

	dw_pcie_msi_init(&ca77xx_pcie->pp);

	/* enable MSI interrupt */
	val = ca77xx_pcie_readl(ca77xx_pcie, PCIE_GLBL_INTERRUPT_ENABLE_0);
	val |= INT_MSI_CTR_INT;
	ca77xx_pcie_writel(ca77xx_pcie, val, PCIE_GLBL_INTERRUPT_ENABLE_0);
}

static irqreturn_t ca77xx_pcie_irq_handler(int irq, void *arg)
{
	struct ca77xx_pcie *ca77xx_pcie = arg;
	struct irq_domain *intx_irq_domain = ca77xx_pcie->intx_irq_domain;
	u32 val;

	val = ca77xx_pcie_readl(ca77xx_pcie, PCIE_GLBL_INTERRUPT_0);
	ca77xx_pcie_writel(ca77xx_pcie, val, PCIE_GLBL_INTERRUPT_0);

	if (val & INT_MSI_CTR_INT)
		dw_handle_msi_irq(&ca77xx_pcie->pp);
	else if (val & INT_RADM_INTA_ASSERTED)
		generic_handle_irq(irq_find_mapping(intx_irq_domain, 0));
	else if (val & INT_RADM_INTB_ASSERTED)
		generic_handle_irq(irq_find_mapping(intx_irq_domain, 1));
	else if (val & INT_RADM_INTC_ASSERTED)
		generic_handle_irq(irq_find_mapping(intx_irq_domain, 2));
	else if (val & INT_RADM_INTD_ASSERTED)
		generic_handle_irq(irq_find_mapping(intx_irq_domain, 3));

	/* others */
	if (val & (~(INT_MSI_CTR_INT |
		     INT_RADM_INTA_ASSERTED | INT_RADM_INTA_DEASSERTED |
		     INT_RADM_INTB_ASSERTED | INT_RADM_INTB_DEASSERTED |
		     INT_RADM_INTC_ASSERTED | INT_RADM_INTC_DEASSERTED |
		     INT_RADM_INTD_ASSERTED | INT_RADM_INTD_DEASSERTED))) {
		dev_info(ca77xx_pcie->dev, "%s: value = 0x%X\n", __func__, val);

		if (val & INT_LINK_DOWN) {
			int ltssm = ca77xx_pcie_ltssm(&ca77xx_pcie->pp);

			dev_info(ca77xx_pcie->dev,
				 "Link Down!!!(ltssm = 0x%x - %s)\n",
				 ltssm, ltssm_str[ltssm]);

			ca77xx_pcie->pp.link_down = true;
		}
	}

	return IRQ_HANDLED;
}

static void ca77xx_pcie_enable_interrupts(struct ca77xx_pcie *ca77xx_pcie)
{
	/* link up/down/mis interrupt */
	ca77xx_pcie_misc_enable(ca77xx_pcie);

	//if (IS_ENABLED(CONFIG_PCI_MSI))
	//	ca77xx_pcie_msi_enable(ca77xx_pcie);
	//else
		ca77xx_pcie_intx_enable(ca77xx_pcie);
}

static int ca77xx_pcie_link_up(struct pcie_port *pp)
{
	struct ca77xx_pcie *ca77xx_pcie = to_ca77xx_pcie(pp);
	u32 reg, val;

	reg = PCIE_GLBL_PM_INFO_RESET_VOLT_LOW_PWR_STATUS;
	val = ca77xx_pcie_readl(ca77xx_pcie, reg);
	val &= PWR_STATUS_RDLH_LINK_UP;

	return val ? 1 :  0;
}

static int ca77xx_pcie_ltssm(struct pcie_port *pp)
{
	struct ca77xx_pcie *ca77xx_pcie = to_ca77xx_pcie(pp);
	u32 reg, val;

	reg = PCIE_GLBL_PM_INFO_RESET_VOLT_LOW_PWR_STATUS;
	val = ca77xx_pcie_readl(ca77xx_pcie, reg);
	val = (val & PWR_STATUS_SMLH_LTSSM_STATE_MASK) >>
	       PWR_STATUS_SMLH_LTSSM_STATE_OFFSET;

	return val;
}

static int ca77xx_pcie_host_init(struct pcie_port *pp)
{
	struct ca77xx_pcie *ca77xx_pcie = to_ca77xx_pcie(pp);
	int try_link = 0;
	int ret;

TRY_LINK:
	ca77xx_pcie_host_reset(ca77xx_pcie, SERDES_PHY_INIT);

	ca77xx_pcie_host_setup(ca77xx_pcie);
	ca77xx_pcie_device_reset(ca77xx_pcie);
	ret = ca77xx_pcie_establish_link(ca77xx_pcie);

dev_err(pp->dev, "%s  1\n",__FUNCTION__);
	if (ret == -ENODEV)
		{
			dev_err(pp->dev, "%s  2\n",__FUNCTION__);
		return ret;
	}
	if (ca77xx_pcie->auto_calibration) {
		ca77xx_pcie_host_reset(ca77xx_pcie, SERDES_PHY_AUTO_CAL);
		ca77xx_pcie_host_setup(ca77xx_pcie);
		ca77xx_pcie_device_reset(ca77xx_pcie);
		ret = ca77xx_pcie_establish_link(ca77xx_pcie);
		if ((ret == -EAGAIN) && (try_link < 3)) {
			try_link++;
			goto TRY_LINK;
		}
	}
	if (ret)
{
	dev_err(pp->dev, "%s  3\n",__FUNCTION__);
		return -EIO;
}
	ca77xx_pcie_enable_interrupts(ca77xx_pcie);
}

static struct pcie_host_ops ca77xx_pcie_host_ops = {
	.link_up = ca77xx_pcie_link_up,
	.host_init = ca77xx_pcie_host_init,
};

static int __init ca77xx_add_pcie_port(struct ca77xx_pcie *ca77xx_pcie,
				       struct platform_device *pdev)
{
	struct pcie_port *pp = &ca77xx_pcie->pp;
	struct device *dev = &pdev->dev;
	struct resource *dbi_res;
	int ret;

	pp->dev = &pdev->dev;

	dbi_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rc_dbi");
	pp->dbi_base = devm_ioremap_resource(dev, dbi_res);
	if (IS_ERR(pp->dbi_base)) {
		ret = PTR_ERR(pp->dbi_base);
		return ret;
	}
	dev_info(dev, "resource - %pr mapped at 0x%pK\n", dbi_res,
		 pp->dbi_base);
	ca77xx_pcie->dbi_start = dbi_res->start;
	ca77xx_pcie->dbi_end = dbi_res->end;

	pp->irq = platform_get_irq(pdev, 0); /* one for INTx, MSI, and misc */
	if (pp->irq < 0)
		return pp->irq;
	dev_info(dev, "pcie irq is %d\n", pp->irq);
	/* IRQF_SHARED ? */
	ret = devm_request_irq(dev, pp->irq, ca77xx_pcie_irq_handler,
			       IRQF_SHARED | IRQF_NO_THREAD, "ca77xx-pcie",
			       ca77xx_pcie);
	if (ret) {
		dev_err(dev, "Failed to request irq(%d)\n", ret);
		return ret;
	}

	if (!IS_ENABLED(CONFIG_PCI_MSI)) {
		ret = ca77xx_pcie_intx_host_init(ca77xx_pcie, pdev);
		if (ret < 0)
			return ret;
	}

	pp->root_bus_nr = -1;
	pp->ops = &ca77xx_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "Failed to initialize host(%d)\n", ret);
		return ret;
	}

	return 0;
}


static int ca77xx_pcie_probe(struct platform_device *pdev)
{
	struct ca77xx_pcie *ca77xx_pcie;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct resource *reg_res;
	enum of_gpio_flags flags;
	int reset_gpio, ret;
	u32 reg;        

        reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_IP_1_ENABLE));
        writel((reg ) | BIT(9) , RTL8198F_GLB_REG_ADDR(GLOBAL_IP_1_ENABLE));
        {
	
				writel(0xfffffff7, RTL8198F_GLB_REG_ADDR(GLOBAL_SECURITY_OVERRIDE));
                		writel(0xffffffff, RTL8198F_GLB_REG_ADDR(GLOBAL_ARM_DEBUG));
				writel(0xf, RTL8198F_GLB_REG_ADDR(GLOBAL_MASTER_OVERRIDE_SECURITY));
			   


				// HV_GLOBAL_SECURITY_OVERRIDE
				//HAL_WRITE32_MEM(0xf43200fc, 0x0029853c);
				reg = readl(RTL8198F_GLB_REG_ADDR(HV_GLOBAL_SECURITY_OVERRIDE));
				 writel((reg & ~ BIT(7)) | 0xffffffff , RTL8198F_GLB_REG_ADDR(HV_GLOBAL_SECURITY_OVERRIDE));
			 
				reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_PHY_CTL));
				 writel((reg & ~ BIT(8)) | BIT(7) | BIT(9), RTL8198F_GLB_REG_ADDR(GLOBAL_PHY_CTL));
				 
				 writel(0x2, RTL8198F_GLB_REG_ADDR(GLOBAL_DPHY_RESET));
				 writel(0x0, RTL8198F_GLB_REG_ADDR(GLOBAL_DPHY_RESET));
				 
				 reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_BLOCK_RESET));
				 writel((reg ) | BIT(7) , RTL8198F_GLB_REG_ADDR(GLOBAL_BLOCK_RESET));
				 
				 writel((reg ) & ~(BIT(7)) , RTL8198F_GLB_REG_ADDR(GLOBAL_BLOCK_RESET));
				 
				 reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_IP_1_ENABLE));
				 writel((reg ) | BIT(9) , RTL8198F_GLB_REG_ADDR(GLOBAL_IP_1_ENABLE));
				  mdelay(20);
	}

	ca77xx_pcie = devm_kzalloc(dev, sizeof(*ca77xx_pcie), GFP_KERNEL);
	if (!ca77xx_pcie)
		return -ENOMEM;
	ca77xx_pcie->dev = dev;

#if 0
	ca77xx_pcie->clk = devm_clk_get(dev, "pcie");
	if (IS_ERR(ca77xx_pcie->clk)) {
		ret = PTR_ERR(ca77xx_pcie->clk);
		dev_err(dev, "Failed to get pcie rc clock(%d)\n", ret);
		return ret
	}
	ret = clk_prepare_enable(ca77xx_pcie->clk);
	if (ret)
		return ret;

	ca77xx_pcie->bus_clk = devm_clk_get(dev, "pcie_bus");
	if (IS_ERR(ca77xx_pcie->bus_clk)) {
		ret = PTR_ERR(ca77xx_pcie->bus_clk);
		dev_err(dev, "Failed to get pcie bus clock(%d)\n", ret);
		goto fail_clk;
	}
	ret = clk_prepare_enable(ca77xx_pcie->bus_clk);
	if (ret)
		goto fail_clk;
#endif

	reg_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					       "glbl_regs");
	ca77xx_pcie->reg_base = devm_ioremap_resource(dev, reg_res);
	if (IS_ERR(ca77xx_pcie->reg_base)) {
		ret = PTR_ERR(ca77xx_pcie->reg_base);
		goto fail_bus_clk;
	}
	dev_info(dev, "resource - %pr mapped at 0x%pK\n", reg_res,
		 ca77xx_pcie->reg_base);

	reg_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					       "serdes_phy");
	ca77xx_pcie->serdes_base = devm_ioremap_resource(dev, reg_res);
	if (IS_ERR(ca77xx_pcie->serdes_base)) {
		ret = PTR_ERR(ca77xx_pcie->serdes_base);
		goto fail_bus_clk;
	}
	dev_info(dev, "resource - %pr mapped at 0x%pK\n", reg_res,
		 ca77xx_pcie->serdes_base);

	if (of_property_read_bool(np, "auto-calibration"))
		ca77xx_pcie->auto_calibration = true;
	else
		ca77xx_pcie->auto_calibration = false;

	ca77xx_pcie->core_reset = of_reset_control_get(np, "core_reset");
	if (IS_ERR(ca77xx_pcie->core_reset)) {
		ret = PTR_ERR(ca77xx_pcie->core_reset);
		goto fail_bus_clk;
	}

	ca77xx_pcie->phy_reset = of_reset_control_get(np, "phy_reset");
	if (IS_ERR(ca77xx_pcie->phy_reset)) {
		ret = PTR_ERR(ca77xx_pcie->core_reset);
		goto fail_bus_clk;
	}

	ca77xx_pcie->phy = devm_phy_get(&pdev->dev, "pcie-phy");
	if (IS_ERR(ca77xx_pcie->phy)) {
		ret = PTR_ERR(ca77xx_pcie->phy);
		goto fail_bus_clk;
	}

	reset_gpio = of_get_named_gpio_flags(np, "reset-gpio", 0, &flags);
	if (reset_gpio == -EPROBE_DEFER) {
		ret = reset_gpio;
		goto fail_bus_clk;
	}
 




	if (gpio_is_valid(reset_gpio)) {
		unsigned long gpio_flags;

		if (flags & OF_GPIO_ACTIVE_LOW)
			gpio_flags = GPIOF_ACTIVE_LOW | GPIOF_OUT_INIT_LOW;
		else
			gpio_flags = GPIOF_OUT_INIT_HIGH;
		dev_info(dev, "reset-gpio %d is active %s\n", reset_gpio,
			 (flags & OF_GPIO_ACTIVE_LOW) ? "low" : "high");

		ret = devm_gpio_request_one(dev, reset_gpio, gpio_flags,
					    "reset-gpio");
		if (ret)
			goto fail_bus_clk;

		ca77xx_pcie->dev_reset_gpio = gpio_to_desc(reset_gpio);
	}

	if (of_property_read_u32(np, "id", &ca77xx_pcie->idx)) {
		dev_err(dev, "missing id property\n");
		goto fail_bus_clk;
	}
	dev_info(dev, "id %d\n", ca77xx_pcie->idx);





	ca77xx_pcie->coherent = of_dma_is_coherent(np);
	if (ca77xx_pcie->coherent) {
		dev_info(dev, "turn on ACE for dma coherent\n");
		pcie_ace_add(ca77xx_pcie->idx);
	}

	ret = sysfs_create_group(&ca77xx_pcie->dev->kobj,
				 &ca77xx_pcie_attr_group);
	if (ret) {
		dev_err(dev, "failed to register sysfs\n");
		return ret;
	}

	platform_set_drvdata(pdev, ca77xx_pcie);

	ret = ca77xx_add_pcie_port(ca77xx_pcie, pdev);
	if (ret < 0)
		goto fail_bus_clk;

	return 0;

fail_bus_clk:
#if 0
	clk_disable_unprepare(ca77xx_pcie->bus_clk);
fail_clk:
	clk_disable_unprepare(ca77xx_pcie->clk);
#endif
	return ret;
}

static int __exit ca77xx_pcie_remove(struct platform_device *pdev)
{
	struct ca77xx_pcie *ca77xx_pcie = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &ca77xx_pcie_attr_group);

	if (ca77xx_pcie->coherent)
		pcie_ace_del(ca77xx_pcie->idx);

	clk_disable_unprepare(ca77xx_pcie->bus_clk);
	clk_disable_unprepare(ca77xx_pcie->clk);

	return 0;
}

static const struct of_device_id ca77xx_pcie_of_match[] = {
	{ .compatible = "cortina,ca77xx-pcie"},
	{},
};
MODULE_DEVICE_TABLE(of, ca77xx_pcie_of_match);

static struct platform_driver ca77xx_pcie_driver = {
	.probe = ca77xx_pcie_probe,
	.remove		= __exit_p(ca77xx_pcie_remove),
	.driver = {
		.name	= "ca77xx-pcie",
		.of_match_table = ca77xx_pcie_of_match,
	},
};

static int __init ca77xx_pcie_init(void)
{
	return platform_driver_register(&ca77xx_pcie_driver);
}
late_initcall(ca77xx_pcie_init);

static void __exit ca77xx_pcie_exit(void)
{
	platform_driver_unregister(&ca77xx_pcie_driver);
}
module_exit(ca77xx_pcie_exit);

MODULE_DESCRIPTION("Cortina-Access CA77XX PCIe host controller driver");
MODULE_LICENSE("GPL v2");
