#include "ca_types.h"
#include "os_core.h"
#include "osal_common.h"
#include "event_core.h"
#include "ca_event.h"
#include "pal_comm.h"
/*lint -save -w0*/

ca_int32_t event_cfg_set(
        ca_device_id_t device_id,
        ca_event_cfg_local_t *cfg)
{
    if(cfg == NULL){
        return CA_EVT_ERROR;
    }

    switch(cfg->cfg_type)
    {
    case EVT_CFG_COUNTER_GET:
        return ca_event_counter_get(device_id, cfg->evt_type, &cfg->counter, cfg->read_clear);
    case EVT_CFG_COUNTER_CLR:
        return ca_event_counter_clear(device_id, cfg->evt_type);
    case EVT_CFG_COUNTER_CLR_ALL:
        return ca_event_counter_clear_all(device_id);
    case EVT_CFG_SET:
        return ca_event_set(device_id, cfg->evt_type, &cfg->cfg);
    case EVT_CFG_GET:
        return ca_event_get(device_id, cfg->evt_type, &cfg->cfg);
    case EVT_CFG_DUMP:
        ca_event_dump();
        break;
    default:
        return CA_EVT_ERROR;
    }

    return CA_EVT_OK;
}


#ifndef __KERNEL__
ca_uint32_t evt_dest = 0;
ca_int32_t evt_flag = 0;
extern void ca_event_send_to_sc(
        ca_device_id_t device_id,
        ca_int32_t event_type,
        void *msg,
        ca_int32_t len);

static void event_cfg_handler(ca_device_id_t device_id, ca_event_cfg_local_t *cfg)
{
    ca_uint8_t buff[1024];
    cap_msg_t *pMsg = (cap_msg_t *)buff;
    ca_event_cfg_local_t *temp_cfg = (ca_event_cfg_local_t *)pMsg->data;

    pMsg->reserved = (ca_device_id_t)device_id;
    pMsg->length = sizeof(ca_event_cfg_local_t);
    memcpy(temp_cfg, cfg, sizeof(ca_event_cfg_local_t));
    ca_pal_ioctl(EVT_CFG_CMD, pMsg);
    memcpy(cfg, pMsg->data, sizeof(ca_event_cfg_local_t));

    return ;
}

 void ca_event_dest_set(ca_uint32_t dest)
{
    ca_uint8_t buff[1024];
    cap_msg_t *pMsg = (cap_msg_t *)buff;

    pMsg->to_pid = dest;
    pMsg->length = 0;
    ca_pal_ioctl(EVT_DEST_SET_CMD, pMsg);
    return;
}

void remote_event_process(void *data)
{
    ca_int32_t sock_fd = -1;
    struct sockaddr_nl src_addr;
    fd_set read_set;
    struct sockaddr addr;
    ca_int32_t ret = -1;
    socklen_t addr_len = 0;
    ca_int32_t msg_len = 0;
    cap_msg_t *pMsg = NULL;
    ca_uint8_t buff[2048];

    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_PLAT);
    if (sock_fd < 0) {
        perror("!Create pkt_evt channel failed::\n");
        return ;
    }

    evt_dest = (ca_uint32_t)getppid();
    ca_event_dest_set(evt_dest);
    src_addr.nl_family 	= AF_NETLINK;
    src_addr.nl_pid     = evt_dest;
    src_addr.nl_groups 	= 0;
    if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
        printf("bind socket failed(%d).\n",errno);
        close(sock_fd);
        return ;
    }

    while(1)
    {
        FD_ZERO(&read_set);
        FD_SET(sock_fd , &read_set);
        ret = select(sock_fd+1 , &read_set , NULL, NULL, NULL);
        if (ret <= 0)
            continue;

        if (FD_ISSET(sock_fd , &read_set)) {
            msg_len = recvfrom(sock_fd, buff, sizeof(buff), 0, (struct sockaddr *)&addr, &addr_len);
            if(msg_len < CA_MSG_HDR_LEN)
                continue;

            pMsg = (cap_msg_t *)buff;

            ca_event_send((ca_device_id_t)pMsg->reserved, pMsg->evt_type, pMsg->data, pMsg->length);
        }
    }
}

void cap_plat_event_init()
{
    pthread_t pThread;
    pthread_attr_t      custom_attr;
    struct sched_param param;
    
    if(evt_flag)
        return;

    pthread_attr_init(&custom_attr);
    pthread_attr_setstacksize(&custom_attr, (size_t)32*1024);
    param.sched_priority = 80;
    pthread_attr_setschedpolicy(&custom_attr,SCHED_RR);
    pthread_attr_setschedparam(&custom_attr,&param);
    pthread_attr_setinheritsched(&custom_attr,PTHREAD_EXPLICIT_SCHED);
    pthread_create(&pThread, &custom_attr, (void *)remote_event_process, NULL);

#ifdef HAVE_EXT_MODE_SC
    ca_event_init(CA_EVENT_MAX, ca_event_send_to_sc, event_cfg_handler);
#else
    ca_event_init(CA_EVENT_MAX, NULL, event_cfg_handler);
#endif
    evt_flag = 1;

    ca_printf("ca-event: init OK (max_event_num %d)\n", CA_EVENT_MAX);

    return;
}

#else

extern struct sock *evt_nl;
ca_uint32_t dest_list[MAX_DEST_NUM] = {0};
static void event_to_remote_cb
    (
    ca_device_id_t device_id,
    ca_int32_t event_type,
    void *msg,
    ca_int32_t len
    )
{
    ca_int32_t total_len;
    cap_msg_t *pMsg = NULL;
    struct sk_buff *skb = NULL;
    ca_int32_t i;

    total_len = CA_MSG_HDR_LEN + len;
    for(i = 0; i < MAX_DEST_NUM; i++)
    {
        if(dest_list[i] == 0)
            break;

        skb = alloc_skb(total_len, GFP_ATOMIC);
        if (skb == NULL) {
            return;
        }

        pMsg = (cap_msg_t *)skb->data;
        pMsg->reserved  = (ca_uint16)device_id;
        pMsg->evt_type  = (ca_uint16)event_type;
        pMsg->length    = len;
        if(len > 0)
            memcpy(pMsg->data, msg, len);
        skb->len = total_len;
	skb_set_tail_pointer(skb, skb->len);
        if(evt_nl == NULL)
        {
            ca_printf("evt_nl is not initialized\n");
            return;
        }

        netlink_unicast(evt_nl , skb , dest_list[i], MSG_DONTWAIT);
    }
    return;
}
static ca_int32_t event_cfg_callback(cap_msg_t *pMsg)
{
    ca_device_id_t device_id = 0;
    ca_event_cfg_local_t *cfg = (ca_event_cfg_local_t *)pMsg->data;

    device_id = (ca_device_id_t)pMsg->reserved;
    return event_cfg_set(device_id,cfg);
}


static ca_int32_t event_dest_set_callback(cap_msg_t *pMsg)
{
    ca_int32_t i;

    for(i = 0; i < MAX_DEST_NUM; i++)
    {
        if(dest_list[i] == 0)
        {
            dest_list[i] = pMsg->to_pid;
            return CA_EVT_OK;
        }
    }

    return CA_EVT_ERROR;
}

void cap_plat_event_init(void)
{
    ca_event_init(CA_EVENT_MAX, event_to_remote_cb, NULL);
    ca_pal_cmd_register(EVT_CFG_CMD, CA_GET_CMD, event_cfg_callback);
    ca_pal_cmd_register(EVT_DEST_SET_CMD, CA_GET_CMD, event_dest_set_callback);
    return;
}
#endif
/*lint -restore*/

