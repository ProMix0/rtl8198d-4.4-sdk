#ifndef _EVENT_H_
#define _EVENT_H_
#include "ca_types.h"
#include "util_list.h"

#ifndef CA_EVT_OK
#define CA_EVT_OK           (0)
#define CA_EVT_ERROR        (-1)

typedef struct _ca_event_cfg_t
{
    ca_uint32 throttling_ena;
    ca_uint32 threshold;
    ca_uint32 interval;
} ca_event_cfg_t;

typedef enum
{
    EVT_CFG_COUNTER_GET,
    EVT_CFG_COUNTER_CLR,
    EVT_CFG_COUNTER_CLR_ALL,
    EVT_CFG_SET,
    EVT_CFG_GET,
    EVT_CFG_DUMP,
    MAX_EVT_CFG_TYPE
} EVT_CFG_TYPE_ENUM;

typedef struct _ca_event_cfg_local_t
{
    ca_int32 cfg_type;
    ca_int32 evt_type;
    ca_uint32 dest;
    ca_event_cfg_t cfg;
    ca_uint32 counter;
    ca_boolean read_clear;
} ca_event_cfg_local_t;

typedef void (*event_handler)(ca_device_id_t device_id, ca_int32 event_type, void *msg, ca_int32 len);
typedef void (*event_to_remote)(ca_device_id_t device_id, ca_int32 event_type, void *msg, ca_int32 len);
typedef void (*event_cfg_func)(ca_device_id_t device_id, ca_event_cfg_local_t *cfg);
#endif



#endif

