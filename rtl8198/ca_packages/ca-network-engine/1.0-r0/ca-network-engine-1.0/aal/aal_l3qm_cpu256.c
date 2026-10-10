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
 * aal_l3qm.c: Hardware Abstraction Layer for Layer-3 Queue Manager to access hardware regsiters
 *
 */
#include <linux/sched.h>

#include "ca-scfg/scfg.h"
#include "osal_cmn.h"
#include "aal_common.h"
#include "aal_table.h"
#include "aal_fbm.h"
#include "aal_l3qm.h"
#include "ca_aal_proc.h"

#define CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE       16              /* CPU256 port 0-31 */
#define CA_L3QM_CPU_EPP_FIFO_SIZE_UINT          4               /* 4-FIFO entries */

/* QM_AXI_ATTRIBUTE for CPU 256 EPP */
ca_uint32_t     l3qm_axi_attrib_cpu256_epp_values[CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES_LEN] =
{
        0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010,
        0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010,
        0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010,
        0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010
};

ca_uint32_t     l3qm_cpu256_port_count = 256;                   /* CPU256 port count */
ca_uint8_t      l3qm_cpu256_voq_per_port = 1;                   /* CPU256 VoQ per port */
ca_uint8_t      l3qm_cpu256_epp_per_voq = 32;                   /* CPU256 EPP per VoQ */
ca_uint32_t     l3qm_cpu256_pool_bid_count = 1024;              /* CPU256 pool BID count */

ca_uint32_t     l3qm_epp_profile_cpu256_hi_total_epp_count = 0;    /* CPU256 total EPP count */

/* configuration parameters for CPU256 need to be got from start config */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_id = 2;                     /* EQ profile 2 */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_rule = 1;                   /* use pool 7 for header */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_use_fbm = 1;                /* use FBM pool */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool0_eq_id = 7;            /* QM_EQ_PROFILE4.eqid0=7 */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_buf_count = 1024;        /* 256 buffers for PE upstream shaper */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_bid_start = 512; /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_buf_sz = 2048;        /* pool 0 header buffer size (B) */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool0_location = 1;         /* SRAM (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool0_fbm_pool_id = 2;      /* in SRAM */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_physical_start_address = 0xffffffff;    /* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_eq_id = 8;            /* QM_EQ_PROFILE4.eqid1=9 */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_buf_count = 1024;        /* 256 buffers for PE upstream shaper */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_bid_start = 1536;   /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_buf_sz = 2048;        /* pool 1 buffer size(payload) */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_location = 0;         /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_fbm_pool_id = 0;      /* in DDR */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_physical_start_address = 0xffffffff;    /* pool1 physical start address */
ca_uint8_t      l3qm_eq_profile_cpu256_id_ports[32] = {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};    /* [0,1,....15] in L3QM, LPID[32,33....47] */
ca_uint8_t      l3qm_epp_profile_cpu256_hi_id = 3;                    /* EPP (RX FIFO) profile id */
ca_uint32_t     l3qm_epp_profile_cpu256_hi_sz = 0;                    /* RX FIFO size per entry, 4B */
ca_uint32_t     l3qm_epp_profile_cpu256_hi_cn = 0;                    /* 8 queues, 256 entries per queue */
ca_uint32_t     l3qm_epp_profile_cpu256_hi_start_addresses[8] = {0xffffffff};    /* VoQs 0-7 for QM_CPU_EPP_FIFO0_0_START to QM_CPU_EPP_FIFO15_7_START  */
ca_uint32_t     l3qm_epp_profile_cpu256_hi_map_mode = 0;              /* QM_CPU_EPP1_CFG map_mode=0 */

ca_uint8_t      l3qm_eq_profile_cpu256_lo_id = 3;                     /* EQ profile 3 */
ca_uint8_t      l3qm_eq_profile_cpu256_lo_rule = 1;                   /* use pool 7 for header */
ca_uint8_t      l3qm_eq_profile_cpu256_lo_use_fbm = 1;                /* use FBM pool */
ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool0_eq_id = 7;            /* QM_EQ_PROFILE4.eqid0=7 */
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_buf_count = 1024;        /* 256 buffers for PE upstream shaper */
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_bid_start = 2560; /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_buf_sz = 2048;        /* pool 0 header buffer size (B) */
ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool0_location = 1;         /* SRAM (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool0_fbm_pool_id = 2;      /* in SRAM */
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_physical_start_address = 0xffffffff;    /* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool1_eq_id = 8;            /* QM_EQ_PROFILE4.eqid1=9 */
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_buf_count = 1024;        /* 256 buffers for PE upstream shaper */
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_bid_start = 3584;   /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_buf_sz = 2048;        /* pool 1 buffer size(payload) */
ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool1_location = 0;         /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool1_fbm_pool_id = 0;      /* in DDR */
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_physical_start_address = 0xffffffff;    /* pool1 physical start address */
ca_uint8_t      l3qm_epp_profile_cpu256_lo_id = 3;                    /* EPP (RX FIFO) profile id */
ca_uint32_t     l3qm_epp_profile_cpu256_lo_sz = 0;                    /* RX FIFO size per entry, 4B */
ca_uint32_t     l3qm_epp_profile_cpu256_lo_cn = 0;                    /* 8 queues, 256 entries per queue */
ca_uint32_t     l3qm_epp_profile_cpu256_lo_start_addresses[8] = {0xffffffff};    /* VoQs 0-7 for QM_CPU_EPP_FIFO0_0_START to QM_CPU_EPP_FIFO15_7_START  */
ca_uint32_t     l3qm_epp_profile_cpu256_lo_map_mode = 0;              /* QM_CPU_EPP1_CFG map_mode=0 */

ca_uint_t     l3qm_eq_profile_cpu256_hi_pool0_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu256_hi_pool1_start_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu256_lo_pool0_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu256_lo_pool1_start_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr = 0xffffffff;

ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_epp_profile_cpu256_lo_rx_fifo_phy_addr = 0xffffffff;

spinlock_t l3qm_epp256_inte_lock;

int aal_l3qm_eq_get_cpu256_hi_use_fbm(void)
{
        return l3qm_eq_profile_cpu256_hi_use_fbm;
}

int aal_l3qm_eq_get_cpu256_lo_use_fbm(void)
{
        return l3qm_eq_profile_cpu256_lo_use_fbm;
}

void aal_l3qm_eq_get_cpu256_hi_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
       *phy_start_addr = l3qm_eq_profile_cpu256_hi_pool0_phy_addr;
       *entry_size = l3qm_eq_profile_cpu256_hi_pool0_buf_sz;
       *entry_count = l3qm_eq_profile_cpu256_hi_pool0_buf_count;
       *fbm_pool_id = l3qm_eq_profile_cpu256_hi_pool0_fbm_pool_id;
}

void aal_l3qm_eq_get_cpu256_hi_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
       *phy_start_addr = l3qm_eq_profile_cpu256_hi_pool1_phy_addr;
       *entry_size = l3qm_eq_profile_cpu256_hi_pool1_buf_sz;
       *entry_count = l3qm_eq_profile_cpu256_hi_pool1_buf_count;
       *fbm_pool_id = l3qm_eq_profile_cpu256_hi_pool1_fbm_pool_id;
}

void aal_l3qm_eq_get_cpu256_lo_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
       *phy_start_addr = l3qm_eq_profile_cpu256_lo_pool0_phy_addr;
       *entry_size = l3qm_eq_profile_cpu256_lo_pool0_buf_sz;
       *entry_count = l3qm_eq_profile_cpu256_lo_pool0_buf_count;
       *fbm_pool_id = l3qm_eq_profile_cpu256_lo_pool0_fbm_pool_id;
}

void aal_l3qm_eq_get_cpu256_lo_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
       *phy_start_addr = l3qm_eq_profile_cpu256_lo_pool1_phy_addr;
       *entry_size = l3qm_eq_profile_cpu256_lo_pool1_buf_sz;
       *entry_count = l3qm_eq_profile_cpu256_lo_pool1_buf_count;
       *fbm_pool_id = l3qm_eq_profile_cpu256_lo_pool1_fbm_pool_id;
}

int aal_l3qm_eq_get_cpu256_max_write_ptr(void)
{
        return (l3qm_cpu256_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp);
}

ca_uint32_t aal_l3qm_cpu256_get_rx_status(int instance)
{
        QM_QM_CPU_EPP_256_STATUS0_t status;

        status.wrd = CA_NE_REG_READ(QM_QM_CPU_EPP_256_STATUS0 + instance * QM_QM_CPU_EPP_256_STATUS0_STRIDE);

        return status.bf.status;
}

void aal_l3qm_cpu256_set_rx_read_ptr(ca_uint16_t instance, ca_uint32_t value)
{
        QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_RDPTR_t      rdptr;

        rdptr.wrd = 0;
        rdptr.bf.rptr = value;
        CA_NE_REG_WRITE(rdptr.wrd, QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_RDPTR + instance * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_RDPTR_STRIDE);
}

ca_uint32_t aal_l3qm_cpu256_get_rx_read_ptr(ca_uint16_t instance)
{
        QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_RDPTR_t      rdptr;

        //TODO: instance should be splitted to be cpu_port and voq???
        /* 64 instances 8 CPU ports each port has 8 VOQs */
        rdptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_RDPTR + instance * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_RDPTR_STRIDE);

        return rdptr.bf.rptr;
}

ca_uint32_t aal_l3qm_cpu256_get_rx_write_ptr(ca_uint16_t instance, ca_uint16_t *count)
{
        ca_uint32_t rdptr;
        ca_uint32_t epp_size, size;
        QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_WRPTR_t      wrptr;

        //TODO: instance should be splitted to be cpu_port and voq???
        /* 64 instances 8 CPU ports each port has 8 VOQs */
        wrptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_WRPTR + instance * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_WRPTR_STRIDE);

        rdptr = aal_l3qm_cpu256_get_rx_read_ptr(instance);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: wrptr.bf.wptr=%d, rdptr=%d\n", __func__, wrptr.bf.wptr, rdptr);
        }

        /* take care of wraping around case */
        epp_size = l3qm_desc_size * l3qm_desc_per_epp;
        if (wrptr.bf.wptr >= rdptr) {
                *count = (wrptr.bf.wptr - rdptr) / epp_size;
        }
        else {
                size = aal_l3qm_eq_get_cpu256_max_write_ptr();
                *count = ((size - rdptr) + wrptr.bf.wptr) / epp_size;
                if (ca_aal_debug & AAL_DBG_L3QM) {
                        printk("%s: max write pointer=%d\n", __func__, size);
                }
        }

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: wrptr.bf.wptr=%d, rdptr=%d, count=%d\n", __func__, wrptr.bf.wptr, rdptr, *count);
        }

        return wrptr.bf.wptr;
}

ca_uint32_t aal_l3qm_cpu256_get_rx_start_addr(ca_uint16_t instance)
{
        QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_t paddr_start;

        //TODO: instance should be splitted to be cpu_port and voq???
        /* 64 instances 8 CPU ports each port has 8 VOQs */
        paddr_start.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START + instance * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE);

        return paddr_start.bf.phy_addr;
}

ca_status_t aal_l3qm_cpu256_check_magic_number(void)
{
	int i, j, size;
	ca_uint8_t *virt_addr;

        /* check magic number among CPU256 EPPs */
        size = l3qm_cpu256_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
        for (j = 0; j < l3qm_cpu256_port_count; j++) {
                for (i = 0; i < l3qm_cpu256_voq_per_port; i++) {
                        virt_addr = (ca_uint8_t *)(l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr + (i * size) + (j * l3qm_cpu256_voq_per_port * size));

                        if (ca_aal_debug & AAL_DBG_L3QM) {
                                printk("%s: CPU256 RX FIFO start address(%d,%d)=0x%p\n", __func__, j, i, virt_addr + (size - l3qm_magic_marker_len));
                        }

                        if (aal_l3qm_check_magic_number(virt_addr + (size - l3qm_magic_marker_len)) != CA_E_OK) {
                                printk("%s: magic number check failed at CPU256 EPP (%d, %d)!!!\n", __func__, j, i);
                                return CA_E_ERROR;
                        }
                }
        }
        printk("Magic number among CPU256 EPPs are OK\n");
	return CA_E_OK;
}

void aal_l3qm_init_cpu256_qm_axi_attrib(void)
{
	int i;
	ca_uint32_t offset;

	/* CPU256 EPP port 0-31 */
	for (i = 0; i < CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES_LEN; i++) {
		offset = CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE + i;
		aal_l3qm_set_epp_axi_attrib(offset, l3qm_axi_attrib_cpu256_epp_values[i]);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: CPU256 EPP offset=0x%x, l3qm_axi_attrib_cpu256_epp_values[%d]=0x%x\n", __func__, offset, i, l3qm_axi_attrib_cpu256_epp_values[i]);
		}
	}
}

void aal_l3qm_init_cpu256_epp(void)
{
	int i, j;
	ca_uint32_t reg_off, size;
	ca_uint8_t *virt_addr;
	QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_t cpu_epp256_fifo_paddr_start;
	QM_QM_CPU_EPP_FIFO_256_CFG_t    cpu256_epp_cfg;
	QM_QM_CPU_EPP_FIFO_CFG_profile0_t cpu_epp_fifo_cfg_profile;

#if 0
        QM_QM_DEST_CPU_EPP256_HI_BUF_CFG_t      epp256_hi_buf_cfg;
        QM_QM_DEST_CPU_EPP256_LO_BUF_CFG_t      epp256_lo_buf_cfg;
#endif

        /* only one EPP profile is used for CPU256 */
        reg_off = QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * l3qm_epp_profile_cpu256_hi_id);
        cpu_epp_fifo_cfg_profile.wrd = CA_NE_REG_READ(reg_off);
        cpu_epp_fifo_cfg_profile.bf.size = l3qm_cpu256_epp_per_voq / CA_L3QM_CPU_EPP_FIFO_SIZE_UINT;     /* the unit is 4 FIFO entries */
        cpu_epp_fifo_cfg_profile.bf.timer_ths = 0xf;
        if ((ca_ni_napi_budget / 16) > 0)
                cpu_epp_fifo_cfg_profile.bf.high_ths = (ca_ni_napi_budget / 16) + 1;    /* Interrupt to CPU is generated when EPP-FIFO accumulates at least ca_napi_budget entries */
        else
                cpu_epp_fifo_cfg_profile.bf.high_ths = 2;                               /* Interrupt to CPU is generated when EPP-FIFO accumulates at least 16 entries */
        CA_NE_REG_WRITE(cpu_epp_fifo_cfg_profile.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: l3qm_cpu256_epp_per_voq=%d\n", __func__, l3qm_cpu256_epp_per_voq);
        }

        /* Select one of 4 profiles for EPP-FIFO size, occupancy-high-threshold and time-threshold for 32 CPU ports (CPU256)  */
        size = l3qm_cpu256_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
        for (j = 0; j < l3qm_cpu256_port_count; j++) {
                for (i = 0; i < l3qm_cpu256_voq_per_port; i++) {
                        reg_off = QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START + (i * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE) +
                                        (j * l3qm_cpu256_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE);
                        cpu_epp256_fifo_paddr_start.bf.phy_addr = l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr + (i * size) + (j * l3qm_cpu256_voq_per_port * size);
                        CA_NE_REG_WRITE(cpu_epp256_fifo_paddr_start.wrd, reg_off);

                        if (ca_aal_debug & AAL_DBG_L3QM) {
                                printk("%s: CPU256 RX FIFO register offset(%d, %d)=0x%x\n", __func__, j, i, reg_off);
                                printk("%s: CPU256 RX FIFO start address(%d,%d)=0x%x\n", __func__, j, i, cpu_epp256_fifo_paddr_start.bf.phy_addr);
                        }

                        virt_addr = (ca_uint8_t *)(l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr + (i * size) + (j * l3qm_cpu256_voq_per_port * size) + (size - l3qm_magic_marker_len));
                        aal_l3qm_insert_magic_number(virt_addr);
                }
	}

        /* fill EPP profile id */
        reg_off = QM_QM_CPU_EPP_FIFO_256_CFG;
        cpu256_epp_cfg.wrd = 0;
        cpu256_epp_cfg.bf.profile_sel_lo = l3qm_epp_profile_cpu256_hi_id;
        cpu256_epp_cfg.bf.profile_sel_hi = l3qm_epp_profile_cpu256_hi_id;
        CA_NE_REG_WRITE(cpu256_epp_cfg.wrd, reg_off);

        /* current SoC issue, set head room for CPU256 will cause DEEPQ failed */
#if 0
        epp256_hi_buf_cfg.wrd = CA_NE_REG_READ(QM_QM_DEST_CPU_EPP256_HI_BUF_CFG);
        epp256_hi_buf_cfg.bf.head_room_first = aal_l3qm_get_cpu_port_head_room_first() / 16;       /* reserved 32 bytes for first fragment of packet  */
        epp256_hi_buf_cfg.bf.head_room_rest = epp256_hi_buf_cfg.bf.head_room_first;       /* reserved 32 bytes for reset of segments of packet  */

        CA_NE_REG_WRITE(epp256_hi_buf_cfg.wrd, QM_QM_DEST_CPU_EPP256_HI_BUF_CFG);

        epp256_lo_buf_cfg.wrd = CA_NE_REG_READ(QM_QM_DEST_CPU_EPP256_LO_BUF_CFG);
        epp256_lo_buf_cfg.bf.head_room_first = aal_l3qm_get_cpu_port_head_room_first() / 16;       /* reserved 32 bytes for first fragment of packet  */
        epp256_lo_buf_cfg.bf.head_room_rest = epp256_lo_buf_cfg.bf.head_room_first;       /* reserved 32 bytes for reset of segments of packet  */
        CA_NE_REG_WRITE(epp256_lo_buf_cfg.wrd, QM_QM_DEST_CPU_EPP256_LO_BUF_CFG);
#endif

}

void aal_l3qm_init_empty_buffer_CPU256_hi(void)
{
        ca_uint32_t reg_off;
        QM_QM_EQ_PROFILE0_t eq_profile;
        //QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
        QM_QM_CFG0_EQ0_t cfg0_eq;
        QM_QM_CFG1_EQ0_t cfg1_eq;
        QM_QM_CFG2_EQ0_t cfg2_eq;
        QM_QM_CFG3_EQ0_t cfg3_eq;
        QM_QM_CFG4_EQ0_t cfg4_eq;
        QM_QM_CPU_256_EQ_CFG_t cpu256_eq_cfg;

        /* profile hi for CPU256 */
        eq_profile.wrd = 0;
        eq_profile.bf.eqp0 = l3qm_eq_profile_cpu256_hi_pool0_eq_id;
        eq_profile.bf.eqp1 = l3qm_eq_profile_cpu256_hi_pool1_eq_id;
        eq_profile.bf.rule = l3qm_eq_profile_cpu256_hi_rule;
        reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_cpu256_hi_id);
        CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: CPU256 hi pool0 eq_id=%d, rule=%d\n", __func__,
                        l3qm_eq_profile_cpu256_hi_pool0_eq_id, l3qm_eq_profile_cpu256_hi_rule);
        }

        /* select profile hi/lo for CPU 256 */
        cpu256_eq_cfg.wrd = 0;
        cpu256_eq_cfg.bf.profile_sel_lo = l3qm_eq_profile_cpu256_lo_id;
        cpu256_eq_cfg.bf.profile_sel_hi = l3qm_eq_profile_cpu256_hi_id;
        reg_off = QM_QM_CPU_256_EQ_CFG;
        CA_NE_REG_WRITE(cpu256_eq_cfg.wrd, reg_off);

        /* configure EB pool 0 for CPU256 hi */
        /* set phy_addr_start and enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        //cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_cpu256_hi_pool0_phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool0_eq_id);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: CPU256 hi phy_addr_start = 0x%x\n", __func__, cfg0_eq.bf.phy_addr_start);
        }

        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_cpu256_hi_pool0_buf_count;
        cfg1_eq.bf.bid_start = l3qm_eq_profile_cpu256_hi_pool0_bid_start;
        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool0_eq_id);
        CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: CPU256 hi pool0 buf_count=%d, bid_start=%d\n", __func__,
                        l3qm_eq_profile_cpu256_hi_pool0_buf_count, l3qm_eq_profile_cpu256_hi_pool0_bid_start);
        }

        /* configure buffer size for Empty Buffer */
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool0_eq_id);
        cfg2_eq.wrd = CA_NE_REG_READ(reg_off);

	/* use the same threshold as non-FBM, because we wish QM REFILL interrupt happened every 160 BIDs comsumed */
	/* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
	/* the budget value should not be too small otherwise the interrupts happened too frequently */
	//cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
	cfg2_eq.bf.refill_ths = (l3qm_cpu256_pool_bid_count - ca_ni_napi_budget) / 8;

        if (l3qm_eq_profile_cpu256_hi_use_fbm)
                cfg2_eq.bf.refill_en = 1;
        else
                cfg2_eq.bf.refill_en = 0;
        cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu256_hi_pool0_fbm_pool_id;
        cfg2_eq.bf.cpu_eq = 1;
        cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_cpu256_hi_pool0_buf_sz);      /* 2048 bytes */
        CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: CPU256 hi pool0 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_cpu256_hi_pool0_buf_sz, cfg2_eq.bf.buffer_size);
        }

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool0_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[4];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        /* because PE0 pool1 locates at DDR start from 0x180000000 */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool0_eq_id);
        cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_cpu256_hi_pool0_eq_id];
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

        /* configure EB pool 1 for CPU256 hi */
        /* set phy_addr_start and enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        //cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_cpu256_hi_pool1_phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool1_eq_id);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: CPU256 hi pool1 eq_id=%d\n", __func__, l3qm_eq_profile_cpu256_hi_pool1_eq_id);
        }

        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_cpu256_hi_pool1_buf_count;
        cfg1_eq.bf.bid_start = l3qm_eq_profile_cpu256_hi_pool1_bid_start;
        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool1_eq_id);
        CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: CPU256 hi pool1 buf_count=%d, bid_start=%d\n", __func__,
                                l3qm_eq_profile_cpu256_hi_pool1_buf_count, l3qm_eq_profile_cpu256_hi_pool1_bid_start);
	}

	/* configure buffer size for Empty Buffer */
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool1_eq_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);

        /* use the same threshold as non-FBM, because we wish QM REFILL interrupt happened every 160 BIDs comsumed */
        /* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
        /* the budget value should not be too small otherwise the interrupts happened too frequently */
        //cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
        cfg2_eq.bf.refill_ths = (l3qm_cpu256_pool_bid_count - ca_ni_napi_budget) / 8;

	if (l3qm_eq_profile_cpu256_hi_use_fbm)
		cfg2_eq.bf.refill_en = 1;
	else
		cfg2_eq.bf.refill_en = 0;
	cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu256_hi_pool1_fbm_pool_id;
	cfg2_eq.bf.cpu_eq = 1;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_cpu256_hi_pool1_buf_sz);      /* 2048 bytes */
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU256 hi pool1 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_cpu256_hi_pool1_buf_sz, cfg2_eq.bf.buffer_size);
	}

	/* configure cache_eos/domain/snoop/cache */
	reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool1_eq_id);
	cfg3_eq.wrd = l3qm_eq_cfg3_values[6];
	CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	/* because PE0 pool1 locates at DDR start from 0x180000000 */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool1_eq_id);
	cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_cpu256_hi_pool1_eq_id];
	CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);
}

void aal_l3qm_init_empty_buffer_CPU256_lo(void)
{
	ca_uint32_t reg_off;
	QM_QM_EQ_PROFILE0_t eq_profile;
	//QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
	QM_QM_CFG0_EQ0_t cfg0_eq;
	QM_QM_CFG1_EQ0_t cfg1_eq;
	QM_QM_CFG2_EQ0_t cfg2_eq;
	QM_QM_CFG3_EQ0_t cfg3_eq;
	QM_QM_CFG4_EQ0_t cfg4_eq;
	QM_QM_CPU_256_EQ_CFG_t cpu256_eq_cfg;

	/* profile hi for CPU256 */
	eq_profile.wrd = 0;
	eq_profile.bf.eqp0 = l3qm_eq_profile_cpu256_lo_pool0_eq_id;
	eq_profile.bf.eqp1 = l3qm_eq_profile_cpu256_lo_pool1_eq_id;
	eq_profile.bf.rule = l3qm_eq_profile_cpu256_lo_rule;
	reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_cpu256_lo_id);
	CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU256 lo pool0 eq_id=%d, rule=%d\n", __func__,
				l3qm_eq_profile_cpu256_lo_pool0_eq_id, l3qm_eq_profile_cpu256_lo_rule);
	}

	/* select profile hi/lo for CPU 256 */
	cpu256_eq_cfg.wrd = 0;
	cpu256_eq_cfg.bf.profile_sel_lo = l3qm_eq_profile_cpu256_lo_id;
	cpu256_eq_cfg.bf.profile_sel_hi = l3qm_eq_profile_cpu256_hi_id;
	reg_off = QM_QM_CPU_256_EQ_CFG;
	CA_NE_REG_WRITE(cpu256_eq_cfg.wrd, reg_off);

	/* configure EB pool 0 for CPU256 hi */
	/* set phy_addr_start and enable Empty Buffer */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
	//cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_cpu256_lo_pool0_phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool0_eq_id);
	CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU256 lo phy_addr_start = 0x%x\n", __func__, cfg0_eq.bf.phy_addr_start);
	}

	cfg1_eq.wrd = 0;
	cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_cpu256_lo_pool0_buf_count;
	cfg1_eq.bf.bid_start = l3qm_eq_profile_cpu256_lo_pool0_bid_start;
	reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool0_eq_id);
	CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU256 lo pool0 buf_count=%d, bid_start=%d\n", __func__,
				l3qm_eq_profile_cpu256_lo_pool0_buf_count, l3qm_eq_profile_cpu256_lo_pool0_bid_start);
	}
	/* configure buffer size for Empty Buffer */
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool0_eq_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);

	/* use the same threshold as non-FBM, because we wish QM REFILL interrupt happened every 160 BIDs comsumed */
	/* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
	/* the budget value should not be too small otherwise the interrupts happened too frequently */
	//cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
	cfg2_eq.bf.refill_ths = (l3qm_cpu256_pool_bid_count - ca_ni_napi_budget) / 8;

	if (l3qm_eq_profile_cpu256_lo_use_fbm)
		cfg2_eq.bf.refill_en = 1;
	else
		cfg2_eq.bf.refill_en = 0;
	cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu256_lo_pool0_fbm_pool_id;
	cfg2_eq.bf.cpu_eq = 1;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_cpu256_lo_pool0_buf_sz);      /* 2048 bytes */
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU256 lo pool0 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_cpu256_lo_pool0_buf_sz, cfg2_eq.bf.buffer_size);
	}

	/* configure cache_eos/domain/snoop/cache */
	reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool0_eq_id);
	cfg3_eq.wrd = l3qm_eq_cfg3_values[5];
	CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	/* because PE0 pool1 locates at DDR start from 0x180000000 */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool0_eq_id);
	cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_cpu256_lo_pool0_eq_id];
	CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

	/* configure EB pool 1 for CPU256 hi */
	/* set phy_addr_start and enable Empty Buffer */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
	//cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_cpu256_lo_pool1_phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool1_eq_id);
	CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU256 lo pool1 eq_id=%d\n", __func__, l3qm_eq_profile_cpu256_lo_pool1_eq_id);
	}

	cfg1_eq.wrd = 0;
	cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_cpu256_lo_pool1_buf_count;
	cfg1_eq.bf.bid_start = l3qm_eq_profile_cpu256_lo_pool1_bid_start;
	reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool1_eq_id);
	CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU256 lo pool1 buf_count=%d, bid_start=%d\n", __func__,
				l3qm_eq_profile_cpu256_lo_pool1_buf_count, l3qm_eq_profile_cpu256_lo_pool1_bid_start);
	}

	/* configure buffer size for Empty Buffer */
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool1_eq_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);

	/* use the same threshold as non-FBM, because we wish QM REFILL interrupt happened every 160 BIDs comsumed */
	/* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
	/* the budget value should not be too small otherwise the interrupts happened too frequently */
	//cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
	cfg2_eq.bf.refill_ths = (l3qm_cpu256_pool_bid_count - ca_ni_napi_budget) / 8;

	if (l3qm_eq_profile_cpu256_lo_use_fbm)
		cfg2_eq.bf.refill_en = 1;
	else
		cfg2_eq.bf.refill_en = 0;
	cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu256_lo_pool1_fbm_pool_id;
	cfg2_eq.bf.cpu_eq = 1;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_cpu256_lo_pool1_buf_sz);      /* 2048 bytes */
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU256 lo pool1 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_cpu256_lo_pool1_buf_sz, cfg2_eq.bf.buffer_size);
	}

	/* configure cache_eos/domain/snoop/cache */
	reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool1_eq_id);
	cfg3_eq.wrd = l3qm_eq_cfg3_values[7];
	CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	/* because PE0 pool1 locates at DDR start from 0x180000000 */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool1_eq_id);
	cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_cpu256_lo_pool1_eq_id];
	CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);
}

void aal_l3qm_enable_cpu256_epp_interrupt(ca_uint8_t cpu256_hi, ca_uint8_t enable)
{
        QM_QM_EPP256_INTERRUPT_EN_t     cpu256_int_en;
        unsigned long flags;

        /* these registers are shared by all of CPUs */
        spin_lock_irqsave(&l3qm_epp256_inte_lock, flags);

        cpu256_int_en.wrd = CA_NE_REG_READ(QM_QM_EPP256_INTERRUPT_EN);
        if (cpu256_hi) {
                cpu256_int_en.bf.epp256_hi_int_en = enable;
        }
        else {
                cpu256_int_en.bf.epp256_lo_int_en = enable;
        }
        CA_NE_REG_WRITE(cpu256_int_en.wrd, QM_QM_EPP256_INTERRUPT_EN);

        spin_unlock_irqrestore(&l3qm_epp256_inte_lock, flags);
}

void aal_l3qm_dump_cpu256_ddr_map(void)
{
        printk("CPU256 hi pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool0_start_addr);
        printk("CPU256 hi pool0 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_hi_pool0_phy_addr);
        printk("CPU256 hi pool0 buffer count = %d\n", l3qm_eq_profile_cpu256_hi_pool0_buf_count);
        printk("CPU256 hi pool0 buffer size = %d\n", l3qm_eq_profile_cpu256_hi_pool0_buf_sz);
        //printk("CPU256 hi pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool1_start_addr - l3qm_eq_profile_cpu256_hi_pool0_start_addr);
        printk("CPU256 hi pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool0_buf_count * l3qm_eq_profile_cpu256_hi_pool0_buf_sz);

        printk("CPU256 hi pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool1_start_addr);
        printk("CPU256 hi pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_hi_pool1_phy_addr);
        printk("CPU256 hi pool1 buffer count = %d\n", l3qm_eq_profile_cpu256_hi_pool1_buf_count);
        printk("CPU256 hi pool1 buffer size = %d\n", l3qm_eq_profile_cpu256_hi_pool1_buf_sz);
        //printk("CPU256 hi pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr - l3qm_eq_profile_cpu256_hi_pool1_start_addr);
        printk("CPU256 hi pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool1_buf_count * l3qm_eq_profile_cpu256_hi_pool1_buf_sz);

        printk("CPU256 hi RX FIFO start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr);
        printk("CPU256 hi RX FIFO physical addr = 0x%x\n", l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr);
        printk("CPU256 hi RX FIFO size = %d\n", l3qm_epp_profile_cpu256_hi_sz);
        printk("CPU256 hi RX FIFO count = %d\n", l3qm_epp_profile_cpu256_hi_cn);
        //printk("CPU256 hi RX FIFO consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool0_start_addr - l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr);
        printk("CPU256 hi RX FIFO consumed size = %lld\n", (ca_uint64_t)l3qm_epp_profile_cpu256_hi_sz * l3qm_epp_profile_cpu256_hi_cn);

        printk("CPU256 lo pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool0_start_addr);
        printk("CPU256 lo pool0 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_lo_pool0_phy_addr);
        printk("CPU256 lo pool0 buffer count = %d\n", l3qm_eq_profile_cpu256_lo_pool0_buf_count);
        printk("CPU256 lo pool0 buffer size = %d\n", l3qm_eq_profile_cpu256_lo_pool0_buf_sz);
        //printk("CPU256 lo pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool1_start_addr - l3qm_eq_profile_cpu256_lo_pool0_start_addr);
        printk("CPU256 lo pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool0_buf_count * l3qm_eq_profile_cpu256_lo_pool0_buf_sz);

        printk("CPU256 lo pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool1_start_addr);
        printk("CPU256 lo pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_lo_pool1_phy_addr);
        printk("CPU256 lo pool1 buffer count = %d\n", l3qm_eq_profile_cpu256_lo_pool1_buf_count);
        printk("CPU256 lo pool1 buffer size = %d\n", l3qm_eq_profile_cpu256_lo_pool1_buf_sz);
        //printk("CPU256 lo pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr - l3qm_eq_profile_cpu256_lo_pool1_start_addr);
        printk("CPU256 lo pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool1_buf_count * l3qm_eq_profile_cpu256_lo_pool1_buf_sz);

        printk("CPU256 lo RX FIFO start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr);
        printk("CPU256 lo RX FIFO physical addr = 0x%x\n", l3qm_epp_profile_cpu256_lo_rx_fifo_phy_addr);
        printk("CPU256 lo RX FIFO size = %d\n", l3qm_epp_profile_cpu256_lo_sz);
        printk("CPU256 lo RX FIFO count = %d\n", l3qm_epp_profile_cpu256_lo_cn);
        printk("CPU256 lo RX FIFO consumed size = %d\n", l3qm_epp_profile_cpu256_lo_sz * l3qm_epp_profile_cpu256_lo_cn);

}

ca_uint32_t aal_l3qm_create_cpu256_ddr_map(ca_uint_t buff_start_addr, ca_uint32_t buff_phy_addr)
{
	ca_uint32_t size;

	/* CPU256 hi pool0 buffer */
        l3qm_eq_profile_cpu256_hi_pool0_start_addr = ((buff_start_addr + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        l3qm_eq_profile_cpu256_hi_pool0_phy_addr = ((buff_phy_addr + l3qm_eq_buffer_align_size) /
                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        size = l3qm_eq_profile_cpu256_hi_pool0_buf_count * l3qm_eq_profile_cpu256_hi_pool0_buf_sz;

        /* Magic number */
        size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_cpu256_hi_pool0_start_addr + size);

        /* CPU256 hi pool1 buffer */
        l3qm_eq_profile_cpu256_hi_pool1_start_addr = ((l3qm_eq_profile_cpu256_hi_pool0_start_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        l3qm_eq_profile_cpu256_hi_pool1_phy_addr = ((l3qm_eq_profile_cpu256_hi_pool0_phy_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        size = l3qm_eq_profile_cpu256_hi_pool1_buf_count * l3qm_eq_profile_cpu256_hi_pool1_buf_sz;

        /* Magic number */
        size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_cpu256_hi_pool1_start_addr + size);

        /* CPU256 hi RX FIFO */
        l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr = ((l3qm_eq_profile_cpu256_hi_pool1_start_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr = ((l3qm_eq_profile_cpu256_hi_pool1_phy_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        size = l3qm_epp_profile_cpu256_hi_sz * l3qm_epp_profile_cpu256_hi_cn + (l3qm_magic_marker_len * l3qm_cpu256_port_count * l3qm_cpu256_voq_per_port);

        /* Magic number */
        size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr + size);

        /* CPU256 lo pool0 buffer */
        l3qm_eq_profile_cpu256_lo_pool0_start_addr = ((l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        l3qm_eq_profile_cpu256_lo_pool0_phy_addr = ((l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        size = l3qm_eq_profile_cpu256_lo_pool0_buf_count * l3qm_eq_profile_cpu256_lo_pool0_buf_sz;

        /* Magic number */
        size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_cpu256_lo_pool0_start_addr + size);

        /* CPU256 hi pool1 buffer */
        l3qm_eq_profile_cpu256_lo_pool1_start_addr = ((l3qm_eq_profile_cpu256_lo_pool0_start_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        l3qm_eq_profile_cpu256_lo_pool1_phy_addr = ((l3qm_eq_profile_cpu256_lo_pool0_phy_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        size = l3qm_eq_profile_cpu256_lo_pool1_buf_count * l3qm_eq_profile_cpu256_lo_pool1_buf_sz;

        /* Magic number */
        size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_cpu256_lo_pool1_start_addr + size);

        /* CPU256 hi RX FIFO */
        l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr = ((l3qm_eq_profile_cpu256_lo_pool1_start_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        l3qm_epp_profile_cpu256_lo_rx_fifo_phy_addr = ((l3qm_eq_profile_cpu256_lo_pool1_phy_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        size = l3qm_epp_profile_cpu256_lo_sz * l3qm_epp_profile_cpu256_lo_cn + (l3qm_magic_marker_len * l3qm_cpu256_port_count * l3qm_cpu256_voq_per_port);

        size = (l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr + size) - l3qm_eq_profile_dq_pool0_buff_start_addr;

	return size;
}

void aal_l3qm_init_cpu256_startup_config(void)
{
	aal_scfg_read(CFG_ID_L3QM_CPU256_PORT_COUNT, 4, &l3qm_cpu256_port_count);
	aal_scfg_read(CFG_ID_L3QM_CPU256_VOQ_PER_PORT, 1, &l3qm_cpu256_voq_per_port);
	aal_scfg_read(CFG_ID_L3QM_CPU256_EPP_PER_VOQ, 4, &l3qm_cpu256_epp_per_voq);
	aal_scfg_read(CFG_ID_L3QM_CPU256_POOL_BID_COUNT, 4, &l3qm_cpu256_pool_bid_count);

        /* read CPU256 hi configure */
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_ID, 1, &l3qm_eq_profile_cpu256_hi_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_RULE, 1, &l3qm_eq_profile_cpu256_hi_rule);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_USE_FBM, 1, &l3qm_eq_profile_cpu256_hi_use_fbm);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_EQ_ID, 1, &l3qm_eq_profile_cpu256_hi_pool0_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_hi_pool0_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BID_START, 4, &l3qm_eq_profile_cpu256_hi_pool0_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_SZ, 4, &l3qm_eq_profile_cpu256_hi_pool0_buf_sz);
        //aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_LOCATION, 1, &l3qm_eq_profile_cpu256_hi_pool0_location);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_hi_pool0_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_EQ_ID, 1, &l3qm_eq_profile_cpu256_hi_pool1_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_hi_pool1_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BID_START, 4, &l3qm_eq_profile_cpu256_hi_pool1_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BUF_SZ, 4, &l3qm_eq_profile_cpu256_hi_pool1_buf_sz);
        //aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_LOCATION, 1, &l3qm_eq_profile_cpu256_hi_pool1_location);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_hi_pool1_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_ID, 1, &l3qm_epp_profile_cpu256_hi_id);
        //aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_SZ, 4, &l3qm_epp_profile_cpu256_hi_sz);
        //aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_CN, 4, &l3qm_epp_profile_cpu256_hi_cn);

        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_MAP_MODE, 4, &l3qm_epp_profile_cpu256_hi_map_mode);

        /* read CPU256 lo configure */
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_ID, 1, &l3qm_eq_profile_cpu256_lo_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_RULE, 1, &l3qm_eq_profile_cpu256_lo_rule);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_USE_FBM, 1, &l3qm_eq_profile_cpu256_lo_use_fbm);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_EQ_ID, 1, &l3qm_eq_profile_cpu256_lo_pool0_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_lo_pool0_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BID_START, 4, &l3qm_eq_profile_cpu256_lo_pool0_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_SZ, 4, &l3qm_eq_profile_cpu256_lo_pool0_buf_sz);
        //aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_LOCATION, 1, &l3qm_eq_profile_cpu256_lo_pool0_location);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_lo_pool0_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_EQ_ID, 1, &l3qm_eq_profile_cpu256_lo_pool1_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_lo_pool1_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BID_START, 4, &l3qm_eq_profile_cpu256_lo_pool1_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BUF_SZ, 4, &l3qm_eq_profile_cpu256_lo_pool1_buf_sz);
        //aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_LOCATION, 1, &l3qm_eq_profile_cpu256_lo_pool1_location);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_lo_pool1_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_ID, 1, &l3qm_epp_profile_cpu256_lo_id);
        //aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_SZ, 4, &l3qm_epp_profile_cpu256_lo_sz);
        //aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_CN, 4, &l3qm_epp_profile_cpu256_lo_cn);

        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_MAP_MODE, 4, &l3qm_epp_profile_cpu256_lo_map_mode);

	aal_scfg_read(CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES, CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES_LEN * sizeof(ca_uint32_t), l3qm_axi_attrib_cpu256_epp_values);

	l3qm_epp_profile_cpu256_hi_total_epp_count = ((l3qm_cpu256_port_count / 2) * l3qm_cpu256_voq_per_port * l3qm_cpu256_epp_per_voq);  /* CPU256 total EPP count */

	/* EPP size is the same as CPU64's */
	l3qm_epp_profile_cpu256_hi_sz = l3qm_epp_profile_cpu256_lo_sz = l3qm_epp_size;

	/* calculate total EPP count for CPU256 HI/LO from l3qm_cpu256_epp_per_voq */
	l3qm_epp_profile_cpu256_lo_cn = l3qm_epp_profile_cpu256_hi_cn = l3qm_cpu256_epp_per_voq * (l3qm_cpu256_port_count / 2) * l3qm_cpu256_voq_per_port;
	
	printk("%s: l3qm_epp_profile_cpu256_lo_cn=%d, l3qm_cpu256_epp_per_voq=%d, l3qm_cpu256_port_count=%d, l3qm_cpu256_voq_per_port=%d\n",
		__func__, l3qm_epp_profile_cpu256_lo_cn, l3qm_cpu256_epp_per_voq, l3qm_cpu256_port_count, l3qm_cpu256_voq_per_port);
}


void aal_l3qm_dump_cpu256_epps(void)
{
	int i, j;
	ca_uint32_t reg_off;
	QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_t cpu_epp256_fifo_paddr_start;
	QM_QM_CPU_EPP_FIFO_256_CFG_t    cpu256_epp_cfg;

	printk("=== QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START/QM_QM_CPU_EPP_FIFO_256_CFG ===\n");
	for (j = 0; j < l3qm_cpu256_port_count; j++) {
		for (i = 0; i < l3qm_cpu256_voq_per_port; i++) {

			/* fill EPP profile id */
			reg_off = QM_QM_CPU_EPP_FIFO_256_CFG;
			cpu256_epp_cfg.wrd = CA_NE_REG_READ(reg_off);

			reg_off = QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START + (i * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE) +
				(j * l3qm_cpu256_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE);
			cpu_epp256_fifo_paddr_start.wrd = CA_NE_REG_READ(reg_off);

			printk("port id=%d, voq id=%d, reg_off=0x%x, paddr_start=0x%x, profile sel lo=%d, profile sel hi=%d\n", j, i, reg_off,
					cpu_epp256_fifo_paddr_start.bf.phy_addr, cpu256_epp_cfg.bf.profile_sel_lo, cpu256_epp_cfg.bf.profile_sel_hi);
		}
	}
}

void aal_l3qm_dump_cpu256_empty_buffers_phy_addr(void)
{
	int i;
	ca_uint32_t     phy_addr;

	printk("EQ=%d\n", l3qm_eq_profile_cpu256_lo_pool0_eq_id);
	aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu256_lo_pool0_eq_id, 1);
	i=0;
	while ((phy_addr = aal_l3qm_get_return_phy_addr(l3qm_eq_profile_cpu256_lo_pool0_eq_id)) != 0) {
		printk("[%d] phy_addr=0x%x\n", i, phy_addr);
		i++;
	}
	aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu256_lo_pool0_eq_id, 0);

	printk("EQ=%d\n", l3qm_eq_profile_cpu256_lo_pool1_eq_id);
	aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu256_lo_pool1_eq_id, 1);
	i=0;
	while ((phy_addr = aal_l3qm_get_return_phy_addr(l3qm_eq_profile_cpu256_lo_pool1_eq_id)) != 0) {
		printk("[%d] phy_addr=0x%x\n", i, phy_addr);
		i++;
	}
	aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu256_lo_pool1_eq_id, 0);
}

void aal_l3qm_init_cpu256_empty_buffer(void)
{
        aal_l3qm_init_empty_buffer_CPU256_hi();
        aal_l3qm_init_empty_buffer_CPU256_lo();
        spin_lock_init(&l3qm_epp256_inte_lock);
}

void aal_l3qm_init_cpu256_voq(void)
{
        CA_NE_REG_WRITE(0x0000ffff, QM_QM_CPU_EPP_256_FIFO_INT_LO);
        CA_NE_REG_WRITE(0xffff0000, QM_QM_CPU_EPP_256_FIFO_INT_HI);
}

void aal_l3qm_enable_tx_cpu256(ca_uint16_t cpu_port, ca_uint8_t enable)
{
        QM_QM_ES_CTRL_CPU256_t es_ctrl_cpu256;

        es_ctrl_cpu256.wrd = CA_NE_REG_READ(QM_QM_ES_CTRL_CPU256);
	if (cpu_port == 1) {
		if (enable) {
			es_ctrl_cpu256.wrd |= 0xffff0000;
		}
		else {
			es_ctrl_cpu256.wrd &= 0x0000ffff;
		}
	}
	else {
		if (enable) {
			es_ctrl_cpu256.wrd |= 0x0000ffff;
		}
		else {
			es_ctrl_cpu256.wrd &= 0xffff0000;
		}
	}
        CA_NE_REG_WRITE(es_ctrl_cpu256.wrd, QM_QM_ES_CTRL_CPU256);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: cpu_port=%d, enable=%d, es_ctrl_cpu256.wrd=0x%x\n", __func__, cpu_port, enable, es_ctrl_cpu256.wrd);
        }
}

