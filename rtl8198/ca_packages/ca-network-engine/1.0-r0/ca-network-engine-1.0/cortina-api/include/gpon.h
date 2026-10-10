#ifndef __GPON_H__
#define __GPON_H__

#include "osal_common.h"
#include "log.h"
#include "pon.h"
#include "rate.h"

#define CA_MOD_XGPN_MAC 				1

#define CA_API_DEBUG(arg,...) cap_log_msg(0, CA_MOD_XGPN_MAC,CAP_LOG_DEBUG,"%s():"arg, __FUNCTION__,##__VA_ARGS__)

#define CA_API_ERROR(arg,...) cap_log_msg(0,CA_MOD_XGPN_MAC, CAP_LOG_ERROR,"%s():"arg, __FUNCTION__,##__VA_ARGS__)
#if defined(CONFIG_ARCH_CORTINA_VENUS) 
#define CA_GPON_MAX_GEM_PORTS        	256
#else
#define CA_GPON_MAX_GEM_PORTS        	128
#endif

#define CA_GPON_OMCC_GEM_PORT_START     0
#define CA_GPON_OMCC_GEM_PORT_END       7
#define CA_GPON_USR_DATA_GEM_PORT_START 8
#define CA_GPON_USR_DATA_GEM_PORT_END   (CA_GPON_MAX_GEM_PORTS - 1)
#define CA_GPON_MAX_TCONT_NUM      		32

#if defined(CONFIG_ARCH_CORTINA_VENUS) 
#define CA_GPON_MAX_QUEUE_NUM      		256
#else
#define CA_GPON_MAX_QUEUE_NUM      		128
#endif

#define CA_GPON_MAX_NUM_FLOW_POL  		64
#define CA_OMCI_VOQ_NUM                 8
#define CA_MAX_QUEUE_NUM_PER_TCONT_SCH  8
#define CA_GPON_MAX_SCH_WEIGHT      	255
#define CA_GPON_MAC_GEM_PORTS_MAX       64
#define CA_GPON_MAC_QUEUE_NUM_MAX       64
#define CA_GPON_ALLOC_ID_MAX            0xFFF
#define CA_GPON_OMCI_TCONT_ID           0


#define CA_GPON_US_PRI_Q_ID_BASE        0x8000
#define CA_GPON_DS_PRI_Q_ID_BASE        0x0000
#define CA_MAX_NUM_FLOW_POL  			64
#define CA_MAX_NUM_UNI_QUEUE            12 /* 8 uni0 and 4 uni1 queue */

#define GPON_OPTICAL_SIGNAL_THRESHOLD_MIN    4
#define GPON_OPTICAL_SIGNAL_THRESHOLD_MAX    10


/* hw tcont index 0 used for default alloc-id on PON0
and index 128 is used for default alloc-id on PON1
 */
#define CA_GPON_OMCI_TCONT_INDEX_PON0    0
#define CA_GPON_OMCI_TCONT_INDEX_PON1    128
#define CA_GPON_INVALID_GEM_PORT_ID      0xFFFF
#define CA_MSK_SIZE                      16

/* for G.984 only, G.987, G.988 merged O2 and O3 into O2_3 */
#define CA_GPON_PORT_ONU_STATES_O1  CA_NGP2_ONU_ACT_STATE_O1_1
#define CA_GPON_PORT_ONU_STATES_O2  CA_NGP2_ONU_ACT_STATE_O1_2
#define CA_GPON_PORT_ONU_STATES_O3  CA_NGP2_ONU_ACT_STATE_O2_3
#define CA_GPON_PORT_ONU_STATES_O4  CA_NGP2_ONU_ACT_STATE_O4
#define CA_GPON_PORT_ONU_STATES_O5  CA_NGP2_ONU_ACT_STATE_O5_1
#define CA_GPON_PORT_ONU_STATES_O6  CA_NGP2_ONU_ACT_STATE_O6
#define CA_GPON_PORT_ONU_STATES_O7  CA_NGP2_ONU_ACT_STATE_O7

typedef enum {
    CA_NGP2_ONU_ACT_STATE_O1_1      = 0,
    CA_NGP2_ONU_ACT_STATE_O1_2      = 1,
    CA_NGP2_ONU_ACT_STATE_O2_3      = 2,
    CA_NGP2_ONU_ACT_STATE_O4        = 3,
    CA_NGP2_ONU_ACT_STATE_O5_1      = 4,
    CA_NGP2_ONU_ACT_STATE_O5_2      = 5,
    CA_NGP2_ONU_ACT_STATE_O6        = 6,
    CA_NGP2_ONU_ACT_STATE_O7        = 7,
    CA_NGP2_ONU_ACT_STATE_O8_1      = 8,
    CA_NGP2_ONU_ACT_STATE_O8_2      = 9,
    CA_NGP2_ONU_ACT_STATE_O9        = 10
} ca_gpon_port_onu_states_t;

/*NOTE: some timers (TO3, TO4, TO5, TO_CPI  are applicable for NGPON2 mode only)*/
typedef enum {
    CA_GPON_PORT_TIMER_TYPE_TOZ             = 1,
    CA_GPON_PORT_TIMER_TYPE_TO1             = 2,        //default 10 sec
    CA_GPON_PORT_TIMER_TYPE_TO2             = 3,        //active when WLCP is enabled
    CA_GPON_PORT_TIMER_TYPE_TO3             = 4,        //active when WLCP is dis
    CA_GPON_PORT_TIMER_TYPE_TO4             = 5,        //O8.1, O8.2 timeout
    CA_GPON_PORT_TIMER_TYPE_TO5             = 6,        //O9 timeout
    CA_GPON_PORT_TIMER_TYPE_CPI             = 7,        //CPI scanning timeout
    CA_GPON_PORT_TIMER_TYPE_TK4             = 8,        //TK4 timer
    CA_GPON_PORT_TIMER_TYPE_TK5             = 9         //TK5 timer

} ca_gpon_port_timer_type_t;


typedef enum {
    CA_GPON_PORT_LASER_MODE_BURST = 0, /* Normal working mode */
    CA_GPON_PORT_LASER_BURST_OPERATION = 1,
    CA_GPON_PORT_LASER_MODE_ON = 2/* Laser always ON mode, for debug use only */
} ca_gpon_port_laser_mode_t;


typedef enum {
    CA_GPON_PORT_REPORT_MODE_0 = 0, /* Mode 0 only */
    CA_GPON_PORT_REPORT_MODE_0_1 = 1, /* Mode 0 and 1 */
    CA_GPON_PORT_REPORT_MODE_NOT_SUPPORTED = 4 /* Piggyback reporting not supported */
} ca_gpon_port_report_mode_t;

typedef enum {
    CA_DIR_NONE = 0x0,
    CA_DIR_US   = 0x1,      /* upstream */
    CA_DIR_DS   = 0x2,      /* downstream */
    CA_DIR_BOTH = (CA_DIR_US|CA_DIR_DS)
} ca_dir_e;

typedef enum {
    CA_WRED_UNDEFINED = 0x0,
    CA_WRED_RFC_4115  = 0x1,
    CA_WRED_RFC_2697  = 0x2,
    CA_WRED_RFC_2698  = 0x4
} ca_wred_mode_t;

typedef enum {
    CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP = 0, /* Strict Priority */
    CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_WRR = 1, /* Weighted Round Robin */
    CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP_WRR = 2 /* SP + WRR */
} ca_gpon_port_tcont_scheduler_mode_t;

typedef enum {
    CA_GPON_PORT_LASER_POLARITY_ACTIVE_LO   = 0,
    CA_GPON_PORT_LASER_POLARITY_ACTIVE_HI   = 1
} ca_gpon_port_laser_polarity_t;

typedef struct {
    ca_uint32_t corrected_bytes;    /* The number of bytes that were corrected by the FEC                                                                      */
    ca_uint32_t corrected_codewords;    /* The codewords that were corrected by the FEC */
    ca_uint32_t uncorrectable_codewords;    /* Error codewords that could not be corrected by the                                                                      FEC */
    ca_uint32_t total_codewords;    /* Total received code words */
} ca_gpon_port_fec_stats_t;

typedef struct {
    ca_uint32_t rx_gem_frame;   /* GEM frames received correctly on the GEM port */
    ca_uint32_t rx_gem_payload; /* User payload bytes received correctly on the GEM port */
    ca_uint32_t rx_gem_crc;     /* Received frames with FCS error on the GEM port */
    ca_uint32_t tx_gem_frame;   /* GEM frames transmitted correctly on the GEM port */
    ca_uint32_t tx_gem_payload; /* User payload bytes transmitted correctly on the GEM port */
} ca_gpon_port_gem_stats_t;

typedef struct {
    ca_uint32_t alloc_id;
    ca_uint32_t tcont_id;
    ca_boolean_t tcont_valid;
} ca_gpon_port_tcont_cfg_t;

typedef struct {
    ca_uint32_t mode;   /* Scheduling mode */
    ca_uint32_t sp_queue_num;   /* Number of SP queue */
    ca_uint32_t weight_queue[8];    /* Weight of queue */
} ca_gpon_port_tcont_scheduler_t;

typedef enum {
    CA_GPON_PORT_SCHEDULER_QUEUE_SCHEDULER_SP = 0, /* Strict Priority */
    CA_GPON_PORT_SCHEDULER_QUEUE_SCHEDULER_WRR = 1, /* Weighted Round Robin */
    CA_GPON_PORT_SCHEDULER_QUEUE_SCHEDULER_SP_WRR = 2 /* SP + WRR */
} ca_gpon_port_scheduler_mode_t;

typedef enum {
    CA_GPON_PORT_GEM_PORT_DIRECTION_UP = 0, /* Upstream */
    CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN = 1, /* Downstream */
    CA_GPON_PORT_GEM_PORT_DIRECTION_BI = 2 /* Bidirection */
} ca_gpon_port_gem_port_direction_t;

typedef enum {
    CA_GPON_PORT_GEM_PORT_ENC_DISABLE = 0, /*  No encryption */
    CA_GPON_PORT_GEM_PORT_ENC_UC_BOTH_DIRECTION = 1, /* Unicast payload encryption in both directions */
    CA_GPON_PORT_GEM_PORT_ENC_BC_MC = 2, /* Broadcast,multicast encryption  */
    CA_GPON_PORT_GEM_PORT_ENC_UC_DS_ONLY = 3 /* Unicast encryption and downstream only */
}ca_gpon_port_gem_port_enc_keyring_t;

typedef struct {
    ca_uint32_t max_tcont_number;   /* MAX T-CONT supported */
    ca_uint32_t max_gem_number; /* MAX GEM port supported */
    ca_uint32_t max_queue_number;   /* MAX queue supported */
    ca_uint32_t  queues_per_tcont; /* MAX queue number per TCONT */
    ca_gpon_port_report_mode_t     report_mode;    /* Reporting mode supported */
} ca_gpon_port_capability_t;

typedef enum {
    CA_GMAC_PIGGYBACK_REPORT_MODE_0_ONLY = 0,
    CA_GMAC_PIGGYBACK_REPORT_MODE_0_AND_1 = 1,
    CA_GMAC_PIGGYBACK_REPORT_MODE_NOT_SUPPORTED = 4
} ca_gmac_piggyback_report_mode_t;

typedef enum {
    CA_GPON_GEM_NO_ENCRYPTION = 0,
    CA_GPON_GEM_UC_PAYLOAD_BI_ENCRYPTION = 1,
    CA_GPON_GEM_MC_ENCRYPTION = 2,
    CA_GPON_GEM_UC_PAYLOAD_DN_ENCRYPTION = 3
} ca_gpon_aes_key_ring_mode_t;
typedef struct {
    ca_uint32_t alloc_id;
    ca_uint32_t tcont_id;
    ca_uint32_t  tcont_subpid;
    ca_gpon_port_tcont_scheduler_mode_t tcont_scheduler;/*one class scheduler ,scheduler mode amony queues in a tcont; two class scheduler ,scheduler mode among schedulers in a tcont.*/
}ca_gpon_port_tcont_t;

typedef struct {
    ca_boolean_t vld;
    ca_uint32_t queue_idx;
    ca_uint32_t queue_id;
    ca_uint32_t queue_weight;
    ca_uint32_t queue_prio;
    ca_uint32_t tcont_idx;
    ca_uint32_t tcont_id;
    ca_uint32_t scheduler_id;
}ca_gpon_port_queue_t;

typedef struct {
    ca_uint16_t   gem_id;
    ca_uint16_t   flow_id;
    ca_boolean_t  valid;
} ca_gpon_sni_lpbk_cfg_t;

typedef struct {
    ca_boolean_t              enbl_us_rate_limit;
    ca_uint32_t               us_gem_td_id;
    ca_boolean_t              enbl_ds_rate_limit;
    ca_uint32_t               ds_gem_td_id;
} ca_gpon_gem_rate_limit_cfg_t;

typedef struct {
    ca_wred_mode_t  pol_mode;
    ca_uint32_t        cir_rate;
    ca_uint32_t        pir_rate;
    ca_uint32_t        commit_burst;
    ca_uint32_t        peak_burst;
} ca_gpon_gem_traffic_desc_cfg_t;

typedef struct {
    ca_uint16_t                        gem_td_id;
    ca_uint32_t                     pol_id;
    ca_gpon_gem_traffic_desc_cfg_t  td_cfg;
} ca_gpon_td_cb_t;


typedef struct {
    ca_boolean_t                 valid;
    ca_uint16_t                  pon_port_num;
    ca_uint16_t                  tcont_id;
    ca_uint32_t                  alloc_id;
    ca_uint8_t                   tcont_subpid;
    ca_uint8_t                   num_sp_queues;
    ca_gpon_port_tcont_scheduler_mode_t    sched_mode;
    ca_gpon_port_queue_t      queue_cb[CA_MAX_QUEUE_NUM_PER_TCONT_SCH];

} ca_gpon_tcont_cb_t;

typedef struct {
    ca_boolean_t vld;
    ca_uint32_t scheduler_id;
    ca_uint32_t point_to_scheduler_id;
    ca_gpon_port_scheduler_mode_t mode; /*scheduler mode among queues in a scheduler*/
    ca_uint32_t scheduler_weight;
    ca_uint32_t scheduler_prio;
    ca_uint32_t tcont_id;
    ca_gpon_port_queue_t      queue_cb[CA_MAX_QUEUE_NUM_PER_TCONT_SCH];
}ca_gpon_port_scheduler_t;

typedef struct {
    ca_uint16_t gem_index;      /* VOQ mode 0/1:gem_index = ((tcontidx<<11)|(cos<<8)|internal_gem_idx)
    							   VOQ mode 2  :gem_index = ((internal_gem_idx<<8)|internal_gem_idx)
								*/
    ca_uint32_t gem_port_id;
    ca_uint32_t tcont_id;   /* associated T-CONT alloc-ID */
    ca_uint32_t direction;  /* traffic direction */
    ca_uint32_t up_queue;   /*upstream  traffic queue */
    ca_policer_t up_traffic_descriptor;
    ca_uint32_t dn_queue;   /*downstream  traffic queue */
    ca_policer_t dn_traffic_descriptor;
    ca_boolean_t aes_state; /* aes encrypted or not */
    ca_uint8_t aes_key_ring;    /* Encryption key ring */
} ca_gpon_port_gem_port_t;

typedef struct {
	ca_uint16_t		grant_start_time;   		  
	ca_uint16_t		grant_size;      	
	ca_uint16_t		tcont_en;      	
	ca_uint16_t		tcont_id;      	
	ca_uint16_t		dbru;      	
	ca_uint16_t		ploamu;      	
	ca_uint8_t		fecu_enable;      	
	ca_uint8_t		burst_profile_index;      	
} ca_bwmap_t;

typedef struct {
    ca_uint16_t gem_port_id;
    ca_uint16_t tcont_idx;
    ca_boolean_t valid;
} ca_gpon_gem_tcont_map_t;

typedef struct {
    ca_uint32_t scheduler_id;
    ca_uint32_t pri_q_bitmap;
} queue_to_scheduler_map_t;

typedef struct {
    ca_uint32_t tcont_id;
    ca_uint32_t queue_id;
    ca_boolean_t valid;
} queue_to_tcont_map_t;

typedef struct {
    ca_boolean_t valid;
	ca_uint16_t gem_index;
    ca_uint32_t gem_port_id;
    ca_uint32_t tcont_id;    /* associated T-CONT alloc-ID */
    ca_uint32_t direction;    /* traffic direction */
    ca_uint32_t up_queue;    /*upstream  traffic queue */
	ca_policer_t up_traffic_descriptor;
    ca_uint32_t dn_queue;    /*downstream  traffic queue */
    ca_policer_t dn_traffic_descriptor;
    ca_boolean_t aes_state;    /* aes encrypted or not */
    ca_uint8_t aes_key_ring;    /* Encryption key ring */
} ca_gpon_port_gem_cb_t;

typedef struct {
    ca_boolean_t  state;
    ca_gpon_tcont_cb_t        tcont_cb[CA_GPON_MAX_TCONT_NUM];
    ca_gpon_port_scheduler_t  scheduler_cb[2];
    ca_gpon_port_gem_cb_t   gem_cb[CA_GPON_MAX_GEM_PORTS];

    ca_gpon_gem_tcont_map_t   gem_tcont_map[CA_GPON_MAX_GEM_PORTS];
   /* queue_to_scheduler_map_t  queue_to_sche_map[CA_GPON_MAX_QUEUE_NUM];
    queue_to_tcont_map_t      queue_to_tcont_map[CA_GPON_MAX_QUEUE_NUM];*/

} ca_gpon_port_cb_t;

typedef struct {
    ca_boolean_t omci_auth_completed_flag;   
    ca_uint8_t   new_omci_msk[CA_MSK_SIZE];
} ca_gpon_port_new_omci_msk_t;


ca_status_t ca_gpon_api_test (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id);

ca_status_t ca_gpon_api_test1(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id);

ca_status_t ca_gpon_cb_dump (
    CA_IN ca_device_id_t device_id);

ca_status_t ca_gpon_cb_init (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id
);

ca_status_t ca_gpon_onu_init(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t *vendor_id,
    CA_IN ca_uint8_t *version/*[14:14]*/,
    CA_IN ca_uint8_t *serial_number/*[8:8]*/,
    CA_IN ca_uint8_t *traffic_mgmt_option,
    CA_IN ca_boolean_t *battery_backup,
    CA_IN ca_uint8_t *admin_state,
    CA_IN ca_uint8_t *logical_onu_id/*[24:24]*/,
    CA_IN ca_uint8_t *password/*[12:12]*/
);

ca_status_t ca_gpon_onu_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean_t battery_backup,
    CA_IN ca_uint8_t admin_state
);

ca_status_t ca_gpon_onu_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *vendor_id,
    CA_OUT ca_uint8_t *version/*[14:14]*/,
    CA_OUT ca_uint8_t *serial_number/*[8:8]*/,
    CA_OUT ca_uint8_t *traffic_mgmt_option,
    CA_OUT ca_boolean_t *battery_backup,
    CA_OUT ca_uint8_t *admin_state,
    CA_OUT ca_uint8_t *logical_onu_id/*[24:24]*/,
    CA_OUT ca_uint8_t *password/*[12:12]*/
);


ca_status_t ca_gpon_port_sf_threshold_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t sf_threshold
);

ca_status_t ca_gpon_port_sf_threshold_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * sf_threshold
);

ca_status_t ca_gpon_port_sd_threshold_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t sd_threshold
);

ca_status_t ca_gpon_port_sd_threshold_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * sd_threshold
);

ca_status_t ca_gpon_port_laser_mode_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_gpon_port_laser_mode_t mode
);

ca_status_t ca_gpon_port_laser_mode_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_laser_mode_t *mode
);

ca_status_t ca_gpon_port_timer_set(
    CA_IN       ca_device_id_t                     device_id,
    CA_IN       ca_port_id_t                    port_id,
    CA_IN       ca_gpon_port_timer_type_t       timer_type,
    CA_IN       ca_uint32_t                     timer_msec
);

ca_status_t ca_gpon_port_timer_get(
    CA_IN       ca_device_id_t                     device_id,
    CA_IN       ca_port_id_t                    port_id,
    CA_IN       ca_gpon_port_timer_type_t       timer_type,
    CA_OUT      ca_uint32_t                     *p_timer_msec
);

ca_status_t ca_gpon_port_sync_threshold_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t sync_threshold,
    CA_IN ca_uint32_t miss_sync_threshold
);

ca_status_t ca_gpon_port_sync_threshold_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *sync_threshold,
    CA_OUT ca_uint32_t *miss_sync_threshold
);

ca_status_t ca_gpon_port_aes_enable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t aes_enable
);

ca_status_t ca_gpon_port_aes_enable_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_boolean_t * aes_enable
);

ca_status_t ca_gpon_port_aes_key_set(
    CA_IN   ca_device_id_t  device_id,
    CA_IN   ca_port_id_t    port_id,
    CA_IN   ca_uint8_t      effective_key_length,
    CA_IN   ca_uint8_t      *aes_key/*[effective_key_length:16]*/
);

ca_status_t ca_gpon_port_aes_key_get(
    CA_IN   ca_device_id_t      device_id,
    CA_IN   ca_port_id_t        port_id,
    CA_IN   ca_uint8_t          key_idx,
    CA_OUT  ca_boolean_t        *p_key_active,
    CA_OUT  ca_uint8_t          *p_effective_key_length,
    CA_OUT  ca_uint8_t          *aes_key/*[16:16]*/
);

ca_status_t ca_gpon_port_fec_enable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t fec_encoder_enable,
    CA_IN ca_boolean_t fec_decoder_enable
);

ca_status_t ca_gpon_port_fec_enable_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_boolean_t *fec_encoder_enable,
    CA_OUT ca_boolean_t *fec_decoder_enable
);

ca_status_t ca_gpon_port_fec_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean read_clear,
    CA_OUT ca_gpon_port_fec_stats_t *fec_stats
);

ca_status_t ca_gpon_port_onu_state_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_onu_states_t *onu_state
);

ca_status_t ca_gpon_port_onu_id_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * onu_id
);

ca_status_t ca_gpon_port_onu_eqd_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t * onu_eqd
);

ca_status_t ca_gpon_port_capability_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_capability_t *capability
);

ca_status_t ca_gpon_port_gem_port_add(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_gem_port_t *gem_port_cfg
);

ca_status_t ca_gpon_port_gem_port_delete(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint16_t gem_index
);

ca_status_t ca_gpon_port_gem_port_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_gem_port_t *gem_port_cfg
);

ca_status_t ca_gpon_port_gem_port_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_gpon_port_gem_port_t *gem_port_cfg
);


ca_status_t ca_gpon_port_omci_port_id_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *omci_port_id
);

ca_status_t ca_gpon_port_gem_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean read_clear,
    CA_IN ca_uint32_t gem_port_index,
    CA_OUT ca_gpon_port_gem_stats_t *gem_stats
);


ca_status_t ca_gpon_port_upstream_queue_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t queue_id,
    CA_IN ca_boolean_t queue_enable,
    CA_IN ca_uint32_t tcont_id,
    CA_IN ca_uint32_t scheduler_id,
    CA_IN ca_uint32_t weight,
    CA_IN ca_uint8_t pri
);


ca_status_t ca_gpon_port_upstream_queue_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t queue_id,
    CA_OUT ca_boolean_t *queue_enable,
    CA_OUT ca_uint32_t * tcont_id,
    CA_OUT ca_uint32_t *scheduler_id,
    CA_OUT ca_uint32_t *weight,
    CA_OUT ca_uint8_t *pri
);

ca_status_t ca_gpon_port_upstream_scheduler_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  scheduler_id,
    CA_IN ca_uint32_t tcont_id,
    CA_IN ca_gpon_port_tcont_scheduler_mode_t  policy,
    CA_IN ca_uint32_t pri_weight
);

ca_status_t ca_gpon_port_upstream_scheduler_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  scheduler_id,
    CA_OUT ca_uint32_t *tcont_id,
    CA_OUT ca_gpon_port_tcont_scheduler_mode_t *policy,
    CA_OUT ca_uint32_t *pri_weight
);

ca_status_t ca_gpon_port_tcont_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t alloc_id,
    CA_IN ca_uint32_t tcont_id,
    CA_IN ca_boolean_t t_enable,
    CA_IN ca_gpon_port_tcont_scheduler_t *scheduler
);

ca_status_t ca_gpon_port_tcont_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  alloc_id,
    CA_OUT ca_uint32_t *tcont_id,
    CA_OUT ca_boolean_t *t_enable,
    CA_OUT ca_gpon_port_tcont_scheduler_t *scheduler
);

ca_status_t ca_gpon_port_dying_gasp_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id
);

ca_status_t ca_gpon_port_laser_polarity_set (
    CA_IN        ca_device_id_t                device_id,
    CA_IN        ca_port_id_t                    port_id,
    CA_IN        ca_gpon_port_laser_polarity_t    tx_polarity,
    CA_IN        ca_gpon_port_laser_polarity_t    rx_polarity);

ca_status_t ca_gpon_port_laser_polarity_get (
    CA_IN        ca_device_id_t                device_id,
    CA_IN        ca_port_id_t                    port_id,
    CA_OUT        ca_gpon_port_laser_polarity_t    *tx_polarity,
    CA_OUT        ca_gpon_port_laser_polarity_t    *rx_polarity);

ca_status_t ca_gpon_port_init(
    CA_IN ca_device_id_t device_id);

ca_status_t ca_gpon_port_pon_tag_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t pon_tag_len,
	CA_OUT ca_uint8_t *pon_tag/*[pon_tag_len:8]*/
);

ca_status_t ca_gpon_port_msk_update(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t *msk/*[16:16]*/
);

ca_status_t ca_gpon_port_msk_update_status_get (
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_boolean_t *f_updated
);

ca_status_t ca_gpon_port_new_omci_msk_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_gpon_port_new_omci_msk_t new_msk
);

ca_status_t ca_gpon_port_new_omci_msk_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_gpon_port_new_omci_msk_t *new_msk
);

#if defined(CONFIG_LUNA_G3_SERIES)
#define GPON_ONU_SN_LEN     8
#define GPON_RGSD_ID_LEN	36

ca_status_t ca_gpon_serial_number_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t serial_number[GPON_ONU_SN_LEN]
);

ca_status_t ca_gpon_registration_id_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t registration_id[GPON_RGSD_ID_LEN]
);
#endif

ca_status_t ca_gpon_port_bwmap_get(
	CA_IN		ca_device_id_t	device_id, 
	CA_IN		ca_port_id_t	port_id, 
	CA_OUT		ca_bwmap_t		*bwmap
);

#endif
