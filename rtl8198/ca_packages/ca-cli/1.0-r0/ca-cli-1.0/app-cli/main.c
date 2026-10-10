#include "cli_api.h"
#include "cli_node.h"
#include "cli_shell.h"
#include "cli_auth.h"
#include "cli_product.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef CLI_HAS_TELNETD
/**__is_telnet_d_exist
 *@brief: find the telnetd is alive or not
 */
static int __is_telnet_d_exist(unsigned short l4port)
{
    char shell_str[32] = "netstat -l -t | grep ";
    char str[128] = {0};
    FILE *fd = NULL;

    sprintf(shell_str, "netstat -l -t | grep  %d", l4port);

    //printf("run %s \n", shell_str);
    fd = popen(shell_str, "r");
    if(fd == NULL) {
        printf("call popen failed\n");
        return 0;
    } else {
        if (fgets(str, sizeof(str) - 1, fd) != NULL) {
            if (NULL != strstr(str, "LISTEN")) { /* L4PORT is in listening state  */
                //printf("return %s\n", str);
                pclose(fd);
                return TRUE;            
            }
        }
    }

    pclose(fd);
    return FALSE;

}
#endif

int main(int argc , char ** argv)
{
    int ret = -1;

#ifdef CONFIG_LUNA_G3_SERIES
    system("modprobe cli-drv.ko 2>/dev/null 1>/dev/null");
#endif

#ifdef CLI_HAS_TELNETD
    if (__is_telnet_d_exist(2233)) {
        ret = system("telnet 127.0.0.1 2233");
    }
#endif 
    if (-1 == ret) {
        cli_product_init();
        cli_server_start(0);
    }
    return 0;
}


/*lint +e14*/
/*lint +e516*/
/*lint +e515*/
/*lint +e64*/
/*lint +e18*/

