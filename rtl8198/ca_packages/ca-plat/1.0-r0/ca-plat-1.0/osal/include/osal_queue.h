#ifndef _OSAL_QUEUE_H_
#define _OSAL_QUEUE_H_
#include "ca_types.h"

#define OSAL_NO_WAIT                 (0)
#define OSAL_WAIT_FOREVER            (-1)

ca_int ca_queue_create
    (
    ca_uint       *queue_id,
    const ca_int8 *queue_name,
    ca_uint32     queue_depth,
    ca_uint32     data_size,
    ca_uint32     flags
    );
ca_int ca_queue_delete
    (
    ca_uint queue_id
    );
ca_int ca_queue_get
    (
    ca_uint   queue_id,
    void      *data,
    ca_uint32 size,
    ca_uint32 *size_copied,
    ca_uint32  timeout
    );
ca_int ca_queue_put
    (
    ca_uint     queue_id,
    void        *data,
    ca_uint32   size,
    ca_int32    timeout,
    ca_uint32   priority
    );

#endif
