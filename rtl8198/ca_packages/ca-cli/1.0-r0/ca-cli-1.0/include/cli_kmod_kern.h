#ifndef __CLI_KMOD_KERN_H__
#define __CLI_KMOD_KERN_H__
#include "cli_cmn.h"
#include "cli_kmod_msg.h"

typedef struct{
    cli_int8        cmd_name [CLI_CMD_NAME_MAX_LEN];
    cli_k_func_t * handler;
}cli_kern_cmd_map_t;

#endif

