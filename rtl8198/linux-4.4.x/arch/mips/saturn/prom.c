/*
 * Copyright (C) 2006-2015  Realtek Semiconductor Corp.
 * Copyright (c) 2015-2017, Cortina Access. All rights reserved.
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/prom.h>

#include <asm/fw/fw.h>

const char *get_system_type(void)
{
	return "Saturn";
}

void __init prom_free_prom_memory(void)
{
	return;
}

/* Do basic initialization */
void __init prom_init(void)
{
	extern void plat_smp_init(void);
	extern void early_uart_init(void);

#ifndef CONFIG_OF
	fw_init_cmdline();
#endif

	mips_set_machine_name("Sheipa Platform");

#ifdef CONFIG_EARLY_PRINTK
	early_uart_init();
#endif

#ifdef CONFIG_SMP
	plat_smp_init();
#endif
}
