#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include "cli_api.h"
#include "cli_server.h"
/*lint -e611*/
extern void cli_init_telnet_server(cli_server_t * server);
extern void cli_init_console_server(cli_server_t * server);
extern void cli_init_ecos_telnet_server (cli_server_t * server);
#ifdef CLI_OS_ECOS
extern cli_int ca_thread_create(cli_uint32 *thread_id,  const cli_int8 *thread_name,
                              const void *function_pointer, void *param , cli_uint32 stack_size,
                              cli_uint32 priority, cli_uint32 flags);
#endif

static cli_boolean isecos = FALSE;
void cli_telnet_port_set(cli_int16 port);
#ifdef CLI_HAS_TELNETD
FILE * g_cli_out = NULL;
void cli_bc_print(cli_int8 * fmt,...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(g_cli_out, fmt, args);
    va_end(args);
}

void cli_telnetd()
{
    cli_server_t server;
    cli_int32 s = 0, x = 0;
    FILE * in;
    FILE * out;

    if(!isecos){
        cli_init_telnet_server(&server);
    }else{
        cli_init_ecos_telnet_server(&server);
    }

    s = server.init();

    while((x = server.client_accept(s)) >= 0){
        if(CLI_OK == server.open_stream(x,&in,&out)){
            g_cli_out = out;
            cli_main(CLI_SESSION_TELNET,in,out);
            g_cli_out = NULL;
            server.close_stream(x,&in,&out);
        }
    }

}
#endif
void cli_console()
{
    cli_server_t server;
    cli_int32 s = 0, x = 0;

    FILE * in;
    FILE * out;

    cli_init_console_server(&server);

    s = server.init();
    
    x = server.client_accept(s);

    server.open_stream(x,&in,&out);
    /*use the os to create thread*/
    cli_main(CLI_SESSION_CONSOLE,in,out);
    server.close_stream(x,&in,&out);
}


void cli_server_start(cli_boolean telnetd)
{
#ifdef CLI_HAS_TELNETD
    pthread_t tel_thread_id = 0;
#endif
#ifdef CLI_OS_LINUX
    isecos = FALSE;
    pthread_t thread_id;
#else
    isecos = TRUE;
    cli_uint32 thread_id;
#endif


#ifdef CLI_HAS_TELNETD
    if(telnetd){
#ifdef CLI_OS_LINUX
        pthread_create (&tel_thread_id, NULL, (void *)*cli_telnetd, NULL);
#else
         ca_thread_create(&tel_thread_id, "telnetd", (void *)*cli_telnetd, NULL,
                4*4096,19,0);
#endif
    }
#endif

#ifdef CLI_OS_LINUX
    pthread_create (&thread_id, NULL, (void *)*cli_console, NULL);
#else
    ca_thread_create(&thread_id, "shell", (void *)*cli_console, NULL,
            16*4096,6,0);
#endif

#ifdef CLI_OS_LINUX
#ifdef CLI_HAS_TELNETD
    if(telnetd){
        pthread_join(tel_thread_id, NULL);
    }
#endif
    pthread_join(thread_id, NULL);
#endif
}

#ifdef CLI_HAS_TELNETD
#ifdef CLI_OS_LINUX
void cli_telnetd_start(cli_int16 port)
{
    pthread_t tel_thread_id = 0;
    cli_telnet_port_set(port);
    pthread_create (&tel_thread_id, NULL, (void *)*cli_telnetd, NULL);
    pthread_join(tel_thread_id, NULL);
}
#endif
#endif
/*lint +e611*/
