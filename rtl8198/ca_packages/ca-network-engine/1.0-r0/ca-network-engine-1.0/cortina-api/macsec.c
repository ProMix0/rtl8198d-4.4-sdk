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
#include "ca_types.h"
#include "osal_common.h"
#include "aal_common.h"
#include "aal_uni_macsec.h"
#include "macsec.h"
#include "hw_macsec.h"
#include "sw_macsec.h"
#include "ca-scfg/scfg.h"
#include "util_aes.h"

#ifdef CONFIG_NE_PON
#include "aal_epon_crypto.h"
#include "aal_mpcp.h"
#endif

ca_status_t ca_macsec_sas_get(
	CA_IN	ca_device_id_t			device_id
)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_sas_get(device_id);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_sas_get(device_id);
#else
	return CA_E_NOT_SUPPORT;
#endif
}

ca_status_t ca_macsec_init (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t			port,
	CA_IN	ca_macsec_global_config_t		*macsec_global_conf)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_init(device_id, port, macsec_global_conf);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_init(device_id, port, macsec_global_conf);
#else
	return sw_macsec_init(device_id, port, macsec_global_conf);
#endif
}

ca_status_t ca_macsec_global_config_get(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_OUT	ca_macsec_global_config_t		*macsec_global_conf)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_global_config_get(device_id, port, macsec_global_conf);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_global_config_get(device_id, port, macsec_global_conf);
#else
	return sw_macsec_global_config_get(device_id, port, macsec_global_conf);
#endif
}

ca_status_t ca_macsec_sa_add (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_macsec_sa_t			*sa_context)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_sa_add(device_id, sa_context);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_sa_add(device_id, sa_context);
#else
	return sw_macsec_sa_add(device_id, sa_context);
#endif
}

ca_status_t ca_macsec_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_macsec_sa_t				*sa_context)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_sa_get(device_id, sa_context);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_sa_get(device_id, sa_context);
#else
	return sw_macsec_sa_get(device_id, sa_context);
#endif
}

ca_status_t ca_macsec_sa_delete (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_sa_delete(device_id, sa_id);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_sa_delete(device_id, sa_id);
#else
	return sw_macsec_sa_delete(device_id, sa_id);
#endif
	}

ca_status_t ca_macsec_sa_update(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_t				*sa_context)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_sa_update(device_id, sa_context);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_sa_update(device_id, sa_context);
#else
	return sw_macsec_sa_update(device_id, sa_context);
#endif
	}

ca_status_t ca_macsec_sa_active_index_set(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id,
	CA_IN	ca_uint8_t				active_index)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_sa_active_index_set(device_id, sa_id, active_index);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_sa_active_index_set(device_id, sa_id, active_index);
#else
	return sw_macsec_sa_active_index_set(device_id, sa_id, active_index);
#endif
	}

ca_status_t ca_macsec_sa_active_index_get(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id,
	CA_IN	ca_uint8_t		*active_index)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_sa_active_index_get(device_id, sa_id, active_index);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_sa_active_index_get(device_id, sa_id, active_index);
#else
	return sw_macsec_sa_active_index_get(device_id, sa_id, active_index);
#endif
}

ca_status_t ca_macsec_sa_iterate (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_iterator_t				*return_entry)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_sa_iterate(device_id, return_entry);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_sa_iterate(device_id, return_entry);
#else
	return sw_macsec_sa_iterate(device_id, return_entry);
#endif
}

ca_status_t ca_macsec_port_sa_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t			mpcp_llid,
	CA_IN	ca_macsec_sa_id_t			rx_sa_id,
	CA_IN	ca_macsec_sa_id_t			tx_sa_id)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_port_sa_set(device_id, port, mpcp_llid, rx_sa_id, tx_sa_id);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_port_sa_set(device_id, port, mpcp_llid, rx_sa_id, tx_sa_id);
#else
	return sw_macsec_port_sa_set(device_id, port, mpcp_llid, rx_sa_id, tx_sa_id);
#endif
}

ca_status_t ca_macsec_port_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t			mpcp_llid,
	CA_OUT	ca_macsec_sa_id_t			*rx_sa_id,
	CA_OUT	ca_macsec_sa_id_t			*tx_sa_id)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_port_sa_get(device_id, port, mpcp_llid, rx_sa_id, tx_sa_id);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_port_sa_get(device_id, port, mpcp_llid, rx_sa_id, tx_sa_id);
#else
	return sw_macsec_port_sa_get(device_id, port, mpcp_llid, rx_sa_id, tx_sa_id);
#endif
}

ca_status_t ca_macsec_port_mc_sa_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t			mpcp_llid,
	CA_IN	ca_macsec_sa_id_t			mc_sa_id)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_port_mc_sa_set(device_id, port, mpcp_llid, mc_sa_id);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_port_mc_sa_set(device_id, port, mpcp_llid, mc_sa_id);
#else
	return sw_macsec_port_mc_sa_set(device_id, port, mpcp_llid, mc_sa_id);
#endif
}

ca_status_t ca_macsec_port_mc_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t			mpcp_llid,
	CA_IN	ca_macsec_sa_id_t		*mc_sa_id)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_port_mc_sa_get(device_id, port, mpcp_llid, mc_sa_id);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_port_mc_sa_get(device_id, port, mpcp_llid, mc_sa_id);
#else
	return sw_macsec_port_mc_sa_get(device_id, port, mpcp_llid, mc_sa_id);
#endif
}

ca_status_t ca_macsec_port_stats_get(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t			mpcp_llid,
	CA_OUT	ca_macsec_stats_t			*counter)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_port_stats_get(device_id, port, mpcp_llid, counter);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_port_stats_get(device_id, port, mpcp_llid, counter);
#else
	return sw_macsec_port_stats_get(device_id, port, mpcp_llid, counter);
#endif
}

ca_status_t ca_macsec_port_stats_clear(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t			mpcp_llid)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_port_stats_clear(device_id, port, mpcp_llid);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_port_stats_clear(device_id, port, mpcp_llid);
#else
	return sw_macsec_port_stats_clear(device_id, port, mpcp_llid);
#endif
}

ca_status_t ca_macsec_port_error_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t			mpcp_llid,
	CA_OUT	ca_macsec_error_t			*counter)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_port_error_get(device_id, port, mpcp_llid, counter);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_port_error_get(device_id, port, mpcp_llid, counter);
#else
	return sw_macsec_port_error_get(device_id, port, mpcp_llid, counter);
#endif
}

ca_status_t ca_macsec_port_error_clear (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t			mpcp_llid)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
	return hw_macsec_port_error_clear(device_id, port, mpcp_llid);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	return hw_macsec_port_error_clear(device_id, port, mpcp_llid);
#else
	return sw_macsec_port_error_clear(device_id, port, mpcp_llid);
#endif
}

ca_status_t ca_macsec_port_hw_decryption_wa(
    CA_IN   ca_device_id_t          device_id,
    CA_IN   ca_port_id_t            port,
    CA_IN   ca_uint8_t              said)
{
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    return hw_macsec_decryption_wa(device_id, port, said);
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    return hw_macsec_decryption_wa(device_id, port, said);
#endif
}

EXPORT_SYMBOL(ca_macsec_init);
EXPORT_SYMBOL(ca_macsec_global_config_get);
EXPORT_SYMBOL(ca_macsec_sa_add);
EXPORT_SYMBOL(ca_macsec_sa_get);
EXPORT_SYMBOL(ca_macsec_sas_get);
EXPORT_SYMBOL(ca_macsec_sa_delete);
EXPORT_SYMBOL(ca_macsec_sa_update);
EXPORT_SYMBOL(ca_macsec_sa_active_index_set);
EXPORT_SYMBOL(ca_macsec_sa_active_index_get);
EXPORT_SYMBOL(ca_macsec_sa_iterate);
EXPORT_SYMBOL(ca_macsec_port_sa_set);
EXPORT_SYMBOL(ca_macsec_port_sa_get);
EXPORT_SYMBOL(ca_macsec_port_mc_sa_set);
EXPORT_SYMBOL(ca_macsec_port_mc_sa_get);
EXPORT_SYMBOL(ca_macsec_port_stats_get);
EXPORT_SYMBOL(ca_macsec_port_stats_clear);
EXPORT_SYMBOL(ca_macsec_port_error_get);
EXPORT_SYMBOL(ca_macsec_port_error_clear);
EXPORT_SYMBOL(ca_macsec_port_hw_decryption_wa);

