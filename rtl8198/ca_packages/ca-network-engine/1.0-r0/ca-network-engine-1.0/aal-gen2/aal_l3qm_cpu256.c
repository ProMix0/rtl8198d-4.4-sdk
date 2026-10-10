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
#include <generated/ca_ne_autoconf.h>
#include <linux/sched.h>

#include "scfg.h"
#include "osal_cmn.h"
#include "aal_common.h"
#include "aal_table.h"
#include "aal_fbm.h"
#include "aal_l3qm.h"
#include "aal_l3qm_cpu256.h"
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

ca_uint32_t     l3qm_cpu256_port_count = 32;                   /* CPU256 port count */
ca_uint8_t      l3qm_cpu256_voq_per_port = 8;                   /* CPU256 VoQ per port */
ca_uint32_t      l3qm_cpu256_epp_per_voq = 32;                   /* CPU256 EPP per VoQ */
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

ca_uint8_t	l3qm_cpu256_epp_cpu_voq_map = 0;
ca_uint8_t	l3qm_cpu256_epp_aggr_enable = 0;
ca_uint32_t	l3qm_epp256a_fifo_drop_profile[CFG_ID_QM_QM_CPU_EPP256A_FIFO_DROP_profile_levels_LEN] = {0};
ca_uint32_t	l3qm_epp256_fifo_drop_profile[CFG_ID_QM_QM_CPU_EPP256_FIFO_DROP_profile_levels_LEN] = {0};
ca_uint32_t	l3qm_epp256_fifo_cfg[CFG_ID_QM_QM_CPU_EPP256_FIFO_CFG_LEN] = {0};

spinlock_t l3qm_epp256_inte_lock;

extern ca_uint8_t      l3qm_ace_test;

#ifdef CA_L3TE_SHAPER_TEST
static void aal_l3te_add_shaper(void);
#endif

int aal_l3qm_eq_get_cpu256_hi_rule(void)
{
        return l3qm_eq_profile_cpu256_hi_rule;
}

int aal_l3qm_eq_get_cpu256_lo_rule(void)
{
        return l3qm_eq_profile_cpu256_lo_rule;
}

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
	if (l3qm_cpu256_epp_aggr_enable) {
		return (l3qm_cpu256_epp_per_voq * l3qm_cpu256_voq_per_port * l3qm_desc_size * l3qm_desc_per_epp);
	}
	else {
		return (l3qm_cpu256_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp);
	}
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
                printk("%s: instance=%d, wrptr.bf.wptr=%d, rdptr=%d\n", __func__, instance, wrptr.bf.wptr, rdptr);
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
	QM_QM_AXI_ATTRIBUTE_DATA0_t data0;

	/* CPU256 EPP port 0-31 */
	for (i = 0; i < CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES_LEN; i++) {
		offset = CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE + i;

		/* if QM ACE not turned on, ignore the startup configure settings if ace_cmd=1 or axi_top_bit=2 */
		data0.wrd = l3qm_axi_attrib_cpu256_epp_values[i];
		if (!l3qm_ace_test) {
			if (data0.bf.axi_top_bit == 2 || data0.bf.ace_cmd == 1)
				data0.wrd = 0;
		}
		aal_l3qm_set_epp_axi_attrib(offset, data0.wrd);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			 printk("%s: CPU256 EPP[%d] offset=0x%x, data0.wrd=0x%x, l3qm_axi_attrib_cpu256_epp_values=0x%x\n",
                                __func__, i, offset, data0.wrd, l3qm_axi_attrib_cpu256_epp_values[i]);
		}
	}
}

static void aal_l3qm_init_cpuepp256_fifo_paddr_start(int aggr_mode, int map_mode)
{
	int i, j, map_port_count = 0;
	ca_uint32_t reg_off, size;
	QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_t cpu_epp256_fifo_paddr_start;

	if (aggr_mode) {
		size = (l3qm_cpu256_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp * l3qm_cpu256_voq_per_port) + l3qm_magic_marker_len;
	}
	else {
		size = l3qm_cpu256_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
	}

	if (aggr_mode) {
		if (map_mode) {
			map_port_count = 8;
		}
		for (j = 0; j < l3qm_cpu256_port_count + map_port_count; j++) {
			reg_off = QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START +
				(j * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE);
			cpu_epp256_fifo_paddr_start.bf.phy_addr = l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr +
				(j * size);
			CA_NE_REG_WRITE(cpu_epp256_fifo_paddr_start.wrd, reg_off);

			if (ca_aal_debug & AAL_DBG_L3QM) {
				printk("%s: CPU256 RX FIFO register offset(%d)=0x%x\n", __func__, j, reg_off);
				printk("%s: CPU256 RX FIFO start address(%d)=0x%x\n", __func__, j, cpu_epp256_fifo_paddr_start.bf.phy_addr);
			}
		}
	}
	else {
		for (j = 0; j < l3qm_cpu256_port_count; j++) {
			for (i = 0; i < l3qm_cpu256_voq_per_port; i++) {
				reg_off = QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START +
					  (i * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE) +
					  (j * l3qm_cpu256_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE);
				cpu_epp256_fifo_paddr_start.bf.phy_addr = l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr + (i * size) +
									  (j * l3qm_cpu256_voq_per_port * size);
				CA_NE_REG_WRITE(cpu_epp256_fifo_paddr_start.wrd, reg_off);

				if (ca_aal_debug & AAL_DBG_L3QM) {
					printk("%s: CPU256 RX FIFO register offset(%d, %d)=0x%x\n", __func__, j, i, reg_off);
					printk("%s: CPU256 RX FIFO start address(%d,%d)=0x%x\n", __func__, j, i,
						cpu_epp256_fifo_paddr_start.bf.phy_addr);
				}
			}
		}
	}
}

void aal_l3qm_init_cpu256_epp(void)
{
	int i;
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP_FIFO_256_CFG_t    cpu256_epp_cfg;
	QM_QM_CPU_EPP_FIFO_CFG_profile0_t cpu_epp_fifo_cfg_profile;

        QM_QM_DEST_CPU_EPP256_HI_BUF_CFG_t      epp256_hi_buf_cfg;
        QM_QM_DEST_CPU_EPP256_LO_BUF_CFG_t      epp256_lo_buf_cfg;
	TE_TB2RATE_TB_CTRL_t	tb_ctrl;
	TE_TB2RATE_TB_CTRL1_t	tb_ctrl1;
	TE_TB2RATE_CTRL_t	ctrl;

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
	aal_l3qm_init_cpuepp256_fifo_paddr_start(l3qm_cpu256_epp_aggr_enable, l3qm_cpu256_epp_cpu_voq_map);

        /* fill EPP profile id */
        reg_off = QM_QM_CPU_EPP_FIFO_256_CFG;
        cpu256_epp_cfg.wrd = 0;
        cpu256_epp_cfg.bf.profile_sel_lo = l3qm_epp_profile_cpu256_hi_id;
        cpu256_epp_cfg.bf.profile_sel_hi = l3qm_epp_profile_cpu256_hi_id;
        CA_NE_REG_WRITE(cpu256_epp_cfg.wrd, reg_off);

        epp256_hi_buf_cfg.wrd = CA_NE_REG_READ(QM_QM_DEST_CPU_EPP256_HI_BUF_CFG);
        epp256_hi_buf_cfg.bf.head_room_first = aal_l3qm_get_cpu_port_head_room_first() / 16;       /* reserved 32 bytes for first fragment of packet  */
        epp256_hi_buf_cfg.bf.head_room_rest = epp256_hi_buf_cfg.bf.head_room_first;       /* reserved 32 bytes for reset of segments of packet  */

        CA_NE_REG_WRITE(epp256_hi_buf_cfg.wrd, QM_QM_DEST_CPU_EPP256_HI_BUF_CFG);

        epp256_lo_buf_cfg.wrd = CA_NE_REG_READ(QM_QM_DEST_CPU_EPP256_LO_BUF_CFG);
        epp256_lo_buf_cfg.bf.head_room_first = aal_l3qm_get_cpu_port_head_room_first() / 16;       /* reserved 32 bytes for first fragment of packet  */
        epp256_lo_buf_cfg.bf.head_room_rest = epp256_lo_buf_cfg.bf.head_room_first;       /* reserved 32 bytes for reset of segments of packet  */
        CA_NE_REG_WRITE(epp256_lo_buf_cfg.wrd, QM_QM_DEST_CPU_EPP256_LO_BUF_CFG);

	/* additional profile settings for EPP256 */
	for (i = 0; i < QM_QM_CPU_EPP256_FIFO_CFG_profile0_COUNT; i++) {
		aal_l3qm_cpu256_epp256_config_profile_set(i, l3qm_cpu256_epp_per_voq / 4, 0xf);
	}

	/* additional profile settings for EPP256A, EPP AGGR mode */
	for (i = 0; i < QM_QM_CPU_EPP256_FIFO_CFG_profile0_COUNT; i++) {
		aal_l3qm_cpu256_epp256a_config_profile_set(i, (l3qm_cpu256_epp_per_voq * l3qm_cpu256_voq_per_port) / 4, 0xf);
	}

	for (i = 0; i < QM_QM_CPU_EPP256_FIFO0_CFG_COUNT; i++) {
		aal_l3qm_cpu256_epp256_profile_select(i, l3qm_epp256_fifo_cfg[i]);
	}

	for (i = 0; i < QM_QM_CPU_EPP256_FIFO_DROP_profile0_COUNT; i++) {
		aal_l3qm_cpu256_epp256_drop_profile_set(i, l3qm_epp256_fifo_drop_profile[i]);
	}

	/* in FPGA version QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0_COUNT=4 */
	/* but ASIC vesion QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0_COUNT=32 */
	for (i = 0; i < 32; i++) {
		aal_l3qm_cpu256_epp256a_drop_profile_set(i, l3qm_epp256a_fifo_drop_profile[i]);
	}

	ctrl.wrd = CA_NE_REG_READ(TE_TB2RATE_CTRL);
	ctrl.bf.type = 1;    /* RFC2698 */
	CA_NE_REG_WRITE(ctrl.wrd, TE_TB2RATE_CTRL);

	/* fill the recommended value from Joanna */
	tb_ctrl.wrd = CA_NE_REG_READ(TE_TB2RATE_TB_CTRL);
	tb_ctrl.bf.last_upd_cyc = 0x1bf;
	tb_ctrl.bf.cyc_cnt_lmt = 0x1f3;
	tb_ctrl.bf.upd_cnt_lmt = 0x6;
	tb_ctrl.bf.subtok_en = 1;
	tb_ctrl.bf.addtok_en = 1;
	CA_NE_REG_WRITE(tb_ctrl.wrd, TE_TB2RATE_TB_CTRL);

	tb_ctrl.wrd = CA_NE_REG_READ(TE_TB2RATE_TB_CTRL);
	printk("%s: TE_TB2RATE_TB_CTRL=0x%x, tb_ctrl.wrd=0x%x\n", __func__, TE_TB2RATE_TB_CTRL, tb_ctrl.wrd);

	/* fill the recommended value from Joanna */
	tb_ctrl1.wrd = CA_NE_REG_READ(TE_TB2RATE_TB_CTRL1);
	tb_ctrl1.bf.k_cnt_lmt = 0x3e7;
	CA_NE_REG_WRITE(tb_ctrl1.wrd, TE_TB2RATE_TB_CTRL1);

	tb_ctrl1.wrd = CA_NE_REG_READ(TE_TB2RATE_TB_CTRL1);
	printk("%s: TE_TB2RATE_TB_CTRL1=0x%x, tb_ctrl1.wrd=0x%x\n", __func__, TE_TB2RATE_TB_CTRL1, tb_ctrl1.wrd);

}

void aal_l3qm_init_empty_buffer_CPU256_hi(void)
{
        ca_uint32_t reg_off;
        QM_QM_EQ_PROFILE0_t eq_profile;
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

	/* if QM ACE not turned on, ignore the startup configure settings if acd_cmd=1 */
        cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_cpu256_hi_pool0_eq_id];
        if (!l3qm_ace_test && cfg3_eq.bf.acd_cmd == 1) {
                cfg3_eq.wrd = 0;
        }
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        /* because PE0 pool1 locates at DDR start from 0x180000000 */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool0_eq_id);
	cfg4_eq.wrd = 0;
	if (l3qm_ace_test) {
		cfg4_eq.bf.axi_top_bit= l3qm_eq_cfg4_values[l3qm_eq_profile_cpu256_hi_pool0_eq_id];
	}
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

        /* configure EB pool 1 for CPU256 hi */
        /* set phy_addr_start and enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
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

	/* if QM ACE not turned on, ignore the startup configure settings if acd_cmd=1 */
	cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_cpu256_hi_pool1_eq_id];
	if (!l3qm_ace_test && cfg3_eq.bf.acd_cmd == 1) {
		cfg3_eq.wrd = 0;
	}
	CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	/* because PE0 pool1 locates at DDR start from 0x180000000 */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu256_hi_pool1_eq_id);
	cfg4_eq.wrd = 0;
	if (l3qm_ace_test) {
		cfg4_eq.bf.axi_top_bit= l3qm_eq_cfg4_values[l3qm_eq_profile_cpu256_hi_pool1_eq_id];
	}
	CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);
}

void aal_l3qm_init_empty_buffer_CPU256_lo(void)
{
	ca_uint32_t reg_off;
	QM_QM_EQ_PROFILE0_t eq_profile;
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

	/* if QM ACE not turned on, ignore the startup configure settings if acd_cmd=1 */
	cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_cpu256_lo_pool0_eq_id];
	if (!l3qm_ace_test && cfg3_eq.bf.acd_cmd == 1) {
		cfg3_eq.wrd = 0;
	}
	CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	/* because PE0 pool1 locates at DDR start from 0x180000000 */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool0_eq_id);
	cfg4_eq.wrd = 0;
	if (l3qm_ace_test) {
		cfg4_eq.bf.axi_top_bit= l3qm_eq_cfg4_values[l3qm_eq_profile_cpu256_lo_pool0_eq_id];
	}
	CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

	/* configure EB pool 1 for CPU256 hi */
	/* set phy_addr_start and enable Empty Buffer */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
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

	/* if QM ACE not turned on, ignore the startup configure settings if acd_cmd=1 */
	cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_cpu256_lo_pool1_eq_id];
	if (!l3qm_ace_test && cfg3_eq.bf.acd_cmd == 1) {
		cfg3_eq.wrd = 0;
	}
	CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	/* because PE0 pool1 locates at DDR start from 0x180000000 */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu256_lo_pool1_eq_id);
	cfg4_eq.wrd = 0;
	if (l3qm_ace_test) {
		cfg4_eq.bf.axi_top_bit= l3qm_eq_cfg4_values[l3qm_eq_profile_cpu256_lo_pool1_eq_id];
	}
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
        printk("CPU256 hi pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool0_buf_count * l3qm_eq_profile_cpu256_hi_pool0_buf_sz);

        printk("CPU256 hi pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool1_start_addr);
        printk("CPU256 hi pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_hi_pool1_phy_addr);
        printk("CPU256 hi pool1 buffer count = %d\n", l3qm_eq_profile_cpu256_hi_pool1_buf_count);
        printk("CPU256 hi pool1 buffer size = %d\n", l3qm_eq_profile_cpu256_hi_pool1_buf_sz);
        printk("CPU256 hi pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool1_buf_count * l3qm_eq_profile_cpu256_hi_pool1_buf_sz);

        printk("CPU256 hi RX FIFO start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr);
        printk("CPU256 hi RX FIFO physical addr = 0x%x\n", l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr);
        printk("CPU256 hi RX FIFO size = %d\n", l3qm_epp_profile_cpu256_hi_sz);
        printk("CPU256 hi RX FIFO count = %d\n", l3qm_epp_profile_cpu256_hi_cn);
        printk("CPU256 hi RX FIFO consumed size = %lld\n", (ca_uint64_t)l3qm_epp_profile_cpu256_hi_sz * l3qm_epp_profile_cpu256_hi_cn);

        printk("CPU256 lo pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool0_start_addr);
        printk("CPU256 lo pool0 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_lo_pool0_phy_addr);
        printk("CPU256 lo pool0 buffer count = %d\n", l3qm_eq_profile_cpu256_lo_pool0_buf_count);
        printk("CPU256 lo pool0 buffer size = %d\n", l3qm_eq_profile_cpu256_lo_pool0_buf_sz);
        printk("CPU256 lo pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool0_buf_count * l3qm_eq_profile_cpu256_lo_pool0_buf_sz);

        printk("CPU256 lo pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool1_start_addr);
        printk("CPU256 lo pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_lo_pool1_phy_addr);
        printk("CPU256 lo pool1 buffer count = %d\n", l3qm_eq_profile_cpu256_lo_pool1_buf_count);
        printk("CPU256 lo pool1 buffer size = %d\n", l3qm_eq_profile_cpu256_lo_pool1_buf_sz);
        printk("CPU256 lo pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool1_buf_count * l3qm_eq_profile_cpu256_lo_pool1_buf_sz);

        printk("CPU256 lo RX FIFO start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr);
        printk("CPU256 lo RX FIFO physical addr = 0x%x\n", l3qm_epp_profile_cpu256_lo_rx_fifo_phy_addr);
        printk("CPU256 lo RX FIFO size = %d\n", l3qm_epp_profile_cpu256_lo_sz);
        printk("CPU256 lo RX FIFO count = %d\n", l3qm_epp_profile_cpu256_lo_cn);
        printk("CPU256 lo RX FIFO consumed size = %d\n", l3qm_epp_profile_cpu256_lo_sz * l3qm_epp_profile_cpu256_lo_cn);

}

ca_uint32_t aal_l3qm_create_cpu256_ddr_map(void)
{
	ca_uint32_t size = 0;

	/* CPU256 hi pool0 buffer */
        l3qm_eq_profile_cpu256_hi_pool0_start_addr = ((l3qm_eq_profile_pe1_pool1_buff_start_addr + size + l3qm_eq_buffer_align_size) /
                                                        l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        l3qm_eq_profile_cpu256_hi_pool0_phy_addr = ((l3qm_eq_profile_pe1_pool1_buff_phy_addr + size + l3qm_eq_buffer_align_size) /
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
	int i;
	int ret;

	ret = aal_scfg_read(CFG_ID_L3QM_CPU256_VOQ_PER_PORT, 1, &l3qm_cpu256_voq_per_port);
	aal_scfg_read(CFG_ID_L3QM_CPU256_EPP_PER_VOQ, 4, &l3qm_cpu256_epp_per_voq);
	aal_scfg_read(CFG_ID_L3QM_CPU256_POOL_BID_COUNT, 4, &l3qm_cpu256_pool_bid_count);

	printk("%s: l3qm_cpu256_voq_per_port=%d, ret=%d\n", __func__, l3qm_cpu256_voq_per_port, ret);

        /* read CPU256 hi configure */
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_ID, 1, &l3qm_eq_profile_cpu256_hi_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_RULE, 1, &l3qm_eq_profile_cpu256_hi_rule);

        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_USE_FBM, 1, &l3qm_eq_profile_cpu256_hi_use_fbm);
#if defined(CONFIG_CORTINA_BOARD_FPGA)
        /* if no FBM module included the always set l3qm_eq_profile_cpu256_hi_use_fbm=0 */
        if (SOC_HAS_FBM() == 0) {
                l3qm_eq_profile_cpu256_hi_use_fbm = 0;
                printk("%s: set l3qm_eq_profile_cpu256_hi_use_fbm=0 because no FBM module\n", __func__);
        }
#endif
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_EQ_ID, 1, &l3qm_eq_profile_cpu256_hi_pool0_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_hi_pool0_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BID_START, 4, &l3qm_eq_profile_cpu256_hi_pool0_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_SZ, 4, &l3qm_eq_profile_cpu256_hi_pool0_buf_sz);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_hi_pool0_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_EQ_ID, 1, &l3qm_eq_profile_cpu256_hi_pool1_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_hi_pool1_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BID_START, 4, &l3qm_eq_profile_cpu256_hi_pool1_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BUF_SZ, 4, &l3qm_eq_profile_cpu256_hi_pool1_buf_sz);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_hi_pool1_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_ID, 1, &l3qm_epp_profile_cpu256_hi_id);

        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_MAP_MODE, 4, &l3qm_epp_profile_cpu256_hi_map_mode);

        /* read CPU256 lo configure */
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_ID, 1, &l3qm_eq_profile_cpu256_lo_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_RULE, 1, &l3qm_eq_profile_cpu256_lo_rule);

        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_USE_FBM, 1, &l3qm_eq_profile_cpu256_lo_use_fbm);
#if defined(CONFIG_CORTINA_BOARD_FPGA)
        /* if no FBM module included the always set l3qm_eq_profile_cpu256_lo_use_fbm=0 */
        if (SOC_HAS_FBM() == 0) {
                l3qm_eq_profile_cpu256_lo_use_fbm = 0;
                printk("%s: set l3qm_eq_profile_cpu256_lo_use_fbm=0 because no FBM module\n", __func__);
        }
#endif
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_EQ_ID, 1, &l3qm_eq_profile_cpu256_lo_pool0_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_lo_pool0_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BID_START, 4, &l3qm_eq_profile_cpu256_lo_pool0_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_SZ, 4, &l3qm_eq_profile_cpu256_lo_pool0_buf_sz);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_lo_pool0_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_EQ_ID, 1, &l3qm_eq_profile_cpu256_lo_pool1_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_lo_pool1_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BID_START, 4, &l3qm_eq_profile_cpu256_lo_pool1_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BUF_SZ, 4, &l3qm_eq_profile_cpu256_lo_pool1_buf_sz);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_lo_pool1_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_ID, 1, &l3qm_epp_profile_cpu256_lo_id);

        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_MAP_MODE, 4, &l3qm_epp_profile_cpu256_lo_map_mode);

	aal_scfg_read(CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES, CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES_LEN * sizeof(ca_uint32_t), l3qm_axi_attrib_cpu256_epp_values);

	l3qm_epp_profile_cpu256_hi_total_epp_count = ((l3qm_cpu256_port_count / 2) * l3qm_cpu256_voq_per_port * l3qm_cpu256_epp_per_voq);  /* CPU256 total EPP count */

	/* EPP size is the same as CPU64's */
	l3qm_epp_profile_cpu256_hi_sz = l3qm_epp_profile_cpu256_lo_sz = l3qm_epp_size;

	/* calculate total EPP count for CPU256 HI/LO from l3qm_cpu256_epp_per_voq */
	l3qm_epp_profile_cpu256_lo_cn = l3qm_epp_profile_cpu256_hi_cn = l3qm_cpu256_epp_per_voq * (l3qm_cpu256_port_count / 2) * l3qm_cpu256_voq_per_port;
	
	printk("%s: l3qm_epp_profile_cpu256_lo_cn=%d, l3qm_cpu256_epp_per_voq=%d, l3qm_cpu256_port_count=%d, l3qm_cpu256_voq_per_port=%d\n",
		__func__, l3qm_epp_profile_cpu256_lo_cn, l3qm_cpu256_epp_per_voq, l3qm_cpu256_port_count, l3qm_cpu256_voq_per_port);

	aal_scfg_read(CFG_ID_L3QM_CPU256_EPP_CPU_VOQ_MAP, 1, &l3qm_cpu256_epp_cpu_voq_map);
	aal_scfg_read(CFG_ID_L3QM_CPU256_EPP_AGGR_ENABLE, 1, &l3qm_cpu256_epp_aggr_enable);

	printk("%s: l3qm_cpu256_epp_cpu_voq_map=%d, l3qm_cpu256_epp_aggr_enable=%d\n", __func__,
		l3qm_cpu256_epp_cpu_voq_map, l3qm_cpu256_epp_aggr_enable);

	aal_scfg_read(CFG_ID_QM_QM_CPU_EPP256A_FIFO_DROP_profile_levels,
			CFG_ID_QM_QM_CPU_EPP256A_FIFO_DROP_profile_levels_LEN * sizeof(ca_uint32_t), &l3qm_epp256a_fifo_drop_profile[0]);
	aal_scfg_read(CFG_ID_QM_QM_CPU_EPP256_FIFO_DROP_profile_levels,
			CFG_ID_QM_QM_CPU_EPP256_FIFO_DROP_profile_levels_LEN * sizeof(ca_uint32_t), &l3qm_epp256_fifo_drop_profile[0]);
	aal_scfg_read(CFG_ID_QM_QM_CPU_EPP256_FIFO_CFG, CFG_ID_QM_QM_CPU_EPP256_FIFO_CFG_LEN * sizeof(ca_uint32_t), &l3qm_epp256_fifo_cfg[0]);

	printk("%s: CFG_ID_QM_QM_CPU__EPP256A_FIFO_DROP_profile_levels=", __func__);
	for (i = 0; i < 32; i++) {
		printk("0x%x-", l3qm_epp256a_fifo_drop_profile[i]);
	}
	printk("\n");

	printk("%s: CFG_ID_QM_QM_CPU_EPP256_FIFO_DROP_profile_levels=", __func__);
	for (i = 0; i < 4; i++) {
		printk("0x%x-", l3qm_epp256_fifo_drop_profile[i]);
	}
	printk("\n");

	printk("%s: CFG_ID_QM_QM_CPU_EPP256_FIFO_CFG=", __func__);
	for (i = 0; i < 32; i++) {
		printk("0x%x-", l3qm_epp256_fifo_cfg[i]);
	}
	printk("\n");
}

static void aal_l3qm_dump_cpuepp256_fifo_paddr_start(int aggr_mode, int map_mode)
{
        int i, j, map_port_count = 0;
        ca_uint32_t reg_off;
        QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_t cpu_epp256_fifo_paddr_start;
	QM_QM_CPU_EPP_FIFO_256_CFG_t    cpu256_epp_cfg;

        printk("=== QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START/QM_QM_CPU_EPP_FIFO_256_CFG ===\n");

	/* read EPP256 profile id */
	reg_off = QM_QM_CPU_EPP_FIFO_256_CFG;
	cpu256_epp_cfg.wrd = CA_NE_REG_READ(reg_off);

	if (aggr_mode) {
		if (map_mode) {
			map_port_count = 8;
		}
		for (j = 0; j < l3qm_cpu256_port_count + map_port_count; j++) {
			reg_off = QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START +
				(j * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE);
			cpu_epp256_fifo_paddr_start.wrd = CA_NE_REG_READ(reg_off);

			printk("voq id=%d, reg_off=0x%x, paddr_start=0x%x, profile sel lo=%d, profile sel hi=%d\n",
					j, reg_off, cpu_epp256_fifo_paddr_start.bf.phy_addr, cpu256_epp_cfg.bf.profile_sel_lo,
					cpu256_epp_cfg.bf.profile_sel_hi);
		}
	}
	else {
		for (j = 0; j < l3qm_cpu256_port_count; j++) {
			for (i = 0; i < l3qm_cpu256_voq_per_port; i++) {
				reg_off = QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START +
					  (i * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE) +
					  (j * l3qm_cpu256_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE);
				cpu_epp256_fifo_paddr_start.wrd = CA_NE_REG_READ(reg_off);

				printk("port id=%d, voq id=%d, reg_off=0x%x, paddr_start=0x%x, profile sel lo=%d, profile sel hi=%d\n",
						j, i, reg_off, cpu_epp256_fifo_paddr_start.bf.phy_addr, cpu256_epp_cfg.bf.profile_sel_lo,
						cpu256_epp_cfg.bf.profile_sel_hi);
			}
		}
	}
}

void aal_l3qm_dump_cpu256_epps(void)
{
	int j;
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP256A_FIFO_CFG_profile0_t cpu_epp256a_fifo_cfg;

        printk("=== QM_QM_CPU_EPP256A_FIFO_CFG_profile0 ===\n");
        for (j = 0; j < QM_QM_CPU_EPP_FIFO0_0_CFG_COUNT; j++) {

                        reg_off = QM_QM_CPU_EPP256A_FIFO_CFG_profile0  + (QM_QM_CPU_EPP256A_FIFO_CFG_profile0_STRIDE * j);
                        cpu_epp256a_fifo_cfg.wrd = CA_NE_REG_READ(reg_off);

			printk("profile id=%d, reg_off=0x%x, size=%d\n", j, reg_off, cpu_epp256a_fifo_cfg.bf.size);
        }

	aal_l3qm_dump_cpuepp256_fifo_paddr_start(l3qm_cpu256_epp_aggr_enable, l3qm_cpu256_epp_cpu_voq_map);
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

void aal_l3qm_cpu256_drop_counter_map(ca_uint16_t voq, ca_uint16_t counter_id, ca_uint8_t valid);
void aal_l3qm_init_cpu256_empty_buffer(void)
{
	ca_uint16_t voq, counter_id;

        aal_l3qm_init_empty_buffer_CPU256_hi();
        aal_l3qm_init_empty_buffer_CPU256_lo();
        spin_lock_init(&l3qm_epp256_inte_lock);

	ca_l3qm_cpu256_configure_epp();

	counter_id = 0;
	for (voq = 0; voq < 256; voq++) {
		aal_l3qm_cpu256_drop_counter_map(voq, counter_id, 1);
		counter_id++;
		if (counter_id >= 64) counter_id = 0;
	}
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

/* Provide all of the APIs for new features of Venus */
#include "aal_phy.h"

void aal_l3qm_cpu256_epp256_profile_select(ca_uint16_t cpu_port, ca_uint32_t profile)
{
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP256_FIFO0_CFG_t	epp256_fifo;

	reg_off = QM_QM_CPU_EPP256_FIFO0_CFG + (cpu_port * QM_QM_CPU_EPP256_FIFO0_CFG_STRIDE);
	epp256_fifo.wrd = profile;
	CA_NE_REG_WRITE(epp256_fifo.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		epp256_fifo.wrd = CA_NE_REG_READ(reg_off);
		printk("%s: reg_off=0x%x, cpu_port=%d, profile=0x%x, epp256_fifo.wrd=0x%x\n",
			__func__, reg_off, cpu_port, profile, epp256_fifo.wrd);
	}
}

void aal_l3qm_cpu256_epp256_config_profile_set(ca_uint8_t profile, ca_uint32_t size, ca_uint8_t timer_ths)
{
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP256_FIFO_CFG_profile0_t	epp256_profile;

	reg_off = QM_QM_CPU_EPP256_FIFO_CFG_profile0 + (profile * QM_QM_CPU_EPP256_FIFO_CFG_profile0_STRIDE);
	epp256_profile.wrd = CA_NE_REG_READ(reg_off);
	epp256_profile.bf.size = size;
	epp256_profile.bf.timer_ths = timer_ths;
	if ((ca_ni_napi_budget / 16) > 0) {
		/* Interrupt to CPU is generated when EPP-FIFO accumulates at least ca_napi_budget entries */
		epp256_profile.bf.high_ths = (ca_ni_napi_budget / 16) + 1;
	}
	else {
		/* Interrupt to CPU is generated when EPP-FIFO accumulates at least 16 entries */
		epp256_profile.bf.high_ths = 2;
	}

	CA_NE_REG_WRITE(epp256_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		epp256_profile.wrd = CA_NE_REG_READ(reg_off);
		printk("%s: profile=%d, size=%d, timer_ths=%d, epp256_profile.wrd=0x%x\n",
				__func__, profile, size, (ca_uint32_t)timer_ths, epp256_profile.wrd);
	}
}

void aal_l3qm_cpu256_epp256_drop_profile_set(ca_uint8_t profile, ca_uint32_t drop_level)
{
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP256_FIFO_DROP_profile0_t    epp256_drop_profile;

	reg_off = QM_QM_CPU_EPP256_FIFO_DROP_profile0 + (profile * QM_QM_CPU_EPP256_FIFO_DROP_profile0_STRIDE);
	epp256_drop_profile.wrd = CA_NE_REG_READ(reg_off);
	epp256_drop_profile.bf.drop_level = drop_level;
	CA_NE_REG_WRITE(epp256_drop_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		epp256_drop_profile.wrd = CA_NE_REG_READ(reg_off);
		printk("%s: profile=%d, drop_level=%d, epp256_drop_profile.wrd=0x%x\n", __func__, profile, drop_level, epp256_drop_profile.wrd);
	}
}

void aal_l3qm_cpu256_epp256a_config_profile_set(ca_uint8_t profile, ca_uint32_t size, ca_uint8_t timer_ths)
{
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP256A_FIFO_CFG_profile0_t    epp256a_profile;

	reg_off = QM_QM_CPU_EPP256A_FIFO_CFG_profile0 + (profile * QM_QM_CPU_EPP256A_FIFO_CFG_profile0_STRIDE);
	epp256a_profile.wrd = CA_NE_REG_READ(reg_off);
	epp256a_profile.bf.size = size;
	epp256a_profile.bf.timer_ths = timer_ths;
	CA_NE_REG_WRITE(epp256a_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		epp256a_profile.wrd = CA_NE_REG_READ(reg_off);
		printk("%s: profile=%d, size=%d, timer_ths=%d, epp256a_profile.wrd=0x%x\n",
				__func__, profile, size, (ca_uint32_t)timer_ths, epp256a_profile.wrd);
	}
}

void aal_l3qm_cpu256_epp256a_drop_profile_set(ca_uint8_t profile, ca_uint32_t drop_level)
{
	ca_uint8_t drop_no, profile_no;
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0_t    epp256a_drop_profile;

	/* in FPGA QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0_STRIDE=36  */
	/* QM_QM_CPU_EPP256A_FIFO_DROP_%e_profile%d */
	/* Addresses: base=[0xf4306700]) + (36 * (%d=[0..3])) + (4 * %e=[0..7]) */
	profile_no = profile / 8;
	drop_no = profile % 8;
	reg_off = QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0 + (QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0_STRIDE * profile_no) + (drop_no * 4);

	epp256a_drop_profile.wrd = CA_NE_REG_READ(reg_off);
	epp256a_drop_profile.bf.drop_level_0 = drop_level;
	CA_NE_REG_WRITE(epp256a_drop_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		epp256a_drop_profile.wrd = CA_NE_REG_READ(reg_off);
		printk("%s: reg_off=0x%x, profile=%d, drop_level=0x%x, epp256a_drop_profile.bf.drop_level_0=0x%x\n",
				__func__, reg_off, profile, drop_level, epp256a_drop_profile.bf.drop_level_0);
	}
}

void aal_l3qm_cpu256_buffer_cntl_set(ca_uint8_t jumbo_buffer_enable, ca_uint8_t port_speed_select)
{
	QM_QM_BUFFER_CNTL_t	buf_cntl;

	buf_cntl.wrd = CA_NE_REG_READ(QM_QM_BUFFER_CNTL);
	buf_cntl.bf.jumbo_buffer_en = jumbo_buffer_enable;
	buf_cntl.bf.port_speed_select = port_speed_select;
	CA_NE_REG_WRITE(buf_cntl.wrd, QM_QM_BUFFER_CNTL);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		buf_cntl.wrd = CA_NE_REG_READ(QM_QM_BUFFER_CNTL);
		printk("%s: jumbo_buffer_enable=%d, port_speed_select=%d, buf_cntl.wrd=0x%x\n",
				__func__, jumbo_buffer_enable, port_speed_select, buf_cntl.wrd);
	}
}

void aal_l3qm_cpu256_latency_timer_set(ca_uint8_t ni_port, aal_phy_speed_mode_t speed, ca_uint8_t jumbo_buffer_enable)
{
	QM_QM_LATENCY_TIMER_CFG_t	latency_timer;

	if (jumbo_buffer_enable == 0) {
		printk("%s: Jumbo buffer is disabled!!\n", __func__);
		return;
	}
	if (ni_port > 7) {
		printk("%s: NI port %d exceeds 7!!\n", __func__, ni_port);
		return;
	}

	latency_timer.wrd = CA_NE_REG_READ(QM_QM_LATENCY_TIMER_CFG);
	switch (speed) {
		case AAL_PHY_SPEED_1000:
		case AAL_PHY_SPEED_10000:
			if (jumbo_buffer_enable == 3) {
				latency_timer.bf.ths1 = 255;
			}
			else if (jumbo_buffer_enable == 2) {
				latency_timer.bf.ths1 = 127;
			}
			else if (jumbo_buffer_enable == 1) {
				latency_timer.bf.ths1 = 63;
			}
			break;
		case AAL_PHY_SPEED_2500:
			if (jumbo_buffer_enable == 3) {
				latency_timer.bf.ths1 = 63;
			}
			else if (jumbo_buffer_enable == 2) {
				latency_timer.bf.ths1 = 31;
			}
			else if (jumbo_buffer_enable == 1) {
				latency_timer.bf.ths1 = 15;
			}
			break;
		case AAL_PHY_SPEED_5000:
			if (jumbo_buffer_enable == 3) {
				latency_timer.bf.ths1 = 127;
			}
			else if (jumbo_buffer_enable == 2) {
				latency_timer.bf.ths1 = 63;
			}
			else if (jumbo_buffer_enable == 1) {
				latency_timer.bf.ths1 = 31;
			}
			break;
		default:
			printk("%s: speed=%d does not support!!\n", __func__, speed);
			return;
	}

	if (speed == AAL_PHY_SPEED_5000 || speed == AAL_PHY_SPEED_2500) {
		switch (ni_port) {
			case 0:
				latency_timer.bf.timer_profile_sel_ni0 = 1;
				break;
			case 1:
				latency_timer.bf.timer_profile_sel_ni1 = 1;
				break;
			case 2:
				latency_timer.bf.timer_profile_sel_ni2 = 1;
				break;
			case 3:
				latency_timer.bf.timer_profile_sel_ni3 = 1;
				break;
			case 4:
				latency_timer.bf.timer_profile_sel_ni4 = 1;
				break;
			case 5:
				latency_timer.bf.timer_profile_sel_ni5 = 1;
				break;
			case 6:
				latency_timer.bf.timer_profile_sel_ni6 = 1;
				break;
			case 7:
				latency_timer.bf.timer_profile_sel_ni7 = 1;
				break;
		}
	}
	CA_NE_REG_WRITE(latency_timer.wrd, QM_QM_LATENCY_TIMER_CFG);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: ni_port=%d, speed=%d, jumbo_buffer_enable=%d, latency_timer.wrd=%d\n",
			__func__, ni_port, speed, jumbo_buffer_enable, latency_timer.wrd);
	}
}

void aal_l3qm_cpu256_upper_ldpid_map_set(ca_uint8_t pon_enable, ca_uint8_t pon_offset)
{
	QM_QM_UPPER_LDPID_MAP_t	map;

	map.wrd = CA_NE_REG_READ(QM_QM_UPPER_LDPID_MAP);
	map.bf.pon_en = pon_enable;
	map.bf.pon_offset = pon_offset;
	CA_NE_REG_WRITE(map.wrd, QM_QM_UPPER_LDPID_MAP);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: pon_enable=%d, pon_offset=%d, map.wrd=0x%x\n", __func__, pon_enable, pon_offset, map.wrd);
	}
}

void aal_l3qm_cpu256_schedule_mode_set(ca_uint16_t port)
{
	ca_uint32_t	target_bit;
	QM_QM_CPU_EPP_256_SCH_MODE_t	sch_mode;

	sch_mode.wrd = CA_NE_REG_READ(QM_QM_CPU_EPP_256_SCH_MODE);
	target_bit = (port / 8);
	sch_mode.wrd = (sch_mode.wrd & ~(1 << (target_bit))) | (1 << (target_bit));
	CA_NE_REG_WRITE(sch_mode.wrd, QM_QM_CPU_EPP_256_SCH_MODE);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: port=%d, sch_mode.wrd=0x%x\n", __func__, port, sch_mode.wrd);
	}
}

#define CPU256_DROP_COUNTER_TIMEOUT	100
void aal_l3qm_cpu256_drop_counter_map(ca_uint16_t voq, ca_uint16_t counter_id, ca_uint8_t valid)
{
        int i;
	QM_QM_EPP256_MAP_ACCESS_t	access;
	QM_QM_EPP256_MAP_DATA_t		data;

	data.wrd = 0;
	data.bf.cntr_valid = valid;
	data.bf.cntr_id = counter_id;
	CA_NE_REG_WRITE(data.wrd, QM_QM_EPP256_MAP_DATA);

        access.wrd = 0;
        access.bf.access = 1;
        access.bf.rbw = 1;
        access.bf.ADDR = voq;
        CA_NE_REG_WRITE(access.wrd, QM_QM_EPP256_MAP_ACCESS);

        for (i = 0; i < CPU256_DROP_COUNTER_TIMEOUT; i++) {
                access.wrd = CA_NE_REG_READ(QM_QM_EPP256_MAP_ACCESS);
                if (access.bf.access == 0) {
                        break;
                }
        }
        if (i == CPU256_DROP_COUNTER_TIMEOUT) {
                printk("%s: QM_QM_EPP256_MAP_ACCESS timeout, voq=%d!!\n", __func__, voq);
                return;
        }

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: voq=%d, counter_id=%d, valid=%d\n", __func__, voq, counter_id, valid);
	}
}

void aal_l3qm_cpu256_drop_counter_map_dump(void)
{
        int i;
	ca_uint16_t voq;
        QM_QM_EPP256_MAP_ACCESS_t       access;
        QM_QM_EPP256_MAP_DATA_t         data;

	for (voq = 0; voq < 256; voq++) {

		access.wrd = 0;
		access.bf.access = 1;
		access.bf.rbw = 0;
		access.bf.ADDR = voq;
		CA_NE_REG_WRITE(access.wrd, QM_QM_EPP256_MAP_ACCESS);

		for (i = 0; i < CPU256_DROP_COUNTER_TIMEOUT; i++) {
			access.wrd = CA_NE_REG_READ(QM_QM_EPP256_MAP_ACCESS);
			if (access.bf.access == 0) {
				break;
			}
		}
		if (i == CPU256_DROP_COUNTER_TIMEOUT) {
			printk("%s: QM_QM_EPP256_MAP_ACCESS timeout, voq=%d!!\n", __func__, voq);
			return;
		}

		data.wrd = CA_NE_REG_READ(QM_QM_EPP256_MAP_DATA);
		printk("[voq %d]: counter_id=%d, valid=%d\n", voq, data.bf.cntr_id, data.bf.cntr_valid);
	}
}

ca_status_t aal_l3qm_cpu256_drop_counter_dump(ca_uint16_t counter_id)
{
	int i;
	ca_uint64_t byte_count;
	ca_uint32_t pkt_count;
	QM_QM_EPP_DROP_CNTR_ACCESS_t access;
	QM_QM_EPP_DROP_CNTR_DATA0_t  data0;
	QM_QM_EPP_DROP_CNTR_DATA1_t  data1;
	QM_QM_EPP_DROP_CNTR_DATA2_t  data2;

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.ADDR = counter_id;
	CA_NE_REG_WRITE(access.wrd, QM_QM_EPP_DROP_CNTR_ACCESS);

	for (i = 0; i < CPU256_DROP_COUNTER_TIMEOUT; i++) {
		access.wrd = CA_NE_REG_READ(QM_QM_EPP_DROP_CNTR_ACCESS);
		if (access.bf.access == 0) {
			break;
		}
	}
	if (i == CPU256_DROP_COUNTER_TIMEOUT) {
		printk("%s: QM_QM_EPP_DROP_CNTR_ACCESS timeout, counter_id=%d!!\n", __func__, counter_id);
		return CA_E_ERROR;
	}
	data0.wrd = CA_NE_REG_READ(QM_QM_EPP_DROP_CNTR_DATA0);
	data1.wrd = CA_NE_REG_READ(QM_QM_EPP_DROP_CNTR_DATA1);
	data2.wrd = CA_NE_REG_READ(QM_QM_EPP_DROP_CNTR_DATA2);

	pkt_count = data0.wrd;
	byte_count = ((ca_uint64_t)data2.wrd << 32) + data1.wrd;
	printk("%s: Counter ID=%d, drop packet count=%d, drop byte count=%llu\n", __func__, counter_id, pkt_count, byte_count);
	return CA_E_OK;
}

ca_status_t aal_l3qm_cpu256_drop_counter_clear(ca_uint16_t counter_id)
{
	int i;
	QM_QM_EPP_DROP_CNTR_ACCESS_t access;

	CA_NE_REG_WRITE(0, QM_QM_EPP_DROP_CNTR_DATA0);
	CA_NE_REG_WRITE(0, QM_QM_EPP_DROP_CNTR_DATA1);
	CA_NE_REG_WRITE(0, QM_QM_EPP_DROP_CNTR_DATA2);

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 1;
	access.bf.ADDR = counter_id;
	CA_NE_REG_WRITE(access.wrd, QM_QM_EPP_DROP_CNTR_ACCESS);

	for (i = 0; i < CPU256_DROP_COUNTER_TIMEOUT; i++) {
		access.wrd = CA_NE_REG_READ(QM_QM_EPP_DROP_CNTR_ACCESS);
		if (access.bf.access == 0) {
			break;
		}
	}
	if (i == CPU256_DROP_COUNTER_TIMEOUT) {
		printk("%s: QM_QM_EPP_DROP_CNTR_ACCESS timeout, counter_id=%d!!\n", __func__, counter_id);
		return CA_E_ERROR;
	}

	return CA_E_OK;
}

ca_status_t ca_l3qm_cpu256_configure_epp(void)
{
	QM_QM_EPP_t	epp;

	epp.wrd = CA_NE_REG_READ(QM_QM_EPP);
	epp.bf.aggr_mode = l3qm_cpu256_epp_aggr_enable;
	epp.bf.map = l3qm_cpu256_epp_cpu_voq_map;
	CA_NE_REG_WRITE(epp.wrd, QM_QM_EPP);

	printk("%s: QM_QM_EPP=0x%x\n", __func__, epp.wrd);

	return CA_E_OK;
}

void aal_l3qm_cpu256_fifo_dump(void)
{
	int i;
	QM_QM_CPU_EPP256_FIFO0_CFG_t	config;

	printk("QM_QM_CPU_EPP256_FIFO0_CFG Register dump..\n");
	for (i = 0; i < QM_QM_CPU_EPP256_FIFO0_CFG_COUNT; i++) {
		config.wrd = CA_NE_REG_READ(QM_QM_CPU_EPP256_FIFO0_CFG + (i * QM_QM_CPU_EPP256_FIFO0_CFG_STRIDE));
		printk("CPU256 port=%d, cos0=%d, cos1=%d, cos2=%d, cos3=%d, cos4=%d, cos5=%d, cos6=%d, cos7=%d\n",
			i, config.bf.profile_sel_0, config.bf.profile_sel_1, config.bf.profile_sel_2,
			config.bf.profile_sel_3, config.bf.profile_sel_4, config.bf.profile_sel_5,
			config.bf.profile_sel_6, config.bf.profile_sel_7);
	}
}

void aal_l3qm_cpu256a_fifo_drop_profile_dump(void)
{
	ca_uint8_t drop_no, profile_no;
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0_t    epp256a_drop_profile;

	/* in FPGA QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0_STRIDE=36  */
	/* QM_QM_CPU_EPP256A_FIFO_DROP_%e_profile%d */
	/* Addresses: base=[0xf4306700]) + (36 * (%d=[0..3])) + (4 * %e=[0..7]) */
	printk("QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0 Register dump..\n");
	for (profile_no = 0; profile_no < 4; profile_no++) {
		printk("[profile %d]\n", profile_no);
		for (drop_no = 0; drop_no < 8; drop_no++) {
			reg_off = QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0 + (QM_QM_CPU_EPP256A_FIFO_DROP_0_profile0_STRIDE * profile_no)
				  + (drop_no * 4);
			epp256a_drop_profile.wrd = CA_NE_REG_READ(reg_off);
			printk("\t\tcos %d=%d\n", drop_no, epp256a_drop_profile.bf.drop_level_0);
		}
	}
}

#define COMMITTED_RATE_M_MASK                   0x03FFFC00      /* bit[25:10] */
#define COMMITTED_BURST_BURST_SIZE_MASK_0       0x0000003F      /* bit[31:26] */
#define COMMITTED_BURST_BURST_SIZE_MASK_1       0x00003FC0      /* bit[39:32] */
#define PEAK_RATE_K_MASK                        0x000003FF      /* bit[49:40] */
#define PEAK_RATE_M_MASK_0                      0xFFFC0000      /* bit[63:50] */
#define PEAK_RATE_M_MASK_1                      0x00000003      /* bit[65:64] */
#define PEAK_BURST_SIZE_MASK                    0x00001FFF      /* bit[79:66] */

typedef enum {
        SHAPER_KBPS,
        SHAPER_MBPS
}shaper_granularity_t;

#ifdef CA_L3TE_SHAPER_TEST
static void aal_l3te_add_shaper(void)
{
	ca_uint32_t                     reg_off;
        TE_TB2RATE_PROFILE_MEM0_t       mem0;
        TE_TB2RATE_PROFILE_MEM1_t       mem1;
        TE_TB2RATE_PROFILE_MEM2_t       mem2;
	ca_uint32_t cir, commited_burst, pir, peak_burst;
	ca_uint32_t shaper_id;
	shaper_granularity_t granularity;
	te_tb2rate_profile_mem_t profile_mem;
	te_tb2rate_counter_mem_t counter_mem;
	TE_TB2RATE_COUNTER_MEM1_t	counter_mem1;
	TE_TB2RATE_COUNTER_MEM0_t	counter_mem0;

	if ((CA_NE_REG_READ(TE_TB2RATE_STS) & 0x80000000) == 0) {
		printk("%s: <<<<<<<<<<<<<<<<TE SRAM not ready yet>>>>>>>>>>>\n", __func__);
	}
	else {
		printk("%s: [[[[[[[[[[[[[[[TE SRAM is ready]]]]]]]]]]]]]]]\n", __func__);
	}

	memset(&profile_mem, 0, sizeof(te_tb2rate_profile_mem_t));
	profile_mem.bits.committed_rate_k = 100;
	profile_mem.bits.committed_bs = 1000;
	profile_mem.bits.peak_rate_k = 500;
	profile_mem.bits.peak_bs = 1000;
	printk("%s: profile_mem.wrd[0]=0x%x, profile_mem.wrd[1]=0x%x, profile_mem.wrd[2]=0x%x\n", __func__,
			profile_mem.wrd[0], profile_mem.wrd[1], profile_mem.wrd[2]);

	mem0.wrd = profile_mem.wrd[0];
	mem1.wrd = profile_mem.wrd[1];
	mem2.wrd = profile_mem.wrd[2];

	shaper_id = 0;
	granularity = SHAPER_KBPS;

	/* the write sequency should be TE_TB2RATE_PROFILE_MEM2/1/0 */
	if (granularity == SHAPER_KBPS) {
		reg_off = TE_TB2RATE_PROFILE_MEM2 + (shaper_id * TE_TB2RATE_PROFILE_MEM2_STRIDE);
		printk("%s: mem2.wrd=0x%x, reg_off=0x%x\n", __func__, mem2.wrd, reg_off);
		CA_NE_REG_WRITE(mem2.wrd, reg_off);

		reg_off = TE_TB2RATE_PROFILE_MEM1 + (shaper_id * TE_TB2RATE_PROFILE_MEM1_STRIDE);
		printk("%s: mem1.wrd=0x%x, reg_off=0x%x\n", __func__, mem1.wrd, reg_off);
		CA_NE_REG_WRITE(mem1.wrd, reg_off);

		reg_off = TE_TB2RATE_PROFILE_MEM0 + (shaper_id * TE_TB2RATE_PROFILE_MEM0_STRIDE);
		printk("%s: mem0.wrd=0x%x, reg_off=0x%x\n", __func__, mem0.wrd, reg_off);
		CA_NE_REG_WRITE(mem0.wrd, reg_off);
	}
	else {
		reg_off = TE_TB2RATE_PROFILE_MEM2 + (shaper_id * TE_TB2RATE_PROFILE_MEM2_STRIDE);
		mem2.bf.data2 = (pir & PEAK_RATE_M_MASK_1);
		mem2.bf.data2 += (peak_burst & PEAK_BURST_SIZE_MASK);
		CA_NE_REG_WRITE(mem2.wrd, reg_off);

		reg_off = TE_TB2RATE_PROFILE_MEM1 + (shaper_id * TE_TB2RATE_PROFILE_MEM1_STRIDE);
		mem1.bf.data1 = (commited_burst & COMMITTED_BURST_BURST_SIZE_MASK_1);
		mem1.bf.data1 += (pir & PEAK_RATE_M_MASK_0);
		CA_NE_REG_WRITE(mem1.wrd, reg_off);

		reg_off = TE_TB2RATE_PROFILE_MEM0 + (shaper_id * TE_TB2RATE_PROFILE_MEM0_STRIDE);
		mem0.bf.data0 = (cir & COMMITTED_RATE_M_MASK);
		mem0.bf.data0 += (commited_burst & COMMITTED_BURST_BURST_SIZE_MASK_0);
		CA_NE_REG_WRITE(mem0.wrd, reg_off);
	}

	mem2.wrd = CA_NE_REG_READ(TE_TB2RATE_PROFILE_MEM2 + (shaper_id * TE_TB2RATE_PROFILE_MEM2_STRIDE));
	mem1.wrd = CA_NE_REG_READ(TE_TB2RATE_PROFILE_MEM1 + (shaper_id * TE_TB2RATE_PROFILE_MEM1_STRIDE));
	mem0.wrd = CA_NE_REG_READ(TE_TB2RATE_PROFILE_MEM0 + (shaper_id * TE_TB2RATE_PROFILE_MEM0_STRIDE));
	printk("%s: read back mem0.wrd=0x%x, mem1.wrd=0x%x, mem2.wrd=0x%x\n", __func__, mem0.wrd, mem1.wrd, mem2.wrd);

	/* initialize counter memory */
	memset(&counter_mem, 0, sizeof(te_tb2rate_counter_mem_t));
	counter_mem.bits.committed_tbc = 100 * 1024;
	counter_mem.bits.committed_tbc_sign = 1;
	counter_mem.bits.peak_tbc = 500 * 1024;
	counter_mem.bits.peak_tbc_sign = 1;
	printk("%s: counter_mem.wrd[0]=0x%x, counter_mem.wrd[1]=0x%x\n", __func__, counter_mem.wrd[0], counter_mem.wrd[1]);

	/* should do sequential write mem1 then mem0 */
	reg_off = TE_TB2RATE_COUNTER_MEM1 + (shaper_id * TE_TB2RATE_COUNTER_MEM1_STRIDE);
	CA_NE_REG_WRITE(counter_mem.wrd[1], reg_off);
	reg_off = TE_TB2RATE_COUNTER_MEM0 + (shaper_id * TE_TB2RATE_COUNTER_MEM0_STRIDE);
	CA_NE_REG_WRITE(counter_mem.wrd[0], reg_off);

	counter_mem1.wrd = CA_NE_REG_READ(TE_TB2RATE_COUNTER_MEM1 + (shaper_id * TE_TB2RATE_COUNTER_MEM1_STRIDE));
	counter_mem0.wrd = CA_NE_REG_READ(TE_TB2RATE_COUNTER_MEM0 + (shaper_id * TE_TB2RATE_COUNTER_MEM0_STRIDE));
	printk("%s: read back counter_mem0=0x%x, counter_mem1=0x%x\n", __func__, counter_mem0.wrd, counter_mem1.wrd);
}
#endif

void ca_l3te_dump_shaper(int shaper_id)
{
	ca_uint32_t             reg_off;
	te_tb2rate_profile_mem_t 	profile_mem;

	reg_off = TE_TB2RATE_PROFILE_MEM2 + (shaper_id * TE_TB2RATE_PROFILE_MEM2_STRIDE);
	profile_mem.wrd[2] = CA_NE_REG_READ(reg_off);
	reg_off = TE_TB2RATE_PROFILE_MEM1 + (shaper_id * TE_TB2RATE_PROFILE_MEM1_STRIDE);
	profile_mem.wrd[1] = CA_NE_REG_READ(reg_off);
	reg_off = TE_TB2RATE_PROFILE_MEM0 + (shaper_id * TE_TB2RATE_PROFILE_MEM0_STRIDE);
	profile_mem.wrd[0] = CA_NE_REG_READ(reg_off);

	printk("[shaper_id=%d]\n", shaper_id);
	printk("\t\tcommitted_rate_k=%d, committed_rate_m=%d, committed_bs=%d\n", profile_mem.bits.committed_rate_k,
		profile_mem.bits.committed_rate_m, profile_mem.bits.committed_bs);
	printk("\t\tpeak_rate_k=%d, peak_rate_m=%d, peak_bs=%d\n", profile_mem.bits.peak_rate_k,
		profile_mem.bits.peak_rate_m, profile_mem.bits.peak_bs);
}

ca_status_t ca_l3te_subtract_token(ca_uint32_t shaper_id, ca_boolean_t pir_flag, ca_uint32_t token_count)
{
        TE_TB2RATE_TOKEN_SUBTRACT_t     token_sub;

	printk("%s: shaper_id=%d, pir_flag=%d, token_count=%d\n", __func__,
		shaper_id, pir_flag, token_count);

        token_sub.wrd = 0;
        token_sub.bf.entry_num = shaper_id;
        token_sub.bf.peak = pir_flag;
        token_sub.bf.tokens = token_count;
        token_sub.bf.vld = 1;
        CA_NI_REG_WRITE(token_sub.wrd, TE_TB2RATE_TOKEN_SUBTRACT);

        return CA_E_OK;
}

void ca_l3te_dump_token_counter(ca_uint32_t shaper_id)
{
	TE_TB2RATE_COUNTER_MEM1_t       counter_mem1;
	TE_TB2RATE_COUNTER_MEM0_t       counter_mem0;
	te_tb2rate_counter_mem_t	counter_mem;

	counter_mem1.wrd = CA_NE_REG_READ(TE_TB2RATE_COUNTER_MEM1 + (shaper_id * TE_TB2RATE_COUNTER_MEM1_STRIDE));
	counter_mem0.wrd = CA_NE_REG_READ(TE_TB2RATE_COUNTER_MEM0 + (shaper_id * TE_TB2RATE_COUNTER_MEM0_STRIDE));
	printk("%s: read back counter_mem0=0x%x, counter_mem1=0x%x\n", __func__, counter_mem0.wrd, counter_mem1.wrd);
	counter_mem.wrd[0] = counter_mem0.wrd;
	counter_mem.wrd[1] = counter_mem1.wrd;
	printk("%s: committed_tbc=%d, committed_tbc_sign=%d, peak_tbc=%d, peak_tbc_sign=%d\n", __func__,
		counter_mem.bits.committed_tbc, counter_mem.bits.committed_tbc_sign,
		counter_mem.bits.peak_tbc, counter_mem.bits.peak_tbc_sign);
}
