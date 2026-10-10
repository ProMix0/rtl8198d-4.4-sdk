/*
 * MDIO bus controller driver for Cortina-Access SoCs
 *
 * Copyright (C) 2017 Cortina Access, Inc.
 *		 http://www.cortina-access.com
 *
 * Based on hnx_mdio.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/netdevice.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/spinlock_types.h>
#include <soc/cortina/registers.h>

#define MDIO_DRV_NAME "CA_MDIO"
#define MDIO_BUS_NAME "CA MII Bus"
#define MDIO_DRV_VERSION "1.0.0"
#define MDIO_COPYRIGHT "Copyright(c) 2017 Cortina-Access Corporation."
#define MDIO_DRV_STRING MDIO_BUS_NAME
#define MDIO_DEFAULT_DEVICE_DESCR MDIO_BUS_NAME

#define MDIO_TIMEOUT			1000000

#define __MDIO_CLOCK_MIN     (1)
#define __MDIO_CLOCK_MAX     (20000)
#define __MDIO_CLOCK_DEF     (2000)

struct ca_mdio_device {
	void *vbase;		/* mdio reg base address */
	unsigned int peri_clock;
};

struct ca_mdio_device *mdio_device_priv;


enum mdio_st_clause {
	MDIO_ST_CLAUSE_45 = 0,
	MDIO_ST_CLAUSE_22
};

enum mdio_c22_op_seq {
	MDIO_C22_WRITE = 1,
	MDIO_C22_READ = 2
};

enum mdio_c45_op_seq {
	MDIO_C45_WRITE_ADDR = 0,
	MDIO_C45_WRITE_DATA,
	MDIO_C45_READ_INCREMENT,
	MDIO_C45_READ
};

enum mdio_io_direction {
	MDIO_WRITE = 0,
	MDIO_READ = 1
};

#define mdio_readl(a)	readl(mdio_device_priv->vbase + (a - PER_MDIO_CFG))
#define mdio_writel(v, a)	writel(v, mdio_device_priv->vbase + (a - PER_MDIO_CFG))

static int ca_mdio_wait_ready(struct mii_bus *bus)
{
	PER_MDIO_CTRL_t mdio_ctrl;
	int i;

	/* waitting for PER_MDIO_CTRL's mdiostart==0 */
	/* after that can do read or write */
	for (i = 0; ; i++) {
		mdio_ctrl.wrd = mdio_readl(PER_MDIO_CTRL);
		if(mdio_ctrl.bf.mdiostart == 0)
			break;
		if (i == MDIO_TIMEOUT)
			return -ETIMEDOUT;
	}

	/* Clear done bit */
	mdio_ctrl.bf.mdiodone = 1;
	mdio_writel(mdio_ctrl.wrd, PER_MDIO_CTRL);

	return 0;
}

static void ca_mdio_cmd_write(struct ca_mdio_device *mdio_dev,
			       u8 is_c45, u8 op, u8 phy_id, u16 cmd)
{
	PER_MDIO_ADDR_t mdio_addr;
	PER_MDIO_CTRL_t mdio_ctrl;

	mdio_addr.bf.mdio_st = is_c45 ? 1 : 0;

	mdio_addr.bf.mdio_addr = phy_id;
	mdio_addr.bf.mdio_offset = cmd;
	mdio_addr.bf.mdio_rd_wr = MDIO_WRITE;
	mdio_addr.bf.mdio_op = is_c45 ? op : 0;

	mdio_writel(mdio_addr.wrd, PER_MDIO_ADDR);
	mdio_ctrl.wrd = 0;
	mdio_ctrl.bf.mdiostart = 1;
	mdio_writel(mdio_ctrl.wrd, PER_MDIO_CTRL);
}

static void ca_mdio_cmd_read(struct ca_mdio_device *mdio_dev,
			       u8 is_c45, u8 op, u8 phy_id, u16 cmd)
{
	PER_MDIO_ADDR_t mdio_addr;
	PER_MDIO_CTRL_t mdio_ctrl;

	mdio_addr.bf.mdio_st = is_c45 ? 1 : 0;

	mdio_addr.bf.mdio_addr = phy_id;
	mdio_addr.bf.mdio_offset = cmd;
	mdio_addr.bf.mdio_rd_wr = MDIO_READ;
	mdio_addr.bf.mdio_op = is_c45 ? op : 0;

	mdio_writel(mdio_addr.wrd, PER_MDIO_ADDR);
	mdio_ctrl.wrd = 0;
	mdio_ctrl.bf.mdiostart = 1;
	mdio_writel(mdio_ctrl.wrd, PER_MDIO_CTRL);
}

/**
 * ca_mdio_write - access phy register
 * @bus: mdio bus
 * @phy_id: phy id
 * @regnum: register num
 * @value: register value
 *
 * Return 0 on success, negative on failure
 */
static int ca_mdio_write(struct mii_bus *bus,
			  int phy_id, int regnum, u16 data)
{
	int ret;
	struct ca_mdio_device *mdio_dev = (struct ca_mdio_device *)bus->priv;
	u8 devad = ((regnum >> 16) & 0x1f);
	u8 is_c45 = !!(regnum & MII_ADDR_C45);
	u16 reg = (u16)(regnum & 0xffff);
	u8 op;
	u16 cmd_reg_cfg;

	dev_dbg(&bus->dev, "mdio write %s,base is %p\n",
		bus->id, mdio_dev->vbase);
	dev_dbg(&bus->dev, "phy id=%d, is_c45=%d, devad=%d, reg=%#x, write data=%d\n",
		phy_id, is_c45, devad, reg, data);

	/* wait for ready */
	ret = ca_mdio_wait_ready(bus);
	if (ret) {
		dev_err(&bus->dev, "MDIO bus is busy\n");
		return ret;
	}

	if (!is_c45) {
		cmd_reg_cfg = reg;
		op = MDIO_C22_WRITE;
		mdio_writel(data, PER_MDIO_WRDATA);
	} else {

		mdio_writel(reg, PER_MDIO_WRDATA);

		/* config the cmd-reg to write addr*/
		ca_mdio_cmd_write(mdio_dev, is_c45,
				   MDIO_C45_WRITE_ADDR, phy_id, devad);

		/* check for read or write opt is finished */
		ret = ca_mdio_wait_ready(bus);
		if (ret) {
			dev_err(&bus->dev, "MDIO bus is busy\n");
			return ret;
		}

		/* config the data needed writing */
		cmd_reg_cfg = devad;
		op = MDIO_C45_WRITE_DATA;

		mdio_writel(data, PER_MDIO_WRDATA);
	}

	ca_mdio_cmd_write(mdio_dev, is_c45, op, phy_id, cmd_reg_cfg);

	return 0;
}

/**
 * ca_mdio_read - access phy register
 * @bus: mdio bus
 * @phy_id: phy id
 * @regnum: register num
 * @value: register value
 *
 * Return phy register value
 */
static int ca_mdio_read(struct mii_bus *bus, int phy_id, int regnum)
{
	int ret;
	u16 reg_val = 0;
	u8 devad = ((regnum >> 16) & 0x1f);
	u8 is_c45 = !!(regnum & MII_ADDR_C45);
	u16 reg = (u16)(regnum & 0xffff);
	struct ca_mdio_device *mdio_dev = (struct ca_mdio_device *)bus->priv;

	dev_dbg(&bus->dev, "mdio read %s,base is %p\n",
		bus->id, mdio_dev->vbase);
	dev_dbg(&bus->dev, "phy id=%d, is_c45=%d, devad=%d, reg=%#x!\n",
		phy_id, is_c45, devad, reg);

	/* Step 1: wait for ready */
	ret = ca_mdio_wait_ready(bus);
	if (ret) {
		dev_err(&bus->dev, "MDIO bus is busy\n");
		return ret;
	}

	if (!is_c45) {
		ca_mdio_cmd_read(mdio_dev, is_c45,
				   MDIO_C22_READ, phy_id, reg);
	} else {
		mdio_writel(reg, PER_MDIO_WRDATA);

		/* Step 2; config the cmd-reg to write addr*/
		ca_mdio_cmd_write(mdio_dev, is_c45,
				   MDIO_C45_WRITE_ADDR, phy_id, devad);

		/* Step 3: check for read or write opt is finished */
		ret = ca_mdio_wait_ready(bus);
		if (ret) {
			dev_err(&bus->dev, "MDIO bus is busy\n");
			return ret;
		}

		ca_mdio_cmd_read(mdio_dev, is_c45,
				   MDIO_C45_READ, phy_id, devad);
	}

	/* Step 5: waitting for MDIO_COMMAND_REG 's mdiostart==0,*/
	/* check for read or write opt is finished */
	ret = ca_mdio_wait_ready(bus);
	if (ret) {
		dev_err(&bus->dev, "MDIO bus is busy\n");
		return ret;
	}

	/* Step 6; get out data*/
	reg_val = (u16)mdio_readl(PER_MDIO_RDDATA);

	return reg_val;
}

/**
 * ca_mdio_bus_name - get mdio bus name
 * @name: mdio bus name
 * @np: mdio device node pointer
 */
static void ca_mdio_bus_name(char *name, struct device_node *np)
{
	const u32 *addr;
	u64 taddr = OF_BAD_ADDR;

	addr = of_get_address(np, 0, NULL, NULL);
	if (addr)
		taddr = of_translate_address(np, addr);

	snprintf(name, MII_BUS_ID_SIZE, "%s@%llx", np->name,
		 (unsigned long long)taddr);
}

static int ca_mdio_speed_set(uint32_t peri_clock, uint32_t clock)
{
    uint16_t    prer = 0;
    PER_MDIO_CFG_t cfg;

    if (clock < __MDIO_CLOCK_MIN ||
        clock > __MDIO_CLOCK_MAX ) {
        return -1;
    }

    /* Caculate pre scale for MDIO clock adjust */
    prer = ( peri_clock / (clock << 1)) - 1;
    if (( peri_clock % (clock << 1)) > clock) {
        prer += 1;
    }

    cfg.bf.mdio_pre_scale = prer;
    cfg.bf.mdio_manual    = 1;

    mdio_writel(cfg.wrd, PER_MDIO_CFG);

    return 0;
}

/**
 * ca_mdio_probe - probe mdio device
 * @pdev: mdio platform device
 *
 * Return 0 on success, negative on failure
 */
static int ca_mdio_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct ca_mdio_device *mdio_dev;
	struct mii_bus *new_bus;
	struct resource *res;
	unsigned int of_clock_frequency;
	int ret;

	if (!pdev) {
		dev_err(NULL, "pdev is NULL!\r\n");
		return -ENODEV;
	}
	np = pdev->dev.of_node;
	mdio_dev = devm_kzalloc(&pdev->dev, sizeof(*mdio_dev), GFP_KERNEL);
	if (!mdio_dev)
		return -ENOMEM;

	new_bus = devm_mdiobus_alloc(&pdev->dev);
	if (!new_bus) {
		dev_err(&pdev->dev, "mdiobus_alloc fail!\n");
		return -ENOMEM;
	}

	new_bus->name = MDIO_BUS_NAME;
	new_bus->read = ca_mdio_read;
	new_bus->write = ca_mdio_write;
	new_bus->priv = mdio_dev;
	ca_mdio_bus_name(new_bus->id, np);

	mdio_device_priv = mdio_dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mdio_dev->vbase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mdio_dev->vbase)) {
		ret = PTR_ERR(mdio_dev->vbase);
		return ret;
	}

	if (of_property_read_u32(np, "clock-frequency", &of_clock_frequency)) {
			mdio_dev->peri_clock = of_clock_frequency;
	}
	else {
		mdio_dev->peri_clock = 125000000;	/* Default 125MHz if not specified */
	}
	printk("MDIO clock:%d\n",mdio_dev->peri_clock);

	ca_mdio_speed_set(mdio_dev->peri_clock, __MDIO_CLOCK_DEF);

	new_bus->irq = devm_kcalloc(&pdev->dev, PHY_MAX_ADDR,
				    sizeof(int), GFP_KERNEL);
	if (!new_bus->irq)
		return -ENOMEM;

	new_bus->parent = &pdev->dev;
	platform_set_drvdata(pdev, new_bus);

	ret = of_mdiobus_register(new_bus, np);
	if (ret) {
		dev_err(&pdev->dev, "Cannot register CA MDIO bus!\n");
		platform_set_drvdata(pdev, NULL);
		return ret;
	}

	return 0;
}

/**
 * ca_mdio_remove - remove mdio device
 * @pdev: mdio platform device
 *
 * Return 0 on success, negative on failure
 */
static int ca_mdio_remove(struct platform_device *pdev)
{
	struct mii_bus *bus;

	bus = platform_get_drvdata(pdev);

	mdiobus_unregister(bus);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id ca_mdio_match[] = {
	{.compatible = "cortina,mdio"},
	{.compatible = "cortina,g3-mdio"},
	{.compatible = "cortina,saturn-mdio"},
	{}
};

static struct platform_driver ca_mdio_driver = {
	.probe = ca_mdio_probe,
	.remove = ca_mdio_remove,
	.driver = {
		   .name = MDIO_DRV_NAME,
		   .of_match_table = ca_mdio_match,
		   },
};

module_platform_driver(ca_mdio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cortina-Access Co., Ltd.");
MODULE_DESCRIPTION("Cortina-Access MDIO driver");
MODULE_ALIAS("platform:" MDIO_DRV_NAME);
