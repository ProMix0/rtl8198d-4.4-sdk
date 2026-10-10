#ifndef __AAL_FDB_IPC_H__
#define __AAL_FDB_IPC_H__

#include "aal_fdb.h"


#ifdef AAL_FDB_ENABLE_PE_MAC_LRN

#include <soc/cortina/ca_ipc_pe.h>
#include "ca_types.h"

/*
typedef enum {
    AAL_FDB_IPC_DEBUG = 0x1,
    AAL_FDB_IPC_ADD_MAC,

    AAL_FDB_IPC_MAX
}fdb_ipc_msg_e;
*/

#define AAL_FDB_IPC_DEFINES/*CODEGEN_IGNORE_TAG*/ \
    xx(AAL_FDB_IPC_DEBUG,       "debug")\
    xx(AAL_FDB_IPC_LOG,         "log") \
    xx(AAL_FDB_IPC_MSG_DUMP,    "msg_dump") \
    xx(AAL_FDB_IPC_DEL_ALL,     "del_all")  \
    xx(AAL_FDB_IPC_MAC_LIMIT,   "mac_limit") \
    xx(AAL_FDB_IPC_MAC_ADD_DEL, "mac_ad") \
    xx(AAL_FDB_IPC_FLUSH,       "flush") \
    xx(AAL_FDB_IPC_OVERFLOW,    "overflow") \
    xx(AAL_FDB_IPC_AGING_TIME,  "age_time") \
    xx(AAL_FDB_IPC_STATIC_MAC_GET, "mac_get")



typedef enum {
#undef xx
#define xx(/*CODEGEN_IGNORE_TAG*/msg_id, msg_desc)  msg_id,
    AAL_FDB_IPC_DEFINES
    AAL_FDB_IPC_MAX
} fdb_ipc_msg_e;

typedef struct fdb_ipc_debug_s{
    ca_uint32_t id;
    ca_uint32_t value;
    ca_uint32_t value2;
}fdb_ipc_debug_t;

typedef struct fdb_mac_limit_s{
    ca_uint32_t port;
    ca_uint32_t number;
}fdb_mac_limit_t;

typedef struct fdb_ipc_flush_s{
    ca_uint32_t mask;
    aal_fdb_flush_cfg_t cfg;
    ca_uint32_t type;
}fdb_ipc_flush_t;


typedef struct fdb_add_del_s{
    ca_uint32_t op;
    aal_fdb_entry_cfg_t entry;
}fdb_add_del_t;

typedef struct fdb_ipc_static_mac_get_s{
    ca_uint32_t   idx;
    ca_mac_addr_t mac;
    ca_boolean_t  valid;
}fdb_ipc_static_mac_get_t;

ca_status_t aal_fdb_ipc_init(void);

ca_status_t aal_fdb_ipc_del_all_send(
    CA_IN ca_uint32_t  flag
);

ca_status_t aal_fdb_ipc_mac_limit_send(
    CA_IN ca_uint32_t  port,
    CA_IN ca_uint32_t  number
);

ca_status_t aal_fdb_ipc_add_del_send(
    ca_uint32_t op,
    aal_fdb_entry_cfg_t  *entry
);
ca_status_t aal_fdb_ipc_aging_time_send(
    CA_IN ca_uint32_t  value
);

ca_status_t aal_fdb_ipc_static_mac_get(
    ca_uint32_t idx,
    aal_fdb_entry_cfg_t  *entry
);

ca_status_t aal_fdb_ipc_flush_send(flush_cfg_t *cfg);

#endif

ca_status_t aal_fdb_ipcdebug_set(
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
);


ca_status_t aal_fdb_ipcdebug_get(
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
);

#endif

