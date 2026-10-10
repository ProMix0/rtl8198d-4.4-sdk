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
#include <generated/ca_ne_autoconf.h>
#include <soc/cortina/registers.h>
#include <ca_types.h>
#include <aal_common.h>
#include <aal_fbm.h>
#include "ca_mut.h"

ca_uint8_t *fbm_exstack_vbase;
ca_uint8_t *fbm_exstack_pbase;

#define FBM_POOL_EXSTACK_DEPTH_PER_POOL	0x1000		/* the default value of FBM_POOL_EXSTACK_DEPTH is 0x1000 (4K entries)  */
#define FBM_POOL_EXSTACK_BAR_ALIGN	0x1000		/* the external stack base address is 4K aligned */
#define FBM_EXSTACK_BAR_GET(x)	(long)(fbm_exstack_pbase + (FBM_POOL_EXSTACK_DEPTH_PER_POOL * sizeof(ca_uint64_t)) * x)

ca_uint32_t aal_fbm_init()
{
	FBM_GLOBAL_GLB_CTRL_t glb_ctrl;
	FBM_GLOBAL_AXI_CTRL_t axi_ctrl;
	ca_uint32_t size;

	/* Alloc stack pool in DDR, the start addreess must be in 4K aligned. */
	size = FBM_POOL_EXSTACK_DEPTH_PER_POOL * sizeof(ca_uint64_t) * APB0_FBM_CPU_COUNT + FBM_POOL_EXSTACK_BAR_ALIGN;
	fbm_exstack_vbase = ca_malloc_trace(size, GFP_KERNEL, true);
	if (fbm_exstack_vbase == NULL) {
		printk(KERN_ERR "fbm_exstack_vbase allocate failed!!\n");
		return CA_E_MEMORY;
	}
	fbm_exstack_pbase = (ca_uint8_t *)virt_to_phys(fbm_exstack_vbase);
	if ((long)fbm_exstack_pbase & 0xFFF) {
		printk("%s: fbm_exstack_pbase=%p is not 4KB aligned!!\n", __func__, fbm_exstack_pbase);
		fbm_exstack_pbase = (ca_uint8_t *)((long)(fbm_exstack_pbase + FBM_POOL_EXSTACK_BAR_ALIGN) & ~0xFFF);
		printk("%s: fbm_exstack_pbase=%p after 4KB aligned!!\n", __func__, fbm_exstack_pbase);
	}

	/*Enable LPAE*/
	glb_ctrl.wrd = CA_NE_REG_READ(FBM_GLOBAL_GLB_CTRL);
	glb_ctrl.bf.lpae = 1;
	CA_NE_REG_WRITE(glb_ctrl.wrd, FBM_GLOBAL_GLB_CTRL);

	/* AXI user pattern 0 are all 0 and pattern 3 is not used.
	 * The default value of pattern 1,2 are already modified to MakeInvalid and WriteLineUnique.
	 */
	CA_NE_REG_WRITE(FBM_GLOBAL_AXI_USR_PAT1_dft, FBM_GLOBAL_AXI_USR_PAT1);
	CA_NE_REG_WRITE(FBM_GLOBAL_AXI_USR_PAT2_dft, FBM_GLOBAL_AXI_USR_PAT2);

	axi_ctrl.wrd = FBM_GLOBAL_AXI_CTRL_dft;
	axi_ctrl.bf.minv_usr_pat = 1;
	axi_ctrl.bf.mset_usr_pat = 2;
	axi_ctrl.bf.ace_cmd_en = 1;
	CA_NE_REG_WRITE(axi_ctrl.wrd, FBM_GLOBAL_AXI_CTRL);

	return CA_E_OK;
}

ca_uint32_t aal_fbm_pool_init(aal_fbm_pool_id_t id, struct aal_fbm_pool_config_s *pool_config)
{
#if FBM_RELEASE_MODE
	ca_uint32_t i;
#endif
	APB0_FBM_POOL_t pool_regs;
	FBM_GLOBAL_GLB_CTRL_t glb_ctrl;

	/* Check pool config */
	if (pool_config == NULL)
		return CA_E_ERROR;

	if (pool_config->cache_invalidate || pool_config->memset_mode)
		if (pool_config->mset_cainv_size == 0 ||
			pool_config->mset_cainv_size > FBM_MAX_MSET_SIZE ||
			pool_config->mset_cainv_size % FBM_CNT_BURST_SIZE != 0)
			return CA_E_ERROR;

#if FBM_RELEASE_MODE
	for (i = 0; i < FBM_MAX_SEG_NUM; i++) {
		if (pool_config->seg[i].seg_enable) {
			if (pool_config->seg[i].seg_bar & 0xFFF) //should be 4KB aligned
				return CA_E_ERROR;
		}
	}
#endif

	/* Fill per pool registers */
	pool_regs.POOL_CONTROL.wrd = FBM_POOL_POOL_CONTROL_dft;
	pool_regs.POOL_CONTROL.bf.pool_mset = pool_config->memset_mode;
	pool_regs.POOL_CONTROL.bf.pool_cainv = pool_config->cache_invalidate;
	pool_regs.POOL_CONTROL.bf.ins_lo_thresh = FBM_STACK_LOW_THRESHOLD;
	pool_regs.POOL_CONTROL.bf.ins_hi_thresh = FBM_STACK_HIGH_THRESHOLD;

	/* allow read FBM_POOL_DEBUG_OUT_STND_CNTR */
        pool_regs.POOL_CONTROL.bf.pool_oscntr_dbg = 1;
        pool_regs.POOL_CONTROL.bf.pool_oscntr_en = 1;

	pool_regs.EXSTACK_BAR.wrd = FBM_POOL_EXSTACK_BAR_dft;
	pool_regs.EXSTACK_BAR.bf.exs_bar = FBM_EXSTACK_BAR_GET(id) >> 12;

	pool_regs.BUFFER_SIZE.wrd = FBM_POOL_BUFFER_SIZE_dft;
	pool_regs.BUFFER_SIZE.bf.buff_size = (pool_config->mset_cainv_size - 1) >> 6;

	CA_NE_REG_WRITE(pool_regs.POOL_CONTROL.wrd, FBM_POOL_POOL_CONTROL + APB0_FBM_POOL_STRIDE * id);
	CA_NE_REG_WRITE(pool_regs.EXSTACK_BAR.wrd, FBM_POOL_EXSTACK_BAR + APB0_FBM_POOL_STRIDE * id);
	CA_NE_REG_WRITE(pool_regs.BUFFER_SIZE.wrd, FBM_POOL_BUFFER_SIZE + APB0_FBM_POOL_STRIDE * id);

#if FBM_RELEASE_MODE
	pool_regs.SEG_CONTROL.wrd = FBM_POOL_SEG_CONTROL_dft;
	if (pool_config->seg[0].seg_enable) {
		pool_regs.SEG_CONTROL.bf.se0_enable = TRUE;
		pool_regs.SEG_CONTROL.bf.se0_size = pool_config->seg[0].seg_size_mode;
		pool_regs.SE0_BAR.wrd = FBM_POOL_SE0_BAR_dft;
		pool_regs.SE0_BAR.bf.se0_bar = pool_config->seg[0].seg_bar >> 12;
	}
	if (pool_config->seg[1].seg_enable) {
		pool_regs.SEG_CONTROL.bf.se1_enable = TRUE;
		pool_regs.SEG_CONTROL.bf.se1_size = pool_config->seg[1].seg_size_mode;
		pool_regs.SE1_BAR.wrd = FBM_POOL_SE1_BAR_dft;
		pool_regs.SE1_BAR.bf.se1_bar = pool_config->seg[1].seg_bar >> 12;
	}
	if (pool_config->seg[2].seg_enable) {
		pool_regs.SEG_CONTROL.bf.se2_enable = TRUE;
		pool_regs.SEG_CONTROL.bf.se2_size = pool_config->seg[2].seg_size_mode;
		pool_regs.SE2_BAR.wrd = FBM_POOL_SE2_BAR_dft;
		pool_regs.SE2_BAR.bf.se2_bar = pool_config->seg[2].seg_bar >> 12;
	}
	if (pool_config->seg[3].seg_enable) {
		pool_regs.SEG_CONTROL.bf.se3_enable = TRUE;
		pool_regs.SEG_CONTROL.bf.se3_size = pool_config->seg[3].seg_size_mode;
		pool_regs.SE3_BAR.wrd = FBM_POOL_SE3_BAR_dft;
		pool_regs.SE3_BAR.bf.se3_bar = pool_config->seg[3].seg_bar >> 12;
	}
	CA_NE_REG_WRITE(pool_regs.SEG_CONTROL.wrd, POOL_SEG_CONTROL + APB0_FBM_POOL_STRIDE * id);
	CA_NE_REG_WRITE(pool_regs.SE0_BAR.wrd, POOL_SE0_BAR + APB0_FBM_POOL_STRIDE * id);
	CA_NE_REG_WRITE(pool_regs.SE1_BAR.wrd, POOL_SE1_BAR + APB0_FBM_POOL_STRIDE * id);
	CA_NE_REG_WRITE(pool_regs.SE2_BAR.wrd, POOL_SE2_BAR + APB0_FBM_POOL_STRIDE * id);
	CA_NE_REG_WRITE(pool_regs.SE3_BAR.wrd, POOL_SE3_BAR + APB0_FBM_POOL_STRIDE * id);
#endif

	/* Set FBM_POOL_EXSTACK_DEPTH to be 4K entries */
	CA_NE_REG_WRITE(FBM_POOL_EXSTACK_DEPTH_PER_POOL, FBM_POOL_EXSTACK_DEPTH + APB0_FBM_POOL_STRIDE * id);

	/* Enable pool */
	glb_ctrl.wrd = CA_NE_REG_READ(FBM_GLOBAL_GLB_CTRL);
	glb_ctrl.bf.pool_enable |= 1 << id;
	CA_NE_REG_WRITE(glb_ctrl.wrd, FBM_GLOBAL_GLB_CTRL);

	return CA_E_OK;
}

ca_uint32_t aal_fbm_buf_push(aal_fbm_cpu_id_t cpu, aal_fbm_pool_id_t id, ca_uint32_t addr)
{
	APB0_FBM_CPU_t cpu_regs;

	/* do sanity check */
	if (aal_fbm_get_out_stnd_count(cpu, id) >= FBM_POOL_EXSTACK_DEPTH_PER_POOL)
		return CA_E_ERROR;

	cpu_regs.SW_DATA1.wrd = 0;
	cpu_regs.SW_DATA0.bf.DATA = addr;

	cpu_regs.SW_ACCESS.wrd = 0;
	cpu_regs.SW_ACCESS.bf.POOLID = id;
	cpu_regs.SW_ACCESS.bf.rbw = 1; //write
	cpu_regs.SW_ACCESS.bf.access = 1;

	CA_NE_REG_WRITE(cpu_regs.SW_DATA1.wrd, FBM_CPU_SW_DATA1 + APB0_FBM_CPU_STRIDE * cpu);
	CA_NE_REG_WRITE(cpu_regs.SW_DATA0.wrd, FBM_CPU_SW_DATA0 + APB0_FBM_CPU_STRIDE * cpu);
	/* Polling mode is not enabled by default. */
	CA_NE_REG_WRITE(cpu_regs.SW_ACCESS.wrd, FBM_CPU_SW_ACCESS + APB0_FBM_CPU_STRIDE * cpu);

	return CA_E_OK;
}

ca_uint32_t aal_fbm_buf_pop(aal_fbm_cpu_id_t cpu, aal_fbm_pool_id_t id, ca_uint32_t *addr)
{
	APB0_FBM_CPU_t cpu_regs;

	cpu_regs.SW_ACCESS.wrd = 0;
	cpu_regs.SW_ACCESS.bf.POOLID = id;
	cpu_regs.SW_ACCESS.bf.rbw = 0; //read
	cpu_regs.SW_ACCESS.bf.access = 1;

	/* Polling mode is not enabled by default. */
	CA_NE_REG_WRITE(cpu_regs.SW_ACCESS.wrd, FBM_CPU_SW_ACCESS + APB0_FBM_CPU_STRIDE * cpu);
	*addr = CA_NE_REG_READ(FBM_CPU_SW_DATA0 + APB0_FBM_CPU_STRIDE * cpu);

	return CA_E_OK;
}

ca_uint32_t aal_fbm_get_out_stnd_count(aal_fbm_cpu_id_t cpu, aal_fbm_pool_id_t id)
{
        ca_uint32_t count;
        ca_uint32_t reg_off;

        reg_off = FBM_POOL_DEBUG_OUT_STND_CNTR + (APB0_FBM_POOL_STRIDE * id);
        count = CA_NE_REG_READ(reg_off);
        return count;
}
