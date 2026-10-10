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

#ifndef _BSPCPU_H_
#define _BSPCPU_H_

/*
 * scache => L2 cache
 * dcache => D cache
 * icache => I cache
 */
#ifndef CONFIG_SOC_OPT_OVERRIDE

#define cpu_icache_size		(64 << 10)
#define cpu_dcache_size		(32 << 10)
#define cpu_scache_size		0
#define cpu_icache_line		32
#define cpu_dcache_line		32
#define cpu_scache_line		0
#define cpu_tlb_entry		64
#define cpu_mem_size		(256 << 20)
#define cpu_imem_size		0
#define cpu_dmem_size		0
#define cpu_smem_size		0

#else

# ifndef CONFIG_SOC_OPT_CONFREGS
#  define cpu_icache_size	(CONFIG_SOC_ICACHE_SIZE << 10)
#  define cpu_dcache_size	(CONFIG_SOC_DCACHE_SIZE << 10)
#  define cpu_scache_size	(CONFIG_SOC_SCACHE_SIZE << 10)
#  define cpu_icache_line	(CONFIG_SOC_ICACHE_LINE)
#  define cpu_dcache_line	(CONFIG_SOC_DCACHE_LINE)
#  define cpu_scache_line	(CONFIG_SOC_SCACHE_LINE)
#  define cpu_tlb_entry		(CONFIG_SOC_TLB_ENTRY)
# endif

# define cpu_mem_size		(CONFIG_SOC_MEM_SIZE << 20)
# define cpu_imem_size		(CONFIG_SOC_IMEM_SIZE << 10)
# define cpu_dmem_size		(CONFIG_SOC_DMEM_SIZE << 10)
# define cpu_smem_size		(CONFIG_SOC_SMEM_SIZE << 10)

#endif

#endif
