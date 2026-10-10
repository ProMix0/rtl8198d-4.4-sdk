/******************************************************************************

                            Copyright (c) 2001-2016
                        Lantiq Beteiligungs-GmbH & Co.KG
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_rtk_linux.c
   This file contains an implementation of High-Level TAPI Driver,
   Linux specific part.
*/

#ifdef LINUX

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi_config.h"

#ifdef __KERNEL__
   #include <linux/kernel.h>
   #include <linux/version.h>
#endif /* __KERNEL__ */
#ifdef MODULE
   #include <linux/module.h>
#endif /* MODULE */

#ifdef REALTEK_PATCH_FOR_INTEL
   #include <linux/module.h>
#endif

#ifdef __KERNEL__
   #ifdef TAPI_FEAT_PROCFS
      #include <linux/proc_fs.h>       /*proc-file system*/
      #include <linux/seq_file.h>
   #endif
   #if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
      #define PDE_DATA(inode) PDE(inode)->data
   #endif
   #include <linux/timer.h>            /* init_timer() */
   #include <linux/init.h>
   #include <linux/errno.h>
   #ifdef ENABLE_HOTPLUG
      #include <linux/skbuff.h>
      #include <linux/netlink.h>
      #include <net/sock.h>
      #include <linux/kobject.h>
   #endif /* #ifdef ENABLE_HOTPLUG*/
   #include <asm/byteorder.h>
   #include <asm/io.h>
   #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
      #include <linux/tqueue.h>
      #include <linux/sched.h>
   #else
      #include <linux/device.h>
      #include <linux/sched.h>
   #endif /* LINUX_VERSION_CODE check */
   /* include for UTS_RELEASE */
   #ifndef UTS_RELEASE
      #if   (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
         #include <linux/uts.h>
      #elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33))
         #include <linux/utsrelease.h>
      #else
         #include <generated/utsrelease.h>
      #endif /* LINUX_VERSION_CODE */
   #endif /* UTS_RELEASE */
#endif /* __KERNEL__ */

#include "drv_tapi.h"
#include "drv_tapi_api.h"
#include "drv_tapi_errno.h"

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ================================== */
/* channel specific wrapper structure */
/* ================================== */



/* ============================= */
/* Local Functions               */
/* ============================= */
#ifdef __KERNEL__
/*lint -save
   -esym(528,TAPI_timer_call_back)
   -esym(528,TAPI_tqueue) */
static IFX_void_t TAPI_timer_call_back (IFX_ulong_t arg);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
static IFX_void_t TAPI_tqueue (IFX_void_t *pWork);
#else /* for Kernel newer or equal 2.6.20 */
static IFX_void_t TAPI_tqueue (struct work_struct *pWork);
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) */
/*lint -restore */
#endif /* __KERNEL__ */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
/* TAPI's timers workqueue */
static struct workqueue_struct *pTAPItimersWq;
/* TAPI's deferred tasks (events) workqueue. Events and timers should use
 * separate workqueues, because events might be expecting triggering of
 * timers, which will not happen if a single queue is used for both. */
static struct workqueue_struct *pTAPIeventsWq;
#  if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,8,0))
/* init struct for the RT workqueue to set the scheduling policy
   via TAPI_DeferWork */
static IFX_TAPI_EXT_EVENT_PARAM_t tapi_wq_setscheduler_param;
#  endif
#endif /* LINUX_VERSION_CODE */


#include "drv_tapi_rtk_api.h"
/* ============================= */
/* Global function definition    */
/* ============================= */
/**
   Register the low level driver with the operating system.

   \return
   TAPI_statusOk

   \remarks
   For Linux the device nodes need to be registered and then the driver
   itself is registered with the kernel.
*/
IFX_return_t TAPI_OS_RegisterLLDrv (IFX_TAPI_DRV_CTX_t* pLLDrvCtx,
                                    IFX_TAPI_HL_DRV_CTX_t* pHLDrvCtx)
{

   pLLDrvCtx = pLLDrvCtx;
   pHLDrvCtx = pHLDrvCtx;

   return TAPI_statusOk;
}

/**
   UnRegister the low level driver from the system.

   \return
   TAPI_statusErr on error, otherwise TAPI_statusOk
*/
IFX_return_t TAPI_OS_UnregisterLLDrv (IFX_TAPI_DRV_CTX_t* pLLDrvCtx,
                                      IFX_TAPI_HL_DRV_CTX_t* pHLDrvCtx)
{
   pLLDrvCtx = pLLDrvCtx;
   pHLDrvCtx = pHLDrvCtx;
         
   return TAPI_statusOk;
}

/**
   Open the device.

   At the first time:
   - Initialize the high-level TAPI device structure
   - Call the low-level function to initialise the low-level device structure
   - Initialize the high-level TAPI channel structure
   - Call the low-level function to initialise the low-level channel structure

   \param  inode        Pointer to the inode.
   \param  filp         Pointer to the file descriptor.

   \return
   0 - if no error,
   otherwise error code
*/
int ktapi_open (IFX_uint32_t dev, IFX_uint32_t chn)
{
   IFX_TAPI_DRV_CTX_t      *pDrvCtx    = IFX_NULL;
   TAPI_DEV                *pTapiDev   = IFX_NULL;
   
   IFX_uint32_t            nDev = dev,
                           nCh = chn;

                           
   TRACE (TAPI_DRV, DBG_LEVEL_LOW,
         ("ktapi open %d/%d\n", dev, chn));

   /* Get the pointer to the device driver context based on the major number */
   pDrvCtx = IFX_TAPI_DeviceDriverContextGet (TAPI_MAJOR);

   if (pDrvCtx == IFX_NULL)
   {
      /* This should actually never happen because the file descriptors are
         registered in TAPI_OS_RegisterLLDrv after a driver context is known. */
      printk (KERN_INFO "tapi_open: error finding DrvCtx\n");
      return -ENODEV;
   }

    /* check the device number */
    if (nDev >= pDrvCtx->maxDevs)
    {
     TRACE(TAPI_DRV, DBG_LEVEL_HIGH,
          ("TAPI_DRV: max. device number exceed\n"));
     return -ENODEV;
    }

   /* check the channel number */
   if (nCh > pDrvCtx->maxChannels)
   {
      TRACE(TAPI_DRV, DBG_LEVEL_HIGH,
          ("TAPI_DRV: max. channel number exceed\n"));
      return -ENODEV;
   }

   pTapiDev = pDrvCtx->pTapiDev + nDev;


   /* Call the Low level Device specific open routine */
   if (IFX_TAPI_PtrChk (pDrvCtx->Open))
   {
      IFX_int32_t retLL;

      retLL = pDrvCtx->Open (pTapiDev);

      if (!TAPI_SUCCESS(retLL))
      {
         TRACE(TAPI_DRV, DBG_LEVEL_HIGH,
               ("Open LL channel failed. [dev:%d ch:%d]\n", nDev, nCh));
         return -ENODEV;
      }
   }

   /* increment the use counter */
   pTapiDev->nInUse++;

   return 0;
}


/**
   Close a file.

   This function gets called when a close is called on the file descriptor.
   Both types device and channel file descriptors are handled here. The
   function decrements the usage count, and calls the LL release routine.

   \param  inode        Pointer to the inode.
   \param  filp         Pointer to the file descriptor.

   \return
   0 - if no error,
   otherwise error code
*/
int ktapi_release (IFX_uint32_t dev, IFX_uint32_t chn)
{

   return 0;
}


/* ============================= */
/* Timer abstraction             */
/* ============================= */
#ifdef TAPI_HAVE_TIMERS

#ifdef __KERNEL__
   /* Functionpointer to the callbackfunction */
   typedef IFX_void_t (*linux_timer_callback)(IFX_ulong_t);

   /* Timer ID */
   struct Timer_ID_s
   {
   #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
      /* !!! important, work struct must be the first element, because we
             need to cast it later on to Timer_ID */
      struct tq_struct timerTask;
   #else
      struct work_struct timerTask;
   #endif /* LINUX_VERSION_CODE */
      struct timer_list Timer_List;
      IFX_boolean_t bPeriodical;
      IFX_uint32_t Periodical_Time;
      TIMER_ENTRY pTimerEntry;
      IFX_ulong_t nArgument;
      IFX_boolean_t bStopped;
   };
#else /* ! __KERNEL__ */
/** List entry */
struct list_entry {
   /** Previous list entry */
   struct list_entry *next;
   /** Next list entry */
   struct list_entry *prev;
};

/** List structure */
struct list {
   /** Used list entries, this first element is empty
       and can be used only to pint to other elements */
   struct list_entry first_element;
   /** Size of list entry data */
   size_t payload_size;
   /** List lock */
   IFXOS_lock_t lock;
};

/** timeout descriptor structure */
struct timeout {
   /** set IFX_TRUE if handler should be called periodically,
       otherwise set IFX_FALSE */
   IFX_boolean_t bPeriodical;
   /** timeout argument */
   unsigned long arg1;
   /** timeout handler */
   TIMER_ENTRY handler;
};

/** timeout list entry */
struct timeout_list_entry {
   /** Time when the timeout becomes active (in milliseconds) */
   time_t timeout_time;
   /** Time to wait from setting the timeout, used for periodic events */
   time_t time_in;
   /** timeout descriptor */
   struct timeout timeout;
};

/** timeout control structure */
struct TAPI_TM_Context {
   /* IFX_TRUE if timers were initialized */
   IFX_boolean_t bTimersInialized;
   /** timeout list */
   struct list timeout_list;
   /** Timeout thread control structure */
   IFXOS_ThreadCtrl_t timeout_thread_ctrl;
};

/** default timeout thread poll time (in milliseconds),
    this is the polling time used to check if new entiries/events were added */
#ifndef TIMEOUT_THREAD_POLL_TIME
   #define TIMEOUT_THREAD_POLL_TIME   50
#endif

/** get pointer to payload of given entry */
#define list_entry_data(ENTRY) ((void *)((char *)ENTRY + sizeof(struct list_entry)))

/** check if list is empty */
#define is_list_empty(LIST) \
   (((LIST)->first_element.next == &(LIST)->first_element) ? IFX_TRUE : IFX_FALSE)
#define foreach_list_entry_safe_ll(PLIST, ENTRY, NEXT_ENTRY) \
   for ((ENTRY) = (PLIST)->next, (NEXT_ENTRY) = (ENTRY)->next; \
        (ENTRY)->next != (PLIST)->next; \
        (ENTRY) = (NEXT_ENTRY), (NEXT_ENTRY) = (ENTRY)->next)
#define foreach_list_entry(PLIST, ENTRY) \
   for ((ENTRY) = (PLIST)->first_element.next; \
        (ENTRY)->next != (PLIST)->first_element.next; \
        (ENTRY) = (ENTRY)->next)

static IFX_return_t TAPI_TM_list_init(struct list *list, size_t payload_size);
static void TAPI_TM_list_entry_free(struct list *list,
             struct list_entry *entry);
static IFX_return_t TAPI_TM_lockless_event_remove(struct TAPI_TM_Context *context,
                    struct list_entry *entry_to_remove);
static IFX_return_t TAPI_TM_timeout_event_remove(struct TAPI_TM_Context *context,
                 struct list_entry *timer_entry);
static IFX_return_t TAPI_TM_timeout_event_stop(struct TAPI_TM_Context *context,
                 struct list_entry *timer_entry);
static struct list_entry *TAPI_TM_next_active_event_get(struct TAPI_TM_Context *context,
                    struct timeout *timeout,
                    time_t * time_to_next_entry);
static void TAPI_TM_list_entry_remove(struct list *list,
             struct list_entry *entry);
static IFX_return_t TAPI_TM_lockless_event_stop(struct TAPI_TM_Context *context,
                    struct list_entry *entry_to_stop);
static IFX_int32_t TAPI_TM_timeout_thread_main(struct IFXOS_ThreadParams_s *thr_params);
static void TAPI_TM_list_delete(struct list *list);
static IFX_return_t TAPI_TM_timeout_init(struct TAPI_TM_Context *context);
static struct list_entry *TAPI_TM_event_entry_create(struct TAPI_TM_Context *context,
                   const struct timeout *timeout);
static struct list_entry *TAPI_TM_timeout_event_create(struct TAPI_TM_Context *context,
              TIMER_ENTRY handler,
              unsigned long arg1);
static IFX_return_t TAPI_TM_timeout_event_start(struct TAPI_TM_Context *context,
              struct list_entry *timer_entry,
              time_t timeout_time, IFX_boolean_t bPeriodical);
static IFX_return_t TAPI_TM_event_entry_start(struct TAPI_TM_Context *context,
                   struct list_entry *timer_entry,
                   time_t timeout_time,
                   IFX_boolean_t bPeriodical);
static IFX_return_t TAPI_TM_lockless_event_entry_start(struct TAPI_TM_Context *context,
                   struct list_entry *timer_entry,
                   time_t timeout_time);
static void TAPI_TM_list_entry_add_before(struct list *list,
            struct list_entry *entry,
            struct list_entry *new_entry);
static void TAPI_TM_list_entry_add_tail(struct list *list,
                   struct list_entry *new_entry);
static void TAPI_TM_list_entry_add_after(struct list *list,
           struct list_entry *entry,
           struct list_entry *new_entry);

/** \todo deleting/clean up of timers is missing for user space, it should
    be called upon closing the tapi thread or even maybe for dev stop */
/** control structure for timers in user's space */
struct TAPI_TM_Context G_timers /* = {.bTimersInialized = IFX_FALSE} */;

/**
   Create new list_entry element and assign a callback function to it.

   \param  context      pointer to tapi timer context structure
   \param  handler      pointer to callback function
   \param  arg1         private data for the callback, can be pointer or int

   \return
     - pointer to newly allocated list_entry
     - IFX_NULL in case of error, return value of TAPI_TM_event_entry_create()
*/
static struct list_entry *TAPI_TM_timeout_event_create(struct TAPI_TM_Context *context,
              TIMER_ENTRY handler,
              unsigned long arg1)
{
   struct timeout timeout;

   timeout.handler = handler;
   timeout.arg1 = arg1;
   timeout.bPeriodical = IFX_FALSE;

   return TAPI_TM_event_entry_create(context, &timeout);
}

/**
   Create new list_entry element, allocate memory and copy timeout data.

   \param  context      pointer to tapi timer context structure
   \param  timeout      pointer to timeout stucture

   \return
     - pointer to newly allocated list_entry
     - IFX_NULL in case of error
*/
static struct list_entry *TAPI_TM_event_entry_create(struct TAPI_TM_Context *context,
                   const struct timeout *timeout)
{
   struct list_entry *new_entry;
   struct timeout_list_entry *new_timeout_entry;

   TAPI_OS_LockGet(&context->timeout_list.lock);
   /* allocate memory for list entry with payload */
   new_entry = TAPI_OS_Malloc(sizeof(struct list_entry) + context->timeout_list.payload_size);
   if (!new_entry) {
      TAPI_OS_LockRelease(&context->timeout_list.lock);
      return IFX_NULL;
   }

   new_timeout_entry = list_entry_data(new_entry);

   memcpy(&new_timeout_entry->timeout, timeout,
          sizeof(struct timeout));

   TAPI_OS_LockRelease(&context->timeout_list.lock);

   return new_entry;
}

/**
   Initialize TAPI_TM_Context structure and its list and start timeout control
   thread.

   \param  context      pointer to tapi timer context structure

   \return
     - IFX_SUCCESS or IFX_ERROR in case of errors
*/
static IFX_return_t TAPI_TM_timeout_init(struct TAPI_TM_Context *context)
{
   IFX_return_t error;

   error = TAPI_TM_list_init(&context->timeout_list, sizeof(struct timeout_list_entry));

   if (error)
   {
      TRACE( TAPI_DRV, DBG_LEVEL_HIGH,
           ("TAPI_DRV: TAPI_TM_list_init() failed for timer Initialization\n"));
      return error;
   }

   error = (IFX_SUCCESS == TAPI_OS_ThreadInit(&context->timeout_thread_ctrl,
                 "tapitm",
                 TAPI_TM_timeout_thread_main,
                 IFXOS_DEFAULT_STACK_SIZE,
                 TAPI_OS_THREAD_PRIO_HIGHEST,
                 (unsigned long)context,
                 0)) ? IFX_SUCCESS : IFX_ERROR;

   if (error)
   {
      TRACE( TAPI_DRV, DBG_LEVEL_HIGH,
           ("TAPI_DRV: TAPI_OS_ThreadInit() failed for timer Initialization\n"));
      TAPI_TM_list_delete(&context->timeout_list);
      return error;
   }
   return IFX_SUCCESS;
}

/**
   Release memory and other resources used.

   \param  list         pointer to list

*/
static void TAPI_TM_list_delete(struct list *list)
{
   struct list_entry *entry, *tmp_entry;

   foreach_list_entry_safe_ll(&list->first_element, entry, tmp_entry)
   {
      TAPI_OS_Free(entry);
   }
   /* if list is empty fileds next and prev should point first_element */
   list->first_element.next = &list->first_element;
   list->first_element.prev = &list->first_element;
   (void)TAPI_OS_LockDelete(&list->lock);
}

/** timeout events handling thread

   \param[in] thr_params Thread arguments

   \return 0
*/
static IFX_int32_t TAPI_TM_timeout_thread_main(struct IFXOS_ThreadParams_s *thr_params)
{
   struct TAPI_TM_Context *context = (struct TAPI_TM_Context *)thr_params->nArg1;
   struct timeout timer;
   struct list_entry *timer_to_execute_entry;
   time_t wait_time;

   memset (&timer, 0, sizeof (timer));
   /* while thread is running */
   while (thr_params->bRunning == IFX_TRUE &&
          thr_params->bShutDown == IFX_FALSE)
   {
      /* wait for message in FIFO */
      while (1)
      {
         wait_time = 0;
         TAPI_OS_LockGet(&context->timeout_list.lock);
         timer_to_execute_entry = TAPI_TM_next_active_event_get(context, &timer,
                                     &wait_time);
         TAPI_OS_LockRelease(&context->timeout_list.lock);
         if (timer_to_execute_entry == IFX_NULL
             && thr_params->bShutDown == IFX_FALSE
             && thr_params->bRunning == IFX_TRUE)
         {
            /* if time to the next timeout is longer than the poll time,
               then wait only TIMEOUT_THREAD_POLL_TIME ms */
            if ((wait_time == 0) || (wait_time > TIMEOUT_THREAD_POLL_TIME))
            {
               wait_time = TIMEOUT_THREAD_POLL_TIME;
            }
            /** \todo replace sleep with select, for this some sync mechanism
                with add/start entry is needed, for example select could wait
                for a pipe fd being readable, each function for add/start entry
                would write to this pipe, this could replace the polling,
                if above is implemented then wait_time could be set to exact
                time to next timeout or wait forever untill and event/entry
                is added */
            TAPI_OS_MSecSleep((IFX_time_t) wait_time);
         }
         else
         {
            break;
         }
      }

      /* check if we are shutting down */
      if (thr_params->bShutDown == IFX_TRUE
          || thr_params->bRunning == IFX_FALSE)
      {
         break;
      }

      TAPI_OS_LockGet(&context->timeout_list.lock);
      {
         struct timeout_list_entry *periodical_timeout_entry;
         time_t time_in;

         periodical_timeout_entry = list_entry_data(timer_to_execute_entry);
         (void)TAPI_TM_lockless_event_stop(context, timer_to_execute_entry);
         if (periodical_timeout_entry && timer.bPeriodical)
         {
            /* for periodical events get time to next timeout and add new timeout */
            time_in = periodical_timeout_entry->time_in;
            (void) TAPI_TM_lockless_event_entry_start(context, timer_to_execute_entry,
                                                      time_in);
         }
      }
      TAPI_OS_LockRelease(&context->timeout_list.lock);
      if (timer.handler != NULL)
      {
         (void) timer.handler((Timer_ID) timer_to_execute_entry, (IFX_ulong_t) timer.arg1);
      }
      else
      {
         TRACE( TAPI_DRV, DBG_LEVEL_HIGH,
               ("TAPI_TM_timeout_thread_main - ERROR: found event without timer handler"));
      }
   }
   return 0;
}

/**
   Lockless version of TAPI_TM_timeout_event_remove.

   \param  context         pointer to tapi timer context structure
   \param  entry_to_stop   pointer to tapi entry structure

   \return
     - IFX_SUCCESS
*/
static IFX_return_t TAPI_TM_lockless_event_stop(struct TAPI_TM_Context *context,
                    struct list_entry *entry_to_stop)
{
   struct list_entry *current_entry;

   if (is_list_empty(&context->timeout_list))
   {
      return IFX_SUCCESS;
   }
   foreach_list_entry(&context->timeout_list, current_entry)
   {
      if (current_entry == entry_to_stop)
      {
         TAPI_TM_list_entry_remove(&context->timeout_list, current_entry);
         return IFX_SUCCESS;
      }
   }
   return IFX_SUCCESS;
}

/**
   Remove entry from the list.

   \param  list      pointer to list structure
   \param  entry     pointer to entry structure that will be removed
*/
static void TAPI_TM_list_entry_remove(struct list *list,
             struct list_entry *entry)
{
   entry->next->prev = entry->prev;
   entry->prev->next = entry->next;
   entry->prev = IFX_NULL;
   entry->next = IFX_NULL;
   if (list->first_element.next == entry)
   {
      list->first_element.next = &list->first_element;
   }
}

/** Get next timeouted event

   \param[in]  context              timer context pointer
   \param[out] timeout              Returns timeout descriptor
   \param[out] time_to_next_entry   time till the next timeout expires,
                                    set only if no entry was found

   \return first entry on the list for which timeout expired or IFX_NULL
*/
static struct list_entry *TAPI_TM_next_active_event_get(struct TAPI_TM_Context *context,
                    struct timeout *timeout,
                    time_t * time_to_next_entry)
{
   struct timeout_list_entry *first_entry;
   time_t currentTime;

   if (is_list_empty(&context->timeout_list)) {
      return IFX_NULL;
   }

   /* get the first entry from the list,
      should be the one for which timeout expires first */
   first_entry = list_entry_data(context->timeout_list.first_element.next);
   if (!first_entry)
   {
      return IFX_NULL;
   }
   currentTime = (time_t)IFXOS_ElapsedTimeMSecGet(0);
   if (first_entry->timeout_time <= currentTime)
   {
      /* timeout occured for this entry */
      *timeout = first_entry->timeout;
      return context->timeout_list.first_element.next;
   }
   else
   {
      /* get time to next timeout */
      *time_to_next_entry = first_entry->timeout_time - currentTime;
   }
   return IFX_NULL;
}

/** Stop handling of given entry

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to stop

   \return always IFX_SUCCESS as TAPI_TM_lockless_event_stop does
*/
static IFX_return_t TAPI_TM_timeout_event_stop(struct TAPI_TM_Context *context,
                 struct list_entry *timer_entry)
{
   IFX_return_t error;
   struct timeout_list_entry *timeout_entry_to_start;

   TAPI_OS_LockGet(&context->timeout_list.lock);
   timeout_entry_to_start = list_entry_data(timer_entry);
   timeout_entry_to_start->timeout.bPeriodical = IFX_FALSE;
   error = TAPI_TM_lockless_event_stop(context, timer_entry);
   TAPI_OS_LockRelease(&context->timeout_list.lock);

   return error;
}

/** Remove given entry from the list

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to remove

   \return always IFX_SUCCESS as TAPI_TM_lockless_event_remove does
*/
static IFX_return_t TAPI_TM_timeout_event_remove(struct TAPI_TM_Context *context,
                 struct list_entry *timer_entry)
{
   IFX_return_t error;

   TAPI_OS_LockGet(&context->timeout_list.lock);
   error = TAPI_TM_lockless_event_remove(context, timer_entry);
   TAPI_OS_LockRelease(&context->timeout_list.lock);

   return error;
}

/** Lockless version of TAPI_TM_timeout_event_remove

   \param[in]  context           timer context pointer
   \param[in]  entry_to_remove   entry to remove

   \return always IFX_SUCCESS
 */
static IFX_return_t TAPI_TM_lockless_event_remove(struct TAPI_TM_Context *context,
                    struct list_entry *entry_to_remove)
{
   struct list_entry *current_entry;

   if (is_list_empty(&context->timeout_list))
   {
      return IFX_SUCCESS;
   }
   foreach_list_entry(&context->timeout_list, current_entry)
   {
      if (current_entry == entry_to_remove)
      {
         TAPI_TM_list_entry_free(&context->timeout_list, current_entry);
         return IFX_SUCCESS;
      }
   }
   return IFX_SUCCESS;
}

/** Release memory allocated for given entry

   \param[in]  list     pointeer to the list - unused
   \param[in]  entry    entry to remove and free memory
 */
static void TAPI_TM_list_entry_free(struct list *list,
             struct list_entry *entry)
{
   IFX_UNUSED(list);

   if ((IFX_NULL != entry->next) && (IFX_NULL != entry->prev))
   {
      entry->next->prev = entry->prev;
      entry->prev->next = entry->next;
   }
   TAPI_OS_Free(entry);
}

/** Release memory allocated for given entry

   \param[in]  list           pointeer to the list
   \param[in]  payload_size   size of memory in bytes needed to hold the entry data

   \return value TAPI_OS_LockInit() call
 */
static IFX_return_t TAPI_TM_list_init(struct list *list, size_t payload_size)
{
   /* for first element next and prev point to first element if list is empty */
   list->first_element.next = &list->first_element;
   list->first_element.prev = &list->first_element;
   list->payload_size = payload_size;
   /* get the lock */
   return TAPI_OS_LockInit(&list->lock);
}

/** Start handling of given timeout entry

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to start
   \param[in]  timeout_time   timoeout time for new timer entry
   \param[in]  bPeriodical    if IFX_TRUE, then timeout for event will be
                              checked periodicaly to execute the handler

   \return always IFX_SUCCESS as TAPI_TM_event_entry_start does
*/
static IFX_return_t TAPI_TM_timeout_event_start(struct TAPI_TM_Context *context,
              struct list_entry *timer_entry,
              time_t timeout_time, IFX_boolean_t bPeriodical)
{
   /** \todo add a check if timers are initialized */
   return TAPI_TM_event_entry_start(context, timer_entry, timeout_time, bPeriodical);
}

/** Start handling of given timeout entry

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to start
   \param[in]  timeout_time   timoeout time for new timer entry
   \param[in]  bPeriodical    if IFX_TRUE, then timeout for event will be
                              checked periodicaly to execute the handler

   \return always IFX_SUCCESS
*/
static IFX_return_t TAPI_TM_event_entry_start(struct TAPI_TM_Context *context,
                   struct list_entry *timer_entry,
                   time_t timeout_time,
                   IFX_boolean_t bPeriodical)
{
   struct timeout_list_entry *timeout_entry_to_start;

   TAPI_OS_LockGet(&context->timeout_list.lock);
   timeout_entry_to_start = list_entry_data(timer_entry);
   timeout_entry_to_start->timeout.bPeriodical = bPeriodical;
   TAPI_TM_lockless_event_entry_start(context, timer_entry, timeout_time);
   TAPI_OS_LockRelease(&context->timeout_list.lock);

   return IFX_SUCCESS;
}

/** Add timeout event

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to start
   \param[in]  timeout_time   timoeout time for new timer entry (in ms)

   \return always IFX_SUCCESS
*/
static IFX_return_t TAPI_TM_lockless_event_entry_start(struct TAPI_TM_Context *context,
                   struct list_entry *timer_entry,
                   time_t timeout_time)
{
   struct list_entry *current_entry;
   struct timeout_list_entry *timeout_entry, *timeout_entry_to_start;
   IFX_boolean_t added = IFX_FALSE;

   timeout_entry_to_start = list_entry_data(timer_entry);
   /* get timout time to compare with current time read
      with IFXOS_ElapsedTimeMSecGet(0) */
   timeout_entry_to_start->timeout_time = IFXOS_ElapsedTimeMSecGet(0) + timeout_time;
   timeout_entry_to_start->time_in = timeout_time;
   foreach_list_entry(&context->timeout_list, current_entry)
   {
      timeout_entry = list_entry_data(current_entry);
      if (timeout_entry->timeout_time >
          timeout_entry_to_start->timeout_time)
      {
         TAPI_TM_list_entry_add_before(&context->timeout_list, current_entry,
                     timer_entry);
         added = IFX_TRUE;
         break;
      }
   }
   if (!added)
   {
      TAPI_TM_list_entry_add_tail(&context->timeout_list, timer_entry);
   }
   return IFX_SUCCESS;
}

/** Add add new entry before entry that is already on the list

   \param[in]  list           the list - unused
   \param[in]  entry          entry from the list
   \param[in]  new_entry      entry to add
*/
static void TAPI_TM_list_entry_add_before(struct list *list,
            struct list_entry *entry,
            struct list_entry *new_entry)
{
   IFX_UNUSED(list);

   entry->prev->next = new_entry;
   new_entry->prev = entry->prev;
   new_entry->next = entry;
   entry->prev = new_entry;
}

/** Add add new entry at the end of the list

   \param[in]  list           the list
   \param[in]  entry          entry to add
*/
static void TAPI_TM_list_entry_add_tail(struct list *list,
                   struct list_entry *new_entry)
{
   TAPI_TM_list_entry_add_after(list, list->first_element.prev, new_entry);
}

/** Add add new entry after entry that is already on the list

   \param[in]  list           the list - unused
   \param[in]  entry          entry from the list
   \param[in]  new_entry      entry to add
*/
static void TAPI_TM_list_entry_add_after(struct list *list,
           struct list_entry *entry,
           struct list_entry *new_entry)
{
   IFX_UNUSED(list);

   entry->next->prev = new_entry;
   new_entry->next = entry->next;
   entry->next = new_entry;
   new_entry->prev = entry;
}
#endif /* __KERNEL__ */

/**
   Create a timer.

   \param  pTimerEntry  Function pointer to the call back function.
   \param  nArgument    Pointer to TAPI channel structure.

   \return
   Timer_ID  Pointer to internal timer structure.

   \remarks
   Initialize a task queue which will be scheduled once a timer interrupt occurs
   to execute the appropriate operation in a process context, process in which
   semaphores ... are allowed.
   Please notice that this task has to run under the keventd process, in which
   it can be executed thousands of times within a single timer tick.
*/
Timer_ID TAPI_Create_Timer(TIMER_ENTRY pTimerEntry, IFX_ulong_t nArgument)
{
#ifdef __KERNEL__
   struct Timer_ID_s *pTimerData;

   /* allocate memory for the timer data structure */
   pTimerData = kmalloc(sizeof (*pTimerData), GFP_KERNEL);
   if (pTimerData == IFX_NULL)
      return IFX_NULL;

   /* set function to be called after timer expires */
   pTimerData->pTimerEntry = pTimerEntry;
   pTimerData->nArgument = nArgument;
   pTimerData->bStopped = IFX_FALSE;

   init_timer(&(pTimerData->Timer_List));

   /* set timer call back function */
   pTimerData->Timer_List.function = TAPI_timer_call_back;
   pTimerData->Timer_List.data = (IFX_ulong_t) pTimerData;

   /* Initialize Timer Task */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
   /* initialize tq_struct member of Timer_ID structure */
   memset(&(pTimerData->timerTask), 0, sizeof (struct tq_struct));
   pTimerData->timerTask.routine = TAPI_tqueue;
   pTimerData->timerTask.data = (IFX_void_t *) pTimerData;
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
   INIT_WORK(&(pTimerData->timerTask), TAPI_tqueue, (IFX_void_t *) pTimerData);
#else
   INIT_WORK(&(pTimerData->timerTask), TAPI_tqueue);
#endif /* LINUX_VERSION_CODE */

   return (Timer_ID)pTimerData;
#else /* ! __KERNEL__ */
   if (IFX_TRUE != G_timers.bTimersInialized)
   {
      /* timeout thread needs to be started only once */
      G_timers.bTimersInialized = IFX_TRUE;
      TAPI_TM_timeout_init(&G_timers);
   }
   return (Timer_ID) TAPI_TM_timeout_event_create(&G_timers, pTimerEntry, nArgument);
#endif /* __KERNEL__ */
}

/**
   Sets a timer to the specified time and starts it. It can be choose if the
   timer starts periodically.

   \param  Timer_ID     Pointer to internal timer structure.
   \param  nTime        Time in ms.
   \param  bPeriodically Starts the timer periodically or not.
   \param  bRestart     Restart the timer or normal start.

   \return
   Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_SetTime_Timer(Timer_ID Timer,
                                 IFX_uint32_t nTime,
                                 IFX_boolean_t bPeriodically,
                                 IFX_boolean_t bRestart)
{
#ifdef __KERNEL__
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)Timer;

   if (pTimerData == IFX_NULL)
      return IFX_FALSE;

   pTimerData->Periodical_Time = HZ*nTime/1000;
   pTimerData->bPeriodical = bPeriodically;
   pTimerData->bStopped = IFX_FALSE;

   if (timer_pending(&(pTimerData->Timer_List)))
   {
      if (IFX_FALSE == bRestart)
         return IFX_TRUE;

      if (mod_timer(&(pTimerData->Timer_List),
                    jiffies + pTimerData->Periodical_Time))
         return IFX_TRUE;
   }

   pTimerData->Timer_List.expires = jiffies + pTimerData->Periodical_Time;
   add_timer(&(pTimerData->Timer_List));
#else /* !__KERNEL__ */
   /** \todo add a check if timers are initialized */
   if (bRestart == IFX_TRUE)
   {
      (void) TAPI_TM_timeout_event_stop(&G_timers, (struct list_entry *) Timer);
   }

   if (TAPI_TM_timeout_event_start(&G_timers,
              (struct list_entry *) Timer,
              (time_t) nTime,
              bPeriodically) != 0)
   {
      TRACE( TAPI_DRV, DBG_LEVEL_HIGH, ("TAPI_SetTime_Timer: failed to start timer\n"));
      return IFX_FALSE;
   }
#endif /* __KERNEL__ */

   return IFX_TRUE;
}

/**
   Stop a timer.

   \param  Timer_ID     Pointer to internal timer structure.

   \return
   Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_Stop_Timer(Timer_ID Timer)
{
#ifdef __KERNEL__
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)Timer;

   if (pTimerData != IFX_NULL)
   {
      /* stop timer */
      pTimerData->bStopped = IFX_TRUE;
      /* prevent restart of driver */
      pTimerData->bPeriodical = IFX_FALSE;

      if (timer_pending(&(pTimerData->Timer_List)))
      {
         /* remove driver from list */
         del_timer_sync(&(pTimerData->Timer_List));
      }
   }
#else /* ! __KERNEL__ */
   if(Timer)
      (void) TAPI_TM_timeout_event_stop(&G_timers, (struct list_entry *) Timer);
#endif /* __KERNEL__ */

   return IFX_TRUE;
}

/**
   Delete a timer.

   \param  Timer_ID     Pointer to internal timer structure.

   \return
   Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_Delete_Timer(Timer_ID Timer)
{
   if (Timer == IFX_NULL)
      return IFX_FALSE;

   TAPI_Stop_Timer(Timer);

   /* free memory */
#ifdef __KERNEL__
   kfree(Timer);
#else
   (void) TAPI_TM_timeout_event_remove(&G_timers, (struct list_entry *)Timer);
#endif /* __KERNEL__ */

   return IFX_TRUE;
}
#endif /* TAPI_HAVE_TIMERS */

#ifdef __KERNEL__
/**
   Helper function to get a periodical timer.

   \param  pWork        Pointer to corresponding timer ID.

   \remarks
   This function will be executed in  the process context, so to avoid
   scheduling in Interrupt Mode while working with semaphores etc...
   The task is always running under the keventd process and is also running
   very quickly. Even on a very heavily loaded system, the latency in the
   scheduler queue is quite small
*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
static IFX_void_t TAPI_tqueue (IFX_void_t *pWork)
#else /* for Kernel newer or equal 2.6.20 */
static IFX_void_t TAPI_tqueue (struct work_struct *pWork)
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) */
{
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)pWork;

   if (pTimerData->bStopped)
      return;

   /* Call TAPI Timer function */
   pTimerData->pTimerEntry(pTimerData, pTimerData->nArgument);
   if (pTimerData->bPeriodical)
   {
      if (timer_pending(&(pTimerData->Timer_List)))
      {
         /* update existed timer */
         if (mod_timer(&(pTimerData->Timer_List),
                       jiffies + pTimerData->Periodical_Time))
            return;
      }

      /* create new timer */
      pTimerData->Timer_List.expires = jiffies + pTimerData->Periodical_Time;
      add_timer(&(pTimerData->Timer_List));
   }
}

#ifdef __KERNEL__
/**
   Helper function to get a periodical timer.

   \param  arg          Pointer to corresponding timer ID.
*/
static IFX_void_t TAPI_timer_call_back (IFX_ulong_t arg)
{
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *) arg;
   /* do the operation in process context,
      not in interrupt context */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
   schedule_task (&(pTimerData->timerTask));
#else
   queue_work (pTAPItimersWq, &(pTimerData->timerTask));
#endif /* LINUX_VERSION_CODE */
}
#endif /* __KERNEL__ */


/* ============================= */
/* Defer work to process context */
/* ============================= */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
/* High Level Event Dispatcher function. */
static IFX_void_t Deferred_Worker (IFX_void_t *pWork)
{
   IFX_TAPI_EXT_EVENT_PARAM_t *pEvParam = (IFX_TAPI_EXT_EVENT_PARAM_t *) pWork;
   pEvParam->pFunc(pEvParam);
}
#else /* for Kernel newer or equal 2.6.20 */
static IFX_void_t Deferred_Worker (struct work_struct *pWork)
{
   IFX_TAPI_EXT_EVENT_PARAM_t *pEvParam = (IFX_TAPI_EXT_EVENT_PARAM_t *) pWork;
   pEvParam->pFunc(pEvParam);
}
#endif

/**
   Defer work to process context

   \param  pFunc        Pointer to function to be called.
   \param  pParam       Parameter passed to the function.
                        Attention: this must be a valid structure of
                        type IFX_TAPI_EXT_EVENT_PARAM_t as we need to
                        do some casting for Linux.

   \return TAPI_statusOk or TAPI_statusErr in case of an error.
*/
IFX_int32_t TAPI_DeferWork (IFX_void_t *pFunc, IFX_void_t *pParam)
{
   IFX_int32_t ret = TAPI_statusOk;
   IFX_TAPI_EXT_EVENT_PARAM_t *pEvParam = (IFX_TAPI_EXT_EVENT_PARAM_t *) pParam;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
   struct tq_struct           *pTapiTq;

   pTapiTq           = (struct tq_struct *)&pEvParam->tapiTq;
   INIT_TQUEUE(pTapiTq, Deferred_Worker, (IFX_void_t *)pEvParam);
   pEvParam->pFunc   = (IFX_void_t *)pFunc;
   if (schedule_task (pTapiTq) == 0)
   {
      ret = TAPI_statusWorkFail;
   }
#else
   struct work_struct         *pTapiWs;

   pTapiWs = (struct work_struct *) &pEvParam->tapiWs;
   pEvParam->pFunc   = (IFX_void_t *)pFunc;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
   INIT_WORK(pTapiWs, Deferred_Worker, (IFX_void_t *) pEvParam);
#else
   INIT_WORK(pTapiWs, Deferred_Worker);
#endif

   if (queue_work (pTAPIeventsWq, pTapiWs) == 0)
   {
      ret = TAPI_statusWorkFail;
   }
#endif /* LINUX_VERSION_CODE */
   return ret;
}



/**
   Modify own thread priority.

   \param  newPriority  New thread priority.

   \return
   - IFX_SUCCESS priority changed.
   - IFX_ERROR priority not changed.
*/
IFX_int32_t TAPI_OS_ThreadPriorityModify(IFX_uint32_t newPriority)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#if 0
   ret = setscheduler(current->pid, SCHED_FIFO, &sched_params);
#else /* 0 */
   current->policy = SCHED_FIFO;
   current->rt_priority = newPriority;
#endif /* 0 */
#else
   struct sched_param sched_params;
   IFX_int32_t ret = 0;

   sched_params.sched_priority = newPriority;

   ret = sched_setscheduler(current, SCHED_FIFO, &sched_params);

   if (ret < 0)
   {
      printk(KERN_ERR "Failed to set the thread priority to %d"
             " ret=%d\n", newPriority, ret);

      return IFX_ERROR;
   }
#endif

   return IFX_SUCCESS;
}


/**
   stop a kernel thread. Called by the removing instance

   \param  pThrCntrl    Pointer to a thread control struct.
   \param  pMutex       Pointer to mutex to unblock the thread to be killed.
*/
IFX_void_t TAPI_OS_ThreadKill(IFXOS_ThreadCtrl_t *pThrCntrl,
                              TAPI_OS_lock_t *pLock)
{
   if ((pThrCntrl) &&
       (IFXOS_THREAD_INIT_VALID(pThrCntrl) == IFX_TRUE) &&
       (pThrCntrl->thrParams.bRunning == 1))
   {
      /* signal the thread routine to shutdown */
      pThrCntrl->thrParams.bShutDown = IFX_TRUE;
      mb();

      /* Wake the process so that is able to see the shutdown flag and
         terminate itself. */
      if (pLock != IFX_NULL)
         TAPI_OS_LockRelease(pLock);

      wait_for_completion (&pThrCntrl->thrCompletion);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
      /* Now that we are sure the thread is in zombie state.
         We notify keventd to clean the process up. */
      kill_proc(2, SIGCHLD, 1);
#endif /* kernel 2.6.23 */
      pThrCntrl->bValid = IFX_FALSE;
   }
}



#ifndef RTK_KTAPI
IFX_int_t kTAPI_ModuleCreate(void)
{
   /* copyright trace shall not be prefixed with KERN_INFO */
   printk("%s, (c) 2001-2016 Lantiq Beteiligungs-GmbH & Co.KG\n", &TAPI_WHATVERSION[4]);

   if (IFX_TAPI_Driver_Start() != IFX_SUCCESS)
   {
      printk (KERN_ERR "Driver start failed\n");
      return -1;
   }


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
#  if (LINUX_VERSION_CODE > KERNEL_VERSION(3,8,0))
   /*
     For "Concurrency Managed Workqueue" use related flags to increase the
     priority.

     WQ_MEM_RECLAIM

      All wq which might be used in the memory reclaim paths _MUST_
      have this flag set.  The wq is guaranteed to have at least one
      execution context regardless of memory pressure.

     WQ_HIGHPRI

      Work items of a highpri wq are queued to the highpri
      thread-pool of the target gcwq.  Highpri thread-pools are
      served by worker threads with elevated nice level.

      Note that normal and highpri thread-pools don't interact with
      each other.  Each maintain its separate pool of workers and
      implements concurrency management among its workers.
   */
   pTAPItimersWq = alloc_workqueue("TAPItimers", WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
   pTAPIeventsWq = alloc_workqueue("TAPIevents",
      WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
#  else
   pTAPItimersWq = create_workqueue("TAPItimers");
   pTAPIeventsWq = create_workqueue("TAPIevents");
   TAPI_DeferWork((IFX_void_t *) tapi_wq_setscheduler,
                  (IFX_void_t *)&tapi_wq_setscheduler_param);
#  endif
#endif /* LINUX_VERSION_CODE */

   return 0;
}

/**
   Remove the TAPI Control Driver .

\return
   IFX_SUCCESS successful, all freed and driver removed
   IFX_ERROR   not successful, devices still busy
*/
IFX_int_t kTAPI_ModuleDelete(void)
{

   int i;

   printk (KERN_INFO "Removing Highlevel TAPI module\n");

   /* Actually all LL drivers should have unregistered here. Being careful
      we force unregister of any drivers which may still be registered. */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx [i].pDrvCtx != IFX_NULL)
      {
         IFX_TAPI_Unregister_LL_Drv (gHLDrvCtx [i].pDrvCtx->majorNumber);
      }
   }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
   /* as we are using work queues to schedule events from the interrupt
      context to the process context, we use work queues in case of
      Linux 2.6. they must be flushed on driver unload... */
   flush_workqueue(pTAPItimersWq);
   destroy_workqueue(pTAPItimersWq);
   flush_workqueue(pTAPIeventsWq);
   destroy_workqueue(pTAPIeventsWq);
#endif /* LINUX_VERSION_CODE */

   IFX_TAPI_Driver_Stop();

   TRACE(TAPI_DRV, DBG_LEVEL_NORMAL, ("TAPI_DRV: cleanup successful\n"));

   return IFX_SUCCESS;
}

#ifndef CONFIG_RTK_VOIP_MODULE_BUILD
module_init (kTAPI_ModuleCreate);
module_exit (kTAPI_ModuleDelete);
#endif

#endif /* RTK_KTAPI */

#endif /* __KERNEL__ */

#endif /* LINUX */

/*lint -restore*/
