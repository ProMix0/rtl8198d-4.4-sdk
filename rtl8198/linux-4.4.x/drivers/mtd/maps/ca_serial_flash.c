/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/math64.h>
#include <linux/mod_devicetable.h>

#include <linux/init.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/cfi.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <soc/cortina/ca_flash.h>
#include <asm/current.h>
#include <linux/sched.h>


#define  g_chipen			SERIAL_FLASH_CHIP0_EN
#define SFLASH_4BYTE_ADDR		0x400



#define SPANSION_WRITE_STATUS_OPCODE		( 0x01 | SF_AC_OPCODE_1_DATA  )
#define SPANSION_PROGRAM_OPCODE         ( 0x02 | SF_AC_OPCODE_4_ADDR_1_DATA )
#define SPANSION_READ_OPCODE            ( 0x03 | SF_AC_OPCODE_4_ADDR_1_DATA )
#define SPANSION_READ_STATUS_OPCODE	( 0x05 | SF_AC_OPCODE_1_DATA  )
#define SPANSION_WRITE_ENABLE_OPCODE	( 0x06 | SF_AC_OPCODE )
#define SPANSION_ERASE_OPCODE           ( 0xD8 | SF_AC_OPCODE_4_ADDR )
#define SPANSION_EN_QUAD_OPCODE           ( 0x35 | SF_AC_OPCODE )
#define SPANSION_QREAD4B_OPCODE			(0x6C | SF_AC_OPCODE_4_ADDR_1_DATA)

#define SFLASH_FASTREAD_OPCODE          ( 0x0B | SF_AC_OPCODE_4_ADDR_1_DATA )
#define SFLASH_BE_4K_OPCODE           	( 0x20 | SF_AC_OPCODE_4_ADDR )
#define SFLASH_BE_32K_OPCODE           	( 0x52 | SF_AC_OPCODE_4_ADDR )
#define SFLASH_CHIP_ERASE_OPCODE      	( 0xc7 | SF_AC_OPCODE_4_ADDR )
#define SFLASH_READID_OPCODE           	( 0x9F | SF_AC_OPCODE_4_DATA )

/* Used for SST flashes only. */
//#define OPCODE_BP               0x02    /* Byte program */
//#define OPCODE_WRDI             0x04    /* Write disable */
//#define OPCODE_AAI_WP           0xad    /* Auto address increment word program */

/* Used for Macronix flashes only. */
#define SFLASH_OPCODE_EN4B             ( 0xb7 | SF_AC_OPCODE  )    /* Enter 4-byte mode */
#define SFLASH_OPCODE_EX4B             ( 0xe9 | SF_AC_OPCODE  )    /* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SFLASH_OPCODE_BRWR             ( 0x17 | SF_AC_OPCODE_1_DATA  )   /* Bank register write */

#define SPANSION_STATUS_WEL             0x02
#define SPANSION_STATUS_WIP             0x01
#define SPANSION_STATUS_BP		0x1C
#define SPANSION_STATUS_SRWD            0x80

#define SPANSION_WEL_TIMEOUT             8000
#define SPANSION_PROGRAM_TIMEOUT         8000
#define SPANSION_ERASE_TIMEOUT          400000
#define SPANSION_CMD_TIMEOUT		40

/* Define to check block protection */
//#define SPANSION_BLOCK_PROTECTION		1

//#define S25FL128P_SECTOR_SIZE         SZ_64K
//#define S25FL128P_PAGE_SIZE           SZ_256
//#define S25FL128P_SIZE		      SZ_16M

static u32 sflash_sector_size = 0x00040000;	//0x00010000
static u32 sflash_page_size = 0x00000100;
static u32 sflash_size = 0x01000000;
static u32 sflash_type = 0x0;
struct mutex	sflash_lock;
void __iomem *flash_ctl_base;
static u32 sflash_mem_start_addr;
static u32 sflash_mem_size;

#ifdef CONFIG_MTD_CS752X_MULTIFLASH
#include <linux/semaphore.h>
extern struct semaphore ca_flash_sem;
#endif

struct mtd_info *ca_map_serial_probe(struct map_info *map);
static struct mtd_info *ca_spansion_mtd;

/* Default flash setting */
static struct map_info s25fl128p_map = {
	.name = "ca_serial_flash",
	.size = 0x02000000,
	.bankwidth = 4, 	/* buswidth: 4, */
	.phys =	0,
};

static const char * const probe_types[] = {
	"cmdlinepart", "ofpart" };

/*
 * SPI device driver setup and teardown
 */
 /*
struct spi_device_id {
        char name[32];
        unsigned long driver_data
                        __attribute__((aligned(sizeof(unsigned long))));
};
*/

#define JEDEC_MFR(_jedec_id)    ((_jedec_id) >> 16)

struct flash_info {
        /* JEDEC id zero means "no ID" (most older chips); otherwise it has
         * a high byte of zero plus three data bytes: the manufacturer id,
         * then a two byte device id.
         */
        u32             jedec_id;
        u16             ext_id;

        /* The size listed here is what works with OPCODE_SE, which isn't
         * necessarily called a "sector" by the vendor.
         */
        unsigned        sector_size;
        u16             n_sectors;

        u16             page_size;
        u16             addr_width;

        u16             flags;
#define SECT_4K         0x01            /* OPCODE_BE_4K works uniformly */
#define DUAL_READ		0x02				/* Support DDR */
#define QUAD_READ		0x04				/* Quad I/O */
#define M25P_NO_ERASE   0x02            /* No erase command needed */
};

#define INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags)      \
        ((unsigned long)&(struct flash_info) {                         \
                .jedec_id = (_jedec_id),                                \
                .ext_id = (_ext_id),                                    \
                .sector_size = (_sector_size),                          \
                .n_sectors = (_n_sectors),                              \
                .page_size = 256,                                       \
                .flags = (_flags),                                      \
        })

/* NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
static struct spi_device_id m25p_ids[] = {
        /* Atmel -- some are (confusingly) marketed as "DataFlash" */
        { "at25fs010",  INFO(0x1f6601, 0, 32 * 1024,   4, SECT_4K) },
        { "at25fs040",  INFO(0x1f6604, 0, 64 * 1024,   8, SECT_4K) },

        { "at25df041a", INFO(0x1f4401, 0, 64 * 1024,   8, SECT_4K) },
        { "at25df321a", INFO(0x1f4701, 0, 64 * 1024,  64, SECT_4K) },
        { "at25df641",  INFO(0x1f4800, 0, 64 * 1024, 128, SECT_4K) },

        { "at26f004",   INFO(0x1f0400, 0, 64 * 1024,  8, SECT_4K) },
        { "at26df081a", INFO(0x1f4501, 0, 64 * 1024, 16, SECT_4K) },
        { "at26df161a", INFO(0x1f4601, 0, 64 * 1024, 32, SECT_4K) },
        { "at26df321",  INFO(0x1f4700, 0, 64 * 1024, 64, SECT_4K) },

        /* EON -- en25xxx */
        { "en25f32", INFO(0x1c3116, 0, 64 * 1024,  64, SECT_4K) },
        { "en25p32", INFO(0x1c2016, 0, 64 * 1024,  64, 0) },
        { "en25q32b", INFO(0x1c3016, 0, 64 * 1024,  64, 0) },
        { "en25p64", INFO(0x1c2017, 0, 64 * 1024, 128, 0) },
        { "en25qh64a", INFO(0x1c7017, 0, 64 * 1024,  128, 0) },
        { "en25qh128a", INFO(0x1c7018, 0, 64 * 1024,  256, 0) },

        /* ESMT */
        {"f25l32pa", INFO(0x8c2016, 0, 64 * 1024, 64, SECT_4K)},
        {"f25l32qa", INFO(0x8c4016, 0, 64 * 1024, 64, SECT_4K)},
        {"f25l32qa(2s)", INFO(0x8c4116, 0, 64 * 1024, 64, SECT_4K)},
        {"f25l64qa", INFO(0x8c4117, 0, 64 * 1024, 128, SECT_4K)},
        {"f25l128qa",	 INFO(0x8c4118, 0, 64 * 1024, 256, SECT_4K)},

        /* GigaDevice */
        {"gd25q64", INFO(0xc84017, 0, 64 * 1024, 128, SECT_4K)},
        {"gd25q127csigr", INFO(0xc84018, 0, 64 * 1024, 256, SECT_4K)},

        /* Intel/Numonyx -- xxxs33b */
        { "160s33b",  INFO(0x898911, 0, 64 * 1024,  32, 0) },
        { "320s33b",  INFO(0x898912, 0, 64 * 1024,  64, 0) },
        { "640s33b",  INFO(0x898913, 0, 64 * 1024, 128, 0) },

        /* Macronix */
        { "mx25l4005a",  INFO(0xc22013, 0, 64 * 1024,   8, SECT_4K) },
        { "mx25l8005",   INFO(0xc22014, 0, 64 * 1024,  16, 0) },
        { "mx25l1606e",  INFO(0xc22015, 0, 64 * 1024,  32, SECT_4K) },
        { "mx25l3205d",  INFO(0xc22016, 0, 64 * 1024,  64, 0) },
        { "mx25l6405d",  INFO(0xc22017, 0, 64 * 1024, 128, 0) },
        { "mx25l12845e", INFO(0xc2c218, 0, 64 * 1024, 512, 0) },
        /* { "mx25l12805d", INFO(0xc22018, 0, 64 * 1024, 256, 0) }, */
        { "mx25l12833f", INFO(0xc22018, 0, 64 * 1024, 256, 0) },
        { "mx25l12855e", INFO(0xc22618, 0, 64 * 1024, 256, 0) },
         /* 257 4 bytes address */
        { "mx25l25735e", INFO(0xc22019, 0, 64 * 1024, 512, 0) },
        /* 256 3 bytes address */
        { "mx25l25635e", INFO(0xc22019, 0, 64 * 1024, 512, 0) },
		//{ "mx25l51245g", INFO(0xc2201a, 0, 64 * 1024, 1024, QUAD_READ) },
		{ "mx25l51245g", INFO(0xc2201a, 0, 64 * 1024, 1024, 0) },
        { "mx25l25655e", INFO(0xc22619, 0, 64 * 1024, 512, 0) },
        { "mx25l51245g", INFO(0xc2201a, 0, 64 * 1024, 1024, 0) },
        { "mx25l3235d", INFO(0xc25e16, 0, 64 * 1024, 64, SECT_4K) },

        /* Micron */
        {"n25q064", INFO(0x20ba17, 0, 64 * 1024, 128, 0)},
        {"n25q128a13", INFO(0x20ba18, 0, 64 * 1024, 256, 0)},

        /* Spansion -- single (large) sector size only, at least
         * for the chips listed here (without boot sectors).
         */
        { "s25sl004a",  INFO(0x010212,      0,  64 * 1024,   8, 0) },
        { "s25sl008a",  INFO(0x010213,      0,  64 * 1024,  16, 0) },
        { "s25sl016a",  INFO(0x010214,      0,  64 * 1024,  32, 0) },
        { "s25sl032a",  INFO(0x010215,      0,  64 * 1024,  64, 0) },
        { "s25sl032p",  INFO(0x010215, 0x4d00,  64 * 1024,  64, SECT_4K) },
        { "s25sl064a",  INFO(0x010216,      0,  64 * 1024, 128, 0) },
        { "s25fl256s0", INFO(0x010219, 0x4d00, 256 * 1024, 128, 0) },
        { "s25fl256s1", INFO(0x010219, 0x4d01,  64 * 1024, 512, 0) },
        { "s25fl512s",  INFO(0x010220, 0x4d00, 256 * 1024, 256, 0) },
        { "s70fl01gs",  INFO(0x010221, 0x4d00, 256 * 1024, 256, 0) },
        { "s25sl12800", INFO(0x012018, 0x0300, 256 * 1024,  64, 0) },
        /* { "s25sl12801", INFO(0x012018, 0x0301,  64 * 1024, 256, 0) }, */
        { "s25fl129p0", INFO(0x012018, 0x4d00, 256 * 1024,  64, 0) },
        { "s25fl129p1", INFO(0x012018, 0x4d01,  64 * 1024, 256, 0) },
        { "s25fl016k",  INFO(0xef4015,      0,  64 * 1024,  32, SECT_4K) },
        { "s25fl064k",  INFO(0xef4017,      0,  64 * 1024, 128, SECT_4K) },

        /* SST -- large erase sizes are "overlays", "sectors" are 4K */
        { "sst25vf040b", INFO(0xbf258d, 0, 64 * 1024,  8, SECT_4K) },
        { "sst25vf080b", INFO(0xbf258e, 0, 64 * 1024, 16, SECT_4K) },
        { "sst25vf016b", INFO(0xbf2541, 0, 64 * 1024, 32, SECT_4K) },
        { "sst25vf032b", INFO(0xbf254a, 0, 64 * 1024, 64, SECT_4K) },
        { "sst25wf512",  INFO(0xbf2501, 0, 64 * 1024,  1, SECT_4K) },
        { "sst25wf010",  INFO(0xbf2502, 0, 64 * 1024,  2, SECT_4K) },
        { "sst25wf020",  INFO(0xbf2503, 0, 64 * 1024,  4, SECT_4K) },
        { "sst25wf040",  INFO(0xbf2504, 0, 64 * 1024,  8, SECT_4K) },

        /* ST Microelectronics -- newer production may have feature updates */
        { "m25p05",  INFO(0x202010,  0,  32 * 1024,   2, 0) },
        { "m25p10",  INFO(0x202011,  0,  32 * 1024,   4, 0) },
        { "m25p20",  INFO(0x202012,  0,  64 * 1024,   4, 0) },
        { "m25p40",  INFO(0x202013,  0,  64 * 1024,   8, 0) },
        { "m25p80",  INFO(0x202014,  0,  64 * 1024,  16, 0) },
        { "m25p16",  INFO(0x202015,  0,  64 * 1024,  32, 0) },
        { "m25p32",  INFO(0x202016,  0,  64 * 1024,  64, 0) },
        { "m25p64",  INFO(0x202017,  0,  64 * 1024, 128, 0) },
        { "m25p128", INFO(0x202018,  0, 256 * 1024,  64, 0) },

        //{ "m25p05-nonjedec",  INFO(0, 0,  32 * 1024,   2, 0) },
        //{ "m25p10-nonjedec",  INFO(0, 0,  32 * 1024,   4, 0) },
        //{ "m25p20-nonjedec",  INFO(0, 0,  64 * 1024,   4, 0) },
        //{ "m25p40-nonjedec",  INFO(0, 0,  64 * 1024,   8, 0) },
        //{ "m25p80-nonjedec",  INFO(0, 0,  64 * 1024,  16, 0) },
        //{ "m25p16-nonjedec",  INFO(0, 0,  64 * 1024,  32, 0) },
        { "m25p32-nonjedec",  INFO(0, 0,  64 * 1024,  64, 0) },
        //{ "m25p64-nonjedec",  INFO(0, 0,  64 * 1024, 128, 0) },
        //{ "m25p128-nonjedec", INFO(0, 0, 256 * 1024,  64, 0) },

        { "m45pe10", INFO(0x204011,  0, 64 * 1024,    2, 0) },
        { "m45pe80", INFO(0x204014,  0, 64 * 1024,   16, 0) },
        { "m45pe16", INFO(0x204015,  0, 64 * 1024,   32, 0) },

        { "m25pe80", INFO(0x208014,  0, 64 * 1024, 16,       0) },
        { "m25pe16", INFO(0x208015,  0, 64 * 1024, 32, SECT_4K) },

        { "m25px32",    INFO(0x207116,  0, 64 * 1024, 64, SECT_4K) },
        { "m25px32-s0", INFO(0x207316,  0, 64 * 1024, 64, SECT_4K) },
        { "m25px32-s1", INFO(0x206316,  0, 64 * 1024, 64, SECT_4K) },
        { "m25px64",    INFO(0x207117,  0, 64 * 1024, 128, 0) },

        /* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
        { "w25x10", INFO(0xef3011, 0, 64 * 1024,  2,  SECT_4K) },
        { "w25x20", INFO(0xef3012, 0, 64 * 1024,  4,  SECT_4K) },
        { "w25x40", INFO(0xef3013, 0, 64 * 1024,  8,  SECT_4K) },
        { "w25x80", INFO(0xef3014, 0, 64 * 1024,  16, SECT_4K) },
        { "w25x16", INFO(0xef3015, 0, 64 * 1024,  32, SECT_4K) },
        { "w25x32", INFO(0xef3016, 0, 64 * 1024,  64, SECT_4K) },
        { "w25q32", INFO(0xef4016, 0, 64 * 1024,  64, SECT_4K) },
        { "w25x64", INFO(0xef3017, 0, 64 * 1024, 128, SECT_4K) },
        { "w25q64", INFO(0xef4017, 0, 64 * 1024, 128, SECT_4K) },
        { "w25q128", INFO(0xef4018, 0, 64 * 1024, 256, SECT_4K) },
        { "w25q256", INFO(0xef4019, 0, 64 * 1024, 512, SECT_4K) },

        /* Catalyst / On Semiconductor -- non-JEDEC */
        { },
};

struct flash_info    *info;

static int ca_spansion_cmd(u32 opcode, u32 addr, u32 data)
{
	u32 tmp;
	unsigned long timeout_jiffies;

	write_flash_ctrl_reg(FLASH_SF_ACCESS, opcode | g_chipen);
	write_flash_ctrl_reg(FLASH_SF_ADDRESS, addr);
	write_flash_ctrl_reg(FLASH_SF_DATA, data);
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

	timeout_jiffies = jiffies + msecs_to_jiffies(SPANSION_CMD_TIMEOUT);

	for (;;) {
		tmp = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		if ((tmp & SF_START_BIT_ENABLE) == 0) {
			return 0;
		}
		
		if (time_after(jiffies, timeout_jiffies)) {
			return -ETIME;
		}

		udelay(1);
		if(current->state == TASK_RUNNING)
		schedule();
	}
}
struct spi_device_id *ca_jedec_probe(void)
{
	u32 jedec, tmp, default_jedec;
        struct flash_info       *info;
	unsigned long status;

	if (ca_spansion_cmd(SFLASH_READID_OPCODE, 0, 0)) {
		return NULL;
	}

	status = read_flash_ctrl_reg(FLASH_SF_DATA);

	jedec = ((status&0xff)<<16) | (status&0xff00) | ((status&0xff0000)>>16);

	//For G2 just can read 4 bytesdata only, So without ext_jedec
        //ext_jedec = id[3] << 8 | id[4];

        for (tmp = 0; tmp < ARRAY_SIZE(m25p_ids) - 1; tmp++) {
                info = (void *)m25p_ids[tmp].driver_data;
                if (info->jedec_id == jedec) {
                        //if (info->ext_id != 0 && info->ext_id != ext_jedec)
                        //        continue;
                        return &m25p_ids[tmp];
                }
        }

        printk("%s : unrecognized JEDEC id %06x\n",__func__ , jedec);
	printk("use JEDEC id 0x0 (unknown) instead\n");
	default_jedec = 0x0;
	for (tmp = 0; tmp < ARRAY_SIZE(m25p_ids) - 1; tmp++) {
		info = (void *)m25p_ids[tmp].driver_data;
		if (info->jedec_id == default_jedec) {
			//if (info->ext_id != 0 && info->ext_id != ext_jedec)
			//        continue;
			return &m25p_ids[tmp];
		}
	}

        return NULL;
}


static int wait_flash_status(u32 flag, u32 value, u32 wait_msec)
{
	u32 status;
	unsigned long timeout_jiffies;

	timeout_jiffies = jiffies + msecs_to_jiffies(wait_msec);

	for (;;) {
		if (ca_spansion_cmd(SPANSION_READ_STATUS_OPCODE, 0, 0)) {
			return -ETIME;
		}

		status = read_flash_ctrl_reg(FLASH_SF_DATA);
		if ((status & flag) == value) {
			return 0;
		}

		if (time_after(jiffies, timeout_jiffies)) {
			return -ETIME;
		}

		udelay(1);
		if(current->state == TASK_RUNNING)
		schedule();
	}
}

/*
 * Enable/disable 4-byte addressing mode.
 */
int ca_set_4byte(u32 jedec_id, int enable)
{

	u32 tmp;

        switch (JEDEC_MFR(jedec_id)) {
        case CFI_MFR_MACRONIX:
               // flash->command[0] = enable ? OPCODE_EN4B : OPCODE_EX4B;
               // return spi_write(flash->spi, flash->command, 1);

               	if (ca_spansion_cmd(enable ? SFLASH_OPCODE_EN4B : SFLASH_OPCODE_EX4B, 0, 0)) {
			printk("%s : MACRONIX set 4 bytes error.\n",__func__);
		}
		break;
	case CFI_MFR_ST:
		/* Before issue EN4B or EX4B the WRITE ENABLE command must issued first. */
		if (ca_spansion_cmd(SPANSION_WRITE_ENABLE_OPCODE, 0, 0)) {
			return -ETIME;
		}

		if (wait_flash_status
		    (SPANSION_STATUS_WEL, SPANSION_STATUS_WEL, SPANSION_WEL_TIMEOUT)) {
			return -ETIME;
		}

		if (ca_spansion_cmd(enable ? SFLASH_OPCODE_EN4B : SFLASH_OPCODE_EX4B, 0, 0)) {
			printk("%s : Micron serial flash set 4 bytes error.\n",__func__);
		}

		break;
        default:
                /* Spansion style */
                //flash->command[0] = OPCODE_BRWR;
                //flash->command[1] = enable << 7;
                //return spi_write(flash->spi, flash->command, 2);
                if (ca_spansion_cmd(SFLASH_OPCODE_BRWR, 0, enable << 7)) {
			printk("%s : SFlash set 4 bytes error.\n",__func__);
		}
        }

        tmp = read_flash_ctrl_reg(FLASH_TYPE);
        if(enable)
        	sflash_type = (tmp | SFLASH_4BYTE_ADDR);
	else
		sflash_type = (tmp & ~SFLASH_4BYTE_ADDR);

		write_flash_ctrl_reg(FLASH_TYPE, sflash_type);


	return 0;
}

#ifdef	SPANSION_BLOCK_PROTECTION

static int protect_lower_bound(u32 status)
{
	u32 protect_lower;

	//for spansion uni-256KB sector size if uni-64kB or MX need to check 4bits(BP0 - BP3)
	//And base on TBPROT bit to decide BP starts at bottom or top
	protect_lower = (status & SPANSION_STATUS_BP) >> 2;
	if (protect_lower == 0) {
		return sflash_size;
	}
	else if (protect_lower == 0) {
		return 0;
	}
	else
		return (sflash_size / (1<<(7-protect_lower)) );

/**
	if (protect_lower == 0) {
		return 0xFFFFFF;
	}

	protect_lower = 0xFFFFFF * (0x01 << (protect_lower - 1)) / 64;
	protect_lower = 0xFFFFFF - protect_lower;
**/
	return protect_lower;
}

/* return 1 when it is writeable */
static int ca_s25fl128p_writeable(u32 addr, u32 len)
{
	u32 lower_bound;
	u32 status;

	ca_spansion_cmd(SPANSION_READ_STATUS_OPCODE, 0, 0);
	status = read_flash_ctrl_reg(FLASH_SF_DATA);
	if (status & SPANSION_STATUS_SRWD) {
		return 0;
	}

	lower_bound = protect_lower_bound(status);

	if (lower_bound <= addr) {
		return 0;
	}

	addr += len;

	if (lower_bound < addr) {
		return 0;
	}


	return 1;
}
#else
/* return 1 when it is writeable */
static int ca_s25fl128p_writeable(u32 addr, u32 len)
{
		return 1;
}
#endif



static int ca_s25fl128p_page_program(u32 addr, u8 * data)
{
	if (ca_s25fl128p_writeable(addr, 1) == 0) {
		return -EACCES;
	}

	if (ca_spansion_cmd(SPANSION_WRITE_ENABLE_OPCODE, 0, 0)) {
		return -ETIME;
	}

	if (wait_flash_status
	    (SPANSION_STATUS_WEL, SPANSION_STATUS_WEL, SPANSION_WEL_TIMEOUT)) {
		return -ETIME;
	}

	if (ca_spansion_cmd(SPANSION_PROGRAM_OPCODE, addr, *data)) {
		return -ETIME;
	}

	if (wait_flash_status(SPANSION_STATUS_WIP, 0, SPANSION_WEL_TIMEOUT)) {
		return -ETIME;
	}

	return 0;;
}



int ca_s25fl128p_page_dprogram(struct map_info *map,
					  unsigned long addr, const void *buf,
					  ssize_t len)
{
	u32 opcode;
	u8 *dst;

	if (ca_s25fl128p_writeable(addr, len) == 0) {
		return -EACCES;
	}

	if (ca_spansion_cmd(SPANSION_WRITE_ENABLE_OPCODE, 0, 0)) {
		return -ETIME;
	}

	if (wait_flash_status
	    (SPANSION_STATUS_WEL, SPANSION_STATUS_WEL, SPANSION_WEL_TIMEOUT)) {
		return -ETIME;
	}

  // Jason debug
	opcode = SPANSION_PROGRAM_OPCODE | g_chipen | SFLASH_FORCEBURST;
	//opcode = SPANSION_PROGRAM_OPCODE | g_chipen;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, opcode);
	/* Bug 27438
	   In a direct write cycle, the driver programs SF_ACCESS[sflashOpCode]
	   (through MPIF) and then starts the direct write cycle through AXI
	   interface.
	   It happens occasionally that the direct write cycle (AXI) kicks off
	   before the SF_ACCESS register write completed
	 */
	read_flash_ctrl_reg(FLASH_SF_ACCESS);	/* dummy read */
	wmb();
	dst = (u8 *) (((u8 *) map->virt) + addr);
	memcpy(dst, buf, len);

	opcode = g_chipen | SFLASH_FORCETERM;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, opcode);

	wait_flash_status(SPANSION_STATUS_WIP, 0, SPANSION_PROGRAM_TIMEOUT);

	return 0;
}


//this driver use BLOCK erase actually
static int ca_s25fl128p_sector_erase(u32 address)
{
	if (ca_spansion_cmd(SPANSION_WRITE_ENABLE_OPCODE, 0, 0)) {
		return MTD_ERASE_FAILED;
	}

	if (wait_flash_status
	    (SPANSION_STATUS_WEL, SPANSION_STATUS_WEL, SPANSION_WEL_TIMEOUT)) {
		return MTD_ERASE_FAILED;
	}

	//this driver use BLOCK erase actually
	if (ca_spansion_cmd(SPANSION_ERASE_OPCODE, address, 0)) {
		return MTD_ERASE_FAILED;
	}

	if (wait_flash_status(SPANSION_STATUS_WIP, 0, SPANSION_ERASE_TIMEOUT)) {
		return MTD_ERASE_FAILED;
	}

	return MTD_ERASE_DONE;
}


#ifdef	SPANSION_BLOCK_PROTECTION
int ca_spansion_islocked(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	return !ca_s25fl128p_writeable(ofs, len);
}

/* We implement lock/unlcok entire flash function only */
int ca_spansion_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	u32 flash_status;

	if (ca_spansion_islocked(mtd, 0, sflash_page_size) == 0) {
		return 0;
	}

	if (ca_spansion_cmd(SPANSION_READ_STATUS_OPCODE, 0, 0)) {
		return -ETIME;
	}
	flash_status = read_flash_ctrl_reg(FLASH_SF_DATA);

	if (flash_status & SPANSION_STATUS_SRWD) {
		return -EIO;
	}

	flash_status &= ~SPANSION_STATUS_BP;

	if (ca_spansion_cmd(SPANSION_WRITE_ENABLE_OPCODE, 0, 0)) {
		return -ETIME;
	}

	if (wait_flash_status
	    (SPANSION_STATUS_WEL, SPANSION_STATUS_WEL, SPANSION_WEL_TIMEOUT)) {
		return -ETIME;
	}

	if (ca_spansion_cmd(SPANSION_WRITE_STATUS_OPCODE, 0, flash_status)) {
		return -ETIME;
	}

	if (wait_flash_status(SPANSION_STATUS_BP, 0, SPANSION_WEL_TIMEOUT)) {
		return -ETIME;
	}

	return 0;

}
#endif

static int ca_spansion_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	uint64_t addr;
	uint64_t len;
	unsigned int block;
	unsigned int ret = 0;
	uint32_t rem;

	mutex_lock(&sflash_lock);

#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	down(&ca_flash_sem);
	write_flash_ctrl_reg(FLASH_TYPE, sflash_type);
#endif

	/* Sanity checks */
	if (instr->addr + instr->len > mtd->size) {

#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif
		mutex_unlock(&sflash_lock);
		return -EINVAL;
	}

	div_u64_rem(instr->addr, mtd->erasesize, &rem);
	if (rem) {
#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif
		mutex_unlock(&sflash_lock);
		return -EINVAL;
	}

	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem) {
#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif
		mutex_unlock(&sflash_lock);
		return -EINVAL;
	}



	if (ca_s25fl128p_writeable(instr->addr, mtd->erasesize) == 0) {
#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif
		mutex_unlock(&sflash_lock);
		return -EACCES;
	}

	addr = instr->addr;
	len = instr->len;


	while (len > 0) {
		block = addr / mtd->erasesize;
		ret = ca_s25fl128p_sector_erase(addr);
		if (ret == MTD_ERASE_FAILED) {
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr = ((loff_t) addr);
			goto erase_exit;
		}

		addr = addr + mtd->erasesize;
		len = len - mtd->erasesize;
		if(current->state == TASK_RUNNING)
		schedule();
	}
	instr->state = MTD_ERASE_DONE;

 erase_exit:

#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif
	mutex_unlock(&sflash_lock);
	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;

	if (!ret)
		mtd_erase_callback(instr);

	return (ret);
}


#ifdef	SPANSION_BLOCK_PROTECTION
/* We implement lock/unlcok entire flash function only */
int ca_spansion_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	u32 flash_status;
	if (ca_spansion_islocked(mtd, 0, sflash_page_size) == 1) {
		return 0;
	}

	if (ca_spansion_cmd(SPANSION_READ_STATUS_OPCODE, 0, 0)) {
		return -ETIME;
	}

	flash_status = read_flash_ctrl_reg(FLASH_SF_DATA);
	flash_status |= SPANSION_STATUS_BP;

	if (ca_spansion_cmd(SPANSION_WRITE_ENABLE_OPCODE, 0, 0)) {
		return -ETIME;
	}

	if (wait_flash_status
	    (SPANSION_STATUS_WEL, SPANSION_STATUS_WEL, SPANSION_WEL_TIMEOUT)) {
		return -ETIME;
	}

	if (ca_spansion_cmd(SPANSION_WRITE_STATUS_OPCODE, 0, flash_status)) {
		return -ETIME;
	}

	if (wait_flash_status
	    (SPANSION_STATUS_BP, SPANSION_STATUS_BP, SPANSION_WEL_TIMEOUT)) {
		return -ETIME;
	}

	return 0;
}
#endif

static void ca_spansion_nop(struct mtd_info *mtd)
{
	/* Nothing to see here */
}

int ca_spansion_write(struct mtd_info *mtd, loff_t to, size_t len,
				 size_t * retlen, const u_char * buf)
{
	u32 size, remain_length;
	struct map_info *map = (struct map_info *)mtd->priv;
    int i =0;
    
	mutex_lock(&sflash_lock);

#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	down(&ca_flash_sem);
	write_flash_ctrl_reg(FLASH_TYPE, sflash_type);
#endif
	/* sanity checks */
	if (len == 0 || (to + len > mtd->size)) {
		*retlen = 0;
#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif
		mutex_unlock(&sflash_lock);
		return 0;
	}

	if (ca_s25fl128p_writeable(to, len) == 0) {
#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif
		mutex_unlock(&sflash_lock);
		return -EACCES;
	}


	for (remain_length = len; remain_length > 0;) {
		size = sflash_page_size - ((u32)to % sflash_page_size);

		if (size > remain_length)
			size = remain_length;

		if(size == sflash_page_size) {
                    ca_s25fl128p_page_dprogram(map, to, buf, size);
		} else {
		for (i = 0; i < size; i++)
			ca_s25fl128p_page_program(to + i, buf + i);
                }

		remain_length -= size;
		buf += size;
		to += size;
	};
#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif

	*retlen = len;
	mutex_unlock(&sflash_lock);
	return 0;
}

static int ca_spansion_read(struct mtd_info *mtd, loff_t from, size_t len,
				size_t * retlen, u_char * buf)
{
	u32 opcode;
	u8 *src;
	FLASH_SF_EXT_ACCESS_t sf_ext_access;
	struct map_info *map = (struct map_info *)mtd->priv;
	mutex_lock(&sflash_lock);

#if   0  //defined( _SPANSION_INDIRECT_ACCESS_ )

	for (*retlen = 0; len > *retlen; ++buf, ++from, *retlen += 1) {
		if (ca_spansion_cmd(SPANSION_READ_OPCODE, from,
					SPANSION_ERASE_TIMEOUT)) {
			mutex_unlock(&sflash_lock);
			return 0;
		}

		*buf = read_flash_ctrl_reg(FLASH_SF_DATA) & 0xFF;
	};

#else
	
	opcode = g_chipen;

#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	down(&ca_flash_sem);
	write_flash_ctrl_reg(FLASH_TYPE, sflash_type);
#endif

	if(info->flags & QUAD_READ){
		opcode |= SPANSION_QREAD4B_OPCODE;
		sf_ext_access.wrd = read_flash_ctrl_reg(FLASH_SF_EXT_ACCESS);
		sf_ext_access.bf.sflashDummyCount = 7;
		write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);
	}
	write_flash_ctrl_reg(FLASH_SF_ACCESS, opcode);

	src = (u8 *) (((u8 *) map->virt) + (u32) from);

	memcpy(buf, src, len);

	if(info->flags & QUAD_READ){
		sf_ext_access.bf.sflashDummyCount = 0x1F;
		write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);
	}
	
	*retlen = len;

#ifdef CONFIG_MTD_CS752X_MULTIFLASH
	up(&ca_flash_sem);
#endif

#endif
	mutex_unlock(&sflash_lock);
	return 0;
}

static struct mtd_chip_driver ca_spansion_chipdrv = {
 probe:ca_map_serial_probe,
 name:	"ca_spansion_map_serial",
 module:THIS_MODULE
};

struct mtd_info *ca_map_serial_probe(struct map_info *map)
{
	struct mtd_info *mtd;

	mtd = kmalloc(sizeof(*mtd), GFP_KERNEL);
	if (!mtd) {
		printk("Serial flash allocation failed\n");
		return NULL;
	}

	mutex_init(&sflash_lock);

	memset(mtd, 0, sizeof(*mtd));

	map->fldrv = &ca_spansion_chipdrv;
	mtd->priv = map;
	mtd->name = map->name;
	mtd->type = MTD_NORFLASH;
	mtd->_erase = ca_spansion_erase;
	mtd->size = map->size;
	mtd->_read = ca_spansion_read;
	mtd->_write = ca_spansion_write;
	mtd->_sync = ca_spansion_nop;
	mtd->flags = MTD_CAP_NORFLASH;

#ifdef	SPANSION_BLOCK_PROTECTION
	mtd->_lock = ca_spansion_lock;
	mtd->_unlock = ca_spansion_unlock;
	mtd->_is_locked = ca_spansion_islocked;
#endif
	mtd->erasesize = sflash_sector_size;
	mtd->writesize = sflash_page_size;

	__module_get(THIS_MODULE);

	return mtd;
}

int init_ca_s25fl128p(struct platform_device *pdev)
{
	const struct spi_device_id *jid;
	struct mtd_part_parser_data ppdata;
	u32 tmp;
	int err;
	char *ptr;
	FLASH_TYPE_t reg_flash_type;



#ifdef CONFIG_CORTINA_FPGA
	write_flash_ctrl_reg(FLASH_SF_TIMING, 0x3000000);
#endif

	/* parsing serial flash type */
	ptr = strstr(saved_command_line, "sf_type");
	if (ptr) {
		ptr += strlen("sf_type") + 1;
		sflash_type = simple_strtoul(ptr, NULL, 0);
	}
	else
	{
		sflash_type = read_flash_ctrl_reg(FLASH_TYPE)& 0xffff;
		printk("Serial flash type : 0x%04x \n",sflash_type);
	}


	/* set flash type to serial flash */
	reg_flash_type.wrd = read_flash_ctrl_reg(FLASH_TYPE);
	write_flash_ctrl_reg(FLASH_TYPE, sflash_type);

	jid = ca_jedec_probe();

	if( jid == NULL )
			return -EIO;

	info = (void *)jid->driver_data;

	//flash = kmalloc(sizeof(struct spi_flash), GFP_KERNEL);
        //
	//if (!flash) {
	//	printk("%s : Failed to allocate memory\n",__func__);
	//	return -1;
	//}

	/*
         * Atmel, SST and Intel/Numonyx serial flash tend to power
         * up with the software protection bits set
         */
         printk("found %s.\n",jid->name);

        if (JEDEC_MFR(info->jedec_id) == CFI_MFR_ATMEL ||
            JEDEC_MFR(info->jedec_id) == CFI_MFR_INTEL ||
            JEDEC_MFR(info->jedec_id) == CFI_MFR_SST) {
              //  write_enable(flash);
              //  write_sr(flash, 0);
              printk("Atmel, Intel or SST SFlash.\n");
        }

	/* For S25-FL512SAIFG1 page size is 512B, 64MB with RESET# pin*/
	if(info->jedec_id == 0x010220)
		info->page_size = 512;

	sflash_sector_size = info->sector_size;
	sflash_page_size = info->page_size;
	sflash_size = info->sector_size * info->n_sectors; //this driver use BLOCK erase actually
	/* prefer "small sector" erase if possible */
        if (info->flags & SECT_4K) {
        	printk("Check OPCODE_BE_4K.\n");
                //flash->erase_opcode = OPCODE_BE_4K;
                //flash->mtd.erasesize = 4096;
        }

	/* enable 4-byte addressing if the device exceeds 16MiB */
	if (sflash_size > 0x1000000) {
		printk("4 Bytes address flash.\n");
		tmp = read_flash_ctrl_reg(FLASH_TYPE);
		sflash_type = (tmp | SFLASH_4BYTE_ADDR);
		write_flash_ctrl_reg(FLASH_TYPE, sflash_type);
                ca_set_4byte(info->jedec_id, 1);
	}

	/* Enable QUAD mode if support */
	if(info->flags & QUAD_READ){
			printk("Enable QUAD mode\n");
			ca_spansion_cmd(SPANSION_EN_QUAD_OPCODE, 0, 0);
	}

	s25fl128p_map.phys = sflash_mem_start_addr;
	s25fl128p_map.size = sflash_size;
	s25fl128p_map.virt = ioremap(sflash_mem_start_addr, sflash_size);

	printk("%s s25fl128p_map.virt %x sflash_mem_addr %x sflash_mem_size 0x%x\n", __func__, 
			s25fl128p_map.virt, sflash_mem_start_addr, sflash_size);

	if (!s25fl128p_map.virt) {
		printk(" failed to ioremap \n");
		return -EIO;
	}

	simple_map_init(&s25fl128p_map);
	ca_spansion_mtd =
	    do_map_probe("ca_spansion_map_serial", &s25fl128p_map);
	if (ca_spansion_mtd == NULL) {
		iounmap(s25fl128p_map.virt);
		printk("Serial flash probe failed\n");
		return -EIO;
	}

	ca_spansion_mtd->owner = THIS_MODULE;

	ppdata.of_node = pdev->dev.of_node;
	err = mtd_device_parse_register(ca_spansion_mtd, probe_types,
					&ppdata, NULL, 0);
	if (err) {
		dev_err(&pdev->dev, "failed to add partitions\n");
		return -EIO;
	}

	return 0;
}

void cleanup_ca_s25fl128p(void)
{
	if (ca_spansion_mtd) {
		mtd_device_unregister(ca_spansion_mtd);
		map_destroy(ca_spansion_mtd);
	}
}

#ifdef CONFIG_OF
static struct of_device_id ca_sflash_dt_ids[] = {
	{.compatible = "cortina,sflash",},
	{},
};

MODULE_DEVICE_TABLE(of, ca_sflash_dt_ids);
#endif

static int ca_sflash_probe(struct platform_device *pdev)
{
	int ret;
	struct resource mem_resource;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;

	/* search DT for a match */
	match = of_match_device(ca_sflash_dt_ids, &pdev->dev);
	if (!match)
		return -EINVAL;

	dev_notice(&pdev->dev, "Cortina-Access Serial Flash driver\n");

	ret = of_address_to_resource(np, 0, &mem_resource);
	if (ret) {
		dev_warn(&pdev->dev, "invalid address %d\n", ret);
		return ret;
	}

	flash_ctl_base = devm_ioremap(&pdev->dev, mem_resource.start,
			  resource_size(&mem_resource));

	ret = of_property_read_u32(np,"sflash_mem_addr", &sflash_mem_start_addr);
	if(ret){
		dev_warn(&pdev->dev, "Not found the sflash_mem_addr in dtb, ret = %d\n", ret);
		return ret;
	}

    ret =  of_property_read_u32(np,"sflash_mem_size", &sflash_mem_size);
	if(ret){
		dev_warn(&pdev->dev, "Not found the sflash_mem_size in dtb, ret = %d\n", ret);
		return ret;
	}


	printk("Serial flash: indirect/direct access mode\n");

	register_mtd_chip_driver(&ca_spansion_chipdrv);
	return init_ca_s25fl128p(pdev);
}

static int ca_sflash_remove(struct platform_device *op)
{
	cleanup_ca_s25fl128p();
	unregister_mtd_chip_driver(&ca_spansion_chipdrv);
	return 0;
}
	
static struct platform_driver ca_sflash_driver = {
	.probe = ca_sflash_probe,
	.remove = ca_sflash_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "ca_sflash",
		   .of_match_table = of_match_ptr(ca_sflash_dt_ids),
		   },
};

static int __init ca_slfash_init(void)
{
	return platform_driver_register(&ca_sflash_driver);
}

static void __exit ca_sflash_exit(void)
{
	platform_driver_unregister(&ca_sflash_driver);
}

module_init(ca_slfash_init);
module_exit(ca_sflash_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Li <jason.li@cortina-access.com>");
MODULE_DESCRIPTION("MTD map driver for Cortina Soc");
