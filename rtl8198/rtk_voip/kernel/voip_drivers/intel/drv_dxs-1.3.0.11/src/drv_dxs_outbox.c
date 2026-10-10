/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file dxs_outbox.c
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_fw_cmd_sdd.h"
#include "drv_dxs_cid.h"
#include "drv_dxs_dtmf.h"
#include "drv_dxs_utd.h"
#include "drv_dxs_access.h"
#ifdef DXS_FEAT_NLT
#include "drv_dxs_alm.h"
#include "drv_dxs_alm_priv.h"
#endif
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#ifdef REALTEK_PATCH_FOR_INTEL
extern int debugMode; //define in snd_intel_dxs.c
#endif
/* Internal Error events */
/**  Internal event queue overflow */
#define EVT_INTERR_EQO           1
/**  Clock fail */
#define EVT_INTERR_CLK_FAIL      2
/**  Synchronisation error */
#define EVT_INTERR_SYNC_FAIL     3

/* SDD events */
/** Over temperature detected */
#define EVT_SDD_OTEMP            0
/** Line testing finished */
#define EVT_SDD_LT_FIN           1
/** Ground fault detected */
#define EVT_SDD_GF               3
/** Ground key detected */
#define EVT_SDD_GK               4
/** Opmode changed */
#define EVT_SDD_OPC              5
/** Error in command SDD_CoeffReadConfig */
#define EVT_SDD_CORCE            6
/** Error in command SDD_Coeff */
#define EVT_SDD_COEFE            7
/** On-hook */
#define EVT_SDD_ONH              9
/** Off-hook */
#define EVT_SDD_OFFH             10
/** Ground Fault Finished */
#define EVT_SDD_GF_FIN           12
/** Ground Key Finished */
#define EVT_SDD_GK_FIN           13
/** Overtemp finished which means that
   SLIC temperature is back in normal range */
#define EVT_SDD_OTEMP_FIN        14
/** Operating Mode Ignored */
#define EVT_SDD_OMI              17
/** Operating Mode discarded */
#define EVT_SDD_OPM_DIS          18
/** Line testing aborted */
#define EVT_SDD_LT_ABORT         19

/* SIG events */
/** DTMF detector */
#define EVT_SIG_DTMF_DET       1
/** Caller ID sender request */
#define EVT_SIG_CIS_REQ        2
/** Caller ID sender buffer underflow */
#define EVT_SIG_CIS_BUF        3
/** Caller ID sender has finished sending data */
#define EVT_SIG_CIS_FIN        4
/** Start of tone detected */
#define EVT_SIG_UTD_START      5
/** End of tone detected */
#define EVT_SIG_UTD_END        6
/** Metering pulse sent */
#define EVT_SIG_TTX_FIN        9
/** AC level metering finished */
#define EVT_SIG_AC_LM_FIN      10

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
struct __fw_evt_header
{
#if DXS_BYTE_ORDER == DXS_BIG_ENDIAN
   /* Reserved */
   uint32_t Res00 : 3;
   /* Command Type */
   uint32_t CMD : 5;
   /* Channel */
   uint32_t CHAN : 8;
   /* Command Mode */
   uint32_t MOD : 3;
   /* Command Sub-Mode */
   uint32_t ECMD : 5;
   /* Length of Command Payload */
   uint32_t LENGTH : 8;
#else
   /* Length of Command Payload */
   uint32_t LENGTH : 8;
   /* Command Sub-Mode */
   uint32_t ECMD : 5;
   /* Command Mode */
   uint32_t MOD : 3;
   /* Channel */
   uint32_t CHAN : 8;
   /* Command Type */
   uint32_t CMD : 5;
   /* Reserved */
   uint32_t Res00 : 3;
#endif
} __attribute__ ((packed));

struct __fw_evt_int_err
{
   struct __fw_evt_header hdr;
#if DXS_BYTE_ORDER == DXS_BIG_ENDIAN
   /* Reserved */
   uint32_t Res01 : 27;
   /* Event Code */
   uint32_t ERREVT : 5;
#else
   /* Event Code */
   uint32_t ERREVT : 5;
   /* Reserved */
   uint32_t Res01 : 27;
#endif
} __attribute__ ((packed));

struct __fw_evt_cmd_err
{
   struct __fw_evt_header hdr;
#if DXS_BYTE_ORDER == DXS_BIG_ENDIAN
   /* Reserved */
   uint32_t Res01 : 18;
   /* Error Cause */
   uint32_t CMDERR : 14;
   /* Command Header of Command */
   uint32_t CMDHDR;
#else
   /* Error Cause */
   uint32_t CMDERR : 14;
   /* Reserved */
   uint32_t Res01 : 18;
   /* Command Header of Command */
   uint32_t CMDHDR;
#endif
} __attribute__ ((packed));

struct __fw_evt_sdd
{
   struct __fw_evt_header hdr;
#if DXS_BYTE_ORDER == DXS_BIG_ENDIAN
   /* Time Stamp */
   uint32_t TIME_STAMP : 16;
   /* Analog line operating mode */
   uint32_t OPMODE : 8;
   /* Reserved */
   uint32_t Res01 : 3;
   /* Event code */
   uint32_t EVT : 5;
#else
   /* Event code */
   uint32_t EVT : 5;
   /* Reserved */
   uint32_t Res01 : 3;
   /* Analog line operating mode */
   uint32_t OPMODE : 8;
   /* Time Stamp */
   uint32_t TIME_STAMP : 16;
#endif
} __attribute__ ((packed));

struct __fw_evt_sig
{
   struct __fw_evt_header hdr;
#if DXS_BYTE_ORDER == DXS_BIG_ENDIAN
   /* Time Stamp */
   uint32_t TIME_STAMP : 16;
   /* Reserved */
   uint32_t Res01 : 4;
   /*  DTMF Key */
   uint32_t DTMF_KEY : 4;
   /* Reserved */
   uint32_t Res02 : 3;
   /* Event code */
   uint32_t SIGEVT : 5;
#else
   /* Event code */
   uint32_t SIGEVT : 5;
   /* Reserved */
   uint32_t Res02 : 3;
   /*  DTMF Key */
   uint32_t DTMF_KEY : 4;
   /* Reserved */
   uint32_t Res01 : 4;
   /* Time Stamp */
   uint32_t TIME_STAMP : 16;
#endif
} __attribute__ ((packed));

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_void_t dxs_handle_event(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_header *pHdr);

static IFX_void_t dxs_event_sys(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_header *pHdr);

static IFX_void_t dxs_event_sdd(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_sdd *pSddEvt);

static IFX_void_t dxs_event_sig(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_sig *pSigEvt);

extern IFX_void_t DXS_ObxRead(
                        DXS_DEVICE_t *pDev,
                        IFX_uint16_t *pData,
                        IFX_uint8_t *len);


/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Event handler switchboard

   \param  pDev         Pointer to the device structure.
   \param  pHdr         Pointer to the FW message.
*/
static IFX_void_t dxs_handle_event(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_header *pHdr)
{
   LOG_RD_EVT(pDev->nDevNr, pDev->nChannel, pHdr, pHdr->LENGTH + 4, 0);

   if (pHdr->MOD == 7)
   {
#ifdef REALTEK_PATCH_FOR_INTEL
    if(debugMode)
	{
		if(pHdr->ECMD == 0)
		{
	      	printk("[evt] dev:%d %08X\n", pDev->nDevNr, *((uint32_t *)pHdr));
		}
		else if(pHdr->ECMD == 11)
		{
      		printk("[evt] dev:%d %08X %08X\n", pDev->nDevNr,
                          *((uint32_t *)pHdr), *((uint32_t *)pHdr + 1));
		}
		else if(pHdr->ECMD == 2)
		{
      		printk("[evt] dev:%d %08X %08X %08X\n", pDev->nDevNr,
               *((uint32_t *)pHdr), *((uint32_t *)pHdr + 1), *((uint32_t *)pHdr + 2));
		}
	}
#endif
      /* System events */
      dxs_event_sys(pDev, pHdr);
   }
   else if (pHdr->MOD == 1 && pHdr->ECMD == 4)
   {
#ifdef REALTEK_PATCH_FOR_INTEL
    if(debugMode)
	{
		printk("[evt] dev:%d %08X %08X\n", pDev->nDevNr,
                          *((uint32_t *)pHdr), *((uint32_t *)pHdr + 1));
	}
#endif
      /* SDD event */
      dxs_event_sdd(pDev, (struct __fw_evt_sdd *)pHdr);
   }
   else if (pHdr->MOD == 1 && pHdr->ECMD == 5)
   {
#ifdef REALTEK_PATCH_FOR_INTEL
    if(debugMode){
		printk("[evt] dev:%d %08X %08X\n", pDev->nDevNr,
                          *((uint32_t *)pHdr), *((uint32_t *)pHdr + 1));
	}
#endif
      /* SIG event */
      dxs_event_sig(pDev, (struct __fw_evt_sig *)pHdr);
   }
}


/**
   Handler for System Events

   \param  pDev         Pointer to the device structure.
   \param  pHdr         Pointer to the FW message.
*/
static IFX_void_t dxs_event_sys(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_header *pHdr)
{
   if (pHdr->ECMD == 0)
   {
      /* Boot Finished event */
      pDev->nDevState |= DS_DEV_UP;
   }
   else
   if (pHdr->ECMD == 11)
   {
      /* Internal Error event */
      struct __fw_evt_int_err *pIntErr = (struct __fw_evt_int_err *)pHdr;
      IFX_TAPI_EVENT_t  tapiEvent;

      if (pIntErr->ERREVT == EVT_INTERR_CLK_FAIL)
      {
         /* Send an Clock fail end event to TAPI. */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END;
         DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);

         IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
      }
      else if(pIntErr->ERREVT == EVT_INTERR_SYNC_FAIL)
      {
         /* Send an Clock fail end event to TAPI. */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_HW_SYNC;
         DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);

         IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
      }
      else
      {
         TRACE(DXS, DBG_LEVEL_HIGH,
             ("DXS device %d unknown Internal Error Event code:%d\n",
             pDev->nDevNr, pIntErr->ERREVT));
      }
   }
   else
   if (pHdr->ECMD == 2)
   {
      /* Command Error event */
      struct __fw_evt_cmd_err *pCerr = (struct __fw_evt_cmd_err *)pHdr;
      IFX_TAPI_EVENT_t  tapiEvent;
      IFX_uint32_t cerr_ack_cmd = 0x0600e000;

      /* acknowledge command error */
      (IFX_void_t)DXS_CmdWrite(pDev, &cerr_ack_cmd);

      /* send an CERR event including the details to TAPI */
      memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
      tapiEvent.id = IFX_TAPI_EVENT_DEBUG_CERR;
      DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);
      tapiEvent.data.cerr.fw_id = 0x0002;    /* id = 2 identifies DuSLIC-XS */
      tapiEvent.data.cerr.reason = pCerr->CMDERR;
      tapiEvent.data.cerr.command = pCerr->CMDHDR;
      IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
   }
}


/**
   Handle the SDD event

   \param  pDev         Pointer to the device structure.
   \param  pSddEvent    Pointer to the SDD event structure.
*/
static IFX_void_t dxs_event_sdd(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_sdd *pSddEvt)
{
   DXS_CHANNEL_t *pCh = &pDev->pChannel[pSddEvt->hdr.CHAN];
   IFX_TAPI_EVENT_t tapiEvent;

   if (pSddEvt->hdr.CHAN >= DXS_MAX_CH_NR)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("DXS FW error, reporting SDD event on wrong channel %d\n",
            pSddEvt->hdr.CHAN));
      return;
   }

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      TRACE(DXS, DBG_LEVEL_LOW,
          ("WARNING: Skipping DXS SDD Event on not initialized driver.\n"));
      return;
   }

   memset(&tapiEvent, 0, sizeof(tapiEvent));
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

   switch (pSddEvt->EVT)
   {
      case EVT_SDD_OTEMP:
         pCh->flags |= DXS_CH_OVER_TEMPERATURE;
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, DXS_SDD_Opmode_Disabled);
            DXS_ALM_OnLineModeChanged(pCh, DXS_SDD_Opmode_Disabled);
         }
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_OTEMP_FIN:
         pCh->flags &= ~DXS_CH_OVER_TEMPERATURE;
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_LT_FIN:
         tapiEvent.id = (DXS_ALM_CapMeasInProgress(pCh) == IFX_TRUE) ?
            IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT :
            IFX_TAPI_EVENT_LT_GR909_RDY;
			pCh->sddEvent |= (1 << EVT_SDD_LT_FIN);
			//printk("EVT_SDD_LT_FIN, EVT_SDD_LT_FIN, EVT_SDD_LT_FIN\n");
         IFX_TAPI_Event_DeferredDispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_LT_ABORT:
         if (DXS_ALM_CapMeasInProgress(pCh) == IFX_TRUE)
         {
            tapiEvent.id = IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT;
            IFX_TAPI_Event_DeferredDispatch(pCh->pTapiCh, &tapiEvent);
         }
         break;

      case EVT_SDD_GF:
         pCh->flags |= DXS_CH_GROUND_FAULT;
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, DXS_SDD_Opmode_Disabled);
            DXS_ALM_OnLineModeChanged(pCh, DXS_SDD_Opmode_Disabled);
         }
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_GF_FIN:
         pCh->flags &= ~DXS_CH_GROUND_FAULT;
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_GK:
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_LOW;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_GK_FIN:
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_OPC:
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, pSddEvt->OPMODE);
            DXS_ALM_OnLineModeChanged(pCh, pSddEvt->OPMODE);
         }
         break;

      case EVT_SDD_OMI:
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, OPMODE_IGNORED);
         }
         break;

      case EVT_SDD_OPM_DIS:
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, OPMODE_IGNORED);
         }
         break;

      case EVT_SDD_ONH:
         /* Deactivate the dtmf receiver. */
         DXS_DTMF_REC_CTRL(pCh, IFX_FALSE);
         /* TAPI event */
         tapiEvent.id = IFX_TAPI_EVENT_FXS_ONHOOK_INT;
         tapiEvent.data.hook_int.nTime =
            DXS_ALM_ElapsedTimeSinceLastHook (pCh, pSddEvt->TIME_STAMP);
		 pCh->sddEvent |= (1 << EVT_SDD_ONH);
		 pCh->sddEvent &= ~(1 << EVT_SDD_OFFH);
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_OFFH:
#ifdef DXS_FEAT_CID
         /* call the caller ID state handler if CID transmission is ongoing */
         if (TAPI_Cid_IsActive(pCh->pTapiCh))
         {
            DXS_CID_SH (pCh, DXS_CID_EVT_OFFHOOK);
         }
#endif /* DXS_FEAT_CID */
         /* if no transmission of DTMF digits is ongoing, DXS_DTMF_SH returns
            immediately */
         DXS_DTMF_SH (pCh, DXS_DTMF_EVT_STOP);
         /* Activate the dtmf receiver. */
         DXS_DTMF_REC_CTRL(pCh, IFX_TRUE);
         /* TAPI event */
         tapiEvent.id = IFX_TAPI_EVENT_FXS_OFFHOOK_INT;
         tapiEvent.data.hook_int.nTime =
            DXS_ALM_ElapsedTimeSinceLastHook (pCh, pSddEvt->TIME_STAMP);
		 pCh->sddEvent |= (1 << EVT_SDD_OFFH);
		 pCh->sddEvent &= ~(1 << EVT_SDD_ONH);
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;
      case EVT_SDD_CORCE:
         break;

      case EVT_SDD_COEFE:
         break;

      default:
         break;
   }
}


/**
   Handle the SIG event

   \param  pDev         Pointer to the device structure.
   \param  pSddEvent    Pointer to the SIG event structure.
*/
static IFX_void_t dxs_event_sig(
                           DXS_DEVICE_t *pDev,
                           struct __fw_evt_sig *pSigEvt)
{
   DXS_CHANNEL_t *pCh = &pDev->pChannel[pSigEvt->hdr.CHAN];
   IFX_TAPI_EVENT_t  tapiEvent;

   if (pSigEvt->hdr.CHAN >= DXS_MAX_CH_NR)
   {
      TRACE(DXS, DBG_LEVEL_HIGH,
            ("DXS FW error, reporting SIG event on wrong channel %d\n",
            pSigEvt->hdr.CHAN));
      return;
   }

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      TRACE(DXS, DBG_LEVEL_LOW,
          ("WARNING: Skipping DXS SIG Event on not initialized driver.\n"));
      return;
   }

   memset(&tapiEvent, 0, sizeof(tapiEvent));
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

   switch (pSigEvt->SIGEVT)
   {
      case EVT_SIG_DTMF_DET:
         {
            IFX_uint8_t nKey = pSigEvt->DTMF_KEY;

            /* translate fw-coding to tapi enum coding */
            tapiEvent.data.dtmf.digit = DXS_DTMF_encode_fw2tapi(nKey);
            /* translate fw-coding to ascii charater */
            tapiEvent.data.dtmf.ascii =
               (IFX_uint8_t)DXS_DTMF_encode_fw2ascii(nKey);

            /* DTMF tone can only be received from ALM side */
#ifdef TAPI_VERSION3
            tapiEvent.data.dtmf.local = 1;
#endif /* TAPI_VERSION3 */
#ifdef TAPI_VERSION4
            tapiEvent.data.dtmf.external = 1;
#endif /* TAPI_VERSION4 */
            tapiEvent.id = IFX_TAPI_EVENT_DTMF_DIGIT;
            IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         }
         break;

#ifdef DXS_FEAT_CID
      case EVT_SIG_CIS_REQ:
         /* CIS data request: send one more byte of data  (CID_Sender_Data) */
         /* call the caller ID state handler */
         if (DXS_statusOk != DXS_CID_SH (pCh, DXS_CID_EVT_CIS_REQ))
         {
            TRACE(DXS, DBG_LEVEL_HIGH,
                ("DXS driver error, DXS_CID_SH() failed\n"));
         }
         break;

      case EVT_SIG_CIS_BUF:
         {
            IFX_int8_t nRemainingCIDBytes;

            /* CIS buffer underflow: set CID transmit error and abort sending */
            /* Fill event structure. */
            nRemainingCIDBytes = DXS_CID_GetRemainingBytes(pCh);
            if(nRemainingCIDBytes == 0)
            {
               /* no more data to send */
               tapiEvent.id = IFX_TAPI_EVENT_CID_TX_END;
            }
            else if (nRemainingCIDBytes > 0)
            {
               /* there is more data to send */
               tapiEvent.id = IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR;
            }
            else
            {
               /* negative value should never happen */
               TRACE(DXS, DBG_LEVEL_HIGH,
                   ("DXS driver error, negative remaining CID bytes %d\n",
                     nRemainingCIDBytes));
               return;
            }

            IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);

            /* call the caller ID state handler */
            DXS_CID_SH (pCh, DXS_CID_EVT_CIS_BUF);
         }
         break;

      case EVT_SIG_CIS_FIN:
         break;
#endif /* DXS_FEAT_CID */

#ifdef DXS_FEAT_UTD
      case EVT_SIG_UTD_START:
         tapiEvent.id = IFX_TAPI_EVENT_TONE_DET_CPT;
         tapiEvent.data.tone_det.index = DXS_UTD_ToneIdxGet(pCh);
         /* Translate direction to ALM perspective. */
#ifdef TAPI_VERSION3
         tapiEvent.data.tone_det.local = 1;
         /** \todo handle external direction */
#endif /* TAPI_VERSION3 */
#ifdef TAPI_VERSION4
         tapiEvent.data.tone_det.external = 1;
         /** \todo handle external direction */
#endif /* TAPI_VERSION4 */
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SIG_UTD_END:
         tapiEvent.id = IFX_TAPI_EVENT_TONE_DET_CPT_END;
         tapiEvent.data.tone_det.index = DXS_UTD_ToneIdxGet(pCh);
         /* Translate direction to ALM perspective. */
#ifdef TAPI_VERSION3
         tapiEvent.data.tone_det.local = 1;
         /** \todo handle external direction */
#endif /* TAPI_VERSION3 */
#ifdef TAPI_VERSION4
         tapiEvent.data.tone_det.external = 1;
         /** \todo handle external direction */
#endif /* TAPI_VERSION4 */
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;
#endif /* DXS_FEAT_UTD */

#ifdef DXS_FEAT_METERING
      case EVT_SIG_TTX_FIN:
#if 0
         obx_DXS_SIG_MeterPulseStatusClear(pCh);
#endif
         tapiEvent.id = IFX_TAPI_EVENT_METERING_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;
#endif /* DXS_FEAT_METERING */

      case EVT_SIG_AC_LM_FIN:
#ifdef DXS_FEAT_NLT
         DXS_OS_EventWakeUp (&pCh->pALM->aclm_event);
#endif
         break;

      default:
         break;
   }
}


/**
   Function dxs_outbox_handler

   \param  pDev         Pointer to the device structure.

   \return Always DXS_statusOk.
*/
IFX_int32_t DXS_outbox_handler (DXS_DEVICE_t *pDev)
{
   IFX_uint8_t words16, words32, msg_len/*in 32-bit words*/, tmp;
   /* 16 bit index points to the header of message that has been read out */
   IFX_uint8_t pos1;

   /* release protection for concurrent tasks and interrupts */
   DXS_FW_DL_PROTECT (pDev);

   /* TODO:- take additional mutex (trylock ???) - concurrency with CmdRead() */
   /* TODO: check if SPI works */

   /* read the outbox */
   DXS_ObxRead(pDev, pDev->dmbx, &words16);

   if (words16 == 0)
   {
      /* no data is strange but not an error */
      /* release protection for concurrent tasks and interrupts */
      DXS_FW_DL_RELEASE (pDev);
      return DXS_statusOk;
   }
   if (words16 & 1)
   {
      /* TODO: is this an error ? */

      TRACE(DXS, DBG_LEVEL_HIGH,
            ("OutMbx: Odd word count: %d\n", words16));
      /* release protection for concurrent tasks and interrupts */
      DXS_FW_DL_RELEASE (pDev);
      return DXS_statusOk;
   }

   words32 = words16 >> 1;
   pos1 = 0;

   while ((pos1/2) < words32)
   {
      IFX_uint32_t message[8], idx, i;
      struct __fw_evt_header *pHdr = (struct __fw_evt_header *)message;

      /* get only the message header */
      message[0] = (*(pDev->dmbx+pos1) << 16) | (*(pDev->dmbx+pos1+1));

      /* get the length of the message in 32-bit words */
      msg_len = (pHdr->LENGTH + sizeof(struct __fw_evt_header)) >> 2;

      if (msg_len > words32)
      {
         /* read 2nd segment */

         /* possibly here we need a small delay */

         /* read the outbox one more time to get the full message */
         DXS_ObxRead(pDev, pDev->dmbx + words16, &tmp);

         /* here we don't expect that nothing is read */

         words16 += tmp;
         words32 = words16 >> 1;

         if (msg_len > words32)
         {
            /* if read data is still not sufficient, we are out of sync.
               The concept is to reset the mailbox */
            TRACE(DXS, DBG_LEVEL_HIGH,
                  ("Mailbox out of sync.\n"));
            /* TODO: print the read buffer, reset the mailbox */
            break;
         }
      }

      /* fetch the entire message */
      for (idx=1, i=2; idx<msg_len; idx++, i+=2)
         message[idx] = (*(pDev->dmbx+pos1+i) << 16) | (*(pDev->dmbx+pos1+i+1));

      /* decode message, buffer it, advance to the next */
      if (pHdr->Res00 == 0 &&
          pHdr->CMD == 9 &&
          (pHdr->MOD == 1 || pHdr->MOD == 7) &&
          (pHdr->LENGTH == 0 || pHdr->LENGTH == 4 || pHdr->LENGTH == 8))
      {
         /* this is an event header */
         dxs_handle_event(pDev, pHdr);
      }
      else
      {
         struct DXS_FW_Cmd_Header *pCmdHdr = (struct DXS_FW_Cmd_Header *)message;


         if ((pCmdHdr->RW == 1) &&
            ((pCmdHdr->CMD == DXS_CMD_CMD_SDD) ||
             (pCmdHdr->CMD == DXS_CMD_CMD_EOP)) &&
            ((pCmdHdr->CHAN == DXS_CMD_CHAN_A) ||
             (pCmdHdr->CHAN == DXS_CMD_CHAN_B)) &&
            ((pCmdHdr->MOD == DXS_CMD_MOD_GPIO) ||
             (pCmdHdr->MOD == DXS_CMD_MOD_PCM) ||
             (pCmdHdr->MOD == DXS_CMD_MOD_SIG_GEN) ||
             (pCmdHdr->MOD == DXS_CMD_MOD_SIG_DET) ||
             (pCmdHdr->MOD == DXS_CMD_MOD_SYS)))
         {
            /* this is a command header */
            int i;
            uint32_t *pw32;

            for (i=0, pw32 = (uint32_t *)pHdr; i<msg_len; i++, pw32++)
            {
               fifo_put(pDev->cmd_obx_queue, NULL, *pw32);
            }

            /* wake up sleeping CmdRead */
            pDev->bOutBoxData = IFX_TRUE;
            DXS_OS_EventWakeUp (&pDev->obxDataEvt);
         }
         else
         {
            /* Send outbox header error event to TAPI. */
            IFX_TAPI_EVENT_t  tapiEvent = {0};
            tapiEvent.id = IFX_TAPI_EVENT_FAULT_FW_OBX_HDR_ERR;
            DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);
            IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
         }
      }

      pos1 += (msg_len * 2);
#ifndef DXS_IRQ_LEVEL_TRIGGERED
      /* If the IRQ is edge triggered we have to make sure that the
         mailbox is empty when we exit the loop. Do another read and exit
         only if mailbox is empty. */
      if (pos1 >= words16)
      {
         DXS_ObxRead(pDev, pDev->dmbx, &words16);
         if (words16 == 0)
            break;
         if (words16 & 1)
         {
            /* TODO: is this an error ? */
            TRACE(DXS, DBG_LEVEL_HIGH,
                  ("OutMbx: Odd word count: %d\n", words16));
            break;
         }
         words32 = words16 >> 1;
         pos1 = 0;
      }
#endif
   }

   /* release protection for concurrent tasks and interrupts */
   DXS_FW_DL_RELEASE (pDev);

   return DXS_statusOk;
}


/**
   Function dxs_outbox_handler_init

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_status_t
*/
IFX_int32_t DXS_outbox_handler_init(DXS_DEVICE_t *pDev)
{
   /* Init event that indicates that a command packet arrived in mailbox. */
   DXS_OS_EventInit (&pDev->obxDataEvt);
   pDev->bOutBoxData = IFX_FALSE;

   pDev->cmd_obx_queue = fifo_init (64);
   if (NULL == pDev->cmd_obx_queue)
   {
      /* errmsg: Failed to create fifo for oubox handling. */
      return DXS_statusMsgQueueCreatError;
   }

   return DXS_statusOk;
}


/**
   Function dxs_outbox_handler_exit

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_status_t
*/
IFX_int32_t DXS_outbox_handler_exit(DXS_DEVICE_t *pDev)
{
   fifo_destroy(pDev->cmd_obx_queue);

   DXS_OS_EventDelete (&pDev->obxDataEvt);

   return IFX_SUCCESS;
}
