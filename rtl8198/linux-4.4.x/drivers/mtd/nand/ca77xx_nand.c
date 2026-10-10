/*
 * Copyright (c) 2016, Cortina Access. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/nand_bch.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/mtd/partitions.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/sizes.h>
#include <soc/cortina/ca_flash.h>
#include "ca77xx_nand_protect.h"


#ifndef CONFIG_CA77XX_SHOW_INFO
#define CONFIG_CA77XX_SHOW_INFO 0
#endif

static int check_blank(uint32_t *end_pos, uint32_t *start_pos);
static void show_page(uint8_t *buf, int32_t len);
static int blank_check(uint8_t *buf, int32_t eccsteps, int32_t eccsize, uint8_t* ecc_buf_ptr, uint32_t *eccpos, int32_t eccbytes);

static int
check_blank(uint32_t *end_pos, uint32_t *start_pos)
{
	do {
		if (*start_pos != 0xFFFFFFFF) {
			return 1;
		}
	} while (++start_pos != end_pos);
	return 0;
}

#define SHOW_DATA_LEN 16
static void
show_page(uint8_t *buf, int32_t len)
{
	int i;
	printk("\n");
	for ( i = 0; i < len; i++)
	{
		if (0 == i%SHOW_DATA_LEN){
			printk("\n%04X: ", i);
		}
		printk("%02X ", buf[i]);
	}
	printk("\n--------\n");
	printk("\n");
}

#ifndef CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE
#define CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE 3
#endif /* ifndef CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE */
static int
blank_check(uint8_t *buf, int32_t eccsteps, int32_t eccsize, uint8_t* ecc_buf_ptr, uint32_t *eccpos, int32_t eccbytes)
{
	uint32_t error_bit_n;
	uint32_t val;
	uint8_t val8;
	uint32_t *start_pos, *end_pos;
	uint32_t *idx_start_pos, *idx_end_pos;
	int ret = 0;

	for (start_pos = (uint32_t*)buf, end_pos = (uint32_t*)(buf+eccsize);
			eccsteps;
			end_pos = start_pos+(eccsize>>2), eccsteps--) {
		error_bit_n = 0;
		// check bit flip in syndrome
		if (NULL != ecc_buf_ptr) {
			idx_start_pos = eccpos;
			idx_end_pos = eccpos+eccbytes;
			do {
				val8 = 0xFF ^ ecc_buf_ptr[*idx_start_pos];
				while (val8) {
					error_bit_n += 1;
					if (error_bit_n > CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE) {
						return -1;
					}
					val8 &= val8 - 1;
				}
			} while (++idx_start_pos != idx_end_pos );
			eccpos = idx_end_pos;
		}

		// check bit flip in page data
		do {
			val = 0xFFFFFFFF ^ (*start_pos);
			while (val) {
				error_bit_n += 1;
				if (error_bit_n > CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE) {
					return -1;
				}
				val &= val - 1;
			}
		} while (++start_pos != end_pos);
		ret = error_bit_n>ret ? error_bit_n : ret;
	}
	return ret;
}
#undef SHOW_DATA_LEN
void
process_trace(struct task_struct *curr)
{
	struct task_struct *t = curr;
	printk("---- real parent ----\n");
	while (NULL != t) {
		printk("=> %d %s\n", t->pid, t->comm);
		if (t == t->real_parent) {
			break;
		}
		t = t->real_parent;
	}
	printk("---- parent ----\n");
	t = curr;
	while (NULL != t) {
		printk("=> %d %s\n", t->pid, t->comm);
		if (t == t->parent) {
			break;
		}
		t = t->parent;
	}
	printk("---- end ----\n");
}


static inline int
chkAddrValid(struct mtd_info *mtd, const int *page_id)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int blk_shift = (this->phys_erase_shift - this->page_shift);
	int blk_num = (*page_id)>>blk_shift;
	if (-1 == (*page_id)) {
		return 0;
	}
	if (blk_num < ca77xx_nand_get_blk_protection_top()) {
		printk("[CA77XX_NAND] process %i (%s) \n",  current->pid, current->comm);
		printk("[CA77XX_NAND] Operation on wrong block number %d (blocks number < %d are protected)\n", blk_num, ca77xx_nand_get_blk_protection_top());
		dump_stack();
		process_trace(current);
		return -1;
	}
	return 0;
}



/* Define default oob placement schemes for large and small page devices */
#if 0
static struct nand_ecclayout nand_oob_8 = {
	.eccbytes = 3,
	.eccpos = {0, 1, 2},
	.oobfree = {
				{.offset = 3,
				 .length = 2},
				{.offset = 6,
				 .length = 2}}
};

static struct nand_ecclayout nand_oob_16 = {
	.eccbytes = 6,
	.eccpos = {0, 1, 2, 3, 6, 7},
	.oobfree = {
				{.offset = 8,
				 .length = 8}}
};

static struct nand_ecclayout nand_oob_64 = {
	.eccbytes = 24,
	.eccpos = {
			   40, 41, 42, 43, 44, 45, 46, 47,
			   48, 49, 50, 51, 52, 53, 54, 55,
			   56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {
				{.offset = 2,
				 .length = 38}}
};

static struct nand_ecclayout nand_oob_128 = {
	.eccbytes = 48,
	.eccpos = {
			   80, 81, 82, 83, 84, 85, 86, 87,
			   88, 89, 90, 91, 92, 93, 94, 95,
			   96, 97, 98, 99, 100, 101, 102, 103,
			   104, 105, 106, 107, 108, 109, 110, 111,
			   112, 113, 114, 115, 116, 117, 118, 119,
			   120, 121, 122, 123, 124, 125, 126, 127},
	.oobfree = {
				{.offset = 2,
				 .length = 78}}
};
#endif

/* Cortina added funtion ==> */
static struct nand_ecclayout ca_nand_bch_oob_16 = {
	.eccbytes = 14,
	.eccpos = {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14},
	.oobfree = {
				{.offset = 15,
				 /*  . length = 1}} // resever 1 for erase tags: 1 - 1 = 0 */
				 .length = 0}}	// resever 1 for erase tags: 1 - 1 = 0
};

static struct nand_ecclayout ca_nand_ecclayout;

void static __iomem *flash_ctl_base;
void static __iomem *flash_mem_base;
void static __iomem *dma_ctl_glb_base;
void static __iomem *dma_ctl_base;

struct ca77xx_nand_host *ca77xx_host;
static int nand_page = 0, nand_col = 0;
static volatile int dummy;
static int force_ace=0;

static DMA_SSP_TX_DESC_T *tx_desc;
static DMA_SSP_RX_DESC_T *rx_desc;
static unsigned int CHIP_EN;
static unsigned int *pread, *pwrite;
static FLASH_TYPE_t flash_type;
static FLASH_STATUS_t flash_status;
static FLASH_NF_ACCESS_t nf_access;
static FLASH_NF_COUNT_t nf_cnt;
static FLASH_NF_COMMAND_t nf_cmd;
static FLASH_NF_ADDRESS_1_t nf_addr1;
static FLASH_NF_ADDRESS_2_t nf_addr2;
static FLASH_NF_ECC_STATUS_t ecc_sts;
static FLASH_NF_ECC_CONTROL_t ecc_ctl;
static FLASH_NF_ECC_OOB_t ecc_oob;
static FLASH_NF_ECC_GEN0_t ecc_gen0;
static FLASH_FIFO_CONTROL_t fifo_ctl;
static FLASH_FLASH_ACCESS_START_t flash_start;
static FLASH_NF_ECC_RESET_t ecc_reset;
static FLASH_FLASH_INTERRUPT_t flash_int_sts;
static FLASH_NF_BCH_STATUS_t bch_sts;
static FLASH_NF_BCH_ERROR_LOC01_t bch_err_loc01;
static FLASH_NF_BCH_CONTROL_t bch_ctrl;
static FLASH_NF_BCH_OOB0_t bch_oob0;
static FLASH_NF_BCH_GEN0_0_t bch_gen00;
static uint8_t *tmp_buf;

DMA_SEC_DMA_GLB_DMA_SSP_RX_CTRL_t dma_rxdma_ctrl;
DMA_SEC_DMA_GLB_DMA_SSP_TX_CTRL_t dma_txdma_ctrl;
DMA_SEC_DMA_SSP_Q_TXQ_CONTROL_t dma_txq5_ctrl;
DMA_SEC_DMA_SSP_Q_RXQ_BASE_DEPTH_t dma_rxq5_base_depth;
DMA_SEC_DMA_SSP_Q_RXQ_RPTR_t dma_rxq5_rptr;
DMA_SEC_DMA_SSP_Q_TXQ_BASE_DEPTH_t dma_txq5_base_depth;
DMA_SEC_DMA_SSP_Q_TXQ_WPTR_t dma_txq5_wptr;
DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT_t dma_ssp_txq5_intsts;
DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT_t dma_ssp_rxq5_intsts;

static int nand_get_device(struct mtd_info *mtd, int new_state);

 int ca_nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr,
							  int allowbbt);
 void check_flash_ctrl_status(void);
 void ca_nand_wait_ready(struct mtd_info *mtd);
 int ca_nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
								int page);
 int ca_nand_do_write_oob(struct mtd_info *mtd, loff_t to,
								struct mtd_oob_ops *ops);


#ifdef CONFIG_NAND_HIDE_BAD
#define BAD_BLOCK_LIST_ELEMENT	128
#define BBL_INDICATORS          4
#define CONFIG_BBL_OFFSET         (0x180000)  /* 1.5MB */
#define CONFIG_BBL_OFFSET_REDUND  (0x1C0000)
#define BBL0_NAME 0x42626c30 //'Bbl0'
#define BBL1_NAME 0x316C6242 //'1lbB'

 enum {
     FACTORY_1ST_SCAN = 1,
     FACTORY_FROM_BBT0= 2,
     FACTORY_FROM_BBT1= 3,
 };
 static u8 bbl_source=0;
 static u8 acc_phy_offset=0;
 static u8 bad_block_cnt=0;
 static uint32_t bad_block_list[BAD_BLOCK_LIST_ELEMENT];

 static int ca_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip);

 /* "crc16_tab & cyg_crc16" are Inherited from u-boot-2015.04 */
 /* Table of CRC constants - implements x^16+x^12+x^5+1 */
 static const uint16_t crc16_tab[] = {
     0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
     0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
     0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
     0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
     0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
     0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
     0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
     0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
     0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
     0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
     0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
     0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
     0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
     0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
     0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
     0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
     0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
     0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
     0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
     0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
     0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
     0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
     0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
     0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
     0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
     0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
     0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
     0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
     0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
     0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
     0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
     0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
 };
 static uint16_t cyg_crc16(unsigned char *buf, int len)
 {
     int i;
     uint16_t cksum;

     cksum = 0;
     for (i = 0;  i < len;  i++) {
         cksum = crc16_tab[((cksum>>8) ^ *buf++) & 0xFF] ^ (cksum << 8);
     }
     return cksum;
 }

 static void get_nand_factory_bad_blocks(struct mtd_info *mtd, uint32_t start, uint32_t size)
 {
     struct nand_chip *chip = mtd->priv;

     /* Check factory bad stored in flash chip:
          * 1) Read Bbl0 if the factory bad had already been scanned
          * 2) If Bbl0 is bad. Read Bbl1 for the factory bad
          */
     loff_t offset;
     uint32_t ret=0;
     size_t retlen;
     uint32_t indicator[BBL_INDICATORS];
     uint32_t tempread[BAD_BLOCK_LIST_ELEMENT];
     memset(indicator, 0xF, sizeof(indicator));
     memset(tempread, 0xF, sizeof(tempread));

     acc_phy_offset = 1;
     for(offset=CONFIG_BBL_OFFSET ; offset < chip->chipsize ; offset += mtd->erasesize){
         ret = ca_nand_block_bad(mtd, offset, 1);
         if(ret == 1) continue;
         mtd->_read(mtd, offset, sizeof(indicator), &retlen, (u8 *)indicator);

         if(((indicator[0] == BBL0_NAME) || (indicator[0] == BBL1_NAME)) && (indicator[2] == indicator[3])){
             ret = mtd->_read(mtd, offset+mtd->writesize, sizeof(tempread), &retlen, (u8 *)tempread);
             if(indicator[2] == cyg_crc16((u8 *)tempread, sizeof(tempread))){
                 bad_block_cnt = indicator[1];
                 bbl_source = (indicator[0]==BBL0_NAME)?FACTORY_FROM_BBT0:FACTORY_FROM_BBT1;
                 memcpy(bad_block_list, tempread, sizeof(bad_block_list));
                 acc_phy_offset = 0;
                 return;
             }
         }
     }

     /* If the BBLn is not read from flash, scan the BBI in OOB */
     offset = start;
     bbl_source = FACTORY_1ST_SCAN;
     bad_block_cnt = 0;
     memset(bad_block_list, 0xF, sizeof(bad_block_list));
     while(((offset - start) < size) && (bad_block_cnt<BAD_BLOCK_LIST_ELEMENT)) {
          ret = ca_nand_block_bad(mtd, offset, 1);
          if(ret == 1){
             bad_block_list[bad_block_cnt++] = offset;
         }
         offset += mtd->erasesize;
     }
     mtd->size -= bad_block_cnt*mtd->erasesize;
     acc_phy_offset = 0;
    return;
}

static int xlat_offset_to_skip_factory_bad(struct mtd_info *mtd, uint32_t *page_addr)
{
    struct nand_chip *chip = mtd->priv;
    uint32_t i;
    uint32_t offset;
    /* Just return original offset:
            1. if no bad block found
            2. Physical flash offset access is assigned */
    if((bad_block_cnt==0) || (acc_phy_offset==1)){
        return 0;
    }

    offset = *page_addr<<chip->page_shift;
    for(i=0; i<bad_block_cnt; i++){
        if(bad_block_list[i] <= offset){
            offset += mtd->erasesize;
        }else{
            break;
        }
    }
    if(offset >= chip->chipsize){
        return -EINVAL;
    }
    *page_addr = offset>>chip->page_shift;
    return 0;
}
#endif /* #ifdef CONFIG_NAND_HIDE_BAD */

 void check_flash_ctrl_status(void)
{
	unsigned long timeo;

	timeo = jiffies + HZ;
	do {
		flash_status.wrd = read_flash_ctrl_reg(FLASH_STATUS);
		if (!flash_status.bf.nState)
			return;
	} while (time_before(jiffies, timeo));

	printk("FLASH_STATUS ERROR: %x\n", flash_status.wrd);

}

/*
 * Wait for the ready pin, after a command
 * The timeout is catched later.
 */
 void ca_nand_wait_ready(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	unsigned long timeo = jiffies + 2;

	/* wait until command is processed or timeout occures */
	do {
		if (this->dev_ready(mtd))
			return;
		touch_softlockup_watchdog();
	} while (time_before(jiffies, timeo));
}

 void ca_rst_ecc_bch_registers(void)
{
	ecc_reset.wrd = 3;
	ecc_reset.bf.eccClear = ECC_CLR;
	ecc_reset.bf.fifoClear = FIFO_CLR;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);

	flash_int_sts.bf.regIrq = 1;
	write_flash_ctrl_reg(FLASH_FLASH_INTERRUPT, flash_int_sts.wrd);

	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = 1;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);

	/*  Disable ECC function */
	write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, 0);
	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0);

}

/**
 * ca_nand_read_oob_std - [REPLACABLE] the most common OOB data read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to read
 */
 int ca_nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
								int page)
{
	int i;

	check_flash_ctrl_status();

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */

	nf_cmd.wrd = 0;
	nf_addr1.wrd = 0;
	nf_addr2.wrd = 0;
	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = mtd->oobsize - 1;
	nf_cnt.bf.nflashRegDataCount = NCNT_EMPTY_DATA;

	if (chip->chipsize < (32 << 20)) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_3;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;
		if (mtd->writesize > NCNT_512P_DATA)
			nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READ0;
		else
			nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READOOB;

		nf_addr1.wrd = ((page & 0x00ffffff) << 8);
		nf_addr2.wrd = ((page & 0xff000000) >> 24);
	} else if ((chip->chipsize >= (32 << 20))
			   && (chip->chipsize <= (128 << 20))) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_4;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READ0;

		/*  Jeneng */
		if (mtd->writesize > NCNT_512P_DATA) {
			nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
			nf_cmd.bf.nflashRegCmd1 = NAND_CMD_READSTART;
		}
		nf_addr1.wrd = (((page & 0xffff) << 16) + (mtd->writesize & 0xffff));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);

	} else {					/* if((chip->chipsize > (128 << 20)) )) */

		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_5;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READ0;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_READSTART;
		nf_addr1.wrd = (((page & 0xffff) << 16) + (mtd->writesize & 0xffff));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);
	}

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x0 */
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);

	pread = (unsigned int *)chip->oob_poi;

	for (i = 0; i < mtd->oobsize / 4; i++) {
		nf_access.wrd = 0;
		nf_access.bf.nflashCeAlt = CHIP_EN;
		/* nf_access.bf.nflashDirWr = ; */
		nf_access.bf.nflashRegWidth = NFLASH_WiDTH32;
		write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

		flash_start.wrd = 0;
		flash_start.bf.nflashRegReq = FLASH_GO;
		flash_start.bf.nflashRegCmd = FLASH_RD;
		/* flash_start.bf.nflash_random_access = RND_ENABLE; */
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		while (flash_start.bf.nflashRegReq) {
			udelay(1);
			schedule();
			flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		}
		pread[i] = read_flash_ctrl_reg(FLASH_NF_DATA);

	}

	if (chip->oob_poi[chip->badblockpos] != 0xff)
			mtd->ecc_stats.failed++;

	return 0;
}

/**
 * ca_nand_write_oob_std - [REPLACABLE] the most common OOB data write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to write
 */
 int ca_nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
								 int page)
{
	int status = 0, i;

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	check_flash_ctrl_status();

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);
	/* chip->write_buf(mtd, buf, length); */
	/* Send command to program the OOB data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */

	nf_cmd.wrd = 0;
	nf_addr1.wrd = 0;
	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = mtd->oobsize - 1;
	nf_cnt.bf.nflashRegDataCount = NCNT_EMPTY_DATA;

	nf_addr2.wrd = 0;

	if (chip->chipsize < SZ_32M) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_3;

		if (mtd->writesize > NCNT_512P_DATA) {
			nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
			nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
			nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		} else {
			nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_3;
			nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READOOB;	/* ?? */
			nf_cmd.bf.nflashRegCmd1 = NAND_CMD_SEQIN;
			nf_cmd.bf.nflashRegCmd2 = NAND_CMD_PAGEPROG;
		}
		/* read oob need to add page data size to match correct oob ddress */
		nf_addr1.wrd = (((page & 0x00ffffff) << 8));
		nf_addr2.wrd = ((page & 0xff000000) >> 24);
	} else if (chip->chipsize <= SZ_128M) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_4;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		nf_addr1.wrd = (((page & 0xffff) << 16) + (mtd->writesize & 0xffff));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);

	} else {					/* if((chip->chipsize > (128 << 20)) )) */

		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_5;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		nf_addr1.wrd = (((page & 0xffff) << 16) + (mtd->writesize & 0xffff));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);

	}

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x0 */
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);

	pwrite = (unsigned int *)chip->oob_poi;

	for (i = 0; i < ((mtd->oobsize / 4)); i++) {
		nf_access.wrd = 0;
		nf_access.bf.nflashCeAlt = CHIP_EN;
		/* nf_access.bf.nflashDirWr = ; */
		nf_access.bf.nflashRegWidth = NFLASH_WiDTH32;
		write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

		write_flash_ctrl_reg(FLASH_NF_DATA, pwrite[i]);

		flash_start.wrd = 0;
		flash_start.bf.nflashRegReq = FLASH_GO;
		flash_start.bf.nflashRegCmd = FLASH_WT;
		/* flash_start.bf.nflash_random_access = RND_ENABLE; */
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		while (flash_start.bf.nflashRegReq) {
			udelay(1);
			schedule();
			flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		}
		/* pwrite[i] = read_flash_ctrl_reg(FLASH_NF_DATA); */

	}

	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;

}

 void ca_nand_read_id(int chip_no, unsigned char *id)
{
	unsigned int opcode, i;
	const unsigned int extid = 8;

	check_flash_ctrl_status();

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */

	/* need to check extid byte counts */
	nf_cnt.wrd = 0;

	nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
	nf_cnt.bf.nflashRegDataCount = NCNT_DATA_8;
	nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_1;
	nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);

	nf_cmd.wrd = 0;
	nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READID;
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	nf_addr1.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x00 */
	nf_addr2.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);	/* write address 0x00 */

	/* read maker code */
	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = chip_no;
	/* nf_access.bf.nflashDirWr = ; */
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	for (i = 0; i < extid; i++) {

		flash_start.wrd = 0;
		flash_start.bf.nflashRegReq = FLASH_GO;
		flash_start.bf.nflashRegCmd = FLASH_RD;
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		while (flash_start.bf.nflashRegReq) {
			udelay(1);
			schedule();
			flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		}

		opcode = read_flash_ctrl_reg(FLASH_NF_DATA);
		id[i] = (opcode >> ((i << 3) % 32)) & 0xff;
	}

	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = ECC_CLR;
	ecc_reset.bf.fifoClear = FIFO_CLR;
	ecc_reset.bf.nflash_reset = NF_RESET;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);
}

 void ca_nand_onfi_id(int chip_no, char *sig)
{
	unsigned int opcode, i;

	check_flash_ctrl_status();

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */

	/* need to check extid byte counts */
	nf_cnt.wrd = 0;

	nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
	nf_cnt.bf.nflashRegDataCount = NCNT_DATA_4;
	nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_1;
	nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);

	nf_cmd.wrd = 0;
	nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READID;
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	nf_addr1.wrd = 0x20;		/* subcommand 0x20 for ONFI signature */
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x00 */
	nf_addr2.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);	/* write address 0x00 */

	/* read maker code */
	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = chip_no;
	/* nf_access.bf.nflashDirWr = ; */
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	for (i = 0; i < 4; i++) {

		flash_start.wrd = 0;
		flash_start.bf.nflashRegReq = FLASH_GO;
		flash_start.bf.nflashRegCmd = FLASH_RD;
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		while (flash_start.bf.nflashRegReq) {
			udelay(1);
			schedule();
			flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		}

		opcode = read_flash_ctrl_reg(FLASH_NF_DATA);
		sig[i] = (opcode >> ((i << 3) % 32)) & 0xff;
	}

	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = ECC_CLR;
	ecc_reset.bf.fifoClear = FIFO_CLR;
	ecc_reset.bf.nflash_reset = NF_RESET;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);
}
void ca_nand_jedec_id(int chip_no, char *sig)
{
	unsigned int opcode, i;

	check_flash_ctrl_status();

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */

	/* need to check extid byte counts */
	nf_cnt.wrd = 0;

	nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
	nf_cnt.bf.nflashRegDataCount = NCNT_DATA_5;
	nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_1;
	nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);

	nf_cmd.wrd = 0;
	nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READID;
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	nf_addr1.wrd = 0x40;		/* subcommand 0x20 for ONFI signature */
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x00 */
	nf_addr2.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);	/* write address 0x00 */

	/* read maker code */
	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = chip_no;
	/* nf_access.bf.nflashDirWr = ; */
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	for (i = 0; i < 5; i++) {

		flash_start.wrd = 0;
		flash_start.bf.nflashRegReq = FLASH_GO;
		flash_start.bf.nflashRegCmd = FLASH_RD;
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		while (flash_start.bf.nflashRegReq) {
			udelay(1);
			schedule();
			flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		}

		opcode = read_flash_ctrl_reg(FLASH_NF_DATA);
		sig[i] = (opcode >> ((i << 3) % 32)) & 0xff;
	}

	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = ECC_CLR;
	ecc_reset.bf.fifoClear = FIFO_CLR;
	ecc_reset.bf.nflash_reset = NF_RESET;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);
}

void ca_nand_get_onfi_param(int chip_no, struct nand_onfi_params *onfi_param)
{
	unsigned int opcode, i;
	unsigned char *p = (unsigned char *)onfi_param;

	check_flash_ctrl_status();

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */

	/* need to check extid byte counts */
	nf_cnt.wrd = 0;

	nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
	nf_cnt.bf.nflashRegDataCount = sizeof(*onfi_param) - 1;
	nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_1;
	nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);

	nf_cmd.wrd = 0;
	nf_cmd.bf.nflashRegCmd0 = NAND_CMD_PARAM;
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	nf_addr1.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x00 */
	nf_addr2.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);	/* write address 0x00 */

	/* read maker code */
	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = chip_no;
	/* nf_access.bf.nflashDirWr = ; */
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	for (i = 0; i < sizeof(*onfi_param); i++) {

		flash_start.wrd = 0;
		flash_start.bf.nflashRegReq = FLASH_GO;
		flash_start.bf.nflashRegCmd = FLASH_RD;
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		while (flash_start.bf.nflashRegReq) {
			udelay(1);
			schedule();
			flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		}

		opcode = read_flash_ctrl_reg(FLASH_NF_DATA);
		p[i] = (opcode >> ((i << 3) % 32)) & 0xff;
	}

	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = ECC_CLR;
	ecc_reset.bf.fifoClear = FIFO_CLR;
	ecc_reset.bf.nflash_reset = NF_RESET;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);
}

void ca_nand_get_jedec_param(int chip_no, struct nand_jedec_params *jedec_param)
{
	unsigned int opcode, i;
	unsigned char *p = (unsigned char *)jedec_param;

	check_flash_ctrl_status();

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */

	/* need to check extid byte counts */
	nf_cnt.wrd = 0;

	nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
	nf_cnt.bf.nflashRegDataCount = sizeof(*jedec_param) - 1;
	nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_1;
	nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);

	nf_cmd.wrd = 0;
	nf_cmd.bf.nflashRegCmd0 = NAND_CMD_PARAM;
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	nf_addr1.wrd = 0x40;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x00 */
	nf_addr2.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);	/* write address 0x00 */

	/* read maker code */
	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = chip_no;
	/* nf_access.bf.nflashDirWr = ; */
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	for (i = 0; i < sizeof(*jedec_param); i++) {

		flash_start.wrd = 0;
		flash_start.bf.nflashRegReq = FLASH_GO;
		flash_start.bf.nflashRegCmd = FLASH_RD;
		write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		while (flash_start.bf.nflashRegReq) {
			udelay(1);
			schedule();
			flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		}

		opcode = read_flash_ctrl_reg(FLASH_NF_DATA);
		p[i] = (opcode >> ((i << 3) % 32)) & 0xff;
	}

	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = ECC_CLR;
	ecc_reset.bf.fifoClear = FIFO_CLR;
	ecc_reset.bf.nflash_reset = NF_RESET;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);
}

 void fill_bch_oob_data(struct nand_chip *chip)
{
	u32 i, j;
	u32 *eccpos = chip->ecc.layout->eccpos;
	int eccsteps = chip->ecc.steps;

	FLASH_NF_BCH_GEN0_0_t bch_gen00;
	const u32 reg_offset = FLASH_NF_BCH_GEN0_1 - FLASH_NF_BCH_GEN0_0;
	const u32 group_offset = FLASH_NF_BCH_GEN1_0 - FLASH_NF_BCH_GEN0_0;

	u32 addr = FLASH_NF_BCH_GEN0_0;
	u32 *ecc_end_pos;

	for (; eccsteps; --eccsteps, addr += group_offset) {
		ecc_end_pos = eccpos + chip->ecc.bytes;

		for (i = 0; eccpos != ecc_end_pos; ++i) {
			bch_gen00.wrd = read_flash_ctrl_reg(addr + reg_offset * i);

			for (j = 0; j < 4 && eccpos != ecc_end_pos; ++j, ++eccpos) {
				chip->oob_poi[*eccpos] = ((bch_gen00.wrd >> (j * 8)) & 0xff);
			}
		}
	}

	/*  erase tag */
	*(chip->oob_poi + chip->ecc.layout->oobfree[0].offset +
	  chip->ecc.layout->oobfree[0].length) = 0;
}

 void fill_hamming_oob_data(struct nand_chip *chip, struct mtd_info *mtd)
{
	u32 i, j;
	u32 *eccpos = chip->ecc.layout->eccpos;
	int eccsteps = chip->ecc.steps;
	int eccbytes = chip->ecc.bytes;

	for (i = 0, j = 0; eccsteps; eccsteps--, i++, j += eccbytes) {
#ifdef	NAND_ECC_TEST
		get_cpu();
		if (chip->ecc.size == 256) {
			memset(eccode, 0, 32);
			/* printk("%s : ecc 256 addr %p ",__func__,&tw[i*256]); */
			nand_calculate_256_ecc(&tw[i * 256], &eccode[0]);
			nand_calculate_ecc(mtd, &tw[i * 256], &eccode[3]);
			eccdata = 0;
			eccdata = eccode[0] | eccode[1] << 8 | eccode[2] << 16;
			ecc_gen0.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_GEN0 + 4 * i);
			if (ecc_gen0.wrd != eccdata)
				printk
					(" i (%x)HWecc error ecc_gen0.wrd(%x)  eccdata (%x)!\n",
					 i, ecc_gen0.wrd, eccdata);
		} else {
			memset(eccode, 0, 32);
			/* printk("%s : ecc 512 addr %p ",__func__,&tw[i*512]); */
			nand_calculate_512_ecc(mtd, &tw[i * 512], &eccode[0]);

			eccdata = 0;
			eccdata = eccode[0] | eccode[1] << 8 | eccode[2] << 16;
			ecc_gen0.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_GEN0 + 4 * i);
			if (ecc_gen0.wrd != eccdata)
				printk
					(" i (%x)HWecc error ecc_gen0.wrd(%x)  eccdata (%x)!\n",
					 i, ecc_gen0.wrd, eccdata);
		}
		put_cpu();
#endif							/* NAND_ECC_TEST */

		ecc_gen0.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_GEN0 + 4 * i);
		chip->oob_poi[eccpos[j]] = ecc_gen0.wrd & 0xff;	/*  */
		chip->oob_poi[eccpos[j + 1]] = (ecc_gen0.wrd >> 8) & 0xff;
		chip->oob_poi[eccpos[j + 2]] = (ecc_gen0.wrd >> 16) & 0xff;
		/*  printk("%x ", ecc_gen0.wrd);                 */
	}
}
 int bch_correct(struct mtd_info *mtd, struct nand_chip *chip,
					   uint8_t * buf)
{
	int i, j, k, bitflips = 0;
	int eccsteps, eccbytes, eccsize;
	uint32_t *eccpos, *ecc_end_pos;
	uint8_t *p = buf;
	int32_t b_ret;

	eccsteps = chip->ecc.steps;
	eccbytes = chip->ecc.bytes;
	eccsize = chip->ecc.size;
	eccpos = chip->ecc.layout->eccpos;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		ecc_end_pos = eccpos + chip->ecc.bytes;
		for (j = 0; j < chip->ecc.bytes; j += 4) {
			bch_oob0.wrd = 0;
			for (k = 0; k < 4 && eccpos != ecc_end_pos; ++k, ++eccpos) {
				bch_oob0.wrd |= chip->oob_poi[*eccpos] << (8 * k);
			}

			write_flash_ctrl_reg(FLASH_NF_BCH_OOB0 + j, bch_oob0.wrd);
		}

		/* enable ecc compare */
		bch_ctrl.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_CONTROL);
		bch_ctrl.bf.bchCodeSel = (i / eccbytes);
		bch_ctrl.bf.bchCompare = 0;
		write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, bch_ctrl.wrd);
		bch_ctrl.bf.bchCompare = 1;
		write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, bch_ctrl.wrd);

		bch_sts.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_STATUS);
		while (!bch_sts.bf.bchDecDone) {
			udelay(1);
			schedule();
			bch_sts.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_STATUS);
		}

		switch (bch_sts.bf.bchDecStatus) {
			case BCH_CORRECTABLE_ERR:
				//printk("correctable error(%x)!!\n",bch_sts.bf.bchErrNum);
				for (j = 0; j < ((bch_sts.bf.bchErrNum + 1) / 2); j++) {
				/** printk("nand_page(%x)\n", nand_page); **/
					bch_err_loc01.wrd =
						read_flash_ctrl_reg(FLASH_NF_BCH_ERROR_LOC01 + j * 4);

					if ((j + 1) * 2 <= bch_sts.bf.bchErrNum) {
						if (((bch_err_loc01.bf.bchErrLoc1 & 0x3fff) >> 3) <
							BCH_DATA_UNIT) {
							p[(bch_err_loc01.bf.bchErrLoc1 & 0x3fff) >> 3] ^=
								(1 << (bch_err_loc01.bf.bchErrLoc1 & 0x07));
							bitflips++;
						} else {
							/* Do nothing if error bit inside ecc code */
						}
					}

					if (((bch_err_loc01.bf.bchErrLoc0 & 0x3fff) >> 3) <
						BCH_DATA_UNIT) {
						p[(bch_err_loc01.bf.bchErrLoc0 & 0x3fff) >> 3] ^=
							(1 << (bch_err_loc01.bf.bchErrLoc0 & 0x07));
						bitflips++;
					} else {
						/* Do nothing if error bit inside ecc code */
					}
				}
				break;
			case BCH_UNCORRECTABLE:
				printk("nand_page :%x, uncorrectable error!!\n", nand_page);
				mtd->ecc_stats.failed++;
				bitflips = -1;
				break;
		}
#if 0
		if (ca77x_ecc_check && i == 0) {
			unsigned char *free = buf + mtd->writesize + mtd->oobsize;
			*free = bch_sts.bf.bchDecStatus;
			*(free + 1) = bch_sts.bf.bchErrNum;
		}
#endif
		/* disable ecc compare */
		bch_ctrl.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_CONTROL);
		bch_ctrl.bf.bchCompare = 0;
		write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, bch_ctrl.wrd);

	}

	if (-1 == bitflips) {
		b_ret = blank_check(buf, chip->ecc.steps, chip->ecc.size, chip->oob_poi, chip->ecc.layout->eccpos, chip->ecc.bytes);
		if (-1 == b_ret) {
			show_page(buf, mtd->writesize);
			show_page(chip->oob_poi, mtd->oobsize);
		} else {
			printk("[%s:%d] fix bit flip in blank page(%X)! ret is 0( from %d)\n", __FUNCTION__, __LINE__, nand_page, b_ret );
			show_page(buf, mtd->writesize);
			show_page(chip->oob_poi, mtd->oobsize);
			memset(buf, 0xFF, mtd->writesize);
			memset(chip->oob_poi, 0xFF, mtd->oobsize);
			bitflips = 0;
		}
	}
	return bitflips;
}

 int ca_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,
							 uint8_t * buf, int page)
{
	unsigned int addr;
	uint8_t *vaddr, do_buff_read=0;
	unsigned long page_64 = (unsigned long)page;

#ifndef NAND_DIRECT_ACCESS

	/* disable txq5 */

	dma_txq5_ctrl.bf.txq_en = 0;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_CONTROL, dma_txq5_ctrl.wrd);

	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

	/* for indirect access with DMA, because DMA not ready  */
	nf_cnt.wrd = 0;
	nf_cmd.wrd = 0;
	nf_addr1.wrd = 0;
	nf_addr2.wrd = 0;
	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = mtd->oobsize - 1;
	nf_cnt.bf.nflashRegDataCount = mtd->writesize - 1;

	if (chip->chipsize < SZ_32M) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_3;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READ0;
		nf_addr1.wrd = (((page & 0x00ffffff) << 8));
		nf_addr2.wrd = ((page & 0xff000000) >> 24);
	} else if (chip->chipsize <= SZ_128M) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_4;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READ0;

		if (mtd->writesize > NCNT_512P_DATA) {
			nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
			nf_cmd.bf.nflashRegCmd1 = NAND_CMD_READSTART;
		}
		nf_addr1.wrd = (((page & 0xffff) << 16));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);
	} else {					/* if((chip->chipsize > SZ_128M ) )) */

		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_5;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_READ0;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_READSTART;
		nf_addr1.wrd = (((page_64 & 0xffff) << 16));
		nf_addr2.wrd = ((page_64 & 0xffff0000) >> 16);
	}

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x0 */
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);

	nf_cnt.wrd = read_flash_ctrl_reg(FLASH_NF_COUNT);
	nf_cmd.wrd = read_flash_ctrl_reg(FLASH_NF_COMMAND);
	nf_addr1.wrd = read_flash_ctrl_reg(FLASH_NF_ADDRESS_1);
	nf_addr2.wrd = read_flash_ctrl_reg(FLASH_NF_ADDRESS_2);

	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = CHIP_EN;
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	nf_access.bf.nflashExtAddr = ((page_64 << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	nf_access.wrd = read_flash_ctrl_reg(FLASH_NF_ACCESS);

	addr = (page_64 << chip->page_shift) % SZ_128M + ca77xx_host->flash_mem_base;
	/* The fifo depth is 64 bytes. We have a sync at each frame and frame
	 * length is 64 bytes. --> for vmalloc not kmalloc
	 */
	vaddr = 0;
	if (buf >= (uint8_t *)high_memory) {
		struct page *p1;

		if (((size_t) buf & PAGE_MASK) !=
			((size_t) (buf + mtd->writesize - 1) & PAGE_MASK))
			goto out_copy;
		p1 = vmalloc_to_page(buf);
		if (!p1)
			goto out_copy;
		buf = page_address(p1) + ((size_t) buf & ~PAGE_MASK);
	}
	goto out_copy_done;

  out_copy:
	vaddr = buf;
	chip->pagebuf = -1;
	buf = chip->buffers->databuf;
  out_copy_done:

	dma_txq5_wptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR);

	tx_desc[dma_txq5_wptr.bf.index].word1.bf.own = OWN_DMA;
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.buf_len = mtd->writesize;
	tx_desc[dma_txq5_wptr.bf.index].buf_adr = addr;

	/* page data rx desc */
	dma_rxq5_rptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_RPTR);
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.own = OWN_DMA;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.buf_len = mtd->writesize;

	if ((unsigned int)buf & 0xf) // not aligned
		do_buff_read = 1;

	if (do_buff_read==1) // not aligned
	{
		rx_desc[dma_rxq5_rptr.bf.index].buf_adr =
			dma_map_single(ca77xx_host->dev, (void *)tmp_buf, mtd->writesize,
					DMA_FROM_DEVICE);
	}
	else {
		rx_desc[dma_rxq5_rptr.bf.index].buf_adr =
			dma_map_single(ca77xx_host->dev, (void *)buf, mtd->writesize,
						   DMA_FROM_DEVICE);
	}

	wmb();
	dummy = rx_desc[dma_rxq5_rptr.bf.index].word1.wrd;
	/* set axi_bus_len = 8 */

	/* set fifo control */
	fifo_ctl.wrd = 0;
	fifo_ctl.bf.fifoCmd = FLASH_RD;
	write_flash_ctrl_reg(FLASH_FIFO_CONTROL, fifo_ctl.wrd);

	flash_start.wrd = 0;
	flash_start.bf.nflashFifoReq = FLASH_GO;
	/* flash_start.bf.nflashRegCmd = FLASH_RD; */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

	/* update tx write ptr */
	dma_txq5_wptr.bf.index = (dma_txq5_wptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR, dma_txq5_wptr.wrd);
	/* dma_txq5_wptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR); */

	/* enable txq5 */
	dma_txq5_ctrl.bf.txq_en = 1;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_CONTROL, dma_txq5_ctrl.wrd);

	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);

	//#define DMA_DMA_SSP_TXQ5_RPTR                    0xf009046c

	while (!dma_ssp_rxq5_intsts.bf.rxq_eof) {	//444 + 2
		udelay(1);
		schedule();
		dma_ssp_rxq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	}
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	while (!dma_ssp_txq5_intsts.bf.txq_eof) {	//46c +2
		udelay(1);
		schedule();
		dma_ssp_txq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	}

	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

	if (do_buff_read==1) // not aligned
	{
		memcpy(buf, tmp_buf, mtd->writesize);
	}

	wmb();

	/* The fifo depth is 64 bytes. We have a sync at each frame and frame
	 * length is 64 bytes. --> for vmalloc not kmalloc
	 */
	if (vaddr != 0) {
		memcpy(vaddr, buf, mtd->writesize);
		buf = vaddr;
		vaddr = 0;

	}

	/* oob tx desc */
	//dma_txq5_wptr.bf.index = (dma_txq5_wptr.bf.index + 1) % FDMA_DESC_NUM;

	//addr +=  mtd->writesize;
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.own = OWN_DMA;
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.buf_len = mtd->oobsize;
	tx_desc[dma_txq5_wptr.bf.index].buf_adr = (addr + mtd->writesize);

	/* oob rx desc */
	dma_rxq5_rptr.bf.index = (dma_rxq5_rptr.bf.index + 1) % FDMA_DESC_NUM;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.own = OWN_DMA;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.buf_len = mtd->oobsize;
	rx_desc[dma_rxq5_rptr.bf.index].buf_adr =
		dma_map_single(ca77xx_host->dev, (void *)chip->oob_poi, mtd->oobsize,
					   DMA_FROM_DEVICE);

	wmb();
	dummy = rx_desc[dma_rxq5_rptr.bf.index].word1.wrd;
	/* set axi_bus_len = 8 */

	/* update tx write ptr */
	dma_txq5_wptr.bf.index = (dma_txq5_wptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR, dma_txq5_wptr.wrd);
	/* dma_txq5_wptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR); */

	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	//#define DMA_DMA_SSP_TXQ5_RPTR                    0xf009046c

	while (!dma_ssp_rxq5_intsts.bf.rxq_eof) {	//444 + 2
		udelay(1);
		schedule();
		dma_ssp_rxq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	}
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	while (!dma_ssp_txq5_intsts.bf.txq_eof) {	//46c +2
		udelay(1);
		schedule();
		dma_ssp_txq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	}

	flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	while (flash_start.bf.nflashFifoReq) {
		udelay(1);
		schedule();
		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	}

	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

	dma_rxq5_rptr.bf.index = (dma_rxq5_rptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_RPTR, dma_rxq5_rptr.wrd);

	dma_map_single(ca77xx_host->dev, (void *)chip->oob_poi, mtd->oobsize,
				   DMA_FROM_DEVICE);
	wmb();

#else
	/* direct access nand */
	static unsigned int *pread;
	int i;
	pread = (unsigned int *)buf;

	nf_access.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	nf_access.bf.nflashExtAddr = ((page << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	addr = (unsigned int *)((page << chip->page_shift) % SZ_128M);
	addr =
		(unsigned int *)((unsigned int)addr +
						 (unsigned int)(ca77x_host->io_base));
	/* printk("%s : addr : %p  pread: %p",__func__, addr, pread); */
	for (i = 0; i < (mtd->writesize / 4); i++) {
		*(pread + i) = *((unsigned int *)addr + i);
	}

	/* ecc_sts.wrd=read_flash_ctrl_reg(FLASH_NF_ECC_STATUS); */
	/* while(!ecc_sts.bf.eccDone) */
	/* { */
	/*     ecc_sts.wrd=read_flash_ctrl_reg(FLASH_NF_ECC_STATUS); */
	/*     udelay(1); */
	/*     schedule(); */
	/* } */

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0);	/* disable ecc gen */

	pread = (unsigned int *)(chip->oob_poi);

	nf_access.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	nf_access.bf.nflashExtAddr = ((page << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	addr = (unsigned int *)((page << chip->page_shift) % SZ_128M);
	addr =
		(unsigned int *)((unsigned int)addr +
						 (unsigned int)(ca77x_host->io_base) + mtd->writesize);
	/* printk("   oob : addr (%p)  pread (%p) \n", addr, pread); */
	unsigned int tmp_addr;
	tmp_addr = (unsigned int)addr;
	if (tmp_addr == 0x91000000)
		printk("%s : out of range  page(%x)\n ", __func__, page);
	if (((unsigned int)addr) ==
		((unsigned int)(ca77x_host->io_base) + SZ_128M)) {
		ca77x_nand_read_oob_std(mtd, chip, page, 0);
	} else {
		for (i = 0; i < (mtd->oobsize / 4); i++) {
			*(pread + i) = *((unsigned int *)addr + i);
		}
	}

#endif

	return 0;
}

/**
 * ca_nand_read_page_hwecc - [REPLACABLE] hardware ecc based page read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 * @page:	page number to read
 *
 * Not for syndrome calculating ecc controllers which need a special oob layout
 */
 int ca_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
								   uint8_t * buf, int oob_required, int page)
{
	int col, bitflips = 0;
	uint8_t *p = buf;

	uint8_t *ecc_code = chip->buffers->ecccode;

	col = nand_col;
	p = buf;
	check_flash_ctrl_status();
	ca_rst_ecc_bch_registers();

	if (ca77xx_host->ecc_algorithm > ECC_HAM_512_3B) {	/* BCH */
		bch_ctrl.wrd = 0;
		bch_ctrl.bf.bchEn = BCH_ENABLE;
		bch_ctrl.bf.bchOpcode = BCH_DECODE;
		bch_ctrl.bf.bchErrCap = ca77xx_host->ecc_algorithm - ECC_BCH_8;
		write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, bch_ctrl.wrd);
	} else {					/* Hamming */
		ecc_ctl.wrd = 0;
		ecc_ctl.bf.eccGenMode = ca77xx_host->ecc_algorithm - ECC_HAM_256_3B;
		ecc_ctl.bf.eccEn = ECC_ENABLE;
		write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, ecc_ctl.wrd);
	}

	ca_nand_read_page(mtd, chip, buf, page);

	if (ca77xx_host->ecc_algorithm > ECC_HAM_512_3B) {	/* BCH */
		bch_sts.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_STATUS);
		while (!bch_sts.bf.bchGenDone) {
			udelay(1);
			schedule();
			bch_sts.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_STATUS);
		}
	} else {
		ecc_sts.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_STATUS);
		while (!ecc_sts.bf.eccDone) {
			udelay(1);
			schedule();
			ecc_sts.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_STATUS);
		}

		ecc_ctl.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_CONTROL);
		ecc_ctl.bf.eccEn = 0;
		write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, ecc_ctl.wrd);
	}

#ifdef	NAND_ECC_TEST
	memset(tr, 0, NAND_MAX_PAGESIZE);
	memcpy(tr, buf, mtd->writesize);
	addr = ((page << chip->page_shift) % SZ_128M) + ca77x_host->io_base;
#endif

	if (ca77xx_host->ecc_algorithm > ECC_HAM_512_3B) {	/* BCH */
		if (0xff ==
			*(chip->oob_poi + chip->ecc.layout->oobfree[0].offset +
			  chip->ecc.layout->oobfree[0].length)) {
			int b_ret = blank_check(buf, chip->ecc.steps, chip->ecc.size, chip->oob_poi, chip->ecc.layout->eccpos, chip->ecc.bytes);
			if (b_ret) {
				printk("[%s:%d] fix bit flip in blank page(%X)! ret is 0(from %d)\n", __FUNCTION__, __LINE__, page, b_ret );
				show_page(buf, mtd->writesize);
				show_page(chip->oob_poi, mtd->oobsize);
				memset(buf, 0xFF, mtd->writesize);
				memset(chip->oob_poi, 0xFF, mtd->oobsize);
			}

			/*  Erase tga is on , No needs to check. */
			goto BCH_EXIT;
		}
	}

	if (ca77xx_host->ecc_algorithm >= ECC_BCH_8) {	/* BCH */
		bitflips = bch_correct(mtd, chip, buf);
	} else {					/* Hamming */
		int i;

		int eccsteps, eccbytes, eccsize;
		uint32_t *eccpos;

		eccsteps = chip->ecc.steps;
		eccbytes = chip->ecc.bytes;
		eccsize = chip->ecc.size;
		eccpos = chip->ecc.layout->eccpos;

		for (i = 0; i < chip->ecc.total; i++) {
			ecc_code[i] = chip->oob_poi[eccpos[i]];
		}

		for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
#ifdef	NAND_ECC_TEST
			get_cpu();
			if (chip->ecc.size == 256) {
				memset(eccode, 0, 32);
				if (memcmp(tr, buf, mtd->writesize))
					printk("copy error ");
				/* printk("%s : ecc 256 addr %p ",__func__,&tr[(i/eccbytes)*256]); */
				nand_calculate_256_ecc(&tr[(i / eccbytes) * 256], &eccode[0]);
				nand_calculate_256_ecc(&buf[(i / eccbytes) * 256], &eccode[3]);
				/* nand_calculate_ecc(mtd, &tr[(i/eccbytes)*256], &eccode[3]); */
				eccdata = 0;
				eccdata = eccode[0] | eccode[1] << 8 | eccode[2] << 16;
				ecc_oob.wrd =
					ecc_code[i] | ecc_code[i + 1] << 8 | ecc_code[i + 2] << 16;
				write_flash_ctrl_reg(FLASH_NF_ECC_OOB, ecc_oob.wrd);
				if (ecc_oob.wrd != eccdata)
					printk
						("256: (i/eccbytes) : %x HWecc error ecc_oob.wrd(%x)  eccdata (%x)!\n",
						 (i / eccbytes), ecc_oob.wrd, eccdata);
			} else {
				memset(eccode, 0, 32);
				/* printk("%s : ecc 512 addr %p ",__func__,&tr[(i/eccbytes)*512]); */
				nand_calculate_512_ecc(mtd, &tr[(i / eccbytes) * 512],
									   &eccode[0]);

				/* nand_calculate_ecc(mtd, &tr[(i/eccbytes)*512], &eccode[3]); */
				eccdata = 0;
				eccdata = eccode[0] | eccode[1] << 8 | eccode[2] << 16;
				ecc_oob.wrd =
					ecc_code[i] | ecc_code[i + 1] << 8 | ecc_code[i + 2] << 16;
				write_flash_ctrl_reg(FLASH_NF_ECC_OOB, ecc_oob.wrd);
				if (ecc_oob.wrd != eccdata)
					printk
						("512: (i/eccbytes) : %x HWecc error ecc_oob.wrd(%x)  eccdata (%x)!\n",
						 (i / eccbytes), ecc_oob.wrd, eccdata);
			}
			put_cpu();
#endif
			/* for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) { */
			ecc_oob.wrd =
				ecc_code[i] | ecc_code[i + 1] << 8 | ecc_code[i + 2] << 16;
			write_flash_ctrl_reg(FLASH_NF_ECC_OOB, ecc_oob.wrd);

			ecc_ctl.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_CONTROL);
			ecc_ctl.bf.eccCodeSel = (i / eccbytes);
			write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, ecc_ctl.wrd);

			ecc_sts.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_STATUS);

			switch (ecc_sts.bf.eccStatus) {
				case ECC_NO_ERR:
					/* printk("ECC no error!!\n"); */
					break;
				case ECC_1BIT_DATA_ERR:
					/* flip the bit */
					bitflips = 1;
					p[ecc_sts.bf.eccErrByte] ^= (1 << ecc_sts.bf.eccErrBit);
					/* printk("ecc_code- %x (%x) :\n",i ,chip->ecc.total); */
					ecc_gen0.wrd =
						read_flash_ctrl_reg(FLASH_NF_ECC_GEN0 +
											(4 * (i / eccbytes)));
					/* for (i = 0; i < chip->ecc.total; i++) */
					/*   printk(" %x", ecc_code[i]); */

					printk
						("\nECC one bit data error(%x)!!(org: %x) HW(%xs) page(%x)\n",
						 (i / eccbytes), ecc_oob.wrd, ecc_gen0.wrd, page);
					break;
				case ECC_1BIT_ECC_ERR:
					/* printk("ecc_code- %x (%x) :\n",i ,chip->ecc.total); */
					ecc_gen0.wrd =
						read_flash_ctrl_reg(FLASH_NF_ECC_GEN0 +
											(4 * (i / eccbytes)));
					/* for (i = 0; i < chip->ecc.total; i++) */
					/*   printk(" %x", ecc_code[i]); */
					printk
						("\nECC one bit ECC error(%x)!!(org: %x) HW(%xs) page(%x)\n",
						 (i / eccbytes), ecc_oob.wrd, ecc_gen0.wrd, page);
					break;
				case ECC_UNCORRECTABLE:
					mtd->ecc_stats.failed++;
					ecc_gen0.wrd =
						read_flash_ctrl_reg(FLASH_NF_ECC_GEN0 +
											(4 * (i / eccbytes)));
					printk
						("\nECC uncorrectable error(%x)!!(org: %x) HW(%xs) page(%x)\n",
						 (i / eccbytes), ecc_oob.wrd, ecc_gen0.wrd, page);
					break;
			}

			//      if ( ca77x_ecc_check && i == 0 ) {
			//          unsigned char *free= buf+ mtd->writesize+ mtd->oobsize;
			//          *free=  ecc_sts.bf.eccStatus;
			//      }
		}
	}

  BCH_EXIT:
	bch_ctrl.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, bch_ctrl.wrd);

	return bitflips;
}

/**
 * ca77x_nand_write_page_hwecc - [REPLACABLE] hardware ecc based page write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 */
 int ca_nand_write_page_hwecc(struct mtd_info *mtd,
									struct nand_chip *chip, const uint8_t * buf,
									int oob_required, int page)
{
	unsigned long page_64;
	int col;
	uint32_t addr;
	uint8_t *vaddr;

	// check address
	if (chkAddrValid(mtd,&page)) {
		return -EIO;
	}
	page_64 = (unsigned long)page;
	col = nand_col;

	check_flash_ctrl_status();

	ca_rst_ecc_bch_registers();

	dma_txq5_ctrl.bf.txq_en = 0;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_CONTROL, dma_txq5_ctrl.wrd);

	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

	if (ca77xx_host->ecc_algorithm > ECC_HAM_512_3B) {	/* BCH */
		bch_ctrl.wrd = 0;
		bch_ctrl.bf.bchEn = BCH_ENABLE;
		bch_ctrl.bf.bchErrCap = ca77xx_host->ecc_algorithm - ECC_BCH_8;
		bch_ctrl.bf.bchOpcode = BCH_ENCODE;
		write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, bch_ctrl.wrd);
	} else {					/* Hamming */
		ecc_ctl.wrd = 0;
		ecc_ctl.bf.eccGenMode = ca77xx_host->ecc_algorithm - ECC_HAM_256_3B;
		ecc_ctl.bf.eccEn = ECC_ENABLE;
		write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, ecc_ctl.wrd);
	}

#ifndef NAND_DIRECT_ACCESS

	nf_cnt.wrd = 0;
	nf_cmd.wrd = 0;
	nf_addr1.wrd = 0;
	nf_addr2.wrd = 0;
	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = mtd->oobsize - 1;
	nf_cnt.bf.nflashRegDataCount = mtd->writesize - 1;

	if (chip->chipsize < SZ_32M) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_3;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		nf_addr1.wrd = (((page & 0x00ffffff) << 8));
		nf_addr2.wrd = ((page & 0xff000000) >> 24);

	} else if (chip->chipsize <= SZ_128M) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_4;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		nf_addr1.wrd = (((page & 0xffff) << 16));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);

	} else {					/* if((chip->chipsize > SZ_128M ) )) */

		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_5;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		nf_addr1.wrd = (((page & 0xffff) << 16));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);
	}

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);

	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = CHIP_EN;
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	nf_access.bf.nflashExtAddr = ((page_64 << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	addr = (page_64 << chip->page_shift) % SZ_128M + ca77xx_host->flash_mem_base;

	/* The fifo depth is 64 bytes. We have a sync at each frame and frame
	 * length is 64 bytes. --> for vmalloc not kmalloc
	 */
	vaddr = 0;
	if (buf >= (uint8_t *)high_memory) {
		struct page *p1;

		if (((size_t) buf & PAGE_MASK) !=
			((size_t) (buf + mtd->writesize - 1) & PAGE_MASK))
			goto out_copy;
		p1 = vmalloc_to_page(buf);
		if (!p1)
			goto out_copy;
		buf = page_address(p1) + ((size_t) buf & ~PAGE_MASK);

	}
	goto out_copy_done;
  out_copy:
	vaddr = (uint8_t *)buf;
	chip->pagebuf = -1;
	buf = chip->buffers->databuf;
	memcpy((uint8_t *)buf, vaddr, mtd->writesize);
  out_copy_done:

	/* page data tx desc */
	dma_txq5_wptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR);
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.own = OWN_DMA;
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.buf_len = mtd->writesize;
	tx_desc[dma_txq5_wptr.bf.index].buf_adr =
		dma_map_single(ca77xx_host->dev, (void *)buf, mtd->writesize,
					   DMA_TO_DEVICE);

#ifdef	NAND_ECC_TEST
	memset(tw, 0, NAND_MAX_PAGESIZE);
	memcpy(tw, buf, mtd->writesize);
#endif

	dma_rxq5_rptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_RPTR);
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.own = OWN_DMA;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.buf_len = mtd->writesize;
	rx_desc[dma_rxq5_rptr.bf.index].buf_adr = addr;

	/* update page tx write ptr */
	dma_txq5_wptr.bf.index = (dma_txq5_wptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR, dma_txq5_wptr.wrd);
	/* set axi_bus_len = 8 */
	/* set fifo control */
	fifo_ctl.wrd = 0;
	fifo_ctl.bf.fifoCmd = FLASH_WT;
	write_flash_ctrl_reg(FLASH_FIFO_CONTROL, fifo_ctl.wrd);

	wmb();
	dummy = rx_desc[dma_rxq5_rptr.bf.index].word1.wrd;

	flash_start.wrd = 0;
	flash_start.bf.nflashFifoReq = FLASH_GO;
	/* flash_start.bf.nflashRegCmd = FLASH_WT; */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

	/* enable txq5 */
	dma_txq5_ctrl.bf.txq_en = 1;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_CONTROL, dma_txq5_ctrl.wrd);

	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	/**
	if(dma_ssp_rxq5_intsts.bf.rxq5_full)
	{
		printk("rxq5_full \n");
		while(1);
	}
	**/
	while (!dma_ssp_rxq5_intsts.bf.rxq_eof) {
		udelay(1);
		schedule();
		dma_ssp_rxq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	}

	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	while (!dma_ssp_txq5_intsts.bf.txq_eof) {
		udelay(1);
		schedule();
		dma_ssp_txq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	}

	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

#else

#ifdef	NAND_ECC_TEST
	memset(tw, 0, NAND_MAX_PAGESIZE);
	memcpy(tw, buf, mtd->writesize);
#endif
	/* direct access nand */
	unsigned int *pwrt, i;
	pwrt = (unsigned int *)buf;

	/* addr = (unsigned int *)((page << chip->page_shift) + ca77x_host->io_base ); */
	nf_access.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	nf_access.bf.nflashExtAddr = ((page_64 << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	addr = (unsigned int *)((page_64 << chip->page_shift) % SZ_128M);
	addr =
		(unsigned int *)((unsigned int)addr +
						 (unsigned int)(ca77x_host->io_base));
	/* printk("ca_nand_write_page_hwecc : addr : %p  pwrt: %p",addr, pwrt); */
	for (i = 0; i < ((mtd->writesize) / 4); i++) {
		*((unsigned int *)addr + i) = *(pwrt + i);
	}
#endif

	if (ca77xx_host->ecc_algorithm > ECC_HAM_512_3B) {	/* BCH */
		bch_sts.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_STATUS);
		while (!bch_sts.bf.bchGenDone) {
			udelay(1);
			schedule();
			bch_sts.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_STATUS);
		}
		bch_ctrl.wrd = read_flash_ctrl_reg(FLASH_NF_BCH_CONTROL);	/* disable ecc gen */
		bch_ctrl.bf.bchEn = BCH_DISABLE;
		write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, bch_ctrl.wrd);
	} else {
		ecc_sts.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_STATUS);
		while (!ecc_sts.bf.eccDone) {
			udelay(1);
			schedule();
			ecc_sts.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_STATUS);
		}

		ecc_ctl.wrd = read_flash_ctrl_reg(FLASH_NF_ECC_CONTROL);
		ecc_ctl.bf.eccEn = 0;
		write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, ecc_ctl.wrd);	/* disable ecc gen */
	}

	/* printk("write page ecc(page %x) : ", page); */

	if (ca77xx_host->ecc_algorithm > ECC_HAM_512_3B)	/* BCH */
		fill_bch_oob_data(chip);
	else						/* Hamming */
		fill_hamming_oob_data(chip, mtd);

	/* printk("\n"); */

#ifndef NAND_DIRECT_ACCESS

	/* oob rx desc */
	//addr +=  mtd->writesize;

	dma_rxq5_rptr.bf.index = (dma_rxq5_rptr.bf.index + 1) % FDMA_DESC_NUM;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.own = OWN_DMA;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.buf_len = mtd->oobsize;
	rx_desc[dma_rxq5_rptr.bf.index].buf_adr = (addr + mtd->writesize);

	/* dma_txq5_wptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR); */
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.own = OWN_DMA;
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.buf_len = mtd->oobsize;
	tx_desc[dma_txq5_wptr.bf.index].buf_adr =
		dma_map_single(ca77xx_host->dev, (void *)chip->oob_poi, mtd->oobsize,
					   DMA_TO_DEVICE);

	wmb();
	dummy = tx_desc[dma_txq5_wptr.bf.index].word1.wrd;

	/* dma_cache_sync(NULL, chip->oob_poi, mtd->oobsize, DMA_BIDIRECTIONAL); */
	/* update tx write ptr */
	dma_txq5_wptr.bf.index = (dma_txq5_wptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR, dma_txq5_wptr.wrd);

	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	/**
	if(dma_ssp_rxq5_intsts.bf.rxq5_full)
	{
		printk("rxq5_full \n");
		while(1);
	}
	**/
	while (!dma_ssp_rxq5_intsts.bf.rxq_eof) {
		udelay(1);
		schedule();
		dma_ssp_rxq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	}
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	while (!dma_ssp_txq5_intsts.bf.txq_eof) {
		udelay(1);
		schedule();
		dma_ssp_txq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	}

	flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	while (flash_start.bf.nflashFifoReq) {
		udelay(1);
		schedule();
		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	}

	/* update rx read ptr */
	/* dma_rxq5_rptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_RPTR); */
	dma_rxq5_rptr.bf.index = (dma_rxq5_rptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_RPTR, dma_rxq5_rptr.wrd);

	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

	/* The fifo depth is 64 bytes. We have a sync at each frame and frame
	 * length is 64 bytes. --> for vmalloc not kmalloc
	 */
	if (vaddr != 0) {
		buf = vaddr;
		vaddr = 0;
	}

	/* /////////////// */
	/* chip->write_buf(mtd, chip->oob_poi, mtd->oobsize); */
#else
	/* direct access nand */
	pwrt = (unsigned int *)chip->oob_poi;
	/* addr = (unsigned int *)(((page << chip->page_shift) + mtd->writesize) + ca77xx_host->io_base ); */
	nf_access.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	nf_access.bf.nflashExtAddr = ((page_64 << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	addr = (unsigned int *)((page_64 << chip->page_shift) % SZ_128M);
	addr =
		(unsigned int *)((unsigned int)addr + mtd->writesize +
						 (unsigned int)(ca77x_host->io_base));
	/* printk("  oob : addr(%p)  pwrt(%p) \n",addr, pwrt); */
	unsigned int tmp_addr;
	tmp_addr = (unsigned int)addr;
	if (tmp_addr == 0x91000000)
		printk("%s : out of range  page(%x)\n ", __func__, page);

	if (((unsigned int)addr) ==
		((unsigned int)(ca77x_host->io_base) + SZ_128M)) {
		ca_nand_write_oob_std(mtd, chip, page);
	} else {
		for (i = 0; i < (mtd->oobsize / 4); i++) {
			*((unsigned int *)addr + i) = *(pwrt + i);
		}
	}
#endif
	return 0;
}

/**
 * ca_nand_read_page_raw - [Intern] read raw page data without ecc
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 * @oob_required: caller need oob data, read to chip->oob_poi
 * @page:	page number to read
 *
 * Not for syndrome calculating ecc controllers, which use a special oob layout
 */
 int ca_nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
								 uint8_t * buf, int oob_requied, int page)
{
	check_flash_ctrl_status();

	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = ECC_CLR;
	ecc_reset.bf.fifoClear = FIFO_CLR;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);

	flash_int_sts.bf.regIrq = 1;
	write_flash_ctrl_reg(FLASH_FLASH_INTERRUPT, flash_int_sts.wrd);

	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = 1;
	write_flash_ctrl_reg(FLASH_NF_ECC_RESET, ecc_reset.wrd);

	bch_ctrl.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_BCH_CONTROL, bch_ctrl.wrd);

	ecc_ctl.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, ecc_ctl.wrd);

	return ca_nand_read_page(mtd, chip, buf, page);
}

/**
 * ca_nand_write_page_raw - [Intern] raw page write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 *
 * Not for syndrome calculating ecc controllers, which use a special oob layout
 */
 int ca_nand_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
								  const uint8_t * buf, int oob_required, int page)
{
	unsigned long page_64;
	uint32_t addr;
	uint8_t *vaddr;

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	check_flash_ctrl_status();

	page_64 = (unsigned long)page;

	ca_rst_ecc_bch_registers();

#ifndef NAND_DIRECT_ACCESS

	/* disable txq5 */
	dma_txq5_ctrl.bf.txq_en = 0;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_CONTROL, dma_txq5_ctrl.wrd);

	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

	nf_cnt.wrd = 0;
	nf_cmd.wrd = 0;
	nf_addr1.wrd = 0;
	nf_addr2.wrd = 0;
	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = mtd->oobsize - 1;
	nf_cnt.bf.nflashRegDataCount = mtd->writesize - 1;

	if (chip->chipsize < (32 << 20)) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_3;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		nf_addr1.wrd = (((page & 0x00ffffff) << 8));
		nf_addr2.wrd = ((page & 0xff000000) >> 24);

	} else if ((chip->chipsize >= (32 << 20))
			   && (chip->chipsize <= (128 << 20))) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_4;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		nf_addr1.wrd = (((page & 0xffff) << 16));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);

	} else {					/* if((chip->chipsize > (128 << 20)) )) */

		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_5;
		nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;
		nf_cmd.bf.nflashRegCmd0 = NAND_CMD_SEQIN;
		nf_cmd.bf.nflashRegCmd1 = NAND_CMD_PAGEPROG;
		nf_addr1.wrd = (((page & 0xffff) << 16));
		nf_addr2.wrd = ((page & 0xffff0000) >> 16);
	}

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);

	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = CHIP_EN;
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	/* write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd); */

	/* write */
	/* prepare dma descriptor */
	/* chip->buffers->databuf */
	/* nf_access.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START); */
	nf_access.bf.nflashExtAddr = ((page_64 << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	addr = ((page_64 << chip->page_shift) % SZ_128M) + ca77xx_host->flash_mem_base;

	/* The fifo depth is 64 bytes. We have a sync at each frame and frame
	 * length is 64 bytes. --> for vmalloc not kmalloc
	 */
	vaddr = 0;
	if (buf >= (uint8_t *)high_memory) {
		struct page *p1;

		if (((size_t) buf & PAGE_MASK) !=
			((size_t) (buf + mtd->writesize - 1) & PAGE_MASK))
			goto out_copy;
		p1 = vmalloc_to_page(buf);
		if (!p1)
			goto out_copy;
		buf = page_address(p1) + ((size_t) buf & ~PAGE_MASK);

	}
	goto out_copy_done;
  out_copy:
	vaddr = (uint8_t *)buf;
	chip->pagebuf = -1;
	buf = chip->buffers->databuf;
	memcpy((uint8_t *)buf, vaddr, mtd->writesize);
  out_copy_done:

	/* page data tx desc */
	dma_txq5_wptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR);
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.own = OWN_DMA;
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.buf_len = mtd->writesize;
	tx_desc[dma_txq5_wptr.bf.index].buf_adr =
		dma_map_single(ca77xx_host->dev, (void *)buf, mtd->writesize,
					   DMA_TO_DEVICE);

#ifdef	NAND_ECC_TEST
	memset(tw, 0, NAND_MAX_PAGESIZE);
	memcpy(tw, buf, mtd->writesize);
#endif

	/* page data rx desc */
	/* printk("ca_nand_write_page_hwecc : addr : %p  buf: %p",addr, buf); */

	dma_rxq5_rptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_RPTR);
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.own = OWN_DMA;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.buf_len = mtd->writesize;
	rx_desc[dma_rxq5_rptr.bf.index].buf_adr = addr;

	/* oob rx desc */
	addr = (uint32_t) addr + (uint32_t) mtd->writesize;
	/* printk("  oob : addr(%p)  chip->oob_poi(%p) \n",addr, chip->oob_poi); */

	dma_rxq5_rptr.bf.index = (dma_rxq5_rptr.bf.index + 1) % FDMA_DESC_NUM;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.own = OWN_DMA;
	rx_desc[dma_rxq5_rptr.bf.index].word1.bf.buf_len = mtd->oobsize;
	rx_desc[dma_rxq5_rptr.bf.index].buf_adr = addr;

	wmb();
	dummy = rx_desc[dma_rxq5_rptr.bf.index].word1.wrd;

	/* update page tx write ptr */
	dma_txq5_wptr.bf.index = (dma_txq5_wptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR, dma_txq5_wptr.wrd);
	/* set axi_bus_len = 8 */
	/* set fifo control */
	fifo_ctl.wrd = 0;
	fifo_ctl.bf.fifoCmd = FLASH_WT;
	write_flash_ctrl_reg(FLASH_FIFO_CONTROL, fifo_ctl.wrd);

	flash_start.wrd = 0;
	flash_start.bf.nflashFifoReq = FLASH_GO;
	/* flash_start.bf.nflashRegCmd = FLASH_WT; */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

	/* enable txq5 */
	dma_txq5_ctrl.bf.txq_en = 1;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_CONTROL, dma_txq5_ctrl.wrd);

#else

#ifdef	NAND_ECC_TEST
	memset(tw, 0, NAND_MAX_PAGESIZE);
	memcpy(tw, buf, mtd->writesize);
#endif
	/* direct access nand */
	unsigned int *pwrt;
	unsigned int i;
	pwrt = (unsigned int *)buf;

	/* addr = (unsigned int *)((page << chip->page_shift) + ca77x_host->io_base ); */
	nf_access.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	nf_access.bf.nflashExtAddr = ((page_64 << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	addr = (unsigned int *)((page_64 << chip->page_shift) % SZ_128M);
	addr =
		(unsigned int *)((unsigned int)addr +
						 (unsigned int)(ca77x_host->io_base));
	/* printk("ca77x_nand_write_page_hwecc : addr : %p  pwrt: %p",addr, pwrt); */
	for (i = 0; i < ((mtd->writesize) / 4); i++) {
		*((unsigned int *)addr + i) = *(pwrt + i);
	}
#endif

#ifndef NAND_DIRECT_ACCESS

	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	while (!dma_ssp_rxq5_intsts.bf.rxq_eof) {
		udelay(1);
		schedule();
		dma_ssp_rxq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	}
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	while (!dma_ssp_txq5_intsts.bf.txq_eof) {
		udelay(1);
		schedule();
		dma_ssp_txq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	}
	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

	if(ca77xx_host->ecc_algorithm >= ECC_BCH_8)
	{
		/* jenfeng clear erase tag */
		*(chip->oob_poi + chip->ecc.layout->oobfree[0].offset +
	  	chip->ecc.layout->oobfree[0].length) = 0;
	}

	/* dma_txq5_wptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR); */
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.own = OWN_DMA;
	tx_desc[dma_txq5_wptr.bf.index].word1.bf.buf_len = mtd->oobsize;
	tx_desc[dma_txq5_wptr.bf.index].buf_adr =
		dma_map_single(ca77xx_host->dev, (void *)chip->oob_poi, mtd->oobsize,
					   DMA_TO_DEVICE);

	wmb();
	dummy = tx_desc[dma_txq5_wptr.bf.index].word1.wrd;

	/* update tx write ptr */
	dma_txq5_wptr.bf.index = (dma_txq5_wptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_WPTR, dma_txq5_wptr.wrd);

	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	while (!dma_ssp_rxq5_intsts.bf.rxq_eof) {
		udelay(1);
		schedule();
		dma_ssp_rxq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	}
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	while (!dma_ssp_txq5_intsts.bf.txq_eof) {
		udelay(1);
		schedule();
		dma_ssp_txq5_intsts.wrd =
			read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	}

	flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	while (flash_start.bf.nflashFifoReq) {
		udelay(1);
		schedule();
		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	}

	/* update rx read ptr */
	/* dma_rxq5_rptr.wrd = read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_RPTR); */
	dma_rxq5_rptr.bf.index = (dma_rxq5_rptr.bf.index + 1) % FDMA_DESC_NUM;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_RPTR, dma_rxq5_rptr.wrd);

	/* clr tx/rx eof */
	dma_ssp_txq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT,
					   dma_ssp_txq5_intsts.wrd);
	dma_ssp_rxq5_intsts.wrd =
		read_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT);
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT,
					   dma_ssp_rxq5_intsts.wrd);

	/* The fifo depth is 64 bytes. We have a sync at each frame and frame
	 * length is 64 bytes. --> for vmalloc not kmalloc
	 */
	if (vaddr != 0) {
		buf = vaddr;
		vaddr = 0;
	}

	/* /////////////// */
	/* chip->write_buf(mtd, chip->oob_poi, mtd->oobsize); */
#else
	/* direct access nand */
	pwrt = (unsigned int *)chip->oob_poi;
	/* addr = (unsigned int *)(((page << chip->page_shift) + mtd->writesize) + ca77x_host->io_base ); */
	nf_access.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
	nf_access.bf.nflashExtAddr = ((page_4 << chip->page_shift) / SZ_128M);
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	addr = (unsigned int *)((page_4 << chip->page_shift) % SZ_128M);
	addr =
		(unsigned int *)((unsigned int)addr + mtd->writesize +
						 (unsigned int)(ca77x_host->io_base));
	/* printk("  oob : addr(%p)  pwrt(%p) \n",addr, pwrt); */
	unsigned int tmp_addr;
	tmp_addr = (unsigned int)addr;
	if (tmp_addr == 0x91000000)
		printk("%s : out of range  page(%x)\n ", __func__, page);

	if (((unsigned int)addr) ==
		((unsigned int)(ca77x_host->io_base) + SZ_128M)) {
		ca_nand_write_oob_std(mtd, chip, page);
	} else {
		for (i = 0; i < (mtd->oobsize / 4); i++) {
			*((unsigned int *)addr + i) = *(pwrt + i);
		}
	}

#endif
	return 0;
}

/**
 * ca_nand_read_subpage - [REPLACEABLE] ECC based sub-page read function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @data_offs: offset of requested data within the page
 * @readlen: data length
 * @bufpoi: buffer to store read data
 * @page: page number to read
 */
 int ca_nand_read_subpage(struct mtd_info *mtd, struct nand_chip *chip,
								uint32_t data_offs, uint32_t readlen,
								uint8_t * bufpoi, int page)
{
	int max_bitflips = 0;

	struct nand_chip *this = mtd->priv;

	max_bitflips =
		this->ecc.read_page(mtd, this, this->buffers->databuf, 1, page);
	memcpy(bufpoi, &(this->buffers->databuf[data_offs]), readlen);

	return max_bitflips;
}

/**
 * ca_nand_erase_block - [GENERIC] erase a block
 * @mtd:	MTD device structure
 * @page:	page address
 *
 * Erase a block.
 */

 int ca_nand_erase_block(struct mtd_info *mtd, int page)
{
/* 	int opcode,tst=0,tst1=0,tst2=0; */
	struct nand_chip *this = mtd->priv;
	u64 test;
	unsigned long	timeo ;

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	check_flash_ctrl_status();

	/* Send commands to erase a page */
	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0);	/*  */

	nf_cnt.wrd = 0;
	nf_cmd.wrd = 0;
	nf_addr1.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
	nf_cnt.bf.nflashRegDataCount = NCNT_EMPTY_DATA;
	nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_2;

	/*
	   test = this->chipsize;
	   test = test / mtd->writesize;
	   if((this->chipsize/mtd->writesize) > 0x10000)
	 */

	test = 0x10000 * mtd->writesize;
	if (this->chipsize > test) {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_3;
	} else {
		nf_cnt.bf.nflashRegAddrCount = NCNT_ADDR_2;
	}

	nf_cmd.bf.nflashRegCmd0 = NAND_CMD_ERASE1;
	nf_cmd.bf.nflashRegCmd1 = NAND_CMD_ERASE2;
	nf_addr1.wrd = page;
	nf_addr2.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);

	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = CHIP_EN;
	/* nf_access.bf.nflashDirWr = ; */
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;

	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);
	flash_start.wrd = 0;
	flash_start.bf.nflashRegReq = FLASH_GO;
	flash_start.bf.nflashRegCmd = FLASH_RD;	/* no data access use read.. */
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

	timeo = jiffies + HZ;
	do {
		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		if(!flash_start.bf.nflashRegReq)
			return 0;
	} while (time_before(jiffies, timeo));

	printk("%s: %x\n", __func__,flash_start.wrd);

	return 0;
}

/* <== Cortina added function */

static int check_offs_len(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct nand_chip *chip = mtd->priv;
	int ret = 0;

	/* Start address must align on block boundary */
	if (ofs & ((1ULL << chip->phys_erase_shift) - 1)) {
		pr_debug("%s: unaligned address\n", __func__);
		ret = -EINVAL;
	}

	/* Length must align on block boundary */
	if (len & ((1ULL << chip->phys_erase_shift) - 1)) {
		pr_debug("%s: length not block aligned\n", __func__);
		ret = -EINVAL;
	}

	return ret;
}

/**
 * nand_release_device - [GENERIC] release chip
 * @mtd: MTD device structure
 *
 * Release chip lock and wake up anyone waiting on the device.
 */
static void nand_release_device(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	/* Release the controller and the chip */
	spin_lock(&chip->controller->lock);
	chip->controller->active = NULL;
	chip->state = FL_READY;
	wake_up(&chip->controller->wq);
	spin_unlock(&chip->controller->lock);
}

/**
 * nand_read_byte - [DEFAULT] read one byte from the chip
 * @mtd: MTD device structure
 *
 * Default read function for 8bit buswidth
 */
 uint8_t ca_nand_read_byte(struct mtd_info *mtd)
{
	unsigned int data = 0, page = 0, col = 0;
	struct nand_chip *chip = mtd->priv;

	page = nand_page;
	col = nand_col;
	chip->pagebuf = -1;

	ca_nand_read_page_raw(mtd, chip, chip->buffers->databuf, 1, page);
	data = *(chip->buffers->databuf + col);

	return data & 0xff;
}

/**
 * nand_read_byte16 - [DEFAULT] read one byte endianness aware from the chip
 * nand_read_byte16 - [DEFAULT] read one byte endianness aware from the chip
 * @mtd: MTD device structure
 *
 * Default read function for 16bit buswidth with endianness conversion.
 *
 */
static uint8_t nand_read_byte16(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return (uint8_t) cpu_to_le16(readw(chip->IO_ADDR_R));
}

/**
 * nand_read_word - [DEFAULT] read one word from the chip
 * @mtd: MTD device structure
 *
 * Default read function for 16bit buswidth without endianness conversion.
 */
 u16 ca_nand_read_word(struct mtd_info *mtd)
{
	unsigned char data0, data1;
	unsigned page = 0, col = 0;
	struct nand_chip *chip = mtd->priv;

	page = nand_page;
	col = nand_col;
	chip->pagebuf = -1;

	ca_nand_read_page_raw(mtd, chip, chip->buffers->databuf, 1, page);
	data0 = *(chip->buffers->databuf + col);
	data1 = *(chip->buffers->databuf + col + 1);

	return (data1 << 8) | data0;
}

/**
 * ca_nand_select_chip - [DEFAULT] control CE line
 * @mtd: MTD device structure
 * @chipnr: chipnumber to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
 void ca_nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	switch (chipnr) {
		case -1:
		case 0:
			CHIP_EN = NFLASH_CHIP0_EN;
			break;
		case 1:
			CHIP_EN = NFLASH_CHIP1_EN;
			break;

		default:
			/* BUG(); */
			CHIP_EN = NFLASH_CHIP0_EN;
	}
}

/**
 * nand_write_byte - [DEFAULT] write single byte to chip
 * @mtd: MTD device structure
 * @byte: value to write
 *
 * Default function to write a byte to I/O[7:0]
 */
static void nand_write_byte(struct mtd_info *mtd, uint8_t byte)
{
	struct nand_chip *chip = mtd->priv;

	chip->write_buf(mtd, &byte, 1);
}

/**
 * nand_write_byte16 - [DEFAULT] write single byte to a chip with width 16
 * @mtd: MTD device structure
 * @byte: value to write
 *
 * Default function to write a byte to I/O[7:0] on a 16-bit wide chip.
 */
static void nand_write_byte16(struct mtd_info *mtd, uint8_t byte)
{
	struct nand_chip *chip = mtd->priv;
	uint16_t word = byte;

	/*
	 * It's not entirely clear what should happen to I/O[15:8] when writing
	 * a byte. The ONFi spec (Revision 3.1; 2012-09-19, Section 2.16) reads:
	 *
	 *    When the host supports a 16-bit bus width, only data is
	 *    transferred at the 16-bit width. All address and command line
	 *    transfers shall use only the lower 8-bits of the data bus. During
	 *    command transfers, the host may place any value on the upper
	 *    8-bits of the data bus. During address transfers, the host shall
	 *    set the upper 8-bits of the data bus to 00h.
	 *
	 * One user of the write_byte callback is nand_onfi_set_features. The
	 * four parameters are specified to be written to I/O[7:0], but this is
	 * neither an address nor a command transfer. Let's assume a 0 on the
	 * upper I/O lines is OK.
	 */
	chip->write_buf(mtd, (uint8_t *) & word, 2);
}

/**
 * ca_nand_write_buf - [DEFAULT] write buffer to chip
 * @mtd: MTD device structure
 * @buf: data buffer
 * @len: number of bytes to write
 *
 * Default write function for 8bit buswidth.
 */
 void ca_nand_write_buf(struct mtd_info *mtd, const uint8_t * buf,
							  int len)
{
	int i, page = 0, col = 0;
	struct nand_chip *chip = mtd->priv;
	size_t retlen;
	retlen = 0;

	//check address
	if (chkAddrValid(mtd, &nand_page)) {
		return;
	}
	if (len <= (mtd->writesize + mtd->oobsize)) {

		page = nand_page;
		col = nand_col;
		chip->pagebuf = -1;
		chip->ecc.read_page(mtd, chip, chip->buffers->databuf, 1, page);

		for (i = 0; i < len; i++)
			chip->buffers->databuf[col + i] = buf[i];

		chip->ecc.write_page(mtd, chip, chip->buffers->databuf, 1, page);
	}
}

/**
 * ca_nand_read_buf - [DEFAULT] read chip data into buffer
 * @mtd: MTD device structure
 * @buf: buffer to store date
 * @len: number of bytes to read
 *
 * Default read function for 8bit buswidth.
 */
 void ca_nand_read_buf(struct mtd_info *mtd, uint8_t * buf, int len)
{
	int page, col;
	struct nand_chip *this = mtd->priv;

	if (len <= (mtd->writesize + mtd->oobsize)) {
		page = nand_page;
		col = nand_col;

		this->ecc.read_page(mtd, this, this->buffers->databuf, 1, page);
		memcpy(buf, &(this->buffers->databuf[col]), len);

	}
}

/**
 * nand_write_buf16 - [DEFAULT] write buffer to chip
 * @mtd: MTD device structure
 * @buf: data buffer
 * @len: number of bytes to write
 *
 * Default write function for 16bit buswidth.
 */
static void nand_write_buf16(struct mtd_info *mtd, const uint8_t * buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;

	iowrite16_rep(chip->IO_ADDR_W, p, len >> 1);
}

/**
 * nand_read_buf16 - [DEFAULT] read chip data into buffer
 * @mtd: MTD device structure
 * @buf: buffer to store date
 * @len: number of bytes to read
 *
 * Default read function for 16bit buswidth.
 */
static void nand_read_buf16(struct mtd_info *mtd, uint8_t * buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;

	ioread16_rep(chip->IO_ADDR_R, p, len >> 1);
}

/**
 * ca_nand_block_bad - [DEFAULT] Read bad block marker from the chip
 * @mtd: MTD device structure
 * @ofs: offset from device start
 * @getchip: 0, if the chip is already selected
 *
 * Check, if the block is bad.
 */
 int ca_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int page, chipnr, res = 0;
	struct nand_chip *chip = mtd->priv;

	if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
		ofs += mtd->erasesize - mtd->writesize;

	page = (int)(ofs >> chip->page_shift) & chip->pagemask;

#ifdef CONFIG_NAND_HIDE_BAD
    uint32_t pa_in_out = page;
    res = xlat_offset_to_skip_factory_bad(mtd, &pa_in_out);
    if(res == -EINVAL){
        pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
        return res;
    }
    if(page != pa_in_out){
        pr_debug("Read xlat: 0x%x => 0x%x\n",page,pa_in_out);
    }
    page = pa_in_out;
#endif

	if (getchip) {
		chipnr = (int)(ofs >> chip->chip_shift);

		nand_get_device(mtd, FL_READING);

		/* Select the NAND device */
		chip->select_chip(mtd, chipnr);
	}

	ca_nand_read_oob_std(mtd, chip, page);
#if 0
	do {
		if (chip->options & NAND_BUSWIDTH_16) {
			chip->cmdfunc(mtd, NAND_CMD_READOOB,
						  chip->badblockpos & 0xFE, page);
			bad = cpu_to_le16(chip->read_word(mtd));
			if (chip->badblockpos & 0x1)
				bad >>= 8;
			else
				bad &= 0xFF;
		} else {
			chip->cmdfunc(mtd, NAND_CMD_READOOB, chip->badblockpos, page);
			bad = chip->read_byte(mtd);
		}

		if (likely(chip->badblockbits == 8))
			res = bad != 0xFF;
		else
			res = hweight8(bad) < chip->badblockbits;
		ofs += mtd->writesize;
		page = (int)(ofs >> chip->page_shift) & chip->pagemask;
		i++;
	} while (!res && i < 2 && (chip->bbt_options & NAND_BBT_SCAN2NDPAGE));
#endif

	if (getchip) {
		chip->select_chip(mtd, -1);
		nand_release_device(mtd);
	}
	if (chip->oob_poi[chip->badblockpos] != 0xff)
		res = 1;

	return res;
}

/**
 * nand_default_block_markbad - [DEFAULT] mark a block bad via bad block marker
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This is the default implementation, which can be overridden by a hardware
 * specific driver. It provides the details for writing a bad block marker to a
 * block.
 */
 int ca_nand_default_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	struct mtd_oob_ops ops;
	uint8_t buf[2] = { 0, 0 };
	int block, ret;

	unsigned int chunk_id = ((int) ofs) >> chip->page_shift;
	//check address
	if (chkAddrValid(mtd, &chunk_id)) {
		return -EIO;
	}

	/* Get block number */
	block = (int)(ofs >> chip->bbt_erase_shift);
	if (chip->bbt)
		chip->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

	/* No BBT, just update to oob */
	/* We write two bytes, so we dont have to mess with 16 bit
	 * access
	 */
	memset(&ops, 0, sizeof(struct mtd_oob_ops));
	ofs += mtd->oobsize;
	ops.len = ops.ooblen = 2;
	ops.oobbuf = buf;
	ops.ooboffs = chip->badblockpos & ~0x01;

	ret = ca_nand_do_write_oob(mtd, ofs, &ops);

	if (!ret)
		mtd->ecc_stats.badblocks++;

	return ret;
}

/**
 * nand_block_markbad_lowlevel - mark a block bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This function performs the generic NAND bad block marking steps (i.e., bad
 * block table(s) and/or marker(s)). We only allow the hardware driver to
 * specify how to write bad block markers to OOB (chip->block_markbad).
 *
 * We try operations in the following order:
 *  (1) erase the affected block, to allow OOB marker to be written cleanly
 *  (2) write bad block marker to OOB area of affected block (unless flag
 *      NAND_BBT_NO_OOB_BBM is present)
 *  (3) update the BBT
 * Note that we retain the first error encountered in (2) or (3), finish the
 * procedures, and dump the error in the end.
*/
static int nand_block_markbad_lowlevel(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	int res, ret = 0;

	unsigned int chunk_id = ((int) ofs) >> chip->page_shift;
	//check address
	if (chkAddrValid(mtd, &chunk_id)) {
		return -EIO;
	}
	if (!(chip->bbt_options & NAND_BBT_NO_OOB_BBM)) {
		struct erase_info einfo;

		/* Attempt erase before marking OOB */
		memset(&einfo, 0, sizeof(einfo));
		einfo.mtd = mtd;
		einfo.addr = ofs;
		einfo.len = 1ULL << chip->phys_erase_shift;
		ca_nand_erase_nand(mtd, &einfo, 0);

		/* Write bad block marker to OOB */
		nand_get_device(mtd, FL_WRITING);
		ret = chip->block_markbad(mtd, ofs);
		nand_release_device(mtd);
	}

	/* Mark block bad in BBT */
	if (chip->bbt) {
		res = nand_markbad_bbt(mtd, ofs);
		if (!ret)
			ret = res;
	}

	if (!ret)
		mtd->ecc_stats.badblocks++;

	return ret;
}

/**
 * ca_nand_check_wp - [GENERIC] check if the chip is write protected
 * @mtd: MTD device structure
 *
 * Check, if the device is write protected. The function expects, that the
 * device is already selected.
 */
 int ca_nand_check_wp(struct mtd_info *mtd)
{
	/* Check the WP bit */
	int ready;
	unsigned long timeo;

	check_flash_ctrl_status();

	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */

	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
	nf_cnt.bf.nflashRegDataCount = NCNT_DATA_1;
	nf_cnt.bf.nflashRegAddrCount = NCNT_EMPTY_ADDR;
	nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;
	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);

	nf_cmd.wrd = 0;
	nf_cmd.bf.nflashRegCmd0 = NAND_CMD_STATUS;
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);

	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = CHIP_EN;
	/* nf_access.bf.nflashDirWr = ; */
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;
	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);

	flash_start.wrd = 0;
	flash_start.bf.nflashRegReq = FLASH_GO;
	flash_start.bf.nflashRegCmd = FLASH_RD;
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

	timeo = jiffies + HZ;
	do {
		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		if (!flash_start.bf.nflashRegReq)
			break;
	} while (time_before(jiffies, timeo));

	ready = read_flash_ctrl_reg(FLASH_NF_DATA) & 0xff;

	return (ready & NAND_STATUS_WP) ? 0 : 1;
}

/**
 * nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * Check if the block is mark as reserved.
 */
static int nand_block_isreserved(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;

	if (!chip->bbt)
		return 0;
	/* Return info from the table */
	return nand_isreserved_bbt(mtd, ofs);
}

/**
 * nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 * @getchip: 0, if the chip is already selected
 * @allowbbt: 1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int nand_block_checkbad(struct mtd_info *mtd, loff_t ofs, int getchip,
							   int allowbbt)
{
	struct nand_chip *chip = mtd->priv;

	if (!chip->bbt)
		return chip->block_bad(mtd, ofs, getchip);

	/* Return info from the table */
	return nand_isbad_bbt(mtd, ofs, allowbbt);
}

/**
 * panic_nand_wait_ready - [GENERIC] Wait for the ready pin after commands.
 * @mtd: MTD device structure
 * @timeo: Timeout
 *
 * Helper function for nand_wait_ready used when needing to wait in interrupt
 * context.
 */
#if 0
static void panic_nand_wait_ready(struct mtd_info *mtd, unsigned long timeo)
{
	struct nand_chip *chip = mtd->priv;
	int i;

	/* Wait for the device to get ready */
	for (i = 0; i < timeo; i++) {
		if (chip->dev_ready(mtd))
			break;
		touch_softlockup_watchdog();
		mdelay(1);
	}
}
#endif

/**
 * ca_nand_command - [DEFAULT] Send command to NAND device
 * @mtd: MTD device structure
 * @command: the command to be sent
 * @column: the column address for this command, -1 if none
 * @page_addr: the page address for this command, -1 if none
 *
 * Send command to NAND device. This function is used for small page devices
 * (512 Bytes per page).
 */
 void ca_nand_command(struct mtd_info *mtd, unsigned int command,
							int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;
	int ctrl = NAND_CTRL_CLE | NAND_CTRL_CHANGE;

	//TODO  chk command type and check address
	/*
	 * Write out the command to the device.
	 */
	if (command == NAND_CMD_SEQIN) {
		int readcmd;

		if (column >= mtd->writesize) {
			/* OOB area */
			column -= mtd->writesize;
			readcmd = NAND_CMD_READOOB;
		} else if (column < 256) {
			/* First 256 bytes --> READ0 */
			readcmd = NAND_CMD_READ0;
		} else {
			column -= 256;
			readcmd = NAND_CMD_READ1;
		}
		chip->cmd_ctrl(mtd, readcmd, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
	}
	chip->cmd_ctrl(mtd, command, ctrl);

	/*
	 * Address cycle, when necessary
	 */
	ctrl = NAND_CTRL_ALE | NAND_CTRL_CHANGE;
	/* Serially input address */
	if (column != -1) {
		/* Adjust columns for 16 bit buswidth */
		if (chip->options & NAND_BUSWIDTH_16)
			column >>= 1;
		chip->cmd_ctrl(mtd, column, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
		nand_col = column;
	}
	if (page_addr != -1) {
		chip->cmd_ctrl(mtd, page_addr, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
		chip->cmd_ctrl(mtd, page_addr >> 8, ctrl);
		/* One more address cycle for devices > 32MiB */
		if (chip->chipsize > (32 << 20))
			chip->cmd_ctrl(mtd, page_addr >> 16, ctrl);

		nand_page = page_addr;
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay
	 */
	switch (command) {

		case NAND_CMD_PAGEPROG:
		case NAND_CMD_ERASE1:
		case NAND_CMD_ERASE2:
		case NAND_CMD_SEQIN:
			//check address
			if (chkAddrValid(mtd, &page_addr)) {
				nand_page = page_addr;
				return;
			}
		case NAND_CMD_STATUS:
			/*
			 * Write out the command to the device.
			 */
			if (column != -1 || page_addr != -1) {

				/* Serially input address */
				if (column != -1)
					/* FLASH_WRITE_REG(NFLASH_ADDRESS,column); */
					nand_col = column;

				if (page_addr != -1)
					/* FLASH_WRITE_REG(NFLASH_ADDRESS,opcode|(page_addr<<8)); */
					nand_page = page_addr;

			}
			return;

		case NAND_CMD_RESET:
			check_flash_ctrl_status();
			udelay(chip->chip_delay);
			write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);
			nf_cnt.wrd = 0;
			nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
			nf_cnt.bf.nflashRegDataCount = NCNT_EMPTY_DATA;
			nf_cnt.bf.nflashRegAddrCount = NCNT_EMPTY_ADDR;
			nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;
			write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);

			nf_cmd.wrd = 0;
			nf_cmd.bf.nflashRegCmd0 = NAND_CMD_RESET;
			write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
			nf_addr1.wrd = 0;
			write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x00 */
			nf_addr2.wrd = 0;
			write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);	/* `write address 0x00 */

			nf_access.wrd = 0;
			nf_access.bf.nflashCeAlt = CHIP_EN;
			/* nf_access.bf.nflashDirWr = ; */
			nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;

			write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);
			flash_start.wrd = 0;
			flash_start.bf.nflashRegReq = FLASH_GO;
			flash_start.bf.nflashRegCmd = FLASH_WT;
			write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

			flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
			while (flash_start.bf.nflashRegReq) {
				udelay(1);
				schedule();
				flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
			}

			udelay(100);
			break;

			/* This applies to read commands */
		default:
			/*
			 * If we don't have access to the busy pin, we apply the given
			 * command delay
			 */
			if (!chip->dev_ready) {
				udelay(chip->chip_delay);
				return;
			}
	}
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */

	udelay(100);
	ca_nand_wait_ready(mtd);
}

/**
 * nand_command_lp - [DEFAULT] Send command to NAND large page device
 * @mtd: MTD device structure
 * @command: the command to be sent
 * @column: the column address for this command, -1 if none
 * @page_addr: the page address for this command, -1 if none
 *
 * Send command to NAND device. This is the version for the new large page
 * devices. We don't have the separate regions as we have in the small page
 * devices. We must emulate NAND_CMD_READOOB to keep the code compatible.
 */
static void nand_command_lp(struct mtd_info *mtd, unsigned int command,
							int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}
	switch (command) {

		case NAND_CMD_CACHEDPROG:
		case NAND_CMD_PAGEPROG:
		case NAND_CMD_ERASE1:
		case NAND_CMD_ERASE2:
			nand_page = page_addr;
			//check address
			if (chkAddrValid(mtd, &page_addr)) {
				return;
			}
		default:
			break;
	}

	/* Command latch cycle */
	chip->cmd_ctrl(mtd, command, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	if (column != -1 || page_addr != -1) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (chip->options & NAND_BUSWIDTH_16 && !nand_opcode_8bits(command))
				column >>= 1;
			chip->cmd_ctrl(mtd, column, ctrl);
			ctrl &= ~NAND_CTRL_CHANGE;
			chip->cmd_ctrl(mtd, column >> 8, ctrl);
			nand_col = column;
		}
		if (page_addr != -1) {
			chip->cmd_ctrl(mtd, page_addr, ctrl);
			chip->cmd_ctrl(mtd, page_addr >> 8, NAND_NCE | NAND_ALE);
			/* One more address cycle for devices > 128MiB */
			if (chip->chipsize > (128 << 20))
				chip->cmd_ctrl(mtd, page_addr >> 16, NAND_NCE | NAND_ALE);
			nand_page = page_addr;
		}
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * Program and erase have their own busy handlers status, sequential
	 * in, and deplete1 need no delay.
	 */
	switch (command) {

		case NAND_CMD_CACHEDPROG:
		case NAND_CMD_PAGEPROG:
		case NAND_CMD_ERASE1:
		case NAND_CMD_ERASE2:
		case NAND_CMD_SEQIN:
		case NAND_CMD_RNDIN:
		case NAND_CMD_STATUS:
			return;

		case NAND_CMD_RESET:
			if (chip->dev_ready)
				break;
			udelay(chip->chip_delay);
			chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
						   NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
			chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
			while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
			return;

		case NAND_CMD_RNDOUT:
			/* No ready / busy check necessary */
			chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART,
						   NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
			chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
			return;

		case NAND_CMD_READ0:
			chip->cmd_ctrl(mtd, NAND_CMD_READSTART,
						   NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
			chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

			/* This applies to read commands */
		default:
			/*
			 * If we don't have access to the busy pin, we apply the given
			 * command delay.
			 */
			if (!chip->dev_ready) {
				udelay(chip->chip_delay);
				return;
			}
	}

	/*
	 * Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine.
	 */
	ndelay(100);

	nand_wait_ready(mtd);
}

/**
 * panic_nand_get_device - [GENERIC] Get chip for selected access
 * @chip: the nand chip descriptor
 * @mtd: MTD device structure
 * @new_state: the state which is requested
 *
 * Used when in panic, no locks are taken.
 */
static void panic_nand_get_device(struct nand_chip *chip,
								  struct mtd_info *mtd, int new_state)
{
	/* Hardware controller shared among independent devices */
	chip->controller->active = chip;
	chip->state = new_state;
}

/**
 * nand_get_device - [GENERIC] Get chip for selected access
 * @mtd: MTD device structure
 * @new_state: the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int nand_get_device(struct mtd_info *mtd, int new_state)
{
	struct nand_chip *chip = mtd->priv;
	spinlock_t *lock = &chip->controller->lock;
	wait_queue_head_t *wq = &chip->controller->wq;
	DECLARE_WAITQUEUE(wait, current);
  retry:
	spin_lock(lock);

	/* Hardware controller shared among independent devices */
	if (!chip->controller->active)
		chip->controller->active = chip;

	if (chip->controller->active == chip && chip->state == FL_READY) {
		chip->state = new_state;
		spin_unlock(lock);
		return 0;
	}
	if (new_state == FL_PM_SUSPENDED) {
		if (chip->controller->active->state == FL_PM_SUSPENDED) {
			chip->state = FL_PM_SUSPENDED;
			spin_unlock(lock);
			return 0;
		}
	}
	set_current_state(TASK_UNINTERRUPTIBLE);
	add_wait_queue(wq, &wait);
	spin_unlock(lock);
	schedule();
	remove_wait_queue(wq, &wait);
	goto retry;
}

/**
 * panic_nand_wait - [GENERIC] wait until the command is done
 * @mtd: MTD device structure
 * @chip: NAND chip structure
 * @timeo: timeout
 *
 * Wait for command done. This is a helper function for nand_wait used when
 * we are in interrupt context. May happen when in panic and trying to write
 * an oops through mtdoops.
 */
static void panic_nand_wait(struct mtd_info *mtd, struct nand_chip *chip,
							unsigned long timeo)
{
	int i;
	for (i = 0; i < timeo; i++) {
		if (chip->dev_ready) {
			if (chip->dev_ready(mtd))
				break;
		} else {
			if (chip->read_byte(mtd) & NAND_STATUS_READY)
				break;
		}
		mdelay(1);
	}
}

/**
 * nand_wait - [DEFAULT] wait until the command is done
 * @mtd: MTD device structure
 * @chip: NAND chip structure
 *
 * Wait for command done. This applies to erase and program only. Erase can
 * take up to 400ms and program up to 20ms according to general NAND and
 * SmartMedia specs.
 */
 int ca_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	unsigned long timeo = jiffies;
	struct nand_chip *this = mtd->priv;
	/* unsigned long        timeo = jiffies + 2; */
	int status, state = chip->state;

	if (state == FL_ERASING)
		timeo += (HZ * 400) / 1000;
	else
		timeo += (HZ * 20) / 1000;

	/* wait until command is processed or timeout occures */
	do {
		if (this->dev_ready(mtd))
			break;
		touch_softlockup_watchdog();
	} while (time_before(jiffies, timeo));

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	udelay(100);

	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	while (time_before(jiffies, timeo)) {
		if (chip->dev_ready) {
			if (chip->dev_ready(mtd))
				break;
		} else {
			/* if (chip->read_byte(mtd) & NAND_STATUS_READY) */
			break;
		}
		cond_resched();
	}

	/* status = (int)chip->read_byte(mtd); */
	/* return status; */
	status = read_flash_ctrl_reg(FLASH_NF_DATA) & 0xff;
	return status;
}

/**
 * __nand_unlock - [REPLACEABLE] unlocks specified locked blocks
 * @mtd: mtd info
 * @ofs: offset to start unlock from
 * @len: length to unlock
 * @invert: when = 0, unlock the range of blocks within the lower and
 *                    upper boundary address
 *          when = 1, unlock the range of blocks outside the boundaries
 *                    of the lower and upper boundary address
 *
 * Returs unlock status.
 */
static int __nand_unlock(struct mtd_info *mtd, loff_t ofs,
						 uint64_t len, int invert)
{
	int ret = 0;
	int status, page;
	struct nand_chip *chip = mtd->priv;

	/* Submit address of first page to unlock */
	page = ofs >> chip->page_shift;
	chip->cmdfunc(mtd, NAND_CMD_UNLOCK1, -1, page & chip->pagemask);

	/* Submit address of last page to unlock */
	page = (ofs + len) >> chip->page_shift;
	chip->cmdfunc(mtd, NAND_CMD_UNLOCK2, -1, (page | invert) & chip->pagemask);

	/* Call wait ready function */
	status = chip->waitfunc(mtd, chip);
	/* See if device thinks it succeeded */
	if (status & NAND_STATUS_FAIL) {
		pr_debug("%s: error status = 0x%08x\n", __func__, status);
		ret = -EIO;
	}

	return ret;
}

/**
 * ca_nand_unlock - [REPLACEABLE] unlocks specified locked blocks
 * @mtd: mtd info
 * @ofs: offset to start unlock from
 * @len: length to unlock
 *
 * Returns unlock status.
 */
int ca_nand_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	int ret = 0;
	int chipnr;
	struct nand_chip *chip = mtd->priv;

	pr_debug("%s: start = 0x%012llx, len = %llu\n",
			 __func__, (unsigned long long)ofs, len);

	if (check_offs_len(mtd, ofs, len))
		ret = -EINVAL;

	/* Align to last block address if size addresses end of the device */
	if (ofs + len == mtd->size)
		len -= mtd->erasesize;

	nand_get_device(mtd, FL_UNLOCKING);

	/* Shift to get chip number */
	chipnr = ofs >> chip->chip_shift;

	chip->select_chip(mtd, chipnr);

	/*
	 * Reset the chip.
	 * If we want to check the WP through READ STATUS and check the bit 7
	 * we must reset the chip
	 * some operation can also clear the bit 7 of status register
	 * eg. erase/program a locked block
	 */
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Check, if it is write protected */
	if (ca_nand_check_wp(mtd)) {
		pr_debug("%s: device is write protected!\n", __func__);
		ret = -EIO;
		goto out;
	}

	ret = __nand_unlock(mtd, ofs, len, 0);

  out:
	chip->select_chip(mtd, -1);
	nand_release_device(mtd);

	return ret;
}

EXPORT_SYMBOL(ca_nand_unlock);

/**
 * ca_nand_lock - [REPLACEABLE] locks all blocks present in the device
 * @mtd: mtd info
 * @ofs: offset to start unlock from
 * @len: length to unlock
 *
 * This feature is not supported in many NAND parts. 'Micron' NAND parts do
 * have this feature, but it allows only to lock all blocks, not for specified
 * range for block. Implementing 'lock' feature by making use of 'unlock', for
 * now.
 *
 * Returns lock status.
 */
int ca_nand_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	int ret = 0;
	int chipnr, status, page;
	struct nand_chip *chip = mtd->priv;

	pr_debug("%s: start = 0x%012llx, len = %llu\n",
			 __func__, (unsigned long long)ofs, len);

	if (check_offs_len(mtd, ofs, len))
		ret = -EINVAL;

	nand_get_device(mtd, FL_LOCKING);

	/* Shift to get chip number */
	chipnr = ofs >> chip->chip_shift;

	chip->select_chip(mtd, chipnr);

	/*
	 * Reset the chip.
	 * If we want to check the WP through READ STATUS and check the bit 7
	 * we must reset the chip
	 * some operation can also clear the bit 7 of status register
	 * eg. erase/program a locked block
	 */
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Check, if it is write protected */
	if (ca_nand_check_wp(mtd)) {
		pr_debug("%s: device is write protected!\n", __func__);
		status = MTD_ERASE_FAILED;
		ret = -EIO;
		goto out;
	}

	/* Submit address of first page to lock */
	page = ofs >> chip->page_shift;
	chip->cmdfunc(mtd, NAND_CMD_LOCK, -1, page & chip->pagemask);

	/* Call wait ready function */
	status = chip->waitfunc(mtd, chip);
	/* See if device thinks it succeeded */
	if (status & NAND_STATUS_FAIL) {
		pr_debug("%s: error status = 0x%08x\n", __func__, status);
		ret = -EIO;
		goto out;
	}

	ret = __nand_unlock(mtd, ofs, len, 0x1);

  out:
	chip->select_chip(mtd, -1);
	nand_release_device(mtd);

	return ret;
}

EXPORT_SYMBOL(ca_nand_lock);

/**
 * nand_read_page_raw - [INTERN] read raw page data without ecc
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: buffer to store read data
 * @oob_required: caller requires OOB data read to chip->oob_poi
 * @page: page number to read
 *
 * Not for syndrome calculating ECC controllers, which use a special oob layout.
 */
static int nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
							  uint8_t * buf, int oob_required, int page)
{
	chip->read_buf(mtd, buf, mtd->writesize);
	if (oob_required)
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	return 0;
}

/**
 * nand_read_page_raw_syndrome - [INTERN] read raw page data without ecc
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: buffer to store read data
 * @oob_required: caller requires OOB data read to chip->oob_poi
 * @page: page number to read
 *
 * We need a special oob layout and handling even when OOB isn't used.
 */
static int nand_read_page_raw_syndrome(struct mtd_info *mtd,
									   struct nand_chip *chip, uint8_t * buf,
									   int oob_required, int page)
{
	int eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	uint8_t *oob = chip->oob_poi;
	int steps, size;

	for (steps = chip->ecc.steps; steps > 0; steps--) {
		chip->read_buf(mtd, buf, eccsize);
		buf += eccsize;

		if (chip->ecc.prepad) {
			chip->read_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}

		chip->read_buf(mtd, oob, eccbytes);
		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->read_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	size = mtd->oobsize - (oob - chip->oob_poi);
	if (size)
		chip->read_buf(mtd, oob, size);

	return 0;
}

/**
 * nand_read_page_swecc - [REPLACEABLE] software ECC based page read function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: buffer to store read data
 * @oob_required: caller requires OOB data read to chip->oob_poi
 * @page: page number to read
 */
static int nand_read_page_swecc(struct mtd_info *mtd, struct nand_chip *chip,
								uint8_t * buf, int oob_required, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	unsigned int max_bitflips = 0;

	chip->ecc.read_page_raw(mtd, chip, buf, 1, page);

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	eccsteps = chip->ecc.steps;
	p = buf;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		stat = chip->ecc.correct(mtd, p, &ecc_code[i], &ecc_calc[i]);
		if (stat < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max_t(unsigned int, max_bitflips, stat);
		}
	}
	return max_bitflips;
}

/**
 * nand_read_subpage - [REPLACEABLE] ECC based sub-page read function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @data_offs: offset of requested data within the page
 * @readlen: data length
 * @bufpoi: buffer to store read data
 * @page: page number to read
 */
static int nand_read_subpage(struct mtd_info *mtd, struct nand_chip *chip,
							 uint32_t data_offs, uint32_t readlen,
							 uint8_t * bufpoi, int page)
{
	int start_step, end_step, num_steps;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint8_t *p;
	int data_col_addr, i, gaps = 0;
	int datafrag_len, eccfrag_len, aligned_len, aligned_pos;
	int busw = (chip->options & NAND_BUSWIDTH_16) ? 2 : 1;
	int index;
	unsigned int max_bitflips = 0;

	/* Column address within the page aligned to ECC size (256bytes) */
	start_step = data_offs / chip->ecc.size;
	end_step = (data_offs + readlen - 1) / chip->ecc.size;
	num_steps = end_step - start_step + 1;
	index = start_step * chip->ecc.bytes;

	/* Data size aligned to ECC ecc.size */
	datafrag_len = num_steps * chip->ecc.size;
	eccfrag_len = num_steps * chip->ecc.bytes;

	data_col_addr = start_step * chip->ecc.size;
	/* If we read not a page aligned data */
	if (data_col_addr != 0)
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, data_col_addr, -1);

	p = bufpoi + data_col_addr;
	chip->read_buf(mtd, p, datafrag_len);

	/* Calculate ECC */
	for (i = 0; i < eccfrag_len; i += chip->ecc.bytes, p += chip->ecc.size)
		chip->ecc.calculate(mtd, p, &chip->buffers->ecccalc[i]);

	/*
	 * The performance is faster if we position offsets according to
	 * ecc.pos. Let's make sure that there are no gaps in ECC positions.
	 */
	for (i = 0; i < eccfrag_len - 1; i++) {
		if (eccpos[i + index] + 1 != eccpos[i + index + 1]) {
			gaps = 1;
			break;
		}
	}
	if (gaps) {
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize, -1);
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	} else {
		/*
		 * Send the command to read the particular ECC bytes take care
		 * about buswidth alignment in read_buf.
		 */
		aligned_pos = eccpos[index] & ~(busw - 1);
		aligned_len = eccfrag_len;
		if (eccpos[index] & (busw - 1))
			aligned_len++;
		if (eccpos[index + (num_steps * chip->ecc.bytes)] & (busw - 1))
			aligned_len++;

		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize + aligned_pos, -1);
		chip->read_buf(mtd, &chip->oob_poi[aligned_pos], aligned_len);
	}

	for (i = 0; i < eccfrag_len; i++)
		chip->buffers->ecccode[i] = chip->oob_poi[eccpos[i + index]];

	p = bufpoi + data_col_addr;
	for (i = 0; i < eccfrag_len; i += chip->ecc.bytes, p += chip->ecc.size) {
		int stat;

		stat = chip->ecc.correct(mtd, p,
								 &chip->buffers->ecccode[i],
								 &chip->buffers->ecccalc[i]);
		if (stat < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max_t(unsigned int, max_bitflips, stat);
		}
	}
	return max_bitflips;
}

/**
 * nand_read_page_hwecc - [REPLACEABLE] hardware ECC based page read function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: buffer to store read data
 * @oob_required: caller requires OOB data read to chip->oob_poi
 * @page: page number to read
 *
 * Not for syndrome calculating ECC controllers which need a special oob layout.
 */
static int nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
								uint8_t * buf, int oob_required, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	unsigned int max_bitflips = 0;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);
	}
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	eccsteps = chip->ecc.steps;
	p = buf;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		stat = chip->ecc.correct(mtd, p, &ecc_code[i], &ecc_calc[i]);
		if (stat < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max_t(unsigned int, max_bitflips, stat);
		}
	}
	return max_bitflips;
}

/**
 * nand_read_page_hwecc_oob_first - [REPLACEABLE] hw ecc, read oob first
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: buffer to store read data
 * @oob_required: caller requires OOB data read to chip->oob_poi
 * @page: page number to read
 *
 * Hardware ECC for large page chips, require OOB to be read first. For this
 * ECC mode, the write_page method is re-used from ECC_HW. These methods
 * read/write ECC from the OOB area, unlike the ECC_HW_SYNDROME support with
 * multiple ECC steps, follows the "infix ECC" scheme and reads/writes ECC from
 * the data area, by overwriting the NAND manufacturer bad block markings.
 */
static int nand_read_page_hwecc_oob_first(struct mtd_info *mtd,
										  struct nand_chip *chip, uint8_t * buf,
										  int oob_required, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	unsigned int max_bitflips = 0;

	/* Read the OOB area first */
	chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);

		stat = chip->ecc.correct(mtd, p, &ecc_code[i], NULL);
		if (stat < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max_t(unsigned int, max_bitflips, stat);
		}
	}
	return max_bitflips;
}

/**
 * nand_read_page_syndrome - [REPLACEABLE] hardware ECC syndrome based page read
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: buffer to store read data
 * @oob_required: caller requires OOB data read to chip->oob_poi
 * @page: page number to read
 *
 * The hw generator calculates the error syndrome automatically. Therefore we
 * need a special oob layout and handling.
 */
static int nand_read_page_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
								   uint8_t * buf, int oob_required, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;
	unsigned int max_bitflips = 0;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);

		if (chip->ecc.prepad) {
			chip->read_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}

		chip->ecc.hwctl(mtd, NAND_ECC_READSYN);
		chip->read_buf(mtd, oob, eccbytes);
		stat = chip->ecc.correct(mtd, p, oob, NULL);

		if (stat < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max_t(unsigned int, max_bitflips, stat);
		}

		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->read_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->read_buf(mtd, oob, i);

	return max_bitflips;
}

/**
 * nand_transfer_oob - [INTERN] Transfer oob to client buffer
 * @chip: nand chip structure
 * @oob: oob destination address
 * @ops: oob ops structure
 * @len: size of oob to transfer
 */
static uint8_t *nand_transfer_oob(struct nand_chip *chip, uint8_t * oob,
								  struct mtd_oob_ops *ops, size_t len)
{
	switch (ops->mode) {

		case MTD_OPS_PLACE_OOB:
		case MTD_OPS_RAW:
			memcpy(oob, chip->oob_poi + ops->ooboffs, len);
			return oob + len;

		case MTD_OPS_AUTO_OOB:{
				struct nand_oobfree *free = chip->ecc.layout->oobfree;
				uint32_t boffs = 0, roffs = ops->ooboffs;
				size_t bytes = 0;

				for (; free->length && len; free++, len -= bytes) {
					/* Read request not from offset 0? */
					if (unlikely(roffs)) {
						if (roffs >= free->length) {
							roffs -= free->length;
							continue;
						}
						boffs = free->offset + roffs;
						bytes = min_t(size_t, len, (free->length - roffs));
						roffs = 0;
					} else {
						bytes = min_t(size_t, len, free->length);
						boffs = free->offset;
					}
					memcpy(oob, chip->oob_poi + boffs, bytes);
					oob += bytes;
				}
				return oob;
			}
		default:
			BUG();
	}
	return NULL;
}

/**
 * nand_setup_read_retry - [INTERN] Set the READ RETRY mode
 * @mtd: MTD device structure
 * @retry_mode: the retry mode to use
 *
 * Some vendors supply a special command to shift the Vt threshold, to be used
 * when there are too many bitflips in a page (i.e., ECC error). After setting
 * a new threshold, the host should retry reading the page.
 */
static int nand_setup_read_retry(struct mtd_info *mtd, int retry_mode)
{
	struct nand_chip *chip = mtd->priv;

	pr_debug("setting READ RETRY mode %d\n", retry_mode);

	if (retry_mode >= chip->read_retries)
		return -EINVAL;

	if (!chip->setup_read_retry)
		return -EOPNOTSUPP;

	return chip->setup_read_retry(mtd, retry_mode);
}

/**
 * nand_do_read_ops - [INTERN] Read data with ECC
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob ops structure
 *
 * Internal function. Called with chip held.
 */
static int nand_do_read_ops(struct mtd_info *mtd, loff_t from,
							struct mtd_oob_ops *ops)
{
	int chipnr, page, realpage, col, bytes, aligned, oob_required;
	struct nand_chip *chip = mtd->priv;
	int ret = 0;
	uint32_t readlen = ops->len;
	uint32_t oobreadlen = ops->ooblen;
	uint32_t max_oobsize = ops->mode == MTD_OPS_AUTO_OOB ?
		mtd->oobavail : mtd->oobsize;

	uint8_t *bufpoi, *oob, *buf;
	int use_bufpoi;
	unsigned int max_bitflips = 0;
	int retry_mode = 0;
	bool ecc_fail = false;
    uint32_t pa_be4xlat __attribute((unused));
    uint32_t pa_in_out __attribute((unused));

	chipnr = (int)(from >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);

	realpage = (int)(from >> chip->page_shift);
	page = realpage & chip->pagemask;
    pa_be4xlat = page;
	col = (int)(from & (mtd->writesize - 1));

	buf = ops->datbuf;
	oob = ops->oobbuf;
	oob_required = oob ? 1 : 0;

	while (1) {
#ifdef CONFIG_NAND_HIDE_BAD
        pa_in_out = pa_be4xlat;
        ret = xlat_offset_to_skip_factory_bad(mtd, &pa_in_out);
        if(ret == -EINVAL){
            pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
            return ret;
        }
        if(pa_be4xlat != pa_in_out){
            pr_debug("Read xlat: 0x%x => 0x%x\n",pa_be4xlat,pa_in_out);
        }
        page = pa_in_out;
        pa_be4xlat++;
#endif

		unsigned int ecc_failures = mtd->ecc_stats.failed;

		bytes = min(mtd->writesize - col, readlen);
		aligned = (bytes == mtd->writesize);

		if (!aligned)
			use_bufpoi = 1;
		else if (chip->options & NAND_USE_BOUNCE_BUFFER)
			use_bufpoi = !virt_addr_valid(buf);
		else
			use_bufpoi = 0;

		/* Is the current page in the buffer? */
		if (realpage != chip->pagebuf || oob) {
			bufpoi = use_bufpoi ? chip->buffers->databuf : buf;

			if (use_bufpoi && aligned)
				pr_debug("%s: using read bounce buffer for buf@%p\n",
						 __func__, buf);

		  read_retry:
			chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);

			/*
			 * Now read the page into the buffer.  Absent an error,
			 * the read methods return max bitflips per ecc step.
			 */
			if (unlikely(ops->mode == MTD_OPS_RAW))
				ret = chip->ecc.read_page_raw(mtd, chip, bufpoi,
											  oob_required, page);
			else if (!aligned && NAND_HAS_SUBPAGE_READ(chip) && !oob)
				ret = chip->ecc.read_subpage(mtd, chip,
											 col, bytes, bufpoi, page);
			else
				ret = chip->ecc.read_page(mtd, chip, bufpoi,
										  oob_required, page);
			if (ret < 0) {
				if (use_bufpoi)
					/* Invalidate page cache */
					chip->pagebuf = -1;
				break;
			}

			max_bitflips = max_t(unsigned int, max_bitflips, ret);

			/* Transfer not aligned data */
			if (use_bufpoi) {
				if (!NAND_HAS_SUBPAGE_READ(chip) && !oob &&
					!(mtd->ecc_stats.failed - ecc_failures) &&
					(ops->mode != MTD_OPS_RAW)) {
					chip->pagebuf = realpage;
					chip->pagebuf_bitflips = ret;
				} else {
					/* Invalidate page cache */
					chip->pagebuf = -1;
				}
				memcpy(buf, chip->buffers->databuf + col, bytes);
			}

			if (unlikely(oob)) {
				int toread = min(oobreadlen, max_oobsize);

				if (toread) {
					oob = nand_transfer_oob(chip, oob, ops, toread);
					oobreadlen -= toread;
				}
			}

			if (chip->options & NAND_NEED_READRDY) {
				/* Apply delay or wait for ready/busy pin */
				if (!chip->dev_ready)
					udelay(chip->chip_delay);
				else
					nand_wait_ready(mtd);
			}

			if (mtd->ecc_stats.failed - ecc_failures) {
				if (retry_mode + 1 < chip->read_retries) {
					retry_mode++;
					ret = nand_setup_read_retry(mtd, retry_mode);
					if (ret < 0)
						break;

					/* Reset failures; retry */
					mtd->ecc_stats.failed = ecc_failures;
					goto read_retry;
				} else {
					/* No more retry modes; real failure */
					ecc_fail = true;
				}
			}

			buf += bytes;
		} else {
			memcpy(buf, chip->buffers->databuf + col, bytes);
			buf += bytes;
			max_bitflips = max_t(unsigned int, max_bitflips,
								 chip->pagebuf_bitflips);
		}

		readlen -= bytes;

		/* Reset to retry mode 0 */
		if (retry_mode) {
			ret = nand_setup_read_retry(mtd, 0);
			if (ret < 0)
				break;
			retry_mode = 0;
		}

		if (!readlen)
			break;

		/* For subsequent reads align to page boundary */
		col = 0;
		/* Increment page address */
		realpage++;

		page = realpage & chip->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}
	}
	chip->select_chip(mtd, -1);

	ops->retlen = ops->len - (size_t) readlen;
	if (oob)
		ops->oobretlen = ops->ooblen - oobreadlen;

	if (ret < 0)
		return ret;

	if (ecc_fail)
		return -EBADMSG;

	return max_bitflips;
}

/**
 * nand_read - [MTD Interface] MTD compatibility function for nand_do_read_ecc
 * @mtd: MTD device structure
 * @from: offset to read from
 * @len: number of bytes to read
 * @retlen: pointer to variable to store the number of read bytes
 * @buf: the databuffer to put data
 *
 * Get hold of the chip and call nand_do_read.
 */
static int nand_read(struct mtd_info *mtd, loff_t from, size_t len,
					 size_t * retlen, uint8_t * buf)
{
	struct mtd_oob_ops ops;
	int ret;

	nand_get_device(mtd, FL_READING);
	ops.len = len;
	ops.datbuf = buf;
	ops.oobbuf = NULL;
	ops.mode = MTD_OPS_PLACE_OOB;
	ret = nand_do_read_ops(mtd, from, &ops);
	*retlen = ops.retlen;
	nand_release_device(mtd);
	return ret;
}

/**
 * nand_read_oob_std - [REPLACEABLE] the most common OOB data read function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @page: page number to read
 */
static int nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
							 int page)
{
	chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	return 0;
}

/**
 * nand_read_oob_syndrome - [REPLACEABLE] OOB data read function for HW ECC
 *			    with syndromes
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @page: page number to read
 */
static int nand_read_oob_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
								  int page)
{
	uint8_t *buf = chip->oob_poi;
	int length = mtd->oobsize;
	int chunk = chip->ecc.bytes + chip->ecc.prepad + chip->ecc.postpad;
	int eccsize = chip->ecc.size;
	uint8_t *bufpoi = buf;
	int i, toread, sndrnd = 0, pos;

	chip->cmdfunc(mtd, NAND_CMD_READ0, chip->ecc.size, page);
	for (i = 0; i < chip->ecc.steps; i++) {
		if (sndrnd) {
			pos = eccsize + i * (eccsize + chunk);
			if (mtd->writesize > 512)
				chip->cmdfunc(mtd, NAND_CMD_RNDOUT, pos, -1);
			else
				chip->cmdfunc(mtd, NAND_CMD_READ0, pos, page);
		} else
			sndrnd = 1;
		toread = min_t(int, length, chunk);
		chip->read_buf(mtd, bufpoi, toread);
		bufpoi += toread;
		length -= toread;
	}
	if (length > 0)
		chip->read_buf(mtd, bufpoi, length);

	return 0;
}

/**
 * nand_write_oob_std - [REPLACEABLE] the most common OOB data write function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @page: page number to write
 */
static int nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
							  int page)
{
	int status = 0;
	const uint8_t *buf = chip->oob_poi;
	int length = mtd->oobsize;
	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);
	chip->write_buf(mtd, buf, length);
	/* Send command to program the OOB data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/**
 * nand_write_oob_syndrome - [REPLACEABLE] OOB data write function for HW ECC
 *			     with syndrome - only for large page flash
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @page: page number to write
 */
static int nand_write_oob_syndrome(struct mtd_info *mtd,
								   struct nand_chip *chip, int page)
{
	int chunk = chip->ecc.bytes + chip->ecc.prepad + chip->ecc.postpad;
	int eccsize = chip->ecc.size, length = mtd->oobsize;
	int i, len, pos, status = 0, sndcmd = 0, steps = chip->ecc.steps;
	const uint8_t *bufpoi = chip->oob_poi;

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}
	/*
	 * data-ecc-data-ecc ... ecc-oob
	 * or
	 * data-pad-ecc-pad-data-pad .... ecc-pad-oob
	 */
	if (!chip->ecc.prepad && !chip->ecc.postpad) {
		pos = steps * (eccsize + chunk);
		steps = 0;
	} else
		pos = eccsize;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, pos, page);
	for (i = 0; i < steps; i++) {
		if (sndcmd) {
			if (mtd->writesize <= 512) {
				uint32_t fill = 0xFFFFFFFF;

				len = eccsize;
				while (len > 0) {
					int num = min_t(int, len, 4);
					chip->write_buf(mtd, (uint8_t *) & fill, num);
					len -= num;
				}
			} else {
				pos = eccsize + i * (eccsize + chunk);
				chip->cmdfunc(mtd, NAND_CMD_RNDIN, pos, -1);
			}
		} else
			sndcmd = 1;
		len = min_t(int, length, chunk);
		chip->write_buf(mtd, bufpoi, len);
		bufpoi += len;
		length -= len;
	}
	if (length > 0)
		chip->write_buf(mtd, bufpoi, length);

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/**
 * nand_do_read_oob - [INTERN] NAND read out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operations description structure
 *
 * NAND read out-of-band data from the spare area.
 */
static int nand_do_read_oob(struct mtd_info *mtd, loff_t from,
							struct mtd_oob_ops *ops)
{
	int page, realpage, chipnr;
	struct nand_chip *chip = mtd->priv;
	struct mtd_ecc_stats stats;
	int readlen = ops->ooblen;
	int len;
	uint8_t *buf = ops->oobbuf;
	int ret = 0;
    uint32_t pa_be4xlat __attribute((unused));
    uint32_t pa_in_out __attribute((unused));

	pr_debug("%s: from = 0x%08Lx, len = %i\n",
			 __func__, (unsigned long long)from, readlen);

	stats = mtd->ecc_stats;

	if (ops->mode == MTD_OPS_AUTO_OOB)
		len = chip->ecc.layout->oobavail;
	else
		len = mtd->oobsize;

	if (unlikely(ops->ooboffs >= len)) {
		pr_debug("%s: attempt to start read outside oob\n", __func__);
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->size ||
				 ops->ooboffs + readlen > ((mtd->size >> chip->page_shift) -
										   (from >> chip->page_shift)) * len)) {
		pr_debug("%s: attempt to read beyond end of device\n", __func__);
		return -EINVAL;
	}

	chipnr = (int)(from >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);

	/* Shift to get page */
	realpage = (int)(from >> chip->page_shift);
	page = realpage & chip->pagemask;
    pa_be4xlat = page;

	while (1) {
#ifdef CONFIG_NAND_HIDE_BAD
        pa_in_out = pa_be4xlat;
        ret = xlat_offset_to_skip_factory_bad(mtd, &pa_in_out);
        if(ret == -EINVAL){
            pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
            return ret;
        }
        if(pa_be4xlat != pa_in_out){
            pr_debug("Read xlat: 0x%x => 0x%x\n",pa_be4xlat,pa_in_out);
        }
        page = pa_in_out;
        pa_be4xlat++;
#endif

		if (ops->mode == MTD_OPS_RAW)
			ret = chip->ecc.read_oob_raw(mtd, chip, page);
		else
			ret = chip->ecc.read_oob(mtd, chip, page);

		if (ret < 0)
			break;

		len = min(len, readlen);
		buf = nand_transfer_oob(chip, buf, ops, len);

		if (chip->options & NAND_NEED_READRDY) {
			/* Apply delay or wait for ready/busy pin */
			if (!chip->dev_ready)
				udelay(chip->chip_delay);
			else
				nand_wait_ready(mtd);
		}

		readlen -= len;
		if (!readlen)
			break;

		/* Increment page address */
		realpage++;

		page = realpage & chip->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}
	}
	chip->select_chip(mtd, -1);

	ops->oobretlen = ops->ooblen - readlen;

	if (ret < 0)
		return ret;

	if (mtd->ecc_stats.failed - stats.failed)
		return -EBADMSG;

	return mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0;
}

/**
 * nand_read_oob - [MTD Interface] NAND read data and/or out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operation description structure
 *
 * NAND read data and/or out-of-band data.
 */
static int nand_read_oob(struct mtd_info *mtd, loff_t from,
						 struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;

	ops->retlen = 0;

	/* Do not allow reads past end of device */
	if (ops->datbuf && (from + ops->len) > mtd->size) {
		pr_debug("%s: attempt to read beyond end of device\n", __func__);
		return -EINVAL;
	}

	nand_get_device(mtd, FL_READING);

	switch (ops->mode) {
		case MTD_OPS_PLACE_OOB:
		case MTD_OPS_AUTO_OOB:
		case MTD_OPS_RAW:
			break;

		default:
			goto out;
	}

	if (!ops->datbuf)
		ret = nand_do_read_oob(mtd, from, ops);
	else
		ret = nand_do_read_ops(mtd, from, ops);

  out:
	nand_release_device(mtd);
	return ret;
}

/**
 * nand_write_page_raw - [INTERN] raw page write function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: data buffer
 * @oob_required: must write chip->oob_poi to OOB
 *
 * Not for syndrome calculating ECC controllers, which use a special oob layout.
 */
static int nand_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
							   const uint8_t * buf, int oob_required, int page)
{

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}
	chip->write_buf(mtd, buf, mtd->writesize);
	if (oob_required)
		chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

/**
 * nand_write_page_raw_syndrome - [INTERN] raw page write function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: data buffer
 * @oob_required: must write chip->oob_poi to OOB
 *
 * We need a special oob layout and handling even when ECC isn't checked.
 */
static int nand_write_page_raw_syndrome(struct mtd_info *mtd,
										struct nand_chip *chip,
										const uint8_t * buf, int oob_required, int page)
{
	int eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	uint8_t *oob = chip->oob_poi;
	int steps, size;

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	for (steps = chip->ecc.steps; steps > 0; steps--) {
		chip->write_buf(mtd, buf, eccsize);
		buf += eccsize;

		if (chip->ecc.prepad) {
			chip->write_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}

		chip->write_buf(mtd, oob, eccbytes);
		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->write_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	size = mtd->oobsize - (oob - chip->oob_poi);
	if (size)
		chip->write_buf(mtd, oob, size);

	return 0;
}

/**
 * nand_write_page_swecc - [REPLACEABLE] software ECC based page write function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: data buffer
 * @oob_required: must write chip->oob_poi to OOB
 */
static int nand_write_page_swecc(struct mtd_info *mtd, struct nand_chip *chip,
								 const uint8_t * buf, int oob_required, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	/* Software ECC calculation */
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);

	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	return chip->ecc.write_page_raw(mtd, chip, buf, 1, page);
}

/**
 * nand_write_page_hwecc - [REPLACEABLE] hardware ECC based page write function
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: data buffer
 * @oob_required: must write chip->oob_poi to OOB
 */
static int nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
								 const uint8_t * buf, int oob_required, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);
	}

	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

/**
 * nand_write_subpage_hwecc - [REPLACABLE] hardware ECC based subpage write
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @offset:	column address of subpage within the page
 * @data_len:	data length
 * @buf:	data buffer
 * @oob_required: must write chip->oob_poi to OOB
 */
static int nand_write_subpage_hwecc(struct mtd_info *mtd,
									struct nand_chip *chip, uint32_t offset,
									uint32_t data_len, const uint8_t * buf,
									int oob_required, int page)
{
	uint8_t *oob_buf = chip->oob_poi;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	int ecc_size = chip->ecc.size;
	int ecc_bytes = chip->ecc.bytes;
	int ecc_steps = chip->ecc.steps;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint32_t start_step = offset / ecc_size;
	uint32_t end_step = (offset + data_len - 1) / ecc_size;
	int oob_bytes = mtd->oobsize / ecc_steps;
	int step, i;
	unsigned int chunk_id = (int)(offset>>(chip->page_shift));
	//check address
	if (chkAddrValid(mtd, &chunk_id)) {
		return -EIO;
	}


	for (step = 0; step < ecc_steps; step++) {
		/* configure controller for WRITE access */
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);

		/* write data (untouched subpages already masked by 0xFF) */
		chip->write_buf(mtd, buf, ecc_size);

		/* mask ECC of un-touched subpages by padding 0xFF */
		if ((step < start_step) || (step > end_step))
			memset(ecc_calc, 0xff, ecc_bytes);
		else
			chip->ecc.calculate(mtd, buf, ecc_calc);

		/* mask OOB of un-touched subpages by padding 0xFF */
		/* if oob_required, preserve OOB metadata of written subpage */
		if (!oob_required || (step < start_step) || (step > end_step))
			memset(oob_buf, 0xff, oob_bytes);

		buf += ecc_size;
		ecc_calc += ecc_bytes;
		oob_buf += oob_bytes;
	}

	/* copy calculated ECC for whole page to chip->buffer->oob */
	/* this include masked-value(0xFF) for unwritten subpages */
	ecc_calc = chip->buffers->ecccalc;
	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	/* write OOB buffer to NAND device */
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

/**
 * nand_write_page_syndrome - [REPLACEABLE] hardware ECC syndrome based page write
 * @mtd: mtd info structure
 * @chip: nand chip info structure
 * @buf: data buffer
 * @oob_required: must write chip->oob_poi to OOB
 *
 * The hw generator calculates the error syndrome automatically. Therefore we
 * need a special oob layout and handling.
 */
static int nand_write_page_syndrome(struct mtd_info *mtd,
									struct nand_chip *chip,
									const uint8_t * buf, int oob_required, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	const uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;
	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}


	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {

		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);

		if (chip->ecc.prepad) {
			chip->write_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}

		chip->ecc.calculate(mtd, p, oob);
		chip->write_buf(mtd, oob, eccbytes);
		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->write_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->write_buf(mtd, oob, i);

	return 0;
}

/**
 * nand_write_page - [REPLACEABLE] write one page
 * @mtd: MTD device structure
 * @chip: NAND chip descriptor
 * @offset: address offset within the page
 * @data_len: length of actual data to be written
 * @buf: the data to write
 * @oob_required: must write chip->oob_poi to OOB
 * @page: page number to write
 * @cached: cached programming
 * @raw: use _raw version of write_page
 */
static int nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
						   uint32_t offset, int data_len, const uint8_t * buf,
						   int oob_required, int page, int cached, int raw)
{
	int status, subpage;

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	if (!(chip->options & NAND_NO_SUBPAGE_WRITE) && chip->ecc.write_subpage)
		subpage = offset || (data_len < mtd->writesize);
	else
		subpage = 0;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	if (unlikely(raw))
		status = chip->ecc.write_page_raw(mtd, chip, buf, oob_required, page);
	else if (subpage)
		status = chip->ecc.write_subpage(mtd, chip, offset, data_len, buf, oob_required, page);
	else
		status = chip->ecc.write_page(mtd, chip, buf, oob_required, page);

	if (status < 0)
		return status;

	/*
	 * Cached progamming disabled for now. Not sure if it's worth the
	 * trouble. The speed gain is not very impressive. (2.3->2.6Mib/s).
	 */
	cached = 0;

	if (!cached || !NAND_HAS_CACHEPROG(chip)) {

		chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
		/*
		 * See if operation failed and additional status checks are
		 * available.
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_WRITING, status, page);

		if (status & NAND_STATUS_FAIL)
			return -EIO;
	} else {
		chip->cmdfunc(mtd, NAND_CMD_CACHEDPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
	}

	return 0;
}

/**
 * nand_fill_oob - [INTERN] Transfer client buffer to oob
 * @mtd: MTD device structure
 * @oob: oob data buffer
 * @len: oob data write length
 * @ops: oob ops structure
 */
static uint8_t *nand_fill_oob(struct mtd_info *mtd, uint8_t * oob, size_t len,
							  struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd->priv;

	/*
	 * Initialise to all 0xFF, to avoid the possibility of left over OOB
	 * data from a previous OOB read.
	 */
	memset(chip->oob_poi, 0xff, mtd->oobsize);

	switch (ops->mode) {

		case MTD_OPS_PLACE_OOB:
		case MTD_OPS_RAW:
			memcpy(chip->oob_poi + ops->ooboffs, oob, len);
			return oob + len;

		case MTD_OPS_AUTO_OOB:{
				struct nand_oobfree *free = chip->ecc.layout->oobfree;
				uint32_t boffs = 0, woffs = ops->ooboffs;
				size_t bytes = 0;

				for (; free->length && len; free++, len -= bytes) {
					/* Write request not from offset 0? */
					if (unlikely(woffs)) {
						if (woffs >= free->length) {
							woffs -= free->length;
							continue;
						}
						boffs = free->offset + woffs;
						bytes = min_t(size_t, len, (free->length - woffs));
						woffs = 0;
					} else {
						bytes = min_t(size_t, len, free->length);
						boffs = free->offset;
					}
					memcpy(chip->oob_poi + boffs, oob, bytes);
					oob += bytes;
				}
				return oob;
			}
		default:
			BUG();
	}
	return NULL;
}

#define NOTALIGNED(x)	((x & (chip->subpagesize - 1)) != 0)

/**
 * nand_do_write_ops - [INTERN] NAND write with ECC
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operations description structure
 *
 * NAND write with ECC.
 */
static int nand_do_write_ops(struct mtd_info *mtd, loff_t to,
							 struct mtd_oob_ops *ops)
{
	int chipnr, realpage, page, blockmask, column;
	struct nand_chip *chip = mtd->priv;
	uint32_t writelen = ops->len;

	uint32_t oobwritelen = ops->ooblen;
	uint32_t oobmaxlen = ops->mode == MTD_OPS_AUTO_OOB ?
		mtd->oobavail : mtd->oobsize;

	uint8_t *oob = ops->oobbuf;
	uint8_t *buf = ops->datbuf;
	int ret;
	int oob_required = oob ? 1 : 0;
    uint32_t pa_be4xlat __attribute((unused));
    uint32_t pa_in_out __attribute((unused));
	unsigned int chunk_id = (int)(to>>(chip->page_shift));
	//check address
	if (chkAddrValid(mtd, &chunk_id)) {
		return -EIO;
	}

	ops->retlen = 0;
	if (!writelen)
		return 0;

	/* Reject writes, which are not page aligned */
	if (NOTALIGNED(to) || NOTALIGNED(ops->len)) {
		pr_notice("%s: attempt to write non page aligned data\n", __func__);
		return -EINVAL;
	}

	column = to & (mtd->writesize - 1);

	chipnr = (int)(to >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);

	/* Check, if it is write protected */
	if (ca_nand_check_wp(mtd)) {
		ret = -EIO;
		goto err_out;
	}

	realpage = (int)(to >> chip->page_shift);
	page = realpage & chip->pagemask;
    pa_be4xlat = page;
	blockmask = (1 << (chip->phys_erase_shift - chip->page_shift)) - 1;

	/* Invalidate the page cache, when we write to the cached page */
	if (to <= ((loff_t) chip->pagebuf << chip->page_shift) &&
		((loff_t) chip->pagebuf << chip->page_shift) < (to + ops->len))
		chip->pagebuf = -1;

	/* Don't allow multipage oob writes with offset */
	if (oob && ops->ooboffs && (ops->ooboffs + ops->ooblen > oobmaxlen)) {
		ret = -EINVAL;
		goto err_out;
	}

	while (1) {
#ifdef CONFIG_NAND_HIDE_BAD
        pa_in_out = pa_be4xlat;
        ret = xlat_offset_to_skip_factory_bad(mtd, &pa_in_out);
        if(ret == -EINVAL){
            pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
            return ret;
        }
        if(pa_be4xlat != pa_in_out){
            pr_debug("Write xlat: 0x%x => 0x%x\n",pa_be4xlat,pa_in_out);
        }
        page = pa_in_out;
        pa_be4xlat++;
#endif

		int bytes = mtd->writesize;
		int cached = writelen > bytes && page != blockmask;
		uint8_t *wbuf = buf;
		int use_bufpoi;
		int part_pagewr = (column || writelen < (mtd->writesize - 1));

		if (part_pagewr)
			use_bufpoi = 1;
		else if (chip->options & NAND_USE_BOUNCE_BUFFER)
			use_bufpoi = !virt_addr_valid(buf);
		else
			use_bufpoi = 0;

		/* Partial page write?, or need to use bounce buffer */
		if (use_bufpoi) {
			pr_debug("%s: using write bounce buffer for buf@%p\n",
					 __func__, buf);
			cached = 0;
			if (part_pagewr)
				bytes = min_t(int, bytes - column, writelen);
			chip->pagebuf = -1;
			memset(chip->buffers->databuf, 0xff, mtd->writesize);
			memcpy(&chip->buffers->databuf[column], buf, bytes);
			wbuf = chip->buffers->databuf;
		}

		if (unlikely(oob)) {
			size_t len = min(oobwritelen, oobmaxlen);
			oob = nand_fill_oob(mtd, oob, len, ops);
			oobwritelen -= len;
		} else {
			/* We still need to erase leftover OOB data */
			memset(chip->oob_poi, 0xff, mtd->oobsize);
		}
		ret = chip->write_page(mtd, chip, column, bytes, wbuf,
							   oob_required, page, cached,
							   (ops->mode == MTD_OPS_RAW));
		if (ret)
			break;

		writelen -= bytes;
		if (!writelen)
			break;

		column = 0;
		buf += bytes;
		realpage++;

		page = realpage & chip->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}
	}

	ops->retlen = ops->len - writelen;
	if (unlikely(oob))
		ops->oobretlen = ops->ooblen;

  err_out:
	chip->select_chip(mtd, -1);
	return ret;
}

/**
 * panic_nand_write - [MTD Interface] NAND write with ECC
 * @mtd: MTD device structure
 * @to: offset to write to
 * @len: number of bytes to write
 * @retlen: pointer to variable to store the number of written bytes
 * @buf: the data to write
 *
 * NAND write with ECC. Used when performing writes in interrupt context, this
 * may for example be called by mtdoops when writing an oops while in panic.
 */
static int panic_nand_write(struct mtd_info *mtd, loff_t to, size_t len,
							size_t * retlen, const uint8_t * buf)
{
	struct nand_chip *chip = mtd->priv;
	struct mtd_oob_ops ops;
	int ret;
	unsigned int chunk_id = (int)(to>>(chip->page_shift)) ;
	//check address
	if (chkAddrValid(mtd, &chunk_id)) {
		return -EIO;
	}

	/* Wait for the device to get ready */
	panic_nand_wait(mtd, chip, 400);

	/* Grab the device */
	panic_nand_get_device(chip, mtd, FL_WRITING);

	ops.len = len;
	ops.datbuf = (uint8_t *) buf;
	ops.oobbuf = NULL;
	ops.mode = MTD_OPS_PLACE_OOB;

	ret = nand_do_write_ops(mtd, to, &ops);

	*retlen = ops.retlen;
	return ret;
}

/**
 * nand_write - [MTD Interface] NAND write with ECC
 * @mtd: MTD device structure
 * @to: offset to write to
 * @len: number of bytes to write
 * @retlen: pointer to variable to store the number of written bytes
 * @buf: the data to write
 *
 * NAND write with ECC.
 */
static int nand_write(struct mtd_info *mtd, loff_t to, size_t len,
					  size_t * retlen, const uint8_t * buf)
{
	struct mtd_oob_ops ops;
	int ret;

	struct nand_chip *chip = mtd->priv;
	unsigned int chunk_id = (int)(to>>(chip->page_shift));
	//check address
	if (chkAddrValid(mtd, &chunk_id)) {
		return -EIO;
	}
	nand_get_device(mtd, FL_WRITING);
	ops.len = len;
	ops.datbuf = (uint8_t *) buf;
	ops.oobbuf = NULL;
	ops.mode = MTD_OPS_PLACE_OOB;
	ret = nand_do_write_ops(mtd, to, &ops);
	*retlen = ops.retlen;
	nand_release_device(mtd);
	return ret;
}

/**
 * ca_nand_do_write_oob - [MTD Interface] NAND write out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 *
 * NAND write out-of-band.
 */
 int ca_nand_do_write_oob(struct mtd_info *mtd, loff_t to,
								struct mtd_oob_ops *ops)
{
	int chipnr, page, status, len;
	struct nand_chip *chip = mtd->priv;
    uint32_t pa_in_out __attribute((unused));
	unsigned int chunk_id = (int)(to>>(chip->page_shift));
	//check address
	if (chkAddrValid(mtd, &chunk_id)) {
		return -EIO;
	}


	pr_debug("%s: to = 0x%08x, len = %i\n",
			 __func__, (unsigned int)to, (int)ops->ooblen);

	if (ops->mode == MTD_OPS_AUTO_OOB)
		len = chip->ecc.layout->oobavail;
	else
		len = mtd->oobsize;

	/* Do not allow write past end of page */
	if ((ops->ooboffs + ops->ooblen) > len) {
		pr_debug("%s: attempt to write past end of page\n", __func__);
		return -EINVAL;
	}

	if (unlikely(ops->ooboffs >= len)) {
		pr_debug("%s: attempt to start write outside oob\n", __func__);
		return -EINVAL;
	}

	/* Do not allow write past end of device */
	if (unlikely(to >= mtd->size ||
				 ops->ooboffs + ops->ooblen >
				 ((mtd->size >> chip->page_shift) -
				  (to >> chip->page_shift)) * len)) {
		pr_debug("%s: attempt to write beyond end of device\n", __func__);
		return -EINVAL;
	}

	chipnr = (int)(to >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);

	/* Shift to get page */
	page = (int)(to >> chip->page_shift);
#ifdef CONFIG_NAND_HIDE_BAD
    pa_in_out = page;
    int ret = xlat_offset_to_skip_factory_bad(mtd, &pa_in_out);
    if(ret == -EINVAL){
        pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
        return ret;
    }
    if(page != pa_in_out){
        pr_debug("Read xlat: 0x%x => 0x%x\n",page,pa_in_out);
    }
    page = pa_in_out;
#endif

	/*
	 * Reset the chip. Some chips (like the Toshiba TC5832DC found in one
	 * of my DiskOnChip 2000 test units) will clear the whole data page too
	 * if we don't do this. I have no clue why, but I seem to have 'fixed'
	 * it in the doc2000 driver in August 1999.  dwmw2.
	 */
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Check, if it is write protected */
	if (ca_nand_check_wp(mtd)) {
		chip->select_chip(mtd, -1);
		return -EROFS;
	}

	/* Invalidate the page cache, if we write to the cached page */
	if (page == chip->pagebuf)
		chip->pagebuf = -1;

	nand_fill_oob(mtd, ops->oobbuf, ops->ooblen, ops);

	if (ops->mode == MTD_OPS_RAW)
		status = chip->ecc.write_oob_raw(mtd, chip, page & chip->pagemask);
	else
		status = chip->ecc.write_oob(mtd, chip, page & chip->pagemask);

	chip->select_chip(mtd, -1);

	if (status)
		return status;

	ops->oobretlen = ops->ooblen;

	return 0;
}

/**
 * nand_write_oob - [MTD Interface] NAND write data and/or out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 */
static int nand_write_oob(struct mtd_info *mtd, loff_t to,
						  struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;
	struct nand_chip *chip = mtd->priv;
	unsigned int chunk_id = (int)(to>>(chip->page_shift)) ;
	//check address
	if (chkAddrValid(mtd, &chunk_id)) {
		return -EIO;
	}

	ops->retlen = 0;

	/* Do not allow writes past end of device */
	if (ops->datbuf && (to + ops->len) > mtd->size) {
		pr_debug("%s: attempt to write beyond end of device\n", __func__);
		return -EINVAL;
	}

	nand_get_device(mtd, FL_WRITING);

	switch (ops->mode) {
		case MTD_OPS_PLACE_OOB:
		case MTD_OPS_AUTO_OOB:
		case MTD_OPS_RAW:
			break;

		default:
			goto out;
	}

	if (!ops->datbuf)
		ret = ca_nand_do_write_oob(mtd, to, ops);
	else
		ret = nand_do_write_ops(mtd, to, ops);

  out:
	nand_release_device(mtd);
	return ret;
}

/**
 * single_erase - [GENERIC] NAND standard block erase command function
 * @mtd: MTD device structure
 * @page: the page address of the block which will be erased
 *
 * Standard erase command for NAND chips. Returns NAND status.
 */
static int single_erase(struct mtd_info *mtd, int page)
{
	struct nand_chip *chip = mtd->priv;
	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	/* Send commands to erase a block */
	chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page);
	chip->cmdfunc(mtd, NAND_CMD_ERASE2, -1, -1);

	return chip->waitfunc(mtd, chip);
}

/**
 * nand_erase - [MTD Interface] erase block(s)
 * @mtd: MTD device structure
 * @instr: erase instruction
 *
 * Erase one ore more blocks.
 */
static int nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	return ca_nand_erase_nand(mtd, instr, 0);
}

/**
 * ca_nand_erase_nand - [INTERN] erase block(s)
 * @mtd: MTD device structure
 * @instr: erase instruction
 * @allowbbt: allow erasing the bbt area
 *
 * Erase one ore more blocks.
 */
 int ca_nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr,
							  int allowbbt)
{
	int page, status, pages_per_block, ret, chipnr;
	struct nand_chip *chip = mtd->priv;
	loff_t len;
    uint32_t pa_be4xlat __attribute((unused));
    uint32_t pa_in_out __attribute((unused));

	pr_debug("%s: start = 0x%012llx, len = %llu\n",
			 __func__, (unsigned long long)instr->addr,
			 (unsigned long long)instr->len);

	if (check_offs_len(mtd, instr->addr, instr->len))
		return -EINVAL;

	/* Grab the lock and see if the device is available */
	nand_get_device(mtd, FL_ERASING);

	/* Shift to get first page */
	page = (int)(instr->addr >> chip->page_shift);
    pa_be4xlat = page;
	chipnr = (int)(instr->addr >> chip->chip_shift);

	//check address
	if (chkAddrValid(mtd, &page)) {
		return -EIO;
	}

	/* Calculate pages in each block */
	pages_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);

	/* Select the NAND device */
	chip->select_chip(mtd, chipnr);

	/* Check, if it is write protected */
	if (ca_nand_check_wp(mtd)) {
		pr_debug("%s: device is write protected!\n", __func__);
		instr->state = MTD_ERASE_FAILED;
		goto erase_exit;
	}

	/* Loop through the pages */
	len = instr->len;

	instr->state = MTD_ERASING;

	while (len) {
#ifdef CONFIG_NAND_HIDE_BAD
        pa_in_out = pa_be4xlat;
        ret = xlat_offset_to_skip_factory_bad(mtd, &pa_in_out);
        if(ret == -EINVAL){
            pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
            return ret;
        }
        if(pa_be4xlat != pa_in_out){
            pr_debug("erase xlat: 0x%x => 0x%x\n",pa_be4xlat,pa_in_out);
        }
        page = pa_in_out;
        pa_be4xlat += 64;
#endif

		/* Check if we have a bad block, we do not erase bad blocks! */
		if (nand_block_checkbad(mtd, ((loff_t) page) <<
								chip->page_shift, 0, allowbbt)) {
			pr_warn("%s: attempt to erase a bad block at page 0x%08x\n",
					__func__, page);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}

		/*
		 * Invalidate the page cache, if we erase the block which
		 * contains the current cached page.
		 */
		if (page <= chip->pagebuf && chip->pagebuf < (page + pages_per_block))
			chip->pagebuf = -1;

		ca_nand_erase_block(mtd, page);

		status = chip->waitfunc(mtd, chip);

		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_ERASING, status, page);

		/* See if block erase succeeded */
		if (status & NAND_STATUS_FAIL) {
			pr_debug("%s: failed erase, page 0x%08x\n", __func__, page);
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr = ((loff_t) page << chip->page_shift);
			goto erase_exit;
		}

		/* Increment page address and decrement length */
		len -= (1ULL << chip->phys_erase_shift);
		page += pages_per_block;

		/* Check, if we cross a chip boundary */
		if (len && !(page & chip->pagemask)) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}
	}
	instr->state = MTD_ERASE_DONE;

  erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;

	/* Deselect and wake up anyone waiting on the device */
	chip->select_chip(mtd, -1);
	nand_release_device(mtd);

	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/* Return more or less happy */
	return ret;
}

/**
 * nand_sync - [MTD Interface] sync
 * @mtd: MTD device structure
 *
 * Sync is actually a wait for chip ready function.
 */
static void nand_sync(struct mtd_info *mtd)
{
	pr_debug("%s: called\n", __func__);

	/* Grab the lock and see if the device is available */
	nand_get_device(mtd, FL_SYNCING);
	/* Release it and go back */
	nand_release_device(mtd);
}

/**
 * nand_block_isbad - [MTD Interface] Check if block at offset is bad
 * @mtd: MTD device structure
 * @offs: offset relative to mtd start
 */
static int nand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	return nand_block_checkbad(mtd, offs, 1, 0);
}

/**
 * nand_block_markbad - [MTD Interface] Mark block at the given offset as bad
 * @mtd: MTD device structure
 * @ofs: offset relative to mtd start
 */
static int nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	ret = nand_block_isbad(mtd, ofs);
	if (ret) {
		/* If it was bad already, return success and do nothing */
		if (ret > 0)
			return 0;
		return ret;
	}

	return nand_block_markbad_lowlevel(mtd, ofs);
}

/**
 * nand_onfi_set_features- [REPLACEABLE] set features for ONFI nand
 * @mtd: MTD device structure
 * @chip: nand chip info structure
 * @addr: feature address.
 * @subfeature_param: the subfeature parameters, a four bytes array.
 */
static int nand_onfi_set_features(struct mtd_info *mtd, struct nand_chip *chip,
								  int addr, uint8_t * subfeature_param)
{
	int status;
	int i;

	if (!chip->onfi_version || !(le16_to_cpu(chip->onfi_params.opt_cmd)
								 & ONFI_OPT_CMD_SET_GET_FEATURES))
		return -EINVAL;

	chip->cmdfunc(mtd, NAND_CMD_SET_FEATURES, addr, -1);
	for (i = 0; i < ONFI_SUBFEATURE_PARAM_LEN; ++i)
		chip->write_byte(mtd, subfeature_param[i]);

	status = chip->waitfunc(mtd, chip);
	if (status & NAND_STATUS_FAIL)
		return -EIO;
	return 0;
}

/**
 * nand_onfi_get_features- [REPLACEABLE] get features for ONFI nand
 * @mtd: MTD device structure
 * @chip: nand chip info structure
 * @addr: feature address.
 * @subfeature_param: the subfeature parameters, a four bytes array.
 */
static int nand_onfi_get_features(struct mtd_info *mtd, struct nand_chip *chip,
								  int addr, uint8_t * subfeature_param)
{
	int i;

	if (!chip->onfi_version || !(le16_to_cpu(chip->onfi_params.opt_cmd)
								 & ONFI_OPT_CMD_SET_GET_FEATURES))
		return -EINVAL;

	/* clear the sub feature parameters */
	memset(subfeature_param, 0, ONFI_SUBFEATURE_PARAM_LEN);

	chip->cmdfunc(mtd, NAND_CMD_GET_FEATURES, addr, -1);
	for (i = 0; i < ONFI_SUBFEATURE_PARAM_LEN; ++i)
		*subfeature_param++ = chip->read_byte(mtd);
	return 0;
}

/**
 * nand_suspend - [MTD Interface] Suspend the NAND flash
 * @mtd: MTD device structure
 */
static int nand_suspend(struct mtd_info *mtd)
{
	return nand_get_device(mtd, FL_PM_SUSPENDED);
}

/**
 * nand_resume - [MTD Interface] Resume the NAND flash
 * @mtd: MTD device structure
 */
static void nand_resume(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	if (chip->state == FL_PM_SUSPENDED)
		nand_release_device(mtd);
	else
		pr_err("%s called for a chip which is not in suspended state\n",
			   __func__);
}

/* Set default functions */
static void nand_set_defaults(struct nand_chip *chip, int busw)
{
	/* check for proper chip_delay setup, set 20us if not */
	if (!chip->chip_delay)
		chip->chip_delay = 20;

	/* check, if a user supplied command function given */
	if (chip->cmdfunc == NULL)
		chip->cmdfunc = ca_nand_command;

	/* check, if a user supplied wait function given */
	if (chip->waitfunc == NULL)
		chip->waitfunc = ca_nand_wait;

	if (!chip->select_chip)
		chip->select_chip = ca_nand_select_chip;

	/* set for ONFI nand */
	if (!chip->onfi_set_features)
		chip->onfi_set_features = nand_onfi_set_features;
	if (!chip->onfi_get_features)
		chip->onfi_get_features = nand_onfi_get_features;

	/* If called twice, pointers that depend on busw may need to be reset */
	if (!chip->read_byte || chip->read_byte == ca_nand_read_byte)
		chip->read_byte = busw ? nand_read_byte16 : ca_nand_read_byte;
	if (!chip->read_word)
		chip->read_word = ca_nand_read_word;
	if (!chip->block_bad)
		chip->block_bad = ca_nand_block_bad;
	if (!chip->block_markbad)
		chip->block_markbad = ca_nand_default_block_markbad;
	if (!chip->write_buf || chip->write_buf == ca_nand_write_buf)
		chip->write_buf = busw ? nand_write_buf16 : ca_nand_write_buf;
	if (!chip->write_byte || chip->write_byte == nand_write_byte)
		chip->write_byte = busw ? nand_write_byte16 : nand_write_byte;
	if (!chip->read_buf || chip->read_buf == ca_nand_read_buf)
		chip->read_buf = busw ? nand_read_buf16 : ca_nand_read_buf;
	if (!chip->scan_bbt)
		chip->scan_bbt = nand_default_bbt;

	if (!chip->controller) {
		chip->controller = &chip->hwcontrol;
		spin_lock_init(&chip->controller->lock);
		init_waitqueue_head(&chip->controller->wq);
	}

}

/* Sanitize ONFI strings so we can safely print them */
static void sanitize_string(uint8_t * s, size_t len)
{
	ssize_t i;

	/* Null terminate */
	s[len - 1] = 0;

	/* Remove non printable chars */
	for (i = 0; i < len - 1; i++) {
		if (s[i] < ' ' || s[i] > 127)
			s[i] = '?';
	}

	/* Remove trailing spaces */
	strim(s);
}

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

/* Parse the Extended Parameter Page. */
static int nand_flash_detect_ext_param_page(struct mtd_info *mtd,
											struct nand_chip *chip,
											struct nand_onfi_params *p)
{
	struct onfi_ext_param_page *ep;
	struct onfi_ext_section *s;
	struct onfi_ext_ecc_info *ecc;
	uint8_t *cursor;
	int ret = -EINVAL;
	int len;
	int i;

	len = le16_to_cpu(p->ext_param_page_length) * 16;
	ep = kmalloc(len, GFP_KERNEL);
	if (!ep)
		return -ENOMEM;

	/* Send our own NAND_CMD_PARAM. */
	chip->cmdfunc(mtd, NAND_CMD_PARAM, 0, -1);

	/* Use the Change Read Column command to skip the ONFI param pages. */
	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, sizeof(*p) * p->num_of_param_pages, -1);

	/* Read out the Extended Parameter Page. */
	chip->read_buf(mtd, (uint8_t *) ep, len);
	if ((onfi_crc16(ONFI_CRC_BASE, ((uint8_t *) ep) + 2, len - 2)
		 != le16_to_cpu(ep->crc))) {
		pr_debug("fail in the CRC.\n");
		goto ext_out;
	}

	/*
	 * Check the signature.
	 * Do not strictly follow the ONFI spec, maybe changed in future.
	 */
	if (strncmp(ep->sig, "EPPS", 4)) {
		pr_debug("The signature is invalid.\n");
		goto ext_out;
	}

	/* find the ECC section. */
	cursor = (uint8_t *) (ep + 1);
	for (i = 0; i < ONFI_EXT_SECTION_MAX; i++) {
		s = ep->sections + i;
		if (s->type == ONFI_SECTION_TYPE_2)
			break;
		cursor += s->length * 16;
	}
	if (i == ONFI_EXT_SECTION_MAX) {
		pr_debug("We can not find the ECC section.\n");
		goto ext_out;
	}

	/* get the info we want. */
	ecc = (struct onfi_ext_ecc_info *)cursor;

	if (!ecc->codeword_size) {
		pr_debug("Invalid codeword size\n");
		goto ext_out;
	}

	chip->ecc_strength_ds = ecc->ecc_bits;
	chip->ecc_step_ds = 1 << ecc->codeword_size;
	ret = 0;

  ext_out:
	kfree(ep);
	return ret;
}

static int nand_setup_read_retry_micron(struct mtd_info *mtd, int retry_mode)
{
	struct nand_chip *chip = mtd->priv;
	uint8_t feature[ONFI_SUBFEATURE_PARAM_LEN] = { retry_mode };

	return chip->onfi_set_features(mtd, chip, ONFI_FEATURE_ADDR_READ_RETRY,
								   feature);
}

/*
 * Configure chip properties from Micron vendor-specific ONFI table
 */
static void nand_onfi_detect_micron(struct nand_chip *chip,
									struct nand_onfi_params *p)
{
	struct nand_onfi_vendor_micron *micron = (void *)p->vendor;

	if (le16_to_cpu(p->vendor_revision) < 1)
		return;

	chip->read_retries = micron->read_retry_options;
	chip->setup_read_retry = nand_setup_read_retry_micron;
}

/*
 * Check if the NAND chip is ONFI compliant, returns 1 if it is, 0 otherwise.
 */
 int ca_nand_flash_detect_onfi(struct mtd_info *mtd,
									 struct nand_chip *chip, int *busw)
{
	struct nand_onfi_params *p = &chip->onfi_params;
	int val;
	char onfi_sig[4];

	/* Try ONFI for unknown chip or LP */
	ca_nand_onfi_id(0, onfi_sig);
	if (onfi_sig[0] != 'O' || onfi_sig[1] != 'N' ||
		onfi_sig[2] != 'F' || onfi_sig[3] != 'I')
		return 0;

	ca_nand_get_onfi_param(0, p);
	if (onfi_crc16(ONFI_CRC_BASE, (uint8_t *) p, 254) != le16_to_cpu(p->crc)) {
		pr_err("Could not find valid ONFI parameter page; aborting\n");
		return 0;
	}

	/* Check version */
	val = le16_to_cpu(p->revision);
	if (val & (1 << 5))
		chip->onfi_version = 23;
	else if (val & (1 << 4))
		chip->onfi_version = 22;
	else if (val & (1 << 3))
		chip->onfi_version = 21;
	else if (val & (1 << 2))
		chip->onfi_version = 20;
	else if (val & (1 << 1))
		chip->onfi_version = 10;

	if (!chip->onfi_version) {
		pr_info("unsupported ONFI version: %d\n", val);
		return 0;
	}

	sanitize_string(p->manufacturer, sizeof(p->manufacturer));
	sanitize_string(p->model, sizeof(p->model));
	if (!mtd->name)
		mtd->name = p->model;

	mtd->writesize = le32_to_cpu(p->byte_per_page);

	/*
	 * pages_per_block and blocks_per_lun may not be a power-of-2 size
	 * (don't ask me who thought of this...). MTD assumes that these
	 * dimensions will be power-of-2, so just truncate the remaining area.
	 */
	mtd->erasesize = 1 << (fls(le32_to_cpu(p->pages_per_block)) - 1);
	mtd->erasesize *= mtd->writesize;

	mtd->oobsize = le16_to_cpu(p->spare_bytes_per_page);

	/* See erasesize comment */
	chip->chipsize = 1 << (fls(le32_to_cpu(p->blocks_per_lun)) - 1);
	chip->chipsize *= (uint64_t) mtd->erasesize * p->lun_count;
	chip->bits_per_cell = p->bits_per_cell;

	if (onfi_feature(chip) & ONFI_FEATURE_16_BIT_BUS)
		*busw = NAND_BUSWIDTH_16;
	else
		*busw = 0;

	if (p->ecc_bits != 0xff) {
		chip->ecc_strength_ds = p->ecc_bits * 2;
		chip->ecc_step_ds = 512 * 2;
	} else if (chip->onfi_version >= 21 &&
			   (onfi_feature(chip) & ONFI_FEATURE_EXT_PARAM_PAGE)) {

		/*
		 * The nand_flash_detect_ext_param_page() uses the
		 * Change Read Column command which maybe not supported
		 * by the chip->cmdfunc. So try to update the chip->cmdfunc
		 * now. We do not replace user supplied command function.
		 */
		if (mtd->writesize > 512 && chip->cmdfunc == ca_nand_command)
			chip->cmdfunc = nand_command_lp;

		/* The Extended Parameter Page is supported since ONFI 2.1. */
		if (nand_flash_detect_ext_param_page(mtd, chip, p))
			pr_warn("Failed to detect ONFI extended param page\n");
	} else {
		pr_warn("Could not retrieve ONFI ECC requirements\n");
	}

	if (p->jedec_id == NAND_MFR_MICRON)
		nand_onfi_detect_micron(chip, p);

	return 1;
}

/*
 * Check if the NAND chip is JEDEC compliant, returns 1 if it is, 0 otherwise.
 */
 int ca_nand_flash_detect_jedec(struct mtd_info *mtd,
									  struct nand_chip *chip, int *busw)
{
	struct nand_jedec_params *p = &chip->jedec_params;
	struct jedec_ecc_info *ecc;
	int val;
	char jedec_sig[5];

	/* Try JEDEC for unknown chip or LP */
	/* Try ONFI for unknown chip or LP */
	ca_nand_jedec_id(0, jedec_sig);
	if (jedec_sig[0] != 'J' || jedec_sig[1] != 'E' ||
		jedec_sig[2] != 'D' || jedec_sig[3] != 'E' || jedec_sig[4] != 'C')
		return 0;

	ca_nand_get_jedec_param(0, p);
	if (onfi_crc16(ONFI_CRC_BASE, (uint8_t *) p, 510) != le16_to_cpu(p->crc)) {
		pr_err("Could not find valid JEDEC parameter page; aborting\n");
		return 0;
	}

	/* Check version */
	val = le16_to_cpu(p->revision);
	if (val & (1 << 2))
		chip->jedec_version = 10;
	else if (val & (1 << 1))
		chip->jedec_version = 1;	/* vendor specific version */

	if (!chip->jedec_version) {
		pr_info("unsupported JEDEC version: %d\n", val);
		return 0;
	}

	sanitize_string(p->manufacturer, sizeof(p->manufacturer));
	sanitize_string(p->model, sizeof(p->model));
	if (!mtd->name)
		mtd->name = p->model;

	mtd->writesize = le32_to_cpu(p->byte_per_page);

	/* Please reference to the comment for nand_flash_detect_onfi. */
	mtd->erasesize = 1 << (fls(le32_to_cpu(p->pages_per_block)) - 1);
	mtd->erasesize *= mtd->writesize;

	mtd->oobsize = le16_to_cpu(p->spare_bytes_per_page);

	/* Please reference to the comment for nand_flash_detect_onfi. */
	chip->chipsize = 1 << (fls(le32_to_cpu(p->blocks_per_lun)) - 1);
	chip->chipsize *= (uint64_t) mtd->erasesize * p->lun_count;
	chip->bits_per_cell = p->bits_per_cell;

	if (jedec_feature(chip) & JEDEC_FEATURE_16_BIT_BUS)
		*busw = NAND_BUSWIDTH_16;
	else
		*busw = 0;

	/* ECC info */
	ecc = &p->ecc_info[0];

	if (ecc->codeword_size >= 9) {
		chip->ecc_strength_ds = ecc->ecc_bits;
		chip->ecc_step_ds = 1 << ecc->codeword_size;
	} else {
		pr_warn("Invalid codeword size\n");
	}

	return 1;
}

/*
 * nand_id_has_period - Check if an ID string has a given wraparound period
 * @id_data: the ID string
 * @arrlen: the length of the @id_data array
 * @period: the period of repitition
 *
 * Check if an ID string is repeated within a given sequence of bytes at
 * specific repetition interval period (e.g., {0x20,0x01,0x7F,0x20} has a
 * period of 3). This is a helper function for nand_id_len(). Returns non-zero
 * if the repetition has a period of @period; otherwise, returns zero.
 */
static int nand_id_has_period(u8 * id_data, int arrlen, int period)
{
	int i, j;
	for (i = 0; i < period; i++)
		for (j = i + period; j < arrlen; j += period)
			if (id_data[i] != id_data[j])
				return 0;
	return 1;
}

/*
 * nand_id_len - Get the length of an ID string returned by CMD_READID
 * @id_data: the ID string
 * @arrlen: the length of the @id_data array

 * Returns the length of the ID string, according to known wraparound/trailing
 * zero patterns. If no pattern exists, returns the length of the array.
 */
static int nand_id_len(u8 * id_data, int arrlen)
{
	int last_nonzero, period;

	/* Find last non-zero byte */
	for (last_nonzero = arrlen - 1; last_nonzero >= 0; last_nonzero--)
		if (id_data[last_nonzero])
			break;

	/* All zeros */
	if (last_nonzero < 0)
		return 0;

	/* Calculate wraparound period */
	for (period = 1; period < arrlen; period++)
		if (nand_id_has_period(id_data, arrlen, period))
			break;

	/* There's a repeated pattern */
	if (period < arrlen)
		return period;

	/* There are trailing zeros */
	if (last_nonzero < arrlen - 1)
		return last_nonzero + 1;

	/* No pattern detected */
	return arrlen;
}

/* Extract the bits of per cell from the 3rd byte of the extended ID */
static int nand_get_bits_per_cell(u8 cellinfo)
{
	int bits;

	bits = cellinfo & NAND_CI_CELLTYPE_MSK;
	bits >>= NAND_CI_CELLTYPE_SHIFT;
	return bits + 1;
}

/*
 * Many new NAND share similar device ID codes, which represent the size of the
 * chip. The rest of the parameters must be decoded according to generic or
 * manufacturer-specific "extended ID" decoding patterns.
 */
static void nand_decode_ext_id(struct mtd_info *mtd, struct nand_chip *chip,
							   u8 id_data[8], int *busw)
{
	int extid, id_len;
	/* The 3rd id byte holds MLC / multichip data */
	chip->bits_per_cell = nand_get_bits_per_cell(id_data[2]);
	/* The 4th id byte is the important one */
	extid = id_data[3];

	id_len = nand_id_len(id_data, 8);

	/*
	 * Field definitions are in the following datasheets:
	 * Old style (4,5 byte ID): Samsung K9GAG08U0M (p.32)
	 * New Samsung (6 byte ID): Samsung K9GAG08U0F (p.44)
	 * Hynix MLC   (6 byte ID): Hynix H27UBG8T2B (p.22)
	 *
	 * Check for ID length, non-zero 6th byte, cell type, and Hynix/Samsung
	 * ID to decide what to do.
	 */
	if (id_len == 6 && id_data[0] == NAND_MFR_SAMSUNG &&
		!nand_is_slc(chip) && id_data[5] != 0x00) {
		/* Calc pagesize */
		mtd->writesize = 2048 << (extid & 0x03);
		extid >>= 2;
		/* Calc oobsize */
		switch (((extid >> 2) & 0x04) | (extid & 0x03)) {
			case 1:
				mtd->oobsize = 128;
				break;
			case 2:
				mtd->oobsize = 218;
				break;
			case 3:
				mtd->oobsize = 400;
				break;
			case 4:
				mtd->oobsize = 436;
				break;
			case 5:
				mtd->oobsize = 512;
				break;
			case 6:
				mtd->oobsize = 640;
				break;
			case 7:
			default:			/* Other cases are "reserved" (unknown) */
				mtd->oobsize = 1024;
				break;
		}
		extid >>= 2;
		/* Calc blocksize */
		mtd->erasesize = (128 * 1024) <<
			(((extid >> 1) & 0x04) | (extid & 0x03));
		*busw = 0;
	} else if (id_len == 6 && id_data[0] == NAND_MFR_HYNIX &&
			   !nand_is_slc(chip)) {
		unsigned int tmp;

		/* Calc pagesize */
		mtd->writesize = 2048 << (extid & 0x03);
		extid >>= 2;
		/* Calc oobsize */
		switch (((extid >> 2) & 0x04) | (extid & 0x03)) {
			case 0:
				mtd->oobsize = 128;
				break;
			case 1:
				mtd->oobsize = 224;
				break;
			case 2:
				mtd->oobsize = 448;
				break;
			case 3:
				mtd->oobsize = 64;
				break;
			case 4:
				mtd->oobsize = 32;
				break;
			case 5:
				mtd->oobsize = 16;
				break;
			default:
				mtd->oobsize = 640;
				break;
		}
		extid >>= 2;
		/* Calc blocksize */
		tmp = ((extid >> 1) & 0x04) | (extid & 0x03);
		if (tmp < 0x03)
			mtd->erasesize = (128 * 1024) << tmp;
		else if (tmp == 0x03)
			mtd->erasesize = 768 * 1024;
		else
			mtd->erasesize = (64 * 1024) << tmp;
		*busw = 0;
	} else {
		/* Calc pagesize */
		mtd->writesize = 1024 << (extid & 0x03);
		extid >>= 2;
		/* Calc oobsize */
		mtd->oobsize = (8 << (extid & 0x01)) * (mtd->writesize >> 9);
		extid >>= 2;
		/* Calc blocksize. Blocksize is multiples of 64KiB */
		mtd->erasesize = (64 * 1024) << (extid & 0x03);
		extid >>= 2;
		/* Get buswidth information */
		*busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;

		/*
		 * Toshiba 24nm raw SLC (i.e., not BENAND) have 32B OOB per
		 * 512B page. For Toshiba SLC, we decode the 5th/6th byte as
		 * follows:
		 * - ID byte 6, bits[2:0]: 100b -> 43nm, 101b -> 32nm,
		 *                         110b -> 24nm
		 * - ID byte 5, bit[7]:    1 -> BENAND, 0 -> raw SLC
		 */
		if (id_len >= 6 && id_data[0] == NAND_MFR_TOSHIBA &&
			nand_is_slc(chip) && (id_data[5] & 0x7) == 0x6 /* 24nm */  &&
			!(id_data[4] & 0x80) /* !BENAND */ ) {
			mtd->oobsize = 32 * mtd->writesize >> 9;
		}

	}
}

/*
 * Old devices have chip data hardcoded in the device ID table. nand_decode_id
 * decodes a matching ID table entry and assigns the MTD size parameters for
 * the chip.
 */
static void nand_decode_id(struct mtd_info *mtd, struct nand_chip *chip,
						   struct nand_flash_dev *type, u8 id_data[8],
						   int *busw)
{
	int maf_id = id_data[0];

	mtd->erasesize = type->erasesize;
	mtd->writesize = type->pagesize;
	mtd->oobsize = mtd->writesize / 32;
	*busw = type->options & NAND_BUSWIDTH_16;

	/* All legacy ID NAND are small-page, SLC */
	chip->bits_per_cell = 1;

	/*
	 * Check for Spansion/AMD ID + repeating 5th, 6th byte since
	 * some Spansion chips have erasesize that conflicts with size
	 * listed in nand_ids table.
	 * Data sheet (5 byte ID): Spansion S30ML-P ORNAND (p.39)
	 */
	if (maf_id == NAND_MFR_AMD && id_data[4] != 0x00 && id_data[5] == 0x00
		&& id_data[6] == 0x00 && id_data[7] == 0x00 && mtd->writesize == 512) {
		mtd->erasesize = 128 * 1024;
		mtd->erasesize <<= ((id_data[3] & 0x03) << 1);
	}
}

/*
 * Set the bad block marker/indicator (BBM/BBI) patterns according to some
 * heuristic patterns using various detected parameters (e.g., manufacturer,
 * page size, cell-type information).
 */
static void nand_decode_bbm_options(struct mtd_info *mtd,
									struct nand_chip *chip, u8 id_data[8])
{
	int maf_id = id_data[0];

	/* Set the bad block position */
	if (mtd->writesize > 512 || (chip->options & NAND_BUSWIDTH_16))
		chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
	else
		chip->badblockpos = NAND_SMALL_BADBLOCK_POS;

	/*
	 * Bad block marker is stored in the last page of each block on Samsung
	 * and Hynix MLC devices; stored in first two pages of each block on
	 * Micron devices with 2KiB pages and on SLC Samsung, Hynix, Toshiba,
	 * AMD/Spansion, and Macronix.  All others scan only the first page.
	 */
	/*
	 * We have some legacy Samsung Nand chip. Bad block marker are still
	 * located at first page.
	 */
//	if (!nand_is_slc(chip) &&
//		(maf_id == NAND_MFR_SAMSUNG || maf_id == NAND_MFR_HYNIX))
//		chip->bbt_options |= NAND_BBT_SCANLASTPAGE;
//	else if ((nand_is_slc(chip) &&
	if ((nand_is_slc(chip) &&
			  (maf_id == NAND_MFR_SAMSUNG ||
			   maf_id == NAND_MFR_HYNIX ||
			   maf_id == NAND_MFR_TOSHIBA ||
			   maf_id == NAND_MFR_AMD ||
			   maf_id == NAND_MFR_MACRONIX)) ||
			 (mtd->writesize == 2048 && maf_id == NAND_MFR_MICRON))
		chip->bbt_options |= NAND_BBT_SCAN2NDPAGE;
}

static inline bool is_full_id_nand(struct nand_flash_dev *type)
{
	return type->id_len;
}

static bool find_full_id_nand(struct mtd_info *mtd, struct nand_chip *chip,
							  struct nand_flash_dev *type, u8 * id_data,
							  int *busw)
{
	if (!strncmp(type->id, id_data, type->id_len)) {
		mtd->writesize = type->pagesize;
		mtd->erasesize = type->erasesize;
		mtd->oobsize = type->oobsize;

		chip->bits_per_cell = nand_get_bits_per_cell(id_data[2]);
		chip->chipsize = (uint64_t) type->chipsize << 20;
		chip->options |= type->options;
		chip->ecc_strength_ds = NAND_ECC_STRENGTH(type);
		chip->ecc_step_ds = NAND_ECC_STEP(type);
		chip->onfi_timing_mode_default = type->onfi_timing_mode_default;

		*busw = type->options & NAND_BUSWIDTH_16;

		if (!mtd->name)
			mtd->name = type->name;

		return true;
	}
	return false;
}

/*
 * Get the flash and manufacturer id and lookup if the type is supported.
 */
static struct nand_flash_dev *nand_get_flash_type(struct mtd_info *mtd,
												  struct nand_chip *chip,
												  int *maf_id, int *dev_id,
												  struct nand_flash_dev *type)
{
	int busw;
	int maf_idx;
	u8 id_data[8];

	/* Select the device */
	chip->select_chip(mtd, 0);

	/*
	 * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
	 * after power-up.
	 */
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Send the command for reading device ID */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	memset(id_data, 0, sizeof(id_data));
	ca_nand_read_id(0, &id_data[0]);
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
	*maf_id = id_data[0];
	*dev_id = id_data[1];
#if 0
	/*
	 * Try again to make sure, as some systems the bus-hold or other
	 * interface concerns can cause random data which looks like a
	 * possibly credible NAND flash to appear. If the two results do
	 * not match, ignore the device completely.
	 */

	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read entire ID string */
	for (i = 0; i < 8; i++)
		id_data[i] = chip->read_byte(mtd);

	if (id_data[0] != *maf_id || id_data[1] != *dev_id) {
		pr_info("second ID read did not match %02x,%02x against %02x,%02x\n",
				*maf_id, *dev_id, id_data[0], id_data[1]);
		return ERR_PTR(-ENODEV);
	}
#endif
	if (!type)
		type = nand_flash_ids;

	for (; type->name != NULL; type++) {
		if (is_full_id_nand(type)) {
			if (find_full_id_nand(mtd, chip, type, id_data, &busw))
				goto ident_done;
		} else if (*dev_id == type->dev_id) {
			break;
		}
	}

	chip->onfi_version = 0;
	if (!type->name || !type->pagesize) {
		/* Check if the chip is ONFI compliant */
		if (ca_nand_flash_detect_onfi(mtd, chip, &busw))
			goto ident_done;

		/* Check if the chip is JEDEC compliant */
		if (ca_nand_flash_detect_jedec(mtd, chip, &busw))
			goto ident_done;
	}

	if (!type->name)
		return ERR_PTR(-ENODEV);

	if (!mtd->name)
		mtd->name = type->name;

	chip->chipsize = (uint64_t) type->chipsize << 20;

	if (!type->pagesize) {
		/* Decode parameters from extended ID */
		nand_decode_ext_id(mtd, chip, id_data, &busw);
	} else {
		nand_decode_id(mtd, chip, type, id_data, &busw);
	}
	/* Get chip options */
	chip->options |= type->options;

	/*
	 * Check if chip is not a Samsung device. Do not clear the
	 * options for chips which do not have an extended id.
	 */
	if (*maf_id != NAND_MFR_SAMSUNG && !type->pagesize)
		chip->options &= ~NAND_SAMSUNG_LP_OPTIONS;
  ident_done:
	mtd->oobsize &= ~0x7;
	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == *maf_id)
			break;
	}

	if (chip->options & NAND_BUSWIDTH_AUTO) {
		WARN_ON(chip->options & NAND_BUSWIDTH_16);
		chip->options |= busw;
		nand_set_defaults(chip, busw);
	} else if (busw != (chip->options & NAND_BUSWIDTH_16)) {
		/*
		 * Check, if buswidth is correct. Hardware drivers should set
		 * chip correct!
		 */
		pr_info("device found, Manufacturer ID: 0x%02x, Chip ID: 0x%02x\n",
				*maf_id, *dev_id);
		pr_info("%s %s\n", nand_manuf_ids[maf_idx].name, mtd->name);
		pr_warn("bus width %d instead %d bit\n",
				(chip->options & NAND_BUSWIDTH_16) ? 16 : 8, busw ? 16 : 8);
		return ERR_PTR(-EINVAL);
	}

	nand_decode_bbm_options(mtd, chip, id_data);

	/* Calculate the address shift from the page size */
	chip->page_shift = ffs(mtd->writesize) - 1;
	/* Convert chipsize to number of pages per chip -1 */
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;

	chip->bbt_erase_shift = chip->phys_erase_shift = ffs(mtd->erasesize) - 1;
	if (chip->chipsize & 0xffffffff)
		chip->chip_shift = ffs((unsigned)chip->chipsize) - 1;
	else {
		chip->chip_shift = ffs((unsigned)(chip->chipsize >> 32));
		chip->chip_shift += 32 - 1;
	}

	chip->badblockbits = 8;
	chip->erase = single_erase;

	/* Do not replace user supplied command function! */
	if (mtd->writesize > 512 && chip->cmdfunc == ca_nand_command)
		chip->cmdfunc = nand_command_lp;

	pr_info("device found, Manufacturer ID: 0x%02x, Chip ID: 0x%02x\n",
			*maf_id, *dev_id);

	if (chip->onfi_version)
		pr_info("%s %s\n", nand_manuf_ids[maf_idx].name,
				chip->onfi_params.model);
	else if (chip->jedec_version)
		pr_info("%s %s\n", nand_manuf_ids[maf_idx].name,
				chip->jedec_params.model);
	else
		pr_info("%s %s\n", nand_manuf_ids[maf_idx].name, type->name);

	pr_info("%dMiB, %s, page size: %d, OOB size: %d\n",
			(int)(chip->chipsize >> 20), nand_is_slc(chip) ? "SLC" : "MLC",
			mtd->writesize, mtd->oobsize);
	return type;
}

/**
 * ca_nand_scan_ident - [NAND Interface] Scan for the NAND device
 * @mtd: MTD device structure
 * @maxchips: number of chips to scan for
 * @table: alternative NAND ID table
 *
 * This is the first phase of the normal nand_scan() function. It reads the
 * flash ID and sets up MTD fields accordingly.
 *
 * The mtd->owner field must be set to the module of the caller.
 */
int ca_nand_scan_ident(struct mtd_info *mtd, int maxchips,
					   struct nand_flash_dev *table)
{
	int i, nand_maf_id, nand_dev_id;
	struct nand_chip *chip = mtd->priv;
	struct nand_flash_dev *type;

	/* Set the default functions */
	nand_set_defaults(chip, chip->options & NAND_BUSWIDTH_16);

	/* Read the flash type */
	type = nand_get_flash_type(mtd, chip, &nand_maf_id, &nand_dev_id, table);

	if (IS_ERR(type)) {
		if (!(chip->options & NAND_SCAN_SILENT_NODEV))
			pr_warn("No NAND device found\n");
		chip->select_chip(mtd, -1);
		return PTR_ERR(type);
	}

	chip->select_chip(mtd, -1);

	/* Check for a chip array */
	for (i = 1; i < maxchips; i++) {
		chip->select_chip(mtd, i);
		/* See comment in nand_get_flash_type for reset */
		chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
		/* Send the command for reading device ID */
		chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
		/* Read manufacturer and device IDs */
		if (nand_maf_id != chip->read_byte(mtd) ||
			nand_dev_id != chip->read_byte(mtd)) {
			chip->select_chip(mtd, -1);
			break;
		}
		chip->select_chip(mtd, -1);
	}
	if (i > 1)
		pr_info("%d chips detected\n", i);

	/* Store the number of chips and calc total size for mtd */
	chip->numchips = i;
	mtd->size = i * chip->chipsize;

	/* Reconfigure flash ype */
	switch(ca77xx_host->mtd->writesize){
	case SZ_512:
		flash_type.bf.flashType = 4;
		break;
	case SZ_2K:
		flash_type.bf.flashType = 5;
		break;
	case SZ_4K:
		flash_type.bf.flashType = 6;
		break;
	case SZ_8K:
		flash_type.bf.flashType = 7;
		break;
	default:
		printk("Error: Nand page size not supported! %d\n", \
				ca77xx_host->mtd->writesize);
		break;
	};
	flash_type.bf.flashSize = 0;	/* configurable oob size */
	if(ca77xx_host->mtd->size > SZ_128M)
		flash_type.bf.flashPin = 1;  	/* Flsah size > 128MB */

	write_flash_ctrl_reg(FLASH_TYPE, flash_type.wrd);

	tmp_buf = kmalloc(mtd->writesize, GFP_KERNEL);
	if (IS_ERR(tmp_buf)) {
		printk(KERN_ERR "Error: no memory allocated\n");
		return -ENOMEM;
	}

	return 0;
}

EXPORT_SYMBOL(ca_nand_scan_ident);

/*
 * Check if the chip configuration meet the datasheet requirements.

 * If our configuration corrects A bits per B bytes and the minimum
 * required correction level is X bits per Y bytes, then we must ensure
 * both of the following are true:
 *
 * (1) A / B >= X / Y
 * (2) A >= X
 *
 * Requirement (1) ensures we can correct for the required bitflip density.
 * Requirement (2) ensures we can correct even when all bitflips are clumped
 * in the same sector.
 */
static bool nand_ecc_strength_good(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int corr, ds_corr;

	if (ecc->size == 0 || chip->ecc_step_ds == 0)
		/* Not enough information */
		return true;

	/*
	 * We get the number of corrected bits per page to compare
	 * the correction density.
	 */
	corr = (mtd->writesize * ecc->strength) / ecc->size;
	ds_corr = (mtd->writesize * chip->ecc_strength_ds) / chip->ecc_step_ds;

	return corr >= ds_corr && ecc->strength >= chip->ecc_strength_ds;
}

/**
 * ca_nand_scan_tail - [NAND Interface] Scan for the NAND device
 * @mtd: MTD device structure
 *
 * This is the second phase of the normal nand_scan() function. It fills out
 * all the uninitialized function pointers with the defaults and scans for a
 * bad block table if appropriate.
 */
int ca_nand_scan_tail(struct mtd_info *mtd)
{
	int i, eccStartOffset;
	struct nand_chip *chip = mtd->priv;
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	struct nand_buffers *nbuf;

	/* New bad blocks should be marked in OOB, flash-based BBT, or both */
	BUG_ON((chip->bbt_options & NAND_BBT_NO_OOB_BBM) &&
		   !(chip->bbt_options & NAND_BBT_USE_FLASH));

	if (!(chip->options & NAND_OWN_BUFFERS)) {
		nbuf = kzalloc(sizeof(*nbuf) + mtd->writesize
					   + mtd->oobsize * 3, GFP_KERNEL);
		if (!nbuf)
			return -ENOMEM;
		nbuf->ecccalc = (uint8_t *) (nbuf + 1);
		nbuf->ecccode = nbuf->ecccalc + mtd->oobsize;
		nbuf->databuf = nbuf->ecccode + mtd->oobsize;

		chip->buffers = nbuf;
	} else {
		if (!chip->buffers)
			return -ENOMEM;
	}

	/* Set the internal oob buffer location, just after the page data */
	chip->oob_poi = chip->buffers->databuf + mtd->writesize;

	/*
	 * If no default placement scheme is given, select an appropriate one.
	 */
	if (!ecc->layout && (ecc->mode != NAND_ECC_SOFT_BCH)) {
		if (mtd->oobsize == 16) {
			chip->ecc.layout = &ca_nand_bch_oob_16;
		} else {
			memset(&ca_nand_ecclayout, 0, sizeof(ca_nand_ecclayout));
			ca_nand_ecclayout.eccbytes =
				mtd->writesize / chip->ecc.size * chip->ecc.bytes;
			if (sizeof(ca_nand_ecclayout.eccpos) <
				4 * ca_nand_ecclayout.eccbytes) {
				printk(KERN_WARNING
					   "eccpos memory is less than needed eccbytes");
				return 1;
			}

			if (ca_nand_ecclayout.eccbytes > mtd->oobsize) {
				printk(KERN_WARNING
					   "oob size too small to cover eccbytes %d %d\n",
					   mtd->oobsize, ca_nand_ecclayout.eccbytes);
				return 1;
			}

			memset(ca_nand_ecclayout.eccpos, 0,
				   sizeof(ca_nand_ecclayout.eccpos));
			eccStartOffset = mtd->oobsize - ca_nand_ecclayout.eccbytes;
			for (i = 0; i < ca_nand_ecclayout.eccbytes; ++i) {
				if ((i + eccStartOffset) == chip->badblockpos) {
					continue;
				}
				ca_nand_ecclayout.eccpos[i] = i + eccStartOffset;
			}

			ca_nand_ecclayout.oobfree[0].offset = 2;
			ca_nand_ecclayout.oobfree[0].length =
				mtd->oobsize - ca_nand_ecclayout.eccbytes -
				ca_nand_ecclayout.oobfree[0].offset;

			if(ca77xx_host->ecc_algorithm >= ECC_BCH_8){
				/*  BCH algorithm needs one extra byte to tag erase status */
				if (ca_nand_ecclayout.oobfree[0].length == 0) {
					printk(KERN_WARNING "eccbytes is less than required");
					return 1;
				};
				ca_nand_ecclayout.oobfree[0].length -= 1;
	
				chip->ecc.layout = &ca_nand_ecclayout;
	
				}
			}
	}

	if (!chip->write_page)
		chip->write_page = nand_write_page;

	/*
	 * Check ECC mode, default to software if 3byte/512byte hardware ECC is
	 * selected and we have 256 byte pagesize fallback to software ECC
	 */

	switch (ecc->mode) {
		case NAND_ECC_HW_OOB_FIRST:
			/* Similar to NAND_ECC_HW, but a separate read_page handle */
			if (!ecc->calculate || !ecc->correct || !ecc->hwctl) {
				pr_warn
					("No ECC functions supplied; hardware ECC not possible\n");
				BUG();
			}
			if (!ecc->read_page)
				ecc->read_page = nand_read_page_hwecc_oob_first;

		case NAND_ECC_HW:
			/* Use standard hwecc read page function? */
			if (!ecc->read_page)
				ecc->read_page = ca_nand_read_page_hwecc;
			if (!ecc->write_page)
				ecc->write_page = ca_nand_write_page_hwecc;
			if (!ecc->read_page_raw)
				ecc->read_page_raw = ca_nand_read_page_raw;
			if (!ecc->write_page_raw)
				ecc->write_page_raw = ca_nand_write_page_raw;
			if (!ecc->read_oob)
				ecc->read_oob = ca_nand_read_oob_std;
			if (!ecc->write_oob)
				ecc->write_oob = ca_nand_write_oob_std;
			if (!ecc->read_subpage)
				ecc->read_subpage = ca_nand_read_subpage;
			if (!ecc->write_subpage)
				ecc->write_subpage = nand_write_subpage_hwecc;

		case NAND_ECC_HW_SYNDROME:
			if ((!ecc->calculate || !ecc->correct || !ecc->hwctl) &&
				(!ecc->read_page ||
				 ecc->read_page == nand_read_page_hwecc ||
				 !ecc->write_page ||
				 ecc->write_page == nand_write_page_hwecc)) {
				pr_warn
					("No ECC functions supplied; hardware ECC not possible\n");
				BUG();
			}
			/* Use standard syndrome read/write page function? */
			if (!ecc->read_page)
				ecc->read_page = nand_read_page_syndrome;
			if (!ecc->write_page)
				ecc->write_page = nand_write_page_syndrome;
			if (!ecc->read_page_raw)
				ecc->read_page_raw = nand_read_page_raw_syndrome;
			if (!ecc->write_page_raw)
				ecc->write_page_raw = nand_write_page_raw_syndrome;
			if (!ecc->read_oob)
				ecc->read_oob = nand_read_oob_syndrome;
			if (!ecc->write_oob)
				ecc->write_oob = nand_write_oob_syndrome;

			if (mtd->writesize >= ecc->size) {
				if (!ecc->strength) {
					pr_warn
						("Driver must set ecc.strength when using hardware ECC\n");
					BUG();
				}
				break;
			}
			pr_warn
				("%d byte HW ECC not possible on %d byte page size, fallback to SW ECC\n",
				 ecc->size, mtd->writesize);
			ecc->mode = NAND_ECC_SOFT;

		case NAND_ECC_SOFT:
			ecc->calculate = nand_calculate_ecc;
			ecc->correct = nand_correct_data;
			ecc->read_page = nand_read_page_swecc;
			ecc->read_subpage = nand_read_subpage;
			ecc->write_page = nand_write_page_swecc;
			ecc->read_page_raw = nand_read_page_raw;
			ecc->write_page_raw = nand_write_page_raw;
			ecc->read_oob = nand_read_oob_std;
			ecc->write_oob = nand_write_oob_std;
			if (!ecc->size)
				ecc->size = 256;
			ecc->bytes = 3;
			ecc->strength = 1;
			break;

		case NAND_ECC_SOFT_BCH:
			if (!mtd_nand_has_bch()) {
				pr_warn("CONFIG_MTD_NAND_ECC_BCH not enabled\n");
				BUG();
			}
			ecc->calculate = nand_bch_calculate_ecc;
			ecc->correct = nand_bch_correct_data;
			ecc->read_page = nand_read_page_swecc;
			ecc->read_subpage = nand_read_subpage;
			ecc->write_page = nand_write_page_swecc;
			ecc->read_page_raw = nand_read_page_raw;
			ecc->write_page_raw = nand_write_page_raw;
			ecc->read_oob = nand_read_oob_std;
			ecc->write_oob = nand_write_oob_std;
			/*
			 * Board driver should supply ecc.size and ecc.bytes values to
			 * select how many bits are correctable; see nand_bch_init()
			 * for details. Otherwise, default to 4 bits for large page
			 * devices.
			 */
			if (!ecc->size && (mtd->oobsize >= 64)) {
				ecc->size = 512;
				ecc->bytes = 7;
			}
			ecc->priv = nand_bch_init(mtd, ecc->size, ecc->bytes, &ecc->layout);
			if (!ecc->priv) {
				pr_warn("BCH ECC initialization failed!\n");
				BUG();
			}
			ecc->strength = ecc->bytes * 8 / fls(8 * ecc->size);
			break;

		case NAND_ECC_NONE:
			pr_warn
				("NAND_ECC_NONE selected by board driver. This is not recommended!\n");
			ecc->read_page = nand_read_page_raw;
			ecc->write_page = nand_write_page_raw;
			ecc->read_oob = nand_read_oob_std;
			ecc->read_page_raw = nand_read_page_raw;
			ecc->write_page_raw = nand_write_page_raw;
			ecc->write_oob = nand_write_oob_std;
			ecc->size = mtd->writesize;
			ecc->bytes = 0;
			ecc->strength = 0;
			break;

		default:
			pr_warn("Invalid NAND_ECC_MODE %d\n", ecc->mode);
			BUG();
	}

	/* For many systems, the standard OOB write also works for raw */
	if (!ecc->read_oob_raw)
		ecc->read_oob_raw = ecc->read_oob;
	if (!ecc->write_oob_raw)
		ecc->write_oob_raw = ecc->write_oob;

	/*
	 * The number of bytes available for a client to place data into
	 * the out of band area.
	 */
	ecc->layout->oobavail = 0;
	for (i = 0; ecc->layout->oobfree[i].length
		 && i < ARRAY_SIZE(ecc->layout->oobfree); i++)
		ecc->layout->oobavail += ecc->layout->oobfree[i].length;
	mtd->oobavail = ecc->layout->oobavail;

	/* ECC sanity check: warn if it's too weak */
	if (!nand_ecc_strength_good(mtd))
		pr_warn
			("WARNING: %s: the ECC used on your system is too weak compared to the one required by the NAND chip\n",
			 mtd->name);

	/*
	 * Set the number of read / write steps for one page depending on ECC
	 * mode.
	 */
	ecc->steps = mtd->writesize / ecc->size;
	if (ecc->steps * ecc->size != mtd->writesize) {
		pr_warn("Invalid ECC parameters\n");
		BUG();
	}
	ecc->total = ecc->steps * ecc->bytes;

	/* Allow subpage writes up to ecc.steps. Not possible for MLC flash */
	if (!(chip->options & NAND_NO_SUBPAGE_WRITE) && nand_is_slc(chip)) {
		switch (ecc->steps) {
			case 2:
				mtd->subpage_sft = 1;
				break;
			case 4:
			case 8:
			case 16:
				mtd->subpage_sft = 2;
				break;
		}
	}
	chip->subpagesize = mtd->writesize >> mtd->subpage_sft;

	/* Initialize state */
	chip->state = FL_READY;

	/* Invalidate the pagebuffer reference */
	chip->pagebuf = -1;

	/* Large page NAND with SOFT_ECC should support subpage reads */
	switch (ecc->mode) {
		case NAND_ECC_SOFT:
		case NAND_ECC_SOFT_BCH:
			if (chip->page_shift > 9)
				chip->options |= NAND_SUBPAGE_READ;
			break;

		default:
			break;
	}

	/* Fill in remaining MTD driver data */
	mtd->type = nand_is_slc(chip) ? MTD_NANDFLASH : MTD_MLCNANDFLASH;
	mtd->flags = (chip->options & NAND_ROM) ? MTD_CAP_ROM : MTD_CAP_NANDFLASH;
	mtd->_erase = nand_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read = nand_read;
	mtd->_write = nand_write;
	mtd->_panic_write = panic_nand_write;
	mtd->_read_oob = nand_read_oob;
	mtd->_write_oob = nand_write_oob;
	mtd->_sync = nand_sync;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_suspend = nand_suspend;
	mtd->_resume = nand_resume;
	mtd->_block_isreserved = nand_block_isreserved;
	mtd->_block_isbad = nand_block_isbad;
	mtd->_block_markbad = nand_block_markbad;
	mtd->writebufsize = mtd->writesize;

	/* propagate ecc info to mtd_info */
	mtd->ecclayout = ecc->layout;
	mtd->ecc_strength = ecc->strength;
	mtd->ecc_step_size = ecc->size;
	/*
	 * Initialize bitflip_threshold to its default prior scan_bbt() call.
	 * scan_bbt() might invoke mtd_read(), thus bitflip_threshold must be
	 * properly set.
	 */
	if (!mtd->bitflip_threshold)
		mtd->bitflip_threshold = mtd->ecc_strength;

	/* Check, if we should skip the bad block table scan */
	if (chip->options & NAND_SKIP_BBTSCAN)
		return 0;

#ifdef CONFIG_NAND_HIDE_BAD
    get_nand_factory_bad_blocks(mtd, 0, mtd->size);

    /* Update the the size information to the availabe capacity */
    mtd->size -= bad_block_cnt*mtd->erasesize;
    printk("p-nand total capacity is %dMB(0x%x)\n",(uint32_t)(chip->chipsize/0x100000),(uint32_t)chip->chipsize);
    printk("p-nand available size is %dMB(0x%x)\n",(uint32_t)(mtd->size/0x100000),(uint32_t)mtd->size);

    if(bbl_source == FACTORY_1ST_SCAN)
        printk("p-nand 1st scan factory bad to memory\n");
    else
        printk("p-nand %s from flash\n",(bbl_source==FACTORY_FROM_BBT0)?"Bbl0":"Bbl1");

    printk("p-nand hidden: ");
    for(i=0 ; i<bad_block_cnt ; i++){
        printk("0x%x ", bad_block_list[i]);
    }
    printk("\n");
#endif /* CONFIG_NAND_HIDE_BAD */


	/* Build bad block table */
	return chip->scan_bbt(mtd);
}

EXPORT_SYMBOL(ca_nand_scan_tail);

/*
 * is_module_text_address() isn't exported, and it's mostly a pointless
 * test if this is a module _anyway_ -- they'd have to try _really_ hard
 * to call us from in-kernel code if the core NAND support is modular.
 */
#ifdef MODULE
#define caller_is_module() (1)
#else
#define caller_is_module() \
	is_module_text_address((unsigned long)__builtin_return_address(0))
#endif

/**
 * ca_nand_scan - [NAND Interface] Scan for the NAND device
 * @mtd: MTD device structure
 * @maxchips: number of chips to scan for
 *
 * This fills out all the uninitialized function pointers with the defaults.
 * The flash ID is read and the mtd/chip structures are filled with the
 * appropriate values. The mtd->owner field must be set to the module of the
 * caller.
 */
int ca_nand_scan(struct mtd_info *mtd, int maxchips)
{
	int ret;

	/* Many callers got this wrong, so check for it for a while... */
	if (!mtd->owner && caller_is_module()) {
		pr_crit("%s called with NULL mtd->owner!\n", __func__);
		BUG();
	}

	ret = ca_nand_scan_ident(mtd, maxchips, NULL);
	if (!ret)
		ret = ca_nand_scan_tail(mtd);
	return ret;
}

EXPORT_SYMBOL(ca_nand_scan);

/**
 * nand_release - [NAND Interface] Free resources held by the NAND device
 * @mtd: MTD device structure
 */
void ca_nand_release(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	if (chip->ecc.mode == NAND_ECC_SOFT_BCH)
		nand_bch_free((struct nand_bch_control *)chip->ecc.priv);

	mtd_device_unregister(mtd);

	/* Free bad block table memory */
	kfree(chip->bbt);
	if (!(chip->options & NAND_OWN_BUFFERS))
		kfree(chip->buffers);

	/* Free bad block descriptor memory */
	if (chip->badblock_pattern && chip->badblock_pattern->options
		& NAND_BBT_DYNAMICSTRUCT)
		kfree(chip->badblock_pattern);
}

EXPORT_SYMBOL_GPL(ca_nand_release);

#ifdef CONFIG_OF
static struct of_device_id ca_nflash_dt_ids[] = {
	{.compatible = "cortina,nflash",},
	{},
};

MODULE_DEVICE_TABLE(of, ca_nflash_dt_ids);
#endif

int init_DMA_SSP(struct platform_device *pdev)
{
	int i;

	dma_addr_t dma_tx_handle;
	dma_addr_t dma_rx_handle;

	DMA_SEC_DMA_GLB_DMA_SSP_RX_CTRL_t dma_rxdma_ctrl;
	DMA_SEC_DMA_GLB_DMA_SSP_TX_CTRL_t dma_txdma_ctrl;

	DMA_SEC_DMA_SSP_Q_RXQ_BASE_DEPTH_t dma_rxq_base_depth;
	DMA_SEC_DMA_SSP_Q_TXQ_BASE_DEPTH_t dma_txq_base_depth;

	dma_rxdma_ctrl.wrd = read_dma_glb_ctrl_reg(DMA_SEC_DMA_GLB_DMA_SSP_RX_CTRL);
	dma_txdma_ctrl.wrd = read_dma_glb_ctrl_reg(DMA_SEC_DMA_GLB_DMA_SSP_TX_CTRL);

	if ((dma_rxdma_ctrl.bf.rx_check_own != 1)
		&& (dma_rxdma_ctrl.bf.rx_dma_enable != 1)) {
		dma_rxdma_ctrl.bf.rx_check_own = 1;
		dma_rxdma_ctrl.bf.rx_dma_enable = 1;
		write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_CONTROL, dma_rxdma_ctrl.wrd);
	}
	if ((dma_txdma_ctrl.bf.tx_check_own != 1)
		&& (dma_txdma_ctrl.bf.tx_dma_enable != 1)) {
		dma_txdma_ctrl.bf.tx_check_own = 1;
		dma_txdma_ctrl.bf.tx_dma_enable = 1;
		write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_CONTROL, dma_txdma_ctrl.wrd);
	}

	tx_desc = dma_alloc_coherent(&pdev->dev,
								 (sizeof(DMA_SSP_TX_DESC_T) * FDMA_DESC_NUM),
								 &dma_tx_handle, GFP_KERNEL | GFP_DMA);
	rx_desc = dma_alloc_coherent(&pdev->dev,
								 (sizeof(DMA_SSP_RX_DESC_T) * FDMA_DESC_NUM),
								 &dma_rx_handle, GFP_KERNEL | GFP_DMA);

	if (!rx_desc || !tx_desc) {
		printk("Buffer allocation for failed!\n");
		if (rx_desc) {
			kfree(rx_desc);
		}

		if (tx_desc) {
			kfree(tx_desc);
		}

		return 0;
	}
	//printk("tx_desc_v: %p (p: %x), rx_desc_v: %p (p: %x)\n", tx_desc,
	//       dma_tx_handle, rx_desc, dma_rx_handle);
	/* set base address and depth */

	dma_rxq_base_depth.bf.base = dma_rx_handle >> 4;
	dma_rxq_base_depth.bf.depth = FDMA_DEPTH;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_BASE_DEPTH,
					   dma_rxq_base_depth.wrd);
	if(force_ace==1)
		write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_RXQ_BASE, 4);

	dma_txq_base_depth.bf.base = dma_tx_handle >> 4;
	dma_txq_base_depth.bf.depth = FDMA_DEPTH;
	write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_BASE_DEPTH,
					   dma_txq_base_depth.wrd);
	if(force_ace==1)
		write_dma_ctrl_reg(DMA_SEC_DMA_SSP_Q_TXQ_BASE, 4);

	memset((unsigned char *)tx_desc, 0,
		   (sizeof(DMA_SSP_TX_DESC_T) * FDMA_DESC_NUM));
	memset((unsigned char *)rx_desc, 0,
		   (sizeof(DMA_SSP_RX_DESC_T) * FDMA_DESC_NUM));

	for (i = 0; i < FDMA_DESC_NUM; i++) {
		/* set own by sw */
		tx_desc[i].word1.bf.own = OWN_SW;
		/* enable q5 Scatter-Gather memory copy */
		tx_desc[i].word1.bf.sgm = 0x1;
		if(force_ace==1){
			/* ACE */
			tx_desc[i].word1.bf.buf_adr_hi = 4;
			rx_desc[i].word1.bf.buf_adr_hi = 4;
		}
	}

	return 1;
}

/*
 *	hardware specific access to control-lines
 */
static void ca_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	/* ECC are generated through DMA transaction.
	 * ECC cannot generated indivudually.
	 */
}

/*
*	read device ready pin
*/
 int ca_nand_dev_ready(struct mtd_info *mtd)
{
	int ready, old_sts, new_sts;
	unsigned long timeo;

	check_flash_ctrl_status();

	write_flash_ctrl_reg(FLASH_NF_DATA, 0xffffffff);
	old_sts = read_flash_ctrl_reg(FLASH_STATUS);
	if ((old_sts & 0xffff) != 0)
		printk("old_sts : %x      ", old_sts);

  RD_STATUS:
	write_flash_ctrl_reg(FLASH_NF_ECC_CONTROL, 0x0);	/* disable ecc gen */
	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = NCNT_EMPTY_OOB;
	nf_cnt.bf.nflashRegDataCount = NCNT_DATA_1;
	nf_cnt.bf.nflashRegAddrCount = NCNT_EMPTY_ADDR;
	nf_cnt.bf.nflashRegCmdCount = NCNT_CMD_1;

	write_flash_ctrl_reg(FLASH_NF_COUNT, nf_cnt.wrd);

	nf_cmd.wrd = 0;
	nf_cmd.bf.nflashRegCmd0 = NAND_CMD_STATUS;
	write_flash_ctrl_reg(FLASH_NF_COMMAND, nf_cmd.wrd);	/* write read id command */
	nf_addr1.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_1, nf_addr1.wrd);	/* write address 0x00 */
	nf_addr2.wrd = 0;
	write_flash_ctrl_reg(FLASH_NF_ADDRESS_2, nf_addr2.wrd);	/* write address 0x00 */

	nf_access.wrd = 0;
	nf_access.bf.nflashCeAlt = CHIP_EN;
	/* nf_access.bf.nflashDirWr = ; */
	nf_access.bf.nflashRegWidth = NFLASH_WiDTH8;

	write_flash_ctrl_reg(FLASH_NF_ACCESS, nf_access.wrd);
	flash_start.wrd = 0;
	flash_start.bf.nflashRegReq = FLASH_GO;
	flash_start.bf.nflashRegCmd = FLASH_RD;
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, flash_start.wrd);

	timeo = jiffies + HZ;
	do {
		flash_start.wrd = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		if (!flash_start.bf.nflashRegReq)
			break;
	} while (time_before(jiffies, timeo));

	ready = read_flash_ctrl_reg(FLASH_NF_DATA) & 0xff;
	if (ready == 0xff) {
		new_sts = read_flash_ctrl_reg(FLASH_STATUS);
		printk
			("--------->   %s : FLASH_STATUS : old_sts:%x, new_sts:%x \n",
			 __func__, old_sts, new_sts);
		goto RD_STATUS;
	}

	return (ready & NAND_STATUS_READY);
}

#if CONFIG_CA77XX_SHOW_INFO
void
showInfo(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int blk_shift = (this->phys_erase_shift - this->page_shift);
	//int blk_num = (*page_id)>>blk_shift;
	printk("[NAND] this->phys_erase_shift is %d(%X)\n", this->phys_erase_shift, this->phys_erase_shift);
	printk("[NAND] this->page_shift is %d(%X)\n", this->page_shift, this->page_shift );
	printk("[NAND] blk_shift is %d(%X)\n", blk_shift, blk_shift );
	printk("[NAnD] ECC strength is %d\n", this->ecc.strength );
}
#else
#define showInfo(a) ;
#endif

#define CA77XX_PROC_NAME "nand"
static struct proc_dir_entry *_nand_dir = NULL;
static struct proc_dir_entry *proc_file_mtd_protect = NULL;

static void
createNandProc(struct mtd_info *mtd)
{

	struct nand_chip *this = (struct nand_chip *)mtd->priv;

	_nand_dir = proc_mkdir(CA77XX_PROC_NAME, NULL);

	proc_file_mtd_protect = proc_create(CA77XX_NAND_PROTECTED_NAME, 0644, _nand_dir, get_mtd_protect_proc());
	if (NULL == proc_file_mtd_protect) {
		printk("[NAND] can't create proc:" CA77XX_NAND_PROTECTED_NAME "\n");
	} else {
		ca77xx_nand_set_blk_protection_top(CA77XX_NAND_BOOT_DEFAULT_SIZE>>(this->phys_erase_shift));
	}
}

static void
removeNandProc(struct mtd_info *mtd)
{
	if (_nand_dir == NULL) {
		return;
	}
	if (proc_file_mtd_protect != NULL) {
		remove_proc_entry(CA77XX_NAND_PROTECTED_NAME, _nand_dir);
	}
	remove_proc_entry(CA77XX_PROC_NAME, NULL);
}

 int ca_nflash_probe(struct platform_device *pdev)
{
	struct nand_chip *this;
	struct resource mem_resource;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;
	struct mtd_part_parser_data ppdata;
	int ret, ecc_alg, err = 0;
	unsigned int flash_base;

	printk("CA-77XX NAND init ...\n");

	match = of_match_device(ca_nflash_dt_ids, &pdev->dev);
	if (!match)
		return -EINVAL;

	/* Flash controller base */
	ret = of_address_to_resource(np, 0, &mem_resource);
	if (ret) {
		dev_warn(&pdev->dev, "invalid address %d\n", ret);
		return ret;
	}

	flash_ctl_base = devm_ioremap(&pdev->dev, mem_resource.start,
								  resource_size(&mem_resource));

	/* Flash memory base */
	ret = of_address_to_resource(np, 1, &mem_resource);
	if (ret) {
		dev_warn(&pdev->dev, "invalid address %d\n", ret);
		return ret;
	}

	flash_base = (unsigned int)mem_resource.start;
	flash_mem_base = devm_ioremap(&pdev->dev, mem_resource.start,
								  resource_size(&mem_resource));

	/* DMA global controller */
	ret = of_address_to_resource(np, 2, &mem_resource);
	if (ret) {
		dev_warn(&pdev->dev, "invalid address %d\n", ret);
		return ret;
	}

	dma_ctl_glb_base = devm_ioremap(&pdev->dev, mem_resource.start,
									resource_size(&mem_resource));

	/* DMA controller */
	ret = of_address_to_resource(np, 3, &mem_resource);
	if (ret) {
		dev_warn(&pdev->dev, "invalid address %d\n", ret);
		return ret;
	}

	dma_ctl_base = devm_ioremap(&pdev->dev, mem_resource.start,
								resource_size(&mem_resource));

	/* Allocate memory for MTD device structure and private data */
	ca77xx_host = kzalloc(sizeof(struct ca77xx_nand_host), GFP_KERNEL);
	if (!ca77xx_host) {
		printk("Unable to allocate ca77xx_host NAND MTD device structure.\n");
		return -ENOMEM;
	}

	ca77xx_host->flash_mem_base = flash_base;
	if (!of_property_read_u32(np, "nand-ecc-alg", &ecc_alg))
		ca77xx_host->ecc_algorithm = ecc_alg;
	printk("ECC Algorithm: %d\n", ecc_alg);

	/* Get pointer to private data */
	/* Allocate memory for MTD device structure and private data */
	ca77xx_host->nand_chip =
		(struct nand_chip *)kzalloc(sizeof(struct nand_chip), GFP_KERNEL);
	if (!ca77xx_host->nand_chip) {
		printk("Unable to allocate CA77XX NAND MTD device structure.\n");
		err = -ENOMEM;
		goto err_mtd;
	}
	ca77xx_host->mtd =
		(struct mtd_info *)kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	if (!ca77xx_host->mtd) {
		printk("Unable to allocate CA77XX NAND MTD INFO structure.\n");
		err = -ENOMEM;
		goto err_mtd;
	}

	ca77xx_host->dev = &pdev->dev;
#ifndef NAND_DIRECT_ACCESS
	if(force_ace==1){
		/* domain = 2, cache = 2, ace_cmd = 1 */
		writel(0x02008080, dma_ctl_glb_base + 0x18);
		printk("Force ACE for NAND DMA\n");
	}
	if (init_DMA_SSP(pdev) == 0) {
		goto err_add;
	}
#endif

	this = (struct nand_chip *)(ca77xx_host->nand_chip);

	/* Link the private data with the MTD structure */
	ca77xx_host->mtd->priv = this;
	ca77xx_host->mtd->owner = THIS_MODULE;

	platform_set_drvdata(pdev, ca77xx_host);

	/* Set address of NAND IO lines */
	this->IO_ADDR_R = flash_ctl_base;
	this->IO_ADDR_W = flash_ctl_base;
	/* Set address of hardware control function */

	this->cmd_ctrl = ca_nand_hwcontrol;
	this->dev_ready = ca_nand_dev_ready;
	/* set eccmode using hardware ECC */
	this->ecc.mode = NAND_ECC_HW;

	/* vmalloc buffer cannot used on DMA.
	 * Define NAND_USE_BOUNCE_BUFFER will use temporary buffer for DMA.
	 * And then use memcpy to move to dest buffer.
	 */
	this->options |= NAND_USE_BOUNCE_BUFFER | NAND_NO_SUBPAGE_WRITE;
	
	if (ca77xx_host->ecc_algorithm == ECC_HAM_256_3B){
		this->ecc.size = 256;
		this->ecc.bytes = 3;
		this->ecc.strength = 1;
	}
	else if (ca77xx_host->ecc_algorithm == ECC_HAM_512_3B){
		this->ecc.size = 512;
		this->ecc.bytes = 3;
		this->ecc.strength = 1;
	}
	else{
		this->ecc.size = BCH_DATA_UNIT;
		if(ca77xx_host->ecc_algorithm == ECC_BCH_40){
			this->ecc.bytes = BCH_GF_PARAM_M * 5;
			this->ecc.strength = 40;
		}
		else{
			this->ecc.bytes = BCH_GF_PARAM_M * \
					(ca77xx_host->ecc_algorithm - ECC_BCH_8 + 1);
			this->ecc.strength = 8 * (ca77xx_host->ecc_algorithm - ECC_BCH_8 + 1);
		}
	}

	/* Set as Nand type */
	flash_type.wrd = 0;
	flash_type.bf.flashType = 5;	/* Init at 2K page, will be overwrite in scan */
	flash_type.bf.flashSize = 3;	/* Give a arbitary oob size, overwrite in scan_tail() */
	flash_type.bf.flashPin = 0;  	/* Flsah size > 256MB? */
	write_flash_ctrl_reg(FLASH_TYPE, flash_type.wrd);
	
	/* Scan to find existence of the device */
	err = ca_nand_scan(ca77xx_host->mtd, 1);
	if (err)
		goto err_scan;

	/* Register the partitions */
	ca77xx_host->mtd->name = "ca_nand_flash";

	/* Parse mtd partition and register to mtd framework */
	ppdata.of_node = pdev->dev.of_node;
	err = mtd_device_parse_register(ca77xx_host->mtd, NULL,
					&ppdata, NULL, 0);

	if (err)
		goto err_add;

	/* Return happy */
	printk("cx752x NAND init OK.\n");
	showInfo(ca77xx_host->mtd);
	createNandProc(ca77xx_host->mtd);

#if 0

	/** use to test direct access **/
	//direct access test for 128MB access size on 256MB
	//ca77xx_host->io_base
	//mtd->writesize
	//mtd->oobsize
	int i;
	//printk("Flash base at: 0x%p, access last: %x, mtd->oobsize: %x\n", ca77xx_host->io_base,(ca77xx_host->io_base+ca77xx_host->mtd->writesize-1),ca77xx_host->mtd->oobsize);
	unsigned char test_last_page[4096];

	memset(test_last_page, 0, 4096);
	memcpy(test_last_page,
		   (ca77xx_host->io_base + ca77xx_host->mtd->writesize - 4),
		   (ca77xx_host->mtd->oobsize + 4));

	for (i = 0; i < ca77xx_host->mtd->oobsize; i++) {

		printk(" %02x", test_last_page[i]);
		if ((i % 16) == 15)
			printk("\n");
	}
	printk("\n");

	memset(test_last_page, 0, 4096);
	memcpy(test_last_page,
		   (ca77xx_host->io_base + ca77xx_host->mtd->writesize - 3),
		   (ca77xx_host->mtd->oobsize + 3));

	for (i = 0; i < ca77xx_host->mtd->oobsize; i++) {

		printk(" %02x", test_last_page[i]);
		if ((i % 16) == 15)
			printk("\n");
	}
	printk("\n");
	write_flash_ctrl_reg(0xf00500a8, 7);
	memset(test_last_page, 0, 4096);
	memcpy(test_last_page,
		   (ca77xx_host->io_base + ca77xx_host->mtd->writesize - 2),
		   (ca77xx_host->mtd->oobsize + 2));

	for (i = 0; i < ca77xx_host->mtd->oobsize; i++) {

		printk(" %02x", test_last_page[i]);
		if ((i % 16) == 15)
			printk("\n");
	}
	printk("\n");
	write_flash_ctrl_reg(0xf00500a8, 7);
	memset(test_last_page, 0, 4096);
	memcpy(test_last_page,
		   (ca77xx_host->io_base + ca77xx_host->mtd->writesize - 1),
		   (ca77xx_host->mtd->oobsize + 1));
	for (i = 0; i < ca77xx_host->mtd->oobsize; i++) {

		printk(" %02x", test_last_page[i]);
		if ((i % 16) == 15)
			printk("\n");
	}
	printk("\n");

	write_flash_ctrl_reg(0xf00500a8, 7);
	memset(test_last_page, 0, 4096);
	memcpy(test_last_page, (ca77xx_host->io_base + 0x8000000 - 4),
		   (ca77xx_host->mtd->oobsize + 4));
	for (i = 0; i < ca77xx_host->mtd->oobsize; i++) {

		printk(" %02x", test_last_page[i]);
		if ((i % 16) == 15)
			printk("\n");
	}
	printk("\n");

	write_flash_ctrl_reg(0xf00500a8, 7);
#endif
	return 0;

  err_add:
	printk("Nand driver fail to add!\n");
	nand_release(ca77xx_host->mtd);

  err_scan:
	printk("Nand driver fail to scan!\n");
	platform_set_drvdata(pdev, NULL);
	iounmap(ca77xx_host->io_base);
  err_mtd:
	printk("Nand driver fail to malloc!\n");
	if (ca77xx_host->mtd)
		kfree(ca77xx_host->mtd);
	if (ca77xx_host->nand_chip)
		kfree(ca77xx_host->nand_chip);

	printk("Nand driver fail to ioremap!\n");
	kfree(ca77xx_host);

	return err;
}

 int ca_nflash_remove(struct platform_device *pdev)
{
	if (tmp_buf)
		kfree(tmp_buf);
	removeNandProc(ca77xx_host->mtd);
	return 0;
}

static struct platform_driver ca_nflash_driver = {
	.probe = ca_nflash_probe,
	.remove = ca_nflash_remove,
	.driver = {
			   .owner = THIS_MODULE,
			   .name = "ca_nflash",
			   .of_match_table = of_match_ptr(ca_nflash_dt_ids),
			   },
};

static int __init ca_nlfash_init(void)
{
	return platform_driver_register(&ca_nflash_driver);
}

static void __exit ca_nflash_exit(void)
{
	platform_driver_unregister(&ca_nflash_driver);
}

module_init(ca_nlfash_init);
module_exit(ca_nflash_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR
	("Middle Huang <middle.huang@cortina-systems.com>, Jason Li<jason.li@cortina-access.com>");
MODULE_DESCRIPTION("NAND flash driver for Cortina CA77XX flash module");
MODULE_ALIAS("platform:ca77xx_nand");
