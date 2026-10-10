/*
 *	arch/mips/saturn/ioremap.h
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */
#ifndef __CA_SATURN__IOREMAP_H
#define __CA_SATURN__IOREMAP_H

#include <linux/types.h>

#define APB2_START	0xBC100000
#define APB2_END	0xBC10FFFF

#define APB3_START	0xB2220000
#define APB3_END	0xB2230000

#define NE_APB1_START	0xB4030000
#define NE_APB1_END	    0xB4040000

#define NE_APB4_START	0xB4020000
#define NE_APB4_END	    0xB4030000

#define NE_APB0_START	0xB4000000
#define NE_APB0_END	    0xB4020000

#define SD_EMMC_START	0xB2100000
#define SD_EMMC_END	    0xB2200000

#define SERIAL_FLASH_MEM_START	0xB0000000
#define SERIAL_FLASH_MEM_END    0xB2000000
/*
 * Allow physical addresses to be fixed up to help peripherals located
 * outside the low 32-bit range -- generic pass-through version.
 */
static inline phys_addr_t fixup_bigphys_addr(phys_addr_t phys_addr,
											 phys_addr_t size)
{
	return phys_addr;
}

static inline void __iomem *plat_ioremap(phys_addr_t offset, unsigned long size,
										 unsigned long flags)
{
	if ((offset >= APB2_START) && (offset < APB2_END))
		return (void __iomem *)offset;

	if ((offset >= APB3_START) && (offset < APB3_END))
		return (void __iomem *)offset;

	if ((offset >= NE_APB1_START) && (offset < NE_APB1_END))
		return (void __iomem *)offset;

	if ((offset >= NE_APB4_START) && (offset < NE_APB4_END))
		return (void __iomem *)offset;

	if ((offset >= NE_APB0_START) && (offset < NE_APB0_END))
		return (void __iomem *)offset;

	if ((offset >= SD_EMMC_START) && (offset < SD_EMMC_END))
		return (void __iomem *)offset;

	if ((offset >= SERIAL_FLASH_MEM_START) && (offset < SERIAL_FLASH_MEM_END))
        return (void __iomem *)offset;

	return NULL;
}

static inline int plat_iounmap(const volatile void __iomem * addr)
{
	if (((unsigned long)addr >= APB2_START) && ((unsigned long)addr < APB2_END))
		return (unsigned long)1;

	if (((unsigned long)addr >= APB3_START) && ((unsigned long)addr < APB3_END))
		return (unsigned long)1;

	if (((unsigned long)addr >= NE_APB1_START) && ((unsigned long)addr < NE_APB1_END))
		return (unsigned long)1;

	if (((unsigned long)addr >= NE_APB4_START) && ((unsigned long)addr < NE_APB4_END))
		return (unsigned long)1;

	if (((unsigned long)addr >= NE_APB0_START) && ((unsigned long)addr < NE_APB0_END))
		return (unsigned long)1;

	if (((unsigned long)addr >= SD_EMMC_START) && ((unsigned long)addr < SD_EMMC_END))
		return (unsigned long)1;

	if (((unsigned long)addr >= SERIAL_FLASH_MEM_START) && ((unsigned long)addr < SERIAL_FLASH_MEM_END))
        return (unsigned long)1;

	return 0;
}

#endif	/* __CA_SATURN__IOREMAP_H */
