/*
 * Copyright (C) 2008-2009 Paulius Zaleckas <paulius.zaleckas@teltonika.lt>
 *
 * Based on mach-pxa/include/mach/uncompress.h:
 * Copyright:	(C) 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MACH_UNCOMPRESS_H
#define __MACH_UNCOMPRESS_H

#include <linux/io.h>
#include <linux/serial_reg.h>
#include <soc/cortina/registers.h>

#define UTX_DATA 			0x10		/* UART TX Character data */
#define UINFO				0x18		/* UART Info */

#define  UINFO_TX_FIFO_EMPTY	(1<<3)

static __iomem void *uart_base = (__iomem void *) PER_UART0_CFG;

static inline void uart_w32(u32 val, unsigned reg)
{
	__raw_writel(val, uart_base + reg);
}

static inline u32 uart_r32(unsigned reg)
{
	return __raw_readl(uart_base + reg);
}

/*
 * The following code assumes the serial port has already been
 * initialized by the bootloader.  If you didn't setup a port in
 * your bootloader then nothing will appear (which might be desired).
 */
static inline void putc(char c)
{
	/* Wait till TX FIFO empty */
	while (!(uart_r32(UINFO) & UINFO_TX_FIFO_EMPTY))
		barrier();

	/* Send the character */
	uart_w32(c, UTX_DATA);
}

static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_setup()

#endif /* __MACH_UNCOMPRESS_H */
