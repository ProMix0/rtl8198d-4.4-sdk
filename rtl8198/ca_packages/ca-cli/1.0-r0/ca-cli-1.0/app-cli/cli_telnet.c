#include <stdio.h>
#include <errno.h>
#ifdef CLI_OS_LINUX
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <ctype.h>
#include <network.h>
#include <sys/time.h>
#endif
#include "cli_api.h"
#include "cli_server.h"

/*lint -e40*/
/*lint -e10*/

/*lint -e522*/
/*lint -e419*/
/*lint -e573*/
/*lint -e550*/
/*lint -e522*/
/*lint -e560*/
/*lint -e64*/
/*lint -e526*/
/*lint -e628*/

cli_int32 g_ecos_client = 0;

#define CLI_TELNET_DEFAULT_PORT                23

cli_int16 g_telnet_port = CLI_TELNET_DEFAULT_PORT;

static const char *negotiate =
"\xFF\xFB\x03"
"\xFF\xFB\x01"
"\xFF\xFD\x03"
"\xFF\xFD\x01";
#define NEGO_STR_LEN  12

void cli_telnet_port_set(cli_int16 port)
{
    g_telnet_port = port;
}

void send_nego(int fd)
{
    size_t written = 0;
    ssize_t thisTime =0;
    cli_int32 size = strlen(negotiate);

    while (size != written)
    {
        thisTime = write(fd, (char*)negotiate + written, size - written);
        if (thisTime == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return ;
        }
        written += thisTime;
    }
}

cli_int32 read_nego(int sockfd)
{
    cli_int8 c;
    fd_set r;
    struct timeval tm;
    cli_uint32 i = 0;
    cli_uint32 len = 0;
    tm.tv_sec = 1;
    tm.tv_usec = 0 ;
    FD_ZERO(&r);
    FD_SET(sockfd, &r);
    while(select(sockfd + 1, &r, NULL, NULL, &tm) > 0){
        len = read(sockfd, &c, 1);
        if(len <= 0){ /*socket close */
            return -1;
        }
        i++;
#if 0
        /*the response is the same as negotiate,finish */
        if(i == NEGO_STR_LEN ){
            break;
        }
#endif
    }

    return 0;
}

cli_int32 cli_telnet_init()
{
    cli_int32  s;
    struct sockaddr_in addr;
    cli_int32 on = 1;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))){
        close(s);
        printf("cli telnet setsockopt Fail \n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(g_telnet_port);
    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0){
        close(s);
        printf("cli telnet bind Fail \n");
        return -1;
    }

    if (listen(s, 50) < 0){
        close(s);
        printf("cli telnet listen Fail \n");
        return -1;
    }

    return s;
}


cli_int32 cli_telnet_accept(cli_int32 server)
{
    cli_int32  x;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr,sizeof(addr));  
    x = accept(server, (struct sockaddr *) &addr, &len);

    if(x >= 0)
    {
        printf("Accepted Connection From %d: %s \n", x, inet_ntoa(addr.sin_addr));
    }
    else
    {
        printf("Accepted Connection Fail %d \n", x);
    }
     return x;
}

cli_status_t cli_telnet_open(cli_int32 fd, FILE ** in,  FILE ** out)
{
    cli_int32 dupfd; 

    CLI_ASSERT_RET(in != NULL && out != NULL,CLI_E_PARAM);

    send_nego(fd);
    /* Pick up anything received during the negotiations */
    if(0 != read_nego(fd)){
        return CLI_E_ERROR;
    }

    dupfd = dup(fd);
    if(dupfd == -1){
        return CLI_E_ERROR;
    }

    *in = fdopen(fd, "a+");
    *out = fdopen(dupfd, "w+");

    if((*in)&&(*out)){
        setbuf(*in, (char *)0); 
        setbuf(*out, (char *)0); 
        return CLI_OK;
    }else{
        if(*in)
            fclose(*in);
        if(*out)
            fclose(*out);
        printf("cli telnet open fail\n");
        return CLI_E_ERROR;
    }
}

void cli_telnet_close(cli_int32 fd, FILE ** in,  FILE ** out)
{
    CLI_ASSERT(in != NULL && out != NULL);

    close(fd);
    close(fileno(*out));
    fclose(*in); 
    fclose(*out); 
}


cli_status_t cli_telnet_ecos_open(cli_int32 fd, FILE ** in,  FILE ** out)
{
    CLI_ASSERT_RET(in != NULL && out != NULL,CLI_E_PARAM);

    send_nego(fd);
    /* Pick up anything received during the negotiations */
    if(0 != read_nego(fd)){
        return CLI_E_ERROR;
    }

    /*use stderr for pass sanify check*/
    *in = stderr;
    *out = stderr;
    
    g_ecos_client = fd;

    return CLI_OK;
}

void cli_telnet_ecos_close(cli_int32 fd, FILE ** in,  FILE ** out)
{
    g_ecos_client = -1;
    close(fd);
}


cli_int32 cli_telnet_get_ecos_fd()
{
    return g_ecos_client;
}


void cli_init_telnet_server(cli_server_t * server)
{
    CLI_ASSERT(server != NULL);

    server->init = cli_telnet_init;
    server->open_stream = cli_telnet_open;
    server->close_stream = cli_telnet_close;
    server->client_accept = cli_telnet_accept;
}



void cli_init_ecos_telnet_server(cli_server_t * server)
{
    CLI_ASSERT(server != NULL);

    server->init = cli_telnet_init;
    server->open_stream = cli_telnet_ecos_open;
    server->close_stream = cli_telnet_ecos_close;
    server->client_accept = cli_telnet_accept;
}

/*lint +e40*/
/*lint +e10*/

/*lint +e522*/
/*lint +e419*/
/*lint +e573*/
/*lint +e550*/
/*lint +e522*/
/*lint +e560*/
/*lint +e64*/

/*lint +e526*/
/*lint +e628*/
