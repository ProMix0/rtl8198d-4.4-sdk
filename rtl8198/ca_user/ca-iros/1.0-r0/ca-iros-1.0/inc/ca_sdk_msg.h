#ifndef __CA_SDK_MSG_H__
#define __CA_SDK_MSG_H__
#include "ca_sdk_info.h"

#define  CA_SDK_REQ_TYPE_INFO \
        CA_SDK_API_LIST_INFO \
        xx(0,   CA_SDK_REQ_TYPE_END,     0)

typedef enum
{
 #undef  xx
 #define xx(SEQ,TYPE, HANDLER_FUNC) TYPE,
    CA_SDK_REQ_TYPE_INFO
} ca_sdk_req_type_t;


#endif
