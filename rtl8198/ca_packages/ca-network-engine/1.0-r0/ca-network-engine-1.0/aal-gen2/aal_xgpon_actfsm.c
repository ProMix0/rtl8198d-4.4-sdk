/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ca_types.h"
#include "osal_common.h"
#include "scfg.h"
#include "ca_event.h"
#include <soc/cortina/registers.h>
#include "aal_ngpon2_chmgmt.h"
#include "aal_xcvr.h"
#include "aal_ploam.h"
#include "aal_puc.h"
#if defined(CONFIG_CORTINA_BOARD_FPGA)
#include "aal_idp.h"
#else
#include "aal_psds.h"
#endif
#include "aal_xgpon.h"
#include "ploam_ngpon2.h"
#include "xgpon.h"


/***************************************************************************
**
** import variables and functions
**
****************************************************************************/
extern ca_uint8 aal_gpon_dg_pdu[];
extern void xgpon_aes_ploam_ack_mic_update(ca_device_id_t device_id);

/***************************************************************************
**
** macro and constant definition
**
****************************************************************************/
#define NEW_ACT_FSM 1

#define AAL_PON_MODE_IS_XGPON1(mode) ((mode) == ONU_PON_MAC_XGPON1)
#define AAL_PON_MODE_IS_XGSPON(mode) ((mode) == ONU_PON_MAC_XGSPON)
#define AAL_PON_MODE_IS_XGPON(mode)  ((AAL_PON_MODE_IS_XGPON1(mode)||AAL_PON_MODE_IS_XGSPON(mode)))
#define AAL_PON_MODE_IS_NGP2(mode)   ((mode) == ONU_PON_MAC_NGPON2)
#define AAL_PON_MODE_IS_25GPON(mode) ((mode) == ONU_PON_MAC_GPON)
#define AAL_PON_MODE_IS_GPON(mode)   (AAL_PON_MODE_IS_XGPON(mode)||AAL_PON_MODE_IS_NGP2(mode)||AAL_PON_MODE_IS_25GPON(mode))


#define XGPN_SYS_PROF_PTR(dev) (&g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile)
#define XGPN_BURST_PROF_PTR(dev, prof_id) (&g_ngpon2_channel_mgmt_cb.profile_record[0].burst_profile[prof_id])
#define XGPN_CHN_PROF_PTR(dev, prof_id) (&g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[prof_id])

#define AAL_XGPON_ACTFSM_DBG_IS_ON(flag)  ((g_aal_xgpon_actfsm_debug & (flag)) ? 1 : 0)

#define AAL_XGPON_ACTFSM_PRINT_IS_ON()        AAL_XGPON_ACTFSM_DBG_IS_ON(XGPON_TUNING_DEBUG_PRINT)
#define AAL_XGPON_ACTFSM_SFC0_DROP_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(XGPON_TUNING_DEBUG_SFC_DROP_SFC0)
#define AAL_XGPON_ACTFSM_ROLLBACK_CHK_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK)
#define AAL_XGPON_ACTFSM_CALIBRATION_CHK_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(XGPON_TUNING_DEBUG_CALIBRATION_FLAG_CHK)
#define AAL_XGPON_ACTFSM_RST_CLK_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(XGPON_TUNING_DEBUG_RESET_CLK_FOR_CHN)
#define AAL_XGPON_ACTFSM_RESTORE_TUNING_CLR_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(XGPON_TUNING_DEBUG_RESTORE_CLR_TUNING)
#define AAL_XGPON_ACTFSM_FORCE_TUNING_CLR_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(XGPON_TUNING_DEBUG_CLR_RX_TX_TUNING_FORCE)
#define AAL_XGPON_ACTFSM_FORCE_SYNC_HUNT_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(XGPON_TUNING_DEBUG_SYNC_HUNT_FORCE)
#define AAL_XGPON_ACTFSM_ROLLBACK2TARGET_CHN_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(AAL_XGPON_TUNING_DEBUG_ROLLBACK_TO_TARGET_CHN)
#define AAL_XGPON_ACTFSM_CHN_SCAN_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(AAL_XGPON_ACTFSM_DEBUG_CHN_SCAN_ENA)
#define AAL_XGPON_ACTFSM_CHK_ONUID_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(AAL_XGPON_ACTFSM_DEBUG_ONU_ID_CHK_ENA)
#define AAL_XGPON_ACTFSM_CHK_ALLOCID_IS_ON()    AAL_XGPON_ACTFSM_DBG_IS_ON(AAL_XGPON_ACTFSM_DEBUG_ALLOC_ID_CHK_ENA)



#ifdef GPON_DEBUG
#define XGPN_ACTFSM_LOG_CRI(fmt,...)  \
        ca_printf("%s()::%d:"fmt, __FUNCTION__,__LINE__,##__VA_ARGS__);
#define XGPN_ACTFSM_LOG_ERR(fmt,...)  \
        ca_printf("%s()::%d:"fmt, __FUNCTION__,__LINE__,##__VA_ARGS__);
#define XGPN_ACTFSM_LOG_WARN(fmt,...)  \
        ca_printf("%s()::%d:"fmt, __FUNCTION__,__LINE__,##__VA_ARGS__);
#define XGPN_ACTFSM_LOG_NOTE(fmt,...)  \
        ca_printf("%s()::%d:"fmt, __FUNCTION__,__LINE__,##__VA_ARGS__);
#define XGPN_ACTFSM_LOG_INFO(fmt,...)  \
        ca_printf("%s()::%d:"fmt, __FUNCTION__,__LINE__,##__VA_ARGS__);
#define XGPN_ACTFSM_LOG_DBG(fmt,...)  \
        ca_printf("%s()::%d:"fmt, __FUNCTION__,__LINE__,##__VA_ARGS__);
#else

#define XGPN_ACTFSM_LOG_CRI(fmt,...)  \
        cap_log_msg(AAL_DEFAUL_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_CRITICAL, "%s():"fmt, __FUNCTION__,##__VA_ARGS__);

#define XGPN_ACTFSM_LOG_ERR(fmt,...)  \
        cap_log_msg(AAL_DEFAUL_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_ERROR, "%s():"fmt, __FUNCTION__,##__VA_ARGS__);

#define XGPN_ACTFSM_LOG_WARN(fmt,...)  \
        cap_log_msg(AAL_DEFAUL_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_WARNING, "%s():"fmt, __FUNCTION__,##__VA_ARGS__);

#define XGPN_ACTFSM_LOG_NOTE(fmt,...)  \
        cap_log_msg(AAL_DEFAUL_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_NOTICE, "%s():"fmt, __FUNCTION__,##__VA_ARGS__);

#define XGPN_ACTFSM_LOG_INFO(fmt,...)  \
        cap_log_msg(AAL_DEFAUL_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_INFO, "%s():"fmt, __FUNCTION__,##__VA_ARGS__);

#define XGPN_ACTFSM_LOG_DBG(fmt,...)  \
        cap_log_msg(AAL_DEFAUL_DEV_ID, CA_MOD_GPON_DRV, CAP_LOG_DEBUG, "%s()::%d:"fmt, __FUNCTION__, __LINE__,##__VA_ARGS__);

#endif


#define FOR_ALL_TWDM_CHN_DO(chn) \
        for ((chn) = 0; (chn) < AAL_XGPON_WAVE_CHANNEL_MAX_NUM; (chn) ++)

//#define XGPON_WORK_AROUND_ENABLE 1
ca_uint32_t g_xgpon_work_around = 0;

static ca_uint32_t  g_xgpn_act_fsm_intr_cnt  = 0;
static ca_uint32_t  g_xgpn_act_fsm_run_cnt   = 0;
static ca_uint32_t  g_xgpn_act_fsm_cur_state = 0;
static ca_uint32_t  g_xgpn_act_fsm_pre_state = 0;
static ca_uint32_t  g_xgpn_act_fsm_cur_evt   = 0;
static ca_uint32_t  g_xgpn_act_fsm_handle_fail_cnt = 0;
static ca_uint64_t  g_xgpn_act_time_tick = 0;
static ca_uint32_t  g_xgpn_act_count     = 0;
static ca_uint32_t  g_xgpn_act_fsm_outsync_cnt = 0;
static ca_uint8_t  cur_prof = 0;

#ifdef GPON_DEBUG
ca_uint32_t g_aal_xgpon_actfsm_debug =
    XGPON_TUNING_DEBUG_PRINT |
    XGPON_TUNING_DEBUG_RESET_CLK_FOR_CHN |
    XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK |
    AAL_XGPON_ACTFSM_DEBUG_ONU_ID_CHK_ENA |
    AAL_XGPON_ACTFSM_DEBUG_ALLOC_ID_CHK_ENA |
    AAL_XGPON_ACTFSM_DEBUG_CHN_SCAN_ENA;

#else
ca_uint32_t g_aal_xgpon_actfsm_debug = 
    XGPON_TUNING_DEBUG_PRINT | 
    XGPON_TUNING_DEBUG_RESET_CLK_FOR_CHN | 
    XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK;
#endif

//todo:
ca_uint32_t xfp_op_delay = 10;

#define US_DS_CHANEL 1


#define __GET_CHN_FROM_PONID(ponid)  ((ponid)&0x0F)
#define __XGPN_MOD_ID    CA_MOD_DRVR_DEV
#define __AAL_XGPN_ERROR    XGPN_ACTFSM_LOG_ERR

#define __AAL_XGPN_LOG  XGPN_ACTFSM_LOG_CRI

ca_uint __aal_xgpon_act_lock = 0;

#define __AAL_XGPN_ACT_LOCK(dev)           ca_spin_lock(__aal_xgpon_act_lock)
#define __AAL_XGPN_ACT_UNLOCK(dev)         ca_spin_unlock(__aal_xgpon_act_lock)
extern ca_uint32_t tcont_num_per_chip;
extern ca_uint32_t g_scheduled_sfc;
extern ca_int32_t aal_xgpn_simulation_flag;
extern ca_int32_t aal_xgpn_protect_switch_sim_flag;
extern ca_uint8_t  g_tun_ctr_seqno;
extern ca_uint8_t  g_ds_pon_id ;
extern ca_uint8_t  g_us_pon_id ;

ca_boolean_t g_ngpon2_chn_scan_ena = FALSE;
ca_uint8_t   g_ngpon2_chn_scan_idx = 0;
ca_uint32_t  g_ngpon2_cpi_timeout = 5 * 60 * 1000; /* unit: ms */

#ifdef NGPON2_FSM_DEBUG

/* Common ACTFSM MACROs for input check */
#define NGPON2_MODE_CHK_RET(ret)  do { \
            if (ONU_PON_MAC_NGPON2 != aal_pon_mac_mode_get(0)) \
                return (ret); \
            }while(0)
#else
#define NGPON2_MODE_CHK_RET(ret)

#endif


#define ACT_FSM_HW_STATE_CHK_RET(x,ret)  do { \
            if (!(x)) { \
                OS_PRINTF("%s(): state check FAIL: %s\n", __FUNCTION__, #x); \
                return (ret); \
            }\
        }while(0)

#define ACT_FSM_SET_STATE(state) do { (g_xgpn_act_state) = (state); \
                                    XGPN_ACTFSM_LOG_WARN("Current state: %s\r\n", xgpn_act_state[g_xgpn_act_state]); \
                                 }while(0)


#define ACT_FSM_RESET_BURST_PROF() do { \
            memset(g_ngpon2_channel_mgmt_cb.profile_record[0].burst_profile, \
            0,  CA_XGPON_MAX_BURST_PROFILE_COUNT*sizeof(ca_xgpn_burst_profile_t)); \
        }while(0)

#define ACT_FSM_RESET_CHANNEL_PROF() do { \
            memset(g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile, \
            0, CA_XGPON_MAX_CHANNEL_PROFILE_COUNT * sizeof(ca_xgpn_twdm_channel_profile_t)); \
        }while(0)

#define ACT_FSM_RESET_SYS_PROF() do { \
            memset(&g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile, \
            0,  sizeof(ca_xgpn_sys_profile_t)); \
        }while(0)


#define ACT_FSM_SET_HW_PROF_LEARN(x)   \
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_STS, profile_learning, (x) & 0x3)

#define ACT_FSM_SET_HW_WLCP_ENA(x)   \
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_CTRL, wlcp_enable, (x) & 0x1)


#define ACT_FSM_TRIG_HW_PROF_LEAN() ACT_FSM_SET_HW_PROF_LEARN(AAL_XGPON_ACTP_PROF_LEARNING)
#define ACT_FSM_TRIG_HW_DWLCH_OK() ACT_FSM_SET_HW_PROF_LEARN(AAL_XGPON_ACTP_PROF_DWLCH_OK)
#define ACT_FSM_TRIG_HW_DWLCH_FAIL() ACT_FSM_SET_HW_PROF_LEARN(AAL_XGPON_ACTP_PROF_DWLCH_FAIL)

#define ACT_FSM_SET_HW_TX_TUNING(x)   \
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_STS, tx_optical_tuning, (x) & 0x1)
#define ACT_FSM_TRIG_HW_TX_TUNING() ACT_FSM_SET_HW_TX_TUNING(0)

#define ACT_FSM_SET_HW_RX_TUNING(x)   \
        AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_STS, rx_optical_tuning, (x) & 0x1)
#define ACT_FSM_TRIG_HW_RX_TUNING() ACT_FSM_SET_HW_RX_TUNING(0)

//#define ACT_FSM_XCVR_RX_CHN_TUNNING(d) aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL)
//#define ACT_FSM_XCVR_TX_CHN_TUNNING(u) aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL)
//#define ACT_FSM_XCVR_TXRX_CHN_TUNNING(d,u) aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL)

#define ACT_FSM_IS_ROLLBACKING()         (g_act_fsm_is_rollbacking)
#define ACT_FSM_ENTER_ROLLBACKING()      (g_act_fsm_is_rollbacking = 1)
#define ACT_FSM_LEAVE_ROLLBACKING()      (g_act_fsm_is_rollbacking = 0)

#define ACT_FSM_CHK_DWLCH_PROF()          1 // TODO: single channel
#define ACT_FSM_CHK_TUNING_REQ_TX(chn)    ((chn) < 4)
#define ACT_FSM_CHK_TUNING_REQ_RX(chn)    ((chn) < 4)

#define __LOGICAL_CHN_TO_PHYSICAL_CHN(chn)  ((chn)-1)
#define __PHYSICAL_CHN_TO_LOGICAL_CHN(chn)  ((chn)+1)
#define __GET_XCVR_CHN_CAPABILITY()         4 // TODO: from xcvr driver
#define __XCVR_CHN_IS_VALID(chn)           ((chn) < __GET_XCVR_CHN_CAPABILITY())
#define __NGPON2_CHN_SET(rx_chn,tx_chn)    aal_xgpon_xfp_chanl_set(AAL_DEFAUL_DEV_ID,__LOGICAL_CHN_TO_PHYSICAL_CHN(rx_chn), __LOGICAL_CHN_TO_PHYSICAL_CHN(tx_chn))
#define __NGPON2_CHN_CPI_CHK(chn, cpi)  __xgpn_chn_cpi_chk(AAL_DEFAUL_DEV_ID, chn, cpi)
#define __NGPON2_CHN_SCAN_ENA(ena, scan_start_chn)  g_ngpon2_chn_scan_ena = (ena); \
                                                    g_ngpon2_chn_scan_idx = (scan_start_chn);
#define __NGPON2_CHN_SCAN_IS_ENA()   (g_ngpon2_chn_scan_ena)
#define __NGPON2_CHN_SCAN_IDX()   (g_ngpon2_chn_scan_idx)

#define FOR_ALL_XCVR_CHN(chn) for ((chn) = 0; (chn) < __GET_XCVR_CHN_CAPABILITY(); (chn)++)

/***************************************************************************
**
** typedefs
**
****************************************************************************/
typedef enum {                        /*value to be synced with HW definition later */
    AAL_XGPN_ACT_STATE_O1     = 0,    /*GPON/XG-PON1: O1 */
    AAL_XGPN_ACT_STATE_O1_1   = 1,    /*XGS-PON/NG-PON2: O1.1 */
    AAL_XGPN_ACT_STATE_O1_2   = 2,    /*XGS-PON/NG-PON2: O1.2 */
    AAL_XGPN_ACT_STATE_O2     = 3,    /*GPON: O2 */
    AAL_XGPN_ACT_STATE_O3     = 4,    /*GPON: O3 */
    AAL_XGPN_ACT_STATE_O2_3   = 5,    /*XG-PON1/XGS-PON/NG-PON2: O2-3 */
    AAL_XGPN_ACT_STATE_O4     = 6,    /*GPON/XG-PON1/XGS-PON/NG-PON2: O4 */
    AAL_XGPN_ACT_STATE_O5     = 7,    /*GPON/XG-PON1: O5 */
    AAL_XGPN_ACT_STATE_O5_1   = 8,    /*XGS-PON/NG-PON2: O5.1 */
    AAL_XGPN_ACT_STATE_O5_2   = 9,    /*NG-PON2: O5.2 */
    AAL_XGPN_ACT_STATE_O6     = 10,   /*GPON/XG-PON1/XGS-PON/NG-PON2: O6 */
    AAL_XGPN_ACT_STATE_O7     = 11,   /*GPON/XG-PON1/XGS-PON/NG-PON2: O7 */
    AAL_XGPN_ACT_STATE_O8_1   = 12,   /*NG-PON2: O8.1 */
    AAL_XGPN_ACT_STATE_O8_2   = 13,   /*NG-PON2: O8.2 */
    AAL_XGPN_ACT_STATE_O9     = 14,   /*NG-PON2: O9 */

    AAL_XGPN_ACT_STATE_MAX      = 15
} aal_xgpn_activation_state_t;

ca_int8_t* xgpn_act_state[AAL_XGPN_ACT_STATE_MAX + 1] = 
    {"O1", "O1.1", "O1.2", "O2", "O3", "O2-3", "O4", "O5", "O5.1", "O5.2", "O6", "O7", "O8.1", "O8.2", "O9"};

typedef enum {                                              /*value to be synced with HW definition later */
    /*defined in G.984*/
    AAL_GPON_ACT_EVENT_LOS_LOF                   = 1,    /*GPON: LOS/LOF */
    AAL_GPON_ACT_EVENT_US_OVERHEAD_RECEIVED      = 2,    /*GPON: Upstream overhead parameters received */
    AAL_GPON_ACT_EVENT_DIRECTED_POPUP_RECEIVED   = 3,    /*GPON: Directed POPUP message received */
    AAL_GPON_ACT_EVENT_BROADCAST_POPUP_RECEIVED  = 4,    /*GPON: Broadcast POPUP message received */
    AAL_GPON_ACT_EVENT_DISABLE_SN_REQUEST        = 5,    /*GPON: Disable SN request */
    AAL_GPON_ACT_EVENT_DEACTIVE_ONUID_REQ        = 6,    /*GPON: Deactive ONU-ID request */
    AAL_GPON_ACT_EVENT_TO1_EXPIRE                = 7,    /*GPON: TO1 expire */
    AAL_GPON_ACT_EVENT_ONU_ID_ASSIGNMENT         = 8,    /*GPON: ONU-ID assignment */
    AAL_GPON_ACT_EVENT_EQD_ASSIGNMENT            = 9,    /*GPON: EqD assignment */
    AAL_GPON_ACT_EVENT_TO2_EXPIRE                = 10,   /*GPON: TO2 expire */
    AAL_GPON_ACT_EVENT_ENABLE_SN_REQUEST         = 11,   /*GPON: Enable SN request */

    AAL_GPON_ACT_EVENT_MAX                       = 31
} aal_gpon_activation_event_t;    /*GPON Activation Interrupt/event (detailed reason) definition */

typedef enum {                                              /*value to be synced with HW definition later */
    /*defined in G.987/989/9807.1 */
    AAL_XGPN_ACT_EVENT_LODS                      = 1,    /*XG-PON1/XGS-PON/NG-PON2: LODS */
    AAL_XGPN_ACT_EVENT_DWLCH_NOT_APPROPRIATE     = 2,    /*NG-PON2: DWLCH not appropriate */
    AAL_XGPN_ACT_EVENT_DSYNC                     = 3,    /*XG-PON1/XGS-PON/NG-PON2: DSYNC */
    AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST        = 4,    /*XG-PON1/XGS-PON/NG-PON2: Disable SN request */
    AAL_XGPN_ACT_EVENT_DWLCH_OK_TO_WORK          = 5,    /*NG-PON2: DWLCH ok to work */
    AAL_XGPN_ACT_EVENT_DSYNC_PROFILES_ACQUIRED   = 6,    /*XGS-PON: DSYNC and Profiles acquired */
    AAL_XGPN_ACT_EVENT_TOZ_EXPIRE                = 7,    /*NG-PON2: TOZ expires */
    AAL_XGPN_ACT_EVENT_CALIBRATION_REQUEST       = 8,    /*NG-PON2: calibration request */
    AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ        = 9,    /*XG-PON1/XGS-PON/NG-PON2: Deactive ONU-ID request */
    AAL_XGPN_ACT_EVENT_TO1_EXPIRE                = 10,   /*XG-PON1/XGS-PON/NG-PON2: TO1 expire */
    AAL_XGPN_ACT_EVENT_ONU_ID_ASSIGNMENT         = 11,   /*XG-PON1/XGS-PON/NG-PON2: ONU-ID assignment */
    AAL_XGPN_ACT_EVENT_EQD_ASSIGNMENT            = 12,   /*XG-PON1/XGS-PON/NG-PON2: EqD assignment */
    AAL_XGPN_ACT_EVENT_TUNING_REQUEST            = 13,   /*NG-PON2: Tuning request */
    AAL_XGPN_ACT_EVENT_SFC_MATCH_FOR_TX_TUNING   = 14,   /*NG-PON2: SFC match for Tx tuning */
    AAL_XGPN_ACT_EVENT_SFC_MATCH_FOR_RXTX_TUNING = 15,   /*NG-PON2: SFC match for Rx/Rx tuning */
    AAL_XGPN_ACT_EVENT_TO2_EXPIRE                = 16,   /*XG-PON1/XGS-PON/NG-PON2: TO2 expire */
    AAL_XGPN_ACT_EVENT_TO3_EXPIRE                = 17,   /*NG-PON2: TO3 expire */
    AAL_XGPN_ACT_EVENT_TO4_EXPIRE                = 18,   /*NG-PON2: TO4 expire */
    AAL_XGPN_ACT_EVENT_TO5_EXPIRE                = 19,   /*NG-PON2: TO5 expire */
    AAL_XGPN_ACT_EVENT_US_TUNING_CONFIRM         = 20,   /*NG-PON2: US Tuning confirm */
    AAL_XGPN_ACT_EVENT_ENABLE_SN_REQUEST         = 21,   /*XG-PON1/XGS-PON/NG-PON2: Enable SN request */

    /*defined for NG-PON2 in Saturn*/
    AAL_XGPN_ACT_EVENT_HUNT_SYNC_EXPIRE          = 22,   /*NG-PON2 O1.1, hunt sync expire*/
    AAL_XGPN_ACT_EVENT_PROFILE_LEARNING          = 23,   /*NG-PON2 O1.2 or O8.2, Profile evaluation  */
    AAL_XGPN_ACT_EVENT_TX_RESPONSE_CROSS         = 24,   /*NG-PON2 O2-3 or O9, TX Tuning request */
    AAL_XGPN_ACT_EVENT_TX_RX_TUNING_REQ          = 25,   /*NG-PON2 O8.1, TX/RX Tuning request */
    AAL_XGPN_ACT_EVENT_TUNING_REQ_ACK            = 26,
    AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST    = 27,    /*XG-PON1/XGS-PON/NG-PON2: ASSIGN_ONUID_INCONSIST */
    AAL_XGPN_ACT_EVENT_MAX                       = 31
} aal_xgpn_activation_event_t;    /*XGPON Activation Interrupt/event (detailed reason) definition */

ca_int8_t* xgpn_act_event[AAL_XGPN_ACT_EVENT_MAX + 1] = {"NONE",
                                                         "LODS",               "DWLCH_NOT_APPROPRIATE",   "DSYNC",                     "DISABLE_SN_REQUEST",
                                                         "DWLCH_OK_TO_WORK",   "DSYNC_PROFILES_ACQUIRED", "TOZ_EXPIRE",                "CALIBRATION_REQUEST",
                                                         "DEACTIVE_ONUID_REQ", "TO1_EXPIRE",              "ONU_ID_ASSIGNMENT",         "EQD_ASSIGNMENT",
                                                         "TUNING_REQUEST",     "SFC_MATCH_FOR_TX_TUNING", "SFC_MATCH_FOR_RXTX_TUNING", "TO2_EXPIRE",
                                                         "TO3_EXPIRE",         "TO4_EXPIRE",              "TO5_EXPIRE",                "US_TUNING_CONFIRM",
                                                         "ENABLE_SN_REQUEST",  "HUNT_SYNC_EXPIRE",        "PROFILE_LEARNING",          "TX_RESPONSE_CROSS ",
                                                         "TX_RX_TUNING_REQ ",  "TUNING_REQ_ACK"  ,        "ASSIGN_ONUID_INCONSIST"};

/***************************************************************************
**
** forward declarations
**
****************************************************************************/
static inline int __xgpn_state_to_fsm_state(ca_uint32_t top_state, ca_uint32_t sub_state);
static ca_boolean_t __xgpn_chn_cpi_chk(
    ca_device_id_t device_id,
    ca_uint8_t channel,
    ca_uint8_t cpi);

/***************************************************************************
**
** global variables to be exported
** see extern in header file
**
***************************************************************************/

ca_uint32_t   g_xgpon_act_fsm_err = 0;
aal_xgpn_activation_state_t g_xgpn_act_state;
ca_uint32_t   g_original_us_chn = 0; // TODO: move to g_aal_ngp2_twdm_chn_cb
ca_uint32_t   g_original_ds_chn = 0; // TODO: move to g_aal_ngp2_twdm_chn_cb

typedef struct {
    ca_uint32_t chn_scan_enable : 1; /* enable channel scan function */
    ca_uint32_t chn_scan_base : 3; /* base channel index for channel scan */
    ca_uint32_t rsvd : 10;
    ca_uint32_t pre_us_chn : 3; /* previous upstream channel */
    ca_uint32_t pre_ds_chn : 3; /* previous downstream channel */
    ca_uint32_t cur_us_chn : 3; /* current working upstream channel */
    ca_uint32_t cur_ds_chn : 3; /* current working downstream channel */
    ca_uint32_t target_us_chn : 3; /* target tunning upstream channel */
    ca_uint32_t target_ds_chn : 3; /* target tuning downstream channel */
    ca_uint32_t eqd[AAL_XGPON_WAVE_CHANNEL_MAX_NUM];
    aal_xgpon_twdm_chn_info_t chn_info[AAL_XGPON_WAVE_CHANNEL_MAX_NUM]; /* channel's detail info */
    ca_xgpn_twdm_channel_profile_t    chn_prof[CA_XGPON_MAX_CHANNEL_PROFILE_COUNT]; /*channel profiles for this DS wavelength channel*/
    // TODO: protection, tuning
}twdm_chn_cb_t;

twdm_chn_cb_t   g_aal_ngp2_twdm_chn_cb;

ca_ngpon2_channel_mgmt_t g_ngpon2_channel_mgmt_cb;

/***************************************************************************
**
** global variables NOT to be exported
** extern is absent in header file
**
****************************************************************************/


/***************************************************************************
**
** Functions
**
****************************************************************************/

typedef ca_status_t (*aal_xgpn_act_fsm_handler_func_t) (aal_xgpn_activation_state_t, aal_xgpn_activation_state_t);

aal_xgpn_act_fsm_handler_func_t aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_MAX][AAL_XGPN_ACT_EVENT_MAX];


ca_status_t aal_xgpn_act_fsm_error_handler
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t     new_state    /* new state */
);
ca_status_t aal_xgpn_act_fsm_common_handler
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t     new_state    /* new state */
);
ca_status_t aal_xgpn_act_fsm_s_o11_e_dsync_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t     new_state    /* new state */
);

ca_status_t aal_xgpon_cb_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  channel_partition_index);

ca_status_t aal_xgpon_xfp_cap2_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  index,
    CA_IN ca_uint8_t  dn_support,
    CA_IN ca_uint8_t  up_support,
    CA_IN ca_uint8_t  dwlch_calibration_record);


extern ca_uint32_t aal_pon_mac_mode_get(ca_device_id_t device_id);
extern ca_status_t aal_ploam_calibration_odd_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          value
);
extern ca_status_t aal_ploam_calibration_odd_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *value
);
extern ca_status_t aal_ploam_calibration_even_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          value
);
extern ca_status_t aal_ploam_calibration_even_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *value
);
extern ca_status_t aal_ploam_correlation_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          correlation
);
extern ca_status_t  cap_i2c_write (
                      ca_device_id_t   device_id,
                      ca_uint8      slave_addr,
                      ca_uint8      slave_offset,
                      ca_uint16     data_len,
                      ca_uint8      *p_tx_data/*[data_len:256]*/);

extern ca_status_t cap_i2c_read(
    ca_device_id_t  device_id,
    ca_uint8_t  slave_addr,
    ca_uint8_t  slave_offset,
    ca_uint16_t  data_len,
    ca_uint8_t * p_rx_data);



/***************************************************************************
**
** private function
**
****************************************************************************/
ca_status_t __aal_xgpon_sm_simulation_o9(ca_device_id_t device_id);

ca_boolean_t aal_xgpon_onu_id_assign_chk(ca_uint16_t onu_id)
{
    ca_uint32_t  pon_mode = aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID);

    if (AAL_PON_MODE_IS_XGPON(pon_mode)) {
        if (onu_id <= AAL_XGPON_ONU_ID_ASSIGNABLE_END) {
            return TRUE;
        }
    } else if (AAL_PON_MODE_IS_NGP2(pon_mode)) {
        if (onu_id <= AAL_NGP2_ONU_ID_ASSIGNABLE_END) {
            return TRUE;
        }
    } else if (AAL_PON_MODE_IS_25GPON(pon_mode)) {
        if (onu_id <= AAL_GPON_ONU_ID_ASSIGNABLE_END) {
            return TRUE;
        }
    }

    return FALSE;
}

ca_boolean_t aal_xgpon_alloc_id_assign_chk(ca_uint16_t alloc_id)
{
    ca_uint32_t  pon_mode = aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID);

    if (AAL_PON_MODE_IS_XGPON(pon_mode)) {
        if (alloc_id >= AAL_XGPON_ALLOC_ID_ASSIGNABLE_START
            && alloc_id <= AAL_XGPON_ALLOC_ID_ASSIGNABLE_END) {
            return TRUE;
        }
    } else if (AAL_PON_MODE_IS_NGP2(pon_mode)) {
        if (alloc_id >= AAL_NGP2_ALLOC_ID_ASSIGNABLE_START
            && alloc_id <= AAL_NGP2_ALLOC_ID_ASSIGNABLE_END) {
            return TRUE;
        }
    } else if (AAL_PON_MODE_IS_25GPON(pon_mode)) {
        if (alloc_id >= AAL_GPON_ALLOC_ID_ASSIGNABLE_START
            && alloc_id <= AAL_GPON_ALLOC_ID_ASSIGNABLE_END) {
            return TRUE;
        }
    }

    return FALSE;
}


static void __tcpi_callback (void *data)
{
    ca_uint32_t  dsync = 0;

    aal_xgpon_dsfp_sync_state_get(AAL_DEFAUL_DEV_ID, &dsync);

    if (dsync != CA_NGP2_ONU_SYNCED) {
        ca_uint8_t cpi = 0;

        /*
         * If the ONU cannot find a channel belonging to the matching channel
         * partition by the expiration of the timer, the ONU resets its CPI
         * in non-volatile memory to the default value (zero) in order to waive
         * the CPI restriction. The suggested value of TCPI is 5 minutes.
         */

        XGPN_ACTFSM_LOG_WARN("NGPON2 timer Tcpi timout: reset CPI to 0.\n");

        scfg_write(AAL_DEFAUL_DEV_ID, CFG_ID_PON_NGPON2_CHAN_PARTITION_INDEX,
            sizeof(ca_uint8_t), &cpi);

        aal_xgpon_actp_profile_learning_set(AAL_DEFAUL_DEV_ID,
            AAL_XGPON_ACTP_STS_DWLCH_NOT_OK);
    }
}

static ca_uint32_t g_ngpon2_cpi_timer_id = 0;

static void __xgpn_act_fsm_start_tcpi(void)
{
    g_ngpon2_cpi_timer_id = ca_timer_add(g_ngpon2_cpi_timeout, __tcpi_callback, NULL);
}

static void __xgpn_act_fsm_stop_tcpi(void)
{
    if (g_ngpon2_cpi_timer_id){
        ca_timer_del(g_ngpon2_cpi_timer_id);
        g_ngpon2_cpi_timer_id = 0;
    }
}

static inline void __xgpn_act_fsm_set_current_chn(
    ca_device_id_t device_id,
    ca_uint8_t  us_chn,
    ca_uint8_t  ds_chn)
{
    g_aal_ngp2_twdm_chn_cb.cur_ds_chn = ds_chn;
    g_aal_ngp2_twdm_chn_cb.cur_us_chn = us_chn;
}

static inline ca_uint8_t __xgpn_act_fsm_get_current_us_chn(
    ca_device_id_t device_id)
{
    return g_aal_ngp2_twdm_chn_cb.cur_us_chn;
}


static inline ca_uint8_t __xgpn_act_fsm_get_current_ds_chn(
    ca_device_id_t device_id)
{
    return g_aal_ngp2_twdm_chn_cb.cur_ds_chn;
}

static inline void __xgpn_act_fsm_set_previous_chn(
    ca_device_id_t device_id,
    ca_uint8_t  us_chn,
    ca_uint8_t  ds_chn)
{
    g_aal_ngp2_twdm_chn_cb.pre_ds_chn= ds_chn;
    g_aal_ngp2_twdm_chn_cb.pre_us_chn= us_chn;


    // TODO: need remove old code
    // Update the parameters in pre_working_channel:
    g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id = ds_chn;
    g_ngpon2_channel_mgmt_cb.pre_working_channel.us_pon_id = us_chn;

}


static inline ca_uint8_t __xgpn_act_fsm_get_previous_ds_chn(
    ca_device_id_t device_id)
{
    return g_aal_ngp2_twdm_chn_cb.pre_ds_chn;
}


static inline ca_uint8_t __xgpn_act_fsm_get_previous_us_chn(
    ca_device_id_t device_id)
{
    return g_aal_ngp2_twdm_chn_cb.pre_us_chn;
}

static inline ca_uint8_t __xgpn_act_fsm_get_target_us_chn(
    ca_device_id_t device_id)
{
    return g_aal_ngp2_twdm_chn_cb.target_us_chn;
}

static inline ca_uint8_t __xgpn_act_fsm_get_target_ds_chn(
    ca_device_id_t device_id)
{
    return g_aal_ngp2_twdm_chn_cb.target_ds_chn;
}

static inline void __xgpn_act_fsm_set_target_chn(
    ca_device_id_t device_id,
    ca_uint8_t  us_chn,
    ca_uint8_t  ds_chn)
{
    g_aal_ngp2_twdm_chn_cb.target_ds_chn = ds_chn;
    g_aal_ngp2_twdm_chn_cb.target_us_chn = us_chn;
}

static inline void __xgpn_act_fsm_set_active_chn(
    ca_device_id_t  device_id,
    ca_uint8_t   chn,
    ca_boolean_t active)
{
    if (!g_aal_ngp2_twdm_chn_cb.chn_info[chn].valid)
        return;

    if (active) {
        g_aal_ngp2_twdm_chn_cb.chn_info[chn].active_status = TRUE;
        g_aal_ngp2_twdm_chn_cb.chn_info[chn].map_989dot2_ds_chn = chn + 1;
        g_aal_ngp2_twdm_chn_cb.chn_info[chn].map_989dot2_us_chn = chn + 1;
    } else {
        g_aal_ngp2_twdm_chn_cb.chn_info[chn].active_status = FALSE;
        g_aal_ngp2_twdm_chn_cb.chn_info[chn].map_989dot2_ds_chn = AAL_NGPON2_TWDM_INVALID_CHN_ID;
        g_aal_ngp2_twdm_chn_cb.chn_info[chn].map_989dot2_us_chn = AAL_NGPON2_TWDM_INVALID_CHN_ID;
    }
}


static inline void __xgpn_act_fsm_store_chn_eqd(
    ca_device_id_t  device_id,
    ca_uint32_t  eqd)
{

    /*if (g_aal_ngp2_twdm_chn_cb.cur_us_chn >= AAL_XGPON_WAVE_CHANNEL_MAX_NUM) {
        XGPN_ACTFSM_LOG_ERR("wrong cur_us_chn %u.\n",
            g_aal_ngp2_twdm_chn_cb.cur_us_chn);
        return;
    }*/

    g_aal_ngp2_twdm_chn_cb.eqd[g_aal_ngp2_twdm_chn_cb.cur_us_chn] = eqd;
    XGPN_ACTFSM_LOG_INFO("Store EQD %#x to Hardware for channel %u.\n",
        eqd, g_aal_ngp2_twdm_chn_cb.cur_us_chn);
}

static inline void __xgpn_act_fsm_load_chn_eqd(
    ca_device_id_t  device_id,
    ca_uint8_t   chn)
{
    /* Set Eqd to hardware */
    if (g_aal_ngp2_twdm_chn_cb.eqd[chn]) {
        AAL_PON_REG_WRITE(g_aal_ngp2_twdm_chn_cb.eqd[chn],
            XGPN_BWMP_EQD_STS);
        XGPN_ACTFSM_LOG_INFO("Config Eqd #x to Hardware from channel %u.\n",
            g_aal_ngp2_twdm_chn_cb.eqd[chn],
            chn);
    }

}

static inline void __xgpn_act_fsm_tuning_flags_set(
    ca_device_id_t  device_id,
    ca_boolean_t tuning_flag,
    ca_boolean_t rollback_flag)
{
    g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag = tuning_flag;
    g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag    = rollback_flag;
}

void aal_clear_actp_rx_tx_optical_tuning
(
    CA_IN ca_uint8 clear_rx,
    CA_IN ca_uint8 clear_tx
);

static ca_status_t __xgpn_protect_switch_simulation(
    ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_actp_ctrl_mask_t  xgpn_actp_ctrl_mask;
    aal_xgpon_actp_ctrl_t       xgpn_actp_ctrl;
    ca_printf("start protect_switch simulate......\r\n");
    ca_printf("set sync hunt.\r\n");
    ret = aal_xgpon_actp_sync_hunt_set(AAL_DEFAUL_DEV_ID, 1000 * 8000); // Set TO4
    CA_RET_VALUE_CHECK(ret);
    ca_printf("set to4.\r\n");
    ret = aal_xgpon_actp_to4_set(AAL_DEFAUL_DEV_ID, 100 * 8000); // Set TO4
    CA_RET_VALUE_CHECK(ret);
    ca_printf("set to5.\r\n");
    ret = aal_xgpon_actp_to5_set(AAL_DEFAUL_DEV_ID, 100 * 8000); // Set TO4
    CA_RET_VALUE_CHECK(ret);
    ca_printf("set to23.\r\n");
    ret = aal_xgpon_actp_to23_set(AAL_DEFAUL_DEV_ID, 30 * 8000); // Set TO4
    CA_RET_VALUE_CHECK(ret);

    memset(&xgpn_actp_ctrl_mask, 0, sizeof(aal_xgpon_actp_ctrl_mask_t));
    memset(&xgpn_actp_ctrl, 0, sizeof(aal_xgpon_actp_ctrl_t));
    xgpn_actp_ctrl_mask.s.wlcp_enable = TRUE;
    xgpn_actp_ctrl.wlcp_enable = TRUE;   // 1'b0: WLCP enable
    ca_printf("wlcp_enable Enabled.\r\n");
    ret = aal_xgpon_actp_ctrl_set(device_id, xgpn_actp_ctrl_mask, &xgpn_actp_ctrl);
    CA_RET_VALUE_CHECK(ret);
    //aal_xgpon_sm_simulation(device_id); // to O8

    return CA_E_OK;
}

ca_status_t __aal_xgpon_clear_mibs(ca_device_id_t device_id)
{
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgtc_frame, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgtc_hec_corrected, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgtc_hec_uncorrected_mib, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgem_err1, hd, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgem_frame, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgem_hec_corrected, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgem_hec_uncorrected_mib, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgem_nolf_mib, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgem_los_word_mib, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_xgem_err_key_mib, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_rxmac_xgem_frame, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_rxmac_omci_frame, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_rxmac_pkt_frame, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_rxmac_pkt_crc_err, cnt, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_rxmac_xgem_frame_status, fg_xgem_err, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_DSFP_async_ff_status, overflow, 0);

    return CA_E_OK;
}

ca_status_t __aal_xgpon_clear_gem_tcont_tbl(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t dsfp_gem_idx = 0;
    aal_xgpon_dsfp_xgem_cfg_msk_t dsfp_xgem_mask;
    aal_xgpon_dsfp_xgem_cfg_t  dsfp_xgem_cfg;

    ca_uint32_t usbg_gem_idx = 0;
    aal_xgpon_usbg_gem_config_msk_t usbg_xgem_mask;
    aal_xgpon_usbg_gem_config_t  usbg_xgem_cfg;

    ca_uint32_t wave_channel =0;
    ca_uint32_t tcont_idx = 0;
    aal_xgpon_bwmp_alloc_id_cfg_msk_t alloc_id_mask;
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;

    memset(&dsfp_xgem_cfg, 0 ,sizeof(aal_xgpon_dsfp_xgem_cfg_t));
    dsfp_xgem_mask.u32 = ~0;

    memset(&usbg_xgem_cfg, 0 ,sizeof(aal_xgpon_usbg_gem_config_t));
    usbg_xgem_mask.u32 = ~0;

    memset(&alloc_id_cfg, 0 ,sizeof(aal_xgpon_bwmp_alloc_id_cfg_t));
    alloc_id_mask.u32 = ~0;

    for(dsfp_gem_idx = 1;dsfp_gem_idx <AAL_XGPON_MAX_GEM_NUM; dsfp_gem_idx++){
        ret = aal_xgpon_dsfp_xgem_get(device_id, dsfp_gem_idx,&dsfp_xgem_cfg);
        CA_RET_VALUE_CHECK(ret);
#if 0  // TODO: according to G.98x, the GEM configured by OLT should keep after ONU de-activate
        if(dsfp_xgem_cfg.gem_vld == 1 ){
            dsfp_xgem_cfg.gem_id = 0xFFFF;
            dsfp_xgem_cfg.gem_mc = 0;
            dsfp_xgem_cfg.gem_vld = 0;
            ret = aal_xgpon_dsfp_xgem_set(device_id,dsfp_gem_idx,dsfp_xgem_mask,&dsfp_xgem_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
#endif
    }

    for(usbg_gem_idx = 1;usbg_gem_idx <AAL_XGPON_MAX_GEM_NUM; usbg_gem_idx++){
        ret = aal_xgpon_usbg_xgem_cfg_get(device_id, usbg_gem_idx,&usbg_xgem_cfg);
        CA_RET_VALUE_CHECK(ret);
#if 0  // TODO: according to G.98x, the GEM configured by OLT should keep after ONU de-activate

        if(usbg_xgem_cfg.port_en == 1 ){
            usbg_xgem_cfg.xgem_id = 0xFFFF;
            usbg_xgem_cfg.mc_flag = 0;
            usbg_xgem_cfg.port_en = 0;
            usbg_xgem_cfg.aes_mode = 0;

            ret = aal_xgpon_usbg_xgem_cfg_set(device_id,usbg_gem_idx,usbg_xgem_mask,&usbg_xgem_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
#endif
    }

    alloc_id_cfg.alloc_id = AAL_XGPON_ALLOC_ID_INVALID;
    alloc_id_cfg.omci_en = 1; // must set for S/W access
    alloc_id_cfg.ploam_en = 0;
    for(wave_channel = 0 ;wave_channel <8; wave_channel++){
        for(tcont_idx = 1 ;tcont_idx <tcont_num_per_chip; tcont_idx++){
            ret = aal_xgpon_bwmp_tcont_set(device_id,wave_channel,tcont_idx,alloc_id_mask,&alloc_id_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
    }
    return ret;
}

typedef struct {
        XGPN_XGPN_GLB_CTRL_t   glb_ctrl;
        ca_uint8_t             sds_tx_enable;
        ca_uint8_t             laser_on;
        ca_uint32_t            to1;
        ca_uint32_t            to2;
} __xgpon_cfg_record_t; /* save the runing configuration which cannot be set from OLT */

static __xgpon_cfg_record_t   g_xgpon_cfg_record = {{.wrd = 0}, .sds_tx_enable = 0, .laser_on = 0, .to1 = 0, .to2 = 0};
static ca_uint32_t            g_xgpon_reset_timer = 0;
#define __XGPON_RESET_TIMER_INTERVAL_MS          1000
static void __xgpon_cfg_record_save(ca_device_id_t device_id)
{
#if defined(CONFIG_CORTINA_BOARD_FPGA)
#else
    AAL_GLB_REG_FIELD_READ(GLOBAL_PSDS_INIT_CNTL, ben_oen, g_xgpon_cfg_record.sds_tx_enable);
#endif
    g_xgpon_cfg_record.glb_ctrl.wrd = AAL_PON_REG_READ(XGPN_XGPN_GLB_CTRL);
    AAL_PON_REG_FIELD_READ( XGPN_USBG_GBL_CTRL, laser_on, g_xgpon_cfg_record.laser_on);
    AAL_PON_REG_FIELD_READ(XGPN_ACTP_TO1, threshold, g_xgpon_cfg_record.to1);
    AAL_PON_REG_FIELD_READ(XGPN_ACTP_TO23, threshold, g_xgpon_cfg_record.to2);
}

static void __xgpon_cfg_record_recover(ca_device_id_t device_id)
{
    AAL_PON_REG_FIELD_WRITE( XGPN_USBG_GBL_CTRL, laser_on, g_xgpon_cfg_record.laser_on);
    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_TO1, threshold, g_xgpon_cfg_record.to1);
    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_TO23, threshold, g_xgpon_cfg_record.to2);
    /* note: enable PON MAC after other PON MAC config */
    AAL_PON_REG_WRITE( g_xgpon_cfg_record.glb_ctrl.wrd, XGPN_XGPN_GLB_CTRL);
    if (g_xgpon_cfg_record.sds_tx_enable) {
        ca_udelay(500);
    }

#if defined(CONFIG_CORTINA_BOARD_FPGA)
#else
    /* note: enable PSDS tx after PON MAC enable */
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PSDS_INIT_CNTL, ben_oen, g_xgpon_cfg_record.sds_tx_enable);
#endif
}

static void __xgpon_disable(ca_device_id_t device_id)
{
#if defined(CONFIG_CORTINA_BOARD_FPGA)
#else
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PSDS_INIT_CNTL, ben_oen, 0); /* disable PSDS tx */
#endif

    AAL_PON_REG_FIELD_WRITE(XGPN_XGPN_GLB_CTRL, mac_en, 0); /* disable PON MAC */
    ca_udelay(500);
}

/* Check PON optic Rx signal ready or NOT */
static ca_boolean_t __xgpon_chk_opt_rx(ca_device_id_t device_id)
{
    ca_boolean_t  rx_los = 0;
    ca_uint32_t   rx_invert = 0;
    int           chk_times = 3;

    if (g_pon_chip_version >= 'C') { /* ONLY rev-C and later support, old reversion doesnt support */
        AAL_GLB_REG_FIELD_READ(GLOBAL_IO_DRIVE_CONTROL, nu_ds, rx_invert);
    }

    while (chk_times-- > 0) {
        AAL_GLB_REG_FIELD_READ(GLOBAL_OPT_MODULE_STATUS, opt_rx_los, rx_los);
        if (rx_invert)
            rx_los = !rx_los;
        if (rx_los) { /* LOS */
            return FALSE;
        } else {
            ca_mdelay(5);
        }
    }
    return TRUE;
}

/* Check PON SERDES CDR clock stable or NOT */
static ca_boolean_t __xgpon_chk_psds_rx_clock(ca_device_id_t device_id)
{
    int  chk_times = 100; /* 100ms timeout */

#if defined(CONFIG_CORTINA_BOARD_FPGA) /* FPGA */
#else /* ASIC */
    PSDS_RGB8_t   psds_rgb8;

    while (chk_times-- > 0) {
        
        psds_rgb8.wrd = AAL_PON_REG_READ(PSDS_RGB8);
        if ((psds_rgb8.wrd & 0x8c01) != 0x8c00) {/* PON SERDES clock recovery ready */
            return FALSE;
        } else {
            ca_mdelay(1);
        }
    }
#endif

    return TRUE;

}

/* Check PON MAC Rx clean or NOT */
static ca_boolean_t __xgpon_chk_no_rx_error(ca_device_id_t device_id)
{
    XGPN_XGFEC_FECUNCORCODE_t  uncorrect_err;
    XGPN_XGFEC_FECTOTALCODE_t  total_code;
    XGPN_PLMP_MIC_ERR_MIB_t    mir_err_ploam;

    total_code.wrd    = AAL_PON_REG_READ(XGPN_XGFEC_FECTOTALCODE);
    uncorrect_err.wrd = AAL_PON_REG_READ(XGPN_XGFEC_FECUNCORCODE);
    mir_err_ploam.wrd = AAL_PON_REG_READ(XGPN_PLMP_MIC_ERR_MIB);

    ca_udelay(1000); /* delay 8 DBA cycle : 125us * 8 */

    total_code.wrd    = AAL_PON_REG_READ(XGPN_XGFEC_FECTOTALCODE);
    uncorrect_err.wrd = AAL_PON_REG_READ(XGPN_XGFEC_FECUNCORCODE);
    mir_err_ploam.wrd = AAL_PON_REG_READ(XGPN_PLMP_MIC_ERR_MIB);

    if (total_code.wrd == 0) {
        ca_printf("%s(): %d no downstream fec codes, may loss of dsync\r\n", __func__, __LINE__);
    }

    if (uncorrect_err.wrd > 0 || mir_err_ploam.wrd > 0) {
        return FALSE;
    }

    return TRUE;
}

static void __xgpon_reset_timer_callback(void *param)
{
    int  max_reset_times = 3;

    if (!__xgpon_chk_opt_rx(AAL_DEFAUL_DEV_ID)) { /* RX LOS */
        return ;
    }

    (void)aal_xgpon_reset(AAL_DEFAUL_DEV_ID, AAL_XGPON_RESET_FLAG_SDS); /* reset SERDES */

    if (!__xgpon_chk_psds_rx_clock(AAL_DEFAUL_DEV_ID)) {/* wait CDR clock stable */
        return;
    }

    (void)aal_xgpon_reset(AAL_DEFAUL_DEV_ID, AAL_XGPON_RESET_FLAG_MAC); /* reset PON MAC */

    if (!__xgpon_chk_psds_rx_clock(AAL_DEFAUL_DEV_ID)) { /* check CDR again before enable PON MAC */
        ca_printf("%s(): %d PON SERDES clock loss again\r\n", __func__, __LINE__);
        return;
    }

    while (max_reset_times-- > 0) {
        __xgpon_cfg_record_recover(AAL_DEFAUL_DEV_ID);/* re-enable PON MAC */
        if (__xgpon_chk_no_rx_error(AAL_DEFAUL_DEV_ID)) { /* if the line is good and stable then re-enable PON MAC */
            ca_timer_del(g_xgpon_reset_timer);
            g_xgpon_reset_timer = 0;
#ifndef CONFIG_CORTINA_BOARD_FPGA
            aal_psds_burst_enable(AAL_DEFAUL_DEV_ID, 1);
#endif
            AAL_XGPON_DBG("%s(): recover XGPON successfully\r\n", __func__);
            return;
        } else if (__xgpon_chk_psds_rx_clock(AAL_DEFAUL_DEV_ID)) { /* reset PON MAC again */
            __xgpon_disable(AAL_DEFAUL_DEV_ID);
            aal_xgpon_reset(AAL_DEFAUL_DEV_ID, AAL_XGPON_RESET_FLAG_MAC); /* reset PON MAC */
        } else {
            AAL_XGPON_DBG("%s(): %d PON SERDES clock loss again\r\n", __func__, __LINE__);
            return;
        }
    }

}

/** __xgpon_lods_proc
 *@brief: handler of Loss of PON Optic signal.
 *@note: once the Rx optic singal is disapper, H/W SERDES need to re-start CDR logic,
 *       and PON MAC need to flush all junk in buffer/FIFOs, and restart PON MAC logic
 */
static void __xgpon_lods_proc(ca_device_id_t device_id)
{
    __xgpon_cfg_record_save(device_id);
    __xgpon_disable(device_id);

    ca_printf("!EVENT: PON LODS (Loss Of Dsync)!\r\n");

#if defined(CONFIG_CORTINA_BOARD_FPGA)
    (void)aal_xgpon_reset(AAL_DEFAUL_DEV_ID, AAL_XGPON_RESET_FLAG_ALL);
    __xgpon_cfg_record_recover(device_id);
#else
    /*
     * Start polling timer to poll the RX_LOS status,
     * once the RX_LOS change to be 0 (i.e. Rx optic), then reset PON SERDES
     * and re-init PON MAC, to recover the PON H/W logic
     */
    if (!g_xgpon_reset_timer) {
        g_xgpon_reset_timer = ca_circle_timer_add(__XGPON_RESET_TIMER_INTERVAL_MS,
            __xgpon_reset_timer_callback, NULL);

        if (g_xgpon_reset_timer == 0) {
            ca_printf("%s(): error! fail to add xgpon reset timer!\r\n", __func__);
        }
    }
#endif
}

/***************************************************************************
**
** common
**
****************************************************************************/

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_error_handler
**
**  Description:
**    XGPON event handler for error status
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        06/26/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_error_handler
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_status_t rc = CA_E_OK;
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
    XGPN_ACTP_STS_t current_cfg;
    aal_xgpn_activation_state_t sw_state;

    AAL_XGPON_DBG_ACTFSM("Pre state: %s, new state: %s(%d)\r\n",
        xgpn_act_state[pre_state], xgpn_act_state[new_state], new_state);

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        return CA_E_OK;

    //sync up HW/SW ONU states
    current_cfg.wrd = AAL_PON_REG_READ(XGPN_ACTP_STS);
    sw_state = __xgpn_state_to_fsm_state(current_cfg.bf.onu_state, current_cfg.bf.onu_sub_state);
    if(sw_state != new_state) {

        __AAL_XGPN_ERROR("HW-SW state mismatch: HW state 0x%x, SW state: 0x%x\r\n", sw_state, new_state);
        new_state = sw_state;
    }
#if 0
    __AAL_XGPN_ERROR("Pre state: %s, new state: %s(%d)\r\n",
        xgpn_act_state[pre_state], xgpn_act_state[new_state], new_state);
#endif
    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
    if(new_state == AAL_XGPN_ACT_STATE_O1_2 || new_state == AAL_XGPN_ACT_STATE_O2_3
        || new_state == AAL_XGPN_ACT_STATE_O8_2 || new_state == AAL_XGPN_ACT_STATE_O9) {
        memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
        memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
        xgpn_actp_status_mask.s.tx_optical_tuning = TRUE;
        xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
        xgpn_actp_status_mask.s.profile_learning = TRUE;
        xgpn_actp_status.tx_optical_tuning = FALSE;   // 1'b0: ONU resume US transmission
        xgpn_actp_status.rx_optical_tuning = FALSE;
        xgpn_actp_status.profile_learning = 1;
        aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
        __AAL_XGPN_ERROR("seting profile learning\r\n");
    }

    g_xgpn_act_state = new_state;

    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_common_handler
**
**  Description:
**    XGPON event handler for common status
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        07/27/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_common_handler
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    AAL_XGPON_DBG_ACTFSM("Record state: %s, Pre state: %s, new state: %s\r\n",
         xgpn_act_state[g_xgpn_act_state], xgpn_act_state[pre_state], xgpn_act_state[new_state]);

    if (g_xgpn_act_state != pre_state) {
        __AAL_XGPN_ERROR("State Out-of-sync: H/W Pre state: %s, but S/W recorded state: %s\r\n",
            xgpn_act_state[pre_state], xgpn_act_state[g_xgpn_act_state]);
    }

    g_xgpn_act_state = new_state;

    return CA_E_OK;
}

static ca_status_t __xgpn_act_fsm_dsync_handler
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    AAL_XGPON_DBG_ACTFSM("Record state: %s, Pre state: %s, new state: %s\r\n",
         xgpn_act_state[g_xgpn_act_state], xgpn_act_state[pre_state], xgpn_act_state[new_state]);

    if (g_xgpn_act_state != pre_state && g_xgpn_act_state == new_state) {
        __AAL_XGPN_ERROR("Duplicated DSYNC interrupt: H/W Pre state: %s new state %s, S/W recorded state: %s\r\n",
            xgpn_act_state[pre_state], xgpn_act_state[new_state], xgpn_act_state[g_xgpn_act_state]);
    }

    g_xgpn_act_state = new_state;

    return CA_E_OK;
}



ca_status_t aal_xgpn_act_fsm_lods_common_handler
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);

    AAL_XGPON_DBG_ACTFSM("Record: %s, Pre state: %s, new state: %s\r\n",
         xgpn_act_state[g_xgpn_act_state], xgpn_act_state[pre_state], xgpn_act_state[new_state]);

    if (ONU_PON_MAC_NGPON2 == onu_pon_mac_mode) { /* NGPON2 case, need support tuning */
        if(aal_xgpn_protect_switch_sim_flag){
            AAL_XGPON_DBG_ACTFSM("aal_xgpn_protect_switch_sim_flag is %d\r\n",aal_xgpn_protect_switch_sim_flag);
            __xgpn_protect_switch_simulation(AAL_DEFAUL_DEV_ID);
        }
        (void)aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(AAL_DEFAUL_DEV_ID, AAL_CLK_SEQ_RESET_TRY_COUNT);
    } else { /* XGPON1, XGSPON, need re-init PON SERDES and MAC */
        g_xgpn_act_state = new_state;
        /* Loss of DSYNC, then reset PON SERDES, MAC to re-sync */
        __xgpon_lods_proc(AAL_DEFAUL_DEV_ID);
    }

    return CA_E_OK;
}


/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_swworkaround_handler
**
**  Description:
**    XGPON event handler for swworkaround status
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        03/07/17   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_swworkaround_handler
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_status_t rc = CA_E_OK;
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
#if 0
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
#endif
    AAL_XGPON_DBG_ACTFSM("Record: %s, Pre state: %s, new state: %s\r\n",
         xgpn_act_state[g_xgpn_act_state], xgpn_act_state[pre_state], xgpn_act_state[new_state]);

    if ((ONU_PON_MAC_NGPON2 == onu_pon_mac_mode) || (ONU_PON_MAC_XGSPON == onu_pon_mac_mode))
        g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;
    else if((ONU_PON_MAC_GPON == onu_pon_mac_mode) || (ONU_PON_MAC_XGPON1 == onu_pon_mac_mode))
        g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1;
    else
        rc = CA_E_ERROR;

    return rc;
}


/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o11_e_rxcalibratereq_change
**
**  Description:
**    XGPON event RX_CALIBRATION_REQ handler for state change @ O1.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

static ca_status_t __xgpn_act_fsm_chn_scan_next(void)
{
    ca_status_t ret = CA_E_OK;

    if (g_ngpon2_chn_scan_idx >= (__GET_XCVR_CHN_CAPABILITY() - 1)) {
        return CA_E_ERROR;
    }

    g_ngpon2_chn_scan_idx++;
    ret = aal_xgpon_xfp_chanl_set(AAL_DEFAUL_DEV_ID,
        g_ngpon2_chn_scan_idx, g_ngpon2_chn_scan_idx);
    if (CA_E_OK == ret) {
        aal_xgpon_xfp_chanl_get(AAL_DEFAUL_DEV_ID,
            &g_original_ds_chn, &g_original_us_chn);
    }

    return ret;

}

static void __xgpn_act_fsm_sync_expire_handle(void)
{
    ca_status_t ret = CA_E_OK;
    //ca_uint8_t  chn = __NGPON2_CHN_SCAN_IDX();

    if (__NGPON2_CHN_SCAN_IS_ENA()) {
        ret = __xgpn_act_fsm_chn_scan_next();
    }

    if (CA_E_OK != ret) {
        __AAL_XGPN_ERROR("Channel scan has finished, but no channel can sync.\r\n");
    }
}

ca_status_t aal_xgpn_act_fsm_s_o11_e_sync_hunt_expire
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O1_1 == new_state),
        CA_E_CONFLICT);

    __xgpn_act_fsm_sync_expire_handle();

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;
}

static ca_status_t __xgpn_act_fsm_to1_expire
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
/* Actions:
    change TX/RX wavelength (optional)
    SW set XGPN_ACTP_CTRL.wlcp_enable = 0
    XGPN_ACTP_STS.rx_optical_tuning = 0
    discard III, stop TO1
*/
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_SET_HW_WLCP_ENA(0);
    ACT_FSM_SET_HW_RX_TUNING(0);
    memset(&g_ngpon2_channel_mgmt_cb.profile_record[0].burst_profile, 0, sizeof(ca_xgpn_burst_profile_t) * CA_XGPON_MAX_BURST_PROFILE_COUNT);
    g_xgpn_basic_info.onu_id = AAL_XGPON_INVALID_ONU_ID;

    return CA_E_OK;
}


ca_status_t aal_xgpn_act_fsm_s_o11_e_rxcalibratereq_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;
    ca_uint32_t index = 0;
    //, dwlch_id;
    //ca_uint8_t dwlch_config_index;
    ca_uint32_t dsync_sts = 0;
#if 0
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
#endif


    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
    {
        __AAL_XGPN_ERROR("PON MAC mode error for handling: %d\r\n", onu_pon_mac_mode);
        goto ret;
    }

    if ((AAL_XGPN_ACT_STATE_O1_1 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
    {
        __AAL_XGPN_ERROR("Error: Pre state: %s, new state: %s\r\n",
            xgpn_act_state[pre_state], xgpn_act_state[new_state]);
        goto ret;
    }

    AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);



    /*(From 989.3: The ONU may repeat the process for two or more available downstream wavelength channels,
     and may create and store the calibration record for those channels.)*/
    /* calibration_request case has been covered in O2.3 -> O1.1, with vector clean to continue HW DSYNC check,
    so HW should not trigger RX calibration event again here; */

    #if 0

    // original channel record to be updated
    dwlch_id = g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id & 0x0F;
    if (dwlch_id < CA_NGPON2_MAX_DWLCH)
    {
        //Get channel profile CB by value ds_pon_id;
        for (index = 0; index < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT; index ++)
        {
            if (dwlch_id == (g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].pon_id & 0x0F))
                g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].calibration_record_ds.calibration_record_status = CA_NGPON2_CALIBRATION_RECORD_LOOSE; //update record: loose
        }
    }

    //check parameters in structure transceiver_cap;
    dwlch_config_index = g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index;
    if (0xFF == dwlch_config_index)// RX still not configured after power up
    {
        for (index = 0; index < CA_NGPON2_MAX_DWLCH; index ++)
            if (g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_dn[index])//transceiver support this DCHID
            {
                //Config transceiver RX parameter according to this DCHID
                // TODO: Config transceiver RX parameter, need update according to para requirement
                ca_printf("%s::%d::Channel set para: index 0x%x, us_pon_id 0x%x.\r\n",__FUNCTION__,__LINE__, index);
                rc = aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL);

                g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index = index;
                break;
            }
    }
    else //DS wavelength configured before, but not synced with OLT;
    {
        dwlch_config_index &= 0x7;  // ensure value not exceed
        g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_calibration_record[dwlch_config_index] = CA_NGPON2_CALIBRATION_RECORD_LOOSE; // update record: loose
        for (index = dwlch_config_index + 1; index < CA_NGPON2_MAX_DWLCH; index ++) // select another channel for dsync try next
        {
            if (g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_dn[index] &&
              (CA_NGPON2_CALIBRATION_RECORD_UNCALIBRATED == g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_calibration_record[index]))// this DCHID is support and not calibrated yet
            {
                //Config transceiver RX parameter according to this DCHID index;
                // TODO: Config transceiver RX parameter, need update according to para requirement
                ca_printf("%s::%d::Channel set para: index 0x%x, us_pon_id 0x%x.\r\n",__FUNCTION__,__LINE__, index);
                rc = aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL);

                g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index = index;
                break;
            }
        }
        if (CA_NGPON2_MAX_DWLCH == index)
        {
            // debug CLI command to be added to configure channel selection manually
            //Donggun>Also need CLI to set channel lamda offset manually.
            // TODO: debug CLI command to be added

            /*
            If (debug_flag)
            {    dwlch_config_index = debug_index;
                //Config transceiver RX parameter according to this debug index;
            }
            //none synced during all DWLCH transceiver supported, error
            */
        }
    }


    #endif

    //
    for (index = 0; index < CA_NGPON2_MAX_DWLCH; index ++)
    {
        //Config transceiver RX parameter according to this DCHID
        // TODO: Config transceiver RX parameter, need update according to para requirement
        AAL_XGPON_DBG_ACTFSM("Channel set para: index 0x%x.\r\n",index);
        if(aal_pon_mac_mode_get(0) == ONU_PON_MAC_NGPON2)
        {
            rc = aal_xgpon_xfp_chanl_set(0, index, index);
            CA_RET_VALUE_CHECK(rc);
        }
        aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);

        rc = aal_xgpon_dsfp_sync_state_get(0,&dsync_sts);
        CA_RET_VALUE_CHECK(rc);

        if(dsync_sts == CA_NGP2_ONU_SYNCED)
        {
            g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index = index;
            break;
        }
    }
    //aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);
#ifdef LCHEN_NOT_REMOVE
    aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);
#endif
    AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);

ret:
    return rc;
}


/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o11_e_dsync_change
**
**  Description:
**    XGPON event DSYNC handler for state change @ O1.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o11_e_dsync_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
#ifdef NEW_ACT_FSM
{
    ca_uint8_t dwlch_id = 0;

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O1_2 == new_state),
        CA_E_CONFLICT);

    ACT_FSM_SET_HW_WLCP_ENA(0);

    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);

    dwlch_id = __xgpn_act_fsm_get_current_ds_chn(AAL_DEFAUL_DEV_ID);

    __xgpn_act_fsm_set_active_chn(AAL_DEFAUL_DEV_ID, dwlch_id, TRUE);

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;
}
#else
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;
    ca_uint32_t index, dwlch_id;
    ca_uint8_t dwlch_config_index;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O1_1 != pre_state) || (AAL_XGPN_ACT_STATE_O1_2 != new_state) )
        goto ret;

     //DSYNC attained, update calibration record
    AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);

    //if valid, record of configured channel to be updated
    dwlch_id = g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id & 0x0F;
    if (dwlch_id < CA_NGPON2_MAX_DWLCH)
    {
        //Get channel profile CB by value ds_pon_id;
        for (index = 0; index < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT; index ++)
        {
            if (dwlch_id == (g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].pon_id & 0x0F))
                g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].calibration_record_ds.calibration_record_status = CA_NGPON2_CALIBRATION_RECORD_SUFFICIENT; //update record: sufficient
        }
        __xgpn_act_fsm_set_active_chn(AAL_DEFAUL_DEV_ID, dwlch_id, TRUE);

    }
    else    //config not from profile still;
    {
        dwlch_config_index = g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index;
        g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_calibration_record[dwlch_config_index] = CA_NGPON2_CALIBRATION_RECORD_SUFFICIENT; //update record: sufficient
    }

    //Note for HW: If DWLCH ID has changed, Discard VII;
    if (g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id != g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id)
    {
        //discard all system and channel profiles;
        dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F;
        g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].learnt_flag = FALSE;
        memset(&g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].system_profile, 0, sizeof(ca_xgpn_sys_profile_t));
        memset(g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile, 0, CA_XGPON_MAX_CHANNEL_PROFILE_COUNT*sizeof(ca_xgpn_twdm_channel_profile_t));
        g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].channel_profile_count_learnt = 0;

    }

    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_STS, profile_learning, 1);
    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);/*why clear two bits,according to sm, only need clear rx*/

    //     Update SW previous state record to O1.2
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_2;

    AAL_XGPON_DBG_ACTFSM("line <%d> Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);

    return CA_E_OK;

ret:
    return rc;
}
#endif


/****************************************************************************
**
**  Function Name: aal_ngpon2_profile_evaluate
**
**  Description:
**    Do NGPON2 profile evaluate operation
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        09/27/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_ngpon2_profile_evaluate(ca_boolean_t full_scan)
{
    ca_status_t rc = CA_E_OK;
    ca_uint32_t index, dwlch_id, uwlch_id;
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;

    /*     Purpose is to get result of the internal system and channel profile evaluation:
    whether the channel partition, upstream optical link type, and the upstream line rate match those of the ONU or not.*/

    AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);

     //confirm whether system profile info matches ONU capability or not;

    //confirm which channel profile matches ONU
    for (index = 0; index < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT; index ++)
    {
        if (FALSE == (g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].learnt_flag))// not learnt
        {
            AAL_XGPON_DBG_ACTFSM("channel_profile[%d].learnt_flag: FALSE\r\n", index);
            continue;
        }

        if (FALSE == (g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].channel_profile_idx & 0x4))// indicate to another channel
        {
            AAL_XGPON_DBG_ACTFSM("channel_profile[%d].channel_profile_idx: to another TWDM channel\r\n", index);
            continue;
        }

        if (TRUE == g_ngpon2_channel_mgmt_cb.calibration_request.calibration_request_flag)
        {
            if (TRUE == (g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].channel_profile_idx & 0x1))// to be ignored
            {
                AAL_XGPON_DBG_ACTFSM("channel_profile[%d].channel_profile_idx: Upstream descriptor to be ignored\r\n", index);
                continue;
            }

            uwlch_id = g_ngpon2_channel_mgmt_cb.calibration_request.target_us_pon_id;

            if (uwlch_id != g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].uwlch_id)
            {
                AAL_XGPON_DBG_ACTFSM("channel_profile[%d].uwlch_id 0x%x != target_us_pon_id 0x%x\r\n",
                    index, g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].uwlch_id, uwlch_id);
                continue;
            }

            dwlch_id = g_ngpon2_channel_mgmt_cb.calibration_request.target_ds_pon_id;
        }
        else
        {
            dwlch_id = g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].pon_id;
            if ((dwlch_id & 0xF) != g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index)
            {
                AAL_XGPON_DBG_ACTFSM("channel_profile[%d].pon_id 0x%x != transceiver_cap.dwlch_config_index 0x%x\r\n",
                    index, dwlch_id, g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index);
                // TODO: error handler
            }
            uwlch_id = g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].uwlch_id;
        }

        //check whether current ds/us pon-id/channel could be supported by ONU
        if (FALSE == g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_dn[dwlch_id & 0xF])
        {
            AAL_XGPON_DBG_ACTFSM("transceiver_cap.channel_partition_dn 0x%x Not support\r\n", dwlch_id);
            continue;
        }
        if (FALSE == g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_up[uwlch_id & 0xF])
        {
            AAL_XGPON_DBG_ACTFSM("transceiver_cap.channel_partition_up 0x%x Not support\r\n", uwlch_id);
            continue;
        }

        // check whether channel partition matches what ONU recorded working before.
        if (0 != g_ngpon2_channel_mgmt_cb.calibration_record.channel_partition_index)
        {
            if (g_ngpon2_channel_mgmt_cb.calibration_record.channel_partition_index !=
                g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].channel_partition)
            {
                AAL_XGPON_DBG_ACTFSM("calibration_record.channel_partition_index 0x%x != channel_profile[%d].channel_partition 0x%x\r\n",
                    g_ngpon2_channel_mgmt_cb.calibration_record.channel_partition_index, index, g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].channel_partition);
                continue;
            }
        }
        // check whether upstream optical link type match that of what ONU supports.
        // TODO:

        // check whether the upstream line rate match that of what ONU supports.
        // TODO:

        //scaned ds pon-id match this channel: update DS channel ID configured;
        g_ngpon2_channel_mgmt_cb.channel_configured.ds_channel_profile_index = index;
        g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = dwlch_id;
        break;  // found
    }
    if (index < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT)    // profile match
    {
        // write CSR (DWLCH OK to work), trigger HW state entering O2-3
        memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
        memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
        xgpn_actp_status_mask.s.profile_learning = TRUE;
        xgpn_actp_status.profile_learning = CA_NGPON2_ACTP_STS_DWLCH_OK;   // 2'b01: DWLCH OK
        rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
        if (CA_E_OK != rc){
            __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
        }

        // clear profile_evaluate_request
        g_ngpon2_channel_mgmt_cb.profile_record[0].profile_evaluate_request = FALSE;
    }
    else if (full_scan)    //either does not match, and FULL scan (if not full scan, wait for next evaluate)
    {
        AAL_XGPON_DBG_ACTFSM("No index match, and Full scan: DWLCH_NOT_OK!\r\n");
        // write CSR (DWLCH not appropriate),  trigger HW state back to O1.1
        memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
        memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
        xgpn_actp_status_mask.s.profile_learning = TRUE;
#if 1
        AAL_XGPON_DBG_ACTFSM("Force to set DWLCH_OK for debug purpose!\r\n");
        xgpn_actp_status.profile_learning = CA_NGPON2_ACTP_STS_DWLCH_OK;   // 2'b01: DWLCH OK
#else
        xgpn_actp_status.profile_learning = CA_NGPON2_ACTP_STS_DWLCH_NOT_OK;   // 2'b10: DWLCH not OK
#endif
        rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
        if (CA_E_OK != rc){
            __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
        }

        // clear profile_evaluate_request
        g_ngpon2_channel_mgmt_cb.profile_record[0].profile_evaluate_request = FALSE;
    }

    AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);

    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o12_e_profileevaluate_change
**
**  Description:
**    XGPON event PROFILE_EVALUATION handler for state change @ O1.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o12_e_profileevaluate_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
#ifdef NEW_ACT_FSM
{
    ca_uint8_t cpi = 0;

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET(AAL_XGPN_ACT_STATE_O1_2 == new_state,
        CA_E_CONFLICT);

    cpi = AAL_PON_SCFG_GET_CPI(); /* Get Channel Partition Index */

    if (AAL_XGPON_ACTFSM_CHN_SCAN_IS_ON() && cpi) { /* need check CPI */

        if (__xgpn_chn_cpi_chk(AAL_DEFAUL_DEV_ID, g_original_ds_chn, cpi)) {
            XGPN_ACTFSM_LOG_DBG("Check CPI %d OK.\n", cpi);
            aal_xgpon_actp_profile_learning_set(AAL_DEFAUL_DEV_ID, AAL_XGPON_ACTP_STS_DWLCH_OK);
            __xgpn_act_fsm_stop_tcpi();

        } else { /* Start Tcpi to find out the correct CPI channel, if fail, reset CPI to zero */

            XGPN_ACTFSM_LOG_DBG("Check CPI %d FAIL, start Tcpi timer.\n", cpi);
            aal_xgpon_actp_profile_learning_set(AAL_DEFAUL_DEV_ID, AAL_XGPON_ACTP_STS_DWLCH_NOT_OK);
            __xgpn_act_fsm_start_tcpi();
        }

    } else {

        aal_xgpon_actp_profile_learning_set(AAL_DEFAUL_DEV_ID, AAL_XGPON_ACTP_STS_DWLCH_OK);
    }

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;
}

#else
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    //if ((AAL_XGPN_ACT_STATE_O1_2 != pre_state) || (AAL_XGPN_ACT_STATE_O1_2 != new_state))
    if (AAL_XGPN_ACT_STATE_O1_2 != new_state)   //delete pre_state check for workaround, as HW will trigger profileEva at O1.1
        goto ret;

    //AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);
    AAL_XGPON_DBG_ACTFSM("Current state: %s\r\n", xgpn_act_state[g_xgpn_act_state]);

    if (g_ngpon2_channel_mgmt_cb.profile_record[0].learnt_flag) //profiles ready for evaluate
    {
        rc = aal_ngpon2_profile_evaluate(TRUE);
    }
    else    // profiles learnt not done, set flag to do evaluate in PLOAM module
    {
        g_ngpon2_channel_mgmt_cb.profile_record[0].profile_evaluate_request = TRUE;
        rc = CA_E_OK;
    }

    //     Update SW previous state record to O1.2
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_2;

    AAL_XGPON_DBG_ACTFSM("line <%d> , Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);
    return CA_E_OK;
ret:
    return rc;
}
#endif

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o12_e_dwlchoktowork_change
**
**  Description:
**    XGPON event DWLCH_OK_TO_WORK handler for state change @ O1.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o12_e_dwlchoktowork_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
#if 0
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
#endif
    ca_status_t rc = CA_E_ERROR;
//    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
//    aal_xgpon_actp_status_t xgpn_actp_status;
    ca_uint32_t index = 0, us_pon_id = 0;
    aal_xgpon_xgpn_glb_ctrl_config_msk_t mask;
    aal_xgpon_xgpn_glb_ctrl_config_t glb_ctrl_cfg;
    ca_uint16_t correlation_tag = 0;
    ca_uint32_t calibration_status = 0;
    ca_uint32_t move, value, calibration_temp;
#if 0
    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O1_2 != pre_state) || (AAL_XGPN_ACT_STATE_O2_3 != new_state) )
        goto ret;
#endif
    //AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);
    AAL_XGPON_DBG_ACTFSM("Current state: %s\r\n", xgpn_act_state[g_xgpn_act_state]);

    // The ONU makes the best effort to tune its transmitter to the specified operating upstream wavelength channel
    if (TRUE == g_ngpon2_channel_mgmt_cb.calibration_request.calibration_request_flag)
    {
        // TODO: config HW TC layer and transceiver TX parameter according to calibration_request.target_us_pon_id;
        //rc = aal_xgpon_xfp_chanl_set(0, 0xFF, g_ngpon2_channel_mgmt_cb.calibration_request.target_us_pon_id);
        AAL_XGPON_DBG_ACTFSM("Channel set para:us_pon_id 0x%x.\r\n",g_ngpon2_channel_mgmt_cb.calibration_request.target_us_pon_id);
        //if(aal_pon_mac_mode_get(0) == ONU_PON_MAC_NGPON2)
        //{
        // TODO: the following code should be not good, should change to target channel
            rc = aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL);
        //}
        us_pon_id = g_ngpon2_channel_mgmt_cb.calibration_request.target_us_pon_id;
        g_ngpon2_channel_mgmt_cb.calibration_request.calibration_request_flag = FALSE;
    }
    else //get default upstream channel according to ds configured:
    {
        //  update downstream  calibration_record_status
        index = g_ngpon2_channel_mgmt_cb.channel_configured.ds_channel_profile_index;
        if (index < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT)
        {
        	g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].calibration_record_ds.calibration_record_status = CA_NGPON2_CALIBRATION_RECORD_SUFFICIENT; //update record: sufficient

        	//  Get channel profile US PON ID according to channel_configured.ds_pon_id; //channel_configured.ds_pon_id -> channel profile -> default_response_channel
        	if (g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].channel_profile_idx & 0x1) // Upstream descriptor to be ignored, use default ID
        	{
        		us_pon_id = g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].default_response_channel;
        	}
        	else    // Upstream descriptor valid
        	{
        		us_pon_id = g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].uwlch_id;
        	}
        }

        // TODO: config HW TC layer and transceiver TX parameter according to US PON ID found;
        AAL_XGPON_DBG_ACTFSM("Channel set us_pon_id 0x%x.\r\n", us_pon_id);

        //rc = aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL);

        mask.s.uwlch_id = TRUE;
        glb_ctrl_cfg.uwlch_id = us_pon_id;
        rc = aal_xgpon_xgpn_glb_ctrl_set(0, mask, &glb_ctrl_cfg);
        // TODO:  09/30  config HW upstream rate, HW reg still not ready now, to be filled later;
        /**/

    }
    // channel_configured.us_pon_id = US PON ID found
    g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = us_pon_id;

    //  According to channel_configured.us_pon_id, write Correlation tag and Calibration record status into corresponding CSRs     //prepare for HW SN reporting;
    /*correlation tag is an ONU-generated non-zero 16-bit field, which should take a different value each time the transmitter optical power or frequency are changed.*/
    correlation_tag = (us_pon_id & 0xFF) << 8;  // Frequency or channel
    correlation_tag |= 0;   // TODO: transmitter optical power, 0 temporarily NOW
    rc = aal_ploam_correlation_set(0, correlation_tag);

    /* Calibration record status update into HW */
    if (us_pon_id%2)    //odd us channel
    {
        rc = aal_ploam_calibration_odd_get(0, &calibration_status);
        move = (15 - us_pon_id)/2;
        value = (0xF << move);
        calibration_temp = calibration_status & (~value);
        value = (CA_NGPON2_CALIBRATION_RECORD_UNCALIBRATED << move);
        calibration_temp |= value;
        rc = aal_ploam_calibration_odd_set(0, calibration_temp);

    }
    else    //even us channel
    {
        rc = aal_ploam_calibration_even_get(0, &calibration_status);
        move = (14 - us_pon_id)/2;
        value = (0xF << move);
        calibration_temp = calibration_status & (~value);
        value = (CA_NGPON2_CALIBRATION_RECORD_UNCALIBRATED << move);
        calibration_temp |= value;
        rc = aal_ploam_calibration_even_set(0, calibration_temp);

    }

     // Update SW previous state record to O2-3;
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O2_3;

    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);/* Clear to let H/W FSM go ahead */

    #if 0
     // Vector clean, to trigger ASIC to respond to SN grants;
    memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    xgpn_actp_status_mask.s.tx_optical_tuning = TRUE;
    xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
    xgpn_actp_status.tx_optical_tuning = FALSE;   // 1'b0: ONU resume US transmission
    xgpn_actp_status.rx_optical_tuning = FALSE;
    xgpn_actp_status.profile_learning = 1;
    rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
    if (CA_E_OK != rc){
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
    }
    #endif

    /*clear mib*/

    __aal_xgpon_clear_mibs(0);

    AAL_XGPON_DBG_ACTFSM("line <%d>, Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);
    return CA_E_OK;
}


/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o12_e_disablesn_change
**
**  Description:
**    XGPON event DISABLE_SN_REQUEST handler for state change @ O1.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o12_e_disablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O1_2 != pre_state) || (AAL_XGPN_ACT_STATE_O7 != new_state) )
        goto ret;

    /* Update SW previous state record to O7;*/
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O7;
    return CA_E_OK;

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_lods_common
**
**  Description:
**    XGPON event LODS common handler
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/
ca_status_t aal_xgpn_act_lods_common(void)
{
    ca_status_t rc = CA_E_OK;
    //ca_uint32_t dwlch_id;
#if 0
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
#endif
    // Discard all burst profiles record;
    //dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F;  // this dwlch_id?
    memset(g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].burst_profile, 0, CA_XGPON_MAX_BURST_PROFILE_COUNT*sizeof(ca_xgpn_burst_profile_t));

    if (ONU_PON_MAC_NGPON2 != aal_pon_mac_mode_get(0))
        return CA_E_OK;
    
    //Update SW previous state record to O1.1;
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;

#if 0
    //Clear channel_configured record in ca_ngpon2_channel_mgmt_t structure;
    g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.pre_running_configured_count = 0;


    // TODO: recover pre-working channel, necessary or not? TBD
    dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F;
    //if (ds_pon_id valid in pre_working_channel):
    {
        //Config HW TC layer and transceiver RX parameter according to ds_pon_id in pre_working_channel;
        // TODO: Config HW TC layer and transceiver RX parameter according to ds_pon_id in pre_working_channel
        g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id =  g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id;
        g_ngpon2_channel_mgmt_cb.pre_working_channel.pre_running_configured_count ++;
        //if pre_running_configured_count exceed max,
        // TODO: clear pre_working_channel and stop??, controlled by debug CLI command)
    }
#endif
    //aal_puc_voq_flush();
    //__aal_xgpon_gem_enable(0,1,__AAL_MAX_GEM_IDX-1,0);
    //aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning();
    //aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);

#if 0
    //Vector clean, to trigger ASIC to continue DSYNC checking;
    memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
    xgpn_actp_status.rx_optical_tuning = FALSE;   // 1'b0: ONU resume DS synchronization
    rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
    if (CA_E_OK != rc){
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
    }
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)
    /*disable RGMII TX and RX*/
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_TXMAC_CFG, tx_en, 0);
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_RXMAC_CFG, rx_en, 0);
#endif

    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);

    if (g_xgpon_work_around) {
        aal_xgpon_datapath_reset(AAL_DEFAUL_DEV_ID);
        aal_xgpon_reset(AAL_DEFAUL_DEV_ID, AAL_XGPON_RESET_FLAG_ALL);
    }

#if 0
    aal_pon_xge_set(AAL_DEFAUL_DEV_ID, 0, 0); // disable XGE RX/TX firstly before other actions
    /* Flush VOQ and User XGEM ports */
    (void)aal_puc_voq_flush();

    AAL_PON_REG_FIELD_WRITE(TITANGLB_CTRL0, RESET_ALL_XCVR, 0); //force reset = 0, else ONU will hang

    usbg_intr.wrd = AAL_PON_REG_READ(XGPN_USBG_INTERRUPT);
    if (usbg_intr.bf.xgem_port_disable_intI) {
        ca_printf("detect xgem_port_disable_intI, need reset IDP.\r\n");
        AAL_PON_REG_FIELD_WRITE(XGPN_USBG_INTERRUPT, xgem_port_disable_intI, 1);
        /* reset IDP for ASIC IDP hang issue */
        (void)aal_idp_reset(AAL_DEFAUL_DEV_ID);
        //AAL_PON_REG_WRITE(0x30E, TITANGLB_FPGA_IDP_CNTL);
    }


    //AAL_PON_REG_FIELD_WRITE(TITANGLB_CTRL0, RESET_ALL_XCVR, 0); //force reset = 0, else ONU will hang

    //aal_pon_xge_set(AAL_DEFAUL_DEV_ID, 0, 0); // disable XGE RX/TX

#if 0 // TODO: re-enable after OMCI resotre is OK
    (void)__aal_xgpon_gem_enable(AAL_DEFAUL_DEV_ID, 1, AAL_XGPON_MAX_GEM_PORT_IDX, FALSE);

    /* reset PUC and init, all TCONT/GEM cfg should be restored from OMCI */
    aal_puc_init(AAL_DEFAUL_DEV_ID, aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID));
#endif

    /* reset clock sequence for FPGA serdes out of clock issue */
    aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(AAL_DEFAUL_DEV_ID, AAL_CLK_SEQ_RESET_TRY_COUNT);

    // aal_pon_xge_set(AAL_DEFAUL_DEV_ID, 0, 0); // disable XGE RX/TX

#endif

    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o12_e_lods_change
**
**  Description:
**    XGPON event LODS handler for state change @ O1.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o12_e_lods_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        return CA_E_OK;

    if ((AAL_XGPN_ACT_STATE_O1_2 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        return CA_E_ERROR;

    return aal_xgpn_act_lods_common();

}


/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o12_e_dwlchnotappropriate_change
**
**  Description:
**    XGPON event DWLCH_NOT_APPROPRIATE handler for state change @ O1.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o12_e_dwlchnotappropriate_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;
    ca_uint32_t index;//, dwlch_id;
    ca_uint8_t dwlch_config_index;
#if 0
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
#endif

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O1_2 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        goto ret;

    AAL_XGPON_DBG_ACTFSM("line <%d>, Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);
    // Discard all burst profiles record;
    //dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F; // this dwlch_id?
    memset(g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].burst_profile, 0, CA_XGPON_MAX_BURST_PROFILE_COUNT*sizeof(ca_xgpn_burst_profile_t));

     //change DS channel (From 989.3: The ONU may repeat the process for two or more available downstream wavelength channels, and may create and store the calibration record for those channels.)
    //DS wavelength configured before, but not synced with OLT;
    dwlch_config_index = g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index;
    g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_calibration_record[dwlch_config_index] = CA_NGPON2_CALIBRATION_RECORD_LOOSE; //update record: loose
    for (index = dwlch_config_index + 1; index < CA_NGPON2_MAX_DWLCH; index ++) // select another channel for dsync try next
    {
        if (g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_dn[index] &&
          (CA_NGPON2_CALIBRATION_RECORD_UNCALIBRATED == g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_calibration_record[index]))    // this DCHID is support and not calibrated yet
        {
            // TODO: Config transceiver RX parameter according to this DCHID index;
            ca_printf("%s::%d::Channel set para:index 0x%x.\r\n",__FUNCTION__,__LINE__, index);
            if(aal_pon_mac_mode_get(0) == ONU_PON_MAC_NGPON2)
            {
                rc = aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL);
            }
            g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index = index;
            break;
        }
    }
    if (CA_NGPON2_MAX_DWLCH == index)
    {
        // TODO: debug CLI command to be added to configure channel selection manually
        // TODO: Donggun>Also need CLI to set channel lamda offset manually.
        /*If (debug_flag)
        {
            dwlch_config_index = debug_index;
            Config transceiver RX parameter according to this debug index;
        }*/
        //none synced during all DWLCH transceiver supported, error
    }
    //if transceiver support RX frequency tuning, different offset setting needs to be added here

    index = g_ngpon2_channel_mgmt_cb.channel_configured.ds_channel_profile_index;
    if ((index < CA_NGPON2_MAX_DWLCH)    //this channel record to be updated
        && (g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].channel_profile_idx & 0x04)) //The channel profile pertains to the TWDM channel in which it is transmitted
        g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].calibration_record_ds.calibration_record_status = CA_NGPON2_CALIBRATION_RECORD_LOOSE; //update record: loose
#if 0    //or by this way
    dwlch_id = g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id & 0x0F;
    if (dwlch_id < CA_NGPON2_MAX_DWLCH)    //this channel record to be updated
    {
        //Get channel profile CB by value ds_pon_id;
        for (index = 0; index < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT; index ++)
        {
            if ((dwlch_id == (g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].pon_id & 0x0F))
                && (g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].channel_profile_idx & 0x04)) //The channel profile pertains to the TWDM channel in which it is transmitted
                g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].calibration_record_ds.calibration_record_status = CA_NGPON2_CALIBRATION_RECORD_LOOSE; //update record: loose
        }
    }
#endif

     //Clear channel_configured record in ca_ngpon2_channel_mgmt_t structure;
    g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.pre_running_configured_count = 0;
    g_ngpon2_channel_mgmt_cb.channel_configured.ds_channel_profile_index = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.us_channel_profile_index = 0xFF;


     //Update SW previous state record to O1.1;
     g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;

    aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);
#if 0
     //Vector clean, to trigger ASIC to continue DSYNC checking;
    memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
    xgpn_actp_status.rx_optical_tuning = FALSE;   // 1'b0: ONU resume DS synchronization
    rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
    if (CA_E_OK != rc){
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
    }
#endif

    AAL_XGPON_DBG_ACTFSM("line <%d>, Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);
    return CA_E_OK;
ret:
    return rc;
}


/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o23_e_onuidassign_change
**
**  Description:
**    XGPON event ONU_ID_ASSIGNMENT handler for state change @ O2-3
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o23_e_onuidassign_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
#if 0
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
#endif
    ca_status_t rc = CA_E_ERROR;
    ca_uint32_t index, uwlch_id;
//    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
 //   aal_xgpon_actp_status_t xgpn_actp_status;
#if 0
    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;
#endif
    /*if ((AAL_XGPN_ACT_STATE_O2_3 != pre_state) || (AAL_XGPN_ACT_STATE_O4 != new_state) )
        goto ret;*/     // comments as HW state change too fast, to O5.1

    //AAL_XGPON_DBG_ACTFSM("line <%d>, Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);

    //aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE); // ldchen: move to the behand of sw state update
    #if 0
    memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    xgpn_actp_status_mask.s.tx_optical_tuning = TRUE;
    xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
    xgpn_actp_status.tx_optical_tuning = FALSE;   // 1'b0: ONU resume US transmission
    xgpn_actp_status.rx_optical_tuning = FALSE;
    xgpn_actp_status.profile_learning = 1;
    rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
    if (CA_E_OK != rc){
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
    }
    #endif
    //According to channel_configured.us_pon_id, update calibration_record_status in corresponding channel profile calibration record;
    uwlch_id = g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id & 0x0F;
    if (uwlch_id < CA_NGPON2_MAX_DWLCH)    //this channel record to be updated
    {
        //Get channel profile CB by value ds_pon_id;
        for (index = 0; index < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT; index ++)
        {
            if (uwlch_id == (g_ngpon2_channel_mgmt_cb.profile_record[0/*uwlch_id*/].twdm_channel_profile[index].uwlch_id & 0x0F))
                g_ngpon2_channel_mgmt_cb.profile_record[0/*uwlch_id*/].twdm_channel_profile[index].calibration_record_us.calibration_record_status = CA_NGPON2_CALIBRATION_RECORD_SUFFICIENT; //update record: sufficient
        }
    }

     //Update ONU-ID if necessary
    // TODO:

     //Update SW previous state record to O4;

    g_xgpn_act_state = new_state;

    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);

     if (new_state != AAL_XGPN_ACT_STATE_O4)
        __AAL_XGPN_ERROR("HW state is move too fast, is now %s\r\n", xgpn_act_state[g_xgpn_act_state]);

    AAL_XGPON_DBG_ACTFSM("Current state: %s\r\n", xgpn_act_state[g_xgpn_act_state]);

    return CA_E_OK;
}


/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o23_e_disablesn_change
**
**  Description:
**    XGPON event DISABLE_SN_REQUEST handler for state change @ O2-3
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o23_e_disablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O2_3 != pre_state) || (AAL_XGPN_ACT_STATE_O7 != new_state) )
        goto ret;

    //Update SW previous state record to O7;
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O7;

    return CA_E_OK;
ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o23_e_txtuningreq_change
**
**  Description:
**    XGPON event TX_TUNING_REQ handler for state change @ O2-3
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o23_e_txtuningreq_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;
    ca_uint32_t us_pon_id;
//    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
 //   aal_xgpon_actp_status_t xgpn_actp_status;
    ca_uint16_t correlation_tag = 0;
    ca_uint32_t calibration_status = 0;
    ca_uint32_t move, value, calibration_temp;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    //if ((AAL_XGPN_ACT_STATE_O2_3 != pre_state) || (AAL_XGPN_ACT_STATE_O2_3 != new_state) )
    if (AAL_XGPN_ACT_STATE_O2_3 != new_state )  // workaround HW pre-state incorrect bug
        goto ret;

    AAL_XGPON_DBG_ACTFSM("line <%d>, Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);

     // The ONU makes the best effort to tune its transmitter to the specified operating upstream wavelength channel
    // TODO: Mainly tune transceiver frequency offset at central frequency(same PON_ID), TX power level.

#if 0
    if (TRUE == g_ngpon2_channel_mgmt_cb.calibration_request.calibration_request_flag)
    {
        /*config transceiver TX parameter according to
            1) calibration_request.target_us_pon_id;
            2) tuning capability of transceiver;
            //to be enriched later */
        // TODO:

        //Update TX calibration record according to tuning configuration, same as Adjust_Tx_wavelength;  //to be enriched later;
        // TODO:
        g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = g_ngpon2_channel_mgmt_cb.calibration_request.target_us_pon_id;
        g_ngpon2_channel_mgmt_cb.calibration_request.calibration_request_flag = FALSE;
    }
    else //get default upstream channel according to ds configured:
    {
        /*Get channel profile US PON ID according to channel_configured.ds_pon_id;
            channel_configured.ds_pon_id -> channel profile -> default_response_channel*/
        // TODO: Above correct ??
        us_pon_id = g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id;
        //config transceiver TX parameter according to US PON ID found and tuning capability of transceiver; //to be enriched later
        // TODO:
        //Update TX calibration record according to tuning configuration, same as Adjust_Tx_wavelength;  //to be enriched later;
        // TODO:
        //channel_configured.us_pon_id = US PON ID found    ?? correct?
    }
#endif

    //According to channel_configured.us_pon_id, write Correlation tag and Calibration record status into corresponding CSRs     //prepare for HW SN reporting;

    us_pon_id = g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id;

    /*correlation tag is an ONU-generated non-zero 16-bit field, which should take a different value each time the transmitter optical power or frequency are changed.*/
    correlation_tag = (us_pon_id & 0xFF) << 8;  // Frequency or channel
    correlation_tag |= 0;   // TODO: transmitter optical power, 0 temporarily NOW
    rc = aal_ploam_correlation_set(0, correlation_tag);
    /* Calibration record status update into HW */
    if (us_pon_id%2)    //odd us channel
    {
        rc = aal_ploam_calibration_odd_get(0, &calibration_status);
        move = (15 - us_pon_id)/2;
        value = (0xF << move);
        calibration_temp = calibration_status & (~value);
        value = (CA_NGPON2_CALIBRATION_RECORD_LOOSE << move);
        calibration_temp |= value;
        g_ngpon2_channel_mgmt_cb.calibration_record.us_channel_calibration_record_odd = calibration_temp;
        rc = aal_ploam_calibration_odd_set(0, calibration_temp);
    }
    else    //even us channel
    {
        rc = aal_ploam_calibration_even_get(0, &calibration_status);
        move = (14 - us_pon_id)/2;
        value = (0xF << move);
        calibration_temp = calibration_status & (~value);
        value = (CA_NGPON2_CALIBRATION_RECORD_LOOSE << move);
        calibration_temp |= value;
        g_ngpon2_channel_mgmt_cb.calibration_record.us_channel_calibration_record_even = calibration_temp;
        rc = aal_ploam_calibration_even_set(0, calibration_temp);

    }

    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
    #if 0
     // Vector clean, to trigger ASIC to respond to SN grants;
    memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    xgpn_actp_status_mask.s.tx_optical_tuning = TRUE;
    xgpn_actp_status.tx_optical_tuning = FALSE;   // 1'b0: ONU resume US transmission
    rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
    if (CA_E_OK != rc){
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
    }
    #endif
    AAL_XGPON_DBG_ACTFSM("line <%d>, Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);


ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o23_e_lods_change
**
**  Description:
**    XGPON event LODS handler for state change @ O2-3
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o23_e_lods_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O2_3 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1!= new_state) )
        goto ret;

    rc = aal_xgpn_act_lods_common();

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o23_e_tozexpire_change
**
**  Description:
**    XGPON event TOZ_EXPIRE handler for state change @ O2-3
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o23_e_tozexpire_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
#ifdef NEW_ACT_FSM
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O1_1 == new_state),
        CA_E_CONFLICT);

    aal_xgpn_act_lods_common();

    if (AAL_XGPON_ACTFSM_CHN_SCAN_IS_ON()) {/* Goto O1.1 and do channel scan next */
        aal_xgpon_actp_profile_learning_set(AAL_DEFAUL_DEV_ID, AAL_XGPON_ACTP_STS_DWLCH_NOT_OK);
        __xgpn_act_fsm_sync_expire_handle();
    }

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;
}
#else
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O2_3 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        goto ret;

     /*do not discard all system and channel profiles;
    Some confliction from G.989.3:
    a)    Table 12-1 O2-3: If the discovery timer TOZ expires without the ONU receiving the feedback from the OLT CT,
        the ONU abandons the host wavelength channel and returns to O1 to search for an alternative downstream wavelength channel.
        In this case the ONU discards any collected system, channel, and burst profile information.
    b)    Table 12-4 Timer TOZ expires: Discard I; ==> O1.1; for I, in table 12-5, only Burst profile parameters included.

    According to sync with Nathan, we will follow Table 12-4 to do the design, as in 12-4,
    the behavior is described more clearly in Table 12-4.
    */
    // TODO: need do lods_common?
    rc = aal_xgpn_act_lods_common();

    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;
ret:
    return rc;
}
#endif

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o23_e_calibrationreq_change
**
**  Description:
**    XGPON event CALIBRATION_REQUEST handler for state change @ O2-3
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o23_e_calibrationreq_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;
    ca_uint32_t index, us_pon_id;//, dwlch_id
#if 0
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
#endif
    ca_uint32_t calibration_status = 0;
    ca_uint32_t move, value, calibration_temp;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O2_3 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        goto ret;

    AAL_XGPON_DBG_ACTFSM("line <%d>, Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);

    // Discard all burst profiles record;
    //dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F; // this dwlch_id?
    memset(g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].burst_profile, 0, CA_XGPON_MAX_BURST_PROFILE_COUNT*sizeof(ca_xgpn_burst_profile_t));

    // Scan structure ca_ngpon2_channel_mgmt_t:
    if (TRUE == g_ngpon2_channel_mgmt_cb.calibration_request.calibration_request_flag)
    {
        //Upon receiving the OLT CT feedback, the ONU may create and store a calibration record for the upstream wavelength channel.
        /* Calibration record status update into HW */
        us_pon_id = g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id;
        if (us_pon_id%2)    //odd us channel
        {
            rc = aal_ploam_calibration_odd_get(0, &calibration_status);
            CA_RET_VALUE_CHECK(rc);
            move = (15 - us_pon_id)/2;
            value = (0xF << move);
            calibration_temp = calibration_status & (~value);
            value = (CA_NGPON2_CALIBRATION_RECORD_UNCALIBRATED << move);
            calibration_temp |= value;
            g_ngpon2_channel_mgmt_cb.calibration_record.us_channel_calibration_record_odd = calibration_temp;
            rc = aal_ploam_calibration_odd_set(0, calibration_temp);
            CA_RET_VALUE_CHECK(rc);
        }
        else    //even us channel
        {
            rc = aal_ploam_calibration_even_get(0, &calibration_status);
            CA_RET_VALUE_CHECK(rc);
            move = (14 - us_pon_id)/2;
            value = (0xF << move);
            calibration_temp = calibration_status & (~value);
            value = (CA_NGPON2_CALIBRATION_RECORD_UNCALIBRATED << move);
            calibration_temp |= value;
            g_ngpon2_channel_mgmt_cb.calibration_record.us_channel_calibration_record_even = calibration_temp;
            rc = aal_ploam_calibration_even_set(0, calibration_temp);
            CA_RET_VALUE_CHECK(rc);
        }

        //Clear channel_configured record in ca_ngpon2_channel_mgmt_t structure;
        g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = 0xFF;
        g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = 0xFF;
        g_ngpon2_channel_mgmt_cb.channel_configured.pre_running_configured_count = 0;

        //Config HW TC layer and transceiver RX parameter according to target_ds_pon_id;
        // TODO: Config HW TC layer and transceiver RX parameter according to target_ds_pon_id
        index = g_ngpon2_channel_mgmt_cb.calibration_request.target_ds_pon_id;
        ca_printf("%s::%d::Channel set para:index 0x%x.\r\n",__FUNCTION__,__LINE__, index);
        if(aal_pon_mac_mode_get(0) == ONU_PON_MAC_NGPON2)
        {
            rc = aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL);
            CA_RET_VALUE_CHECK(rc);
        }
        g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index = index;
        g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = index;

    }

    //Update SW previous state record to O1.1;
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;

    aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);
    //aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);
#if 0
    //Vector clean, to trigger ASIC to continue DSYNC checking;
    memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
    xgpn_actp_status.rx_optical_tuning = FALSE;   // 1'b0: ONU resume DS synchronization
    rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
    if (CA_E_OK != rc){
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
    }
#endif

    AAL_XGPON_DBG_ACTFSM("line <%d>, Current state: %s\r\n", __LINE__, xgpn_act_state[g_xgpn_act_state]);

    return CA_E_OK;

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_deactiveonuid_common
**
**  Description:
**    XGPON event DEACTIVE_ONUID_REQ common handler
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_deactiveonuid_common(void)
{
    ca_status_t rc = CA_E_OK;
//    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    //ca_uint32_t dwlch_id;
#if 0
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
#endif

    // Discard all burst profiles record;
    //dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F;  // this dwlch_id?
    memset(g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].burst_profile, 0, CA_XGPON_MAX_BURST_PROFILE_COUNT*sizeof(ca_xgpn_burst_profile_t));

     //Update ONU ID record if necessary;
     // TODO: Update ONU ID record if necessary;

    //Update SW previous state record to O1.1;
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;

    // TODO: recover pre-working channel, necessary or not? TBD
#if 0
    //Clear channel_configured record in ca_ngpon2_channel_mgmt_t structure;
    g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.pre_running_configured_count = 0;

    dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F;
    //if (ds_pon_id valid in pre_working_channel):
    {
        //Config HW TC layer and transceiver RX parameter according to ds_pon_id in pre_working_channel;
        // TODO: Config HW TC layer and transceiver RX parameter according to ds_pon_id in pre_working_channel
        g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id =  g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id;
        g_ngpon2_channel_mgmt_cb.pre_working_channel.pre_running_configured_count ++;
        //if pre_running_configured_count exceed max,
        // TODO: clear pre_working_channel and stop??, controlled by debug CLI command)
    }

    //Vector clean, to trigger ASIC to continue DSYNC checking;
    // TODO: Vector clean, to trigger ASIC to continue DSYNC checking
#endif
    //aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);
    //if (ONU_PON_MAC_NGPON2 == aal_pon_mac_mode_get(0))
      //  aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
#if 0
    //to trigger ASIC to continue DSYNC checking;
    memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
    xgpn_actp_status_mask.s.tx_optical_tuning = TRUE;
    xgpn_actp_status.rx_optical_tuning = FALSE;   // 1'b0: ONU resume DS synchronization
    xgpn_actp_status.tx_optical_tuning = FALSE;
    rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
    if (CA_E_OK != rc){
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
    }
#endif


#if defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)
    /*disable RGMII TX and RX*/
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_TXMAC_CFG, tx_en, 0);
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_RXMAC_CFG, rx_en, 0);
#endif

    /*US/DS GEM/TCONT/ALLOC ID tables when ONU de-activate*/
    __aal_xgpon_clear_gem_tcont_tbl(AAL_DEFAUL_DEV_ID);

    if (ONU_PON_MAC_NGPON2 == aal_pon_mac_mode_get(0)) {
        /* Set Eqd to hardware */
        __xgpn_act_fsm_load_chn_eqd(AAL_DEFAUL_DEV_ID,
            __xgpn_act_fsm_get_current_us_chn(AAL_DEFAUL_DEV_ID));

        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
        //aal_puc_voq_flush();

        if (g_xgpon_work_around) {
            aal_xgpon_datapath_reset(AAL_DEFAUL_DEV_ID);
            aal_xgpon_reset(AAL_DEFAUL_DEV_ID, AAL_XGPON_RESET_FLAG_MAC);
        }
    }
    
    AAL_XGPON_DBG_ACTFSM("ONU De-activate handle done.\r\n");

    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o23_e_deactiveonuid_change
**
**  Description:
**    XGPON event DEACTIVE_ONUID_REQ handler for state change @ O2-3
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o23_e_deactiveonuid_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;
    //ca_uint32_t dwlch_id;

    if ((ONU_PON_MAC_NGPON2 != onu_pon_mac_mode) && (ONU_PON_MAC_XGSPON != onu_pon_mac_mode))
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O2_3 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        goto ret;

    rc = aal_xgpn_act_deactiveonuid_common();

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o4_e_eqdassign_change
**
**  Description:
**    XGPON event EQD_ASSIGNMENT handler for state change @ O4
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o4_e_eqdassign_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
#ifdef NEW_ACT_FSM
{
    /*
     *  If Wavelength protection enable, ONU transfer from O6 state to O8.1
     *  when TO2 timer expire, otherwise ONU transfer to O1.1
     */
    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O5_1 == new_state),
        CA_E_CONFLICT);

     /* Update the parameters in tuning_control */
    __xgpn_act_fsm_tuning_flags_set(AAL_DEFAUL_DEV_ID,
        FALSE, FALSE);

    ACT_FSM_SET_STATE(AAL_XGPN_ACT_STATE_O5_1);

    return CA_E_OK;

}
#else
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O4 != pre_state) || (AAL_XGPN_ACT_STATE_O5_1 != new_state) )
        goto ret;

    //AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);
    AAL_XGPON_DBG_ACTFSM("Current state: %s\r\n", xgpn_act_state[g_xgpn_act_state]);

    // Update the parameters in pre_working_channel:
    g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id = g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id;
    g_ngpon2_channel_mgmt_cb.pre_working_channel.us_pon_id = g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id;
    g_ngpon2_channel_mgmt_cb.pre_working_channel.pre_running_configured_count = 0;

     // Update the parameters in tuning_control:
     g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag = FALSE; //tuning request operation done
     g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag = FALSE;


    /* Set Eqd to hardware */
    __xgpn_act_fsm_load_chn_eqd(AAL_DEFAUL_DEV_ID,
        __xgpn_act_fsm_get_current_us_chn(AAL_DEFAUL_DEV_ID));

    // Update SW previous state record to O5.1;
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O5_1;

    //AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);
    AAL_XGPON_DBG_ACTFSM("Current state: %s\r\n", xgpn_act_state[g_xgpn_act_state]);

#if defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)
    /*Enable RGMII TX and RX*/
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_TXMAC_CFG, tx_en, 1);
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_RXMAC_CFG, rx_en, 1);
#endif

    return CA_E_OK;

ret:
    return rc;
}
#endif

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o4_e_disablesn_change
**
**  Description:
**    XGPON event DISABLE_SN_REQUEST handler for state change @ O4
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o4_e_disablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O4 != pre_state) || (AAL_XGPN_ACT_STATE_O7 != new_state) )
        goto ret;

    /* Update SW previous state record to O7;*/
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O7;

    return CA_E_OK;

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o4_e_lods_change
**
**  Description:
**    XGPON event LODS handler for state change @ O4
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o4_e_lods_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O4 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        goto ret;

    rc = aal_xgpn_act_lods_common();

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o4_e_deactiveonuid_change
**
**  Description:
**    XGPON event DEACTIVE_ONUID_REQ handler for state change @ O4
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o4_e_deactiveonuid_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O4 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        goto ret;

    rc = aal_xgpn_act_deactiveonuid_common();

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o51_e_tuningreq_change
**
**  Description:
**    XGPON event TUNING_REQUEST handler for state change @ O5.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/
#define __NGPON2_TUNING_LEAD_SFC  2
static inline void __xgpn_act_fsm_tuning_accept(ca_boolean_t skip_rx_tuning)
{
    /* Set tuning config */
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_TUNING_CONFIG, skip_rx,
        skip_rx_tuning);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_TUNING_CONFIG, rollback_allow,
        g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_TUNING_CONFIG, request_lead_sfc,
        __NGPON2_TUNING_LEAD_SFC);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_TUNING_CONFIG, request_schedule_sfc,
        g_ngpon2_channel_mgmt_cb.tuning_control.scheduled_sfc);

    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_ACK);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
    ca_printf("%s::%d:sending ACK success. seqno:0x%x\r\n", __FUNCTION__,__LINE__,g_tun_ctr_seqno);

}

static inline void __xgpn_act_fsm_tuning_reject(void)
{
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_NACK);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, __NACK_DS_VOID);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
    ca_printf("%s::%d:sending NACK success. seqno:0x%x\r\n", __FUNCTION__,__LINE__,g_tun_ctr_seqno);

}


static ca_boolean_t  __xgpn_act_fsm_tuning_exam(
    ca_xgpn_twdm_channel_profile_t *chn_prof)
{
    ca_uint8_t chn = 0;
    ca_uint8_t ds_chn = 0;
    ca_uint8_t us_chn = 0;
    ca_uint8_t cpi = 0;

    ds_chn = __xgpn_act_fsm_get_target_ds_chn(AAL_DEFAUL_DEV_ID);
    us_chn = __xgpn_act_fsm_get_target_us_chn(AAL_DEFAUL_DEV_ID);

    if (__XCVR_CHN_IS_VALID(us_chn)) {
        NGPON2_TUN_DEBUG("XCVR cannot support target US channel %d\n", us_chn);
        return FALSE;
    }


    if (!g_ngpon2_channel_mgmt_cb.profile_record[0].channel_profile_count_learnt) {
        NGPON2_TUN_DEBUG("NOT learn any channel profile yet.\n");
        return FALSE;
    }

    FOR_ALL_TWDM_CHN_DO(chn) {
        if (!g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[chn].learnt_flag) {
            continue;
        }

        if (g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[chn].uwlch_id == us_chn) {
            /* Find channel profile for the target channel */
            cpi = AAL_PON_SCFG_GET_CPI(AAL_DEFAUL_DEV_ID);
            if (cpi && cpi == g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[chn].channel_partition) {
                chn_prof = &g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[chn];
                return TRUE;
            } else if (!cpi) {
                chn_prof = &g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[chn];
                return TRUE;
            }
        }
    }

    return FALSE;
}

ca_status_t aal_xgpn_act_fsm_s_o51_e_tuningreq_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
#ifdef NEW_ACT_FSM
{
    ca_uint32_t  retry_times = 100;
    ca_boolean_t accept_tuning = FALSE;
    ca_boolean_t skip_rx = FALSE;
    ca_xgpn_twdm_channel_profile_t chn_prof;

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET(AAL_XGPN_ACT_STATE_O5_2 == new_state,
        CA_E_CONFLICT);

    while(retry_times--) {
        if (g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag)
            break;
    }

    if (!retry_times) {
        NGPON2_TUN_DEBUG("NOT learnt tuning profile.\n");
        return CA_E_TIMEOUT;
    }

    memset(&chn_prof, 0, sizeof(chn_prof));
    accept_tuning = __xgpn_act_fsm_tuning_exam(&chn_prof);
    if (accept_tuning) {
        if (__GET_CHN_FROM_PONID(chn_prof.pon_id) != __xgpn_act_fsm_get_target_ds_chn(AAL_DEFAUL_DEV_ID)){
            skip_rx = TRUE;
        }
        __xgpn_act_fsm_tuning_accept(skip_rx);
    } else {
        __xgpn_act_fsm_tuning_reject();
    }

    return CA_E_OK;
}
#else
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_OK;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O5_1 != pre_state) || (AAL_XGPN_ACT_STATE_O5_2 != new_state) )
        goto ret;
    // TODO:
    /*check */

    /* Update SW previous state record to O5.2;*/
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O5_2;
    return CA_E_OK;

ret:
    return rc;
}
#endif

ca_status_t aal_xgpn_act_fsm_s_o51_e_tuningreq_ack
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_OK;
    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;
    if ((AAL_XGPN_ACT_STATE_O5_1 != pre_state) || (AAL_XGPN_ACT_STATE_O5_2 != new_state) )
        goto ret;

    //xgpon_ploam_key_exchange_fsm_init(0);
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O5_2;
    AAL_PON_REG_FIELD_WRITE(PUC_BTCCFG, schmode, 1);

    if ((aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK)
        && ( aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_FORCE_HW_FSM)){

        aal_xgpon_actp_ctrl_mask_t  actp_ctrl_mask;
        aal_xgpon_actp_ctrl_t        actp_ctrl;
        aal_xgpon_actp_status_mask_t  actp_status_mask;
        aal_xgpon_actp_status_t      actp_status;

        actp_ctrl_mask.u32_data = 0;
        actp_ctrl_mask.s.sw_mode            = 1;
        actp_ctrl.sw_mode                   = 1;
        (void)aal_xgpon_actp_ctrl_set(AAL_DEFAUL_DEV_ID, actp_ctrl_mask, &actp_ctrl);

        actp_status_mask.u32_data = 0;
        actp_status_mask.s.onu_state = 1;
        actp_status_mask.s.onu_sub_state = 1;
        /* Force HW to go to O52 */
        actp_status.onu_state = AAL_XGPON_ACTP_STATE_O5;
        actp_status.onu_sub_state = AAL_XGPON_ACTP_SUB_STATE_OX2;
        aal_xgpon_actp_status_set(AAL_DEFAUL_DEV_ID, actp_status_mask, &actp_status);

        actp_ctrl.sw_mode = 0;
        (void)aal_xgpon_actp_ctrl_set(AAL_DEFAUL_DEV_ID, actp_ctrl_mask, &actp_ctrl);
        ca_printf("%s::%d::Force FSM go to O8.1 by SW.\r\n",__FUNCTION__,__LINE__);
    }

    /* Stop Rx traffic, and wait upstream go out */

    return CA_E_OK;

ret:
    return rc;
}
/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o51_e_disablesn_change
**
**  Description:
**    XGPON event DISABLE_SN_REQUEST handler for state change @ O5.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o51_e_disablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O5_1 != pre_state) || (AAL_XGPN_ACT_STATE_O7 != new_state) )
        goto ret;

    //xgpon_ploam_key_exchange_fsm_init(0);
    /* Update SW previous state record to O7;*/
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O7;
    return CA_E_OK;

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o51_e_lods_change
**
**  Description:
**    XGPON event LODS handler for state change @ O5.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o51_e_lods_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O5_1 != pre_state) || (AAL_XGPN_ACT_STATE_O6 != new_state) )
        goto ret;
    AAL_PON_REG_WRITE(0,XGPN_ACTP_STS);
    //aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);
    /* Update SW previous state record to O6;*/
    if(aal_xgpn_protect_switch_sim_flag){
        __xgpn_protect_switch_simulation(0);
    }
    //xgpon_ploam_key_exchange_fsm_init(0);
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O6;
    return CA_E_OK;

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o51_e_deactiveonuid_change
**
**  Description:
**    XGPON event DEACTIVE_ONUID_REQ handler for state change @ O5.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o51_e_deactiveonuid_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_OK;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    /*if ((AAL_XGPN_ACT_STATE_O5_1 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        goto ret;*/    // workaround for HW moving too fast
    //xgpon_ploam_key_exchange_fsm_init(0);
    rc = aal_xgpn_act_deactiveonuid_common();

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o52_e_lods_change
**
**  Description:
**    XGPON event LODS handler for state change @ O5.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/01/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o52_e_lods_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_OK;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O5_2 != pre_state) || (AAL_XGPN_ACT_STATE_O6 != new_state) )
        goto ret;
    AAL_PON_REG_WRITE(0,XGPN_ACTP_STS);
    //aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);
    /* Update SW previous state record to O6;*/
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O6;
    return CA_E_OK;

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o52_e_sfcmatchtxtuning_change
**
**  Description:
**    XGPON event SFC_MATCH_FOR_TX_TUNING handler for state change @ O5.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o52_e_sfcmatchtxtuning_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    //ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;
    //static ca_uint8_t  cur_prof = 0;

    NGPON2_MODE_CHK_RET(CA_E_NOT_SUPPORT);

    if ((AAL_XGPN_ACT_STATE_O5_2 != pre_state) || (AAL_XGPN_ACT_STATE_O9 != new_state) )
        goto ret;

#if 1 /* simulation ONLY */
    if (aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK) {/* rollback simulation flag */
        aal_xgpon_dsfp_sync_ctrl_config_msk_t  sync_ctrl_mask;
        aal_xgpon_dsfp_sync_ctrl_config_t      sync_ctrl;

        ca_printf("%s::%d::Simuate channel switch from  %d to %d...\r\n",
            __FUNCTION__,__LINE__,g_original_us_chn, g_original_us_chn+1);
        memset(&sync_ctrl, 0, sizeof(sync_ctrl));
        sync_ctrl_mask.u32 = 0;
        sync_ctrl_mask.s.bit_swap_en = 1;
        sync_ctrl.bit_swap_en = 1;
        aal_xgpon_dsfp_sync_ctrl_set(AAL_DEFAUL_DEV_ID,
            sync_ctrl_mask, &sync_ctrl); // LODS

         // Update SW previous state record to O8.1;
        g_xgpn_act_state = new_state;

        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);

        return CA_E_OK;
    }
#endif


    // target channel of the wavelength tuning operation
     if (TRUE == g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag)
    {
        //Config HW TC layer and transceiver TX parameter according to tuning_control.target_us_pon_id;
        // TODO: Config HW TC layer and transceiver TX parameter
        g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id;
        g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id;

        // According to channel_configured.us_pon_id, write Correlation tag and Calibration record status into corresponding CSRs     //prepare for HW SN reporting or PLOAM grant;
        // TODO: write Correlation tag and Calibration record status into CSRs
    }


    g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag = TRUE;
    if (g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag) {
        // TODO: convert the us pon id to physical channel
        if (cur_prof < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT && ACT_FSM_CHK_TUNING_REQ_TX(g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[cur_prof].uwlch_id)){
            ca_printf("%s::%d::Channel set para:target_ds_pon_id 0x%x.\r\n",__FUNCTION__,__LINE__, g_ds_pon_id);
            ca_printf("%s::%d::Channel set para:target_us_pon_id 0x%x.\r\n",__FUNCTION__,__LINE__, g_us_pon_id);
            //rc = aal_xgpon_xfp_chanl_set(0, US_DS_CHANEL, US_DS_CHANEL);
            if(aal_pon_mac_mode_get(0) == ONU_PON_MAC_NGPON2)
            {
                if (g_ds_pon_id != g_original_ds_chn) {
                    ca_printf("%s::%d::Channel Tx ONLY but target_ds_pon_id 0x%x is different as original 0x%d.\r\n",
                        __FUNCTION__,__LINE__, g_ds_pon_id, g_original_ds_chn);
                }

                __xgpn_act_fsm_set_previous_chn(AAL_DEFAUL_DEV_ID,
                    __xgpn_act_fsm_get_previous_us_chn(AAL_DEFAUL_DEV_ID),
                    __xgpn_act_fsm_get_current_ds_chn(AAL_DEFAUL_DEV_ID));

                rc = aal_xgpon_xfp_chanl_set(0, g_ds_pon_id, g_us_pon_id);
            }

            if(CA_E_OK == rc) { /* reset clk sequence after channel switch */
                //reset CLK - needed according to HW
                aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);
            }

            g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id;
            g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id;

            cur_prof ++;
        }
        #if 1
        if (CA_E_OK == rc) {
            ca_printf("%s::%d::tx_tuning_request interrupt handle, send out complete_u!!\r\n",__FUNCTION__,__LINE__);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_COMPLETE);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, 0);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
        }else {
            // TODO: if all fail, should do rollback
            ca_printf("%s::%d::tx_tuning_request interrupt handle, send out nack!!\r\n",__FUNCTION__,__LINE__);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_NACK);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, __NACK_US_VOID);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
        }
        #endif
    }
#if 0 // not rollback here, but wait TO4 timeout
    else {
            ca_printf("%s::%d::tx_tuning_request interrupt handle,send out rollback!!\r\n",__FUNCTION__,__LINE__);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_ROLLBACK);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, __ROLLBACK_US_TUNR);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
    }
#endif
    ACT_FSM_SET_STATE(AAL_XGPN_ACT_STATE_O9);

    //ACT_FSM_TRIG_HW_TX_TUNING();
    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o52_e_sfcmatchrxtxtuning_change
**
**  Description:
**    XGPON event SFC_MATCH_FOR_RXTX_TUNING handler for state change @ O5.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o52_e_sfcmatchrxtxtuning_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_status_t rc = CA_E_ERROR;
    //ca_uint32_t dwlch_id;
    //aal_xgpon_bwmp_alloc_id_cfg_msk_t mask;
    //aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;
    // mask.u32 = 0;
    //memset(&alloc_id_cfg, 0, sizeof(aal_xgpon_bwmp_alloc_id_cfg_t));

    NGPON2_MODE_CHK_RET(CA_E_NOT_SUPPORT); /* only for NGPON2 mode, not for other modes */

    ACT_FSM_HW_STATE_CHK_RET(
        (AAL_XGPN_ACT_STATE_O5_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O8_1 == new_state), CA_E_CONFLICT);
#if 0
    if ((AAL_XGPN_ACT_STATE_O5_2 != pre_state) || (AAL_XGPN_ACT_STATE_O8_1 != new_state) )
        goto ret;
#endif

    // TODO: ldchen: may flusn VoQ when start tuning Tx, disable VoQ here is not good for the traffic in VoQ
    aal_puc_enable_voq(0, FALSE); // TODO: should disable VOQ here, we just disable XGE ??

#if 1 /* simulation ONLY */
    if (aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK) {/* rollback simulation flag */
        aal_xgpon_dsfp_sync_ctrl_config_msk_t  sync_ctrl_mask;
        aal_xgpon_dsfp_sync_ctrl_config_t      sync_ctrl;

        ca_printf("%s::%d::Simuate channel switch from  %d to %d...\r\n",
            __FUNCTION__,__LINE__,g_original_us_chn, g_original_us_chn+1);
        memset(&sync_ctrl, 0, sizeof(sync_ctrl));
        sync_ctrl_mask.u32 = 0;
        sync_ctrl_mask.s.bit_swap_en = 1;
        sync_ctrl.bit_swap_en = 1;
        aal_xgpon_dsfp_sync_ctrl_set(AAL_DEFAUL_DEV_ID,
            sync_ctrl_mask, &sync_ctrl); // LODS
        //aal_clear_actp_rx_tx_optical_tuning(1, 0);

         // Update SW previous state record to O8.1;
        g_xgpn_act_state = AAL_XGPN_ACT_STATE_O8_1;

        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);

        return CA_E_OK;
    }
#endif

    /* we always permit tuning once our FSM come into O8, since the judge is done in O52 */
    g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag = TRUE;
    if (g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag) {
        ca_printf("%s::%d::Channel set para:target_ds_pon_id 0x%x.\r\n",__FUNCTION__,__LINE__, g_ds_pon_id);
        ca_printf("%s::%d::Channel set para:target_us_pon_id 0x%x.\r\n",__FUNCTION__,__LINE__, g_us_pon_id);
        //if(aal_pon_mac_mode_get(0) == ONU_PON_MAC_NGPON2)
        //{
           /* start to tuning Rx&Tx channel */
            rc = aal_xgpon_xfp_chanl_set(0, (g_ds_pon_id & 0x3), (g_us_pon_id & 0x3));
        //}
        /*comment for simulation only, normally should not be commented*/

        //AAL_PON_REG_FIELD_WRITE(XGPN_USBG_GBL_CTRL,laser_polarity,1);
        //AAL_PON_REG_WRITE(0x3,TITANGLB_CTRL1);
        //AAL_PON_REG_FIELD_WRITE(TITANGLB_CTRL1, EN_PON_LASER,1);

        // TODO: may move the clk seq reset to cahnel set API
        aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);

        //mask.s.
        //rc = aal_xgpon_bwmp_tcont_set(0,(g_us_pon_id & 0x3),0,mask,&alloc_id_cfg);

        cur_prof ++;

        if (CA_E_OK == rc) {/* tunning successfully */

            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_COMPLETE);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, 0);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
            ca_printf("%s::%d:sending COMPLETE_u success. seqno:0x%x\r\n", __FUNCTION__,__LINE__,g_tun_ctr_seqno);
        }else {
            //ca_printf("%s::%d::tx_tuning_request interrupt handle, send out nack!!\r\n",__FUNCTION__,__LINE__);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_NACK);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, __NACK_US_VOID);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
            ca_printf("%s::%d:sending NACK success. seqno:0x%x\r\n", __FUNCTION__,__LINE__,g_tun_ctr_seqno);

        }

    }
#if 0 // not rollback here, but wait TO4 timeout
     else {
            //ca_printf("%s::%d::tx_tuning_request interrupt handle,send out rollback!!\r\n",__FUNCTION__,__LINE__);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_ROLLBACK);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, __ROLLBACK_US_TUNR);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
            ca_printf("%s::%d:sending ROLLBACK success. seqno:0x%x\r\n", __FUNCTION__,__LINE__,g_tun_ctr_seqno);

    }
#endif

    /* Set Eqd to hardware */
    __xgpn_act_fsm_load_chn_eqd(AAL_DEFAUL_DEV_ID,
        __xgpn_act_fsm_get_current_us_chn(AAL_DEFAUL_DEV_ID));

    ACT_FSM_SET_STATE(AAL_XGPN_ACT_STATE_O8_1);

    /* clear rx optical tuning flag to let HW state go ahead */
    aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o52_e_disablesn_change
**
**  Description:
**    XGPON event DISABLE_SN_REQUEST handler for state change @ O5.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o52_e_disablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O5_2 != pre_state) || (AAL_XGPN_ACT_STATE_O7 != new_state) )
        goto ret;

    /* Update SW previous state record to O7;*/
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O7;
    return CA_E_OK;

ret:
    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o52_e_deactiveonuid_change
**
**  Description:
**    XGPON event DEACTIVE_ONUID_REQ handler for state change @ O5.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o52_e_deactiveonuid_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t onu_pon_mac_mode = aal_pon_mac_mode_get(0);
    ca_status_t rc = CA_E_ERROR;

    if (ONU_PON_MAC_NGPON2 != onu_pon_mac_mode)
        goto ret;

    if ((AAL_XGPN_ACT_STATE_O5_2 != pre_state) || (AAL_XGPN_ACT_STATE_O1_1 != new_state) )
        goto ret;

    rc = aal_xgpn_act_deactiveonuid_common();

ret:
    return rc;
}


ca_boolean_t g_act_fsm_is_rollbacking = 0;

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o6_e_dsync_change
**
**  Description:
**    XGPON event DSYNC handler for state change @ O6
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o6_e_dsync_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O6 == pre_state)
        && ((AAL_XGPN_ACT_STATE_O5_1 == new_state) || (AAL_XGPN_ACT_STATE_O5_2 ==  new_state)),
        CA_E_CONFLICT);

    ACT_FSM_SET_STATE(new_state);

#if defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)
    /*Enable RGMII TX and RX*/
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_TXMAC_CFG, tx_en, 1);
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_RXMAC_CFG, rx_en, 1);
#endif

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_toxexpire_common
**
**  Description:
**    XGPON event TOx_EXPIRE common handler
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_toxexpire_common(void)
{
    ca_status_t rc = CA_E_OK;
    ca_uint32_t dwlch_id = 0;
#if 0
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
#endif

    // Discard all burst profiles record;
    dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F;
    // TODO: this dwlch_id?  to be thought further
    memset(g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].burst_profile, 0, CA_XGPON_MAX_BURST_PROFILE_COUNT*sizeof(ca_xgpn_burst_profile_t));

    //Clear channel_configured record in ca_ngpon2_channel_mgmt_t structure;
    g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = 0xFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.pre_running_configured_count = 0;

    //Update SW previous state record to O1.1;
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;

    // TODO: recover pre-working channel, necessary or not? TBD
#if 1
    dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F;
    //if (ds_pon_id valid in pre_working_channel):
    {
        //Config HW TC layer and transceiver RX parameter according to ds_pon_id in pre_working_channel;
        // TODO: Config HW TC layer and transceiver RX parameter according to ds_pon_id in pre_working_channel
        g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id =  g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id;
        g_ngpon2_channel_mgmt_cb.pre_working_channel.pre_running_configured_count ++;
        //if pre_running_configured_count exceed max,
        // TODO: clear pre_working_channel and stop??, controlled by debug CLI command)
    }
    aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);
    //aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);

#if defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)
    /*disable RGMII TX and RX*/
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_TXMAC_CFG, tx_en, 0);
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_RXMAC_CFG, rx_en, 0);
#endif

#endif
    aal_puc_voq_flush();

    AAL_XGPON_DBG_ACTFSM("ONU TOx-Expire handle done.\r\n");

    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o6_e_to2expire_change
**
**  Description:
**    XGPON event TO2_EXPIRE handler for state change @ O6
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o6_e_to2expire_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    /*
     *  If Wavelength protection enable, ONU transfer from O6 state to O8.1
     *  when TO2 timer expire, otherwise ONU transfer to O1.1
     */
    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O6 == pre_state)
        && ((AAL_XGPN_ACT_STATE_O1_1 == new_state)
            || (AAL_XGPN_ACT_STATE_O8_1 == new_state)),
        CA_E_CONFLICT);

    (void)aal_xgpn_act_toxexpire_common();

    ACT_FSM_SET_STATE(new_state);

    //aal_xgpon_clk_seq_reset(AAL_DEFAUL_DEV_ID);

    aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, 10);

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o6_e_to3expire_change
**
**  Description:
**    XGPON event TO3_EXPIRE handler for state change @ O6
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o6_e_to3expire_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
#ifdef NEW_ACT_FSM
{
    ACT_FSM_HW_STATE_CHK_RET(AAL_XGPN_ACT_STATE_O8_1 == new_state,
        CA_E_CONFLICT);

    /*need add:protect switch handle*/
    if(aal_xgpn_protect_switch_sim_flag){
        ca_printf("Switch to protected channel.\r\n");

        ca_printf("XGPN_DSFP_sync_ctl:clear bit_swap_enable\r\n");
        AAL_PON_REG_WRITE(0x3002d,XGPN_DSFP_sync_ctl);
    }
    /*
    XGPN_PLMP_TUNING_CONFIG.rollback_allow = 0
    HW state XGPN_ACTP_CTRL.wlcp_enable = 1
    change TX/RX wavelength to proection PON-ID
    SW set XGPN_ACTP_CTRL.wlcp_enable = 0
    discard I, start TO4
    XGPN_ACTP_STS.rx_optical_tuning = 0
    */
    if (g_ngpon2_channel_mgmt_cb.protection_control.protection_ctrl_flag) {

        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_TUNING_CONFIG, rollback_allow, 0);

        ACT_FSM_SET_HW_WLCP_ENA(TRUE);

        aal_xgpon_xfp_chanl_set(AAL_DEFAUL_DEV_ID,
            g_ngpon2_channel_mgmt_cb.protection_control.protection_ds_pon_id & 0xF,
            g_ngpon2_channel_mgmt_cb.protection_control.protection_us_pon_id & 0xF);

        ACT_FSM_SET_HW_WLCP_ENA(FALSE);
        XGPN_ACTFSM_LOG_WARN("Change to protection channels us %d, ds %d\n",
            g_ngpon2_channel_mgmt_cb.protection_control.protection_us_pon_id & 0xF,
            g_ngpon2_channel_mgmt_cb.protection_control.protection_ds_pon_id & 0xF);
    }

    aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);

    ACT_FSM_SET_STATE(new_state);


    return CA_E_OK;
}

#else
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O6 == pre_state)
        && (AAL_XGPN_ACT_STATE_O8_1 == new_state), CA_E_CONFLICT);

    (void)aal_xgpn_act_toxexpire_common();

    /*need add:protect switch handle*/
    if(aal_xgpn_protect_switch_sim_flag){
        ca_printf("Switch to protected channel.\r\n");

        ca_printf("XGPN_DSFP_sync_ctl:clear bit_swap_enable\r\n");
        AAL_PON_REG_WRITE(0x3002d,XGPN_DSFP_sync_ctl);
    }

    aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);

    ACT_FSM_SET_STATE(new_state);
    return CA_E_OK;
}
#endif

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o7_e_enablesn_change
**
**  Description:
**    XGPON event ENABLE_SN_REQUEST handler for state change @ O7
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o7_e_enablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O7 == pre_state)
        && (AAL_XGPN_ACT_STATE_O1_1 == new_state), CA_E_CONFLICT);

    /* Discard burst profiles, keep others */
    ACT_FSM_RESET_BURST_PROF();

    ACT_FSM_SET_STATE(new_state);

    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
    aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);

    return CA_E_OK;


}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o81_e_dsync_change
**
**  Description:
**    XGPON event DSYNC handler for state change @ O8.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o81_e_dsync_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
/*
 * This substate is the entry point to O8.
 * The ONU tunes its receiver attempting to synchronize to the downstream
 * signal. The ONU may also tune its transmitter. Once the downstream
 * synchronization is attained, the ONU transitions to the O8/Profile
 * Learning substate.
 */
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O8_1 == pre_state)
        && (AAL_XGPN_ACT_STATE_O8_2 == new_state), CA_E_CONFLICT);

    /* Learn new burst profile */
    ACT_FSM_RESET_BURST_PROF();
    ACT_FSM_TRIG_HW_PROF_LEAN();

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_txrxtuningreq_common
**
**  Description:
**    XGPON event TX_RX_TUNING_REQ common handler
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_txrxtuningreq_common(void)
{
    ca_status_t rc = CA_E_OK;
    ca_uint32_t index, dwlch_id;
#if 0
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
#endif

    // Discard all burst profiles record;
    dwlch_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id & 0x0F;
    // TODO: this dwlch_id?  to be thought further
    memset(g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].burst_profile, 0, CA_XGPON_MAX_BURST_PROFILE_COUNT*sizeof(ca_xgpn_burst_profile_t));

    //Tuning request from O8.1
    if (0 == g_ngpon2_channel_mgmt_cb.pre_working_channel.pre_running_configured_count)
    {
        if ((TRUE == g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag) &&
            (TRUE == g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag)) // the previous working channel in case of tuning operation rollback
        {
            // Config HW TC layer and transceiver RX parameter according to pre_working_channel.ds_pon_id;
            // TODO: Config HW TC layer and transceiver RX parameter
            // Config transceiver TX parameter according to pre_working_channel us_pon_id;
            // TODO: Config transceiver TX parameter

            // Set Tuning ACK register with ROLLBACK;
            // TODO: Set Tuning ACK register with ROLLBACK

            g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.ds_pon_id;
            g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = g_ngpon2_channel_mgmt_cb.pre_working_channel.us_pon_id;
        }
        g_ngpon2_channel_mgmt_cb.pre_working_channel.pre_running_configured_count ++;
    }
    else    // scan any available TWDM channel with matching parameters
    {
        for (index = 0; index < CA_NGPON2_MAX_DWLCH; index ++)
        {
            if (FALSE == g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].learnt_flag)    //profile not learnt, skip
                continue;
            // TODO: if (twdm_channel_profile[index].ds_freq_offset, ds_rate, channel_partition DO NOT match current downstream channel)    //current ds pon-id does not match this channel
                // continue;
            // Get the channel partition, upstream optical link type, and the upstream line rate parameters based on twdm_channel_profile[index].default_response_channel
            // TODO: if (channel partition, upstream optical link type, and the upstream line rate match those of ONU)    //
                //break;
        }
        if (CA_NGPON2_MAX_DWLCH == index)
            ;// all channel profiles scanned, none match, error handler
        else
        {
            // Config HW TC layer and transceiver RX parameter according to twdm_channel_profile[index].pon_id;
            // TODO: Config HW TC layer and transceiver RX parameter
            // Config transceiver TX parameter according to twdm_channel_profile[index].default_response_channel;
            // TODO: Config transceiver TX parameter

            g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].pon_id;
            g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile[index].default_response_channel;
        }
    }

    // According to channel_configured.us_pon_id, write Correlation tag and Calibration record status into corresponding CSRs     //prepare for HW PLOAM grant;
    // TODO: write Correlation tag and Calibration record status into corresponding CSRs

    /* Update SW previous state record to O8.1;*/
    g_xgpn_act_state = AAL_XGPN_ACT_STATE_O8_1;
    aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(0, AAL_CLK_SEQ_RESET_TRY_COUNT);
    //aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);
#if 0
    //Vector clean, to trigger ASIC to continue DSYNC checking;
    memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
    xgpn_actp_status.rx_optical_tuning = FALSE;   // 1'b0: ONU resume DS synchronization
    rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
    if (CA_E_OK != rc){
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_set: %d\r\n", rc);
    }
#endif

    return rc;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o81_e_txrxtuningreq_change
**
**  Description:
**    XGPON event TX_RX_TUNING_REQ handler for state change @ O8.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o81_e_txrxtuningreq_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_status_t rc = CA_E_OK;

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET((AAL_XGPN_ACT_STATE_O8_1 == new_state),
        CA_E_CONFLICT);

    rc = aal_xgpn_act_txrxtuningreq_common();
    if (rc != CA_E_OK) {
        __AAL_XGPN_ERROR("%s(): call aal_xgpn_act_txrxtuningreq_common fail.\n", __FUNCTION__);
    }
    XGPN_ACTFSM_LOG_WARN("Channel set para:target_ds_pon_id 0x%x,target_us_pon_id 0x%x\r\n",g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id, g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id);

    ACT_FSM_TRIG_HW_RX_TUNING();

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o81_e_disablesn_change
**
**  Description:
**    XGPON event DISABLE_SN_REQUEST handler for state change @ O8.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o81_e_disablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_1 == pre_state)
        && (AAL_XGPN_ACT_STATE_O7 == new_state),  CA_E_CONFLICT);
    // TODO: discard burst profile?

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o81_e_to4expire_change
**
**  Description:
**    XGPON event TX_RX_TUNING_REQ handler for state change @ O8.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o81_e_to4expire_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    //ca_status_t ret = CA_E_OK;
    //aal_ploam_tuning_cfg_mask_t tuning_cfg_mask;
    aal_ploam_tuning_cfg_t      tuning_cfg;
    aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

/*  NOTE: if 0 following state check @lidong/2017-0519
    Since ASIC has a bug(NOT update prestate to O81, keep O52) for pre-state when rollback, so, remove following limit.
    bug is as following:
    [ 2442.793603] __xgpn_act_intr_proc()::5427:Event: TO4_EXPIRE, pre state: O5_2, new state: O8_1
    [ 2442.802437] aal_xgpn_act_fsm_s_o81_e_to4expire_change(): state check FAIL: (AAL_XGPN_ACT_STATE_O8_1 == pre_state) && (AAL_XGPN_ACT_STATE_O1_1 == new_state || AAL_XGPN_ACT_STATE_O8_1 ==
    new_state)
*/
#if 0
    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_1 == pre_state || AAL_XGPN_ACT_STATE_O5_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O1_1 == new_state || AAL_XGPN_ACT_STATE_O8_1 == new_state),
        CA_E_CONFLICT);
#endif

    memset(&tuning_cfg, 0, sizeof(tuning_cfg));

    (void)aal_ploam_tuning_cfg_get(0, &tuning_cfg);

     XGPN_ACTFSM_LOG_WARN("tuning rollback allow = %d, skip_rx = %d.\r\n",
        tuning_cfg.rollback_allow, tuning_cfg.skip_rx);

    if (AAL_XGPN_ACT_STATE_O8_1 == new_state) {/* new O8.1 */
        if (tuning_cfg.rollback_allow) {
            memset(&tunning_sw_resp, 0, sizeof(tunning_sw_resp));
            tunning_sw_resp_mask.wrd = 0;

            tunning_sw_resp_mask.s.tuning_request        = 1;
            tunning_sw_resp_mask.s.tuning_operation_code = 1;
            tunning_sw_resp_mask.s.seqno                 = 1;
            tunning_sw_resp_mask.s.tuning_response_code  = 1;

            tunning_sw_resp.tuning_request        = 1;
            tunning_sw_resp.tuning_operation_code = __PLOAM_OP_ROLLBACK;
            tunning_sw_resp.seqno                 = g_tun_ctr_seqno;
            tunning_sw_resp.tuning_response_code  = __ROLLBACK_COM_DS;

            aal_ploam_tuning_sw_resp_set(0, tunning_sw_resp_mask, &tunning_sw_resp);
            NGPON2_TUN_DEBUG("send Tuning response seq %u ROLLBACK success!!\r\n", g_tun_ctr_seqno);

        } else {
            ACT_FSM_SET_HW_WLCP_ENA(0);
            NGPON2_TUN_DEBUG("WLCP Disable!!\r\n");
        }

#if 1
    if (aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK) {
        aal_xgpon_dsfp_sync_ctrl_config_msk_t  sync_ctrl_mask;
        aal_xgpon_dsfp_sync_ctrl_config_t      sync_ctrl;

        memset(&sync_ctrl, 0, sizeof(sync_ctrl));
        sync_ctrl_mask.u32 = 0;
        sync_ctrl_mask.s.bit_swap_en = 1;
        sync_ctrl_mask.s.byte_swap_en = 1;
        sync_ctrl.bit_swap_en = 0;
        sync_ctrl.byte_swap_en = 0;
        ca_printf(":%s:%d:: simulation, rollback to original channel %d.\r\n",__FUNCTION__,__LINE__, g_original_ds_chn);
        aal_xgpon_dsfp_sync_ctrl_set(AAL_DEFAUL_DEV_ID,
            sync_ctrl_mask, &sync_ctrl); // LODS
        /*if (new_state == AAL_XGPN_ACT_STATE_O1_1) {
            aal_xgpn_simulation_flag = 0;
        }*/
        if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_CLR_RX_TX_TUNING_FORCE)
            aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE); // force clear tx/rx tuning
        else
            aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE); // ONLY clear rx tuning
        ACT_FSM_SET_STATE(new_state);
        //aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(AAL_DEFAUL_DEV_ID, AAL_CLK_SEQ_RESET_TRY_COUNT);
        return CA_E_OK;
    }

    if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK) {
        if (tuning_cfg.rollback_allow) {
            aal_xgpon_xfp_chanl_set(0, g_original_ds_chn, g_original_us_chn);
            NGPON2_TUN_DEBUG("Rollback to ds_channel %d, us_channel %d!\r\n", g_original_ds_chn, g_original_us_chn);
        }
    } else {
        //aal_xgpon_xfp_chanl_set(0, g_original_ds_chn, g_original_us_chn);
        //NGPON2_TUN_DEBUG("Rollback to ds_channel %d, us_channel %d!\r\n", g_original_ds_chn, g_original_us_chn);
    }
#endif

    } else if (new_state == AAL_XGPN_ACT_STATE_O1_1) { /* new_state O1.1*/

    /* TO4 Expire 2nd */
        ACT_FSM_SET_HW_WLCP_ENA(0);
        NGPON2_TUN_DEBUG("WLCP Disable!!\r\n");
    }

    if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_CLR_RX_TX_TUNING_FORCE)
        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE); // force clear tx/rx tuning
    else
        aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE); // ONLY clear rx tuning


    (void)aal_xgpn_act_toxexpire_common();
#if 0 // move up
    if (aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK) {
        aal_xgpon_dsfp_sync_ctrl_config_msk_t  sync_ctrl_mask;
        aal_xgpon_dsfp_sync_ctrl_config_t      sync_ctrl;

        memset(&sync_ctrl, 0, sizeof(sync_ctrl));
        sync_ctrl_mask.u32 = 0;
        sync_ctrl_mask.s.bit_swap_en = 1;
        sync_ctrl_mask.s.byte_swap_en = 1;
        sync_ctrl.bit_swap_en = 0;
        sync_ctrl.byte_swap_en = 0;
        ca_printf(":%s:%d:: simulation, rollback to original channel %d.\r\n",__FUNCTION__,__LINE__, g_original_ds_chn);
        aal_xgpon_dsfp_sync_ctrl_set(AAL_DEFAUL_DEV_ID,
            sync_ctrl_mask, &sync_ctrl); // LODS
        aal_xgpn_simulation_flag = 0;
        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
        ACT_FSM_SET_STATE(AAL_XGPN_ACT_STATE_O1_1);
        //aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(AAL_DEFAUL_DEV_ID, AAL_CLK_SEQ_RESET_TRY_COUNT);
        return CA_E_OK;
    }

    if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK) {
        if (tuning_cfg.rollback_allow) {
            aal_xgpon_xfp_chanl_set(0, g_original_ds_chn, g_original_us_chn);
            NGPON2_TUN_DEBUG("Rollback to ds_channel %d, us_channel %d!\r\n", g_original_ds_chn, g_original_us_chn);
        }
    } else {
        aal_xgpon_xfp_chanl_set(0, g_original_ds_chn, g_original_us_chn);
        NGPON2_TUN_DEBUG("Rollback to ds_channel %d, us_channel %d!\r\n", g_original_ds_chn, g_original_us_chn);
    }
#endif
    ACT_FSM_SET_STATE(new_state);

    //aal_clear_actp_rx_tx_optical_tuning(TRUE,FALSE);
    aal_clear_actp_rx_tx_optical_tuning(TRUE,TRUE);
    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o81_e_deactiveonuid_change
**
**  Description:
**    XGPON event DEACTIVE_ONUID_REQ handler for state change @ O8.1
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o81_e_deactiveonuid_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_1 == pre_state)
        && (AAL_XGPN_ACT_STATE_O1_1 == new_state),  CA_E_CONFLICT);

    ACT_FSM_SET_STATE(new_state);

    return aal_xgpn_act_deactiveonuid_common();
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o82_e_dwlchoktowork_change
**
**  Description:
**    XGPON event DWLCH_OK_TO_WORK handler for state change @ O8.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o82_e_dwlchoktowork_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_status_t ret = CA_E_OK;
    aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O9 == new_state),  CA_E_CONFLICT);

    aal_puc_enable_voq(0, TRUE); // TODO: enable here or after O9?


    if(aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_TXRX){/*for simulation only*/
        __aal_xgpon_sm_simulation_o9(0);
    }else{/*normal operation */
        //ca_printf("%s:%d!!\r\n",__FUNCTION__,__LINE__);

        memset(&tunning_sw_resp,0,sizeof(tunning_sw_resp));
        tunning_sw_resp_mask.wrd = 0;
        tunning_sw_resp_mask.s.tuning_operation_code = 1;
        tunning_sw_resp_mask.s.tuning_response_code = 1;
        tunning_sw_resp.tuning_operation_code = __PLOAM_OP_COMPLETE;
        tunning_sw_resp.tuning_response_code = 0;
        ret = aal_ploam_tuning_sw_resp_set(0,tunning_sw_resp_mask,&tunning_sw_resp);
        CA_RET_VALUE_CHECK(ret);
        NGPON2_TUN_DEBUG("send COMPLETE_U success!!\r\n");
        aal_clear_actp_rx_tx_optical_tuning(TRUE,TRUE);
    }

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o82_e_profileevaluate_change
**
**  Description:
**    XGPON event PROFILE_EVALUATION handler for state change @ O8.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o82_e_profileevaluate_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    ca_uint32_t actp_sts = 0;
    aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    /*ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O8_2 == new_state),  CA_E_CONFLICT);*/

    // TODO: ldchen
    memset(&tunning_sw_resp, 0, sizeof(tunning_sw_resp));
    tunning_sw_resp_mask.wrd = 0;

    tunning_sw_resp_mask.s.tuning_request        = 1;
    tunning_sw_resp_mask.s.tuning_operation_code = 1;
    tunning_sw_resp_mask.s.seqno                 = 1;
    tunning_sw_resp_mask.s.tuning_response_code  = 1;

    tunning_sw_resp.tuning_request        = 1;
    tunning_sw_resp.tuning_operation_code = __PLOAM_OP_COMPLETE;
    tunning_sw_resp.seqno                 = g_tun_ctr_seqno;
    tunning_sw_resp.tuning_response_code  = 0;

    aal_ploam_tuning_sw_resp_set(0, tunning_sw_resp_mask, &tunning_sw_resp);
    NGPON2_TUN_DEBUG("send Tuning response seq %u complete_u success!!\r\n", g_tun_ctr_seqno);

    if (aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK) {
        ca_printf(":%s:%d::rollback simulate to DWLCH OK.\r\n",__FUNCTION__,__LINE__);
        //ACT_FSM_TRIG_HW_DWLCH_OK();
        aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);
        //ACT_FSM_SET_STATE(new_state);
        //return CA_E_OK;
    }

    if (ACT_FSM_CHK_DWLCH_PROF()) {
        ACT_FSM_TRIG_HW_DWLCH_OK();
        actp_sts = AAL_PON_REG_READ(XGPN_ACTP_STS);
        ca_printf(":%s:%d::actp status is 0x%x!!\r\n",__FUNCTION__,__LINE__,actp_sts);
    } else {
        ACT_FSM_TRIG_HW_DWLCH_FAIL();
        // TODO: rollback
    }

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o82_e_lods_change
**
**  Description:
**    XGPON event LODS handler for state change @ O8.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o82_e_lods_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O8_1 == new_state),  CA_E_CONFLICT);

    ACT_FSM_RESET_BURST_PROF();
    aal_clear_actp_rx_tx_optical_tuning(TRUE, FALSE);
    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o82_e_dwlchnotappropriate_change
**
**  Description:
**    XGPON event DWLCH_NOT_APPROPRIATE handler for state change @ O8.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o82_e_dwlchnotappropriate_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O8_1 == new_state),  CA_E_CONFLICT);

    ACT_FSM_RESET_BURST_PROF();

    ACT_FSM_SET_STATE(new_state);

    return aal_xgpn_act_txrxtuningreq_common();

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o82_e_disablesn_change
**
**  Description:
**    XGPON event DISABLE_SN_REQUEST handler for state change @ O8.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o82_e_disablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O7 == new_state),  CA_E_CONFLICT);

    ACT_FSM_SET_STATE(new_state);

    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o82_e_to4expire_change
**
**  Description:
**    XGPON event TO4_EXPIRE handler for state change @ O8.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o82_e_to4expire_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
#if 0
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O1_1 == new_state),  CA_E_CONFLICT);

    ACT_FSM_SET_STATE(new_state);

    (void)aal_xgpn_act_toxexpire_common();

    return CA_E_OK;
}
#endif
{
    //ca_status_t ret = CA_E_OK;
    //aal_ploam_tuning_cfg_mask_t tuning_cfg_mask;
    aal_ploam_tuning_cfg_t      tuning_cfg;
    aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;

    NGPON2_MODE_CHK_RET(CA_E_ERROR);

#if 0
    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O8_2 == pre_state)
        && (AAL_XGPN_ACT_STATE_O8_2 == new_state
        || AAL_XGPN_ACT_STATE_O1_1 == new_state
        || AAL_XGPN_ACT_STATE_O9 == new_state),
        CA_E_CONFLICT);
#endif

    memset(&tuning_cfg, 0, sizeof(tuning_cfg));

    (void)aal_ploam_tuning_cfg_get(0, &tuning_cfg);
    ca_printf(":%s:%d::tuning rollback allow = %d, skip_rx = %d.\r\n",__FUNCTION__,__LINE__,
        tuning_cfg.rollback_allow, tuning_cfg.skip_rx);

    if (AAL_XGPN_ACT_STATE_O8_1 == new_state) {/* new O8.1 */
        if (tuning_cfg.rollback_allow) {
            memset(&tunning_sw_resp, 0, sizeof(tunning_sw_resp));
            tunning_sw_resp_mask.wrd = 0;

            tunning_sw_resp_mask.s.tuning_request        = 1;
            tunning_sw_resp_mask.s.tuning_operation_code = 1;
            tunning_sw_resp_mask.s.seqno                 = 1;
            tunning_sw_resp_mask.s.tuning_response_code  = 1;

            tunning_sw_resp.tuning_request        = 1;
            tunning_sw_resp.tuning_operation_code = __PLOAM_OP_ROLLBACK;
            tunning_sw_resp.seqno                 = g_tun_ctr_seqno;
            tunning_sw_resp.tuning_response_code  = __ROLLBACK_COM_DS;

            aal_ploam_tuning_sw_resp_set(0, tunning_sw_resp_mask, &tunning_sw_resp);
            NGPON2_TUN_DEBUG("send ROLLBACK success!!\r\n");

        } else  {
            ACT_FSM_SET_HW_WLCP_ENA(0);
            NGPON2_TUN_DEBUG("WLCP Disable!!\r\n");
        }

    } else if (AAL_XGPN_ACT_STATE_O1_1 == new_state){ /* new_state O1.1*/

        ACT_FSM_SET_HW_WLCP_ENA(0);
        NGPON2_TUN_DEBUG("WLCP Disable!!\r\n");
    }


    (void)aal_xgpn_act_toxexpire_common();

    if (aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK) {
        aal_xgpon_dsfp_sync_ctrl_config_msk_t  sync_ctrl_mask;
        aal_xgpon_dsfp_sync_ctrl_config_t      sync_ctrl;

        if (new_state == AAL_XGPN_ACT_STATE_O1_1) {
            memset(&sync_ctrl, 0, sizeof(sync_ctrl));
            sync_ctrl_mask.u32 = 0;
            sync_ctrl_mask.s.bit_swap_en = 1;
            sync_ctrl_mask.s.byte_swap_en = 1;
            sync_ctrl.bit_swap_en = 0;
            sync_ctrl.byte_swap_en = 0;
            ca_printf(":%s:%d::simulation rollback to original channel %d.\r\n",__FUNCTION__,__LINE__, g_original_ds_chn);
            aal_xgpon_dsfp_sync_ctrl_set(AAL_DEFAUL_DEV_ID,
                sync_ctrl_mask, &sync_ctrl); // LODS
            aal_xgpn_simulation_flag = 0;
        }
        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
        ACT_FSM_SET_STATE(new_state);
        return CA_E_OK;
    }

    if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK) {
        if (tuning_cfg.rollback_allow) {
            ca_uint8_t  to_ds_chn = g_original_ds_chn;
            ca_uint8_t  to_us_chn = g_original_us_chn;

            if (g_aal_xgpon_actfsm_debug & AAL_XGPON_TUNING_DEBUG_ROLLBACK_TO_TARGET_CHN) {
                to_ds_chn = __GET_CHN_FROM_PONID(g_ds_pon_id);
                to_us_chn = __GET_CHN_FROM_PONID(g_us_pon_id);;
            }
            aal_xgpon_xfp_chanl_set(AAL_DEFAUL_DEV_ID, to_ds_chn, to_us_chn);
            NGPON2_TUN_DEBUG("Rollback to ds_channel %d, us_channel %d!\r\n", to_ds_chn, to_us_chn);
        }
    } else {
        if (aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK) {
            aal_xgpon_xfp_chanl_set(0, g_original_ds_chn, g_original_us_chn);
            NGPON2_TUN_DEBUG("Simulation rollback to ds_channel %d, us_channel %d!\r\n", g_original_ds_chn, g_original_us_chn);
        }
    }

    ACT_FSM_SET_STATE(new_state);

    aal_clear_actp_rx_tx_optical_tuning(TRUE,TRUE);
    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o81_e_deactiveonuid_change
**
**  Description:
**    XGPON event DEACTIVE_ONUID_REQ handler for state change @ O8.2
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o82_e_deactiveonuid_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    /* The same as TO4 Expire */
    return aal_xgpn_act_fsm_s_o82_e_to4expire_change(pre_state, new_state);
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o9_e_ustuningconfirm_change
**
**  Description:
**    XGPON event US_TUNING_CONFIRM handler for state change @ O9
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o9_e_ustuningconfirm_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O9 == pre_state)
        && (AAL_XGPN_ACT_STATE_O5_1 == new_state),  CA_E_CONFLICT);

    /*change to fragment  mode scheduling before switching*/
    AAL_PON_REG_FIELD_WRITE(PUC_BTCCFG, schmode, 0);

    ACT_FSM_SET_STATE(AAL_XGPN_ACT_STATE_O5_1);

    aal_puc_enable_voq(0, TRUE);

#if defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)
    /*Enable RGMII TX and RX*/
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_TXMAC_CFG, tx_en, 1);
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_RXMAC_CFG, rx_en, 1);
#endif

    /* Update new channle record */
    aal_xgpon_xfp_chanl_get(AAL_DEFAUL_DEV_ID, &g_original_ds_chn, &g_original_us_chn);

    if (AAL_XGPON_ACTFSM_CHN_SCAN_IS_ON()) {
        /* write the channel to SCFG default channel, for next power up*/
        ca_uint8_t  fixed_chn = g_original_ds_chn % __GET_XCVR_CHN_CAPABILITY();

        fixed_chn = __PHYSICAL_CHN_TO_LOGICAL_CHN(fixed_chn);
        scfg_write(0, CFG_ID_PON_NGPON2_DEFAULT_CHAN, sizeof(ca_uint8_t), &fixed_chn);
    }

     /* Update the parameters in tuning_control after tuning */
    __xgpn_act_fsm_tuning_flags_set(AAL_DEFAUL_DEV_ID,
        FALSE, FALSE);

    NGPON2_TUN_DEBUG("tuning to ds_channel %d up_channel %d successfully!!\r\n",
        g_original_ds_chn, g_original_us_chn);

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o9_e_txtuningreq_change
**
**  Description:
**    XGPON event TX_TUNING_REQ handler for state change @ O9
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t __aal_xgpon_sm_simulation_o9(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    //ca_uint32_t sfc0 = 0;
    //ca_uint64_t sfc1 = 0;
    //ca_uint64_t sfc = 0;
    //ca_uint32_t val = 0;
    //aal_ploam_tuning_cfg_mask_t tuning_cfg_mask;
    aal_ploam_tuning_cfg_t      tuning_cfg;

    aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;

    aal_xgpn_plmp_cfg_t plm_cfg;

    XGPN_ACTFSM_LOG_DBG("Start simulate O9...\r\n");

    memset(&tuning_cfg,0,sizeof(tuning_cfg));
    memset(&tunning_sw_resp,0,sizeof(tunning_sw_resp));
    memset(&plm_cfg,0,sizeof(plm_cfg));

    tunning_sw_resp_mask.wrd = 0;
    tunning_sw_resp_mask.s.tuning_operation_code = 1;
    tunning_sw_resp_mask.s.tuning_response_code = 1;
    tunning_sw_resp.tuning_operation_code = 2;
    tunning_sw_resp.tuning_response_code = 0;
    (void)aal_ploam_tuning_sw_resp_set(device_id,tunning_sw_resp_mask,&tunning_sw_resp);

    ACT_FSM_TRIG_HW_TX_TUNING();

#if 0
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en,0x1);
    //plm_cfg.done = 0x1;
    //plm_cfg.msg_type = 0x15;
    //aal_ploam_cfg_set(device_id,&plm_cfg);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_PLOAM_CFG_DATA0, value,0x01000000);/*31:24 bit*/

    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_PLOAM_CFG, msg_type,0x15);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_PLOAM_CFG, done,0x1);

    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en,0x0);
#endif

    #if 0
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, 0x3);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, 0x0);

    AAL_PON_REG_FIELD_WRITE(XGPN_ACTP_STS, tx_optical_tuning, 0x0);

    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en,0x1);

    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_PLOAM_CFG_DATA0, value,0x01000000);/*31:24 bit*/

    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_PLOAM_CFG, msg_type,0x15);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_PLOAM_CFG, done,0x1);

    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en,0x0);

    #endif

    return ret;
}
ca_status_t aal_xgpn_act_fsm_s_o9_e_txtuningreq_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
/*
The ONU starts upstream tuning timer TO5, whose initial value depends on whether the ONU is calibrated for the in-band response, and completes its transmitter tuning.
An ONU that satisfies the minimum calibration accuracy requirement for the desired upstream wavelength channel waits for an in-band directed PLOAM grant. An ONU that does not satisfy the minimum calibration accuracy requirement for the desired upstream wavelength channel waits for an AMCC directed PLOAM grant. Once the ONU receives an applicable PLOAM grant, it transmits an appropriate upstream PLOAM message, contingent upon the specific wavelength management scenario: Tuning_Response(Complete_u) in case of target TWDM channel or preconfigured TWDM channel, Tuning_Response(Rollback) in case of previous working channel after failed tuning. The ONU ignores the values of the DBRu flag and GrantSize field the directed PLOAM grant allocation structure.
If the ONU supports power levelling, it may exercise the ONU-activated power levelling while responding to the directed PLOAM grants with the Tuning_Response PLOAM messages.
The ONU awaits for and acts upon the upstream tuning feedback from the OLT CT, either staying in the Upstream tuning state (O9) while fine-tuning the transmitter, or completing upstream tuning and resuming normal operation in the Operation state (O5), or, if timer TO5 expires, discarding the TC layer configuration and returning to the Initial state (O1).
*/
    ca_status_t rc = CA_E_ERROR;
#if 0
    static ca_uint8_t  cur_prof = 0;
    aal_ploam_tuning_cfg_mask_t tuning_cfg_mask;
    aal_ploam_tuning_cfg_t      tuning_cfg;
#endif

    NGPON2_MODE_CHK_RET(CA_E_ERROR);
    ca_printf("%s::%d::Channel set para:us_pon_id0x%x !!\r\n",__FUNCTION__,__LINE__,  g_ngpon2_channel_mgmt_cb.pre_working_channel.us_pon_id);
    if(aal_pon_mac_mode_get(0) == ONU_PON_MAC_NGPON2)
    {
        rc = aal_xgpon_xfp_chanl_set(0,US_DS_CHANEL,US_DS_CHANEL);
    }
    if (CA_E_OK == rc) {

        // TODO: if all fail, should do rollback
        ca_printf("%s::%d::tx_tuning_request interrupt handle, send out nack!!\r\n",__FUNCTION__,__LINE__);
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_ROLLBACK);
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, __NACK_US_VOID);
    }
    #if 0
    if (CA_E_OK == rc) {
        ca_printf("%s::%d::tx_tuning_request interrupt handle, send out complete_u!!\r\n",__FUNCTION__,__LINE__);
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_COMPLETE);
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, 0);
    }else {
        // TODO: if all fail, should do rollback
        ca_printf("%s::%d::tx_tuning_request interrupt handle, send out nack!!\r\n",__FUNCTION__,__LINE__);
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_NACK);
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, __NACK_US_VOID);
    }

    } else {
            ca_printf("%s::%d::tx_tuning_request interrupt handle,send out rollback!!\r\n",__FUNCTION__,__LINE__);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_ROLLBACK);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, __ROLLBACK_US_TUNR);
    }
    #endif
    /*only for simulation,normal condition should comment*/
    //__aal_xgpon_sm_simulation_o9(0);

    /*only for simulation,normal condition should not be commented*/

    aal_clear_actp_rx_tx_optical_tuning(TRUE,TRUE);

    ACT_FSM_SET_STATE(AAL_XGPN_ACT_STATE_O9);

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o9_e_disablesn_change
**
**  Description:
**    XGPON event DISABLE_SN_REQUEST handler for state change @ O9
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o9_e_disablesn_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O9 == pre_state)
        && (AAL_XGPN_ACT_STATE_O7 == new_state),  CA_E_CONFLICT);

    ACT_FSM_SET_STATE(AAL_XGPN_ACT_STATE_O7);

    return CA_E_OK;
}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o9_e_to5expire_change
**
**  Description:
**    XGPON event TO5_EXPIRE handler for state change @ O9
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o9_e_to5expire_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    aal_ploam_tuning_cfg_t tuning_cfg;
    aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;


    NGPON2_MODE_CHK_RET(CA_E_ERROR);

#if 0 // remove state check, since S/W state may out of sync with HW, so do it force and force sync
    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O9 == pre_state)
        && (AAL_XGPN_ACT_STATE_O8_1 == new_state || AAL_XGPN_ACT_STATE_O1_1 == new_state || AAL_XGPN_ACT_STATE_O9 == new_state),  CA_E_CONFLICT);
#endif

    memset(&tuning_cfg, 0, sizeof(tuning_cfg));

    (void)aal_ploam_tuning_cfg_get(0, &tuning_cfg);

    if (tuning_cfg.rollback_allow) {
        memset(&tunning_sw_resp, 0, sizeof(tunning_sw_resp));
        tunning_sw_resp_mask.wrd = 0;

        tunning_sw_resp_mask.s.tuning_request        = 1;
        tunning_sw_resp_mask.s.tuning_operation_code = 1;
        tunning_sw_resp_mask.s.seqno                 = 1;
        tunning_sw_resp_mask.s.tuning_response_code  = 1;

        tunning_sw_resp.tuning_request        = 1;
        tunning_sw_resp.tuning_operation_code = __PLOAM_OP_ROLLBACK;
        tunning_sw_resp.seqno                 = g_tun_ctr_seqno;
        tunning_sw_resp.tuning_response_code  = __ROLLBACK_US_VOID;

        aal_ploam_tuning_sw_resp_set(0, tunning_sw_resp_mask, &tunning_sw_resp);
        NGPON2_TUN_DEBUG("send Tuning Response ROLLBACK success!!\r\n");

    } else {
        ACT_FSM_SET_HW_WLCP_ENA(0);
        NGPON2_TUN_DEBUG("WLCP Disable!!\r\n");
    }

    ACT_FSM_SET_STATE(new_state);

    (void)aal_xgpn_act_toxexpire_common();

    if (aal_xgpn_simulation_flag & AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK) {
        aal_xgpon_dsfp_sync_ctrl_config_msk_t  sync_ctrl_mask;
        aal_xgpon_dsfp_sync_ctrl_config_t      sync_ctrl;

        ca_printf("%s::%d::simulation, rollback to original channels!!\r\n",__FUNCTION__,__LINE__);

        memset(&sync_ctrl, 0, sizeof(sync_ctrl));
        sync_ctrl_mask.u32 = 0;
        sync_ctrl_mask.s.bit_swap_en = 1;
        sync_ctrl_mask.s.byte_swap_en = 1;
        sync_ctrl.bit_swap_en = 0;
        sync_ctrl.byte_swap_en = 0;
        aal_xgpon_dsfp_sync_ctrl_set(AAL_DEFAUL_DEV_ID,
            sync_ctrl_mask, &sync_ctrl); // recover
        aal_xgpn_simulation_flag = 0;

        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
        return CA_E_OK;

    }

    if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK) {
        if (tuning_cfg.rollback_allow) {
            ca_uint32_t  cur_ds_chn = 0, cur_up_chn = 0;

            aal_xgpon_xfp_chanl_get(0, &cur_ds_chn, &cur_up_chn);
            if (tuning_cfg.skip_rx){
                aal_xgpon_xfp_chanl_set(0, g_original_ds_chn, g_original_us_chn);
                NGPON2_TUN_DEBUG("Rollback(Rx/Tx) to ds_channel %d, us_channel %d!\r\n", g_original_ds_chn, g_original_us_chn);
             } else {
                aal_xgpon_xfp_chanl_set(0, cur_ds_chn, g_original_us_chn);
                NGPON2_TUN_DEBUG("Rollback(Tx ONLY) to ds_channel %d, us_channel %d!\r\n", cur_ds_chn, g_original_us_chn);
             }
        }
    } else {
        //aal_xgpon_xfp_chanl_set(0, g_original_ds_chn, g_original_us_chn);
        //NGPON2_TUN_DEBUG("Rollback to ds_channel %d, us_channel %d!\r\n", g_original_ds_chn, g_original_us_chn);
    }

    /* Update the parameters in tuning_control after tuning */
    __xgpn_act_fsm_tuning_flags_set(AAL_DEFAUL_DEV_ID,
        FALSE, FALSE);

    /* Set Eqd to hardware */
    __xgpn_act_fsm_load_chn_eqd(AAL_DEFAUL_DEV_ID,
    __xgpn_act_fsm_get_current_us_chn(AAL_DEFAUL_DEV_ID));

    aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);

    return CA_E_OK;

}

/****************************************************************************
**
**  Function Name: aal_xgpn_act_fsm_s_o9_e_deactiveonuid_change
**
**  Description:
**    XGPON event DEACTIVE_ONUID_REQ handler for state change @ O9
**
**  Argument        Type                          IO        Description
**  --------------- ----------------------------- -- ------------------------
**  pre_state       aal_xgpn_activation_state_t   I  Previous state
**  new_state       aal_xgpn_activation_state_t   I  New state
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        08/02/16   Initial Writing
**
****************************************************************************/

ca_status_t aal_xgpn_act_fsm_s_o9_e_deactiveonuid_change
(
    CA_IN aal_xgpn_activation_state_t   pre_state,   /* pre state */
    CA_IN aal_xgpn_activation_state_t   new_state    /* new state */
)
{
    NGPON2_MODE_CHK_RET(CA_E_ERROR);

    ACT_FSM_HW_STATE_CHK_RET( (AAL_XGPN_ACT_STATE_O9 == pre_state)
        && (AAL_XGPN_ACT_STATE_O1_1 == new_state),  CA_E_CONFLICT);

    ACT_FSM_SET_STATE(new_state);

    (void)aal_xgpn_act_deactiveonuid_common();

    return CA_E_OK;
}

#define __ACT_FSM_DBG_TUNNING_SHOW     0
#define __ACT_FSM_DBG_TUNNING_TRIGGER  1
#define __ACT_FSM_DBG_TUNNING_RESET    2

ca_status_t aal_xgpon_act_fsm_dbg_tunning_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  option,
    CA_IN ca_uint32_t value1,
    CA_IN ca_uint32_t value2)
{
    ca_status_t rc = CA_E_OK;
    aal_xgpn_plmp_cfg_t plmp_cfg;
    ploam_ngpon2_tuning_ctrl_t ploam_tuning_ctrl;

    ca_printf("%s(): option %#x value1%#x value2%#x\n", __FUNCTION__,
        option, value1, value2);

    switch (option) {
    case __ACT_FSM_DBG_TUNNING_SHOW:
        ca_printf("Tunning Control Info:\r\n");
        ca_printf("operation_code   = %u\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.operation_code   );
        ca_printf("scheduled_sfc    = %u\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.scheduled_sfc    );
        ca_printf("rollback_flag    = %u\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag    );
        ca_printf("target_ds_pon_id = %u\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id );
        ca_printf("target_us_pon_id = %u\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id );
        ca_printf("calibration_flag = %u\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.calibration_flag );
        ca_printf("tuning_ctrl_flag = %u\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag );
        break;

    case __ACT_FSM_DBG_TUNNING_TRIGGER:
        memset(&plmp_cfg, 0, sizeof(aal_xgpn_plmp_cfg_t));
        memset(&ploam_tuning_ctrl, 0, sizeof(ploam_ngpon2_tuning_ctrl_t));

        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 1);
        plmp_cfg.done = 0;
        plmp_cfg.msg_type = 0x15; /* 0x15, "Tuning_Control" */
        ploam_tuning_ctrl.op_code = (value1 >> 24) & 0xFF; /* 0: Request */
        ploam_tuning_ctrl.scheduled_sfc = (value1 >> 16) & 0xFF;
        ploam_tuning_ctrl.rollback_flag = (value1 >> 8) & 0xFF;
/*
R - Calibration flag
R = 0: if ONU has no calibration information for target wavelength channel, the ONU responds with NACK by lack of calibration and ignores tuning request;
R = 1: if ONU has no calibration information for target wavelength channel, the ONU should execute tuning request with necessary calibration.

*/
        ploam_tuning_ctrl.calibration_flag = (value1 >> 0) & 0xFF;

/* PON-ID:
Operator(3 bits)
Central office(3 bits)
OLT chassis(6 bits)
OLT card(4 bits)
Wavlength port(5 bits)
DS wavelength (4 bits)
-    Number of independent operators: 4..8 (i.e., ~23)
-    Number of adjacent central offices: 4..7 (i.e., ~23)
-    Number of OLT chassis in a central office: 64 (i.e., 26)
-    Number of cards in an OLT chassis: up to 16 (i.e., 24)
-    Number of wavelength-ports on a line card: suggest supporting up to 32 (i.e., 25)
-    Downstream wavelength channel identity (i.e., 24))
*/
        ploam_tuning_ctrl.target_ds_pon_id = value2;
        ploam_tuning_ctrl.target_us_pon_id = value2;

        ca_printf("op_code %#x scheduled_sfc %#x rollback_flag %#x target_ds_pon_id %#x target_us_pon_id %#x \r\n",
            ploam_tuning_ctrl.op_code,
            ploam_tuning_ctrl.scheduled_sfc,
            ploam_tuning_ctrl.rollback_flag,
            ploam_tuning_ctrl.target_ds_pon_id,
            ploam_tuning_ctrl.target_us_pon_id
            );

        memcpy(&plmp_cfg.data[0], &ploam_tuning_ctrl.op_code, XGPN_PLMP_MSG_BODY_SIZE);
        rc = aal_ploam_cfg_set(device_id, &plmp_cfg);
        if (CA_E_OK != rc) {
            ca_printf("call aal_ploam_cfg_set fail, return %d\n", rc);
            return rc;
        }

        plmp_cfg.done = 1;
        rc = aal_ploam_cfg_set(device_id, &plmp_cfg);
        if (CA_E_OK != rc) {
            ca_printf("call aal_ploam_cfg_set fail, return %d\n", rc);
            return rc;
        }
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 0);
        break;

    case __ACT_FSM_DBG_TUNNING_RESET:
        AAL_PON_REG_WRITE(0, XGPN_PLMP_SW_RESPONSE);
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 0);
        break;

    default:
        ca_printf("Usage: 0: show; 1: trigger tunning; 2: reset tunning\r\n");
        break;
    }

    return CA_E_OK;
/*
State O1.1:
1. SW needs to do RX/TX channel tuning before PON MAC enable in NG-PON2 mode after boot. Not needed for XG-PON1 and XGS PON mode.
2. No HW INTERRUPT_rx_tuning_O1p1 after power up
3. HW to remove redundant interrupt bits XGPN_ACTP_ST_CHG_INT (bits 0-5)
4. SW will maintain SM state based on HW state-change interrupt and previous state

To test tuning control, SW trigger tuning_request_rx from HW
1. SW fill data with tuning request PLOAM in XGPN_PLMP_PLOAM_CFG_DATA0-8,
2. SW fill msg type in XGPN_PLMP_PLOAM_CFG.msg_type
3. SW set XGPN_PLMP_CTRL.sw_mode_en = 1
4. SW set XGPN_PLMP_PLOAM_CFG.done = 1
5. SW poll till XGPN_PLMP_PLOAM_CFG.done = 0
6. SW set XGPN_PLMP_CTRL.sw_mode_en = 0
7. HW supposed to generate tuning_request_rx interrupt at this point
8. SW do channel tuning
9. ONU should go through O8.1=>O8.2=>O9=>O5.1

NOTE:
1. SW fill XGPN_PLMP_TUNING_CONFIG.request_lead_sfc = 5 during system init.(specify minimum lead time 0-7)
2. HW will send insufficient SFC NACK, other NACKs are sent by SW, HW may add insufficient SFC bit.
3. SW to set XGPN_ACTP_CTRL.wlcp_enable after receiving protection switch PLOAM.

HW change:
1. XGPN_PLMP_TUNING_CONFIG.response_code should be bitmap.
2. HW to clear PLMP_SW_RESPONSE_tuning_request_set when tuning control is aborted (by de-activate ONU or de-activate SN)

*/
}

extern void xgpon_key_exchange_fsm_reset(void);

static inline int __xgpn_state_to_fsm_state(ca_uint32_t top_state, ca_uint32_t sub_state)
{
    /*
        0 : Initial state(O1)
        1 : Serail number state(O23)
        2 : Ranging state(O4)
        3 : operation state(O5)
        4 : Intermittent LODS state(O6)
        5 : Emergency stop state(O7)
        6 : DS Tuning state(O8)
        7 : US Tuning state(O9)
    SUB-STATE: 0: O1.1/O5.1/O8.1; 1:O1.2/O5.2/O8.2
    */
    ca_uint32_t mode = aal_pon_mac_mode_get(0);

    switch (top_state) {
        case 0:
            if ((ONU_PON_MAC_XGSPON == mode) || (ONU_PON_MAC_NGPON2 == mode))
                return ((0 == sub_state) ? AAL_XGPN_ACT_STATE_O1_1 : AAL_XGPN_ACT_STATE_O1_2 );
            else
                return AAL_XGPN_ACT_STATE_O1;
        case 1:
            return (AAL_XGPN_ACT_STATE_O2_3);
        case 2:
            return (AAL_XGPN_ACT_STATE_O4);
        case 3:
            if ((ONU_PON_MAC_XGSPON == mode) || (ONU_PON_MAC_NGPON2 == mode))
                return ((0 == sub_state) ? AAL_XGPN_ACT_STATE_O5_1 : AAL_XGPN_ACT_STATE_O5_2 );
            else
                return AAL_XGPN_ACT_STATE_O5;
        case 4:
            return (AAL_XGPN_ACT_STATE_O6);
        case 5:
            return (AAL_XGPN_ACT_STATE_O7);
        case 6:
            return ((0 == sub_state) ? AAL_XGPN_ACT_STATE_O8_1 : AAL_XGPN_ACT_STATE_O8_2);
        case 7:
            return (AAL_XGPN_ACT_STATE_O9);

        default:
            return AAL_XGPN_ACT_STATE_MAX;

    }

    return AAL_XGPN_ACT_STATE_MAX;
}

/****************************************************************************
**
**  Function Name: __xgpn_act_intr_proc
**
**  Description:
**    XGPON activation message process, handling state change/activations for event/interrupt
**
**  Argument        Type     IO           Description
**  --------------- -------- -- ---------------------------------
**
**  Return Value: void
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        07/22/16   Initial Writing
**
****************************************************************************/
static void __xgpn_act_fsm(aal_xgpn_activation_event_t event, aal_xgpn_activation_state_t pre_state, aal_xgpn_activation_state_t new_state)
{
    //__AAL_XGPN_ACT_LOCK(0);
    g_xgpn_act_fsm_pre_state = pre_state;
    g_xgpn_act_fsm_cur_state = new_state;
    g_xgpn_act_fsm_cur_evt   = event;

    g_xgpn_act_fsm_run_cnt++;

    if (g_xgpn_act_state != pre_state && event != AAL_XGPN_ACT_EVENT_PROFILE_LEARNING && g_xgpn_act_state != new_state) {
        g_xgpn_act_fsm_outsync_cnt++;
        ca_printf("!warning: ACTFSM S/W prev state is outof sync with H/W, S/W: %d but H/W: %d\n", g_xgpn_act_state, pre_state);
        g_xgpn_act_state = pre_state;
    } else if (g_xgpn_act_state == new_state) {
        AAL_XGPON_DBG_ACTFSM("Duplicated state change: pre_state %d, new_state %d, event %d.\r\n", pre_state, new_state, event);
        g_xgpn_act_state = pre_state;
    }

    //__AAL_XGPN_ACT_UNLOCK(0);

    if (CA_E_OK != aal_xgpn_act_fsm_handlers[g_xgpn_act_state][event](pre_state, new_state)) {
        g_xgpn_act_fsm_handle_fail_cnt++;
        AAL_XGPON_DBG_ACTFSM("Handle event %s fail when state %s => %s fail.\r\n",
            xgpn_act_event[event], xgpn_act_state[pre_state], xgpn_act_state[new_state]);
    }

}

static void __xgpn_act_intr_proc(
    ca_uint8_t previous_onu_state,
    ca_uint8_t previous_sub_state,
    ca_uint8_t onu_state,
    ca_uint8_t onu_sub_state,
    aal_xgpn_activation_event_t event)
{
    aal_xgpn_activation_state_t  pre_state = __xgpn_state_to_fsm_state(previous_onu_state, previous_sub_state);
    aal_xgpn_activation_state_t  new_state = __xgpn_state_to_fsm_state(onu_state, onu_sub_state);


    if (pre_state >= AAL_XGPN_ACT_STATE_MAX ||
        new_state >= AAL_XGPN_ACT_STATE_MAX ||
        event >= AAL_XGPN_ACT_EVENT_MAX) { /* unknown interrupts */
        g_xgpn_act_fsm_handle_fail_cnt++;
        AAL_XGPON_DBG_ACTFSM("Wrong pre_state %d, new_state %d, event %d.\r\n", pre_state, new_state, event);
        return;
    }

    AAL_XGPON_DBG_ACTFSM("Event: %s, pre state: %s, new state: %s\r\n",
        xgpn_act_event[event], xgpn_act_state[pre_state], xgpn_act_state[new_state]);

    /* workaround for ASIC TOZ~TO5, it can work only when < sync_hunt */
    if (g_xgpon_work_around && (ONU_PON_MAC_NGPON2 == aal_pon_mac_mode_get(0))) {
        if (new_state == AAL_XGPN_ACT_STATE_O1_1 || new_state == AAL_XGPN_ACT_STATE_O8_1) {
            (void)aal_xgpon_actp_sync_hunt_set(AAL_DEFAUL_DEV_ID, AAL_XGPON_ACTP_SYNC_HUNT_THRESHOLD / 10);
        } else {/* enlarge sync-hunt when leave O51, O81*/
            (void)aal_xgpon_actp_sync_hunt_set(AAL_DEFAUL_DEV_ID, AAL_XGPON_ACTP_SYNC_HUNT_THRESHOLD * 10);
        }
    }

    __xgpn_act_fsm(event, pre_state, new_state);
    
#if 0
    //__AAL_XGPN_ACT_LOCK(0);

    g_xgpn_act_fsm_pre_state = pre_state;
    g_xgpn_act_fsm_cur_state = new_state;
    g_xgpn_act_fsm_cur_evt   = event;

    g_xgpn_act_fsm_run_cnt++;

    if (g_xgpn_act_state != pre_state && event != AAL_XGPN_ACT_EVENT_PROFILE_LEARNING && g_xgpn_act_state != new_state) {
        g_xgpn_act_fsm_outsync_cnt++;
        ca_printf("!warning: ACTFSM S/W prev state is outof sync with H/W, S/W: %d but H/W: %d\n", g_xgpn_act_state, pre_state);
        g_xgpn_act_state = pre_state;
    } else if (g_xgpn_act_state == new_state) {
        AAL_XGPON_DBG_ACTFSM("Duplicated state change: pre_state %d, new_state %d, event %d.\r\n", pre_state, new_state, event);
        g_xgpn_act_state = pre_state;
    }

    //__AAL_XGPN_ACT_UNLOCK(0);

    if (CA_E_OK != aal_xgpn_act_fsm_handlers[g_xgpn_act_state][event](pre_state, new_state)) {
        g_xgpn_act_fsm_handle_fail_cnt++;
        AAL_XGPON_DBG_ACTFSM("Handle event %s fail when state %s => %s fail.\r\n",
            xgpn_act_event[event], xgpn_act_state[pre_state], xgpn_act_state[new_state]);
    }
#endif
}

static aal_xgpn_activation_event_t  __intr_to_evt(ca_uint32_t st_chg_int)
{
    XGPN_ACTP_ST_CHG_INT_t   actp_st_chg_int;

    actp_st_chg_int.wrd = st_chg_int;

    if (actp_st_chg_int.bf.deactivate_OID){
        return  AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ;
    }

    if (actp_st_chg_int.bf.ds_synced){
        return AAL_XGPN_ACT_EVENT_DSYNC;
    }

    if (actp_st_chg_int.bf.DWLCH_appropriate){
        return AAL_XGPN_ACT_EVENT_DWLCH_OK_TO_WORK;
    }

    if (actp_st_chg_int.bf.DWLCH_inappropriate){
        return AAL_XGPN_ACT_EVENT_DWLCH_NOT_APPROPRIATE;
    }

    if (actp_st_chg_int.bf.assign_OID){
        return  AAL_XGPN_ACT_EVENT_ONU_ID_ASSIGNMENT;
    }

    if (actp_st_chg_int.bf.calibr_req_mismatch){
        return  AAL_XGPN_ACT_EVENT_CALIBRATION_REQUEST;
    }

    if (actp_st_chg_int.bf.TOZ_expire){
        return AAL_XGPN_ACT_EVENT_TOZ_EXPIRE;
    }

    if (actp_st_chg_int.bf.TO1_expire){
        return AAL_XGPN_ACT_EVENT_TO1_EXPIRE;
    }

    if (actp_st_chg_int.bf.assign_EqD){
        return AAL_XGPN_ACT_EVENT_EQD_ASSIGNMENT;
    }

    if (actp_st_chg_int.bf.tuning_request_rx){
        return AAL_XGPN_ACT_EVENT_SFC_MATCH_FOR_RXTX_TUNING;
    }

    if (actp_st_chg_int.bf.tuning_request_tx){
        return AAL_XGPN_ACT_EVENT_SFC_MATCH_FOR_TX_TUNING;
    }

    if (actp_st_chg_int.bf.TO2_expire){
        return AAL_XGPN_ACT_EVENT_TO2_EXPIRE;
    }

    if (actp_st_chg_int.bf.TO3_expire){
        return AAL_XGPN_ACT_EVENT_TO3_EXPIRE;
    }

    if (actp_st_chg_int.bf.TO4_expire){
        return  AAL_XGPN_ACT_EVENT_TO4_EXPIRE;
    }

    if (actp_st_chg_int.bf.TO5_expire){
        return AAL_XGPN_ACT_EVENT_TO5_EXPIRE;
    }

    if (actp_st_chg_int.bf.assign_OID_inconsist){
        return AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST;
    }

    if (actp_st_chg_int.bf.loss_sync){
        if (AAL_PON_REG_READ(XGPN_DSFP_ds_sync)) /* LOS recover, dont retport event */
            return 0;
        else
            return AAL_XGPN_ACT_EVENT_LODS;
    }

    if (actp_st_chg_int.bf.disable_SN){
        return AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST;
    }
    if (actp_st_chg_int.bf.enable_SN){
        return AAL_XGPN_ACT_EVENT_ENABLE_SN_REQUEST;
    }

    if (actp_st_chg_int.bf.tuning_request_ack){
        return AAL_XGPN_ACT_EVENT_TUNING_REQ_ACK;
    }

    if (actp_st_chg_int.bf.us_tuning_confirm){
        return AAL_XGPN_ACT_EVENT_US_TUNING_CONFIRM;
    }

    return 0;
}


static void __xgpn_active_deactive_proc(ca_uint32_t active_chg_intr)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_usbg_gem_config_msk_t usbg_xgem_cfg_msk;
    aal_xgpon_usbg_gem_config_t usbg_xgem_cfg;
    aal_xgpon_dsfp_xgem_cfg_msk_t dsfp_xgem_cfg_msk;
    aal_xgpon_dsfp_xgem_cfg_t dsfp_xgem_cfg;
    int i = 0;
    
    aal_xgpon_bwmp_alloc_id_cfg_t   def_allocid; /* default Alloc-ID, which is equal to ONU-ID */
    ca_event_omcc_link_t            omcc_evt;
    ca_event_gpon_port_link_t       link_evt;
    XGPN_ACTP_ST_CHG_INT_t          actp_st_chg_int;

    actp_st_chg_int.wrd = active_chg_intr;

    link_evt.status  = actp_st_chg_int.bf.deregst ? 0 : 1;

    if (link_evt.status) { /* Active: Online */
        (void)aal_xgpon_bwmp_tcont_get(AAL_DEFAUL_DEV_ID, g_original_ds_chn,
            AAL_XGPON_OMCC_TCONT_IDX, &def_allocid);
        g_xgpn_basic_info.onu_id = def_allocid.alloc_id;

        usbg_xgem_cfg_msk.u32 = ~0U;
        memset(&usbg_xgem_cfg, 0, sizeof(aal_xgpon_usbg_gem_config_t));
        usbg_xgem_cfg.aes_mode = 0;
        usbg_xgem_cfg.mc_flag = 0;
        usbg_xgem_cfg.port_en = 1;
        usbg_xgem_cfg.xgem_id = def_allocid.alloc_id;       
        
        for(i = 0 ;i< queue_num_per_tcont; i++){
            ret = aal_xgpon_usbg_xgem_cfg_set(0, i, usbg_xgem_cfg_msk, &usbg_xgem_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
        
        dsfp_xgem_cfg_msk.u32 = 0;    
        memset(&dsfp_xgem_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_cfg_t));
        dsfp_xgem_cfg_msk.s.gem_id = 1 ;
        dsfp_xgem_cfg_msk.s.gem_vld = 1 ;
        dsfp_xgem_cfg.gem_id =  def_allocid.alloc_id;       
        dsfp_xgem_cfg.gem_vld = 1;
        
        for(i = 0 ;i< queue_num_per_tcont; i++){
            ret = aal_xgpon_dsfp_xgem_set(0, i, dsfp_xgem_cfg_msk, &dsfp_xgem_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
        
        if (0 == g_xgpn_act_time_tick) {
            g_xgpn_act_count++;
        }
        g_xgpn_act_time_tick = ca_current_time();
    } else { /* Deactive: Offline */
        g_xgpn_act_time_tick = 0;
        g_xgpn_basic_info.onu_id = AAL_XGPON_INVALID_ONU_ID;
    }

    /* Update local informations: DYING GASP PDU, ONU ID */
    if (link_evt.status) {
        (void)aal_gpon_dying_gasp_set(AAL_DEFAUL_DEV_ID, 0, aal_gpon_dg_pdu, CA_GPON_DYING_GASP_PDU_LEN);
    }

    link_evt.port_id = g_xgpn_basic_info.onu_id;
    link_evt.reason  =  __intr_to_evt(actp_st_chg_int.wrd);

    /* report PON port link status change event */
    (void)ca_event_send(AAL_DEFAUL_DEV_ID, CA_EVENT_GPON_PORT_LINK, &link_evt, sizeof(link_evt));

    omcc_evt.port_id = CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7);;
    omcc_evt.status  = link_evt.status;

    if (omcc_evt.status && (!def_allocid.omci_en || def_allocid.alloc_id == AAL_XGPON_ALLOC_ID_INVALID)) {
        ca_printf("!error: invalid OMCC info get: omci_en = %d, alloc_id = %d\r\n", def_allocid.omci_en, def_allocid.alloc_id);
    } else {
        /* report OMCC channel link change event */
        (void)ca_event_send(AAL_DEFAUL_DEV_ID, CA_EVENT_OMCC_LINK, &omcc_evt, sizeof(omcc_evt));
    }

}


ca_status_t aal_xgpn_act_intr_handler(
    ca_uint32_t xgpn_actp_int,
    ca_uint32_t xgpn_actp_st_chg_int,
    ca_uint32_t xgpn_actp_sts,
    void *cookie)
{
    XGPN_ACTP_INTERRUPT_t actp_int;
    XGPN_ACTP_ST_CHG_INT_t actp_st_chg_int;
    XGPN_ACTP_STS_t actp_status;
#ifdef CONFIG_LUNA_G3_SERIES
    ca_event_gpon_stat_t gpon_evt;
#endif

    g_xgpn_act_fsm_intr_cnt ++;

    actp_int.wrd        = xgpn_actp_int;
    actp_st_chg_int.wrd = xgpn_actp_st_chg_int;
    actp_status.wrd     = xgpn_actp_sts;

    AAL_XGPON_DBG_ACTFSM("XGPN_ACTP_INTERRUPT = 0x%x, ACTP_ST_CHG_INT = 0x%x, ACTP_STS = 0x%x, act_fsm_intr_cnt = %u.\n",
        xgpn_actp_int, xgpn_actp_st_chg_int, xgpn_actp_sts, g_xgpn_act_fsm_intr_cnt);

    if (actp_int.bf.onu_state_change)
    {
        if (actp_st_chg_int.bf.deactivate_OID)
        {
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ);
        }
        if (actp_st_chg_int.bf.ds_synced)
        {
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_DSYNC);
        }
        if (actp_st_chg_int.bf.assign_OID)
        {   /* Update PLOAM MIC for ACK when assigned ONU ID */
            xgpon_aes_ploam_ack_mic_update(AAL_DEFAUL_DEV_ID);
            
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_ONU_ID_ASSIGNMENT);
        }
        if (actp_st_chg_int.bf.TO1_expire)
        {
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_TO1_EXPIRE);
        }
        if (actp_st_chg_int.bf.TO2_expire)
        {
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_TO2_EXPIRE);
        }
        if (actp_st_chg_int.bf.assign_EqD)
        {
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_EQD_ASSIGNMENT);
        }
        if (actp_st_chg_int.bf.loss_sync)
        {
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_LODS);
        }
        if (actp_st_chg_int.bf.disable_SN)
        {
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST);
        }

        if (aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID) == ONU_PON_MAC_NGPON2) { /* NGPON2 mode only interrupts */
            if (actp_st_chg_int.bf.calibr_req_mismatch)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_CALIBRATION_REQUEST);
            }

            if (actp_st_chg_int.bf.DWLCH_appropriate)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_DWLCH_OK_TO_WORK);
            }
            if (actp_st_chg_int.bf.DWLCH_inappropriate)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_DWLCH_NOT_APPROPRIATE);
            }

            if (actp_st_chg_int.bf.tuning_request_rx)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_SFC_MATCH_FOR_RXTX_TUNING);
            }
            if (actp_st_chg_int.bf.tuning_request_tx)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_SFC_MATCH_FOR_TX_TUNING);
            }
            
            if (actp_st_chg_int.bf.TOZ_expire)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_TOZ_EXPIRE);
            }

            if (actp_st_chg_int.bf.TO3_expire)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_TO3_EXPIRE);
            }
            if (actp_st_chg_int.bf.TO4_expire)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_TO4_EXPIRE);
            }
            if (actp_st_chg_int.bf.TO5_expire)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_TO5_EXPIRE);
            }
            if (actp_st_chg_int.bf.assign_OID_inconsist)    //989.3 table 12-4, same as DEACTIVE_ONUID
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST);
            }
            if (actp_st_chg_int.bf.tuning_request_ack)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_TUNING_REQ_ACK);
            }
            if (actp_st_chg_int.bf.us_tuning_confirm)
            {
                (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                    actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_US_TUNING_CONFIRM);
            }
        }

        /* report Activation envets */
        if (actp_st_chg_int.bf.deregst || actp_st_chg_int.bf.assign_EqD) {
            __xgpn_active_deactive_proc(actp_st_chg_int.wrd);
        }

#ifdef CONFIG_LUNA_G3_SERIES
        gpon_evt.port_id = CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7);
        gpon_evt.status  =  actp_status.bf.onu_state;
	
        (void)ca_event_send(AAL_DEFAUL_DEV_ID, CA_EVENT_GPON_REG_STAT_CHNG, &gpon_evt, sizeof(gpon_evt));

        if (actp_status.bf.onu_state == 0)
            memcpy(g_pon_tag, g_pon_tag_tmp, PON_TAG_SIZE); //force to regenerate Keys on next activation process.
		
#endif
    }

    if (aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID) == ONU_PON_MAC_NGPON2) { /* NGPON2 mode only interrupts */

        // No state change handler
        if (actp_int.bf.hunt_sync_expire) {  // hunt_sync_expire, need RX turning or calibration
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_HUNT_SYNC_EXPIRE);
        }

        if (actp_int.bf.profile_learning) { // profile_learning
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_PROFILE_LEARNING);
        }

        if (actp_int.bf.tx_response_cross) { // tx_response_cross, need TX turning or calibration
            (void)__xgpn_act_intr_proc(actp_status.bf.previous_onu_state,  actp_status.bf.previous_sub_state,
                actp_status.bf.onu_state, actp_status.bf.onu_sub_state, AAL_XGPN_ACT_EVENT_TX_RESPONSE_CROSS);
        }
    }

    return CA_E_OK;
}

static ca_boolean_t __xgpn_chn_cpi_chk(
    ca_device_id_t device_id,
    ca_uint8_t channel,
    ca_uint8_t cpi)
{
    ca_uint8_t  i   = 0;
    ca_uint8_t  chn = 0;
    ca_xgpn_twdm_channel_profile_t *p_chn_prof = NULL;

    for (i = 0; i < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT; ++i) {
        p_chn_prof = XGPN_CHN_PROF_PTR(device_id, i);

        if (p_chn_prof->learnt_flag) {
            continue;
        }

        chn = __GET_CHN_FROM_PONID(p_chn_prof->pon_id);

        /* check whether channel partition is matched with channel profile */
        if (chn == channel && p_chn_prof->channel_partition == cpi) {
            return TRUE;
        }

    }

    return FALSE;
}
static ca_status_t __xgpn_chn_init(
    ca_device_id_t device_id,
    ca_uint32_t pon_mac_mode)
{
    ca_uint8_t  def_channel = 0;
    ca_status_t ret         = CA_E_OK;

    if (pon_mac_mode != ONU_PON_MAC_NGPON2) {
        return CA_E_OK;
    }

    if (!AAL_XGPON_ACTFSM_CHN_SCAN_IS_ON()) {
        return CA_E_OK;
    }

    def_channel = AAL_PON_SCFG_GET_DEFAULT_CHN(device_id);

    if (!def_channel) { /* channel scan */
        __NGPON2_CHN_SCAN_ENA(TRUE, 0); /* Enable channel scan from channel 0 */
        ret = aal_xgpon_xfp_chanl_set(AAL_DEFAUL_DEV_ID, 0, 0);

    } else { /* fixed channel */
        ret = aal_xgpon_xfp_chanl_set(AAL_DEFAUL_DEV_ID,
                __LOGICAL_CHN_TO_PHYSICAL_CHN(def_channel),
                __LOGICAL_CHN_TO_PHYSICAL_CHN(def_channel));
    }

    if (CA_E_OK == ret) {
        aal_xgpon_xfp_chanl_get(0, &g_original_ds_chn, &g_original_us_chn);
    }

    return ret;

}

static inline void __xgpn_discard_burst_prof(ca_device_id_t  device_id)
{
    ca_uint32_t  i = 0;
    aal_xgpon_usbg_burst_prof_cfg_msk_t  prof_mask;
    aal_xgpon_usbg_burst_prof_cfg_t      prof_cfg;

    prof_mask.u32 = 0;
    prof_mask.s.vld = 1;
    prof_cfg.vld = 0;
    for (i = 0; i < AAL_XGPON_BURST_PROF_NUM; i++) {
        (void)aal_xgpon_usbg_burst_prof_cfg_set(device_id, i, prof_mask, &prof_cfg);
    }
}

static inline void __xgpn_discard_onuid(ca_device_id_t  device_id)
{
    g_xgpn_basic_info.onu_id = AAL_XGPON_INVALID_ONU_ID;
}

static inline void __xgpn_discard_alloc_id(ca_device_id_t  device_id)
{
    ca_status_t ret   = CA_E_OK;
    ca_uint32_t chn   = g_original_us_chn;
    ca_uint32_t tcont = 0;
    aal_xgpon_bwmp_alloc_id_cfg_msk_t alloc_id_mask;
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;

    alloc_id_mask.u32        = 0;
    alloc_id_mask.s.alloc_id = 1;
    alloc_id_mask.s.omci_en  = 1;
    alloc_id_mask.s.ploam_en = 1;

    alloc_id_cfg.alloc_id = AAL_XGPON_ALLOC_ID_INVALID;
    alloc_id_cfg.omci_en  = 0;
    alloc_id_cfg.ploam_en = 0;

    /* tcont_num_per_chip */
    for(tcont = 0; tcont < tcont_num_per_chip; tcont++){
        (void)aal_xgpon_tcont_flush(device_id, chn, tcont);
        (void)aal_xgpon_bwmp_tcont_set(device_id, chn, tcont, alloc_id_mask, &alloc_id_cfg);
    }
}

static inline void __xgpn_discard_default_xgem_id(ca_device_id_t  device_id)
{
    aal_xgpon_usbg_gem_config_msk_t      usbg_xgem_mask;
    aal_xgpon_usbg_gem_config_t          usbg_xgem_cfg;
    aal_xgpon_dsfp_xgem_cfg_msk_t        dsfp_xgem_mask;
    aal_xgpon_dsfp_xgem_cfg_t            dsfp_xgem_cfg;
    ca_uint32_t                          i = 0;
    ca_uint32_t                          def_xgem_id = 0;

    if (CA_E_OK != aal_xgpon_usbg_xgem_cfg_get(device_id, AAL_XGPON_DEFAULT_XGEM_IDX,  &usbg_xgem_cfg)) {
        return;
    } else {
        def_xgem_id = usbg_xgem_cfg.xgem_id;
        //ca_printf("default gem port id %d\r\n", def_xgem_id);
    }

    usbg_xgem_mask.u32 = ~0;
    dsfp_xgem_mask.u32 = ~0;
    for (i = 0; i < queue_num_per_tcont ; i++) { /* MAX 8 internal GEM used for default XEGM port */
        (void)aal_xgpon_usbg_xgem_cfg_get(device_id, i,  &usbg_xgem_cfg);
        if (usbg_xgem_cfg.xgem_id != def_xgem_id)
            continue;
        memset(&usbg_xgem_cfg, 0, sizeof(usbg_xgem_cfg));
        (void)aal_xgpon_usbg_xgem_cfg_set(device_id, i, usbg_xgem_mask, &usbg_xgem_cfg);
        (void)aal_xgpon_dsfp_xgem_get(device_id, i, &dsfp_xgem_cfg);
        if (dsfp_xgem_cfg.gem_id != def_xgem_id)
            continue;
        memset(&dsfp_xgem_cfg, 0, sizeof(dsfp_xgem_cfg));
        (void)aal_xgpon_dsfp_xgem_set(device_id, i, dsfp_xgem_mask, &dsfp_xgem_cfg);
    }
}

/** __xgpn_discard_v
 * @brief: disacard XGPON TC layer parameters V
 * @note3: please refer to G.98701 Table C.12-5
 *            Burst profile parameters    X       X       X       
 *            ONU-ID      X   X       X       
 *            Default Alloc-ID        X   X       X       
 *            Default XGEM Port-ID        X   X       X       
 *            Assigned Alloc-IDs                  X       
 *            Equalization delay              X   X       
 */
static void __xgpn_discard_v(ca_device_id_t  device_id)
{
    __xgpn_discard_burst_prof(device_id);
    __xgpn_discard_onuid(device_id);
#ifndef CONFIG_LUNA_G3_SERIES
    __xgpn_discard_alloc_id(device_id);
#endif
    __xgpn_discard_default_xgem_id(device_id);

}

/****************************************************************************
**
**  Function Name: aal_xgpn_activation_fsm_init
**
**  Description:
**    AAL ONU Activation FSM init
**
**  Argument        Type         IO           Description
**  --------------- ------------ -- ---------------------------------
**  device_id       ca_device_id_t  I
**
**  Return Value: the status of the operation
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        06/26/16   Initial Writing
**
****************************************************************************/
ca_uint32_t __onu_link_evt_handler(
    ca_device_id_t  device_id,
    ca_uint32_t  event_id,
    ca_void_t    *event,
    ca_uint32_t  len,
    ca_void_t    *cookie
)
{
    ca_event_gpon_port_link_t *link_evt = (ca_event_gpon_port_link_t *)event;

    if (!link_evt) {
        return CA_E_PARAM;
    }

    if (link_evt->status) {
        ca_printf("!EVENT: ONU Active(onuid = %u)!\n", link_evt->port_id);
#ifdef CONFIG_LUNA_G3_SERIES
        aal_xgpon_restore_tcont(device_id);
#endif
    } else {
        ca_printf("!EVENT: ONU De-Active(reason = %s)!\n",
            (link_evt->reason <= AAL_XGPN_ACT_EVENT_MAX) ? xgpn_act_event[link_evt->reason] : "unkown");

        __xgpn_discard_v(device_id);

        if (link_evt->reason == AAL_XGPN_ACT_EVENT_TO2_EXPIRE || link_evt->reason == AAL_XGPN_ACT_EVENT_LODS) {
            /* reset both of PON SERDES and MAC */
            //__xgpon_lods_proc(AAL_DEFAUL_DEV_ID); /* NOT handle LODS caused de-active, let LODS hander do it */
        } else {
            /* reset PON MAC only */
            aal_xgpon_reset(device_id, AAL_XGPON_RESET_FLAG_MAC);
        }
            
    }

    return CA_E_OK;
}
ca_status_t aal_xgpn_activation_fsm_init(
    ca_device_id_t device_id,
    ca_uint32_t onu_pon_mac_mode)
{
    ca_status_t rc = CA_E_OK;
    ca_uint32_t state = 0;
    ca_uint32_t index = 0;
    static  ca_boolean_t  act_fsm_initialized = FALSE;

    if (act_fsm_initialized) {
        ca_printf("ca-pon: Re-init the activation FSM!\r\n");
    }

    if (!act_fsm_initialized) { /* init event handlers, spin locks */

        if (ONU_PON_MAC_NGPON2 == onu_pon_mac_mode) {
            for (state = 0; state < AAL_XGPN_ACT_STATE_MAX; state ++) {
                  for  (index = 0; index < AAL_XGPN_ACT_EVENT_MAX; index ++) {
                    aal_xgpn_act_fsm_handlers[state][index] = aal_xgpn_act_fsm_error_handler;
                  }
            }
        } else {
            for (state = 0; state < AAL_XGPN_ACT_STATE_MAX; state ++) {
                  for  (index = 0; index < AAL_XGPN_ACT_EVENT_MAX; index ++) {
                    aal_xgpn_act_fsm_handlers[state][index] = aal_xgpn_act_fsm_common_handler;
                  }
            }
        }

        if (0 == __aal_xgpon_act_lock) {
            rc = ca_spin_lock_init(&__aal_xgpon_act_lock, SPINLOCK_BH);
            CA_RET_VALUE_CHECK(rc);
        }
    }

    memset(&g_aal_ngp2_twdm_chn_cb, 0, sizeof(g_aal_ngp2_twdm_chn_cb));

    FOR_ALL_TWDM_CHN_DO(index) {
        g_aal_ngp2_twdm_chn_cb.chn_info[index].valid         = TRUE;
        g_aal_ngp2_twdm_chn_cb.chn_info[index].oper_state    = TRUE;
        g_aal_ngp2_twdm_chn_cb.chn_info[index].active_status = FALSE;
        g_aal_ngp2_twdm_chn_cb.chn_info[index].map_989dot2_ds_chn = AAL_NGPON2_TWDM_INVALID_CHN_ID;
        g_aal_ngp2_twdm_chn_cb.chn_info[index].map_989dot2_us_chn = AAL_NGPON2_TWDM_INVALID_CHN_ID;
    }

    switch (onu_pon_mac_mode) {
    case ONU_PON_MAC_GPON:
          g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1;
        break;

    case ONU_PON_MAC_XGPON1:
          g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1;
          if (!act_fsm_initialized) {
            /*to be updated according to requirement further, e.g: ONU-ID update, LED control according to state   */
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1][AAL_XGPN_ACT_EVENT_DSYNC] = __xgpn_act_fsm_dsync_handler;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_lods_common_handler;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O4][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_lods_common_handler;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_lods_common_handler;
        }
        break;

    case ONU_PON_MAC_XGSPON:
        g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;
        if (!act_fsm_initialized) {
            /*to be updated according to requirement further, e.g: ONU-ID update, LED control according to state   */
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_1][AAL_XGPN_ACT_EVENT_DSYNC] = __xgpn_act_fsm_dsync_handler;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_2][AAL_XGPN_ACT_EVENT_DSYNC_PROFILES_ACQUIRED] = __xgpn_act_fsm_dsync_handler;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_2][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_lods_common_handler;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_lods_common_handler;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O4][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_lods_common_handler;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_1][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_lods_common_handler;
        }
        break;

    case ONU_PON_MAC_NGPON2:

          g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;

        /* get parameters from scfg and update g_ngpon2_channel_mgmt_cb*/
        // TODO: parameter restore from SCFG
        aal_xgpon_cb_set(0, 0);      // for debug only, 0 set temporarily same as debug setting, not from OLT profile
        for (index = 0; index < CA_NGPON2_MAX_DWLCH; index ++) {
            if (index < 4) // TODO: current XFP is Light66XX, only 4 channels support
                aal_xgpon_xfp_cap2_set(0, index, 1, 1, CA_NGPON2_CALIBRATION_RECORD_CHANNEL_NOT_ANNOUNCED);
            else
                aal_xgpon_xfp_cap2_set(0, index, 0, 0, CA_NGPON2_CALIBRATION_RECORD_CHANNEL_NOT_ANNOUNCED);

        }
        //aal_xgpon_xfp_chanl_get(0, &g_original_ds_chn, &g_original_us_chn);
        if (!act_fsm_initialized) {

            /*to be updated according to requirement further, e.g: ONU-ID update, LED control according to state   */
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_1][AAL_XGPN_ACT_EVENT_DSYNC] = aal_xgpn_act_fsm_s_o11_e_dsync_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_1][AAL_XGPN_ACT_EVENT_HUNT_SYNC_EXPIRE] = aal_xgpn_act_fsm_s_o11_e_sync_hunt_expire; // aal_xgpn_act_fsm_s_o11_e_rxcalibratereq_change;
            //aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_1][AAL_XGPN_ACT_EVENT_PROFILE_LEARNING] = aal_xgpn_act_fsm_s_o12_e_profileevaluate_change;//temp here for debug, to be deleted

            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_2][AAL_XGPN_ACT_EVENT_DWLCH_OK_TO_WORK] = aal_xgpn_act_fsm_s_o12_e_dwlchoktowork_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_2][AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o12_e_disablesn_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_2][AAL_XGPN_ACT_EVENT_PROFILE_LEARNING] = aal_xgpn_act_fsm_s_o12_e_profileevaluate_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_2][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_s_o12_e_lods_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O1_2][AAL_XGPN_ACT_EVENT_DWLCH_NOT_APPROPRIATE] = aal_xgpn_act_fsm_s_o12_e_dwlchnotappropriate_change;

            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_ONU_ID_ASSIGNMENT] = aal_xgpn_act_fsm_s_o23_e_onuidassign_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o23_e_disablesn_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_TX_RESPONSE_CROSS] = aal_xgpn_act_fsm_s_o23_e_txtuningreq_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_s_o23_e_lods_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_TOZ_EXPIRE] = aal_xgpn_act_fsm_s_o23_e_tozexpire_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_CALIBRATION_REQUEST] = aal_xgpn_act_fsm_s_o23_e_calibrationreq_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O2_3][AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ] = aal_xgpn_act_fsm_s_o23_e_deactiveonuid_change;

            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O4][AAL_XGPN_ACT_EVENT_TO1_EXPIRE] = __xgpn_act_fsm_to1_expire;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O4][AAL_XGPN_ACT_EVENT_EQD_ASSIGNMENT] = aal_xgpn_act_fsm_s_o4_e_eqdassign_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O4][AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o4_e_disablesn_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O4][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_s_o4_e_lods_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O4][AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ] = aal_xgpn_act_fsm_s_o4_e_deactiveonuid_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O4][AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST] = aal_xgpn_act_fsm_s_o4_e_deactiveonuid_change;

            //aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_1][AAL_XGPN_ACT_EVENT_TUNING_REQUEST] = aal_xgpn_act_fsm_s_o51_e_tuningreq_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_1][AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o51_e_disablesn_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_1][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_s_o51_e_lods_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_1][AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ] = aal_xgpn_act_fsm_s_o51_e_deactiveonuid_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_1][AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST] = aal_xgpn_act_fsm_s_o51_e_deactiveonuid_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_1][AAL_XGPN_ACT_EVENT_TUNING_REQ_ACK] = aal_xgpn_act_fsm_s_o51_e_tuningreq_ack;

            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_2][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_s_o52_e_lods_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_2][AAL_XGPN_ACT_EVENT_SFC_MATCH_FOR_TX_TUNING] = aal_xgpn_act_fsm_s_o52_e_sfcmatchtxtuning_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_2][AAL_XGPN_ACT_EVENT_SFC_MATCH_FOR_RXTX_TUNING] = aal_xgpn_act_fsm_s_o52_e_sfcmatchrxtxtuning_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_2][AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o52_e_disablesn_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_2][AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ] = aal_xgpn_act_fsm_s_o52_e_deactiveonuid_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O5_2][AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST] = aal_xgpn_act_fsm_s_o52_e_deactiveonuid_change;

            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O6][AAL_XGPN_ACT_EVENT_DSYNC] = aal_xgpn_act_fsm_s_o6_e_dsync_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O6][AAL_XGPN_ACT_EVENT_TO2_EXPIRE] = aal_xgpn_act_fsm_s_o6_e_to2expire_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O6][AAL_XGPN_ACT_EVENT_TO3_EXPIRE] = aal_xgpn_act_fsm_s_o6_e_to3expire_change;

            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O7][AAL_XGPN_ACT_EVENT_ENABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o7_e_enablesn_change;

            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_1][AAL_XGPN_ACT_EVENT_DSYNC] = aal_xgpn_act_fsm_s_o81_e_dsync_change;
            //aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_1][AAL_XGPN_ACT_EVENT_TX_TUNING_REQ] = aal_xgpn_act_fsm_s_o81_e_txrxtuningreq_change;
            //aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_1][AAL_XGPN_ACT_EVENT_TX_RX_TUNING_REQ] = aal_xgpn_act_fsm_s_o81_e_txrxtuningreq_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_1][AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o81_e_disablesn_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_1][AAL_XGPN_ACT_EVENT_TO4_EXPIRE] = aal_xgpn_act_fsm_s_o81_e_to4expire_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_1][AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ] = aal_xgpn_act_fsm_s_o81_e_deactiveonuid_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_1][AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST] = aal_xgpn_act_fsm_s_o81_e_deactiveonuid_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_1][AAL_XGPN_ACT_EVENT_HUNT_SYNC_EXPIRE] = aal_xgpn_act_fsm_s_o81_e_to4expire_change;

            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_2][AAL_XGPN_ACT_EVENT_DWLCH_OK_TO_WORK] = aal_xgpn_act_fsm_s_o82_e_dwlchoktowork_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_2][AAL_XGPN_ACT_EVENT_PROFILE_LEARNING] = aal_xgpn_act_fsm_s_o82_e_profileevaluate_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_2][AAL_XGPN_ACT_EVENT_LODS] = aal_xgpn_act_fsm_s_o82_e_lods_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_2][AAL_XGPN_ACT_EVENT_DWLCH_NOT_APPROPRIATE] = aal_xgpn_act_fsm_s_o82_e_dwlchnotappropriate_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_2][AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o82_e_disablesn_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_2][AAL_XGPN_ACT_EVENT_TO4_EXPIRE] = aal_xgpn_act_fsm_s_o82_e_to4expire_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_2][AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ] = aal_xgpn_act_fsm_s_o82_e_deactiveonuid_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O8_2][AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST] = aal_xgpn_act_fsm_s_o82_e_deactiveonuid_change;


            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O9][AAL_XGPN_ACT_EVENT_US_TUNING_CONFIRM] = aal_xgpn_act_fsm_s_o9_e_ustuningconfirm_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O9][AAL_XGPN_ACT_EVENT_TX_RESPONSE_CROSS] = aal_xgpn_act_fsm_s_o9_e_txtuningreq_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O9][AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST] = aal_xgpn_act_fsm_s_o9_e_disablesn_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O9][AAL_XGPN_ACT_EVENT_TO5_EXPIRE] = aal_xgpn_act_fsm_s_o9_e_to5expire_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O9][AAL_XGPN_ACT_EVENT_DEACTIVE_ONUID_REQ] = aal_xgpn_act_fsm_s_o9_e_deactiveonuid_change;
            aal_xgpn_act_fsm_handlers[AAL_XGPN_ACT_STATE_O9][AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST] = aal_xgpn_act_fsm_s_o9_e_deactiveonuid_change;
        }
        __xgpn_chn_init(device_id, onu_pon_mac_mode);

        break;
    default:
        ca_printf("!ERROR: ca-pon: Wrong ONU PON MAC mode: %d\r\n", onu_pon_mac_mode);
        return CA_E_PARAM;
    }
    g_xgpn_act_fsm_cur_state = g_xgpn_act_state;
    g_xgpn_act_fsm_pre_state = g_xgpn_act_state;

    /*init log for FSM */
    if (!act_fsm_initialized) {
        rc = ca_event_register(device_id, CA_EVENT_GPON_PORT_LINK,
            __onu_link_evt_handler,  NULL);
        if (CA_E_OK != rc) {
            ca_printf("!ERROR: ca_event_register CA_EVENT_GPON_PORT_LINK fail, return %d\r\n", rc);
            return rc;
        }
    }

    act_fsm_initialized = TRUE;

    return rc;
}


/****************************************************************************
**
**  Function Name: aal_xgpn_dbg_activation_dump
**
**  Description:
**    Dump XGPON activation state info
**
**  Argument        Type     IO           Description
**  --------------- -------- -- -------------------------------------------
**  void
**
**  Return Value: N/A
**
**  HISTORY (Recommended but Optional):
**
**  Programmer           Date       Description of Revision
**  -------------------- ---------- ------------------------------
**  Junfeng Zhang        07/23/16   Initial Writing
**
****************************************************************************/

void aal_xgpn_dbg_activation_dump(void)
{
    ca_uint32_t index = 0;
    ca_uint64_t up_time = 0;
    aal_xgpon_actp_status_t  hw_actp_status;
    char *pon_mode_str[ONU_PON_MAC_MAX] = {
        "EPON",
        "EPON-D10G",
        "EPON-BI10G",
        "GPON",
        "XGPON1",
        "XGSPON",
        "NGPON2",
        "ETHERNET"
    };
    char *dsync_str[] = {"hunt", "wait_hec", "pre-sync", "sync", "re-sync"};
        
    ca_uint8_t  pon_mode = g_xgpn_basic_info.pon_mode;
    ca_uint32_t toz = 0, to1 = 0, to2 = 0, to4 = 0, to5 = 0; /* TOZ, TO1, TO23, TO4, TO5*/

    memset(&hw_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    (void)aal_xgpon_actp_status_get(AAL_DEFAUL_DEV_ID, &hw_actp_status);

    /*dump CB record */
    if (ONU_PON_MAC_NGPON2 == aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID)) {
        ca_printf("Profile Learnt flag: %d\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].learnt_flag);
        if (g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.learnt_flag)
            ca_printf("  System profile twdm channel counter: %d\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.twdm_channel_count);
        for (index = 0; index < CA_XGPON_MAX_CHANNEL_PROFILE_COUNT; index ++)
        {
            if (g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].learnt_flag)
            {
                ca_printf("  Channel profile index: %d\r\n", index);
                ca_printf("    Control octet: 0x%x\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].channel_profile_idx);
                ca_printf("    Version: 0x%x\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].channel_profile_version);
                ca_printf("    PON ID: 0x%x\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].pon_id);
                ca_printf("    DS_Frq_offset: 0x%x\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].ds_freq_offset);
                ca_printf("    DS Rate: 0x%x\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].ds_rate);
                ca_printf("    CPI: 0x%x\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].channel_partition);
                ca_printf("    US PON ID: 0x%08x\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].uwlch_id);
                ca_printf("    US Rate: 0x%x\r\n", g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].us_rate);
            }
        }

        ca_printf("    Original US channel : 0x%x\r\n", g_original_us_chn);
        ca_printf("    Original DS channel : 0x%x\r\n", g_original_ds_chn);
    }

    /*dump all log of XGPON state/event */

    ca_printf("XGPON Activation Finite State Machine Information\r\n");
    ca_printf("----------------------------------------------------------------\r\n");
    ca_printf("%-25s: %u(%s)\r\n", "PON mode", pon_mode, 
        CA_PON_MODE_IS_GPON_FAMILY(pon_mode) ? pon_mode_str[pon_mode] : "Wrong PON mode scfg for XGPON");
    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        ca_uint8_t tmp_str[5] = {0};
        tmp_str[0] = (g_xgpn_basic_info.vendor_id >> 24) & 0xFF;
        tmp_str[1] = (g_xgpn_basic_info.vendor_id >> 16) & 0xFF;
        tmp_str[2] = (g_xgpn_basic_info.vendor_id >>  8) & 0xFF;
        tmp_str[3] = (g_xgpn_basic_info.vendor_id >>  0) & 0xFF;
        ca_printf("%-25s: 0x%08x(%s)\r\n", "VendorID", g_xgpn_basic_info.vendor_id, 
            g_xgpn_basic_info.vendor_id ? (char *)tmp_str : "(null)");
        ca_printf("%-25s: 0x%08x\r\n", "VSSN", g_xgpn_basic_info.vssn);
        (void)aal_xgpon_actp_to1_get(AAL_DEFAUL_DEV_ID, &to1);
        (void)aal_xgpon_actp_to23_get(AAL_DEFAUL_DEV_ID, &to2);
        ca_printf("%-25s: %u(unit:125us)\r\n", "TO1", to1);
        ca_printf("%-25s: %u(unit:125us)\r\n", "TO2", to2);

        if (pon_mode == ONU_PON_MAC_NGPON2) {
            (void)aal_xgpon_actp_toz_get(AAL_DEFAUL_DEV_ID, &toz);
            (void)aal_xgpon_actp_to4_get(AAL_DEFAUL_DEV_ID, &to4);
            (void)aal_xgpon_actp_to5_get(AAL_DEFAUL_DEV_ID, &to5);
            ca_printf("%-25s: %u(unit:125us)\r\n", "TOZ", toz);
            ca_printf("%-25s: %u(unit:125us)\r\n", "TO4", to4);
            ca_printf("%-25s: %u(unit:125us)\r\n", "TO5", to5);
        }

        ca_printf("%-25s: %s\r\n", "S/W FSM current event",  xgpn_act_event[g_xgpn_act_fsm_cur_evt] );
        ca_printf("%-25s: %s\r\n", "S/W FSM current state", xgpn_act_state[g_xgpn_act_fsm_cur_state]);
        ca_printf("%-25s: %s\r\n", "S/W FSM previous state", xgpn_act_state[g_xgpn_act_fsm_pre_state]);
        ca_printf("%-25s: %s\r\n", "S/W FSM running state",
            (g_xgpn_act_state < AAL_XGPN_ACT_STATE_MAX) ? xgpn_act_state[g_xgpn_act_state] : "Unkown state");
        index = __xgpn_state_to_fsm_state(hw_actp_status.onu_state, hw_actp_status.onu_sub_state);
        ca_printf("%-25s: %s\r\n", "H/W FSM current state",
            (index < AAL_XGPN_ACT_STATE_MAX) ? xgpn_act_state[index] : "Unkown state");
        index = __xgpn_state_to_fsm_state(hw_actp_status.previous_onu_state, hw_actp_status.previous_sub_state);
        ca_printf("%-25s: %s\r\n", "H/W FSM previous state",
            (index < AAL_XGPN_ACT_STATE_MAX) ? xgpn_act_state[index] : "Unkown state");

        ca_printf("%-25s: %u\r\n", "Activated counter", g_xgpn_act_count);
        //ca_printf("%-30s: %u\r\n", "FSM SW Rx interrupt count", g_xgpn_act_fsm_intr_cnt);
        if (g_xgpn_act_fsm_outsync_cnt)
            ca_printf("%-25s: %u\r\n", "FSM SW state out of SYNC count", g_xgpn_act_fsm_outsync_cnt);

        if (g_xgpon_act_fsm_err) {
            ca_printf("%-25s: %u\r\n", "FSM latest error code", g_xgpon_act_fsm_err);
        }
        
        if (g_xgpn_act_fsm_handle_fail_cnt) {
            ca_printf("%-25s: %u\r\n", "FSM handler error count", g_xgpn_act_fsm_handle_fail_cnt);
        }

        /*dump new state */
        if (g_xgpn_act_state >= AAL_XGPN_ACT_STATE_O4 && g_xgpn_basic_info.onu_id != AAL_XGPON_INVALID_ONU_ID) {
            aal_xgpon_dsfp_oc_body_config_t  oc_body;
            memset(&oc_body, 0, sizeof(oc_body));
            (void)aal_xgpon_dsfp_oc_body_get(AAL_DEFAUL_DEV_ID, &oc_body);
            up_time = ca_current_time() - g_xgpn_act_time_tick;
            ca_printf("%-25s: %u \r\n", "ONU assigned ID", g_xgpn_basic_info.onu_id);
            ca_printf("%-25s: 0x%08x \r\n", "PON-ID", oc_body.ponid0);
            ca_printf("%-25s: %llu miliseconds ( %llu system ticks)\r\n", "Online time", up_time * 10, up_time);
        } else {
            (void)aal_xgpon_dsfp_sync_state_get(AAL_DEFAUL_DEV_ID, &index);
            ca_printf("%-25s: %s \r\n", "ONU Dsync state", index < 5 ? dsync_str[index]: "unkown");
        }
        ca_printf("Registration ID: 0x");
        for (index=0; index<36; index++)
            ca_printf("%02x", g_xgpn_basic_info.registration_id[index]);
        ca_printf("\r\n");
    }

    return;
}

ca_status_t  aal_xgpon_activation_state_get(ca_device_id_t device_id)
{
    aal_xgpn_dbg_activation_dump();
    return CA_E_OK;
}

ca_status_t aal_ngpon2_ploam_calibration_req_handler(ca_uint8_t *source)
{
    __AAL_XGPN_ACT_LOCK(0);

    memcpy(&g_ngpon2_channel_mgmt_cb.calibration_request.vendor_id, source, 22);
    g_ngpon2_channel_mgmt_cb.calibration_request.calibration_request_flag = TRUE;

     __AAL_XGPN_ACT_UNLOCK(0);

    return CA_E_OK;
}
ca_status_t aal_ngpon2_ploam_tuning_ctrl_handler(ca_uint8_t *source)
{
    ploam_ngpon2_tuning_ctrl_t *tuning_ctrl_pkt = (ploam_ngpon2_tuning_ctrl_t *)source;

    CA_NULL_PTR_CHECK(source);

    __AAL_XGPN_ACT_LOCK(0);

    g_ngpon2_channel_mgmt_cb.tuning_control.operation_code = tuning_ctrl_pkt->op_code;
    g_ngpon2_channel_mgmt_cb.tuning_control.scheduled_sfc = ntohs(tuning_ctrl_pkt->scheduled_sfc);
    g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag = tuning_ctrl_pkt->rollback_flag;
    g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id = ntohl(tuning_ctrl_pkt->target_ds_pon_id);
    g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id = ntohl(tuning_ctrl_pkt->target_us_pon_id);
    g_ngpon2_channel_mgmt_cb.tuning_control.calibration_flag = tuning_ctrl_pkt->calibration_flag;

    //memcpy(&g_ngpon2_channel_mgmt_cb.tuning_control.operation_code, source,13);

    // TODO:
    __xgpn_act_fsm_set_target_chn(AAL_DEFAUL_DEV_ID,
        __GET_CHN_FROM_PONID(g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id),
        __GET_CHN_FROM_PONID(g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id));

    g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag = TRUE;

     __AAL_XGPN_ACT_UNLOCK(0);

    ca_printf("%s::%d==============================================================\r\n",__FUNCTION__,__LINE__);
    ca_printf("g_ngpon2_channel_mgmt_cb.tuning_control.operation_code   = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.operation_code   );
    ca_printf("g_ngpon2_channel_mgmt_cb.tuning_control.scheduled_sfc    = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.scheduled_sfc    );
    ca_printf("g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag    = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag    );
    ca_printf("g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id );
    ca_printf("g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id );
    ca_printf("g_ngpon2_channel_mgmt_cb.tuning_control.calibration_flag = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.calibration_flag );
    ca_printf("g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag );
    ca_printf("===================================================================\r\n");

    return CA_E_OK;
}

static void __xgpn_actfsm_reset(ca_device_id_t device_id)
{
    //ca_status_t                  ret = CA_E_OK;
    aal_xgpon_actp_ctrl_mask_t   actp_ctrl_mask;
    aal_xgpon_actp_ctrl_t        actp_ctrl;
    aal_xgpon_actp_status_mask_t actp_status_mask;
    aal_xgpon_actp_status_t      actp_status;

    memset(&actp_ctrl, 0, sizeof(actp_ctrl));
    memset(&actp_status, 0, sizeof(actp_status));
    actp_ctrl_mask.u32_data = 0;
    actp_ctrl_mask.s.sw_mode = 1;
    (void)aal_xgpon_actp_ctrl_set(device_id, actp_ctrl_mask, &actp_ctrl);

    actp_status_mask.u32_data = 0;
    actp_status_mask.s.onu_state = 1;
    actp_status.onu_state = 0;
    (void)aal_xgpon_actp_status_set(device_id, actp_status_mask, &actp_status);

    actp_ctrl_mask.s.sw_mode = 0;
    (void)aal_xgpon_actp_ctrl_set(device_id, actp_ctrl_mask, &actp_ctrl);

}

static void __xgpn_actfsm_refuse_assign_onuid(ca_device_id_t device_id)
{
    __xgpn_actfsm_reset(device_id);
    g_xgpn_basic_info.onu_id = AAL_XGPON_INVALID_ONU_ID;

}

static void __xgpn_actfsm_refuse_assign_allocid(ca_device_id_t device_id)
{
    ca_uint8_t wave_channel = 0;
    aal_xgpon_bwmp_alloc_id_cfg_msk_t alloc_id_mask;
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;
    aal_xgpn_plmp_ctrl_mask_t  ploam_ctrl_mask;
    aal_xgpn_plmp_ctrl_t       ploam_ctrl;

    ploam_ctrl_mask.wrd = 0;
    ploam_ctrl_mask.bf.sw_mode_en = 1;
    memset(&ploam_ctrl, 0, sizeof(ploam_ctrl));
    ploam_ctrl.sw_mode_en = 1;
    (void)aal_ploam_ctrl_set(device_id, ploam_ctrl_mask, &ploam_ctrl);

    alloc_id_mask.u32 = 0;
    alloc_id_mask.s.alloc_id = 1;
    alloc_id_mask.s.omci_en  = 1;
    alloc_id_mask.s.ploam_en = 1;
    alloc_id_cfg.alloc_id = AAL_XGPON_ALLOC_ID_INVALID;
    alloc_id_cfg.omci_en  = 1;
    alloc_id_cfg.ploam_en = 0;
    for(wave_channel = 0; wave_channel < AAL_XGPON_WAVE_CHANNEL_MAX_NUM; wave_channel++){
        (void)aal_xgpon_bwmp_tcont_set(device_id,
                wave_channel, 0, alloc_id_mask, &alloc_id_cfg);
    }
    // TODO: need send UPSTREAM by SW?
    ploam_ctrl.sw_mode_en = 0;
    (void)aal_ploam_ctrl_set(device_id, ploam_ctrl_mask, &ploam_ctrl);

}


ca_status_t aal_ngpon2_ploam_ranging_handler(ca_uint8_t *source)
{
    ploam_ngpon2_header_t             *ploam_hdr = (ploam_ngpon2_header_t*)source;
    ploam_ngpon2_ranging_time_t *ranging_pkt     = (ploam_ngpon2_ranging_time_t *)(source + PLOAM_NGPON2_HEADER_SIZE);
    //ca_uint8_t                         us_chn    = 0;

    CA_NULL_PTR_CHECK(ploam_hdr);

    __xgpn_act_fsm_store_chn_eqd(AAL_DEFAUL_DEV_ID,
        ntohl(ranging_pkt->equ_delay));

    return CA_E_OK;

}


ca_status_t aal_ngpon2_ploam_assign_onuid_handler(ca_uint8_t *source)
{
    //ploam_ngpon2_header_t             *ploam_hdr      = (ploam_ngpon2_header_t*)source;
    ploam_ngpon2_assign_onuid_t *assign_onuid_pkt     = (ploam_ngpon2_assign_onuid_t *)(source + PLOAM_NGPON2_HEADER_SIZE);
    ca_uint16_t                         onu_id        = 0;

    CA_NULL_PTR_CHECK(source);

    if (AAL_XGPON_ACTFSM_CHK_ONUID_IS_ON()) {
        return CA_E_OK;
    }

    onu_id = ntohs(assign_onuid_pkt->assign_onu_id);
    if (aal_xgpon_onu_id_assign_chk(onu_id)) {
        /* ONU ID is permit */
        //g_xgpn_basic_info.onu_id = onu_id;
        return CA_E_OK;
    }

    /* Refuse ONU ID assign and reset H/W FSM */
    __AAL_XGPN_ACT_LOCK(AAL_DEFAUL_DEV_ID);
    __xgpn_actfsm_refuse_assign_onuid(AAL_DEFAUL_DEV_ID);
    __AAL_XGPN_ACT_UNLOCK(AAL_DEFAUL_DEV_ID);

    XGPN_ACTFSM_LOG_CRI("Refuse ONU ID %u assigned.\n", onu_id);

    return CA_E_OK;

}

ca_status_t aal_ngpon2_ploam_allocid_handler(ca_uint8_t *source)
{
    //ploam_ngpon2_header_t             *ploam_hdr      = (ploam_ngpon2_header_t*)source;
    ploam_ngpon2_assign_allocid_t     *alloc_id_pkt     = (ploam_ngpon2_assign_allocid_t *)(source + PLOAM_NGPON2_HEADER_SIZE);
    ca_uint16_t                         alloc_id      = 0;

    CA_NULL_PTR_CHECK(source);

    if (!AAL_XGPON_ACTFSM_CHK_ALLOCID_IS_ON()) {
        return CA_E_OK;
    }

    /*
     * 0x01: XGEM-encapsulated payload.
     * 0xFF: Deallocate this Alloc-ID.
     */
    if (alloc_id_pkt->alloc_id_type != 0x01) {
        return CA_E_OK;
    }

    alloc_id = ntohs(alloc_id_pkt->alloc_id_value);
    if (aal_xgpon_alloc_id_assign_chk(alloc_id)) {
        /* Alloc ID is permit */
        return CA_E_OK;
    }

    /* Refuse ONU ID assign and reset H/W FSM */
    __AAL_XGPN_ACT_LOCK(AAL_DEFAUL_DEV_ID);
    __xgpn_actfsm_refuse_assign_allocid(AAL_DEFAUL_DEV_ID);
    __AAL_XGPN_ACT_UNLOCK(AAL_DEFAUL_DEV_ID);

    XGPN_ACTFSM_LOG_CRI("Ignore Alloc ID %u assigned.\n", alloc_id);

    return CA_E_OK;

}

ca_status_t aal_ngpon2_ploam_sys_profile_handler(ca_uint8_t *source)
{
    ploam_ngpon2_sys_profile_t *sys_prof_pkt = (ploam_ngpon2_sys_profile_t *)source;
    ca_xgpn_sys_profile_t      *sys_prof_cfg = &g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile;

    CA_NULL_PTR_CHECK(source);

    if (sys_prof_cfg->learnt_flag) {
        AAL_XGPON_DBG_ACTFSM("System profile was already learnt, version: 0x%x,"\
            "new version: %0x%x\r\n",
            sys_prof_cfg->version,
            sys_prof_pkt->sys_profile_ver);
    } else {
        AAL_XGPON_DBG_ACTFSM("System profile learn: version: 0x%x\r\n",
            sys_prof_pkt->sys_profile_ver);
    }

    __AAL_XGPN_ACT_LOCK(AAL_DEFAUL_DEV_ID);

   /* System Profile save at ONU:
    * The ONU retains the system and channel profile parameters through the
    * completion of the activation cycle, unless it enters state O1.1 from state
    * O7, that is, recovers from Emergency Stop
    */
    sys_prof_cfg->ng2sysid[0] = sys_prof_pkt->ng2sys_id[0];
    sys_prof_cfg->ng2sysid[1] = sys_prof_pkt->ng2sys_id[1];
    sys_prof_cfg->ng2sysid[2] = sys_prof_pkt->ng2sys_id[2];
    sys_prof_cfg->version = sys_prof_pkt->sys_profile_ver;
    sys_prof_cfg->us_op_wl_bands = sys_prof_pkt->us_op_wave_len_bands;
    sys_prof_cfg->twdm_channel_count = sys_prof_pkt->twdm_channel_cnt;
    sys_prof_cfg->channel_spacing_twdm = sys_prof_pkt->channel_spacing;
    sys_prof_cfg->us_mse_twdm = sys_prof_pkt->us_mse;
    sys_prof_cfg->fsr_twdm = sys_prof_pkt->fsr;
    sys_prof_cfg->twmd_amcc_control = sys_prof_pkt->amcc_ctrl;
    sys_prof_cfg->channel_loose_calibratiob_bound_twdm = sys_prof_pkt->loose_calibration;
    sys_prof_cfg->ptp_wdm_channel_count = sys_prof_pkt->channel_cnt;
    sys_prof_cfg->channel_spacing_ptp = sys_prof_pkt->channel_spacing_ptp;
    sys_prof_cfg->us_mse_ptp = sys_prof_pkt->us_mse_ptp;
    sys_prof_cfg->fsr_ptp = sys_prof_pkt->fsr_ptp;
    sys_prof_cfg->ptp_calibration_accuracy = sys_prof_pkt->calibration_accuracy;
    sys_prof_cfg->loose_calibration_bound_ptp = sys_prof_pkt->loose_calibration_bound;
    /* lidong@2017/06/12: modify since the below code has byte-allignment issue */
    //memcpy(&g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.ng2sysid, source, 20);

    sys_prof_cfg->learnt_flag = TRUE;
     __AAL_XGPN_ACT_UNLOCK(AAL_DEFAUL_DEV_ID);

    return CA_E_OK;

}

ca_status_t aal_ngpon2_ploam_burst_profile_handler(ca_uint8_t *source)
{
    ploam_ngpon2_header_t             *ploam_hdr      = (ploam_ngpon2_header_t*)source;
    ploam_ngpon2_twdm_burst_profile_t *burst_prof_pkt = (ploam_ngpon2_twdm_burst_profile_t *)(source + PLOAM_NGPON2_HEADER_SIZE);
    ca_xgpn_burst_profile_t           *burst_prof     = NULL;
    ca_uint8_t                         prof_idx       = 0;

    CA_NULL_PTR_CHECK(source);


   /* The burst profile information accumulated by the ONU does not persist
    * across ONU activations. A newly activated ONU may respond to a serial
    * number grant only after it has acquired the burst profile information
    * associated with the grant. More generally, an ONU in any state can
    * respond to an allocation structure only if it has previously acquired
    * the corresponding burst profile information
    */
    prof_idx = (burst_prof_pkt->burst_profile_ctrl & 0x7)
                + ((g_original_us_chn & 0x7) << 3);
    if (prof_idx >= CA_XGPON_MAX_BURST_PROFILE_COUNT) {
        AAL_XGPON_DBG_ACTFSM("Wrong burst profile index got %d\r\n", prof_idx);
        return CA_E_OK;
    }

    /*
        F - Applicability of the message to specific upstream line rates:
        F = 0: The profile applies to ONUs transmitting at 2.48832 Gbit/s upstream line rate.
        F = 1: The profile applies to ONUs transmitting at  9.95328 Gbit/s upstream line rate.
    */
    if (0 == ((burst_prof_pkt->burst_profile_ctrl >> 2) & 0x1)) {
        AAL_XGPON_DBG_ACTFSM("XGPON1 burst profile index got %d\r\n",
            burst_prof_pkt->burst_profile_ctrl);
    }

    __AAL_XGPN_ACT_LOCK(AAL_DEFAUL_DEV_ID);

    burst_prof = &g_ngpon2_channel_mgmt_cb.profile_record[0].burst_profile[prof_idx];
    burst_prof->onu_id = ntohs(ploam_hdr->onu_id);
    burst_prof->msg_type   = ploam_hdr->msg_id;
    burst_prof->seq_number = ploam_hdr->seq_no;

    burst_prof->version_profile_idx   = burst_prof_pkt->burst_profile_ctrl;
    burst_prof->up_fec_indication     = burst_prof_pkt->us_fec_ind;
    burst_prof->delimiter_len         = burst_prof_pkt->delimiter_len;
    burst_prof->preamble_len          = burst_prof_pkt->preamble_len;
    burst_prof->preamble_repeat_count = burst_prof_pkt->preamble_repeat_cnt;
    memcpy(burst_prof->delimiter, burst_prof_pkt->delimiter, CA_XGPON_BP_PRF_DELMT_LEN);
    memcpy(burst_prof->preamble, burst_prof_pkt->preamble_pattern, CA_XGPON_BP_PRF_PREEM_LEN);
    memcpy(burst_prof->pon_tag, burst_prof_pkt->pon_tag, CA_XGPON_BP_PRF_PONTAG_LEN);

     __AAL_XGPN_ACT_UNLOCK(AAL_DEFAUL_DEV_ID);

    return CA_E_OK;

}

ca_status_t aal_xgpon_burst_profile_get(
    CA_IN  ca_device_id_t               device_id,
    CA_IN  ca_uint8_t                burst_prof_idx,
    CA_OUT ca_xgpn_burst_profile_t  *burst_prof)
{
    ca_uint8_t  local_prof_idx = 0;

    CA_DEV_ID_CHECK(device_id);
    CA_NULL_PTR_CHECK(burst_prof);


    if (aal_pon_mac_mode_get(device_id) == ONU_PON_MAC_XGPON1)
        local_prof_idx = burst_prof_idx;
    else
        local_prof_idx = burst_prof_idx + (0x01 << 2)+ ((g_original_ds_chn & 0x7) << 3);

    __AAL_XGPN_ACT_LOCK(AAL_DEFAUL_DEV_ID);
    memcpy(burst_prof, XGPN_BURST_PROF_PTR(device_id, local_prof_idx), sizeof(ca_xgpn_burst_profile_t));
     __AAL_XGPN_ACT_UNLOCK(AAL_DEFAUL_DEV_ID);

    return CA_E_OK;
}


ca_status_t aal_xgpon_sys_profile_get(
    CA_IN  ca_device_id_t             device_id,
    CA_OUT ca_xgpn_sys_profile_t  *sys_prof)
{
    CA_DEV_ID_CHECK(device_id);
    CA_NULL_PTR_CHECK(sys_prof);

    __AAL_XGPN_ACT_LOCK(AAL_DEFAUL_DEV_ID);
    memcpy(sys_prof, XGPN_SYS_PROF_PTR(device_id), sizeof(ca_xgpn_sys_profile_t));
     __AAL_XGPN_ACT_UNLOCK(AAL_DEFAUL_DEV_ID);

    return CA_E_OK;
}


ca_status_t aal_xgpon_channel_profile_get(
    CA_IN  ca_device_id_t               device_id,
    CA_IN  ca_uint8_t                chn_prof_idx,
    CA_OUT ca_xgpn_twdm_channel_profile_t  *chn_prof)
{
    CA_DEV_ID_CHECK(device_id);
    CA_NULL_PTR_CHECK(chn_prof);


    __AAL_XGPN_ACT_LOCK(AAL_DEFAUL_DEV_ID);
    memcpy(chn_prof, XGPN_CHN_PROF_PTR(device_id, chn_prof_idx), sizeof(ca_xgpn_twdm_channel_profile_t));
     __AAL_XGPN_ACT_UNLOCK(AAL_DEFAUL_DEV_ID);

    return CA_E_OK;
}

ca_status_t aal_ngpon2_ploam_channel_profile_handler(ca_uint8_t *source)
{
    //ploam_ngpon2_channel_profile_t  *prof_pkt = (ploam_ngpon2_channel_profile_t *)source;
    ca_uint8_t profile_index;
    ca_status_t rc = CA_E_OK;

    __AAL_XGPN_ACT_LOCK(0);

    //AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);

    /*Update into corresponding ca_ngpon2_channel_mgmt_t profile record; */
    profile_index = ((*source) >> 4) & 0xF;
    // TODO: COPY channel profile one by one
    memcpy(&g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[profile_index].channel_profile_idx, source, 1);

    /*Update into corresponding ca_ngpon2_channel_mgmt_t profile record;
    Create Correlation tag if not created, update Calibration record status;
    If DS/UP void indicator, clear/void calibration record for this profile; */
    g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[profile_index].correlation_tag = (profile_index & 0xFF) << 8;
    if ((*source) & 0x1) //Upstream descriptor to be ignored
        g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[profile_index].calibration_record_us.calibration_record_status = CA_NGPON2_CALIBRATION_RECORD_CHANNEL_NOT_ANNOUNCED;
    if ((*source) & 0x2) //Downstream descriptor to be ignored
        g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[profile_index].calibration_record_ds.calibration_record_status = CA_NGPON2_CALIBRATION_RECORD_CHANNEL_NOT_ANNOUNCED;

    /*Update corresponding learnt_flag = TRUE; update channel_profile_count_learnt;
    If all profiles learnt count reaches (N (defined in system profile reaches), set learnt_flag in ca_ngpon2_profile_record_t, and if (profile_evaluate_request): channel profile ready bit in CSR System and channel profile indication register.
    {
    Start channel evaluation;
    If any profile match, configure transceiver RX parameters, and then set HW register profile_learning to OK; if not match, set HW register profile learning to NOK;
    Clear profile_evaluate_request;
    }*/

    /*learn each channel profile and put it into the global variable*/
    if (FALSE == g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[profile_index].learnt_flag)
    {
        g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[profile_index].learnt_flag = TRUE;
        g_ngpon2_channel_mgmt_cb.profile_record[0].channel_profile_count_learnt ++;

        if ((TRUE == g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.learnt_flag) &&
            (TRUE == g_ngpon2_channel_mgmt_cb.profile_record[0].profile_evaluate_request))
        {
            if (g_ngpon2_channel_mgmt_cb.profile_record[0].channel_profile_count_learnt
                < g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.twdm_channel_count)
            {
                // profile evaluation, and set result to HW if profile match
                rc = aal_ngpon2_profile_evaluate(FALSE);
            }
            else if (g_ngpon2_channel_mgmt_cb.profile_record[0].channel_profile_count_learnt
                == g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.twdm_channel_count)
            {
                g_ngpon2_channel_mgmt_cb.profile_record[0].learnt_flag = TRUE;
                // profile evaluation, and set result to HW
                rc = aal_ngpon2_profile_evaluate(TRUE);
            }
        }
    }

//    AAL_XGPON_DBG_ACTFSM("line <%d>\r\n", __LINE__);

    __AAL_XGPN_ACT_UNLOCK(0);

    return rc;
}
ca_status_t aal_ngpon2_ploam_protection_ctrl_handler(void *data_p)
{
    ca_uint8_t index_d, index_u;
    ca_status_t rc = CA_E_OK;
    ca_uint8_t  protect_chn = 0;
    aal_xgpon_actp_ctrl_mask_t xgpn_actp_ctrl_mask;
    aal_xgpon_actp_ctrl_t xgpn_actp_ctrl;
    ploam_ngpon2_protection_ctrl_t *pkt = (ploam_ngpon2_protection_ctrl_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);
    ca_uint32_t ds_pon_id = 0;
    ca_uint32_t us_pon_id = 0;

    __AAL_XGPN_ACT_LOCK(0);

    // protection control PLOAM could only be handled in O5.1 state
    if (AAL_XGPN_ACT_STATE_O5_1 != g_xgpn_act_state)
    {
        __AAL_XGPN_ERROR("Protection control PLOAM should not be handled at state: %s\r\n", xgpn_act_state[g_xgpn_act_state]);
        //return CA_E_ERROR;
    }

    ds_pon_id = ntohl(pkt->protect_ds_pon_id);
    us_pon_id = ntohl(pkt->protect_us_pon_id);

    //check whether supported or not
    index_d = ds_pon_id & 0xF;
    index_u = us_pon_id & 0xF;
#ifdef NEW_ACT_FSM
    if (pkt->enable_flag) {
        protect_chn = (index_d << 4) | (index_u);
        rc = scfg_write(0, CFG_ID_PON_NGPON2_PROTECTION_CHAN, sizeof(ca_uint8_t), (ca_uint8_t *) &protect_chn);
        if (CA_E_OK != rc){
            __AAL_XGPN_ERROR("Save channel id to scfg fail.aal_xgpon_actp_ctrl_set: %d\r\n", rc);
        }
    }
#endif
    if (__XCVR_CHN_IS_VALID(index_d) && __XCVR_CHN_IS_VALID(index_u))
    {   // transceiver can support
        g_ngpon2_channel_mgmt_cb.protection_control.protection_ds_pon_id = ds_pon_id;
        g_ngpon2_channel_mgmt_cb.protection_control.protection_us_pon_id = us_pon_id;

        g_ngpon2_channel_mgmt_cb.protection_control.protection_ctrl_flag = pkt->enable_flag;

        //to indicate ASIC to enable WLCP;
        memset(&xgpn_actp_ctrl_mask, 0, sizeof(aal_xgpon_actp_ctrl_mask_t));
        memset(&xgpn_actp_ctrl, 0, sizeof(aal_xgpon_actp_ctrl_t));
        xgpn_actp_ctrl_mask.s.wlcp_enable = 1;
        xgpn_actp_ctrl.wlcp_enable = pkt->enable_flag;   // 1'b0: WLCP enable
        rc = aal_xgpon_actp_ctrl_set(0, xgpn_actp_ctrl_mask, &xgpn_actp_ctrl);
        if (CA_E_OK != rc){
            __AAL_XGPN_ERROR("aal_xgpon_actp_ctrl_set: %d\r\n", rc);
        }
    }

    __AAL_XGPN_ACT_UNLOCK(0);

    return rc;
}

/****************************************************************************
**
**  The following for debug purpose, some to be called after system up, before PON MAC enable
**
**
****************************************************************************/
/*
For the fibre cut cause the Xilinx serdes out of sync issue:
reset the Xilinx serdes and rx-elastic FIFO by below steps to force the Xilinx Serdes re-lock with the line clock:
1.       Reg_op w 0xf3020008 0x1 // reset Xilinx serdes
2.       Wait 10ms
3.       Reg_op w 0xf3020008 0x2 // resync rx elastic FIFO
4.       Wait 10ms
5.       Reg_op w 0xf3020008 0x0
6.       Wait 10ms
7.       Reg_op r 0xf302000c 1 // check PLL status
    a.       If the value is: 0xxxxxx3f8, it means all the PLL was locked
*/
ca_status_t aal_xgpon_serdes_reset(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t delay
)
{
    ca_status_t ret = CA_E_OK;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
#elif defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)

    GLB_FPGA_IO_CNTL0_t value;

    AAL_PON_REG_WRITE(0x1, GLB_FPGA_IO_CNTL0);  // reset Xilinx serdes
    ca_mdelay(delay);

    AAL_PON_REG_WRITE(0x2, GLB_FPGA_IO_CNTL0);  // resync rx elastic FIFO
    ca_mdelay(delay);

    AAL_PON_REG_WRITE(0x0, GLB_FPGA_IO_CNTL0);
    ca_mdelay(delay);

    value.bf.fpga_cntl = AAL_PON_REG_READ(GLB_FPGA_IO_STS0);
    if (0x3F8 == (value.wrd & 0x3FF))
        ret = CA_E_OK;  //all the PLL was locked
    else{
        AAL_XGPON_DBG_ACTFSM("GLB_FPGA_IO_STS0 = 0x%08x, PLL Not Locked!\n",value.wrd);
        ret = CA_E_ERROR;
    }

#endif

    return ret;
}

/*
 *@__xgpn_tuning_rollback_simulation
 * do simulation for tuning rollback in O8, O9
 */
static ca_status_t __xgpn_tuning_rollback_simulation(
    ca_device_id_t device_id,
    ca_uint32_t to4_ms)
{
    //aal_xgpon_dsfp_sync_ctrl_config_msk_t  sync_ctrl_mask;
    //aal_xgpon_dsfp_sync_ctrl_config_t      sync_ctrl;
    aal_xgpon_actp_status_t                act_status;
    aal_ploam_tuning_cfg_mask_t            tuning_cfg_mask;
    aal_ploam_tuning_cfg_t                 tuning_cfg;
    aal_ploam_tuning_sw_resp_mask_t        tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t             tunning_sw_resp;
    ca_uint32_t                            sfc0 = 0;
    ca_uint32_t                            sfc1 = 0;
    ca_uint64_t                            sfc = 0;
    ca_status_t                            ret = CA_E_OK;


    ca_printf("Start rollback simulate......\r\n");
    memset(&tuning_cfg,0,sizeof(tuning_cfg));
    memset(&tunning_sw_resp,0,sizeof(tunning_sw_resp));

    ret = aal_xgpon_actp_status_get(device_id, &act_status);
    CA_RET_VALUE_CHECK(ret);

    ca_printf("Check ONU Activation state ...\r\n");
    if (act_status.onu_state != AAL_XGPON_ACTP_STATE_O5
        && act_status.onu_state != AAL_XGPON_ACTP_STATE_O8) {
        ca_printf("ONU Activation state should be O5 or O8 when do simulate!\r\n");
        return CA_E_OK;
    }

    ca_printf("Change TO4, TO5 values to be %u ...\r\n", to4_ms * 8);
    ret = aal_xgpon_actp_to4_set(device_id,
        to4_ms * 8); // Set TO4
    CA_RET_VALUE_CHECK(ret);

    ret = aal_xgpon_actp_to5_set(device_id,
        to4_ms * 8); // Set TO5
    CA_RET_VALUE_CHECK(ret);

    ret = aal_xgpon_actp_to23_set(device_id,
        to4_ms * 100); // Set TO2 >> TO4
    CA_RET_VALUE_CHECK(ret);

    ret = aal_xgpon_actp_toz_set(device_id,
        to4_ms * 100); // Set TO2 >> TO4
    CA_RET_VALUE_CHECK(ret);

    ret = aal_xgpon_dsfp_super_frame_get(device_id, &sfc0, &sfc1);
    CA_RET_VALUE_CHECK(ret);
    sfc = sfc0 | ((ca_uint64)sfc1<<32);
    sfc += 4000; /* 500ms for SFC match */

    ca_printf("Set tuning config SFC 0x%08x ...\r\n", sfc & 0xFFFF);

    tuning_cfg_mask.wrd = ~0;
    tuning_cfg.request_schedule_sfc = (ca_uint16_t)(sfc & 0xFFFF);
    tuning_cfg.skip_rx = 0 ;
    tuning_cfg.rollback_allow = 1; // allow rollback
    tuning_cfg.request_lead_sfc = 2;
    ret = aal_ploam_tuning_cfg_set(device_id, tuning_cfg_mask, &tuning_cfg);
    CA_RET_VALUE_CHECK(ret);

    ca_printf("Simulate tuning  ...\r\n");
    tunning_sw_resp_mask.wrd = 0;
    tunning_sw_resp_mask.s.tuning_request = 1;
    tunning_sw_resp_mask.s.tuning_operation_code = 1;
    tunning_sw_resp_mask.s.tuning_response_code = 1;
    tunning_sw_resp.tuning_request = 1;
    tunning_sw_resp.tuning_operation_code = 0; /* ACK */
    tunning_sw_resp.tuning_response_code = 0;
    /* it will cause HW FSM go from O5.1 to O5.2 */
    ret = aal_ploam_tuning_sw_resp_set(device_id,tunning_sw_resp_mask,&tunning_sw_resp);
    CA_RET_VALUE_CHECK(ret);

    //aal_clear_actp_rx_tx_optical_tuning(1, 0);

    //ca_printf("Simuate channel switch  ...\r\n");

    /* fake tuning & channel switch, just for LOS old channel */
    aal_xgpn_simulation_flag = AAL_XGPON_SIMULATION_FLAG_TURNING_ROLLBACK;

    return CA_E_OK;
}

extern  ca_int32_t aal_xgpn_mac_debug;
extern  ca_int32_t aal_xgpn_simulation_flag;
extern  ca_int32_t aal_xgpn_protect_switch_sim_flag;
extern  ca_uint32_t aal_xgpn_auto_reset;

extern void aal_xgpon_datapath_reset(ca_device_id_t device_id);

ca_status_t aal_xgpon_sm_simulation(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t sfc0 = 0;
    ca_uint32_t sfc1 = 0;
    ca_uint64_t sfc = 0;
    aal_ploam_tuning_cfg_mask_t tuning_cfg_mask;
    aal_ploam_tuning_cfg_t      tuning_cfg;
    aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;

    //ca_printf("%s::%d::sfc0 is 0x%x!!\r\n",__FUNCTION__,__LINE__,sfc0);

    memset(&tuning_cfg,0,sizeof(tuning_cfg));
    memset(&tunning_sw_resp,0,sizeof(tunning_sw_resp));

    ret = aal_xgpon_dsfp_super_frame_get(device_id, &sfc0, &sfc1);
    CA_RET_VALUE_CHECK(ret);
    if(aal_xgpn_mac_debug)
    {
        ca_printf("%s::%d::sfc0 is 0x%x!!\r\n",__FUNCTION__,__LINE__,sfc0);
        ca_printf("%s::%d::sfc1 is 0x%x\r!!\n",__FUNCTION__,__LINE__,sfc1);
    }
    sfc = sfc0|((ca_uint64_t)sfc1<<32);
    if(aal_xgpn_mac_debug)
    {
        ca_printf("%s::%d::sfc is 0x%x!!\r\n",__FUNCTION__,__LINE__,sfc);
    }
    sfc += 24000;
    if(aal_xgpn_mac_debug)
    {
        ca_printf("%s::%d::sfc is 0x%x!!\r\n",__FUNCTION__,__LINE__,sfc);
    }
    sfc &=0xFFFF;
    if(aal_xgpn_mac_debug)
    {
        ca_printf("%s::%d::sfc is 0x%x!!\r\n",__FUNCTION__,__LINE__,sfc);
    }
    tuning_cfg_mask.wrd = ~0;
    tuning_cfg.request_schedule_sfc = (ca_uint16_t)sfc;
    //tuning_cfg.response_code = 0;
    //tuning_cfg.operation_code = 0;
    tuning_cfg.skip_rx = 0 ;
    tuning_cfg.rollback_allow = 0;
    tuning_cfg.request_lead_sfc = 2;
    ret = aal_ploam_tuning_cfg_set(device_id, tuning_cfg_mask, &tuning_cfg);
    CA_RET_VALUE_CHECK(ret);

    tunning_sw_resp_mask.wrd = 0;
    tunning_sw_resp_mask.s.tuning_request = 1;
    tunning_sw_resp_mask.s.tuning_operation_code = 1;
    tunning_sw_resp_mask.s.tuning_response_code = 1;
    tunning_sw_resp.tuning_request = 1;
    tunning_sw_resp.tuning_operation_code = 0;
    tunning_sw_resp.tuning_response_code = 0;
    ret = aal_ploam_tuning_sw_resp_set(device_id,tunning_sw_resp_mask,&tunning_sw_resp);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

ca_status_t aal_xgpon_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t temp = 0;

    switch (value1) {
        case 0:
            aal_xgpn_mac_debug = value2;
            ca_printf("Set xgpon mac debug : %d\r\n", aal_xgpn_mac_debug);
            break;
       case 1:
            //g_xgpn_debug = value2;
            break;
       case 2:
        #if 0
            // simulate what in NG-PON2, in case system profile/channel profile not received from OLT
            {
                ca_xgpn_sys_profile_t sys_profile;
                ca_xgpn_twdm_channel_profile_t channel_profile;
            
                // system profile first
                sys_profile.ng2sysid[0] = 0x5A;
                sys_profile.ng2sysid[1] = 0x5A;
                sys_profile.ng2sysid[2] = 0x5A;
            
                sys_profile.version = 0x10;
                sys_profile.twdm_channel_count = 0x1;
            
                ret = aal_ngpon2_ploam_sys_profile_handler(sys_profile.ng2sysid);
            
                // channel profile second
                channel_profile.channel_profile_idx = 0 |   // Channel profile index 0
                    CA_NGPON2_CHAN_PRF_US_VOID_IND_BIT | CA_NGPON2_CHAN_PRF_DS_VOID_IND_BIT | CA_NGPON2_CHAN_PRF_THIS_CHANNEL_IND_BIT;
                channel_profile.channel_profile_version = 0;
                channel_profile.pon_id = 0x00;    // DWLCH ID: 0
                channel_profile.ds_freq_offset = 0;     //0: zero offset
                channel_profile.ds_rate = CA_NGPON2_CHAN_PRF_DS_RATE_10G | CA_NGPON2_CHAN_PRF_DS_FEC_ON | CA_NGPON2_CHAN_PRF_DS_LINE_CODE;
                channel_profile.channel_partition = 0;
                channel_profile.default_response_channel = 0x00;
                channel_profile.uwlch_id = 0;
                channel_profile.optical_link_type = 0;
                channel_profile.us_rate = CA_NGPON2_CHAN_PRF_US_RATE_10G | CA_NGPON2_CHAN_PRF_US_RATE_2G;
                channel_profile.default_onu_attenuationt = 0;
                channel_profile.response_threshold = 10;
                ret = aal_ngpon2_ploam_channel_profile_handler(&channel_profile.channel_profile_idx);
            }
            #endif
            break;
            
        case 5:
            {
                ca_uint8_t data[4] = {0};
                ret = aal_xgpon_xfp_chanl_set(0, value2&0xFF, (value2&0xFF00)>>8);
                if (CA_E_OK == ret)
                {
                    //ca_printf("aal_xgpon_xfp_chanl_set done\r\n");
                    ret = cap_i2c_read(0, 0xA0, 0x70, 2, &(data[0]));
                    if (CA_E_OK == ret)
                    {
                        ca_printf("channel set done, read back: TX - %02d, RX - %02d\r\n", data[0], data[1]);
                    }
                }
                else
                {
                    ca_printf("channel set fail, return %d\r\n", ret);
                }
            }
            break;

        case 6: //transceiver channel control
            xfp_op_delay = value2;
            break;
        case 7:
            g_aal_xgpon_actfsm_debug = value2;
            break;
        case 8:
            if (value2 == 0) /* TO4 should not be 0*/
                __xgpn_tuning_rollback_simulation(device_id, 1);
            else
                __xgpn_tuning_rollback_simulation(device_id, value2);
            break;
        case 9:
            g_xgpon_work_around = value2;
            break;
        case 10:
            ret = aal_xgpn_cpi_timeout_set(0, value2);
            ca_printf("Set CPI timer return %d\n", ret);
            break;
        case 11:
            //g_xgpon_actfsm_log = value2;
            break;
        case 12:            
            switch (value2){
                case 4:
                    break;

                case 5:
                    ca_printf("Start NGPON2 activation state machine!!\r\n ");
                    ret = aal_xgpon_sm_simulation(0);
                    CA_RET_VALUE_CHECK(ret);
                    break;

                default:
                    break;
            }
            break;
            
        case 13:
            //aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(device_id, value2);
            break;

        case 14:
            //aal_xgpon_reset_puc(device_id);
            break;

        case 15:
            //aal_xgpon_clk_seq_reset(device_id);
            break;

        case 16:
            temp = aal_xgpn_auto_reset;
            aal_xgpn_auto_reset = 1;
            aal_xgpon_reset(device_id, value2);
            aal_xgpn_auto_reset = temp;
            break;
        case 17:
            aal_xgpn_auto_reset = value2;
            break;
        case 18:
            aal_xgpon_datapath_reset(device_id);
            break;
        case 19:
            ca_printf("Enable aal_xgpn_simulation_flag for activation sm simulation.\r\n");
            aal_xgpn_simulation_flag = value2;
            break;
        case 20:
            ca_printf("Enable aal_xgpn_protect_switch_sim_flag for activation sm simulation.\r\n");
            aal_xgpn_protect_switch_sim_flag = value2;
            break;

        default:
            ca_printf("Usage:\r\n ");
            ca_printf("\r\nparam1 for debug selection, param2 for debug config\r\n");
            ca_printf("param1 = 1 : set xgpon debug print on/off.\r\n");
            ca_printf("           : param2: \r\n");
            ca_printf("            - 0 : disable print msg\r\n");
            ca_printf("            - 1 : enable print msg\r\n");
            ca_printf("            - 2 : enable log msg\r\n");
            ca_printf("param1 = 2 : simualtion NGPON2 sys_profile, channel_profile config .\r\n");
            ca_printf("           : param2: \r\n");
            ca_printf("            - LSB 0-7bits : ds pon id\r\n");
            ca_printf("            - LSB 8-15bits: us pon id\r\n");
            ca_printf("param1 = 3 : simualtion NGPON2 protection handler .\r\n");
            ca_printf("           : param2: \r\n");
            ca_printf("            - LSB 0-7bits : ds pon id\r\n");
            ca_printf("            - LSB 8-15bits: us pon id\r\n");
            ca_printf("param1 = 4 : serdes reset, param2 for delay ms .\r\n");
            ca_printf("           : param2: delay ms when do serdes reset\r\n");
            ca_printf("param1 = 5 : XCVR channel switch, param2 16LSBs for chanel_up, channel_dn .\r\n");
            ca_printf("           : param2: \r\n");
            ca_printf("            - LSB 0-7bits : ds channel\r\n");
            ca_printf("            - LSB 8-15bits: us channel\r\n");
            ca_printf("param1 = 6 : XCVR I2C delay.\r\n");
            ca_printf("           : param2: delay ms when access XCVR with I2C\r\n");
            ca_printf("param1 = 7 : tuning debug set, param2 for debug flags.\r\n");
            ca_printf("           : param2: \r\n");
            ca_printf("            - bit0: enable tuning debug print\r\n");
            ca_printf("            - bit1: enable tuning zero SFC reject\r\n");
            ca_printf("            - bit2: enable tuning rollback check\r\n");
            ca_printf("            - bit3: enable tuning calibration check\r\n");
            ca_printf("            - bit4: enable reset clock sequence when tuning new channel\r\n");
            ca_printf("            - bit5: enable clear turning tx,rx optical when TO5 expire to O5\r\n");
            ca_printf("            - bit17: enable rollback to target channel but not original channel feature\r\n");
            ca_printf("            - bit18: enable channel scan feature\r\n");
            ca_printf("            - bit19: enable illegal ONU ID assign filter feature\r\n");
            ca_printf("            - bit20: enable illegal Alloc ID assign filter feature\r\n");
            ca_printf("param1 = 8 : simulation of tuning rollback param2 for TO4.\r\n");
            ca_printf("param1 = 9 : XGPON workaround enable flag.\r\n");
            ca_printf("param1 = 10 : CPI timer timeout value (unit:ms).\r\n");
            ca_printf("param1 = 11: Set value2 to aal_xgpn_mac_debug\r\n ");
            ca_printf("param1 = 12: 1. cortina_api test\r\n ");
            ca_printf("                2. p_gpon_cb dump\r\n ");
            ca_printf("                2. aal xgpon test\r\n ");
            ca_printf("                4. ngpon2 state machine simulation\r\n ");
            ca_printf("param1 = 13: reset clock and clear optical tuning \r\n ");
            ca_printf("param1 = 14: reset PUC \r\n ");
            ca_printf("param1 = 15: reset clock sequence \r\n ");
            ca_printf("param1 = 16: reset pon mac and PUC\r\n ");
            ca_printf("param1 = 17: reset datapath modules\r\n ");
            ca_printf("param1 = 18: configure the auto-reset PON MAC function when ONU de-active\r\n ");
            ca_printf("param1 = 19: Enable aal_xgpn_simulation_flag.\r\n ");
            ca_printf("param1 = 20: Enable aal_xgpn_protect_switch_sim_flag.\r\n ");

            break;
        }

    return ret;
}

ca_status_t aal_xgpon_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
)
{
    ca_status_t rc = CA_E_ERROR;
    ca_uint8_t xfp_reg;

    if (value1 == 0)
    {
        ca_printf("usage:\r\n ");
        ca_printf("param1 = 1 : show xgpon debug and actfsm managment info.\r\n");
        ca_printf("param1 = 2 : show simualtion NGPON2 sys_profile, channel_profile debug .\r\n");
        ca_printf("param1 = 3 : show simualtion NGPON2 protection debug .\r\n");
        ca_printf("param1 = 4 : show serdes resetdelay ms .\r\n");
        ca_printf("param1 = 5 : show XCVR channels: chanel_up, channel_dn .\r\n");
        ca_printf("param1 = 6 : show XCVR I2C delayms.\r\n");
        ca_printf("param1 = 7 : show tuning debug .\r\n");
        ca_printf("param1 = 10 : show CPI timer.\r\n");
        return CA_E_OK;
    }

    if (1 == value1)
    {
        *value2 = aal_xgpn_mac_debug;

        ca_printf("aal_xgpn_mac_debug %d\r\n", aal_xgpn_mac_debug);

        aal_xgpn_dbg_activation_dump();
    }
    if (2 == value1)
    {
        rc = cap_i2c_read(0, 0xA0, 0x70, 1, &xfp_reg);
        if (CA_E_OK == rc)
        {
            ca_printf("TX ID %d\r\n", xfp_reg);
        }
        else
        {
            ca_printf("TX get error 0x%x\r\n", rc);
        }
        ca_mdelay(xfp_op_delay);
        rc = cap_i2c_read(0, 0xA0, 0x71, 1, &xfp_reg);
        if (CA_E_OK == rc)
        {
            ca_printf("RX ID %d\r\n", xfp_reg);
        }
        else
        {
            ca_printf("RX get error 0x%x\r\n", rc);
        }
        ca_mdelay(xfp_op_delay);
        rc = cap_i2c_read(0, 0xA0, 110, 1, &xfp_reg);//General Control/Status Bits
        if (CA_E_OK == rc)
        {
            ca_printf("General Control/Status Bits 0x%x\r\n", xfp_reg);
        }
        else
        {
            ca_printf("General Control/Status Bits read error 0x%x\r\n", rc);
        }
#if 0
        {
            float ad = 0, offset = 0, measu = 0 ;
            unsigned char buf56[40]={0};
            int i,j;
            unsigned char  regAddr;
            unsigned short rxPowerad=0, txPowerad=0;
            ca_uint8_t i2c_addr = 0xa0;

            /*read Real Time Diagnostic Registers for A/D values*/
            i2c_addr = 0xa0;
            regAddr = 96;

            for (i = 0; i < 12; i++)
            {
                ca_mdelay(xfp_op_delay);
                rc = cap_i2c_read(0,i2c_addr,(regAddr+i),1, &buf56[i]);
                if (CA_E_OK == rc)
                {
                    ca_printf("%03d: 0x02x\r\n", (regAddr+i), buf56[i]);
                }
                else
                {
                    ca_printf("Addr %03d get error 0x%x\r\n", (regAddr+i), rc);
                }
                ca_mdelay(xfp_op_delay);
            }

            /*measure result show*/
            { // temperature
                ad = buf56[0] <<8 |buf56[1];
                measu = ad /256;
                if(measu <= 128 && measu >= -128  )
                {
                    ca_printf("Temperature=%f Centigrade \r\n",measu);
                }
                else
                {
                    ca_printf("error in getting temperature \r\n");
                }
            }

            { // supply voltage
                ad = buf56[2] <<8 |buf56[3];
                measu = ad * 0.0001;
                if(measu <= 6.55 && measu >= 0  )
                {
                    ca_printf("Voltage=%f V \r\n",measu);
                }
                else
                {
                    ca_printf("error in getting voltage \r\n");
                }
            }

            { // 10G Tx Bias Current. Unit: 4uA
                ad = buf56[4] <<8 |buf56[5];
                measu = ad * 4;
                //if(measu <= 132 && measu >= 0  )
                {
                    ca_printf("10G Tx Bias Current=%f uA \r\n",measu);
                }
                /*else
                {
                    ca_printf("error in getting current \r\n");
                } */
            }

            // 10G Tx output power. Unit: 0.1uW
            {
                txPowerad = buf56[6] <<8 |buf56[7];
                if(txPowerad != 0)
                {
                    measu = log10(txPowerad)*10-40;
                    ca_printf("10G Tx output power=%d*0.1uW/%fdBm \r\n",txPowerad, measu);
                }
                /*else
                {
                    ca_printf("error in getting outPower \r\n");
                } */
            }

            {//Rx input power. Unit :0.1uW
                rxPowerad=buf56[8]<<8|buf56[9];
                if(rxPowerad!=0)
                {
                    measu = log10(rxPowerad)*10-40;
                    ca_printf("Rx input power=%d*0.1uW/%fdBm\r\n",rxPowerad, measu);
                }
                /*else
                {
                    ca_printf("error in getting rxPower \r\n");
                } */
            }
        }
#endif
    }
    if (3 == value1)
    {
        ca_uint8_t data[132] = {0,};
        ca_uint8_t i;

        rc = cap_i2c_read(0, 0xA0, 128, 128, &(data[0]));
        if (CA_E_OK == rc)
        {
            for (i = 0; i < 128; i++)
            {
                if (0 == (i % 16))
                    ca_printf("\r\n%02x:", i);
                ca_printf(" %02x ", data[i]);
            }
            ca_printf("\r\n");
        }
        else
        {
            ca_printf("Transceiver get error 0x%x\r\n", rc);
        }
        ca_mdelay(xfp_op_delay);
    }
    if (4 == value1)
    {
        ca_uint8_t para[16] = {0,};
        ca_uint8_t i;

        rc = cap_i2c_read(0, 0xA0, 0x60, 14, &(para[0]));
        if (CA_E_OK == rc)
        {
            ca_printf("Temp:");
            for (i = 0; i < 2; i++)
            {
                ca_printf(" %02x ", para[i]);
            }
            ca_printf("\r\n");

            ca_printf("10G Bias:");
            for (i = 4; i < 6; i++)
            {
                ca_printf(" %02x ", para[i]);
            }
            ca_printf("\r\n");

            ca_printf("10G Tx-Power:");
            for (i = 6; i < 8; i++)
            {
                ca_printf(" %02x ", para[i]);
            }
            ca_printf("\r\n");

            ca_printf("Rx-Power:");
            for (i = 8; i < 10; i++)
            {
                ca_printf(" %02x ", para[i]);
            }
            ca_printf("\r\n");

            ca_printf("VCC/Auxiliary measurement:");
            for (i = 10; i < 14; i++)
            {
                ca_printf(" %02x ", para[i]);
            }
            ca_printf("\r\n");
        }
        else
        {
            ca_printf("XFP Measure Value get error 0x%x\r\n", rc);
        }
        ca_mdelay(xfp_op_delay);
    }

    if (7 == value1)
    {
        ca_printf("tuning_debug = 0x%x \r\n", g_aal_xgpon_actfsm_debug);

        ca_printf("tuning debug print: %d\r\n",
            (XGPON_TUNING_DEBUG_PRINT & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("tuning zero SFC reject: %d\r\n",
            (XGPON_TUNING_DEBUG_SFC_DROP_SFC0 & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("tuning rollback check: %d\r\n",
            (XGPON_TUNING_DEBUG_ROLLBACK_FLAG_CHK & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("tuning calibration check: %d\r\n",
            (XGPON_TUNING_DEBUG_CALIBRATION_FLAG_CHK & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("tuning reset clock sequences when switch channel: %d\r\n",
            (XGPON_TUNING_DEBUG_RESET_CLK_FOR_CHN & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("tuning restore to O5 with clear tx,rx opticl: %d\r\n",
            (XGPON_TUNING_DEBUG_RESTORE_CLR_TUNING & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("fore clear tx,rx opticl always when state change: %d\r\n",
            (XGPON_TUNING_DEBUG_CLR_RX_TX_TUNING_FORCE & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("rollback to target channel but not original channel: %d\r\n",
            (AAL_XGPON_TUNING_DEBUG_ROLLBACK_TO_TARGET_CHN & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("channel scan enable: %d\r\n",
            (AAL_XGPON_ACTFSM_DEBUG_CHN_SCAN_ENA & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("illegal ONU ID filter enable: %d\r\n",
            (AAL_XGPON_ACTFSM_DEBUG_ONU_ID_CHK_ENA & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        ca_printf("illegal Alloc ID filter enable: %d\r\n",
            (AAL_XGPON_ACTFSM_DEBUG_ALLOC_ID_CHK_ENA & g_aal_xgpon_actfsm_debug) ? 1 : 0);

        *value2 = g_aal_xgpon_actfsm_debug;
    }

    if (value1 == 10)
    {
        ca_uint32_t  time_out = 0;

        aal_xgpn_cpi_timeout_get(0, &time_out);
        ca_printf("Get CPI timer timeout : %d ms\n", time_out);
    }

#if 0
    if (value1 == 11)
    {
        ca_printf("XGPON ACTFSM Log enable = %d\n", g_xgpon_actfsm_log);
    }
#endif

    return CA_E_OK;
}

ca_status_t aal_xgpon_cb_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index)
{

    return CA_E_OK;
}

ca_status_t aal_xgpon_cb_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  channel_partition_index)
{
    __AAL_XGPN_ACT_LOCK(0);

    /* Init g_ngpon2_channel_mgmt_cb */
    g_ngpon2_channel_mgmt_cb.profile_record[0].learnt_flag = FALSE;
    memset(&g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].system_profile, 0, sizeof(ca_xgpn_sys_profile_t));
    memset(&g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].twdm_channel_profile, 0, CA_XGPON_MAX_CHANNEL_PROFILE_COUNT * sizeof(ca_xgpn_twdm_channel_profile_t));
    memset(&g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].burst_profile, 0, CA_XGPON_MAX_BURST_PROFILE_COUNT * sizeof(ca_xgpn_burst_profile_t));
    g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].channel_profile_count_learnt = 0;
    g_ngpon2_channel_mgmt_cb.profile_record[0/*dwlch_id*/].profile_evaluate_request = 0;

    memset(&g_ngpon2_channel_mgmt_cb.calibration_request, 0, sizeof(ca_ngpon2_calibration_request_t));

    memset(&g_ngpon2_channel_mgmt_cb.tuning_control, 0, sizeof(ca_ngpon2_tuning_control_t));

    g_ngpon2_channel_mgmt_cb.protection_control.protection_ds_pon_id = 0xFFFF;
    g_ngpon2_channel_mgmt_cb.protection_control.protection_us_pon_id = 0xFFFF;
    g_ngpon2_channel_mgmt_cb.protection_control.protection_ctrl_flag = 0;

    g_ngpon2_channel_mgmt_cb.channel_configured.us_pon_id = 0xFFFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.ds_pon_id = 0xFFFF;
    g_ngpon2_channel_mgmt_cb.channel_configured.pre_running_configured_count = 0;

    memset(&g_ngpon2_channel_mgmt_cb.transceiver_cap, 0, sizeof(ca_ngpon2_transceiver_t));
    g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index = 0xFF;

    g_ngpon2_channel_mgmt_cb.calibration_record.channel_partition_index = channel_partition_index;

    __AAL_XGPN_ACT_UNLOCK(0);

    return CA_E_OK;
}

ca_status_t aal_xgpon_sysprof_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_count)
{
    if (NULL == channel_count)
        return CA_E_PARAM;

    __AAL_XGPN_ACT_LOCK(0);
    *channel_count = g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.twdm_channel_count;
    __AAL_XGPN_ACT_UNLOCK(0);

    return CA_E_OK;
}

ca_status_t aal_xgpon_sysprof_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  channel_count/*,
    CA_IN ca_uint8_t  channel_spacing,
    CA_IN ca_uint8_t  us_mse,
    CA_IN ca_uint16_t  fsr_twdm,
    CA_IN ca_uint16_t  loose_bound*/)
{
    __AAL_XGPN_ACT_LOCK(0);

    /* restore parameters from scfg and update g_ngpon2_channel_mgmt_cb*/
    // Update from input temporarily as no file supported now
    g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.twdm_channel_count =  channel_count;
#if 0
    g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.channel_spacing_twdm = channel_spacing;
    g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.us_mse_twdm = us_mse;
    g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.fsr_twdm = fsr_twdm;
    g_ngpon2_channel_mgmt_cb.profile_record[0].system_profile.channel_loose_calibratiob_bound_twdm = loose_bound;
#endif
    __AAL_XGPN_ACT_UNLOCK(0);

    return CA_E_OK;
}

ca_status_t aal_xgpon_channelprof_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index)
{
    return CA_E_OK;
}
ca_status_t aal_xgpon_channelprof_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  index,
    CA_IN ca_uint32_t  learnt_flag)
{
    __AAL_XGPN_ACT_LOCK(0);

    /* restore parameters from scfg and update g_ngpon2_channel_mgmt_cb*/
    // Update from input temporarily as no file supported now
    g_ngpon2_channel_mgmt_cb.profile_record[0].twdm_channel_profile[index].learnt_flag = learnt_flag;

    __AAL_XGPN_ACT_UNLOCK(0);

    return CA_E_OK;
}

ca_status_t aal_xgpon_xfp_cap1_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT ca_uint8_t  *channel_partition_index)
{
    return CA_E_OK;
}

ca_status_t aal_xgpon_xfp_cap1_set(
    CA_IN ca_device_id_t                 device_id,
/*    CA_IN ca_uint16_t  laser_first_frequency_1,
    CA_IN ca_uint16_t  laser_first_frequency_2,
    CA_IN ca_uint16_t  laser_last_frequency_1,
    CA_IN ca_uint16_t  laser_last_frequency_2,
    CA_IN ca_uint16_t  laser_grid,*/
    CA_IN ca_uint8_t  upstream_optical_link_type,
    CA_IN ca_uint8_t  upstream_line_rate,
    CA_IN ca_uint8_t  dwlch_config_index)
{
    __AAL_XGPN_ACT_LOCK(0);

    /* restore parameters from scfg and update g_ngpon2_channel_mgmt_cb*/
    // Update from input temporarily as no file supported now
#if 0
    g_ngpon2_channel_mgmt_cb.transceiver_cap.laser_first_frequency_1 = laser_first_frequency_1;    /*Unit: THZ*/
    g_ngpon2_channel_mgmt_cb.transceiver_cap.laser_first_frequency_2 = laser_first_frequency_2;    /*Unit: 0.1GHZ*/
    g_ngpon2_channel_mgmt_cb.transceiver_cap.laser_last_frequency_1 = laser_last_frequency_1;    /*Unit: THZ*/
    g_ngpon2_channel_mgmt_cb.transceiver_cap.laser_last_frequency_2 = laser_last_frequency_2;    /*Unit: 0.1GHZ*/
    g_ngpon2_channel_mgmt_cb.transceiver_cap.laser_grid = laser_grid;    /*maximum supported grid spacing, Unit: 0.1GHZ*/
#endif
    g_ngpon2_channel_mgmt_cb.transceiver_cap.upstream_optical_link_type = upstream_optical_link_type;              /* 2 bits, Link type unspecified, A, B or Both     */
    g_ngpon2_channel_mgmt_cb.transceiver_cap.upstream_line_rate = upstream_line_rate;              /*  10G, 2.5G     */
    g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index = dwlch_config_index;              /*  10G, 2.5G     */

    __AAL_XGPN_ACT_UNLOCK(0);

    return CA_E_OK;
}

ca_status_t aal_xgpon_xfp_cap2_get(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_uint8_t    index,
    CA_OUT ca_uint8_t  *dn_support,
    CA_OUT ca_uint8_t  *up_support,
    CA_OUT ca_uint8_t  *dwlch_calibration_record)
{
    CA_NULL_PTR_CHECK(dn_support);
    CA_NULL_PTR_CHECK(up_support);
    CA_NULL_PTR_CHECK(dwlch_calibration_record);

    if (index >= CA_NGPON2_MAX_DWLCH) {
        __AAL_XGPN_ERROR("index %d is too big.\r\n", index);
        return CA_E_PARAM;
    }

    *dn_support = g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_dn[index];
    *up_support = g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_up[index];
    *dwlch_calibration_record = g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_calibration_record[index];

    return CA_E_OK;
}

ca_status_t aal_xgpon_xfp_cap2_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t  index,
    CA_IN ca_uint8_t  dn_support,
    CA_IN ca_uint8_t  up_support,
    CA_IN ca_uint8_t  dwlch_calibration_record)
{
    __AAL_XGPN_ACT_LOCK(0);

    /* restore parameters from scfg and update g_ngpon2_channel_mgmt_cb*/
    // Update from input temporarily as no file supported now
    g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_dn[index] = dn_support;
    g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_up[index] = up_support;
    g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_calibration_record[index] = dwlch_calibration_record;

    __AAL_XGPN_ACT_UNLOCK(0);

    return CA_E_OK;
}

ca_status_t aal_xgpon_xfp_chanl_get(
    CA_IN ca_device_id_t   device_id,
    CA_OUT ca_uint32_t  *ds_pon_id,
    CA_OUT ca_uint32_t  *us_pon_id)

{
    ca_status_t rc = CA_E_OK;
    ca_uint8_t  xfp_reg= 0;
    ca_uint8_t  pswd[] = {0x12, 0x34, 0x56, 0x78};

    CA_NULL_PTR_CHECK(ds_pon_id);
    CA_NULL_PTR_CHECK(us_pon_id);

    rc = cap_i2c_write(0, 0xA0, 0x7B, 4, pswd);
    if (CA_E_OK != rc)
    {
        ca_printf("cap_i2c_write 0x7B error: %d\r\n", rc);
        return  rc;
    }
    ca_mdelay(xfp_op_delay);

    rc = cap_i2c_read(0, 0xA0, 0x70, 1, &xfp_reg);
    if (CA_E_OK != rc)
    {
        ca_printf("TX get error 0x%x\r\n", rc);
        return rc;
    }

    *us_pon_id = xfp_reg;

    ca_mdelay(xfp_op_delay);

    rc = cap_i2c_read(0, 0xA0, 0x71, 1, &xfp_reg);
    if (CA_E_OK != rc)
    {
        ca_printf("RX get error 0x%x\r\n", rc);
        return rc;
    }

    *ds_pon_id = xfp_reg;

    return CA_E_OK;

}

ca_status_t aal_xgpon_xfp_chanl_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  ds_pon_id,
    CA_IN ca_uint32_t  us_pon_id)
{
    ca_status_t rc = CA_E_OK;
#if 1
    ca_uint8_t pswd[5] = {0x12, 0x34, 0x56, 0x78};
    ca_uint8_t channel[] = {0x0, 0x1, 0x2, 0x3};
#endif
    ca_uint8_t ds_chn = 0; //ds_pon_id & ;
    ca_uint8_t us_chn = 0;
    ca_uint8_t pre_ds_chn = __xgpn_act_fsm_get_previous_ds_chn(AAL_DEFAUL_DEV_ID);
    ca_uint8_t pre_us_chn = __xgpn_act_fsm_get_previous_us_chn(AAL_DEFAUL_DEV_ID);

    ds_chn = ds_pon_id % __GET_XCVR_CHN_CAPABILITY();
    us_chn = us_pon_id % __GET_XCVR_CHN_CAPABILITY();

    //__AAL_XGPN_ACT_LOCK(0);

    ca_printf("%s::%d::Channel set para:target_ds_pon_id 0x%x target_us_pon_id 0x%x.\r\n",
        __FUNCTION__,__LINE__, ds_pon_id, us_pon_id);
    #if 0
    //check parameters in structure transceiver_cap;
    if ((ds_pon_id & 0xFF) < 0xF)   // input id valid
    {
        index = ds_pon_id & 0x3;
        if (g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_dn[index])//transceiver support this DCHID
            g_ngpon2_channel_mgmt_cb.transceiver_cap.dwlch_config_index = index;
        else
        {
            __AAL_XGPN_ERROR("transceiver_cap error, please confirm input channel index.\r\n");
            goto ret;
        }
    }
    if ((us_pon_id & 0xFF) < 0xF)   // input id valid
    {
        index = us_pon_id & 0x3;
        if (g_ngpon2_channel_mgmt_cb.transceiver_cap.channel_partition_up[index])//transceiver support this DCHID
            g_ngpon2_channel_mgmt_cb.transceiver_cap.uwlch_config_index = index;
        else
        {
            __AAL_XGPN_ERROR("transceiver_cap error, please confirm input channel index.\r\n");
            goto ret;
        }
    }
    #endif

    /*1Write the passwords 0x12 0x34 0x56 0x78 into the bytes 0x7B 0x7C 0x7D 0x7E on the 2-Wire Serial Address 1010000X (A0H) .
    2After the first step :
    write 0x00 or 0x01 or 0x02 or 0x03 into the byte 0x70 on 1010000X (A0H) to choose the TX channel.
    wirte 0x00 or 0x01 or 0x02 or 0x03 into the byte 0x71 on 1010000X (A0H) to choose the RX channel.
    3Passwords should be written into again as long as the module got power-off in any condition.    */

    rc = cap_i2c_write(0, 0xA0, 0x7B, 4, pswd);
    if (CA_E_OK != rc)
    {
        ca_printf("cap_i2c_write 0x7B error: %d\r\n", rc);
        goto ret;
    }
    ca_mdelay(xfp_op_delay);


   // if ((us_pon_id & 0xFF) <= 0xFF)    // valid, config TX
    //{
        //index = us_pon_id & 0x3;
        rc = cap_i2c_write(0, 0xA0, 0x70, 1, &(channel[us_chn]));
        if (CA_E_OK != rc)
        {
            ca_printf("cap_i2c_write 0x70(TX) error: %d\r\n", rc);
            goto ret;
        }
        ca_mdelay(xfp_op_delay);
    //}

    //if ((ds_pon_id & 0xFF) <= 0xFF)    // valid, config RX
    //{
        //index = ds_pon_id & 0x3;
        rc = cap_i2c_write(0, 0xA0, 0x71, 1, &(channel[ds_chn]));
        if (CA_E_OK != rc)
        {
            ca_printf("cap_i2c_write 0x71(RX) error: %d\r\n", rc);
            goto ret;
        }
        ca_mdelay(xfp_op_delay);
    //}


    rc = cap_i2c_write(0, 0xA0, 0x7B, 4, pswd);
    if (CA_E_OK != rc)
    {
        ca_printf("cap_i2c_write 0x7B error: %d\r\n", rc);
        goto ret;
    }
    ca_mdelay(xfp_op_delay);

#if 1 // TODO: check the clk seq reset need ONLY when change ds channel or NOT with ASIC
    if (ds_chn != g_original_ds_chn ||
        us_chn != g_original_us_chn) {
        /* Reset XCVR,SERDES clocks after change channel */
        if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_RESET_CLK_FOR_CHN) {
            //aal_xgpon_clk_seq_reset(0);
            aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(AAL_DEFAUL_DEV_ID, 10);
        }
    }

    if (us_chn != __xgpn_act_fsm_get_current_us_chn(AAL_DEFAUL_DEV_ID)) {
        pre_us_chn = __xgpn_act_fsm_get_current_us_chn(AAL_DEFAUL_DEV_ID);
    }

    if (ds_chn != __xgpn_act_fsm_get_current_ds_chn(AAL_DEFAUL_DEV_ID)) {
        pre_ds_chn = __xgpn_act_fsm_get_current_ds_chn(AAL_DEFAUL_DEV_ID);
    }

    /*
     * Update channel management status after channel switch
     */
    __xgpn_act_fsm_set_previous_chn(AAL_DEFAUL_DEV_ID, pre_us_chn, pre_ds_chn);

    __xgpn_act_fsm_set_current_chn(AAL_DEFAUL_DEV_ID, us_chn, ds_chn);

#else
    if (ds_pon_id != g_original_ds_chn) {
        /* Reset XCVR,SERDES clocks after change channel */
        if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_RESET_CLK_FOR_CHN) {
            //aal_xgpon_clk_seq_reset(0);
            aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(AAL_DEFAUL_DEV_ID, 10);
        }
    }

    aal_clear_actp_rx_tx_optical_tuning(TRUE,TRUE);
#endif

ret:
    //__AAL_XGPN_ACT_UNLOCK(0);

    return rc;
}

void aal_xgpn_act_fsm_state_reset(ca_device_id_t device_id)
{
    if (aal_pon_mac_mode_get(device_id) >= ONU_PON_MAC_XGSPON)
        g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1_1;
    else
        g_xgpn_act_state = AAL_XGPN_ACT_STATE_O1;

    g_xgpn_act_fsm_pre_state = g_xgpn_act_state;
    g_xgpn_act_fsm_cur_state = g_xgpn_act_state;

}

void aal_xgpn_act_fsm_restore_to_o5(ca_device_id_t device_id)
{
    if (aal_pon_mac_mode_get(device_id) >= ONU_PON_MAC_XGSPON)
        g_xgpn_act_state = AAL_XGPN_ACT_STATE_O5_1;
    else
        g_xgpn_act_state = AAL_XGPN_ACT_STATE_O5;

    if (g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_RESET_CLK_FOR_CHN) {
        aal_clear_actp_rx_tx_optical_tuning(TRUE,TRUE);
    }

}


void aal_clear_actp_rx_tx_optical_tuning
(
    CA_IN ca_uint8 clear_rx,
    CA_IN ca_uint8 clear_tx
)
{
    ca_status_t rc = CA_E_ERROR;
    aal_xgpon_actp_status_mask_t xgpn_actp_status_mask;
    aal_xgpon_actp_status_t xgpn_actp_status;
    ca_uint8 flag_to_check = 0;
    ca_uint8 count = 0;

    if(!clear_rx && !clear_tx)
        return;

    memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
    rc = aal_xgpon_actp_status_get(0,&xgpn_actp_status);
    if(CA_E_OK != rc) {
        __AAL_XGPN_ERROR("aal_xgpon_actp_status_get failed rc 0x%x\r\n", rc);
        return;
    }

    if(clear_rx)
        flag_to_check = xgpn_actp_status.rx_optical_tuning;
    if(clear_tx)
        flag_to_check = xgpn_actp_status.tx_optical_tuning;
    if(clear_tx && clear_rx)
        flag_to_check = xgpn_actp_status.rx_optical_tuning || xgpn_actp_status.tx_optical_tuning;
    while(flag_to_check) {
        memset(&xgpn_actp_status_mask, 0, sizeof(aal_xgpon_actp_status_mask_t));
        memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
        if(clear_rx) {
                xgpn_actp_status_mask.s.rx_optical_tuning = TRUE;
                xgpn_actp_status.rx_optical_tuning = FALSE;   // 1'b0: ONU resume DS synchronization
                __AAL_XGPN_ERROR("clear rx optical tuning flag\r\n");
        }
        if(clear_tx) {
                xgpn_actp_status_mask.s.tx_optical_tuning = TRUE;
                xgpn_actp_status.tx_optical_tuning = FALSE;
                __AAL_XGPN_ERROR("clear tx optical tuning flag\r\n");
        }
        rc = aal_xgpon_actp_status_set(0, xgpn_actp_status_mask, &xgpn_actp_status);
        memset(&xgpn_actp_status, 0, sizeof(aal_xgpon_actp_status_t));
        rc = aal_xgpon_actp_status_get(0,&xgpn_actp_status);
        if(CA_E_OK != rc) {
            __AAL_XGPN_ERROR("aal_xgpon_actp_status_get failed rc 0x%x\r\n", rc);
            break;
        }
        if(clear_rx)
            flag_to_check = xgpn_actp_status.rx_optical_tuning;
        if(clear_tx)
            flag_to_check = xgpn_actp_status.tx_optical_tuning;
        if(clear_tx && clear_rx)
            flag_to_check = xgpn_actp_status.rx_optical_tuning || xgpn_actp_status.tx_optical_tuning;
        if(count++ > 100) {
            __AAL_XGPN_ERROR("failed to clear rx/tx optical tuning %d\r\n", __LINE__);
            break;
        }
    }
    AAL_XGPON_DBG_ACTFSM("rx/tx optical tuning cleared\r\n");
}

void   aal_xgpn_act_fsm_state_sync_hw(ca_device_id_t device_id)
{
    if (g_xgpn_act_state != g_xgpn_act_fsm_cur_state) {

        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
        __AAL_XGPN_ACT_LOCK(0);
        g_xgpn_act_state = g_xgpn_act_fsm_cur_state;
        __AAL_XGPN_ACT_UNLOCK(0);
        AAL_XGPON_DBG_ACTFSM("SW ACT FSM is out of sync with HW, now force sync !\r\n");
    }
}

ca_status_t aal_xgpn_cpi_timeout_set(
    ca_device_id_t  device_id,
    ca_uint32_t  time_out)
{
    if (time_out == 0)
        return CA_E_PARAM;

    g_ngpon2_cpi_timeout = time_out;

    return CA_E_OK;

}

ca_status_t aal_xgpn_cpi_timeout_get(
    ca_device_id_t  device_id,
    ca_uint32_t  *time_out)
{
    CA_NULL_PTR_CHECK(time_out);

    *time_out = g_ngpon2_cpi_timeout;

    return CA_E_OK;
}


ca_status_t aal_xgpn_current_chan_get(
    ca_device_id_t   device_id,
    ca_uint32_t  *up_chn,
    ca_uint32_t  *dn_chn)
{
    CA_NULL_PTR_CHECK(up_chn);
    CA_NULL_PTR_CHECK(dn_chn);

    *up_chn = g_original_us_chn;
    *dn_chn = g_original_ds_chn;
    return CA_E_OK;

}

ca_status_t aal_xgpn_chn_info_get(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint8_t    chn_id,
    CA_OUT aal_xgpon_twdm_chn_info_t *chn_info)
{
    CA_ASSERT_RET(chn_id < AAL_XGPON_WAVE_CHANNEL_MAX_NUM, CA_E_PARAM);
    CA_ASSERT_RET(chn_info != NULL, CA_E_PARAM);

    memcpy(chn_info, &g_aal_ngp2_twdm_chn_cb.chn_info[chn_id],
        sizeof(aal_xgpon_twdm_chn_info_t));

    return CA_E_OK;
}


ca_status_t aal_xgpn_chn_info_set(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint8_t    chn_id,
    CA_IN  aal_xgpon_twdm_chn_info_t *chn_info)
{
    CA_ASSERT_RET(chn_id < AAL_XGPON_WAVE_CHANNEL_MAX_NUM, CA_E_PARAM);
    CA_ASSERT_RET(chn_info != NULL, CA_E_PARAM);

    // TODO: should NOT export channel set
    memcpy(&g_aal_ngp2_twdm_chn_cb.chn_info[chn_id], chn_info,
        sizeof(aal_xgpon_twdm_chn_info_t));

    return CA_E_OK;
}


#ifdef __KERNEL__
EXPORT_SYMBOL(aal_xgpn_chn_info_get);
EXPORT_SYMBOL(aal_xgpn_chn_info_set);
EXPORT_SYMBOL(aal_xgpn_cpi_timeout_set);
EXPORT_SYMBOL(aal_xgpn_cpi_timeout_get);
EXPORT_SYMBOL(aal_xgpn_current_chan_get);

EXPORT_SYMBOL(aal_xgpon_debug_set);
EXPORT_SYMBOL(aal_xgpon_debug_get);
EXPORT_SYMBOL(aal_xgpon_cb_get);
EXPORT_SYMBOL(aal_xgpon_cb_set);
EXPORT_SYMBOL(aal_xgpon_sysprof_get);
EXPORT_SYMBOL(aal_xgpon_sysprof_set);
EXPORT_SYMBOL(aal_xgpon_channelprof_get);
EXPORT_SYMBOL(aal_xgpon_channelprof_set);
EXPORT_SYMBOL(aal_xgpon_xfp_cap1_get);
EXPORT_SYMBOL(aal_xgpon_xfp_cap1_set);
EXPORT_SYMBOL(aal_xgpon_xfp_cap2_get);
EXPORT_SYMBOL(aal_xgpon_xfp_cap2_set);
EXPORT_SYMBOL(aal_xgpon_xfp_chanl_get);
EXPORT_SYMBOL(aal_xgpon_xfp_chanl_set);
EXPORT_SYMBOL(aal_xgpon_act_fsm_dbg_tunning_set);
EXPORT_SYMBOL(aal_xgpn_act_fsm_state_reset);
EXPORT_SYMBOL(aal_xgpn_act_fsm_restore_to_o5);
EXPORT_SYMBOL(aal_xgpn_activation_fsm_init);
EXPORT_SYMBOL(aal_xgpon_activation_state_get);

#endif

