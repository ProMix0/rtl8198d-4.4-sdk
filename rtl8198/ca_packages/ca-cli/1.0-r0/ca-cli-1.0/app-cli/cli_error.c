#include "cli_api.h"

static cli_int8 * g_error_str[] = 
{
#undef xx
#define xx(ERR_CODE,VALUE,ERR_STR) ERR_STR,
    CLI_ERROR_DEF
};


cli_int8 * cli_get_error_str(cli_cmd_status_t error_code)
{
    if(error_code > CLI_CMD_E_MAX){
        return "";
    }

    return g_error_str[error_code];
}
