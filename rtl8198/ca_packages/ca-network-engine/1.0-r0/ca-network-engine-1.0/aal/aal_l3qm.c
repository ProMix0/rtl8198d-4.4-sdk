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

#if defined(CONFIG_NE_CPU256)
#include "aal_l3qm_cpu256.h"
#endif

#include "aal_common.h"

#if 0
#define CA_L3QM_SRAM_START_ADDR		0xC0000000
#define CA_L3QM_SRAM_END_ADDR		0xC003FFFF
#define CA_L3QM_SRAM_SIZE		0x00040000	/* 256K */

#define CA_L3QM_DDR_START_ADDR          0x7EC00000
#define CA_L3QM_DDR_END_ADDR            0x7FFFFFFF
#define CA_L3QM_DDR_SIZE                0x01400000	/* 20M */
#endif

#define CA_L3QM_MAGIC_NUMBER		0xDEADBEEF
//#define CA_L3QM_MAGIC_NUMBER_SIZE 	8

//#define CA_L3QM_PHY_ADDR_SHIFT		7

#define CA_L3QM_AXI_TOP_DDR_NONCACHE_BIT		1
#define CA_L3QM_AXI_TOP_DDR_COHERENT_BIT	 	4

#define CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT		300

#define CA_L3QM_AXI_ATTRIBUTE_EQ_BASE		0		/* EQ0-15 */

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#define CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE	16		/* CPU256 port 0-31 */
#define CA_L3QM_AXI_ATTRIBUTE_CPU_BASE		48		/* CPU port 0-7 */
#else
#define CA_L3QM_AXI_ATTRIBUTE_CPU_BASE          16              /* CPU port 0-7 */
#endif

#define CA_L3QM_CPU_EPP_FIFO_SIZE_UINT		4		/* 4-FIFO entries */
#define	EQ_POOL_SRAM_SIZE	(0x8000)

#if defined(CONFIG_LUNA_G3_SERIES)
#define	EQ_POOL_DDR_SIZE	(0x100000)
#else
#define	EQ_POOL_DDR_SIZE	(0x300000)
#endif

/* user defined constants should get from startup configs */
ca_uint8_t	l3qm_magic_marker_len = 8;			/* length of magic marker */
ca_uint8_t	l3qm_desc_size = 4;  				/* size of RX descriptor */
ca_uint8_t	l3qm_desc_per_epp = 2;				/* number of RX descriptor per EPP */
ca_uint32_t	l3qm_pe0_1_port_count = 1;			/* PE0/PE1 port count */
ca_uint8_t	l3qm_pe0_1_voq_per_port = 8;			/* PE0/PE1 VoQ per port */
ca_uint32_t	l3qm_pe0_1_epp_per_voq = 32;			/* PE0/PE1 EPP per VoQ */
ca_uint32_t	l3qm_pe0_1_pool_bid_count = 128;		/* PE0/PE1 pool BID count */
ca_uint8_t	l3qm_eq0_sram_eq1_dram = 0;			/* EQ0 SRAM and EQ1 DRAM Mode*/
ca_uint8_t	l3qm_eq_share_pool_id = 1;			/* EQ0 SRAM and EQ1 DRAM Mode*/

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
ca_uint32_t	l3qm_cpu256_port_count = 256;			/* CPU256 port count */
ca_uint8_t	l3qm_cpu256_voq_per_port = 1;			/* CPU256 VoQ per port */
ca_uint8_t	l3qm_cpu256_epp_per_voq = 32;			/* CPU256 EPP per VoQ */
ca_uint32_t     l3qm_cpu256_pool_bid_count = 1024;              /* CPU256 pool BID count */
#endif

ca_uint32_t     l3qm_cpu_port_count = 6;                   	/* CPU port count */
ca_uint8_t      l3qm_cpu_voq_per_port = 8;                      /* CPU VoQ per port */
ca_uint32_t     l3qm_cpu_epp_per_voq = 128;                     /* CPU EPP per VoQ */
ca_uint32_t     l3qm_cpu_pool_bid_count = 1500;                 /* CPU pool BID count */
//ca_uint32_t     l3qm_dq_pool_bid_count = 4096;                  /* DEEPQ pool BID count */
ca_uint8_t	l3qm_cpu_cache_line_size = 64;			/* CPU cache line size */
ca_uint32_t	l3qm_eq_buffer_align_size = 128;		/* EQ buffer align size */
ca_uint8_t	l3qm_pe_buffer_align_size = 4;			/* PE buffer align size */
ca_uint32_t	l3qm_cci0_start_addr = 0x00000000;		/* CCI0 start address */
ca_uint32_t	l3qm_cci1_start_addr = 0x00000000;		/* CCI1 start address */
ca_uint32_t	l3qm_sram_start_addr = 0xc0000000;		/* SRAM start address */
ca_uint32_t	l3qm_ddr_cache_coherent_start_addr = 0x00000000;	/* DDR cache coherent start address */
ca_uint32_t	l3qm_ddr_non_cacheable_start_addr = 0x80000000;		/* DDR non-cacheable start address */
ca_uint8_t	l3qm_eq_axi_top8_bits = 4;			/* 0x400000000, in DDR cache coherent */
ca_uint8_t	l3qm_epp_axi_top8_bits = 4;			/* 0x400000000, in DDR cache coherent */
ca_uint8_t	l3qm_deepq_test_with_sram = 0;			/* use sram to test DEEPQ function */
ca_uint8_t	l3qm_ace_test = 0;				/* do QM ACE test do not do dma sync by software */
ca_uint8_t	l3qm_main_hash_bm_test = 0;			/* do Main Hash Bench Mark test */

/* caculated from defined constants */
ca_uint8_t      l3qm_epp_size = 0;      /* EPP size in byte */

//ca_uint32_t     l3qm_eq_profile_pe0_1_pool_id_count = 0;        /* PE0/PE1 pool id count */
ca_uint32_t     l3qm_eq_profile_pe0_1_total_epp_count = 0;      /* PE0/PE1 total EPP count */

/* moved to aal_l3qm_cpu256.c */
//#ifndef CONFIG_ARCH_CORTINA_G3LITE
#if 0
//ca_uint32_t     l3qm_eq_profile_cpu256_pool_id_count = 0;       /* CPU256 pool id count */
ca_uint32_t     l3qm_epp_profile_cpu256_hi_total_epp_count = 0;    /* CPU256 total EPP count */
#endif

//ca_uint32_t     l3qm_eq_profile_cpu_pool_id_count = 0;          /* CPU pool id count */
ca_uint32_t     l3qm_epp_profile_cpu_total_epp_count = 0;       /* CPU total EPP count */

ca_uint32_t	l3qm_cpu_port_head_room_first = 32;		/* head room first for CPU port */

/* EQ(QM_CFG3/4_EQ%d) startup configures */
ca_uint32_t	l3qm_eq_cfg3_values[CFG_ID_L3QM_EQ_CFG3_VALUES_LEN] =
{
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1a008017, 0x1a008017, 0x0, 0x0, 0x0, 0x0, 0x10, 0x10
};

ca_uint32_t     l3qm_eq_cfg4_values[CFG_ID_L3QM_EQ_CFG4_VALUES_LEN] =
{
	0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x1, 0x1, 0x4, 0x4, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1
};

/* QM_AXI_ATTRIBUTE for EQ 0-15 */
ca_uint32_t	l3qm_axi_attrib_eq_values[CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES_LEN] =
{
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x04000010, 0x04000010
};

/* moved to aal_l3qm_cpu256.c */
//#ifndef CONFIG_ARCH_CORTINA_G3LITE
#if 0
/* QM_AXI_ATTRIBUTE for CPU 256 EPP */
ca_uint32_t     l3qm_axi_attrib_cpu256_epp_values[CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES_LEN] =
{
	0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010,
	0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010,
	0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010,
	0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010, 0x04000010
};
#endif

/* QM_AXI_ATTRIBUTE for CPU EPP */
ca_uint32_t     l3qm_axi_attrib_cpu_epp_values[CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES_LEN] =
{
	0x12008060, 0x12008060, 0x12008060, 0x12008060, 0x12008060, 0x12008060, 0x12008060, 0x12008060
};

/* configuration parameters for PE0 need to be got from start config */
ca_uint8_t      l3qm_eq_profile_pe0_id = 0;                     /* EQ profile 0 */
ca_uint8_t      l3qm_eq_profile_pe0_rule = 1;                   /* use pool 0 for header */
ca_uint8_t      l3qm_eq_profile_pe0_use_fbm = 1;                /* use FBM pool */
ca_uint8_t      l3qm_eq_profile_pe0_pool0_eq_id = 1;            /* QM_EQ_PROFILE0.eqid0=1 */
//ca_uint32_t     l3qm_eq_profile_pe0_pool0_buf_count = 128;        /* number of buffers for PE0 pool 0 */
ca_uint32_t     l3qm_eq_profile_pe0_pool0_bid_start = 0;        /* strating buffer id (must start from 0) */
//ca_uint32_t     l3qm_eq_profile_pe0_pool0_buf_sz = 256;         /* pool 0 header buffer size (B) */
//ca_uint8_t      l3qm_eq_profile_pe0_pool0_location = 1;         /* SRAM (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_pe0_pool0_fbm_pool_id = 0;      /* in SRAM */
ca_uint32_t	l3qm_eq_profile_pe0_pool0_physical_start_address = 0xffffffff;	/* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_pe0_pool1_eq_id = 2;            /* QM_EQ_PROFILE1.eqid0=2 */
ca_uint32_t     l3qm_eq_profile_pe0_pool1_buf_count = 128;        /* number of  buffers for PE0 pool 1 */
ca_uint32_t     l3qm_eq_profile_pe0_pool1_bid_start = 128;       /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_pe0_pool1_buf_sz = 2048;        /* pool 1 buffer size(payload) */
//ca_uint8_t      l3qm_eq_profile_pe0_pool1_location = 0;         /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_pe0_pool1_fbm_pool_id = 1;      /* in DDR */
ca_uint32_t	l3qm_eq_profile_pe0_pool1_physical_start_address = 0xffffffff;	/* pool1 physical start address */
//ca_uint32_t     l3qm_eq_profile_pe0_id_ports = 23;     		/* LPID=23, CPU port 7 */
ca_uint8_t      l3qm_epp_profile_pe0_id = 0;                    /* EPP (RX FIFO) profile id */
ca_uint32_t     l3qm_epp_profile_pe0_sz = 0;                    /* RX FIFO size per entry, 4B */
ca_uint32_t     l3qm_epp_profile_pe0_cn = 0;	                /* 8 queues, 64 entries per queue */
ca_uint32_t     l3qm_epp_profile_pe0_start_addresses[8] = {0xffffffff};       /* VoQs 0-7 for PE0  */
ca_uint32_t	l3qm_epp_profile_pe0_map_mode = 0;		/* QM_CPU_EPP0_CFG map_mode=0 */

/* configuration parameters for PE1 need to be got from start config */
ca_uint8_t      l3qm_eq_profile_pe1_id = 1;                     /* EQ profile 1 */
ca_uint8_t      l3qm_eq_profile_pe1_rule = 1;                   /* use pool 0 for header */
ca_uint8_t      l3qm_eq_profile_pe1_use_fbm = 1;                /* use FBM pool */
ca_uint8_t      l3qm_eq_profile_pe1_pool0_eq_id = 3;            /* QM_EQ_PROFILE1.eqid0=3 */
//ca_uint32_t     l3qm_eq_profile_pe1_pool0_buf_count = 128;        /* 64 buffers for PE1 pool 0 */
ca_uint32_t     l3qm_eq_profile_pe1_pool0_bid_start = 256;      /* strating buffer id */
//ca_uint32_t     l3qm_eq_profile_pe1_pool0_buf_sz = 256;         /* pool 0 header buffer size (B) */
//ca_uint8_t      l3qm_eq_profile_pe1_pool0_location = 1;         /* SRAM (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_pe1_pool0_fbm_pool_id = 1;      /* in SRAM */
ca_uint32_t	l3qm_eq_profile_pe1_pool0_physical_start_address = 0xffffffff;	/* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_pe1_pool1_eq_id = 4;            /* QM_EQ_PROFILE1.eqid1=4 */
ca_uint32_t     l3qm_eq_profile_pe1_pool1_buf_count = 128;        /* 64 buffers for PE1 pool 1 */
ca_uint32_t     l3qm_eq_profile_pe1_pool1_bid_start = 384;   /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_pe1_pool1_buf_sz = 2048;        /* pool 1 buffer size(payload) */
//ca_uint8_t      l3qm_eq_profile_pe1_pool1_location = 0;         /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_pe1_pool1_fbm_pool_id = 0;      /* in DDR */
ca_uint32_t	l3qm_eq_profile_pe1_pool1_physical_start_address = 0xffffffff;	/* pool1 physical start address */
//ca_uint32_t     l3qm_eq_profile_pe1_id_ports = 22;              /* LPID=22, CPU port 6 */
ca_uint8_t      l3qm_epp_profile_pe1_id = 1;                    /* EPP (RX FIFO) profile id */
ca_uint32_t     l3qm_epp_profile_pe1_sz = 0;                    /* RX FIFO size per entry, 4B */
ca_uint32_t     l3qm_epp_profile_pe1_cn = 0;                    /* 8 queues, 64 entries per queue */
ca_uint32_t     l3qm_epp_profile_pe1_start_addresses[8] = {0xffffffff};       /* VoQs 0-7 for PE1  */
ca_uint32_t     l3qm_epp_profile_pe1_map_mode = 0;              /* QM_CPU_EPP1_CFG map_mode=0 */

/* configuration parameters for CPU need to be got from start config */
ca_uint8_t      l3qm_eq_profile_cpu_id = 2;                     /* EQ profile 2 */
ca_uint8_t      l3qm_eq_profile_cpu_rule = 0;                   /* use pool 0 then pool 1 */
ca_uint8_t      l3qm_eq_profile_cpu_use_fbm = 0;                /* do not use FBM pool */
ca_uint8_t      l3qm_eq_profile_cpu_pool0_eq_id = 5;            /* QM_EQ_PROFILE2.eqid0=5 */
ca_uint32_t     l3qm_eq_profile_cpu_pool0_buf_count = 1500;        /* 1024 buffers for CPU pool 0 */
ca_uint32_t     l3qm_eq_profile_cpu_pool0_bid_start = 4608;     /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_cpu_pool0_buf_sz = 2048;        /* pool 0 buffer size */
//ca_uint8_t      l3qm_eq_profile_cpu_pool0_location = 0;         /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_cpu_pool0_fbm_pool_id = 0;      /* SRAM */
ca_uint32_t	l3qm_eq_profile_cpu_pool0_physical_start_address = 0xffffffff;	/* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_cpu_pool1_eq_id = 6;            /* QM_EQ_PROFILE2.eqid0=6 */
ca_uint32_t     l3qm_eq_profile_cpu_pool1_buf_count = 1500;        /* 2048 buffers for CPU pool 1 */
ca_uint32_t     l3qm_eq_profile_cpu_pool1_bid_start = 6108;     /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_cpu_pool1_buf_sz = 2048;        /* pool 1 buffer size */
//ca_uint8_t      l3qm_eq_profile_cpu_pool1_location = 0;         /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_cpu_pool1_fbm_pool_id = 0;      /* SRAM */
ca_uint32_t	l3qm_eq_profile_cpu_pool1_physical_start_address = 0xffffffff;	/* pool1 physical start address */
//ca_uint8_t      l3qm_eq_profile_cpu_id_ports[6] = {16, 17, 18, 19, 20, 21};     /* CPU port 0-5 */
ca_uint8_t      l3qm_epp_profile_cpu_id = 2;                    		/* EPP (RX FIFO) profile id */

ca_uint32_t     l3qm_epp_profile_cpu_sz = 0;                    /* RX FIFO size per entry, 8B */
ca_uint32_t     l3qm_epp_profile_cpu_cn = 256 * 8 * 6;          /* CPU ports 0-5, 6 ports, 8 queues per port, 256 entries per queue */

ca_uint32_t     l3qm_epp_profile_cpu_map_mode = 0;              /* QM_CPU_EPP1_CFG map_mode=0 */
//ca_uint32_t     l3qm_epp_profile_cpu_start_addresses = 0;	 		/* VoQs 0-7 for port0-7 */

/* configuration parameters for Deep Queue need to be got from start config, only use pool0 and no EPP profile */
ca_uint8_t      l3qm_eq_profile_dq_id = 3;                     /* EQ profile 3 */
ca_uint8_t      l3qm_eq_profile_dq_rule = 0;                   /* use pool 0 then pool 1 */
//ca_uint8_t      l3qm_eq_profile_dq_use_fbm = 0;                /* do not use FBM pool */
ca_uint8_t      l3qm_eq_profile_dq_pool0_eq_id = 8;            /* QM_EQ_PROFILE3.eqid0=0, eqid1=0 */
ca_uint32_t     l3qm_eq_profile_dq_pool0_buf_count = 4096;        /* 8192 buffers for DQ pool 0 */
ca_uint32_t     l3qm_eq_profile_dq_pool0_bid_start = 7608;     /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_dq_pool0_buf_sz = 2048;        /* pool 0 buffer size */
ca_uint32_t     l3qm_eq_profile_dq_pool0_start_physical_addr = 0xffffffff;  	/* QM_CFG0_EQ0.phy_addr_start = DDR addess of DQ EQ pool */
//ca_uint8_t      l3qm_eq_profile_dq_pool0_location = 0;         			/* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_dq_pool0_fbm_pool_id = 7;      /* DDR (pushed into FBM) */
//ca_uint8_t      l3qm_eq_profile_dq_id_ports[6] = {26, 27, 28, 29, 30, 31};     	/* L3QM port 0-5 */
ca_uint8_t      l3qm_eq_profile_dq_pool1_eq_id = 9;            /* QM_EQ_PROFILE3.eqid0=0, eqid1=0 */
ca_uint32_t     l3qm_eq_profile_dq_pool1_buf_count = 4096;        /* 8192 buffers for DQ pool 0 */
ca_uint32_t     l3qm_eq_profile_dq_pool1_bid_start = 11704;    /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_dq_pool1_buf_sz = 2048;        /* pool 0 buffer size */
ca_uint32_t     l3qm_eq_profile_dq_pool1_start_physical_addr = 0xffffffff;      /* QM_CFG0_EQ0.phy_addr_start = DDR addess of DQ EQ pool */
//ca_uint8_t      l3qm_eq_profile_dq_pool1_location = 0;                          /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_dq_pool1_fbm_pool_id = 7;      /* DDR (pushed into FBM) */

ca_uint8_t	l3qm_eq_profile_dq_wan_pool_id = 12;
ca_uint32_t	l3qm_eq_profile_dq_wan_pool_start_physical_addr = 0xc0039000;
ca_uint32_t	l3qm_eq_profile_dq_wan_pool_buf_sz = 256;
ca_uint32_t     l3qm_eq_profile_dq_wan_pool_buf_count = 32;

#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(l3qm_eq_profile_dq_pool0_buf_count);
EXPORT_SYMBOL(l3qm_eq_profile_dq_pool1_buf_count);
#endif


/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
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
ca_uint32_t	l3qm_eq_profile_cpu256_hi_pool0_physical_start_address = 0xffffffff;	/* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_eq_id = 8;            /* QM_EQ_PROFILE4.eqid1=9 */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_buf_count = 1024;        /* 256 buffers for PE upstream shaper */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_bid_start = 1536;   /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_buf_sz = 2048;        /* pool 1 buffer size(payload) */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_location = 0;         /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_fbm_pool_id = 0;      /* in DDR */
ca_uint32_t	l3qm_eq_profile_cpu256_hi_pool1_physical_start_address = 0xffffffff;	/* pool1 physical start address */
ca_uint8_t      l3qm_eq_profile_cpu256_id_ports[32] = {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};    /* [0,1,....15] in L3QM, LPID[32,33....47] */
ca_uint8_t      l3qm_epp_profile_cpu256_hi_id = 3;                    /* EPP (RX FIFO) profile id */
ca_uint32_t     l3qm_epp_profile_cpu256_hi_sz = 0;		      /* RX FIFO size per entry, 4B */
ca_uint32_t     l3qm_epp_profile_cpu256_hi_cn = 0;           	      /* 8 queues, 256 entries per queue */
ca_uint32_t     l3qm_epp_profile_cpu256_hi_start_addresses[8] = {0xffffffff};  	 /* VoQs 0-7 for QM_CPU_EPP_FIFO0_0_START to QM_CPU_EPP_FIFO15_7_START  */
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
#endif

/* configuration parameters for main hash table need to be got from start config */
ca_uint32_t	l3fe_main_hash_overflow_hash_fib_length	= 32;		/* FIB length of main hash overflow hash table */
ca_uint32_t	l3fe_main_hash_overflow_hash_fib_count	= 32;		/* number of FIB of main hash overflow hash table */
ca_uint32_t	l3fe_main_hash_default_hash_fib_length	= 32;		/* FIB length of main hash default hash table */
ca_uint32_t	l3fe_main_hash_default_hash_fib_count	= 32;		/* number of FIB of main hash default hash table */
ca_uint32_t	l3fe_main_hash_action_cache_fib_length	= 32;		/* FIB length of main hash action cache table */
ca_uint32_t	l3fe_main_hash_action_cache_fib_count	= 2048;		/* number of FIB of main hash action cache table */
ca_uint32_t	l3fe_main_hash_table_entry_length	= 4;		/* length of main hash table entry */
ca_uint32_t	l3fe_main_hash_table_entry_count	= 65536;	/* count of main hash table entry */
ca_uint32_t	l3fe_main_hash_action_table_entry_length= 32;		/* length of main hash action table entry */
ca_uint32_t	l3fe_main_hash_action_table_entry_count = 65536;	/* count of main hash action table entry */

/* DEEPQ flag: 1-enable deep queu, 0-disable deep queue act as normal NI ports */
//ca_uint8_t      l3qm_ni_p0_7_deep_q_en = 1;				 /* 1:for L3 test, 0:for L2 test */

/* SRAM/DRR addresses calculated from startup config */
/* virtual address should be 64-bit long */
/* SRAM/DRR addresses calculated from startup config */
/* virtual addresses in DDR non-cache area should be 64-bit long */
ca_uint_t     l3qm_eq_profile_dq_pool0_buff_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_dq_pool1_buff_start_addr = 0xffffffff;
ca_uint_t     l3fe_main_hash_table_start_addr = 0xffffffff;
ca_uint_t     l3fe_main_action_table_start_addr = 0xffffffff;

/* there is no more memory space reserved for PE0/1 in L3QM */
//ca_uint_t     l3qm_eq_profile_pe0_pool1_buff_start_addr = 0xffffffff;
//ca_uint_t     l3qm_eq_profile_pe1_pool1_buff_start_addr = 0xffffffff;

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
ca_uint_t     l3qm_eq_profile_cpu256_hi_pool0_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu256_hi_pool1_start_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu256_lo_pool0_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu256_lo_pool1_start_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr = 0xffffffff;
#endif

/* physical addresses in DDR non-cache area should be 32-bit long */
ca_uint32_t     l3qm_eq_profile_dq_pool0_buff_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_dq_pool1_buff_phy_addr = 0xffffffff;
ca_uint32_t     l3fe_main_hash_table_phy_addr = 0xffffffff;
ca_uint32_t     l3fe_main_action_table_phy_addr = 0xffffffff;

/* there is no more memory space reserved for PE0/1 in L3QM */
//ca_uint32_t     l3qm_eq_profile_pe0_pool1_buff_phy_addr = 0xffffffff;
//ca_uint32_t     l3qm_eq_profile_pe1_pool1_buff_phy_addr = 0xffffffff;

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_epp_profile_cpu256_lo_rx_fifo_phy_addr = 0xffffffff;
#endif

/* virtual addresses in DDR cache coherent area should be 64-bit long */
ca_uint_t     l3qm_eq_profile_cpu_pool0_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu_pool1_start_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_cpu_rx_fifo_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu_pool0_phy_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu_pool1_phy_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_cpu_rx_fifo_phy_addr = 0xffffffff;

/* virtual addresses in SRAM area should be 64-bit long */
ca_uint_t     l3fe_main_hash_overflow_hash_fib_start_addr = 0xffffffff;
ca_uint_t     l3fe_main_hash_default_hash_fib_start_addr = 0xffffffff;
ca_uint_t     l3fe_main_hash_action_cache_fib_start_addr = 0xffffffff;
#if 0
ca_uint_t     l3qm_epp_profile_pe0_rx_fifo_start_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_pe1_rx_fifo_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_pe0_pool0_buff_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_pe1_pool0_buff_start_addr = 0xffffffff;
#endif

/* physical address in SRAM area should be 32-bit long */
ca_uint32_t     l3fe_main_hash_overflow_hash_fib_phy_addr = 0xffffffff;
ca_uint32_t     l3fe_main_hash_default_hash_fib_phy_addr = 0xffffffff;
ca_uint32_t     l3fe_main_hash_action_cache_fib_phy_addr = 0xffffffff;
#if 0
ca_uint32_t     l3qm_epp_profile_pe0_rx_fifo_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_epp_profile_pe1_rx_fifo_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_pe0_pool0_buff_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_pe1_pool0_buff_phy_addr = 0xffffffff;
#endif

#ifdef CONFIG_RTK_8198F_NE_PATCH
ca_uint32_t     l3qm_rx_start_addresses[64] = {0xffffffff};
ca_uint8_t      l3qm_rx_read_start_addresses_flg = 0;
#endif

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#if 0
spinlock_t l3qm_epp256_inte_lock;
#endif

spinlock_t l3qm_epp64_inte_lock;

//static int aal_l3qm_insert_magic_number(void *addr);

/* moved to aal_l3qm.h */
//ca_status_t aal_l3qm_check_magic_number(void *addr);

#if defined(CONFIG_NE_CPU256)
ca_uint8_t support_cpu256_in_linux = 0;
#endif

void aal_l3qm_dump_registers(void)
{
        u32     reg;
        u32     value;

        printk("====================================L3 QM=================================\n");
        for (reg = QM_QM_REVISION; reg <= QM_QM_SPARE_STS; reg+=4) {
                value = CA_NE_REG_READ(reg);
                printk("register=0x%x, value=0x%x\n", reg, value);
        }
}

int aal_l3qm_get_ace_test(void)
{
	return l3qm_ace_test;
}

static void aal_l3qm_get_axi_attrib(int entry, QM_QM_AXI_ATTRIBUTE_DATA0_t *attrib_data0, QM_QM_AXI_ATTRIBUTE_DATA1_t *attrib_data1)
{
        int i;
        QM_QM_AXI_ATTRIBUTE_ACCESS_t    attrib;

	attrib.wrd = 0;
        attrib.bf.access = 1;
        attrib.bf.rbw = 0;
        attrib.bf.ADDR = entry;
	CA_NE_REG_WRITE(attrib.wrd, QM_QM_AXI_ATTRIBUTE_ACCESS);
        for (i = 0; i < CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT; i++) {
                attrib.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_ACCESS);
                if (!attrib.bf.access)
                        break;
        }
        if (i == CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT) {
                printk("%s: CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT at entry=%d!!!\n", __func__, entry);
                return;
        }

        attrib_data0->wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_DATA0);
        attrib_data1->wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_DATA1);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("[%d] data0=0x%x, data1=0x%x\n", entry, attrib_data0->wrd, attrib_data1->wrd);
		printk("     qos=%d, cache=%d, snoop=%d, bar=%d, domain=%d, user=%d, prot=%d\n", attrib_data0->bf.qos, attrib_data0->bf.cache, attrib_data0->bf.snoop,
				attrib_data0->bf.bar, attrib_data0->bf.domain, attrib_data0->bf.user, attrib_data0->bf.prot);

		printk("     axi_top_bit=0x%x, ace_cmd=%d\n", attrib_data0->bf.axi_top_bit | (attrib_data1->bf.axi_top_bit << 6), attrib_data0->bf.ace_cmd);
	}
}

int aal_l3qm_get_cpuepp_axi_attrib(int cpu_port)
{
	QM_QM_AXI_ATTRIBUTE_DATA0_t attrib_data0;
	QM_QM_AXI_ATTRIBUTE_DATA1_t attrib_data1;

	aal_l3qm_get_axi_attrib(CA_L3QM_AXI_ATTRIBUTE_CPU_BASE + cpu_port, &attrib_data0, &attrib_data1);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: cpu_port=%d, axi_top_bit=%d\n", __func__,  cpu_port, attrib_data0.bf.axi_top_bit | (attrib_data1.bf.axi_top_bit << 6));
	}
	return attrib_data0.bf.axi_top_bit | (attrib_data1.bf.axi_top_bit << 6);
}

int aal_l3qm_get_eq_pool_axi_top_8bit(int eqid)
{
	QM_QM_CFG4_EQ0_t	cfg4_eq;

	cfg4_eq.wrd = CA_NE_REG_READ(QM_QM_CFG4_EQ0 + eqid * QM_QM_CFG4_EQ0_STRIDE);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: eqid=%d, axi_top_bit=%d\n", __func__,  eqid, cfg4_eq.bf.axi_top_bit);
	}

	return cfg4_eq.bf.axi_top_bit;
}

int aal_l3qm_get_cpu_port_head_room_first(void)
{
	return l3qm_cpu_port_head_room_first;
}

int aal_l3qm_eq_get_cpu_rule(void)
{
	return l3qm_eq_profile_cpu_rule;
}

int aal_l3qm_eq_get_cpu_use_fbm(void)
{
        return l3qm_eq_profile_cpu_use_fbm;
}

int aal_l3qm_eq_get_cpu_pool0_buf_size(void)
{
	return l3qm_eq_profile_cpu_pool0_buf_sz;
}

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
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
	printk("%s: l3qm_cpu256_epp_per_voq=%d, l3qm_desc_size=%d, l3qm_desc_per_epp=%d\n", __func__,
		l3qm_cpu256_epp_per_voq, l3qm_desc_size, l3qm_desc_per_epp);

        return (l3qm_cpu256_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp);
}
#endif

int aal_l3qm_eq_get_desc_per_epp(void)
{
	return l3qm_desc_per_epp;
}

int aal_l3qm_eq_get_desc_size(void)
{
	return l3qm_desc_size;
}

int aal_l3qm_eq_get_max_write_ptr(void)
{
	return (l3qm_cpu_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp);
}

int aal_l3qm_eq_get_cpu_pool_bid_count(void)
{
	return l3qm_cpu_pool_bid_count;
}

void aal_l3qm_eq_get_cpu_pool0_1_start_addr(ca_uint_t *cpu_pool0_start_addr, ca_uint_t *cpu_pool1_start_addr)
{
	*cpu_pool0_start_addr = l3qm_eq_profile_cpu_pool0_start_addr;
	*cpu_pool1_start_addr = l3qm_eq_profile_cpu_pool1_start_addr;
}

void aal_l3qm_eq_get_cpu_pool0_1_eq_id(ca_uint8_t *cpu_pool0_eq_id, ca_uint8_t *cpu_pool1_eq_id)
{
	*cpu_pool0_eq_id = l3qm_eq_profile_cpu_pool0_eq_id;
	*cpu_pool1_eq_id = l3qm_eq_profile_cpu_pool1_eq_id;
}

#if 0
int aal_l3qm_eq_get_pe0_use_fbm(void)
{
        return l3qm_eq_profile_pe0_use_fbm;
}

int aal_l3qm_eq_get_pe1_use_fbm(void)
{
        return l3qm_eq_profile_pe1_use_fbm;
}
#endif

#if 0
void aal_l3qm_eq_get_pe0_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
       *phy_start_addr = l3qm_eq_profile_pe0_pool0_buff_phy_addr;
       *entry_size = l3qm_eq_profile_pe0_pool0_buf_sz;
       *entry_count = l3qm_eq_profile_pe0_pool0_buf_count;
       *fbm_pool_id = l3qm_eq_profile_pe0_pool0_fbm_pool_id;
}

void aal_l3qm_eq_get_pe0_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
	*phy_start_addr = l3qm_eq_profile_pe0_pool1_buff_start_addr;
	*entry_size = l3qm_eq_profile_pe0_pool1_buf_sz;
	*entry_count = l3qm_eq_profile_pe0_pool1_buf_count;
	*fbm_pool_id = l3qm_eq_profile_pe0_pool1_fbm_pool_id;
}

void aal_l3qm_eq_get_pe1_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
       *phy_start_addr = l3qm_eq_profile_pe1_pool0_buff_phy_addr;
       *entry_size = l3qm_eq_profile_pe1_pool0_buf_sz;
       *entry_count = l3qm_eq_profile_pe1_pool0_buf_count;
       *fbm_pool_id = l3qm_eq_profile_pe1_pool0_fbm_pool_id;
}

void aal_l3qm_eq_get_pe1_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
        *phy_start_addr = l3qm_eq_profile_pe1_pool1_buff_start_addr;
        *entry_size = l3qm_eq_profile_pe1_pool1_buf_sz;
        *entry_count = l3qm_eq_profile_pe1_pool1_buf_count;
        *fbm_pool_id = l3qm_eq_profile_pe1_pool1_fbm_pool_id;
}
#endif

void aal_l3qm_eq_get_cpu_pool0_info(ca_uint_t *virt_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
       *virt_start_addr = l3qm_eq_profile_cpu_pool0_start_addr;
       *entry_size = l3qm_eq_profile_cpu_pool0_buf_sz;
       *entry_count = l3qm_eq_profile_cpu_pool0_buf_count;
       *fbm_pool_id = l3qm_eq_profile_cpu_pool0_fbm_pool_id;
}

void aal_l3qm_eq_get_cpu_pool1_info(ca_uint_t *virt_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
        *virt_start_addr = l3qm_eq_profile_cpu_pool1_start_addr;
        *entry_size = l3qm_eq_profile_cpu_pool1_buf_sz;
        *entry_count = l3qm_eq_profile_cpu_pool1_buf_count;
        *fbm_pool_id = l3qm_eq_profile_cpu_pool1_fbm_pool_id;
}

void aal_l3qm_get_l3fe_main_hash_action_info(aal_l3qm_l3fe_main_hash_info_t *l3fe_main_hash_info)
{
        l3fe_main_hash_info->overflow_action_start_virt_addr = l3fe_main_hash_overflow_hash_fib_start_addr;
        l3fe_main_hash_info->default_action_start_virt_addr  = l3fe_main_hash_default_hash_fib_start_addr;
        l3fe_main_hash_info->action_cache_start_virt_addr    = l3fe_main_hash_action_cache_fib_start_addr;
        l3fe_main_hash_info->hash_table_start_virt_addr      = l3fe_main_hash_table_start_addr;
        l3fe_main_hash_info->action_table_start_virt_addr    = l3fe_main_action_table_start_addr;
        l3fe_main_hash_info->overflow_action_start_phy_addr  = l3fe_main_hash_overflow_hash_fib_phy_addr;
        l3fe_main_hash_info->default_action_start_phy_addr   = l3fe_main_hash_default_hash_fib_phy_addr;
        l3fe_main_hash_info->action_cache_start_phy_addr     = l3fe_main_hash_action_cache_fib_phy_addr;
        l3fe_main_hash_info->hash_table_start_phy_addr       = l3fe_main_hash_table_phy_addr;
        l3fe_main_hash_info->action_table_start_phy_addr     = l3fe_main_action_table_phy_addr;
        l3fe_main_hash_info->overflow_hash_fib_length	     = l3fe_main_hash_overflow_hash_fib_length;
        l3fe_main_hash_info->overflow_hash_fib_count         = l3fe_main_hash_overflow_hash_fib_count;
        l3fe_main_hash_info->default_hash_fib_length	     = l3fe_main_hash_default_hash_fib_length;
        l3fe_main_hash_info->default_hash_fib_count	     = l3fe_main_hash_default_hash_fib_count;
        l3fe_main_hash_info->action_cache_fib_length	     = l3fe_main_hash_action_cache_fib_length;
        l3fe_main_hash_info->action_cache_fib_count	     = l3fe_main_hash_action_cache_fib_count;
        l3fe_main_hash_info->table_entry_length	     	     = l3fe_main_hash_table_entry_length;
        l3fe_main_hash_info->table_entry_count	             = l3fe_main_hash_table_entry_count;
        l3fe_main_hash_info->action_table_entry_length	     = l3fe_main_hash_action_table_entry_length;
        l3fe_main_hash_info->action_table_entry_count	     = l3fe_main_hash_action_table_entry_count;
	l3fe_main_hash_info->main_hash_bm_test = l3qm_main_hash_bm_test;
}

void aal_l3qm_set_l3fe_main_hash(ca_uint_t main_hash_table_saddr, ca_uint32_t main_hash_table_paddr,
					ca_uint_t main_hash_action_saddr, ca_uint32_t main_hash_action_paddr)
{
	l3fe_main_hash_table_start_addr = main_hash_table_saddr;
	l3fe_main_action_table_start_addr = main_hash_action_saddr;
	l3fe_main_hash_table_phy_addr = main_hash_table_paddr;
	l3fe_main_action_table_phy_addr = main_hash_action_paddr;
}

void aal_l3qm_set_rx_read_ptr(ca_uint16_t cpu_port, ca_uint8_t voq, ca_uint32_t value)
{
	QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_t	rdptr;

	rdptr.wrd = 0;
	rdptr.bf.rptr = value;

	CA_NE_REG_WRITE(rdptr.wrd, QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR + (cpu_port * l3qm_cpu_voq_per_port + voq) * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_STRIDE);
}

ca_uint32_t aal_l3qm_get_rx_read_ptr(ca_uint16_t cpu_port, ca_uint8_t voq)
{
        QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_t      rdptr;

	/* 64 instances 8 CPU ports each port has 8 VOQs */
	rdptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR + (cpu_port * l3qm_cpu_voq_per_port + voq) * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_STRIDE);

	return rdptr.bf.rptr;
}

ca_uint32_t aal_l3qm_get_rx_write_ptr(ca_uint16_t cpu_port, ca_uint8_t voq, ca_uint16_t *count)
{
	QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_t	wrptr;
	ca_uint32_t rdptr;
	int epp_size, size;

	/* 64 instances 8 CPU ports each port has 8 VOQs */
	wrptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR + (cpu_port * l3qm_cpu_voq_per_port + voq) * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_STRIDE);

	rdptr = aal_l3qm_get_rx_read_ptr(cpu_port, voq);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: wrptr.bf.wptr=%d, rdptr=%d\n", __func__, wrptr.bf.wptr, rdptr);
	}

	/* take care of wraping around case */
	epp_size = l3qm_desc_size * l3qm_desc_per_epp;
	if (wrptr.bf.wptr >= rdptr) {
		*count = (wrptr.bf.wptr - rdptr) / epp_size;
	}
	else {
		size = aal_l3qm_eq_get_max_write_ptr();
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

ca_uint32_t aal_l3qm_get_rx_start_addr(ca_uint16_t cpu_port, ca_uint8_t voq)
{
	QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_t paddr_start;

#ifdef CONFIG_RTK_8198F_NE_PATCH
	int i, j;
	
	if (l3qm_rx_read_start_addresses_flg == 0)
	{
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
			   l3qm_rx_start_addresses[8*i+j] = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (i * l3qm_cpu_voq_per_port + j) * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
			}
		}			
		l3qm_rx_read_start_addresses_flg++;
	}	
	paddr_start.wrd = l3qm_rx_start_addresses[8*cpu_port + voq];
#else
	paddr_start.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (cpu_port * l3qm_cpu_voq_per_port + voq) *
					   QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
#endif	
        if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: cpu_port=%d, voq=%d, paddr_start.bf.phy_addr=0x%x\n", __func__, cpu_port, voq, paddr_start.bf.phy_addr);
	}

	return paddr_start.bf.phy_addr;
}

ca_uint32_t aal_l3qm_get_rx_status0(void)
{
	/* get CPU EPP FIFO (VoQ#31-0) status */
	QM_QM_CPU_EPP_STATUS0_t	status0;

	status0.wrd = CA_NE_REG_READ(QM_QM_CPU_EPP_STATUS0);

	return status0.bf.status;
}

ca_uint32_t aal_l3qm_get_rx_status1(void)
{
        /* get CPU EPP FIFO (VoQ#63-32) status */
        QM_QM_CPU_EPP_STATUS1_t status1;

        status1.wrd = CA_NE_REG_READ(QM_QM_CPU_EPP_STATUS1);

        return status1.bf.status;
}

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
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

        /* 64 instances 8 CPU ports each port has 8 VOQs */
        rdptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_RDPTR + instance * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_RDPTR_STRIDE);

        return rdptr.bf.rptr;
}

ca_uint32_t aal_l3qm_cpu256_get_rx_write_ptr(ca_uint16_t instance, ca_uint16_t *count)
{
	ca_uint32_t rdptr;
	ca_uint32_t epp_size, size;
        QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_WRPTR_t      wrptr;

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

        /* 64 instances 8 CPU ports each port has 8 VOQs */
        paddr_start.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START + instance * QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_STRIDE);

        return paddr_start.bf.phy_addr;
}
#endif

ca_uint16_t aal_l3qm_get_inactive_bid_cntr(int eqid)
{
	int j;
	QM_QM_EQM_PA_REQ0_t eqm_pa_req;

	eqm_pa_req.wrd = 0;
	for (j = 0; j < L3QM_INACTIVE_BID_CNTR; j++) {
		eqm_pa_req.wrd = CA_NE_REG_READ(QM_QM_EQM_PA_REQ0 + QM_QM_EQM_PA_REQ0_STRIDE * eqid);
		if (eqm_pa_req.bf.req) {
			break;
		}
	}

	if (j == L3QM_INACTIVE_BID_CNTR) {
		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: EQM is not ready for push physical address!!!\n", __func__);
		}
		return 0;
	}

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: eqid=%d, bid_cntr=%d\n", __func__, eqid, eqm_pa_req.bf.inactive_bid_cntr);
	}
	return eqm_pa_req.bf.inactive_bid_cntr;
}

void aal_l3qm_load_eq_config(void)
{
	/* load the configuration */
	CA_NE_REG_WRITE(0x05102013, QM_QM_HDM_WRITE_PROTECTION);
	CA_NE_REG_WRITE(0xffff, QM_QM_EQ_CFG_LOAD);
	CA_NE_REG_WRITE(0, QM_QM_EQ_CFG_LOAD);
	CA_NE_REG_WRITE(0, QM_QM_HDM_WRITE_PROTECTION);
}

void aal_l3qm_load_epp_config(void)
{
#if 0
        /* load the configuration */
        CA_NE_REG_WRITE(0x05102013, QM_QM_HDM_WRITE_PROTECTION);
        CA_NE_REG_WRITE(0xff, QM_QM_CPU_EPP_CFG_LOAD);
        CA_NE_REG_WRITE(0, QM_QM_CPU_EPP_CFG_LOAD);
        CA_NE_REG_WRITE(0, QM_QM_HDM_WRITE_PROTECTION);
#endif
}

ca_uint32_t aal_l3qm_check_cpu_push_ready(int cpu_port)
{
	int i;
	QM_QM_CPU_PUSH_RDY0_t	cpu_push_rdy;

	/* check ready bit */
	for (i = 0; i < L3QM_CPU_PUSH_RDY_TIMEOUT; i++) {
		cpu_push_rdy.wrd = CA_NE_REG_READ(QM_QM_CPU_PUSH_RDY0 + (QM_QM_CPU_PUSH_RDY0_STRIDE * cpu_port));
		if (cpu_push_rdy.bf.rdy) {
			break;
		}
	}
	if (i == L3QM_CPU_PUSH_RDY_TIMEOUT) {
		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: EQM is not ready for push physical address!!!\n", __func__);
		}
		return 0;
	}
	return 1;
}

void aal_l3qm_set_cpu_push_paddr(int cpu_port, int eqid, ca_uint32_t phy_addr)
{
	QM_QM_CPU_PUSH_PADDR0_t cpu_push_paddr;

	cpu_push_paddr.bf.addr = phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
	cpu_push_paddr.bf.eqid = eqid;

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: cpu_port=%d, eqid=%d, phy_addr=0x%x, cpu_push_paddr=0x%x\n", __func__, cpu_port, eqid, phy_addr, cpu_push_paddr.wrd);
	}

	CA_NE_REG_WRITE(cpu_push_paddr.wrd, QM_QM_CPU_PUSH_PADDR0 + (QM_QM_CPU_PUSH_PADDR0_STRIDE * cpu_port));
}

void aal_l3qm_set_epp_axi_attrib(int entry, ca_uint32_t data0)
{
	int i;
	QM_QM_AXI_ATTRIBUTE_ACCESS_t	attrib;
	QM_QM_AXI_ATTRIBUTE_DATA0_t	attrib_data0;

	attrib_data0.wrd = data0;
	CA_NE_REG_WRITE(attrib_data0.wrd, QM_QM_AXI_ATTRIBUTE_DATA0);

	attrib.bf.access = 1;
	attrib.bf.rbw = 1;
	attrib.bf.ADDR = entry;
	CA_NE_REG_WRITE(attrib.wrd, QM_QM_AXI_ATTRIBUTE_ACCESS);
	for (i = 0; i < CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT; i++) {
		attrib.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_ACCESS);
		if (!attrib.bf.access)
			break;
	}
	if (i == CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT) {
		printk("%s: CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT!!!\n", __func__);
		return;
	}

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: attrib_data0.wrd=0x%x, QM_QM_AXI_ATTRIBUTE_DATA0=0x%x\n", __func__, attrib_data0.wrd, QM_QM_AXI_ATTRIBUTE_DATA0);
	}
}

#define AAL_L3QM_MAX_BID        16384
typedef struct aal_l3qm_bid_entry {
	ca_uint8_t  eqid;
	ca_uint16_t bid_start;
	ca_uint16_t bid_count;
} aal_l3qm_bid_entry_t;
static int aal_l3qm_bid_entry_count = 0;
static aal_l3qm_bid_entry_t bid_entry[QM_QM_CFG1_EQ0_COUNT];
static void aal_l3qm_add_bid(ca_uint8_t eqid, ca_uint16_t bid_start, ca_uint16_t bid_count)
{
	int i;

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: aal_l3qm_bid_entry_count=%d\n", __func__, aal_l3qm_bid_entry_count);
	}
	if (aal_l3qm_bid_entry_count >= AAL_L3QM_MAX_BID) {
		printk("%s: aal_l3qm_bid_entry_count=%d bid table is full!!\n", __func__, aal_l3qm_bid_entry_count);
		return;
	}

	/* find the index for new added bid */
	i = 0;
	while (i < aal_l3qm_bid_entry_count) {
		if (bid_start < bid_entry[i].bid_start) {
			if (ca_aal_debug & AAL_DBG_L3QM) {
				printk("%s: aal_l3qm_bid_entry_count - i=%d\n", __func__, aal_l3qm_bid_entry_count - i);
			}
			memcpy(&(bid_entry[i+1]), &(bid_entry[i]), (aal_l3qm_bid_entry_count - i) * sizeof(aal_l3qm_bid_entry_t));
			break;
		}
		i++;
	}
	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: i=%d, eqid=%d, bid_start=%d, bid_count=%d\n", __func__, i, eqid, bid_start, bid_count);
	}
	bid_entry[i].eqid = eqid;
	bid_entry[i].bid_start = bid_start;
	bid_entry[i].bid_count = bid_count;
	aal_l3qm_bid_entry_count++;
}

ca_status_t aal_l3qm_check_bid_start(ca_device_id_t device_id)
{
	int i;
	ca_uint16_t next_bid_start;
	ca_uint16_t total_bid_count;
	QM_QM_CFG1_EQ0_t cfg1_eq;

	next_bid_start = 0;

	aal_l3qm_bid_entry_count = 0;
	memset(&(bid_entry[0]), 0, sizeof(bid_entry));

	for (i = 0; i < QM_QM_CFG1_EQ0_COUNT; i++) {
		cfg1_eq.wrd = CA_NE_REG_READ(QM_QM_CFG1_EQ0 + i * QM_QM_CFG1_EQ0_STRIDE);
		if (cfg1_eq.bf.total_buffer_num == 0)		/* skip the eqid not in use */
			continue;

		aal_l3qm_add_bid(i, cfg1_eq.bf.bid_start, cfg1_eq.bf.total_buffer_num);
	}
#if !defined(CONFIG_LUNA_G3_SERIES)
	printk("%s: aal_l3qm_bid_entry_count=%d\n", __func__, aal_l3qm_bid_entry_count);
	total_bid_count = 0;
	for (i = 0; i < aal_l3qm_bid_entry_count; i++) {
		printk("[%d]: eqid=%d bid_start=%d bid_count=%d\n", i, bid_entry[i].eqid, bid_entry[i].bid_start, bid_entry[i].bid_count);
		printk("next_bid_start=%d\n", next_bid_start);

		/* check whether the bid range overlapped */
		if (next_bid_start == 0) {
			next_bid_start = bid_entry[i].bid_start + bid_entry[i].bid_count;
			total_bid_count += bid_entry[i].bid_count;
			continue;
		}
		if (bid_entry[i].bid_start < next_bid_start) {
			printk("%s: bid overlap at eqid=%d, bid_entry[%d].bid_start=%d, next_bid_start=%d!!\n",
					__func__, bid_entry[i].eqid, i, bid_entry[i].bid_start, next_bid_start);
			BUG();
		}
		next_bid_start = bid_entry[i].bid_start + bid_entry[i].bid_count;
		total_bid_count += bid_entry[i].bid_count;

		printk("total_bid_count=%d\n", total_bid_count);
		if (total_bid_count > AAL_L3QM_MAX_BID) {
			printk("%s: total bid count %d exceed %d!!\n", __func__, total_bid_count, AAL_L3QM_MAX_BID);
			BUG();
		}
		}
#endif
	return CA_E_OK;
}

ca_status_t aal_l3qm_check_epps(ca_device_id_t device_id)
{
	int i, j, size;
	ca_uint8_t *virt_addr;

#if 0
	/* check magic number among PE0 EPPs */
	size = l3qm_pe0_1_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
	for (i = 0; i < l3qm_pe0_1_voq_per_port; i++) {
		virt_addr = l3qm_epp_profile_pe0_rx_fifo_start_addr + (i * size);
		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: PE0 RX FIFO start address(%d)=0x%p\n", __func__, i, virt_addr + (size - l3qm_magic_marker_len));
		}
		if (aal_l3qm_check_magic_number(virt_addr + (size - l3qm_magic_marker_len)) != CA_E_OK) {
			printk("%s: magic number check failed at PE0 EPP (%d)!!!\n", __func__, i);
			return CA_E_ERROR;
		}
	}
	printk("Magic number among PE0 EPPs are OK\n");

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        /* check magic number among PE0 EPPs */
        size = l3qm_pe0_1_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
        for (i = 0; i < l3qm_pe0_1_voq_per_port; i++) {
                virt_addr = l3qm_epp_profile_pe1_rx_fifo_start_addr + (i * size);
                if (ca_aal_debug & AAL_DBG_L3QM) {
                        printk("%s: PE1 RX FIFO start address(%d)=0x%p\n", __func__, i, virt_addr + (size - l3qm_magic_marker_len));
                }
                if (aal_l3qm_check_magic_number(virt_addr + (size - l3qm_magic_marker_len)) != CA_E_OK) {
                        printk("%s: magic number check failed at PE1 EPP (%d)!!!\n", __func__, i);
                        return CA_E_ERROR;
                }
        }
	printk("Magic number among PE1 EPPs are OK\n");
#endif
#endif
	/* check magic number among CPU EPPs */
	size = l3qm_cpu_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
	for (j = 0; j < ca_ni_cpu_cpu_port_count; j++) {
		for (i = 0; i < l3qm_cpu_voq_per_port; i++) {
			virt_addr = (ca_uint8_t *)(l3qm_epp_profile_cpu_rx_fifo_start_addr + (i * size) + (j * l3qm_cpu_voq_per_port * size));
			if (ca_aal_debug & AAL_DBG_L3QM) {
				printk("%s: CPU RX FIFO start address(%d)=0x%p\n", __func__, i, virt_addr + (size - l3qm_magic_marker_len));
			}
			if (aal_l3qm_check_magic_number(virt_addr + (size - l3qm_magic_marker_len)) != CA_E_OK) {
				printk("%s: magic number check failed at CPU EPP (%d, %d)!!!\n", __func__, j, i);
				return CA_E_ERROR;
			}
		}
	}
	printk("Magic number among CPU EPPs are OK\n");

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	/* check magic number among CPU256 EPPs */
        size = l3qm_cpu256_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
        for (j = 0; j < l3qm_cpu256_port_count; j++) {
                for (i = 0; i < l3qm_cpu256_voq_per_port; i++) {
                        virt_addr = l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr + (i * size) + (j * l3qm_cpu256_voq_per_port * size);

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
#else
  #if defined(CONFIG_NE_CPU256)
	if (support_cpu256_in_linux)
	aal_l3qm_cpu256_check_magic_number();
  #endif
#endif
#endif

	return CA_E_OK;
}

static void aal_l3qm_init_qm_axi_attrib(void)
{
	int i;
	ca_uint32_t offset, value;
	QM_AXIM2_CONFIG_t	axim2_cfg;

	/* AXIM2 */
	axim2_cfg.wrd = CA_NE_REG_READ(QM_AXIM2_CONFIG);
	axim2_cfg.bf.axi_write_outtrans_num = 0xf;
	axim2_cfg.bf.axi_read_outtrans_num = 0xf;
	axim2_cfg.bf.write_cacheline_trans_en = 0;
	axim2_cfg.bf.read_cacheline_trans_en = 0;
	CA_NE_REG_WRITE(axim2_cfg.wrd, QM_AXIM2_CONFIG);

	/* EQ 0-15 */
	for (i = 0; i < CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES_LEN; i++) {
		offset = CA_L3QM_AXI_ATTRIBUTE_EQ_BASE + i;

		value = l3qm_axi_attrib_eq_values[i];
		if (i == l3qm_eq_profile_dq_pool0_eq_id || i == l3qm_eq_profile_dq_pool1_eq_id) {
			if (l3qm_deepq_test_with_sram) {
				value = 0;
			}
		}
		aal_l3qm_set_epp_axi_attrib(offset, value);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: EQ 0-15 offset=0x%x, l3qm_axi_attrib_eq_values[%d]=0x%x\n", __func__, offset, i, l3qm_axi_attrib_eq_values[i]);
		}
	}

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	/* CPU256 EPP port 0-31 */
	for (i = 0; i < CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES_LEN; i++) {
		offset = CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE + i;
		aal_l3qm_set_epp_axi_attrib(offset, l3qm_axi_attrib_cpu256_epp_values[i]);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: CPU256 EPP offset=0x%x, l3qm_axi_attrib_cpu256_epp_values[%d]=0x%x\n", __func__, offset, i, l3qm_axi_attrib_cpu256_epp_values[i]);
		}
	}
#else
  #if defined(CONFIG_NE_CPU256)
	if (support_cpu256_in_linux)
	aal_l3qm_init_cpu256_qm_axi_attrib();
  #endif
#endif
#endif

	/* CPU EPP port 0-7 */
	for (i = 0; i < CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES_LEN; i++) {
		offset = CA_L3QM_AXI_ATTRIBUTE_CPU_BASE + i;
		aal_l3qm_set_epp_axi_attrib(offset, l3qm_axi_attrib_cpu_epp_values[i]);

                if (ca_aal_debug & AAL_DBG_L3QM) {
                        printk("%s: CPU EPP offset=0x%x, l3qm_axi_attrib_cpu_epp_values[%d]=0x%x\n", __func__, offset, i, l3qm_axi_attrib_cpu_epp_values[i]);
                }
	}
}

/* Sending packets to CPU port through L3QM - Transmitting */
/* Configure CPU EPP FIFO for receiving packets for CPU */
//void aal_l3qm_init_cpu_epp(ca_uint32_t *virt_start_addr[])
void aal_l3qm_init_cpu_epp(void)
{
	int i, j, size;
	ca_uint32_t reg_off;
	//ca_uint32_t phy_addr;
	QM_QM_CPU_EPP0_CFG_t cpu_epp0_cfg;
	QM_QM_CPU_EPP_FIFO_CFG_profile0_t cpu_epp_fifo_cfg_profile;
	QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_t cpu_epp64_fifo_paddr_start;

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_t cpu_epp256_fifo_paddr_start;
	QM_QM_CPU_EPP_FIFO_256_CFG_t	cpu256_epp_cfg;

	QM_QM_DEST_CPU_EPP256_HI_BUF_CFG_t	epp256_hi_buf_cfg;
	QM_QM_DEST_CPU_EPP256_LO_BUF_CFG_t	epp256_lo_buf_cfg;
#endif
#endif
	ca_uint8_t *virt_addr;

	/* CPU EPP Port Configuration Register */
	/* Select per-destionation VOQ-to-EPP-FIFO-Ring map-mode */
	for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
		reg_off = QM_QM_CPU_EPP0_CFG + (QM_QM_CPU_EPP0_CFG_STRIDE * i);
		cpu_epp0_cfg.wrd = CA_NE_REG_READ(reg_off);
		//cpu_epp0_cfg.bf.axi_top_bits = 0;
		cpu_epp0_cfg.bf.map_mode = l3qm_epp_profile_cpu_map_mode;
		CA_NE_REG_WRITE(cpu_epp0_cfg.wrd, reg_off);
	}

	/* CPU EPP Interrupt Coalescing Timer Configuration Register */
	/* Select coalescing Timer-base */
	CA_NE_REG_WRITE(0, QM_QM_CPU_EPP_CT_CFG);

	/* CPU EPP FIFO Configuration Register */
	/* only one EPP profile is used for CPU, the size is for one VoQ */
	reg_off = QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * l3qm_epp_profile_cpu_id);
	cpu_epp_fifo_cfg_profile.wrd = CA_NE_REG_READ(reg_off);
	cpu_epp_fifo_cfg_profile.bf.size = l3qm_cpu_epp_per_voq / CA_L3QM_CPU_EPP_FIFO_SIZE_UINT;  	/* the unit is 4 FIFO entries */
	cpu_epp_fifo_cfg_profile.bf.timer_ths = 0xf;
	if ((ca_ni_napi_budget / 16) > 0)
		cpu_epp_fifo_cfg_profile.bf.high_ths = (ca_ni_napi_budget / 16) + 1;	/* Interrupt to CPU is generated when EPP-FIFO accumulates at least ca_napi_budget entries */
	else
		cpu_epp_fifo_cfg_profile.bf.high_ths = 2;                               /* Interrupt to CPU is generated when EPP-FIFO accumulates at least 16 entries */
	CA_NE_REG_WRITE(cpu_epp_fifo_cfg_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: l3qm_cpu_epp_per_voq=%d\n", __func__, l3qm_cpu_epp_per_voq);
	}

	/* CPU EPP FIFO Starting address Register */
	/* Select one of 4 profiles for EPP-FIFO size, occupancy-high-threshold and time-threshold for CPU ports 0-5 */
	//size = l3qm_epp_profile_cpu_cn/CA_NI_NUM_VOQ_PER_PORT/(CA_NI_TOTAL_CPU_PORT - 2);
	for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
		for (j = 0; j < l3qm_cpu_voq_per_port; j++) {
			/* select profile 2 */
			reg_off = QM_QM_CPU_EPP_FIFO0_0_CFG + (QM_QM_CPU_EPP_FIFO0_0_CFG_STRIDE * i) + (4 * j);
			CA_NE_REG_WRITE(l3qm_epp_profile_cpu_id, reg_off);
		}
	}
	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: l3qm_epp_profile_cpu_id=%d\n", __func__, l3qm_epp_profile_cpu_id);
	}

	/* fill the physical start address for CPU EPPs */
	size = l3qm_cpu_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
        for (j = 0; j < ca_ni_cpu_cpu_port_count; j++) {
                for (i = 0; i < l3qm_cpu_voq_per_port; i++) {
                        reg_off = QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE) +
				(j * l3qm_cpu_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
                        cpu_epp64_fifo_paddr_start.bf.phy_addr = l3qm_epp_profile_cpu_rx_fifo_phy_addr + (i * size) + (j * l3qm_cpu_voq_per_port * size);
                        CA_NE_REG_WRITE(cpu_epp64_fifo_paddr_start.wrd, reg_off);

                        if (ca_aal_debug & AAL_DBG_L3QM) {
				printk("%s: CPU RX FIFO register offset(%d, %d)=0x%x\n", __func__, j, i, reg_off);
                                printk("%s: CPU RX FIFO start address(%d,%d)=0x%x\n", __func__, j, i, cpu_epp64_fifo_paddr_start.bf.phy_addr);
				printk("%s: size=%d, offset=%d\n", __func__, size, (i * size) + (j * l3qm_cpu_voq_per_port * size));
                        }
			virt_addr = (ca_uint8_t *)(l3qm_epp_profile_cpu_rx_fifo_start_addr + (i * size) + (j * l3qm_cpu_voq_per_port * size) + (size - l3qm_magic_marker_len));
			aal_l3qm_insert_magic_number(virt_addr);
                }
        }

#if 0
	/* fill AXI top bits for DDR coherent for CPU EPPs */
	for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
		aal_l3qm_set_epp_axi_attrib(CA_L3QM_AXI_ATTRIBUTE_CPU_BASE + i, CA_L3QM_AXI_TOP_DDR_COHERENT_BIT);
	}
#endif
#if 0
	/* only one EPP profile is used for PE0 */
	reg_off = QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * l3qm_epp_profile_pe0_id);
	cpu_epp_fifo_cfg_profile.wrd = CA_NE_REG_READ(reg_off);
	cpu_epp_fifo_cfg_profile.bf.size = l3qm_pe0_1_epp_per_voq / CA_L3QM_CPU_EPP_FIFO_SIZE_UINT;	 /* the unit is 4 FIFO entries */
	CA_NE_REG_WRITE(cpu_epp_fifo_cfg_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: l3qm_pe0_1_epp_per_voq=%d\n", __func__, l3qm_pe0_1_epp_per_voq);
	}

	/* Select one of 4 profiles for EPP-FIFO size, occupancy-high-threshold and time-threshold for CPU port 7 (PE0) */
	for (j = 0; j < CA_NI_NUM_VOQ_PER_PORT; j++) {
		/* select profile 0 */
		reg_off = QM_QM_CPU_EPP_FIFO0_0_CFG + (QM_QM_CPU_EPP_FIFO0_0_CFG_STRIDE * CA_NI_PE0_PORT + (4 * j));
		CA_NE_REG_WRITE(l3qm_epp_profile_pe0_id, reg_off);
	}

	/* fill physical start address for PE0 EPP */
	size = l3qm_pe0_1_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
	for (i = 0; i < l3qm_pe0_1_voq_per_port; i++) {
		reg_off = QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE) +
			(CA_NI_PE0_PORT * l3qm_pe0_1_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
		cpu_epp64_fifo_paddr_start.bf.phy_addr = l3qm_epp_profile_pe0_rx_fifo_phy_addr + (i * size);
		CA_NE_REG_WRITE(cpu_epp64_fifo_paddr_start.wrd, reg_off);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: PE0 RX FIFO register offset(%d)=0x%x\n", __func__, i, reg_off);
			printk("%s: PE0 RX FIFO start address(%d)=0x%x\n", __func__, i, cpu_epp64_fifo_paddr_start.bf.phy_addr);
		}
		virt_addr = l3qm_epp_profile_pe0_rx_fifo_start_addr + (i * size) + (size - l3qm_magic_marker_len);
		aal_l3qm_insert_magic_number(virt_addr);
	}
#endif


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#if 0
	/* only one EPP profile is used for PE1 */
	reg_off = QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * l3qm_epp_profile_pe1_id);
        cpu_epp_fifo_cfg_profile.wrd = CA_NE_REG_READ(reg_off);
        cpu_epp_fifo_cfg_profile.bf.size = l3qm_pe0_1_epp_per_voq / CA_L3QM_CPU_EPP_FIFO_SIZE_UINT;    /* the unit is 4 FIFO entries */
        CA_NE_REG_WRITE(cpu_epp_fifo_cfg_profile.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: l3qm_pe0_1_epp_per_voq=%d\n", __func__, l3qm_pe0_1_epp_per_voq);
        }


        /* Select one of 4 profiles for EPP-FIFO size, occupancy-high-threshold and time-threshold for CPU port 6 (PE1)  */
        for (j = 0; j < CA_NI_NUM_VOQ_PER_PORT; j++) {
                /* select profile 1 */
                reg_off = QM_QM_CPU_EPP_FIFO0_0_CFG + (QM_QM_CPU_EPP_FIFO0_0_CFG_STRIDE * CA_NI_PE1_PORT + (4 * j));
                CA_NE_REG_WRITE(l3qm_epp_profile_pe1_id, reg_off);
        }

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: l3qm_epp_profile_pe1_id=%d\n", __func__, l3qm_epp_profile_pe1_id);
        }

        /* fill physical start address for PE1 EPP */
	size = l3qm_pe0_1_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;;
	for (i = 0; i < l3qm_pe0_1_voq_per_port; i++) {
		reg_off = QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE) +
			(CA_NI_PE1_PORT * l3qm_pe0_1_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
		cpu_epp64_fifo_paddr_start.bf.phy_addr = l3qm_epp_profile_pe1_rx_fifo_phy_addr + (i * size);
		CA_NE_REG_WRITE(cpu_epp64_fifo_paddr_start.wrd, reg_off);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: PE0 RX FIFO register offset(%d)=0x%x\n", __func__, i, reg_off);
			printk("%s: PE0 RX FIFO start address(%d)=0x%x\n", __func__, i, cpu_epp64_fifo_paddr_start.bf.phy_addr);
		}
		virt_addr = l3qm_epp_profile_pe1_rx_fifo_start_addr + (i * size) + (size - l3qm_magic_marker_len);
		aal_l3qm_insert_magic_number(virt_addr);
	}
#endif

/* moved to aal_l3qm_cpu256.c */
#if 0
	/* only one EPP profile is used for CPU256 */
	reg_off = QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * l3qm_epp_profile_cpu256_hi_id);
	cpu_epp_fifo_cfg_profile.wrd = CA_NE_REG_READ(reg_off);
	cpu_epp_fifo_cfg_profile.bf.size = l3qm_cpu256_epp_per_voq / CA_L3QM_CPU_EPP_FIFO_SIZE_UINT; 	 /* the unit is 4 FIFO entries */
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

			virt_addr = l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr + (i * size) + (j * l3qm_cpu256_voq_per_port * size) + (size - l3qm_magic_marker_len);
			aal_l3qm_insert_magic_number(virt_addr);
		}
	}

	/* fill EPP profile id */
	reg_off = QM_QM_CPU_EPP_FIFO_256_CFG;
	cpu256_epp_cfg.wrd = 0;
	cpu256_epp_cfg.bf.profile_sel_lo = l3qm_epp_profile_cpu256_hi_id;
	cpu256_epp_cfg.bf.profile_sel_hi = l3qm_epp_profile_cpu256_hi_id;
	CA_NE_REG_WRITE(cpu256_epp_cfg.wrd, reg_off);

	epp256_hi_buf_cfg.wrd = CA_NE_REG_READ(QM_QM_DEST_CPU_EPP256_HI_BUF_CFG);
	epp256_hi_buf_cfg.bf.head_room_first = l3qm_cpu_port_head_room_first / 16;       /* reserved 32 bytes for first fragment of packet  */
	epp256_hi_buf_cfg.bf.head_room_rest = epp256_hi_buf_cfg.bf.head_room_first;       /* reserved 32 bytes for reset of segments of packet  */
	CA_NE_REG_WRITE(epp256_hi_buf_cfg.wrd, QM_QM_DEST_CPU_EPP256_HI_BUF_CFG);

	epp256_lo_buf_cfg.wrd = CA_NE_REG_READ(QM_QM_DEST_CPU_EPP256_LO_BUF_CFG);
	epp256_lo_buf_cfg.bf.head_room_first = l3qm_cpu_port_head_room_first / 16;       /* reserved 32 bytes for first fragment of packet  */
	epp256_lo_buf_cfg.bf.head_room_rest = epp256_lo_buf_cfg.bf.head_room_first;       /* reserved 32 bytes for reset of segments of packet  */
	CA_NE_REG_WRITE(epp256_lo_buf_cfg.wrd, QM_QM_DEST_CPU_EPP256_LO_BUF_CFG);
#else
#if defined(CONFIG_NE_CPU256)
	if (support_cpu256_in_linux)
	aal_l3qm_init_cpu256_epp();
#endif
#endif
#endif

#if 0
        /* fill AXI top bits for DDR coherent for CPU256 EPPs */
        for (i = 0; i < l3qm_cpu256_port_count; i++) {
                aal_l3qm_set_epp_axi_attrib(CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE + i, CA_L3QM_AXI_TOP_DDR_NONCACHE_BIT);
        }
#endif

	aal_l3qm_load_epp_config();
}

ca_uint8_t aal_l3qm_get_buffer_size_index(ca_uint32_t buffer_size)
{
	ca_uint8_t index;

	switch (buffer_size) {
		case 128:
			index = 1;
			break;
		case 256:
			index = 2;
			break;
		case 512:
			index = 3;
			break;
		case 1024:
			index = 4;
			break;
		case 2048:
			index = 5;
			break;
		case 4096:
			index = 6;
			break;
		default:
			printk("%s: buffer_size=%d is not defined!!\n", __func__, buffer_size);
			index = 0;
			break;
	}
	return index;
}

#if 0
static void aal_l3qm_init_empty_buffer_PE(void)
{
	volatile ca_uint32_t reg_off;
	QM_QM_EQ_PROFILE0_t eq_profile;
	QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
	QM_QM_CFG0_EQ0_t cfg0_eq;
	QM_QM_CFG1_EQ0_t cfg1_eq;
	QM_QM_CFG2_EQ0_t cfg2_eq;
	QM_QM_CFG3_EQ0_t cfg3_eq;
	QM_QM_CFG4_EQ0_t cfg4_eq;
	ca_uint32_t phy_start_addr, entry_size, entry_count;
	ca_uint8_t fbm_pool_id;

	/* one profile for PE0 */
	eq_profile.wrd = 0;
	eq_profile.bf.eqp0 = l3qm_eq_profile_pe0_pool0_eq_id;
	eq_profile.bf.eqp1 = l3qm_eq_profile_pe0_pool1_eq_id;
	eq_profile.bf.rule = l3qm_eq_profile_pe0_rule;
	reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_pe0_id);
	CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE0 pool0 eqid=%d, pool1 eqid=%d, rule=%d\n", __func__,
			l3qm_eq_profile_pe0_pool0_eq_id, l3qm_eq_profile_pe0_pool1_eq_id, l3qm_eq_profile_pe0_rule);
	}

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU
	/* one profile for PE1 */
	eq_profile.wrd = 0;
	eq_profile.bf.eqp0 = l3qm_eq_profile_pe1_pool0_eq_id;
	eq_profile.bf.eqp1 = l3qm_eq_profile_pe1_pool1_eq_id;
	eq_profile.bf.rule = l3qm_eq_profile_pe1_rule;
	reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_pe1_id);
	CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE1 pool0 eqid=%d, pool1 eqid=%d, rule=%d\n", __func__,
				l3qm_eq_profile_pe1_pool0_eq_id, l3qm_eq_profile_pe1_pool1_eq_id, l3qm_eq_profile_pe1_rule);
	}
#endif

	dest_port_eq_cfg.wrd = 0;
	/* for CPU ports profile select, profile 0 for PE0 at CPU port 6 */
	dest_port_eq_cfg.bf.profile_sel = l3qm_eq_profile_pe0_id;
	reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * CA_NI_PE0_PORT);
	CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	/* for CPU ports profile select, profile 0 for PE1 at CPU port 7 */
	dest_port_eq_cfg.bf.profile_sel = l3qm_eq_profile_pe1_id;
	reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * CA_NI_PE1_PORT);
	CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: profile PE0 id=%d, PE1 id=%d\n", __func__, l3qm_eq_profile_pe0_id, l3qm_eq_profile_pe1_id);
	}
#endif
	/* configure EB pool 0/1 for PE 0 */


	/* pool 0, enable Empty Buffer and set phy_addr_start */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
	//cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_pe0_pool0_buff_phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_pe0_pool0_eq_id);
	CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE0 pool0 phy_addr_start=0x%x\n", __func__, cfg0_eq.bf.phy_addr_start);
	}

	cfg1_eq.wrd = 0;
	cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_pe0_pool0_buf_count;
	cfg1_eq.bf.bid_start = l3qm_eq_profile_pe0_pool0_bid_start;
	reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_pe0_pool0_eq_id);
	CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);


        if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE0 pool0 buf_count=%d, bid_start=%d\n", __func__,
			l3qm_eq_profile_pe0_pool0_buf_count, l3qm_eq_profile_pe0_pool0_bid_start);
	}

	/* configure buffer size for Empty Buffer */
	aal_l3qm_eq_get_pe0_pool0_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_pe0_pool0_eq_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
	cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
	if (l3qm_eq_profile_pe0_use_fbm)
		cfg2_eq.bf.refill_en = 1;
	else
		cfg2_eq.bf.refill_en = 0;
	cfg2_eq.bf.refill_fbm_eqid = fbm_pool_id;
	cfg2_eq.bf.cpu_eq = 1;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_pe0_pool0_buf_sz);      /* 256 bytes */
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE0 pool0 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_pe0_pool0_buf_sz, cfg2_eq.bf.buffer_size);
	}

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_pe0_pool0_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[0];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        /* because PE0 pool1 locates at DDR start from 0x180000000 */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_pe0_pool0_eq_id);
        cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_pe0_pool0_eq_id];
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);


	/* pool 1, enable Empty Buffer and set physical address */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
        //cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_pe0_pool1_buff_phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;      /* in DDR */
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_pe0_pool1_eq_id);
	CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE0 pool1 eq_id=%d\n", __func__, l3qm_eq_profile_pe0_pool1_eq_id);
		printk("%s: PE0 pool1 phy_addr_start = 0x%x\n", __func__, cfg0_eq.bf.phy_addr_start);
	}

	cfg1_eq.wrd = 0;
	cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_pe0_pool1_buf_count;
	cfg1_eq.bf.bid_start = l3qm_eq_profile_pe0_pool1_bid_start;
	reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_pe0_pool1_eq_id);
	CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE0 pool1 buf_count=%d, bid_start=%d\n", __func__,
			l3qm_eq_profile_pe0_pool1_buf_count, l3qm_eq_profile_pe0_pool1_bid_start);
	}

	/* configure buffer size for Empty Buffer */
	aal_l3qm_eq_get_pe0_pool1_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_pe0_pool1_eq_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
	cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
	if (l3qm_eq_profile_pe0_use_fbm)
		cfg2_eq.bf.refill_en = 1;
	else
		cfg2_eq.bf.refill_en = 0;
	cfg2_eq.bf.refill_fbm_eqid = fbm_pool_id;
	cfg2_eq.bf.cpu_eq = 1;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_pe0_pool1_buf_sz);      /* 2048 bytes */
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE0 pool1 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_pe0_pool1_buf_sz, cfg2_eq.bf.buffer_size);
	}

	/* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_pe0_pool1_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[1];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	/* because PE0 pool1 locates at DDR start from 0x180000000 */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_pe0_pool1_eq_id);
	cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_pe0_pool1_eq_id];
	CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);


#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	/* configure EB pool 0/1 for PE 1 */
	/* pool 0, enable Empty Buffer, no need to set phy_addr_start because the buffer should be pushed into L3QM */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
	//cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_pe1_pool0_buff_phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_pe1_pool0_eq_id);
	CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE1 pool0 eq_id=%d\n", __func__, l3qm_eq_profile_pe1_pool0_eq_id);
		printk("%s: PE0 pool0 phy_addr_start=0x%x\n", __func__, cfg0_eq.bf.phy_addr_start);
	}


	cfg1_eq.wrd = 0;
	cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_pe1_pool0_buf_count;
	cfg1_eq.bf.bid_start = l3qm_eq_profile_pe1_pool0_bid_start;
	reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_pe1_pool0_eq_id);
	CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE1 pool0 buf_cnt=%d, bid_start=%d\n",
			__func__, l3qm_eq_profile_pe1_pool0_buf_count, l3qm_eq_profile_pe1_pool0_bid_start);
	}

	/* configure buffer size for Empty Buffer */
	aal_l3qm_eq_get_pe1_pool0_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_pe1_pool0_eq_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
	cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
	if (l3qm_eq_profile_pe1_use_fbm)
		cfg2_eq.bf.refill_en = 1;
	else
		cfg2_eq.bf.refill_en = 0;
	cfg2_eq.bf.refill_fbm_eqid = fbm_pool_id;
	cfg2_eq.bf.cpu_eq = 1;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_pe1_pool0_buf_sz);      /* 256 bytes */
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE1 pool0 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_pe1_pool0_buf_sz, cfg2_eq.bf.buffer_size);
	}

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_pe1_pool0_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[2];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        /* because PE0 pool1 locates at DDR start from 0x180000000 */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_pe1_pool0_eq_id);
        cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_pe1_pool0_eq_id];
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

	/* pool 1, enable Empty Buffer, no need to set phy_addr_start because the buffer should be pushed into L3QM */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
	//cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_pe1_pool1_buff_phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_pe1_pool1_eq_id);
	CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE1 pool1 eq_id=%d\n", __func__, l3qm_eq_profile_pe1_pool1_eq_id);
		printk("%s: PE1 pool1 phy_addr_start=0x%x\n", __func__, cfg0_eq.bf.phy_addr_start);
	}

	cfg1_eq.wrd = 0;
	cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_pe1_pool1_buf_count;
	cfg1_eq.bf.bid_start = l3qm_eq_profile_pe1_pool1_bid_start;
	reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_pe1_pool1_eq_id);
	CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE1 pool1 buf_count=%d, bid_start=%d\n",
			__func__, l3qm_eq_profile_pe1_pool1_buf_count, l3qm_eq_profile_pe1_pool1_bid_start);
	}

	/* configure buffer size for Empty Buffer */
	aal_l3qm_eq_get_pe1_pool1_info(&phy_start_addr, &entry_size, &entry_count, &fbm_pool_id);
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_pe1_pool1_eq_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
	cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
	if (l3qm_eq_profile_pe0_use_fbm)
		cfg2_eq.bf.refill_en = 1;
	else
		cfg2_eq.bf.refill_en = 0;
	cfg2_eq.bf.refill_fbm_eqid = fbm_pool_id;
	cfg2_eq.bf.cpu_eq = 1;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_pe1_pool1_buf_sz);      /* 2048 bytes */
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: PE1 pool1 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_pe1_pool1_buf_sz, cfg2_eq.bf.buffer_size);
	}

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_pe1_pool1_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[3];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        /* because PE0 pool1 locates at DDR start from 0x180000000 */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_pe1_pool1_eq_id);
        cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_pe1_pool1_eq_id];
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);
#endif

}
#endif

static void aal_l3qm_init_empty_buffer_CPU(void)
{
	int i;
	ca_uint32_t reg_off;
	QM_QM_EQ_PROFILE0_t eq_profile;
	QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
	QM_QM_DEST_PORT0_PKT_BUF_CFG_t dest_port_pkt_buf_cfg;
	QM_QM_CFG0_EQ0_t cfg0_eq;
	QM_QM_CFG1_EQ0_t cfg1_eq;
	QM_QM_CFG2_EQ0_t cfg2_eq;
	QM_QM_CFG3_EQ0_t cfg3_eq;
        QM_QM_CFG4_EQ0_t cfg4_eq;

	/* only one profile is assigned to CPU */
	eq_profile.wrd = 0;
	eq_profile.bf.eqp0 = l3qm_eq_profile_cpu_pool0_eq_id;
	eq_profile.bf.eqp1 = l3qm_eq_profile_cpu_pool1_eq_id;
	eq_profile.bf.rule = l3qm_eq_profile_cpu_rule;
	reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_cpu_id);
	CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU pool0 eq_id=%d, rule=%d\n", __func__, l3qm_eq_profile_cpu_pool0_eq_id, l3qm_eq_profile_cpu_rule);
	}

#if 0
	/* one profile for NI ports */
	eq_profile.wrd = 0;
	eq_profile.bf.eqp0 = L3QM_EQ_PROFILE_CPU_NI;
	reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * L3QM_EQ_PROFILE_CPU_NI);
	CA_NE_REG_WRITE(eq_profile.wrd, reg_off);
#endif

	/* for CPU ports profile select, profile 2 for CPU CPU ports 0-5 */
	for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
		dest_port_eq_cfg.bf.profile_sel = l3qm_eq_profile_cpu_id;
		reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * i);
		CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);
	}

        if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: CPU profile id=%d\n", __func__, l3qm_eq_profile_cpu_id);
	}

	for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
		reg_off = QM_QM_DEST_PORT0_PKT_BUF_CFG + (QM_QM_DEST_PORT0_PKT_BUF_CFG_STRIDE * i);
		dest_port_pkt_buf_cfg.wrd = CA_NE_REG_READ(reg_off);
		dest_port_pkt_buf_cfg.bf.head_room_first = l3qm_cpu_port_head_room_first / 16;       /* reserved 32 bytes for first fragment of packet  */
		dest_port_pkt_buf_cfg.bf.head_room_rest = dest_port_pkt_buf_cfg.bf.head_room_first;  /* reserved 32 bytes for reset of segments of packet  */
		CA_NE_REG_WRITE(dest_port_pkt_buf_cfg.wrd, reg_off);
	}

	/* configure total_buffer_num and bid_start for CPU ports, EB pool 0-13 */
	/* only two EB pools are assigned to CPU */
	for (i = l3qm_eq_profile_cpu_pool0_eq_id; i <= l3qm_eq_profile_cpu_pool1_eq_id; i++) {

		/* enable Empty Buffer */
		cfg0_eq.wrd = 0;
		cfg0_eq.bf.eq_en = 1;

#if 0
		/* if rule=1 should assign starting address here */
		if (l3qm_eq_profile_cpu_rule == 1) {
			if (i == l3qm_eq_profile_cpu_pool0_eq_id)
				cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_cpu_pool0_start_addr >> CA_L3QM_PHY_ADDR_SHIFT;
			else
				cfg0_eq.bf.phy_addr_start = l3qm_eq_profile_cpu_pool1_start_addr >> CA_L3QM_PHY_ADDR_SHIFT;
		}
#endif
		reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * i);
		CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

		/* configure total_buffer_num and bid_start */
		cfg1_eq.wrd = 0;
		cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_cpu_pool0_buf_count;

		if (i == l3qm_eq_profile_cpu_pool0_eq_id)
			cfg1_eq.bf.bid_start = l3qm_eq_profile_cpu_pool0_bid_start;
		else
			cfg1_eq.bf.bid_start = l3qm_eq_profile_cpu_pool1_bid_start;

		reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * i);
		CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

	        if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: CPU eq_id=%d, pool0 bid_start=%d\n", __func__, i, l3qm_eq_profile_cpu_pool0_bid_start);
			printk("%s: CPU pool0 total_buffer_num=%d\n", __func__, cfg1_eq.bf.total_buffer_num);
		}

		/* configure buffer size for Empty Buffer */
		reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
		cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
		//cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
		//cfg2_eq.bf.refill_en = 1;
		//cfg2_eq.bf.refill_fbm_eqid = 0;

		printk("%s: l3qm_eq_profile_cpu_use_fbm=%d, l3qm_eq_profile_cpu_pool0_fbm_pool_id=%d, l3qm_eq_profile_cpu_pool1_fbm_pool_id=%d\n", __func__, l3qm_eq_profile_cpu_use_fbm, l3qm_eq_profile_cpu_pool0_fbm_pool_id, l3qm_eq_profile_cpu_pool1_fbm_pool_id);

		if (l3qm_eq_profile_cpu_use_fbm) {
			cfg2_eq.bf.cpu_eq = 1;

			if (i == l3qm_eq_profile_cpu_pool0_eq_id)
				cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu_pool0_fbm_pool_id;
			else
				cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu_pool1_fbm_pool_id;

			/* use the same threshold as non-FBM, because we wish QM REFILL interrupt happened every 160 BIDs comsumed */
			/* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
			/* the budget value should not be too small otherwise the interrupts happened too frequently */
			//cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
                        cfg2_eq.bf.refill_ths = (l3qm_cpu_pool_bid_count - ca_ni_napi_budget) / 8;

			cfg2_eq.bf.refill_en = 1;
		}
		else {
			cfg2_eq.bf.cpu_eq = 1;
			cfg2_eq.bf.refill_fbm_eqid = 0;

			/* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
			/* the budget value should not be too small otherwise the interrupts happened too frequently */
                        cfg2_eq.bf.refill_ths = (l3qm_cpu_pool_bid_count - ca_ni_napi_budget) / 8;

			cfg2_eq.bf.refill_en = 0;
			cfg2_eq.bf.refill_fbm_eqid = 0;
		}

		if (i == l3qm_eq_profile_cpu_pool0_eq_id)
			cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_cpu_pool0_buf_sz);      /* 2048 bytes */
		else
			cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_cpu_pool1_buf_sz);      /* 2048 bytes */

		reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
		CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

		printk("%s: i=%d, reg_off=0x%x, cfg2_eq.wrd=0x%x\n", __func__, i, reg_off, cfg2_eq.wrd);

	        if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: CPU pool buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_cpu_pool0_buf_sz, cfg2_eq.bf.buffer_size);
		}
	}

	/* CPU pool 0 */
        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_cpu_pool0_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_cpu_pool0_eq_id];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu_pool0_eq_id);
        cfg4_eq.wrd = 0;
        cfg4_eq.bf.axi_top_bit = l3qm_eq_cfg4_values[l3qm_eq_profile_cpu_pool0_eq_id];

	printk("%s: l3qm_eq_profile_cpu_pool0_eq_id=%d, cfg4_eq.bf.axi_top_bit=%d\n", __func__, l3qm_eq_profile_cpu_pool0_eq_id, cfg4_eq.bf.axi_top_bit);
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

	/* CPU pool1 */
	/* configure cache_eos/domain/snoop/cache */
	reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_cpu_pool1_eq_id);
	cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_cpu_pool1_eq_id];
	CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu_pool1_eq_id);
        cfg4_eq.wrd = 0;
        cfg4_eq.bf.axi_top_bit = l3qm_eq_cfg4_values[l3qm_eq_profile_cpu_pool1_eq_id];
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);
}

static void aal_saturn_l3qm_init_empty_buffer_DQ(void);
static void aal_l3qm_init_empty_buffer_DQ(void)
{
	int i, pool_id, buf_cnt, buf_size;
	ca_uint32_t reg_off;
	QM_QM_EQ_PROFILE0_t eq_profile;
	QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
	QM_QM_CFG0_EQ0_t cfg0_eq;
	QM_QM_CFG1_EQ0_t cfg1_eq;
	QM_QM_CFG2_EQ0_t cfg2_eq;
	QM_QM_CFG2_EQ0_t cfg3_eq;
	QM_QM_CFG4_EQ0_t cfg4_eq;

	/* one profile for DQ or normal NI ports */
	eq_profile.wrd = 0;
	eq_profile.bf.eqp0 = l3qm_eq_profile_dq_pool0_eq_id;
	eq_profile.bf.eqp1 = l3qm_eq_profile_dq_pool1_eq_id;
	eq_profile.bf.rule = l3qm_eq_profile_dq_rule;
	reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_dq_id);
	CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: DQ pool0 eq_id=%d, rule=%d\n", __func__, l3qm_eq_profile_dq_pool0_eq_id, l3qm_eq_profile_dq_rule);
	}

	dest_port_eq_cfg.wrd = 0;

	/* for CPU ports profile select, profile 0 for DQ at NI ports  */
	for (i = 0; i < CA_NI_TOTAL_NI_PORT; i++) {
		dest_port_eq_cfg.bf.profile_sel = l3qm_eq_profile_dq_id;
		reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * (i + CA_NI_TOTAL_CPU_PORT));
		CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);
	}

	if(l3qm_eq0_sram_eq1_dram == 2) {
		/* one profile for WAN NI ports */
		eq_profile.wrd = 0;
		eq_profile.bf.eqp0 = 12;//eqid 12
		eq_profile.bf.eqp1 = l3qm_eq_profile_dq_pool1_eq_id;
		eq_profile.bf.rule = l3qm_eq_profile_dq_rule;
		reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * 6); //profile6
		CA_NE_REG_WRITE(eq_profile.wrd, reg_off);
		/* for NI ports profile select, profile 6 for DQ at wan NI port  */
		dest_port_eq_cfg.bf.profile_sel = 6 ;
		reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * (CA_NI_TOTAL_NI_PORT + CA_NI_TOTAL_CPU_PORT-1));
		CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);
	}

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: DQ profile id=%d\n", __func__, l3qm_eq_profile_dq_id);
	}

	/* configure EB pool 0 for DQ */
	/* set phy_addr_start and enable Empty Buffer */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
	if (l3qm_deepq_test_with_sram)
		cfg0_eq.bf.phy_addr_start = (0xc0028000) >> CA_L3QM_PHY_ADDR_SHIFT;
	else
		cfg0_eq.bf.phy_addr_start = (l3qm_eq_profile_dq_pool0_buff_phy_addr) >> CA_L3QM_PHY_ADDR_SHIFT;
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
	CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: DQ pool0 buff_phy_addr=0x%x\n", __func__, l3qm_eq_profile_dq_pool0_buff_phy_addr);
	}

	/* configure total buffer number and bid_start */
	cfg1_eq.wrd = 0;
	cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_dq_pool0_buf_count;
	cfg1_eq.bf.bid_start = l3qm_eq_profile_dq_pool0_bid_start;
	reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
	CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: DQ pool0 buf_count=%d, bid_start=%d\n", __func__, l3qm_eq_profile_dq_pool0_buf_count, l3qm_eq_profile_dq_pool0_bid_start);
	}

	/* configure buffer size for Empty Buffer */
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
	cfg2_eq.bf.cpu_eq = 0;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_dq_pool0_buf_sz);      /* 2048 bytes */
	cfg2_eq.bf.refill_en = 0;
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: DQ pool0 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_dq_pool0_buf_sz, cfg2_eq.bf.buffer_size);
	}

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_dq_pool0_eq_id];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        /* because PE0 pool1 locates at DDR start from 0x180000000 */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
	if (l3qm_deepq_test_with_sram)
		cfg4_eq.wrd = 0;
	else
		cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_dq_pool0_eq_id];
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

        /* configure EB pool 1 for DQ */
        /* set phy_addr_start and enable Empty Buffer */
        cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
	if (l3qm_deepq_test_with_sram)
		cfg0_eq.bf.phy_addr_start = (0xc0030000) >> CA_L3QM_PHY_ADDR_SHIFT;
	else
		cfg0_eq.bf.phy_addr_start = (l3qm_eq_profile_dq_pool1_buff_phy_addr) >> CA_L3QM_PHY_ADDR_SHIFT;
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: DQ pool1 buff_phy_addr=0x%x\n", __func__, l3qm_eq_profile_dq_pool1_buff_phy_addr);
        }

        /* configure total buffer number and bid_start */
        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_dq_pool1_buf_count;
        cfg1_eq.bf.bid_start = l3qm_eq_profile_dq_pool1_bid_start;
        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
        CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: DQ pool1 buf_count=%d, bid_start=%d\n", __func__,
                        l3qm_eq_profile_dq_pool1_buf_count, l3qm_eq_profile_dq_pool1_bid_start);
        }

        /* configure buffer size for Empty Buffer */
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
        cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
        cfg2_eq.bf.cpu_eq = 0;
        cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_dq_pool1_buf_sz);      /* 2048 bytes */
        CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: DQ pool1 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_dq_pool1_buf_sz, cfg2_eq.bf.buffer_size);
        }

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_dq_pool1_eq_id];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	/* because PE0 pool1 locates at DDR start from 0x180000000 */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
	if (l3qm_deepq_test_with_sram)
		cfg4_eq.wrd = 0;
	else
		cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_dq_pool1_eq_id];
	CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

	if(l3qm_eq0_sram_eq1_dram == 2) {
		/* configure EB pool 2 for DQ */
		/* set phy_addr_start and enable Empty Buffer */
		pool_id = l3qm_eq_profile_dq_wan_pool_id;
		buf_cnt = l3qm_eq_profile_dq_wan_pool_buf_count;
		buf_size = l3qm_eq_profile_dq_wan_pool_buf_sz;
		cfg0_eq.wrd = 0;
		cfg0_eq.bf.eq_en = 1;
		cfg0_eq.bf.phy_addr_start = (l3qm_eq_profile_dq_wan_pool_start_physical_addr) >> CA_L3QM_PHY_ADDR_SHIFT;
		reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * pool_id);
		CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: DQ pool2 buff_phy_addr=0x%x\n", __func__, l3qm_eq_profile_dq_pool0_buff_phy_addr);
		}

		/* configure total buffer number and bid_start */
		cfg1_eq.wrd = 0;
		cfg1_eq.bf.total_buffer_num = buf_cnt;
		cfg1_eq.bf.bid_start = l3qm_eq_profile_dq_pool1_bid_start + l3qm_eq_profile_dq_pool1_buf_count;
		if ((cfg1_eq.bf.bid_start + cfg1_eq.bf.total_buffer_num) > 16383)
			printk("%s: EQ Pool bid cnt is oversized\n", __func__);

		//BUG_ON((cfg1_eq.bf.bid_start + buf_cnt) > 16383);

		reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * pool_id);
		CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: DQ pool2 buf_count=%d, bid_start=%d\n", __func__, l3qm_eq_profile_dq_pool0_buf_count, l3qm_eq_profile_dq_pool0_bid_start);
		}

		/* configure buffer size for Empty Buffer */
		reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * pool_id);
		cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
		cfg2_eq.bf.cpu_eq = 0;
		cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(buf_size);/* 512 bytes */
		cfg2_eq.bf.refill_en = 0;
		CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

		if (ca_aal_debug & AAL_DBG_L3QM) {
			printk("%s: DQ pool2 buf_sz=%d, size_index=%d\n", __func__, buf_size, cfg2_eq.bf.buffer_size);
		}

		/* configure cache_eos/domain/snoop/cache */
		reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * pool_id);
		cfg3_eq.wrd = l3qm_eq_cfg3_values[pool_id];
		CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

		/* configure AXI address top 8 bits */
		/* because PE0 pool1 locates at DDR start from 0x180000000 */
		reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * pool_id);
		cfg4_eq.wrd = l3qm_eq_cfg4_values[pool_id];
		CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);
	}
	//aal_saturn_l3qm_init_empty_buffer_DQ();

}

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
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
	cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
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
        cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
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
        cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
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
        cfg2_eq.bf.refill_ths = L3QM_EQ_REFILL_THRESHOLD;
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
#endif
#endif

/* Sending packets to CPU port through L3QM - Receiving */
/* Configure Empty Buffer Pool for CPU and NI and set up the physical start address for NI ports */
static void aal_l3qm_init_empty_buffer_DQ_1(void);
void aal_l3qm_init_empty_buffer(ca_uint8_t is_use_fbm)
{
	aal_l3qm_init_empty_buffer_CPU();
	if(l3qm_eq0_sram_eq1_dram == 1)
		aal_l3qm_init_empty_buffer_DQ_1();
	else
	aal_l3qm_init_empty_buffer_DQ();

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if  0
	aal_l3qm_init_empty_buffer_CPU256_hi();
	aal_l3qm_init_empty_buffer_CPU256_lo();
#else
	/* must be called before aal_l3qm_load_eq_config() */
#if defined(CONFIG_NE_CPU256)
	if(0 == l3qm_eq0_sram_eq1_dram) {
		if (support_cpu256_in_linux)
	aal_l3qm_init_cpu256_empty_buffer();
	}
#endif
#endif
#endif
	aal_l3qm_load_eq_config();

	aal_l3qm_init_qm_axi_attrib();

	aal_l3qm_check_bid_start(0);

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#if 0
	spin_lock_init(&l3qm_epp256_inte_lock);
#endif

	spin_lock_init(&l3qm_epp64_inte_lock);
}

void aal_saturn_l3qm_enable_rx(ca_uint8_t enable);
void aal_l3qm_enable_rx(ca_uint8_t enable)
{
	QM_QM_RMU0_CTRL_t rmu0_ctrl;

	rmu0_ctrl.wrd = CA_NE_REG_READ(QM_QM_RMU0_CTRL);
	if (enable) {
		rmu0_ctrl.bf.rx_en = 1;
	}
	else {
		rmu0_ctrl.bf.rx_en = 0;
	}
	CA_NE_REG_WRITE(rmu0_ctrl.wrd, QM_QM_RMU0_CTRL);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: enable=%d\n", __func__, enable);
	}

	//aal_saturn_l3qm_enable_rx(enable);
}

static void aal_saturn_l3qm_enable_tx(ca_uint8_t enable);
void aal_l3qm_enable_tx(ca_uint8_t enable)
{
	QM_QM_ES_CTRL_t es_ctrl;

	es_ctrl.wrd = CA_NE_REG_READ(QM_QM_ES_CTRL);
	if (enable) {
		es_ctrl.bf.tx_en = 1;
		//es_ctrl.bf.cpu_en = 0xff;
		es_ctrl.bf.cpu_en = 0;
		es_ctrl.bf.ni_en = 0xff;
	}
	else {
		es_ctrl.bf.tx_en = 0;
		es_ctrl.bf.cpu_en = 0;
		es_ctrl.bf.ni_en = 0;
	}
	CA_NE_REG_WRITE(es_ctrl.wrd, QM_QM_ES_CTRL);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: enable=%d\n", __func__, enable);
        }

	///TOOD: debug
	//aal_saturn_l3qm_enable_tx(enable);
}

void aal_l3qm_enable_tx_cpu(ca_uint16_t cpu_port, ca_uint8_t enable)
{
        QM_QM_ES_CTRL_t es_ctrl;

        es_ctrl.wrd = CA_NE_REG_READ(QM_QM_ES_CTRL);
        if (enable) {
                es_ctrl.bf.cpu_en |= (1 << cpu_port);
        }
        else {
                es_ctrl.bf.cpu_en &= ~(1 << cpu_port);
        }
        CA_NE_REG_WRITE(es_ctrl.wrd, QM_QM_ES_CTRL);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: cpu_port=%d, enable=%d, es_ctrl.bf.cpu_en=0x%x\n", __func__, cpu_port, enable, es_ctrl.bf.cpu_en);
        }
}

void aal_l3qm_config_DWRR(void)
{
	int i;
	QM_QM_DRR_WEIGHT_BASE_t drr_weight_base;
	QM_QM_VOQ3_0_WEIGHT_RATIO_CFG0_t voq3_0_weight_ratio;
	QM_QM_VOQ7_4_WEIGHT_RATIO_CFG0_t voq7_4_weight_ratio;

	/* register QM_QM_PORT3_0_WEIGHT_RATIO_CFG_t has been removed */
	QM_QM_PORT7_0_WEIGHT_RATIO_CFG_t   port7_0_weight_ratio;
	QM_QM_PORT15_8_WEIGHT_RATIO_CFG_t   port15_8_weight_ratio;
	//QM_QM_PORT11_8_WEIGHT_RATIO_CFG_t  port11_8_weight_ratio;
	//QM_QM_PORT15_12_WEIGHT_RATIO_CFG_t port15_12_weight_ratio;
	//QM_QM_PORT19_16_WEIGHT_RATIO_CFG_t port19_16_weight_ratio;
	//QM_QM_PORT23_20_WEIGHT_RATIO_CFG_t port23_20_weight_ratio;
	//QM_QM_PORT27_24_WEIGHT_RATIO_CFG_t port27_24_weight_ratio;

	/* configure DRR Scheduler Weight Base */
	drr_weight_base.wrd = CA_NE_REG_READ(QM_QM_DRR_WEIGHT_BASE);

	/* scheduler (VOQ and port) use "packet" as unit for all CPU ports */
	//drr_weight_base.bf.cpu_dwrr_unit = 1;

	CA_NE_REG_WRITE(drr_weight_base.wrd, QM_QM_DRR_WEIGHT_BASE);

	/* Configure Weight Ratio for VOQ */
	for (i = 0; i < QM_QM_VOQ3_0_WEIGHT_RATIO_CFG0_COUNT; i++) {
		voq3_0_weight_ratio.bf.voq0 = 1;
		voq3_0_weight_ratio.bf.voq1 = 1;
		voq3_0_weight_ratio.bf.voq2 = 1;
		voq3_0_weight_ratio.bf.voq3 = 1;
		CA_NE_REG_WRITE(voq3_0_weight_ratio.wrd, QM_QM_VOQ3_0_WEIGHT_RATIO_CFG0 + (QM_QM_VOQ3_0_WEIGHT_RATIO_CFG0_STRIDE * i));
	}
	for (i = 0; i < QM_QM_VOQ7_4_WEIGHT_RATIO_CFG0_COUNT; i++) {
		voq7_4_weight_ratio.bf.voq4 = 1;
		voq7_4_weight_ratio.bf.voq5 = 1;
		voq7_4_weight_ratio.bf.voq6 = 1;
		voq7_4_weight_ratio.bf.voq7 = 1;
		CA_NE_REG_WRITE(voq3_0_weight_ratio.wrd, QM_QM_VOQ7_4_WEIGHT_RATIO_CFG0 + (QM_QM_VOQ7_4_WEIGHT_RATIO_CFG0_STRIDE * i));
	}

	/* Configure Weight Ratio for CPU ports */
	port7_0_weight_ratio.bf.port0 = 1;
	port7_0_weight_ratio.bf.port1 = 1;
	port7_0_weight_ratio.bf.port2 = 1;
	port7_0_weight_ratio.bf.port3 = 1;
	port7_0_weight_ratio.bf.port4 = 1;
	port7_0_weight_ratio.bf.port5 = 1;
	port7_0_weight_ratio.bf.port6 = 1;
	port7_0_weight_ratio.bf.port7 = 1;
	CA_NE_REG_WRITE(port7_0_weight_ratio.wrd, QM_QM_PORT7_0_WEIGHT_RATIO_CFG);

	/* Configure Weight Ratio for NI ports */
	/* the DRR_WEIGHT_BASE*weight_ratio=128*6=768= should equal to (max packet size/2)=1514/2=757 */
	port15_8_weight_ratio.bf.port8  = 1;
	port15_8_weight_ratio.bf.port9  = 1;
	port15_8_weight_ratio.bf.port10 = 1;
	port15_8_weight_ratio.bf.port11 = 1;
	port15_8_weight_ratio.bf.port12 = 1;
	port15_8_weight_ratio.bf.port13 = 1;
	port15_8_weight_ratio.bf.port14 = 1;
	port15_8_weight_ratio.bf.port15 = 1;
	CA_NE_REG_WRITE(port15_8_weight_ratio.wrd, QM_QM_PORT15_8_WEIGHT_RATIO_CFG);

#if 0
	/* Configure Weight Ratio for NI ports */
	/* the DRR_WEIGHT_BASE*weight_ratio=128*6=768= should equal to (max packet size/2)=1514/2=757 */
	port11_8_weight_ratio.bf.port8 = 6;
	port11_8_weight_ratio.bf.port9 = 6;
	port11_8_weight_ratio.bf.port10 = 6;
	port11_8_weight_ratio.bf.port11 = 6;
	CA_NE_REG_WRITE(port11_8_weight_ratio.wrd, QM_QM_PORT11_8_WEIGHT_RATIO_CFG);

	/* Configure Weight Ratio for NI ports */
	/* the DRR_WEIGHT_BASE*weight_ratio=128*6=768= should equal to (max packet size/2)=1514/2=757 */
	port15_12_weight_ratio.bf.port12 = 6;
	port15_12_weight_ratio.bf.port13 = 6;
	port15_12_weight_ratio.bf.port14 = 6;
	port15_12_weight_ratio.bf.port15 = 6;
	CA_NE_REG_WRITE(port15_12_weight_ratio.wrd, QM_QM_PORT15_12_WEIGHT_RATIO_CFG);

	/* Configure Weight Ratio for NI ports */
	/* the DRR_WEIGHT_BASE*weight_ratio=128*6=768= should equal to (max packet size/2)=1514/2=757 */
	port19_16_weight_ratio.bf.port16 = 6;
	port19_16_weight_ratio.bf.port17 = 6;
	port19_16_weight_ratio.bf.port18 = 6;
	port19_16_weight_ratio.bf.port19 = 6;
	CA_NE_REG_WRITE(port19_16_weight_ratio.wrd, QM_QM_PORT19_16_WEIGHT_RATIO_CFG);

	/* Configure Weight Ratio for NI ports */
	/* the DRR_WEIGHT_BASE*weight_ratio=128*6=768= should equal to (max packet size/2)=1514/2=757 */
	port23_20_weight_ratio.bf.port20 = 6;
	port23_20_weight_ratio.bf.port21 = 6;
	port23_20_weight_ratio.bf.port22 = 6;
	port23_20_weight_ratio.bf.port23 = 6;
	CA_NE_REG_WRITE(port23_20_weight_ratio.wrd, QM_QM_PORT23_20_WEIGHT_RATIO_CFG);

	/* Configure Weight Ratio for NI ports */
        /* the DRR_WEIGHT_BASE*weight_ratio=128*6=768= should equal to (max packet size/2)=1514/2=757 */
        port27_24_weight_ratio.bf.port24 = 6;
        port27_24_weight_ratio.bf.port25 = 6;
        port27_24_weight_ratio.bf.port26 = 6;
        port27_24_weight_ratio.bf.port27 = 6;
        CA_NE_REG_WRITE(port27_24_weight_ratio.wrd, QM_QM_PORT27_24_WEIGHT_RATIO_CFG);
#endif
}

void aal_l3qm_enable_int_src(void)
{
	QM_QM_INT_SRCE_t	int_srcE;

	int_srcE.wrd = 0;
	int_srcE.bf.header_ecc_error_int_srcE 		= 1;
	int_srcE.bf.voqinfo_ecc_error_int_srcE 		= 1;
	//int_srcE.bf.mmu_ecc_error_int_srcE 		= 1;
	int_srcE.bf.mmu_lkup_bid_out_of_range_int_srcE 	= 1;
	int_srcE.bf.rmu0_no_buffer_drop_int_srcE	= 1;
	int_srcE.bf.rmu0_check_error_int_srcE		= 1;
	int_srcE.bf.rmu0_fifo_error_int_srcE		= 1;
	int_srcE.bf.ni_bridge_fifo_error_int_srcE	= 1;
	int_srcE.bf.dqm_fifo_error_int_srcE		= 1;
	int_srcE.bf.eqm_fifo_error_int_srcE		= 1;
	//int_srcE.bf.eqm_stack_error_int_srcE		= 1;
	int_srcE.bf.cpuepp_fifo_error_int_srcE		= 1;
	int_srcE.bf.tmu_fifo_error_int_srcE		= 1;
	int_srcE.bf.eqm_cfg_error_int_srcE		= 1;
	int_srcE.bf.axiarb_fifo_error_int_srcE		= 1;
	//int_srcE.bf.len_chk_fifo_error_int_srcE		= 1;
	int_srcE.bf.pkt_lenght_error_int_srcE		= 1;
	int_srcE.bf.fbm_returns_no_buffer_int_srcE	= 0;
	int_srcE.bf.eq_overflow_int_srcE		= 1;
	int_srcE.bf.axim_rmu0_resp_error_int_srcE	= 1;
	int_srcE.bf.axim_cpuepp_resp_error_int_srcE	= 1;
	int_srcE.bf.axim_qm_int_srcE			= 1;
	CA_NE_REG_WRITE(int_srcE.wrd, QM_QM_INT_SRCE);
}

ca_uint32_t aal_l3qm_read_rx_interrupt_status(void)
{
#if 0
	QM_QM_INT_SRC_t		int_src;

	int_src.wrd = CA_NE_REG_READ(QM_QM_INT_SRC);
	printk("%s: int_src.wrd=0x%x\n", __func__, int_src.wrd);
	return int_src.wrd;
#endif
	QM_QM_VOQ_STATUS0_t	voq_status0;
	QM_QM_VOQ_STATUS1_t	voq_status1;
	QM_QM_VOQ_STATUS2_t	voq_status2;
	QM_QM_VOQ_STATUS3_t	voq_status3;

	voq_status0.wrd = CA_NE_REG_READ(QM_QM_VOQ_STATUS0);
	voq_status1.wrd = CA_NE_REG_READ(QM_QM_VOQ_STATUS1);
	voq_status2.wrd = CA_NE_REG_READ(QM_QM_VOQ_STATUS2);
	voq_status3.wrd = CA_NE_REG_READ(QM_QM_VOQ_STATUS3);

	printk("%s: voq_status0.wrd=0x%x, voq_status1.wrd=0x%x, voq_status2.wrd=0x%x, voq_status3.wrd=0x%x\n", __func__,
		voq_status0.wrd, voq_status1.wrd, voq_status2.wrd, voq_status3.wrd);

	return 0;
}

void aal_l3qm_write_rx_interrupt_status(ca_uint32_t status)
{
	QM_QM_INT_SRC_t         int_src;

	printk("%s: status = 0x%x\n", __func__, status);
	int_src.wrd = status;
	CA_NE_REG_WRITE(int_src.wrd, QM_QM_INT_SRC);
}

void aal_l3qm_enable_refill_threshold_interrupt(ca_uint16_t eqid, ca_uint8_t enable)
{
        QM_QM_EQ_REFILL_INTERRUPT_EN_t refill_int_en;

        refill_int_en.wrd = CA_NE_REG_READ(QM_QM_EQ_REFILL_INTERRUPT_EN);
        if (enable)
                refill_int_en.bf.eq_refill_en |= (1 << eqid);
        else
                refill_int_en.bf.eq_refill_en &= ~(1 << eqid);

        //printk("%s: refill_int_en.wrd=0x%x\n", __func__, refill_int_en.wrd);
        CA_NE_REG_WRITE(refill_int_en.wrd, QM_QM_EQ_REFILL_INTERRUPT_EN);
}

void aal_l3qm_setup_refill_threshold_interrup(void)
{
	CA_NE_REG_WRITE(0, QM_QM_EQ_REFILL_INTERRUPT_EN);
	aal_l3qm_enable_refill_threshold_interrupt(l3qm_eq_profile_cpu_pool0_eq_id, 1);
	aal_l3qm_enable_refill_threshold_interrupt(l3qm_eq_profile_cpu_pool1_eq_id, 1);
}

void aal_l3qm_enable_cpu_epp_interrupt(ca_uint16_t cpu_port, ca_uint8_t enable)
{
	QM_QM_EPP64_P3_0_INTERRUPT_EN_t	p3_0_int_en;
	QM_QM_EPP64_P7_4_INTERRUPT_EN_t p7_4_int_en;
	unsigned long flags;

	/* these registers are shared by all of CPUs */
	spin_lock_irqsave(&l3qm_epp64_inte_lock, flags);

	if (cpu_port >= 0 && cpu_port <= 3) {
		p3_0_int_en.wrd = CA_NE_REG_READ(QM_QM_EPP64_P3_0_INTERRUPT_EN);
		switch (cpu_port) {
			case 0:
				if (enable)
					p3_0_int_en.bf.p0_int_en = 0xff;
				else
					p3_0_int_en.bf.p0_int_en = 0;
				break;
			case 1:
                                if (enable)
                                        p3_0_int_en.bf.p1_int_en = 0xff;
                                else
                                        p3_0_int_en.bf.p1_int_en = 0;
                                break;
			case 2:
                                if (enable)
                                        p3_0_int_en.bf.p2_int_en = 0xff;
                                else
                                        p3_0_int_en.bf.p2_int_en = 0;
                                break;
			case 3:
                                if (enable)
                                        p3_0_int_en.bf.p3_int_en = 0xff;
                                else
                                        p3_0_int_en.bf.p3_int_en = 0;
                                break;
		}
		CA_NE_REG_WRITE(p3_0_int_en.wrd, QM_QM_EPP64_P3_0_INTERRUPT_EN);
	}
	else {
		p7_4_int_en.wrd = CA_NE_REG_READ(QM_QM_EPP64_P7_4_INTERRUPT_EN);
		switch (cpu_port) {
			case 4:
				if (enable)
					p7_4_int_en.bf.p4_int_en = 0xff;
				else
					p7_4_int_en.bf.p4_int_en = 0;
				break;
			case 5:
				if (enable)
					p7_4_int_en.bf.p5_int_en = 0xff;
				else
					p7_4_int_en.bf.p5_int_en = 0;
				break;
			case 6:
				if (enable)
					p7_4_int_en.bf.p6_int_en = 0xff;
				else
					p7_4_int_en.bf.p6_int_en = 0;
				break;
			case 7:
				if (enable)
					p7_4_int_en.bf.p7_int_en = 0xff;
				else
					p7_4_int_en.bf.p7_int_en = 0;
				break;
		}
		CA_NE_REG_WRITE(p7_4_int_en.wrd, QM_QM_EPP64_P7_4_INTERRUPT_EN);
	}
	spin_unlock_irqrestore(&l3qm_epp64_inte_lock, flags);
}

int aal_l3qm_insert_magic_number(void *addr)
{
	int i;
        ca_uint32_t *magic_number = (ca_uint32_t *)addr;

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: addr=%p, l3qm_magic_marker_len=%d\n", __func__, addr, l3qm_magic_marker_len);
	}

        for (i = 0; i < l3qm_magic_marker_len/4; i++) {
                *magic_number++ = CA_L3QM_MAGIC_NUMBER;
        }
	return l3qm_magic_marker_len;
}

ca_status_t aal_l3qm_check_magic_number(void *addr)
{
        int i;
        ca_uint32_t *magic_number = (ca_uint32_t *)addr;

        for (i = 0; i < l3qm_magic_marker_len/4; i++) {
                if (*magic_number != CA_L3QM_MAGIC_NUMBER) {
			printk("%s: addr=%p, magic_number=0x%x\n", __func__, magic_number, *magic_number);
			return CA_E_ERROR;
		}
		magic_number++;
        }
        return CA_E_OK;
}

static void aal_l3qm_dump_sram_map(void)
{
	printk("%s: Dump SRAM map...\n", __func__);

	if (l3qm_main_hash_bm_test) {
		printk("[ Main hash ]\n");
		printk("overflow hash fib start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_overflow_hash_fib_start_addr);
		printk("overflow hash fib physical addr = 0x%x\n", l3fe_main_hash_overflow_hash_fib_phy_addr);
		printk("overflow hash fib length = %d\n", l3fe_main_hash_overflow_hash_fib_length);
		printk("overflow hash fib count = %d\n", l3fe_main_hash_overflow_hash_fib_count);

		printk("default hash fib start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_default_hash_fib_start_addr);
		printk("default hash fib physical addr = 0x%x\n", l3fe_main_hash_default_hash_fib_phy_addr);
		printk("default hash fib length = %d\n", l3fe_main_hash_default_hash_fib_length);
		printk("default hash fib count = %d\n", l3fe_main_hash_default_hash_fib_count);

		printk("hash action cache fib start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_action_cache_fib_start_addr);
		printk("hash action cache fib physical addr = 0x%x\n", l3fe_main_hash_action_cache_fib_phy_addr);
		printk("hash action cache fib length = %d\n", l3fe_main_hash_action_cache_fib_length);
		printk("hash action cache fib count = %d\n", l3fe_main_hash_action_cache_fib_count);
	}
}

static ca_status_t aal_l3qm_create_sram_map(void *sram_base, ca_uint32_t sram_size, void *sram_phy_base)
{
	ca_uint32_t size = 0;
	ca_uint8_t *sram_start_addr = sram_base;
	ca_uint8_t *sram_phy_addr = sram_phy_base;

	if (l3qm_main_hash_bm_test) {
		/* Main hash overflow action */
		l3fe_main_hash_overflow_hash_fib_start_addr = (ca_uint_t)sram_start_addr;
		l3fe_main_hash_overflow_hash_fib_phy_addr   = (ca_uint_t)sram_phy_base;
		size += l3fe_main_hash_overflow_hash_fib_length * l3fe_main_hash_overflow_hash_fib_count;

		/* Magic number */
		size += aal_l3qm_insert_magic_number(sram_start_addr + size);

		/* Main hash default action */
		l3fe_main_hash_default_hash_fib_start_addr = (ca_uint_t)sram_start_addr + size;
		l3fe_main_hash_default_hash_fib_phy_addr = (ca_uint_t)sram_phy_addr + size;
		size += l3fe_main_hash_default_hash_fib_length * l3fe_main_hash_default_hash_fib_count;

		/* Magic number */
		size += aal_l3qm_insert_magic_number(sram_start_addr + size);

		/* Main hash action */
		l3fe_main_hash_action_cache_fib_start_addr = (ca_uint_t)sram_start_addr + size;
		l3fe_main_hash_action_cache_fib_phy_addr = (ca_uint_t)sram_phy_addr + size;
		size += l3fe_main_hash_action_cache_fib_length * l3fe_main_hash_action_cache_fib_count;

		/* Magic number */
		size += aal_l3qm_insert_magic_number(sram_start_addr + size);
		if (l3qm_eq0_sram_eq1_dram == 2) {
			l3qm_eq_profile_dq_wan_pool_start_physical_addr = (ca_uint_t)sram_phy_addr + size;
			size += l3qm_eq_profile_dq_wan_pool_buf_sz * l3qm_eq_profile_dq_wan_pool_buf_count;
		}
	}

	printk("%s: total required SRAM size 0x%x\n", __func__, size);

	if (size > sram_size) {
		printk(KERN_ERR "%s: out of SRAM space! (required=0x%x, total=0x%x)\n", __func__, size, sram_size);
		printk(KERN_ERR "%s: NE driver initialized failed!!!\n", __func__);
	}
	BUG_ON(size > sram_size);

	aal_l3qm_dump_sram_map();

	return CA_E_OK;
}

static void aal_l3qm_dump_ddr_map(void)
{
	printk("%s: Dump DDR map...\n", __func__);
	printk("[ DDR coherent ]\n");

	/* Patched from Aaron/Cortina 1116 */
	/* for memory shrink */
#if !defined(CONFIG_LUNA_G3_SERIES)
	printk("CPU pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool0_start_addr);
	printk("CPU pool0 buffer physical addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool0_phy_addr);
	printk("CPU pool0 buffer count = %d\n", l3qm_eq_profile_cpu_pool0_buf_count);
	printk("CPU pool0 buffer size = %d\n", l3qm_eq_profile_cpu_pool0_buf_sz);
	printk("CPU pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool1_start_addr - l3qm_eq_profile_cpu_pool0_start_addr);

        printk("CPU pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool1_start_addr);
        printk("CPU pool1 buffer physical addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool1_phy_addr);
        printk("CPU pool1 buffer count = %d\n", l3qm_eq_profile_cpu_pool1_buf_count);
        printk("CPU pool1 buffer size = %d\n", l3qm_eq_profile_cpu_pool1_buf_sz);
	printk("CPU pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_epp_profile_cpu_rx_fifo_start_addr - l3qm_eq_profile_cpu_pool1_start_addr);
#endif

        printk("CPU RX FIFO buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu_rx_fifo_start_addr);
        printk("CPU RX FIFO buffer physical addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu_rx_fifo_phy_addr);
        printk("CPU RX FIFO count = %d\n", l3qm_epp_profile_cpu_cn);
        printk("CPU RX FIFO size = %d\n", l3qm_epp_profile_cpu_sz);
	printk("CPU RX FIFO consumed size = %d\n", l3qm_epp_profile_cpu_cn * l3qm_epp_profile_cpu_sz);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	printk("[ DDR non-cache ]\n");
#endif
        printk("DEEPQ pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_dq_pool0_buff_start_addr);
	printk("DEEPQ pool0 buffer physical addr = 0x%x\n", l3qm_eq_profile_dq_pool0_buff_phy_addr);
        printk("DEEPQ pool0 buffer count = %d\n", l3qm_eq_profile_dq_pool0_buf_count);
        printk("DEEPQ pool0 buffer size = %d\n", l3qm_eq_profile_dq_pool0_buf_sz);
	printk("DEEPQ pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_dq_pool1_buff_start_addr - l3qm_eq_profile_dq_pool0_buff_start_addr);

        printk("DEEPQ pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_dq_pool1_buff_start_addr);
	printk("DEEPQ pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_dq_pool1_buff_phy_addr);
        printk("DEEPQ pool1 buffer count = %d\n", l3qm_eq_profile_dq_pool1_buf_count);
        printk("DEEPQ pool1 buffer size = %d\n", l3qm_eq_profile_dq_pool1_buf_sz);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	printk("DEEPQ pool1 consumed size = %lld\n", (ca_uint64_t)l3fe_main_hash_table_start_addr - l3qm_eq_profile_dq_pool1_buff_start_addr);
#else
	printk("DEEPQ pool1 consumed size = %d\n", l3qm_eq_profile_dq_pool1_buf_count * l3qm_eq_profile_dq_pool1_buf_sz);
#endif

#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
        printk("Main Hash table entry start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_table_start_addr);
	printk("Main Hash table entry physical addr = 0x%x\n", l3fe_main_hash_table_phy_addr);
        printk("Main Hash tabel entry length = %d\n", l3fe_main_hash_table_entry_length);
        printk("Main Hash table entry count = %d\n", l3fe_main_hash_table_entry_count);
	printk("Main Hash table consumed size = %lld\n", (ca_uint64_t)l3fe_main_action_table_start_addr - l3fe_main_hash_table_start_addr);

        printk("Main FIB action table entry addr = 0x%llx\n", (ca_uint64_t)l3fe_main_action_table_start_addr);
	printk("Main FIB action table entry physical addr = 0x%x\n", l3fe_main_action_table_phy_addr);
        printk("Main FIB action table entry length = %d\n", l3fe_main_hash_action_table_entry_length);
        printk("Main FIB action table entry count = %d\n", l3fe_main_hash_action_table_entry_count);
	//printk("Main FIB action table consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_pe0_pool1_buff_start_addr - l3fe_main_action_table_start_addr);
	printk("Main FIB action table consumed size = %lld\n", (ca_uint64_t)l3fe_main_hash_action_table_entry_length * l3fe_main_hash_action_table_entry_count);

        if (l3qm_main_hash_bm_test == 0) {
                //printk("[ Main hash ]\n");
                printk("Main hash overflow hash fib start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_overflow_hash_fib_start_addr);
                printk("Main hash overflow hash fib physical addr = 0x%x\n", l3fe_main_hash_overflow_hash_fib_phy_addr);
                printk("Main hash overflow hash fib length = %d\n", l3fe_main_hash_overflow_hash_fib_length);
                printk("Main hash overflow hash fib count = %d\n", l3fe_main_hash_overflow_hash_fib_count);

                printk("Main hash default hash fib start addr = 0x%llx\n", (ca_uint64_t)(ca_uint64_t)l3fe_main_hash_default_hash_fib_start_addr);
                printk("Main hash default hash fib physical addr = 0x%x\n", l3fe_main_hash_default_hash_fib_phy_addr);
                printk("Main hash default hash fib length = %d\n", l3fe_main_hash_default_hash_fib_length);
                printk("Main hash default hash fib count = %d\n", l3fe_main_hash_default_hash_fib_count);

                printk("Main hash hash action cache fib start addr = 0x%llx\n", (ca_uint64_t)(ca_uint64_t)l3fe_main_hash_action_cache_fib_start_addr);
                printk("Main hash hash action cache fib physical addr = 0x%x\n", l3fe_main_hash_action_cache_fib_phy_addr);
                printk("Main hash hash action cache fib length = %d\n", l3fe_main_hash_action_cache_fib_length);
                printk("Main hash hash action cache fib count = %d\n", l3fe_main_hash_action_cache_fib_count);
        }
#endif

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
        printk("CPU256 hi pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool0_start_addr);
	printk("CPU256 hi pool0 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_hi_pool0_phy_addr);
        printk("CPU256 hi pool0 buffer count = %d\n", l3qm_eq_profile_cpu256_hi_pool0_buf_count);
        printk("CPU256 hi pool0 buffer size = %d\n", l3qm_eq_profile_cpu256_hi_pool0_buf_sz);
	printk("CPU256 hi pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool1_start_addr - l3qm_eq_profile_cpu256_hi_pool0_start_addr);

        printk("CPU256 hi pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_hi_pool1_start_addr);
	printk("CPU256 hi pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_hi_pool1_phy_addr);
        printk("CPU256 hi pool1 buffer count = %d\n", l3qm_eq_profile_cpu256_hi_pool1_buf_count);
        printk("CPU256 hi pool1 buffer size = %d\n", l3qm_eq_profile_cpu256_hi_pool1_buf_sz);
	printk("CPU256 hi pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr - l3qm_eq_profile_cpu256_hi_pool1_start_addr);

        printk("CPU256 hi RX FIFO start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr);
	printk("CPU256 hi RX FIFO physical addr = 0x%x\n", l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr);
        printk("CPU256 hi RX FIFO size = %d\n", l3qm_epp_profile_cpu256_hi_sz);
        printk("CPU256 hi RX FIFO count = %d\n", l3qm_epp_profile_cpu256_hi_cn);
	printk("CPU256 hi RX FIFO consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool0_start_addr - l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr);

        printk("CPU256 lo pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool0_start_addr);
	printk("CPU256 lo pool0 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_lo_pool0_phy_addr);
        printk("CPU256 lo pool0 buffer count = %d\n", l3qm_eq_profile_cpu256_lo_pool0_buf_count);
        printk("CPU256 lo pool0 buffer size = %d\n", l3qm_eq_profile_cpu256_lo_pool0_buf_sz);
	printk("CPU256 lo pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool1_start_addr - l3qm_eq_profile_cpu256_lo_pool0_start_addr);

        printk("CPU256 lo pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu256_lo_pool1_start_addr);
	printk("CPU256 lo pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_cpu256_lo_pool1_phy_addr);
        printk("CPU256 lo pool1 buffer count = %d\n", l3qm_eq_profile_cpu256_lo_pool1_buf_count);
        printk("CPU256 lo pool1 buffer size = %d\n", l3qm_eq_profile_cpu256_lo_pool1_buf_sz);
	printk("CPU256 lo pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr - l3qm_eq_profile_cpu256_lo_pool1_start_addr);

        printk("CPU256 lo RX FIFO start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr);
	printk("CPU256 lo RX FIFO physical addr = 0x%x\n", l3qm_epp_profile_cpu256_lo_rx_fifo_phy_addr);
        printk("CPU256 lo RX FIFO size = %d\n", l3qm_epp_profile_cpu256_lo_sz);
        printk("CPU256 lo RX FIFO count = %d\n", l3qm_epp_profile_cpu256_lo_cn);
	printk("CPU256 lo RX FIFO consumed size = %d\n", l3qm_epp_profile_cpu256_lo_sz * l3qm_epp_profile_cpu256_lo_cn);
#else
#if defined(CONFIG_NE_CPU256)
	if (support_cpu256_in_linux)
	aal_l3qm_dump_cpu256_ddr_map();
#endif
#endif
#else
	/* there are no more memory space reserved by L3QM */
	//printk("PE0 pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_pe0_pool1_buf_sz * l3qm_eq_profile_pe0_pool1_buf_count);
#endif
}

static ca_status_t aal_l3qm_create_ddr_map(void *ddr_coherent_base, void *ddr_noncache_base, void *ddr_noncache_phy_base)
{
	//int i;
	ca_uint32_t size = 0;
	ca_uint8_t *ddr_coherent_start_addr = ddr_coherent_base;
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	ca_uint8_t *ddr_noncache_start_addr = ddr_noncache_base;
 	ca_uint8_t *ddr_noncache_phy_addr = ddr_noncache_phy_base;
#endif
	//ca_uint8_t *ptr;
	ca_uint_t	next_buff_start_addr;
	ca_uint32_t	next_buff_phy_addr;

	/* DDR coherent */
	/* CPU pool0 and pool1 */
	/* ddr_coherent_base is physical address */

#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_RTL8198F)

	/* CPU RX FIFO ,  for memory shrink, Patched from Aaron/Cortina 1116 */
	l3qm_epp_profile_cpu_rx_fifo_start_addr = (ca_uint_t)phys_to_virt((ca_uint_t)ddr_coherent_start_addr);
#else
	l3qm_eq_profile_cpu_pool0_phy_addr = (ca_uint_t)ddr_coherent_start_addr + size;
	l3qm_eq_profile_cpu_pool0_start_addr = (ca_uint_t)phys_to_virt(l3qm_eq_profile_cpu_pool0_phy_addr);
	size = l3qm_eq_profile_cpu_pool0_buf_sz * l3qm_eq_profile_cpu_pool0_buf_count;

	/* Magic number */
	size += aal_l3qm_insert_magic_number((void *)(l3qm_eq_profile_cpu_pool0_start_addr + size));

	l3qm_eq_profile_cpu_pool1_start_addr = ((l3qm_eq_profile_cpu_pool0_start_addr + size + l3qm_eq_buffer_align_size) /
			l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
	l3qm_eq_profile_cpu_pool1_phy_addr = virt_to_phys((void *)l3qm_eq_profile_cpu_pool1_start_addr);
	size = l3qm_eq_profile_cpu_pool1_buf_sz * l3qm_eq_profile_cpu_pool1_buf_count;

	/* Magic number */
	size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_cpu_pool1_start_addr + size);

	/* CPU RX FIFO */
	l3qm_epp_profile_cpu_rx_fifo_start_addr = ((l3qm_eq_profile_cpu_pool1_start_addr + size + l3qm_eq_buffer_align_size) /
			l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
#endif
	l3qm_epp_profile_cpu_rx_fifo_phy_addr = virt_to_phys((void *)l3qm_epp_profile_cpu_rx_fifo_start_addr);
	size = l3qm_epp_profile_cpu_sz * l3qm_epp_profile_cpu_cn + (l3qm_magic_marker_len * ca_ni_cpu_cpu_port_count * l3qm_cpu_voq_per_port);

	/* Magic number */
	size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_epp_profile_cpu_rx_fifo_start_addr + size);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if defined(CONFIG_LUNA_G3_SERIES)
   /* for memory shrink , Patched from Aaron/Cortina 1116 */
	//size = (l3qm_epp_profile_cpu_rx_fifo_start_addr + size) - l3qm_eq_profile_cpu_pool0_start_addr;
#else
	size = (l3qm_epp_profile_cpu_rx_fifo_start_addr + size) - l3qm_eq_profile_cpu_pool0_start_addr;
#endif
	printk("%s: total size of DDR coherent consumed=0x%x\n", __func__, size);

	if (size > ca_ne_ddr_coherent_reserved_size) {
		printk("%s: out of DDR Coherent! NE driver initialized failed!!!\n", __func__);
		aal_l3qm_dump_ddr_map();
	}
	BUG_ON(size > ca_ne_ddr_coherent_reserved_size);

	/* DDR non-cacheable */
	/* DEEPQ pool0 buffer */
	size = 0;
	l3qm_eq_profile_dq_pool0_buff_start_addr = (ca_uint_t)ddr_noncache_start_addr + size;
	l3qm_eq_profile_dq_pool0_buff_phy_addr = (ca_uint32_t)(uintptr_t) ddr_noncache_phy_addr + size;
#else
#if defined(CONFIG_ARCH_RTL8198F)
	/* To shrink memory usage, we do not need to reserve RX FIFO here
	  * because it will be allocated at ca_ni_alloc_cpu_rx_fifo() again.
	  */
	l3qm_eq_profile_dq_pool0_buff_start_addr = l3qm_epp_profile_cpu_rx_fifo_start_addr;
	l3qm_eq_profile_dq_pool0_buff_phy_addr = l3qm_epp_profile_cpu_rx_fifo_phy_addr;
#else
	l3qm_eq_profile_dq_pool0_buff_start_addr = l3qm_epp_profile_cpu_rx_fifo_start_addr + size;
	l3qm_eq_profile_dq_pool0_buff_phy_addr = l3qm_epp_profile_cpu_rx_fifo_phy_addr + size;
#endif
#endif
	if(l3qm_eq0_sram_eq1_dram == 1) {
		size = 0;
	} else {
		size = l3qm_eq_profile_dq_pool0_buf_count * l3qm_eq_profile_dq_pool0_buf_sz;
	}
	/* Magic number */
	size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_dq_pool0_buff_start_addr + size);

	/* DEEPQ pool1 buffer */
	l3qm_eq_profile_dq_pool1_buff_start_addr = (ca_uint_t)l3qm_eq_profile_dq_pool0_buff_start_addr + size;
	l3qm_eq_profile_dq_pool1_buff_phy_addr = (ca_uint32_t)l3qm_eq_profile_dq_pool0_buff_phy_addr + size;
	size = l3qm_eq_profile_dq_pool1_buf_count * l3qm_eq_profile_dq_pool1_buf_sz;

	/* Magic number */
	size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_dq_pool1_buff_start_addr + size);

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_CORTINA_G3LITE)
	/* Main hash */
	l3fe_main_hash_table_start_addr = (ca_uint_t)l3qm_eq_profile_dq_pool1_buff_start_addr + size;
	l3fe_main_hash_table_phy_addr = (ca_uint_t)l3qm_eq_profile_dq_pool1_buff_phy_addr + size;
	size = l3fe_main_hash_table_entry_count * l3fe_main_hash_table_entry_length;

	/* Magic number */
	size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3fe_main_hash_table_start_addr + size);

	/* Main FIB */
#if defined(CONFIG_ARCH_RTL8198F)
	/* To shrink memory usage, we do not need to reserve Main hash & Main FIB here
	  * because they will be allocated at ca_ni_alloc_main_hash() again.
	  */
	l3fe_main_action_table_start_addr = (ca_uint_t)l3fe_main_hash_table_start_addr;
	l3fe_main_action_table_phy_addr = (ca_uint_t)l3fe_main_hash_table_phy_addr;
	size = 0;
#else
	l3fe_main_action_table_start_addr = (ca_uint_t)l3fe_main_hash_table_start_addr + size;
	l3fe_main_action_table_phy_addr = (ca_uint_t)l3fe_main_hash_table_phy_addr + size;
	size = l3fe_main_hash_action_table_entry_count * l3fe_main_hash_action_table_entry_length;

	/* Magic number */
	size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3fe_main_action_table_start_addr + size);
#endif
	size = l3fe_main_action_table_start_addr - l3qm_eq_profile_dq_pool0_buff_start_addr + size;

       if (l3qm_main_hash_bm_test == 0) {
                /* Main hash overflow action */
                l3fe_main_hash_overflow_hash_fib_start_addr = (ca_uint_t)l3fe_main_action_table_start_addr + size;
                l3fe_main_hash_overflow_hash_fib_phy_addr   = (ca_uint_t)l3fe_main_action_table_phy_addr + size;
                size += l3fe_main_hash_overflow_hash_fib_length * l3fe_main_hash_overflow_hash_fib_count;

                /* Magic number */
                size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3fe_main_action_table_start_addr + size);

                /* Main hash default action */
                l3fe_main_hash_default_hash_fib_start_addr = (ca_uint_t)l3fe_main_action_table_start_addr + size;
                l3fe_main_hash_default_hash_fib_phy_addr = (ca_uint_t)l3fe_main_action_table_phy_addr + size;
                size += l3fe_main_hash_default_hash_fib_length * l3fe_main_hash_default_hash_fib_count;

                /* Magic number */
                size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3fe_main_action_table_start_addr + size);

                /* Main hash action */
                l3fe_main_hash_action_cache_fib_start_addr = (ca_uint_t)l3fe_main_action_table_start_addr + size;
                l3fe_main_hash_action_cache_fib_phy_addr = (ca_uint_t)l3fe_main_action_table_phy_addr + size;
                size += l3fe_main_hash_action_cache_fib_length * l3fe_main_hash_action_cache_fib_count;

                /* Magic number */
                size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3fe_main_action_table_start_addr + size);

		size = l3fe_main_hash_action_cache_fib_start_addr - l3qm_eq_profile_dq_pool0_buff_start_addr + size;
        }

       next_buff_start_addr = (ca_uint_t)((l3fe_main_action_table_start_addr + size + l3qm_eq_buffer_align_size) /
		       l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
       next_buff_phy_addr = (ca_uint_t)((l3fe_main_action_table_phy_addr + size + l3qm_eq_buffer_align_size) /
		       l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;

#if defined(CONFIG_NE_CPU256)
	if (support_cpu256_in_linux)
	size = aal_l3qm_create_cpu256_ddr_map(next_buff_start_addr, next_buff_phy_addr);
#endif

#if defined(CONFIG_ARCH_CORTINA_G3LITE)
        printk("%s: total size of DDR coherent consumed=0x%x\n", __func__, size);

        if (size > ca_ne_ddr_coherent_size) {
                printk("%s: out of DDR coherent! NE driver initialized failed!!!\n", __func__);
                aal_l3qm_dump_ddr_map();
        }
        BUG_ON(size > ca_ne_ddr_coherent_size);		
#else 
        printk("%s: total size of DDR non-cache consumed=0x%x\n", __func__, size);

        if (size > ca_ne_ddr_noncache_reserved_size) {
                printk("%s: out of DDR non-cache! NE driver initialized failed!!!\n", __func__);
		aal_l3qm_dump_ddr_map();
	}
        BUG_ON(size > ca_ne_ddr_noncache_reserved_size);
#endif
#else 

	/* DEEPQ pool1 buffer */
	size = l3qm_eq_profile_dq_pool1_buf_count * l3qm_eq_profile_dq_pool1_buf_sz;

	size = l3qm_eq_profile_dq_pool1_buff_start_addr - l3qm_eq_profile_cpu_pool0_start_addr + size;

	printk("%s: next_buff_start_addr=0x%x, l3qm_eq_profile_cpu_pool0_start_addr=0x%x\n", __func__,
		next_buff_start_addr, l3qm_eq_profile_cpu_pool0_start_addr);
	printk("%s: ca_ne_ddr_coherent_reserved_size=%d\n", __func__, ca_ne_ddr_coherent_reserved_size);
	printk("%s: total size of DDR coherent consumed=0x%x\n", __func__, size);

        if (size > ca_ne_ddr_coherent_reserved_size) {
                printk("%s: out of DDR coherent! NE driver initialized failed!!!\n", __func__);
                aal_l3qm_dump_ddr_map();
        }
        BUG_ON(size > ca_ne_ddr_coherent_reserved_size);
#endif

	aal_l3qm_dump_ddr_map();

	return CA_E_OK;
}

void aal_l3qm_init_startup_config(void)
{
	/* read additional configure */
	aal_scfg_read(CFG_ID_L3QM_MAGIC_MARKER_LEN, 1, &l3qm_magic_marker_len);
	aal_scfg_read(CFG_ID_L3QM_DESC_SIZE, 1, &l3qm_desc_size);
	aal_scfg_read(CFG_ID_L3QM_DESC_PER_EPP, 1, &l3qm_desc_per_epp);
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
	aal_scfg_read(CFG_ID_L3QM_EQ0_SRAM_EQ1_DRAM, 1, &l3qm_eq0_sram_eq1_dram);
#endif

#if 0
	aal_scfg_read(CFG_ID_L3QM_PE0_1_PORT_COUNT, 4, &l3qm_pe0_1_port_count);
	aal_scfg_read(CFG_ID_L3QM_PE0_1_VOQ_PER_PORT, 1, &l3qm_pe0_1_voq_per_port);
	aal_scfg_read(CFG_ID_L3QM_PE0_1_EPP_PER_VOQ, 4, &l3qm_pe0_1_epp_per_voq);
	aal_scfg_read(CFG_ID_L3QM_PE0_1_POOL_BID_COUNT, 4, &l3qm_pe0_1_pool_bid_count);
#endif

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	aal_scfg_read(CFG_ID_L3QM_CPU256_PORT_COUNT, 4, &l3qm_cpu256_port_count);
	aal_scfg_read(CFG_ID_L3QM_CPU256_VOQ_PER_PORT, 1, &l3qm_cpu256_voq_per_port);
	aal_scfg_read(CFG_ID_L3QM_CPU256_EPP_PER_VOQ, 4, &l3qm_cpu256_epp_per_voq);
	aal_scfg_read(CFG_ID_L3QM_CPU256_POOL_BID_COUNT, 4, &l3qm_cpu256_pool_bid_count);
#endif

	aal_scfg_read(CFG_ID_L3QM_CPU_PORT_COUNT, 4, &l3qm_cpu_port_count);
	aal_scfg_read(CFG_ID_L3QM_CPU_VOQ_PER_PORT, 1, &l3qm_cpu_voq_per_port);
	aal_scfg_read(CFG_ID_L3QM_CPU_EPP_PER_VOQ, 4, &l3qm_cpu_epp_per_voq);
	aal_scfg_read(CFG_ID_L3QM_CPU_POOL_BID_COUNT, 4, &l3qm_cpu_pool_bid_count);
	//aal_scfg_read(CFG_ID_L3QM_DEEPQ_POOL_BID_COUNT, 4, &l3qm_dq_pool_bid_count);

#if 0
	aal_scfg_read(CFG_ID_L3QM_CPU_CACHE_LINE_SIZE, 1, &l3qm_cpu_cache_line_size);
	aal_scfg_read(CFG_ID_L3QM_PE_BUFFER_ALIGN_SIZE, 1, &l3qm_pe_buffer_align_size);
#endif

	aal_scfg_read(CFG_ID_L3QM_EQ_BUFFER_ALIGN_SIZE, 4, &l3qm_eq_buffer_align_size);

#if 0
	/* read PE0 configure */
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_ID, 1, &l3qm_eq_profile_pe0_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_RULE, 1, &l3qm_eq_profile_pe0_rule);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_USE_FBM, 1, &l3qm_eq_profile_pe0_use_fbm);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL0_EQ_ID, 1, &l3qm_eq_profile_pe0_pool0_eq_id);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_pe0_pool0_buf_count);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL0_BID_START, 4, &l3qm_eq_profile_pe0_pool0_bid_start);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL0_BUF_SZ, 4, &l3qm_eq_profile_pe0_pool0_buf_sz);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL0_LOCATION, 1, &l3qm_eq_profile_pe0_pool0_location);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_pe0_pool0_fbm_pool_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL1_EQ_ID, 1, &l3qm_eq_profile_pe0_pool1_eq_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_pe0_pool1_buf_count);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL1_BID_START, 4, &l3qm_eq_profile_pe0_pool1_bid_start);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL1_BUF_SZ, 4, &l3qm_eq_profile_pe0_pool1_buf_sz);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL1_LOCATION, 1, &l3qm_eq_profile_pe0_pool1_location);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_pe0_pool1_fbm_pool_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE0_ID_PORTS, 4, &l3qm_eq_profile_pe0_id_ports);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_PE0_ID, 1, &l3qm_epp_profile_pe0_id);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_PE0_SZ, 4, &l3qm_epp_profile_pe0_sz);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_PE0_CN, 4, &l3qm_epp_profile_pe0_cn);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_PE0_MAP_MODE, 4, &l3qm_epp_profile_pe0_map_mode);

	/* read PE 1 configure */
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_ID, 1, &l3qm_eq_profile_pe1_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_RULE, 1, &l3qm_eq_profile_pe1_rule);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_USE_FBM, 1, &l3qm_eq_profile_pe1_use_fbm);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_EQ_ID, 1, &l3qm_eq_profile_pe1_pool0_eq_id);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_pe1_pool0_buf_count);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_BID_START, 4, &l3qm_eq_profile_pe1_pool0_bid_start);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_BUF_SZ, 4, &l3qm_eq_profile_pe1_pool0_buf_sz);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_LOCATION, 1, &l3qm_eq_profile_pe1_pool0_location);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_pe1_pool0_fbm_pool_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL1_EQ_ID, 1, &l3qm_eq_profile_pe1_pool1_eq_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_pe1_pool1_buf_count);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL1_BID_START, 4, &l3qm_eq_profile_pe1_pool1_bid_start);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL1_BUF_SZ, 4, &l3qm_eq_profile_pe1_pool1_buf_sz);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL1_LOCATION, 1, &l3qm_eq_profile_pe1_pool1_location);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_pe1_pool1_fbm_pool_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_ID_PORTS, 4, &l3qm_eq_profile_pe1_id_ports);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_PE1_ID, 1, &l3qm_epp_profile_pe1_id);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_PE1_SZ, 4, &l3qm_epp_profile_pe1_sz);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_PE1_CN, 4, &l3qm_epp_profile_pe1_cn);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_PE1_MAP_MODE, 4, &l3qm_epp_profile_pe1_map_mode);
#endif

	/* read CPU configure */
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_ID, 1, &l3qm_eq_profile_cpu_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_RULE, 1, &l3qm_eq_profile_cpu_rule);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_USE_FBM, 1, &l3qm_eq_profile_cpu_use_fbm);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_EQ_ID, 1, &l3qm_eq_profile_cpu_pool0_eq_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_cpu_pool0_buf_count);

	if(l3qm_eq0_sram_eq1_dram == 1)
		l3qm_eq_profile_cpu_pool0_bid_start = 0;
	else
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BID_START, 4, &l3qm_eq_profile_cpu_pool0_bid_start);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BUF_SZ, 4, &l3qm_eq_profile_cpu_pool0_buf_sz);

	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_LOCATION, 1, &l3qm_eq_profile_cpu_pool0_location);

	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu_pool0_fbm_pool_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_EQ_ID, 1, &l3qm_eq_profile_cpu_pool1_eq_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_cpu_pool1_buf_count);

	if(l3qm_eq0_sram_eq1_dram == 1)
		l3qm_eq_profile_cpu_pool1_bid_start = 1500;
	else
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BID_START, 4, &l3qm_eq_profile_cpu_pool1_bid_start);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BUF_SZ, 4, &l3qm_eq_profile_cpu_pool1_buf_sz);

	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_LOCATION, 1, &l3qm_eq_profile_cpu_pool1_location);

	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu_pool1_fbm_pool_id);

	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_ID_PORTS, 6, l3qm_eq_profile_cpu_id_ports);

	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU_ID, 1, &l3qm_epp_profile_cpu_id);

	//aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU_SZ, 4, &l3qm_epp_profile_cpu_sz);
	//aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU_CN, 4, &l3qm_epp_profile_cpu_cn);

	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU_MAP_MODE, 4, &l3qm_epp_profile_cpu_map_mode);

	/* read DEEPQ configure */
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_ID, 1, &l3qm_eq_profile_dq_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_RULE, 1, &l3qm_eq_profile_dq_rule);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_USE_FBM, 1, &l3qm_eq_profile_dq_use_fbm);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_EQ_ID, 1, &l3qm_eq_profile_dq_pool0_eq_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_dq_pool0_buf_count);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BID_START, 4, &l3qm_eq_profile_dq_pool0_bid_start);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BUF_SZ, 4, &l3qm_eq_profile_dq_pool0_buf_sz);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_LOCATION, 1, &l3qm_eq_profile_dq_pool0_location);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_dq_pool0_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_EQ_ID, 1, &l3qm_eq_profile_dq_pool1_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_dq_pool1_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BID_START, 4, &l3qm_eq_profile_dq_pool1_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BUF_SZ, 4, &l3qm_eq_profile_dq_pool1_buf_sz);
        //aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_LOCATION, 1, &l3qm_eq_profile_dq_pool1_location);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_dq_pool1_fbm_pool_id);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_ID_PORTS, 6, l3qm_eq_profile_dq_id_ports);

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	/* read CPU256 hi configure */
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_ID, 1, &l3qm_eq_profile_cpu256_hi_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_RULE, 1, &l3qm_eq_profile_cpu256_hi_rule);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_USE_FBM, 1, &l3qm_eq_profile_cpu256_hi_use_fbm);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_EQ_ID, 1, &l3qm_eq_profile_cpu256_hi_pool0_eq_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_hi_pool0_buf_count);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BID_START, 4, &l3qm_eq_profile_cpu256_hi_pool0_bid_start);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_BUF_SZ, 4, &l3qm_eq_profile_cpu256_hi_pool0_buf_sz);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_LOCATION, 1, &l3qm_eq_profile_cpu256_hi_pool0_location);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_hi_pool0_fbm_pool_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_EQ_ID, 1, &l3qm_eq_profile_cpu256_hi_pool1_eq_id);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_hi_pool1_buf_count);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BID_START, 4, &l3qm_eq_profile_cpu256_hi_pool1_bid_start);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_BUF_SZ, 4, &l3qm_eq_profile_cpu256_hi_pool1_buf_sz);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_LOCATION, 1, &l3qm_eq_profile_cpu256_hi_pool1_location);
	aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_HI_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_hi_pool1_fbm_pool_id);
	//aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_ID_PORTS, 16, l3qm_eq_profile_cpu256_id_ports);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_ID, 1, &l3qm_epp_profile_cpu256_hi_id);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_SZ, 4, &l3qm_epp_profile_cpu256_hi_sz);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_CN, 4, &l3qm_epp_profile_cpu256_hi_cn);
	aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_HI_MAP_MODE, 4, &l3qm_epp_profile_cpu256_hi_map_mode);

        /* read CPU256 lo configure */
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_ID, 1, &l3qm_eq_profile_cpu256_lo_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_RULE, 1, &l3qm_eq_profile_cpu256_lo_rule);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_USE_FBM, 1, &l3qm_eq_profile_cpu256_lo_use_fbm);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_EQ_ID, 1, &l3qm_eq_profile_cpu256_lo_pool0_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_lo_pool0_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BID_START, 4, &l3qm_eq_profile_cpu256_lo_pool0_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_BUF_SZ, 4, &l3qm_eq_profile_cpu256_lo_pool0_buf_sz);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_LOCATION, 1, &l3qm_eq_profile_cpu256_lo_pool0_location);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_lo_pool0_fbm_pool_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_EQ_ID, 1, &l3qm_eq_profile_cpu256_lo_pool1_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_cpu256_lo_pool1_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BID_START, 4, &l3qm_eq_profile_cpu256_lo_pool1_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_BUF_SZ, 4, &l3qm_eq_profile_cpu256_lo_pool1_buf_sz);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_LOCATION, 1, &l3qm_eq_profile_cpu256_lo_pool1_location);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_LO_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu256_lo_pool1_fbm_pool_id);
        //aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU256_ID_PORTS, 16, l3qm_eq_profile_cpu256_id_ports);
        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_ID, 1, &l3qm_epp_profile_cpu256_lo_id);
        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_SZ, 4, &l3qm_epp_profile_cpu256_lo_sz);
        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_CN, 4, &l3qm_epp_profile_cpu256_lo_cn);
        aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU256_LO_MAP_MODE, 4, &l3qm_epp_profile_cpu256_lo_map_mode);
#endif

	/* read L3FE Main hash configure */
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_OVERFLOW_FIB_LENGTH, 4, &l3fe_main_hash_overflow_hash_fib_length);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_OVERFLOW_FIB_COUNT, 4, &l3fe_main_hash_overflow_hash_fib_count);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_DEFAULT_HASH_FIB_LENGTH, 4, &l3fe_main_hash_default_hash_fib_length);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_DEFAULT_HASH_FIB_COUNT, 4, &l3fe_main_hash_default_hash_fib_count);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_CACHE_FIB_LENGTH, 4, &l3fe_main_hash_action_cache_fib_length);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_CACHE_FIB_COUNT, 4, &l3fe_main_hash_action_cache_fib_count);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_TABLE_ENTRY_LENGTH, 4, &l3fe_main_hash_table_entry_length);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_TABLE_ENTRY_COUNT, 4, &l3fe_main_hash_table_entry_count);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_TABLE_ENTRY_LENGTH, 4, &l3fe_main_hash_action_table_entry_length);
	aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_TABLE_ENTRY_COUNT, 4, &l3fe_main_hash_action_table_entry_count);

	/* read DDR/SRAM base address and AXI top bits */
#if 0
	aal_scfg_read(CFG_ID_L3QM_CCI0_START_ADDR, 4, &l3qm_cci0_start_addr);
	aal_scfg_read(CFG_ID_L3QM_CCI1_START_ADDR, 4, &l3qm_cci1_start_addr);
	aal_scfg_read(CFG_ID_L3QM_SRAM_START_ADDR, 4, &l3qm_sram_start_addr);
	aal_scfg_read(CFG_ID_L3QM_DDR_CACHE_COHERENT_START_ADDR, 4, &l3qm_ddr_cache_coherent_start_addr);
	aal_scfg_read(CFG_ID_L3QM_DDR_NON_CACHE_START_ADDR, 4, &l3qm_ddr_non_cacheable_start_addr);
	aal_scfg_read(CFG_ID_L3QM_EQ_AXI_TOP8_BITS, 1, &l3qm_eq_axi_top8_bits);
	aal_scfg_read(CFG_ID_L3QM_EPP_AXI_TOP8_BITS, 1, &l3qm_epp_axi_top8_bits);
#endif

	/* read CPU port head room first */
	aal_scfg_read(CFG_ID_L3QM_CPU_PORT_HEAD_ROOM_FIRST, 4, &l3qm_cpu_port_head_room_first);

	/* read DEEPQ test bit */
	//aal_scfg_read(CFG_ID_L3QM_DEEPQ_TEST_WITH_SRAM, 1, &l3qm_deepq_test_with_sram);

	/* read QM ACE test bit */
	aal_scfg_read(CFG_ID_L3QM_ACE_TEST, 1, &l3qm_ace_test);

	/* read Main Hash Bench Mark test */
	aal_scfg_read(CFG_ID_MAIN_HASH_BM_TEST, 1, &l3qm_main_hash_bm_test);

	/* read EQ QM_CFG3/4 */
	aal_scfg_read(CFG_ID_L3QM_EQ_CFG3_VALUES, CFG_ID_L3QM_EQ_CFG3_VALUES_LEN * sizeof(ca_uint32_t), l3qm_eq_cfg3_values);
	aal_scfg_read(CFG_ID_L3QM_EQ_CFG4_VALUES, CFG_ID_L3QM_EQ_CFG4_VALUES_LEN * sizeof(ca_uint32_t), l3qm_eq_cfg4_values);

	/* read QM AXI ATTRIBUTE */
        aal_scfg_read(CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES, CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES_LEN * sizeof(ca_uint32_t), l3qm_axi_attrib_eq_values);

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
        aal_scfg_read(CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES, CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES_LEN * sizeof(ca_uint32_t), l3qm_axi_attrib_cpu256_epp_values);
#endif

        aal_scfg_read(CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES, CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES_LEN * sizeof(ca_uint32_t), l3qm_axi_attrib_cpu_epp_values);

	/* calculate from startup configs */
	l3qm_epp_size = (l3qm_desc_size * l3qm_desc_per_epp);   /* EPP size in byte */

	//l3qm_eq_profile_pe0_1_pool_id_count = (l3qm_pe0_1_port_count * l3qm_pe0_1_voq_per_port * l3qm_pe0_1_epp_per_voq);   /* PE0/PE1 pool id count */
	l3qm_eq_profile_pe0_1_total_epp_count = (l3qm_pe0_1_port_count * l3qm_pe0_1_voq_per_port * l3qm_pe0_1_epp_per_voq);    /* PE0/PE1 total EPP count */

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	//l3qm_eq_profile_cpu256_pool_id_count = (l3qm_cpu256_port_count * l3qm_cpu256_voq_per_port * l3qm_cpu256_epp_per_voq);  /* CPU256 pool id count */
	l3qm_epp_profile_cpu256_hi_total_epp_count = (l3qm_cpu256_port_count * l3qm_cpu256_voq_per_port * l3qm_cpu256_epp_per_voq);  /* CPU256 total EPP count */
#endif

	l3qm_epp_profile_cpu_total_epp_count = (ca_ni_cpu_cpu_port_count * l3qm_cpu_voq_per_port * l3qm_cpu_epp_per_voq);  /* CPU total EPP count */

	/* derived from start configs */
	//l3qm_eq_profile_pe0_pool0_buf_count = (l3qm_pe0_1_port_count * l3qm_pe0_1_voq_per_port * l3qm_pe0_1_epp_per_voq);
	//l3qm_eq_profile_pe0_pool1_buf_count = (l3qm_pe0_1_port_count * l3qm_pe0_1_voq_per_port * l3qm_pe0_1_epp_per_voq);

	l3qm_epp_profile_pe0_sz = l3qm_epp_size;
	l3qm_epp_profile_pe0_cn = l3qm_eq_profile_pe0_1_total_epp_count;

	//l3qm_eq_profile_pe1_pool0_buf_count = (l3qm_pe0_1_port_count * l3qm_pe0_1_voq_per_port * l3qm_pe0_1_epp_per_voq);
	//l3qm_eq_profile_pe1_pool1_buf_count = (l3qm_pe0_1_port_count * l3qm_pe0_1_voq_per_port * l3qm_pe0_1_epp_per_voq);

	//printk("%s: l3qm_eq_profile_pe1_pool0_buf_count=%d, l3qm_eq_profile_pe1_pool1_buf_count=%d\n", __func__,
	//	l3qm_eq_profile_pe1_pool0_buf_count, l3qm_eq_profile_pe1_pool1_buf_count);

	l3qm_epp_profile_pe1_sz = l3qm_epp_size;
	l3qm_epp_profile_pe1_cn = l3qm_eq_profile_pe0_1_total_epp_count;

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	//l3qm_eq_profile_cpu256_hi_pool0_buf_count = l3qm_epp_profile_cpu256_hi_total_epp_count + l3qm_cpu256_pool_bid_count;
	printk("%s: l3qm_eq_profile_cpu256_hi_pool0_buf_count=%d\n", __func__, l3qm_eq_profile_cpu256_hi_pool0_buf_count);

	//l3qm_eq_profile_cpu256_hi_pool1_buf_count = l3qm_cpu256_pool_bid_count;
	printk("%s: l3qm_eq_profile_cpu256_hi_pool1_buf_count=%d\n", __func__, l3qm_eq_profile_cpu256_hi_pool1_buf_count);

	l3qm_epp_profile_cpu256_hi_sz = l3qm_epp_size;
	//l3qm_epp_profile_cpu256_hi_cn = l3qm_epp_profile_cpu256_hi_total_epp_count;
#endif

	//l3qm_eq_profile_cpu_pool0_buf_count = l3qm_epp_profile_cpu_total_epp_count + l3qm_cpu_pool_bid_count;
	//l3qm_eq_profile_cpu_pool1_buf_count = l3qm_epp_profile_cpu_total_epp_count + l3qm_cpu_pool_bid_count;
	//printk("%s: l3qm_eq_profile_cpu_pool1_buf_count=%d\n", __func__, l3qm_eq_profile_cpu_pool1_buf_count);

	l3qm_epp_profile_cpu_sz = l3qm_epp_size;
	l3qm_epp_profile_cpu_cn = l3qm_epp_profile_cpu_total_epp_count;

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	/* caculate bid_start from bid count */
	//l3qm_eq_profile_pe1_pool0_bid_start = l3qm_eq_profile_pe0_pool0_bid_start + l3qm_pe0_1_pool_bid_count;
	//l3qm_eq_profile_pe0_pool1_bid_start = l3qm_eq_profile_pe1_pool0_bid_start + l3qm_pe0_1_pool_bid_count;
	//l3qm_eq_profile_pe1_pool1_bid_start = l3qm_eq_profile_pe0_pool1_bid_start + l3qm_pe0_1_pool_bid_count;

	//l3qm_eq_profile_cpu256_hi_pool0_bid_start = l3qm_eq_profile_pe1_pool1_bid_start + l3qm_pe0_1_pool_bid_count;
	/* get the BID start from startup config directly */
#if 0
	l3qm_eq_profile_cpu256_hi_pool1_bid_start = l3qm_eq_profile_cpu256_hi_pool0_bid_start + l3qm_cpu256_pool_bid_count;
	l3qm_eq_profile_cpu256_lo_pool0_bid_start = l3qm_eq_profile_cpu256_hi_pool1_bid_start + l3qm_cpu256_pool_bid_count;
	l3qm_eq_profile_cpu256_lo_pool1_bid_start = l3qm_eq_profile_cpu256_lo_pool0_bid_start + l3qm_cpu256_pool_bid_count;

	l3qm_eq_profile_cpu_pool0_bid_start	  = l3qm_eq_profile_cpu256_lo_pool1_bid_start + l3qm_cpu256_pool_bid_count;
	l3qm_eq_profile_cpu_pool1_bid_start	  = l3qm_eq_profile_cpu_pool0_bid_start + l3qm_cpu_pool_bid_count;

	l3qm_eq_profile_dq_pool0_bid_start	  = l3qm_eq_profile_cpu_pool1_bid_start + l3qm_cpu_pool_bid_count;
	l3qm_eq_profile_dq_pool1_bid_start	  = l3qm_eq_profile_dq_pool0_bid_start + l3qm_dq_pool_bid_count;
#endif

	//l3qm_eq_profile_dq_pool0_buf_count = l3qm_dq_pool_bid_count;
	//printk("%s: l3qm_eq_profile_dq_pool0_buf_count=%d\n", __func__, l3qm_eq_profile_dq_pool0_buf_count);
#else
 #if defined(CONFIG_NE_CPU256)
	aal_scfg_read(CFG_ID_SUPPORT_CPU256_IN_LINUX, 1, &support_cpu256_in_linux);
	if (support_cpu256_in_linux) {
	aal_l3qm_init_cpu256_startup_config();
	}
 #endif
#endif
#endif

#if 0
	/* read DEEPQ enable configure */
	aal_scfg_read(CFG_ID_L3QM_NI_P0_7_DEEP_Q_EN, 1, &l3qm_ni_p0_7_deep_q_en);
	aal_l3qm_ni_deep_queue_enable(l3qm_ni_p0_7_deep_q_en);
#endif
}

void aal_l3qm_set_cpu_rx_fifo_addr(ca_uint_t cpu_rx_fifo_phy_addr, ca_uint_t cpu_rx_fifo_virt_addr)
{
        l3qm_epp_profile_cpu_rx_fifo_phy_addr = cpu_rx_fifo_phy_addr;
        l3qm_epp_profile_cpu_rx_fifo_start_addr = cpu_rx_fifo_virt_addr;
}

ca_uint32_t aal_l3qm_get_cpu_rx_fifo_size(void)
{
        return l3qm_epp_profile_cpu_sz * l3qm_epp_profile_cpu_cn + (l3qm_magic_marker_len * ca_ni_cpu_cpu_port_count * l3qm_cpu_voq_per_port);
}

void aal_l3qm_init_reserved_mem(void *sram_base, ca_uint32_t sram_size,
	void *sram_phy_base, void *ddr_coherent_base, void *ddr_noncache_base, void *ddr_noncache_phy_base)
{
	/* create sram map */
	aal_l3qm_create_sram_map(sram_base, sram_size, sram_phy_base);

	/* create ddr map */
	aal_l3qm_create_ddr_map(ddr_coherent_base, ddr_noncache_base, ddr_noncache_phy_base);
}

static void aal_saturn_l3qm_init_voq(void);

void aal_l3qm_init_voq(void)
{
	int i;
	QM_QM_SCH_CFG0_t sch_cfg;

	sch_cfg.wrd = CA_NE_REG_READ(QM_QM_SCH_CFG0);
	for (i = 0; i < QM_QM_SCH_CFG0_COUNT; i++) {

		sch_cfg.bf.voq_en = 0xff;
		CA_NE_REG_WRITE(sch_cfg.wrd, QM_QM_SCH_CFG0 + (i * QM_QM_SCH_CFG0_STRIDE));
	}

	/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	CA_NE_REG_WRITE(0xffff, QM_QM_CPU_EPP_256_FIFO_INT_LO);
#else
 #if defined(CONFIG_NE_CPU256)
	if (support_cpu256_in_linux)
	aal_l3qm_init_cpu256_voq();
 #endif
#endif

#endif

	//aal_saturn_l3qm_init_voq();
}

ca_uint32_t aal_saturn_l3qm_check_init_done(void);

ca_uint32_t aal_l3qm_check_init_done(void)
{
	int i;
	QM_QM_PHY_PORT_STS_t	init_done;

	printk("%s: ===========================Enter========================\n", __func__);
        /* Check initializtion of NI block is done */
        for (i = 0; i < CA_L3QM_INIT_DONE_LOOP; i++) {
                init_done.wrd = CA_NE_REG_READ(QM_QM_PHY_PORT_STS);
                if (init_done.bf.qm_init_done)
                        break;
        }
        if (i == CA_L3QM_INIT_DONE_LOOP) {
                printk("%s: initializtion of QM block is NOT done!!!\n", __func__);
		return -1;
        }

	aal_saturn_l3qm_check_init_done();

	return 0;
}

/*
 * Procedure of retrieving a buffer back to CPU:
 * 1. Disable the Empty-buffer pool which physical address is going to be
 * retrieveled from;
 * 2. Enable "buf_retrieval" by writing "1" to this field and
 * the corresponding EQ-ID;
 * 3. read "EQ_RETURN_PHY_ADDR%d_phy_addr", with valid entry indication;
 * 4. repeat "#2" if more buffers need to be retrieved;
 * 5. Write "0" to "buf_retrieval_en" to disable this function;
*/
void aal_l3qm_get_return_phy_addr_enable(int eq_id, int enable)
{
	volatile ca_uint32_t reg_off;
	QM_QM_CFG0_EQ0_t cfg0_eq;
	QM_QM_EQ_PHY_ADDR_RETURN_CTRL_t   phy_addr_return_ctrl;

	if (enable) {
		/* disable Empty Buffer Pool */
		cfg0_eq.wrd = 0;
		cfg0_eq.bf.eq_en = 0;
		reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * eq_id);
		CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

		/* Enable "buf_retieval" */
		phy_addr_return_ctrl.bf.phy_addr_retrieval_en = 1;
		phy_addr_return_ctrl.bf.eq_id = eq_id;
		CA_NE_REG_WRITE(phy_addr_return_ctrl.wrd, QM_QM_EQ_PHY_ADDR_RETURN_CTRL);
	}
	else {
		/* disable "buf_retieval" */
		phy_addr_return_ctrl.bf.phy_addr_retrieval_en = 0;
		phy_addr_return_ctrl.bf.eq_id = eq_id;
		CA_NE_REG_WRITE(phy_addr_return_ctrl.wrd, QM_QM_EQ_PHY_ADDR_RETURN_CTRL);

		/* enable Empty Buffer Pool */
		cfg0_eq.wrd = 0;
		cfg0_eq.bf.eq_en = 1;
		reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * eq_id);
		CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);
	}
}

ca_uint32_t aal_l3qm_get_return_phy_addr(int eq_id)
{
	//volatile ca_uint32_t reg_off;
	QM_QM_EQ_PHY_ADDR_RETURN_CTRL_t   phy_addr_return_ctrl;
	QM_QM_EQ_RETURN_PHY_ADDR_t        return_phy_addr;

	/* Enable "buf_retieval" */
	phy_addr_return_ctrl.bf.phy_addr_retrieval_en = 1;
	phy_addr_return_ctrl.bf.eq_id = eq_id;
	CA_NE_REG_WRITE(phy_addr_return_ctrl.wrd, QM_QM_EQ_PHY_ADDR_RETURN_CTRL);

        /* read "EQ_RETURN_PHY_ADD%d_phy_addr" */
	//printk("%s: QM_QM_EQ_RETURN_PHY_ADDR=0x%x\n", __func__, QM_QM_EQ_RETURN_PHY_ADDR);
        return_phy_addr.wrd = CA_NE_REG_READ(QM_QM_EQ_RETURN_PHY_ADDR);

	if (ca_aal_debug & AAL_DBG_L3QM) {
		printk("%s: eqid=%d, retrieval_done=%d, valid=%d\n", __func__, eq_id, return_phy_addr.bf.retrieval_done, return_phy_addr.bf.valid);
		printk("%s: return_phy_addr.wrd=0x%x, phy_addr=0x%x\n", __func__, return_phy_addr.wrd, return_phy_addr.bf.phy_addr << 7);
	}

	/* check retrieval_done and valid bits */
	if (return_phy_addr.bf.retrieval_done) {
		return 0;
	}

	if (return_phy_addr.bf.valid)
		return (return_phy_addr.bf.phy_addr << 7);

        return 0;
}

/* functions for debugging print out used with iROS */
#define VOQ_INFO_ACCESS_TIMEOUT         1000000
#define HEADER_MEMORY_ACCESS_TIMEOUT    1000000
#define BID_PADDR_ACCESS_TIMEOUT    	1000000
ca_status_t aal_l3qm_get_voq_info_mem(ca_device_id_t device_id, ca_uint16_t addr, QM_QM_VOQ_INFO_MEMORY_DATA0_t *data0,
					QM_QM_VOQ_INFO_MEMORY_DATA1_t *data1, QM_QM_VOQ_INFO_MEMORY_DATA2_t *data2)
{
	int i;
	QM_QM_VOQ_INFO_MEMORY_ACCESS_t	access;

	access.bf.access = 1;
	access.bf.rbw = 0;
	access.bf.ADDR = addr;
	CA_NE_REG_WRITE(access.wrd, QM_QM_VOQ_INFO_MEMORY_ACCESS);

	for (i = 0; i < VOQ_INFO_ACCESS_TIMEOUT; i++) {
		access.wrd = CA_NE_REG_READ(QM_QM_VOQ_INFO_MEMORY_ACCESS);
		if (access.bf.access == 0) {
			break;
		}
	}
	if (i == VOQ_INFO_ACCESS_TIMEOUT) {
		printk("%s: VoQ addr=%d access timeout!!!\n", __func__, addr);
		return CA_E_ERROR;
	}
	data0->wrd = CA_NE_REG_READ(QM_QM_VOQ_INFO_MEMORY_DATA0);
	data1->wrd = CA_NE_REG_READ(QM_QM_VOQ_INFO_MEMORY_DATA1);
	data2->wrd = CA_NE_REG_READ(QM_QM_VOQ_INFO_MEMORY_DATA2);

	printk("[addr=%d]\n", addr);
	printk("voq_header=0x%x\n", data0->bf.voq_header);

	printk("voq_header=%d, voq_tail=0x%x, voq_head=0x%x, bcntr=0x%x\n",
		data1->bf.voq_header, data1->bf.voq_tail, data1->bf.voq_head, data1->bf.bcntr);

	printk("bcntr=0x%x, ecc=0x%x\n", data2->bf.bcntr, data2->bf.ecc);

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_get_voq_info_mem);

ca_status_t aal_l3qm_get_header_mem(ca_device_id_t device_id, ca_uint16_t addr, QM_QM_HEADER_MEMORY_DATA0_t *data0, QM_QM_HEADER_MEMORY_DATA1_t *data1)
{
        int i;
        QM_QM_HEADER_MEMORY_ACCESS_t      access;

        access.bf.access = 1;
        access.bf.rbw = 0;
        access.bf.ADDR = addr;
        CA_NE_REG_WRITE(access.wrd, QM_QM_HEADER_MEMORY_ACCESS);

        for (i = 0; i < HEADER_MEMORY_ACCESS_TIMEOUT; i++) {
                access.wrd = CA_NE_REG_READ(QM_QM_HEADER_MEMORY_ACCESS);
                if (access.bf.access == 0) {
                        break;
                }
        }
        if (i == HEADER_MEMORY_ACCESS_TIMEOUT) {
                printk("%s: header memory addr=%d access timeout!!!\n", __func__, addr);
                return CA_E_ERROR;
        }
        data0->wrd = CA_NE_REG_READ(QM_QM_HEADER_MEMORY_DATA0);
        data1->wrd = CA_NE_REG_READ(QM_QM_HEADER_MEMORY_DATA1);

	printk("[addr=%d]\n", addr);

	printk("next_buffer_ptr=%d, sop=%d, eop=%d, pkt_length=%d\n", data0->bf.next_buffer_ptr, data0->bf.sop, data0->bf.eop, data0->bf.pkt_length);

	printk("ecc=%d, pkt_type_ptp=%d\n", data1->bf.ecc, data1->bf.pkt_type_ptp);

        return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_get_header_mem);

ca_status_t aal_l3qm_get_bid_paddr(ca_device_id_t device_id, ca_uint16_t addr, QM_QM_BID_PADDR_LKUP_DATA_t *data)
{
        int i;
        QM_QM_BID_PADDR_LKUP_ACCESS_t      access;

        access.bf.access = 1;
        access.bf.rbw = 0;
        access.bf.ADDR = addr;
        CA_NE_REG_WRITE(access.wrd, QM_QM_BID_PADDR_LKUP_ACCESS);

        for (i = 0; i < BID_PADDR_ACCESS_TIMEOUT; i++) {
                access.wrd = CA_NE_REG_READ(QM_QM_BID_PADDR_LKUP_ACCESS);
                if (access.bf.access == 0) {
                        break;
                }
        }
        if (i == BID_PADDR_ACCESS_TIMEOUT) {
                printk("%s: BID paddr addr=%d access timeout!!!\n", __func__, addr);
                return CA_E_ERROR;
        }
        data->wrd = CA_NE_REG_READ(QM_QM_BID_PADDR_LKUP_DATA);

        printk("%s: paddr=0x%x\n", __func__, data->bf.phy_addr);

        return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_get_bid_paddr);

ca_status_t aal_l3qm_get_error_drop_cntr(ca_device_id_t device_id)
{
#if 0
	QM_QM_RMU0_NO_BUF_DROP_PKT_INFO_t rmu0_no_buf_drop;
	QM_QM_RMU0_FE_DROP_PKT_CNTR_t	rmu0_fe_drop;
	QM_QM_RMU0_RX_ERROR_DROP_PKT_CNTR_t rmu0_rx_error_drop;
	QM_QM_TX_CRC_ERROR_NI_CNTR_t	rx_crc_error;

	rmu0_no_buf_drop.wrd = CA_NE_REG_READ(QM_QM_RMU0_NO_BUF_DROP_PKT_INFO);
	printk("%s: RMU0 no buf drop ldpid=%d, eqid=%d\n", __func__, rmu0_no_buf_drop.bf.ldpid, rmu0_no_buf_drop.bf.eqid);

	rmu0_fe_drop.wrd = CA_NE_REG_READ(QM_QM_RMU0_FE_DROP_PKT_CNTR);
	printk("%s: RMU0 FE drop count=%d\n", __func__, rmu0_fe_drop.bf.cntr);

	rmu0_rx_error_drop.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_ERROR_DROP_PKT_CNTR);
	printk("%s: RMU0 RX error drop count=%d\n", __func__, rmu0_rx_error_drop.bf.cntr);

	rx_crc_error.wrd = CA_NE_REG_READ(QM_QM_TX_CRC_ERROR_NI_CNTR);
	printk("%s: TX CRC error NI count=%d\n", __func__, rx_crc_error.bf.cntr);
#endif

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_get_error_drop_cntr);

ca_status_t aal_l3qm_get_tx_rx_cntr(ca_device_id_t device_id)
{
//	int i;
	QM_QM_TX_PKT_CNTR_t  tx_pkt_cntr;
	//QM_QM_TX_PKT_CNTR_ALL_NI_t	tx_pkt_cntr_all_ni;
	//QM_QM_TX_PKT_CNTR_CPU0_t		tx_pkt_cntr_cpu;
	QM_QM_RMU0_RX_PKT_CNTR_t		rmu0_rx_pkt_cntr;

	tx_pkt_cntr.wrd = CA_NE_REG_READ(QM_QM_TX_PKT_CNTR);
	printk("%s: packtes transmitted from L3QM count=%d\n", __func__, tx_pkt_cntr.bf.cntr);

#if 0
	tx_pkt_cntr_all_ni.wrd = CA_NE_REG_READ(QM_QM_TX_PKT_CNTR_ALL_NI);
	printk("%s: packtes transmitted from L3QM to NI ports count=%d\n", __func__, tx_pkt_cntr_all_ni.bf.cntr);

	for (i = 0; i < QM_QM_TX_PKT_CNTR_CPU0_COUNT; i++) {
		tx_pkt_cntr_cpu.wrd = CA_NE_REG_READ(QM_QM_TX_PKT_CNTR_CPU0 + i * QM_QM_TX_PKT_CNTR_CPU0_STRIDE);
		printk("%s: packtes transmitted from L3QM to CPU port %d count=%d\n", __func__, i, tx_pkt_cntr_cpu.bf.cntr);
	}
#endif

	rmu0_rx_pkt_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_PKT_CNTR);
	printk("%s: packets received by L3QM count=%d\n", __func__, rmu0_rx_pkt_cntr.bf.cntr);

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_get_tx_rx_cntr);

ca_status_t aal_l3qm_get_monitor_eq(ca_device_id_t device_id)
{
	QM_QM_MONITOR0_EQ0_t monitor0_eq0;
	QM_QM_MONITOR1_EQ0_t monitor1_eq0;
	QM_QM_MONITOR2_EQ0_t monitor2_eq0;
#if 0
	QM_QM_MONITOR0_EQ1_t monitor0_eq1;
	QM_QM_MONITOR1_EQ1_t monitor1_eq1;
	QM_QM_MONITOR2_EQ1_t monitor2_eq1;
        QM_QM_MONITOR0_EQ2_t monitor0_eq2;
        QM_QM_MONITOR1_EQ2_t monitor1_eq2;
        QM_QM_MONITOR2_EQ2_t monitor2_eq2;
#endif

	monitor0_eq0.wrd = CA_NE_REG_READ(QM_QM_MONITOR0_EQ0);
	printk("%s: Monitor0 EQ0, head=%d, tail=%d\n", __func__, monitor0_eq0.bf.head, monitor0_eq0.bf.tail);

	monitor1_eq0.wrd = CA_NE_REG_READ(QM_QM_MONITOR1_EQ0);
	printk("%s: Monitor1 EQ0, inactive_head=%d, cntr=%d\n", __func__, monitor1_eq0.bf.inactive_head, monitor1_eq0.bf.cntr);

	monitor2_eq0.wrd = CA_NE_REG_READ(QM_QM_MONITOR2_EQ0);
	printk("%s: Monitor2 EQ0, inactive_tail=%d, inactive_cntr=%d\n", __func__, monitor2_eq0.bf.inactive_tail, monitor2_eq0.bf.inactive_cntr);

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_get_monitor_eq);

ca_status_t aal_l3qm_dump_epps(ca_device_id_t device_id)
{
	int i, j;
	ca_uint32_t reg_off;
	QM_QM_CPU_EPP0_CFG_t cpu_epp0_cfg;
	QM_QM_CPU_EPP_FIFO_CFG_profile0_t cpu_epp_fifo_cfg_profile;
	QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_t cpu_epp64_fifo_paddr_start;

/* moved to aal_l3qm_cpu256.c */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
	QM_QM_CPUEPP_POINTER_CPUEPP256_FIFO_PADDR_START_t cpu_epp256_fifo_paddr_start;
	QM_QM_CPU_EPP_FIFO_256_CFG_t    cpu256_epp_cfg;
#endif
	QM_QM_CPU_EPP_FIFO0_0_CFG_t cpu_epp_fifo_cfg;

	/* CPU EPP Port Configuration Register */
	/* Select per-destionation VOQ-to-EPP-FIFO-Ring map-mode */
	printk("=== QM_QM_CPU_EPP0_CFG ===\n");
	for (i = 0; i < CA_NI_TOTAL_CPU_PORT; i++) {
		reg_off = QM_QM_CPU_EPP0_CFG + (QM_QM_CPU_EPP0_CFG_STRIDE * i);
		cpu_epp0_cfg.wrd = CA_NE_REG_READ(reg_off);
		printk("index=%d, reg_off=0x%x, map_mode=%d\n", i, reg_off, cpu_epp0_cfg.bf.map_mode);
	}

	/* CPU EPP FIFO Configuration Register */
	printk("=== QM_QM_CPU_EPP_FIFO_CFG_profile0 ===\n");
	for (i = 0; i < QM_QM_CPU_EPP_FIFO_CFG_profile0_COUNT; i++) {
		reg_off = QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * i);
		cpu_epp_fifo_cfg_profile.wrd = CA_NE_REG_READ(reg_off);
		printk("profile id=%d, reg_off=0x%x, size=%d\n", i, reg_off, cpu_epp_fifo_cfg_profile.bf.size);
	}

        printk("=== QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START/QM_QM_CPU_EPP_FIFO0_0_CFG ===\n");
	for (j = 0; j < QM_QM_CPU_EPP_FIFO0_0_CFG_COUNT; j++) {
		for (i = 0; i < l3qm_cpu_voq_per_port; i++) {

                        reg_off = QM_QM_CPU_EPP_FIFO0_0_CFG + (QM_QM_CPU_EPP_FIFO0_0_CFG_STRIDE * j) + (4 * i);
                        cpu_epp_fifo_cfg.wrd = CA_NE_REG_READ(reg_off);

			reg_off = QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE) +
				(j * l3qm_cpu_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
			cpu_epp64_fifo_paddr_start.wrd = CA_NE_REG_READ(reg_off);

			printk("port id=%d, voq id=%d, reg_off=0x%x, paddr_start=0x%x, profile sel=%d\n", j, i, reg_off,
					cpu_epp64_fifo_paddr_start.bf.phy_addr, cpu_epp_fifo_cfg.bf.profile_sel);
		}
	}

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
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
#else
 #if defined(CONFIG_NE_CPU256)
	if (support_cpu256_in_linux)
	aal_l3qm_dump_cpu256_epps();
 #endif
#endif
#endif

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_dump_epps);

ca_status_t aal_l3qm_dump_empty_buffers(ca_device_id_t device_id)
{
	int i;
	volatile ca_uint32_t reg_off;
	QM_QM_EQ_PROFILE0_t eq_profile;
	QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
	QM_QM_CFG0_EQ0_t cfg0_eq;
	QM_QM_CFG1_EQ0_t cfg1_eq;
	QM_QM_CFG2_EQ0_t cfg2_eq;
	QM_QM_CFG3_EQ0_t cfg3_eq;
	QM_QM_CFG4_EQ0_t cfg4_eq;

	printk("=== QM_QM_EQ_PROFILE0 ===\n");
	for (i = 0; i < QM_QM_EQ_PROFILE0_COUNT; i++) {
		reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * i);
		eq_profile.wrd = CA_NE_REG_READ(reg_off);
		printk("profile id=%d, reg_off=0x%x, rule=%d, eqp0=%d, eqp1=%d\n", i, reg_off, eq_profile.bf.rule, eq_profile.bf.eqp0, eq_profile.bf.eqp1);
	}

	printk(" === QM_QM_DEST_PORT0_EQ_CFG ===\n");
	for (i = 0; i < QM_QM_DEST_PORT0_EQ_CFG_COUNT; i++) {
		reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * i);
		dest_port_eq_cfg.wrd = CA_NE_REG_READ(reg_off);
		printk("port id=%d, reg_off=0x%x, profile_sel=%d\n", i, reg_off, dest_port_eq_cfg.bf.profile_sel);
	}

	printk("=== QM_QM_CFG0_EQ0 ===\n");
	for (i = 0; i < QM_QM_CFG0_EQ0_COUNT; i++) {
		reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * i);
		cfg0_eq.wrd = CA_NE_REG_READ(reg_off);
		printk("index=%d, reg_off=0x%x, phy_addr_start=0x%x, eq_en=%d\n", i, reg_off, cfg0_eq.bf.phy_addr_start << CA_L3QM_PHY_ADDR_SHIFT, cfg0_eq.bf.eq_en);
	}

	printk("=== QM_QM_CFG1_EQ0 ===\n");
	for (i = 0; i < QM_QM_CFG1_EQ0_COUNT; i++) {
		reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * i);
		cfg1_eq.wrd = CA_NE_REG_READ(reg_off);
		printk("index=%d, reg_off=0x%x, total_buffer_num=%d, bid_start=%d\n", i, reg_off, cfg1_eq.bf.total_buffer_num, cfg1_eq.bf.bid_start);
	}

	printk("=== QM_QM_CFG2_EQ0 ===\n");
	for (i = 0; i < QM_QM_CFG2_EQ0_COUNT; i++) {
		reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
		cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
		printk("index=%d, reg_off=0x%x, refill_ths=%d, refill_en=%d, refill_fbm_eqid=%d, cpu_eq=%d, buffer_size=%d\n",
				i, reg_off, cfg2_eq.bf.refill_ths, cfg2_eq.bf.refill_en, cfg2_eq.bf.refill_fbm_eqid, cfg2_eq.bf.cpu_eq, cfg2_eq.bf.buffer_size);
	}

	printk("=== QM_QM_CFG3_EQ0 ===\n");
	for (i = 0; i < QM_QM_CFG3_EQ0_COUNT; i++) {
		reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * i);
		cfg3_eq.wrd = CA_NE_REG_READ(reg_off);
		printk("index=%d, reg_off=0x%x, cfg3_eq.wrd=0x%x\n", i, reg_off, cfg3_eq.wrd);
		printk("qos=%d, cache=%d, snoop=%d, bar=%d, domain=%d, user=%d, acd_cmd=%d, cache_line_split=%d, cache_eos=%d\n",
			cfg3_eq.bf.qos, cfg3_eq.bf.cache, cfg3_eq.bf.snoop, cfg3_eq.bf.bar, cfg3_eq.bf.domain, cfg3_eq.bf.user,
			cfg3_eq.bf.acd_cmd, cfg3_eq.bf.cache_line_split, cfg3_eq.bf.cache_eos);
	}

	printk("=== QM_QM_CFG4_EQ0 ===\n");
	for (i = 0; i < QM_QM_CFG4_EQ0_COUNT; i++) {
		reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * i);
		cfg4_eq.wrd = CA_NE_REG_READ(reg_off);
		printk("index=%d, reg_off=0x%x, cfg4_eq.wrd=0x%x\n", i, reg_off, cfg4_eq.wrd);
	}
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_dump_empty_buffers);

ca_status_t aal_l3qm_dump_empty_buffers_phy_addr(ca_device_id_t device_id)
{
	int i;
	ca_uint32_t	phy_addr;

	printk("=== QM_QM_EQ_RETURN_PHY_ADDR ===\n");
	printk("EQ=%d\n", l3qm_eq_profile_cpu_pool0_eq_id);
	aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu_pool0_eq_id, 1);
	i=0;
	while ((phy_addr = aal_l3qm_get_return_phy_addr(l3qm_eq_profile_cpu_pool0_eq_id)) != 0) {
		printk("[%d] phy_addr=0x%x\n", i, phy_addr);
		i++;
	}
	aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu_pool0_eq_id, 0);

        printk("EQ=%d\n", l3qm_eq_profile_cpu_pool1_eq_id);
        aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu_pool1_eq_id, 1);
        i=0;
        while ((phy_addr = aal_l3qm_get_return_phy_addr(l3qm_eq_profile_cpu_pool1_eq_id)) != 0) {
                printk("[%d] phy_addr=0x%x\n", i, phy_addr);
                i++;
        }
        aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu_pool1_eq_id, 0);

/* moved to aal_l3qm_cpu256.c */
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
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
#else
 #if defined(CONFIG_NE_CPU256)
	if (support_cpu256_in_linux)
	aal_l3qm_dump_cpu256_empty_buffers_phy_addr();
 #endif
#endif
#endif

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_dump_empty_buffers_phy_addr);

ca_status_t aal_l3qm_dump_rmu_fe_drop_counter(ca_device_id_t device_id)
{
	int addr;
	QM_QM_RMU0_FE_DROP_PKT_CNTR_t	drop_cntr;
	QM_QM_RMU0_CTRL_t		ctrl;
	QM_QM_INT_SRC_t			int_src;
	QM_QM_RMU0_RX_PKT_CNTR_t	rx_pkt_cntr;
	QM_QM_RMU0_RX_PACKET_HEADER_INFO1_t	header_info1;
	QM_QM_RMU0_RX_PACKET_HEADER_INFO0_t	header_info0;
	QM_QM_RMU0_NO_BUF_DROP_PKT_INFO_t	no_buf_drop_info;
	QM_QM_RMU0_NO_BUF_DROP_PKT_CNTR_t	no_buf_drop_cntr;
	QM_QM_RMU0_FE_DROP_PKT_CNTR_t		fe_drop_cntr;
	QM_QM_RMU0_FIFO_STS_t			fifo_sts;
	QM_QM_VOQ_INFO_MEMORY_DATA0_t		voq_data0;
	QM_QM_VOQ_INFO_MEMORY_DATA1_t		voq_data1;
	QM_QM_VOQ_INFO_MEMORY_DATA2_t		voq_data2;
	QM_QM_HEADER_MEMORY_DATA0_t		header_data0;
	QM_QM_HEADER_MEMORY_DATA1_t		header_data1;
	HEADER_A_T header_a;

	printk("=== QM_QM_RMU0_FE_DROP_PKT_CNTR ===\n");
	drop_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_FE_DROP_PKT_CNTR);
	printk("cntr=%d\n", drop_cntr.wrd);

	printk("=== QM_QM_RMU0_CTRL ===\n");
	ctrl.wrd = CA_NE_REG_READ(QM_QM_RMU0_CTRL);
	printk("voq_drop_en=%d, rx_en=%d\n", ctrl.bf.voq_drop_en, ctrl.bf.rx_en);

	printk("=== QM_QM_INT_SRC ===\n");
	int_src.wrd = CA_NE_REG_READ(QM_QM_INT_SRC);
	printk("rmu0_no_buffer_drop_int_src=%d, rmu0_check_error_int_src=%d, rmu0_fifo_error_int_src=%d, pkt_lenght_error_int_src=%d, dqm_fe_drop_int_src=%d\n",
			int_src.bf.rmu0_no_buffer_drop_int_src, int_src.bf.rmu0_check_error_int_src, int_src.bf.rmu0_fifo_error_int_src,
			int_src.bf.pkt_lenght_error_int_src, int_src.bf.dqm_fe_drop_int_src);

	printk("=== QM_QM_RMU0_RX_PKT_CNTR ===\n");
	rx_pkt_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_PKT_CNTR);
	printk("cntr=%d\n", rx_pkt_cntr.bf.cntr);

	printk("=== QM_QM_RMU0_RX_PACKET_HEADER_INFO ===\n");
	header_info1.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_PACKET_HEADER_INFO1);
	header_info0.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_PACKET_HEADER_INFO0);
	printk("header_info1.wrd=0x%x, header_info0.wrd=0x%x\n", header_info1.wrd, header_info0.wrd);

	header_a.bits64 = ((ca_uint64_t)header_info1.wrd << 32) | header_info0.wrd;
	printk("ldpid=%d, lspid=%d, pkt_size=%d, fe_bypass=%d, hdr_type=%d\n",
		header_a.bits.ldpid, header_a.bits.lspid, header_a.bits.pkt_size, header_a.bits.fe_bypass, header_a.bits.hdr_type);

        printk("=== QM_QM_RMU0_NO_BUF_DROP_PKT_INFO ===\n");
	no_buf_drop_info.wrd = CA_NE_REG_READ(QM_QM_RMU0_NO_BUF_DROP_PKT_INFO);
	printk("eqid=%d, voqid=%d\n", no_buf_drop_info.bf.eqid, no_buf_drop_info.bf.voqid);

        printk("=== QM_QM_RMU0_NO_BUF_DROP_PKT_CNTR ===\n");
	no_buf_drop_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_NO_BUF_DROP_PKT_CNTR);
	printk("cntr=%d\n", no_buf_drop_cntr.bf.cntr);

        printk("=== QM_QM_RMU0_FE_DROP_PKT_CNTR ===\n");
	fe_drop_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_FE_DROP_PKT_CNTR);
	printk("cntr=%d\n", fe_drop_cntr.bf.cntr);

        printk("=== QM_QM_RMU0_FIFO_STS ===\n");
	fifo_sts.wrd = CA_NE_REG_READ(QM_QM_RMU0_FIFO_STS);
	//printk("entry=%d, pkt=%d\n", fifo_sts.bf.entry, fifo_sts.bf.pkt);
	printk("data_fifo_depth=%d, header_fifo_depth=%d, cmd_fifo_depth=%d, cmd_resp_fifo_depth=%d, axi_fifo_depth=%d\n",
		fifo_sts.bf.data_fifo_depth, fifo_sts.bf.header_fifo_depth, fifo_sts.bf.cmd_fifo_depth, fifo_sts.bf.cmd_resp_fifo_depth, fifo_sts.bf.axi_fifo_depth);

        printk("=== QM_QM_VOQ_INFO_MEMORY_DATA0 ===\n");
	for (addr = 0; addr < 64; addr++) {
		aal_l3qm_get_voq_info_mem(0, addr, &voq_data0, &voq_data1, &voq_data2);
	}

        printk("=== QM_QM_HEADER_MEMORY_DATA0 ===\n");
	for (addr = 0; addr < 64; addr++) {
		aal_l3qm_get_header_mem(0, addr, &header_data0, &header_data1);
	}

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_dump_rmu_fe_drop_counter);

ca_uint32_t aal_l3qm_dump_cpu64_fifo_read_ptr(ca_device_id_t device_id)
{
	int i;
	QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_t      rdptr;

	printk("=== QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR ===\n");
	for (i = 0; i < QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_COUNT; i++) {
		rdptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR + i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_STRIDE);
		printk("[%d] rdptr=%d\n", i, rdptr.bf.rptr);
	}
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_dump_cpu64_fifo_read_ptr);

ca_uint32_t aal_l3qm_dump_cpu64_fifo_write_ptr(ca_device_id_t device_id)
{
	int i;
	QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_t      wrptr;

	printk("=== QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR ===\n");
	for (i = 0; i < QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_COUNT; i++) {
		wrptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR + i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_STRIDE);
		printk("[%d] wrptr=%d\n", i, wrptr.bf.wptr);
	}
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_dump_cpu64_fifo_write_ptr);

ca_uint32_t aal_l3qm_dump_cpu64_fifo_paddr_start(ca_device_id_t device_id)
{
	int i;
	QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_t      paddr_start;

	printk("=== QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START ===\n");
	for (i = 0; i < QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_COUNT; i++) {
		paddr_start.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
		printk("[%d] phy_addr=0x%x\n", i, paddr_start.bf.phy_addr);
	}
	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_dump_cpu64_fifo_paddr_start);

ca_uint32_t aal_l3qm_dump_monitor_eq(ca_device_id_t device_id)
{
	int i;
	QM_QM_MONITOR0_EQ0_t  monitor0_eq;
	QM_QM_MONITOR1_EQ0_t  monitor1_eq;
	QM_QM_MONITOR2_EQ0_t  monitor2_eq;

	printk("=== QM_QM_MONITOR0_EQ0 ===\n");
	for (i = 0; i < QM_QM_MONITOR0_EQ0_COUNT; i++) {
		monitor0_eq.wrd = CA_NE_REG_READ(QM_QM_MONITOR0_EQ0 + i * QM_QM_MONITOR0_EQ0_STRIDE);
		printk("[%d] head=0x%x, tail=0x%x\n", i, monitor0_eq.bf.head, monitor0_eq.bf.tail);
	}

        printk("=== QM_QM_MONITOR1_EQ1 ===\n");
        for (i = 0; i < QM_QM_MONITOR1_EQ0_COUNT; i++) {
                monitor1_eq.wrd = CA_NE_REG_READ(QM_QM_MONITOR1_EQ0 + i * QM_QM_MONITOR1_EQ0_STRIDE);
                printk("[%d] inactive_head=0x%x, cntr=0x%x\n", i, monitor1_eq.bf.inactive_head, monitor1_eq.bf.cntr);
        }

        printk("=== QM_QM_MONITOR2_EQ1 ===\n");
        for (i = 0; i < QM_QM_MONITOR2_EQ0_COUNT; i++) {
                monitor2_eq.wrd = CA_NE_REG_READ(QM_QM_MONITOR2_EQ0 + i * QM_QM_MONITOR2_EQ0_STRIDE);
                printk("[%d] inactive_cntr=0x%x, inactive_tail=0x%x\n", i, monitor2_eq.bf.inactive_cntr, monitor2_eq.bf.inactive_tail);
        }

	return CA_E_OK;
}
EXPORT_SYMBOL(aal_l3qm_dump_monitor_eq);

static void aal_l3qm_dump_axi_attrib(int entry)
{
	int i;
        QM_QM_AXI_ATTRIBUTE_ACCESS_t    attrib;
        QM_QM_AXI_ATTRIBUTE_DATA0_t   attrib_data0;
        QM_QM_AXI_ATTRIBUTE_DATA1_t   attrib_data1;

	attrib.wrd = 0;
	attrib.bf.access = 1;
	attrib.bf.rbw = 0;
	attrib.bf.ADDR = entry;
	CA_NE_REG_WRITE(attrib.wrd, QM_QM_AXI_ATTRIBUTE_ACCESS);
	for (i = 0; i < CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT; i++) {
		attrib.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_ACCESS);
		if (!attrib.bf.access)
			break;
	}
	if (i == CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT) {
		printk("%s: CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT at entry=%d!!!\n", __func__, entry);
		return;
	}

        attrib_data0.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_DATA0);
        attrib_data1.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_DATA1);

	printk("[%d] data0=0x%x, data1=0x%x\n", entry, attrib_data0.wrd, attrib_data1.wrd);
	printk("     qos=%d, cache=%d, snoop=%d, bar=%d, domain=%d, user=%d, prot=%d\n", attrib_data0.bf.qos, attrib_data0.bf.cache, attrib_data0.bf.snoop,
		attrib_data0.bf.bar, attrib_data0.bf.domain, attrib_data0.bf.user, attrib_data0.bf.prot);

	printk("     axi_top_bit=0x%x, ace_cmd=%d\n", attrib_data0.bf.axi_top_bit | (attrib_data1.bf.axi_top_bit << 6), attrib_data0.bf.ace_cmd);
}

ca_status_t aal_l3qm_dump_axi_attribs(ca_device_id_t device_id)
{
	int i;

	printk("=== EQ 0-15 AXI ATTRIBUTE ===\n");

	for (i = CA_L3QM_AXI_ATTRIBUTE_EQ_BASE; i < CA_L3QM_AXI_ATTRIBUTE_EQ_BASE+16; i++) {
		aal_l3qm_dump_axi_attrib(i);
	}

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
        printk("=== CPUEPP-256 port 0-31 AXI ATTRIBUTE ===\n");

        for (i = CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE; i < CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE+32; i++) {
                aal_l3qm_dump_axi_attrib(i);
        }

        printk("=== CPUEPP port 0-7 AXI ATTRIBUTE ===\n");

        for (i = CA_L3QM_AXI_ATTRIBUTE_CPU_BASE; i < CA_L3QM_AXI_ATTRIBUTE_CPU_BASE+8; i++) {
                aal_l3qm_dump_axi_attrib(i);
        }
#else
	printk("=== CPUEPP port 0-7 AXI ATTRIBUTE ===\n");

        for (i = CA_L3QM_AXI_ATTRIBUTE_CPU_BASE; i < CA_L3QM_AXI_ATTRIBUTE_CPU_BASE+8; i++) {
                aal_l3qm_dump_axi_attrib(i);
        }
#endif

	return CA_E_OK;
}

EXPORT_SYMBOL(aal_l3qm_dump_axi_attrib);


ca_uint32_t aal_saturn_l3qm_check_init_done(void)
{
        int i;
        QM_QM_PHY_PORT_STS_t    init_done;

	printk("%s: ==========================Enter=======================\n", __func__);
        /* Check initializtion of NI block is done */
        for (i = 0; i < CA_L3QM_INIT_DONE_LOOP; i++) {
                init_done.wrd = CA_DEV_NE_REG_READ(1, QM_QM_PHY_PORT_STS);
		printk("%s: init_done.wrd=0x%x\n", __func__, init_done.wrd);
                if (init_done.bf.qm_init_done)
                        break;
        }
        if (i == CA_L3QM_INIT_DONE_LOOP) {
                printk("%s: initializtion of QM block is NOT done!!!\n", __func__);
                return -1;
        }
        return 0;
}

void aal_saturn_l3qm_enable_rx(ca_uint8_t enable)
{
        QM_QM_RMU0_CTRL_t rmu0_ctrl;

        rmu0_ctrl.wrd = CA_DEV_NE_REG_READ(1, QM_QM_RMU0_CTRL);
        if (enable) {
                rmu0_ctrl.bf.rx_en = 1;
        }
        else {
                rmu0_ctrl.bf.rx_en = 0;
        }
        CA_DEV_NE_REG_WRITE(1, rmu0_ctrl.wrd, QM_QM_RMU0_CTRL);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                printk("%s: enable=%d\n", __func__, enable);
        }
	rmu0_ctrl.wrd = CA_DEV_NE_REG_READ(1, QM_QM_RMU0_CTRL);
	printk("%s: rmu0_ctrl.wrd=0x%x, enable=%d\n", __func__, rmu0_ctrl.wrd, enable);
}

static void aal_saturn_l3qm_init_empty_buffer_DQ(void)
{
        int i;
        ca_uint32_t reg_off;
        QM_QM_EQ_PROFILE0_t eq_profile;
        QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
        QM_QM_CFG0_EQ0_t cfg0_eq;
        QM_QM_CFG1_EQ0_t cfg1_eq;
        QM_QM_CFG2_EQ0_t cfg2_eq;
        QM_QM_CFG2_EQ0_t cfg3_eq;
        QM_QM_CFG4_EQ0_t cfg4_eq;
	static int saturn_l3qm_eq_profile_dq_id;
	static int saturn_l3qm_eq_profile_dq_pool0_eq_id;
	static int saturn_l3qm_eq_profile_dq_pool1_eq_id;
	static int saturn_l3qm_eq_profile_dq_pool0_buf_count;
	static int saturn_l3qm_eq_profile_dq_pool1_buf_count;
	static int saturn_l3qm_eq_profile_dq_pool0_bid_start;
	static int saturn_l3qm_eq_profile_dq_pool1_bid_start;
	static int saturn_l3qm_eq_profile_dq_pool0_buf_sz;
	static int saturn_l3qm_eq_profile_dq_pool1_buf_sz;
	
	saturn_l3qm_eq_profile_dq_id = 2;
	saturn_l3qm_eq_profile_dq_pool0_eq_id = 14;
	saturn_l3qm_eq_profile_dq_pool1_eq_id = 15;
	saturn_l3qm_eq_profile_dq_pool0_buf_count = 1872;
	saturn_l3qm_eq_profile_dq_pool1_buf_count = 1872;
	saturn_l3qm_eq_profile_dq_pool0_bid_start = 0;
	saturn_l3qm_eq_profile_dq_pool1_bid_start = 2048;
	saturn_l3qm_eq_profile_dq_pool0_buf_sz = 128;
	saturn_l3qm_eq_profile_dq_pool1_buf_sz = 128;

        /* one profile for DQ or normal NI ports */
        eq_profile.wrd = 0;
        eq_profile.bf.eqp0 = saturn_l3qm_eq_profile_dq_pool0_eq_id;
        eq_profile.bf.eqp1 = saturn_l3qm_eq_profile_dq_pool1_eq_id;
        eq_profile.bf.rule = 0;
        CA_DEV_NE_REG_WRITE(1, eq_profile.wrd, QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * saturn_l3qm_eq_profile_dq_id));

	eq_profile.wrd = CA_DEV_NE_REG_READ(1, QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * saturn_l3qm_eq_profile_dq_id));
	printk("%s: eq_profile.wrd=0x%x\n", __func__, eq_profile.wrd);
	
        /* for CPU ports profile select, profile 0 for DQ at NI ports  */
        dest_port_eq_cfg.wrd = 0;
        for (i = 0; i < CA_NI_TOTAL_NI_PORT; i++) {
                dest_port_eq_cfg.bf.profile_sel = saturn_l3qm_eq_profile_dq_id;
		CA_DEV_NE_REG_WRITE(1, dest_port_eq_cfg.wrd,  QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * (i + CA_NI_TOTAL_CPU_PORT)));
		dest_port_eq_cfg.wrd = CA_DEV_NE_REG_READ(1, QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * (i + CA_NI_TOTAL_CPU_PORT)));	
		printk("%s: dest_port_eq_cfg.wrd=%d\n", __func__, dest_port_eq_cfg.wrd);
        }

        /* configure EB pool 0 for DQ */
        /* set phy_addr_start and enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        cfg0_eq.bf.phy_addr_start = (0x40000000) >> CA_L3QM_PHY_ADDR_SHIFT;
	CA_DEV_NE_REG_WRITE(1, cfg0_eq.wrd, QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool0_eq_id));
	cfg0_eq.wrd = CA_DEV_NE_REG_READ(1, QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool0_eq_id));
	printk("%s: pool 0, cfg0_eq.wrd=0x%x\n", __func__, cfg0_eq.wrd);
	
        /* configure total buffer number and bid_start */
        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = saturn_l3qm_eq_profile_dq_pool0_buf_count;
        cfg1_eq.bf.bid_start = saturn_l3qm_eq_profile_dq_pool0_bid_start;
	CA_DEV_NE_REG_WRITE(1, cfg1_eq.wrd, QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool0_eq_id));
	cfg1_eq.wrd = CA_DEV_NE_REG_READ(1, QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool0_eq_id));
	printk("%s: pool 0, cfg1_eq.wrd=0x%x\n", __func__, cfg1_eq.wrd);
	
        /* configure buffer size for Empty Buffer */
        cfg2_eq.wrd = CA_DEV_NE_REG_READ(1, QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool0_eq_id));
        cfg2_eq.bf.cpu_eq = 0;
        cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(saturn_l3qm_eq_profile_dq_pool0_buf_sz);      /* 128 bytes */
        cfg2_eq.bf.refill_en = 0;
        CA_DEV_NE_REG_WRITE(1, cfg2_eq.wrd, QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool0_eq_id));
	cfg2_eq.wrd = CA_DEV_NE_REG_READ(1, QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool0_eq_id));
	printk("%s: pool 0, cfg2_eq.wrd=0x%x\n", __func__, cfg2_eq.wrd);

        /* configure EB pool 1 for DQ */
        /* set phy_addr_start and enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        cfg0_eq.bf.phy_addr_start = (0x4003A800) >> CA_L3QM_PHY_ADDR_SHIFT;
        CA_DEV_NE_REG_WRITE(1, cfg0_eq.wrd, QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool1_eq_id));
        cfg0_eq.wrd = CA_DEV_NE_REG_READ(1, QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool1_eq_id));
        printk("%s: pool 1, cfg0_eq.wrd=0x%x\n", __func__, cfg0_eq.wrd);

        /* configure total buffer number and bid_start */
        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = saturn_l3qm_eq_profile_dq_pool1_buf_count;
        cfg1_eq.bf.bid_start = saturn_l3qm_eq_profile_dq_pool1_bid_start;
        CA_DEV_NE_REG_WRITE(1, cfg1_eq.wrd, QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool1_eq_id));
        cfg1_eq.wrd = CA_DEV_NE_REG_READ(1, QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool1_eq_id));
        printk("%s: pool 1, cfg1_eq.wrd=0x%x\n", __func__, cfg1_eq.wrd);

        /* configure buffer size for Empty Buffer */
        cfg2_eq.wrd = CA_DEV_NE_REG_READ(1, QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool1_eq_id));
        cfg2_eq.bf.cpu_eq = 0;
        cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(saturn_l3qm_eq_profile_dq_pool1_buf_sz);      /* 128 bytes */
        cfg2_eq.bf.refill_en = 0;
        CA_DEV_NE_REG_WRITE(1, cfg2_eq.wrd, QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool1_eq_id));
        cfg2_eq.wrd = CA_DEV_NE_REG_READ(1, QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * saturn_l3qm_eq_profile_dq_pool1_eq_id));
        printk("%s: pool 1, cfg2_eq.wrd=0x%x\n", __func__, cfg2_eq.wrd);
}

static void aal_saturn_l3qm_init_voq(void)
{
        int i;
        QM_QM_SCH_CFG0_t sch_cfg;

        sch_cfg.wrd = CA_DEV_NE_REG_READ(1, QM_QM_SCH_CFG0);
        for (i = 0; i < QM_QM_SCH_CFG0_COUNT; i++) {

                sch_cfg.bf.voq_en = 0xff;
                CA_DEV_NE_REG_WRITE(1, sch_cfg.wrd, QM_QM_SCH_CFG0 + (i * QM_QM_SCH_CFG0_STRIDE));
		sch_cfg.wrd = CA_DEV_NE_REG_READ(1, QM_QM_SCH_CFG0);
		printk("%s: [%d] sch_cfg.wrd=0x%x\n", __func__, i, sch_cfg.wrd);
        }
}

static void aal_saturn_l3qm_enable_tx(ca_uint8_t enable)
{
        QM_QM_ES_CTRL_t es_ctrl;

        es_ctrl.wrd = CA_DEV_NE_REG_READ(1, QM_QM_ES_CTRL);
        if (enable) {
                es_ctrl.bf.tx_en = 1;
                //es_ctrl.bf.cpu_en = 0xff;
                es_ctrl.bf.cpu_en = 0;
                es_ctrl.bf.ni_en = 0xff;
        }
        else {
                es_ctrl.bf.tx_en = 0;
                es_ctrl.bf.cpu_en = 0;
                es_ctrl.bf.ni_en = 0;
        }
        CA_DEV_NE_REG_WRITE(1, es_ctrl.wrd, QM_QM_ES_CTRL);

	es_ctrl.wrd = CA_DEV_NE_REG_READ(1, QM_QM_ES_CTRL);
	printk("%s: enable=%d, es_ctrl.wrd=0x%x\n", __func__, enable, es_ctrl.wrd);

}

static void aal_saturn_l3qm_load_eq_config(void)
{
	/* load the configuration */
	CA_DEV_NE_REG_WRITE(1, 0x05102013, QM_QM_HDM_WRITE_PROTECTION);
	CA_DEV_NE_REG_WRITE(1, 0xffff, QM_QM_EQ_CFG_LOAD);
	CA_DEV_NE_REG_WRITE(1, 0, QM_QM_EQ_CFG_LOAD);
	CA_DEV_NE_REG_WRITE(1, 0, QM_QM_HDM_WRITE_PROTECTION);

	printk("%s: Load EQ configuration....\n", __func__);
}

//re-configure DeepQ
static void aal_l3qm_init_DQ_pools(int profile_index, ca_uint32_t sram_phy_start, ca_uint32_t ddr_phy_start, ca_uint32_t mode);
static void aal_l3qm_init_empty_buffer_DQ_1(void)
{
	int i;
	ca_uint32_t reg_off;
	QM_QM_EQ_PROFILE0_t eq_profile;
	QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;

	/* l3qm_eq_share_pool_id should be read from scfg */
	l3qm_eq_share_pool_id = 1;

	/* 4 profile for DQ or normal NI ports */
	for (i = 0; i < 4; i++) {
		eq_profile.wrd = 0;
		eq_profile.bf.eqp0 = i*2;
		//eq_profile.bf.eqp1 = i*2+1;
		eq_profile.bf.eqp1 = l3qm_eq_share_pool_id;
		eq_profile.bf.rule = 0;
		reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * i);
		CA_NE_REG_WRITE(eq_profile.wrd, reg_off);
	}

	/* profile 5 for NI port 7 */
	i = 5;
	eq_profile.wrd = 0;
	eq_profile.bf.eqp0 = i*2;
	eq_profile.bf.eqp1 = l3qm_eq_share_pool_id;
	eq_profile.bf.rule = 0;
	reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * i);
	CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

	/* for CPU ports profile select, profile 0 for DQ at NI ports  */
	dest_port_eq_cfg.wrd = 0;
	for (i = 0; i < 4; i++) {
		dest_port_eq_cfg.bf.profile_sel = i;
		reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * (i + CA_NI_TOTAL_CPU_PORT));
		CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);
	}

	/* select profile 5 for NI port 7 */
	i = 5;
	dest_port_eq_cfg.bf.profile_sel = i;
	reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * (7 + CA_NI_TOTAL_CPU_PORT));
	CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);

	for (i = 0; i < 4; i++) {
		aal_l3qm_init_DQ_pools(i, 0xc0001000 + (i * EQ_POOL_SRAM_SIZE), l3qm_eq_profile_dq_pool0_buff_phy_addr + (i * EQ_POOL_DDR_SIZE), L3QM_EQ_SHARE_MODE);
	}

	/* for NI port 7, EQ pool 10, 1 */
	aal_l3qm_init_DQ_pools(i + 1, 0xc0001000 + (i * EQ_POOL_SRAM_SIZE), l3qm_eq_profile_dq_pool0_buff_phy_addr + (i * EQ_POOL_DDR_SIZE), L3QM_EQ_SHARE_MODE);
}

static void aal_l3qm_init_DQ_pools(int profile_index, ca_uint32_t sram_phy_start, ca_uint32_t ddr_phy_start, ca_uint32_t mode)
{
	int i, pool_id, buf_cnt, buf_size;
	ca_uint32_t reg_off;
	QM_QM_CFG0_EQ0_t cfg0_eq;
	QM_QM_CFG1_EQ0_t cfg1_eq;
	QM_QM_CFG2_EQ0_t cfg2_eq;
	QM_QM_CFG2_EQ0_t cfg3_eq;
	QM_QM_CFG4_EQ0_t cfg4_eq;
	static int bid_start = 3000;

	buf_cnt = 64;
	buf_size = 512;
	pool_id = profile_index * 2;

	printk("%s: sram_phy_start=0x%x, ddr_phy_start=0x%x\n", __func__, sram_phy_start, ddr_phy_start);
	printk("%s: pool_id=%d, bid_start=%d\n", __func__, pool_id, bid_start);

	/* configure EB pool 0 for DQ */
	/* set phy_addr_start and enable Empty Buffer */
	cfg0_eq.wrd = 0;
	cfg0_eq.bf.eq_en = 1;
	cfg0_eq.bf.phy_addr_start = sram_phy_start >> CA_L3QM_PHY_ADDR_SHIFT;
	reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * pool_id);
	CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

	printk("%s: cfg0_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg0_eq.wrd, reg_off);

	/* configure total buffer number and bid_start */
	cfg1_eq.wrd = 0;
	cfg1_eq.bf.total_buffer_num = buf_cnt;
	cfg1_eq.bf.bid_start = bid_start;
	reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * pool_id);
	CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

	printk("%s: cfg1_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg1_eq.wrd, reg_off);

	/* configure buffer size for Empty Buffer */
	reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * pool_id);
	cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
	cfg2_eq.bf.cpu_eq = 0;
	cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(buf_size);      /* 512 bytes */
	cfg2_eq.bf.refill_en = 0;
	CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);
	
	printk("%s: cfg2_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg2_eq.wrd, reg_off);

	/* configure cache_eos/domain/snoop/cache */
	reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * pool_id);
	cfg3_eq.wrd = 0x10;
	CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

	cfg3_eq.wrd = CA_NE_REG_READ(reg_off);
	printk("%s: cfg3_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg3_eq.wrd, reg_off);

	/* configure AXI address top 8 bits */
	reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * pool_id);
	cfg4_eq.wrd = 0;
	CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

	cfg4_eq.wrd = CA_NE_REG_READ(reg_off);
	printk("%s: cfg4_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg4_eq.wrd, reg_off);

	/* configure EB pool 1 for DQ */

	pool_id += 1;
	bid_start += buf_cnt;
	if(pool_id == 11) {
		buf_cnt = 64;
		buf_size = 512;
	} else {
		buf_cnt = l3qm_eq_profile_dq_pool1_buf_count;
		buf_size = l3qm_eq_profile_dq_pool1_buf_sz;
	}

	if((mode == L3QM_EQ_SHARE_MODE && pool_id == l3qm_eq_share_pool_id) || mode != L3QM_EQ_SHARE_MODE) {
		/* set phy_addr_start and enable Empty Buffer */
		cfg0_eq.wrd = 0;
		cfg0_eq.bf.eq_en = 1;
		cfg0_eq.bf.phy_addr_start = ddr_phy_start >> CA_L3QM_PHY_ADDR_SHIFT;
		reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * pool_id);
		CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

		printk("%s: cfg0_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg0_eq.wrd, reg_off);

		/* configure total buffer number and bid_start */
		cfg1_eq.wrd = 0;
		cfg1_eq.bf.total_buffer_num = buf_cnt;
		cfg1_eq.bf.bid_start = bid_start;
		reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * pool_id);
		CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

		printk("%s: cfg1_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg1_eq.wrd, reg_off);

		/* configure buffer size for Empty Buffer */
		reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * pool_id);
		cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
		cfg2_eq.bf.cpu_eq = 0;
			cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(buf_size);      /* 1024 bytes */
		cfg2_eq.bf.refill_en = 0;
		CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

		printk("%s: cfg2_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg2_eq.wrd, reg_off);

		/* configure cache_eos/domain/snoop/cache */
		reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * pool_id);
		cfg3_eq.wrd = 0x10;
		CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

		cfg3_eq.wrd = CA_NE_REG_READ(reg_off);
		printk("%s: cfg3_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg3_eq.wrd, reg_off);

		/* configure AXI address top 8 bits */
		reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * pool_id);
		cfg4_eq.wrd = 0;
		CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

		cfg4_eq.wrd = CA_NE_REG_READ(reg_off);
		printk("%s: cfg4_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg4_eq.wrd, reg_off);

		bid_start += buf_cnt;	
	}
}
