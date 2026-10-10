#ifndef __CLI_AUTH_H__
#define __CLI_AUTH_H__
#include "cli_utils.h"

#define CLI_MAX_USER_NUM  10

#define CLI_MAX_USER_NAME_LEN  32
#define CLI_MAX_USER_PASSWD_LEN  32

typedef struct{
    cli_int8    perm;
    cli_int8    valid;
    cli_int8    user[CLI_MAX_USER_NAME_LEN + 1];
    cli_int8    passwd[CLI_MAX_USER_PASSWD_LEN + 1];
}cli_user_t;

cli_boolean cli_user_login(
        cli_shell_t * shell,
        cli_int32 retry_time);

void cli_auth_init(cli_boolean auth);

cli_boolean cli_user_valid(
        cli_int8 * name,
        cli_int8 * passwd);
#endif

