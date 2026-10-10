#ifndef __CA_US_RATE_MANAGER_H__
#define __CA_US_RATE_MANAGER_H__

#include "rate.h"

#define CA_US_RATE_MANAGER_MAX          		256
#define CA_US_RATE_MANAGER_MAX_STREAM_PRIO     		247

typedef enum {
	cir_only = 0,                   /* for each app_spec_stream, send CIR only, the rest of
					   bandwidth given to default_streams (8 VoQs) */
	strict_priority = 1,            /* remaining bandwidth allocated to app_spec_stream based on
					   stream priority (0-255) */
	cir_ratio = 2                   /* remaining bandwidth allocated to application specific
					   streams based on CIR ratio */
} us_manager_bandwidth_distribution_scheme_t;

typedef enum {
	schedule_sp  = 0, 	               	/* strict priority */
	schedule_wrr = 1			/* weighted round robin */
} us_rate_manager_default_scheduler_scheme_t;


typedef struct us_rate_manager_global_control {

	ca_uint32_t max_rate_manager_app_spec_streams;           /* valid stream 0 - 255 */

	/* for the case app_specific_stream <= 247, streams 248-255 are default streams
	   %(0-100)of total egress bandwidth allocated to default streams.
	   Default streams (8 VoQs) are for traffic that is not classified onto any of the
	   app_specific_streams */
	ca_uint32_t default_stream_bandwidth_percentage;

	/* see definition for bandwidth_distribution_scheme_t */
	us_manager_bandwidth_distribution_scheme_t remaining_bandwidth_distribution_scheme;

	/* default stream scheduler scheme */
	us_rate_manager_default_scheduler_scheme_t default_stream_scheduler;

	ca_uint32_t default_stream_voq_weigths[8];      /* used in case of WRR, weight for each voq */

	/* max egress rate. set to -1, packet engine will check egress back pressure on egress voq
	   before sending */
	ca_uint32_t max_egress_rate;

} us_rate_manager_global_control_t;

typedef struct us_rate_manager_app_spec_stream {

	ca_uint8_t  app_stream_id;      /* unique id used to identify app spec stream (255 - highest priority, 0 - lowest priority */
        ca_uint32_t cir_kbps;
        ca_uint32_t pir_kbps;
        ca_uint32_t strict_prio;        /* strict priority : 0 or 1 */
} us_rate_manager_app_spec_stream_t;

typedef struct us_rate_manager_ipc_app_spec_stream {
	ca_uint8_t				dest_port;
	us_rate_manager_app_spec_stream_t	app_spec_stream;
} us_rate_manager_ipc_app_spec_stream_t;

ca_status_t     ca_us_rate_manager_global_configuration_set (
	CA_IN	ca_device_id_t                  	device_id,
	CA_IN   us_rate_manager_global_control_t        *p_config
);

ca_status_t     ca_us_rate_manager_global_configuration_get (
	CA_IN	ca_device_id_t                  	device_id,
	CA_OUT	us_rate_manager_global_control_t        *p_config
);

ca_status_t     ca_us_rate_manager_entry_add (
	CA_IN	ca_device_id_t                  	device_id,
	CA_IN	us_rate_manager_app_spec_stream_t       *p_app_stream
);

ca_status_t ca_us_rate_manager_entry_delete (
	CA_IN  ca_device_id_t  device_id,
	CA_IN  ca_uint8_t app_stream_id
);

ca_status_t ca_us_rate_manager_delete_all (
    CA_IN  ca_device_id_t device_id
);

ca_status_t ca_us_rate_manager_entry_get (
    CA_IN  ca_device_id_t  device_id,
    CA_OUT us_rate_manager_app_spec_stream_t       *p_app_stream
);

ca_status_t ca_us_rate_manager_entry_iterate (
    CA_IN  ca_device_id_t        device_id,
    CA_OUT ca_iterator_t *return_entry
);
#endif

