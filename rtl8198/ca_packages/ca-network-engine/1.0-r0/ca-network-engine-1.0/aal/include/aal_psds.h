#ifndef __AAL_PSDS_H__
#define __AAL_PSDS_H__

#include "ca_types.h"

typedef enum {
    AAL_PSDS_MODE_XGEPON_SYM     = 0,
    AAL_PSDS_MODE_XGPON_SYM_25M  = 1,
    AAL_PSDS_MODE_XGPON_ASYM_25M = 2,
    AAL_PSDS_MODE_GPON           = 3,
    AAL_PSDS_MODE_XGEPON_ASYM    = 4,
    AAL_PSDS_MODE_ETHERNET       = 5,
    AAL_PSDS_MODE_EPON           = 6,
    AAL_PSDS_MODE_TUBO_EPON      = 7,
    AAL_PSDS_MODE_NUM
} aal_psds_mode_t;

typedef enum {
	PSDS_CALI_MODE_AUTO = 0,
	PSDS_CALI_MODE_MANUAL,
	MAX_PSDS_CALI_MODE,
} aal_psds_cali_mode_t;

typedef enum {
	PSDS_RX_EQSEL_MODE_AUTO = 0,
	PSDS_RX_EQSEL_MODE_MANUAL,
	MAX_PSDS_RX_EQSEL_MODE,
} aal_psds_rx_eqsel_mode_t;

typedef enum {
	PSDS_TX_MAINAMP_MODE_DIS = 0,
	PSDS_TX_MAINAMP_MODE_EN,
	MAX_PSDS_TX_MAINEMP_MODE,
} aal_psds_tx_mainamp_mode_t;

typedef enum {
	PSDS_TX_PREAMP_MODE_DIS = 0,
	PSDS_TX_PREAMP_MODE_EN,
	MAX_PSDS_TX_PREAMP_MODE,
} aal_psds_tx_preamp_mode_t;

typedef enum {
	PSDS_TX_POSTAMP_MODE_DIS = 0,
	PSDS_TX_POSTAMP_MODE_EN,
	MAX_PSDS_TX_POSTAMP_MODE,
} aal_psds_tx_postAmp_mode_t;

#define AAL_PSDS_XTAL_24MHZ   0 /* crystal for RX reference : 24.576MHz */
#define AAL_PSDS_XTAL_25MHZ   1 /* crystal for RX reference : 25MHz */

typedef union {
	struct {
		ca_uint32_t reg0_main_en : 1;
		ca_uint32_t reg0_main_amp: 1;
		ca_uint32_t reg0_pre_en  : 1;
		ca_uint32_t reg0_pre_amp : 1;
		ca_uint32_t reg0_post_en : 1;
		ca_uint32_t reg0_post_amp: 1;
		ca_uint32_t rsvd           : 26;
	} s;
	ca_uint32_t u32;
} aal_psds_tx_amp_mask_t;

typedef struct {
	ca_uint8_t reg0_main_en;
	ca_uint8_t reg0_main_amp;
	ca_uint8_t reg0_pre_en;
	ca_uint8_t reg0_pre_amp;
	ca_uint8_t reg0_post_en;
	ca_uint8_t reg0_post_amp;
} aal_psds_tx_amp_t;

typedef union {/* mask for aal_psds_rg24_t */
    struct {
        ca_uint32_t reg_ttl_dri_sel             :  1 ;
        ca_uint32_t reg_ben_ttl_out             :  1 ;
        ca_uint32_t reg_ben_por_sel            :  1 ;
        ca_uint32_t rsvd                        :  29;
    } s;
    ca_uint32_t u32;
} aal_psds_rg24_mask_t;

typedef struct {
    ca_uint8_t reg_ttl_dri_sel             ; /* BEN  TTL Drive selection: 0-4mA; 1-6mA;2-8mA;3-12mA */
    ca_uint8_t reg_ben_ttl_out             ;/* BEN  TTL Output */
    ca_uint8_t reg_ben_por_sel            ;/* BEN  polarity : 0 - original BEN output; 1- ~BEN output */
} aal_psds_rg24_t;

typedef union {
	struct {
		ca_uint32_t reg0_eq_selreg : 1;
		ca_uint32_t reg0_filter_out: 1;
		ca_uint32_t rsvd           : 30;
	} s;
	ca_uint32_t u32;
} aal_psds_rg42_mask_t;

typedef struct {
	ca_uint8_t reg0_eq_selreg;
	ca_uint8_t reg0_filter_out;
} aal_psds_rg42_t;

typedef union {
	struct {
		ca_uint32_t reg_ckgphy_sel : 1;
		ca_uint32_t rsvd           : 31;
	} s;
	ca_uint32_t u32;
} aal_psds_rg9f_mask_t;

typedef struct {
	ca_uint32_t reg_ckgphy_sel;
} aal_psds_rg9f_t;

typedef union {/* mask for aal_psds_gbox_ctrl_t */
    struct {
        ca_uint32_t prbs_rx_en                  :  1 ;
        ca_uint32_t prbs_tx_en                  :  1 ;
        ca_uint32_t rx_bit_ordering             :  1 ;
        ca_uint32_t tx_bit_ordering             :  1 ;
        ca_uint32_t rx_elst_ff_rsync            :  1 ;
        ca_uint32_t tx_elst_ff_rsync            :  1 ;
        ca_uint32_t rx_1gepn_turbo_bit_ordering :  1 ;
        ca_uint32_t lpbk_rx_tx_near_en          :  1 ;
        ca_uint32_t lpbk_tx_rx_en               :  1 ;
        ca_uint32_t lao_en                      :  1 ;
        ca_uint32_t rsvd                        :  24;
    } s;
    ca_uint32_t data32;
} aal_psds_gbox_ctrl_mask_t;

typedef struct {/* Control of gearbox */
    ca_uint8_t prbs_rx_en                  ;
    ca_uint8_t prbs_tx_en                  ;
	ca_uint8_t rx_bit_ordering; /* test Bit[1] reserved. Bit[0]: bit reversed control for receiving data bus*/
    ca_uint8_t tx_bit_ordering             ;/* Bit[1] reserved. Bit[0]: bit reversed control for transmit data bus*/
    ca_uint8_t rx_elst_ff_rsync            ;/* receiving path bitwidth converting module resync control*/
    ca_uint8_t tx_elst_ff_rsync            ;/* transmit path bitwidth converting module resync control.*/
    ca_uint8_t rx_1gepn_turbo_bit_ordering ;/* Bit reversing control for rx data bus between GBOX and EPON MAC under EPON burbo mode*/
    ca_uint8_t lpbk_rx_tx_near_en          ; /*  this loopback may cause the loopback traffic corruption*/
    ca_uint8_t lpbk_tx_rx_en               ; /*Only active under 10G/10G symmetric mode*/
    ca_uint8_t lao_en                      ; /*Optical transceiver Transmit Laser always on when set*/
} aal_psds_gbox_ctrl_t;

typedef union {
	struct {
		ca_uint32_t rx_prbs_mode               : 1;
		ca_uint32_t rx_invrt_prbs              : 1;
		ca_uint32_t rx_clr_cnt                 : 1;
		ca_uint32_t rx_prbs_enbl               : 1;
		ca_uint32_t rx_ld_prbs_state           : 1;
		ca_uint32_t tx_prbs_mode               : 1;
		ca_uint32_t tx_insrt_one_bit_err_once  : 1;
		ca_uint32_t tx_insrt_one_bit_err       : 1;
		ca_uint32_t tx_invrt_prbs              : 1;
		ca_uint32_t tx_insrt_err_once          : 1;
		ca_uint32_t tx_insrt_err               : 1;
		ca_uint32_t rsvd                       : 21;
	} s;
	ca_uint32_t data32;
} aal_psds_prbs_ctrl_mask_t;

typedef struct { /* Control for prbs*/
	ca_uint8_t rx_prbs_mode;
	ca_uint8_t rx_invrt_prbs;
	ca_uint8_t rx_clr_cnt;
	ca_uint8_t rx_prbs_enbl;
	ca_uint8_t rx_ld_prbs_state;
	ca_uint8_t tx_prbs_mode;
	ca_uint8_t tx_insrt_one_bit_err_once;
	ca_uint8_t tx_insrt_one_bit_err;
	ca_uint8_t tx_invrt_prbs;
	ca_uint8_t tx_insrt_err_once;
	ca_uint8_t tx_insrt_err;
} aal_psds_prbs_ctrl_t;

typedef struct { /* PON SERDES status */
    ca_uint8_t rx_los        ;
    ca_uint8_t ckrdy_rx      ;
    ca_uint8_t ckrdy_tx      ;
    ca_uint8_t ckrdy_mpcp    ;
    ca_uint8_t ckrdy_sync2xfi;
    ca_uint8_t rxidle        ;
    ca_uint8_t ber_notify    ;
	ca_uint32_t prbs_err_cnt;
} aal_psds_status_t;

typedef struct {
	ca_uint8_t ckgphy_sel;
}aal_psds_cfg_t;

typedef union {
	struct {
		ca_uint32_t ckgphy_sel: 1;
		ca_uint32_t rsvd      : 31;
	} s;
	ca_uint32_t data32;
}aal_psds_cfg_mask_t;

ca_status_t aal_psds_rx_polarity_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint32_t    polarity);

ca_status_t aal_psds_rx_polarity_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint32_t   *polarity);

ca_status_t
aal_psds_tx_amp_set(
	CA_IN ca_device_id_t       device_id,
	CA_IN aal_psds_tx_amp_mask_t txamp_mask,
	CA_IN aal_psds_tx_amp_t*     txamp);

ca_status_t
aal_psds_tx_amp_get(
	CA_IN  ca_device_id_t   device_id,
	CA_OUT aal_psds_tx_amp_t* txamp);

ca_status_t aal_psds_rg24_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN aal_psds_rg24_mask_t  rg24_mask,
    CA_IN aal_psds_rg24_t      *rg24);

ca_status_t aal_psds_rg24_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_psds_rg24_t      *rg24);

ca_status_t
aal_psds_rg42_set(
	CA_IN ca_device_id_t       device_id,
	CA_IN aal_psds_rg42_mask_t rg42_mask,
	CA_IN aal_psds_rg42_t*     rg42);

ca_status_t aal_psds_rg42_get(
	CA_IN  ca_device_id_t       device_id,
	CA_OUT aal_psds_rg42_t      *rg42);

ca_status_t aal_psds_gbox_ctrl_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN aal_psds_gbox_ctrl_mask_t  ctrl_mask,
    CA_IN aal_psds_gbox_ctrl_t      *ctrl);

ca_status_t aal_psds_gbox_ctrl_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_psds_gbox_ctrl_t      *ctrl);

ca_status_t aal_psds_prbs_ctrl_set(
	CA_IN ca_device_id_t             device_id,
	CA_IN aal_psds_prbs_ctrl_mask_t  ctrl_mask,
	CA_IN aal_psds_prbs_ctrl_t      *ctrl);

ca_status_t aal_psds_prbs_ctrl_get(
	CA_IN  ca_device_id_t            device_id,
	CA_OUT aal_psds_prbs_ctrl_t      *ctrl);

ca_status_t aal_psds_status_get(
    CA_IN  ca_device_id_t             device_id,
    CA_OUT aal_psds_status_t      *sts);

ca_status_t aal_psds_cfg_set(
	CA_IN ca_device_id_t      device_id,
	CA_IN aal_psds_cfg_mask_t mask,
	CA_IN aal_psds_cfg_t      *config);

ca_status_t aal_psds_cfg_get(
	CA_IN  ca_device_id_t device_id,
	CA_OUT aal_psds_cfg_t *config);

ca_status_t aal_psds_prbs_intr_enable( /*CODEGEN_IGNORE_TAG*/
	CA_IN ca_boolean_t enable);

ca_status_t aal_psds_prbs_intr_status_clear( /*CODEGEN_IGNORE_TAG*/
	void);

bool aal_psds_prbs_intr_status_get( /*CODEGEN_IGNORE_TAG*/
	void);

bool aal_psds_prbs_intr_enable_get( /*CODEGEN_IGNORE_TAG*/
	void);

void aal_psds_burst_enable(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_boolean_t enable);

ca_status_t aal_psds_out_of_reset(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  sds_mode);

ca_status_t aal_psds_eth_reset(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_psds_eth_link_status_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_boolean_t  *link_up
);

ca_status_t aal_psds_cdr_freq_offset_get( /*CODEGEN_IGNORE_TAG*/
        CA_IN ca_device_id_t device_id,
        CA_OUT ca_uint32_t *offset
	);

ca_status_t aal_psds_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t pon_mode
);

ca_status_t aal_psds_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t debug_selection/* PON SerDes debug select options: 
                                       * 0: disable PON SerDes debug print;
                                       * 1: enable PON SerDes debug print;
                                       * 253: reset Active Ethernet Serdes;
                                       * 254: reset PON SerDes;
                                       * 255: re-initialize PON SerDes;
                                       */
);

ca_status_t aal_psds_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *debug_selection);

ca_status_t aal_psds_reset(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t pon_mode);

#endif /* __AAL_PSDS_H__ */

