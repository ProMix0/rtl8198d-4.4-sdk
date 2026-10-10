#include "ca_types.h"
#include "os_core.h"
#include "osal_cmn.h"

ca_int32 ca_mutex_init(ca_uint     *mut_id,
                    const ca_int8 *mut_name,
                    ca_uint32     options)
{
    *mut_id = (ca_uint)OS_MUTEX_CREATE(mut_name);
    if(*mut_id == 0)
        return CA_OSAL_ERROR;

    return CA_OSAL_OK;
}

ca_int32 ca_mutex_destroy(ca_uint mut_id)
{
    OS_MUTEX_DESTROY(mut_id);
    return CA_OSAL_OK;
}

ca_int32 ca_mutex_lock(ca_uint mut_id)
{
    if (mut_id == 0)
        return CA_OSAL_ERROR;

    OS_MUTEX_LOCK(mut_id);
    return CA_OSAL_OK;
}

ca_int32 ca_mutex_trylock(ca_uint mut_id)
{
    if (mut_id == 0)
        return CA_OSAL_ERROR;

    return OS_MUTEX_TRYLOCK(mut_id);
}

ca_int32 ca_mutex_unlock(ca_uint mut_id)
{
    OS_MUTEX_UNLOCK(mut_id);
    return CA_OSAL_OK;
}

