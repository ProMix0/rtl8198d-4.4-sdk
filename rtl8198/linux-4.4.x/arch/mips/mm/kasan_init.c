// SPDX-License-Identifier: GPL-2.0-only
/*
 * This file contains kasan initialization code for RTK MIPS
 * which borrowed from ARM64
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Author: Andrey Ryabinin <ryabinin.a.a@gmail.com>
 */

#define pr_fmt(fmt) "kasan: " fmt
#include <linux/kasan.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/start_kernel.h>
#include <linux/mm.h>

#include <asm/mmu_context.h>
#include <asm/page.h>
#include <asm/pgalloc.h>
#include <asm/sections.h>
#include <asm/tlbflush.h>

void __init kasan_init(void)
{
	/* At this point kasan is fully initialized. Enable error messages */
	init_task.kasan_depth = 0;
	pr_info("KernelAddressSanitizer initialized\n");
}
