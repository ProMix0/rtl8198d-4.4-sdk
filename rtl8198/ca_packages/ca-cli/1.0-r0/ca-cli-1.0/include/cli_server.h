#ifndef __CLI_SERVER_H__
#define __CLI_SERVER_H__
#include "cli_utils.h"

typedef  cli_int32 cli_init_t();
typedef  cli_status_t cli_client_open_t(cli_int32 fd, FILE ** in,  FILE ** out);
typedef  void cli_client_close_t(cli_int32 fd, FILE ** in,  FILE ** out);
typedef  cli_int32 cli_accept_client_t(cli_int32 server);

typedef struct{
    cli_init_t * init;
    cli_client_open_t   * open_stream;
    cli_client_close_t* close_stream;
    cli_accept_client_t  * client_accept;
}cli_server_t;

#endif
