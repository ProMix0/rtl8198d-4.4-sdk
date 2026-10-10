#ifndef __CLI_KMOD_MSG_H__
#define __CLI_KMOD_MSG_H__

#define CLI_CMD_NAME_MAX_LEN   128
#define CLI_CMD_MAX_NUM        7000
#define CLI_CMD_MAX_ARG_NUM    15
#define CLI_CMD_MAX_ARG_LEN    32
#define CLI_CMD_BUFF_LEN       8192/*4096*/

typedef struct{
   cli_int32     ret;
   cli_int8      ret_buf[CLI_CMD_BUFF_LEN];
   cli_int8      cmd_name[CLI_CMD_NAME_MAX_LEN];
   cli_uint32    argc;
   cli_int8      argv[CLI_CMD_MAX_ARG_NUM][CLI_CMD_MAX_ARG_LEN];
}cli_cmd_msg_t;


#define CLI_IOCTL_CMD        1

#define CLI_IOCTL_NAME      "cortina-cli-dev"
#define CLI_IOCTL_MAGIC     'C'



#endif
