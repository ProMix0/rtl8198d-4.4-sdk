#include "ca_types.h"
#include "os_core.h"
#include "osal_common.h"
#include "pal_comm.h"

/*lint -save -w0*/

#define CA_PAL_IOCTL_NAME   "ca_pal_ioctl"
#define CA_PAL_IOCTL_MAGIC  'C'
#define CA_PAL_PAGE_SIZE    (4096)
#define CA_PAL_MAX_CFG_LEN  (1*CA_PAL_PAGE_SIZE)

#ifdef __KERNEL__
#define PAL_IOCTL_MAX_CMD_NUM   MAX_CMD_NUM

typedef struct _ca_ioctl_cmd_t
{
    ca_int32_t cmd_type;
    ca_int32_t (*ioctl_cb)(cap_msg_t *pMsg);
} ca_pal_ioctl_cmd_t;

ca_pal_ioctl_cmd_t  ca_pal_ioctl_cmd[MAX_CMD_NUM];
unsigned long pal_dev_cfg = 0;
static struct file_operations ca_pal_ioctl_fops;
static struct class *ca_pal_ioctl_class;
static ca_int32_t ca_pal_ioctl_major = 0;

static long ca_pal_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    ca_int32_t cmd_nr;
    cap_msg_t *pMsg = NULL;
    ca_int32_t ret = CA_PAL_ERR;
    ca_uint8_t* cfg_buff = (ca_uint8_t *)pal_dev_cfg;

    if (_IOC_TYPE(cmd) != CA_PAL_IOCTL_MAGIC)
        return ret;

    if ((cmd_nr = _IOC_NR(cmd)) > PAL_IOCTL_MAX_CMD_NUM)
        return ret;

    if (ca_pal_ioctl_cmd[cmd_nr].ioctl_cb == NULL) {
        ca_printf("Found no command routine.\n");
        return ret;
    }

    if (ca_pal_ioctl_cmd[cmd_nr].cmd_type == CA_SET_CMD) {
        pMsg = (cap_msg_t *)cfg_buff;
        if (copy_from_user(cfg_buff, (ca_int8_t *)arg, CA_MSG_HDR_LEN))
            return ret;

        if (copy_from_user(cfg_buff, (ca_int8_t *)arg, pMsg->length + CA_MSG_HDR_LEN))
            return ret;

        ret = ca_pal_ioctl_cmd[cmd_nr].ioctl_cb(pMsg);
        return ret;
    }

    if (ca_pal_ioctl_cmd[cmd_nr].cmd_type == CA_GET_CMD) {
        pMsg = (cap_msg_t *)cfg_buff;
        if (copy_from_user(cfg_buff, (ca_int8_t *)arg, CA_MSG_HDR_LEN))
            return CA_PAL_ERR;
        if (copy_from_user(cfg_buff, (ca_int8_t *)arg, CA_MSG_HDR_LEN + pMsg->length))
            return CA_PAL_ERR;

        ret = ca_pal_ioctl_cmd[cmd_nr].ioctl_cb(pMsg);
        if(copy_to_user((ca_int8_t *)arg , pMsg , pMsg->length + CA_MSG_HDR_LEN))
            return CA_PAL_ERR;
        return ret;
    }

    return ret;
}


ca_int32  ca_pal_kioctl_init(void)
{
    ca_int32_t size;
    unsigned long addr = 0;

    memset(&ca_pal_ioctl_fops , 0 , sizeof(struct file_operations));
    ca_pal_ioctl_fops.unlocked_ioctl = (void *)ca_pal_ioctl;
    ca_pal_ioctl_major = register_chrdev(0, CA_PAL_IOCTL_NAME , &ca_pal_ioctl_fops);
    if (ca_pal_ioctl_major < 0) {
        ca_printf("ca-pal: init: failed to register chrdev %s\n", CA_PAL_IOCTL_NAME);
        return -1;
    } else {
        ca_printf("ca-pal: init %s OK\n", CA_PAL_IOCTL_NAME);
    }
    ca_pal_ioctl_class = class_create(THIS_MODULE, CA_PAL_IOCTL_NAME);
    if (IS_ERR(ca_pal_ioctl_class)) {
        ca_printf("ca-pal: init: failed to create class %s\n", CA_PAL_IOCTL_NAME);
        return -1;
    }

    if (NULL == device_create(ca_pal_ioctl_class, NULL, MKDEV(ca_pal_ioctl_major, 0), NULL, CA_PAL_IOCTL_NAME)) {
        return -1;
    }

    pal_dev_cfg =__get_free_pages(GFP_KERNEL, get_order(CA_PAL_MAX_CFG_LEN));
    if(!pal_dev_cfg)
    {
        //ca_printf("Create dev_cfg failed\n");
        ca_printf("ca-pal: init: failed to get free pages\n");
        device_destroy(ca_pal_ioctl_class, MKDEV(ca_pal_ioctl_major, 0));
        class_destroy(ca_pal_ioctl_class);
        unregister_chrdev(ca_pal_ioctl_major, CA_PAL_IOCTL_NAME);
        return -1;
    }
    for (addr = pal_dev_cfg, size = CA_PAL_MAX_CFG_LEN; size > 0; addr += PAGE_SIZE, size -= PAGE_SIZE)
    {
        SetPageReserved(virt_to_page(addr));
    }
    return 0;
}

void ca_pal_kioctl_exit(void)
{
    ca_int32_t size;
    unsigned long addr = 0;

    unregister_chrdev(ca_pal_ioctl_major, CA_PAL_IOCTL_NAME);
    device_destroy(ca_pal_ioctl_class, MKDEV(ca_pal_ioctl_major, 0));
    class_destroy(ca_pal_ioctl_class);
    if(pal_dev_cfg)
    {
        for (addr = pal_dev_cfg, size = CA_PAL_MAX_CFG_LEN; size > 0; addr += PAGE_SIZE, size -= PAGE_SIZE)
        {
            ClearPageReserved(virt_to_page(addr));
        }
        free_pages(pal_dev_cfg, get_order(CA_PAL_MAX_CFG_LEN));
        pal_dev_cfg = 0;
    }

    printk(KERN_ALERT"%s unregister successfully\n", CA_PAL_IOCTL_NAME);
}

ca_int32_t ca_pal_cmd_register(ca_int32_t cmd , ca_int32_t cmd_type , ca_int32_t(*callback)(cap_msg_t *pMsg))
{
    if (cmd < 0 || cmd >= MAX_CMD_NUM || callback == NULL)
        return CA_PAL_ERR;

    if (ca_pal_ioctl_cmd[cmd].cmd_type == cmd_type &&
            ca_pal_ioctl_cmd[cmd].ioctl_cb == callback)
        return CA_PAL_OK;

    if (ca_pal_ioctl_cmd[cmd].cmd_type == cmd_type &&
            ca_pal_ioctl_cmd[cmd].ioctl_cb != callback &&
            ca_pal_ioctl_cmd[cmd].ioctl_cb != NULL) {
        ca_printf("This cmd has been already registered.\n");
        return CA_PAL_ERR;
    }

    ca_pal_ioctl_cmd[cmd].cmd_type = cmd_type;
    ca_pal_ioctl_cmd[cmd].ioctl_cb = callback;

    return CA_PAL_OK;
}

static void ca_plat_recv(struct sk_buff *skb)
{
    ca_printf("<%s, %d>\r\n", __FUNCTION__, __LINE__);
    return;
}

struct sock     *evt_nl = NULL;

static struct netlink_kernel_cfg nl_cfg_plat = {
	.input  = ca_plat_recv,
};

int ca_pal_init(void)
{
    ca_pal_kioctl_init();
	evt_nl = netlink_kernel_create(&init_net, NETLINK_PLAT, &nl_cfg_plat);
	if (evt_nl == NULL) {
		printk("%s: Create plat netlink failed!!\n", __func__);
                return -1;
	}
	return 0;
}

#else

ca_int32_t ca_pal_ioctl(ca_int32_t cmd, cap_msg_t *pMsg)
{
    int ioctl_fd;
    ca_int8_t dev_name[32];

    if (pMsg == NULL)
        return CA_PAL_ERR;

    if(pMsg->length + CA_MSG_HDR_LEN > CA_PAL_MAX_CFG_LEN)
    {
        ca_printf("ioctl: input param is too long.\n");
        return CA_PAL_ERR;
    }

    memset(dev_name , 0 , sizeof(dev_name));
    sprintf(dev_name, "/dev/%s", CA_PAL_IOCTL_NAME);
    ioctl_fd = open(dev_name, O_RDWR);
    if (ioctl_fd != -1) {
        ca_int32_t ret = -1;
        ret = ioctl(ioctl_fd, _IO(CA_PAL_IOCTL_MAGIC, cmd), pMsg);
        if (ret == CA_PAL_ERR)
            ca_printf("Can not run command '%d', ret=%d.\n", cmd, ret);

        close(ioctl_fd);
        return ret;
    }else{
        printf("Device open failure /dev/"CA_PAL_IOCTL_NAME"\n");
        return CA_PAL_ERR;
    }
}

#endif
/*lint -restore*/


