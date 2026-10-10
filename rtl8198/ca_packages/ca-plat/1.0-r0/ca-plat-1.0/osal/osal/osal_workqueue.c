#include "ca_types.h"
#include "os_core.h"
#include "osal_workqueue.h"

ca_uint_t ca_workqueue_init(const ca_int8_t *wq_name, ca_int32_t priority)
{
    ca_uint_t wq_id = 0;

    wq_id = OS_WORKQUEUE_CREATE(wq_name,priority);
    if(wq_id == 0)
        return CA_INVALID_WQ_ID;

    return wq_id;
}

ca_int_t ca_work_add(ca_uint_t wq_id,
                void (*callback)(void *data),
                void *data)
{
    return OS_WORK_ADD(wq_id, callback, data);
}

void ca_workqueue_destroy(ca_uint_t wq_id)
{
    OS_WORKQUEUE_DESTROY(wq_id);
    return;
}

