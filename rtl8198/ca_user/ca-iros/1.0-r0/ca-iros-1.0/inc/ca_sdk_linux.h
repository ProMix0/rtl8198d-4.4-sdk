#ifndef __CA_SDK_LINUX_H__
#define __CA_SDK_LINUX_H__
typedef struct
{
    ca_uint16   sdk_id;
    ca_uint16   len;

    ca_uint8    data[1];
} __attribute__((packed))ca_sdk_msg_t;

#define CA_SDK_MSG_HDR_LEN          ((ca_uint32)(((ca_sdk_msg_t*)0)->data))

#define CA_SDK_MSG_MAX_LEN  4096

#define CA_SDK_IOCTL_MAX_NAME_LEN   32
#define CA_SDK_IOCTL_NAME	        "ca-sdk-dev"
#define CA_SDK_IOCTL_MAGIC		    'C'


#define CA_SWITCH_IOCTL_NAME	        "switch"

ca_int32 ca_sdk_ioctl(ca_uint16 device_id,ca_sdk_msg_t *pMsg);


ca_int32 ca_switch_ioctl(ca_uint16 device_id,void *pMsg);


#endif
