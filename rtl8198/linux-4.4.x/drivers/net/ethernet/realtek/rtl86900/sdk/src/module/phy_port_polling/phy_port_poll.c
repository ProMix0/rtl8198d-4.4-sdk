/*
 * Copyright (C) 2011 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : esd detection
 *
 * Feature : Use kernel thread to perodically polling the esd state, if state is abnormal, try to make it normal.
 *
 */

 /*
  * Include Files
  */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/* For RTK APIs */
#include <common/rt_error.h>
#include <rtk/port.h>
#include <rtk/mdio.h>
#include <rtk/gpio.h>

/*
 * Data Declaration
 */
unsigned int esd_detection_flag = 0;
extern struct proc_dir_entry *realtek_proc;
struct task_struct *esdDetectionTask;

#define BIT(nr)			(1UL << (nr))

int probe_esd_detection(void)
{
	unsigned int reg0,reg4;
	unsigned int port,ret;

	for(port=0;port<4;port++){
		ret=rtk_port_phyReg_get(port, 0, 0, &reg0);
		if(ret!=RT_ERR_OK)
			return ret;
		if(reg0 & BIT(11)){
			reg0 |= BIT(15);
			reg4 = 0xDE1;
			ret=rtk_port_phyReg_set(port,0,0,reg0);
			if(ret!=RT_ERR_OK){
				printk("[%s:%d]\n",__FUNCTION__,__LINE__);
				return ret;
			}
			rtk_port_phyReg_set(port,0,4,reg4);
			if(ret!=RT_ERR_OK){
				printk("[%s:%d]\n",__FUNCTION__,__LINE__);
				return ret;
			}
		}
	}

	return RT_ERR_OK;

}

int esd_detection_thread(void *data)
{
	while(!kthread_should_stop())
	{
		/* No need to wake up earlier */
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(5 * HZ);
		if(esd_detection_flag){
			probe_esd_detection();
		}
	}
	return 0;
}

static int esd_detection_read(struct seq_file *s, void *v)
{
	seq_printf(s, "[esd_detection_flag = %d]\n", esd_detection_flag);
	seq_printf(s, "1: enable esd detection\n");

	return 0;
}

static int esd_detection_open(struct inode *inode, struct file *file)
{
	return single_open(file, esd_detection_read, (void *) file);
}

static ssize_t esd_detection_write(struct file * file, const char __user * buffer, size_t count, loff_t * off)
{
	unsigned char tmpBuf[64] = {0};
	char	*strptr, *cmd_addr, *tokptr;

	if(count>=64)
		goto errout;
	
	if (buffer && !copy_from_user(tmpBuf, buffer, count))
	{
		tmpBuf[count] = '\0';
		strptr=tmpBuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
			goto errout;

		if (!memcmp(cmd_addr, "help", 4)) {
			printk("echo [esd_detection_flag] > /proc/realtek/esd_detection\n");
			printk("1:open esd_detection 0:close esd_detection > /proc/realtek/esd_detection\n");
		}
		else {
			esd_detection_flag = simple_strtoul(cmd_addr, NULL, 16);
			printk("write esd_detection_flag to 0x%08x\n", esd_detection_flag);
		}
		return count;
	}
errout:
	return -EFAULT;
}

static const struct file_operations esd_detection_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= esd_detection_open,
	.read		= seq_read,
	.write		= esd_detection_write,
	.release	= single_release,
};

static void esd_detection_dbg_init(void)
{
	/* Create proc debug commands */
	if (proc_create("esd_detection", 0, realtek_proc, &esd_detection_proc_fops) == NULL) {
		printk("Fail to create esd_detection proc entry.\n");
	}
}

static void esd_detection_dbg_exit(void)
{
	/* Remove proc debug commands */
	remove_proc_entry("esd_detection", realtek_proc);
}

int __init esd_detection_init(void)
{
	esdDetectionTask = kthread_create(esd_detection_thread, NULL, "esd_detection");
	if(IS_ERR(esdDetectionTask))
	{
		printk("[%s-%d] init failed %ld!\n", __FILE__, __LINE__, PTR_ERR(esdDetectionTask));
	}
	else
	{
		wake_up_process(esdDetectionTask);
	}
	esd_detection_dbg_init();

	return 0;
}

void __exit esd_detection_exit(void)
{
	printk("[%s-%d] exit!\n", __FILE__, __LINE__);
	kthread_stop(esdDetectionTask);
	esd_detection_dbg_exit();
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek esd auto detection module");
MODULE_AUTHOR("Sarah_shi <sarah_shi@realsil.com.cn>");
module_init(esd_detection_init);
module_exit(esd_detection_exit);

