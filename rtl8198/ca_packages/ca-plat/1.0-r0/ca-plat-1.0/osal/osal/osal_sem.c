#include "ca_types.h"
#include "os_core.h"
#include "osal_cmn.h"

ca_int32 ca_sem_init(
    ca_uint     *sem_id,
    const ca_int8 *sem_name,
    ca_uint32     sem_initial_value,
    ca_uint32     options)
{
    *sem_id = OS_SEM_CREATE(sem_initial_value);
    if(*sem_id == 0)
        return CA_OSAL_ERROR;

    return CA_OSAL_OK;
}

ca_int32 ca_sem_destroy(ca_uint sem_id)
{
    OS_SEM_DESTROY(sem_id);

    return CA_OSAL_OK;
}

ca_int32 ca_sem_wait(ca_uint sem_id, ca_int32 timeout_msec)
{
    OS_SEM_WAIT(sem_id, timeout_msec);
    return CA_OSAL_OK;
}

ca_int32 ca_sem_post(ca_uint sem_id)
{
    OS_SEM_POST(sem_id);
    return CA_OSAL_OK;
}

