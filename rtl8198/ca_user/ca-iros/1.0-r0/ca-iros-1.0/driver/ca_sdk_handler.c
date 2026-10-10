#include <linux/module.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sockios.h>
#include <linux/net.h>
#include <linux/skbuff.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/netlink.h>

#include <ca_types.h>
#include "ca_sdk_linux.h"
#include "ca_sdk_msg.h"
#include "ca_sdk_handler.h"

typedef ca_status_t (*ca_sdk_data_handler_func_t)(ca_uint8 *, ca_uint32);



ca_sdk_data_handler_func_t g_sdk_handler_table[CA_SDK_REQ_TYPE_END+1] =
{
   #undef xx
   #define xx(SEQ,TYPE,HANDLER_FUNC ) (HANDLER_FUNC),
   CA_SDK_REQ_TYPE_INFO
};

ca_int32 ca_sdk_kern_api_cb(ca_sdk_msg_t *pMsg)
{

    if(pMsg->sdk_id >= CA_SDK_REQ_TYPE_END){
        return -1;
    }

    return g_sdk_handler_table[pMsg->sdk_id](pMsg->data, pMsg->len);
}
