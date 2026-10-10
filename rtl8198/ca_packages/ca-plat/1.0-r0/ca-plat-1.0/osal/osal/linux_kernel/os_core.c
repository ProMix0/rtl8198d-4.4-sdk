#include "ca_types.h"
#include "os_core.h"
/*lint -save -w1*/
typedef struct {
    struct mutex mtx;
    ca_int8 mtx_name[16];
} OS_MUTEX_t;

ca_uint OS_MUTEX_CREATE(const ca_int8 *mtx_name)
{
    OS_MUTEX_t *mutex = NULL;

    mutex = (OS_MUTEX_t *)kmalloc(sizeof(OS_MUTEX_t), GFP_ATOMIC);
    if(mutex == NULL)
        return 0;

    memset(mutex, 0, sizeof(OS_MUTEX_t));
    memcpy(mutex->mtx_name, mtx_name, 15);
    mutex_init(&mutex->mtx);

    return (ca_uint)mutex;
}

void OS_MUTEX_LOCK(ca_uint mtx)
{
    OS_MUTEX_t *mutex = (OS_MUTEX_t *)mtx;

    if(mutex == NULL)
        return;

    mutex_lock(&mutex->mtx);
    return;
}

ca_int32 OS_MUTEX_TRYLOCK(ca_uint mtx)
{
    OS_MUTEX_t *mutex = (OS_MUTEX_t *)mtx;

    if(mutex == NULL)
        return -1;

    return (ca_int32)mutex_trylock(&mutex->mtx);
}


void OS_MUTEX_UNLOCK(ca_uint mtx)
{
    OS_MUTEX_t *mutex = (OS_MUTEX_t *)mtx;

    if(mutex == NULL)
        return;

    mutex_unlock(&mutex->mtx);
    return;
}

void OS_MUTEX_DESTROY(ca_uint mtx)
{
    OS_MUTEX_t *mutex = (OS_MUTEX_t *)mtx;

    if(mutex)
        kfree(mutex);

    return;
}

ca_uint OS_SEM_CREATE(ca_int init_cnt)
{
    struct semaphore *sem = NULL;
    sem = (struct semaphore *) kmalloc(sizeof(struct semaphore), GFP_KERNEL);
    if (sem == NULL) {
        return 0;
    }
    sema_init(sem, init_cnt);

    return (ca_uint)sem;
}

ca_uint OS_SEM_DESTROY(ca_uint sem_id)
{
    DEFINE_SPINLOCK(lock);

    if(!down_trylock((struct semaphore*)sem_id))
    {
        spin_lock_irq(&lock);
        up((struct semaphore*)sem_id);
        kfree((struct semaphore*)sem_id);
        spin_unlock_irq(&lock);
        return 0;
    }
    else
    {
        printk("Sem destroy fail, used by others\n");
        return -1;
    }


}

void OS_SEM_WAIT(ca_uint sem_id, ca_int32 timeout_msec)
{
    if (down_interruptible((struct semaphore*)sem_id)) {
        printk("Sem down_interruptible failed\n");
    }
    return;
}

void OS_SEM_POST(ca_uint sem_id)
{
    up((struct semaphore*)sem_id);
    return;
}

typedef struct
{
    ca_int32 level;
    unsigned long flags;
    spinlock_t lock;
} OS_SPINLOCK_t;

ca_uint OS_SPINLOCK_CREATE(ca_int32 level)
{
    OS_SPINLOCK_t *lock = NULL;

    lock = kmalloc(sizeof(OS_SPINLOCK_t), GFP_ATOMIC);
    if(lock == NULL)
        return (ca_uint)lock;

    lock->level = level;
    lock->flags = 0;
    spin_lock_init(&lock->lock);
    return (ca_uint)lock;
}

void OS_SPINLOCK_LOCK(ca_uint lock)
{
    OS_SPINLOCK_t *spinlock = (OS_SPINLOCK_t *)lock;

    if(spinlock == NULL)
        return ;

    switch(spinlock->level)
    {
    case 0:/*SPINLOCK_CMN*/
        spin_lock(&spinlock->lock);
        break;
    case 1:/*SPINLOCK_BH*/
        spin_lock_bh(&spinlock->lock);
        break;
    case 2:/*SPINLOCK_IRQ*/
        spin_lock_irq(&spinlock->lock);
        break;
    case 3:/*SPINLOCK_SMP*/
    default:
        spin_lock_irqsave(&spinlock->lock, spinlock->flags);
        break;
    }

    return ;
}

void OS_SPINLOCK_UNLOCK(ca_uint lock)
{
    OS_SPINLOCK_t *spinlock = (OS_SPINLOCK_t *)lock;

    if(spinlock == NULL)
        return ;

    switch(spinlock->level)
    {
    case 0:/*SPINLOCK_CMN*/
        spin_unlock(&spinlock->lock);
        break;
    case 1:/*SPINLOCK_BH*/
        spin_unlock_bh(&spinlock->lock);
        break;
    case 2:/*SPINLOCK_IRQ*/
        spin_unlock_irq(&spinlock->lock);
        break;
    case 3:/*SPINLOCK_SMP*/
    default:
        spin_unlock_irqrestore(&spinlock->lock, spinlock->flags);
        break;
    }

    return ;
}

void OS_SPINLOCK_DESTROY(ca_uint lock)
{
    kfree((void *)lock);
    return;
}

ca_uint64 OS_TICK_GET(void)
{
    return get_jiffies_64();
}

ca_uint64 OS_TIME_GET(void)
{
    return get_jiffies_64();
}

void OS_DELAY(ca_int32 usec)
{
    if(usec >= 2000)
    {
        usec = usec/1000;
        mdelay(usec);
    }
    else
    {
        udelay(usec);
    }

    return;
}

void OS_SLEEP(ca_int32 msec)
{
    if(msec)
        msleep(msec);
    return;
}

typedef struct
{
    struct work_struct work;
    void (*callback)(void *data);
    void *data;
} OS_WORK_t;
static void OS_WORK_PROCESS_INTERNAL(void *data)
{
    OS_WORK_t *my_work = (OS_WORK_t *)data;

    if(my_work == NULL)
        return;

    if(my_work->callback)
        my_work->callback(my_work->data);

    kfree(my_work);
    return;
}

ca_uint OS_WORKQUEUE_CREATE(const ca_int8 *name, ca_int32 pri)
{
    struct workqueue_struct *wq = NULL;

    if(name == NULL)
        return 0;

    wq = (struct workqueue_struct *)create_workqueue(name);
    if(wq == NULL)
        return 0;

    return (ca_uint)wq;
}

ca_int32 OS_WORK_ADD(ca_uint wq_id,
                void (*callback)(void *data),
                void *data)
{
    OS_WORK_t *work = NULL;
    struct workqueue_struct *wq = (struct workqueue_struct *)wq_id;

    if(wq == NULL)
        return -1;

    work = kmalloc(sizeof(OS_WORK_t), GFP_ATOMIC);
    if(work == NULL)
        return -1;

    memset(work, 0, sizeof(OS_WORK_t));
    INIT_WORK(&work->work, (void *)OS_WORK_PROCESS_INTERNAL);
    work->callback = callback;
    work->data = data;
    queue_work(wq, &work->work);

    return 0;
}

void OS_WORKQUEUE_DESTROY(ca_uint wq_id)
{
    if(wq_id == 0)
        return;

    destroy_workqueue((struct workqueue_struct *)wq_id);
    return;
}

ca_int OS_FILE_OPEN(const ca_int8 *path, ca_int32 flag,ca_int32 mode)
{
    struct file *fp = NULL;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(get_ds());
    fp=filp_open(path, flag, mode);
    set_fs(oldfs);

    if(IS_ERR(fp))
        return -1; /*The same return var as open*/
    else
        return (ca_int)fp;
}

ca_int OS_FILE_CLOSE(ca_int fd)
{
    ca_int ret;
    mm_segment_t oldfs = get_fs();
    struct file *fp = (struct file *)fd;

    set_fs(get_ds());
    ret = filp_close(fp,NULL);
    set_fs (oldfs);

    return ret;
}

ca_int OS_FILE_READ(ca_int fd, ca_int8 *buf, ca_int32 readlen)
{
    mm_segment_t oldfs = get_fs();
    ca_int fileret = -1;
    struct file *fp = (struct file *)fd;

    set_fs(get_ds());

    if (fp->f_op && fp->f_op->read)
    {
		fileret = fp->f_op->read(fp,buf,readlen, &fp->f_pos);
    }
    else if (fp->f_op && fp->f_op->read_iter)
    {
		struct iovec iov = { .iov_base = buf, .iov_len = readlen };
		struct kiocb kiocb;
		struct iov_iter iter;

		init_sync_kiocb(&kiocb, fp);
		kiocb.ki_pos = fp->f_pos;

		iov_iter_init(&iter, READ, &iov, 1, readlen);

		fileret = fp->f_op->read_iter(&kiocb, &iter);
		if(0 < fileret)
			fp->f_pos = kiocb.ki_pos;

    }

    set_fs(oldfs);

    return fileret;
}

ca_int OS_FILE_WRITE(ca_int fd, ca_int8 *buf, ca_int32 len)
{
    mm_segment_t oldfs = get_fs();
    ca_int fileret = -1;
    struct file *fp = (struct file *)fd;

    set_fs(get_ds());
    if (fp->f_op && fp->f_op->write)
    {
		fileret = fp->f_op->write(fp,buf,len, &fp->f_pos);
    }
    else if (fp->f_op && fp->f_op->write_iter)
    {
		struct iovec iov = { .iov_base = (void __user *)buf, .iov_len = len };
		struct kiocb kiocb;
		struct iov_iter iter;

		init_sync_kiocb(&kiocb, fp);
		kiocb.ki_pos = fp->f_pos;

		iov_iter_init(&iter, WRITE, &iov, 1, len);

		fileret = fp->f_op->write_iter(&kiocb, &iter);
		if (0 < fileret)
			fp->f_pos = kiocb.ki_pos;
    }

    set_fs(oldfs);

    return fileret;
}

ca_int OS_FILE_SIZE(ca_int fd)
{
    struct file *file = (struct file *)fd;
    struct inode *inode = NULL;
    ca_int size = 0;

    if(file == NULL)
        return 0;
    inode = file->f_path.dentry->d_inode;
    size = (ca_int)inode->i_size;

    if ( size != 0 ) {
        return size;
    }
    return file->f_inode->i_size;
}

ca_int OS_FILE_CP(const char *dst, const char *src)
{
    struct file *fp_dst = NULL;
    struct file *fp_src = NULL;
    struct inode *inode_src = NULL;
    mm_segment_t old_fs = get_fs();
    ca_char *buf = NULL;
    ca_int32 size = 0;
    ca_int retval = 0;

    set_fs(KERNEL_DS);
    fp_dst = filp_open(dst, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
    if (IS_ERR(fp_dst)){
        printk("open dst %s failure.\r\n", dst);
        set_fs(old_fs);
        return -1;
    }
    fp_src = filp_open(src, O_RDONLY, 0);
    if (IS_ERR(fp_src)){
        printk("open src %s failure.\r\n", src);
        filp_close(fp_dst, NULL);
        set_fs(old_fs);
        return -1;
    }

    /* get the src file size. */
    inode_src = fp_src->f_path.dentry->d_inode;
    size = (ca_int32)inode_src->i_size;

    /* malloc buf */
    buf = kmalloc(size, GFP_KERNEL);
    if (NULL == buf){
        printk("malloc buf failure.\r\n");
        filp_close(fp_dst, NULL);
        filp_close(fp_src, NULL);
        set_fs(old_fs);
        return -1;
    }

    /*read file*/
    if (!(fp_src->f_op))
    {
        printk("file %s is unreadable.\r\n", src);
        goto err_ret;
    }

    if (fp_src->f_op && fp_src->f_op->read)
    {
		retval = (ca_int)fp_src->f_op->read(fp_src, buf, size, &fp_src->f_pos);
    }
    else if (fp_src->f_op && fp_src->f_op->read_iter)
    {
		struct iovec iov = { .iov_base = buf, .iov_len = size };
		struct kiocb kiocb;
		struct iov_iter iter;

		init_sync_kiocb(&kiocb, fp_src);
		kiocb.ki_pos = fp_src->f_pos;

		iov_iter_init(&iter, READ, &iov, 1, size);

		retval = fp_src->f_op->read_iter(&kiocb, &iter);
		if (0 < retval)
			fp_src->f_pos = kiocb.ki_pos;
    }
	else
	{
        printk("read fail in file %s.\r\n", src);
        goto err_ret;
	}

    if(0>retval){
        printk("read file %s failure.\r\n", src);
        retval = -1;
        goto err_ret;
    }

    /*write file*/
    if(!(fp_dst->f_op)){
        printk("file %s is unwritable.\r\n", dst);
        goto err_ret;
    }

	if (fp_dst->f_op && fp_dst->f_op->write)
    {
		retval = (ca_int)fp_dst->f_op->write(fp_dst, buf, size, &fp_dst->f_pos);
    }
    else if (fp_dst->f_op && fp_dst->f_op->write_iter)
    {
		struct iovec iov = { .iov_base = (void __user *)buf, .iov_len = size };
		struct kiocb kiocb;
		struct iov_iter iter;

		init_sync_kiocb(&kiocb, fp_dst);
		kiocb.ki_pos = fp_dst->f_pos;

		iov_iter_init(&iter, WRITE, &iov, 1, size);

		retval = fp_dst->f_op->write_iter(&kiocb, &iter);
		if (0 < retval)
			fp_dst->f_pos = kiocb.ki_pos;
    }
	else
	{
        printk("write fail in file %s.\r\n", dst);
        goto err_ret;
	}

    if(0>retval){
        printk("write file %s failure.\r\n", dst);
        retval = -1;
        goto err_ret;
    }

err_ret:
    filp_close(fp_dst, NULL);
    filp_close(fp_src, NULL);
    set_fs(old_fs);
    kfree(buf);
    return retval;
}

ca_int OS_CURRENT_THREAD(void)
{
    pid_t pid = 0;
    struct task_struct *p = (struct task_struct *)current;

    pid = p->pid;
    return (ca_int)pid;
}

ca_int OS_THREAD_CREATE(ca_uint *thread_id , const ca_int8 *thread_name,
                        const void *function_pointer , void *param,
                        ca_uint32 stack_size,
                        ca_uint32 priority, ca_uint32 flags)
{
    struct task_struct *task = NULL;

    task = (struct task_struct *)kthread_run(function_pointer , param , thread_name);
    if(task == NULL)
        return -1;

    *thread_id = (ca_uint)task;

    return 0;
}

ca_int OS_QUEUE_CREATE
    (
    ca_uint       *queue_id,
    const ca_int8 *queue_name,
    ca_uint32     queue_depth,
    ca_uint32     data_size,
    ca_uint32     flags
    )
{
    return -1;
}

ca_int OS_QUEUE_DELETE
    (
    ca_uint queue_id
    )
{
    return -1;
}

ca_int OS_QUEUE_GET
    (
    ca_uint queue_id,
    void      *data,
    ca_uint32 size,
    ca_uint32 *size_copied,
    ca_uint32  timeout
    )
{
    return -1;
}

ca_int OS_QUEUE_PUT
    (
    ca_uint     queue_id,
    void        *data,
    ca_uint32   size,
    ca_int32    timeout,
    ca_uint32   priority
    )
{
    return -1;
}

ca_uint32 OS_STRTOUL(
    const char *nptr,
    char **endptr,
    int base){
    return simple_strtoul(nptr, endptr, base);
}

void OS_DO_DIV(ca_uint64 dividend, ca_uint32 divisor, ca_uint64 *quotient, ca_uint32 *remainder)
{
    *quotient = dividend;
    *remainder = do_div(*quotient, divisor);
}

MODULE_LICENSE("GPL");

/*lint -restore*/

