#ifndef _OSAL_MUTEX_H_
#define _OSAL_MUTEX_H_
#include "ca_types.h"

ca_int ca_mutex_init(
    ca_uint     *mut_id,
    const ca_int8 *mut_name,
    ca_uint32     options);
ca_int32 ca_mutex_destroy(ca_uint mut_id);
ca_int32 ca_mutex_lock(ca_uint mut_id);
ca_int32 ca_mutex_unlock(ca_uint mut_id);
ca_int32 ca_mutex_trylock(ca_uint mut_id);

#endif

