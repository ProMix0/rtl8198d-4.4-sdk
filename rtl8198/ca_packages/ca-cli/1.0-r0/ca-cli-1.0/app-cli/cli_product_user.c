#include <stdio.h>
#include "ca_types.h"
#include "cli_product.h"

/*lint -e18*/
/*lint -e40*/
extern void cli_aal_init(void);

int cli_product_init()
{
    unsigned int node;
    cli_init(CLI_NODE_ID_MAX,0);
    for(node=CLI_NODE_ID_SDK;node<CLI_NODE_ID_MAX;node++)
        cli_add_node(node);
    cli_global_cmd_node_range(CLI_NODE_ID_CONFIG,CLI_NODE_ID_MAX - 1);
     cli_aal_init();

    return 0;
}
/*lint +e18*/
/*lint +e40*/
