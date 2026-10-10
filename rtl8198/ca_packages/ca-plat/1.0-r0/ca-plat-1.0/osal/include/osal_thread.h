#ifndef _OSAL_THREAD_H_
#define _OSAL_THREAD_H_

ca_int ca_thread_create(ca_uint *thread_id , const ca_int8 *thread_name,
                        const void *function_pointer , void *param,
                        ca_uint32 stack_size,
                        ca_uint32 priority, ca_uint32 flags);

#endif

