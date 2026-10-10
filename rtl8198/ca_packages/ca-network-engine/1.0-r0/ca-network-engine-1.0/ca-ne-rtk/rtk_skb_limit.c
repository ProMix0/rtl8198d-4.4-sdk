#include <generated/ca_ne_autoconf.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <soc/cortina/cortina-soc.h>
#include "ca_types.h"

#define RTK_SKB_DEBUG  0
#ifndef pr_fmt
#define pr_fmt(fmt)     KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__
#endif
unsigned long ni_skb_limit_kb = 12288;//12MB
#define K(x) ((x) << (PAGE_SHIFT - 10))
extern long si_mem_available(void);

int rtk_netdev_alloc_skb_limit(void){
        unsigned long free_page;
        if(ni_skb_limit_kb == 0){
                return 0;
        }
        free_page = si_mem_available();

        if (K(free_page) < ni_skb_limit_kb)     {
                printk_ratelimited("[%s %d] %lu, %lu\n", __func__, __LINE__, K(free_page), ni_skb_limit_kb);
                return 1;
        }
        return 0;
}



static int ni_skb_limit_show(struct seq_file *m, void *v)
{

        seq_printf(m, "When Kernel MemAvailable < ni_skb_limit_kb(%lu), ca_ni is not allowed to alloc skb.\n", ni_skb_limit_kb);

        return 0;
}

static int ni_skb_limit_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, ni_skb_limit_show, (void *) file);
}

static ssize_t ni_skb_limit_write(struct file *file, const char __user *buff, size_t len, loff_t *ppos)
{
        char                    tmpbuf[64];

        if (buff && !copy_from_user(tmpbuf, buff, len))
        {
                tmpbuf[len] = '\0';
                ni_skb_limit_kb=simple_strtoul(tmpbuf, NULL, 0);
                //printk("ni_skb_limit_kb = %lu\n", ni_skb_limit_kb);
        }
        else
        {
                printk("ni_skb_limit_write operation is wrong\n");
        }

        return len;
}


const struct file_operations fops_ni_skb_limit __read_mostly = {
        .owner          = THIS_MODULE,
        .open           = ni_skb_limit_proc_open,
        .read           = seq_read,
        .write          = ni_skb_limit_write,
        .release	= single_release,
};

