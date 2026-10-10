#ifndef __CLI_CMN_H__
#define __CLI_CMN_H__

#ifdef CLI_OS_LINUX
#ifndef CLI_KERNEL_SPACE
#include "cli_api.h"

extern cli_int32 cli_cmd_ioctl(cli_shell_t *cli,cli_int8 * cmd ,cli_uint32 argc, cli_int8 **argv);

#define CLI_KERNEL_DEFINE(func,cmd,cmd_str,cmd_desc) \
    cli_cmd_status_t func(cli_shell_t *,cli_uint32   argc,cli_int8 ** argv); \
    cli_shell_cmd_def_t  cmd = { \
        func, \
        cmd_str, \
        cmd_desc \
    };\
    cli_cmd_status_t func (cli_shell_t *cli,cli_uint32   argc,cli_int8 ** argv)\
    { \
        /*call ioctl */ \
        return cli_cmd_ioctl(cli,#func,argc,argv); \
    }

#define CLI_KERNEL_CMD_INSTALL(node,pcmd)    \
     cli_install_cmd(node,pcmd);

#define CLI_KERNEL_GLOBAL_CMD_INSTALL(pcmd)    \
     cli_install_global_cmd(pcmd);

#else

#define CLI_ERROR_DEF \
    xx(CLI_CMD_OK,              0,  "") \
    xx(CLI_CMD_E_PARAM,             1,  "Invalid parameter") \
    xx(CLI_CMD_E_AMBIGUOUS,         2,  "Ambiguous command") \
    xx(CLI_CMD_E_CMD_NOT_COMPLETE,  3,  "Command not complete") \
    xx(CLI_CMD_E_BAD_COMMAND,       4,  "Bad command") \
    xx(CLI_CMD_E_CMD_NOT_FOUND,     5,  "Command not found") \
    xx(CLI_CMD_E_NO_RESOURCE,       6,  "No resource") \
    xx(CLI_CMD_E_IOCTL ,            7,  "CLI ioctl error") \
    xx(CLI_CMD_E_NO_KERNEL_HDL ,    8,  "Not found handle in kernel") \
    xx(CLI_CMD_E_USER_EXIST,        9,  "User already exist") \
    xx(CLI_CMD_E_USER_NOT_EXIST,    10, "User not exist") \
    xx(CLI_CMD_E_USER_TOO_MANY,     11, "User num reach the maximum") \
    xx(CLI_CMD_E_USER_WRONG_PASSWD, 12, "User password is wrong") \
    xx(CLI_CMD_E_USER_NEW_PASSWD_NOT_MATCH, 13, "User new password and confirm password not matched") \
    xx(CLI_CMD_E_MAX,               14, "") \

typedef enum{
#undef xx
#define xx(ID,VALUE,ERROR) ID = VALUE,
    CLI_ERROR_DEF
}cli_cmd_status_t;

typedef char cli_shell_t;

/* since kernel space will no use this value
 * for compitable the code in user space and kernel space, 
 * we can define this value as any vaule*/
#define CLI_NODE_ID_USER_START 4


typedef unsigned long long      cli_uint64;
typedef long long               cli_int64;
typedef unsigned int            cli_uint32;
typedef int       	            cli_int32;
typedef unsigned short          cli_uint16;
typedef short	                cli_int16;
typedef unsigned char       	cli_uint8;
typedef char	                cli_int8;
typedef unsigned char       	cli_boolean;
typedef double                 	cli_double;

typedef int cli_k_func_t(
        void *,
        unsigned int argc,
        char ** argv);
typedef struct{
    char name[128];
    cli_k_func_t * handler;
}cli_k_cmd_def_t;

extern void cli_k_cmd_reg(char * cmd, cli_k_func_t* func);

extern int cli_print(
        cli_shell_t * buf,
        const char *fmt,...
        );

#define CLI_KERNEL_DEFINE(func,cmd,cmd_str,cmd_desc) \
    int __##func(void *,unsigned int argc,char ** argv); \
    cli_k_cmd_def_t cmd = { \
        #func , \
        __##func \
    };  \
    int __##func (void *cli,unsigned int argc,char ** argv)

#define CLI_KERNEL_CMD_INSTALL(node,pcmd)    \
     /*add to the table*/ \
     cli_k_cmd_reg((pcmd)->name,(pcmd)->handler);

#define CLI_KERNEL_GLOBAL_CMD_INSTALL(pcmd)    \
     /*add to the table*/ \
     cli_k_cmd_reg((pcmd)->name,(pcmd)->handler);

cli_int32 cli_kioctl_init(void);

void cli_kioctl_exit(void);

#endif

#else
#define CLI_KERNEL_DEFINE(func,cmd,cmd_str,cmd_desc) \
    CLI_DEFINE(func,cmd,cmd_str,cmd_desc)

#define CLI_KERNEL_CMD_INSTALL(node,pcmd)    \
     cli_install_cmd(node,pcmd)

#define CLI_KERNEL_GLOBAL_CMD_INSTALL(pcmd)    \
     cli_install_global_cmd(pcmd)

#endif


#endif
