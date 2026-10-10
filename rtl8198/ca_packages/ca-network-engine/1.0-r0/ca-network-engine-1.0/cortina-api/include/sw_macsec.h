#ifndef __SW_MACSEC_H__
#define __SW_MACSEC_H__

#include "ca_types.h"
#include "macsec.h"

/***************************************************************************
 * Software MACSec API; not exported
 ***************************************************************************/

ca_status_t sw_macsec_init (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_macsec_global_config_t		*macsec_global_conf
);

ca_status_t sw_macsec_global_config_get (
	CA_IN	ca_device_id_t 				device_id,
	CA_IN	ca_port_id_t				port,
	CA_OUT	ca_macsec_global_config_t		*macsec_global_conf
);

ca_status_t sw_macsec_sa_add (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_macsec_sa_t				*sa_context
);

ca_status_t sw_macsec_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_macsec_sa_t				*sa_context
);

ca_status_t sw_macsec_sa_delete (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id
);

ca_status_t sw_macsec_sa_update (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_t				*sa_context
);

ca_status_t sw_macsec_sa_active_index_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id,
	CA_IN	ca_uint8_t				active_index
);

ca_status_t sw_macsec_sa_active_index_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id,
	CA_OUT	ca_uint8_t				*active_index
);

ca_status_t sw_macsec_sa_iterate (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_iterator_t				*return_entry
);

ca_status_t sw_macsec_port_sa_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_IN	ca_macsec_sa_id_t			rx_sa_id,
	CA_IN	ca_macsec_sa_id_t			tx_sa_id
);

ca_status_t sw_macsec_port_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_sa_id_t			*rx_sa_id,
	CA_OUT	ca_macsec_sa_id_t			*tx_sa_id
);

ca_status_t sw_macsec_port_mc_sa_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_IN	ca_macsec_sa_id_t			mc_sa_id
);

ca_status_t sw_macsec_port_mc_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_sa_id_t			*mc_sa_id
);

ca_status_t sw_macsec_port_stats_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_stats_t			*counter
);

ca_status_t sw_macsec_port_stats_clear (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid
);

ca_status_t sw_macsec_port_error_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_error_t			*counter
);

ca_status_t sw_macsec_port_error_clear (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN	ca_mpcp_llid_t				mpcp_llid
);

#endif /* __SW_MACSEC_H__ */

