/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_cid.c
   This file contains the implementation of the functions for CID operations.
*/

/* ========================================================================= */
/*                                 Includes                                  */
/* ========================================================================= */

#include "drv_dxs_config.h"
#include "drv_dxs_api.h"

#ifdef DXS_FEAT_CID
#include "drv_dxs_cid_priv.h"
#include "drv_dxs_cid.h"
#include "drv_dxs_dtmf_priv.h"
#include "drv_dxs_dtmf.h"
#include "drv_dxs_pcm.h"
#include "drv_dxs_mbx.h"

/* ========================================================================= */
/*                             Macro definitions                             */
/* ========================================================================= */

/* ========================================================================= */
/*                             Type definitions                              */
/* ========================================================================= */

/* ========================================================================= */
/*                             Global variables                              */
/* ========================================================================= */
/*
  0dB,   -1dB,    -2dB,   -3dB,   -4dB,   -5dB,   -6dB,   -7dB,   -8dB,   -9dB,
-10dB,  -11dB,   -12dB,  -13dB,  -14dB,  -15dB,  -16dB,  -17dB,  -18dB,  -19dB,
-20dB,  -21dB,   -22dB,  -23dB,  -24dB,  -25dB,  -26dB,  -27dB,  -28dB,  -29dB,
-30dB,  -31dB,   -32dB,  -33dB,  -34dB,  -35dB,  -36dB,  -37dB,  -38dB,  -39dB,
-40dB,  -41dB,   -42dB,  -43dB,  -44dB,  -45dB,  -46dB,  -47dB,  -48dB,  -49dB,
-50dB,  -51dB,   -52dB,  -53dB,  -54dB,  -55dB,  -56dB,  -57dB,  -58dB,  -59dB,
-60dB,  -61dB,   -62dB,  -63dB,  -64dB,  -65dB,  -66dB,  -67dB,  -68dB,  -69dB,
-70dB,  -71dB,   -72dB,  -73dB,  -74dB,  -75dB,  -76dB,  -77dB,  -78dB,  -79dB,
-80dB,  -81dB,   -82dB,  -83dB,  -84dB,  -85dB,  -86dB,  -87dB,  -88dB,  -89dB,
-90dB,  -91dB,   -92dB,  -93dB,  -94dB,  -95dB,  -96dB
*/
static const IFX_uint16_t cid_tx_levels[] =
{0x7FFF, 0x7214, 0x65AC, 0x5A9D, 0x50C3, 0x47FA, 0x4026, 0x392C, 0x32F5, 0x2D6A,
 0x287A, 0x2413, 0x2027, 0x1CA8, 0x198A, 0x16C3, 0x1449, 0x1214, 0x101D, 0x0E5D,
 0x0CCD, 0x0B68, 0x0A2B, 0x0910, 0x0813, 0x0733, 0x066A, 0x05B8, 0x0518, 0x048B,
 0x040C, 0x039B, 0x0337, 0x02DE, 0x028E, 0x0247, 0x0207, 0x01CF, 0x019D, 0x0170,
 0x0148, 0x0124, 0x0104, 0x00E8, 0x00CF, 0x00B8, 0x00A4, 0x0092, 0x0082, 0x0074,
 0x0068, 0x005C, 0x0052, 0x0049, 0x0041, 0x003A, 0x0034, 0x002E, 0x0029, 0x0025,
 0x0021, 0x001D, 0x001A, 0x0017, 0x0015, 0x0012, 0x0010, 0x000F, 0x000D, 0x000C,
 0x000A, 0x0009, 0x0008, 0x0007, 0x0007, 0x0006, 0x0005, 0x0005, 0x0004, 0x0004,
 0x0003, 0x0003, 0x0003, 0x0002, 0x0002, 0x0002, 0x0002, 0x0001, 0x0001, 0x0001,
 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001};

/* ========================================================================= */
/*                           Function prototypes                             */
/* ========================================================================= */
static IFX_int32_t DXS_CID_SetCidCoeff (DXS_CHANNEL_t *pCh,
                                        IFX_TAPI_CID_HOOK_MODE_t cidHookMode,
                                        IFX_TAPI_CID_FSK_CFG_t *pFskConf);


/* ========================================================================= */
/*                         Function implementation                           */
/* ========================================================================= */
/**
   Disables or Enables CID Sender according to bEn

   \param   pCh  - pointer to DXS channel structure

   \param   bEn  - IFX_TRUE : enable / IFX_FALSE : disable

   \return
   - DXS_statusOk
   - DXS_statusCidCtrlErr
*/
IFX_int32_t DXS_CID_CidGenCtrl (DXS_CHANNEL_t *pCh, IFX_boolean_t bEn)
{
   IFX_int32_t          err   = IFX_SUCCESS;
   DXS_DEVICE_t         *pDev = pCh->pParent;
   DXS_CID_GEN_CTRL_t   *pCidGenCtrl = &pCh->pCID->cid_gen_ctrl;

   if ((bEn == IFX_TRUE) && (pCidGenCtrl->EN == CID_GEN_CTRL_EN_DIS))
   {
      pCidGenCtrl->EN = CID_GEN_CTRL_EN_EN;

      /* only when cid is activated, bellcore or ITU spec is set */
      if (pCh->pCID->cidSend.nCidDataType == IFX_TAPI_CID_DATA_TYPE_FSK_BEL202)
         pCh->pCID->cid_gen_ctrl.V23 = CID_GEN_CTRL_V23_V23_BEL202;
      else
         pCh->pCID->cid_gen_ctrl.V23 = CID_GEN_CTRL_V23_V23_ITU_T;

      /* mute the voice path */
      err = DXS_PCM_ChRxMute (pCh, IFX_ENABLE);
   }
   else if ((bEn == IFX_FALSE) && (pCidGenCtrl->EN == CID_GEN_CTRL_EN_EN))
   {
      pCidGenCtrl->EN = CID_GEN_CTRL_EN_DIS;

      /* unmute the voice path */
      err = DXS_PCM_ChRxMute (pCh, IFX_DISABLE);
   }

   if (DXS_SUCCESS (err))
   {
      err = DXS_CmdWrite (pDev, (IFX_uint32_t *)(IFX_void_t *)pCidGenCtrl);
   }
   if (DXS_statusOk != err)
   {
      /* errmsg: Changing the state of the CID generator failed.*/
      RETURN_STATUS(DXS_statusCidCtrlErr, IFX_NULL);
   }
   else
   {
      RETURN_STATUS(DXS_statusOk, IFX_NULL);
   }
}

/**
   Set the CID sender coefficients

   \param pCh           pointer to DXS channel structure

   \param cidHookMode   cid hook mode as specified in IFX_TAPI_CID_HOOK_MODE_t

   \param pFskConf      handle to IFX_TAPI_CID_FSK_CFG_t structure

   \param nSize         size of CID data to send, used to set the BRS level.

   \return
   - DXS_statusOk
   - DXS_statusCidSetCoefErr

   \remark
   - The CID sender must be disabled before programming the coefficients, what
     is done in this function
*/
static IFX_int32_t DXS_CID_SetCidCoeff (DXS_CHANNEL_t *pCh,
                                        IFX_TAPI_CID_HOOK_MODE_t cidHookMode,
                                        IFX_TAPI_CID_FSK_CFG_t *pFskConf)
{
   IFX_int32_t          err = DXS_statusOk;
   DXS_DEVICE_t         *pDev = pCh->pParent;
   IFX_uint16_t         ch = (pCh->nChannel - 1);
   DXS_CID_GEN_COEF_t   *pCidGenCoef = &pCh->pCID->cid_gen_coef;

   /* protect fwmsg against concurrent tasks */
   DXS_OS_MutexGet (&pCh->mtxChAcc);

   /* to set the coefficients the cid generator must be inactive */
   err = DXS_CID_CidGenCtrl (pCh, IFX_FALSE);

   if (err == DXS_statusOk)
   {
      /* set the new coefficients */
      pCidGenCoef->CHAN    = ch;
      /* write all three parameters */
      pCidGenCoef->LENGTH  = CID_GEN_COEF_LENGTH;

      /* get coefficient from table cid_tx_levels */
      pCh->pCID->cid_gen_coef.LEVEL =
         cid_tx_levels[(-1) * pFskConf->levelTX / 10];

      /* set seizure and mark according to CID type */
      switch (cidHookMode)
      {
         /* offhook CID, called CID type 2, NTT inclusive */
         case  IFX_TAPI_CID_HM_OFFHOOK:
            /* set Seizure - off hook CID has always 0 seizure bits */
            pCidGenCoef->SEIZURE = 0;
            /* set Mark */
            pCidGenCoef->MARK = pFskConf->markTXOffhook;
            break;
         /* onhook CID, called CID type 1, NTT inclusive */
         case IFX_TAPI_CID_HM_ONHOOK:
            /* set Seizure */
            pCidGenCoef->SEIZURE = pFskConf->seizureTX;
            /* set Mark */
            pCidGenCoef->MARK = pFskConf->markTXOnhook;
            break;
         default:
            break;
      }
      err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pCidGenCoef);
   }
   if (DXS_statusOk != err)
   {
      TRACE (DXS,DBG_LEVEL_HIGH, ("CID Coefficients setting failed\n"));
   }

   /* remove protection of fwmsg against concurrent tasks */
   DXS_OS_MutexRelease (&pCh->mtxChAcc);

   if (DXS_statusOk != err)
   {
      /* errmsg: Setting coefficients for CID failed.*/
      RETURN_STATUS(DXS_statusCidSetCoefErr,IFX_NULL);
   }
   else
   {
      RETURN_STATUS(DXS_statusOk,IFX_NULL);
   }
}

/**
   Start CID data transmission

   This function is non blocking. It handles all necessary steps to transmit
   CID data by either DTMF or FSK. It returns an error if a CID transmission
   is already running.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pCidData     Pointer to CID TX configuration structure.

   \return Return value according to DXS_status_t
   - DXS_statusOk
   - DXS_statusDtmfAct
   - DXS_statusDtmfTimingErr
   - DXS_statusCidStdNotSupported
   - DXS_statusCidStartSeqErr

   \remarks
   This function is non blocking. It handles all required steps to transmit a
   CID. It returns an error if a CID transmission is already running.
*/
IFX_int32_t DXS_TAPI_LL_CID_TX_Start (IFX_TAPI_LL_CH_t *pLLChannel,
                                      IFX_TAPI_CID_TX_t const *pCidData)
{
   DXS_CHANNEL_t  *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t    err   = DXS_statusOk;

   /* remember the data type for stopping the correct machine */
   pCh->pCID->cidSend.nCidDataType = pCidData->cidDataType;

   switch (pCidData->cidDataType)
   {
      case IFX_TAPI_CID_DATA_TYPE_FSK_BEL202:
      case IFX_TAPI_CID_DATA_TYPE_FSK_V23:
         if (pCh->pDTMF->dtmf_at_gen_ctrl.EN == DTMF_AT_GEN_CTRL_EN_EN)
         {
            TRACE (DXS, DBG_LEVEL_LOW, ("\nDRV_ERROR: Cid Sender "
                    "cannot be used while DTMF/AT is active!\n"));
            /* errmsg: A DTMF transmission is active.*/
            RETURN_STATUS(DXS_statusDtmfAct, IFX_NULL);
         }

         if (pCh->pCID->cidSend.state_cid == DXS_CID_STATE_SETUP)
         {
            /* prevent data buffer overrun */
            if ( pCidData->nCidParamLen > IFX_TAPI_CID_TX_SIZE_MAX )
            {
               TRACE (DXS, DBG_LEVEL_LOW, ("\nDRV_ERROR: "
                      "Too much data for CID sender buffer - aborting\n"));
               /* errmsg: Parameter is out of range*/
               RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);
            }
            /* length in bytes */
            pCh->pCID->cidSend.nCidCnt = pCidData->nCidParamLen;
            memcpy(pCh->pCID->cidSend.pCid,
                   pCidData->pCidParam, pCidData->nCidParamLen);

            /* set callerID coefficients (level, seizure, mark bits) */
            err = DXS_CID_SetCidCoeff (pCh, pCidData->txHookMode,
                                       pCidData->pFskConf);

            /* now start non blocking low level machine */
            if (DXS_statusOk == err)
            {
               if (pCidData->cidDataType == IFX_TAPI_CID_DATA_TYPE_FSK_BEL202)
               {
                  /* Bellcore specification shall be used */
                  pCh->pCID->cid_gen_ctrl.V23 = CID_GEN_CTRL_V23_V23_BEL202;
               }
               else
               {
                  /* ITU-T V.23 specification shall be used */
                  pCh->pCID->cid_gen_ctrl.V23 = CID_GEN_CTRL_V23_V23_ITU_T;
               }
               /* call the caller ID state handler */
               err = DXS_CID_SH (pCh, DXS_CID_EVT_START);
            }
         }
         else
         {
            /* errmsg: A CID transmission is already active.*/
            RETURN_STATUS(DXS_statusCidAct, IFX_NULL);
         }
         break;

      case IFX_TAPI_CID_DATA_TYPE_DTMF:
      {
         IFX_uint8_t    pData[DXS_DTMF_MAX_BYTES];
         IFX_uint16_t   i;

         if (pCh->pDTMF->dtmfSend.state_dtmf == DXS_DTMF_STATE_SETUP)
         {
            /* check digit and interdigit times */
            if ((pCidData->pDtmfConf->digitTime > DXS_DTMF_MAX_DIGIT_TIME) ||
                (pCidData->pDtmfConf->interDigitTime > DXS_DTMF_MAX_INTERDIGIT_TIME))
            {
               TRACE (DXS, DBG_LEVEL_LOW, ("DXS_ERROR: Digit time or "
                    "inter digit time exceeds limit of 127 ms\n"));
               /* errmsg: DTMF digit or interdigit timing invalid.*/
               RETURN_STATUS(DXS_statusDtmfTimingErr, IFX_NULL);
            }

            /* prevent data buffer overrun */
            if ( pCidData->nCidParamLen > DXS_DTMF_MAX_BYTES )
            {
               TRACE (DXS, DBG_LEVEL_LOW, ("\nDRV_ERROR: "
                      "Too much data for CID sender buffer - aborting\n"));
               /* errmsg: Parameter is out of range*/
               RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);
            }
            /* store DTMF data in channel structure */
            pCh->pDTMF->dtmfSend.nDtmfCnt = pCidData->nCidParamLen;
            memcpy (pData,
                    pCidData->pCidParam, (IFX_uint32_t)pCidData->nCidParamLen);
            pCh->pDTMF->dtmfSend.digitTime       = pCidData->pDtmfConf->digitTime;
            pCh->pDTMF->dtmfSend.interDigitTime  = pCidData->pDtmfConf->interDigitTime;
            /*
               transcode DTMF digits to DUSLIC XS specific setting:
               for each digit a table index is retrieved, which can be used to get
               all required settings for the corresponding DTMF digit
            */
            for (i=0; i < pCh->pDTMF->dtmfSend.nDtmfCnt; i++)
               DXS_DTMF_GetTblIndex (pData[i], pData+i);

            memcpy (pCh->pDTMF->dtmfSend.pDtmfDigTblIndex, pData,
                    (IFX_uint32_t)pCh->pDTMF->dtmfSend.nDtmfCnt);

            err = DXS_DTMF_SH  (pCh, DXS_DTMF_EVT_START);
         }
         else
         {
            /* errmsg: A DTMF transmission is active.*/
            RETURN_STATUS(DXS_statusDtmfAct, IFX_NULL);
         }
         break;
      }
      default:
         /* errmsg: CID standard not supported.*/
         RETURN_STATUS(DXS_statusCidStdNotSupported, IFX_NULL);
   }

   if (DXS_SUCCESS (err))
   {
      return DXS_statusOk;
   }
   else
   {
      /* errmsg: Initiating a CID sequence failed */
      RETURN_STATUS(DXS_statusCidStartSeqErr, IFX_NULL);
   }
}

/**
   Stop CID data transmission

   \param pLLChannel      Handle to TAPI low level channel structure

   \return
   - DXS_statusCidStdNotSupported
   - DXS_statusCidTxStopErr
   - DXS_statusOk if successful
*/
IFX_int32_t DXS_TAPI_LL_CID_TX_Stop (IFX_TAPI_LL_CH_t *pLLChannel)
{
   IFX_int32_t    err   = DXS_statusOk;
   DXS_CHANNEL_t  *pCh  = (DXS_CHANNEL_t *) pLLChannel;

   TRACE (DXS,DBG_LEVEL_LOW, ("DXS_INFO: %s\n", __FUNCTION__));

   switch (pCh->pCID->cidSend.nCidDataType)
   {
      case IFX_TAPI_CID_DATA_TYPE_FSK_BEL202:
      case IFX_TAPI_CID_DATA_TYPE_FSK_V23:
         if (pCh->pCID->cidSend.state_cid == DXS_CID_STATE_TRANSMIT)
         {
            /* flag a fake offhook to stop the FSK state machine */
            err = DXS_CID_SH (pCh, DXS_CID_EVT_OFFHOOK);
         }
         break;
      case IFX_TAPI_CID_DATA_TYPE_DTMF:
         if ((pCh->pDTMF->dtmfSend.state_dtmf == DXS_DTMF_STATE_TRANSMIT) ||
             (pCh->pDTMF->dtmfSend.state_dtmf == DXS_DTMF_STATE_PAUSE))
         {
            /* stop dtmf transmission */
            err = DXS_DTMF_SH (pCh, DXS_DTMF_EVT_STOP);
         }
         break;
      default:
         /* errmsg: CID standard not supported.*/
         RETURN_STATUS(DXS_statusCidStdNotSupported,IFX_NULL);
   }

   if (DXS_SUCCESS (err))
      return err;
   else
      /* errmsg: CID Tx could not be stopped.*/
      RETURN_STATUS(DXS_statusCidTxStopErr, IFX_NULL);
}

/**
   Initalize the cached firmware message for the callerID generator.

   \param  pCh          Pointer to the channel structure.

   \return
   None.
*/
IFX_void_t DXS_CID_InitCh (DXS_CHANNEL_t *pCh)
{
   DXS_CID_GEN_DATA_t  *pCidGenData   = IFX_NULL;
   DXS_CID_GEN_COEF_t  *pCidGenCoef   = IFX_NULL;
   DXS_CID_GEN_CTRL_t  *pCidGenCtrl   = IFX_NULL;
   IFX_uint8_t          ch            = pCh->nChannel - 1;

   /* initialize FW message for activation/deactivation of dtmf/at generator */
   pCidGenCtrl = &pCh->pCID->cid_gen_ctrl;
   memset (pCidGenCtrl, 0, sizeof (DXS_CID_GEN_CTRL_t));

   pCidGenCtrl->CMD        = DXS_CMD_CMD_EOP;
   pCidGenCtrl->CHAN       = ch;
   pCidGenCtrl->MOD        = DXS_CMD_MOD_SIG_GEN;
   pCidGenCtrl->ECMD       = CID_GEN_CTRL_ECMD_EOP_CIDSEND;
   pCidGenCtrl->LENGTH     = CID_GEN_CTRL_LENGTH;
   pCidGenCtrl->EN         = CID_GEN_CTRL_EN_DIS;
   pCidGenCtrl->AD         = CID_GEN_CTRL_AD_OFF;
   pCidGenCtrl->HLEV       = CID_GEN_CTRL_HLEV_HLEV_HIGH;
   pCidGenCtrl->V23        = CID_GEN_CTRL_V23_V23_BEL202;

   /* initialize FW message for configuration of dtmf/at signal levels */
   pCidGenCoef = &pCh->pCID->cid_gen_coef;
   memset (pCidGenCoef, 0, sizeof (DXS_CID_GEN_COEF_t));

   pCidGenCoef->CMD        = DXS_CMD_CMD_EOP;
   pCidGenCoef->CHAN       = ch;
   pCidGenCoef->MOD        = DXS_CMD_MOD_SIG_GEN;
   pCidGenCoef->ECMD       = CID_GEN_COEF_ECMD_EOP_CIDS_COEFF;
   pCidGenCoef->LENGTH     = CID_GEN_COEF_LENGTH;
   pCidGenCoef->LEVEL      = CID_GEN_COEF_LEVEL_RESET;
   pCidGenCoef->SEIZURE    = CID_GEN_COEF_SEIZURE_RESET;
   pCidGenCoef->MARK       = CID_GEN_COEF_MARK_RESET;

   /* initialize FW message for configuration of dtmf/at frequencies */
   pCidGenData = &pCh->pCID->cid_gen_data;
   memset (pCidGenData, 0, sizeof (DXS_CID_GEN_DATA_t));

   pCidGenData->CMD     = DXS_CMD_CMD_EOP;
   pCidGenData->CHAN    = ch;
   pCidGenData->MOD     = DXS_CMD_MOD_SIG_GEN;
   pCidGenData->ECMD    = CID_GEN_DATA_ECMD_CIDS_DATA;
   pCidGenData->LENGTH  = 0;
}

/**
   Allocate data structures of the CID module for the given channel

   \param  pCh          Pointer to the channel structure.

   \return
   - DXS_statusOk
   - DXS_statusNoMem    in case the stucture could not be created

   \remarks The channel parameter is not checked because the calling
   function assures correct values.
*/
IFX_int32_t DXS_CID_Allocate_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   DXS_CID_Free_Ch_Structures (pCh);

   pCh->pCID = DXS_OS_MALLOC(sizeof(*pCh->pCID));
   if (pCh->pCID == IFX_NULL)
   {
      /* errmsg: No memory could be allocated. */
      RETURN_STATUS(DXS_statusNoMem, IFX_NULL);
   }
   memset(pCh->pCID, 0, sizeof(*pCh->pCID));

   return DXS_statusOk;
}

/**
   Free data structure of the CID module in the given channel.

   \param  pCh          Pointer to the channel structure.
*/
IFX_void_t DXS_CID_Free_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   if (pCh->pCID != IFX_NULL)
   {
      DXS_OS_FREE(pCh->pCID);
      pCh->pCID = IFX_NULL;
   }
}

/**
   Return the remaining byte count in the CID buffer.

   \param  pCh          Pointer to the channel structure.
   \return              Remaining byte count.
*/
IFX_int8_t DXS_CID_GetRemainingBytes (DXS_CHANNEL_t *pCh)
{
   IFX_int8_t remaining;

   /* protect fwmsg against concurrent tasks */
   DXS_OS_MutexGet (&pCh->mtxChAcc);
   remaining = (pCh->pCID->cidSend.nCidCnt - pCh->pCID->cidSend.nPos);
   DXS_OS_MutexRelease(&pCh->mtxChAcc);

   return remaining;
}

/* ========================================================================= */
/*                         Function pointer exports                          */
/* ========================================================================= */
#endif /* DXS_FEAT_CID */
