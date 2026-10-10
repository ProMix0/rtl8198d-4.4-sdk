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
#include <net/sock.h>
#include <linux/version.h>
#include <ca_types.h>
#include "osal_common.h"
#include "ca_sdk_linux.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cortina Access Inc");
MODULE_DESCRIPTION("SDK-KERN");
extern ca_int32 ca_sdk_kern_api_cb(ca_sdk_msg_t *pMsg);

static struct file_operations   ca_ioctl_fops;
#if LINUX_VERSION_CODE >= 0x020618
static struct class *sdk_ioctl_class;
#endif
static ca_int32 sdk_ioctl_major = 0;


static long ca_sdk_dev_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
    ca_int32 ret = -1;
    ca_int8 * cfg_value = NULL;
    ca_sdk_msg_t *pMsg = NULL;

    if(cmd != 0){
        return -1;
    }

    cfg_value = ca_malloc(CA_SDK_MSG_MAX_LEN);
    if(cfg_value == NULL){
        return -1;
    }

    memset(cfg_value, 0, CA_SDK_MSG_MAX_LEN);

    pMsg = (ca_sdk_msg_t*) cfg_value;

    if (copy_from_user(cfg_value, (ca_int8 *)arg, CA_SDK_MSG_HDR_LEN)){
        ca_free(cfg_value);
        return -1;
    }

    if (copy_from_user(cfg_value, (ca_int8 *)arg, CA_SDK_MSG_HDR_LEN + pMsg->len)){
        ca_free(cfg_value);
        return -1;
    }

    ret = ca_sdk_kern_api_cb(pMsg);

    copy_to_user((ca_int8 *)arg , pMsg , pMsg->len + CA_SDK_MSG_HDR_LEN);

    ca_free(cfg_value);
    return ret;
}


ca_int32 __init ca_sdk_ioctl_init(void)
{
#if LINUX_VERSION_CODE < 0x020618
#ifdef CONFIG_DEVFS_FS
    int ret  =0;
#endif
#endif

    memset(&ca_ioctl_fops , 0 , sizeof(struct file_operations));
#if LINUX_VERSION_CODE < 0x020611
    ca_ioctl_fops.ioctl = (void *)ca_sdk_dev_ioctl;
#else
    ca_ioctl_fops.unlocked_ioctl = (void *)ca_sdk_dev_ioctl;
#endif
    sdk_ioctl_major = register_chrdev(0, CA_SDK_IOCTL_NAME , &ca_ioctl_fops);
    if (sdk_ioctl_major < 0) {
        printk(KERN_ALERT"sdk ioctl init failure\n");
        return -1;
    } else {
        printk(KERN_ALERT"sdk ioctl init success\n");
    }
#if LINUX_VERSION_CODE >= 0x020618
    sdk_ioctl_class = class_create(THIS_MODULE, CA_SDK_IOCTL_NAME);
    if (IS_ERR(sdk_ioctl_class)) {
        printk(KERN_ALERT"%s create class faild!\n", CA_SDK_IOCTL_NAME);
        return -1;
    }

    if (NULL == device_create(sdk_ioctl_class, NULL, MKDEV(sdk_ioctl_major, 0), NULL, CA_SDK_IOCTL_NAME)) {
        return -1;
    }
#else
#ifdef CONFIG_DEVFS_FS
    ret = devfs_mk_cdev(MKDEV(sdk_ioctl_major, 0), S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP, CA_SDK_IOCTL_NAME);
#endif
#endif

    return 0;
}

void __exit ca_sdk_ioctl_exit(void)
{
    unregister_chrdev(sdk_ioctl_major, CA_SDK_IOCTL_NAME);
#if LINUX_VERSION_CODE >= 0x020618
    device_destroy(sdk_ioctl_class, MKDEV(sdk_ioctl_major, 0));
    class_destroy(sdk_ioctl_class);
#endif
    printk(KERN_ALERT"%s unregister successfully\n", CA_SDK_IOCTL_NAME);
}



module_init(ca_sdk_ioctl_init);
module_exit(ca_sdk_ioctl_exit);
