
#include <linux/fs.h>

#include "osal_common.h"
#include "aal_common.h"

#include "aal_fdb_ipc.h"
#include "ca_event.h"

#ifdef AAL_FDB_ENABLE_PE_MAC_LRN
extern ca_int32_t fdb_init_done;
extern ca_int32_t using_sw_lrn;

static char msg_desc[][16]  =
{
#undef xx
#define xx(msg_id,msg_desc)  msg_desc,
    AAL_FDB_IPC_DEFINES
};

ca_status_t aal_fdb_ipc_send_handle(
    ca_uint16_t msg_no,
    void *msg_data,
    ca_uint16_t msg_size)
{
    ca_status_t ret = CA_E_OK;

#if (defined(CONFIG_ARCH_CORTINA_SATURN_SFU) && !defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS))
    ca_int8_t *rcv_buff;
    ca_uint16_t rcv_size = msg_size;
    ca_ipc_pkt_t ipc_pkt;

    CA_ASSERT_RET(msg_data != NULL, CA_E_PARAM);

    if (!fdb_init_done || !using_sw_lrn) return CA_E_OK;

    rcv_buff = ca_malloc(rcv_size);
    ca_printf("<%s, %d>msg_no: %d/%s to peer, size:%d\r\n", __FUNCTION__, __LINE__, 
        msg_no, msg_desc[msg_no], msg_size);

    ipc_pkt.session_id = CA_IPC_SESSION_FDB;
    ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE1;
    ipc_pkt.priority = 0;
    ipc_pkt.msg_no = msg_no;
    ipc_pkt.msg_data = msg_data;
    ipc_pkt.msg_size = msg_size;
    ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &rcv_size);
    if (ret != CA_E_OK) {
        ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
        ca_free(rcv_buff);
        return ret;
    }
    else
        ca_printf("%s: ca_ipc_msg_sync_send() OK\n", __func__);

    memcpy(msg_data, rcv_buff, rcv_size);

    ca_free(rcv_buff);

#endif
    return ret;
}
ca_status_t aal_fdb_ipc_debug_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
    ca_uint16_t trans_id, const void * msg_data,ca_uint16_t* msg_size)
{
    ca_status_t ret = CA_E_OK;

#if (defined(CONFIG_ARCH_CORTINA_SATURN_SFU) && !defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS))
    char rcv_buff[16];
    ca_uint16_t rcv_size = 16;
    ca_ipc_pkt_t ipc_pkt;

    CA_ASSERT_RET(msg_data != NULL, CA_E_PARAM);

    //ca_printf("<%s, %d>value2: %d, size:%d\r\n", __FUNCTION__, __LINE__, *(ca_uint32_t *)msg_data, *msg_size);

    ipc_pkt.session_id = CA_IPC_SESSION_FDB;
    ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE1;
    ipc_pkt.priority = 0;
    ipc_pkt.msg_no = AAL_FDB_IPC_DEBUG;
    ipc_pkt.msg_data = msg_data;                  /* the message data to transmit. */
    ipc_pkt.msg_size = *msg_size;                  /* the length of message data. */
    ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &rcv_size);
    //ret = ca_ipc_msg_async_send(&ipc_pkt);
    if (ret != CA_E_OK) {
        ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
        return ret;
    }
    else
        ca_printf("%s: ca_ipc_msg_sync_send() OK\n", __func__);
#endif
    //ca_buffer_dump("rcv", rcv_buff, rcv_size);
    return CA_E_OK;
}


ca_status_t aal_fdb_ipc_log_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
    ca_uint16_t trans_id, const void * msg_data,ca_uint16_t* msg_size)
{
    ca_int8_t *msg_p = (ca_int8_t *)msg_data;
    
    msg_p[*msg_size] = '\0';

    ca_printf("[PE]%s", msg_p);

    return CA_E_OK;
}

ca_status_t aal_fdb_ipc_overflow_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
    ca_uint16_t trans_id, const void * msg_data,ca_uint16_t* msg_size)
{
    ca_uint32_t report_cnt = *(ca_uint32_t *)msg_data;
    ca_status_t ret = CA_E_OK;

    ca_printf("overflow:%d, send out CA_EVENT_MAC_LIMIT_3\r\n", report_cnt);

    ret = ca_event_send(0, CA_EVENT_MAC_LIMIT_3, (void *)&report_cnt, sizeof(ca_uint32_t));

    return CA_E_OK;
}

ca_status_t aal_fdb_ipc_async_debug_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
    ca_uint16_t trans_id, const void * msg_data,ca_uint16_t* msg_size)
{
    ca_status_t ret = CA_E_OK;
#if (defined(CONFIG_ARCH_CORTINA_SATURN_SFU) && !defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS))
//    char rcv_buff[4];
//    ca_uint16_t rcv_size = 4;
    ca_ipc_pkt_t ipc_pkt;


    CA_ASSERT_RET(msg_data != NULL, CA_E_PARAM);

    ca_printf("<%s, %d>value2: %d, size:%d\r\n", __FUNCTION__, __LINE__, *(ca_uint32_t *)msg_data, *msg_size);

    ipc_pkt.session_id = CA_IPC_SESSION_FDB;
    ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE1;
    ipc_pkt.priority = 0;
    ipc_pkt.msg_no = AAL_FDB_IPC_DEBUG;
    ipc_pkt.msg_data = msg_data;                  /* the message data to transmit. */
    ipc_pkt.msg_size = *msg_size;                  /* the length of message data. */
    //ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &rcv_size);
    ret = ca_ipc_msg_async_send(&ipc_pkt);
    if (ret != CA_E_OK) {
        ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
        return ret;
    }
    else
        ca_printf("%s: ca_ipc_msg_sync_send() OK\n", __func__);
#endif
    return CA_E_OK;
}

ca_status_t aal_fdb_ipc_msg_dump_handle(ca_ipc_addr_t peer, ca_uint16_t msg_no,
                ca_uint16_t trans_id, const void * msg_data,ca_uint16_t* msg_size)
{
    ca_uint32_t i;
    const char *data = msg_data;
    ca_printf("msg_no %d trans_id %d msg_size %d\n", msg_no, trans_id, *msg_size);
    ca_printf("session_id %d cpu_id %d \n\t", peer.session_id, peer.cpu_id);
    for (i = 0; i < *msg_size; i++) {
        ca_printf("%02X ", data[i]);
        if ((i % 32) == 31)
            ca_printf("\n\t");
    }
    ca_printf("\n");

    return CA_E_OK;
}

#if 0
ca_status_t aal_fdb_ipc_mac_add(ca_ipc_addr_t peer, ca_uint16_t msg_no,
    ca_uint16_t trans_id, const void * msg_data,ca_uint16_t* msg_size)
{
    ca_status_t ret;
    char rcv_buff[sizeof(aal_fdb_entry_cfg_t)];
    ca_uint16_t rcv_size = sizeof(aal_fdb_entry_cfg_t);
    ca_ipc_pkt_t ipc_pkt;

    CA_ASSERT_RET(msg_data != NULL, CA_E_PARAM);

    ca_printf("<%s, %d>\r\n", __FUNCTION__, __LINE__);

    ipc_pkt.session_id = CA_IPC_SESSION_FDB;
    ipc_pkt.dst_cpu_id = CA_IPC_CPU_PE1;
    ipc_pkt.priority = 0;
    ipc_pkt.msg_no = AAL_FDB_IPC_ADD_MAC;
    ipc_pkt.msg_data = msg_data;                                      /* the message data to transmit. */
    ipc_pkt.msg_size = sizeof(aal_fdb_entry_cfg_t);                  /* the length of message data. */
    ret = ca_ipc_msg_sync_send(&ipc_pkt, rcv_buff, &rcv_size);
    if (ret != CA_E_OK) {
        ca_printf("%s: ca_ipc_msg_sync_send() fail ret %d\n", __func__, ret);
        return ret;
    }
    else
        ca_printf("%s: ca_ipc_msg_sync_send() OK\n", __func__);

    return CA_E_OK;
}

#endif
static ca_ipc_msg_handle_t aal_fdb_ipc_msg_table[] = {
//    { AAL_FDB_IPC_DEBUG,             0,              aal_fdb_ipc_debug_handle},
    { AAL_FDB_IPC_LOG,               0,              aal_fdb_ipc_log_handle},
//    { AAL_FDB_IPC_MSG_DUMP,          0,              aal_fdb_ipc_msg_dump_handle},
//    { AAL_FDB_IPC_ADD_MAC,           0,              aal_fdb_ipc_mac_add},
    { AAL_FDB_IPC_OVERFLOW,               0,         aal_fdb_ipc_overflow_handle},

};

#define NUMOF_FDB_IPC_MSG_ENT  (sizeof(aal_fdb_ipc_msg_table)/sizeof(ca_ipc_msg_handle_t))

ca_status_t aal_fdb_ipc_proc(ca_uint8 *frame_ptr, ca_uint32_t len)
{
    ca_ipc_pkt_t *ipc_pkt = (ca_ipc_pkt_t *)frame_ptr;
    ca_ipc_msg_handle_t *entry_p = &aal_fdb_ipc_msg_table[0];
    ca_int32_t i;
    ca_uint32_t ret = CA_E_ERROR;

    CA_ASSERT_RET((frame_ptr != NULL), CA_E_PARAM);

    ca_buffer_dump("fdb ipc", frame_ptr, len);

    for (i = 0; i < NUMOF_FDB_IPC_MSG_ENT; i++, entry_p++){
        if (CA_IPC_SESSION_FDB == ipc_pkt->session_id && ipc_pkt->msg_no == entry_p->msg_no){
            L2_DEBUG("msg type:%s \r\n", msg_desc[entry_p->msg_no]);
            if (NULL != entry_p->proc) {
                ;//ret = entry_p->proc(ipc_pkt);
            }

            break;
        }
    }

    return ret;
}

#define LINE_LEN         256
static char pelog_buf[LINE_LEN + 1];

ca_uint32_t cat_pelog0(char *name)
{
	ca_int fd;
	ca_int rc;

	fd = ca_file_open(name, O_RDONLY, 0);
	if (fd == -1) {
		printk(KERN_INFO "Open file failed\n");
		return -1;
	}

    while (0 < (rc = ca_file_read(fd, pelog_buf, LINE_LEN))){
    	pelog_buf[rc] = '\0';
    	printk("%s", pelog_buf);
    }

	ca_file_close(fd);

	return 0;
}

/*
ca_uint32_t cat_pelog1(char *name)
{
	ca_int fd;
	ca_int size;
	ca_int rc, i;
	char *buffer;

	fd = ca_file_open(name, O_RDONLY, 0);
	if (fd == -1) {
		printk(KERN_INFO "Open file failed\n");
		return NULL;
	}

	size = ca_file_size(fd);
	buffer = ca_malloc(size + 1);

	if (buffer == NULL) {
		ca_file_close(fd);
		printk(KERN_ERR "Memory allocation failed\n");
		return NULL;
	}

	rc = ca_file_read(fd, buffer, size);
	ca_file_close(fd);

	if (size != rc) {
		ca_free(buffer);
		printk(KERN_DEBUG "read size isn't expected\n");
		return NULL;
	}

	buffer[size] = '\0';
	for (i = 0; i < size; i++){
        printk("%c", buffer[i]);
	}

	return size;
}

*/
ca_status_t aal_fdb_ipcdebug_set(
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
)
{
    printk("\r\n<%s, %d> value:[%d %d]\r\n", __FUNCTION__, __LINE__, value1, value2);

    switch (value1){
        case 1:
            {
                //ca_ipc_debug_level_set(value2);
            }
            break;
        case 3:
            {
                if (value2 > 10)
                    cat_pelog0("/config/scfg.txt");
                else
                    cat_pelog0("/proc/pelog0");
            }
            break;
        case 20:
            break;

        /* start from 100 */
        default:
            {
                ca_ipc_addr_t ipc_peer;
                fdb_ipc_debug_t dbg_msg;

                ca_uint16_t msg_size = sizeof(fdb_ipc_debug_t);

                memset(&ipc_peer, 0, sizeof(ca_ipc_addr_t));
                memset(&dbg_msg, 0, sizeof(fdb_ipc_debug_t));
                dbg_msg.id = value1;
                dbg_msg.value = value2;

                aal_fdb_ipc_debug_handle(ipc_peer, 0 , 0 , (void *)&dbg_msg, &msg_size);
            }
            break;
    }

    return CA_E_OK;
}

ca_status_t aal_fdb_ipcdebug_get(
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
)
{

    if (value2) *value2 = 1;

    ca_printf("fdb debug %d\r\n", 1);

    return CA_E_OK;
}

ca_status_t aal_fdb_ipc_flush_send(flush_cfg_t *cfg)
{
    fdb_ipc_flush_t data;

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    memcpy(&data.mask, &cfg->mask, sizeof(ca_uint32_t));
    data.cfg = cfg->cfg;
    data.type = cfg->type;

    ca_printf("<%s, %d>flush type:%d\r\n", __FUNCTION__, __LINE__, data.type);
    return aal_fdb_ipc_send_handle(AAL_FDB_IPC_FLUSH, (void *)&data, sizeof(data));
}

ca_status_t aal_fdb_ipc_del_all_send(
    CA_IN ca_uint32_t  flag
)
{
    ca_uint32_t value;

    value = flag;
    return aal_fdb_ipc_send_handle(AAL_FDB_IPC_DEL_ALL, (void *)&value, sizeof(ca_uint32_t));
}

ca_status_t aal_fdb_ipc_mac_limit_send(
    CA_IN ca_uint32_t  port,
    CA_IN ca_uint32_t  number
)
{
    fdb_mac_limit_t data;

    data.port = port;
    data.number = number;

    return aal_fdb_ipc_send_handle(AAL_FDB_IPC_MAC_LIMIT, (void *)&data, sizeof(data));
}

ca_status_t aal_fdb_ipc_add_del_send(
    ca_uint32_t op,
    aal_fdb_entry_cfg_t  *entry
)
{
    fdb_add_del_t data;

    CA_ASSERT_RET(entry, CA_E_PARAM);

    data.op = op;
    data.entry = *entry;

    return aal_fdb_ipc_send_handle(AAL_FDB_IPC_MAC_ADD_DEL, (void *)&data, sizeof(data));
}

ca_status_t aal_fdb_ipc_aging_time_send(
    CA_IN ca_uint32_t  value
)
{
    return aal_fdb_ipc_send_handle(AAL_FDB_IPC_AGING_TIME, (void *)&value, sizeof(ca_uint32_t));
}


ca_status_t aal_fdb_ipc_static_mac_get(
    ca_uint32_t idx,
    aal_fdb_entry_cfg_t  *entry
)
{
    fdb_ipc_static_mac_get_t data;
    ca_status_t ret;

    CA_ASSERT_RET(entry, CA_E_PARAM);

    data.idx = idx;

    ret = aal_fdb_ipc_send_handle(AAL_FDB_IPC_STATIC_MAC_GET, (void *)&data, sizeof(data));

    ca_printf("<%s, %d>valid:%d, 0x%02x:%02x:%02x:%02x:%02x:%02x\r\n", __FUNCTION__, __LINE__, data.valid, 
            data.mac[0], data.mac[1], data.mac[2], data.mac[3], data.mac[4], data.mac[5]);
    if (!data.valid)
        return CA_E_UNAVAIL;

    memcpy(entry->mac, data.mac, sizeof(ca_mac_addr_t));
    entry->static_flag = 1;

    return CA_E_OK;
}

ca_status_t aal_fdb_ipc_init(void)
{
    /* Init IPC start */
    ca_status_t ret;

#if (defined(CONFIG_ARCH_CORTINA_SATURN_SFU) && !defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS))
    ret = ca_ipc_msg_handle_register(CA_IPC_SESSION_FDB, aal_fdb_ipc_msg_table, NUMOF_FDB_IPC_MSG_ENT);

    if (ret != CA_E_OK) {
        ca_printf("%s: ca-ipc_msg_handle_register() fail ret %d\n", __func__, ret);
        return CA_E_INIT;
    }
    else
        ca_printf("%s: ca-ipc_msg_handle_register() Success!!!!!!\n", __func__);
#endif
    return 0;
}
#else
ca_status_t aal_fdb_ipcdebug_set(
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
)
{
    ca_int32 i = 0;
    ca_status_t ret = CA_E_OK;

    ca_printf("\r\nvalue1 1: Set value2\r\n ");

    return CA_E_OK;
}

ca_status_t aal_fdb_ipcdebug_get(
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
)
{

    if (value2) *value2 = 1;

    ca_printf("fdb debug %d\r\n", 1);

    return CA_E_OK;
}

#endif

EXPORT_SYMBOL(aal_fdb_ipcdebug_set);
EXPORT_SYMBOL(aal_fdb_ipcdebug_get);



