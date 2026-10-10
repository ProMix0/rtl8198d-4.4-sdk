/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_utd.c
   This file contains the implementation of the UTD (universal tone detector).
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_utd.h"

#ifdef DXS_FEAT_UTD


/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#if !defined(UTD_COEF_TABLE_MAX)
   #define UTD_COEF_TABLE_MAX  5
#endif /* UTD_COEF_TABLE_MAX */


/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
struct DXS_UTD {
   struct DXS_UTD_CTRL  fw_utd_ctrl;
   struct DXS_UTD_COEF  fw_utd_coef;
   IFX_uint16_t         nToneIdx;
};

struct UTD_COEF_TABLE
{
   /* Number of used slots in the coefficient table. */
   IFX_uint16_t nCount;
   /* Protects access to the coefficient table. */
   DXS_OS_mutex_t mtxAcc;
   struct {
      IFX_uint16_t nToneIndex;
      struct DXS_UTD_COEF_ENTRY utd_coef;
   } coef [UTD_COEF_TABLE_MAX];
};


/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
/* The UTD tone table is driver global. */
static struct UTD_COEF_TABLE *pUtdToneTable = IFX_NULL;


/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t dxs_UTD_CoeffGet (
                        IFX_uint16_t nToneIdx,
                        struct DXS_UTD_COEF_ENTRY *pUtdCoef);


/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Allocate UTD coefficient storage

   \return
   - DXS_statusOk if successful
   - DXS_statusNoMem if no memory could be allocated
*/
IFX_int32_t DXS_UTD_AllocateStorage (IFX_void_t)
{
   DXS_UTD_DeleteStorage ();

   pUtdToneTable = DXS_OS_MALLOC (sizeof (*pUtdToneTable));
   if (pUtdToneTable == IFX_NULL)
   {
      /* errmsg: No memory could be allocated. */
      return DXS_statusNoMem;
   }

   memset(pUtdToneTable, 0, sizeof (*pUtdToneTable));
   DXS_OS_MutexInit (&pUtdToneTable->mtxAcc);

   return DXS_statusOk;
}


/**
   Free allocated UTD coefficient storage
*/
IFX_void_t DXS_UTD_DeleteStorage (IFX_void_t)
{
   if (pUtdToneTable == IFX_NULL)
      return;

   DXS_OS_MutexDelete (&pUtdToneTable->mtxAcc);
   DXS_OS_FREE (pUtdToneTable);
   pUtdToneTable = IFX_NULL;
}


/**
   Allocate data structure for the UTD handling on the given channel.

   \param  pCh          Pointer to DXS channel structure.

   \return
   - DXS_statusOk
   - DXS_statusNoMem    in case the stucture could not be created

   \remarks The channel parameter is not checked because the calling
   function assures correct values.
*/
IFX_int32_t DXS_UTD_Allocate_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   DXS_UTD_Free_Ch_Structures (pCh);

   pCh->pUTD = DXS_OS_MALLOC(sizeof(*pCh->pUTD));
   if (pCh->pUTD == IFX_NULL)
   {
      /* errmsg: No memory could be allocated. */
      RETURN_STATUS(DXS_statusNoMem, IFX_NULL);
   }
   memset(pCh->pUTD, 0, sizeof(*pCh->pUTD));

   return DXS_statusOk;
}


/**
   Free data structure for the UTD handling on the given channel.

   \param  pCh          Pointer to DXS channel structure.
*/
IFX_void_t DXS_UTD_Free_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   if (pCh->pUTD != IFX_NULL)
   {
      DXS_OS_FREE(pCh->pUTD);
      pCh->pUTD = IFX_NULL;
   }
}


/**
   Initialise the cached firmware message for the UTD

   \param  pCh          Pointer to DXS channel structure.
*/
IFX_void_t DXS_UTD_InitCh (DXS_CHANNEL_t *pCh)
{
   struct DXS_UTD_CTRL  *pUtdCtrl = IFX_NULL;
   struct DXS_UTD_COEF  *pUtdCoef = IFX_NULL;
   IFX_uint8_t  ch = pCh->nChannel - 1;

   /* Initialize FW message for activation/deactivation of the UTD. */
   pUtdCtrl = &pCh->pUTD->fw_utd_ctrl;
   memset (pUtdCtrl, 0, sizeof(*pUtdCtrl));
   pUtdCtrl->CMD     = DXS_CMD_CMD_EOP;
   pUtdCtrl->CHAN    = ch;
   pUtdCtrl->MOD     = DXS_CMD_MOD_SIG_DET;
   pUtdCtrl->ECMD    = UTD_CTRL_ECMD;
   pUtdCtrl->LENGTH  = UTD_CTRL_LENGTH;
   pUtdCtrl->EN      = UTD_CTRL_EN_DIS;

   /* Initialize FW message for setting the UTD coefficients. */
   pUtdCoef = &pCh->pUTD->fw_utd_coef;
   memset (pUtdCoef, 0, sizeof(*pUtdCoef));
   pUtdCoef->CMD     = DXS_CMD_CMD_EOP;
   pUtdCtrl->CHAN    = ch;
   pUtdCoef->MOD     = DXS_CMD_MOD_SIG_DET;
   pUtdCoef->ECMD    = UTD_COEF_ECMD;
   pUtdCoef->LENGTH  = UTD_COEF_LEN;
}


/**
   Start the universal tone detector (UTD)

   If the UTD is already running it is automatically stopped, reconfigured
   and then started again.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pTone        Unused.
   \param  pSignal      Pointer to the signal configuration.

   \return
   - DXS_statusNotSupported UTD is not supported.
   - DXS_statusUtdDirError Direction specification is invalid.
   - DXS_statusUtdCoeffMissing Missing UTD coefficients for specified tone.
   - DXS_statusUtdCtrlErr Activation of UTD failed.
   - DXS_statusCmdWr Writing the command failed.
   - DXS_statusOk if successful
*/
/*lint -save -esym(715,pTone) */
IFX_int32_t DXS_TAPI_LL_UTD_Start (
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_TONE_SIMPLE_t const *pTone,
                        IFX_TAPI_TONE_CPTD_t const *pSignal)
{
   DXS_CHANNEL_t     *pCh  = (DXS_CHANNEL_t *)pLLChannel;
   DXS_DEVICE_t      *pDev = pCh->pParent;
   IFX_int32_t       ret = DXS_statusOk;
   struct DXS_UTD_CTRL  *pUtdCtrl = IFX_NULL;
   struct DXS_UTD_COEF  *pUtdCoef = IFX_NULL;
   struct DXS_UTD_COEF_ENTRY UtdCoef;

   DXS_ASSERT(pCh);

   /* Abort if FW does not support the universal tone detector. */
   if (!pDev->caps.bfw_UTD)
   {
      RETURN_STATUS (DXS_statusNotSupported, IFX_NULL);
   }

   DXS_ASSERT(pCh->pUTD);

   /* Verify that the direction parameter is valid. */
#ifdef TAPI_VERSION4
   if (((pSignal->external == 0) && (pSignal->internal == 0)) ||
       ((pSignal->external != 0) && (pSignal->internal != 0)))
#endif /* TAPI_VERSION4 */
#ifdef TAPI_VERSION3
   if ((pSignal->signal != IFX_TAPI_TONE_CPTD_DIRECTION_RX) &&
       (pSignal->signal != IFX_TAPI_TONE_CPTD_DIRECTION_TX))
#endif /* TAPI_VERSION3 */
   {
      /* errmsg: Direction specification for UTD activation is invalid. */
      RETURN_STATUS(DXS_statusUtdDirError, IFX_NULL);
   }

   /* retrieve UTD coefficients */
   ret = dxs_UTD_CoeffGet(pSignal->tone, &UtdCoef);
   if (!DXS_SUCCESS (ret))
   {
      RETURN_STATUS (ret, IFX_NULL);
   }

   /* Protect channel data from concurrent access. */
   DXS_OS_MutexGet (&pCh->mtxChAcc);

   pUtdCtrl = &pCh->pUTD->fw_utd_ctrl;

   /* Make sure the UTD is disabled before programming new coefficients. */
   if (pUtdCtrl->EN != UTD_CTRL_EN_DIS)
   {
      pUtdCtrl->EN = UTD_CTRL_EN_DIS;
      ret = DXS_CmdWrite(pDev, (IFX_uint16_t *)(IFX_void_t *)pUtdCtrl);
      if (!DXS_SUCCESS (ret))
      {
         /* Restore the last state. */
         pUtdCtrl->EN = UTD_CTRL_EN_EN;
         goto error;
      }
   }

   /* Write the UTD coefficients. */
   pUtdCoef = &pCh->pUTD->fw_utd_coef;

   pUtdCoef->BW = UtdCoef.BW;
   pUtdCoef->CF = UtdCoef.CF;
   pUtdCoef->NLEV = UtdCoef.NLEV;
   pUtdCoef->SLEV = UtdCoef.SLEV;
   pUtdCoef->DLEV = UtdCoef.DLEV;
   pUtdCoef->LPCOEF = UtdCoef.LPCOEF;
   pUtdCoef->DUP = UtdCoef.DUP;

   ret = DXS_CmdWrite(pDev, (IFX_uint16_t *)(IFX_void_t *)pUtdCoef);
   if (!DXS_SUCCESS (ret))
   {
      goto error;
   }

   /* Store the tone index because it is needed for the event reporting. */
   pCh->pUTD->nToneIdx = pTone->index;

   /* Translate the direction parameter into the FW message. */
#ifdef TAPI_VERSION4
   /* External means comming from ALM alias RX direction on the DXS. */
   pUtdCtrl->DIR = (pSignal->external == 1) ?
      UTD_CTRL_DIR_RX : UTD_CTRL_DIR_TX;
#endif /* TAPI_VERSION4 */
#ifdef TAPI_VERSION3
   pUtdCtrl->DIR = (pSignal->signal == IFX_TAPI_TONE_CPTD_DIRECTION_RX) ?
      UTD_CTRL_DIR_RX : UTD_CTRL_DIR_TX;
#endif /* TAPI_VERSION3 */

   /* Here the UTD is always disabled so enable it now. */
   pUtdCtrl->EN = UTD_CTRL_EN_EN;
   ret = DXS_CmdWrite(pDev, (IFX_uint16_t *)(IFX_void_t *)pUtdCtrl);
   if (!DXS_SUCCESS (ret))
   {
      /* Restore the last state. */
      pUtdCtrl->EN = UTD_CTRL_EN_DIS;
      /* errmsg: Activation of universal tone detector failed.*/
      ret = DXS_statusUtdCtrlErr;
   }

error:
   DXS_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}
/*lint -restore */


/**
   Stop the universal tone detector (UTD)

   The signal specificiation is ignored in this function.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pSignal      Unused.

   \return
   - DXS_statusNotSupported UTD is not supported.
   - DXS_statusCmdWr Writing the command failed
   - DXS_statusOk if successful
*/
/*lint -save -esym(715,pSignal) */
IFX_int32_t DXS_TAPI_LL_UTD_Stop (
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_TONE_CPTD_t const *pSignal)
{
   DXS_CHANNEL_t     *pCh = (DXS_CHANNEL_t *)pLLChannel;
   DXS_DEVICE_t      *pDev = pCh->pParent;
   DXS_UTD_CTRL_t    *pUtdCtrl = IFX_NULL;
   IFX_int32_t       ret = DXS_statusOk;

   DXS_ASSERT(pCh);

   /* Abort if FW does not support the universal tone detector. */
   if (!pDev->caps.bfw_UTD)
   {
      RETURN_STATUS (DXS_statusNotSupported, IFX_NULL);
   }

   DXS_ASSERT(pCh->pUTD);
   pUtdCtrl = &pCh->pUTD->fw_utd_ctrl;

   /* Protect channel data from concurrent access. */
   DXS_OS_MutexGet (&pCh->mtxChAcc);

   /* Deactivate the universal tone detector only if enabled. */
   if (pUtdCtrl->EN != UTD_CTRL_EN_DIS)
   {
      pUtdCtrl->EN = UTD_CTRL_EN_DIS;
      ret = DXS_CmdWrite(pDev, (IFX_uint16_t *)(IFX_void_t *)pUtdCtrl);
      if (!DXS_SUCCESS (ret))
      {
         /* Restore the last state. */
         pUtdCtrl->EN = UTD_CTRL_EN_EN;
      }
   }

   DXS_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}
/*lint -restore */


/**
   Return the index of the last programmed UTD coefficients

   \param  pCh          Pointer to DXS channel structure.
*/
IFX_uint16_t DXS_UTD_ToneIdxGet (DXS_CHANNEL_t *pCh)
{
   if (pCh && pCh->pUTD)
   {
      return pCh->pUTD->nToneIdx;
   }

   return 0;
}


/**
   Add universal tone detector coefficient to the table

   Existing entries with the same tone index will be updated with the provided
   coefficient. If the table is full this function will return with an error.

   \param  nToneIdx     Tone index.
   \param  pUtdCoef     Pointer to the new UTD coefficients.

   \return
   - DXS_statusOk if successful
   - DXS_statusUtdTableFull if there is no more free slot in the internal table
*/
IFX_int32_t DXS_UTD_CoeffAdd (
                        IFX_uint16_t nToneIdx,
                        struct DXS_UTD_COEF_ENTRY *pUtdCoef)
{
   IFX_int32_t ret = IFX_ERROR;
   IFX_uint16_t i;

   DXS_ASSERT (pUtdCoef);
   DXS_ASSERT (pUtdToneTable);

   DXS_OS_MutexGet (&pUtdToneTable->mtxAcc);

   /* Check used slots of the table if the tone index is already stored. */
   for (i = 0; i < pUtdToneTable->nCount; i++)
   {
      if (pUtdToneTable->coef[i].nToneIndex == nToneIdx)
      {
         break;
      }
   }

   if ((i >= pUtdToneTable->nCount) &&
       (pUtdToneTable->nCount < UTD_COEF_TABLE_MAX))
   {
      /* There is still an empty slot in the table so use it. */
      i = pUtdToneTable->nCount;
      pUtdToneTable->nCount++;
   }

   /* If a slot in the table is found store the coefficients. */
   if (i < UTD_COEF_TABLE_MAX)
   {
      pUtdToneTable->coef[i].nToneIndex = nToneIdx;
      memcpy (&pUtdToneTable->coef[i].utd_coef,
              pUtdCoef, /*lint !e64 */ sizeof (*pUtdCoef));
      ret = DXS_statusOk;
   }
   else
   {
      /* errmsg: UTD coefficents cannot be stored because the table is full. */
      ret = DXS_statusUtdTableFull;
   }

   DXS_OS_MutexRelease (&pUtdToneTable->mtxAcc);

   return ret;
}


/**
   Retrieve universal tone detector coefficients

   \param nToneIdx   Tone index
   \param pUtdCoef   Out Pointer to the UTD coefficients

   \return
   - DXS_statusOk coefficients were found for the given index
   - DXS_statusUtdCoeffMissing no coefficients were found for the given index
*/
static IFX_int32_t dxs_UTD_CoeffGet (
                        IFX_uint16_t nToneIdx,
                        struct DXS_UTD_COEF_ENTRY *pUtdCoef)
{
   IFX_uint16_t i;

   DXS_ASSERT (pUtdCoef);
   DXS_ASSERT (pUtdToneTable);

   DXS_OS_MutexGet (&pUtdToneTable->mtxAcc);

   for (i = 0; i < pUtdToneTable->nCount; i++)
   {
      if (pUtdToneTable->coef[i].nToneIndex == nToneIdx)
      {
         memcpy (pUtdCoef, &pUtdToneTable->coef[i].utd_coef, /*lint !e64 */
                 sizeof (*pUtdCoef));
         break;
      }
   }

   DXS_OS_MutexRelease (&pUtdToneTable->mtxAcc);

   if ( i >= pUtdToneTable->nCount )
   {
      /* errmsg: UTD coefficients for specified tone missing.*/
      return DXS_statusUtdCoeffMissing;
   }

   return DXS_statusOk;
}
#endif /* (DXS_FEAT_UTD) */
