#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "ca_types.h"
#include "ca_sdk_linux.h"
#include "ca_sdk_msg.h"

ca_int32 ca_sdk_ioctl(ca_uint16 device_id,ca_sdk_msg_t *pMsg)
{
    ca_int32 ioctl_fd;
    ca_int32 ret = -1;
    ca_int8  dev_name[CA_SDK_IOCTL_MAX_NAME_LEN]={0};

    if (pMsg == NULL)
        return -1;

    if(pMsg->len + CA_SDK_MSG_HDR_LEN > CA_SDK_MSG_MAX_LEN)
    {
        printf("The length of input param is too long.\n");
        return -1;
    }
    
    memset(dev_name , 0 , sizeof(dev_name));
    sprintf(dev_name, "/dev/%s", CA_SDK_IOCTL_NAME);
    ioctl_fd = open(dev_name, O_RDWR);
    if (ioctl_fd != -1) {
        ret = ioctl(ioctl_fd,  0, pMsg);
        close(ioctl_fd);
        return ret;
    }
    printf("Device open failure(%s) /dev/"CA_SDK_IOCTL_NAME"\n",  strerror(errno));
    return -1;
}


ca_int32 ca_switch_ioctl(ca_uint16 device_id,void *pMsg)
{
    ca_int32 ioctl_fd;
    ca_int32 ret = -1;
    ca_int8  dev_name[CA_SDK_IOCTL_MAX_NAME_LEN];

    if (pMsg == NULL)
        return -1;


    
    memset(dev_name , 0 , sizeof(dev_name));
    sprintf(dev_name, "/dev/%s", CA_SWITCH_IOCTL_NAME);
    ioctl_fd = open(dev_name, O_RDWR);
    if (ioctl_fd != -1) {
        ret = ioctl(ioctl_fd,  0x89F0, pMsg);
        close(ioctl_fd);
        return ret;
    }

    printf("Device open failure /dev/"CA_SWITCH_IOCTL_NAME"\n");
    return -1;
}



