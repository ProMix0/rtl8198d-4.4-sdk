#ifndef _MACSEC_H_
#define _MACSEC_H_
#include "ca_types.h"
#include "port.h"

extern ca_uint32_t aal_pon_mac_mode;

#define MACSEC_KEY_LENGTH		16
#define MACSEC_MAX_KEYS			4

/*************************************************
 * Unofficial API
 *************************************************/

ca_status_t ca_macsec_sas_get(
	CA_IN ca_device_id_t				device_id
);

/*************************************************
 * MACSec API
 *************************************************/

/*MACSec global configurations*/
typedef struct {
	/* RX direction SecTag check */
	ca_uint32_t	fwd_untagged_oam : 1;		/* 0 - drop un-SecTagged OAM frames, 1 - forward */
	ca_uint32_t	fwd_untagged_mpcp : 1;		/* 0 - drop un-SecTagged MPCP frames, 1 - forward */
	ca_uint32_t	fwd_untagged_ethertype : 1;	/* 0 - drop un-SecTagged user-defined frames, 1- forward */

	/* TX direction SecTag */
	ca_uint32_t	skip_mpcp: 1;			/* 1 - Tx MPCP not encrypted, 0 - Tx MPCP encrypted */
	ca_uint32_t	skip_oam: 1;			/* 1 - Tx OAM not encrypted, 0 - Tx OAM encrypted */
	ca_uint32_t	skip_untagged_ethertype: 1;	/* 1 - Tx ethertype not encrypted, 0 - Tx ethertype encrypted */

	ca_uint16_t	untagged_ethertype;		/* User-defined ethertype.
							   Used for fwd_untagged_ethertype and skip_untagged_ethertype.
							 */

	ca_uint32_t	confidential_offset_error : 1;	/* 0 - "confidential offset > data length" is not error.
							 * 1 - "confidential offset > data length" is error.
							 */
	ca_uint32_t	data_path_reset : 1;		/* reset MACSec data path (for all LLIDs) - active high; for debug only */

	ca_uint32_t	KaY_reserved : 1;		/* 0 - drop, 1 - forward (KaY: 802.1X Key Agreement Entity) */
	ca_uint32_t	drop_untagged_rx_frames: 2;	/* 0 - don't drop untagged frames.
							   1 - forward untagged frames specified in fwd_untagged_xxx.
							       drop other untagged frames.
							   2 - drop all untagged frames.
							   NOTE: this configuration takes precedence over SADB flags configuration.
							 */
} ca_macsec_global_config_t;

/* SAK control flags */
typedef struct {
	ca_uint32_t	end_station : 1;
	ca_uint32_t	sci_encoded : 1;		/* 0 - SCI not encoded, 1 -SCI encoded */
	ca_uint32_t	single_copy_broadcast : 1;	/* SCB field (single copy broadcast) */
	ca_uint32_t	frame_validation : 2;		/* 00 - strict (802.1AE 10.6)
							 * 01 - check
							 * 10 - disable
							 * 11 - reserved
							 */
	ca_uint32_t	replay_check : 1;		/* check for replay when decrypting.
							 * NOTE: E and C bits are set by HW, not configurable by SW.
							 */
	ca_uint32_t	association_number : 2;		/* AN (select KEY0 or KEY1) */
	ca_uint32_t	confidential_offset : 7;	/* Confidentiality Offset */
	ca_uint32_t	security_mode : 2;		/* 0=enc+auth (Tx/Rx), 1=auth only, 2=decrypt only (Rx) */
} ca_sa_db_flags_t;

/*SAK definition*/
typedef struct ca_macsec_sa_mask_s {
	ca_uint32_t	key0_valid : 1;			/* key 0 valid flag */
	ca_uint32_t	key1_valid : 1;			/* key 1 valid flag */
	ca_uint32_t	key2_valid : 1;			/* key 2 valid flag */
	ca_uint32_t	key3_valid : 1;			/* key 3 valid flag */
	ca_uint32_t	lapn_valid : 1;
	ca_uint32_t	npn_valid : 1;
	ca_uint32_t	sa_db_control_flags_valid : 1;
	ca_uint32_t	sci_valid : 1;
	ca_uint32_t	pn_threshold_valid : 1;
	ca_uint32_t	replay_window_valid : 1;
	ca_uint32_t	mac_addr : 1;
} ca_macsec_sa_db_mask_t;

typedef struct {
	ca_macsec_sa_db_mask_t	mask;

	ca_uint8_t	key0[MACSEC_KEY_LENGTH];	/* key0 distributed by key server */
	ca_uint8_t	key1[MACSEC_KEY_LENGTH];	/* key1 distributed by key server */
	ca_uint8_t		key2[MACSEC_KEY_LENGTH];	/* key2 distributed by key server */
	ca_uint8_t		key3[MACSEC_KEY_LENGTH];	/* key3 distributed by key server */

	ca_uint32_t	lapn;				/* least acceptable packet number.
							 * For downstream SA, this is used for decryption.
							 * For upstream SA, this is used as a PN for encryption.
							 */
	ca_uint32_t	npn;				/* expected next packet number for decryption */
	ca_sa_db_flags_t	sa_db_control_flags;		/* SA_DB control flags */

	/* NOTE: sci field in SecTag is optional (SecTag can be 8 or 16 bytes) */
	ca_uint64_t	sci;				/* SecTag control fields - MAC + Port ID
							 * SCI is 64 bits
							 * SCI bits: 15:0 ==> port id
							 * SCI bits: 63:16 ==> MAC address
							 */

	ca_uint32_t	pn_threshold;			/* packet number threshold for SA0, 1 */
	ca_uint32_t	replay_window;			/* replay window for replay checking */
	ca_mac_addr_t	mac_addr;			/* SRC_MAC address to use on port.
							 * For EPON LLID, this field is optional,
							 * HW will use MPCP MAC address of ONU.
							 */
} ca_macsec_sa_db_t;

/* two SAKs supported per SA */
typedef struct {
	ca_macsec_sa_id_t	sa_id;			/* unique ID for SA.
							 * Picked by driver, returned to caller after add.
							 * Caller use this sa_id for get/delete/update.
							 */
	ca_boolean_t		direction;		/* 0=Tx, 1=Rx */
	ca_uint32_t		association_number;	/* current active AN returned by driver for ca_macsec_sa_get */
	ca_macsec_sa_db_t	sa_db;
} ca_macsec_sa_t;

/* MACSec frame count statistics */
typedef struct {
	ca_uint32_t	rx_delayed;			/* received packet number less than lowest acceptable packet number */
	ca_uint32_t	rx_octets_not_encrypted;	/* bytes recovered that are integrity protected, not encrypted */
	ca_uint32_t	rx_octets_encrypted;		/* bytes recovered that are integrity protected, and encrpyted */
	ca_uint32_t	rx_valid;			/* valid frames received */
	ca_uint32_t	rx_invalid;			/* invalid frames received */
} ca_macsec_stats_t;

/* MACSec error frame counte statistics */
typedef struct {
	ca_uint32_t	tx_untagged;			/* frames sent without SecTag */
	ca_uint32_t	tx_too_long;			/* frames sent with length > MTU */

	/* received untagged frames with validateFrames != strict (see 802.1AE) */
	ca_uint32_t	rx_untagged_validate_frames_not_strict;

	/* received frames without sci when validateFrames == strict (see 802.1AE, section 10.6.1) */
	ca_uint32_t	rx_no_sci_validate_frames_strict;

	/* received frames with invalid SecTag, PN=0 or invalid ICV(see 802.1AE) */
	ca_uint32_t	rx_bad_tag;

	/* received frames with unknown SCI, validateFrames != strict, C bit in SecTag = 0 */
	ca_uint32_t	rx_unknown_sci_validate_frames_not_strict_cbit_off;

	/* received frames with validateFrames == strict, or C bit in SecTag = 1 */
	ca_uint32_t	rx_validate_frames_strict_or_cbit_on;

	/* number for received discarded due to HW processing capability */
	ca_uint32_t	rx_overrun;

	/* confidentail offset > payload length */
	ca_uint32_t	rx_invalid_confidential_offset_err;

	/* version flag in TCI != 0 */
	ca_uint32_t	rx_version_number_err_count;

	/* 2 MSBs of SL field of SecTag != 0 */
	ca_uint32_t	rx_two_msbs_of_short_length_field_err;

	ca_uint32_t	rx_authentication_fail_err;
	ca_uint32_t	rx_unknown_llid;

	/* received frames with length < 64 bytes */
	ca_uint32_t	rx_runt;
} ca_macsec_error_t;

ca_status_t ca_macsec_init (
	CA_IN	ca_device_id_t				device_id,
	CA_IN ca_port_id_t				    port,
	CA_IN	ca_macsec_global_config_t		*macsec_global_conf
);

ca_status_t ca_macsec_global_config_get(
	CA_IN	ca_device_id_t 				device_id,
	CA_IN	ca_port_id_t				port,
	CA_OUT	ca_macsec_global_config_t		*macsec_global_conf
);

ca_status_t ca_macsec_sa_add (
	CA_IN	ca_device_id_t				device_id,
	CA_IN_OUT ca_macsec_sa_t			*sa_context
);

ca_status_t ca_macsec_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN_OUT ca_macsec_sa_t			*sa_context
);

ca_status_t ca_macsec_sa_delete (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id
);

ca_status_t ca_macsec_sa_update (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_t				*sa_context
);

ca_status_t ca_macsec_sa_active_index_set(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id,
	CA_IN	ca_uint8_t				active_index
);

ca_status_t ca_macsec_sa_active_index_get(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_macsec_sa_id_t			sa_id,
	CA_OUT	ca_uint8_t				*active_index
);

ca_status_t ca_macsec_sa_iterate (
	CA_IN	ca_device_id_t				device_id,
	CA_OUT	ca_iterator_t				*return_entry
);

ca_status_t ca_macsec_port_sa_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN		ca_mpcp_llid_t			mpcp_llid,
	CA_IN	ca_macsec_sa_id_t			rx_sa_id,
	CA_IN	ca_macsec_sa_id_t			tx_sa_id
);

ca_status_t ca_macsec_port_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN		ca_mpcp_llid_t			mpcp_llid,
	CA_OUT	ca_macsec_sa_id_t			*rx_sa_id,
	CA_OUT	ca_macsec_sa_id_t			*tx_sa_id
);

ca_status_t ca_macsec_port_mc_sa_set (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid,
	CA_IN	ca_macsec_sa_id_t			mc_sa_id
);

ca_status_t ca_macsec_port_mc_sa_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_sa_id_t			*mc_sa_id
);

ca_status_t ca_macsec_port_stats_get(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_stats_t			*counter
);

ca_status_t ca_macsec_port_stats_clear(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid
);

ca_status_t ca_macsec_port_error_get (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid,
	CA_OUT	ca_macsec_error_t			*counter
);

ca_status_t ca_macsec_port_error_clear (
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t				mpcp_llid
);

ca_status_t ca_macsec_port_hw_decryption_wa(
    CA_IN   ca_device_id_t          device_id,
    CA_IN   ca_port_id_t            port,
    CA_IN   ca_uint8_t              said
);

#endif /* _MACSEC_H_ */
