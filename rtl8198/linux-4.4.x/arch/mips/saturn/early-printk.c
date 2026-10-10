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

#include <linux/io.h>
#include <linux/serial_reg.h>
#include <asm/addrspace.h>

#include "bspchip.h"

static __iomem void *uart_base = (__iomem void *) KSEG1ADDR(BSP_UART0_PADDR);

static inline void uart_w32(u32 val, unsigned reg)
{
	__raw_writel(val, uart_base + reg);
}

static inline u32 uart_r32(unsigned reg)
{
	return __raw_readl(uart_base + reg);
}

void early_uart_init(void)
{
	unsigned int baudrate, cfg;
    unsigned int io_clk_hz = plat_saturn_io_clk();
	/* 115200 baudrate based on 25MHz clock
	 */
	baudrate = io_clk_hz/BSP_UART0_BAUD;
	cfg = (baudrate << 8) | 0xe3;
	uart_w32(cfg, UCFG);
}

void prom_putchar(unsigned char ch)
{
	volatile unsigned int status;

	/* Wait till TX FIFO empty */
	do{
		status = uart_r32(UINFO);
	} while(!(status & UINFO_TX_FIFO_EMPTY));

	/* Send the character */
	uart_w32(ch, UTX_DATA);
}
