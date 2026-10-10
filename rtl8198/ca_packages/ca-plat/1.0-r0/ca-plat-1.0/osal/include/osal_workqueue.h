#ifndef _OSAL_WORKQUEUE_H_
#define _OSAL_WORKQUEUE_H_
#include "ca_types.h"

#define CA_INVALID_WQ_ID            (0)

ca_uint     ca_workqueue_init(const ca_int8 *wq_name, ca_int32 priority);
ca_int      ca_work_add(ca_uint wq_id, void (*callback)(void *data), void *data);
void        ca_workqueue_destroy(ca_uint wq_id);
#endif
