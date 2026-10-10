#include "cli_api.h"
#include "cli_node.h"
#include "cli_shell.h"
#include "cli_auth.h"
#include "cli_product.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

/*lint -e14*/
/*lint -e516*/
/*lint -e515*/
/*lint -e64*/
/*lint -e18*/
CLI_DEFINE(cli_node_enter_enable,cli_cmd_enter_enable,"enable","Enable the config mode")
{
    cli_enter_node(cli,CLI_NODE_ID_ENABLE,"");

    return CLI_CMD_OK;
}

CLI_DEFINE(cli_node_enter_config,cli_cmd_enter_config,"config",
        "Enter config node\n"
        )
{
    cli_enter_node(cli,CLI_NODE_ID_CONFIG,"config");

    return CLI_CMD_OK;
}

CLI_DEFINE(cli_ssh_debug,cli_cmd_ssh_debug,
    "ssh-debug OPERATION",
    "ssh debug print info\n"
    "(enter tty cmd return in linux shell to on, for example(/dev/pts/0) or off \n")
{
    int tty = -1;
    if(!strcmp(argv[0], "off"))
    {
        tty = open("/dev/console", O_RDONLY | O_WRONLY);
        if(tty != -1)
        {
            ioctl(tty, TIOCCONS);
            cli_shell_show_remote_set(0);
            close(tty);
            cli_print(cli,"Direct to /dev/console \n");
        }
    }
    else
    {
        tty = open(argv[0], O_RDONLY | O_WRONLY);
        if(tty != -1)
        {
            ioctl(tty, TIOCCONS);
            cli_shell_show_remote_set(1);
            close(tty);
            cli_print(cli,"Direct to %s \n", argv[0]);
        }
    }
    return CLI_CMD_OK;
}

CLI_DEFINE(cli_show_err_info,cli_cmd_error_info,
    "show error <0-1>",
    "show\n"
    "error info\n"
    "Disable(0)/Enable(1)\n")
{
    cli_uint32 enable = 0;

    enable = ca_strtol((ca_int8 *)argv[0]);
    cli_shell_show_error_set(enable);
    cli_print(cli, "Error Show Set to %d\n",enable);

    return CLI_CMD_OK;
}

cli_status_t cli_init(cli_uint32 max_node,cli_boolean auth)
{
    cli_status_t ret = CLI_CMD_OK;

    ret = cli_shell_ctrl_init(max_node);

    if(ret != CLI_OK){
        return ret;
    }

    /* add build in node add here  */
    cli_add_root_node(CLI_NODE_ID_VIEW);
    cli_add_node(CLI_NODE_ID_ENABLE);
    cli_add_node(CLI_NODE_ID_CONFIG);
    
    cli_auth_init(auth);

    /* build in node cmd add here */
    cli_install_cmd(CLI_NODE_ID_VIEW,&cli_cmd_enter_enable);
    cli_install_cmd(CLI_NODE_ID_ENABLE,&cli_cmd_enter_config);
    cli_install_cmd(CLI_NODE_ID_CONFIG,&cli_cmd_error_info);
    cli_install_cmd(CLI_NODE_ID_CONFIG,&cli_cmd_ssh_debug);

    return CLI_CMD_OK;
}


/*lint +e14*/
/*lint +e516*/
/*lint +e515*/
/*lint +e64*/
/*lint +e18*/

