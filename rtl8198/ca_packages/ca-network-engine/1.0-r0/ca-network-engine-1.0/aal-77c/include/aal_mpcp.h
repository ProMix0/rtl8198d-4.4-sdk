
/*
 * $Id: aal_mpcp.h,v 1.1.2.15 2015/03/20 07:53:02 jli Exp $
 */

#ifndef __AAL_MPCP_H__
#define __AAL_MPCP_H__

#include "ca_types.h"

#define AAL_EPON_LLID_NUM_MAX   32
#define AAL_EPON_LLID_INDEX_MAX   63 /* 32 ucast llid + 32 mcast llid */

/* reg_mode:
    0: No LLID registration after ONU bootup
    1: Register all LLIDs as specified in CFG_ID_PON_LLID_NUM if M-LLID mode is enabled (CFG_ID_PON_LLID_MODE = 1), or
    register the base LLID if M-LLID mode is disabled (CFG_ID_PON_LLID_MODE = 0)
    2: Only register the base LLID after ONU bootup. If M-LLID mode is enabled, ONU will wait for OLT instruction in
    OAM (0x07/0x7FF1) to only register the LLIDs specified in the OAM
    */
#define AAL_EPON_LLID_REG_MODE_MANUAL    0
#define AAL_EPON_LLID_REG_MODE_AUTO      1
#define AAL_EPON_LLID_REG_MODE_OAM       2

/* mpcp trace level */
typedef enum
{
    AAL_EPON_MPCP_TRACE_LOG = 0x1,
    AAL_EPON_MPCP_TRACE_MIN = 0x2,
    AAL_EPON_MPCP_TRACE_MAJ = 0x4,
    AAL_EPON_MPCP_TRACE_CRI = 0x8,
    AAL_EPON_MPCP_TRACE_ALL = 0xf
}aal_epon_mpcp_trace_level_t;

typedef enum {
    AAL_MPCP_REG_MODE_OFF                  = 0, /* No LLID registration after ONU bootup */
    AAL_MPCP_REG_MODE_AUTO                 = 1,  /* Register all LLIDs as specified in CFG_ID_PON_LLID_NUM if M-LLID mode is enabled (CFG_ID_PON_LLID_MODE = 1), or
                                                    register the base LLID if M-LLID mode is disabled (CFG_ID_PON_LLID_MODE = 0) */
    AAL_MPCP_REG_MODE_MANUAL               = 2  /* Only register the base LLID after ONU bootup. If M-LLID mode is enabled, ONU will wait for OLT instruction in
                                                    OAM (0xD7/0x7FF1) to only register the LLIDs specified in the OAM */
} aal_mpcp_reg_mode_t;

typedef enum {
    AAL_MPCP_LLID_MODE_SINGLE              = 0, /* single LLID mode */
    AAL_MPCP_LLID_MODE_MULTIPLE            = 1  /* multiple LLID mode */
} aal_mpcp_llid_mode_t;

typedef enum
{
    AAL_MPCP_REQ_LLID_ASSIGNED             = 0, /* use assigned LLID value as LLID of MPCP Register Request Frame */
    AAL_MPCP_REQ_LLID_0X7FFE               = 1  /* use 0x7FFE as LLID of MPCP Register Request Frame */
}aal_mpcp_req_llid_t;

typedef enum
{
    AAL_MPCP_RPT_S_8Q_2S                    = 0, /* MPCP Report Mode: Single LLID, 8 queues, 2 sets */
    AAL_MPCP_RPT_S_8Q_1S                    = 1, /* MPCP Report Mode: Single LLID, 8 queues, 1 sets */
    AAL_MPCP_RPT_B_1Q_1S                    = 2, /* MPCP Report Mode: Single LLID or Multiple LLID, 1 queue, 1 set */
    AAL_MPCP_RPT_B_1Q_2S                    = 3, /* MPCP Report Mode: Single LLID or Multiple LLID, 1 queue, 2 set2 */
    AAL_MPCP_RPT_M_1Q_3S                    = 4, /* MPCP Report Mode: Multiple LLID, 1 queue, 3 sets */
    AAL_MPCP_RPT_M_1Q_4S                    = 5  /* MPCP Report Mode: Multiple LLID, 1 queue, 4 sets */
}aal_mpcp_rpt_mode_t;

typedef enum
{
    AAL_MPCP_PENDING_GRANT_1                = 1, /* support maximum mpcp pending grant number is 1 */
    AAL_MPCP_PENDING_GRANT_2                = 2, /* support maximum mpcp pending grant number is 2 */
    AAL_MPCP_PENDING_GRANT_3                = 3, /* support maximum mpcp pending grant number is 3 */
    AAL_MPCP_PENDING_GRANT_4                = 4, /* support maximum mpcp pending grant number is 4 */
    AAL_MPCP_PENDING_GRANT_5                = 5, /* S_LLID ONLY: support maximum mpcp pending grant number is 5 */
    AAL_MPCP_PENDING_GRANT_6                = 6, /* S_LLID ONLY: support maximum mpcp pending grant number is 6 */
    AAL_MPCP_PENDING_GRANT_7                = 7, /* S_LLID ONLY: support maximum mpcp pending grant number is 7 */
    AAL_MPCP_PENDING_GRANT_8                = 8  /* S_LLID ONLY: support maximum mpcp pending grant number is 8 */
} aal_mpcp_pending_grant_t;

typedef enum
{
    AAL_MPCP_TIMEDRIFT_THRESHOLD_8TQ        = 8, /* MPCP time drift shreshold: 8 TQ */
    AAL_MPCP_TIMEDRIFT_THRESHOLD_16TQ       = 16, /* MPCP time drift shreshold: 16 TQ */
    AAL_MPCP_TIMEDRIFT_THRESHOLD_32TQ       = 32, /* MPCP time drift shreshold: 32 TQ */
    AAL_MPCP_TIMEDRIFT_THRESHOLD_63TQ       = 63  /* MPCP time drift shreshold: 63 TQ */
}aal_mpcp_tdrift_thresh_t;

typedef enum
{
    AAL_MPCP_REG_INIT_SW                    = 0, /* Software triger register tring progress */
    AAL_MPCP_REG_INIT_HW                    = 1  /* Hardware triger register tring progress */
}aal_mpcp_reg_init_t;

typedef enum
{
    AAL_MPCP_DEREG_UNKNOWN              = 0, /* no deregistration from last time (or last clear) */
    AAL_MPCP_DEREG_TIMEDRFT             = 1, /* caused by time drift */
    AAL_MPCP_DEREG_OLT                  = 2, /* caused by remote OLT deregistration */
    AAL_MPCP_DEREG_LOCAL                = 3, /* caused by local deregistration */
    AAL_MPCP_DEREG_REREG                = 4, /* caused by receiving re-reg flag */
    AAL_MPCP_DEREG_TIMEOUT              = 5, /* caused by MPCP timeout */
    AAL_MPCP_DEREG_GATE                 = 6, /* caused by gate timer timeout */
    AAL_MPCP_DEREG_SW                   = 7, /* caused by software disbling the MPCP */
} aal_mpcp_dereg_cause_t;

typedef enum
{
    AAL_MPCP_STAT_INIT_DEREG            = 0, /* initial or deregistered */
    AAL_MPCP_STAT_SEND_REQ              = 1, /* MAC already sends out Request frame */
    AAL_MPCP_STAT_RECV_REG              = 2, /* MAC already receives Register frame */
    AAL_MPCP_STAT_SEND_ACK              = 3, /* MAC already sends out Acknowledge frame */

} aal_mpcp_stat_t;

typedef enum
{
    AAL_MPCP_LLID_ENTRY_DEL             = 0,
    AAL_MPCP_LLID_ENTRY_ADD             = 1,
}aal_mpcp_llid_entry_oper_t;

typedef enum
{
    AAL_MPCP_LLID_ENTRY_ADD_FAIL_FULL       = 1,
    AAL_MPCP_LLID_ENTRY_DEL_FAIL_NO_MATCH   = 2,
    AAL_MPCP_LLID_ENTRY_DEL_SUCCESS         = 5,
    AAL_MPCP_LLID_ENTRY_ADD_SUCCESS         = 6,
    AAL_MPCP_LLID_ENTRY_CFG_SUCCESS         = 7,

}aal_mpcp_llid_entry_sts_t;

typedef struct {
    ca_uint8_t                    llid_mode                  ; /* 1-multiple LLID, 0-Single LLID */
    ca_uint8_t                    request_frame_llid         ; /* 1-send register request frame with assiged LLID, otherwise with 0x7FFE */
    ca_uint8_t                    mpcp_report_mode           ; /* 0-S/8/2, 1-S/8/1, 2-B/1/1, 3-B/1/2, 4-M/1/3, 5-M/1/4 ((SLLID/MLLID/Both)/queue/set mode) */
    ca_uint8_t                    pending_grant              ; /* Maximum pending grant number support. */
    ca_boolean_t                  grant_fifo_dereg_clr       ; /* 1-flush out used grant from grant fifo when ONU deregister */
    ca_uint8_t                    min_grant_size_1g          ; /* used for grant validation, not include laser-on, laser-off, synctime and FEC overhead, default is 42TQ, max = 0xff, unit: TQ */
    ca_uint8_t                    fec_overhead_1g            ; /* used for grant validation, for mpcp report message FEC overhead, default is 15TQ, max = 0xff, unit: TQ */
    ca_uint8_t                    tx_burst_overhead_10g      ; /* Includle 2 IDLE blocks which follow burst delimiter and 3 burst terminator blocks, default is 2TQ, max = 0xf, unit: TQ */
    ca_uint8_t                    min_grant_size_10g         ; /* Minimum valid bandwidth for u/s except overhead, default is (1*FEC_CW - 2*66)/160 TQ, max = 0xff, unit: TQ  */
    ca_boolean_t                  min_process_time_check_en  ; /* 1-check grant delay for ONU minimum processing time(1024TQ in future) */
    ca_uint16_t                   min_process_time           ; /* The minimum valid time of a grant from receiving (time stamp) to window start, default is 1024TQ, max = 0xfff, unit TQ*/
    ca_uint16_t                   max_future_grant_time      ; /* The maximum valid time of a grant from receiving (time stamp) to window start, default is 0x3ff, max = 0x3ff, unit? */
    ca_uint16_t                   mpcptimer_threshold        ; /* Maximum time not receive gate for trigering event. unit 0.1ms  */
    ca_uint16_t                   gatetimer_threshold        ; /* Maximum time not receive gate for trigering event. unit 0.1ms  */
    ca_boolean_t                  gatetimer_dereg_en         ; /* Enable ONU de-register when gate timer timeout */
    ca_boolean_t                  mpcptimer_dereg_en         ; /* Enable ONU de-register when mpcp timer timeout */
    ca_boolean_t                  timedrift_dereg_en         ; /* Enable ONU de-register when time drift beyond time drift threshold */
    ca_uint8_t                    timedrift_threshold        ; /* TimeStamp Drift Checking Threshold: 0-8TQ, 1-16TQ, 2-32TQ, 3-63TQ */
    ca_boolean_t                  event_force_report_en      ; /* Enable ONU send out event MPCP report when report timer timeout */
    ca_uint32_t                   reporttimer_threshold      ; /* Maximum time not receive gate with force report for trigering report notification, default is 32ms, max = 0xfff unit 0.1ms */
    ca_boolean_t                  register_timeout_en        ; /* 1-enable register pending timeout to re-discovery, 0-always waiting */
    ca_uint32_t                   register_timer             ; /* maximum waiting time for granting-for-ACK after receiving register message, default is 0xfffff, max = 0xfffff, unit? */
    ca_boolean_t                  b2b_grant_en               ; /* 1-enable back to back grant by merging them into one laser window (Single LLID only) */
    ca_boolean_t                  b2b_force_report_en        ; /* 0-disable force report in back to back grants */
    ca_uint8_t                    register_initial_mode      ; /* 0-Software triger register tring progress, 1-hardware triger register tring progress */
    ca_boolean_t                  auto_discovery_en          ; /* 0-disable auto discovery function, for DEBUG  */
    ca_boolean_t                  direct_registration_en     ; /* 1-alow directly register, 0-follow stardard flow, for DEBUG  */
    ca_uint8_t                    laser_on_time              ; /* configure laser on time, unit is TQ                                         */
    ca_uint8_t                    laser_off_time             ; /* configure laser off time, unit is TQ                                        */
    ca_boolean_t                  mpcp_rx_ext_en             ; /* 1 - Indicates receive MPCP extended frame , 0 -not.   */
    ca_boolean_t                  mpcp_tx_ext_en             ; /* 1-  Indicates permit CPU transmitting MPCP extended frame, 0 -not. */
    ca_boolean_t                  mpcp_to_cpu_en             ; /* 1 - Indicates forward MPCP extended frame to CPU, 0 - not.  */
	ca_boolean_t				  fake_bc_llid_pass		     ; /* Control option for d/s frame with {mode, LLID[14:0]} is 0x7FFF (1G) or 0x7FFE(10G).*/
	ca_uint32_t                   en_local_offline           ;														   /* 1 - Indicate EPON MAC will accept frames with such LLID value */
} aal_mpcp_cfg_t;


typedef union {
    struct {
        ca_uint64             llid_mode                  : 1;
        ca_uint64             request_frame_llid         : 1;
        ca_uint64             mpcp_report_mode           : 1;
        ca_uint64             pending_grant              : 1;
        ca_uint64             grant_fifo_dereg_clr       : 1;
        ca_uint64             min_grant_size_1g          : 1;
        ca_uint64             fec_overhead_1g            : 1;
        ca_uint64             tx_burst_overhead_10g      : 1;
        ca_uint64             min_grant_size_10g         : 1;
        ca_uint64             min_process_time_check_en  : 1;
        ca_uint64             min_process_time           : 1;
        ca_uint64             max_future_grant_time      : 1;
        ca_uint64             mpcptimer_threshold        : 1;
        ca_uint64             gatetimer_threshold        : 1;
        ca_uint64             gatetimer_dereg_en         : 1;
        ca_uint64             mpcptimer_dereg_en         : 1;
        ca_uint64             timedrift_dereg_en         : 1;
        ca_uint64             timedrift_threshold        : 1;
        ca_uint64             event_force_report_en      : 1;
        ca_uint64             reporttimer_threshold      : 1;
        ca_uint64             register_timeout_en        : 1;
        ca_uint64             register_timer             : 1;
        ca_uint64             b2b_grant_en               : 1;
        ca_uint64             b2b_force_report_en        : 1;
        ca_uint64             register_initial_mode      : 1;
        ca_uint64             auto_discovery_en          : 1;
        ca_uint64             direct_registration_en     : 1;
        ca_uint64             laser_on_time              : 1;
        ca_uint64             laser_off_time             : 1;
        ca_uint64             mpcp_rx_ext_en             : 1;
        ca_uint64             mpcp_tx_ext_en             : 1;
        ca_uint64             mpcp_ext_to_cpu_en         : 1;
		ca_uint64 			  fake_bc_llid_pass          : 1;
		ca_uint64 			  en_local_offline           : 1;
    } s;
    ca_uint64 u64;
} aal_mpcp_msk_t;

typedef struct {
    ca_boolean_t                  register_permit          ; /* 1-permit the virtual LLID register, 0-not permit registering*/
    ca_boolean_t                  offline                  ; /* Deregister the virtual LLID, 1-deregister/non-register, 0-enable register */
    ca_boolean_t                  grant_permit             ; /* 1-Allow the virtual LLID handle the received grant, 0-not handle the grant */
    ca_boolean_t                  queue_id                 ; /* Assign a queue to the virtual LLID */
    ca_boolean_t                  sch_oam_en               ; /* 1-enable schedule OAM frame from the virtual LLID queue out, 0-stop schedule OAM frame from it */
    ca_boolean_t                  sch_data_en              ; /* 1-enable schedule data frame from the virtual LLID queue out, 0-stop schedule data frame from it */
    ca_boolean_t                  rpt_oam_en               ; /* 1-enable report OAM packet length in the virtual LLID report message, 0-not report OAM packet length  */
    ca_boolean_t                  rpt_data_en              ; /* 1-enable report data packet length in the virtual LLID report message, 0-not report data packet length  */
    ca_boolean_t                  gatetimer_dereg_en       ; /* Enable LLID de-register when gate timer timeout */
    ca_boolean_t                  mpcptimer_dereg_en       ; /* Enable LLID de-register when mpcp timer timeout */
    ca_boolean_t                  timedrift_dereg_en       ; /* Enable LLID de-register when time drift beyond time drift threshold */
    ca_boolean_t                  keep_silence_en          ; /* 1-enable, 0-disable */
    ca_boolean_t                  period_report_en         ; /* 1-enable, 0-disable */
    ca_boolean_t                  mpcptimer_intrp_en       ; /*  Enable LLID interrupt when mpcp timer timeout */
    ca_boolean_t                  gatetimer_intrp_en       ; /*  Enable LLID interrupt when gate timer timeout */
    ca_boolean_t                  timedrift_intrp_en       ; /*  Enable LLID interrupt when time drift beyond time drift threshold */
    ca_uint32_t                   keep_silence_time          ; /* Configure keep silence time, default is 60s, max = 0xffffff, Unit is 0.1ms */
} aal_mpcp_llid_cfg_t;

typedef union {
    struct {
         ca_uint32_t             register_permit         : 1;
         ca_uint32_t             offline                 : 1;
         ca_uint32_t             grant_permit            : 1;
         ca_uint32_t             queue_id                : 1;
         ca_uint32_t             sch_oam_en              : 1;
         ca_uint32_t             sch_data_en             : 1;
         ca_uint32_t             rpt_oam_en              : 1;
         ca_uint32_t             rpt_data_en             : 1;
         ca_uint32_t             gatetimer_dereg_en      : 1;
         ca_uint32_t             mpcptimer_dereg_en      : 1;
         ca_uint32_t             timedrift_dereg_en      : 1;
         ca_uint32_t             keep_silence_en         : 1;
         ca_uint32_t             period_report_en        : 1;
         ca_uint32_t             mpcptimer_intrp_en      : 1;
         ca_uint32_t             gatetimer_intrp_en      : 1;
         ca_uint32_t             timedrift_intrp_en      : 1;
         ca_uint32_t             keep_silence_time       : 1;
    } s;
    ca_uint32_t u32;
} aal_mpcp_llid_msk_t;




typedef struct
{
    ca_uint32_t mpcp_pkt_rx;      /* Indicates received MPCP packet number from 10G RxMAC.  */
    ca_uint32_t mpcp_ext_rx;      /* Indicates received extended MPCP packet number from 10G RxMAC */
    ca_uint32_t dscvry_rx_drop;   /* Indicates dropped discovery packet because of discovery information mismatch between OLT and ONU. */
    ca_uint32_t ext_rx_drop;      /* Indicates dropped extended MPCP packet because of the overflow of buffer  */
    ca_uint32_t grant_rx_drop[8]; /* Indicates the dropped MPCP grant number. System defined eight kinds of drop reasons*/
    ca_uint32_t mpcp_pkt_tx;      /* Indicates transmitted MPCP packet number to 10G TxMAC or 1G TxMAC. */
    ca_uint32_t mpcp_ext_tx;      /* Indicates transmitted extended MPCP packet number to 10G TxMAC or 1G TxMAC.  */
}aal_mpcp_mib_stats_t;

typedef struct
{
    ca_uint8_t  mac_idx;
    ca_uint8_t  llid_idx;
    ca_uint16_t llid_value;
    ca_boolean_t mcast;
}aal_mpcp_llid_entry_t;

typedef struct
{
    ca_boolean_t enable;
    ca_uint32_t  timer_value;
    ca_uint32_t  timer_id;
    ca_boolean_t status;
}aal_mpcp_llid_silence_t;

/**/
typedef struct
{
    ca_boolean_t valid;
    ca_llid_t    llid;
}aal_mpcp_mc_llid_t;

#define AAL_MPCP_MC_LLID_OFFSET   32
#define AAL_MPCP_MAX_MC_LLID_NUMS 32
#define AAL_MPCP_BC_LLID          0xffff
#define AAL_MPCP_INVALID_LLID     0xffff


ca_status_t aal_mpcp_mc_llid_add
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_mpcp_llid_t llid,
    CA_OUT ca_llid_t      *llid_instance
);

ca_status_t aal_mpcp_mc_llid_del
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_mpcp_llid_t llid
);

ca_status_t aal_mpcp_mc_llid_list
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *mc_llid_nums,
    CA_OUT ca_mpcp_llid_t *mc_llids
);

ca_status_t aal_mpcp_debug_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  level
);

ca_status_t aal_mpcp_debug_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_uint32_t  *level
);

ca_status_t aal_mpcp_cfg_set
(
    CA_IN  ca_device_id_t    device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  aal_mpcp_msk_t msk,
    CA_IN aal_mpcp_cfg_t  *cfg
);

ca_status_t aal_mpcp_cfg_get
(
    CA_IN  ca_device_id_t    device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT aal_mpcp_cfg_t *cfg
);

ca_status_t aal_mpcp_rx_ext_ctrl_set /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t en_rx_fwd,
    CA_IN ca_boolean_t en_rx_ext
);

ca_status_t aal_mpcp_rx_ext_ctrl_get /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t   port_id,
    CA_OUT ca_boolean_t *en_rx_fwd,
    CA_OUT ca_boolean_t *en_rx_ext
);

ca_status_t aal_mpcp_llid_cfg_set
(
    CA_IN  ca_device_id_t             device_id,
    CA_IN  ca_port_id_t            port_id,
    CA_IN  ca_uint8_t              index,
    CA_IN  aal_mpcp_llid_msk_t     mask,
    CA_IN  aal_mpcp_llid_cfg_t     *cfg
);

ca_status_t aal_mpcp_llid_cfg_get
(
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_port_id_t        port_id,
    CA_IN  ca_uint8_t          index,
    CA_OUT aal_mpcp_llid_cfg_t *cfg
);

ca_status_t aal_mpcp_llid_mode_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port,
    CA_IN ca_uint32 mode
);

ca_status_t aal_mpcp_llid_mode_get
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port,
    CA_OUT ca_uint32   *mode
);

ca_status_t aal_mpcp_mac_addr_set
(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_port_id_t  port_id,
    CA_IN ca_uint8_t    index,
    CA_IN ca_mac_addr_t mac
);

ca_status_t aal_mpcp_mac_addr_get
(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     index,
    CA_OUT ca_mac_addr_t mac
);

ca_status_t aal_mpcp_report_threshold_set
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_IN  ca_uint8_t   set_id,
    CA_IN  ca_uint8_t   queue_id,
    CA_IN  ca_uint16_t  threshold
);

ca_status_t aal_mpcp_report_threshold_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_IN  ca_uint8_t   set_id,
    CA_IN  ca_uint8_t   queue_id,
    CA_OUT ca_uint16_t  *threshold
);

ca_status_t aal_mpcp_reg_start_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t   llid_idx,
    CA_IN ca_uint8_t   mac_idx
);

ca_status_t aal_mpcp_reg_start_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_uint8_t   llid_idx,
    CA_OUT ca_uint8_t   *mac_idx
);


ca_status_t aal_mpcp_reg_toggle
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t   llid_idx,
    CA_IN ca_uint8_t   mac_idx
);

ca_status_t aal_mpcp_reg_llid_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_IN  ca_uint8_t   index,
    CA_OUT ca_uint16_t  *llid
);

ca_uint8_t aal_mpcp_llid_index_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_llid_t      llid
);

ca_status_t aal_mpcp_reg_llid_index_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_IN  ca_uint16_t  llid,
    CA_OUT  ca_uint8_t  *index
);

ca_status_t aal_mpcp_llid_entry_set (/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_IN  aal_mpcp_llid_entry_oper_t oper,
    CA_IN  aal_mpcp_llid_entry_t *entry
);

ca_status_t aal_mpcp_cmd_llid_entry_set
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint32_t    oper,
    CA_IN  ca_uint8_t     mac_idx,
    CA_IN  ca_uint16_t    llid_value,
    CA_IN  ca_boolean_t   mc_flag
);

ca_status_t aal_mpcp_cmd_llid_entry_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint32_t    oper,
    CA_IN  ca_uint8_t     mac_idx,
    CA_IN  ca_uint16_t    llid_value,
    CA_OUT ca_boolean_t   *mc_flag
);

ca_status_t aal_mpcp_llid_entry_all_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id
);

ca_status_t aal_mpcp_reg_status_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint8_t     index,
    CA_OUT ca_boolean_t  *reg
);

ca_status_t aal_mpcp_reg_flag_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     index,
    CA_OUT ca_uint8_t     *flag
);

ca_status_t aal_mpcp_dereg_cause_get
(
    CA_IN  ca_device_id_t            device_id,
    CA_IN  ca_port_id_t           port_id,
    CA_IN  ca_uint8_t             index,
    CA_OUT aal_mpcp_dereg_cause_t *cause
);

ca_status_t aal_mpcp_local_time_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_uint32_t  *time
);

ca_status_t aal_mpcp_reg_sync_time_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_IN  ca_uint8_t   index,
    CA_OUT ca_uint16_t  *time
);

ca_status_t aal_mpcp_reg_laser_time_status_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     index,
    CA_OUT ca_uint16_t    *laser_on_time,
    CA_OUT ca_uint16_t    *laser_off_time
);

ca_status_t aal_mpcp_reg_retry_times_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint8_t     index,
    CA_OUT ca_uint16_t    *count
);

ca_status_t aal_mpcp_local_discovery_info_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_uint16_t  *info
);

ca_status_t aal_mpcp_local_remote_info_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_uint16_t  *info
);

ca_status_t aal_mpcp_mib_stats_get
(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_OUT aal_mpcp_mib_stats_t *mib
);

ca_status_t aal_mpcp_pca_olt_mac_addr_set
(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_port_id_t  port_id,
    CA_IN ca_mac_addr_t mac
);

ca_status_t aal_mpcp_pca_olt_mac_addr_get
(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_port_id_t  port_id,
    CA_OUT ca_mac_addr_t mac
);

ca_status_t aal_mpcp_olt_mac_addr_get
(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_port_id_t  port_id,
    CA_OUT ca_mac_addr_t mac
);

void aal_epon_mpcp_reg_change_handler /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_uint32_t cookie
);

void aal_epon_mpcp_timeout_handler /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_uint32_t cookie
);

void aal_epon_mpcp_gate_timeout_handler /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_uint32_t cookie
);

void aal_epon_mpcp_timestamp_drift_handler /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_uint32_t cookie
);

void aal_epon_mpcp_rx_reg_frame_handler /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_uint32_t cookie
);

ca_status_t aal_mpcp_registration_restart /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_uint8_t llid_idx
);

ca_uint8_t aal_mpcp_reg_mode_get/*CODEGEN_IGNORE_TAG*/
(
	CA_IN ca_device_id_t  device_id
);

ca_uint8_t aal_mpcp_llid_num_get /*CODEGEN_IGNORE_TAG*/
(
	CA_IN ca_device_id_t  device_id
);

ca_status_t aal_mpcp_reg_llid_add_all/*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t  device_id
);

ca_status_t aal_mpcp_reg_llid_del_all/*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t  device_id
);

ca_status_t aal_mpcp_reg_llid_add/*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t llid_bmp
);

ca_status_t aal_mpcp_reg_llid_del/*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t llid_bmp
);

ca_status_t aal_mpcp_init  /*CODEGEN_IGNORE_TAG*/
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id
);

ca_status_t aal_mpcp_llid_silence_status_get/*CODEGEN_IGNORE_TAG*/
(
    ca_device_id_t device_id,
    ca_uint8_t llid_idx,
    ca_boolean_t *status
);

#endif /* End if __AAL_MPCP_H__ */

