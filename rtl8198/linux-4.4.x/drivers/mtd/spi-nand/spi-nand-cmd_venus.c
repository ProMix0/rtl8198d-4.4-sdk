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

static struct spi_nand_cmd_cfg gigadevice_cmd_cfg_table[] = {
    /*opcode    addr_bytes    addr_io_bits    dummy_cycles    data_io_bits*/
	{SPINAND_CMD_GET_FEATURE,		     1,	1,	0,	1},
	{SPINAND_CMD_SET_FEATURE,		     1,	1,	0,	1},
	{SPINAND_CMD_PAGE_READ,			     3,	1,	0,	0},
	{SPINAND_CMD_READ_FROM_CACHE,		 2,	1,	0,	1},
	{SPINAND_CMD_READ_FROM_CACHE_FAST,	 2,	1,	8,	1},
	{SPINAND_CMD_READ_FROM_CACHE_X2,	 2,	1,	8,	2},
	{SPINAND_CMD_READ_FROM_CACHE_DUAL_IO,2,	2,	4,	2},
	{SPINAND_CMD_READ_FROM_CACHE_X4,	 2,	1,	8,	4},
	{SPINAND_CMD_READ_FROM_CACHE_QUAD_IO,2,	4,	2,	4},
	{SPINAND_CMD_BLK_ERASE,			     3,	1,	0,	0},
	{SPINAND_CMD_PROG_EXC,			     3,	1,	0,	0},
	{SPINAND_CMD_PROG_LOAD,			     2,	1,	0,	1},
	{SPINAND_CMD_PROG_LOAD_X4,		     2,	1,	0,	4},
	{SPINAND_CMD_WR_ENABLE,			     0,	0,	0,	0},
	{SPINAND_CMD_WR_DISABLE,		     0,	0,	0,	0},
	{SPINAND_CMD_END},
};

static struct spi_nand_cmd_cfg dosilicon_cmd_cfg_table[] = {
    /*opcode    addr_bytes    addr_io_bits    dummy_cycles    data_io_bits*/
	{SPINAND_CMD_GET_FEATURE,		     1,	1,	0,	1},
	{SPINAND_CMD_SET_FEATURE,		     1,	1,	0,	1},
	{SPINAND_CMD_PAGE_READ,			     3,	1,	0,	0},
	{SPINAND_CMD_READ_FROM_CACHE,		 2,	1,	0,	1},
	{SPINAND_CMD_READ_FROM_CACHE_FAST,	 2,	1,	8,	1},
	{SPINAND_CMD_READ_FROM_CACHE_X2,	 2,	1,	8,	2},
	{SPINAND_CMD_READ_FROM_CACHE_DUAL_IO,2,	2,	4,	2},
	{SPINAND_CMD_READ_FROM_CACHE_X4,	 2,	1,	8,	4},
	{SPINAND_CMD_BLK_ERASE,			     3,	1,	0,	0},
	{SPINAND_CMD_PROG_EXC,			     3,	1,	0,	0},
	{SPINAND_CMD_PROG_LOAD,			     2,	1,	0,	1},
	{SPINAND_CMD_PROG_LOAD_X4,		     2,	1,	0,	4},
	{SPINAND_CMD_WR_ENABLE,			     0,	0,	0,	0},
	{SPINAND_CMD_WR_DISABLE,		     0,	0,	0,	0},
	{SPINAND_CMD_END},
};

static struct spi_nand_cmd_cfg fmsh_s01_cmd_cfg_table[] = {
    /*opcode    addr_bytes    addr_io_bits    dummy_cycles    data_io_bits*/
	{SPINAND_CMD_GET_FEATURE,		     1,	1,	0,	1},
	{SPINAND_CMD_SET_FEATURE,		     1,	1,	0,	1},
	{SPINAND_CMD_PAGE_READ,			     3,	1,	0,	0},
	{SPINAND_CMD_READ_FROM_CACHE,		 2,	1,	0,	1},
	{SPINAND_CMD_READ_FROM_CACHE_FAST,	 2,	1,	8,	1},
	{SPINAND_CMD_READ_FROM_CACHE_X2,	 2,	1,	8,	2},
	{SPINAND_CMD_READ_FROM_CACHE_DUAL_IO,2,	2,	4,	2},
	{SPINAND_CMD_READ_FROM_CACHE_X4,	 2,	1,	8,	4},
	{SPINAND_CMD_READ_FROM_CACHE_QUAD_IO,2,	4,	4,	4},
	{SPINAND_CMD_BLK_ERASE,			     3,	1,	0,	0},
	{SPINAND_CMD_PROG_EXC,			     3,	1,	0,	0},
	{SPINAND_CMD_PROG_LOAD,			     2,	1,	0,	1},
	{SPINAND_CMD_PROG_LOAD_X4,		     2,	1,	0,	4},
	{SPINAND_CMD_WR_ENABLE,			     0,	0,	0,	0},
	{SPINAND_CMD_WR_DISABLE,		     0,	0,	0,	0},
	{SPINAND_CMD_END},
};


static struct spi_nand_cmd_cfg micron_cmd_cfg_table[] = {
    /*opcode    addr_bytes    addr_io_bits    dummy_cycles    data_io_bits*/
	{SPINAND_CMD_GET_FEATURE,		     1,	1,	0,	1},
	{SPINAND_CMD_SET_FEATURE,		     1,	1,	0,	1},
	{SPINAND_CMD_PAGE_READ,			     3,	1,	0,	0},
	{SPINAND_CMD_READ_FROM_CACHE,		 2,	1,	0,	1},
	{SPINAND_CMD_READ_FROM_CACHE_FAST,	 2,	1,	8,	1},
	{SPINAND_CMD_READ_FROM_CACHE_X2,	 2,	1,	8,	2},
	{SPINAND_CMD_READ_FROM_CACHE_DUAL_IO,2,	2,	4,	2},
	{SPINAND_CMD_READ_FROM_CACHE_X4,	 2,	1,	8,	4},
	{SPINAND_CMD_READ_FROM_CACHE_QUAD_IO,2,	4,	4,	4},
	{SPINAND_CMD_BLK_ERASE,			     3,	1,	0,	0},
	{SPINAND_CMD_PROG_EXC,			     3,	1,	0,	0},
	{SPINAND_CMD_PROG_LOAD,			     2,	1,	0,	1},
	{SPINAND_CMD_PROG_LOAD_X4,		     2,	1,	0,	4},
	{SPINAND_CMD_WR_ENABLE,			     0,	0,	0,	0},
	{SPINAND_CMD_WR_DISABLE,		     0,	0,	0,	0},
	{SPINAND_CMD_END},
};



static struct spi_nand_cmd_cfg mxic_cmd_cfg_table[] = {
    /*opcode    addr_bytes    addr_io_bits    dummy_cycles    data_io_bits*/
    {SPINAND_CMD_MXIC_ECC_STS_READ,      0,  1,  8,  1},
    {SPINAND_CMD_GET_FEATURE,            1,  1,  0,  1},
    {SPINAND_CMD_SET_FEATURE,            1,  1,  0,  1},
    {SPINAND_CMD_PAGE_READ,              3,  1,  0,  0},
    {SPINAND_CMD_READ_FROM_CACHE,        2,  1,  0,  1},
    {SPINAND_CMD_READ_FROM_CACHE_FAST,   2,  1,  8,  1},
    {SPINAND_CMD_READ_FROM_CACHE_X2,     2,  1,  8,  2},
    {SPINAND_CMD_READ_FROM_CACHE_X4,     2,  1,  8,  4},
    {SPINAND_CMD_BLK_ERASE,              3,  1,  0,  0},
    {SPINAND_CMD_PROG_EXC,               3,  1,  0,  0},
    {SPINAND_CMD_PROG_LOAD,              2,  1,  0,  1},
    {SPINAND_CMD_PROG_LOAD_X4,           2,  1,  0,  4},
    {SPINAND_CMD_WR_ENABLE,              0,  0,  0,  0},
    {SPINAND_CMD_WR_DISABLE,             0,  0,  0,  0},
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

int spi_nand_set_cmd_cfg_table(int mfr_id, int dev_id)
{
    switch(mfr_id) {
        case SPINAND_MFR_ATO:
            cmd_table = gigadevice_cmd_cfg_table;
            break;
        case SPINAND_MFR_DOSILICON:
            cmd_table = micron_cmd_cfg_table;
            break;
        case SPINAND_MFR_IS_ESMT:
            cmd_table = micron_cmd_cfg_table;
            break;
        case SPINAND_MFR_ETRON:
            cmd_table = gigadevice_cmd_cfg_table;
            break;
        case SPINAND_MFR_FMSH:
            if((dev_id == 0xA1)||(dev_id == 0xE4)){
                cmd_table = fmsh_s01_cmd_cfg_table;
            }else if(dev_id == 0x92){
                cmd_table = gigadevice_cmd_cfg_table;
            }
            break;
        case SPINAND_MFR_IS_GD:
        case SPINAND_MFR_HEYANGTEK:
            cmd_table = gigadevice_cmd_cfg_table;
            break;
        case SPINAND_MFR_LONGSYS:
            cmd_table = gigadevice_cmd_cfg_table;
            break;
        case SPINAND_MFR_MACRONIX:
            cmd_table = mxic_cmd_cfg_table;
            break;
        case SPINAND_MFR_MICRON:
        case SPINAND_MFR_TOSHIBA:
        case SPINAND_MFR_UNION:
        case SPINAND_MFR_WINBOND:
            cmd_table = micron_cmd_cfg_table;
            break;
        case SPINAND_MFR_XTX:
            cmd_table = gigadevice_cmd_cfg_table;
            break;
        case SPINAND_MFR_IS_ZENTEL:
            cmd_table = micron_cmd_cfg_table;
            break;
        default:
		pr_err("Unknown device\n");
            return -ENODEV;
    }
    return 0;
}
