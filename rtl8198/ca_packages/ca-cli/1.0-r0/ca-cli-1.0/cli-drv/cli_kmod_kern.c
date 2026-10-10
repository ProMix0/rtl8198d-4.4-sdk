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
//#include <asm/system.h>
#include <net/sock.h>
#include <linux/version.h>
#include "cli_cmn.h"
#include "cli_kmod_msg.h"
#include "cli_kmod_kern.h"

/*lint -e10*/ 
/*lint -e101*/ 
/*lint -e1075*/ 
/*lint -e115*/ 
/*lint -e119*/ 
/*lint -e129*/ 
/*lint -e155*/ 
/*lint -e160*/ 
/*lint -e18*/ 
/*lint -e2*/ 
/*lint -e14*/ 
/*lint -e26*/ 
/*lint -e31*/ 
/*lint -e40*/ 
/*lint -e401*/ 
/*lint -e409*/ 
/*lint -e438*/ 
/*lint -e49*/ 
/*lint -e515*/ 
/*lint -e516*/ 
/*lint -e52*/ 
/*lint -e522*/ 
/*lint -e528*/ 
/*lint -e550*/ 
/*lint -e551*/ 
/*lint -e601*/ 
/*lint -e611*/ 
/*lint -e63*/ 
/*lint -e64*/ 
/*lint -e84*/ 
/*lint -e86*/
/*lint -e578*/
/*lint -esym(528, cli_int32)*/
/*lint -esym(551, cli_int32)*/
/*lint -esym(551, g_cli_cmd_mapping)*/


static struct file_operations cli_ioctl_fops;
#if LINUX_VERSION_CODE >= 0x020618
static struct class *cli_ioctl_class;
#endif
static cli_int32 cli_ioctl_major = 0;
static cli_int8 cfg_value[sizeof(cli_cmd_msg_t)];

static cli_kern_cmd_map_t  g_cli_cmd_mapping[CLI_CMD_MAX_NUM];
static cli_uint32 g_cli_cmd_num = 0;
static cli_int8   g_cli_format[CLI_CMD_BUFF_LEN * 2];

void ca_printf(const cli_int8 * fmt, ...)
{
    cli_int8 buffer[512];
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    printk(KERN_ALERT"%s",buffer);
}

cli_int32 cli_print(
        cli_int8  * buf,
        const cli_int8 *fmt,...
        )
{

    va_list args;
    cli_int32 len = CLI_CMD_BUFF_LEN - 1 - strlen(buf);
    cli_int32 ret = 0;
    const cli_int8 * p = fmt;
    cli_int8 * next = NULL;
    cli_int8 * format = NULL;
    cli_int8   new_line = 0;

    if(len <=0 || strlen(fmt) >= CLI_CMD_BUFF_LEN){
        return 0;
    }
    
    do{
        next = strchr(p,'\n');
        new_line = 0;
        if(next != NULL){
            if(next > p && *(next - 1) == '\r'){
                new_line = 1;
            }

            if(format == NULL){
                format = g_cli_format;
                memset(format,0x00,CLI_CMD_BUFF_LEN*2);
                if(next != p){
                    strncpy(format,p,next - p);
                }
            }else{
                if(next != p){
                    strncat(format,p,next - p);
                }
            }
            p = next + 1;
            if(new_line == 1){
                strcat(format,"\n");
            }else{
                strcat(format,"\r\n");
            }
        }else{
            if(format == NULL){
                format = g_cli_format;
                memset(format,0x00,CLI_CMD_BUFF_LEN*2);
                strcpy(format,p);
            }else{
                strcat(format,p);
            }
            break;
        }
    }while(p != NULL);

    va_start(args,fmt);
    ret = vsnprintf(buf + strlen(buf),len,format,args);
    va_end(args);

    return len;

}


void  cli_k_cmd_init(void)
{
    cli_uint32 i = 0; 

    for(i = 0 ; i < CLI_CMD_MAX_NUM; i++){
        memset(g_cli_cmd_mapping[i].cmd_name,0x00,CLI_CMD_NAME_MAX_LEN);
        g_cli_cmd_mapping[i].handler = NULL;
    }

    g_cli_cmd_num = 0;

}

void cli_k_cmd_reg(cli_int8 * cmd, cli_k_func_t* func)
{
    if(g_cli_cmd_num >= CLI_CMD_MAX_NUM){
        ca_printf("cmd reach max num \n");
        return;
    }

    g_cli_cmd_mapping[g_cli_cmd_num].handler = func; 
    strncpy(g_cli_cmd_mapping[g_cli_cmd_num].cmd_name,cmd,CLI_CMD_NAME_MAX_LEN -1);
    g_cli_cmd_num ++;
}


cli_int32 cli_k_run_cmd(cli_int8 * cmd,void * cli,cli_uint32 argc, cli_int8 ** argv)
{
    cli_uint32 i = 0; 

    for(i = 0 ; i < CLI_CMD_MAX_NUM; i++){
        if(0 == strcmp(g_cli_cmd_mapping[i].cmd_name,cmd)){
            if(g_cli_cmd_mapping[i].handler != NULL){
                return g_cli_cmd_mapping[i].handler(cli,argc,argv);
            }
        }
    }
    return CLI_CMD_E_NO_KERNEL_HDL;
}

static cli_int32 cli_cmd_ioctl(struct file *filp, cli_uint32 cmd, unsigned long arg)
{
    cli_int32 cmd_nr;
    //cli_int8 cfg_value[CA_MAX_CFG_LEN];
    cli_cmd_msg_t *pMsg = NULL;
    cli_int32 ret =  -1;
    cli_int8*argv[CLI_CMD_MAX_ARG_NUM]  = {NULL};
    cli_int32 i  = 0 ;

    
    memset(cfg_value,0x00,sizeof(cfg_value));
    if (_IOC_TYPE(cmd) != CLI_IOCTL_MAGIC)
        return ret;

    if ((cmd_nr = _IOC_NR(cmd)) != CLI_IOCTL_CMD)
        return ret;
   
    pMsg = (cli_cmd_msg_t*)cfg_value;
    if (copy_from_user(cfg_value, (cli_int8 *)arg, sizeof(cli_cmd_msg_t)))
        return -1;

    for(i = 0; i < pMsg->argc; i++){
        argv[i] = pMsg->argv[i];
    }

    pMsg->ret = cli_k_run_cmd(pMsg->cmd_name,pMsg->ret_buf,pMsg->argc,argv);
    copy_to_user((cli_int8 *)arg , pMsg , sizeof(cli_cmd_msg_t));
    return 0;
}



cli_int32 cli_kioctl_init(void)
{
#if LINUX_VERSION_CODE < 0x020618
#ifdef CONFIG_DEVFS_FS
    int ret  =0;
#endif
#endif

    memset(&cli_ioctl_fops , 0 , sizeof(struct file_operations));
    cli_ioctl_fops.unlocked_ioctl = (void *)cli_cmd_ioctl;
    cli_ioctl_major = register_chrdev(0, CLI_IOCTL_NAME , &cli_ioctl_fops);
    if (cli_ioctl_major < 0) {
        ca_printf("kioctl init failure\n");
        return -1;
    } else {
        ca_printf("kioctl init success\n");
    }
#if LINUX_VERSION_CODE >= 0x020618
    cli_ioctl_class = class_create(THIS_MODULE, CLI_IOCTL_NAME);
    if (IS_ERR(cli_ioctl_class)) {
        ca_printf("%s create class faild!\n", CLI_IOCTL_NAME);
        return -1;
    }

    if (NULL == device_create(cli_ioctl_class, NULL, MKDEV(cli_ioctl_major, 0), NULL, CLI_IOCTL_NAME)) {
        return -1;
    }
#else
#ifdef CONFIG_DEVFS_FS
    ret = devfs_mk_cdev(MKDEV(cli_ioctl_major, 0), S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP, CLI_IOCTL_NAME);
#endif
#endif


    cli_k_cmd_init();
    return 0;
}

void cli_kioctl_exit(void)
{
    unregister_chrdev(cli_ioctl_major, CLI_IOCTL_NAME);
#if LINUX_VERSION_CODE >= 0x020618
    device_destroy(cli_ioctl_class, MKDEV(cli_ioctl_major, 0));
    class_destroy(cli_ioctl_class);
#endif

    ca_printf("%s unregister successfully\n", CLI_IOCTL_NAME);
}

/*lint +e10*/ 
/*lint +e101*/ 
/*lint +e1075*/ 
/*lint +e115*/ 
/*lint +e119*/ 
/*lint +e129*/ 
/*lint +e155*/ 
/*lint +e160*/ 
/*lint +e18*/ 
/*lint +e2*/ 
/*lint +e14*/ 
/*lint +e26*/ 
/*lint +e31*/ 
/*lint +e40*/ 
/*lint +e401*/ 
/*lint +e409*/ 
/*lint +e438*/ 
/*lint +e49*/ 
/*lint +e515*/ 
/*lint +e516*/ 
/*lint +e52*/ 
/*lint +e522*/ 
/*lint +e528*/ 
/*lint +e550*/ 
/*lint +e551*/ 
/*lint +e601*/ 
/*lint +e611*/ 
/*lint +e63*/ 
/*lint +e64*/ 
/*lint +e84*/ 
/*lint +e86*/
/*lint +e578*/

/*
module_init(cli_kioctl_init);
module_exit(cli_kioctl_exit);
MODULE_LICENSE("GPL");
EXPORT_SYMBOL(cli_k_cmd_reg);
EXPORT_SYMBOL(cli_print);
*/

