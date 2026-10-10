#ifndef _DRV_OS_H
#define _DRV_OS_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_os.h
   This file contains operating system specific defines and includes like IFXOS
   as well as endianess related macros.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_dxs_config.h"
#include "ifx_types.h"           /* ifx type definitions       */

#ifdef GREENHILLS_CHECK
   #include "drv_dxs_ghs.h"
#endif /* GREENHILLS_CHECK */

/* Major and minor device number for OS registration. */
#define DXS_MAJOR            125
#define DXS_MINOR_BASE       10

#ifndef HAVE_IFX_ULONG_T
   #warning please update your ifx_types.h, using local definition of IFX_ulong_t
   /* unsigned long type - valid for 32bit systems only */
   typedef unsigned long               IFX_ulong_t;
   #define HAVE_IFX_ULONG_T
#endif /* HAVE_IFX_ULONG_T */

#ifndef HAVE_IFX_LONG_T
   #warning please update your ifx_types.h, using local definition of IFX_long_t
   /* long type - valid for 32bit systems only */
   typedef long                        IFX_long_t;
   #define HAVE_IFX_LONG_T
#endif /* HAVE_IFX_LONG_T */

#ifndef HAVE_IFX_UINTPTR_T
   #warning please update your ifx_types.h, using local definition of IFX_uintptr_t
   typedef IFX_ulong_t                 IFX_uintptr_t;
   #define HAVE_IFX_UINTPTR_T
#endif /* HAVE_IFX_UINTPTR_T */

#ifndef HAVE_IFX_INTPTR_T
   #warning please update your ifx_types.h, using local definition of IFX_intptr_t
   typedef IFX_long_t                  IFX_intptr_t;
   #define HAVE_IFX_INTPTR_T
#endif /* HAVE_IFX_INTPTR_T */

#ifndef HAVE_IFX_SIZE_T
   #warning please update your ifx_types.h, using local definition of IFX_size_t
   typedef IFX_ulong_t                 IFX_size_t;
   #define HAVE_IFX_SIZE_T
#endif /* HAVE_IFX_SIZE_T */


/* this included file is supposed to support following OS:
   - LINUX
   - VXWORKS
   - NO_OS
   - WINDOWS
*/
#ifdef DXS_HAVE_INTERRUPTS
#include "ifxos_interrupt.h"
#endif /* DXS_HAVE_INTERRUPTS */
#include "ifxos_copy_user_space.h"
#include "ifxos_memory_alloc.h"
#include "ifxos_debug.h"
#include "ifxos_event.h"
#include "ifxos_select.h"
#include "ifxos_mutex.h"
#include "ifxos_lock.h"
#include "ifxos_time.h"
#include "ifxos_thread.h"
#ifndef __KERNEL__
#include "ifxos_std_defs.h"
#endif
#ifdef DXS_USE_DEV_IO
#include "ifxos_device_access.h"
#endif /* DXS_USE_DEV_IO */
#ifndef __KERNEL__
#include <netinet/in.h> /* ntohs(), etc. */
#endif
#ifdef DXS_LINUX_KERNEL_SPACE
#include <asm/io.h> /* memset(), memcpy() */
#include <linux/interrupt.h>  /* in_interrupt(), irqreturn_t, disable_irq */
/*#else / * DXS_LINUX_KERNEL_SPACE * /
#include <string.h> *//* memset() */
#endif /* DXS_LINUX_KERNEL_SPACE */


/* Endian defines are derived from the OS configuration. */
#define DXS_BYTE_ORDER                 IFXOS_BYTE_ORDER
#define DXS_LITTLE_ENDIAN              IFXOS_LITTLE_ENDIAN
#define DXS_BIG_ENDIAN                 IFXOS_BIG_ENDIAN


#define DBG_LEVEL_OFF      IFXOS_PRN_LEVEL_OFF
#define DBG_LEVEL_HIGH     IFXOS_PRN_LEVEL_HIGH
#define DBG_LEVEL_NORMAL   IFXOS_PRN_LEVEL_NORMAL
#define DBG_LEVEL_LOW      IFXOS_PRN_LEVEL_LOW

#define CREATE_TRACE_GROUP(name) \
               IFXOS_PRN_USR_MODULE_CREATE (name, DBG_LEVEL_HIGH)
#define DECLARE_TRACE_GROUP(name) \
               IFXOS_PRN_USR_MODULE_DECL (name)

#ifdef ENABLE_TRACE
   #define TRACE(name, dbg_level, print_message) \
               IFXOS_PRN_USR_DBG_NL (name, dbg_level, print_message)
   #define TRACE_LEVEL_SET(name, new_level) \
               IFXOS_PRN_USR_LEVEL_SET (name, new_level)
#else
   #define TRACE(name, dbg_level, print_message)
   #define TRACE_LEVEL_SET(name, new_level)
#endif /* ENABLE_TRACE */

#ifdef DEBUG
#define DXS_ASSERT(expr) \
   do { \
      /*lint -e(506, 774) */ \
      if(!(expr)) { \
         (void) IFXOS_DBG_PRINT_USR ("\n\r" __FILE__ ":%d: Assertion %s " \
                                     "failed!\n\r",__LINE__, #expr); \
      } \
   } while (0)
#else /* DEBUG */
/** assert in debug code
\param expr - expression to be evaluated. If expr != TRUE assert is printed
              out with line number */
#define DXS_ASSERT(expr)
#endif /* DEBUG */



#ifdef DXS_LINUX_KERNEL_SPACE
/* Linux Includes*/
#include <linux/timer.h>
#include <linux/workqueue.h>
#endif

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* define prototypes for VxWorks 5 */
#if (defined(VXWORKS)) && (!defined(__PROTOTYPE_5_0))
   #define __PROTOTYPE_5_0
#endif /* #if (defined(VXWORKS)) && (!defined(__PROTOTYPE_5_0)) */

/* define to enable the shared interrupt handling */
#if defined(VXWORKS) || defined(LINUX)
   #define DXS_SHARED_INTERRUPT
#endif /* defined(VXWORKS) || defined(LINUX) */

/**
   Macro to map the system function with takes care of interrupt handling
   registration.

   \param irq  irq number

   \param func interrupt handler callback function

   \param arg  argument of interrupt handler callback function

   \remarks
   The macro is by default mapped to the operating system method. For systems
   integrating different routines, this macro must be adapted in the user con-
   figuration header file.
*/
#ifndef DXS_SYS_REGISTER_INT_HANDLER
   #ifdef LINUX
     #ifndef IRQF_DISABLED
       #define IRQF_DISABLED   SA_INTERRUPT
     #endif /* IRQF_DISABLED */
     #ifndef IRQF_SHARED
       #define IRQF_SHARED     SA_SHIRQ
     #endif /* IRQF_SHARED */
     #define DXS_SYS_REGISTER_INT_HANDLER(irq,flags,func,arg)                  \
              request_irq ((irq),(func),(flags),DEV_NAME,(IFX_void_t*)(arg))
     #define DXS_SYS_REGISTER_INT_HANDLER_THREAD(irq,flags,fun_irq,fun_th,arg) \
              request_threaded_irq((irq), (fun_irq), (fun_th), (flags),        \
              DEV_NAME, (IFX_void_t*)arg);
   #endif /* LINUX */
   #ifdef VXWORKS
     #define DXS_SYS_REGISTER_INT_HANDLER(irq,func,arg)                   \
                 sysGpioIntConnect(((pDev)->nDevNr == 1) ?                \
                    DXS_EXINT_DEV1 : DXS_EXINT_DEV0, (VOIDFUNCPTR)(func), \
                    (IFX_int32_t)(arg))
   #endif /* VXWORKS */
#endif /* DXS_SYS_REGISTER_INT_HANDLER */

#ifdef DXS_HAVE_INTERRUPTS
/**
   Macro to map the system function which takes care of unregistration of
   the interrupt handler.
\param
   irq    irq number
\remarks
   The macro is by default mapped to the operating system method. For systems
   integrating different routines, this macro must be adapted in the user con-
   figuration header file.
*/
#ifndef DXS_SYS_UNREGISTER_INT_HANDLER
   #ifdef LINUX
     #define DXS_SYS_UNREGISTER_INT_HANDLER(irq,arg)  \
              free_irq((irq), (arg))
   #endif /* LINUX */
   #ifdef VXWORKS
     #define DXS_SYS_UNREGISTER_INT_HANDLER(irq)  \
              DXS_SYS_REGISTER_INT_HANDLER((irq), OS_IRQHandler_Dummy, (irq))
   #endif /* VXWORKS */
#endif /* DXS_SYS_UNREGISTER_INT_HANDLER */

/**
   Disables the interrupt line

   \param irq  irq number

   \remarks
   This macro is system specific. In case the Operating system methods can not
   be used for this purpose, define the correct macro in your user configuration
   file.
*/
#ifndef DXS_DISABLE_IRQLINE
   #ifdef LINUX
   #define DXS_DISABLE_IRQLINE(irq)       disable_irq_nosync(irq)
   #else
   extern int sysIntDisableICU(int);
   #define DXS_DISABLE_IRQLINE(irq) \
if (irq == 252) \
   {\
      if (--pInterruptCounters[0] == 0) \
      { \
         sysIntDisableICU(irq); \
      } \
   }\
   else \
   { \
      if (--pInterruptCounters[1] == 0) \
      { \
         sysIntDisableICU(irq); \
      } \
   }

   #endif /* LINUX */
#endif

/**
   Enables the interrupt line

   \param irq  irq number

   \remarks
   This macro is system specific. In case the Operating system methods can not
   be used for this purpose, define the correct macro in your user configuration
   file.
*/
#ifndef DXS_ENABLE_IRQLINE
#ifdef VXWORKS
extern int sysIntEnableICU(int);
#define DXS_ENABLE_IRQLINE(irq) \
if (irq == 252) \
      {\
         if (pInterruptCounters[0]++ == 0) \
         { \
            sysIntEnableICU(irq); \
         } \
      }\
      else \
      { \
         if (pInterruptCounters[1]++ == 0) \
         { \
            sysIntEnableICU(irq); \
         } \
      }
#else
#define DXS_ENABLE_IRQLINE(irq)        IFXOS_IRQ_ENABLE(irq)
#endif
#endif

/**
   Disables the global interrupt

   \param var  local mask that has to be stored for DXS_ENABLE_IRQGLOBAL

   \remarks
   This macro is system specific. In case the Operating system methods can not
   be used for this purpose, define the correct macro in your user configuration
   file.
*/
#ifndef DXS_DISABLE_IRQGLOBAL
#define DXS_DISABLE_IRQGLOBAL(var)     IFXOS_LOCKINT(var)
#endif

/**
   Enables the global interrupt

   \param var  local mask that has to be stored for DXS_ENABLE_IRQGLOBAL

   \remarks
   This macro is system specific. In case the Operating system methods can not
   be used for this purpose, define the correct macro in your user configuration
   file.
   This macro is used when no device specific interrupt handler or interrupt
   line is configured (nIrg < 0).
*/
#ifndef DXS_ENABLE_IRQGLOBAL
#define DXS_ENABLE_IRQGLOBAL(var)      IFXOS_UNLOCKINT(var)
#endif

#ifndef DXS_OS_INTSTAT
#define DXS_OS_INTSTAT                 IFXOS_INTSTAT
#endif
#else /* DXS_HAVE_INTERRUPTS */
#ifndef DXS_OS_INTSTAT
#define DXS_OS_INTSTAT                 IFX_int32_t
#endif
#ifndef DXS_ENABLE_IRQLINE
#define DXS_ENABLE_IRQLINE(irq)
#endif
#ifndef DXS_DISABLE_IRQLINE
#define DXS_DISABLE_IRQLINE(irq)
#endif
#endif /* DXS_HAVE_INTERRUPTS */


/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/** return value of the interrupt service routine telling that ... */
typedef enum {
   /** no interrupt has been handled for this device */
   IFX_IRQ_NONE    = 0,
   /** an interrupt has been handled for this device */
   IFX_IRQ_HANDLED = 1
} IFX_irqreturn_t;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
#ifdef DXS_LINUX_KERNEL_SPACE
extern IFX_void_t* OS_MapBuffer(IFX_void_t* p_buffer, IFX_size_t size);
extern IFX_void_t OS_UnmapBuffer(IFX_void_t* p_buffer);
#else /* DXS_LINUX_KERNEL_SPACE */
#define OS_MapBuffer(p_buffer,size)    (IFX_void_t*)(p_buffer)
#define OS_UnmapBuffer(p_buffer)       /* nothing */
#endif /* DXS_LINUX_KERNEL_SPACE */
/*
   Function map - IFXOS Printout and debug.
*/

/** mark variable as unused, to suppress compilation warrnings

   \remark: (ANSI X3.159-1989)
      void is used, in any context where the value of an expression
      is to be discarded, to indicate explicitly that a value is
      ignored by writing the cast (void).
*/
#define DXS_OS_UNUSED(var) ((IFX_void_t)(var))

/** Define the used CR/LF sequence */
#define DXS_CRLF                         IFXOS_CRLF

/*
   Mapping table - Dynamic memory handling.
*/
#define DXS_OS_MALLOC                    IFXOS_BlockAlloc
#define DXS_OS_FREE                      IFXOS_BlockFree

/*
   Function map - Driver User Space data exchange.
*/
#if 1 //def DXS_LINUX_USER_SPACE
/* By default ifxos 1.4.17 library is compiled for kernel space
   and the user space implementation of IFXOS_CpyToUser and IFXOS_CpyFromUser
   is not available. With the workaround below driver can run in user space
   without recompiling the ifxos library. */
#define DXS_OS_CpyKern2Usr(pTo,pFrom,size) \
      memcpy(pTo,pFrom,size)
#define DXS_OS_CpyUsr2Kern(pTo,pFrom,size) \
      memcpy(pTo,pFrom,size)
#else /* DXS_LINUX_USER_SPACE */
#define DXS_OS_CpyKern2Usr               IFXOS_CpyToUser
#define DXS_OS_CpyUsr2Kern               IFXOS_CpyFromUser
#endif /* DXS_LINUX_USER_SPACE */


/*
   Function map - Time and Wait Functions and Defines.
*/
#ifdef DXS_LINUX_USER_SPACE
/* IFXOS_USecSleep is missing for user space in IFXOS libray,
   so a separate implementation is needed */
#include <time.h>
#include <sys/time.h>
#define DXS_OS_USecSleep(sleepTime_us) \
do \
{ \
   struct timespec tv; \
   tv.tv_sec = sleepTime_us/1000000; \
   tv.tv_nsec = (long) ((sleepTime_us - (tv.tv_sec * 1000000)) * 1000); \
   while (1) \
   { \
      int rval = nanosleep(&tv, &tv); \
      if (rval == 0) \
      { \
         break; \
      } \
      else \
      { \
         if (errno == EINTR) \
            continue; \
         else \
            break; \
      } \
   } \
} while(0)

#else /* DXS_LINUX_USER_SPACE */
#define DXS_OS_USecSleep                 IFXOS_USecSleep
#endif /* DXS_LINUX_USER_SPACE */

#define DXS_OS_MSecSleep                 IFXOS_MSecSleep
#define IFXOS_GET_TICK()                 IFXOS_ElapsedTimeMSecGet(0)

/*
   Mapping table - Mutex handling.
*/
#define DXS_OS_mutex_t                   IFXOS_mutex_t
#define DXS_OS_MutexInit                 IFXOS_MutexInit
#define DXS_OS_MutexDelete               IFXOS_MutexDelete
#define DXS_OS_MutexGet                  IFXOS_MutexGet
#define DXS_OS_MutexRelease              IFXOS_MutexRelease
#define DXS_OS_SigMutexInit              IFXOS_MutexInit
/* not supported in lib_ifxos */
#ifdef  DXS_LINUX_KERNEL_SPACE
#define DXS_OS_MutexLockInterruptible(pmtx)  \
                          down_interruptible(&((pmtx)->object))
#elif defined (DXS_LINUX_USER_SPACE)
#define DXS_OS_MutexLockInterruptible    DXS_OS_MutexGet
#endif /* DXS_LINUX_KERNEL_SPACE/DXS_LINUX_USER_SPACE */

#ifdef VXWORKS
#define DXS_OS_MutexLockInterruptible(pmtx)  \
                          semTake((pmtx)->object, WAIT_FOREVER)
#endif /* VXWORKS */

/*
   Mapping table - Event signalling.
*/
#define DXS_OS_event_t                   IFXOS_event_t
#define DXS_OS_EventInit                 IFXOS_EventInit
#define DXS_OS_EventWakeUp               IFXOS_EventWakeUp
#define DXS_OS_EventWait                 IFXOS_EventWait
#define DXS_OS_EventDelete(object)       /* no delete for events */

/*
   Mapping table - Thread handling.
*/
#ifdef DXS_LINUX_USER_SPACE
   /** LINUX User Thread - priority - HIGH */
   #define DXS_OS_THREAD_PRIO_HIGH          60
#else /* DXS_LINUX_KERNEL_SPACE */
   #define DXS_OS_THREAD_PRIO_HIGH          IFXOS_THREAD_PRIO_HIGH
#endif /* DXS_LINUX_KERNEL_SPACE */

#define DXS_OS_THREAD_PRIO_HIGHEST       IFXOS_THREAD_PRIO_HIGHEST
#define DXS_OS_ThreadCtrl_t              IFXOS_ThreadCtrl_t
#define DXS_OS_ThreadParams_t            IFXOS_ThreadParams_t
#define DXS_OS_ThreadInit                IFXOS_ThreadInit
#define DXS_OS_THREAD_PRIORITY_MODIFY    DXS_OS_ThreadPriorityModify
#define DXS_OS_THREAD_KILL               DXS_OS_ThreadKill
#define DXS_OS_ThreadDelete              IFXOS_ThreadDelete
#define DXS_OS_ThreadFunction_t          IFXOS_ThreadFunction_t

#ifdef LINUX
#ifdef DXS_LINUX_USER_SPACE
   #define DXS_OS_ThreadPriorityModify IFXOS_ThreadPriorityModify
#else /* DXS_LINUX_USER_SPACE */
   extern IFX_int32_t DXS_OS_ThreadPriorityModify(IFX_uint32_t newPriority);
#endif /* DXS_LINUX_USER_SPACE */
   extern IFX_void_t  DXS_OS_ThreadKill(IFXOS_ThreadCtrl_t *pThrCntrl,
                                        DXS_OS_mutex_t *pMutex);
#endif /* LINUX */

#ifdef VXWORKS
   #ifndef DXS_OS_ThreadPriorityModify
      #define DXS_OS_ThreadPriorityModify(prio) \
            taskPrioritySet(taskIdSelf(), prio)
   #endif /* DXS_OS_ThreadPriorityModify */

   #ifndef DXS_OS_ThreadKill
      #define DXS_OS_ThreadKill(pThrCntrl, pMutex)
   #endif /* DXS_OS_ThreadKill */
#endif /* VXWORKS */

/* reboot notifier */
#ifdef LINUX
extern IFX_void_t     DXS_OS_RebootNotifierRegister(IFX_void_t);
extern IFX_void_t     DXS_OS_RebootNotifierUnRegister(IFX_void_t);
extern IFX_boolean_t  DXS_OS_isRebootOngoing(IFX_void_t);
#else
#define DXS_OS_RebootNotifierRegister()
#define DXS_OS_RebootNotifierUnRegister()
#define DXS_OS_isRebootOngoing()  (IFX_FALSE)
#endif /* LINUX */

#endif /* _DRV_OS_H */
