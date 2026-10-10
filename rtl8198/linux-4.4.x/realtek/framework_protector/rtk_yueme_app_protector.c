/*
* ----------------------------------------------------------------
* Copyright c                  Realtek Semiconductor Corporation, 2016
* All rights reserved.
*
* Abstract: For YUEME APP Framework Protection
* ---------------------------------------------------------------
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/mempolicy.h>
#include <linux/statfs.h>
#include <linux/cgroup.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/hashtable.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include "../../kernel/sched/sched.h"

/********************************************
 * Debug purpose,
 * Limit Normal Linux processes to experiment
 * #define NORMAL_PROCESS_TEST
 *******************************************/
//#define NORMAL_PROCESS_TEST     

//#define RTL_APP_DEBUG
#if defined(RTL_APP_DEBUG)
#define PRINTK_APPD(fmt, ...)   printk("[DEBUG:(%s,%d)]" fmt, __FUNCTION__ ,__LINE__,##__VA_ARGS__)
#else
#define PRINTK_APPD(fmt, ...)
#endif

#define  MIN_PROCESS_CNT          (64)	//essential processes

static char group_path[PATH_MAX];
static const char appframework_path[] = "/lxc/saf/lxc/";

char *task_group_path1(struct task_struct *pt)
{
	struct task_group *tg = task_group(pt);
#if defined(CONFIG_SCHED_AUTOGROUP) || defined(CONFIG_SCHED_DEBUG)
	if (autogroup_path(tg, group_path, PATH_MAX))
		return group_path;
#endif
	return cgroup_path(tg->css.cgroup, group_path, PATH_MAX);
}

int rtk_appframework_path_check(struct task_struct *pt)
{
	/* Bypass some process
	   1. Kthread
	   2. Process_id <= (32), like init(1)
	 */
	if ((pt->pid <= MIN_PROCESS_CNT) || (pt->flags & PF_KTHREAD)) {
		return false;
	}
	if (strncmp
	    (task_group_path1(pt), appframework_path,
	     strlen(appframework_path)) == 0) {
		PRINTK_APPD("task_group_path1(pt) = %s\n",
			    task_group_path1(pt));
		return true;
	}
#ifdef NORMAL_PROCESS_TEST
	//Just for debug, not check wether process is cgroup 
	return true;
#endif
	return false;

}

static inline int rtk_appframework_child_check(struct task_struct *pt)
{
	struct task_struct *p, *n;
	int child_cnt = 0;
	read_lock(&tasklist_lock);
	list_for_each_entry_safe(p, n, &pt->children, sibling) {
		child_cnt++;
	}
	read_unlock(&tasklist_lock);
	return child_cnt;
}

#ifdef CONFIG_YUEME_APP_FORK_LIMIT_FEATURE
#define RTL_APP_FORK_INHIBIT     1
#define RTL_APP_FORK_ALLOW       0
unsigned long app_limit = CONFIG_YUEME_APP_FORK_LIMIT_NUM;

int rtk_appframework_app_check(void)
{
	int child_cnt = 0;
	child_cnt = rtk_appframework_child_check(current);
	if (child_cnt >= app_limit) {
		if (rtk_appframework_path_check(current)) {
			if (printk_ratelimit()) {
				printk(KERN_WARNING
				       "\033[1;31m[%s] current process(%s, %d), parent(%s,%d),current children_cnt(%d)\033[0m\n",
				       __func__, current->comm, current->pid,
				       current->parent->comm,
				       current->parent->pid, child_cnt);
			}
			return RTL_APP_FORK_INHIBIT;
		}
	}
	return RTL_APP_FORK_ALLOW;
}

/*Internal switch option*/
#define THREAD_LIMIT 1

#ifdef THREAD_LIMIT
extern unsigned long app_thread_limit;
int rtk_appframework_app_thread_limit(void)
{
	/*
	 * thread_group_leader and child thread is reached app_limit
	 * and the application is in cgroup "/lxc/saf/lxc/" for YUEME framework
	 */
	if (thread_group_leader(current)
	    && (current->signal->nr_threads > app_thread_limit)
	    && rtk_appframework_path_check(current)) {
		if (printk_ratelimit()) {
			printk(KERN_WARNING
			       "\033[1;31mCurrent task(pid: %d, name: %s, Threads: %u ) reached to YUEME_APP_THREAD_LIMIT_NUM(%lu)!\033[0m\n",
			       current->pid, current->comm,
			       current->signal->nr_threads, app_thread_limit);
		}
		return RTL_APP_FORK_INHIBIT;
	}
	return RTL_APP_FORK_ALLOW;
}
#else
int rtk_appframework_app_thread_limit(void)
{
	return RTL_APP_FORK_ALLOW;
}
#endif

#define APP_M_HASH_BITS       9

extern char *task_group_path1(struct task_struct *pt);

#ifdef THREAD_LIMIT
unsigned long app_thread_limit = CONFIG_YUEME_APP_THREAD_LIMIT_NUM;
#endif
#ifdef CONFIG_YUEME_APP_FORK_LIMIT_KILLER

static DEFINE_HASHTABLE(app_m_table, APP_M_HASH_BITS);
struct app_m {
	struct hlist_node hlist;
	pid_t ppid;
	int count;
	char comm[16];
};

#ifdef NORMAL_PROCESS_TEST
int app_limit_seconds = 20;
#else
int app_limit_seconds = 120;
#endif
struct task_struct *appframe_monitor_task;

static int app_m_hash(int pid)
{
	return (pid % 512);
}

static struct app_m *appm_hash_search(pid_t pid)
{
	int key = app_m_hash(pid);
	struct app_m *app_m_p;
	hash_for_each_possible(app_m_table, app_m_p, hlist, key) {
		if (app_m_p->ppid != pid)
			continue;
		/* This css_set matches what we need */
		return app_m_p;
	};
	return NULL;
}

static void appm_hash_add(struct task_struct *p)
{
	struct app_m *app_m_p;

	app_m_p = kmalloc(sizeof(struct app_m), GFP_ATOMIC);
	if (!app_m_p)
		return;

	INIT_HLIST_NODE(&app_m_p->hlist);
	memcpy(app_m_p->comm, p->comm, 16);
	app_m_p->ppid = p->parent->pid;
	app_m_p->count = 1;
	hash_add(app_m_table, &app_m_p->hlist, app_m_hash(app_m_p->ppid));
}

static void appm_hash_clear(void)
{
	int num;
	struct app_m *app_m_p;
	struct hlist_node *tmp;

	//     hash_for_each(app_m_table, num, app_m_p, hlist) {
	hash_for_each_safe(app_m_table, num, tmp, app_m_p, hlist) {
		hash_del(&app_m_p->hlist);
		kfree(app_m_p);
	};

	return;
}

 /*
  * The process p may have detached its own ->mm while exiting or through
  * use_mm(), but one or more of its subthreads may still have a valid
  * pointer.  Return p, or any of its subthreads with a valid ->mm, with
  * task_lock() held.
  */
#if 0
static struct task_struct *find_lock_task_mm(struct task_struct *p)
{
	struct task_struct *t;

	rcu_read_lock();

	for_each_thread(p, t) {
		task_lock(t);
		if (likely(t->mm))
			goto found;
		task_unlock(t);
	}
	t = NULL;
 found:
	rcu_read_unlock();

	return t;
}
#endif
extern long sys_kill(int pid, int sig);
static void appframe_kill_process(struct task_struct *selectedp)
{
	int total_process = 0;

	struct task_struct *p;
	struct task_struct *parent;
	struct task_struct *pparent;
	int parent_pid = 0;
	int kill_cnt = 0;
	printk("[appframework_monitor]vitim:%s (%d) ; parent:%s (%d)\n",
	       selectedp->comm, selectedp->pid, selectedp->parent->comm,
	       selectedp->parent->pid);
	parent = selectedp->parent;
	pparent = parent->parent;
	//rcu_read_lock();
	read_lock(&tasklist_lock);
	parent_pid = selectedp->parent->pid;
	for_each_process(p) {
		if (p->parent->pid == selectedp->parent->pid) {
#if 0
			task_lock(p);	/* Protect ->comm from prctl() */
			printk_once("Kill process %d (%s) sharing parent id\n",
				    task_pid_nr(p), p->comm);
			task_unlock(p);
#endif
			kill_cnt++;
			do_send_sig_info(SIGKILL, SEND_SIG_FORCED, p, true);
		} else {
			total_process++;
		}
	}
	//rcu_read_unlock();
	read_unlock(&tasklist_lock);
	if (parent) {
		printk
		    ("[appframework_monitor] Kill process with parent pid:%d(%s). %d Childs\n",
		     parent->pid, parent->comm, kill_cnt);
		do_send_sig_info(SIGKILL, SEND_SIG_FORCED, parent, true);
		total_process--;
		if (pparent->pid > 63) {
			printk
			    ("[appframework_monitor]Kill pparent process pid:%d(%s).\n",
			     pparent->pid, pparent->comm);
			do_send_sig_info(SIGKILL, SEND_SIG_FORCED, pparent,
					 true);
			total_process--;
		}
	}
}

static void appframework_monitor_main(void)
{
	struct task_struct *g, *p;
	struct app_m *app_m_p;
	struct task_struct *viticm_p = NULL;
	int viticm_count = 0;

	if (atomic_read(&current->real_cred->user->processes) <
	    (app_limit + MIN_PROCESS_CNT)) {
#ifdef DEBUG_APPF_KILLER
		printk
		    ("[appframework_monitor] Total_Process=%d. < (%lu).Nothing to do.\n",
		     atomic_read(&current->real_cred->user->processes),
		     (app_limit + MIN_PROCESS_CNT));
#endif
		/* Nothing to */
		return;
	}

	rcu_read_lock();
	for_each_process_thread(g, p) {
		if (rtk_appframework_path_check(p)) {

			app_m_p = appm_hash_search(p->parent->pid);
			if (app_m_p) {
				app_m_p->count = app_m_p->count + 1;
#ifdef DEBUG_APPF_KILLER
				printk
				    ("     parent_pid:%d , parent:%s, count:%d\n",
				     app_m_p->ppid, app_m_p->comm,
				     app_m_p->count);
#endif
				if ((app_m_p->count >= app_limit)
				    && (app_m_p->count > viticm_count)) {
					viticm_p = p;
					viticm_count = app_m_p->count;
				}
			} else {
				appm_hash_add(p);
			}
		}
	}

	appm_hash_clear();
	rcu_read_unlock();

	if (viticm_count) {
#ifdef DEBUG_APPF_KILLER
		printk("[appframework_monitor] kill  pid:%d ,%s, count:%d\n",
		       viticm_p->pid, viticm_p->comm, viticm_count);
#endif
		appframe_kill_process(viticm_p);
	}

	return;
}

static int appframework_monitor(void *arg)
{
	while (!kthread_should_stop()) {
		appframework_monitor_main();
		set_current_state(TASK_INTERRUPTIBLE);
		/* Now sleep */
		schedule_timeout(app_limit_seconds * HZ);
	}

	return 0;
}

static void appframework_app_monitor(void)
{

	appframe_monitor_task =
	    kthread_create(appframework_monitor, NULL, "appf_m");
	if (WARN_ON(!appframe_monitor_task)) {
		printk
		    ("appframework_app_monitor: create appframework_monitor failed!");
		goto out_free;
	}

	wake_up_process(appframe_monitor_task);
 out_free:
	return;

}

#endif

static int app_limit_num_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, " app_limit : %lu\n", app_limit);
	return 0;
}

static ssize_t app_limit_num_write(struct file *file,
				   const char __user * buf, size_t size,
				   loff_t * _pos)
{

	unsigned char tmpBuf[16] = { 0 };
	int len = (size > 15) ? 15 : size;
	int input_val = 0;
	if (buf && !copy_from_user(tmpBuf, buf, len)) {
		input_val = simple_strtoul(tmpBuf, NULL, 0);
		if (input_val > 0) {
			app_limit = input_val;
		} else {
			printk("Process limit num <= 0 \n");
		}
		return size;
	}
	return -EFAULT;
}

struct proc_dir_entry *yueme_app_proc_dir = NULL;

static int app_limit_num_open(struct inode *inode, struct file *file)
{
	return single_open(file, app_limit_num_show, inode->i_private);
}

static const struct file_operations luna_app_limit_fops = {
	.owner = THIS_MODULE,
	.open = app_limit_num_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = app_limit_num_write,
};

/* killer */
#ifdef CONFIG_YUEME_APP_FORK_LIMIT_KILLER
static int app_killertime_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, " appframework_app_monitor period time : %d (s)\n",
		   app_limit_seconds);
	return 0;
}

static ssize_t app_killertime_write(struct file *file,
				    const char __user * buf, size_t size,
				    loff_t * _pos)
{

	unsigned char tmpBuf[16] = { 0 };
	int len = (size > 15) ? 15 : size;
	int input_val = 0;
	if (buf && !copy_from_user(tmpBuf, buf, len)) {
		input_val = simple_strtoul(tmpBuf, NULL, 0);
		if ((input_val > 0) && (input_val < 86400)) {	//between 1s and 24 hours
			app_limit_seconds = input_val;
		} else if (input_val == 0) {
			if (appframe_monitor_task)
				kthread_stop(appframe_monitor_task);

			printk("Stop kthread[appf_m] \n");
			appframe_monitor_task = NULL;
		}
		return size;
	}
	return -EFAULT;
}

static int app_killertime_open(struct inode *inode, struct file *file)
{
	return single_open(file, app_killertime_show, inode->i_private);
}

static const struct file_operations bsp_luna_killer_fops = {
	.owner = THIS_MODULE,
	.open = app_killertime_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = app_killertime_write,
};

#endif

#ifdef THREAD_LIMIT
static int thread_limit_num_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, " app thread_limit : %lu\n", app_thread_limit);
	return 0;
}

static ssize_t thread_limit_num_write(struct file *file,
				      const char __user * buf, size_t size,
				      loff_t * _pos)
{

	unsigned char tmpBuf[16] = { 0 };
	int len = (size > 15) ? 15 : size;
	int input_val = 0;
	if (buf && !copy_from_user(tmpBuf, buf, len)) {
		input_val = simple_strtoul(tmpBuf, NULL, 0);
		if (input_val > 0) {
			app_thread_limit = input_val;
		} else {
			printk("Process limit num <= 0 \n");
		}
		return size;
	}
	return -EFAULT;
}

static int thread_limit_num_open(struct inode *inode, struct file *file)
{
	return single_open(file, thread_limit_num_show, inode->i_private);
}

static const struct file_operations bsp_luna_threadlimit_fops = {
	.owner = THIS_MODULE,
	.open = thread_limit_num_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = thread_limit_num_write,
};
#endif
#endif				//CONFIG_YUEME_APP_FORK_LIMIT_FEATURE

/********************************************************
 TEMPFS or UBIFS or YAFFS Protection
 It will check fs usage. If write will cause fs usage over
 CONFIG_YUEME_FS_LIMIT_PERCENTAGE, it will not allow write 
 operation
*********************************************************/
#ifdef CONFIG_YUEME_FS_CHECK_FEATURE
//#define RTL_FSW_DEBUG
#if defined(RTL_FSW_DEBUG)
#define PRINTK_FSWD(fmt, ...)   printk("[FSW:(%s,%d)]" fmt, __FUNCTION__ ,__LINE__,##__VA_ARGS__)
#else
#define PRINTK_FSWD(fmt, ...)
#endif

#define RTL_APP_FS_W_INHIBIT     1
#define RTL_APP_FS_W_ALLOW       0
#define RTL_APP_FS_PERCNT      CONFIG_YUEME_FS_LIMIT_PERCENTAGE
unsigned long yueme_appf_fs_check(struct file *file, loff_t pos, size_t count)
{
	int ret;
	long appf_fs_avail;
	long appf_fs_usead;
	struct kstatfs stats;

	/* If the process is not in cgroup ""/lxc/saf/lxc/""
	 * We don't limit the quota
	 */

	if (rtk_appframework_path_check(current) == 0) {
		return RTL_APP_FS_W_ALLOW;
	}

	ret = vfs_statfs(&file->f_path, &stats);
	if (ret == 0) {
		appf_fs_usead = stats.f_blocks - stats.f_bavail;
		appf_fs_avail =
		    (long)(stats.f_blocks * RTL_APP_FS_PERCNT) / 100;

		if (appf_fs_usead >= appf_fs_avail) {
			PRINTK_FSWD
			    (" appf_fs_usead = %ld, appf_fs_avail = %ld \n",
			     appf_fs_usead, appf_fs_avail);
			PRINTK_FSWD
			    (" %ld, %ld, %llu, %llu, %llu, %llu, %llu \n",
			     stats.f_type, stats.f_bsize, stats.f_blocks,
			     stats.f_bfree, stats.f_bavail, stats.f_files,
			     stats.f_ffree);
			PRINTK_FSWD
			    ("FS(yaffs) usage is higher than %d percent.\n",
			     RTL_APP_FS_PERCNT);
			if (printk_ratelimit())
				pr_warning
				    ("%s write(%zu Byte) inihibited. FS_usage over %d Percentage.\n",
				     current->comm, count, RTL_APP_FS_PERCNT);
			return RTL_APP_FS_W_INHIBIT;
		}

		if (((count / stats.f_bsize) + appf_fs_usead) >= appf_fs_avail) {
			PRINTK_FSWD
			    ("Write: %zu, appf_fs_usead = %ld, appf_fs_avail = %ld \n",
			     count, appf_fs_usead, appf_fs_avail);
			PRINTK_FSWD
			    ("Type: 0x%lx, Blcok_Size: %ld,  Total data block: %llu, Free blocks: %llu, Free blocks available: %llu, Total file nodes: %llu, Free file nodes: %llu \n",
			     stats.f_type, stats.f_bsize, stats.f_blocks,
			     stats.f_bfree, stats.f_bavail, stats.f_files,
			     stats.f_ffree);
			if (printk_ratelimit())
				pr_warning
				    ("%s write(%zu Byte) will cause FS_usage over %d Percentage.\n",
				     current->comm, count, RTL_APP_FS_PERCNT);
			return RTL_APP_FS_W_INHIBIT;
		}

	} else {
		PRINTK_FSWD("[%s.%d]ret =%d\n", __func__, __LINE__, ret);
	}

	return RTL_APP_FS_W_ALLOW;
}

#endif				//CONFIG_YUEME_FS_CHECK_FEATURE

#ifdef CONFIG_YUEME_TMPFS_FEATURE
#ifdef CONFIG_YUEME_TMPFS_FILE_SIZE
unsigned long tmpfs_file_size_limit = (CONFIG_YUEME_TMPFS_FILE_SIZE << 20);
#else
unsigned long tmpfs_file_size_limit = (256 << 20);	//256MB, by default.
#endif
static int tmp_file_size_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, " tmpfs's file size limit : %lu\n",
		   tmpfs_file_size_limit);
	return 0;
}

static ssize_t tmpfs_file_size_write(struct file *file,
				     const char __user * buf, size_t size,
				     loff_t * _pos)
{

	unsigned char tmpBuf[16] = { 0 };
	int len = (size > 15) ? 15 : size;
	int input_val = 0;
	if (buf && !copy_from_user(tmpBuf, buf, len)) {
		input_val = simple_strtoul(tmpBuf, NULL, 0);
		if (input_val >= (1 << 20)) {
			tmpfs_file_size_limit = input_val;
		} else {
			printk("tmp file size should > 1MB\n");
			tmpfs_file_size_limit = (1 << 20);
		}
		return size;
	}
	return -EFAULT;
}

struct proc_dir_entry *yueme_proc_dir = NULL;

static int tmpfs_file_size_open(struct inode *inode, struct file *file)
{
	return single_open(file, tmp_file_size_show, inode->i_private);
}

static const struct file_operations luna_tmpfs_fops = {
	.owner = THIS_MODULE,
	.open = tmpfs_file_size_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = tmpfs_file_size_write,
};

#endif				//CONFIG_YUEME_TMPFS_FEATURE

#define YUEME_APPLIMIT_PROC_DIR    "luna_yueme"

int __init luna_yueme_app_pro_init(void)
{

	yueme_proc_dir = proc_mkdir(YUEME_APPLIMIT_PROC_DIR, NULL);
	if (yueme_proc_dir == NULL) {
		printk("create /proc/%s failed!\n", YUEME_APPLIMIT_PROC_DIR);
		return 1;
	}
#ifdef CONFIG_YUEME_FS_CHECK_FEATURE
	if (!proc_create
	    ("tmpfs_file_size", 0444, yueme_proc_dir, &luna_tmpfs_fops)) {
		printk("create proc luna_yueme/tmpfs_file_size failed!\n");
		return 1;
	}
#endif

#ifdef CONFIG_YUEME_APP_FORK_LIMIT_FEATURE

	if (!proc_create
	    ("AppFramework_applimit", 0444, yueme_proc_dir,
	     &luna_app_limit_fops)) {
		printk("create proc %s/AppFramework_applimit failed!\n",
		       YUEME_APPLIMIT_PROC_DIR);
		return 1;
	}
#ifdef THREAD_LIMIT
	if (!proc_create
	    ("AppFramework_threadlimit", 0444, yueme_proc_dir,
	     &bsp_luna_threadlimit_fops)) {
		printk("create proc %s/AppFramework_threadlimit failed!\n",
		       YUEME_APPLIMIT_PROC_DIR);
		return 1;
	}
#endif
#ifdef CONFIG_YUEME_APP_FORK_LIMIT_KILLER
	if (!proc_create
	    ("appf_monitor_time", 0444, yueme_proc_dir,
	     &bsp_luna_killer_fops)) {
		printk("create proc %s/AppFramework_killer_time failed!\n",
		       YUEME_APPLIMIT_PROC_DIR);
		return 1;
	}
#endif
#ifdef CONFIG_YUEME_APP_FORK_LIMIT_KILLER
	appframework_app_monitor();
#endif
#endif				//CONFIG_YUEME_APP_FORK_LIMIT_FEATURE
	return 0;
}

void __exit luna_yueme_app_pro_exit(void)
{
	remove_proc_entry("tmpfs_file_size", yueme_proc_dir);
	proc_remove(yueme_proc_dir);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek YUEME APP Protector");
module_init(luna_yueme_app_pro_init);
module_exit(luna_yueme_app_pro_exit);
