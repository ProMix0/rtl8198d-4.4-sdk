#include "ca_types.h"
#include "os_core.h"
#include "osal_timer.h"
#include "osal_spinlock.h"
#include "osal_mem.h"
#include "osal_mutex.h"
#include "osal_cmn.h"
#include "util_list.h"

#define OSAL_MAX_TIMER_NUM 256
typedef struct {
    ca_list_t   *timer_list;
    ca_uint_t   timer_tid;
    ca_uint_t   timer_lock;
    ca_int32_t  max_timer;
    ca_uint32_t max_unique_id;
    ca_uint32_t last_drain;
    ca_uint32_t max_drain;
    ca_uint64_t max_drain_cb;
    ca_uint64_t last_tick;
    ca_uint32_t expire_timers;
} ca_timer_info_t;

#define OSAL_TIMER_LOCK(p)   ca_mutex_lock(p.timer_lock)
#define OSAL_TIMER_UNLOCK(p) ca_mutex_unlock(p.timer_lock)



ca_int32_t  timer_initialized = 0;
ca_timer_info_t timer_info;
ca_int_t tmr_tid = 0;

ca_int32 ca_timer_compare(void *data, void *key)
{
	ca_timer_t *pTimer = data;

	if(pTimer->unique_id > *(ca_uint32*)key){
		return 1;
	}else if(pTimer->unique_id < *(ca_uint32*)key){

		return -1;
	}
	return 0;

}

static ca_uint32_t ca_timer_unique_id(void)
{
    ca_uint32_t unique_id = timer_info.max_unique_id;

GET_UNI_ID_AGAIN:
    if(unique_id == (ca_uint32_t)-1)
    {
        unique_id = 1;
    }
    else
    {
        unique_id += 1;
    }

	if(ca_list_query(timer_info.timer_list, &unique_id) != NULL)
    {
        goto GET_UNI_ID_AGAIN;
    }

    timer_info.max_unique_id = unique_id;

    return unique_id;
}

static void ca_timer_process(void)
{
    ca_list_node_t *pNode = NULL;
    ca_timer_t *pTimer = NULL;
    ca_list_node_t *pNext = NULL;
    ca_uint64_t prev = 0, next = 0;
    ca_uint64_t delta = 0;

    OSAL_TIMER_LOCK(timer_info);
    /*msleep is not accuracy, Need adjustment*/
    delta =  ca_current_tick() - timer_info.last_tick +timer_info.last_drain;
    for(pNode = ca_list_first(timer_info.timer_list); pNode != NULL; )
    {
        pNext = ca_list_next(pNode);

        pTimer = (ca_timer_t *)(pNode->data);
        if(pTimer->del_apply)
        {
            ca_list_delete(timer_info.timer_list, pNode);
            pNode = pNext;
            continue;
        }

        if(pTimer->stop_apply)
        {
            pNode = pNext;
            continue;
        }

        if(pTimer->remain > delta)
        {
            pTimer->remain -= delta;
            pNode = pNext;
            continue;
        }
        else
            pTimer->remain = 0;

        timer_info.expire_timers++;
        prev = ca_current_tick();
        pTimer->is_processed = 1;
        OSAL_TIMER_UNLOCK(timer_info);

        pTimer->callback(pTimer->data);
        pTimer->is_processed = 0;

        OSAL_TIMER_LOCK(timer_info);
        next = ca_current_tick();
        timer_info.last_drain = (next > prev)?(next-prev):(((ca_uint64_t)-1)-prev+next+1);
        delta += timer_info.last_drain;
        if(timer_info.last_drain > pTimer->time_drain)
        {
            pTimer->time_drain = timer_info.last_drain;
        }
        if(timer_info.last_drain > timer_info.max_drain)
        {
            timer_info.max_drain = timer_info.last_drain;
            timer_info.max_drain_cb = (long)pTimer->callback;
        }

        if(pTimer->timer_type == CA_ONCE_TIMER)
        {
            if(pTimer->remain == 0)
            {
                ca_list_delete(timer_info.timer_list, pNode);
            }
        }
        else
        {
            pTimer->remain = pTimer->interval;
        }
        pNode = pNext;

    }
    timer_info.last_tick = ca_current_tick();
    OSAL_TIMER_UNLOCK(timer_info);

    return;
}

static void ca_timer_entry(void *data)
{
    tmr_tid = OS_CURRENT_THREAD();

    while(1)
    {
        OS_SLEEP(10);
        ca_timer_process();
    }
}

static void ca_timer_init(ca_int32_t max_timer){
    ca_int32_t ret = CA_OSAL_ERROR;

    if(timer_initialized)
        return;

    memset(&timer_info, 0, sizeof(ca_timer_info_t));
    ca_mutex_init(&timer_info.timer_lock, "Iros_Timer", 0);
    timer_info.timer_list = ca_list_create(sizeof(ca_timer_t), ca_timer_compare);
    if(timer_info.timer_list == NULL)
    {
        return;
    }
    ret = OS_THREAD_CREATE(&timer_info.timer_tid, "IROS-Timer", ca_timer_entry, NULL, 32*1024, 3, 0);
    if(ret)
        return;

    timer_info.max_timer = max_timer;
    timer_initialized = 1;

    return;
}

ca_uint32_t ca_timer_add(
    ca_int32_t interval,
    void (*callback)(void *data),
    void *data)
{
    ca_timer_t pTimer;

    ca_timer_init(OSAL_MAX_TIMER_NUM);

    if(callback == NULL)
        return 0;

    OSAL_TIMER_LOCK(timer_info);
    if(ca_list_count(timer_info.timer_list) >= (ca_uint32_t)timer_info.max_timer)
    {
        OSAL_TIMER_UNLOCK(timer_info);
        OS_PRINTF("Timer number exceed(%d)\n", timer_info.max_timer);
        return 0;
    }

    memset(&pTimer, 0, sizeof(ca_timer_t));
    interval = (interval <= 10)?1:(interval/10);
    pTimer.interval    = pTimer.remain = interval;
    pTimer.callback    = callback;
    pTimer.data        = data;
    pTimer.timer_type  = CA_ONCE_TIMER;
    pTimer.unique_id   = ca_timer_unique_id();

    ca_list_add(timer_info.timer_list, &pTimer);
    OSAL_TIMER_UNLOCK(timer_info);

    return (ca_uint32_t)pTimer.unique_id;
}

ca_uint32_t ca_circle_timer_add(
    ca_int32_t interval,
    void (*callback)(void *data),
    void *data)
{
    ca_timer_t pTimer;

    ca_timer_init(OSAL_MAX_TIMER_NUM);

    if(callback == NULL)
        return 0;

    OSAL_TIMER_LOCK(timer_info);
    if(ca_list_count(timer_info.timer_list) >= (ca_uint32_t)timer_info.max_timer)
    {
        OSAL_TIMER_UNLOCK(timer_info);
        OS_PRINTF("Timer number exceed(%d)\n", timer_info.max_timer);
        return 0;
    }

    memset(&pTimer, 0, sizeof(ca_timer_t));
    interval = (interval <= 10)?1:(interval/10);
    pTimer.interval    = pTimer.remain = interval;
    pTimer.callback    = callback;
    pTimer.data        = data;
    pTimer.timer_type  = CA_CIRCLE_TIMER;
    pTimer.unique_id   = ca_timer_unique_id();

    ca_list_add(timer_info.timer_list, &pTimer);
    OSAL_TIMER_UNLOCK(timer_info);

    return (ca_uint32_t)pTimer.unique_id;
}

ca_int32_t ca_timer_del(ca_uint32_t timer_id)
{
    ca_list_node_t *pNode = NULL;
    ca_timer_t *pTimer = NULL;
    ca_int32_t ret = CA_TIMER_OK;

    OSAL_TIMER_LOCK(timer_info);

    ca_list_scan(timer_info.timer_list, pNode, ca_list_node_t *)
    {
        if(pNode)
        {
            pTimer = (ca_timer_t *)(pNode->data);
            if(pTimer->unique_id == timer_id)
            {
                break;
            }
        }
    }

    if(pNode != NULL)
    {
        if(tmr_tid != OS_CURRENT_THREAD())
        {
            pTimer = (ca_timer_t *)(pNode->data);
            if(pTimer->is_processed)
            {
                ret = CA_TIMER_IN_PROC;
            }
        }
        pTimer->del_apply = 1;
    }
    else
        OS_PRINTF("Oops: Delete an invalid timer!!\r\n");
    OSAL_TIMER_UNLOCK(timer_info);

    return ret;
}

ca_int32_t ca_timer_stop(ca_uint32_t timer_id)
{
    ca_list_node_t *pNode = NULL;
    ca_timer_t *pTimer = NULL;
    ca_int32_t ret = CA_TIMER_OK;

    OSAL_TIMER_LOCK(timer_info);

    ca_list_scan(timer_info.timer_list, pNode, ca_list_node_t *)
    {
        if(pNode)
        {
            pTimer = (ca_timer_t *)(pNode->data);
            if(pTimer->unique_id == timer_id)
            {
                break;
            }
        }
    }

    if(pNode != NULL)
    {
        if(tmr_tid != OS_CURRENT_THREAD())
        {
            pTimer = (ca_timer_t *)(pNode->data);
            if(pTimer->is_processed)
            {
                ret = CA_TIMER_IN_PROC;
            }
        }
        pTimer->stop_apply = 1;
    }
    else
        OS_PRINTF("Oops: Stop an invalid timer!!\r\n");
    OSAL_TIMER_UNLOCK(timer_info);

    return ret;
}

void ca_timer_start(ca_uint32_t timer_id)
{
    ca_list_node_t *pNode = NULL;
    ca_timer_t *pTimer = NULL;

    OSAL_TIMER_LOCK(timer_info);

    ca_list_scan(timer_info.timer_list, pNode, ca_list_node_t *)
    {
        if(pNode)
        {
            pTimer = (ca_timer_t *)(pNode->data);
            if(pTimer->unique_id == timer_id)
            {
                break;
            }
        }
    }

    if(pNode != NULL)
    {
        pTimer = (ca_timer_t *)(pNode->data);
        pTimer->remain  = pTimer->interval;
        pTimer->stop_apply = 0;
        pTimer->del_apply = 0;
    }

    OSAL_TIMER_UNLOCK(timer_info);
    return;
}

void ca_timer_restart(ca_uint32_t timer_id, ca_uint32_t   interval)
{
    ca_list_node_t *pNode = NULL;
    ca_timer_t *pTimer = NULL;

    OSAL_TIMER_LOCK(timer_info);

    ca_list_scan(timer_info.timer_list, pNode, ca_list_node_t *)
    {
        if(pNode)
        {
            pTimer = (ca_timer_t *)(pNode->data);
            if(pTimer->unique_id == timer_id)
            {
                break;
            }
        }
    }

    if(pNode != NULL)
    {
        pTimer = (ca_timer_t *)(pNode->data);
		interval = (interval <= 10)?1:(interval/10);
    	pTimer->interval = interval;
        pTimer->remain  = interval;
        pTimer->stop_apply = 0;
        pTimer->del_apply = 0;
    }

    OSAL_TIMER_UNLOCK(timer_info);
    return;

}

ca_int32_t ca_timer_count()
{
    ca_int32_t cnt = 0;
    OSAL_TIMER_LOCK(timer_info);
    cnt = ca_list_count(timer_info.timer_list);
    OSAL_TIMER_UNLOCK(timer_info);

    return cnt;
}

void ca_timer_show(void)
{
    ca_list_node_t *pNode = NULL;
    ca_timer_t *pTimer = NULL;
    ca_int32_t Number = 0;

    OS_PRINTF("\r\nNo. Id.       Remain Interval Type    Drain Callback\r\n");
    OSAL_TIMER_LOCK(timer_info);
    ca_list_scan(timer_info.timer_list, pNode, ca_list_node_t *)
    {
        pTimer = (ca_timer_t *)(pNode->data);
        OS_PRINTF("%-4d%-10d%-7d%-9d%-8s%-6d0x%p\r\n", Number++,
                    pTimer->unique_id, pTimer->remain, pTimer->interval,
                    (pTimer->timer_type == CA_CIRCLE_TIMER)?"Circle":"OneShot",
                    (ca_uint32_t)pTimer->time_drain, pTimer->callback);
    }
    OS_PRINTF("Max timer drain:(ptr=0x%08llx, drain=%d)\r\n",
                (ca_uint64_t)timer_info.max_drain_cb, timer_info.max_drain);
    OS_PRINTF("Expired timer Number: %d\r\n", timer_info.expire_timers);
    OS_PRINTF("\r\n");
    OSAL_TIMER_UNLOCK(timer_info);
}

