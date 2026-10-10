#ifndef __ASM_KASAN_H
#define __ASM_KASAN_H

#ifndef __ASSEMBLY__

#ifdef CONFIG_KASAN

#include <linux/linkage.h>
#include <asm/addrspace.h>

#ifndef KASAN_SHADOW_SCALE_SHIFT
#define KASAN_SHADOW_SCALE_SHIFT (3)
#endif
#define KASAN_MEMORY_SIZE (0x10000000)
/*
 * KASAN_SHADOW_START: beginning of the kernel virtual addresses.
 * KASAN_SHADOW_END: KASAN_SHADOW_START + 1/8 of kernel virtual addresses.
 */
#define KASAN_SHADOW_START      (KSEG0 + KASAN_MEMORY_SIZE - (KASAN_MEMORY_SIZE >> KASAN_SHADOW_SCALE_SHIFT))
#define KASAN_SHADOW_END        (KASAN_SHADOW_START + (KASAN_MEMORY_SIZE >> KASAN_SHADOW_SCALE_SHIFT))

/*
 * KASAN_SHADOW_FIXED: take addr(>0xb0000000) as KASAN_SHADOW_FIXED
 */
#define KASAN_SHADOW_FIXED		(KSEG0 + 0x100000)

/*
 * This value is used to map an address to the corresponding shadow
 * address by the following formula:
 *     shadow_addr = (address >> 3) + KASAN_SHADOW_OFFSET;
 *
 */
#define KASAN_SHADOW_OFFSET     (KASAN_SHADOW_END - (KASAN_MEMORY_SIZE >> KASAN_SHADOW_SCALE_SHIFT) - (KSEG0 >> KASAN_SHADOW_SCALE_SHIFT))

void kasan_init(void);
asmlinkage void kasan_early_init(void);

#else
static inline void kasan_init(void) { }
#endif

#endif
#endif
