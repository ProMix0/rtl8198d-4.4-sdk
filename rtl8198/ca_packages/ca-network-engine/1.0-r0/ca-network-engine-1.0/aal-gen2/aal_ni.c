/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * aal_ni.c: Hardware Abstraction Layer for NI driver to access hardware regsiters
 *
 */

#include <generated/ca_ne_autoconf.h>
#include <linux/sched.h>

#include <dt-bindings/soc/cortina_irqs.h>

#include "aal_table.h"
#include "aal_l3qm.h"
#include "aal_common.h"
#include "aal_port.h"
#include "aal_ni.h"
#include "ca_aal_proc.h"
#include "osal_common.h"
#include "scfg.h"

#define XRAM_CFG_PROF   CPU_XRAM_CFG_PROFILE_SEPARATE_LSPID

ca_uint8_t ca_ni_cpu_cpu_port_count = CA_NI_TOTAL_CPU_PORT - 2;; /* 2 ports reserved for PEs */

ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_vlan(int index, ca_vlan_id_t vlan_id, ca_uint8_t op);

ca_uint32_t ca_ne_ddr_noncache_reserved_size;
ca_uint32_t ca_ne_ddr_coherent_reserved_size;

void aal_ni_dump_registers(void)
{
	u32    reg;
	u32    value;

	printk("=====================================NI_HV GLB================================\n");
	for (reg = NI_HV_GLB_REVISION; reg <= NI_HV_GLB_DEBUG_2; reg+=4) {
		value = CA_NI_REG_READ(reg);
		printk("register=0x%x, value=0x%x\n", reg, value);
	}
	printk("=====================================NI_HV PT 5================================\n");
	for (reg = NI_HV_PT_PORT_STATIC_CFG + (5 * APB0_NI_HV_PT_STRIDE); reg <= NI_HV_PT_SPARESAMPLE + ( 5 * APB0_NI_HV_PT_STRIDE); reg+=4) {
		value = CA_NI_REG_READ(reg);
		printk("register=0x%x, value=0x%x\n", reg, value);
	}
	printk("=====================================NI_HV XRAM================================\n");
	for (reg = NI_HV_XRAM_CPUXRAM_ADRCFG_RX; reg <= NI_HV_XRAM_DMA_BYTE_CNT; reg+=4) {
		value = CA_NI_REG_READ(reg);
		printk("register=0x%x, value=0x%x\n", reg, value);
	}
}

void aal_ni_dma_lso_dump_registers(void)
{
	u32     reg;
	u32     value;

	printk("=====================================DMA LSO GLB================================\n");
	for (reg = DMA_SEC_DMA_GLB_DMA_LSO_CTRL; reg <= DMA_SEC_DMA_GLB_DMA_SSP_SPARE1; reg+=4) {
		value = CA_DMA_LSO_REG_READ(reg);
		printk("register=0x%x, value=0x%x\n", reg, value);
	}

	printk("=====================================DMA LSO VP================================\n");
	for (reg = DMA_SEC_DMA_LSO_VP_CONTROL; reg <= DMA_SEC_DMA_LSO_VP_HDRA_CFG; reg+=4) {
		value = CA_DMA_LSO_REG_READ(reg);
		printk("register=0x%x, value=0x%x\n", reg, value);
	}
}

static char *nirx_l3fe_demux_id_str[3] = {"L3QM", "WAN", "L2FE"};
void aal_ni_rx_l3fe_demux_dump_registers(void)
{
	ca_ni_rx_l3fe_demux_t           demux;

	printk("=== NIRX_L3FE_DEMUX ===\n");
	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0);
	printk("ldpid=0, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	printk("ldpid=1, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	printk("ldpid=2, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	printk("ldpid=3, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	printk("ldpid=4, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	printk("ldpid=5, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	printk("ldpid=6, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	printk("ldpid=7, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	printk("ldpid=8, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	printk("ldpid=9, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	printk("ldpid=10, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	printk("ldpid=11, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	printk("ldpid=12, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	printk("ldpid=13, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	printk("ldpid=14, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	printk("ldpid=15, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG1);
	printk("ldpid=16, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	printk("ldpid=17, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	printk("ldpid=18, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	printk("ldpid=19, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	printk("ldpid=20, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	printk("ldpid=21, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	printk("ldpid=22, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	printk("ldpid=23, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	printk("ldpid=24, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	printk("ldpid=25, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	printk("ldpid=26, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	printk("ldpid=27, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	printk("ldpid=28, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	printk("ldpid=29, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	printk("ldpid=30, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	printk("ldpid=31, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG2);
	printk("ldpid=32, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	printk("ldpid=33, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	printk("ldpid=34, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	printk("ldpid=35, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	printk("ldpid=36, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	printk("ldpid=37, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	printk("ldpid=38, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	printk("ldpid=39, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	printk("ldpid=40, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	printk("ldpid=41, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	printk("ldpid=42, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	printk("ldpid=43, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	printk("ldpid=44, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	printk("ldpid=45, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	printk("ldpid=46, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	printk("ldpid=47, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG3);
	printk("ldpid=48, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	printk("ldpid=49, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	printk("ldpid=50, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	printk("ldpid=51, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	printk("ldpid=52, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	printk("ldpid=53, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	printk("ldpid=54, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	printk("ldpid=55, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	printk("ldpid=56, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	printk("ldpid=57, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	printk("ldpid=58, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	printk("ldpid=59, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	printk("ldpid=60, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	printk("ldpid=61, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	printk("ldpid=62, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	printk("ldpid=63, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	printk("=== NIRX_L3FE_DPQ_DEMUX ===\n");
	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0);
	printk("ldpid=0, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	printk("ldpid=1, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	printk("ldpid=2, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	printk("ldpid=3, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	printk("ldpid=4, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	printk("ldpid=5, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	printk("ldpid=6, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	printk("ldpid=7, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	printk("ldpid=8, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	printk("ldpid=9, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	printk("ldpid=10, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	printk("ldpid=11, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	printk("ldpid=12, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	printk("ldpid=13, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	printk("ldpid=14, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	printk("ldpid=15, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG1);
	printk("ldpid=16, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	printk("ldpid=17, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	printk("ldpid=18, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	printk("ldpid=19, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	printk("ldpid=20, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	printk("ldpid=21, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	printk("ldpid=22, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	printk("ldpid=23, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	printk("ldpid=24, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	printk("ldpid=25, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	printk("ldpid=26, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	printk("ldpid=27, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	printk("ldpid=28, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	printk("ldpid=29, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	printk("ldpid=30, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	printk("ldpid=31, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG2);
	printk("ldpid=32, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	printk("ldpid=33, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	printk("ldpid=34, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	printk("ldpid=35, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	printk("ldpid=36, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	printk("ldpid=37, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	printk("ldpid=38, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	printk("ldpid=39, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	printk("ldpid=40, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	printk("ldpid=41, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	printk("ldpid=42, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	printk("ldpid=43, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	printk("ldpid=44, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	printk("ldpid=45, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	printk("ldpid=46, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	printk("ldpid=47, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG3);
	printk("ldpid=48, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	printk("ldpid=49, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	printk("ldpid=50, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	printk("ldpid=51, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	printk("ldpid=52, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	printk("ldpid=53, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	printk("ldpid=54, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	printk("ldpid=55, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	printk("ldpid=56, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	printk("ldpid=57, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	printk("ldpid=58, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	printk("ldpid=59, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	printk("ldpid=60, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	printk("ldpid=61, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	printk("ldpid=62, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	printk("ldpid=63, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);
}

ca_status_t aal_ni_set_mac_address(ca_uint8_t port_id, ca_uint8_t *mac_addr)
{
	NI_HV_GLB_MAC_ADDR_CFG0_t mac_addr_cfg0;
	NI_HV_GLB_MAC_ADDR_CFG1_t mac_addr_cfg1;
	NI_HV_PT_PORT_STATIC_CFG_t port_static_cfg;

	/* The complete MAC address consists of {MAC_ADDR0_mac_addr0[0-3], MAC_ADDR1_mac_addr1[4],
	   PT_PORT_STATIC_CFG_mac_addr6[5]}. */
	mac_addr_cfg0.wrd = 0;
	mac_addr_cfg0.bf.mac_addr0 = (mac_addr[0]<<24) + (mac_addr[1]<<16) + (mac_addr[2]<<8) + mac_addr[3];
	CA_NI_REG_WRITE(mac_addr_cfg0.wrd, NI_HV_GLB_MAC_ADDR_CFG0);

	mac_addr_cfg1.wrd = 0;
	mac_addr_cfg1.bf.mac_addr1 = mac_addr[4];
	CA_NI_REG_WRITE(mac_addr_cfg1.wrd, NI_HV_GLB_MAC_ADDR_CFG1);

	port_static_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * port_id);
	port_static_cfg.bf.mac_addr6 = mac_addr[5];

	CA_NI_REG_WRITE(port_static_cfg.wrd, NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * port_id);

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: port_id=%d, mac_addr=%pM\n", __func__, port_id, mac_addr);
	}

	return CA_E_OK;
}

void aal_ni_set_tx_write_ptr(ca_uint32_t reg, ca_uint32_t value)
{
	CA_DMA_LSO_REG_WRITE(value, reg);
}

ca_uint32_t aal_ni_get_tx_read_ptr(ca_uint32_t reg)
{
	ca_uint32_t value;

	value = CA_DMA_LSO_REG_READ(reg);

	return value;
}

void aal_ni_enable_rx_interrupt(ca_uint16_t cpu_port, ca_uint8_t enable)
{
	/* cpu_port = irq_no - IRQ_GLB_QM_CPU_FIFO0_REQ - 32 */
	aal_l3qm_enable_cpu_epp_interrupt(cpu_port, enable);

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: cpu_port=%d, enable=%d\n", __func__, cpu_port, enable);
	}
}

void aal_ni_enable_rx_interrupt_by_cpu_port(ca_uint16_t cpu_port, ca_uint8_t enable)
{
	aal_l3qm_enable_cpu_epp_interrupt(cpu_port, enable);

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: cpu_port=%d, enable=%d\n", __func__, cpu_port, enable);
	}
}

void aal_ni_write_rx_interrupt_status(ca_uint16_t irq_no, ca_uint32_t status)
{
	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: irq_no=%d, status=%d\n", __func__, irq_no, status);
	}
}

ca_uint32_t aal_ni_read_rx_interrupt_status(ca_uint16_t irq_no)
{
	ca_uint32_t status;

	status = CA_GLB_NE_INTR_REG_READ(GLOBAL_NETWORK_ENGINE_INTERRUPT_0);

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: irq_no=%d, status=%d\n", __func__, irq_no, status);
	}

	return status;
}

int aal_ni_has_work(int instance)
{
	return 1;   /* L3QM will check the packet available alway */
}

static void aal_ni_reset(void)
{
	int i;
	NI_HV_GLB_INIT_DONE_t    init_done;

	/* Check initializtion of NI block is done */
	for (i = 0; i < CA_NI_INIT_DONE_LOOP; i++) {
		init_done.wrd = CA_NI_REG_READ(NI_HV_GLB_INIT_DONE);
		if (init_done.bf.ni_init_done)
			break;
	}
	if (i == CA_NI_INIT_DONE_LOOP) {
		printk("%s: initializtion of NI block is NOT done!!!\n", __func__);
	}

	CA_NI_REG_WRITE(0x00000000, NI_HV_GLB_INTF_RST_CONFIG);
	printk("%s: reset NI done!\n", __func__);
}

#if 0
static void aal_ni_mgmt_init_xram_mem(void)
{
	NI_HV_XRAM_CPUXRAM_ADRCFG_RX_t    cpuxram_adrcfg_rx;
	NI_HV_XRAM_CPUXRAM_ADRCFG_TX_0_t cpuxram_adrcfg_tx;
	NI_HV_XRAM_CPUXRAM_CFG_t    cpuxram_cfg;

	cpuxram_adrcfg_rx.wrd = 0;
	cpuxram_adrcfg_tx.wrd = 0;

	/* RX XRAM ADDRESS CONFIG (start and end address) */
	cpuxram_adrcfg_rx.wrd = 0;
	cpuxram_adrcfg_rx.bf.rx_top_addr = CA_NI_RX_TOP_ADDR;
	cpuxram_adrcfg_rx.bf.rx_base_addr = CA_NI_RX_BASE_ADDR;
	CA_NI_REG_WRITE(cpuxram_adrcfg_rx.wrd, NI_HV_XRAM_CPUXRAM_ADRCFG_RX);

	/* TX XRAM ADDRESS CONFIG (start and end address) */
	cpuxram_adrcfg_tx.wrd = 0;
	cpuxram_adrcfg_tx.bf.tx_top_addr = CA_NI_TX_TOP_ADDR;
	cpuxram_adrcfg_tx.bf.tx_base_addr = CA_NI_TX_BASE_ADDR;
	CA_NI_REG_WRITE(cpuxram_adrcfg_tx.wrd, NI_HV_XRAM_CPUXRAM_ADRCFG_TX_0);

	cpuxram_cfg.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CFG);
	cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 1;
	cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 1;
	CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_HV_XRAM_CPUXRAM_CFG);
	cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 0;
	cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 0;
	CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_HV_XRAM_CPUXRAM_CFG);
}
#endif

static void aal_ni_init_tx_dma_lso(void)
{
	int i;
	DMA_SEC_DMA_GLB_DMA_LSO_CTRL_t    dma_lso_ctrl;
	DMA_SEC_DMA_LSO_VP_CONTROL_t    vp_control;
	DMA_SEC_AXIM2_CONFIG_t        axim2_config;
	DMA_SEC_SS_CTRL_t ss_ctrl;
	DMA_SEC_DMA_GLB_DMA_LSO_AXI_USER_SEL0_t user_sel;
	ca_ni_dma_lso_read_user_t       read_user;

	/* DMA VP TXQ control */
	vp_control.wrd = 0;
	vp_control.bf.txq0_en = 1;
	vp_control.bf.txq1_en = 1;
	vp_control.bf.txq2_en = 1;
	vp_control.bf.txq3_en = 1;
	vp_control.bf.txq4_en = 1;
	vp_control.bf.txq5_en = 1;
	vp_control.bf.txq6_en = 1;
	vp_control.bf.txq7_en = 1;

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: register DMA_SEC_DMA_LSO_VP_CONTROL offset=0x%x, DMA_SEC_DMA_LSO_VP_STRIDE value=%d\n",
				__func__, DMA_SEC_DMA_LSO_VP_CONTROL, DMA_SEC_DMA_LSO_VP_STRIDE);
	}
	for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
		CA_DMA_LSO_REG_WRITE(vp_control.wrd, DMA_SEC_DMA_LSO_VP_CONTROL + (i * DMA_SEC_DMA_LSO_VP_STRIDE));

		vp_control.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_CONTROL + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
		printk("%s: i = %d, vp_control.wrd=0x%x\n", __func__, i, vp_control.wrd);
	}

	/* setup AXI configuration */
	axim2_config.wrd = 0;
	axim2_config.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_AXIM2_CONFIG);
	axim2_config.bf.axi_write_outtrans_num = 0xf;
	axim2_config.bf.axi_read_outtrans_num = 0xf;
	axim2_config.bf.axi_xbus_len    = 2;
	axim2_config.bf.write_cacheline_trans_en = 1;
	axim2_config.bf.write_cacheline_trans_size = 1;
	axim2_config.bf.read_cacheline_trans_en = 1;
	axim2_config.bf.read_cacheline_trans_size = 1;
	CA_DMA_LSO_REG_WRITE(axim2_config.wrd, DMA_SEC_AXIM2_CONFIG);

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: register DMA_SEC_AXIM2_CONFIG offset=0x%x, axim2_config.wrd=0x%x\n", __func__, DMA_SEC_AXIM2_CONFIG, axim2_config.wrd);
	}

	/* set user pattern for DMA LSO ACE */
	user_sel.wrd=0;
	user_sel.bf.vp0_rd_pat = 1;
	user_sel.bf.vp1_rd_pat = 1;
	user_sel.bf.vp2_rd_pat = 1;
	user_sel.bf.vp3_rd_pat = 1;
	user_sel.bf.vp4_rd_pat = 1;
	user_sel.bf.vp5_rd_pat = 1;
	user_sel.bf.vp6_rd_pat = 1;
	user_sel.bf.vp7_rd_pat = 1;
	user_sel.bf.vp8_rd_pat = 1;
	user_sel.bf.vp9_rd_pat = 1;
	user_sel.bf.vp10_rd_pat = 1;
	user_sel.bf.vp11_rd_pat = 1;
	CA_DMA_LSO_REG_WRITE(user_sel.wrd, DMA_SEC_DMA_GLB_DMA_LSO_AXI_USER_SEL0);

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: register DMA_SEC_DMA_GLB_DMA_LSO_AXI_USER_SEL0 offset=0x%x, user_sel.wrd=0x%x\n", __func__,
				DMA_SEC_DMA_GLB_DMA_LSO_AXI_USER_SEL0, user_sel.wrd);
	}

	read_user.wrd = 0;
	read_user.bf.domain = 2;    /* outer sharable */
	read_user.bf.cache = 2;
	read_user.bf.ace_cmd = 1;
	CA_DMA_LSO_REG_WRITE(read_user.wrd, DMA_SEC_DMA_GLB_AXI_USER_PAT0);
	CA_DMA_LSO_REG_WRITE(read_user.wrd, DMA_SEC_DMA_GLB_AXI_USER_PAT1);
	CA_DMA_LSO_REG_WRITE(read_user.wrd, DMA_SEC_DMA_GLB_AXI_USER_PAT2);
	CA_DMA_LSO_REG_WRITE(read_user.wrd, DMA_SEC_DMA_GLB_AXI_USER_PAT3);

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: register DMA_SEC_DMA_GLB_AXI_USER_PAT0 offset=0x%x, read_user.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_AXI_USER_PAT0, read_user.wrd);
		printk("%s: register DMA_SEC_DMA_GLB_AXI_USER_PAT1 offset=0x%x, read_user.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_AXI_USER_PAT1, read_user.wrd);
		printk("%s: register DMA_SEC_DMA_GLB_AXI_USER_PAT2 offset=0x%x, read_user.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_AXI_USER_PAT2, read_user.wrd);
		printk("%s: register DMA_SEC_DMA_GLB_AXI_USER_PAT3 offset=0x%x, read_user.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_AXI_USER_PAT3, read_user.wrd);
	}

	/* setup DMA_SEC_SS_CTRL */
	ss_ctrl.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_SS_CTRL);
	ss_ctrl.bf.tx_en = 1;
	CA_DMA_LSO_REG_WRITE(ss_ctrl.wrd, DMA_SEC_SS_CTRL);
	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: register DMA_SEC_SS_CTRL=0x%x, ss_ctrl.wrd=0x%x\n", __func__, DMA_SEC_SS_CTRL, ss_ctrl.wrd);
	}

	dma_lso_ctrl.wrd = 0;
	dma_lso_ctrl.bf.tx_dma_enable = 1;
	dma_lso_ctrl.bf.tx_burst_len  = 3;      /* 64*64 bits */
	dma_lso_ctrl.bf.lso_padding_en = 1;     /* add padding data for under 64 bytes data */
	CA_DMA_LSO_REG_WRITE(dma_lso_ctrl.wrd, DMA_SEC_DMA_GLB_DMA_LSO_CTRL);

	dma_lso_ctrl.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_DMA_LSO_CTRL);
	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: register DMA_SEC_DMA_GLB_DMA_LSO_CTRL offset=0x%x, dma_lso_ctrl.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_DMA_LSO_CTRL, dma_lso_ctrl.wrd);
	}
}

static void aal_ni_set_pkt_len(void)
{
	NI_HV_GLB_PKT_LEN_CFG_t  pkt_len_cfg;

	pkt_len_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_PKT_LEN_CFG);
	pkt_len_cfg.bf.max_pkt_size = CA_NI_MAX_FRAME_SIZE;
	pkt_len_cfg.bf.min_pkt_size = CA_NI_MIN_FRAME_SIZE;
	CA_NI_REG_WRITE(pkt_len_cfg.wrd, NI_HV_GLB_PKT_LEN_CFG);
}

static void aal_ni_set_port_to_cpu(ca_uint8_t port)
{
	NI_HV_GLB_STATIC_CFG_t  static_cfg;

	static_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_STATIC_CFG);
	static_cfg.bf.port_to_cpu = port;
	static_cfg.bf.txmib_mode = 1;
	static_cfg.bf.rxmib_mode = 1;
	CA_NI_REG_WRITE(static_cfg.wrd, NI_HV_GLB_STATIC_CFG);
}

#if 0
static void aal_ni_mgmt_enable_tx_rx(void)
{
	NI_HV_XRAM_CPUXRAM_CFG_t cpuxram_cfg;

	/* Need to toggle the tx and rx cpu_pkt_dis bit */
	/* after changing Address config register.      */
	cpuxram_cfg.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CFG);
	cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 1;
	cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 1;
	CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_HV_XRAM_CPUXRAM_CFG);

	cpuxram_cfg.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CFG);
	cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 0;
	cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 0;
	cpuxram_cfg.bf.xram_mgmt_promisc_mode = 1;
	CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_HV_XRAM_CPUXRAM_CFG);
}
#endif

ca_status_t aal_ni_hv_glb_internal_port_id_cfg_get(
		CA_IN ca_device_id_t          device_id,
		CA_OUT aal_ni_hv_glb_internal_port_id_cfg_t *cfg
		)
{
	NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;

	port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	cfg->dma_to_l3fe = (ca_uint8_t) port_id_cfg.bf.dma_to_l3fe;
	cfg->wan_rxsel = (ca_uint8_t) port_id_cfg.bf.wan_rxsel;
	cfg->l3qmrx_demux_sel = (ca_uint8_t) port_id_cfg.bf.l3qmrx_demux_sel;
	cfg->mrr_dsel = (ca_uint8_t) port_id_cfg.bf.mrr_dsel;
	cfg->mrr_ldpid = (ca_uint8_t) port_id_cfg.bf.mrr_ldpid;
	cfg->mrr_cos = (ca_uint8_t) port_id_cfg.bf.mrr_cos;
	cfg->mrr_fe_bypass = (ca_uint8_t) port_id_cfg.bf.mrr_fe_bypass;
	cfg->egr_mrr_sel = (ca_uint8_t) port_id_cfg.bf.egr_mrr_sel;

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_ni_hv_glb_internal_port_id_cfg_get);

ca_status_t aal_ni_hv_glb_internal_port_id_cfg_set(
		CA_IN ca_device_id_t          device_id,
		CA_IN aal_ni_hv_glb_internal_port_id_cfg_mask_t mask,
		CA_IN aal_ni_hv_glb_internal_port_id_cfg_t *cfg
		)
{
	NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;
	port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);

	if (mask.bf.dma_to_l3fe) {
		port_id_cfg.bf.dma_to_l3fe = cfg->dma_to_l3fe;
	}

	if (mask.bf.wan_rxsel) {
		port_id_cfg.bf.wan_rxsel = cfg->wan_rxsel;
	}

	if (mask.bf.l3qmrx_demux_sel) {
		port_id_cfg.bf.l3qmrx_demux_sel = cfg->l3qmrx_demux_sel;
	}
	
	if (mask.bf.mrr_dsel) {
		port_id_cfg.bf.mrr_dsel = cfg->mrr_dsel;
	}
	
	if (mask.bf.mrr_ldpid) {
		port_id_cfg.bf.mrr_ldpid = cfg->mrr_ldpid;
	}
	
	if (mask.bf.mrr_cos) {
		port_id_cfg.bf.mrr_cos = cfg->mrr_cos;
	}
	
	if (mask.bf.mrr_fe_bypass) {
		port_id_cfg.bf.mrr_fe_bypass = cfg->mrr_fe_bypass;
	}
	
	if (mask.bf.egr_mrr_sel) {
		port_id_cfg.bf.egr_mrr_sel = cfg->egr_mrr_sel;
	}

	CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_ni_hv_glb_internal_port_id_cfg_set);

void aal_ni_init_nirx_l3fe_demux(void)
{
	ca_status_t ret;
	NI_HV_GLB_NIRX_L3FE_DEMUX_CFG2_t    l3fe_demux_cfg2;
	NI_HV_GLB_NIRX_L3FE_DEMUX_CFG3_t    l3fe_demux_cfg3;
	NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG2_t        l3fe_dpq_demux_cfg2;
	NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG3_t        l3fe_dpq_demux_cfg3;

	NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0_t    l3fe_demux_cfg0;
	NI_HV_GLB_NIRX_L3FE_DEMUX_CFG1_t    l3fe_demux_cfg1;
	NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0_t        l3fe_dpq_demux_cfg0;
	NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG1_t        l3fe_dpq_demux_cfg1;
	ca_ni_rx_l3fe_demux_t        demux;

	ca_uint8_t l3fe_to_wan_port;

	ret = aal_scfg_read(CFG_ID_L3FE_TO_WAN_PORT, 1, &l3fe_to_wan_port);
	if (ret != CA_E_OK) {
		printk("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT, ret=%d\n", __func__, ret);
		l3fe_to_wan_port = 1;
	}

	/* ldpid 0-15: L2FE */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* determined by start up config to go to WAN port or L3QM  */
	if (l3fe_to_wan_port)
		demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_WAN;
	else
		demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L2FE;
	l3fe_demux_cfg0.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_demux_cfg0.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0);

	/* ldpid 16-23: L3QM (CPU) */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 24-27: L2FE */
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 28: no assigned */
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 29: L3QM (CPUQ) */
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 30: L2FE (SPL) */
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 31: L3QM (BH) */
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_demux_cfg1.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_demux_cfg1.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG1);

	/* ldpid 32-47: L3QM (CPU256) */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_demux_cfg2.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_demux_cfg2.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG2);

	/* ldpid 48-63: L3QM (CPU256) */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_demux_cfg3.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_demux_cfg3.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG3);

	/* DEEPQ configure */
	/* all of ports default go through L3QM */
	demux.wrd = 0;

	/* ldpid 0-7: L3QM */
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* determined by start up config to go to WAN port or L3QM  */
	if (l3fe_to_wan_port)
		demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_WAN;
	else
		demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 8-15: L2FE */
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L2FE;
	l3fe_dpq_demux_cfg0.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_dpq_demux_cfg0.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0);

	/* ldpid 16-23: L3QM (CPU) */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 24-27: L2FE */
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 28: no assigned */
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 29: L3QM (CPUQ) */
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 30: L2FE (SPL) */
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 31: L3QM (BH) */
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_dpq_demux_cfg1.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_dpq_demux_cfg1.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG1);

	/* ldpid 32-47: L3QM (CPU256) */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_dpq_demux_cfg2.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_dpq_demux_cfg2.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG2);

	/* ldpid 48-63: L3QM (CPU256) */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_dpq_demux_cfg3.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_dpq_demux_cfg3.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG3);
}

static void aal_ni_init_dma_to_l3fe(void)
{
	int i;
	char dma_to_l3fe[CFG_ID_NE_DMA_TO_L3FE_LEN];
	NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;

	memset(&dma_to_l3fe[0], 0, CFG_ID_NE_DMA_TO_L3FE_LEN);
	if (aal_scfg_read(CFG_ID_NE_DMA_TO_L3FE, CFG_ID_NE_DMA_TO_L3FE_LEN, &(dma_to_l3fe[0])) == CA_E_OK) {
		port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
		for (i = 0; i < CFG_ID_NE_DMA_TO_L3FE_LEN; i++) {
			port_id_cfg.bf.dma_to_l3fe += (dma_to_l3fe[i] << i);
		}
		printk("%s: port_id_cfg.bf.dma_to_l3fe=0x%x\n", __func__, port_id_cfg.bf.dma_to_l3fe);
		CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	}
}

void aal_ni_init_ni(int port_id)
{
	NI_HV_GLB_NIRX_MISC_CFG_t nirx_misc;
#ifndef CONFIG_CORTINA_BOARD_FPGA
	NI_HV_GLB_XGE_BYPASS_CFG_t xge_bypass_cfg;
#endif

	if (aal_scfg_read(CFG_ID_NE_CPU_CPU_PORT_COUNT, 1, &ca_ni_cpu_cpu_port_count) == CA_E_OK) {
		if (!ca_ni_cpu_cpu_port_count || ca_ni_cpu_cpu_port_count > CA_NI_TOTAL_CPU_PORT) {
			ca_ni_cpu_cpu_port_count = CA_NI_TOTAL_CPU_PORT - 2;
			printk("%s: CFG_ID_NE_CPU_CPU_PORT_COUNT=%d too large!!\n", __func__, ca_ni_cpu_cpu_port_count);
		}
	}

	aal_ni_reset();

	/* set packet length globally */
	aal_ni_set_pkt_len();

	/* Initial TX DMA LSO */
	aal_ni_init_tx_dma_lso();

	aal_ni_set_port_to_cpu(0xff);   /* only for debugging */

	aal_ni_init_nirx_l3fe_demux();

	printk("%s: ======= NI_HV_GLB_INTERNAL_PORT_ID_CFG=0xef ==============\n", __func__);
	{
		NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;

		/* L3QM to L2TM for port 0-6 and to WAN for port 7 */
		port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
		port_id_cfg.bf.l3qmrx_demux_sel = 0x7f; /* according to bug 50073*/
		CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	}

	{
		NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;
		ca_device_config_tlv_t scfg_tlv;
		ca_status_t ret = CA_E_OK;
		ca_int8_t wan_rxsel = 0;

		memset(&scfg_tlv, 0, sizeof(scfg_tlv));
		scfg_tlv.type = CA_CFG_ID_WAN_TO_L3FE;
		ret = ca_device_config_tlv_get(0, &scfg_tlv);
		if (CA_E_OK == ret){
			wan_rxsel = scfg_tlv.value[0] ? 0 : 2;
		}

		/* L3QM to L2TM for port 0-6 and to WAN for port 7 */
		port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
		port_id_cfg.bf.wan_rxsel = wan_rxsel;
		CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	}

	nirx_misc.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_MISC_CFG);
#ifdef CONFIG_CORTINA_BOARD_FPGA
	printk("%s: ======= NI_HV_GLB_NIRX_MISC_CFG, use_p0_as_wan_port_en=1 for test ======\n", __func__);

	/* hardware not ready yet */
	//nirx_misc.bf.use_p0_as_wan_port_en = 1;
#else
	printk("%s: ======= NI_HV_GLB_NIRX_MISC_CFG, use_p0_as_wan_port_en=0 for ASIC ======\n", __func__);
	nirx_misc.bf.use_p0_as_wan_port_en = 0;
	nirx_misc.bf.l2te_ni_txdma_l3fe_rdy_en = 1;
	nirx_misc.bf.l2te_ni_txdma_l2fe_rdy_en = 1;
	nirx_misc.bf.l2te_ni_l3felan_port_rdy_en = 1;
	nirx_misc.bf.l2te_ni_l3fewan_port_rdy_en = 1;
	nirx_misc.bf.l2te_ni_mc_rdy_en = 1;  
#endif
	CA_NI_REG_WRITE(nirx_misc.wrd, NI_HV_GLB_NIRX_MISC_CFG);

}

static void aal_ni_enable_interrupts(void)
{
	NI_HV_GLB_PORT_8_INTERRUPTE_t   port_8_inte;
	NI_HV_GLB_INTERRUPTE_t       inte;
	NI_HV_GLB_TXEM_INTERRUPTE_t       txem_inte;

	/* Enable NI interrupt for CPU port */
	port_8_inte.wrd = 0;
	port_8_inte.bf.rxcntrl_overrunIE    = 1;
	port_8_inte.bf.rxmib_cntmsb_setIE    = 1;
	port_8_inte.bf.txmib_cntmsb_setIE    = 1;
	CA_NI_REG_WRITE(port_8_inte.wrd, NI_HV_GLB_PORT_8_INTERRUPTE + (CA_NI_PPORT_CPU * NI_HV_GLB_PORT_8_INTERRUPTE_STRIDE));

	/* Enable NI global interrupts */
	inte.wrd = 0;
	inte.bf.cpuxram_rxpktE = 1;
	inte.bf.cpuxram_txpktE = 1;
	CA_NI_REG_WRITE(inte.wrd, NI_HV_GLB_INTERRUPTE);

	/* Enable NI TXEM interrupt */
	txem_inte.wrd = 0;
	txem_inte.bf.txem_ififo_ovfIE     = 1;
	txem_inte.bf.txrm_cntovfIE     = 1;
	txem_inte.bf.txmib_fifo_ovfIE     = 1;
	txem_inte.bf.rxmib_fifo_ovfIE    = 1;
	txem_inte.bf.txrm_timerexpIE    = 1;
	CA_NI_REG_WRITE(txem_inte.wrd, NI_HV_GLB_TXEM_INTERRUPTE);
}

void aal_ni_get_dma_glb_lso_interrupt_status(void)
{
	DMA_SEC_DMA_GLB_LSO_INTERRUPT_t	dma_lso_status;

	dma_lso_status.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_LSO_INTERRUPT);
	printk("%s: dma_lso_status.wrd=0x%x\n", __func__, dma_lso_status.wrd);
	CA_DMA_LSO_REG_WRITE(dma_lso_status.wrd, DMA_SEC_DMA_GLB_LSO_INTERRUPT);
}

void aal_ni_get_dma_lso_interrupt_status(ca_uint8_t dma_vp)
{
        DMA_SEC_DMA_LSO_VP_VP_INTERRUPT_t vp_int_status;
        DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT_t vp_misc_int_status;
        DMA_SEC_DMA_LSO_VP_COAL_INTERRUPT_t vp_coal_int_status;
	ca_uint32_t offset;

	/* only test for VP 10 with CPU256 */
	offset = DMA_SEC_DMA_LSO_VP_VP_INTERRUPT + dma_vp * DMA_SEC_DMA_LSO_VP_STRIDE;
	vp_int_status.wrd = CA_DMA_LSO_REG_READ(offset);
	if (vp_int_status.bf.COAL_INTERRUPTi == 1 || vp_int_status.bf.MISC_INTERRUPTi == 1) {
		/* write back to diable interrupt */
		CA_DMA_LSO_REG_WRITE(vp_int_status.wrd, offset);

		offset = DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + dma_vp * DMA_SEC_DMA_LSO_VP_STRIDE;
		vp_misc_int_status.wrd = CA_DMA_LSO_REG_READ(offset);

		/* write back to diable interrupt */
		if (vp_misc_int_status.wrd) {
			if (vp_misc_int_status.bf.l2te_te_bp) {
				if (ca_aal_debug & AAL_DBG_DMA_LSO)
					printk("DMA LSO BP!!\n");
			}
			printk("%s: vp_misc_int_status.wrd=0x%x\n", __func__, vp_misc_int_status.wrd);
			CA_DMA_LSO_REG_WRITE(vp_misc_int_status.wrd, offset);
		}

		offset = DMA_SEC_DMA_LSO_VP_COAL_INTERRUPT + dma_vp * DMA_SEC_DMA_LSO_VP_STRIDE;
		vp_coal_int_status.wrd = CA_DMA_LSO_REG_READ(offset);

		/* write back to diable interrupt */
		if (vp_coal_int_status.wrd) {
			CA_DMA_LSO_REG_WRITE(vp_coal_int_status.wrd, offset);
		}
	}
}

static void aal_ni_enable_dma_lso_interrupts(void)
{
	int i;
	DMA_SEC_DMA_GLB_LSO_INTENABLE_t	dma_glb_lso_inten;
	DMA_SEC_DMA_LSO_VP_VP_INTENABLE_t vp_inten;
	DMA_SEC_DMA_LSO_VP_MISC_INTENABLE_t vp_misc_inten;
	DMA_SEC_DMA_LSO_VP_COAL_INTENABLE_t vp_coal_inten;

	vp_inten.wrd = 0;
	vp_inten.bf.MISC_INTERRUPTi_en = 1;
	vp_inten.bf.COAL_INTERRUPTi_en = 1;
	for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
		CA_DMA_LSO_REG_WRITE(vp_inten.wrd, DMA_SEC_DMA_LSO_VP_VP_INTENABLE + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
	}

	vp_misc_inten.wrd = 0;
	vp_misc_inten.bf.txq0_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq0_des_err_en     = 1;
	vp_misc_inten.bf.txq1_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq1_des_err_en    = 1;
	vp_misc_inten.bf.txq2_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq2_des_err_en        = 1;
	vp_misc_inten.bf.txq3_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq3_des_err_en        = 1;
	vp_misc_inten.bf.txq4_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq4_des_err_en        = 1;
	vp_misc_inten.bf.txq5_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq5_des_err_en        = 1;
	vp_misc_inten.bf.txq6_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq6_des_err_en        = 1;
	vp_misc_inten.bf.txq7_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq7_des_err_en        = 1;
	vp_misc_inten.bf.fbm_timeout_err_en     = 1;
	vp_misc_inten.bf.l2te_te_bp_en   	= 1;
	for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
		CA_DMA_LSO_REG_WRITE(vp_misc_inten.wrd, DMA_SEC_DMA_LSO_VP_MISC_INTENABLE + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
	}

	vp_coal_inten.wrd = 0;
	for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
		CA_DMA_LSO_REG_WRITE(vp_coal_inten.wrd, DMA_SEC_DMA_LSO_VP_COAL_INTENABLE + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
	}

	dma_glb_lso_inten.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_LSO_INTENABLE);
	dma_glb_lso_inten.bf.dma_aft_int_en = 1;
	dma_glb_lso_inten.bf.pkt_len_err_en = 1;
	dma_glb_lso_inten.bf.udp_dec_err_en = 1;
	dma_glb_lso_inten.bf.tcp_dec_err_en = 1;
	dma_glb_lso_inten.bf.ipv6_dec_err_en = 1;
	dma_glb_lso_inten.bf.ipv4_dec_err_en = 1;
	CA_DMA_LSO_REG_WRITE(dma_glb_lso_inten.wrd, DMA_SEC_DMA_GLB_LSO_INTENABLE);
}

void aal_ni_init_interrupt_cfg(void)
{

	/* Enable NI interrupts */
	aal_ni_enable_interrupts();

	/* L3QM enable interrupt source */
	aal_l3qm_enable_int_src();

	/* Enable DMA LSO interrupt */
	aal_ni_enable_dma_lso_interrupts();
}

void aal_ni_get_dma_lso_base_depth_addr(ca_uint8_t instance, ca_uint8_t txq_id)
{
	int i;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS_t      txq_base_depth_access;
	ca_uint32_t     reg_off = (instance * DMA_SEC_DMA_LSO_VP_STRIDE);
	ca_uint32_t    value;

	txq_base_depth_access.wrd = 0;
	txq_base_depth_access.bf.txq_id = txq_id;
	txq_base_depth_access.bf.access = 1;
	txq_base_depth_access.bf.rbw = 0;
	CA_DMA_LSO_REG_WRITE(txq_base_depth_access.wrd, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + reg_off);

	/* wait for complete */
	for (i = 0; i < CA_NI_DMA_LSO_BASE_DEPTH_LOOP; i++) {
		txq_base_depth_access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + reg_off);
		if (txq_base_depth_access.bf.access == 0)
			break;
	}

	if (i == CA_NI_DMA_LSO_BASE_DEPTH_LOOP) {
		printk("%s: indirect access write not finished!!!\n", __func__);
	}

	value = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0 + reg_off);

	CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1 + reg_off);

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0=0x%x, reg_off=%d, value=0x%x\n", __func__,
				DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0, reg_off, value);
		printk("%s: DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1=0x%x, reg_off=%d, value=0x%x\n", __func__,
				DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1, reg_off, value);
	}

}

void aal_ni_set_dma_lso_base_depth_addr(ca_uint8_t instance, ca_uint8_t txq_id, ca_uint32_t tx_desc_paddr)
{
	int i;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0_t    txq_base_depth_data0;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1_t    txq_base_depth_data1;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS_t    txq_base_depth_access;
	ca_uint32_t    reg_off = (instance * DMA_SEC_DMA_LSO_VP_STRIDE);

	txq_base_depth_data0.wrd = 0;
	txq_base_depth_data0.bf.base_addr = tx_desc_paddr >> 4;        /* base address bit [4:31] should be 16-byte alignment */
	txq_base_depth_data0.bf.depth = CA_NI_DMA_LSO_TXQ_DEPTH;      /* depth=log2(1024)=10 */

	CA_DMA_LSO_REG_WRITE(txq_base_depth_data0.wrd, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0 + reg_off);

	txq_base_depth_data1.wrd = 0;
	txq_base_depth_data1.bf.base_addr = 0;   /* should be 0  */

	CA_DMA_LSO_REG_WRITE(txq_base_depth_data1.wrd, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1 + reg_off);

	txq_base_depth_access.wrd = 0;
	txq_base_depth_access.bf.txq_id = txq_id;
	txq_base_depth_access.bf.access = 1;
	txq_base_depth_access.bf.rbw = 1;

	CA_DMA_LSO_REG_WRITE(txq_base_depth_access.wrd, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + reg_off);

	/* wait for complete */
	for (i = 0; i < CA_NI_DMA_LSO_BASE_DEPTH_LOOP; i++) {
		txq_base_depth_access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + reg_off);
		if (txq_base_depth_access.bf.access == 0)
			break;
	}

	if (i == CA_NI_DMA_LSO_BASE_DEPTH_LOOP) {
		printk("%s: indirect access write not finished!!!\n", __func__);
	}

	aal_ni_get_dma_lso_base_depth_addr(instance, txq_id);
}

void aal_ni_set_dma_lso_headerA(ca_uint16_t port_no, ca_uint16_t ldpid, ca_uint8_t febypass)
{
	DMA_SEC_DMA_LSO_VP_HDRA_CFG_t   hdra_cfg;

	printk("%s: port_no=%d, ldpid=%d, febypass=%d\n", __func__, port_no, ldpid, febypass);

	hdra_cfg.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_HDRA_CFG + port_no * DMA_SEC_DMA_LSO_VP_STRIDE);
	hdra_cfg.bf.hdra_ldpid = ldpid;
	hdra_cfg.bf.hdra_febypass = febypass;
	CA_DMA_LSO_REG_WRITE(hdra_cfg.wrd, DMA_SEC_DMA_LSO_VP_HDRA_CFG + port_no * DMA_SEC_DMA_LSO_VP_STRIDE);
}

void aal_ni_clear_dma_lso_misc_interrupts(ca_uint16_t cpu_port, ca_uint16_t txq_index)
{
	DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT_t	misc_int;

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: cpu_port=%d, txq_index=%d\n", __func__, cpu_port, txq_index);
	}
	misc_int.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);
	switch (txq_index) {
		case 0:
			misc_int.bf.txq0_empty = 1;
			break;
		case 1:
			misc_int.bf.txq1_empty = 1;
			break;
		case 2:
			misc_int.bf.txq2_empty = 1;
			break;
		case 3:
			misc_int.bf.txq3_empty = 1;
			break;
		case 4:
			misc_int.bf.txq4_empty = 1;
			break;
		case 5:
			misc_int.bf.txq5_empty = 1;
			break;
		case 6:
			misc_int.bf.txq5_empty = 1;
			break;
		case 7:
			misc_int.bf.txq5_empty = 1;
			break;
		default:
			ca_printf("%s: txq_index=%d out of range!!\n", __func__, txq_index);
			return;
	}
	CA_DMA_LSO_REG_WRITE(misc_int.wrd, DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);

	if (ca_aal_debug & AAL_DBG_NI) {
		misc_int.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);
		printk("%s: after write misc_int.wrd=0x%x\n", __func__, misc_int.wrd);
	}
}

#define CA_NI_DMA_LSO_EMPTY_TIMEOUT	200
ca_status_t aal_ni_wait_dma_lso_misc_interrupts(ca_uint16_t cpu_port, ca_uint16_t txq_index)
{
	int i;
	ca_uint8_t	empty;
	DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT_t     misc_int;

	if (ca_aal_debug & AAL_DBG_NI) {
		printk("%s: cpu_port=%d, txq_index=%d\n", __func__, cpu_port, txq_index);
	}
	for (i = 0; i < CA_NI_DMA_LSO_EMPTY_TIMEOUT; i++) {
		empty = 0;
		misc_int.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);
		if (ca_aal_debug & AAL_DBG_NI) {
			printk("%s: misc_int.wrd=0x%x\n", __func__, misc_int.wrd);
		}
		switch (txq_index) {
			case 0:
				empty = misc_int.bf.txq0_empty;
				break;
			case 1:
				empty = misc_int.bf.txq1_empty;
				break;
			case 2:
				empty = misc_int.bf.txq2_empty;
				break;
			case 3:
				empty = misc_int.bf.txq3_empty;
				break;
			case 4:
				empty = misc_int.bf.txq4_empty;
				break;
			case 5:
				empty = misc_int.bf.txq5_empty;
				break;
			case 6:
				empty = misc_int.bf.txq6_empty;
				break;
			case 7:
				empty = misc_int.bf.txq7_empty;
				break;
			default:
				ca_printf("%s: txq_index=%d out of range!!\n", __func__, txq_index);
				return CA_E_ERROR;
		}
		if (empty) {
			break;
		}
	}

	if (i == CA_NI_DMA_LSO_EMPTY_TIMEOUT) {
		ca_printf("%s: Wait for DMA LSO MISC interrupt empty bit timeout!!\n", __func__);
		ca_printf("%s: misc_int.wrd=0x%x\n", __func__, misc_int.wrd);
		ca_printf("%s: cpu_port=%d, txq_index=%d, offset=0x%x\n", __func__,
				cpu_port, txq_index, DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);
		return CA_E_ERROR;
	}
	aal_ni_clear_dma_lso_misc_interrupts(cpu_port, txq_index);
	return CA_E_OK;
}

/* for DMA AFT test, Venus only */
#define CA_NI_DMA_LSO_AFT_TIMEOUT     200
ca_status_t aal_ni_dma_lso_get_aft_l2fib(
	int index,
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t *data0,
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t *data1,
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t *data2)
{
	int i;
	DMA_SEC_DMA_AFT_L2FIB_ACCESS_t  access;

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 0;
	access.bf.addr = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_AFT_L2FIB_ACCESS);

	for (i = 0; i < CA_NI_DMA_LSO_AFT_TIMEOUT; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_ACCESS);
		if (access.bf.access == 0) {
			break;
		}
	}
	if (i == CA_NI_DMA_LSO_AFT_TIMEOUT) {
		printk("%s: read DMA_SEC_DMA_AFT_L2FIB_DATA timeout failed!!\n", __func__);
		return CA_E_ERROR;
	}

	data0->wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_DATA0);
	data1->wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_DATA1);
	data2->wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_DATA2);

	return CA_E_OK;
}
ca_status_t aal_ni_dma_lso_set_aft_l2fib(
	int index,
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t data0,
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t data1,
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t data2)
{
	int i;
	DMA_SEC_DMA_AFT_L2FIB_ACCESS_t  access;

        CA_DMA_LSO_REG_WRITE(data0.wrd, DMA_SEC_DMA_AFT_L2FIB_DATA0);
        CA_DMA_LSO_REG_WRITE(data1.wrd, DMA_SEC_DMA_AFT_L2FIB_DATA1);
        CA_DMA_LSO_REG_WRITE(data2.wrd, DMA_SEC_DMA_AFT_L2FIB_DATA2);

        access.wrd = 0;
        access.bf.access = 1;
        access.bf.rbw = 1;
        access.bf.addr = index;
        CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_AFT_L2FIB_ACCESS);

        for (i = 0; i < CA_NI_DMA_LSO_AFT_TIMEOUT; i++) {
                access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_ACCESS);
                if (access.bf.access == 0) {
                        break;
                }
        }
        if (i == CA_NI_DMA_LSO_AFT_TIMEOUT) {
                printk("%s: write DMA_SEC_DMA_AFT_L2FIB_DATA timeout failed!!\n", __func__);
                return CA_E_ERROR;
        }
        return CA_E_OK;
}

#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(aal_ni_dma_lso_get_aft_l2fib);
EXPORT_SYMBOL(aal_ni_dma_lso_set_aft_l2fib);
#endif
ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_vlan(int index, ca_vlan_id_t vlan_id, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t	data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t	data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t	data2;

	printk("%s: index=%d, vlan_id=%d, op=%d\n", __func__, index, vlan_id, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	printk("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.top_vid = vlan_id;
	printk("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;		/* stacking mode */
	data2.bf.vlan_cnt = op;		/* push(1)/pop(2)/swap(3) */
	data2.bf.top_tpid_enc = 1;	/* select TPID 1 */
	printk("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_vlan(int index, ca_vlan_id_t vlan_id, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	printk("%s: index=%d, vlan_id=%d, op=%d\n", __func__, index, vlan_id, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	data0.bf.inner_vid = (vlan_id & 0x3f);      /* bit 0-5 */
	printk("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.inner_tpid_enc = 1;	/* TPID 1 */
	data1.bf.inner_vid = (vlan_id >> 6) & 0x3f;    /* bit 6-11 */
	printk("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	data2.bf.inner_vlan_cmd = op;
	printk("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_vlan_8021p_dei(int index, ca_uint8_t vlan_8021p_dei, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	printk("%s: index=%d, vlan_8021p_dei=%d, op=%d\n", __func__, index, vlan_8021p_dei, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	printk("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.top_dei = (vlan_8021p_dei & 0x1);
	data1.bf.top_dei_sel = op;
	data1.bf.top_802_1p = (vlan_8021p_dei & 0xe) >> 1;
	data1.bf.top_1p_sel = op;
	printk("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	printk("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_vlan_8021p_dei(int index, ca_uint8_t vlan_8021p_dei, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	printk("%s: index=%d, vlan_8021p_dei=%d, op=%d\n", __func__, index, vlan_8021p_dei, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	data0.bf.inner_dei = (vlan_8021p_dei & 0x1);
	data0.bf.inner_dei_sel = op;
	data0.bf.inner_802_1p = (vlan_8021p_dei & 0xe) >> 1;
	data0.bf.inner_1p_sel = op;
	printk("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	printk("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	printk("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_tpid(int index, ca_uint8_t tpid_enc, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	printk("%s: index=%d, tpid_enc=%d, op=%d\n", __func__, index, tpid_enc, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	printk("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.top_tpid_sel = op & 1;    /* bit 0 */
	printk("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	data2.bf.top_tpid_enc = tpid_enc;
	data2.bf.top_tpid_sel = (op & 2) >> 1;    /* bit 1 */
	printk("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_tpid(int index, ca_uint8_t tpid_enc, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	printk("%s: index=%d, tpid_enc=%d, op=%d\n", __func__, index, tpid_enc, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	printk("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.inner_tpid_enc = tpid_enc;
	data1.bf.inner_tpid_sel = op;
	printk("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	printk("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_pppoe(int index, ca_uint16_t session_id, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	printk("%s: index=%d, session_id=%d, op=%d\n", __func__, index, session_id, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	data0.bf.pppoe_cmd = op;
	data0.bf.session_id = session_id;
	printk("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	printk("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	printk("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_dma_lso_dump_aft_l2fib(int index)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	printk("[index=%d]\n", index);
	printk("inner_vid[5:0]=%d, inner_dei=%d, inner_dei_sel=%d, inner_802_1p=%d, inner_1p_sel=%d, pppoe_cmd=%d, session_id=%d\n",
			data0.bf.inner_vid, data0.bf.inner_dei, data0.bf.inner_dei_sel, data0.bf.inner_802_1p,
			data0.bf.inner_1p_sel, data0.bf.pppoe_cmd, data0.bf.session_id);

	printk("top_tpid_sel[0]=%d, top_vid=%d, top_dei=%d, top_dei_sel=%d, top_802_1p=%d, top_1p_sel=%d\n",
			data1.bf.top_tpid_sel, data1.bf.top_vid, data1.bf.top_dei, data1.bf.top_dei_sel, data1.bf.top_802_1p, data1.bf.top_1p_sel);
	printk("inner_tpid_enc=%d, inner_tpid_sel=%d, inner_vid[11:6]=%d\n",
			data1.bf.inner_tpid_enc, data1.bf.inner_tpid_sel, data1.bf.inner_vid);

	printk("vlan_vld=%d, vlan_cnt=%d, inner_vlan_cmd=%d, top_tpid_enc=%d, top_tpid_sel[1]=%d\n",
			data2.bf.vlan_vld, data2.bf.vlan_cnt, data2.bf.inner_vlan_cmd, data2.bf.top_tpid_enc, data2.bf.top_tpid_sel);

	return CA_E_OK;
}

ca_status_t aal_ni_dma_lso_clean_aft_l2fib(int index)
{
        DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
        DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
        DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	data0.wrd = data1.wrd = data2.wrd = 0;
	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

        return CA_E_OK;
}
