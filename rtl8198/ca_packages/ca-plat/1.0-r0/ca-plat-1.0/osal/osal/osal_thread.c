#include "ca_types.h"
#include "os_core.h"

ca_int ca_thread_create(ca_uint *thread_id , const ca_int8 *thread_name,
                        const void *function_pointer , void *param,
                        ca_uint32 stack_size,
                        ca_uint32 priority, ca_uint32 flags)
{/*Unsupport Priority*/
    return OS_THREAD_CREATE(thread_id, thread_name, function_pointer,
                        param, stack_size, priority, flags);
}


