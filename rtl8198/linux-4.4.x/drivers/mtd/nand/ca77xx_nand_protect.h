/*
 * =====================================================================================
 *
 *       Filename:  ca77xx_nand_protect.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:
 *        Company:
 *
 * =====================================================================================
 */



#include <linux/proc_fs.h>

#ifndef CA77XX_NAND_PROTECT_H_9H0ZTOMA
#define CA77XX_NAND_PROTECT_H_9H0ZTOMA





// protect 4MB in 4K page size
#define CA77XX_NAND_PROTECTED_BLOCK_DEFAULT_TOP (16)
// default boot section size is 4M (0x400000)
#define CA77XX_NAND_BOOT_DEFAULT_SIZE (0x400000)
#define CA77XX_NAND_PROTECTED_NAME "protected"

const struct file_operations *get_mtd_protect_proc(void);
uint32_t ca77xx_nand_get_blk_protection_top(void);
void ca77xx_nand_set_blk_protection_top(uint32_t new_top_value);

#endif /* end of include guard: CA77XX_NAND_PROTECT_H_9H0ZTOMA */
