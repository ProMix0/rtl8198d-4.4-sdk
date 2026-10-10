#include "ca_types.h"
#include "os_core.h"
#include "util_list.h"

/*lint -save -w1*/
/*lint -e454 -e647 -e605 -e650 */
#define OS_MUTEX_t pthread_mutex_t

ca_uint OS_MUTEX_CREATE(const ca_int8 *mtx_name)
{
    OS_MUTEX_t *mutex = NULL;
    pthread_mutexattr_t mutex_attr ;

    mutex = (OS_MUTEX_t *)malloc(sizeof(OS_MUTEX_t));
    if(mutex == NULL)
        return 0;

    memset(mutex, 0, sizeof(OS_MUTEX_t));
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutex_init(mutex, &mutex_attr);

    return (ca_uint)mutex;
}
/*lint -e454 */
void OS_MUTEX_LOCK(ca_uint mtx)
{
    OS_MUTEX_t *mutex = (OS_MUTEX_t *)mtx;

    if(mutex == NULL)
        return;
    pthread_mutex_lock(mutex);
    return;
}
/*lint +e454 */
ca_int32 OS_MUTEX_TRYLOCK(ca_uint mtx)
{
    OS_MUTEX_t *mutex = (OS_MUTEX_t *)mtx;

    if(mutex == NULL)
        return -1;

    return (ca_int32)pthread_mutex_trylock(mutex);
}

void OS_MUTEX_UNLOCK(ca_uint mtx)
{
    OS_MUTEX_t *mutex = (OS_MUTEX_t *)mtx;

    if(mutex == NULL)
        return;
    /*lint -e455 */
    pthread_mutex_unlock(mutex);
    /*lint +e455 */
    return;
}

void OS_MUTEX_DESTROY(ca_uint mtx)
{
    OS_MUTEX_t *mutex = (OS_MUTEX_t *)mtx;

    if(mutex)
    {
        pthread_mutex_destroy(mutex);
        free(mutex);
    }

    return;
}

ca_uint OS_SEM_CREATE(ca_int init_cnt)
{
    sem_t *sem = NULL;
    sem = (sem_t *)malloc(sizeof(sem_t));
    if (sem == NULL) {
        return 0;
    }
    sem_init(sem , 0 , init_cnt);

    return (ca_uint)sem;
}

ca_uint OS_SEM_DESTROY(ca_uint sem_id)
{
    sem_t *sem = (sem_t *)sem_id;

    if(sem == NULL)
        return -1;

    sem_destroy(sem);
    free(sem);

    return 0;
}

void OS_SEM_WAIT(ca_uint sem_id, ca_int32 timeout)
{
    sem_t *sem = (sem_t *)sem_id;

    if(sem == NULL)
        return;

    if(timeout == 0)
    {
        sem_trywait(sem);
    }
    else if(timeout == -1)
    {
        sem_wait(sem);
    }
    else
    {
        struct timeval current_time;
        struct timespec ts;

        gettimeofday(&current_time, 0);
        ts.tv_sec = current_time.tv_sec;
        ts.tv_sec += timeout/1000;
        if((timeout%1000)*1000 + current_time.tv_usec >= 1000000)
        {
            ts.tv_sec += 1;
            ts.tv_nsec = ((timeout%1000)*1000 + current_time.tv_usec)%1000000;
            ts.tv_nsec *= 1000;
        }
        else
        {
            ts.tv_nsec = (timeout%1000)*1000 + current_time.tv_usec;
            ts.tv_nsec *= 1000;
        }
        /*lint -e438*/
        while ((-1 == sem_timedwait(sem, &ts))
            && (errno == EINTR)){
            continue;
        }
        /*lint +e438*/
    }

    return;
}

void OS_SEM_POST(ca_uint sem_id)
{
    sem_t *sem = (sem_t *)sem_id;

    sem_post(sem);

    return;
}

typedef struct
{
    ca_int32 level;
    unsigned long flags;
} OS_SPINLOCK_t;

ca_uint OS_SPINLOCK_CREATE(ca_int32 level)
{
    return OS_MUTEX_CREATE(NULL);
}

void OS_SPINLOCK_LOCK(ca_uint lock)
{
    OS_MUTEX_LOCK(lock);
    return ;
}

void OS_SPINLOCK_UNLOCK(ca_uint lock)
{
    OS_MUTEX_UNLOCK(lock);
    return ;
}

void OS_SPINLOCK_DESTROY(ca_uint lock)
{
    OS_MUTEX_DESTROY(lock);
    return;
}

ca_uint64 OS_TICK_GET(void)
{
    struct timespec now;
    ca_uint64 cur_ticks = 0;

    clock_gettime(CLOCK_MONOTONIC, &now);;
    cur_ticks = (now.tv_sec) * 100 + (now.tv_nsec /10000000);

    return (ca_uint64) cur_ticks;
}

ca_uint64 OS_TIME_GET(void)
{
    struct timeval now;
    ca_uint64 cur_ticks = 0;

    gettimeofday(&now, NULL);
    cur_ticks = (now.tv_sec - 1185252800) * 100 + (now.tv_usec / 10000);

    return (ca_uint64) cur_ticks;
}

static void usleep_ex(unsigned usecs)
{
    struct timeval t;
    t.tv_sec = usecs/1000000;
    t.tv_usec = usecs%1000000;
    (void)select(1, NULL, NULL, NULL, &t);
}


void OS_DELAY(ca_int32 usec)
{
    usleep_ex(usec);
    return;
}

void OS_SLEEP(ca_int32 msec)
{
    if(msec)
        usleep_ex(msec*1000);
    return;
}

typedef struct
{
    /*struct work_struct work;*/
    void (*callback)(void *data);
    void *data;
} OS_WORK_t;

void OS_WORK_PROCESS_INTERNAL(void *data)
{

    return;
}

ca_uint OS_WORKQUEUE_CREATE(const ca_int8 *name, ca_int32 pri)
{
    //struct workqueue_struct *wq = NULL;

    return (ca_uint)NULL;
}

ca_int32 OS_WORK_ADD(ca_uint wq_id,
                void (*callback)(void *data),
                void *data)
{
    return 0;
}

void OS_WORKQUEUE_DESTROY(ca_uint wq_id)
{
    return;
}

ca_int OS_FILE_OPEN(const ca_int8 *path, ca_int32 flag,ca_int32 mode)
{
    ca_int fd;
    fd = open(path, flag, mode);
    return fd;
}

ca_int OS_FILE_CLOSE(ca_int fd)
{
    return close(fd);
}

ca_int OS_FILE_READ(ca_int fd, ca_int8 *buf, ca_int32 readlen)
{
	return read(fd, buf, readlen);
}

ca_int OS_FILE_WRITE(ca_int fd, ca_int8 *buf, ca_int32 len)
{
	return write(fd, buf, len);
}

ca_int OS_FILE_SIZE(ca_int fd)
{
    ca_int32 size = 0;
    fseek((FILE *)fd, 0L, SEEK_END);
    size = ftell((FILE *)fd);
    rewind((FILE *)fd);
    return size;
}

ca_int OS_FILE_CP(
    const char *dst,
    const char *src){
    FILE *fd_dst, *fd_src;
    int ch;

    if(NULL==src||NULL==dst){
        return -1;
    }

    fd_src = fopen(src, "r");
    if(NULL==fd_src)
        return -1;
    
    fd_dst = fopen(dst, "w");
    if(NULL==fd_dst)
    {
        fclose(fd_src);
        return -1;
    }

    while((ch=fgetc(fd_src)) != EOF){
        fputc(ch, fd_dst);
    }

    fclose(fd_src);
    fclose(fd_dst);
    return 0;
}


ca_int OS_THREAD_CREATE(ca_uint *thread_id , const ca_int8 *thread_name,
                        const void *function_pointer , void *param,
                        ca_uint32 stack_size,
                        ca_uint32 priority, ca_uint32 flags)
{
    int                 return_code = 0;
    pthread_attr_t      custom_attr;
    unsigned int        local_stack_size;
    pthread_t           *pThread = NULL;

    if (stack_size < 8*1024)
        local_stack_size = 8*1024;
    else
        local_stack_size = stack_size;

    pthread_attr_init(&custom_attr);
    pthread_attr_setstacksize(&custom_attr, (size_t)local_stack_size);

    pThread = (pthread_t *)malloc(sizeof(pthread_t));
    if(pThread == NULL)
        return -1;

    memset(pThread, 0, sizeof(pthread_t));
    return_code = pthread_create(pThread,
                                 &custom_attr,
                                 function_pointer,
                                 (void*)param);
    if (return_code != 0) {
        free(pThread);
        return -1;
    }

   *thread_id = (ca_uint)*pThread;
    free(pThread);
    return 0;
}

ca_int OS_CURRENT_THREAD(void)
{
    pid_t pid = 0;

    pid = syscall(SYS_gettid);
    return (ca_int)pid;
}

#define MQ_NAME_LEN 128
ca_int OS_QUEUE_CREATE
    (
    ca_uint 	  *queue_id,
    const ca_int8 *queue_name,
    ca_uint32 	  queue_depth,
    ca_uint32 	  data_size,
    ca_uint32 	  flags
    )
{
    struct mq_attr attrs;
    char  mq_name[MQ_NAME_LEN+1] = {0};
    mqd_t id = -1;

    if(queue_name == NULL || queue_id == NULL){
        return -1;
    }

    snprintf(mq_name,MQ_NAME_LEN,"/%s",queue_name);

    attrs.mq_flags = 0;
    attrs.mq_maxmsg = queue_depth;
    attrs.mq_msgsize = data_size;

    id = mq_open(
            mq_name,
            O_RDWR | O_CREAT, S_IRWXU | S_IRWXG,
            &attrs);

    if(id == -1){
        return -1;
    }

    *queue_id = (ca_uint)id;

    return 0;
}


ca_int OS_QUEUE_DELETE
    (
    ca_uint queue_id
    )
{
    mq_close((mqd_t)queue_id);
    return 0;
}


ca_int OS_QUEUE_GET
    (
    ca_uint queue_id,
    void 	  *data,
    ca_uint32 size,
    ca_uint32 *size_copied,
    ca_uint32  timeout
    )
{
    ca_int32 len = 0 ;

    if(data == NULL || size_copied == NULL){
        return -1;
    }

    switch(timeout){
        case -1:
            len = mq_receive((mqd_t)queue_id,data,size,0);
            break;
        case 0:
            len = mq_receive((mqd_t)queue_id,data,size,0);
            break;
        default:
            len = mq_receive((mqd_t)queue_id,data,size,0);
            break;
    }

    if(len < 0){
        return -1;
    }

    *size_copied = len;

    return 0;
}


ca_int OS_QUEUE_PUT
    (
    ca_uint     queue_id,
    void 		*data,
    ca_uint32 	size,
    ca_int32 	timeout,
    ca_uint32 	priority
    )
{
    ca_int ret = -1;

    ret = mq_send((mqd_t)queue_id, data, size,0);

    return ret;
}

ca_uint32 OS_STRTOUL(
    const char *nptr,
    char **endptr,
    int base){
    return strtoul(nptr, endptr, base);
}

void OS_DO_DIV(ca_uint64 dividend, ca_uint32 divisor, ca_uint64 *quotient, ca_uint32 *remainder)
{
    *quotient = dividend / divisor;
    *remainder = dividend % divisor;
}


/*lint +e454 +e647 +e605 +e650 */
/*lint -restore */

