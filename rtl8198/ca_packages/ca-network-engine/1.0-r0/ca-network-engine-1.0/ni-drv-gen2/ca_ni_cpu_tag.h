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
#ifndef __CA_NI_CPU_TAG_H__
#define __CA_NI_CPU_TAG_H__

#include "ca_types.h"

#define RTK_TAG_ETH_TYPE 0x8899
#define RTK_TAG_PROTO 0x4
#define RTK_TAG_APB0_NI_HV_PT_STRIDE    80

typedef enum {
	RTK_TAG_PROFILE_0       = 0,
	RTK_TAG_PROFILE_1       = 1
} rtk_tag_cfg_sel_t;

typedef enum {
	RTK_NI_PORT_0           = 0,
	RTK_NI_PORT_1           = 1,
	RTK_NI_PORT_2           = 2,
	RTK_NI_PORT_3           = 3,
	RTK_NI_PORT_4           = 4
} rtk_ni_port_t;

typedef enum {
	RTK_EXT_SWITCH_PORT_0 = 0x0,
	RTK_EXT_SWITCH_PORT_1 = 0x1,
	RTK_EXT_SWITCH_PORT_2 = 0x2,
	RTK_EXT_SWITCH_PORT_3 = 0x3,
	RTK_EXT_SWITCH_PORT_4 = 0x4,
	RTK_EXT_SWITCH_PORT_5 = 0x5,
	RTK_EXT_SWITCH_PORT_6 = 0x6,
	RTK_EXT_SWITCH_PORT_7 = 0x7
} rtk_ext_switch_port_t;

typedef volatile union {
  struct {
                ca_uint32_t port_id_0           : 8 ; /* bits 7:0 */
                ca_uint32_t port_id_1           : 8 ; /* bits 15:8 */
                ca_uint32_t port_id_2           : 8 ; /* bits 23:16 */
                ca_uint32_t port_id_3           : 8 ; /* bits 31:24 */
  } bf ;
  ca_uint32_t     wrd ;
} NI_HV_GLB_CPU_TAGx_PORT_ID0_t;

typedef volatile union {
  struct {
                ca_uint32_t port_id_4           : 8 ; /* bits 39:32 */
                ca_uint32_t port_id_5           : 8 ; /* bits 47:40 */
                ca_uint32_t port_id_6           : 8 ; /* bits 55:48 */
                ca_uint32_t rsrvd1                : 8 ; /* bits 63:56 */
  } bf ;
  ca_uint32_t     wrd ;
} NI_HV_GLB_CPU_TAGx_PORT_ID1_t;

typedef volatile union {
        struct {
                ca_uint32_t cos                       : 3 ; /* bits 2:0 */
                ca_uint32_t ldpid                     : 6 ; /* bits 8:3 */
                ca_uint32_t lspid                     : 6 ; /* bits 14:9 */
                ca_uint32_t pkt_size               : 14 ; /* bits 28:15 */
                ca_uint32_t fe_bypass            : 1 ; /* bits 29 */
                ca_uint32_t hdr_type              : 2 ; /* bits 31:30 */
        } bf ;
        ca_uint32_t     wrd ;
} NI_HV_GLB_CPU_TAGx_HDR_Ay_L_t;

typedef volatile union {
        struct {
                ca_uint32_t mcgid                  : 8 ; /* bits 39:32 */
                ca_uint32_t drop_code           : 3 ; /* bits 42:40 */
                ca_uint32_t rx_pkt_type         : 2 ; /* bits 44:43 */
                ca_uint32_t no_drop               : 1 ; /* bits 45 */
                ca_uint32_t mirror                  : 1 ; /* bits 46 */
                ca_uint32_t mark                    : 1 ; /* bits 47 */
                ca_uint32_t pol_en                 : 2 ; /* bits 49:48 */
                ca_uint32_t pol_id                  : 9 ; /* bits 58:50 */
                ca_uint32_t pol_grp_id           : 3 ; /* bits 61:59 */
                ca_uint32_t deep_q                : 1 ; /* bits 62 */
                ca_uint32_t cpu_flag              : 1 ; /* bits 63 */
        } bf ;
        ca_uint32_t     wrd ;
} NI_HV_GLB_CPU_TAGx_HDR_Ay_H_t;

typedef volatile union {
        struct {
                ca_uint32_t port_mask_0           : 8 ; /* bits 7:0 */
                ca_uint32_t port_mask_1           : 8 ; /* bits 15:8 */
                ca_uint32_t port_mask_2           : 8 ; /* bits 23:16 */
                ca_uint32_t port_mask_3           : 8 ; /* bits 31:24 */
        } bf ;
        ca_uint32_t     wrd ;
} NI_HV_GLB_CPU_TAGx_PORT_MASK0_t;

typedef volatile union {
        struct {
                ca_uint32_t port_mask_4           : 8 ; /* bits 39:32 */
                ca_uint32_t port_mask_5           : 8 ; /* bits 47:40 */
                ca_uint32_t port_mask_6           : 8 ; /* bits 55:48 */
                ca_uint32_t rsrvd1                     : 8 ; /* bits 63:56 */
        } bf ;
        ca_uint32_t     wrd ;
} NI_HV_GLB_CPU_TAGx_PORT_MASK1_t;


ca_status_t ca_ni_cpu_tag_init(ca_uint32_t enable, ca_uint32_t port, ca_uint32_t profile);

#endif /* __CA_NI_CPU_TAG_H__ */
