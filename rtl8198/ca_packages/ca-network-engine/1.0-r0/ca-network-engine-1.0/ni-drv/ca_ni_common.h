#ifndef __CA_NI_COMMON_H__
#define __CA_NI_COMMON_H__

typedef enum {
        HEADER_TYPE_UC = 0,
        HEADER_TYPE_MC,
        HEADER_TYPE_CPU_BOUND,
        HEADER_TYPE_PTP
} ca_ni_hdr_type_t;

typedef struct {
    uint16_t eth_type;
    uint8_t  * eth_type_name;
}ca_eth_type_name_t;
/* Map to Mdata field of Header_I; host order. */
typedef union {
	uint64_t bits64;
	struct {
		uint32_t mdata_h		: 32;
		uint32_t lpm_result		: 12;	/* lpm_result key to NHOP */
		uint32_t reserved_0		: 4;
		uint32_t l3_egress_if_id	: 8;	/* L3_egress_IF_ID */
		uint32_t l3_ingress_if_id	: 8;	/* L3_ingress_IF_ID */
	} __attribute__((packed)) mdata_uc;
	struct {
		uint32_t mdata_h		: 32;
		uint32_t mc_fib_id		: 8;	/* FIB ID */
		uint32_t reserved_0		: 8;
		uint32_t l3_egress_if_id	: 8;	/* L3_egress_IF_ID */
		uint32_t l3_ingress_if_id	: 8;	/* L3_ingress_IF_ID */
	} __attribute__((packed)) mdata_mc;
	struct {
		uint32_t hash_dbl_chk_fail	: 3;
		uint32_t hash_idx		: 21;
		uint32_t cls_action		: 12;
		uint32_t fwd_vld		: 9;
		uint32_t l2l3_flag		: 5;
		uint32_t l4_offset		: 7;
		uint32_t l3_offset		: 7;
	} __attribute__((packed)) hdr_i_info;
	struct {
		uint32_t mdata_h		: 32;
		uint32_t mdata_l		: 32;
	} __attribute__((packed)) mdata_raw;
} HEADER_CPU_T;

typedef union {
        uint32_t bits32[4];
        struct header_ptp {
                uint64_t correction_l           : 32;   /*
                                                         * When cmd_mode is 0, this field is 0
                                                         * When cmd_mode is 1, this is part of 64 bit correction field
                                                         */
                uint64_t correction_h           : 32;   /*
                                                         * When cmd_mode is 0, this is the ns part of the TimeStamp in packet
                                                         * When cmd_mode is 1, this is part of 64 bit correction field
                                                         */
                uint64_t rx_time_stamp          : 32;   /* PTP timer time latched by ingress PTP process packet SOP.
                                                         * LSB align to 16ns
                                                         */
                uint64_t cmd_op                 : 2;    /* SW command:
                                                         * 00: no op
                                                         * 01: timestamp correct
                                                         * 10: timestamp and asymmetry with (+) sign
                                                         * 11: timestamp and asymmetry with (-) sign
                                                         */
                uint64_t cmd_asel               : 2;    /* SW Command Asymmetry Offset select */
                uint64_t cmd_mode               : 1;    /* If set, 32-bit timestamp is filled to the field starting with sw_cmd_offset.
                                                         * Software is expected to provide 48-bit timer second field.
                                                         * When this bit is 0, correction field is calculated, and saved to field start
                                                         * with sw_cmd_offset.
                                                         */
                uint64_t cmd_rewrite            : 1;    /* If set enables the packet CorrectionField/TimeStamp to be rewritten */
                uint64_t ptp_type               : 2;    /* Indicated PTP type:
                                                         * 00: PTP over IEEE802.3 /Ethernet
                                                         * 01: CFM
                                                         * 10: PTP over UDP over IPv4
                                                         * 11: PTP over UDP over IPv6
                                                         */
                uint64_t udp_chksum_offset      : 7;    /* Valid when ptp_type indicates UDP type.  Byte offset of the UDP checksum */
                uint64_t reserved_0             : 1;
                uint64_t cmd_offset             : 7;    /* Indicates position of correction field or timestamp in the packet */
                uint64_t reserved_1             : 9;
        } bits;
} HEADER_PTP_T;

#endif /* __CA_NI_COMMON_H__ */
