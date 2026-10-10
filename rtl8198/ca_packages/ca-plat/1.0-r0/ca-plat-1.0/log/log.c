#include "ca_types.h"
#include "log.h"
#include <osal_common.h>
#ifdef __KERNEL__
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

ca_uint8 cap_log_level[CAP_LOG_OPER_MAX][CAP_LOG_ID_MAX];
cap_log_ioctl_cmd_t  cap_log_ioctl_cmd[LOG_MAX_CMD];
static struct file_operations cap_log_ioctl_fops;
static struct class *cap_log_ioctl_class;
static ca_int32 cap_log_ioctl_major = 0;

ca_status_t cap_log_cmd_register(ca_int32 cmd, ca_uint32(*callback)(cap_log_ioctl_msg_t *pMsg))
{
    if (cmd < 0 || cmd >= LOG_MAX_CMD || callback == NULL)
        return CA_E_PARAM;

    if (cap_log_ioctl_cmd[cmd].ioctl_cb == callback)
        return CA_E_OK;

    if (cap_log_ioctl_cmd[cmd].ioctl_cb != callback &&
            cap_log_ioctl_cmd[cmd].ioctl_cb != NULL)
    {
        ca_printf("This cmd has been already registered.\n");
        return CA_E_RESOURCE;
    }

    cap_log_ioctl_cmd[cmd].ioctl_cb = callback;

    return CA_E_OK;
}

ca_status_t cap_log_kern_ioctl(struct file *filp, ca_uint32 cmd, ca_uint arg)
{
    ca_int32 cmd_nr;
    cap_log_ioctl_msg_t msg_hdr;
    ca_int8 *cfg_value = NULL;
    cap_log_ioctl_msg_t *pMsg = (cap_log_ioctl_msg_t *)&msg_hdr;
    ca_int32 ret = CA_E_ERROR;
    ca_int32 total_len = 0;

    if (_IOC_TYPE(cmd) != CAP_LOG_IOCTL_MAGIC)
        return ret;

    if ((cmd_nr = _IOC_NR(cmd)) > LOG_MAX_CMD)
        return ret;

    if (cap_log_ioctl_cmd[cmd_nr].ioctl_cb == NULL)
        return ret;

    if (copy_from_user((void *)pMsg, (void *)arg, CAP_LOG_MSG_HDR_LEN))
        return CA_E_ERROR;

    total_len = pMsg->length + CAP_LOG_MSG_HDR_LEN;

    cfg_value = kmalloc(total_len, GFP_ATOMIC);
    if(cfg_value == NULL)
        return ret;

    if (copy_from_user(cfg_value, (ca_int8 *)arg, total_len))
    {
        kfree(cfg_value);
        return ret;
    }

    pMsg = (cap_log_ioctl_msg_t *)cfg_value;
    if(cap_log_ioctl_cmd[cmd_nr].ioctl_cb(pMsg))
    {
        ca_printf("log ioctl failed, ret = %d \n",ret);
        kfree(cfg_value);
        return ret;
    }

    ret = copy_to_user((ca_int8 *)arg , pMsg , pMsg->length + CAP_LOG_MSG_HDR_LEN);
    if(ret != 0){
        ca_printf("copy to user failed, ret = %d \n",ret);
        kfree(cfg_value);
        return ret;
    }
    kfree(cfg_value);

    return ret;
}

static ca_uint32_t log_level_set_callback(cap_log_ioctl_msg_t *pMsg)
{
    cap_log_msg_level_t *plogMsg = (cap_log_msg_level_t *)pMsg->data;

    if(plogMsg->module_id >= CAP_LOG_ID_MAX)
    {
        return CA_E_PARAM;
    }

    if(((plogMsg->log_oper == CAP_LOG_OPER_PRINT)||(plogMsg->log_oper == CAP_LOG_OPER_RECORD))
      &&(plogMsg->data >= CAP_LOG_LEVEL_MAX))
    {
        return CA_E_PARAM;
    }

    if((plogMsg->log_oper == CAP_LOG_OPER_ENABLE)&&
       (plogMsg->data > CAP_LOG_ENABLE))
    {
        return CA_E_PARAM;
    }

    if(plogMsg->log_oper >= CAP_LOG_OPER_MAX)
    {
        return CA_E_PARAM;
    }

    cap_log_level[plogMsg->log_oper][plogMsg->module_id] = plogMsg->data;
    return 0;
}

void cap_log_msg(ca_device_id_t device_id,ca_uint8 mod_id,ca_uint8 level,const ca_int8 * fmt,...)
{
    ca_uint32 idx = 0;
    ca_int8 log[LOG_MSG_LEN];
    va_list   ptr;
    /*Init all the modules log level*/

    if(fmt == NULL)
    {
        return;
    }

    if(mod_id >= CAP_LOG_ID_MAX)
    {
        return;
    }

    if(level >= CAP_LOG_LEVEL_MAX)
    {
        return;
    }

    if(cap_log_level[CAP_LOG_OPER_ENABLE][mod_id] != CAP_LOG_ENABLE)
    {
        return;
    }

    if((level > cap_log_level[CAP_LOG_OPER_PRINT][mod_id])&&
       (level > cap_log_level[CAP_LOG_OPER_RECORD][mod_id]))
    {
        return;
    }

    memset(log, 0, LOG_MSG_LEN);
    va_start(ptr, fmt);
    vsnprintf(log, LOG_MSG_LEN-1, fmt, ptr);
    va_end(ptr);

    for(idx=0;idx<LOG_MSG_LEN-1;idx++)
    {
        if((log[idx] == '\n')||(log[idx] == '\r'))
            log[idx] = ' ';
    }

    if(level > cap_log_level[CAP_LOG_OPER_RECORD][mod_id])
        printk("<%02d-%d>%s\n", mod_id, level, log);
    else
        printk("<cslog-%02d-%d>%s<cslog>\n", mod_id, level, log);

    return ;
}

int cap_log_kern_init(void)
{
    ca_uint32 i =0;
    for(i=0; i<CAP_LOG_ID_MAX; i++)
    {
        cap_log_level[CAP_LOG_OPER_PRINT][i]  = LOG_DEFAULT_PRINT_LEVEL;
        cap_log_level[CAP_LOG_OPER_RECORD][i] = LOG_DEFAULT_RECORD_LEVEL;
        cap_log_level[CAP_LOG_OPER_ENABLE][i] = CAP_LOG_ENABLE;
    }

    memset(&cap_log_ioctl_fops , 0 , sizeof(struct file_operations));
    cap_log_ioctl_fops.unlocked_ioctl = (void *)cap_log_kern_ioctl;
    cap_log_ioctl_major = register_chrdev(0, CAP_LOG_IOCTL_NAME , &cap_log_ioctl_fops);
    if (cap_log_ioctl_major < 0) {
        ca_printf("ca-log: init: failed to register chrdev %s\n", CAP_LOG_IOCTL_NAME);
        return CA_E_PARAM;
    }

    cap_log_ioctl_class = class_create(THIS_MODULE, CAP_LOG_IOCTL_NAME);
    if (IS_ERR(cap_log_ioctl_class)) {
        ca_printf("ca-log: init: failed to create class %s\n", CAP_LOG_IOCTL_NAME);
        return CA_E_RESOURCE;
    }

    if (NULL == device_create(cap_log_ioctl_class, NULL, MKDEV(cap_log_ioctl_major, 0), NULL, CAP_LOG_IOCTL_NAME))
    {
        return CA_E_RESOURCE;
    }

    for(i = 0; i < LOG_MAX_CMD;i++)
    {
        cap_log_ioctl_cmd[i].ioctl_cb = NULL;
    }

    cap_log_cmd_register(LOG_LEVEL_SET_CMD, log_level_set_callback);

    ca_printf("ca-log: init %s OK (max_level %d, default_level %d)\n",
        CAP_LOG_IOCTL_NAME, CAP_LOG_LEVEL_MAX, LOG_DEFAULT_PRINT_LEVEL);

    return CA_E_OK;
}


void cap_log_kern_exit(void)
{
    unregister_chrdev(cap_log_ioctl_major, CAP_LOG_IOCTL_NAME);
    device_destroy(cap_log_ioctl_class, MKDEV(cap_log_ioctl_major, 0));
    class_destroy(cap_log_ioctl_class);
    ca_printf("%s unregister successfully\n", CAP_LOG_IOCTL_NAME);
    return;
}

EXPORT_SYMBOL(cap_log_msg);
EXPORT_SYMBOL(cap_log_kern_init);
EXPORT_SYMBOL(cap_log_kern_exit);

#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include "ca_types.h"
#include "osal_common.h"
#include "log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <netinet/in.h>

static ca_int8 * g_log_level_str[] =
{
    "Off",
    "Emergency",
    "Alert",
    "Critical",
    "Error",
    "Warning",
    "Notice",
    "Info",
    "Debug"
};

static cap_log_ctrl_t  g_log_info;

ca_int8 * cap_log_level_str_get(
        ca_uint8 level)
{
    if(level >= CAP_LOG_LEVEL_MAX){
        return "";
    }
    return g_log_level_str[level];
}


ca_status_t cap_log_init(
        ca_device_id_t device_id,
        ca_uint16 mod_num,
        ca_int8 mod_names[][ LOG_MOD_NAME_LEN])
{
    ca_uint32 i = 0;

    LOG_ASSERT_RET(mod_names != NULL, CA_E_PARAM);

    memset(&g_log_info,0x00,sizeof(cap_log_ctrl_t));

    g_log_info.mod_ctrl = LOG_MALLOC(mod_num * sizeof(cap_log_mod_ctrl_t));

    if(g_log_info.mod_ctrl == NULL){
        LOG_PRINT("No memory \r\n");
        return CA_E_MEMORY;
    }

    strcpy(g_log_info.log_file, LOG_FILE_DEFAULT_NAME);
    strcpy(g_log_info.usr_file, LOG_FILE_USERFILE_NAME);

    if(CA_E_OK != ca_mutex_init(&g_log_info.mutex ,"log_mutex",0)){
        LOG_PRINT("create mutex error \r\n");
        LOG_FREE(g_log_info.mod_ctrl);
        return CA_E_ERROR;
    }

    for(i = 0; i < mod_num; i++){
        g_log_info.mod_ctrl[i].print_level = LOG_DEFAULT_PRINT_LEVEL;
        g_log_info.mod_ctrl[i].record_level = LOG_DEFAULT_RECORD_LEVEL;
        g_log_info.mod_ctrl[i].enable = CAP_LOG_ENABLE;
        strcpy(g_log_info.mod_ctrl[i].mod_name,mod_names[i]);
    }

    g_log_info.socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_log_info.socket < 0)
    {
        LOG_PRINT("Log socket failed\n");
    }

    g_log_info.record_index = 0;
    g_log_info.mod_num = mod_num;
    g_log_info.mode = LOG_DEFAULT_MODE;
    g_log_info.remote_ip = LOG_DEFAULT_REMOTE_IP;
    g_log_info.init = 1;

    return CA_E_OK;
}

/*lint -e526 -e530 -e628 */
void cap_log_msg(
    ca_device_id_t device_id,
    ca_uint8 mod_id,
    ca_uint8 level,
    ca_uint32 err_code,
    const ca_int8 * fmt,
    ...)
{
    ca_int8 log[LOG_FMT_MSG_LEN + 1];
    va_list   ptr;
    cap_log_msg_t * msg = NULL;
    time_t timep;

    if(fmt == NULL){
        return;
    }

    if(!g_log_info.init){
        return;
    }

    if(mod_id > g_log_info.mod_num){
        LOG_PRINT("unknow module id:%d\r\n",mod_id);
        return;
    }

    if(level >= CAP_LOG_LEVEL_MAX){
        LOG_PRINT("unknow level id:%d\r\n",level);
        return;
    }

    if(g_log_info.mod_ctrl[mod_id].enable != CAP_LOG_ENABLE){
        return;
    }

    if((level > g_log_info.mod_ctrl[mod_id].record_level)&&
        (level > g_log_info.mod_ctrl[mod_id].print_level)){
        return;
    }

    memset(log,0x00,LOG_FMT_MSG_LEN + 1);

    va_start(ptr,fmt);
    vsnprintf(log, LOG_FMT_MSG_LEN, fmt, ptr);
    va_end(ptr);

    time (&timep);

    ca_mutex_lock(g_log_info.mutex);
    if((err_code != CAP_LOG_NOPRINT_ERROR_CODE)&&
       (level <= g_log_info.mod_ctrl[mod_id].print_level))
    {
        if(err_code == 0){
            LOG_PRINT("%s\r\n",log);
        }else{
            LOG_PRINT("[%s-%s: %d]: %s\r\n",
                    g_log_info.mod_ctrl[mod_id].mod_name,
                    g_log_level_str[level],
                    err_code,
                    log);
        }
    }

    if(level > g_log_info.mod_ctrl[mod_id].record_level)
    {
        goto Exit;
    }

    if((g_log_info.mode >= CAP_LOG_REMOTE_ONLY)&&(g_log_info.socket >= 0))
    {
        struct sockaddr_in dest_addr;
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = ca_htons(REMOTE_SYSLOG_PORT);
        dest_addr.sin_addr.s_addr = ca_htonl(g_log_info.remote_ip);
        if (sendto(g_log_info.socket,log,strlen(log),0,(struct sockaddr *)&dest_addr,sizeof(dest_addr)) < 0 )
        {
            LOG_PRINT("Error writing buffer");
        }
        if(g_log_info.mode == CAP_LOG_REMOTE_ONLY)
        {
            goto Exit;
        }
    }
    if(err_code == CAP_LOG_NOPRINT_ERROR_CODE)
        err_code = 0;

    if(mod_id > CA_MOD_CUST_MIN)
    {
        msg = &(g_log_info.usr_pool[g_log_info.user_index]);
        msg->mod = mod_id;
        msg->level = level;
        msg->err_code = err_code;
        msg->time = timep;
        memset(msg->log,0x00,LOG_MSG_LEN);
        strncpy(msg->log,log,LOG_MSG_LEN -1);
        g_log_info.user_index  = (g_log_info.user_index + 1) % LOG_USER_NUM;
        g_log_info.need_save = g_log_info.need_save|LOG_USRFILE_BIT;
        goto Exit;
    }

    msg = &(g_log_info.log_pool[g_log_info.record_index]);
    msg->mod = mod_id;
    msg->level = level;
    msg->err_code = err_code;
    msg->time = timep;
    memset(msg->log,0x00,LOG_MSG_LEN);
    strncpy(msg->log,log,LOG_MSG_LEN -1);
    g_log_info.record_index  = (g_log_info.record_index + 1) % LOG_MSG_NUM;
    g_log_info.need_save =  g_log_info.need_save|LOG_SYSFILE_BIT;

Exit:
    ca_mutex_unlock(g_log_info.mutex);
    return;
}
/*lint +e526 +e530 +e628 */

void cap_log_show(ca_device_id_t device_id, ca_boolean_t clear)
{
    ca_uint32   start = 0 ;
    ca_int8     str[LOG_FMT_MSG_LEN] = {0};

    if(!g_log_info.init){
        return ;
    }

    ca_mutex_lock(g_log_info.mutex);
    if(clear)
    {
        start = g_log_info.record_index;
        do{
            if(g_log_info.log_pool[start].log[0])
            {
                memset(g_log_info.log_pool[start].log, 0, LOG_MSG_LEN);
            }
            start = (start + 1)%LOG_MSG_NUM;
        }while(start != g_log_info.record_index);
        g_log_info.record_index = 0;
        g_log_info.need_save = g_log_info.need_save|LOG_SYSFILE_BIT;
    }
    else
    {
        start = g_log_info.record_index;
        do{
            if(g_log_info.log_pool[start].log[0])
            {
                memset(str, 0, LOG_FMT_MSG_LEN);
                ca_log2str(&g_log_info.log_pool[start], str, LOG_FMT_MSG_LEN);
                LOG_PRINT("%s",str);
            }
            start = (start + 1)%LOG_MSG_NUM;
        }while(start != g_log_info.record_index);
    }
    ca_mutex_unlock(g_log_info.mutex);

}

void  ca_log2str(
        cap_log_msg_t * log,
        ca_int8* str,
        ca_uint32 len)
{
    struct tm tmp_tm;

    LOG_ASSERT(log != NULL && str != NULL);

    gmtime_r(&(log->time), &tmp_tm);
    if(log->err_code == 0){
        snprintf(str,len,"[%02d/%02d-%02d:%02d:%02d]-[%s]%s: %s\r\n",
                (1 + tmp_tm.tm_mon),
                tmp_tm.tm_mday,
                tmp_tm.tm_hour,
                tmp_tm.tm_min,
                tmp_tm.tm_sec,
                g_log_level_str[log->level],
                g_log_info.mod_ctrl[log->mod].mod_name,
                log->log);
    }else{
        snprintf(str,len,"[%02d/%02d-%02d:%02d:%02d]-[%s %d]%s: %s\r\n",
                (1 + tmp_tm.tm_mon),
                tmp_tm.tm_mday,
                tmp_tm.tm_hour,
                tmp_tm.tm_min,
                tmp_tm.tm_sec,
                g_log_level_str[log->level],
                log->err_code,
                g_log_info.mod_ctrl[log->mod].mod_name,
                log->log);
    }
}


ca_status_t cap_log_level_set(
    ca_device_id_t device_id,
    ca_int8 * mod_name,
    ca_uint8  oper,
    ca_uint8  data)
{
    ca_uint32 i = 0;

    LOG_ASSERT_RET(mod_name != NULL, CA_E_PARAM);
    if(!g_log_info.init)
    {
        LOG_PRINT("log system was not init \n");
        return CA_E_INIT;
    }

    if(((oper == CAP_LOG_OPER_RECORD)||(oper == CAP_LOG_OPER_PRINT))
      &&(data >= CAP_LOG_LEVEL_MAX))
    {
        return CA_E_PARAM;
    }

    if((oper == CAP_LOG_OPER_ENABLE)&&(data > CAP_LOG_ENABLE))
    {
        return CA_E_PARAM;
    }

    for(i = 0; i < g_log_info.mod_num; i++)
    {
        if(strcmp(g_log_info.mod_ctrl[i].mod_name,mod_name) == 0)
        {
            break;
        }
    }

    if(i == g_log_info.mod_num)
    {
        return CA_E_PARAM;
    }

    if(oper == CAP_LOG_OPER_RECORD)
        g_log_info.mod_ctrl[i].record_level = data;
    else if(oper == CAP_LOG_OPER_PRINT)
        g_log_info.mod_ctrl[i].print_level = data;
    else if(oper == CAP_LOG_OPER_ENABLE)
        g_log_info.mod_ctrl[i].enable = data;

    if(g_log_info.log_level_cb)
        g_log_info.log_level_cb(i,oper,data);

    return CA_E_OK;

}


ca_status_t cap_log_level_get(
    ca_device_id_t device_id,
    ca_int8 * mod_name,
    ca_uint8  oper,
    ca_uint8* data)
{
    ca_uint32 i = 0;

    LOG_ASSERT_RET(mod_name != NULL, CA_E_PARAM);
    if(!g_log_info.init)
    {
        LOG_PRINT("log system was not init \n");
        return CA_E_INIT;
    }


    if((oper >= CAP_LOG_OPER_MAX)||(!data))
    {
        return CA_E_PARAM;
    }

    for(i = 0; i < g_log_info.mod_num; i++)
    {
        if(strcmp(g_log_info.mod_ctrl[i].mod_name,mod_name) == 0)
        {
            break;
        }
    }

    if(i == g_log_info.mod_num)
    {
        return CA_E_PARAM;
    }

    if(oper == CAP_LOG_OPER_RECORD)
        *data = g_log_info.mod_ctrl[i].record_level;
    else if(oper == CAP_LOG_OPER_PRINT)
        *data = g_log_info.mod_ctrl[i].print_level;
    else if(oper == CAP_LOG_OPER_ENABLE)
        *data = g_log_info.mod_ctrl[i].enable;

    return CA_E_OK;

}

cap_log_ctrl_t * cap_log_ctrl_get()
{
    return &g_log_info;
}



void cap_log_file_remove()
{
    unlink(LOG_FILE_DEFAULT_NAME);
    unlink(LOG_FILE_USERFILE_NAME);

    printf("remove the log file \n");
}



static cap_log_file_header_t g_log_file_head;


static unsigned long ca_mktime (unsigned int year, unsigned int mon,
                         unsigned int day, unsigned int hour,
                         unsigned int min, unsigned int sec)
{
    if (0 >= (int) (mon -= 2))
    { /* 1..12 -> 11,12,1..10 */
        mon += 12; /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((unsigned long) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
                year*365 - 719499
                )*24 + hour /* now have hours */
                )*60 + min  /* now have minutes */
                )*60 + sec; /* finally seconds */
}


void cap_log_usr_header_set(cap_log_file_header_t * header)
{
    cap_log_ctrl_t * ctrl = cap_log_ctrl_get();
    ca_mutex_lock(ctrl->mutex);
    memcpy((char *)&g_log_file_head, (char *)header, sizeof(cap_log_file_header_t));
    ctrl->need_save = ctrl->need_save|LOG_USRFILE_BIT;
    ca_mutex_unlock(ctrl->mutex);
    return;
}

void cap_log_usr_header_get(cap_log_file_header_t * header)
{
    cap_log_ctrl_t * ctrl = cap_log_ctrl_get();
    ca_mutex_lock(ctrl->mutex);
    memcpy((char *)header, (char *)&g_log_file_head, sizeof(cap_log_file_header_t));
    ca_mutex_unlock(ctrl->mutex);
    return;
}

void cap_log_set(ca_boolean enable)
{
    cap_log_ctrl_t * ctrl = cap_log_ctrl_get();
    ca_uint32 log_enable = 0;
    ca_device_id_t device_id = 0;
    int i = 0;

    if(enable)
        log_enable = CAP_LOG_ENABLE;

    ca_mutex_lock(ctrl->mutex);
    for(i = 0; i < CAP_LOG_ID_MAX; i++)
    {
        cap_log_level_set(device_id, ctrl->mod_ctrl[i].mod_name, CAP_LOG_OPER_ENABLE, log_enable);
    }
    ca_mutex_unlock(ctrl->mutex);

    return;
}

void cap_log_usr_set(ca_boolean enable)
{
    cap_log_ctrl_t * ctrl = cap_log_ctrl_get();
    ca_uint32 log_enable = 0;
    int i = 0;
    ca_device_id_t device_id = 0;

    if(enable)
        log_enable = CAP_LOG_ENABLE;

    ca_mutex_lock(ctrl->mutex);
    for(i = CA_MOD_CUST_MIN+1; i < CA_MOD_CUST_MAX; i++)
    {
        cap_log_level_set(device_id, ctrl->mod_ctrl[i].mod_name, CAP_LOG_OPER_ENABLE, log_enable);
    }
    ca_mutex_unlock(ctrl->mutex);

    return;
}

int cap_log_usr_clean(void)
{
    ca_uint32   start = 0 ;
    cap_log_ctrl_t * ctrl = cap_log_ctrl_get();
    if(!ctrl->init)
        return CA_E_RESOURCE;
    ca_mutex_lock(ctrl->mutex);
    start = ctrl->user_index;
    do{
        if(ctrl->usr_pool[start].log[0])
        {
            memset(ctrl->usr_pool[start].log, 0, LOG_MSG_LEN);
        }
        start = (start + 1)%LOG_USER_NUM;
    }while(start != ctrl->user_index);
    ctrl->user_index = 0;
    ctrl->need_save = ctrl->need_save|LOG_USRFILE_BIT;
    ca_mutex_unlock(ctrl->mutex);
    return CA_E_OK;
}


int cap_log_file_get(ca_int8 * file_name, ca_int8 is_usr, ca_int8 * mod_name, ca_uint8 level)
{
    ca_uint32 len = 0;
    ca_uint32 i = 0;
    ca_int8 buff[320] = {0};
    ca_int8 mod_str[36]= {0};
    ca_int8 logfile[LOG_FILE_NAME_LEN]={0};
    FILE* fsrc = NULL;
    FILE* fdest = NULL;
    ca_int8 all_mod = 0;
    cap_log_ctrl_t * ctrl = cap_log_ctrl_get();
    char time_buff[32] = {0};

    if(!file_name)
        return CA_E_PARAM;

    if(!ctrl->init)
        return CA_E_RESOURCE;

    if(is_usr)
        strncpy(logfile, ctrl->usr_file, LOG_FILE_NAME_LEN-1);
    else
        strncpy(logfile, ctrl->log_file, LOG_FILE_NAME_LEN-1);

    if(mod_name)
    {
        if(strlen(mod_name) > 32)
            return CA_E_PARAM;
        snprintf(mod_str, 35, "]%s:", mod_name);
    }
    else
    {
        all_mod = 1;
    }

    ca_mutex_lock(ctrl->mutex);
    fsrc = fopen(logfile, "r");
    fdest = fopen(file_name, "w");
    if(fsrc&&fdest)
    {
        while(fgets(buff, 320, fsrc))
        {
            for(len = 0; len < 320; len++)
            {
                if(!buff[len])
                    break;
            }

            if((buff[4]=='-')&&(buff[7]=='-'))
            {
                struct tm ttime;
                time_t time;
                memset((char *)&ttime,0,sizeof(ttime));
                memset((char *)&time,0,sizeof(time));
                memset((char *)&time_buff,0,sizeof(time_buff));
                sscanf(buff,"%4d-%2d-%2d %2d:%2d:%2d",
                            &ttime.tm_year,
                            &ttime.tm_mon,
                            &ttime.tm_mday,
                            &ttime.tm_hour,
                            &ttime.tm_min,
                            &ttime.tm_sec);

                time = ca_mktime(ttime.tm_year,ttime.tm_mon,ttime.tm_mday,
                                 ttime.tm_hour,ttime.tm_min,ttime.tm_sec);
                localtime_r(&time, &ttime);
                snprintf(time_buff, 31, "%04u-%02u-%02u %02u:%02u:%02u",
                 ttime.tm_year + 1900, ttime.tm_mon + 1, ttime.tm_mday,
                 ttime.tm_hour, ttime.tm_min, ttime.tm_sec);

                memcpy(buff, time_buff, 19);
            }

            if((!all_mod)&&(buff[4]=='-')&&(buff[7]=='-'))
            {
                /*find the mod*/
                if(strstr(buff, mod_str))
                {
                    for(i = 0; i < level; i++)
                    {
                        if(strstr(buff, cap_log_level_str_get(i)))
                        {
                            fwrite(buff,1,len,fdest);
                            break;
                        }
                    }
                }
            }
            else/*Header info*/
            {
                fwrite(buff,1,len,fdest);
            }

            memset(buff, 0, 320);
        }
        fclose(fsrc);
        fclose(fdest);
    }
    else
    {
        if(fsrc)
            fclose(fsrc);
        if(fdest)
            fclose(fdest);
        ca_printf("open file fail \n");
    }
    ca_mutex_unlock(ctrl->mutex);
    return CA_E_OK;
}


void cap_log_usr_savefile(void *data)
{
    ca_uint32 i,j;
    cap_log_ctrl_t * ctrl = cap_log_ctrl_get();
    FILE* fp = NULL;
    char log_line[LOG_FMT_MSG_LEN];
    struct tm ltime;

    memset((char *)&ltime, 0, sizeof(ltime));

    if(!ctrl->need_save)
        return;

    ca_mutex_lock(ctrl->mutex);

    if(((ctrl->need_save&LOG_SYSFILE_BIT) == LOG_SYSFILE_BIT)&&
       (fp = fopen((char *)ctrl->log_file, "w")))
    {
        memset(log_line, 0, LOG_FMT_MSG_LEN);
        snprintf(log_line, 512, "SysLog:\n");
        fwrite(log_line,strlen(log_line),1,fp);

        i = ctrl->record_index;
        do{
            if(ctrl->log_pool[i].log[0])
            {
                memset((char *)&ltime, 0, sizeof(ltime));
                gmtime_r(&(ctrl->log_pool[i].time), &ltime);

                for(j=0;j<LOG_MSG_LEN;j++)
                {
                    if((ctrl->log_pool[i].log[j]=='\n')||(ctrl->log_pool[i].log[j]=='\r'))
                        ctrl->log_pool[i].log[j] = ' ';
                }

                memset(log_line, 0, LOG_FMT_MSG_LEN);

                snprintf(log_line, 512, "%04d-%02d-%02d %02d:%02d:%02d[%s]%s:%s\n",
                 ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday,
                 ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
                 cap_log_level_str_get(ctrl->log_pool[i].level),
                 ctrl->mod_ctrl[ctrl->log_pool[i].mod].mod_name,
                 ctrl->log_pool[i].log);

                fwrite((ca_int8*)log_line,strlen(log_line),1,fp);
            }
            i = (i + 1)%LOG_MSG_NUM;
        }while(i != ctrl->record_index);
        fclose(fp);
    }

    if(((ctrl->need_save&LOG_USRFILE_BIT) == LOG_USRFILE_BIT)&&
       (fp = fopen((char *)ctrl->usr_file, "w")))
    {

        memset(log_line, 0, LOG_FMT_MSG_LEN);
        snprintf(log_line, 512, "Manufacturer:%s;\nProductClass:%s;\nSerialNumber:%s;\nIP:%s;\nHWVer:%s;\nSWVer:%s;\n",
                 g_log_file_head.manu,g_log_file_head.product,g_log_file_head.sn,
                 g_log_file_head.ip,g_log_file_head.hwver,g_log_file_head.swver);
        fwrite(log_line,strlen(log_line),1,fp);

        memset(log_line, 0, LOG_FMT_MSG_LEN);
        snprintf(log_line, 512, "\n");
        fwrite(log_line,strlen(log_line),1,fp);
        i = ctrl->user_index;
        do{
            if(ctrl->usr_pool[i].log[0])
            {
                memset((char *)&ltime, 0, sizeof(ltime));
                gmtime_r(&(ctrl->usr_pool[i].time),&ltime);

                for(j=0;j<LOG_MSG_LEN;j++)
                {
                    if((ctrl->usr_pool[i].log[j]=='\n')||(ctrl->usr_pool[i].log[j]=='\r'))
                        ctrl->usr_pool[i].log[j] = ' ';
                }
                memset(log_line, 0, LOG_FMT_MSG_LEN);
                snprintf(log_line, 512, "%04d-%02d-%02d %02d:%02d:%02d[%s]%s:%s\n",
                 ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday,
                 ltime.tm_hour, ltime.tm_min, ltime.tm_sec,
                 cap_log_level_str_get(ctrl->usr_pool[i].level),
                 ctrl->mod_ctrl[ctrl->usr_pool[i].mod].mod_name,
                 ctrl->usr_pool[i].log);
                fwrite((ca_int8*)log_line,strlen(log_line),1,fp);
            }
            i = (i + 1)%LOG_USER_NUM;
        }while(i != ctrl->user_index);
        fclose(fp);
    }

    ctrl->need_save = 0;
    ca_mutex_unlock(ctrl->mutex);

    return;
}


void cap_log_kern_level_set(ca_uint32 module_id,ca_uint32 oper, ca_uint32 data)
{
    ca_uint8 buff[256]={0};
    char dev_name[32]={0};
    int ioctl_fd;
    
    cap_log_ioctl_msg_t *pMsg = (cap_log_ioctl_msg_t *)buff;
    cap_log_msg_level_t *plogMsg = (cap_log_msg_level_t *)(pMsg->data);
    plogMsg->module_id = (ca_uint8)module_id;
    plogMsg->log_oper = (ca_uint8)oper;
    plogMsg->data= (ca_uint8)data;
    pMsg->length = sizeof(cap_log_msg_level_t);

    sprintf(dev_name, "/dev/%s", CAP_LOG_IOCTL_NAME);
    ioctl_fd = open(dev_name, O_RDWR);
    if (ioctl_fd != -1) {
        ioctl(ioctl_fd, _IO(CAP_LOG_IOCTL_MAGIC, LOG_LEVEL_SET_CMD), pMsg);
        close(ioctl_fd);
    }else{
        printf("Device open failure /dev/"CAP_LOG_IOCTL_NAME"\n");
    }
    return;
}

void * log_msg_process(void *data)
{
    ca_int32 log_fd = -1;
    ca_uint32 i,j;
    ca_uint8 buff[1024]={0};
    ca_uint32 rvd_len = 0;
    ca_uint32 read_len = 0;
    ca_uint32 rvd_ptr = 0;
    ca_uint8  find = 0;

#ifndef __X86_DEMO__
    sem_t    *sem = (sem_t *) data;
    sem_post(sem);
#endif
    if(!access("/overlay/klogd",0))
    {
        ca_printf("Klogd enable!!\n");
        return NULL;
    }

    if((log_fd = open("/proc/kmsg",O_RDONLY)) < 0)
    {
        ca_printf("Open kmsg fail! \n");
        return NULL;
    }

    while(1)
    {
        if ((read_len = read(log_fd, &buff[rvd_len], 512)) > 0)
        {
            rvd_ptr = 0;
            if((read_len + rvd_len) > 7)
            {
                rvd_ptr = (read_len + rvd_len)-7;

                for(i = 0; i < (read_len + rvd_len -7); i++)
                {
                    /*Parse format: <cslog-XX-X>xxxxx<cslog>*/
                    if(!memcmp(&buff[i],"<cslog-",7))
                    {
                        rvd_ptr = i;
                        find = 0;
                        for(j = i; j < (read_len + rvd_len -7); j++)
                        {
                            if(!memcmp(&buff[j],"<cslog>",7))
                            {
                                ca_uint8 mod_id = (buff[i+7]-0x30)*10 + (buff[i+8]-0x30);
                                ca_uint8 level = buff[i+10]-0x30;
                                buff[j] = 0;
                                cap_log_msg(0, mod_id, level, CAP_LOG_NOPRINT_ERROR_CODE, "%s\n", &buff[i+12]);
                                rvd_ptr = (read_len + rvd_len)-7;
                                i=j+6;
                                find = 1;
                                break;
                            }
                        }
                        if(!find)
                            break;
                    }
                }
            }
            rvd_len = (rvd_len + read_len) - rvd_ptr;
            memcpy(buff, &buff[rvd_ptr], rvd_len);
            memset(&buff[rvd_len], 0, 1024-rvd_len);
            if(rvd_len > 512)
            {
                buff[rvd_len-1] = 0;
                cap_log_msg(0, CA_MOD_LOG_KERN, CAP_LOG_INFO, CAP_LOG_NOPRINT_ERROR_CODE, "%s\n", buff);
                rvd_len = 0;
                memset(buff, 0, 1024);
            }
        }
        ca_sleep(100);
    }
    return NULL;
}

void cap_log_usr_init(ca_uint16 mod_num, ca_int8 mod_names[][LOG_MOD_NAME_LEN])
{

    ca_uint32 i = 0;
    ca_uint32 j = 0;
    ca_uint32 k = 0;
    ca_uint32 idx = 0;
    ca_uint32 rvd_len = 0;
    ca_uint32 rvd_ptr = 0;
    ca_uint8 flag = 0;
    ca_uint32 read_hdr_len = 0;
    ca_uint32 read_len = 0;
    FILE* fp = NULL;

    ca_int8* buff = NULL;
    cap_log_ctrl_t * ctrl = cap_log_ctrl_get();

    cap_log_init(0, mod_num,mod_names);

    if(ctrl)
        ctrl->log_level_cb = cap_log_kern_level_set;


    buff = (ca_int8 *)ca_malloc(1024);
    if(!buff)
    {
        ca_printf("Log Malloc fail \n");
        return;
    }
    memset(buff, 0, 1024);
    ca_mutex_lock(ctrl->mutex);
    if((!access(ctrl->log_file,0))&&((fp = fopen(ctrl->log_file, "r"))!=NULL))
    {
        rvd_len = 0;
        if((read_hdr_len = fread(buff, 1, 512, fp)) > 0)
        {
            if(read_hdr_len > 8)
            {
                for(i = 0; i < read_hdr_len-8; i++)
                {
                    if(!memcmp(&buff[i],"SysLog:\n",8))
                    {
                        flag = 1;
                        rvd_len = read_hdr_len-i-8;
                        memcpy(buff,&buff[i+8],rvd_len);
                        memset(&buff[rvd_len],0,1024-rvd_len);
                        break;
                    }
                }

                if(flag)
                {
                    flag = 0;
                    while((read_hdr_len < 512)||((read_len = fread(&buff[rvd_len], 1, 512, fp)) > 0))
                    {
                        /*header parse end*/
                        if(read_hdr_len < 512)
                        {
                            read_hdr_len = 512;
                        }
                        rvd_ptr = 0;
                        for(i = 0; i < (read_len + rvd_len); i++)
                        {
                            if(buff[i] == '\n')
                            {
                                buff[i] = 0;
                                if((buff[rvd_ptr+4]=='-')&&(buff[rvd_ptr+7]=='-'))
                                {
                                    struct tm ttime;
                                    ca_int8 mod_name[16]={0};
                                    ca_int8 level_name[16]={0};
                                    memset((char *)&ttime,0,sizeof(ttime));
                                    memset((char *)ctrl->log_pool[idx].log,0,LOG_MSG_LEN);

                                    sscanf(&buff[rvd_ptr],"%4d-%2d-%2d %2d:%2d:%2d",
                                        &ttime.tm_year,
                                        &ttime.tm_mon,
                                        &ttime.tm_mday,
                                        &ttime.tm_hour,
                                        &ttime.tm_min,
                                        &ttime.tm_sec);

                                    ttime.tm_year = ttime.tm_year - 1900;
                                    ttime.tm_mon = ttime.tm_mon-1;
                                    ctrl->log_pool[idx].time = mktime(&ttime);

                                    for(j = rvd_ptr; j < i; j++)
                                    {
                                        if(buff[j]=='[')
                                        {
                                            j++;
                                            for(k=0;k<16;k++)
                                            {
                                                if(buff[j+k] == ']')
                                                    break;
                                                level_name[k] = buff[j+k];
                                            }
                                            j = j+k+1;
                                            for(k=0;k<16;k++)
                                            {
                                                if(buff[j+k] == ':')
                                                    break;
                                                 mod_name[k] = buff[j+k];
                                            }
                                            j = j+k+1;
                                            strncpy(ctrl->log_pool[idx].log,&buff[j],LOG_MSG_LEN-1);
                                            break;
                                        }
                                    }

                                    for(j = 0; j < ctrl->mod_num; j++)
                                    {
                                        if(!strcmp(ctrl->mod_ctrl[j].mod_name, mod_name))
                                        {
                                            ctrl->log_pool[idx].mod = j;
                                            break;
                                        }
                                    }

                                    if(j == ctrl->mod_num)
                                    {
                                        flag = 1;
                                        break;
                                    }

                                    for(j = 0; j < CAP_LOG_LEVEL_MAX; j++)
                                    {
                                        if(!strcmp(cap_log_level_str_get(j), level_name))
                                        {
                                            ctrl->log_pool[idx].level= j;
                                            break;
                                        }
                                    }
                                    if(j >= CAP_LOG_LEVEL_MAX)
                                    {
                                        flag = 1;
                                        break;
                                    }

                                    if(ctrl->log_pool[idx].log[0])
                                    {
                                        idx++;
                                    }
                                }
                                else
                                {
                                    flag = 1;
                                    break;
                                }
                                rvd_ptr = i + 1;
                            }
                        }
                        if(flag == 1)
                        {
                            break;
                        }
                        rvd_len = (rvd_len + read_len) - rvd_ptr;
                        memcpy(buff, &buff[rvd_ptr], rvd_len);
                        memset(&buff[rvd_len], 0, 1024-rvd_len);
                        if(rvd_len > 512)
                        {
                            ca_printf("Log file:%s Parse fail\n",ctrl->log_file);
                            break;
                        }
                    }
                }
            }
        }
        ctrl->record_index = (idx%LOG_MSG_NUM);
        fclose(fp);
    }
    else
    {
        if((fp = fopen(ctrl->log_file, "w")) != NULL)
        {
            fwrite("SysLog:\n", 9, 1, fp);
            fclose(fp);
            ca_printf("Log file:%s init\n",ctrl->log_file);
        }
    }

    memset(buff, 0, 1024);
    flag = 0;
    idx = 0;
    read_hdr_len = 0;
    read_len = 0;
    if((!access(ctrl->usr_file,0))&&((fp = fopen(ctrl->usr_file, "r"))!=NULL))
    {
        rvd_len = 0;
        if((read_hdr_len = fread(buff, 1, 512, fp)) > 0)
        {
            if(read_hdr_len > 8)
            {
                for(i = 0; i < read_hdr_len-2; i++)
                {
                    if(!memcmp(&buff[i],"\n\n",2))
                    {
                        flag = 1;
                        rvd_len = read_hdr_len-i-2;
                        memcpy(buff,&buff[i+2],rvd_len);
                        memset(&buff[rvd_len],0,1024-rvd_len);
                        break;
                    }
                }

                if(flag)
                {
                    flag = 0;

                    while((read_hdr_len < 512)||(read_len = fread(&buff[rvd_len], 1, 512, fp)) > 0)
                    {
                        if(read_hdr_len < 512)
                        {
                            read_hdr_len = 512;
                        }
                        rvd_ptr = 0;

                        for(i = 0; i < (read_len + rvd_len); i++)
                        {
                            if(buff[i] == '\n')
                            {
                                buff[i] = 0;
                                if((buff[rvd_ptr+4]=='-')&&(buff[rvd_ptr+7]=='-'))
                                {
                                    struct tm ttime;
                                    ca_int8 mod_name[16]={0};
                                    ca_int8 level_name[16]={0};
                                    memset((char *)&ttime,0,sizeof(ttime));
                                    memset((char *)ctrl->usr_pool[idx].log,0,LOG_MSG_LEN);

                                    sscanf(&buff[rvd_ptr],"%4d-%2d-%2d %2d:%2d:%2d",
                                        &ttime.tm_year,
                                        &ttime.tm_mon,
                                        &ttime.tm_mday,
                                        &ttime.tm_hour,
                                        &ttime.tm_min,
                                        &ttime.tm_sec);

                                    ttime.tm_year = ttime.tm_year - 1900;
                                    ttime.tm_mon = ttime.tm_mon-1;
                                    ctrl->usr_pool[idx].time = mktime(&ttime);

                                    for(j = rvd_ptr; j < i; j++)
                                    {
                                        if(buff[j]=='[')
                                        {
                                            j++;
                                            for(k=0;k<16;k++)
                                            {
                                                if(buff[j+k] == ']')
                                                    break;
                                                level_name[k] = buff[j+k];
                                            }
                                            j = j+k+1;
                                            for(k=0;k<16;k++)
                                            {
                                                if(buff[j+k] == ':')
                                                    break;
                                                 mod_name[k] = buff[j+k];
                                            }
                                            j = j+k+1;
                                            strncpy(ctrl->usr_pool[idx].log,&buff[j],LOG_MSG_LEN-1);
                                            break;
                                        }
                                    }
                                    for(j = 0; j < ctrl->mod_num; j++)
                                    {
                                        if(!strcmp(ctrl->mod_ctrl[j].mod_name, mod_name))
                                        {
                                            ctrl->usr_pool[idx].mod = j;
                                            break;
                                        }
                                    }

                                    if(j == ctrl->mod_num)
                                    {
                                        flag = 1;
                                        break;
                                    }

                                    for(j = 0; j < CAP_LOG_LEVEL_MAX; j++)
                                    {
                                        if(!strcmp(cap_log_level_str_get(j), level_name))
                                        {
                                            ctrl->usr_pool[idx].level= j;
                                            break;
                                        }
                                    }
                                    if(j >= CAP_LOG_LEVEL_MAX)
                                    {
                                        flag = 1;
                                        break;
                                    }
                                    if(ctrl->usr_pool[idx].log[0])
                                    {
                                        idx++;
                                    }
                                }
                                else
                                {
                                    flag = 1;
                                    break;
                                }
                                rvd_ptr = i + 1;
                            }
                        }
                        if(flag == 1)
                        {
                            break;
                        }
                        rvd_len = (rvd_len + read_len) - rvd_ptr;
                        memcpy(buff, &buff[rvd_ptr], rvd_len);
                        memset(&buff[rvd_len], 0, 1024-rvd_len);
                        if(rvd_len > 512)
                        {
                            ca_printf("Log file:%s Parse fail\n",ctrl->usr_file);
                            break;
                        }
                    }
                }
            }
        }
        ctrl->user_index = (idx%LOG_USER_NUM);
        fclose(fp);
    }
    else
    {
        if((fp = fopen(ctrl->usr_file, "w")) != NULL)
        {
            ca_printf("Log file:%s init\n",ctrl->usr_file);
            fclose(fp);
        }
    }

    ctrl->need_save = 0;
    ca_mutex_unlock(ctrl->mutex);
    ca_free(buff);
    /*3 seconds to save file*/
    ca_circle_timer_add(3000, cap_log_usr_savefile, NULL);
    ca_printf("ca-log: init usr log OK (module_num %d, max_log_num %d, save_log_timer %d seconds)\n",
        mod_num, LOG_MSG_NUM, 3);
    return;
}

#endif

