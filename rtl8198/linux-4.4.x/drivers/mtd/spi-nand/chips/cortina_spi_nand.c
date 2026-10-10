/*
 * Copyright (c) 2017-2019 Cortina Access, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/mtd/spi-nand.h>
#include <soc/cortina/ca_flash.h>

#define SPINAND_STATUS_WEL		0x02
#define SPINAND_STATUS_WIP		0x01
#define SPINAND_STATUS_BP		0x1C
#define SPINAND_STATUS_SRWD		0x80

#define SPINAND_WEL_TIMEOUT		8000
#define SPINAND_PROGRAM_TIMEOUT	8000
#define SPINAND_ERASE_TIMEOUT	400000
#define SPINAND_CMD_TIMEOUT		40

#define SF_ACCESS_MIO_SINGLE	0
#define SF_ACCESS_MIO_DUAL		1
#define SF_ACCESS_MIO_QUARD		2

#define	SPI_TX_DUAL	0x100			/* transmit with 2 wires */
#define	SPI_TX_QUAD	0x200			/* transmit with 4 wires */
#define	SPI_RX_DUAL	0x400			/* receive with 2 wires */
#define	SPI_RX_QUAD	0x800			/* receive with 4 wires */

static int debug = 0;
extern int spi_nand_set_cmd_cfg_table(int mfr_id);
/**
*  Default OOB area specification layout
*/
static struct nand_ecclayout micron_ecc_layout_64 = {
	.eccbytes = 32,
	.eccpos = {
		   8, 9, 10, 11, 12, 13, 14, 15,
		   24, 25, 26, 27, 28, 29, 30, 21,
		   40, 41, 42, 43, 44, 45, 46, 47,
		   56, 57, 58, 59, 60, 61, 62, 63},
	.oobavail = 30,
	.oobfree = {
		{.offset = 2,
		 .length = 6},
		{.offset = 16,
		 .length = 8},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 8}, }
};

static struct nand_ecclayout mxic_ecc_layout_64 = {
	.eccbytes = 14,
	.eccpos = { 2, 3, 16, 17, 18, 19, 32, 33, 34, 35, 48, 49, 50, 51},
	.oobavail = 48,
	.oobfree = {
		{.offset = 4,
		 .length = 12},
		{.offset = 20,
		 .length = 12},
		{.offset = 36,
		 .length = 12},
		{.offset = 52,
		 .length = 12}, }
};

static struct nand_ecclayout micron_ecc_layout_128 = {
	.eccbytes = 64,
	.eccpos = {
		64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127},
	.oobavail = 62,
	.oobfree = {
		{.offset = 2,
		 .length = 62}, }
};

static struct nand_ecclayout gd5f_ecc_layout_64 = {
	.eccbytes = 64,
	.eccpos = {		
		64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127},
	.oobavail = 63,
	.oobfree = { {1, 63} }
};

static struct nand_ecclayout gd5f_ecc_layout_256 = {
	.eccbytes = 128,
	.eccpos = {
		128, 129, 130, 131, 132, 133, 134, 135,
		136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151,
		152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167,
		168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183,
		184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199,
		200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215,
		216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231,
		232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247,
		248, 249, 250, 251, 252, 253, 254, 255
	},
	.oobavail = 127,
	.oobfree = { {1, 127} }
};

void static __iomem *flash_ctl_base;
void static __iomem *flash_mem_base;

#define GET_UP8BITS(x)	((x >> 8) & 0xff)

static void spi_nand_mt29f_ecc_status(struct spi_nand_chip *chip,
					unsigned int status,
					unsigned int *corrected,
					unsigned int *ecc_error)
{
	unsigned int ecc_status = status & SPI_NAND_MT29F_ECC_MASK;

	*ecc_error = (ecc_status == SPI_NAND_MT29F_ECC_UNCORR);
	switch (ecc_status) {
	case SPI_NAND_MT29F_ECC_0_BIT:
		*corrected = 0;
		break;
	case SPI_NAND_MT29F_ECC_1_3_BIT:
		*corrected = 3;
		break;
	case SPI_NAND_MT29F_ECC_4_6_BIT:
		*corrected = 6;
		break;
	case SPI_NAND_MT29F_ECC_7_8_BIT:
		*corrected = 8;
		break;
	}
}

static void spi_nand_gd5f_ecc_status(struct spi_nand_chip *chip,
					unsigned int status,
					unsigned int *corrected,
					unsigned int *ecc_error)
{
	unsigned int ecc_status = (status >> SPI_NAND_GD5F_ECC_SHIFT) &
					     SPI_NAND_GD5F_ECC_MASK;

	*ecc_error = (ecc_status == SPI_NAND_GD5F_ECC_UNCORR);
	/*TODO fix corrected bits*/
	if (*ecc_error == 0)
		*corrected = ecc_status;
}

static int spi_nand_manufacture_init(struct spi_nand_chip *chip)
{
	switch (chip->mfr_id) {
	case SPINAND_MFR_TOSHIBA:
	case SPINAND_MFR_MICRON:
	case SPINAND_MFR_ETRON:
	case SPINAND_MFR_WINBOND:
		chip->get_ecc_status = spi_nand_mt29f_ecc_status;

		if (chip->oob_size == 64)
			chip->ecclayout = &micron_ecc_layout_64;
		else if (chip->oob_size == 128)
			chip->ecclayout = &micron_ecc_layout_128;

		break;
	case SPINAND_MFR_MACRONIX:
		chip->get_ecc_status = spi_nand_mt29f_ecc_status;
		if (chip->oob_size == 64)
			chip->ecclayout = &mxic_ecc_layout_64;
		break;
	case SPINAND_MFR_ESMT_GD_ZENTEL:
		chip->get_ecc_status = spi_nand_gd5f_ecc_status;
		chip->ecc_strength = 8;
		if (chip->oob_size == 64)
			chip->ecclayout = &gd5f_ecc_layout_64;
		else if (chip->oob_size == 256)
			chip->ecclayout = &gd5f_ecc_layout_256;

		break;
	default:
		chip->get_ecc_status = spi_nand_mt29f_ecc_status;
		chip->ecclayout = &micron_ecc_layout_64;
		break;
	}

	return 0;
}

static int wait_flash_cmd_rdy(void)
{
	int tmp;
	int timeout_loop;

	timeout_loop = SPINAND_CMD_TIMEOUT * 10;
	for (;;) {
		tmp = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		if ((tmp & SF_START_BIT_ENABLE) == 0) {
			return 0;
		}

		if (timeout_loop-- <= 0) {
			return -ETIME;
		}

		cond_resched();
	}
}

/*
 * ca_spinand_get_feature_reg - send command 0xf with with address <addr> to get register value
 * Description:
 */
static int ca_spinand_get_feature_reg(struct spi_nand_cmd *cmd)
{
	struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd);
	uint8_t addr = cmd->addr[0];
	u8 *value = cmd->rx_buf;
	unsigned int reg_data;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Get feature use 1 byte address as sub-command.
	 * We must use extend access code to meet this protocol.
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = cmd_cfg->opcode;
	/* No dummy cnt */
	sf_ext_access.bf.sflashDummyCount = -1;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 1 byte address */
	sf_ext_access.bf.sflashAddrCount = 0;/* cmd_cfg->addr_bytes - 1; */
	/* 1 byte data */
	sf_ext_access.bf.sflashDataCount = 0;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Command to get status */
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, addr);

	/* Issue command */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0)
		return -1;

	/* Read out status */
	reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
	*value = reg_data & 0xFF;

	return 0;
}

/*
 * ca_spinand_get_status_reg - send command 0xf with with address <addr> to get register value
 * Description:
 */
static int ca_spinand_get_status_reg(uint8_t *value)
{
	unsigned int reg_data;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Get feature use 1 byte address as sub-command.
	 * We must use extend access code to meet this protocol.
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = SPINAND_CMD_GET_FEATURE;
	/* No dummy cnt */
	sf_ext_access.bf.sflashDummyCount = -1;
	/* 1 byte address */
	sf_ext_access.bf.sflashAddrCount = 0;
	/* 1 byte data */
	sf_ext_access.bf.sflashDataCount = 0;
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Command to get status */
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, REG_STATUS);

	/* Issue command */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0)
		return -1;

	/* Read out status */
	reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
	*value = reg_data & 0xFF;

	return 0;
}

/*
 * ca_spinand_set_feature_reg - send command 0x1f with address <addr> to set register value
 * Description:
 */
static int ca_spinand_set_feature_reg(struct spi_nand_cmd *cmd)
{
	struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd);
	uint8_t addr = cmd->addr[0];
	const u8 *value = cmd->tx_buf;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Set feature use 1 byte address as sub-command.
	 * We must use extend access code to meet this protocol.
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = cmd_cfg->opcode;
	/* No dummy cnt */
	sf_ext_access.bf.sflashDummyCount = -1;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 1 byte address */
	sf_ext_access.bf.sflashAddrCount = 0;/* cmd_cfg->addr_bytes - 1; */
	/* 1 byte data */
	sf_ext_access.bf.sflashDataCount = 0;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Command to get status */
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, addr);

	/* Data register */
	write_flash_ctrl_reg(FLASH_SF_DATA, *value);

	/* Issue command */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0)
		return -1;

	return 0;
}

/* wait_till_ready - Check device status
 * Description:
 *    Read device status register until timeout or OIP cleared.
 */
#define MAX_WAIT_MS  (10)	/* erase might take 4ms */
static int wait_till_ready(void)
{
	int retval;
	uint8_t status = 0;
	unsigned long timeout_jiffies;

	timeout_jiffies = jiffies + msecs_to_jiffies(MAX_WAIT_MS);
	for (;;) {
		retval = ca_spinand_get_status_reg(&status);
		if (retval < 0)
			return -1;
		else if ((status & (STATUS_E_FAIL_MASK | STATUS_P_FAIL_MASK)))
			break;
		else if (!(status & STATUS_BUSY))
			break;

		if (time_after(jiffies, timeout_jiffies)) {
			return -ETIME;
		}

		udelay(1);
		schedule();
	}

	if (status & (STATUS_E_FAIL_MASK | STATUS_P_FAIL_MASK)) {
		printk("Status[0x%x] error!\n", status);
		return -1;
	}

	if ((status & STATUS_BUSY) == 0)
		return 0;

	return -1;
}

/*
 * ca_spinand_die_select - send command 0xc2 with address <addr> to set die num
 * Description:
 */
static int ca_spinand_die_select(struct spi_nand_cmd *cmd)
{
	uint8_t addr = cmd->addr[0];
	const u8 *value = cmd->tx_buf;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;

	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Set feature use 1 byte address as sub-command.
	 * We must use extend access code to meet this protocol.
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = SPINAND_CMD_DIE_SELECT;
	/* No dummy cnt */
	sf_ext_access.bf.sflashDummyCount = -1;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 0 byte address */
	sf_ext_access.bf.sflashAddrCount = 0;/* cmd_cfg->addr_bytes - 1; */
	/* 1 byte data */
	sf_ext_access.bf.sflashDataCount = -1;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Command to get status */
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, addr);

	/* Issue command */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0) {
		printk("%s fail!\n", __func__);
		return -1;
	}

	udelay(1000);

	if (wait_till_ready())
		printk("%s wait timeout!\n", __func__);

	return 0;
}


/*
 * ca_spinand_write_enable- Enable write operation(erase,program)
 * Description:
 *    Enable write operation
 */
static int ca_spinand_write_enable(struct spi_nand_cmd *cmd)
{
	struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd);
	FLASH_SF_ACCESS_t sf_access;
	sf_access.wrd = 0;

	/* Write enable
	 * Opcode only protocol
	 */
	sf_access.bf.sflashOpCode = cmd_cfg->opcode;
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0)
		printk("%s: command timeout!\n", __func__);

	return 0;
}

/*
 * ca_spi_nand_reset - Reset SPI Nand
 * Description:
 *     Reset: reset SPI Nand device
 */
static int ca_spinand_reset(void/* struct spi_nand_cmd *cmd */)
{
	/* struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd); */
	FLASH_SF_ACCESS_t sf_access;
	sf_access.wrd = 0;

	/* Reset SPI-NAND
	 * Opcode only protocol
	 */
	sf_access.bf.sflashOpCode = SPINAND_CMD_RESET/* cmd_cfg->opcode */;
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE);

	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

    /* Wait command ready */
    if (wait_flash_cmd_rdy() != 0) {
		printk("%s fail!\n", __func__);
		return -1;
    }

    /* elapse 1ms before issuing any other command */
    udelay(1000);

    if (wait_till_ready())
		printk("%s wait timeout!\n", __func__);

    return 0;
}

/*
 * ca_spinand_read_id- Read SPI Nand ID
 * Description:
 *    Read ID: read two ID bytes from the SPI Nand device
 */
static int ca_spinand_read_id(u8 *id/* struct spi_nand_cmd *cmd */)
{
	/* struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd); */
	/* u8 *id = cmd->rx_buf; */
	unsigned int reg_data;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;

	/* Read ID
	 * Micron/Macronix:
	 *  Opcode -> 8 dummy bits -> MAF_ID => DEV_ID
	 * GigaDevice:
	 *  Opcode -> MAF_ID => DEV_ID
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = SPINAND_CMD_READ_ID/* cmd_cfg->opcode */;
	/* 0 dummy bits */
	sf_ext_access.bf.sflashDummyCount = -1;/* cmd_cfg->dummy_bytes - 1; */
	/* 0 address byte */
	sf_ext_access.bf.sflashAddrCount = -1;/* cmd_cfg->addr_bytes - 1; */
	/* 3 of data length */
	sf_ext_access.bf.sflashDataCount = 2;
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Start request */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	if (wait_flash_cmd_rdy() != 0) {
		id[0] = id[1] = 0;
		return -1;
	}

	reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);

	if (id[0] != SPINAND_MFR_ESMT_GD_ZENTEL) {
		id[0] = (reg_data & 0x0000ff00) >> 8;
		id[1] = (reg_data & 0x00ff0000) >> 16;
	} else {
		id[0] = reg_data & 0x000000ff;
		id[1] = (reg_data & 0x0000ff00) >> 8;
	}

	return 0;
}

static int ca_spinand_read_page_to_cache(struct spi_nand_cmd *cmd)
{
	struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd);
	u32 page_id = (u32)(cmd->addr[0] << 16) | (u32)(cmd->addr[1] << 8) | (u32)(cmd->addr[2]);
	uint8_t status;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Read to cache
	 * Opcode -> 24bits offset addr => Get feature ....
	 * 3 bytes address must rely on extend access mothod
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	sf_access.bf.sflashMIO = SF_ACCESS_MIO_SINGLE;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = cmd_cfg->opcode;
	/* 0 dummy bits */
	sf_ext_access.bf.sflashDummyCount = -1;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 3 byte address */
	sf_ext_access.bf.sflashAddrCount = 2;/* cmd_cfg->addr_bytes - 1; */
	/* ECC is handled by device, we just push data  */
	sf_ext_access.bf.sflashDataCount = -1;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Fill address */
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, page_id);

	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0) {
		printk("%s fail!\n", __func__);
		return -1;
	}

	/* 80uS with ECC enabled per Micron datasheet */
	udelay(10);

	if (wait_till_ready()) {
		printk("%s wait timeout %d!\n", __func__, page_id);
		return -1;
	}

	ca_spinand_get_status_reg(&status);
	if ((status & STATUS_OIP_MASK) == STATUS_READY) {
		if ((status & STATUS_ECC_MASK) == STATUS_ECC_ERROR) {
			printk("ECC error! page:%d\n", page_id);
			return -1;
		}
		if ((status & STATUS_ECC_MASK) == STATUS_ECC_1BIT_CORRECTED)
			printk("Bit corrected! page:%d\n", page_id);
	}
	return 0;
}

static int ca_spinand_read_cache_x1(struct spi_nand_cmd *cmd)
{
	/* struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd); */
	u32 offset = (u32)(cmd->addr[0] << 8) | (u32)(cmd->addr[1]);
	u8 *buf = cmd->rx_buf;
	u32 len = cmd->n_rx;
	unsigned int reg_data = 0;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Read from cache
	 * Opcode -> 16bits offset addr -> data
	 * 2 bytes address must use extend access mothod
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	sf_access.bf.sflashForceBurst = 1;

	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = SPINAND_CMD_READ_FROM_CACHE_FAST; /* cmd_cfg->opcode; */
	/* 8 dummy bits */
	sf_ext_access.bf.sflashDummyCount = 7;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 2 byte address */
	sf_ext_access.bf.sflashAddrCount = 1;/* cmd_cfg->addr_bytes - 1; */
	/* 4 bytes in one command request */
	sf_ext_access.bf.sflashDataCount = 3;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Fill address */
	if (debug)
		printk("%s: offset =0x%x\n", __func__, offset);
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, offset);

	/* Read out data */
	while (len >= 4) {
		/* Start access */
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

		/* Wait command ready */
		if (wait_flash_cmd_rdy() != 0) {
			printk("%s fail!\n", __func__);
			return -1;
		}

		reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		*buf++ = (reg_data >> 16) & 0xFF;
		*buf++ = (reg_data >> 24) & 0xFF;
		len -= 4;
	}

	if (len > 0) {
		/* Start access */
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);
		/* Wait command ready */
		if (wait_flash_cmd_rdy() != 0) {
			printk("%s fail!\n", __func__);
			return -1;
		}

		reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
	}

	switch (len) {
	case 3:
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		*buf++ = (reg_data >> 16) & 0xFF;
		break;
	case 2:
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		break;
	case 1:
		*buf++ = reg_data & 0xFF;
		break;
	case 0:
		break;
	default:
		printk("%s error length %d!\n", __func__, len);
	}

	/* Finish burst read  */
	sf_access.bf.sflashForceBurst = 0;
	sf_access.bf.sflashForceTerm = 1;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	return 0;
}

static int ca_spinand_read_cache_x2(struct spi_nand_cmd *cmd)
{
	/* struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd); */
	u32 offset = (u32)(cmd->addr[0] << 8) | (u32)(cmd->addr[1]);
	u8 *buf = cmd->rx_buf;
	u32 len = cmd->n_rx;
	unsigned int reg_data = 0;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Read from cache
	 * Opcode -> 16bits offset addr -> data
	 * 2 bytes address must rely on extend access mothod
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	sf_access.bf.sflashForceBurst = 1;
	sf_access.bf.sflashMIO = SF_ACCESS_MIO_DUAL;
	sf_access.bf.sflashMData = 1;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = SPINAND_CMD_READ_FROM_CACHE_X2;/* cmd_cfg->opcode; */
	/* 8 dummy bits */
	sf_ext_access.bf.sflashDummyCount = 7;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 2 byte address */
	sf_ext_access.bf.sflashAddrCount = 1;/* cmd_cfg->addr_bytes - 1; */
	/* 4 bytes in one command request */
	sf_ext_access.bf.sflashDataCount = 3;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Fill address */
	if (debug)
		printk("%s: offset =0x%x\n", __func__, offset);
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, offset);

	/* Read out data */
	while (len >= 4) {
		/* Start access */
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

		/* Wait command ready */
		if (wait_flash_cmd_rdy() != 0) {
			printk("%s fail!\n", __func__);
			return -1;
		}

		reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		*buf++ = (reg_data >> 16) & 0xFF;
		*buf++ = (reg_data >> 24) & 0xFF;
		len -= 4;
	}

	if (len > 0) {
		/* Start access */
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);
		/* Wait command ready */
		if (wait_flash_cmd_rdy() != 0) {
			printk("%s fail!\n", __func__);
			return -1;
		}

		reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
	}

	switch (len) {
	case 3:
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		*buf++ = (reg_data >> 16) & 0xFF;
		break;
	case 2:
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		break;
	case 1:
		*buf++ = reg_data & 0xFF;
		break;
	case 0:
		break;
	default:
		printk("%s error length!\n", __func__);
	}

	/* Finish burst read  */
	sf_access.bf.sflashForceBurst = 0;
	sf_access.bf.sflashForceTerm = 1;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	return 0;
}

static int ca_spinand_read_cache_x4(struct spi_nand_cmd *cmd)
{
	/* struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd); */
	u32 offset = (u32)(cmd->addr[0] << 8) | (u32)(cmd->addr[1]);
	u8 *buf = cmd->rx_buf;
	u32 len = cmd->n_rx;
	unsigned int reg_data = 0;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Read from cache
	 * Opcode -> 16bits offset addr -> data
	 * 2 bytes address must rely on extend access mothod
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	sf_access.bf.sflashForceBurst = 1;
	sf_access.bf.sflashMIO = SF_ACCESS_MIO_QUARD;
	sf_access.bf.sflashMData = 1;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = SPINAND_CMD_READ_FROM_CACHE_X4;/* cmd_cfg->opcode; */
	/* 8 dummy bits */
	sf_ext_access.bf.sflashDummyCount = 7;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 2 byte address */
	sf_ext_access.bf.sflashAddrCount = 1;/* cmd_cfg->addr_bytes - 1; */
	/* 4 bytes in one command request */
	sf_ext_access.bf.sflashDataCount = 3;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Fill address */
	if (debug)
		printk("%s: offset =0x%x\n", __func__, offset);
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, offset);

	/* Read out data */
	while (len >= 4) {
		/* Start access */
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

		/* Wait command ready */
		if (wait_flash_cmd_rdy() != 0) {
			printk("%s fail!\n", __func__);
			return -1;
		}
		reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		*buf++ = (reg_data >> 16) & 0xFF;
		*buf++ = (reg_data >> 24) & 0xFF;
		len -= 4;
	}

	if (len > 0) {
		/* Start access */
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);
		/* Wait command ready */
		if (wait_flash_cmd_rdy() != 0) {
			printk("%s fail!\n", __func__);
			return -1;
		}

		reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
	}

	switch (len) {
	case 3:
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		*buf++ = (reg_data >> 16) & 0xFF;
		break;
	case 2:
		*buf++ = reg_data & 0xFF;
		*buf++ = (reg_data >> 8) & 0xFF;
		break;
	case 1:
		*buf++ = reg_data & 0xFF;
		break;
	case 0:
		break;
	default:
		printk("%s error length!\n", __func__);
	}

	/* Finish burst read  */
	sf_access.bf.sflashForceBurst = 0;
	sf_access.bf.sflashForceTerm = 1;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	return 0;
}

static int ca_spinand_write_cache_x1(struct spi_nand_cmd *cmd)
{
	struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd);
	u32 offset = (u32)(cmd->addr[0] << 8) | (u32)(cmd->addr[1]);
	u8 *buf = cmd->tx_buf;
	u32 len = cmd->n_tx;
	unsigned int reg_data;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	FLASH_FLASH_ACCESS_START_t access_start;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;
	access_start.wrd = 0;

	/* Write to cache
	 * Opcode -> 16bits offset addr -> data ....
	 * 2 bytes address must rely on extend access mothod
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	sf_access.bf.sflashMIO = SF_ACCESS_MIO_SINGLE;
	sf_access.bf.sflashMData = 0;
	sf_access.bf.sflashForceBurst = 1;	/* Burst mode */
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = cmd_cfg->opcode;
	/* 0 dummy bits */
	sf_ext_access.bf.sflashDummyCount = -1;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 2 byte address */
	sf_ext_access.bf.sflashAddrCount = 1;/* cmd_cfg->addr_bytes - 1; */
	/* ECC is handled by device, we just push data  */
	sf_ext_access.bf.sflashDataCount = 3;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Fill address */
	if (debug)
		printk("%s: offset =0x%x\n", __func__, offset);
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, offset);

	access_start.bf.sflashRegReq = 1;
	access_start.bf.sflashRegCmd = 1;
	/* Burst mode */
	while (len > 0) {
		reg_data = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
		write_flash_ctrl_reg(FLASH_SF_DATA, reg_data);
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, access_start.wrd);

		/* Wait command ready */
		if (wait_flash_cmd_rdy() != 0) {
			printk("%s timeout\n", __func__);
			return -1;
		}

		len -= 4;
		buf += 4;
	}

	sf_access.bf.sflashForceBurst = 0;
	sf_access.bf.sflashForceTerm = 1;	/* Terminate burst mode */
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	return 0;
}

static int ca_spinand_write_cache_x4(struct spi_nand_cmd *cmd)
{
	struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd);
	u32 offset = (u32)(cmd->addr[0] << 8) | (u32)(cmd->addr[1]);
	u8 *buf = cmd->tx_buf;
	u32 len = cmd->n_tx;
	unsigned int reg_data;
	FLASH_SF_ACCESS_t sf_access;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	FLASH_FLASH_ACCESS_START_t access_start;
	sf_access.wrd = 0;
	sf_ext_access.wrd = 0;
	access_start.wrd = 0;

	/* Write to cache
	 * Opcode -> 16bits offset addr -> data ....
	 * 2 bytes address must rely on extend access mothod
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	sf_access.bf.sflashMIO = SF_ACCESS_MIO_QUARD;
	sf_access.bf.sflashMData = 1;
	sf_access.bf.sflashForceBurst = 1;	/* Burst mode */
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = cmd_cfg->opcode;
	/* 0 dummy bits */
	sf_ext_access.bf.sflashDummyCount = -1;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 2 byte address */
	sf_ext_access.bf.sflashAddrCount = 1;/* cmd_cfg->addr_bytes - 1; */
	/* 4 bytes in one command request */
	sf_ext_access.bf.sflashDataCount = 3;/* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Fill address */
	if (debug)
		printk("%s: offset =0x%x\n", __func__, offset);
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, offset);

	access_start.bf.sflashRegReq = 1;
	access_start.bf.sflashRegCmd = 1;
	/* Burst mode */
	while (len > 0) {
		reg_data = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
		write_flash_ctrl_reg(FLASH_SF_DATA, reg_data);
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, access_start.wrd);

		/* Wait command ready */
		if (wait_flash_cmd_rdy() != 0) {
			printk("%s timeout\n", __func__);
			return -1;
		}

		len -= 4;
		buf += 4;
	}

	sf_access.bf.sflashForceBurst = 0;
	sf_access.bf.sflashForceTerm = 1;	/* Terminate burst mode */
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	return 0;
}

static int ca_spinand_program_execute(struct spi_nand_cmd *cmd)
{
	struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd);
	u32	offset = (u32)(cmd->addr[0] << 16) | (u32)(cmd->addr[1] << 8) | (u32)(cmd->addr[2]);
	FLASH_SF_ACCESS_t sf_access;
	FLASH_FLASH_ACCESS_START_t access_start;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	sf_access.wrd = 0;
	access_start.wrd = 0;
	sf_ext_access.wrd = 0;

	/* Flush cache to flash chip
	 * Opcode -> 24bits offset addr
	 */
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_EXTEND);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);

	/* Use command in extend_acc register */
	sf_ext_access.bf.sflashDirRdCmdEn = 1;
	sf_ext_access.bf.sflashOpCode = cmd_cfg->opcode;
	/* 0 dummy bits */
	sf_ext_access.bf.sflashDummyCount = -1;/* (cmd_cfg->dummy_bytes * 8) - 1; */
	/* 3 byte address */
	sf_ext_access.bf.sflashAddrCount = 2;/* cmd_cfg->addr_bytes - 1; */
	/* zero data byte */
	sf_ext_access.bf.sflashDataCount = -1; /* cmd_cfg->data_bits - 1; */
	write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);

	/* Fill address */
	if (debug)
		printk("%s: offset =0x%x\n", __func__, offset);
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, offset);

	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0) {
		printk("%s fail!\n", __func__);
		return -1;
	}

	return 0;
}

static int ca_spinand_erase_block(struct spi_nand_cmd *cmd)
{
	struct spi_nand_cmd_cfg *cmd_cfg = spi_nand_get_cmd_cfg(cmd->cmd);
	u32 block_id = (u32)(cmd->addr[0] << 16) | (u32)(cmd->addr[1] << 8) | (u32)(cmd->addr[2]);
	FLASH_SF_ACCESS_t sf_access;
	sf_access.wrd = 0;

	/* Write enable
	 * Opcode only protocol
	 */
	sf_access.bf.sflashOpCode = SPINAND_CMD_WR_ENABLE;
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0)
		printk("%s: command timeout!\n", __func__);


	/* Erase block
	 * Opcode -> 24bits Addr
	 */
	sf_access.bf.sflashOpCode = cmd_cfg->opcode;
	sf_access.bf.sflashAcCode = GET_UP8BITS(SF_AC_OPCODE_4_ADDR);
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, block_id);
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	/* Wait command ready */
	if (wait_flash_cmd_rdy() != 0) {
		printk("%s fail!\n", __func__);
		return -1;
	}

	/* elapse 1ms before issuing any other command */
	udelay(10);

	if (wait_till_ready()) {
		printk("%s wait timeout %d!\n", __func__, block_id);
		return -1;
	}

	return 0;
}

#define ONFI_CRC_BASE	0x4F4E
static u16 onfi_crc16(u16 crc, u8 const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}

/*
 * ca_spinand_cmd_fn - to process a command to send to the SPI-NAND
 * by cortina SPI NAND controller
 * @chip: SPI-NAND device structure
 * @cmd: command structure
 * Description:
 *   Set up the command buffer to send to the SPI controller.
 *   The command buffer has to initialized to 0.
 */
static int ca_spinand_cmd_fn(struct spi_nand_chip *chip,
				struct spi_nand_cmd *cmd)
{
	int ret = 0;

	switch (cmd->cmd) {
	case SPINAND_CMD_RESET:
		ret = ca_spinand_reset(/* cmd */);
		break;
	case SPINAND_CMD_GET_FEATURE:
		ret = ca_spinand_get_feature_reg(cmd);
		break;
	case SPINAND_CMD_SET_FEATURE:
		ret = ca_spinand_set_feature_reg(cmd);
		break;
	case SPINAND_CMD_DIE_SELECT:
		ret = ca_spinand_die_select(cmd);
		break;
	case SPINAND_CMD_PAGE_READ:
		ret = ca_spinand_read_page_to_cache(cmd);
		break;
	case SPINAND_CMD_READ_PAGE_CACHE_RDM:
		/* Todo */
		break;
	case SPINAND_CMD_READ_PAGE_CACHE_LAST:
		/* Todo */
		break;
	case SPINAND_CMD_READ_FROM_CACHE:
	case SPINAND_CMD_READ_FROM_CACHE_FAST:
		ret = ca_spinand_read_cache_x1(cmd);
		break;
	case SPINAND_CMD_READ_FROM_CACHE_X2:
	case SPINAND_CMD_READ_FROM_CACHE_DUAL_IO:
		ret = ca_spinand_read_cache_x2(cmd);
		break;
	case SPINAND_CMD_READ_FROM_CACHE_X4:
	case SPINAND_CMD_READ_FROM_CACHE_QUAD_IO:
		ret = ca_spinand_read_cache_x4(cmd);
		break;
	case SPINAND_CMD_BLK_ERASE:
		ret = ca_spinand_erase_block(cmd);
		break;
	case SPINAND_CMD_PROG_EXC:
		ret = ca_spinand_program_execute(cmd);
		break;
	case SPINAND_CMD_PROG_LOAD:
		ret = ca_spinand_write_cache_x1(cmd);
		break;
	case SPINAND_CMD_PROG_LOAD_RDM_DATA:
		/* Todo */
		break;
	case SPINAND_CMD_PROG_LOAD_X4:
		ret = ca_spinand_write_cache_x4(cmd);
		break;
	case SPINAND_CMD_PROG_LOAD_RDM_DATA_X4:
		/* Todo */
		break;
	case SPINAND_CMD_READ_ID:
		ret = ca_spinand_read_id(cmd->rx_buf/* cmd */);
		break;
	case SPINAND_CMD_WR_DISABLE:
		/* Todo */
		break;
	case SPINAND_CMD_WR_ENABLE:
		ret = ca_spinand_write_enable(cmd);
		break;
	case SPINAND_CMD_END:
	default:
		break;
	}

    return ret;
}

#ifdef CONFIG_OF
static struct of_device_id ca_spinand_dt_ids[] = {
    {.compatible = "cortina,spi-nand",},
    {},
};

MODULE_DEVICE_TABLE(of, ca_spi_nand_dt_ids);
#endif

static int ca_spinand_probe(struct platform_device *pdev)
{
	struct resource mem_resource;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;
	struct spi_nand_chip *chip;
	struct mtd_info *mtd;
	struct mtd_part_parser_data ppdata;
	const char mfr_name[11];
	u32 spi_mode;
	u8 mfr_id[2], ret;

	match = of_match_device(ca_spinand_dt_ids, &pdev->dev);
	if (!match)
		return -EINVAL;

	dev_notice(&pdev->dev, "Cortina-Access SPI-NAND Flash driver\n");

	/* SPI Nand Flash Controller addr base */
	ret = of_address_to_resource(np, 0, &mem_resource);
	if (ret) {
		dev_warn(&pdev->dev, "invalid address %d\n", ret);
		return ret;
	}

	flash_ctl_base = devm_ioremap(&pdev->dev, mem_resource.start,
		resource_size(&mem_resource));

	chip = kzalloc(sizeof(struct spi_nand_chip), GFP_KERNEL);
	if (!chip) {
		ret = -ENOMEM;
		goto err1;
	}

	mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd) {
		ret = -ENOMEM;
		goto err2;
	}

	mtd->priv = chip;
	chip->mtd = mtd;
	chip->command_fn = ca_spinand_cmd_fn;

	if (!of_property_read_u32(np, "io-mode", &spi_mode))
		printk("SPI-NAND io-mode:%x\n", spi_mode);
	else
		spi_mode = 0;

	if (spi_mode & SPI_RX_QUAD)
		chip->controller_caps |= SPINAND_RX_QUAD;
	if (spi_mode & SPI_RX_DUAL)
		chip->controller_caps |= SPINAND_RX_DUAL;
	if (spi_mode & SPI_TX_QUAD)
		chip->controller_caps |= SPINAND_TX_QUAD;
	if (spi_mode & SPI_TX_DUAL)
		chip->controller_caps |= SPINAND_TX_DUAL;
	chip->priv = pdev;
	platform_set_drvdata(pdev, chip);

#if 0
	/*
	 * read ID command format might be different for different manufactory
	 * such as, Micron SPI NAND need extra one dummy byte after perform
	 * read ID command but Giga device don't need.
	 *
	 * So, specify manufactory of device in device tree is obligatory
	 */
	of_property_read_string(np, "spinand,mfr-name", &mfr_name);
	if (!strncmp(mfr_name, "micron", 6)) {
		mfr_id = SPINAND_MFR_MICRON;
	} else if (!strncmp(mfr_name, "gigadevice", 10)) {
		mfr_id = SPINAND_MFR_GIGADEVICE;
	} else if (!strncmp(mfr_name, "macronix", 10)) {
		mfr_id = SPINAND_MFR_MACRONIX;
	} else if (!strncmp(mfr_name, "toshiba", 10)) {
		mfr_id = SPINAND_MFR_TOSHIBA;
	} else {
		printk("Unknown SPI-NAND Device Manufactory ID!\n");
		goto err3;
	}
#endif

	/* If don't want to get mfr_id througth device tree. Then get it here.*/
	ca_spinand_reset();
	ca_spinand_read_id(mfr_id);

	switch (mfr_id[0]) {
	case SPINAND_MFR_MICRON:
		strcpy(mfr_name, "Micron");
		break;
	case SPINAND_MFR_ESMT_GD_ZENTEL:
		strcpy(mfr_name, "Gigadevice");
		break;
	case SPINAND_MFR_MACRONIX:
		strcpy(mfr_name, "Macronix");
		break;
	case SPINAND_MFR_TOSHIBA:
		strcpy(mfr_name, "Toshiba");
		break;
	case SPINAND_MFR_ETRON:
                strcpy(mfr_name, "Etron");
                break;
	case SPINAND_MFR_WINBOND:
		strcpy(mfr_name, "Winbond");
		break;
	case SPINAND_MFR_UNION:
		strcpy(mfr_name, "Unionchip");
		break;

	default:
		strcpy(mfr_name, "Unknown");
		goto err3;
	}

	printk("SPI-NAND flash mfr-name: %s, mdr_id: 0x%x, dev_id: 0x%x\n",
		mfr_name, mfr_id[0], mfr_id[1]);

	ret = spi_nand_set_cmd_cfg_table(mfr_id[0]);
	if (ret)
		goto err3;

	ret = spi_nand_scan_ident(mtd);
	if (ret)
		goto err3;

	spi_nand_manufacture_init(chip);
	ret = spi_nand_scan_tail(mtd);
	if (ret)
		goto err4;

	ppdata.of_node = pdev->dev.of_node;

	ret = mtd_device_parse_register(mtd, NULL, &ppdata, NULL, 0);
	if (!ret)
		return 0;

	spi_nand_scan_tail_release(mtd);
err4:
	spi_nand_scan_ident_release(mtd);
err3:
	platform_set_drvdata(pdev, NULL);
	kfree(mtd);
err2:
	kfree(chip);
err1:
	return ret;
}

static int ca_spinand_remove(struct platform_device *pdev)
{
	struct spi_nand_chip *chip = platform_get_drvdata(pdev);
	struct mtd_info *mtd = chip->mtd;

	kfree(mtd);
	kfree(chip);

	return 0;
}

static struct platform_driver ca_spinand_driver = {
	.probe	= ca_spinand_probe,
	.remove	= ca_spinand_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ca_spi_nand",
		.of_match_table = of_match_ptr(ca_spinand_dt_ids),
	},
};

static int __init ca_spinand_init(void)
{
	return platform_driver_register(&ca_spinand_driver);
}

static void __exit ca_spinand_exit(void)
{
	platform_driver_unregister(&ca_spinand_driver);
}

module_init(ca_spinand_init);
module_exit(ca_spinand_exit);

MODULE_DESCRIPTION("Cortina Access SPI-NAND Flash Controller Driver");
MODULE_AUTHOR("Pengpeng.Chen<pengpeng.chen@cortina-access.com>");
MODULE_LICENSE("GPL v2");
