#ifndef _OSAL_SEM_H_
#define _OSAL_SEM_H_
#include "ca_types.h"

ca_int32    ca_sem_init(
                ca_uint       *sem_id,
                const ca_int8 *sem_name,
                ca_uint32     sem_initial_value,
                ca_uint32     options);
ca_int32    ca_sem_destroy(ca_uint sem_id);
ca_int32    ca_sem_wait(ca_uint sem_id, ca_int32 timeout_msec);
ca_int32    ca_sem_post(ca_uint sem_id);

#endif

