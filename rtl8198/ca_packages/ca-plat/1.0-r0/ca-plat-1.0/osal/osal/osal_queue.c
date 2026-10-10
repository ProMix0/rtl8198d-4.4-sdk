#include "ca_types.h"
#include "os_core.h"

ca_int ca_queue_create
    (
    ca_uint     *queue_id,
    const ca_int8 *queue_name,
    ca_uint32     queue_depth,
    ca_uint32     data_size,
    ca_uint32     flags
    )
{
    return OS_QUEUE_CREATE(queue_id, queue_name, queue_depth, data_size, flags);
}

ca_int ca_queue_delete
    (
    ca_uint queue_id
    )
{
    return OS_QUEUE_DELETE(queue_id);
}

ca_int ca_queue_get
    (
    ca_uint queue_id,
    void      *data,
    ca_uint32 size,
    ca_uint32 *size_copied,
    ca_uint32  timeout
    )
{
    return OS_QUEUE_GET(queue_id, data, size, size_copied, timeout);
}

ca_int ca_queue_put
    (
    ca_uint   queue_id,
    void        *data,
    ca_uint32   size,
    ca_int32    timeout,
    ca_uint32   priority
    )
{
    return OS_QUEUE_PUT(queue_id, data, size, timeout, priority);
}


