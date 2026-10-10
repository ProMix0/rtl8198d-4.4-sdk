/*
 * PHY driver for Cortina-Access CA77XX SoCs
 *
 * Copyright (C) 2016 Cortina Access, Inc.
 *		http://www.cortina-access.com
 *
 * Based on phy-bcm-cygnus-pcie.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <dt-bindings/phy/phy-ca77xx.h>
#include <soc/cortina/g3lite_registers.h>  

  
#define PHY_CONTROL		0x00
#define   S0_PCIE2_ISOLATE_0	  BIT(2)
#define   S0_POW_PCIE2_0	  BIT(3)
#define   S0_PCIE2_ISOLATE_1	  BIT(4)
#define   S0_POW_PCIE2_1	  BIT(5)
#define   S1_SATA_ISOLATE	  BIT(6)
#define   S1_POW_SATA		  BIT(7)
#define   S1_PCIE2_ISOLATE	  BIT(8)
#define   S1_POW_PCIE2		  BIT(9)
#define   S2_PCIE2_ISOLATE	  BIT(10)
#define   S2_POW_PCIE2		  BIT(12)
#define   S2_USB3_ISOLATE	  BIT(11)
#define   S2_POW_USB3		  BIT(13)
#define   S3_USB3_ISOLATE	  BIT(14)
#define   S3_SATA_SGMII_ISOLATE	  BIT(15)
#define   S3_POW_SATA		  BIT(16)
#define   S3_POW_USB3		  BIT(17)
#define   S1_COMBO_SEL		  BIT(22)
#define   S2_COMBO_SEL		  BIT(23)
#define   S3_COMBO_SEL		  BIT(24)
#define   S3_SATA_SGMII_SEL	  BIT(25)
#define   S0_RXAUI_MODE		  BIT(26)
#define   SATA_SEL_PHY		  BIT(27)
#define   S0_P_MDIO_ENABLE_REG	  BIT(28)
#define   S1_P_MDIO_ENABLE_REG	  BIT(29)
#define   S2_P_MDIO_ENABLE_REG	  BIT(30)
#define   S3_P_MDIO_ENABLE_REG	  BIT(31)
#define PHY_MISC_CONTROL	0x04
#define   S3_RX50_LINK            BIT(28)
#define   S1_RX50_LINK            BIT(27)
#define   S3_MBIAS_EN             BIT(11)
#define   S1_MBIAS_EN             BIT(1)
#define PHY_STATUS		0x08
#define   STATUS_USB2_CKUSABLE	  BIT(0)
#define   STATUS_S1_CKUSABLE	  BIT(1)
#define   STATUS_S2_CKUSABLE	  BIT(2)
#define   STATUS_S3_CKUSABLE	  BIT(3)
#define   STATUS_S0_CKUSABLE0	  BIT(4)
#define   STATUS_S0_CKUSABLE1	  BIT(5)
#define   STATUS_SATA0_PHYRDY	  BIT(6)
#define   STATUS_SATA1_PHYRDY	  BIT(7)
#define PHY_SGMII_MISC_CONTROL	0x0c
#define PHY_SGMII_PCS_CONTROL	0x10
#define PHY_SGMII_PCS_INFO	0x14
#define SGMII_PCS_INTERRUPT	0x18
#define SGMII_PCS_INTENABLE	0x1c

static int debug=1;
struct ca77xx_phy_core;

struct serdes_cfg {
	u32 addr;
	u32 val;
};

/**
 * struct ca77xx_phy
 * @core: pointer to the PHY core control
 * @id: internal ID to identify the PHY
 * @phy: pointer to the kernel PHY device
 */
struct ca77xx_phy {
	struct ca77xx_phy_core *core;
	struct phy *phy;
	char *dts_name;
	phys_addr_t serdes_addr, serdes_size;
	void __iomem *serdes_base;
	struct serdes_cfg *cfg;
	int cfg_cnt;
	int id;
};

/**
 * struct ca77xx_phy_core - PHY core control
 * @dev: pointer to device
 * @base: base register
 * @lock: mutex to protect access to individual PHYs
 * @phys: pointer to PHY device
 */
struct ca77xx_phy_core {
	struct device *dev;
	void __iomem *base;
	struct mutex lock;
	struct ca77xx_phy phys[PHY_NUMS];
	u32 flag;
};

#define PHY_FORCE_ON	BIT(1)

static int ca77xx_phy_serdes_config(struct ca77xx_phy *phy, bool enable)
{
	struct device *dev = phy->core->dev;
	u32 off, val;
	int i;
	
	if (!phy->serdes_addr || !phy->serdes_size)
		return -ENXIO;
	if (!phy->serdes_base) {
		if (!devm_request_mem_region(dev, phy->serdes_addr,
			phy->serdes_size, phy->dts_name)) {
			dev_err(dev, "can't request region\n");
			phy->serdes_base = NULL;
		} else {
			phy->serdes_base = devm_ioremap(dev, phy->serdes_addr,
							phy->serdes_size);
			if (!phy->serdes_base) {
				dev_err(dev, "ioremap failed for resource\n");
				devm_release_mem_region(dev, phy->serdes_addr,
							phy->serdes_size);
			}
		}

	if (!phy->serdes_base)
	{
			return -ENXIO;
	}
	}

	//if (!enable)jas
		//return 0;

	if (phy->cfg_cnt > 0 && phy->cfg) {
		for (i = 0; i < phy->cfg_cnt; i++) {
			off = (phys_addr_t)phy->cfg[i].addr - phy->serdes_addr;
			val = phy->cfg[i].val;

			if (off >= 0 && off < phy->serdes_size)
				writel(val, phy->serdes_base + off);
		}
dev_err(dev, "phy->5\n",phy->serdes_base);
		return 0;
	}

	switch (phy->id) {
	case PHY_S0_PCIE0:
		/* S0_0, 0xf433_3000 */
		/* Gen1 */
		/* AN01 */
		dev_err(dev, "7phy->serdes_base =%x\n",phy->serdes_base);
		writel(0xa812, phy->serdes_base + 0x0004);
		/* AN04 (default 0x4000), bit[13] ssc_enable */
		writel(0x5000, phy->serdes_base + 0x0010);
		/* AN06 */
		writel(0x001f, phy->serdes_base + 0x0018);
		/* AN0a - EQ manual mode */
		writel(0x9670, phy->serdes_base + 0x0028);
		/* AN0d */
		writel(0xef1a, phy->serdes_base + 0x0034);
		/* AN1b */
		writel(0xaea1, phy->serdes_base + 0x1b*4);
		
		/* AN20 */
		writel(0x94aa, phy->serdes_base + 0x20*4);
		/* AN21 */
		writel(0x66aa, phy->serdes_base + 0x21*4);
		/* AN22 */
		writel(0x53, phy->serdes_base + 0x22*4);
		/* AN2a */
		writel(0x1040, phy->serdes_base + 0x2a*4);
		/* AN28 */
		writel(0xf802, phy->serdes_base + 0x28*4);
		/* AN29 */
		writel(0xff04, phy->serdes_base + 0x29*4);
		/* AN2f */
		writel(0x65bd, phy->serdes_base + 0x2f*4);
		/* AN0b */
		writel(0x7905, phy->serdes_base + 0x0b*4);
		/* AN09 */
		writel(0x520c, phy->serdes_base + 0x09*4);
		/* AN09 */
		writel(0x520c, phy->serdes_base + 0x09*4);
		/* AN09 */
		writel(0x520c, phy->serdes_base + 0x09*4);
		/////////////////////////////////////////////
		//PHY GEN 2
		/* AN28 */
		writel(0xa819, phy->serdes_base + 0x41*4);
		

		/* Gen2 */
		/* AN41 */
		writel(0xa819, phy->serdes_base + 0x41*4);
		/* AN42 (default 0x4000), bit[13] ssc_enable */
		writel(0x6042, phy->serdes_base + 0x42*4);
		/* AN44 */
		writel(0x5000, phy->serdes_base + 0x44*4);
		
		/* AN45 - EQ manual mode */
		writel(0x23da, phy->serdes_base + 0x45*4);
		/* AN46 - lcp */
		writel(0x1f, phy->serdes_base + 0x46*4);
		/* AN47 - cmu */
		writel(0x2e40, phy->serdes_base + 0x47*4);
	

		/* AN4a */
		writel(0x9670, phy->serdes_base + 0x4a*4);

		/* AN4b */
		writel(0x7905, phy->serdes_base + 0x4b*4);

		/* AN4c */
		writel(0xc008, phy->serdes_base + 0x4c*4);
		/* AN4e */
		writel(0x1000, phy->serdes_base + 0x4e*4);

		/* AN4f */
		writel(0x0009, phy->serdes_base + 0x4f*4);
		/* AN5b */
		writel(0xaea1, phy->serdes_base + 0x5b*4);

		/* AN5e */
		writel(0x24eb, phy->serdes_base + 0x5e*4);
		/* AN60 */
		writel(0x9488, phy->serdes_base + 0x60*4);
		/* AN61 */
		writel(0x8877, phy->serdes_base + 0x61*4);
		/* AN62 */
		writel(0x53, phy->serdes_base + 0x62*4);
		/* AN63 */
		writel(0xaa26, phy->serdes_base + 0x63*4);

		/* AN68 */
		writel(0xf802, phy->serdes_base + 0x68*4);
		/* AN69 */
		writel(0xff04, phy->serdes_base + 0x69*4);
		/* AN6a */
		writel(0x1040, phy->serdes_base + 0x6a*4);
		/* AN6f */
		writel(0x65bd, phy->serdes_base + 0x6f*4);
			
		/* AN49 - oobs */
//		writel(0x520c, phy->serdes_base + 0x49*4);
		/* AN49 - oobs */
//		writel(0x500c, phy->serdes_base + 0x49*4);
		/* AN49 - oobs */
//		writel(0x520c, phy->serdes_base + 0x49*4);

		
	

		break;

	case PHY_S1_PCIE0:
		/* S1, 0xf433_5000 */
		/* Gen1 */
		/* AN01 */
		writel(0xa80a, phy->serdes_base + 0x0004);
		/* AN04 (default 0x7000), bit[13] ssc_enable */
		writel(0x5000, phy->serdes_base + 0x0010);
		/* AN06 */
		writel(0x001f, phy->serdes_base + 0x0018);
		/* AN0A - EQ manual mode */
		writel(0xd673, phy->serdes_base + 0x0028);
		/* page1, 00 */
		writel(0x9477, phy->serdes_base + 0x0080);
		/* page1, 04 */
		writel(0x080c, phy->serdes_base + 0x0090);
		/* page1, 0b */
		writel(0xa803, phy->serdes_base + 0x00ac);
		/* page1, 0f */
		writel(0x61bd, phy->serdes_base + 0x00bc);

		/* Gen2 */
		/* AN01 */
		writel(0xa80a, phy->serdes_base + 0x0104);
		/* AN04 (default 0x7000), bit[13] ssc_enable */
		writel(0x5000, phy->serdes_base + 0x0110);
		/* AN06 */
		writel(0x001f, phy->serdes_base + 0x0118);
		/* AN0A - EQ manual mode */
		writel(0xd673, phy->serdes_base + 0x0128);
		/* page1, 00 */
		writel(0x9477, phy->serdes_base + 0x0180);
		/* page1, 04 */
		writel(0x080c, phy->serdes_base + 0x0190);
		/* page1, 0b */
		writel(0xa803, phy->serdes_base + 0x01ac);
		/* page1, 0f */
		writel(0x61bd, phy->serdes_base + 0x01bc);

		break;

	case PHY_S2_PCIE0:
		/* S2, 0xf433_6000 */
		/* Gen1 */
		/* AN01 */
		writel(0xa80a, phy->serdes_base + 0x0004);
		/* AN04 (default 0x7000), bit[13] ssc_enable */
		writel(0x5000, phy->serdes_base + 0x0010);
		/* AN06 */
		writel(0x001f, phy->serdes_base + 0x0018);
		/* AN0A */
		writel(0xd653, phy->serdes_base + 0x0028);
		/* page1, 00 */
		writel(0x9477, phy->serdes_base + 0x0080);
		/* page1, 04 */
		writel(0x080c, phy->serdes_base + 0x0090);
		/* page1, 0b */
		writel(0xa803, phy->serdes_base + 0x00ac);
		/* page1, 0b */
		writel(0x61bd, phy->serdes_base + 0x00bc);

		/* Gen2 */
		/* AN01 */
		writel(0xa80a, phy->serdes_base + 0x0104);
		/* AN04 (default 0x7000), bit[13] ssc_enable */
		writel(0x5000, phy->serdes_base + 0x0110);
		/* AN06 */
		writel(0x001f, phy->serdes_base + 0x0118);
		/* AN0A */
		writel(0xd653, phy->serdes_base + 0x0128);
		/* page1, 00 */
		writel(0x9477, phy->serdes_base + 0x0180);
		/* page1, 04 */
		writel(0x080c, phy->serdes_base + 0x0190);
		/* page1, 0b */
		writel(0xa803, phy->serdes_base + 0x01ac);
		/* page1, 0b */
		writel(0x61bd, phy->serdes_base + 0x01bc);

		break;

	case PHY_S2_USB:
		/* USB phy parameters migrate to phy_ca77xx_usb3.c */
		break;
	case PHY_S3_USB:
		/* USB phy parameters migrate to phy_ca77xx_usb3.c */
		break;

	case PHY_S3_SGMII:
		writel(0x5359, phy->serdes_base + 0x001c);
		break;
	case PHY_S1_SATA:
	case PHY_S3_SATA:
		writel(0x0000e070, phy->serdes_base + 0x004);
		writel(0x0000e050, phy->serdes_base + 0x104);
		writel(0x0000e047, phy->serdes_base + 0x204);
		writel(0x00007000, phy->serdes_base + 0x008);
		writel(0x00007004, phy->serdes_base + 0x108);
		writel(0x00007004, phy->serdes_base + 0x208);
		writel(0x00002774, phy->serdes_base + 0x00c);
		writel(0x0000276d, phy->serdes_base + 0x10c);
		writel(0x0000276b, phy->serdes_base + 0x20c);
		writel(0x00005000, phy->serdes_base + 0x010);
		writel(0x00005000, phy->serdes_base + 0x110);
		writel(0x00005000, phy->serdes_base + 0x210);
		writel(0x000033b2, phy->serdes_base + 0x014);
		writel(0x000033b2, phy->serdes_base + 0x114);
		writel(0x000033b2, phy->serdes_base + 0x214);
		writel(0x00000002, phy->serdes_base + 0x018);
		writel(0x00000002, phy->serdes_base + 0x118);
		writel(0x00000002, phy->serdes_base + 0x218);
		writel(0x0000a660, phy->serdes_base + 0x028);
		writel(0x0000a660, phy->serdes_base + 0x128);
		writel(0x0000a660, phy->serdes_base + 0x228);
		writel(0x00002900, phy->serdes_base + 0x040);
		writel(0x00002900, phy->serdes_base + 0x140);
		writel(0x00002900, phy->serdes_base + 0x240);
		writel(0x00000000, phy->serdes_base + 0x044);
		writel(0x00000000, phy->serdes_base + 0x144);
		writel(0x00000000, phy->serdes_base + 0x244);
		writel(0x00000027, phy->serdes_base + 0x05c);
		writel(0x00000027, phy->serdes_base + 0x15c);
		writel(0x00000027, phy->serdes_base + 0x25c);
		writel(0x00002004, phy->serdes_base + 0x064);
		writel(0x00002804, phy->serdes_base + 0x164);
		writel(0x00001804, phy->serdes_base + 0x264);
		writel(0x000094ba, phy->serdes_base + 0x080);
		writel(0x000094ba, phy->serdes_base + 0x180);
		writel(0x000094ba, phy->serdes_base + 0x280);
		writel(0x00000000, phy->serdes_base + 0x084);
		writel(0x0000a80a, phy->serdes_base + 0x184);
		writel(0x0000a80a, phy->serdes_base + 0x284);
		break;
	}

	return 0;
}

static int ca77xx_phy_sgmii_config(struct ca77xx_phy *phy, bool enable)
{
	struct ca77xx_phy_core *core = phy->core;

	if (!enable)
		return 0;

	switch (phy->id) {
	case PHY_S3_SGMII:
		writel(0x00000030, core->base + PHY_SGMII_MISC_CONTROL);
		writel(0x03600248, core->base + PHY_SGMII_PCS_CONTROL);
		break;
	}

	return 0;
}

static int ca77xx_sata_config(struct ca77xx_phy *phy, bool enable)
{
	struct ca77xx_phy_core *core = phy->core;
	u32 val;

	if (!enable)
		return 0;

	switch (phy->id) {
	case PHY_S1_SATA:
		val = readl(core->base + PHY_MISC_CONTROL);
		val |= S1_MBIAS_EN | S1_RX50_LINK;
		val &= ~(S3_MBIAS_EN | S3_RX50_LINK);
		writel(val, core->base + PHY_MISC_CONTROL);
		break;
	case PHY_S3_SATA:
		val = readl(core->base + PHY_MISC_CONTROL);
		val |= S3_MBIAS_EN | S3_RX50_LINK;
		val &= ~(S1_MBIAS_EN | S1_RX50_LINK);
		writel(val, core->base + PHY_MISC_CONTROL);
		break;
	}
	return 0;
}
#define GLOBAL_PHY_CTL 0xf4320020
#define GLOBAL_DPHY_RESET 0xf4320010
#define GLOBAL_BLOCK_RESET 0xf432000c
#define GLOBAL_IP_1_ENABLE 0xf43203ac
static int ca77xx_phy_power_config(struct ca77xx_phy *phy, bool enable)
{
	struct ca77xx_phy_core *core = phy->core;
	u32 val, enable_set_mask, enable_clr_mask, ckusable_mask;
	if(phy->id == PHY_S2_PCIE0)
	{
			 u32 reg;
			 
				reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_PHY_CTL));
				dev_info(core->dev, "%s -reg 0x%x write val 0x%X\n", RTL8198F_GLB_REG_ADDR(GLOBAL_PHY_CTL),__func__, reg);
				 writel((reg & ~ BIT(8)) | BIT(7) | BIT(9), RTL8198F_GLB_REG_ADDR(GLOBAL_PHY_CTL));
				 
				 writel(0x2, RTL8198F_GLB_REG_ADDR(GLOBAL_DPHY_RESET));
				 writel(0x0, RTL8198F_GLB_REG_ADDR(GLOBAL_DPHY_RESET));
				 
				 reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_BLOCK_RESET));
				dev_info(core->dev, "%s -reg 0x%x write val 0x%X\n", RTL8198F_GLB_REG_ADDR(GLOBAL_BLOCK_RESET),__func__, reg);
				 writel((reg ) | BIT(7) , RTL8198F_GLB_REG_ADDR(GLOBAL_BLOCK_RESET));
				 reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_PHY_CTL));
				 writel((reg ) & ~(BIT(7)) , RTL8198F_GLB_REG_ADDR(GLOBAL_PHY_CTL));
				 
				 reg = readl(RTL8198F_GLB_REG_ADDR(GLOBAL_IP_1_ENABLE));
				dev_info(core->dev, "%s -reg 0x%x write val 0x%X\n", RTL8198F_GLB_REG_ADDR(GLOBAL_IP_1_ENABLE),__func__, reg);
				 writel((reg ) | BIT(9) , RTL8198F_GLB_REG_ADDR(GLOBAL_IP_1_ENABLE));
				 
				  mdelay(20);
			return 0;
	}

	switch (phy->id) {
	case PHY_S0_PCIE0:
		enable_set_mask = S0_POW_PCIE2_0 | S0_POW_PCIE2_1;
		enable_clr_mask = S0_PCIE2_ISOLATE_0 | S0_PCIE2_ISOLATE_1;
		ckusable_mask = STATUS_S0_CKUSABLE0 | STATUS_S0_CKUSABLE1;
		break;
	case PHY_S1_PCIE0:
		enable_set_mask = S1_POW_PCIE2;
		enable_clr_mask = S1_PCIE2_ISOLATE;
		ckusable_mask = STATUS_S1_CKUSABLE;
		break;
	case PHY_S2_PCIE0:
		enable_set_mask = S2_POW_PCIE2;
		enable_clr_mask = S2_PCIE2_ISOLATE;
		ckusable_mask = STATUS_S2_CKUSABLE;
		break;
	case PHY_S2_USB:
		enable_set_mask = S2_POW_USB3 | S2_COMBO_SEL;
		enable_clr_mask = S2_USB3_ISOLATE;
		ckusable_mask = STATUS_S2_CKUSABLE;
		break;
	case PHY_S3_USB:
		enable_set_mask = S3_POW_USB3 | S3_COMBO_SEL;
		enable_clr_mask = S3_USB3_ISOLATE;
		ckusable_mask = STATUS_S3_CKUSABLE;
		break;
	case PHY_S3_SGMII:
		enable_set_mask = S3_POW_SATA;
		enable_clr_mask = S3_SATA_SGMII_ISOLATE | S3_SATA_SGMII_SEL;
		break;

	case PHY_S1_SATA:
		enable_set_mask = S1_COMBO_SEL;
		enable_clr_mask = SATA_SEL_PHY;
		break;

	case PHY_S3_SATA:
		enable_set_mask = S3_SATA_SGMII_SEL | SATA_SEL_PHY;
		enable_clr_mask = S3_COMBO_SEL;
		break;
	default:
		dev_err(core->dev, "PHY %d invalid\n", phy->id);
		return -EINVAL;
	}

	mutex_lock(&core->lock);

	val = readl(core->base + PHY_CONTROL);
	if (debug)
		dev_info(core->dev, "%s - read val 0x%X\n", __func__, val);
	if (enable) {
		val |= enable_set_mask;
		val &= ~enable_clr_mask;
	} else {
		val &= ~enable_set_mask;
		val |= enable_clr_mask;
	}
	if (core->flag & PHY_FORCE_ON)
		val |= 0x3322b;
	if (debug)
		dev_info(core->dev, "%s - write val 0x%X\n", __func__, val);
	writel(val, core->base + PHY_CONTROL);
	mutex_unlock(&core->lock);
	dev_dbg(core->dev, "PHY %d %s\n", phy->id,
		enable ? "enabled" : "disabled");
		
#if 0
	ca77xx_sata_config(phy, enable);
	ca77xx_phy_serdes_config(phy, enable);

	ca77xx_phy_sgmii_config(phy, enable);
#endif
	return 0;
}

static int ca77xx_phy_power_on(struct phy *p)
{
	struct ca77xx_phy *phy = phy_get_drvdata(p);

	if (debug)
		dev_info(phy->core->dev, "Request to power_on phy(%d)\n",
			 phy->id);

	return ca77xx_phy_power_config(phy, true);
}

static int ca77xx_phy_power_off(struct phy *p)
{
	struct ca77xx_phy *phy = phy_get_drvdata(p);

	if (debug)
		dev_info(phy->core->dev, "Request to power_off phy(%d)\n",
			 phy->id);

	return ca77xx_phy_power_config(phy, false);
}

static struct phy_ops ca77xx_phy_ops = {
	.init = NULL,
	.exit = NULL,
	.power_on = ca77xx_phy_power_on,
	.power_off = ca77xx_phy_power_off,
	.owner = THIS_MODULE,
};

static void ca77xx_serdes_probe(struct device *dev, struct device_node *np,
				struct ca77xx_phy *p)
{
	phys_addr_t serdes[2];
	int ret, size, cnt;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
	ret = of_property_read_u64_array(np, "serdes-reg", serdes, 2);
#else
	ret = of_property_read_u32_array(np, "serdes-reg", serdes, 2);
#endif
	if (ret) {
		p->dts_name = NULL;
		p->serdes_addr = 0;
		p->serdes_size = 0;
		p->serdes_base = NULL;

		return;
	}

	p->dts_name = np->name;
	p->serdes_addr = serdes[0];
	p->serdes_size = serdes[1];
		p->serdes_base = NULL;

	size = sizeof(struct serdes_cfg);
	p->cfg_cnt = of_property_count_elems_of_size(np, "serdes-cfg", size);
	if (p->cfg_cnt > 0) {
		p->cfg = devm_kmalloc_array(dev, p->cfg_cnt, size, GFP_KERNEL);

		cnt = p->cfg_cnt * sizeof(struct serdes_cfg) / sizeof(u32);
		of_property_read_u32_array(np, "serdes-cfg", (u32 *)p->cfg,
					   cnt);
	} else {
		p->cfg_cnt = 0;
		p->cfg = NULL;
	}
}

static int ca77xx_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node, *child;
	struct ca77xx_phy_core *core;
	struct phy_provider *provider;
	struct resource *res;
	unsigned cnt = 0;
	int ret;

	if (of_get_child_count(node) == 0) {
		dev_err(dev, "PHY no child node\n");
		return -ENODEV;
	}

	core = devm_kzalloc(dev, sizeof(*core), GFP_KERNEL);
	if (!core)
		return -ENOMEM;

	core->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	core->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(core->base))
		return PTR_ERR(core->base);
	dev_info(dev, "resource - %pr mapped at 0x%pK\n", res, core->base);

	if (of_property_read_bool(node, "forced-on"))
		core->flag |= PHY_FORCE_ON;

	mutex_init(&core->lock);

	for_each_available_child_of_node(node, child) {
		unsigned int id;
		struct ca77xx_phy *p;

		if (of_property_read_u32(child, "reg", &id)) {
			dev_err(dev, "missing reg property for %s\n",
				child->name);
			ret = -EINVAL;
			goto put_child;
		}

		if (id >= PHY_NUMS) {
			dev_err(dev, "invalid PHY id: %u\n", id);
			ret = -EINVAL;
			goto put_child;
		}

		if (core->phys[id].phy) {
			dev_err(dev, "duplicated PHY id: %u\n", id);
			ret = -EINVAL;
			goto put_child;
		}

		p = &core->phys[id];
		p->phy = devm_phy_create(dev, child, &ca77xx_phy_ops);
		if (IS_ERR(p->phy)) {
			dev_err(dev, "failed to create PHY\n");
			ret = PTR_ERR(p->phy);
			goto put_child;
		}

		p->core = core;
		p->id = id;

		ca77xx_serdes_probe(dev, child, p);

		phy_set_drvdata(p->phy, p);
		cnt++;
	}

	dev_set_drvdata(dev, core);

	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(provider)) {
		dev_err(dev, "failed to register PHY provider\n");
		return PTR_ERR(provider);
	}
	dev_dbg(dev, "registered %u PHY(s)\n", cnt);

	return 0;
put_child:
	of_node_put(child);
	return ret;
}

static const struct of_device_id ca77xx_phy_match_table[] = {
	{ .compatible = "cortina,ca77xx-phy" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ca77xx_phy_match_table);

static struct platform_driver ca77xx_phy_driver = {
	.driver = {
		.name = "ca77xx-phy",
		.of_match_table = ca77xx_phy_match_table,
	},
	.probe = ca77xx_phy_probe,
};
module_platform_driver(ca77xx_phy_driver);

MODULE_DESCRIPTION("Cortina-Access CA77XX PHY driver");
MODULE_LICENSE("GPL v2");
