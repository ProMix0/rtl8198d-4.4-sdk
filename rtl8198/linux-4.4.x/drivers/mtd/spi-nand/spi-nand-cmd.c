/**
* spi-nand-cmd.c
*
* Copyright (c) 2009-2015 Micron Technology, Inc.
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
#include <linux/mtd/spi-nand.h>
#include <linux/kernel.h>

static struct spi_nand_cmd_cfg *cmd_table;

static struct spi_nand_cmd_cfg micron_cmd_cfg_table[] = {
/*opcode	addr_bytes	addr_bits	dummy_bytes	data_nbits*/
	{SPINAND_CMD_GET_FEATURE,		1,	1,	0,	1},
	{SPINAND_CMD_SET_FEATURE,		1,	1,	0,	1},
	{SPINAND_CMD_PAGE_READ,			3,	1,	0,	0},
	{SPINAND_CMD_READ_PAGE_CACHE_RDM,	3,	1,	0,	0},
	{SPINAND_CMD_READ_PAGE_CACHE_LAST,	0,	0,	0,	0},
#ifdef CONFIG_SPI_NAND_USE_SPI_NOR_RD_PROT
	{SPINAND_CMD_READ_FROM_CACHE,		3,	1,	0,	1},
	{SPINAND_CMD_READ_FROM_CACHE_FAST,	3,	1,	1,	1},
#else
	{SPINAND_CMD_READ_FROM_CACHE,		2,	1,	1,	1},
	{SPINAND_CMD_READ_FROM_CACHE_FAST,	2,	1,	1,	1},
#endif
	{SPINAND_CMD_READ_FROM_CACHE_X2,	2,	1,	1,	2},
	{SPINAND_CMD_READ_FROM_CACHE_DUAL_IO,	2,	2,	1,	2},
	{SPINAND_CMD_READ_FROM_CACHE_X4,	2,	1,	1,	4},
	{SPINAND_CMD_READ_FROM_CACHE_QUAD_IO,	2,	4,	2,	4},
	{SPINAND_CMD_BLK_ERASE,			3,	1,	0,	0},
	{SPINAND_CMD_PROG_EXC,			3,	1,	0,	0},
	{SPINAND_CMD_PROG_LOAD,			2,	1,	0,	1},
	{SPINAND_CMD_PROG_LOAD_RDM_DATA,	2,	1,	0,	1},
	{SPINAND_CMD_PROG_LOAD_X4,		2,	1,	0,	4},
	{SPINAND_CMD_PROG_LOAD_RDM_DATA_X4,	2,	1,	0,	4},
	{SPINAND_CMD_WR_ENABLE,			0,	0,	0,	0},
	{SPINAND_CMD_WR_DISABLE,		0,	0,	0,	0},
	{SPINAND_CMD_READ_ID,			0,	0,	1,	1},
	{SPINAND_CMD_RESET,			0,	0,	0,	0},
	{SPINAND_CMD_END},
};

static struct spi_nand_cmd_cfg gigadevice_cmd_cfg_table[] = {
/*opcode	addr_bytes	addr_bits	dummy_bytes	data_nbits*/
	{SPINAND_CMD_GET_FEATURE,		1,	1,	0,	1},
	{SPINAND_CMD_SET_FEATURE,		1,	1,	0,	1},
	{SPINAND_CMD_PAGE_READ,			3,	1,	0,	0},
	{SPINAND_CMD_READ_FROM_CACHE,		3,	1,	0,	1},
	{SPINAND_CMD_READ_FROM_CACHE_FAST,	3,	1,	1,	1},
	{SPINAND_CMD_BLK_ERASE,			3,	1,	0,	0},
	{SPINAND_CMD_PROG_EXC,			3,	1,	0,	0},
	{SPINAND_CMD_PROG_LOAD,			2,	1,	0,	1},
	{SPINAND_CMD_WR_ENABLE,			0,	0,	0,	0},
	{SPINAND_CMD_WR_DISABLE,		0,	0,	0,	0},
	{SPINAND_CMD_READ_ID,			0,	0,	0,	1},
	{SPINAND_CMD_RESET,			0,	0,	0,	0},
	{SPINAND_CMD_END},
};

struct spi_nand_cmd_cfg *spi_nand_get_cmd_cfg(u8 opcode)
{
	struct spi_nand_cmd_cfg *index = cmd_table;

	for (; index->opcode != SPINAND_CMD_END; index++) {
		if (index->opcode == opcode)
			return index;
	}

	pr_err("Invalid spi nand opcode %x\n", opcode);
	BUG();
}

int spi_nand_set_cmd_cfg_table(int mfr_id)
{
	switch (mfr_id) {
	case SPINAND_MFR_MICRON:
	case SPINAND_MFR_MACRONIX:
	case SPINAND_MFR_TOSHIBA:
	case SPINAND_MFR_ETRON:
	case SPINAND_MFR_WINBOND:
	case SPINAND_MFR_UNION:
		cmd_table = micron_cmd_cfg_table;
		break;
	case SPINAND_MFR_ESMT_GD_ZENTEL:
		cmd_table = gigadevice_cmd_cfg_table;
		break;
	default:
		pr_err("Unknown device\n");
		return -ENODEV;
	}
	return 0;
}
