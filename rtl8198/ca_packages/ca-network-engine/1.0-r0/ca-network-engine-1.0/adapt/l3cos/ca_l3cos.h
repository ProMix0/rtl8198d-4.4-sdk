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
#ifndef __CA_L3COS_H__
#define __CA_L3COS_H__

#include "ca_types.h"

ca_status_t ca_l3cos_rule_add(ca_device_id_t device_id, ca_uint16_t egress_top_vid, ca_uint8_t egress_top_8021p, ca_uint32_t cos);
ca_status_t ca_l3cos_rule_delete(ca_device_id_t device_id, ca_uint16_t egress_top_vid, ca_uint8_t egress_top_8021p);
ca_status_t ca_l3cos_update_all(ca_device_id_t device_id);

ca_status_t ca_l3cos_proc_init(void);
void ca_l3cos_proc_exit(void);

ca_status_t ca_l3cos_init(void);
ca_status_t ca_l3cos_exit(void);

#endif /* __CA_L3COS_H__ */
