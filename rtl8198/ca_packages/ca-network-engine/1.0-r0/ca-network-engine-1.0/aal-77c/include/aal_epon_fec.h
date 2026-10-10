
#ifndef __AAL_EPON_FEC_H__
#define __AAL_EPON_FEC_H__

#include "ca_types.h"

typedef enum
{
    AAL_FEC_PREAMBLE_MODE_STANDARD      = 0, /* standard FEC encoding */
    AAL_FEC_PREAMBLE_MODE_FIX8B         = 1  /* Cortina-specific FEC encoding. Always keep 8-byte preamble */
}aal_epon_fec_preamble_mode_t;

typedef enum
{
    AAL_FEC_STD_MODE_2003      = 0, /* select TFEC 2003 standard */
    AAL_FEC_STD_MODE_2005      = 1  /* select TFEC 2005 standard */
}aal_epon_fec_std_mode_t;


typedef struct
{
    ca_uint8_t  tx_encoding_ena;
    ca_uint8_t  tx_encoding_mode;       /* FEC encoding style. should be set before ONU starts registration
                                        ** Valid when fec_encode_enable is set to 1.
                                        ** 1'h1 : Cortina-specific FEC encoding. Always keep 8-byte preamble.
                                        ** 1'h0 : standard FEC encoding
                                        */
    ca_uint8_t  tx_tfec_pattern;        /* standards on the FEC tailor byte patterns
                                        ** There are two versions of standards on the FEC tailor byte patterns.
                                        ** The 2005 version is the currently standard compliant one.
                                        ** This bit is used to select 2005 version of TFEC byte pattern.
                                        ** 1'h1: select TFEC 2005 standard;
                                        ** 1'h0: select TFEC 2003 standard;
                                        */
    ca_uint8_t  tx_ipg_adjust;          /* This value is to adjust when to generate fec_ready signal back to TxMAC.
                                        ** default value is 4.
                                        */
    ca_uint8_t  tx_1st_preamble_mode;  /* default value is 1 */
    ca_uint8_t  tx_path_clear;         /* 0: FEC TX Path free running; 1: clear FEC Tx Path */

    ca_uint8_t  rx_decoding_ena;
    ca_uint8_t  rx_path_clear;         /* 0: FEC Rx path fre reunning; 1: clear FEC Rx path */
    ca_uint16_t rx_ctrl;               /* max value : 0x7ff */
    ca_uint8_t  non_fec_mode;          /* 1: non-fec mode; 0: fec mode */
    ca_uint8_t  cnt_clr;               /* 1: FEC counter read clear, 0: read keep */      
} aal_epon_fec_cfg_t;


typedef union
{
    struct
    {
        ca_uint32_t             tx_encoding_ena      : 1;
        ca_uint32_t             tx_encoding_mode     : 1;
        ca_uint32_t             tx_tfec_pattern      : 1;
        ca_uint32_t             tx_ipg_adjust        : 1;
        ca_uint32_t             tx_1st_preamble_mode : 1;
        ca_uint32_t             tx_path_clear        : 1;
        ca_uint32_t             rx_decoding_ena      : 1;
        ca_uint32_t             rx_path_clear        : 1;
        ca_uint32_t             rx_ctrl              : 1;
        ca_uint32_t             non_fec_mode         : 1;
        ca_uint32_t             cnt_clr              : 1;
    } s;
    ca_uint32_t u32;
} aal_epon_fec_msk_t;

typedef struct{
    ca_uint32_t rx_good_pkt;
    ca_uint32_t rx_err_pkt;
    ca_uint32_t rx_corr_codeword;
    ca_uint32_t rx_uncorr_codeword;
    ca_uint32_t rx_err_pattern;
    ca_uint32_t rx_err_blocks;
    ca_uint32_t rx_err_ber;
    ca_uint32_t rx_corr_bytes;
    ca_uint32_t rx_total_codeword;
}aal_epon_fec_stats_t;

typedef enum
{
    AAL_FEC_XG_RX_MODE_ON    = 0,
    AAL_FEC_XG_RX_MODE_OFF   = 1,
    AAL_FEC_XG_RX_MODE_AUTO  = 2,
    AAL_FEC_XG_RX_MODE_MAX   = 3
}aal_epon_fec_xg_rx_mode_t;

typedef struct {
    aal_epon_fec_xg_rx_mode_t rx_mode;              /* Indicate whether or not to replace data with error
                                                   * code when uncorrect FEC errors are detected
                                                   * TRUE: Replace data with error code
                                                   * FALSE: Do NOT replace data */
    ca_boolean_t              rx_bypass_delay_ena;  /* TRUE: delay of fec decoder is on */

    ca_boolean_t              tx_ena;               /* Indicate whether or not to disable counting fec blocks
                                                   * TRUE: continue counting fec blocks even when fec decoding is not set
                                                   * FALSE: disable counting fec blocks when fec decoding is not set
                                                  */
}aal_epon_fec_xg_cfg_t;


typedef union
{
    struct
    {
#ifdef CA_BIG_ENDIAN
        ca_uint32_t        f_rsvd                                  :29;
        ca_uint32_t        tx_ena                                  :1;
        ca_uint32_t        rx_bypass_delay_ena                     :1;
        ca_uint32_t        rx_mode                                 :1;
#else
        ca_uint32_t        rx_mode                                 :1;
        ca_uint32_t        rx_bypass_delay_ena                     :1;
        ca_uint32_t        tx_ena                                  :1;
        ca_uint32_t        f_rsvd                                  :29;
#endif
    }s;
    ca_uint32_t        u32;
}aal_epon_fec_xg_cfg_msk_t;

typedef struct aal_epon_fec_llid_ena_s{
    ca_boolean_t             tx_ena;
    ca_boolean_t             rx_ena;
}aal_fec_llid_ena_t;

ca_status_t aal_epon_fec_cfg_set
(
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_port_id_t             port_id,
    CA_IN aal_epon_fec_msk_t         mask,
    CA_IN aal_epon_fec_cfg_t        *cfg
);

ca_status_t aal_epon_fec_cfg_get
(
    CA_IN ca_device_id_t              device_id,
    CA_IN ca_port_id_t             port_id,
    CA_OUT aal_epon_fec_cfg_t        *cfg
);

ca_status_t aal_epon_fec_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT aal_epon_fec_stats_t *stats
    );

ca_status_t aal_epon_fec_xg_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN aal_epon_fec_xg_cfg_msk_t msk,
    CA_IN const aal_epon_fec_xg_cfg_t *cfg
    );

ca_status_t aal_epon_fec_xg_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT aal_epon_fec_xg_cfg_t *cfg
    );

ca_status_t aal_epon_fec_xg_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT aal_epon_fec_stats_t *stats
    );

ca_status_t aal_epon_fec_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id, 
    CA_IN ca_uint32_t mode);


#endif /* __AAL_EPON_FEC_H__ */
