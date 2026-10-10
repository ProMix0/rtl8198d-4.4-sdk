#include "ca_types.h"
#include "os_core.h"
#include "osal_common.h"
#include "ca_version.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "peri.h"
#include <mtd/mtd-user.h>

char bl_version[10] = {0};

int ca_env_set (char *name, char *var)
{
    char env_buf[ENV_SIZE]={0};
    snprintf(env_buf, ENV_SIZE-1, "fw_setenv %s %s ", name, var);
    if ( system(env_buf) != 0 )  {
        return CA_E_ERROR;
    }
    return 0;
}

char *ca_env_get (char *name, char *data)
{
    FILE *fd = NULL;
    int err_cnt = 0;
    char env_cmd[ENV_SIZE]={0};
    char env_buf[ENV_SIZE]={0};
    int len = strlen(name);
    snprintf(env_cmd, ENV_SIZE-1, "fw_printenv %s", name);

fetch_env:
    fd = popen(env_cmd, "r");
    if(fd)
    {
        if(fgets(env_buf, sizeof(env_buf) - 1, fd) != NULL)
        {
            if(!memcmp(env_buf, name, len))
            {
                char *val;
                val = env_buf+len+1;/*Skip the "="*/
                len = strlen(val);
                *(val+len-1) = 0;      /*Set \n to 0*/
                strcpy(data, val);
                pclose(fd);
                return data;
            }
        }
        pclose(fd);
    }

    if(err_cnt > 3)
        return NULL;
    else{
        err_cnt++;
        goto fetch_env;
    }
}


ca_status_t ca_img_env_set (ca_device_id_t device_id, ca_int8_t* env_name, ca_uint8_t img_idx, ca_uint8_t flag)
{
    ca_int8_t img_str[2] = {0};
    ca_int8_t p_img[64] = {0};
    ca_uint32_t img_data = 0;
    ca_uint32_t idx_map = 0;

    if(img_idx)
        img_idx = 1;

    if(flag)
        flag = 1;

    if(!ca_env_get(env_name, p_img))
    {
        return CA_E_PARAM;
    }

    if(ca_str2dec(p_img, &img_data) !=CA_OSAL_OK)
    {
        return CA_E_ERROR;
    }
    
    if(img_data>3)
    {
        return CA_E_ERROR;
    }

    idx_map = 1<<(img_idx);
    img_data &= ~idx_map;
    img_data |= (flag?idx_map:0);

    img_str[0] = 0x30 + img_data;
    ca_env_set(env_name, img_str);

    return CA_E_OK;
}

ca_status_t ca_img_executed_set (ca_device_id_t device_id, ca_uint8_t img_idx, ca_uint8_t flag)
{
    return ca_img_env_set(device_id, ENV_EXEC, img_idx, flag);
}

ca_status_t ca_img_active_set (ca_device_id_t device_id, ca_uint8_t img_idx, ca_uint8_t flag)
{
    return ca_img_env_set(device_id, ENV_ACTIVE, img_idx, flag);
}

ca_status_t ca_img_commit_set (ca_device_id_t device_id, ca_uint8_t img_idx, ca_uint8_t flag)
{
    return ca_img_env_set(device_id, ENV_COMMIT, img_idx, flag);
}

/*The version format should be "xx.xx.xx.xx.xx" string, Length should be 14*/
ca_status_t ca_img_ver_set (ca_device_id_t device_id, ca_uint8_t img_idx, ca_uint8_t* ver_str)
{
    ca_uint8_t t_ver[ENV_VER_LEN+1] = {0};
    if(!ver_str)
    {
        return CA_E_PARAM;
    }
    memcpy(t_ver, ver_str, ENV_VER_LEN);
    if(img_idx == 0)
    {
        ca_env_set(ENV_VERSION0, (char *)t_ver);
    }
    else
    {
        ca_env_set(ENV_VERSION1, (char *)t_ver);
    }
    return CA_E_OK;
}

/*The Info length should be 32*/
ca_status_t ca_img_info_set (ca_device_id_t device_id, ca_uint8_t img_idx, ca_uint8_t* info_str)
{
    ca_uint8_t t_ver[ENV_INFO_LEN+1] = {0};
    if(!info_str)
    {
        return CA_E_PARAM;
    }
    memcpy(t_ver, info_str, ENV_INFO_LEN);
    if(img_idx == 0)
    {
        ca_env_set(ENV_INFO0, (char *)t_ver);
    }
    else
    {
        ca_env_set(ENV_INFO1, (char *)t_ver);
    }
    return CA_E_OK;
}
ca_uint8 ca_img_env_int_get(ca_device_id_t device_id, ca_uint8_t img_idx, ca_int8_t* env_name)
{
    ca_int8_t img_str[64] ={0};
    ca_uint32_t img_data = 0;
    ca_uint32_t idx_map = 0;
    ca_uint8_t ret = 0;
    
    if(img_idx)
        img_idx = 1;

    if(!ca_env_get(env_name, img_str))
    {
        return ret;
    }

    if(ca_str2dec(img_str, &img_data) !=CA_OSAL_OK)
    {
        return ret;
    }
    
    if(img_data>3)
    {
        return ret;
    }

    idx_map = 1<<(img_idx);
    img_data &= idx_map;
    ret = (img_data?1:0);

   return ret;
}

ca_status_t ca_img_info_get (ca_device_id_t device_id, ca_uint8_t img_idx, ca_img_info_t* img_info)
{
    char  ver [64] = {0};
    char  info [64] = {0};
    char  sb_srt[64] = {0};
    ca_uint32_t sb_data = 0;

    if(!img_info)
    {
        return CA_E_PARAM;
    }

    if(ca_env_get(ENV_SB_FLAG, sb_srt))
    {
        sb_data = ca_strtol(sb_srt);
    }

    img_info->exec_flag = ca_img_env_int_get(device_id, img_idx, ENV_EXEC);
    img_info->active_flag = ca_img_env_int_get(device_id, img_idx, ENV_ACTIVE);
    img_info->commit_flag = ca_img_env_int_get(device_id, img_idx, ENV_COMMIT);

    /*bit 0 -> active bit 1 -> valid bit 2 -> commit*/
    if(img_idx == 0)
    {
        ca_env_get(ENV_VERSION0, ver);
        ca_env_get(ENV_INFO0, info);
        if(sb_data&0x2)
            img_info->valid_flag = 1;
    }
    else
    {
        ca_env_get(ENV_VERSION1, ver);
        ca_env_get(ENV_INFO1, info);
        if(sb_data&0x20)
            img_info->valid_flag = 1;
    }

    if(*ver)
        memcpy(img_info->version, ver, ENV_VER_LEN);

    if(*info)
        memcpy(img_info->info, info, ENV_INFO_LEN);

    return CA_E_OK;
}

ca_status_t ca_img_env_load (ca_device_id_t device_id)
{
    return CA_E_OK;
}

char g_month_str[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
ca_cert_info_t g_cert_info;
static ca_status_t ca_cert_date_parse(ca_device_id_t device_id,  ca_uint8_t * cert_str, ca_cert_date_t * date)
{
    int i,j,year,day,hour, min, sec;
    char mon_str[4] = {0};
    for(i=0; i < 64; i++)
    {
        if(cert_str[i] == '=')         
            break;
    }
    if(i>=64)
    {
        return CA_E_PARAM;
    }
    i++;
    sscanf((char *)&cert_str[i], "%s %d %02d:%02d:%02d %04d GMT", mon_str, &day, &hour, &min, &sec, &year);
    for(j=0; j < 12; j++)
    {
        if(!memcmp(mon_str, g_month_str[j], 3 ))
        {
            break;
        }
    }
    date->year = year;
    date->month = j+1;
    date->day = day;
    date->hour = hour;
    date->minute = min;
    date->second = sec;
    return CA_E_OK;
}

static int cert_init = 0;
ca_status_t ca_cert_info_init(ca_device_id_t device_id)
{
    ca_uint8_t cert_data[256]={0};
    int fd = 0;
    ssize_t rw_size = 0;

    if(cert_init)
    {
        return CA_E_OK;
    }
    memset(&g_cert_info, 0,sizeof(ca_cert_info_t));

    if(access("/etc/cert/client.der", F_OK)!=0)
    {
        ca_printf("NO CVC Certificated file!\n");
        return CA_E_RESOURCE;
    }

    if ( system("openssl x509 -inform der -noout -subject -in /etc/cert/client.der > /overlay/cvc_cert_subject") != 0) {
	    return CA_E_ERROR;
    }
    ca_sleep(10);

    if ( system("openssl x509 -inform der -noout -startdate -in /etc/cert/client.der > /overlay/cvc_access_start") != 0 ) {
	    return CA_E_ERROR;
    }
    ca_sleep(10);

    if ( system("openssl x509 -inform der -noout -startdate -in /etc/cert/client.der > /overlay/code_access_start") != 0 ) {
	    return CA_E_ERROR;
    }

    ca_sleep(10);
    
    fd = open("/overlay/cvc_cert_subject", O_RDONLY);
    if(fd != -1)
    {
        rw_size = read(fd, g_cert_info.org_name, 256);
        close(fd);
        if (rw_size <= 0) {
              return CA_E_ERROR;
        }
    }

    fd = open("/overlay/cvc_access_start", O_RDONLY);
    if(fd != -1){
        rw_size = read(fd, cert_data, 256);
        close(fd);
        if (rw_size <= 0) {
              return CA_E_ERROR;
        }
        ca_cert_date_parse(device_id, cert_data, &g_cert_info.date[CA_CERT_CVC_BEFORE_DATE]);
    }

    fd = open("/overlay/code_access_start", O_RDONLY);
    if(fd != -1){
        rw_size = read(fd, cert_data, 256);
        close(fd);
        if (rw_size <= 0) {
              return CA_E_ERROR;
        }
        ca_cert_date_parse(device_id, cert_data, &g_cert_info.date[CA_CERT_CODE_BEFORE_DATE]);
    }


    /*Backup the certificate and important file*/    
    if(access("/etc/cvc_cert_subject", 0))
    {
        if ( (system("cp -rf /etc/cert/client.der /etc/") != 0) ||
             (system("cp -rf /overlay/cvc_cert_subject /etc/") != 0) ||
             (system("cp -rf /overlay/cvc_access_start /etc/") != 0) ||
             (system("cp -rf /overlay/code_access_start /etc/") != 0))
        {
             ca_printf("failed to copy certificate\n");
             return CA_E_ERROR;
        }
    }
    cert_init = 1;
    return CA_E_OK;
}

ca_status_t ca_cert_info_set(ca_device_id_t device_id, ca_uint32_t cert_type, ca_cert_info_t * cert_data)
{
    char cmd[128]={0};
    if(!cert_data)
    {
        return CA_E_PARAM;
    }
    ca_cert_info_init(device_id);
    switch(cert_type){
        case CA_CERT_CVC_BEFORE_DATE:
            if(access("/overlay/cvc_access_start", 0))
            {
                return CA_E_RESOURCE;
            }
            sprintf(cmd, "echo notBefore=%s %d %02d:%02d:%02d %04d GMT > /overlay/cvc_access_start", 
                g_month_str[cert_data->date[cert_type].month-1], cert_data->date[cert_type].day, 
                cert_data->date[cert_type].hour, cert_data->date[cert_type].minute, 
                cert_data->date[cert_type].second, cert_data->date[cert_type].year);
            memcpy(&g_cert_info.date[cert_type], &cert_data->date[cert_type], sizeof(ca_cert_date_t));
            if( system(cmd) == 0 ) {
                ca_sleep(10);
            }
            if (system("cp -rf /overlay/cvc_access_start /etc/") != 0) {
                ca_printf("Failed to copy\n");
            }
            break;
        case CA_CERT_CODE_BEFORE_DATE:
            if(access("/overlay/code_access_start", 0))
            {
                return CA_E_RESOURCE;
            }
            sprintf(cmd, "echo notBefore=%s %d %02d:%02d:%02d %04d GMT > /overlay/code_access_start", 
                g_month_str[cert_data->date[cert_type].month-1], cert_data->date[cert_type].day, 
                cert_data->date[cert_type].hour, cert_data->date[cert_type].minute, 
                cert_data->date[cert_type].second, cert_data->date[cert_type].year);
            memcpy(&g_cert_info.date[cert_type], &cert_data->date[cert_type], sizeof(ca_cert_date_t));
            if (system(cmd) == 0){
                ca_sleep(10);
            }
            if ( system("cp -rf /overlay/code_access_start /etc/") != 0) {
                ca_printf("Failed to copy\n");
            }
            break;
       default:
            break;
    }
    return CA_E_OK;
}

#define GPT_DATA_SIZE  (1024*1024)
ca_status_t ca_loader_info_get(ca_device_id_t device_id,  ca_uint8_t * version)
{
    static char fetch_flag = 0;
    char *gpt_data = NULL;
    char *p_data = NULL;
    int fd;
    ca_uint32_t ind_len = 0;
    mtd_info_t mtd_info;   
    ssize_t read_size;
    ca_uint32_t year = 0;
    ca_uint32_t month = 0;
    ca_uint32_t day = 0;

    if(!version)
    {
        return CA_E_PARAM;
    }
    if(fetch_flag)
    {
        memcpy(version, bl_version, 8);
        return CA_E_OK;
    }
    gpt_data = malloc (GPT_DATA_SIZE);
    if(!gpt_data)
    {
        return CA_E_MEMORY;
    }
    memset(gpt_data, 0, GPT_DATA_SIZE);
    fd = open("/dev/mtd0", O_RDONLY);
    if(fd == -1)
    {
        ca_printf("Open GPT failed \n");
        free(gpt_data);
        return CA_E_EMPTY;
    }
    ioctl(fd, MEMGETINFO, &mtd_info);
    lseek(fd, 0, SEEK_SET);
    read_size = read(fd, gpt_data, GPT_DATA_SIZE);
    close(fd);
    if(read_size != GPT_DATA_SIZE) 
    {
        ca_printf("Read GPT failed \n");
        free(gpt_data);
        return CA_E_DEV;
    }

    for(ind_len=0; ind_len<GPT_DATA_SIZE; ind_len++)
    {
        if (!memcmp(gpt_data+ind_len, "U-Boot 20", 9))
        {
            p_data = gpt_data + ind_len+16;

            for(ind_len=0; ind_len<12; ind_len++)
            {
                if(!memcmp(p_data, g_month_str[ind_len], 3))
                {
                    month = ind_len+1;
                    break;
                }
            }
            p_data = p_data + 4;
            *(p_data+2) = 0;
            day = ca_atoi(p_data);
            p_data = p_data + 3;
            *(p_data+4) = 0;
            year = ca_atoi(p_data);
            snprintf((char *)bl_version, sizeof(bl_version), "%04d%02d%02d", year, month, day);
            break;
        }
    }
    free(gpt_data);
    memcpy(version, bl_version, 8);
    fetch_flag = 1;
    return CA_E_OK;
}

ca_status_t ca_cert_info_get(ca_device_id_t device_id,  ca_cert_info_t * cert_data)
{
    if(!cert_data)
    {
        return CA_E_PARAM;
    }
    ca_cert_info_init(device_id);
    memcpy(cert_data, &g_cert_info, sizeof(ca_cert_info_t));    
    return CA_E_OK;
}

