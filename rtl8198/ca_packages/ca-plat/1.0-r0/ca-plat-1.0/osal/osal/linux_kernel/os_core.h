#ifndef _OS_CORE_H_
#define _OS_CORE_H_

#include <linux/module.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sockios.h>
#include <linux/net.h>
#include <linux/skbuff.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/netlink.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <net/sock.h>
#include <linux/version.h>
#include <asm/div64.h>
#include "ca_types.h"


#define     OS_MALLOC_API(len) kmalloc(len, GFP_ATOMIC)
#define     OS_FREE_API(ptr)   kfree(ptr)
#if defined(CONFIG_LUNA_G3_SERIES)
#define     OS_PRINTF(fmt, arg...)          printk(KERN_INFO fmt, ## arg)
#else
#define     OS_PRINTF          printk
#endif
#define     OS_MEMSET_API(ptr,var,len)   memset(ptr,var,len)

ca_uint     OS_MUTEX_CREATE(const ca_int8 *name);
void        OS_MUTEX_DESTROY(ca_uint mutex);
void        OS_MUTEX_LOCK(ca_uint mutex);
ca_int32  OS_MUTEX_TRYLOCK(ca_uint mutex);
void        OS_MUTEX_UNLOCK(ca_uint mutex);

ca_uint     OS_SEM_CREATE(ca_int init_cnt);
ca_uint     OS_SEM_DESTROY(ca_uint sem_id);
void        OS_SEM_WAIT(ca_uint sem_id, ca_int32 timeout_msec);
void        OS_SEM_POST(ca_uint sem_id);

ca_uint     OS_SPINLOCK_CREATE(ca_int32 level);
void        OS_SPINLOCK_LOCK(ca_uint lock);
void        OS_SPINLOCK_UNLOCK(ca_uint lock);
void        OS_SPINLOCK_DESTROY(ca_uint lock);

ca_uint64   OS_TICK_GET(void);
ca_uint64   OS_TIME_GET(void);
void        OS_DELAY(ca_int32 usec);
void        OS_SLEEP(ca_int32 msec);

ca_uint     OS_WORKQUEUE_CREATE(const ca_int8 *name, ca_int32 pri);
ca_int32    OS_WORK_ADD(ca_uint wq_id,
                        void (*callback)(void *data),
                        void *data);
void        OS_WORKQUEUE_DESTROY(ca_uint wq_id);

ca_int      OS_FILE_OPEN(const ca_int8 *path, ca_int32 flag,ca_int32 mode);
ca_int      OS_FILE_CLOSE(ca_int fd);
ca_int      OS_FILE_READ(ca_int fd, ca_int8 *buf, ca_int32 readlen);
ca_int      OS_FILE_WRITE(ca_int fd, ca_int8 *buf, ca_int32 readlen);
ca_int      OS_FILE_SIZE(ca_int fd);
ca_int      OS_FILE_CP(const char *dst,const char *src);

ca_int      OS_CURRENT_THREAD(void);
ca_int      OS_THREAD_CREATE(ca_uint *thread_id , const ca_int8 *thread_name,
                        const void *function_pointer , void *param,
                        ca_uint32 stack_size,
                        ca_uint32 priority, ca_uint32 flags);

ca_int      OS_QUEUE_CREATE
                        (
                        ca_uint       *queue_id,
                        const ca_int8 *queue_name,
                        ca_uint32     queue_depth,
                        ca_uint32     data_size,
                        ca_uint32     flags
                        );
ca_int      OS_QUEUE_DELETE
                        (
                        ca_uint   queue_id
                        );
ca_int      OS_QUEUE_GET
                        (
                        ca_uint   queue_id,
                        void      *data,
                        ca_uint32 size,
                        ca_uint32 *size_copied,
                        ca_uint32  timeout
                        );
ca_int      OS_QUEUE_PUT
                        (
                        ca_uint     queue_id,
                        void        *data,
                        ca_uint32   size,
                        ca_int32    timeout,
                        ca_uint32   priority
                        );
ca_uint32 OS_STRTOUL(
    const char *nptr,
    char **endptr,
    int base);

void OS_DO_DIV(ca_uint64 dividend, ca_uint32 divisor, ca_uint64 *quotient, ca_uint32 *remainder);

#endif

