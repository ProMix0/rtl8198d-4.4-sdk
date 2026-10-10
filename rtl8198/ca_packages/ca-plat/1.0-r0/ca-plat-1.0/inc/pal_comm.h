#ifndef _PAL_COMM_H_
#define _PAL_COMM_H_

typedef enum
{
    RESERVED_CMD              = 0,
    EVT_CFG_CMD               = 1,
    EVT_DEST_SET_CMD          = 2,
    /*LOG_DEST_SET_CMD          = 3,
    LOG_LEVEL_SET_CMD         = 4,
    FLASH_CMD                 = 5,*/
	MAX_CMD_NUM
} CA_CMD_TYPE_ENUM;

typedef struct _cs_msg_t
{
    ca_uint8_t     version;
    ca_uint8_t     msg_type;
    ca_uint16_t    length;
    ca_uint16_t    port;
    ca_uint16_t    evt_type;
    ca_uint16_t    obj_id;
    ca_uint16_t    reserved; /*used as device_id*/
    ca_int32_t     ret_value;
    ca_uint32_t    from_pid;
    ca_uint32_t    to_pid;
    ca_uint8_t     data[1];
}cap_msg_t;
#define CA_MSG_HDR_LEN ((ca_uint32_t)(((cap_msg_t *)10000)->data - (ca_uint8_t*)10000))

#define CA_PAL_ERR  (-1)
#define CA_PAL_OK   (0)

#define CA_SET_CMD  (0)
#define CA_GET_CMD  (1)
#define NETLINK_PLAT 19

#ifdef __KERNEL__
ca_int32_t ca_pal_kioctl_init(void);
ca_int32_t ca_pal_cmd_register(ca_int32_t cmd , ca_int32_t cmd_type , ca_int32_t(*callback)(cap_msg_t *pMsg));
int ca_pal_init(void);
#else
ca_int32_t ca_pal_ioctl(ca_int32_t cmd, cap_msg_t *pMsg);
#endif
#endif

