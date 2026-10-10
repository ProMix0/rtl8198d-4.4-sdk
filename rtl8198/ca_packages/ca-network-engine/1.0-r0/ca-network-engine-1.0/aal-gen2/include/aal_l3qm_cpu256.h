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
 * aal_qm_cpu256.h: Hardware Abstraction Layer for Layer-3 Queue Manager to access hardware regsiters
 *
 */
#ifndef __AAL_L3QM_CPU256_H__
#define __AAL_L3QM_CPU256_H__

#if defined(CONFIG_ARCH_CORTINA_VENUS)
typedef union {
        ca_uint32_t __attribute__((packed))  wrd[3];

        struct __te_tb2rate_profile_mem_t {
                ca_uint32_t committed_rate_k  : 10;    /* bit[0:9] Rate in Kilobits per second or packet per second.
                                                                   The valid value is 0~999. */
                ca_uint32_t committed_rate_m  : 16;    /* bit[25:10] committed_rate_m, Rate in 1000 Kilobits per second or 1000 packets
                                                                   per second. */
                ca_uint32_t committed_bs      : 14;    /* bit[39:26] burst size. The granularity is 256 bytes or 1 packet.
                                                                   The minimum value is roundup (committed_rate_m/512) in byte mode and roudup
                                                                   (committed_rate_m/2) in packet mode. */
                ca_uint32_t peak_rate_k       : 10;    /* bit[49:40] peak_rate_k, Rate in Kilobits per second or packet per second. The
                                                                   valid value is 0~999. */
                ca_uint32_t peak_rate_m       : 16;    /* bit[65:50] peak_rate_m, Rate in 1000 Kilobits per second or 1000 packets per second. */
                ca_uint32_t peak_bs           : 14;    /* bit[79:66], peak_bs, burst size. The granularity is 256 bytes or 1 packet. The
                                                                   minimum value is roundup (peak_rate_m/512) in byte mode and roudup
                                                                   (peak_rate_m/2) in packet mode. */
        } __attribute__((packed)) bits;
}  __attribute__((packed)) te_tb2rate_profile_mem_t;

typedef union {
        ca_uint32_t __attribute__((packed))  wrd[2];

        struct __te_tb2rate_counter_mem_t {
                ca_uint32_t committed_tbc       : 22;   /* bit[0:21] Committed token bucket count. */
                ca_uint32_t committed_tbc_sign  : 1;    /* bit[22] Sign bit, 1=positive value. 0=negative value. */
                ca_uint32_t peak_tbc            : 22;   /* bit[23:44] Peak token bucket count. */
                ca_uint32_t peak_tbc_sign       : 1;    /* bit[45] Sign bit, 1=positive value. 0=negative value. */
        } __attribute__((packed)) bits;

} __attribute__((packed)) te_tb2rate_counter_mem_t;

#endif

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)

extern ca_uint32_t     l3qm_epp_profile_cpu256_hi_total_epp_count;
extern spinlock_t l3qm_epp256_inte_lock;
extern ca_uint32_t l3qm_axi_attrib_cpu256_epp_values[];

extern ca_uint32_t     l3qm_cpu256_port_count ;
extern ca_uint8_t      l3qm_cpu256_voq_per_port;
extern ca_uint32_t     l3qm_cpu256_epp_per_voq;
extern ca_uint32_t     l3qm_cpu256_pool_bid_count;

extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_id;
extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_rule;
extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_use_fbm;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_id;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_rule;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_use_fbm;
extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool0_eq_id;
extern ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_bid_start;
extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool0_location;
extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool0_fbm_pool_id;
extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_eq_id;
extern ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_bid_start;
extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_location;
extern ca_uint8_t      l3qm_eq_profile_cpu256_hi_pool1_fbm_pool_id;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool0_eq_id;
extern ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_bid_start;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool0_location;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool0_fbm_pool_id;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool1_eq_id;
extern ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_bid_start;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool1_location;
extern ca_uint8_t      l3qm_eq_profile_cpu256_lo_pool1_fbm_pool_id;
extern ca_uint32_t     l3qm_epp_profile_cpu256_hi_map_mode;
extern ca_uint32_t     l3qm_epp_profile_cpu256_lo_map_mode;

extern ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_buf_count;
extern ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_buf_sz;
extern ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_buf_count;
extern ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_buf_sz;
extern ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_buf_count;
extern ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_buf_sz;
extern ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_buf_count;
extern ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_buf_sz;
extern ca_uint32_t     l3qm_epp_profile_cpu256_hi_sz;
extern ca_uint32_t     l3qm_epp_profile_cpu256_hi_cn;
extern ca_uint8_t      l3qm_epp_profile_cpu256_hi_id;
extern ca_uint32_t     l3qm_epp_profile_cpu256_lo_sz;
extern ca_uint32_t     l3qm_epp_profile_cpu256_lo_cn;
extern ca_uint8_t      l3qm_epp_profile_cpu256_lo_id;

extern ca_uint_t     l3qm_eq_profile_cpu256_hi_pool0_start_addr;
extern ca_uint_t     l3qm_eq_profile_cpu256_hi_pool1_start_addr;
extern ca_uint_t     l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr;
extern ca_uint_t     l3qm_epp_profile_cpu256_hi_rx_fifo_start_addr;
extern ca_uint_t     l3qm_eq_profile_cpu256_lo_pool0_start_addr;
extern ca_uint_t     l3qm_eq_profile_cpu256_lo_pool1_start_addr;
extern ca_uint_t     l3qm_epp_profile_cpu256_lo_rx_fifo_start_addr;

extern ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool0_phy_addr;
extern ca_uint32_t     l3qm_eq_profile_cpu256_hi_pool1_phy_addr;
extern ca_uint32_t     l3qm_epp_profile_cpu256_hi_rx_fifo_phy_addr;
extern ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool0_phy_addr;
extern ca_uint32_t     l3qm_eq_profile_cpu256_lo_pool1_phy_addr;
extern ca_uint32_t     l3qm_epp_profile_cpu256_lo_rx_fifo_phy_addr;

int aal_l3qm_eq_get_cpu256_hi_rule(void);
int aal_l3qm_eq_get_cpu256_lo_rule(void);
int aal_l3qm_eq_get_cpu256_hi_use_fbm(void);
int aal_l3qm_eq_get_cpu256_lo_use_fbm(void);
void aal_l3qm_eq_get_cpu256_hi_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_cpu256_hi_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_cpu256_lo_pool0_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
void aal_l3qm_eq_get_cpu256_lo_pool1_info(ca_uint32_t *phy_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id);
int aal_l3qm_eq_get_cpu256_max_write_ptr(void);
ca_uint32_t aal_l3qm_cpu256_get_rx_status(int instance);
void aal_l3qm_cpu256_set_rx_read_ptr(ca_uint16_t instance, ca_uint32_t value);
ca_uint32_t aal_l3qm_cpu256_get_rx_read_ptr(ca_uint16_t instance);
ca_uint32_t aal_l3qm_cpu256_get_rx_write_ptr(ca_uint16_t instance, ca_uint16_t *count);
ca_uint32_t aal_l3qm_cpu256_get_rx_start_addr(ca_uint16_t instance);

ca_status_t aal_l3qm_cpu256_check_magic_number(void);
void aal_l3qm_init_cpu256_qm_axi_attrib(void);
void aal_l3qm_init_cpu256_epp(void);
void aal_l3qm_init_empty_buffer_CPU256_hi(void);
void aal_l3qm_init_empty_buffer_CPU256_lo(void);
void aal_l3qm_enable_cpu256_epp_interrupt(ca_uint8_t cpu256_hi, ca_uint8_t enable);
void aal_l3qm_dump_cpu256_ddr_map(void);
ca_uint32_t aal_l3qm_create_cpu256_ddr_map(void);
void aal_l3qm_init_cpu256_startup_config(void);
void aal_l3qm_dump_cpu256_epps(void);
void aal_l3qm_dump_cpu256_empty_buffers_phy_addr(void);
void aal_l3qm_init_cpu256_empty_buffer(void);
void aal_l3qm_init_cpu256_voq(void);
void aal_l3qm_enable_tx_cpu256(ca_uint16_t cpu_port, ca_uint8_t enable);

#endif

#if defined(CONFIG_ARCH_CORTINA_VENUS)
#include "aal_phy.h"
void aal_l3qm_cpu256_epp256_profile_select(ca_uint16_t cpu_port, ca_uint32_t profile);
void aal_l3qm_cpu256_epp256_config_profile_set(ca_uint8_t profile, ca_uint32_t size, ca_uint8_t timer_ths);
void aal_l3qm_cpu256_epp256_drop_profile_set(ca_uint8_t profile, ca_uint32_t drop_level);
void aal_l3qm_cpu256_epp256a_config_profile_set(ca_uint8_t profile, ca_uint32_t size, ca_uint8_t timer_ths);
void aal_l3qm_cpu256_epp256a_drop_profile_set(ca_uint8_t profile, ca_uint32_t drop_level);
void aal_l3qm_cpu256_buffer_cntl_set(ca_uint8_t jumbo_buffer_enable, ca_uint8_t port_speed_select);
void aal_l3qm_cpu256_latency_timer_set(ca_uint8_t ni_port, aal_phy_speed_mode_t speed, ca_uint8_t jumbo_buffer_enable);
void aal_l3qm_cpu256_upper_ldpid_map_set(ca_uint8_t pon_enable, ca_uint8_t pon_offset);
void aal_l3qm_cpu256_schedule_mode_set(ca_uint16_t port);
void aal_l3qm_cpu256_drop_counter_map(ca_uint16_t voq, ca_uint16_t counter_id, ca_uint8_t valid);
ca_status_t aal_l3qm_cpu256_drop_counter_dump(ca_uint16_t counter_id);
ca_status_t ca_l3qm_cpu256_configure_epp(void);
#endif

#endif /* __AAL_L3QM_H__ */


