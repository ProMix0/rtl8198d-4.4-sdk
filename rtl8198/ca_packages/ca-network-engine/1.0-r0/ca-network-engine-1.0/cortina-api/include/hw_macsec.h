#ifndef _HW_MACSEC_H_
#define _HW_MACSEC_H_
#include "ca_types.h"
#include "port.h"
#include "macsec.h"

#define MACSEC_SA_NODE_NUM  64

extern ca_uint32_t aal_pon_mac_mode;
extern ca_uint8_t  epon_current_an;
extern ca_uint8_t  uni_current_an;

#define CA_MACSEC_INVALID_VALUE 	0xFFFF
#define CA_MACSEC_SA_ID_CHECK(val) /*CODEGEN_IGNORE_TAG*/  if(((val) < 0) || ((val) >= 64)) { ca_printf("%s:[%d] val %d!!\n", __FUNCTION__, __LINE__, val);  return CA_E_PARAM; }
#define CA_MACSEC_SECTAG_TYPE_CHECK_CHECK(val) /*CODEGEN_IGNORE_TAG*/  if(((val) != 0) && ((val) != 1)) { ca_printf("%s:[%d] val %d!!\n", __FUNCTION__, __LINE__, val);  return CA_E_PARAM; }

#ifdef CONFIG_NE_PON
#define CA_PON_MAC_IS_10GEPON() /*CODEGEN_IGNORE_TAG*/ if((aal_pon_mac_mode != ONU_PON_MAC_EPON_D10G)&&(aal_pon_mac_mode != ONU_PON_MAC_EPON_BI10G)) { ca_printf("parameter error!!pon_mac_mode is :%p\n",aal_pon_mac_mode); return CA_E_PARAM; }
#endif

typedef enum {
	CA_MACSEC_TX_DIR = 0x00,
	CA_MACSEC_RX_DIR = 0x01,
	CA_MACSEC_RSVD_DIR = 0x2
} ca_macsec_direction_t;

typedef enum {
	CA_MACSEC_DROP_PKT		= 0x0,
	CA_MACSEC_FORWARD_PKT	= 0x1
} ca_macsec_forward_or_not_t;

typedef enum {
	CA_MACSEC_STRICT 		= 0x0,
	CA_MACSEC_CHECK   		= 0x01,
	CA_MACSEC_DISABLED	  	= 0x02,
	CA_MACSEC_NOT_DEFINED 	= 0x03
} ca_macsec_valid_frame_t;

typedef enum {
	CA_MACSEC_ENCRYP_AUTH = 0x0,
	CA_MACSEC_AUTH_ONLY   = 0x01,
	CA_MACSEC_DECRYP	  = 0x02,
	CA_MACSEC_RSVD	  	  = 0x03
} ca_macsec_sec_mode_t;

typedef enum {
	CA_MACSEC_SCI_NOT_ENCODED	= 0x0,
	CA_MACSEC_SCI_ENCODED		= 0x1
} ca_macsec_sci_indication_t;

typedef enum {
	CA_MACSEC_FWD_ALL_UNTAG_PKT		= 0x0,
	CA_MACSEC_FWD_SPECIFIED_UNTAG_PKT	= 0x1,
	CA_MACSEC_DROP_ALL_UNTAG_PKT		= 0x2,
	CA_MACSEC_DROP_PKT_RSVD			= 0x3
} ca_macsec_drop_untag_pkt_mode_t;

typedef struct {
	ca_uint32_t	said;
	ca_uint32_t	sa_port;/*For UNI , it is UNI portid, for EPON , it is LLID value*/
} ca_said_port_mapping;

typedef struct {
	ca_uint32_t	tx_said;
	ca_uint32_t	tx_sa_port;/*For UNI , it is UNI portid, for EPON , it is LLID value*/
} ca_tx_said_port_mapping;

typedef struct {
	ca_boolean_t	valid; /*0-free, 1-used*/
	ca_uint32_t	port_type;/*UNI port or EPON port*/
	ca_uint32_t	port_id;
	ca_uint32_t	sa_port;/*For UNI , it is UNI portid, for EPON , it is LLID value*/
	ca_uint32_t	logic_sa_id;
	ca_macsec_sa_t	sa_context;
} ca_sa_entry_t;

/*************************************************
 * Internal API
 *************************************************/

ca_status_t hw_macsec_sas_get(
	CA_IN ca_device_id_t	device_id
);

/*************************************************
 * HW MACSec API
 *************************************************/

ca_status_t hw_macsec_init(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_macsec_global_config_t			*macsec_global_conf
);

ca_status_t hw_macsec_global_config_get(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_OUT	ca_macsec_global_config_t		*macsec_global_conf
);

ca_status_t hw_macsec_decryption_wa(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t	 port,
    CA_IN ca_uint8_t     said
);

ca_status_t hw_macsec_sa_add(
	CA_IN ca_device_id_t				device_id,
	CA_IN_OUT ca_macsec_sa_t			*sa_context
);

ca_status_t hw_macsec_sa_get(
	CA_IN ca_device_id_t				device_id,
	CA_IN_OUT ca_macsec_sa_t			*sa_context
);

ca_status_t hw_macsec_sas_get(
	CA_IN ca_device_id_t				device_id
);

ca_status_t hw_macsec_sa_delete(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_macsec_sa_id_t				sa_id
);

ca_status_t hw_macsec_sa_update(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_macsec_sa_t				*sa_context
);

ca_status_t hw_macsec_sa_active_index_set(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_macsec_sa_id_t				sa_id,
	CA_IN ca_uint8_t				active_index
);

ca_status_t hw_macsec_sa_active_index_get(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_macsec_sa_id_t				sa_id,
	CA_OUT ca_uint8_t				*active_index
);

ca_status_t hw_macsec_sa_iterate(
	CA_IN ca_device_id_t				device_id,
	CA_OUT ca_iterator_t				*return_entry
);

ca_status_t hw_macsec_port_sa_set(
	CA_IN		ca_device_id_t			device_id,
	CA_IN		ca_port_id_t			port,
	CA_IN		ca_mpcp_llid_t			mpcp_llid,
	CA_IN		ca_macsec_sa_id_t		rx_sa_id,
	CA_IN		ca_macsec_sa_id_t		tx_sa_id
);

ca_status_t hw_macsec_port_sa_get(
	CA_IN		ca_device_id_t			device_id,
	CA_IN		ca_port_id_t			port,
	CA_IN		ca_mpcp_llid_t			mpcp_llid,
	CA_OUT		ca_macsec_sa_id_t		*rx_sa_id,
	CA_OUT		ca_macsec_sa_id_t		*tx_sa_id
);

ca_status_t hw_macsec_port_mc_sa_set(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid,
	CA_IN ca_macsec_sa_id_t				mc_sa_id
);

ca_status_t hw_macsec_port_mc_sa_get(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid,
	CA_OUT ca_macsec_sa_id_t			*mc_sa_id
);

ca_status_t hw_macsec_port_stats_get(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid,
	CA_OUT ca_macsec_stats_t			*counter
);

ca_status_t hw_macsec_port_stats_clear(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid
);

ca_status_t hw_macsec_port_error_get(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid,
	CA_OUT ca_macsec_error_t			*counter
);

ca_status_t hw_macsec_port_error_clear(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid
);

#endif /* _HW_MACSEC_H_ */
