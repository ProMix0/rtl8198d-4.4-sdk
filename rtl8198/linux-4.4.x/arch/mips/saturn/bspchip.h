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

#ifndef _BSPCHIP_H_
#define _BSPCHIP_H_

#include <linux/version.h>
#include <soc/cortina/registers.h>

#define BSP_PLL_FREQ		25000000	   /* 25 MHz */
#define BSP_CPU_FREQ		(1 * BSP_PLL_FREQ) /* 25 MHz */

/*
 * IRQ Mapping
 *
 * We have two mappings:
 *
 * 1. UMP with ICTL
 * 2. SMP with GIC
 */
#define BSP_IRQ_GIC		(MIPS_CPU_IRQ_BASE + 2)  /* GIC IRQ */
#define BSP_IRQ_ICTL		(MIPS_CPU_IRQ_BASE + 2)  /* ICTL IRQ */
#define BSP_IRQ_IPI_RESCHED	(MIPS_CPU_IRQ_BASE + 3)
#define BSP_IRQ_IPI_CALL	(MIPS_CPU_IRQ_BASE + 4)
#define BSP_IRQ_PERFCOUNT       (MIPS_CPU_IRQ_BASE + 6)
#define BSP_IRQ_TIMER		(MIPS_CPU_IRQ_BASE + 7)  /* external timer */
#define BSP_IRQ_COMPARE         (MIPS_CPU_IRQ_BASE + 7)  /* local timer */

/*
 * Cortina UART
 */
#define BSP_UART0_VADDR		KSEG1_ATU_XLAT(PER_UART0_CFG) 
#define BSP_UART0_BAUD		115200
#define BSP_UART0_FREQ		(BSP_PLL_FREQ)
#define BSP_UART0_PADDR		ATU_XLAT(PER_UART0_CFG) 
#define BSP_UART0_PSIZE		0x100

#define BSP_UART1_VADDR		KSEG1_ATU_XLAT(PER_UART1_CFG)
#define BSP_UART1_BAUD		115200
#define BSP_UART1_FREQ		(BSP_PLL_FREQ)
#define BSP_UART1_PADDR		ATU_XLAT(PER_UART0_CFG) 
#define BSP_UART1_PSIZE		0x100

#define BSP_GLB_STRAP_ADDR  KSEG1_ATU_XLAT(GLOBAL_STRAP)

/* Register definitions */
#define UCFG				0x00		/* UART config register */
#define UFC					0x04		/* Flow Control */
#define URX_SAMPLE 			0x08		/* UART RX Sample register */
#define URT_TUNE			0x0C		/* Fine tune of UART clk */
#define UTX_DATA 			0x10		/* UART TX Character data */
#define URX_DATA 			0x14		/* UART RX Character data */
#define UINFO				0x18		/* UART Info */
#define UINT_EN0 			0x1C		/* UART Interrupt enable 0*/
#define UINT_EN1 			0x20		/* UART Interrupt enable 1*/
#define UINT0				0x24		/* UART Interrupt 0 setting/clearing */
#define UINT1				0x28		/* UART Interrupt 1 setting/clearing */
#define UINT_STAT			0x2C		/* UART Interrupt Status */
/*  */
#define  UINFO_TX_FIFO_EMPTY	(1<<3)
#define  UINFO_TX_FIFO_FULL		(1<<2)
#define  UINFO_RX_FIFO_EMPTY	(1<<1)
#define  UINFO_RX_FIFO_FULL		(1<<0)

unsigned int plat_saturn_io_clk(void);

unsigned int plat_saturn_cpu_clk(void);

#endif   /* _BSPCHIP_H */
