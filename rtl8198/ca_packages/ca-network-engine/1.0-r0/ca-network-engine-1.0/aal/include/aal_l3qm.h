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
 * aal_qm.h: Hardware Abstraction Layer for Layer-3 Queue Manager to access hardware regsiters
 *
 */
#ifndef __AAL_L3QM_H__
#define __AAL_L3QM_H__

#include "aal_table.h"
#include "aal_ni.h"

#define L3QM_CPU_PUSH_RDY_TIMEOUT		100

#if defined(CONFIG_ARCH_CORTINA_G3LITE)
#define L3QM_INACTIVE_BID_CNTR			5
#else
#define L3QM_INACTIVE_BID_CNTR			1	//default is 100
#endif

#define CA_L3QM_INIT_DONE_LOOP			1000000

/* EQ profile for CPU */
#define L3QM_EQ_PROFILE_CPU	2
#define L3QM_EQ_PROFILE_CPU_NI	7

/* Empty buffer pools for CPU */
#define L3QM_EQ_POOL_CPU_1	5
#define L3QM_EQ_POOL_CPU_2	6

/* EPP (RX FIFO) profile for CPU */
#define L3QM_EPP_PROFILE_CPU	2
#define L3QM_EPP_PROFILE_CPU_NI	7

/* EQ refill threshold */
#define L3QM_EQ_REFILL_THRESHOLD 	12

#define CA_L3QM_PHY_ADDR_SHIFT          7

enum {
	L3QM_EQ_SHARE_MODE,
	L3QM_EQ_EXCLUSIVE_MODE,
};

/* startup config parameters */
#if 0
extern ca_uint8_t      l3qm_eq_profile_cpu_id;                     /* EQ profile 2 */
extern ca_uint8_t      l3qm_eq_profile_cpu_rule;                   /* use pool 0 then pool 1 */
extern ca_uint8_t      l3qm_eq_profile_cpu_use_fbm;                /* do not use FBM pool */
extern ca_uint32_t     l3qm_eq_profile_cpu_pool0_buffer_count;     /* 2048 buffers for CPU pool 0 */
extern ca_uint32_t     l3qm_eq_profile_cpu_pool0_bid_start;        /* strating buffer id */
extern ca_uint32_t     l3qm_eq_profile_cpu_pool0_buf_sz;           /* pool 0 buffer size */
extern ca_uint8_t      l3qm_eq_profile_cpu_pool0_location;         /* DDR (pushed into FBM) */
extern ca_uint8_t      l3qm_eq_profile_cpu_pool0_fbm_pool_id;      /* SRAM */
extern ca_uint32_t     l3qm_eq_profile_cpu_pool1_buffer_count;     /* 2048 buffers for CPU pool 0 */
extern ca_uint32_t     l3qm_eq_profile_cpu_pool1_bid_start;        /* starting buffer id */
extern ca_uint32_t     l3qm_eq_profile_cpu_pool1_buf_sz;           /* pool 1 buffer size */
extern ca_uint8_t      l3qm_eq_profile_cpu_pool1_location;         /* DDR (pushed into FBM) */
extern ca_uint8_t      l3qm_eq_profile_cpu_pool1_fbm_pool_id;      /* SRAM */
extern ca_uint8_t     l3qm_eq_profile_cpu_id_ports[];             /* CPU port 0-5 */
extern ca_uint8_t      l3qm_epp_profile_cpu_id;                    /* EPP (RX FIFO) profile id */
extern ca_uint32_t     l3qm_epp_profile_cpu_sz;                    /* RX FIFO size */
//extern ca_uint32_t     l3qm_epp_profile_cpu_start_addresses[];	   /* VoQs 0-7 for port0-7 */
#endif

extern ca_uint8_t      l3qm_eq_profile_cpu_pool0_eq_id;            /* QM_EQ_PROFILE2.eqid0=5 */
extern ca_uint8_t      l3qm_eq_profile_cpu_pool1_eq_id;            /* QM_EQ_PROFILE2.eqid0=6 */
extern ca_uint8_t      l3qm_ni_p0_7_deep_q_en;
extern ca_uint8_t      l3qm_eq_profile_cpu_id_ports[];     /* CPU port 0-5 */
extern ca_uint8_t      l3qm_eq_profile_dq_id_ports[];      /* L3QM port 0-5 */

#if defined(CONFIG_NE_CPU256)
/* export for aal_l3qm_cpu256.c */
extern ca_uint8_t      l3qm_desc_size;
extern ca_uint8_t      l3qm_desc_per_epp;
extern ca_uint8_t      l3qm_magic_marker_len;
extern ca_uint32_t     l3qm_eq_cfg3_values[];
extern ca_uint32_t     l3qm_eq_cfg4_values[];
extern ca_uint32_t     l3qm_axi_attrib_eq_values[];
extern ca_uint_t       l3qm_eq_profile_pe1_pool1_buff_start_addr;
extern ca_uint32_t     l3qm_eq_buffer_align_size;
extern ca_uint32_t     l3qm_eq_profile_pe1_pool1_buff_phy_addr;
extern ca_uint_t       l3qm_eq_profile_dq_pool0_buff_start_addr;
extern ca_uint8_t      l3qm_epp_size;
#endif

typedef struct aal_l3qm_l3fe_main_hash_info {
        ca_uint_t overflow_action_start_virt_addr;
        ca_uint_t default_action_start_virt_addr;
        ca_uint_t action_cache_start_virt_addr;
        ca_uint_t hash_table_start_virt_addr;
        ca_uint_t action_table_start_virt_addr;
	ca_uint32_t overflow_action_start_phy_addr;
        ca_uint32_t default_action_start_phy_addr;
        ca_uint32_t action_cache_start_phy_addr;
        ca_uint32_t hash_table_start_phy_addr;
        ca_uint32_t action_table_start_phy_addr;
	ca_uint32_t overflow_hash_fib_length;
	ca_uint32_t overflow_hash_fib_count;
	ca_uint32_t default_hash_fib_length;
	ca_uint32_t default_hash_fib_count;
	ca_uint32_t action_cache_fib_length;
	ca_uint32_t action_cache_fib_count;
	ca_uint32_t table_entry_length;
	ca_uint32_t table_entry_count;
	ca_uint32_t action_table_entry_length;
	ca_uint32_t action_table_entry_count;
	ca_uint8_t main_hash_bm_test;
} aal_l3qm_l3fe_main_hash_info_t;

extern spinlock_t l3qm_epp64_inte_lock;

/* functions proto-type */
int aal_l3qm_get_ace_test(void);
int aal_l3qm_get_cpuepp_axi_attrib(int cpu_port);
int aal_l3qm_get_eq_pool_axi_top_8bit(int eqid);
int aal_l3qm_get_cpu_port_head_room_first(void);
int aal_l3qm_eq_get_cpu_rule(void);
int aal_l3qm_eq_get_cpu_use_fbm(void);
int aal_l3qm_eq_get_cpu_pool0_buf_size(void);

/* moved to aal_l3qm_cpu256.h */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
#if 0
int aal_l3qm_eq_get_cpu256_hi_use_fbm(void);
int aal_l3qm_eq_get_cpu256_lo_use_fbm(void);
void aal_l3qm_eq_get_cpu256_hi_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_cpu256_hi_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_cpu256_lo_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_cpu256_lo_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
int aal_l3qm_eq_get_cpu256_max_write_ptr(void);
#endif

int aal_l3qm_eq_get_desc_per_epp(void);
int aal_l3qm_eq_get_desc_size(void);
int aal_l3qm_eq_get_max_write_ptr(void);
int aal_l3qm_eq_get_cpu_pool_bid_count(void);
void aal_l3qm_eq_get_cpu_pool0_1_start_addr(ca_uint_t *cpu_pool0_start_addr, ca_uint_t *cpu_pool1_start_addr);
void aal_l3qm_eq_get_cpu_pool0_1_eq_id(ca_uint8_t *cpu_pool0_eq_id, ca_uint8_t *cpu_pool1_eq_id);
#if 0
int aal_l3qm_eq_get_pe0_use_fbm(void);
int aal_l3qm_eq_get_pe1_use_fbm(void);
void aal_l3qm_eq_get_pe0_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_pe0_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_pe1_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_pe1_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
#endif
void aal_l3qm_eq_get_cpu_pool0_info(ca_uint_t *virt_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_cpu_pool1_info(ca_uint_t *virt_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_get_l3fe_main_hash_action_info(aal_l3qm_l3fe_main_hash_info_t *l3fe_main_hash_info);
void aal_l3qm_set_l3fe_main_hash(ca_uint_t main_hash_table_saddr, ca_uint32_t main_hash_table_paddr, ca_uint_t main_hash_action_paddr, ca_uint32_t main_hash_action_saddr);
ca_uint16_t aal_l3qm_get_inactive_bid_cntr(int eqid);
void aal_l3qm_set_rx_read_ptr(ca_uint16_t cpu_port, ca_uint8_t voq, ca_uint32_t value);
ca_uint32_t aal_l3qm_get_rx_read_ptr(ca_uint16_t cpu_port, ca_uint8_t voq);
ca_uint32_t aal_l3qm_get_rx_write_ptr(ca_uint16_t cpu_port, ca_uint8_t voq, ca_uint16_t *count);
void aal_l3qm_setup_refill_threshold_interrup(void);
void aal_l3qm_enable_refill_threshold_interrupt(ca_uint16_t eqid, ca_uint8_t enable);

/* moved to aal_l3qm_cpu256.h */
//#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#if 0
void aal_l3qm_enable_cpu256_epp_interrupt(ca_uint8_t cpu256_hi, ca_uint8_t enable);
ca_uint32_t aal_l3qm_cpu256_get_rx_status(int instance);
void aal_l3qm_cpu256_set_rx_read_ptr(ca_uint16_t instance, ca_uint32_t value);
ca_uint32_t aal_l3qm_cpu256_get_rx_read_ptr(ca_uint16_t instance);
ca_uint32_t aal_l3qm_cpu256_get_rx_write_ptr(ca_uint16_t instance, ca_uint16_t *count);
ca_uint32_t aal_l3qm_cpu256_get_rx_start_addr(ca_uint16_t instance);
#endif

void aal_l3qm_enable_cpu_epp_interrupt(ca_uint16_t cpu_port, ca_uint8_t enable);
ca_uint32_t aal_l3qm_get_rx_start_addr(ca_uint16_t cpu_port, ca_uint8_t voq);
ca_uint32_t aal_l3qm_get_rx_status0(void);
ca_uint32_t aal_l3qm_get_rx_status1(void);
void aal_l3qm_load_eq_config(void);
void aal_l3qm_load_epp_config(void);
ca_uint32_t aal_l3qm_check_cpu_push_ready(int cpu_port);
void aal_l3qm_set_cpu_push_paddr(int cpu_port, int eqid, ca_uint32_t phy_addr);
void aal_l3qm_init_cpu_epp(void);
void aal_l3qm_init_empty_buffer(ca_uint8_t is_use_fbm);
void aal_l3qm_enable_rx(ca_uint8_t enable);
void aal_l3qm_enable_tx(ca_uint8_t enable);
void aal_l3qm_enable_tx_cpu(ca_uint16_t cpu_port, ca_uint8_t enable);
void aal_l3qm_config_DWRR(void);
void aal_l3qm_enable_int_src(void);
void aal_l3qm_init_startup_config(void);
void aal_l3qm_set_cpu_rx_fifo_addr(ca_uint_t cpu_rx_fifo_phy_addr, ca_uint_t cpu_rx_fifo_virt_addr);
ca_uint32_t aal_l3qm_get_cpu_rx_fifo_size(void);
void aal_l3qm_init_reserved_mem(void *sram_base, ca_uint32_t sram_size, void *sram_phy_base, void *ddr_coherent_base, void *ddr_noncache_base, void *ddr_noncache_phy_base);
void aal_l3qm_init_voq(void);
ca_uint32_t aal_l3qm_check_init_done(void);
void aal_l3qm_get_return_phy_addr_enable(int eq_id, int enable);
ca_uint32_t aal_l3qm_get_return_phy_addr(int eq_id);

/* function for testing with iROS, was moved to aal_l3qm_iros.h */
ca_status_t aal_l3qm_get_voq_info_mem(CA_IN ca_device_id_t device_id, CA_IN ca_uint16_t addr, CA_OUT QM_QM_VOQ_INFO_MEMORY_DATA0_t *data0,
					CA_OUT QM_QM_VOQ_INFO_MEMORY_DATA1_t *data1, CA_OUT QM_QM_VOQ_INFO_MEMORY_DATA2_t *data);
ca_status_t aal_l3qm_get_monitor_eq(ca_device_id_t device_id);

ca_uint32_t aal_l3qm_read_rx_interrupt_status(void);
void aal_l3qm_write_rx_interrupt_status(ca_uint32_t status);

ca_status_t aal_l3qm_check_magic_number(void *addr);
void aal_l3qm_set_epp_axi_attrib(int entry, ca_uint32_t data0);
int aal_l3qm_insert_magic_number(void *addr);
ca_uint8_t aal_l3qm_get_buffer_size_index(ca_uint32_t buffer_size);

#endif /* __AAL_L3QM_H__ */
