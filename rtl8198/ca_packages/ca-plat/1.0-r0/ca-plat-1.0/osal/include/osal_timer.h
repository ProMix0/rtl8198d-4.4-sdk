#ifndef _OSAL_TIMER_H_
#define _OSAL_TIMER_H_
#include "ca_types.h"
typedef enum
{
    CA_ONCE_TIMER   = 0,
    CA_CIRCLE_TIMER = 1,
} TIMER_TYPE;

#define CA_INVALID_TIMER    0

typedef enum
{
    CA_TIMER_OK      = 0,
    CA_TIMER_IN_PROC = 1,
    CA_TIMER_ERROR   = -1
} TIMER_RETURN_CODE;

typedef struct {
    ca_uint32_t   interval;
    ca_uint32_t   remain;
    void          (*callback)(void *);
    void          *data;
    ca_int32_t    timer_type;
    ca_uint32_t   unique_id;
    ca_int32_t    is_processed;
    ca_int32_t    del_apply;
    ca_int32_t    stop_apply;
    ca_uint32_t   time_drain;
    ca_uint32     queue_id;
    void        * queue_data;
} ca_timer_t;

ca_uint32  ca_timer_add(
                    ca_int32 interval,
                    void (*callback)(void *data),
                    void *data);
ca_uint32  ca_circle_timer_add(
                    ca_int32 interval,
                    void (*callback)(void *data),
                    void *data);
ca_int32    ca_timer_del(ca_uint32 timer_id);
ca_int32    ca_timer_stop(ca_uint32 timer_id);
void        ca_timer_start(ca_uint32 timer_id);
void       ca_timer_restart(ca_uint32_t timer_id, ca_uint32_t   interval);
void        ca_timer_show(void);
ca_int32    ca_timer_count(void);
#endif
