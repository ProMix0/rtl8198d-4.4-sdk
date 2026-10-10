#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "ca_types.h"


#define  CA_QUEUE_WRED_POINT_COUNT          (16)
#define  CA_QUEUE_WEIGHT_STRICT_PRIORITY    (0)
#define  CA_QUEUE_COUNT                     (8)

typedef enum
{
    CA_QUEUE_WRED_COLOR_GREEN  = 0, /* green  */
    CA_QUEUE_WRED_COLOR_YELLOW = 1, /* yellow */
}ca_queue_wred_color_t;


typedef struct
{
    ca_uint32_t marked_dp[CA_QUEUE_WRED_POINT_COUNT];   /*drop percentage of each point*/
    ca_uint32_t unmarked_dp[CA_QUEUE_WRED_POINT_COUNT]; /*drop percentage of each point*/
}ca_queue_wred_profile_t;

typedef enum
{
    CA_SCHEDULE_STRICT_PRIORITY,
    CA_SCHEDULE_DEFICIT_WEIGHT_ROUND_ROBIN,
    CA_SCHEDULE_DEFICIT_ROUND_ROBIN,
    CA_SCHEDULE_WEIGHT_ROUND_ROBIN,
    CA_SCHEDULE_ROUND_ROBIN
} ca_queue_schedule_mode_t;

typedef struct
{
    ca_uint32_t weights[CA_QUEUE_COUNT]; /*schedule weight of each queue*/
}ca_queue_weights_t;


typedef struct
{
    ca_uint32_t forwarded;                /* total forwarded packets, unit - packets       */
    ca_uint32_t tail_dropped;              /* tail-drop dropped packets, unit - packets     */
    ca_uint32_t marked_dropped;            /* WRED dropped marked packets, unit - packets   */
    ca_uint32_t unmarked_dropped;          /* WRED dropped unmarked packets, unit - packets */

    ca_uint32_t forwarded_bytes;          /* total forwarded packets, unit - bytes       */
    ca_uint32_t tail_dropped_bytes;        /* tail-drop dropped packets, unit - bytes       */
    ca_uint32_t marked_dropped_bytes;      /* WRED dropped marked packets, unit - bytes     */
    ca_uint32_t unmarked_dropped_bytes;    /* WRED dropped unmarked packets, unit - bytes   */
}ca_queue_stats_t;

/* PIE */
typedef struct
{
    ca_uint32_t qfull_onethird  : 1;
    ca_uint32_t dp_weight_A     : 1;
    ca_uint32_t dp_weight_B     : 1;
    ca_uint32_t dp_ceiling      : 1;
    ca_uint32_t dp_high_threshold : 1;
    ca_uint32_t dp_low_threshold  : 1;
    ca_uint32_t max_burst       : 1;
    ca_uint32_t mean_pkt_size   : 1;
    ca_uint32_t voq_high_water_mark : 1;
    ca_uint32_t interval        : 1;
    ca_uint32_t burst_decrement : 1;
}ca_pie_cb_mask_t;

#define CA_MAX_FLOWS_PER_PIE 128

typedef struct
{
    ca_pie_cb_mask_t mask;
    ca_uint8_t  qfull_onethird;         /* range : 0 - 2MB, buffer_size / 3*/
    ca_uint8_t  dp_weight_A;            /* range : A(0 1023), B(0 ~ 63) */
    ca_uint8_t  dp_weight_B;
    ca_uint8_t  dp_ceiling;             /* range : 0 - 4G  */
    ca_uint32_t dp_high_threshold;      /* range : 0 - 64K */
    ca_uint32_t dp_low_threshold;       /* range : 0 - 64K */
    ca_uint32_t max_burst;              /* range : 0 - 2s, unit : usec */
    ca_uint32_t mean_pkt_size;          /* rnage : 1 - 64K, size must be power of 2 */
    ca_uint32_t voq_high_water_mark;    /* range : 0 - 2MB, buffer size */
    ca_uint32_t interval;               /* range : 0 - 2s */
    ca_uint32_t burst_decrement;        /* rnage : 0 - 2s, unit : usec */
}ca_pie_global_configuration_t;

/* per PIE VoQ configurations */
typedef struct
{
    ca_uint32_t flow_ids            : 1;
    ca_uint32_t pie_voq_idx         : 1;
    ca_uint32_t burst_reset_timeout : 1;
    ca_uint32_t latency_high        : 1;
    ca_uint32_t latency_low         : 1;
    ca_uint32_t latency_target      : 1;
    ca_uint32_t msr                 : 1;
    ca_uint32_t peak_rate           : 1;
}ca_pie_voq_cb_mask_t;

typedef struct
{
    ca_pie_voq_cb_mask_t mask;
    ca_uint32_t flow_ids[CA_MAX_FLOWS_PER_PIE];
    ca_uint32_t pie_voq_idx;            /* 0 - 15             */
    ca_uint32_t burst_reset_timeout;    /* 0 - 2s, unit : usec*/
    ca_uint32_t latency_high;           /* 0 - 2s, unit : usec*/
    ca_uint32_t latency_low;            /* 0 - 2s, unit : usec*/
    ca_uint32_t latency_target;         /* 0 - 2s, unit : usec*/
    ca_uint32_t msr;                    /* 0 - 4Gbps          */
    ca_uint32_t peak_rate;              /* 0 - 4Gbps          */
}ca_pie_voq_control_block_t;


ca_status_t ca_queue_length_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    queue,
    CA_IN ca_uint32_t    length
);

ca_status_t ca_queue_length_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint32_t    queue,
    CA_OUT ca_uint32_t    *length
);

ca_status_t ca_queue_pause_threshold_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    queue,
    CA_IN ca_uint32_t    xon,
    CA_IN ca_uint32_t    xoff,
    CA_IN ca_uint32_t    pause_quanta
);

ca_status_t ca_queue_pause_threshold_get
(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_IN  ca_uint32_t    queue,
    CA_OUT ca_uint32_t    *xon,
    CA_OUT ca_uint32_t    *xoff,
    CA_OUT ca_uint32_t    *pause_quanta
);

ca_status_t ca_queue_schedule_set
(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_port_id_t             port_id,
    CA_IN ca_queue_schedule_mode_t mode,
    CA_IN ca_queue_weights_t       *weights
);

ca_status_t ca_queue_schedule_get
(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_port_id_t             port_id,
    CA_OUT ca_queue_schedule_mode_t *mode,
    CA_OUT ca_queue_weights_t       *weights
);

ca_status_t ca_queue_wred_set
(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_port_id_t            port_id,
    CA_IN ca_uint32_t             queue,
    CA_IN ca_queue_wred_profile_t *profile
);

ca_status_t ca_queue_wred_get
(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_port_id_t            port_id,
    CA_IN  ca_uint32_t             queue,
    CA_OUT ca_queue_wred_profile_t *profile
);

ca_status_t ca_queue_wred_ecn_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean_t   ecn_support
);

ca_status_t ca_queue_wred_ecn_get
(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_boolean_t   *ecn_support
);

ca_status_t ca_queue_stats_get
(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_port_id_t     port_id,
    CA_IN  ca_uint32_t      queue,
    CA_IN  ca_boolean_t     read_clear,
    CA_OUT ca_queue_stats_t *stats
);

ca_status_t ca_queue_stats_clear
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    queue
);

ca_status_t ca_queue_flush
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  queue
);

ca_status_t ca_pie_config_set
(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_pie_global_configuration_t *config
);

ca_status_t ca_pie_config_get
(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_pie_global_configuration_t *config
);

ca_status_t ca_pie_queue_set
(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_pie_voq_control_block_t *config
);

ca_status_t ca_pie_queue_get
(
    CA_IN  ca_device_id_t             device_id,
    CA_OUT ca_pie_voq_control_block_t *config
);

#endif


