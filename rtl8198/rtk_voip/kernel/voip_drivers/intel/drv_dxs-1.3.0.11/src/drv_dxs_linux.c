/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_linux.c
   This file contains the implementation of linux specific driver functions.
*/

#ifdef LINUX
/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39))
#include <linux/smp_lock.h>         /* lock_kernel() */
#endif

#ifndef UTS_RELEASE
   /* include for UTS_RELEASE */
   #if   (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
      #include <linux/uts.h>
   #elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33))
      #include <linux/utsrelease.h>
   #else
      #include <generated/utsrelease.h>
   #endif /* LINUX_VERSION_CODE */
#endif /* UTS_RELEASE */

#include "drv_dxs_config.h"
#include "drv_dxs_os.h"
#include "drv_dxs_api.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
   #ifndef DXS_BROKEN_LX_MIPS_BSP_DEVICETREE
      #include <linux/platform_device.h>
      #include <linux/device.h>
      #include <linux/of.h>
   #endif /* DXS_BROKEN_LX_MIPS_BSP_DEVICETREE */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) */
#include <linux/spi/spi.h>
#ifndef CONFIG_RTK_VOIP
#ifdef __LINUX_SPI_H
#include <linux/list.h>
#endif /* __LINUX_SPI_H */
#endif

#include <linux/reboot.h>
#include <linux/notifier.h>

#include "drv_dxs_irq.h"
#include "drv_dxs_init.h"
#include "drv_dxs_alm_priv.h"

#include "drv_tapi_ll_interface.h"

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
IFX_uint16_t major      = 0; /* default for dynamic allocation */
IFX_uint16_t minorBase  = DXS_MINOR_BASE;
IFX_char_t *devName     = "dxs";

#if defined (ENABLE_TRACE) && defined (DXS_LINUX_KERNEL_SPACE)
extern IFX_uint32_t debug_level;
#endif /* defined (ENABLE_TRACE) && defined (DXS_LINUX_KERNEL_SPACE) */
extern IFX_char_t *dcdc_type;

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/*lint -save -e528 -e19 -e546 */
MODULE_DESCRIPTION("DUSLIC XS device driver");
MODULE_AUTHOR("Lantiq Beteiligungs-GmbH & Co.KG");
MODULE_SUPPORTED_DEVICE("DUSLIC XS");
MODULE_LICENSE("Dual BSD/GPL");

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17))
/*lint -save -e826 */
MODULE_PARM(major, "h" );
MODULE_PARM(minorBase, "h" );
MODULE_PARM(devName, "s" );
#ifdef ENABLE_TRACE
MODULE_PARM(debug_level, "i");
#endif /* ENABLE_TRACE */
MODULE_PARM(dcdc_type, "s" );
/*lint -restore */
#else
module_param(major, ushort, 0);
module_param(minorBase, ushort, 0);
module_param(devName, charp, 0);
#ifdef ENABLE_TRACE
module_param(debug_level, uint, 0);
#endif /* ENABLE_TRACE */
module_param(dcdc_type, charp, 0);
#endif /* < 2.6.17 */

MODULE_PARM_DESC(major ,"Device Major number");
MODULE_PARM_DESC(minorBase ,"Minor number of the node for the first device");
MODULE_PARM_DESC(devName ,"Basename of the devicenode");
#ifdef ENABLE_TRACE
MODULE_PARM_DESC(debug_level, "Debug level: 1 (maxium) - 4 (no) trace output");
#endif /* ENABLE_TRACE */
MODULE_PARM_DESC(dcdc_type, "DC/DC HW type ID string");
/*lint restore*/

static volatile IFX_boolean_t reboot_ongoing = IFX_FALSE;

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
#ifndef CONFIG_RTK_VOIP
#ifdef __LINUX_SPI_H
struct DXS_SPI_DEV {
   struct spi_device *spi;
   struct list_head  device_entry;
} __PACKED__;
#endif /* __LINUX_SPI_H */
#endif

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

#ifdef DXS_HAVE_INTERRUPTS
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
/* operating system callback for polling/interrupt */
#if   (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
static IFX_void_t OS_IRQ_Wrapper     (IFX_int32_t irq, IFX_void_t *pDev,
                                      struct pt_regs *regs);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
static irqreturn_t OS_IRQ_Wrapper    (IFX_int32_t irq, IFX_void_t *pDev,
                                      struct pt_regs *regs);
#else
static irqreturn_t OS_IRQ_Wrapper    (IFX_int32_t irq, IFX_void_t *pDev);
#endif /* LINUX_VERSION_CODE */
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
#endif /* DXS_HAVE_INTERRUPTS */

/* operating system proc interface */
#ifdef DXS_USE_PROC
static IFX_int32_t dxs_proc_VersionGet     (IFX_char_t *buf);
static IFX_int32_t dxs_proc_ConfigureGet   (IFX_char_t *buf);
static IFX_int32_t dxs_proc_StatusGet      (IFX_char_t *buf);

#ifdef EVALUATION
static IFX_int32_t dxs_proc_Write (struct file* file,
                                   const IFX_char_t* buffer,
                                   IFX_ulong_t count,
                                   IFX_void_t* data);

static IFX_int32_t dxs_proc_SafeRead (IFX_char_t *page,
                                      IFX_char_t **start,
                                      off_t off,
                                      IFX_int32_t count,
                                      IFX_int32_t *eof,
                                      IFX_void_t *data);

extern IFX_int32_t DXS_EVAL_Proc_InterruptGet (IFX_char_t *buf,
                                               IFX_char_t **start,
                                               off_t offset,
                                               IFX_int32_t count,
                                               IFX_int32_t *eof,
                                               IFX_void_t *data);

extern IFX_int32_t DXS_EVAL_Proc_InterruptPut (struct file *pFile,
                                               const IFX_char_t *buf,
                                               IFX_ulong_t count,
                                               IFX_void_t *data);
#endif /* #ifdef EVALUATION */

/*lint -save -e10 -e49 -e601 */
static IFX_int32_t dxs_proc_Read (IFX_char_t *buf,
                                  IFX_char_t **start,
                                  off_t offset,
                                  IFX_int32_t len,
                                  IFX_int32_t *eof,
                                  IFX_void_t *data);
/*lint -restore */

static IFX_int32_t dxs_proc_EntriesInstall  (IFX_void_t);
static IFX_void_t  dxs_proc_EntriesRemove   (IFX_void_t);
#endif /* DXS_USE_PROC */


#ifndef CONFIG_RTK_VOIP
#ifdef __LINUX_SPI_H
static int DXS_SPI_Probe (struct spi_device *pSpiDev);
static int DXS_SPI_Remove (struct spi_device *pSpiDev);
#endif /* __LINUX_SPI_H */
#endif

/* ========================================================================== */
/*                             Local variables                                */
/* ========================================================================== */
#ifdef DXS_HAVE_INTERRUPTS
/* list of DUSLIC XS interrupts, neccessary for shared interrupt handling */
static DXS_IRQ_t *DXS_irq_head = IFX_NULL;
#endif /* DXS_HAVE_INTERRUPTS */

#ifndef CONFIG_RTK_VOIP
#ifdef __LINUX_SPI_H
static IFX_uint32_t nSpiUsage = 0;

static LIST_HEAD(device_list);
static DXS_OS_mutex_t device_list_lock;

#ifndef DXS_BROKEN_LX_MIPS_BSP_DEVICETREE
static const struct of_device_id dxs_match_table[] =
{{.compatible = "lantiq,duslicxs"},{}};

MODULE_DEVICE_TABLE(of, dxs_match_table);
#endif /* DXS_BROKEN_LX_MIPS_BSP_DEVICETREE */

static struct spi_driver sDxsSpiDriver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
   /* this name must match the name in arch/mips/ifxmips/spi_setup.c */
   .name = "duslicxs",
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(x,x,x) */
   #ifdef DXS_BROKEN_LX_MIPS_BSP_DEVICETREE
      .driver = {
              .name   = "duslicxs",
              .owner  = THIS_MODULE,
              .bus    = &spi_bus_type,
      },
   #else /* DXS_BROKEN_LX_MIPS_BSP_DEVICETREE */
      .driver = {
              .owner  = THIS_MODULE,
              .of_match_table = of_match_ptr(dxs_match_table),
              .name = "duslicxs"
      },
   #endif /* DXS_BROKEN_LX_MIPS_BSP_DEVICETREE */
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(x,x,x) */
   .probe = DXS_SPI_Probe,
   .remove = DXS_SPI_Remove,
};
#endif /* __LINUX_SPI_H */
#endif

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Reboot notification callback.

   \param   self   pointer to notifier_block structure
   \param   event
   \param   data
   \return  NOTIFY_DONE
   \todo    Adapt to different kernel versions.
*/
static int dxs_reboot_notifier(struct notifier_block *self,
                                  unsigned long event, void *data)
{
    reboot_ongoing = IFX_TRUE;
    printk("TAPIdxs reboot notifier called\n");
    return NOTIFY_DONE;
}

static struct notifier_block TAPI_DXS_reboot_notifier = {
    .notifier_call =    dxs_reboot_notifier,
};

/**
   Register a callback for reboot notification.

   \param   pointer to notifier_block structure
   \return  none
   \todo    Adapt to different kernel versions.
*/
IFX_void_t  DXS_OS_RebootNotifierRegister()
{
   register_reboot_notifier(&TAPI_DXS_reboot_notifier);
}

/**
   Unregister a callback for reboot notification.

   \param   pointer to notifier_block structure
   \return  none
   \todo    Adapt to different kernel versions.
*/
IFX_void_t DXS_OS_RebootNotifierUnRegister()
{
   unregister_reboot_notifier(&TAPI_DXS_reboot_notifier);
}

/**
   Check for ongoing reboot.

   \return
      IFX_TRUE  if reboot was triggered
      IFX_FALSE if not
*/
IFX_boolean_t  DXS_OS_isRebootOngoing()
{
   return reboot_ongoing;
}

/**
   This function allocates a temporary kernel buffer
      and copies the user buffer contents to it.

   \param  p_buffer     Pointer to the user buffer.
   \param  size         Size of the buffer.

   \return
   Pointer to kernel buffer or NULL.

   \remarks
   Buffer has to be released with \ref OS_UnmapBuffer.
*/
IFX_void_t* OS_MapBuffer(IFX_void_t* p_buffer, IFX_size_t size)
{
   IFX_void_t* kernel_buffer;

   if (p_buffer == IFX_NULL)
   {
      return IFX_NULL;
   }
   kernel_buffer = DXS_OS_MALLOC(size);
   if (kernel_buffer == IFX_NULL)
   {
      return IFX_NULL;
   }
   /*lint -save -e506 -e774 */
   if (DXS_OS_CpyUsr2Kern(kernel_buffer, p_buffer, size) == IFX_NULL)
   {
      DXS_OS_FREE(kernel_buffer);
      return IFX_NULL;
   }
   /*lint -restore */

   return kernel_buffer;
}

/**
   This function releases a temporary kernel buffer.

   \param  p_buffer     Pointer to the kernel buffer.

   \remarks
   Counterpart of \ref OS_MapBuffer.
*/
IFX_void_t OS_UnmapBuffer(IFX_void_t* p_buffer)
{
   if (p_buffer != IFX_NULL)
   {
      DXS_OS_FREE(p_buffer);
      p_buffer = IFX_NULL;
   }
}

#ifdef DXS_HAVE_INTERRUPTS
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
#if   (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
/**
   IRQ handler

   \param  irq          IRQ number. (unused)
   \param  pDev         Pointer to the device structure.
   \param  regs         (unused)
*/
static IFX_void_t OS_IRQ_Wrapper  (IFX_int32_t irq, IFX_void_t *pDev,
                                   struct pt_regs *regs)
{
   DXS_OS_UNUSED(irq);
   DXS_OS_UNUSED(regs);

   (void)irq_DXS_interrupt_routine((DXS_DEVICE_t*)pDev);
}
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
/**
   IRQ handler

   \param  irq          IRQ number. (unused)
   \param  pDev         Pointer to the device structure.
   \param  regs         (unused)

   \return IFX_IRQ_HANDLED
*/
static irqreturn_t OS_IRQ_Wrapper (IFX_int32_t irq, IFX_void_t *pDev,
                                   struct pt_regs *regs)
{
   DXS_OS_UNUSED(irq);
   DXS_OS_UNUSED(regs);

   return irq_DXS_interrupt_routine((DXS_DEVICE_t*)pDev);
}
#else
/**
   IRQ handler

   \param  irq          IRQ number. (unused)
   \param  pDev         Pointer to the device structure.

   \return IFX_IRQ_HANDLED
*/
static irqreturn_t OS_IRQ_Wrapper (IFX_int32_t irq, IFX_void_t *pDev)
{
   DXS_OS_UNUSED(irq);

   return irq_DXS_interrupt_routine((DXS_DEVICE_t*)pDev);
}
#endif /* LINUX_VERSION_CODE */
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */


/**
   Install the Interrupt Handler.

   \param  pDev         Pointer to DUSLIC XS device structure.
   \param  nIrq         Interrupt number.
*/
IFX_void_t OS_Install_DxsIRQHandler (DXS_DEVICE_t *pDev,
                                     IFX_int32_t nIrq)
{
/*lint -save -esym(529,intstatus) -esym(530,intstatus) */
   DXS_IRQ_t         *pIrq,
                     *pIrqPrev = IFX_NULL;
   DXS_DEVICE_t      *tmp_pDev;
   DXS_OS_INTSTAT    intstatus;
   IFX_int32_t       err = 0;

   /* Install interrupt handler for irq */
   if (pDev->pIrq != IFX_NULL)
      OS_UnInstall_DxsIRQHandler(pDev);

   pIrq = DXS_irq_head;

   /* try to find existing irq with same number */
   while (pIrq != IFX_NULL)
   {
      pIrqPrev = pIrq;
      if (pIrq->nIrq == nIrq)
         break;
      pIrq = pIrq->next_irq;
   }

   if (pIrq == IFX_NULL)
   {
      /* IRQ number not yet registered */

      pIrq = DXS_OS_MALLOC(sizeof(DXS_IRQ_t));
      if (pIrq == IFX_NULL)
         return;

      memset(pIrq, 0, sizeof(DXS_IRQ_t));
      DXS_OS_MutexInit(&pIrq->mtxIrqAcc);
      DXS_OS_MutexGet(&pIrq->mtxIrqAcc);

      DXS_DISABLE_IRQGLOBAL(intstatus);

      /* Install the Interrupt routine */
#ifdef DXS_FEAT_LINUX_THREADED_IRQ
      err = DXS_SYS_REGISTER_INT_HANDLER_THREAD(
                        (IFX_uint32_t)nIrq,
                        IRQF_TRIGGER_LOW,
                        DXS_irq_handler,
                        DXS_irq_thread_handler,
                        pIrq);
#else
      err = DXS_SYS_REGISTER_INT_HANDLER (
                        (IFX_uint32_t)nIrq,
                        (IRQF_DISABLED| IRQF_SHARED),
                        OS_IRQ_Wrapper,
                        pDev);
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
      if (err == 0)
      {
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
         DXS_DISABLE_IRQLINE((IFX_uint32_t)nIrq);
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */

         /* New dedicated or first shared interrupt. */
         pIrq->pdev_head = pDev;
         pIrq->next_irq = IFX_NULL;
         pIrq->nIrq = nIrq;
         pIrq->bRegistered = IFX_TRUE;

         pDev->pIrq = pIrq;
         pDev->pInt_NextDev = IFX_NULL;

         /* Add to the global list of interrupts. */
         /* this list is not used by the interrupt handler itself,
            so no additional intLock necessary! */
         if (DXS_irq_head == IFX_NULL)
            DXS_irq_head = pIrq;
         else
         {
            pIrq->next_irq = pIrqPrev->next_irq;
            pIrqPrev->next_irq = pIrq;
         }
      }

      DXS_ENABLE_IRQGLOBAL(intstatus);

      if (err != 0)
      {
         pDev->pIrq = IFX_NULL;
         DXS_OS_MutexDelete(&pIrq->mtxIrqAcc);
         DXS_OS_FREE(pIrq);
         TRACE (DXS, DBG_LEVEL_HIGH,
                ("ERROR: request interrupt failed, 0x%04X\n", (-err)));
      }
   }
   else
   {
      DXS_DISABLE_IRQGLOBAL(intstatus);

      /* Install the Interrupt routine (add to shared irq list) */
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
      err = DXS_SYS_REGISTER_INT_HANDLER (
                        (IFX_uint32_t)nIrq,
                        (IRQF_DISABLED| IRQF_SHARED),
                        OS_IRQ_Wrapper,
                        pDev);
      if (err == 0)
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
      {
         /* add into the list of an existing shared interrupt */
         pDev->pIrq = pIrq;
         pDev->pInt_NextDev = IFX_NULL;
         tmp_pDev = pIrq->pdev_head;
         while (tmp_pDev->pInt_NextDev != IFX_NULL)
         {
            tmp_pDev = tmp_pDev->pInt_NextDev;
         }
         tmp_pDev->pInt_NextDev = pDev;
      }

      DXS_ENABLE_IRQGLOBAL(intstatus);

#ifndef DXS_FEAT_LINUX_THREADED_IRQ
      if (err != 0)
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
      {
         TRACE (DXS, DBG_LEVEL_HIGH,
            ("ERROR: request shared interrupt failed, 0x%04X\n", (-err)));
      }
   }
/*lint -restore */
}


/**
   Uninstall the Interrupt Handler

   \param  pDev         Pointer to DUSLIC XS device structure.
*/
IFX_void_t OS_UnInstall_DxsIRQHandler(DXS_DEVICE_t *pDev)
{
   DXS_IRQ_t         *pIrq,
                     *pIrqPrev;
   DXS_DEVICE_t      *tmp_pDev;

   if (pDev->pIrq == IFX_NULL)
      return;

   /* uninstall regular interrupt */
   pIrq = pDev->pIrq;

   /* remove this pDev from the list of this pIrq */
   if (pIrq->pdev_head == pDev)
   {
      pIrq->pdev_head = pDev->pInt_NextDev;
   }
   else
   {
      /* find the element which has this pDev as next */
      tmp_pDev = pIrq->pdev_head;
      while (tmp_pDev->pInt_NextDev != pDev)
      {
         if (tmp_pDev->pInt_NextDev == IFX_NULL)
         {
            TRACE (DXS, DBG_LEVEL_HIGH,
               ("ERROR: device not found in interrupt list, "
                "cannot uninstall!\n"));
            return;
         }
         tmp_pDev = tmp_pDev->pInt_NextDev;
      }
      tmp_pDev->pInt_NextDev = pDev->pInt_NextDev;
   }
   pDev->pInt_NextDev = IFX_NULL;
   pDev->pIrq = IFX_NULL;

   /* if this was the last device, so cleanup the irq */
   if (pIrq->pdev_head == IFX_NULL)
   {
      /* IFXOS_MutexLock(pIrq->mtxIrqAcc); */
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
      if (pIrq->bIntEnabled == IFX_TRUE)
      {
         DXS_DISABLE_IRQLINE ((IFX_uint32_t)pIrq->nIrq);
      }
      if (pIrq->bRegistered == IFX_TRUE)
      {
         DXS_SYS_UNREGISTER_INT_HANDLER((IFX_uint32_t)pIrq->nIrq, pDev);
      }
#else /* DXS_FEAT_LINUX_THREADED_IRQ */
      if (pIrq->bRegistered == IFX_TRUE)
      {
         DXS_SYS_UNREGISTER_INT_HANDLER((IFX_uint32_t)pIrq->nIrq, pIrq);
      }
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
      /* remove this pIrq from irq-list */
      if (DXS_irq_head == pIrq)
      {
         DXS_irq_head = pIrq->next_irq;
      }
      else
      {
         pIrqPrev = DXS_irq_head;
         while (pIrqPrev->next_irq != IFX_NULL)
         {
            if (pIrqPrev->next_irq == pIrq)
               break;
            pIrqPrev = pIrqPrev->next_irq;
         }
         if (pIrqPrev->next_irq != IFX_NULL)
            pIrqPrev->next_irq = pIrq->next_irq;
      }
      DXS_OS_MutexDelete(&pIrq->mtxIrqAcc);
      DXS_OS_FREE(pIrq);
   }
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
   else
   {
      DXS_SYS_UNREGISTER_INT_HANDLER((IFX_uint32_t)pIrq->nIrq, pDev);
   }
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
}
#endif /* DXS_HAVE_INTERRUPTS */


/** \defgroup DxsKernel DUSLIC XS Kernel API
\remarks
  Kernel interface for open/close from Linux kernel mode
*/

/* @{ */

/**
   Open the device or channel from kernel mode.

   \param pHandle  pointer to LL device structure

   \return
   IFX_SUCCESS

*/
IFX_int32_t DXS_OpenKernel(IFX_void_t *pHandle)
{
   /* increment module use counter */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
   MOD_INC_USE_COUNT;
#else
   try_module_get(THIS_MODULE);
#endif
   return IFX_SUCCESS;
}

/**
   Release the device or channel.

   \param pHandle  pointer to LL device structure

   \return
   IFX_SUCCESS
*/
IFX_int32_t DXS_ReleaseKernel(IFX_void_t *pHandle)
{
   /* decrement use counter */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
   MOD_DEC_USE_COUNT;
#else
   module_put(THIS_MODULE);
#endif
   return IFX_SUCCESS;
}
/* @} */


#ifdef DXS_USE_PROC
/* ============================= */
/* /proc filesystem support      */
/* ============================= */

/**
   Read the version information from the driver.

   \param  buf    Destination buffer.

   \return
   Length of the string returned in buf.
*/
static IFX_int32_t dxs_proc_VersionGet (IFX_char_t *buf)
{
   IFX_int32_t len;
   IFX_uint8_t          dev;
   DXS_DEVICE_t         *pDev    =  IFX_NULL;

   len  = sprintf(buf, "%s\n", &DRV_DXS_WHATVERSION[4]);
   len += sprintf(buf + len, "Compiled on %s, %s for Linux kernel %s\n",
                  __DATE__, __TIME__, UTS_RELEASE);

   for (dev = 0; dev < DXS_MAX_DEVICES; dev++)
   {
      DXS_GetDevice(dev, &pDev);

      if ((pDev != IFX_NULL) &&
          (pDev->nDevState & DS_BASIC_INIT) &&
          (DXS_ReadFwVersion (pDev) == DXS_statusOk))
      {
         len += sprintf(buf + len,
                        "Device #%d: CHIP 0x%02X FW %d.%d.%d "
                        "DeviceID %d (0x%04X)\n",
                        dev, pDev->nChipRev,
                        (pDev->nFwRev >> 24) & 0xFF,
                        (pDev->nFwRev >> 16) & 0xFF,
                        (pDev->nFwRev >>  8) & 0xFF,
                        pDev->nDevId, pDev->nDevId);
      }
   }

   return len;
}

#ifdef HAVE_CONFIG_H
/**
   Read the configure parameters from the driver.

   \param buf  destination buffer

   \return
   length
*/
static IFX_int32_t dxs_proc_ConfigureGet (IFX_char_t *buf)
{
   IFX_int32_t len = 0;

#if DXS_BYTE_ORDER == DXS_BIG_ENDIAN
   len += sprintf(buf + len, "Compiled for big-endian\n");
#else
   len += sprintf(buf + len, "Compiled for little-endian\n");
#endif

   len += sprintf(buf + len, "configure: %s\n", &DRV_DXS_WHICHCONFIG[0]);

   return len;
}
#endif /* HAVE_CONFIG_H */

/**
   Read the device status from the driver.

   \param buf  destination buffer

   \return
   length
*/
static IFX_int32_t dxs_proc_StatusGet (IFX_char_t *buf)
{
   IFX_int32_t          len      =  0;
   IFX_uint8_t          dev, ch;
   DXS_DEVICE_t         *pDev    =  IFX_NULL;
   DXS_CHANNEL_t        *pCh     =  IFX_NULL;
   const char const     *DcDcNames[] =
      {
         "DC/DC type unknown",
         "Dedicated Inverting Buck-Boost Converter",
         "Combined Inverting Buck-Boost Converter",
         "Dedicated Inverting Boost Converter",
         "Combined Inverting Boost Converter",
         "Dedicated Buck-or-Boost Converter",
         "Combined Buck-or-Boost Converter",
         "Dedicated Inverting Flyback Converter",
         "Combined Inverting Flyback Converter",
         "Dedicated Inverting Boost Converter with Gate Driver",
         "Combined Inverting Boost Converter with Gate Driver",
         "Dedicated Inverting Boost Converter with Voltage Doubler",
         "Combined Inverting Boost Converter with Voltage Doubler"
      };

   if ((nAllowedDcDcType != DXS_DCDC_TYPE_EVAL) &&
       (nAllowedDcDcType < (sizeof(DcDcNames)/sizeof(char *))))
   {
      len += sprintf(buf + len,
                     "Driver enforces %s\n", DcDcNames[nAllowedDcDcType]);
   }

   for (dev = 0; dev < DXS_MAX_DEVICES; dev++)
   {
      DXS_GetDevice(dev, &pDev);

      if ((pDev != IFX_NULL) &&
          (pDev->nDevState & DS_BASIC_INIT))
      {
         len += sprintf(buf + len,
                        "Device #%d:\n", dev);

         /* DC/DC converter setting */
         for (ch = 0; ch < pDev->caps.nALI; ch++)
         {
            pCh = &pDev->pChannel[ch];

            if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
            {
               len += sprintf(buf + len,
                              "  Channel %d: "
                              "DC/DC converter operation mode not set\n", ch);
            }
            else
            {
               const char *pConverterName;

               pConverterName = DcDcNames[0];
               if (pCh->pALM->nDcDcType <= (sizeof(DcDcNames)/sizeof(char *))-1)
               {
                  pConverterName = DcDcNames[pCh->pALM->nDcDcType];
               }

               len += sprintf(buf + len,
                              "  Channel %d: "
                              "%s\n",
                              ch,
                              pConverterName);
            }
         }
      }
      /* PCM timeslot setting */
      if ((pDev != IFX_NULL) &&
          (pDev->nDevState & DS_PCM_EN))
      {
         IFX_uint32_t   i, j, Idx, BitValue;

         /* Print the current timeslot allocation bitmap in groups of 8 bits. */

         len += sprintf(buf + len, "  PCM TS RX: ");
         for (i = j = 0;  i < pDev->nMaxTimeslot; i++)
         {
            Idx = i >> 5;
            BitValue = 1 << (i & 0x1F);
            DXS_ASSERT(Idx < DXS_PCM_TS_ARRAY );
            strncat(buf+len, (pDev->PcmRxTs[0][Idx] & BitValue) ? "1" : "0", 1);
            len++;
            if ((++j % 8) == 0)
            {
               strncat(buf+len, " ", 1);
               len++;
            }
         }
         len += sprintf(buf + len, "\n");

         len += sprintf(buf + len, "  PCM TS TX: ");
         for (i = j = 0;  i < pDev->nMaxTimeslot; i++)
         {
            Idx = i >> 5;
            BitValue = 1 << (i & 0x1F);
            DXS_ASSERT(Idx < DXS_PCM_TS_ARRAY );
            strncat(buf+len, (pDev->PcmTxTs[0][Idx] & BitValue) ? "1" : "0", 1);
            len++;
            if ((++j % 8) == 0)
            {
               strncat(buf+len, " ", 1);
               len++;
            }
         }
         len += sprintf(buf + len, "\n");
      }
   }

   return len;
}

#ifdef EVALUATION
/**
   Eventlogger function which write values from the user to proc entry.

   \param file - file object which is ignored here
   \param buffer - contain new data passed from the user
   \param count - length of the passed data
   \param data - data (used as structure pointer)

   \return length of the used data which was passed from the user

   \remark Because new data is in the user space is must be copied to the
           kernel space with copy_from_user().
*/
static IFX_int32_t dxs_proc_Write(struct file* pFile,
                                  const IFX_char_t* buffer,
                                  IFX_ulong_t count,
                                  IFX_void_t* data)
{
   DXS_ProcEntry_t* entry = IFX_NULL;
   enum { MAX_IN_BUF_SIZE = 1000 };
   IFX_char_t temp[MAX_IN_BUF_SIZE] = {0};
   ptrWrFunc_t fn;
   IFX_ulong_t len;

   DXS_OS_UNUSED (pFile);

   if (data == IFX_NULL)
      return -EFAULT;

   entry = (DXS_ProcEntry_t *)data;
   fn = entry->funcWrite;

   if (count > MAX_IN_BUF_SIZE)
      len = MAX_IN_BUF_SIZE;
   else
      len = count;

   if (!fn || (copy_from_user(temp, buffer, len) != 0))
      return -EFAULT;

   /* Because we get string terminated with linefeed (0x0A)
            we terminate it with null terminator. */
   temp[len - 1] = '\0';

   fn(temp, (IFX_int32_t)len);

   return (IFX_int32_t)len;
}

/**
   The proc filesystem: function to read an entry being safe
   by making sure it does not overflow its buffer

   \param page      pointer to page buffer
   \param start     pointer to start address pointer
   \param off       offset
   \param count     length of buffer page
   \param eof       end of file
   \param data      data (used as structure pointer)
   \return length read
*/
static IFX_int32_t dxs_proc_SafeRead (IFX_char_t *page,
                                      IFX_char_t **start,
                                      off_t off,
                                      IFX_int32_t count,
                                      IFX_int32_t *eof,
                                      IFX_void_t *data)
{
   ptrRdFunc_t fn;
   DXS_ProcEntry_t* entry = IFX_NULL;
   IFX_int32_t len;

   DXS_OS_UNUSED (start);

   /* we call the function only once => we do not call it if there is an offset */
   if (data == IFX_NULL || off != 0)
      return 0;

   entry = (DXS_ProcEntry_t *)data;
   fn = entry->funcRead;

   if (!fn)
      return 0;

   len = fn (page, count);

   *eof = 1;

   if (len > count)
   {
      len = count;
   }
   if (len < 0)
   {
      len = 0;
   }

   return len;
}
#endif /* #ifdef EVALUATION */

/**
   The proc filesystem: function to read an entry.

   \param  page         Pointer to page buffer.
   \param  start        Pointer to start address pointer.
   \param  off          Offset.
   \param  count        Length to read.
   \param  eof          End of file.
   \param  data         Data (used as function pointer).

   \return
   Length of the string returned in buf.
*/
/*lint -save -e610 -e10 -e49 -e40 -e601 -e63 */
static IFX_int32_t dxs_proc_Read (IFX_char_t *page,
                                  IFX_char_t **start,
                                  off_t off,
                                  IFX_int32_t count,
                                  IFX_int32_t *eof,
                                  IFX_void_t *data)
{
   IFX_int32_t (*fn)(IFX_char_t *buf), len;

   if ((data != IFX_NULL) && (off == 0))
   {
      fn = data;
      len = fn (page);
   }
   else
   {
      return 0;
   }
   if (len <= off+count)
   {
      *eof = 1;
   }
   *start = page + off;
   len -= (IFX_int32_t)off;
   if (len > count)
   {
      len = count;
   }
   if (len < 0)
   {
      len = 0;
   }

   return len;
}
/*lint -restore */

/**
   Initialize and install the /proc filesystem entries.

   \return
   IFX_SUCCESS or IFX_ERROR

   \remark
   Called by the kernel.
*/
static IFX_int32_t dxs_proc_EntriesInstall (IFX_void_t)
{
   struct proc_dir_entry *driver_proc_node,
                         *proc_file;

   /* install the proc entry */
   TRACE(DXS, DBG_LEVEL_NORMAL, ("DUSLIC XS: using proc fs\n"));

#if 0 
   driver_proc_node = proc_mkdir( "driver/" DEV_NAME, IFX_NULL);
   /*lint -save -e611 */
   if (driver_proc_node != IFX_NULL)
   {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      driver_proc_node->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */

      proc_file = create_proc_read_entry("version" , 0, driver_proc_node,
                     dxs_proc_Read, (IFX_void_t *)dxs_proc_VersionGet);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (proc_file != NULL)
         proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */

#ifdef HAVE_CONFIG_H
      proc_file = create_proc_read_entry("configure", 0, driver_proc_node,
                     dxs_proc_Read, (IFX_void_t *)dxs_proc_ConfigureGet);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (proc_file != NULL)
         proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
#endif /* HAVE_CONFIG_H */

      proc_file = create_proc_read_entry("status", 0, driver_proc_node,
                     dxs_proc_Read, (IFX_void_t *)dxs_proc_StatusGet);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
      if (proc_file != NULL)
         proc_file->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */

#ifdef EVALUATION
      {
         struct proc_dir_entry *driver_proc_interrupt;

         driver_proc_interrupt = create_proc_read_entry("interrupt", 0,
                                 driver_proc_node, DXS_EVAL_Proc_InterruptGet,
                                 IFX_NULL);
         if (driver_proc_interrupt != NULL)
         {
            driver_proc_interrupt->write_proc = DXS_EVAL_Proc_InterruptPut;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
            driver_proc_interrupt->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
         }

         driver_proc_interrupt = create_proc_read_entry("cmdread", 0,
                                 driver_proc_node, dxs_proc_SafeRead,
                                 (IFX_void_t *) &DXS_cmdread_Entry);
         if (driver_proc_interrupt != NULL)
         {
            driver_proc_interrupt->write_proc = dxs_proc_Write;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
            driver_proc_interrupt->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
         }

         driver_proc_interrupt = create_proc_read_entry("cmdwrite", 0,
                                 driver_proc_node, dxs_proc_SafeRead,
                                 (IFX_void_t *) &DXS_cmdwrite_Entry);
         if (driver_proc_interrupt != NULL)
         {
            driver_proc_interrupt->write_proc = dxs_proc_Write;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
            driver_proc_interrupt->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
         }

         driver_proc_interrupt = create_proc_read_entry("regread", 0,
                                 driver_proc_node, dxs_proc_SafeRead,
                                 (IFX_void_t *) &DXS_regread_Entry);
         if (driver_proc_interrupt != NULL)
         {
            driver_proc_interrupt->write_proc = dxs_proc_Write;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
            driver_proc_interrupt->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
         }

         driver_proc_interrupt = create_proc_read_entry("regwrite", 0,
                                 driver_proc_node, dxs_proc_SafeRead,
                                 (IFX_void_t *) &DXS_regwrite_Entry);
         if (driver_proc_interrupt != NULL)
         {
            driver_proc_interrupt->write_proc = dxs_proc_Write;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
            driver_proc_interrupt->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
         }

         driver_proc_interrupt = create_proc_read_entry("evten", 0,
                                 driver_proc_node, dxs_proc_SafeRead,
                                 (IFX_void_t *) &DXS_evtEnable_Entry);
         if (driver_proc_interrupt != NULL)
         {
            driver_proc_interrupt->write_proc = dxs_proc_Write;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
            driver_proc_interrupt->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
         }

         driver_proc_interrupt = create_proc_read_entry("evtread", 0,
                                 driver_proc_node, dxs_proc_SafeRead,
                                 (IFX_void_t *) &DXS_evtRead_Entry);
         if (driver_proc_interrupt != NULL)
         {
            driver_proc_interrupt->write_proc = dxs_proc_Write;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
            driver_proc_interrupt->owner = THIS_MODULE;
#endif /* < Linux 2.6.30 */
         }
      }
#endif /* #ifdef EVALUATION */
   }
   else
   {
      TRACE(DXS, DBG_LEVEL_HIGH,("DUSLIC XS: cannot create proc entry\n"));
      return IFX_ERROR;
   }
   /*lint -restore */
#endif
   return IFX_SUCCESS;
}

/**
   Remove proc filesystem entries.

   \return
   None.

   \remark
   Called by the kernel.
*/
static IFX_void_t dxs_proc_EntriesRemove(IFX_void_t)
{
#ifdef EVALUATION
   remove_proc_entry("driver/" DEV_NAME "/evtread", 0);
   remove_proc_entry("driver/" DEV_NAME "/evten", 0);
   remove_proc_entry("driver/" DEV_NAME "/regwrite", 0);
   remove_proc_entry("driver/" DEV_NAME "/regread", 0);
   remove_proc_entry("driver/" DEV_NAME "/cmdwrite", 0);
   remove_proc_entry("driver/" DEV_NAME "/cmdread", 0);
   remove_proc_entry("driver/" DEV_NAME "/interrupt", 0);
#endif /* #ifdef EVALUATION */
   remove_proc_entry("driver/" DEV_NAME "/status" ,0);
   remove_proc_entry("driver/" DEV_NAME "/version" ,0);
#ifdef HAVE_CONFIG_H
   remove_proc_entry("driver/" DEV_NAME "/configure", 0);
#endif /* HAVE_CONFIG_H */
   remove_proc_entry("driver/" DEV_NAME ,0);

   return;
}
#endif /* DXS_USE_PROC */

#ifdef DXS_HAVE_INTERRUPTS
/**
   Disables irq line if the driver is in interrupt mode and irq line is
   actually enabled according to device flag bIntEnabled.
   Disable the global interrupt if the device is not in polling mode and no
   interrupt line is connected.

   \param pLLDev pointer to DUSLIC XS device

   \return
   None.

   \remarks
   If the driver works in Polling mode, nothing is done.
   If the driver works in interrupt mode and the irq was already disabled
   (flag bIntEnabled is IFX_FALSE ), the os disable function will
   not be called.

   Very important: It is assumed that disable and enable irq are done
   subsequently and that no routine calling disable/enable is executed
   inbetween as stated in following code example:

   \verbatim
   Allowed :

   DXS_IrqLockDevice;
   .. some instructions
   DXS_IrqUnlockDevice

   Not allowed:

   routineX (IFX_int32_t x)
   {
      DXS_IrqLockDevice;
      .. some instructions;
      DXS_IrqUnlockDevice
   }

   routineY (IFX_int32_t y)
   {
      DXS_IrqLockDevice;
      routine (x);    <---------------- routineX unlocks interrupts..
      be carefull!
      ... some more instructions;
      DXS_IrqUnlockDevice;
   }
   \endverbatim
*/
IFX_void_t DXS_IrqLockDevice (IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t      *pDev = (DXS_DEVICE_t *) pLLDev;

   if (pDev != IFX_NULL)
   {
      DXS_IRQ_t         *pIrq = pDev->pIrq;

      if ((pIrq != IFX_NULL) &&
          (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
      {
         /* device specific interrupt routine is initialized */
         /* interrupt line was disabled already: exit */
         if (pIrq->bIntEnabled == IFX_FALSE)
            return;
         /* it must be possible to take the mutex before disabling the irq */
         if (!in_interrupt())
            DXS_OS_MutexGet(&pIrq->mtxIrqAcc);
         /* invoke board or os routine to disable irq */
         DXS_DISABLE_IRQLINE ((IFX_uint32_t)pIrq->nIrq);
         /* reset enable flag to signalize that interrupt line is disabled */
         pIrq->bIntEnabled = IFX_FALSE;
      }
      else
      {
         /* there is no specific interrupt service routine configured for that
            device. Disable the global interrupts instead */
         if (pDev->nIrqMask == 0)
         {
            DXS_DISABLE_IRQGLOBAL(pDev->nIrqMask);
         }
      }
   }
}

/**
   Enables the irq line if the driver is in interrupt mode and irq line is
   actually disabled according to device flag bIntEnabled.
   Enable the global interrupt, if this was disabled by 'DXS_IrqDisable'

   \param pLLDev  pointer to DUSLIC XS device

   \return
   None.
*/
IFX_void_t DXS_IrqEnable (IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t   *pDev = (DXS_DEVICE_t *) pLLDev;

   if (pDev != IFX_NULL)
   {
      DXS_IRQ_t      *pIrq = pDev->pIrq;

      if ((pIrq != IFX_NULL) &&
          (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
      {
         /* invoke board or os routine to enable irq */
         DXS_ENABLE_IRQLINE((IFX_uint32_t)pIrq->nIrq);
      }
   }
}

/**
   Disables irq line if the driver is in interrupt mode and irq line is
   actually enabled according to device flag bIntEnabled.
   Disable the global interrupt, if this was enabled by 'DXS_IrqEnable'

   \param pLLDev  pointer to DUSLIC XS device

   \return
   None.
*/
IFX_void_t DXS_IrqDisable (IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t   *pDev = (DXS_DEVICE_t *) pLLDev;

   if (pDev != IFX_NULL)
   {
      DXS_IRQ_t      *pIrq = pDev->pIrq;

      if ((pIrq != IFX_NULL) &&
          (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
      {
         /* invoke board or os routine to disable irq */
         DXS_DISABLE_IRQLINE((IFX_uint32_t)pIrq->nIrq);
      }
   }
}

/**
   Enables irq line if the driver is in interrupt mode and irq line is
   actually disabled according to device flag bIntEnabled.
   Enable the global interrupt, if this was disabled by 'DXS_IrqLockDevice'.

   \param pLLDev  pointer to DUSLIC XS device

   \return
   None.

   \remarks
   cf Remarks of DXS_IrqLockDevice () in this file.
*/
IFX_void_t DXS_IrqUnlockDevice (IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t      *pDev = (DXS_DEVICE_t *) pLLDev;

   if (pDev != IFX_NULL)
   {
      DXS_IRQ_t         *pIrq = pDev->pIrq;

      if ((pIrq != IFX_NULL) &&
          (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
      {
         /* device specific interrupt routine is initialized */
         /* interrupt line was enabled already: exit */
         if (pIrq->bIntEnabled == IFX_TRUE)
            return;
         /* set enable flag to signalize that interrupt line is enabled */
         pIrq->bIntEnabled = IFX_TRUE;
         /* invoke board or os routine to enable irq */
         DXS_ENABLE_IRQLINE((IFX_uint32_t)pIrq->nIrq);
         DXS_OS_MutexRelease(&pIrq->mtxIrqAcc);
      }
      else
      {
         /*
            there is no specific interrupt service routine configured for that
            device. Enable the global interrupts again
         */
         /*lint -save -esym(529,nMask) */
         if (pDev->nIrqMask != 0)
         {
            DXS_OS_INTSTAT nMask = pDev->nIrqMask;
            pDev->nIrqMask = 0;
            DXS_ENABLE_IRQGLOBAL(nMask);
         }
         /*lint -restore */
      }
   }
}
#endif /* DXS_HAVE_INTERRUPTS */

/**
   Race condition free implmentation for IFXOS_WaitEvent_timeout
   \param  pDev         Reference to device context.

   \return
   - DXS_statusOk: Data available for reading.
   - DXS_statusReadInterrupted: Waiting interrupted by signal.
   - DXS_statusCmdObTimeout: Timeout occured while waiting for data.

   \remarks
   Porting instructions

   Different OS implement the wait queue mechanism in different ways. The
   major difference is the way they handle a wakeup which occurs before
   the thread is completely sleeping. Linux is known not to handle this
   "early" wakeup properly, i.e. we'll sleep forever - or when we are lucky
   the same event occurs again. To prevent this case, we need to check a
   condition "half the way to sleep" - where we are still able to rollback
   our preparations to sleep.
   If your OS doesn't require this special handling, you can continue using
   IFXOS_WaitEvent_timeout - or your specific port of this macro - (and
   ignore the additional condition in pDev->bCmdOutBoxData).
*/
IFX_int32_t DXS_WaitForCmdMbxData(DXS_DEVICE_t *pDev)
{
   int   ret;

   /*lint -esym(715, pDev) */
   ret = wait_event_interruptible_timeout(pDev->obxDataEvt.object,
                                          pDev->bOutBoxData,
                                          (3000 * HZ / 1000));

   if (ret == 0)
   {
      /* errmsg: Timeout while waiting for data in command outbox. */
      return DXS_statusCmdObTimeout;
   }
   else
   if (ret < 0)
   {
      /* errmsg: CmdRead interrupted by signal. */
      return DXS_statusReadInterrupted;
   }

   return DXS_statusOk;
}

/**
   Waiting for completion of SDD Operating Mode change or timeout.

   \param  pCh          Pointer to DXS channel context .

   \return  >0      wakeup occured, remaining jiffies
            0       timeout
            <0      interrupted by a signal
*/
IFX_int32_t DXS_WaitForSddOpmodeChEvt(DXS_CHANNEL_t *pCh)
{
   return wait_event_interruptible_timeout(pCh->pALM->sdd_event.object,
                                           !pCh->pALM->bOpmodeChangePending,
                                           (SDD_EVT_TIMEOUT_MS * HZ / 1000));
}

/**
   Modify own thread priority.

   \param  newPriority  New thread priority.

   \return
   - IFX_SUCCESS priority changed.
   - IFX_ERROR priority not changed.
*/
IFX_int32_t DXS_OS_ThreadPriorityModify (IFX_uint32_t newPriority)
{
   struct sched_param sched_params;
   IFX_int32_t ret = 0;

   sched_params.sched_priority = newPriority;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#if 0
   ret = setscheduler(current->pid, SCHED_FIFO, &sched_params);
#else /* 0 */
   current->policy = SCHED_FIFO;
   current->rt_priority = newPriority;
#endif /* 0 */
#else /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)) */
   ret = sched_setscheduler (current, SCHED_FIFO, &sched_params);
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)) */

   if (ret < 0)
   {
      printk(KERN_ERR "Failed to set the thread priority to %d"
             " ret=%d" DXS_CRLF, newPriority, ret);
   }

   return (ret < 0) ? IFX_ERROR : IFX_SUCCESS;
}

/**
   stop a kernel thread. Called by the removing instance

   \param  pThrCntrl    Pointer to a thread control struct.
   \param  pMutex       Pointer to mutex to unblock the thread to be killed.
*/
IFX_void_t DXS_OS_ThreadKill (IFXOS_ThreadCtrl_t *pThrCntrl,
                              DXS_OS_mutex_t *pMutex)
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
      if (pMutex != IFX_NULL)
         DXS_OS_MutexRelease(pMutex);

      /* Wait for the confirmation that the thread has stopped */
      wait_for_completion (&pThrCntrl->thrCompletion);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
      /* Now that we are sure the thread is in zombie state.
         We notify keventd to clean the process up. */
      kill_proc(2, SIGCHLD, 1);
#endif /* kernel 2.6.23 */
      pThrCntrl->bValid = IFX_FALSE;
   }
}

#ifndef CONFIG_RTK_VOIP
#ifdef __LINUX_SPI_H
/**
   SPI device driver setup and teardown

   Binds this driver to the spi device.  Drivers can verify
   that the device is actually present, and may need to configure
   characteristics (such as bits_per_word) which weren't needed for
   the initial configuration done during system setup.

   \param  pSpiDev      Pointer to the SPI device.

   \return
   <  0 if incorrect spi device used
   >= 0 if successful
 */
/*lint -save -esym(515,strcmp) -esym(516,strcmp) -e746 */
static int DXS_SPI_Probe (struct spi_device *pSpiDev)
{
   struct DXS_SPI_DEV *spidev;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
   if (strcmp (pSpiDev->modalias, "duslicxt"))
      return 0;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16) */

   /* Allocate spi device manager data */
   spidev = DXS_OS_MALLOC (sizeof(*spidev));
   if (!spidev)
      return -ENOMEM;

   /* Initialize spi device manager data */
   memset (spidev, 0, sizeof(*spidev));
   spidev->spi = pSpiDev;
   INIT_LIST_HEAD(&spidev->device_entry);

   /* Add spi device to the common list */
   DXS_OS_MutexGet (&device_list_lock);
   list_add_tail(&spidev->device_entry, &device_list);
   DXS_OS_MutexRelease (&device_list_lock);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
   spi_set_drvdata(pSpiDev, spidev);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) */

   return 0;
}
/*lint -restore */

/**
   SPI device driver setup and teardown

   Unbinds this driver from the spi device.

   \param  pSpiDev      Pointer to the SPI device.

   \return
   <  0 if incorrect spi device used
   >= 0 if successful
 */
/*lint -save -esym(515,strcmp) -esym(516,strcmp) -e746 */
static int DXS_SPI_Remove (struct spi_device *pSpiDev)
{
   struct DXS_SPI_DEV *spidev = IFX_NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
   struct DXS_SPI_DEV *tmp_spidev = IFX_NULL;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16) */

   DXS_OS_MutexGet (&device_list_lock);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
   /* find device in list */
   /*lint ++flb */
   list_for_each_entry_safe (spidev, tmp_spidev, &device_list, device_entry)
   /*lint --flb */
   {
      if (spidev->spi == pSpiDev)
      {
         /* prevent new opens */
         list_del(&spidev->device_entry);
         DXS_OS_Free (spidev);
         break;
      }
   }
#else
   spidev = (struct DXS_SPI_DEV *)spi_get_drvdata(pSpiDev);

   if (IFX_NULL != spidev)
   {
      /* make sure ops can abort cleanly */
      spi_set_drvdata(spidev->spi, IFX_NULL);
      spidev->spi = IFX_NULL;

      /* prevent new opens */
      list_del(&spidev->device_entry);
      DXS_OS_FREE (spidev);
   }
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16) */

   DXS_OS_MutexRelease (&device_list_lock);

   return 0;
}
/*lint -restore */

/**
   Register SPI driver

   \param  pDev      Pointer to device structure.
*/
/*lint -save -esym(413, spidev) */
IFX_void_t DXS_SPI_drvRegister (DXS_DEVICE_t * pDev)
{
   IFX_uint32_t i = 0;
   struct DXS_SPI_DEV *spidev = IFX_NULL;

   /* for first usage do SPI driver initialization */
   if (0 == nSpiUsage)
   {
      DXS_OS_MutexInit (&device_list_lock);

      /* register driver to capture SPI devices */
      spi_register_driver (&sDxsSpiDriver);
   }

   nSpiUsage ++;

   DXS_OS_MutexGet (&device_list_lock);
   /* mark as used spi device */
   /*lint ++flb */
   list_for_each_entry(spidev, &device_list, device_entry)
   /*lint --flb */
   {
      if (pDev->nDevNr == i)
      {
         pDev->pSpiDev = spidev->spi;
      }
      i++;
   }
   DXS_OS_MutexRelease (&device_list_lock);
}
/*lint -restore */

/**
   Unregister SPI driver

   \param  pDev      Pointer to device structure.
*/
/*lint -save -esym(413, spidev) */
IFX_void_t DXS_SPI_drvUnregister (DXS_DEVICE_t * pDev)
{
   struct DXS_SPI_DEV *spidev = IFX_NULL;

   DXS_OS_MutexGet (&device_list_lock);

   /* mark as unused spi device */
   /*lint ++flb */
   list_for_each_entry(spidev, &device_list, device_entry)
   /*lint --flb */
   {
      if (pDev->pSpiDev == spidev->spi) {
         pDev->pSpiDev = IFX_NULL;
      }
   }

   DXS_OS_MutexRelease (&device_list_lock);

   /* unregister SPI driver after last usage */
   if (1 == nSpiUsage)
   {
      /* unregister driver if no any users */
      spi_unregister_driver (&sDxsSpiDriver);

      DXS_OS_MutexDelete (&device_list_lock);
   }

   nSpiUsage --;
}
/*lint -restore */
#endif /* __LINUX_SPI_H */
#endif

/**
   Initialize the module.

   \return 0 if successful or an negative error code otherwise.

   \remarks
   Called by the kernel.
*/
int __init dxs_module_init(void)
{
   IFX_int32_t ret = IFX_SUCCESS;

   ret = DXS_DeviceDriverStart();

#ifdef DXS_USE_PROC
   if (ret == 0)
   {
      dxs_proc_EntriesInstall();
   }
#endif /* DXS_USE_PROC */

   /* Report error codes as negative numbers. */
   return (0 - ret);
}

/**
   Clean up the module when unloaded.

   \remarks
   Called by the kernel.
*/
void __exit dxs_module_exit(void)
{
#ifdef DXS_USE_PROC
   dxs_proc_EntriesRemove();
#endif /* DXS_USE_PROC */

   DXS_DeviceDriverStop();

   TRACE (DXS, DBG_LEVEL_LOW, ("Cleaned up %s module.\n", DEV_NAME));
}

/*lint -save -e19 -e546*/
#ifndef CONFIG_RTK_VOIP_MODULE_BUILD
module_init(dxs_module_init);
module_exit(dxs_module_exit);
#endif
/* ========================================================================== */
/*                         Function pointer exports                           */
/* ========================================================================== */
#ifdef DXS_FEAT_KERNEL_FUNCTIONS_EXPORT
EXPORT_SYMBOL(DXS_OpenKernel);
EXPORT_SYMBOL(DXS_ReleaseKernel);
#endif /* DXS_FEAT_KERNEL_FUNCTIONS_EXPORT */
#ifdef DXS_FEAT_GPIO_FUNCTIONS_EXPORT
EXPORT_SYMBOL(DXS_GpioReserve);
EXPORT_SYMBOL(DXS_GpioRelease);
EXPORT_SYMBOL(DXS_GpioConfig);
EXPORT_SYMBOL(DXS_GpioSet);
EXPORT_SYMBOL(DXS_GpioGet);
EXPORT_SYMBOL(DXS_GpioIntMask);
#endif /* DXS_FEAT_GPIO_FUNCTIONS_EXPORT */
/*lint -restore */
#endif /* LINUX */
