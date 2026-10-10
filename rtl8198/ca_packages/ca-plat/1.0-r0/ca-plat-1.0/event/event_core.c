#include "ca_types.h"
#include "os_core.h"
#include "osal_common.h"
#include "event_core.h"
#include "ca_event.h"
/*
typedef struct _ca_event_cb
{
    ca_list_node_t         node;
    ca_uint                   cb_ptr;
} ca_event_cb_t;
*/
typedef struct _ca_event_cb
{
    ca_uint32_t     event_id;
    ca_uint         cb_ptr;
} ca_event_cb_t;

typedef struct _event_entry
{
    ca_int8     registered;
    ca_int8     suprs_flag; /*0: disable 1:enable*/
    ca_int8     suppressed; /*1: event was suppressed 0: not suppressed*/
    ca_int32    event_type;
    ca_uint32   event_cnt;
    ca_uint32   suprs_evt_cnt;
    ca_uint32   threshold;
    ca_uint32   cur_number;
    ca_uint32   send_error;
    ca_uint32   threshold_interval;
    ca_uint32   cur_interval;
    ca_list_t   *cb_list;
    void        (*enable_evt)(void *data);
    void        (*disable_evt)(void *data);
    void        *data;
} ca_event_entry_t;

typedef struct _event_info
{
    ca_int32    evt_initialized;
    ca_int32    max_evt_type;
    ca_uint      evt_lock;
    ca_uint32   polling_timer;
    event_to_remote send_handler;
    event_cfg_func remote_cfg_local;
    ca_event_entry_t *evt_list;
} ca_event_glb_t;

ca_event_glb_t glb_evt;

#define EVT_INITIALIZED() (glb_evt.evt_initialized)
#define MAX_EVT_TYPE()    (glb_evt.max_evt_type)
#define EVT_LOCK()        ca_mutex_lock(glb_evt.evt_lock)
#define EVT_UNLOCK()      ca_mutex_unlock(glb_evt.evt_lock)

ca_int32_t ca_event_compare(void *data, void *key)
{
	ca_event_cb_t *event_p = data;

	if(event_p->event_id > *(ca_uint32_t*)key){
		return 1;
	}else if(event_p->event_id < *(ca_uint32*)key){

		return -1;
	}
	return 0;

}

static void evt_polling_entry(void *data)
{
    ca_int32 evt_type;
    ca_event_entry_t *p = NULL;

    EVT_LOCK();
    for(evt_type = 0; evt_type < MAX_EVT_TYPE(); evt_type++)
    {
        p = (ca_event_entry_t *)&glb_evt.evt_list[evt_type];
        if(!p->suprs_flag)
            continue;

        p->cur_interval++;
        if(p->cur_interval < p->threshold_interval)
        {
            continue;
        }

        if(p->suppressed)
        {
            p->suppressed   = 0;
            if(p->enable_evt)
            {
                p->enable_evt(p->data);
            }
            continue;
        }
        p->cur_number   = 0;
        p->cur_interval = 0;
    }
    EVT_UNLOCK();

    return;
}

/***********************************************************************
ca_event_init is used to initialize event global parameters:
    memory, we used an array to store event
    event lock
    circle timer(interval = 10ms)

Parameter:
    max_event_type -> Because we used array to store event information,
                      so max_event_type should not be too big, it is
                      betther its value is less than 256
    handler -> This handler is provided by user, this handler is used to
               send event to remote site.
               For example:
                 Linux kernel space ==> Linux user space
                   o local site is kernel space, remote site is user space
                 Firmware ==> SC
                   o local site is Firmware, remote site is SC

Event module don't care event channel, so user must create the channel,
and user could use the parameter 'dest' of 'ca_event_remote_register' to
tell event module where event should be send to.
***********************************************************************/
ca_int32 ca_event_init
    (
    ca_int32 max_event_type,
    event_to_remote send_evt_to_remote,
    event_cfg_func send_cfg_to_local
    )
{
    ca_int32_t i = 0;
    ca_event_entry_t *p = NULL;

    if(max_event_type == 0)
        return CA_EVT_ERROR;

    memset(&glb_evt, 0, sizeof(ca_event_glb_t));
    glb_evt.evt_list = (ca_event_entry_t *)ca_malloc(max_event_type*sizeof(ca_event_entry_t));
    if(glb_evt.evt_list == NULL)
        return CA_EVT_ERROR;

    memset(glb_evt.evt_list, 0, max_event_type*sizeof(ca_event_entry_t));
    glb_evt.max_evt_type = max_event_type;
    ca_mutex_init(&glb_evt.evt_lock, "EVENT_LOCK", 0);
    glb_evt.polling_timer = ca_circle_timer_add(500, evt_polling_entry, NULL);
    glb_evt.send_handler = send_evt_to_remote;
    glb_evt.remote_cfg_local = send_cfg_to_local;
    if(!glb_evt.polling_timer)
    {
        ca_printf("Create event polling timer failed\n");
    }

    for (i = 0; i < max_event_type; i++){
        p = (ca_event_entry_t *)&glb_evt.evt_list[i];
        p->cb_list = ca_list_create(sizeof(ca_event_cb_t), ca_event_compare);
        if(p->cb_list == NULL){
            ca_printf("Create event %d cb_list failed\n", i);
        }
    }

    glb_evt.evt_initialized = 1;

    return CA_EVT_OK;
}

/***********************************************************************
ca_event_attach:      <<<**IMPORTANT**>>>
  This api must be called at local site(SC or kernel space), if you don't
attach an Event, then this event is invalid for system, and you can't
'set/get/send/register/deregister' this event.

Parameter:
  enable -> if throltting is enable, and current event was suppressed,
            after event timer expired, event module will call this api
            to enable event(Usually, this is used to enable related
            interrupt)
  disable -> if throltting is enable, and current event was not suppressed,
             if threshold exceeds the config threshold, event module
             will call this api to disable event(Usually, this is used to
             disable related interrupt.
***********************************************************************/
ca_int32 ca_event_suprs_attach
    (
    ca_int32 evt_type,
    void (*enable)(void *data),
    void (*disable)(void *data),
    void *data
    )
{
    ca_event_entry_t *p = NULL;

    if(!EVT_INITIALIZED() || evt_type >= MAX_EVT_TYPE())
        return CA_EVT_ERROR;

    p = (ca_event_entry_t *)&glb_evt.evt_list[evt_type];

    if((enable == NULL && disable != NULL) ||
        (enable != NULL && disable == NULL))
        return CA_EVT_ERROR;

    memset(p, 0, sizeof(ca_event_entry_t));
    p->event_type       = evt_type;
    p->suprs_flag       = 0;
    p->threshold        = 0;
    p->enable_evt       = enable;
    p->disable_evt      = disable;
    p->data             = data;

    return CA_EVT_OK;
}

/***********************************************************************
ca_event_send :
  This api is used to send event at local site.
  If someone register this event in local site, it will call user's callback
  directly, if this event is register by remote site, it will call
  'event_to_remote' which registered in 'ca_event_init' to send this event
  to remote.
***********************************************************************/
ca_int32 ca_event_send
    (
    ca_device_id_t device_id,
    ca_int32 evt_type,
    void *msg,
    ca_int32 msg_len
    )
{
    ca_list_node_t *node_p = NULL;
    ca_event_entry_t *p = NULL;
    ca_event_cb_t *evt_cb_p = NULL;
    ca_event_callback_ptr local_cb = NULL;

    if(!EVT_INITIALIZED() || evt_type >= MAX_EVT_TYPE())
        return CA_EVT_ERROR;

    EVT_LOCK();
    p = (ca_event_entry_t *)&glb_evt.evt_list[evt_type];
    if(p->suprs_flag && p->suppressed)
    {
        p->suprs_evt_cnt++;
        EVT_UNLOCK();
        return CA_EVT_ERROR;
    }

    if(p->suprs_flag &&
        p->cur_number >= p->threshold)
    {
        if(p->disable_evt)
        {
            p->disable_evt(p->data);
        }
        p->suppressed = 1;
        p->suprs_evt_cnt++;
        EVT_UNLOCK();
        return CA_EVT_ERROR;
    }

    if(glb_evt.send_handler)
    {
        glb_evt.send_handler(device_id, evt_type, msg, msg_len);
    }

    ca_list_scan(p->cb_list, node_p, ca_list_node_t *)
    {
        if (node_p)
        {
            evt_cb_p = (ca_event_cb_t *)node_p->data;

            if(!evt_cb_p->cb_ptr)
                continue;

            local_cb = (ca_event_callback_ptr)evt_cb_p->cb_ptr;
            local_cb(device_id, evt_type, msg, msg_len, NULL);
        }
    }

    p->cur_number++;
    p->event_cnt++;
    EVT_UNLOCK();

    return CA_EVT_OK;
}

ca_status_t ca_event_deregister (
CA_IN		ca_device_id_t			device_id,
CA_IN		ca_uint32_t				event_id,
CA_IN		ca_event_callback_ptr	event_callback)
{
    ca_list_node_t *node_p = NULL;
    ca_event_entry_t *p = NULL;
    ca_event_cb_t *evt_cb_p = NULL;

    if(!EVT_INITIALIZED() || event_id >= MAX_EVT_TYPE())
        return CA_EVT_ERROR;

    EVT_LOCK();
    p = (ca_event_entry_t *)&glb_evt.evt_list[event_id];
    ca_list_scan(p->cb_list, node_p, ca_list_node_t *)
    {
        if (node_p){
            evt_cb_p = (ca_event_cb_t *)(node_p->data);
            if(evt_cb_p->cb_ptr == (ca_uint)event_callback)
            {
                ca_list_delete(p->cb_list, node_p);
                if(ca_list_count(p->cb_list) == 0)
                {
                    p->registered = 0;
                }
                EVT_UNLOCK();
                return CA_EVT_OK;
            }
        }
    }

    EVT_UNLOCK();
    return CA_EVT_OK;
}
ca_status_t ca_event_register (
CA_IN		ca_device_id_t			device_id,
CA_IN		ca_uint32_t				event_id,
CA_IN		ca_event_callback_ptr	event_callback,
CA_IN		ca_void_t				*cookie)
{
    ca_list_node_t *node_p = NULL;
    ca_event_entry_t *p = NULL;
    ca_event_cb_t *evt_cb_p = NULL, event_data;
    if(event_callback == NULL)
        return CA_EVT_ERROR;

    if(!EVT_INITIALIZED() || event_id >= MAX_EVT_TYPE())
        return CA_EVT_ERROR;

    EVT_LOCK();
    p = (ca_event_entry_t *)&glb_evt.evt_list[event_id];
    ca_list_scan(p->cb_list, node_p, ca_list_node_t *)
    {
        if(node_p)
        {
            evt_cb_p = (ca_event_cb_t *)(node_p->data);
            if(evt_cb_p->cb_ptr == (ca_uint)event_callback)
            {
                EVT_UNLOCK();
                return CA_EVT_OK;
            }
        }
    }



    memset(&event_data, 0, sizeof(ca_event_cb_t));
    event_data.event_id = event_id;
    event_data.cb_ptr = (ca_uint)event_callback;
    ca_list_add(p->cb_list, &event_data);
    p->registered = 1;

    EVT_UNLOCK();

    return CA_EVT_OK;
}

ca_status_t ca_event_counter_get (
CA_IN		ca_device_id_t				device_id,
CA_IN		ca_uint32_t				event_id,
CA_OUT		ca_uint32_t				*counter,
CA_IN		ca_boolean_t            read_clear)
{
    ca_event_entry_t *p = NULL;

    *counter = 0;

    if(!EVT_INITIALIZED() || event_id >= MAX_EVT_TYPE())
        return CA_EVT_ERROR;

    EVT_LOCK();
    p = (ca_event_entry_t *)&glb_evt.evt_list[event_id];
    if(glb_evt.remote_cfg_local)
    {
        ca_event_cfg_local_t cfg;
        cfg.cfg_type = EVT_CFG_COUNTER_GET;
        cfg.evt_type = event_id;
        cfg.read_clear = read_clear;
        glb_evt.remote_cfg_local(device_id, &cfg);
        *counter = cfg.counter;
        EVT_UNLOCK();
        return CA_EVT_OK;
    }

    *counter = p->event_cnt;
    if(read_clear)
        p->event_cnt = 0;

    EVT_UNLOCK();
    return CA_EVT_ERROR;
}

ca_status_t ca_event_counter_clear (
CA_IN        ca_device_id_t                device_id,
CA_IN        ca_uint32_t                event_id)
{
    ca_event_entry_t *p = NULL;

    if(!EVT_INITIALIZED() || event_id >= MAX_EVT_TYPE())
        return CA_EVT_ERROR;

    EVT_LOCK();
    p = (ca_event_entry_t *)&glb_evt.evt_list[event_id];
    if(glb_evt.remote_cfg_local)
    {
        ca_event_cfg_local_t cfg;
        cfg.cfg_type = EVT_CFG_COUNTER_CLR;
        cfg.evt_type = event_id;
        glb_evt.remote_cfg_local(device_id, &cfg);
        EVT_UNLOCK();
        return CA_EVT_OK;
    }
    p->event_cnt = 0;
    p->suprs_evt_cnt = 0;
    EVT_UNLOCK();
    return CA_EVT_OK;
}


ca_status_t ca_event_counter_clear_all (
CA_IN        ca_device_id_t                device_id)

{
    ca_int32 evt_type = 0;
    ca_event_entry_t *p = NULL;

    EVT_LOCK();
    if(glb_evt.remote_cfg_local)
    {
        ca_event_cfg_local_t cfg;
        cfg.cfg_type = EVT_CFG_COUNTER_CLR_ALL;
        glb_evt.remote_cfg_local(device_id, &cfg);
        EVT_UNLOCK();
        return CA_EVT_OK;
    }

    for(evt_type = 0; evt_type < MAX_EVT_TYPE(); evt_type++)
    {
        p = (ca_event_entry_t *)&glb_evt.evt_list[evt_type];

        p->event_cnt = 0;
        p->suprs_evt_cnt = 0;
    }
    EVT_UNLOCK();

    return CA_EVT_OK;
}

ca_int32 ca_event_set
    (
    ca_device_id_t device_id,
    ca_int32 evt_type,
    ca_event_cfg_t *cfg
    )
{
    ca_event_entry_t *p = NULL;
    ca_uint32 temp = 0;

    if(cfg == NULL)
        return CA_EVT_ERROR;

    if(!EVT_INITIALIZED() || evt_type >= MAX_EVT_TYPE())
        return CA_EVT_ERROR;

    EVT_LOCK();
    p = (ca_event_entry_t *)&glb_evt.evt_list[evt_type];
    if(glb_evt.remote_cfg_local)
    {
        ca_event_cfg_local_t cfg_temp;
        cfg_temp.cfg_type = EVT_CFG_SET;
        cfg_temp.evt_type = evt_type;
        memcpy(&cfg_temp.cfg, cfg, sizeof(ca_event_cfg_t));
        glb_evt.remote_cfg_local(device_id, &cfg_temp);
        EVT_UNLOCK();
        return CA_EVT_OK;
    }

    p->suprs_evt_cnt      = 0;
    temp = (cfg->interval <= 10)?1:(cfg->interval/10);
    p->threshold_interval = temp;
    p->threshold          = cfg->threshold;
    p->suprs_flag         = (cfg->throttling_ena)?1:0;
    if(p->enable_evt)
    {
        p->enable_evt(p->data);
    }

    EVT_UNLOCK();
    return CA_EVT_OK;
}

ca_int32 ca_event_get
    (
    ca_device_id_t device_id,
    ca_int32 evt_type,
    ca_event_cfg_t *cfg
    )
{
    ca_event_entry_t *p = NULL;

    if(cfg == NULL)
        return CA_EVT_ERROR;

    if(!EVT_INITIALIZED() || evt_type >= MAX_EVT_TYPE())
        return CA_EVT_ERROR;

    EVT_LOCK();
    p = (ca_event_entry_t *)&glb_evt.evt_list[evt_type];
    if(glb_evt.remote_cfg_local)
    {
        ca_event_cfg_local_t cfg_temp;
        cfg_temp.cfg_type = EVT_CFG_GET;
        cfg_temp.evt_type = evt_type;
        memcpy(&cfg_temp.cfg, cfg, sizeof(ca_event_cfg_t));
        glb_evt.remote_cfg_local(device_id, &cfg_temp);
        memcpy(cfg, &cfg_temp.cfg, sizeof(ca_event_cfg_t));
        EVT_UNLOCK();
        return CA_EVT_OK;
    }

    cfg->threshold        = p->threshold;
    cfg->interval         = p->threshold_interval*10;
    cfg->throttling_ena   = p->suprs_flag;
    EVT_UNLOCK();

    return CA_EVT_OK;
}

void ca_event_dump(void)
{
    ca_int32 evt_type = 0;
    ca_event_entry_t *p = NULL;

    if(glb_evt.remote_cfg_local)
    {
        ca_event_cfg_local_t cfg;
        cfg.cfg_type = EVT_CFG_DUMP;
        cfg.evt_type = evt_type;
        glb_evt.remote_cfg_local(0, &cfg);
        return ;
    }

    ca_printf("===============================================================\n");
    ca_printf("%-4s %-10s %-8s %-7s %-9s %-9s %-12s\n","Type","Count","Suprs-en","Suprs-s","Suprs-cnt","Threshold","Period(ms)");
    ca_printf("---------------------------------------------------------------\n");
    for(evt_type = 0; evt_type < MAX_EVT_TYPE(); evt_type++)
    {
        p = (ca_event_entry_t *)&glb_evt.evt_list[evt_type];
        if(!p->suprs_flag)
        {
            ca_printf("%-4d %-10d %-8s %-7s %-9s %-9s %s\n",
                       evt_type, p->event_cnt, "Disable","-","-","-","-");
        }
        else
        {
            ca_printf("%-4d %-10d %-8s %-7s %-9d %-9d %d\n",
                       evt_type, p->event_cnt, "Enable",
                       (p->suppressed)?"Yes":"No",
                       p->suprs_evt_cnt, p->threshold,
                       p->threshold_interval*10);
        }
    }
    ca_printf("\n");
    return;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(ca_event_register);
EXPORT_SYMBOL(ca_event_deregister);
EXPORT_SYMBOL(ca_event_send);
#endif


