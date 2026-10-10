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

#include <linux/sched.h>

#include <dt-bindings/soc/cortina_irqs.h>

#include "aal_table.h"
#include "aal_l3qm.h"
#include "aal_common.h"
#include "aal_port.h"
#include "aal_ni.h"
#include "ca_aal_proc.h"
#include "osal_common.h"
#include "ca-scfg/scfg.h"


/* moved to aal_ni.h */
#if 0
#define CA_NI_REG_READ(offset)        CA_REG_READ(AAL_NI_REG_ADDR(offset - NI_HV_GLB_REVISION))
#define CA_NI_REG_WRITE(data, offset)    CA_REG_WRITE(data, AAL_NI_REG_ADDR(offset - NI_HV_GLB_REVISION))

#define CA_DMA_LSO_REG_READ(offset)            CA_REG_READ(AAL_DMA_REG_ADDR(offset - DMA_SEC_DMA_GLB_DMA_LSO_CTRL))
#define CA_DMA_LSO_REG_WRITE(data, offset)    CA_REG_WRITE(data, AAL_DMA_REG_ADDR(offset - DMA_SEC_DMA_GLB_DMA_LSO_CTRL))

#define CA_GLB_REG_READ(offset)          CA_REG_READ(AAL_GLB_REG_ADDR(offset - GLOBAL_JTAG_ID))
#define CA_GLB_REG_WRITE(data, offset)   CA_REG_WRITE(data, AAL_GLB_REG_ADDR(offset - GLOBAL_JTAG_ID))
#endif

//#define APB0_NI_HV_PT_STRIDE    0x100

#if 0
/*total rx+tx buf need to <= 2048*/
//TODO: need to check with G3 register.db
static unsigned int cpuxram_rx_addr_cfg_prof[CPU_XRAM_CFG_PROFILE_MAX][XRAM_RX_INSTANCE + 1] =
{       {581, 581, 581, 7, 7, 7, 7, 7, 56},
    {525, 525, 525, 7, 7, 7, 91, 91, 56},
    {448, 448, 448, 112, 112, 56, 56, 70, 70}
};

static unsigned int cpuxram_tx_addr_cfg_prof[CPU_XRAM_CFG_PROFILE_MAX][XRAM_TX_INSTANCE] =
{       {214, 0},
    {214, 0},
    {214, 0}
};
#endif

#define XRAM_CFG_PROF   CPU_XRAM_CFG_PROFILE_SEPARATE_LSPID

ca_uint8_t ca_ni_cpu_cpu_port_count = CA_NI_TOTAL_CPU_PORT - 2;; /* 2 ports reserved for PEs */

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

/* remove unused codes */
#if 0
ca_status_t aal_ni_set_eth_cfg(ca_uint8_t port_id, ca_uint8_t int_cfg, ca_uint8_t rmii_clk_src)
{
    NI_HV_PT_PORT_STATIC_CFG_t    port_static_cfg;

    if (port_id > CA_NI_PORT6) {
        printk("%s: port_id=%d not supported!!!\n", __func__, port_id);
        return CA_E_PARAM;
    }

    port_static_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_PORT_STATIC_CFG + (port_id * APB0_NI_HV_PT_STRIDE));
    port_static_cfg.bf.int_cfg = int_cfg;
    port_static_cfg.bf.rmii_clksrc = rmii_clk_src;
    CA_NI_REG_WRITE(port_static_cfg.wrd, NI_HV_PT_PORT_STATIC_CFG + (port_id * APB0_NI_HV_PT_STRIDE));

    if (ca_aal_debug & AAL_DBG_NI) {
        printk("%s: port_id=%d, int_cfg=%d, rmii_clk_src=%d\n", __func__, port_id, int_cfg, rmii_clk_src);
    }
    return CA_E_OK;
}

static void aal_ni_set_mac_tx_rx(ca_uint8_t port_id, ca_uint8_t enable)
{
    NI_HV_PT_RXMAC_CFG_t rxmac_cfg;
    NI_HV_PT_TXMAC_CFG_t txmac_cfg;

    rxmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_RXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
    rxmac_cfg.bf.rx_en = enable;
    CA_NI_REG_WRITE(rxmac_cfg.wrd, NI_HV_PT_RXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));

    txmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_TXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
    txmac_cfg.bf.tx_en = enable;
    CA_NI_REG_WRITE(txmac_cfg.wrd, NI_HV_PT_TXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));

    if (ca_aal_debug & AAL_DBG_NI) {
        printk("%s: enable=%d\n", __func__, enable);
    }
}

void aal_ni_set_flow_control(ca_uint8_t port_id, ca_uint8_t rx_tx, ca_uint8_t disable)
{
    NI_HV_PT_RXMAC_CFG_t rxmac_cfg;
    NI_HV_PT_TXMAC_CFG_t txmac_cfg;

    aal_ni_set_mac_tx_rx(port_id, 0);

    if (rx_tx & CA_NI_RX_FLOW_CTRL) {
        rxmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_RXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
        rxmac_cfg.bf.rx_flow_disable = disable;
        CA_NI_REG_WRITE(rxmac_cfg.wrd, NI_HV_PT_RXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
    }
    else {
        rxmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_RXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
        rxmac_cfg.bf.rx_flow_disable = 1;
        CA_NI_REG_WRITE(rxmac_cfg.wrd, NI_HV_PT_RXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
    }

    if (rx_tx & CA_NI_TX_FLOW_CTRL) {
        txmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_TXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
        txmac_cfg.bf.tx_flow_disable = disable;
        CA_NI_REG_WRITE(txmac_cfg.wrd, NI_HV_PT_TXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
    }
    else {
        txmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_TXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
        txmac_cfg.bf.tx_flow_disable = 1;
        CA_NI_REG_WRITE(txmac_cfg.wrd, NI_HV_PT_TXMAC_CFG + (APB0_NI_HV_PT_STRIDE * port_id));
    }

    aal_ni_set_mac_tx_rx(port_id, 1);

    if (ca_aal_debug & AAL_DBG_NI) {
        printk("%s: port_id=%d, rx_tx=%d, disable=%d\n", __func__, port_id, rx_tx, disable);
    }
}


ca_status_t aal_ni_set_mac_speed_duplex(ca_uint8_t port_id, ca_uint16_t speed, ca_uint8_t duplex, ca_uint16_t mac_interface)
{
    NI_HV_PT_PORT_GLB_CFG_t port_glb_config;

    port_glb_config.wrd = CA_NI_REG_READ(NI_HV_PT_PORT_GLB_CFG + APB0_NI_HV_PT_STRIDE * port_id);
    port_glb_config.bf.speed = speed;   /* 1: 10Mbps, 0: 100Mbps */
    port_glb_config.bf.duplex = duplex; /* 1: half duplex, 0: full duplex */
    CA_NI_REG_WRITE(port_glb_config.wrd, NI_HV_PT_PORT_GLB_CFG + APB0_NI_HV_PT_STRIDE * port_id);

    if (ca_aal_debug & AAL_DBG_NI) {
        printk("%s: port_id=%d, speed=%d, duplex=%d, mac_interface=%d\n", __func__, port_id, speed, duplex, mac_interface);
    }

    return CA_E_OK;
}
#endif /* 0 */

ca_status_t aal_ni_set_mac_address(ca_uint8_t port_id, ca_uint8_t *mac_addr)
{
    NI_HV_GLB_MAC_ADDR_CFG0_t mac_addr_cfg0;
    NI_HV_GLB_MAC_ADDR_CFG1_t mac_addr_cfg1;
    NI_HV_PT_PORT_STATIC_CFG_t port_static_cfg;

    /* in mgmt mode, only port 5 can be set mac address */
#if defined(CONFIG_NE_MANAGEMENT_MODE)
    if (port_id != CA_NI_MGMT_PORT) {
        return CA_E_OK;
    }
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    if (port_id != CA_PORT_ID_NI3 && port_id != CA_PORT_ID_NI4)
	    return CA_E_ERROR;
#endif

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

    /* set MAC address only do not touch int_cfg */
#if 0
#ifdef CONFIG_CORTINA_BOARD_FPGA
    port_static_cfg.bf.int_cfg = 3;
#else
    port_static_cfg.bf.int_cfg = 0;
#endif
#endif

    CA_NI_REG_WRITE(port_static_cfg.wrd, NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * port_id);

        if (ca_aal_debug & AAL_DBG_NI) {
                printk("%s: port_id=%d, mac_addr=%pM, \n", __func__, port_id, mac_addr);
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

#if defined(CONFIG_NE_MANAGEMENT_MODE)
    NI_HV_GLB_CPUXRAM_RXPKT_INTERRUPTE_t    rxpkt_inten;
        rxpkt_inten.wrd = 0;
        rxpkt_inten.bf.xram_rx_doneIE = enable;
        CA_NI_REG_WRITE(rxpkt_inten.wrd, NI_HV_GLB_CPUXRAM_RXPKT_INTERRUPTE);
#else
        /* cpu_port = irq_no - IRQ_GLB_QM_CPU_FIFO0_REQ - 32 */
        //aal_l3qm_enable_cpu_epp_interrupt(irq_no - IRQ_GLB_QM_CPU_FIFO0_REQ - 32, enable);
        aal_l3qm_enable_cpu_epp_interrupt(cpu_port, enable);
#endif

    if (ca_aal_debug & AAL_DBG_NI) {
        printk("%s: cpu_port=%d, enable=%d \n", __func__, cpu_port, enable);
    }
}

void aal_ni_enable_rx_interrupt_by_cpu_port(ca_uint16_t cpu_port, ca_uint8_t enable)
{
        NI_HV_GLB_CPUXRAM_RXPKT_INTERRUPTE_t    rxpkt_inten;

#if defined(CONFIG_NE_MANAGEMENT_MODE)
                rxpkt_inten.wrd = 0;
                rxpkt_inten.bf.xram_rx_doneIE = enable;
                CA_NI_REG_WRITE(rxpkt_inten.wrd, NI_HV_GLB_CPUXRAM_RXPKT_INTERRUPTE);
#else
                aal_l3qm_enable_cpu_epp_interrupt(cpu_port, enable);
#endif

        if (ca_aal_debug & AAL_DBG_NI) {
                printk("%s: cpu_port=%d, enable=%d\n", __func__, cpu_port, enable);
        }
}

void aal_ni_write_rx_interrupt_status(ca_uint16_t irq_no, ca_uint32_t status)
{
#if defined(CONFIG_NE_MANAGEMENT_MODE)
    NI_HV_GLB_CPUXRAM_RXPKT_INTERRUPT_t rxpkt_int;
        rxpkt_int.wrd = 0;
        rxpkt_int.bf.xram_rx_doneI = status;
        CA_NI_REG_WRITE(rxpkt_int.wrd, NI_HV_GLB_CPUXRAM_RXPKT_INTERRUPT);
#endif

    if (ca_aal_debug & AAL_DBG_NI) {
        printk("%s: irq_no=%d, status=%d\n", __func__, irq_no, status);
    }
}

ca_uint32_t aal_ni_read_rx_interrupt_status(ca_uint16_t irq_no)
{
    ca_uint32_t status;
    //GLOBAL_NETWORK_ENGINE_INTERRUPT_0_t  ne_intr;

#if defined(CONFIG_NE_MANAGEMENT_MODE)
        status = CA_NI_REG_READ(NI_HV_GLB_CPUXRAM_RXPKT_INTERRUPT);
#else
        //status = aal_l3qm_read_rx_interrupt_status();
        status = CA_GLB_NE_INTR_REG_READ(GLOBAL_NETWORK_ENGINE_INTERRUPT_0);
#endif

    if (ca_aal_debug & AAL_DBG_NI) {
        printk("%s: irq_no=%d, status=%d \n", __func__, irq_no, status);
    }

    return status;
}

int aal_ni_has_work(int instance)
{
    //ca_uint32_t hw_wr_ptr, next_link;
#if defined(CONFIG_NE_MANAGEMENT_MODE)
    NI_HV_XRAM_CPUXRAM_CPU_STA_RX_0_t    cpu_sta_rx;
    NI_HV_XRAM_CPUXRAM_CPU_CFG_RX_0_t    cpu_cfg_rx;

        cpu_sta_rx.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CPU_STA_RX_0);
        cpu_cfg_rx.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CPU_CFG_RX_0);

        return (cpu_sta_rx.bf.pkt_wr_ptr != cpu_cfg_rx.bf.pkt_rd_ptr);
#else
    return 1;   /* L3QM will check the packet available alway */
#endif
}

/*
 * Initialize the NI port calendar
 *      Goldengate supports 96 (maximum) time slot port calendar.
 *      The source port numbering is as follows:
 *      0 - GE0
 *      1 - GE1
 *      2 - GE2
 *      3 - CPU
 *      4 - Crypto
 *      5 - Encap
 *      6 - Mcast
 *      7 - Mirror
 *      GigE ports, 1.2 Gbps/100mbps = 12 slots * 3 ports = 36 slots
 *      Encap and crypto ports each need 1.2 Gbps/100mbps =
 *                      12 slots * 2 ports = 24 slots
 *      Multicast port needs 1.2Gbps/100mbps = 12 slots * 1 port = 12 slots
 *      CPU port needs 1.2 Gbps/100 mbps=12 slots * 1 port = 12 slots
 *      Mirror port needs 1.2 Gbps/100 mbps=12 slots * 1 port = 12 slots
 */
#if 0
static void aal_ni_set_port_calendar(ca_uint16_t table_address, ca_uint8_t pspid_ts)
{
    NI_RX_PORT_CAL_ACCESS_t rx_port_cal_acc, rx_port_cal_acc_mask;
    NI_RX_PORT_CAL_DATA_t rx_port_cal_data, rx_port_cal_data_mask;
    int access;

    rx_port_cal_data.wrd = 0;
    rx_port_cal_data_mask.wrd = 0;

    rx_port_cal_data.bf.pspid_ts = pspid_ts;
    rx_port_cal_data_mask.bf.pspid_ts = 0x7;
    //write_reg(rx_port_cal_data.wrd, rx_port_cal_data_mask.wrd, NI_RX_PORT_CAL_DATA);

    rx_port_cal_acc.wrd = 0;
    rx_port_cal_acc_mask.wrd = 0;

    rx_port_cal_acc.bf.access = 1;
    rx_port_cal_acc.bf.rxpc_page = 0;
    rx_port_cal_acc.bf.cpu_page = 0;
    rx_port_cal_acc.bf.rbw = 1;             /* 1: Write */
    rx_port_cal_acc.bf.address = table_address;

    rx_port_cal_acc_mask.bf.access = 1;
    rx_port_cal_acc_mask.bf.rxpc_page = 1;
    rx_port_cal_acc_mask.bf.cpu_page = 1;
    rx_port_cal_acc_mask.bf.rbw = 1;
    rx_port_cal_acc_mask.bf.address = 0x7F;
    //write_reg(rx_port_cal_acc.wrd, rx_port_cal_acc_mask.wrd, NI_RX_PORT_CAL_ACCESS);
    do {
        access = (CA_NI_REG_READ(NI_TX_VOQ_LKUP_ACCESS) & 0x80000000);
        udelay(1);
    } while (access == 0x80000000);
    return 0;
}
#endif

#if 0
static void aal_ni_port_cal_cfg(void)
{
    int i, j;
    int slot_per_port = CA_NI_MAX_PORT_CALENDAR / CA_NI_NUM_OF_PORT;

    for (i = 0; i < slot_per_port; i++) {
        for (j = 0; j < CA_NI_NUM_OF_PORT; j++) {
            aal_ni_set_port_calendar((i * CA_NI_NUM_OF_PORT) + j, j);
        }
    }

    for (i = 0; i < slot_per_port; i++)
        aal_ni_set_port_calendar((i * CA_NI_NUM_OF_PORT) + 2, 4);
}
#endif

static void aal_ni_reset(void)
{
    int i;
    //NI_HV_GLB_INTF_RST_CONFIG_t intf_rst_config;
    NI_HV_GLB_INIT_DONE_t    init_done;

    ///TODO: should we reste ni by wrting GLOBAL_BLOCK_RESET???

    /* Check initializtion of NI block is done */
    for (i = 0; i < CA_NI_INIT_DONE_LOOP; i++) {
        init_done.wrd = CA_NI_REG_READ(NI_HV_GLB_INIT_DONE);
        if (init_done.bf.ni_init_done)
            break;
    }
    if (i == CA_NI_INIT_DONE_LOOP) {
        printk("%s: initializtion of NI block is NOT done!!!\n", __func__);
    }

    /* for bit file from 20160514 NI default in reset mode need software to release reset */
#if 0
    /* Apply resets */
    intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);
    intf_rst_config.bf.intf_rst_p0 = 1;
    intf_rst_config.bf.intf_rst_p1 = 1;
    intf_rst_config.bf.intf_rst_p2 = 1;
    intf_rst_config.bf.intf_rst_p3 = 1;
    intf_rst_config.bf.intf_rst_p4 = 1;
    //intf_rst_config.bf.intf_rst_p5 = 1;
    //intf_rst_config.bf.intf_rst_p6 = 1;
    CA_NI_REG_WRITE(intf_rst_config.wrd, NI_HV_GLB_INTF_RST_CONFIG);

    /* Remove resets */
    intf_rst_config.wrd = CA_NI_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);
    intf_rst_config.bf.intf_rst_p0 = 0;
        intf_rst_config.bf.intf_rst_p1 = 0;
        intf_rst_config.bf.intf_rst_p2 = 0;
        intf_rst_config.bf.intf_rst_p3 = 0;
        intf_rst_config.bf.intf_rst_p4 = 0;
        //intf_rst_config.bf.intf_rst_p5 = 0;
        //intf_rst_config.bf.intf_rst_p6 = 0;
    CA_NI_REG_WRITE(intf_rst_config.wrd, NI_HV_GLB_INTF_RST_CONFIG);
#endif
#if defined(CONFIG_ARCH_CORTINA_G3HGU)

    CA_NI_REG_WRITE(0x00000000, NI_HV_GLB_INTF_RST_CONFIG);
    CA_8279_NE_REG_WRITE(0x00006318, NI_HV_GLB_INTF_RST_CONFIG);
    mdelay(10);
#else
    CA_NI_REG_WRITE(0x00000000, NI_HV_GLB_INTF_RST_CONFIG);
#endif
    printk("%s: reset NI done!\n", __func__);

}

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

static void aal_ni_init_tx_dma_lso(void)
{
    int i;
    DMA_SEC_DMA_GLB_DMA_LSO_CTRL_t    dma_lso_ctrl;
    DMA_SEC_DMA_LSO_VP_CONTROL_t    vp_control;
    //DMA_SEC_AXIM2_TIMEOUT_THRESHOLD_t    axim2_timeout_threshold;
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

#if 0
    /* set up AXIM2 timeout threshold */
    axim2_timeout_threshold.wrd = 0;
    axim2_timeout_threshold.bf.read_channel_threshold = 1;
    axim2_timeout_threshold.bf.write_channel_threshold = 1;
    CA_DMA_LSO_REG_WRITE(axim2_timeout_threshold.wrd, DMA_SEC_AXIM2_TIMEOUT_THRESHOLD);
#endif

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

#if 0
static int aal_ni_set_xram_cfg(xram_inst_t xram_inst, xram_direction_t xram_direction)
{
    NI_XRAM_CPUXRAM_CFG_t cpuxram_cfg;

    cpuxram_cfg.wrd = CA_NI_REG_READ(NI_XRAM_CPUXRAM_CFG);

    switch (xram_direction) {

        case XRAM_DIRECTION_TX:
            cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 0;            /* 0: enable */
            CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_XRAM_CPUXRAM_CFG);
            break;

        case XRAM_DIRECTION_RX:
            cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 0;             /* 0: enable */
            CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_XRAM_CPUXRAM_CFG);
            break;
    }
    return 0;
}
#endif

#if 0
static void aal_ni_set_voq_map(ca_uint16_t voq_number, ca_uint8_t voq_did, ca_uint8_t disable_crc)
{
    //TODO: G3 has no NI_TX_VOQ_LKUP_ACCESS_t??
    NI_TX_VOQ_LKUP_ACCESS_t voq_lkup_access, voq_lkup_access_mask;
    NI_TX_VOQ_LKUP_DATA0_t voq_lkup_data0, voq_lkup_data0_mask;
    ca_uint8_t access;

    /* Prepare data */
    voq_lkup_data0.wrd = 0;
    voq_lkup_data0_mask.wrd = 0;
    voq_lkup_data0.bf.txem_voq_did = voq_did;
    voq_lkup_data0.bf.txem_discrc = disable_crc;
    voq_lkup_data0.bf.txem_crcstate = 0;
    voq_lkup_data0_mask.bf.txem_voq_did = 0xF;
    voq_lkup_data0_mask.bf.txem_discrc = 1;
    voq_lkup_data0_mask.bf.txem_crcstate = 0x7FFFFFF;
    //write_reg(voq_lkup_data0.wrd, voq_lkup_data0_mask.wrd, NI_TOP_NI_TX_VOQ_LKUP_DATA0);
    //CA_REG_WRITE(0, NI_TOP_NI_TX_VOQ_LKUP_DATA1);

    voq_lkup_access.wrd = 0;
    voq_lkup_access_mask.wrd = 0;
    voq_lkup_access.bf.access = 1;
    /* access address is the same as the voq_number, range from 0 to 127 */
    voq_lkup_access.bf.address = voq_number;
    voq_lkup_access.bf.debug_mode = NORMAL_MODE; /* NORMAL_MODE = 0 */
    voq_lkup_access.bf.rbw = 1; /* 0:read, 1:write */

    voq_lkup_access_mask.bf.access = 1;
    voq_lkup_access_mask.bf.address = 0x7F;
    voq_lkup_access_mask.bf.debug_mode = 1;
    voq_lkup_access_mask.bf.rbw = 1;
    //write_reg(voq_lkup_access.wrd, voq_lkup_access_mask.wrd, NI_TOP_NI_TX_VOQ_LKUP_ACCESS);
    do {
        //access = (CA_NI_REG_READ(NI_TOP_NI_TX_VOQ_LKUP_ACCESS) & 0x80000000);
        udelay(1);
    } while (access == 0x80000000);
}
#endif

#if 0
static void aal_ni_voq_map_cfg(void)
{
    int i;

#ifdef CS752X_MANAGEMENT_MODE
    /* for management port only */
    /* GE1: 0x0001 */
    for (i = 0; i <= 7; i++)
        aal_ni_set_voq_map(i, NI_VOQ_DID_GE1, 0);
#else
    //TODO: the VoQ has been changed???
    for (i = 0; i <= 7; i++) {
        /* GE0, Voq: 0 ~ 7 */
        aal_ni_set_voq_map(i + GE0_PORT_VOQ_BASE, NI_VOQ_DID_GE0, 1);

        /* GE1, Voq: 8 ~ 15 */
        aal_ni_set_voq_map(i + GE1_PORT_VOQ_BASE, NI_VOQ_DID_GE1, 1);

        /* GE2, Voq: 16 ~ 23 */
        aal_ni_set_voq_map(i + GE2_PORT_VOQ_BASE, NI_VOQ_DID_GE2, 1);

        /* GE4, Voq: 24 ~ 31 */
        aal_ni_set_voq_map(i + GE3_PORT_VOQ_BASE, NI_VOQ_DID_GE2, 1);

        /* WAN, Voq: 32 ~ 39 */
        /* Encap and Crypto Core, Voq: 24 ~ 31 & 32 ~ 39 */
        aal_ni_set_voq_map(i + WAN_PORT_VOQ_BASE, NI_VOQ_DID_WAN, 1);

        /* for Multicast: 40 ~ 47 */
        aal_ni_set_voq_map(i + MC_PORT_VOQ_BASE, NI_VOQ_DID_MC, 1);

        /* for CPU 0 Voq: 48 ~ 55 */
        aal_ni_set_voq_map(i + CPU_PORT0_VOQ_BASE, NI_VOQ_DID_CPU0, 1);

        /* for CPU 1 Voq: 56 ~ 63 */
        aal_ni_set_voq_map(i + CPU_PORT1_VOQ_BASE, NI_VOQ_DID_CPU1, 1);

        /* for CPU 2 Voq: 64 ~ 71 */
        aal_ni_set_voq_map(i + CPU_PORT2_VOQ_BASE, NI_VOQ_DID_CPU2, 1);

        /* for CPU 3 Voq: 72 ~ 79 */
        aal_ni_set_voq_map(i + CPU_PORT3_VOQ_BASE, NI_VOQ_DID_CPU3, 1);

        /* for CPU 4 Voq: 80 ~ 87 */
        aal_ni_set_voq_map(i + CPU_PORT4_VOQ_BASE, NI_VOQ_DID_CPU4, 1);

        /* for CPU 5 Voq: 88 ~ 95 */
        aal_ni_set_voq_map(i + CPU_PORT5_VOQ_BASE, NI_VOQ_DID_CPU5, 1);

        /* for CPU 6 Voq: 96 ~ 103 */
        aal_ni_set_voq_map(i + CPU_PORT6_VOQ_BASE, NI_VOQ_DID_CPU6, 1);

        /* for CPU 7 Voq: 104 ~ 111 */
        aal_ni_set_voq_map(i + CPU_PORT7_VOQ_BASE, NI_VOQ_DID_CPU7, 1);
    }
#endif
}
#endif

static void aal_ni_tx_rx_reset(void)
{
/* remove according to new registers.h */
#if 0
    int i;
    NI_HV_PT_RXMAC_CFG_t rxmac_cfg;
    NI_HV_PT_TXMAC_CFG_t txmac_cfg;

    for (i = 0; i < CA_NI_NUM_OF_PORT; i++) {
        rxmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_RXMAC_CFG + APB0_NI_HV_PT_STRIDE * i);
        rxmac_cfg.bf.mac_rx_rst = 1;
        CA_NI_REG_WRITE(rxmac_cfg.wrd, NI_HV_PT_RXMAC_CFG + APB0_NI_HV_PT_STRIDE * i);

        txmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_TXMAC_CFG + APB0_NI_HV_PT_STRIDE * i);
        txmac_cfg.bf.mac_tx_rst = 1;
        CA_NI_REG_WRITE(txmac_cfg.wrd, NI_HV_PT_TXMAC_CFG + APB0_NI_HV_PT_STRIDE * i);

        rxmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_RXMAC_CFG + APB0_NI_HV_PT_STRIDE * i);
                rxmac_cfg.bf.mac_rx_rst = 0;
                CA_NI_REG_WRITE(rxmac_cfg.wrd, NI_HV_PT_RXMAC_CFG + APB0_NI_HV_PT_STRIDE * i);

                txmac_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_TXMAC_CFG + APB0_NI_HV_PT_STRIDE * i);
                txmac_cfg.bf.mac_tx_rst = 0;
                CA_NI_REG_WRITE(txmac_cfg.wrd, NI_HV_PT_TXMAC_CFG + APB0_NI_HV_PT_STRIDE * i);
    }
#endif
}

static void aal_ni_cpu_port_runt_drop_disable(void)
{
    //NI_HV_PT_RX_CNTRL_CFG_t rx_cntrl_cfg;

    ///TODO:: CA_NI_PPORT_CPU=9???
    //rx_cntrl_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_RX_CNTRL_CFG + APB0_NI_HV_PT_STRIDE * CA_NI_PPORT_CPU);
    //rx_cntrl_cfg.bf.runt_drop_dis = 1;
    //CA_NI_REG_WRITE(rx_cntrl_cfg.wrd, NI_HV_PT_RX_CNTRL_CFG + APB0_NI_HV_PT_STRIDE * CA_NI_PPORT_CPU);
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
    //static_cfg.bf.mgmt_pt_to_fe_also = 1;
    static_cfg.bf.txmib_mode = 1;
    static_cfg.bf.rxmib_mode = 1;
    CA_NI_REG_WRITE(static_cfg.wrd, NI_HV_GLB_STATIC_CFG);
}

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

ca_status_t aal_ni_hv_glb_internal_port_id_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_ni_hv_glb_internal_port_id_cfg_t *cfg
)
{
    //int i;
    NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;
    port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
    cfg->dma_to_l3fe = (ca_uint8_t) port_id_cfg.bf.dma_to_l3fe;
#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
    cfg->wan_rxsel = (ca_uint8_t) port_id_cfg.bf.wan_rxsel;
#endif
    cfg->l3qmrx_demux_sel = (ca_uint8_t) port_id_cfg.bf.l3qmrx_demux_sel;
    return CA_E_OK;
}
EXPORT_SYMBOL(aal_ni_hv_glb_internal_port_id_cfg_get);

ca_status_t aal_ni_hv_glb_internal_port_id_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_ni_hv_glb_internal_port_id_cfg_mask_t mask,
    CA_IN aal_ni_hv_glb_internal_port_id_cfg_t *cfg
)
{
    //int i;
    NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;
    port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);

    if (mask.bf.dma_to_l3fe) {
        port_id_cfg.bf.dma_to_l3fe = cfg->dma_to_l3fe;
    }

#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
    if (mask.bf.wan_rxsel) {
        port_id_cfg.bf.wan_rxsel = cfg->wan_rxsel;
    }
#endif

    if (mask.bf.l3qmrx_demux_sel) {
        port_id_cfg.bf.l3qmrx_demux_sel = cfg->l3qmrx_demux_sel;
    }

    CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
    return CA_E_OK;
}
EXPORT_SYMBOL(aal_ni_hv_glb_internal_port_id_cfg_set);

void aal_ni_init_nirx_l3fe_demux(void)
{
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_status_t ret;
    NI_HV_GLB_NIRX_L3FE_DEMUX_CFG2_t    l3fe_demux_cfg2;
    NI_HV_GLB_NIRX_L3FE_DEMUX_CFG3_t    l3fe_demux_cfg3;
    NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG2_t        l3fe_dpq_demux_cfg2;
    NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG3_t        l3fe_dpq_demux_cfg3;
#endif
    NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0_t    l3fe_demux_cfg0;
    NI_HV_GLB_NIRX_L3FE_DEMUX_CFG1_t    l3fe_demux_cfg1;
    NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0_t        l3fe_dpq_demux_cfg0;
    NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG1_t        l3fe_dpq_demux_cfg1;
    ca_ni_rx_l3fe_demux_t        demux;

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_uint8_t l3fe_to_wan_port;

    ret = aal_scfg_read(CFG_ID_L3FE_TO_WAN_PORT, 1, &l3fe_to_wan_port);
        if (ret != CA_E_OK) {
                printk("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT, ret=%d\n", __func__, ret);
        l3fe_to_wan_port = 1;
        }
#endif

    /* ldpid 0-15: L2FE */
    demux.wrd = 0;
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)
    demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L2FE;
#else
#ifdef CONFIG_ARCH_CORTINA_G3LITE
    demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
#else
    demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L2FE;
#endif
#endif
    demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
    demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
    demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
    demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
    demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
    demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
    demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
#if defined(CONFIG_ARCH_RTL8198F)
#ifdef CONFIG_ARCH_CORTINA_G3LITE	
    #if defined(CONFIG_RTK_WANPORT_AT_PORT4)
    demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
    #elif defined(CONFIG_RTK_WANPORT_AT_PORT1)
    demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
    #else
    demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
    #endif
#endif
#endif
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    /* determined by start up config to go to WAN port or L3QM  */
    if (l3fe_to_wan_port)
        demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_WAN;
    else
        demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
#else
    demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L2FE;
#endif

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

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
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
#endif

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

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    /* determined by start up config to go to WAN port or L3QM  */
    if (l3fe_to_wan_port)
        demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_WAN;
    else
        demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
#endif

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

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
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
#endif
}

static void aal_ni_init_dma_to_l3fe(void)
{
#if defined(CONFIG_ARCH_CORTINA_G3)  || defined(CONFIG_ARCH_CORTINA_G3HGU)
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
#endif
}

void aal_ni_init_ni(int port_id)
{
#if defined(CONFIG_ARCH_CORTINA_G3)  || defined(CONFIG_ARCH_CORTINA_G3HGU)
    NI_HV_GLB_NIRX_MISC_CFG_t nirx_misc;
#ifndef CONFIG_CORTINA_BOARD_FPGA
    NI_HV_GLB_XGE_BYPASS_CFG_t xge_bypass_cfg;
#endif
#endif
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    ca_uint8_t upstream_use_dq = 0;
#endif
	ca_uint32_t tmp_data;
	NI_HV_GLB_RXFIFO_THRESHOLD_MISC_CFG_t rxfifo_threshold;


    if (aal_scfg_read(CFG_ID_NE_CPU_CPU_PORT_COUNT, 1, &ca_ni_cpu_cpu_port_count) == CA_E_OK) {
        if (!ca_ni_cpu_cpu_port_count || ca_ni_cpu_cpu_port_count > CA_NI_TOTAL_CPU_PORT) {
            ca_ni_cpu_cpu_port_count = CA_NI_TOTAL_CPU_PORT - 2;
            printk("%s: CFG_ID_NE_CPU_CPU_PORT_COUNT=%d too large!!\n", __func__, ca_ni_cpu_cpu_port_count);
        }
    }

    aal_ni_reset();

    aal_ni_tx_rx_reset();

    /* Initial TX DMA LSO */
    //aal_ni_init_tx_dma_lso();

    /* Allow NI send small size packet from CPU port */
    aal_ni_cpu_port_runt_drop_disable();

    /* set packet length globally */
    aal_ni_set_pkt_len();

    /* set the packet direction from DMA LSO to L2FE ot L3FE */
    aal_ni_init_dma_to_l3fe();

    /* set management port if NI in management mode */
#if defined(CONFIG_NE_MANAGEMENT_MODE)
        aal_ni_mgmt_init_xram_mem();
        aal_ni_mgmt_enable_tx_rx();

        printk("%s: port_id=%d\n", __func__, port_id);
        aal_ni_set_port_to_cpu(port_id);   /* only for debugging */

        ///TODO: should adjust the port interface configure according to PHY link status
        //aal_ni_set_eth_cfg(port_id, 3, 0);
#else
            /* Initial TX DMA LSO */
        aal_ni_init_tx_dma_lso();

        aal_ni_set_port_to_cpu(0xff);   /* only for debugging */

        aal_ni_init_nirx_l3fe_demux();
#endif

    printk("%s: ======= NI_HV_GLB_INTERNAL_PORT_ID_CFG=0xef ==============\n", __func__);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    scfg_read(0, CFG_ID_PON_UPSTREAM_USE_DEEP_QUEUE,
    sizeof(ca_uint8_t), (ca_uint8_t *)&upstream_use_dq);

    if(upstream_use_dq == 1){
            CA_NI_REG_WRITE(0x00000000, NI_HV_GLB_INTERNAL_PORT_ID_CFG); /* let WAN to L3FE , QM->WAN*/
    }else{

            CA_NI_REG_WRITE(0x0000007e, NI_HV_GLB_INTERNAL_PORT_ID_CFG); /* let WAN to L3FE , QM->WAN*/
    }
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)
{
    NI_HV_GLB_INTERNAL_PORT_ID_CFG_t port_id_cfg;

    printk("%s: ======= NI_HV_GLB_INTERNAL_PORT_ID_CFG=0x7f ==============\n", __func__);

    /* L3QM to L2TM for port 0-6 and to WAN for port 7 */
    port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
    port_id_cfg.bf.l3qmrx_demux_sel = 0x7f;
    CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);

    port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
    printk("%s: NI_HV_GLB_INTERNAL_PORT_ID_CFG = %#x\n", __func__, port_id_cfg.wrd);
    CA_8279_NE_REG_WRITE(0x008000EF, NI_HV_GLB_INTERNAL_PORT_ID_CFG); /* let SATURN WAN to NIRXMUX */
}
#else
    {
        NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;

        /* L3QM to L2TM for port 0-6 and to WAN for port 7 */
        port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
        port_id_cfg.bf.l3qmrx_demux_sel = 0x7f; /* according to bug 50073*/
        CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
    }
#endif
#if !defined(CONFIG_ARCH_CORTINA_G3LITE)
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
#endif
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    nirx_misc.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_MISC_CFG);
#ifdef CONFIG_CORTINA_BOARD_FPGA
    printk("%s: ======= NI_HV_GLB_NIRX_MISC_CFG, use_p0_as_wan_port_en=1 for test ======\n", __func__);
    nirx_misc.bf.use_p0_as_wan_port_en = 1;
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

#ifndef CONFIG_CORTINA_BOARD_FPGA
        xge_bypass_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_XGE_BYPASS_CFG);
        printk("%s: ======= NI_HV_GLB_XGE_BYPASS_CFG, p7_byp_dpid=%d for ASIC ======\n", __func__, wan_port_id);
        xge_bypass_cfg.bf.p7_byp_dpid = wan_port_id;
        CA_NI_REG_WRITE(xge_bypass_cfg.wrd, NI_HV_GLB_XGE_BYPASS_CFG);
#endif
#endif

	/* workaround to swap the hi/lo thrshold for l3fe/l3qm */
	rxfifo_threshold.wrd = CA_NI_REG_READ(NI_HV_GLB_RXFIFO_THRESHOLD_MISC_CFG);
	tmp_data = rxfifo_threshold.bf.l3qm_rxfifo_hi;
	rxfifo_threshold.bf.l3qm_rxfifo_hi = rxfifo_threshold.bf.l3qm_rxfifo_lo;
	rxfifo_threshold.bf.l3qm_rxfifo_lo = tmp_data;
	tmp_data = rxfifo_threshold.bf.l3fe_rxfifo_hi;
	rxfifo_threshold.bf.l3fe_rxfifo_hi = rxfifo_threshold.bf.l3fe_rxfifo_lo;
	rxfifo_threshold.bf.l3fe_rxfifo_lo = tmp_data;
	CA_NI_REG_WRITE(rxfifo_threshold.wrd, NI_HV_GLB_RXFIFO_THRESHOLD_MISC_CFG);
}

static void aal_ni_global_interrupt(void)
{
#if 0
    GLOBAL_NETWORK_ENGINE_INTENABLE_0_t global_interrupt;

    /* enable golable NI, XRAM interrupt */
    global_interrupt.wrd = CA_GLB_REG_READ(GLOBAL_NETWORK_ENGINE_INTENABLE_0);

    /* enable TQM and NI interrupts */
    global_interrupt.bf.TQMe = 1;
    global_interrupt.bf.NIe = 1;

    CA_GLB_REG_WRITE(global_interrupt.wrd, GLOBAL_NETWORK_ENGINE_INTENABLE_0);
#endif
}

static void aal_ni_enable_interrupts(void)
{
#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
    NI_HV_GLB_PORT_0_INTERRUPTE_t   port_0_inte;
#endif
    NI_HV_GLB_PORT_8_INTERRUPTE_t   port_8_inte;
    NI_HV_GLB_INTERRUPTE_t       inte;
    NI_HV_GLB_TXEM_INTERRUPTE_t       txem_inte;

#if defined(CONFIG_NE_MANAGEMENT_MODE)
  #ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
        /* Enable NI interrupt for WAN port */
        port_0_inte.wrd = 0;
        port_0_inte.bf.link_stat_chgIE             = 1;
        port_0_inte.bf.txfifo_underrunIE         = 1;
        port_0_inte.bf.txfifo_overrunIE         = 1;
        port_0_inte.bf.rxcntrl_overrunIE         = 1;
        port_0_inte.bf.rxmib_cntmsb_setIE         = 1;
        port_0_inte.bf.txmib_cntmsb_setIE         = 1;
        //port_0_inte.bf.txem_crcerr_cntmsb_setIE     = 1;
        port_0_inte.bf.lpi_assert_stat_chgIE         = 1;
        port_0_inte.bf.rx_lpi_stat_chgIE         = 1;
        //port_0_inte.bf.rxfifo_pktdrop_cntmsb_setIE     = 1;
        port_0_inte.bf.ni_wol_intE             = 1;
        //CA_NI_REG_WRITE(port_0_inte.wrd, NI_HV_GLB_PORT_0_INTERRUPTE + (CA_NI_MGMT_PORT * NI_HV_GLB_PORT_0_INTERRUPTE_STRIDE));
  #endif
#else
        /* Enable NI interrupt for CPU port */
        port_8_inte.wrd = 0;
        port_8_inte.bf.rxcntrl_overrunIE    = 1;
        port_8_inte.bf.rxmib_cntmsb_setIE    = 1;
        port_8_inte.bf.txmib_cntmsb_setIE    = 1;
        //port_8_inte.bf.txem_crcerr_cntmsb_setIE    = 1;
        //port_8_inte.bf.rxfifo_pktdrop_cntmsb_setIE    = 1;
        CA_NI_REG_WRITE(port_8_inte.wrd, NI_HV_GLB_PORT_8_INTERRUPTE + (CA_NI_PPORT_CPU * NI_HV_GLB_PORT_8_INTERRUPTE_STRIDE));
#endif
    /* Enable NI global interrupts */
    inte.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERRUPTE);
#if 0
    inte.bf.int_stat_pspid_0E = 1;
    inte.bf.int_stat_pspid_1E = 1;
    inte.bf.int_stat_pspid_2E = 1;
    inte.bf.int_stat_pspid_3E = 1;
    inte.bf.int_stat_pspid_4E = 1;
    inte.bf.int_stat_pspid_5E = 1;
    inte.bf.int_stat_pspid_6E = 1;
    inte.bf.int_stat_pspid_7E = 1;
    inte.bf.int_stat_pspid_8E = 1;
    inte.bf.int_stat_pspid_9E = 1;
    inte.bf.int_stat_pspid_10E = 1;
    inte.bf.int_stat_txemE = 1;
#endif
    inte.bf.cpuxram_rxpktE = 1;
    inte.bf.cpuxram_txpktE = 1;
    CA_NI_REG_WRITE(inte.wrd, NI_HV_GLB_INTERRUPTE);

    /* Enable NI TXEM interrupt */
    txem_inte.wrd = CA_NI_REG_READ(NI_HV_GLB_TXEM_INTERRUPTE);
    txem_inte.bf.txem_ififo_ovfIE     = 1;
    txem_inte.bf.txrm_cntovfIE     = 1;
    txem_inte.bf.txmib_fifo_ovfIE     = 1;
    txem_inte.bf.rxmib_fifo_ovfIE    = 1;
    txem_inte.bf.txrm_timerexpIE    = 1;
    CA_NI_REG_WRITE(txem_inte.wrd, NI_HV_GLB_TXEM_INTERRUPTE);
}

static void aal_ni_enable_dma_lso_interrupts(void)
{
    int i;
    DMA_SEC_DMA_INT_DMA_LSO_INTEN0_t dma_lso_inten;
    DMA_SEC_DMA_LSO_VP_VP_INTENABLE_t vp_inten;
    DMA_SEC_DMA_LSO_VP_MISC_INTENABLE_t vp_misc_inten;
    DMA_SEC_DMA_LSO_VP_COAL_INTENABLE_t vp_coal_inten;

    dma_lso_inten.wrd = 0;
    dma_lso_inten.bf.VP0_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP1_INTERRUPTi_en = 1;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    dma_lso_inten.bf.VP2_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP3_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP4_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP5_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP6_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP7_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP8_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP9_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP10_INTERRUPTi_en = 1;
    dma_lso_inten.bf.VP11_INTERRUPTi_en = 1;
#endif
    dma_lso_inten.bf.LSO_INTERRUPTi_en = 1;
    dma_lso_inten.bf.SS_INTERRUPTi_en = 1;

    for (i = 0; i < CA_NI_DMA_LSO_RXQ_NUM; i++) {
        CA_DMA_LSO_REG_WRITE(dma_lso_inten.wrd, DMA_SEC_DMA_INT_DMA_LSO_INTEN0 + (i * 16));
    }

    vp_inten.wrd = 0;
    vp_inten.bf.MISC_INTERRUPTi_en = 1;
    vp_inten.bf.COAL_INTERRUPTi_en = 1;
    for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
        CA_DMA_LSO_REG_WRITE(vp_inten.wrd, DMA_SEC_DMA_LSO_VP_VP_INTERRUPT + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
    }

    vp_misc_inten.wrd = 0;
    vp_misc_inten.bf.txq0_empty_en         = 1;
    vp_misc_inten.bf.txq0_pktcnt_overrun_en = 1;
    vp_misc_inten.bf.txq0_des_err_en     = 1;
    vp_misc_inten.bf.txq1_empty_en         = 1;
    vp_misc_inten.bf.txq1_pktcnt_overrun_en = 1;
    vp_misc_inten.bf.txq1_des_err_en    = 1;
    vp_misc_inten.bf.txq2_empty_en          = 1;
    vp_misc_inten.bf.txq2_pktcnt_overrun_en = 1;
    vp_misc_inten.bf.txq2_des_err_en        = 1;
    vp_misc_inten.bf.txq3_empty_en          = 1;
    vp_misc_inten.bf.txq3_pktcnt_overrun_en = 1;
    vp_misc_inten.bf.txq3_des_err_en        = 1;
    vp_misc_inten.bf.txq4_empty_en          = 1;
    vp_misc_inten.bf.txq4_pktcnt_overrun_en = 1;
    vp_misc_inten.bf.txq4_des_err_en        = 1;
    vp_misc_inten.bf.txq5_empty_en          = 1;
    vp_misc_inten.bf.txq5_pktcnt_overrun_en = 1;
    vp_misc_inten.bf.txq5_des_err_en        = 1;
    vp_misc_inten.bf.txq6_empty_en          = 1;
    vp_misc_inten.bf.txq6_pktcnt_overrun_en = 1;
    vp_misc_inten.bf.txq6_des_err_en        = 1;
    vp_misc_inten.bf.txq7_empty_en          = 1;
    vp_misc_inten.bf.txq7_pktcnt_overrun_en = 1;
    vp_misc_inten.bf.txq7_des_err_en        = 1;
    vp_misc_inten.bf.fbm_timeout_err_en    = 1;
    for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
        CA_DMA_LSO_REG_WRITE(vp_misc_inten.wrd, DMA_SEC_DMA_LSO_VP_MISC_INTENABLE + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
    }

    vp_coal_inten.wrd = 0;
    vp_coal_inten.bf.txq_eof_en = 1;
    for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
        CA_DMA_LSO_REG_WRITE(vp_coal_inten.wrd, DMA_SEC_DMA_LSO_VP_COAL_INTENABLE + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
    }
}

void aal_ni_init_interrupt_cfg(void)
{

    /* enable golable NI, XRAM interrupt */
    aal_ni_global_interrupt();

    /* Enable NI interrupts */
    aal_ni_enable_interrupts();

#if !defined(CONFIG_NE_MANAGEMENT_MODE)
        /* L3QM enable interrupt source */
        aal_l3qm_enable_int_src();

        /* Enable DMA LSO interrupt */
        aal_ni_enable_dma_lso_interrupts();
#endif
}

void aal_ni_get_dma_lso_base_depth_addr(ca_uint8_t instance, ca_uint8_t txq_id)
{
        int i;
        //DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0_t       txq_base_depth_data0;
        //DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1_t       txq_base_depth_data1;
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
	//ca_uint32_t rd_ptr, wr_ptr;

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

