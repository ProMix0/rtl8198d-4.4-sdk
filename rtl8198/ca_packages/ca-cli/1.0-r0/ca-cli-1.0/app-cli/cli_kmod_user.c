#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cli_cmn.h"
#include "cli_kmod_msg.h"

/*lint -e64*/
cli_int32 cli_cmd_ioctl(cli_shell_t *cli,cli_int8 * cmd ,cli_uint32 argc, cli_int8 **argv)
{
    cli_uint32 i = 0; 
    cli_cmd_msg_t msg;
    cli_int32 ioctl_fd;
    cli_int8 dev_name[32];
    cli_int32 ret = 0;

    memset(&msg, 0 , sizeof(msg));

    strcpy(msg.cmd_name,cmd);
    msg.argc = argc; 

    for(i = 0; i < argc; i++){
        strncpy(msg.argv[i],argv[i],CLI_CMD_MAX_ARG_LEN);
    }

    memset(dev_name , 0 , sizeof(dev_name));
    sprintf(dev_name, "/dev/%s", CLI_IOCTL_NAME);
    ioctl_fd = open(dev_name, O_RDWR);
    if (ioctl_fd != -1) {
        ret = ioctl(ioctl_fd, _IO(CLI_IOCTL_MAGIC, CLI_IOCTL_CMD), &msg);
        if(ret == 0){
            close(ioctl_fd);
            cli_print(cli,"%s",msg.ret_buf);
            return msg.ret;
        }
	close(ioctl_fd);
    }

    return CLI_CMD_E_IOCTL;
}

/*lint +e64*/

