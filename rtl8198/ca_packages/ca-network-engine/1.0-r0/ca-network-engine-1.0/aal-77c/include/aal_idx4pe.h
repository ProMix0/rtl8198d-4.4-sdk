/*
 * Copyright (c) Cortina-Access Limited 2016.  All rights reserved.
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
 * aal_idx4pe.h: Security Association (SA ID) management for Packet Engine
 * This is not related to any HW registers; pure software.
 *
 */
#ifndef __AAL_IDX4PE_H__
#define __AAL_IDX4PE_H__

#include "tunnel.h"
#include "aal_table.h"
#include "aal_common.h"

/***************************************************************************
 * IPSec IDX4PE
 ***************************************************************************/

#define IPSEC_IDX4PE_ENTRY_MAX			(24/2)

typedef struct aal_ipsec_idx4pe_entry_s {
	ca_boolean_t			valid;
	ca_ipsec_sa_id_t		sa_id_encrypt;
	ca_ipsec_sa_id_t		sa_id_decrypt;
} __attribute__((packed)) aal_ipsec_idx4pe_entry_t;

int aal_ipsec_idx4pe_tbl_init(void);
void aal_ipsec_idx4pe_tbl_exit(void);

/***************************************************************************
 * L2TP IDX4PE
 ***************************************************************************/

#define L2TP_IDX4PE_ENTRY_MAX			256

typedef struct aal_l2tp_idx4pe_entry_s {
	ca_boolean_t			valid;
} __attribute__((packed)) aal_l2tp_idx4pe_entry_t;

int aal_l2tp_idx4pe_tbl_init(void);
void aal_l2tp_idx4pe_tbl_exit(void);

/***************************************************************************
 * PPTP IDX4PE
 ***************************************************************************/

#define PPTP_IDX4PE_ENTRY_MAX			256

typedef struct aal_pptp_idx4pe_entry_s {
	ca_boolean_t			valid;
} __attribute__((packed)) aal_pptp_idx4pe_entry_t;

int aal_pptp_idx4pe_tbl_init(void);
void aal_pptp_idx4pe_tbl_exit(void);

/***************************************************************************
 * MAP-E IDX4PE
 ***************************************************************************/

#define MAP_E_IDX4PE_ENTRY_MAX			512

typedef struct aal_map_e_idx4pe_entry_s {
	ca_boolean_t			valid;
} __attribute__((packed)) aal_map_e_idx4pe_entry_t;

int aal_map_e_idx4pe_tbl_init(void);
void aal_map_e_idx4pe_tbl_exit(void);

/***************************************************************************
 * MAP-T IDX4PE
 ***************************************************************************/

#define MAP_T_IDX4PE_ENTRY_MAX			512

typedef struct aal_map_t_idx4pe_entry_s {
	ca_boolean_t			valid;
} __attribute__((packed)) aal_map_t_idx4pe_entry_t;

int aal_map_t_idx4pe_tbl_init(void);
void aal_map_t_idx4pe_tbl_exit(void);

/***************************************************************************
 * MACSec IDX4PE
 ***************************************************************************/

#define MACSEC_IDX4PE_ENTRY_MAX			(24/2)

typedef struct aal_macsec_idx4pe_entry_s {
	ca_boolean_t			valid;
	ca_macsec_sa_id_t		sa_id_outbound;
	ca_macsec_sa_id_t		sa_id_inbound;
} __attribute__((packed)) aal_macsec_idx4pe_entry_t;

int aal_macsec_idx4pe_tbl_init(void);
void aal_macsec_idx4pe_tbl_exit(void);

/***************************************************************************
 * Common initialization
 ***************************************************************************/

int aal_idx4pe_tbl_init(void);
void aal_idx4pe_tbl_exit(void);

#endif /* __AAL_IDX4PE_H__ */

