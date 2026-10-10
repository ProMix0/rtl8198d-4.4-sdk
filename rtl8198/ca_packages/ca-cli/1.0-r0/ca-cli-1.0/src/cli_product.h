#ifndef __CLI_PRODUCT_H__
#define __CLI_PRODUCT_H__
#include "ca_types.h"
#include "cli_cmn.h"
#ifdef CLI_KERNEL_SPACE
#include <linux/string.h> 
#endif
typedef enum{
    CLI_NODE_ID_SDK   =    CLI_NODE_ID_USER_START,
    CLI_NODE_ID_LOG,
    CLI_NODE_ID_OAM,
    CLI_NODE_ID_OMCI,
    CLI_NODE_ID_PLAT,
    CLI_NODE_ID_AAL,
    CLI_NODE_ID_PAAL,
    CLI_NODE_ID_SDL,
    CLI_NODE_ID_OPM,
    CLI_NODE_ID_HASH_MASK,
    CLI_NODE_ID_HASH_CRC,


    
    /*__AAL_CLI_CODE_GEN__*/
    

    CLI_NODE_ID_PAAL_MISC,

    CLI_NODE_ID_SDL_ETH_PORT,
    CLI_NODE_ID_SDL_MA,
    CLI_NODE_ID_SDL_MISC,
  
    CLI_NODE_ID_STP,
    CLI_NODE_ID_LOOPDETECT,
    CLI_NODE_ID_MC,
    CLI_NODE_ID_GPIO,    
    CLI_NODE_ID_LED,
    CLI_NODE_ID_DHCP,
    CLI_NODE_ID_RTL,
    
    CLI_NODE_ID_MAX   ,  
}cli_demo_node_id_t;



extern int cli_atoi(char * str);
ca_uint64_t cli_str2ll(char *str);
extern ca_status_t ca_str2ca_ip(ca_int8_t *ip_str , ca_ip_address_t *ipaddr);
extern ca_status_t ca_str2ca_mac(ca_int8_t * str, ca_mac_addr_t * mac);
#ifndef CLI_KERNEL_SPACE
extern int cli_product_init();
#endif
#endif
