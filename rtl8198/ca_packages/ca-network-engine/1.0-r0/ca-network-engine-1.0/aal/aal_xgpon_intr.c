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
/*
 * File   : aal_xgpon_intr.c
 * Brief  : XG(S)PON Interrupt config and handler AAL driver
 * History:
 *          2016-07-15 initial @Lidong
 *          2016-07-25 change __xgpon_intr_tbl to add name, ena_bmp
 *          2017-04-26 add XGPN_USBG_INTERRUPTE0 init and handler
 *
 */

#include <soc/cortina/registers.h>
#include "osal_common.h"
#include <linux/types.h>
#include <linux/resource.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/init.h>
#include <linux/kfifo.h>

#include "aal_pon.h"
#include "aal_xgpon_intr.h"
#include "ploam_ngpon2.h"
#include "xgpon_aes.h"
#include "ca-scfg/scfg.h"


extern pon_wq_t    pon_misc_wq;
extern pon_wq_t    pon_ploam_wq;
extern pon_wq_t    pon_ploam_key_ctrl_wq;
extern pon_wq_t    pon_ploam_key_tk4_wq;
extern pon_wq_t    pon_ploam_key_tk5_wq;
extern pon_wq_t    pon_ploam_rgst_wq;
extern pon_wq_t    pon_act_wq;
extern ca_uint32_t ploam_use_tasklet ;
extern ca_uint32_t act_use_tasklet   ;
extern ca_uint32_t aes_ctrl_use_tasklet   ;
extern ca_uint32_t aes_tk4_use_tasklet   ;
extern ca_uint32_t aes_tk5_use_tasklet   ;
extern ca_uint32_t ploam_rgst_use_tasklet ;
extern ca_uint32_t g_pon_irqs[32]    ;
extern ca_status_t aal_xgpn_act_intr_handler(
    ca_uint32_t xgpn_actp_int,
    ca_uint32_t xgpn_actp_st_chg_int,
    ca_uint32_t xgpn_actp_sts,
    void *cookie);
extern void xgpon_ploam_rx_handler(
    ca_uint32_t intr, 
    ca_uint32_t sts, 
    ca_uint32_t len, 
    void *pkt);
extern void xgpon_aes_key_ctrl_handler(
    ca_uint32_t xgpn_ploam_int,
    ca_uint32_t xgpn_ploam_sts,
    ca_uint32_t key_ctrl_stat,
    void *cookie);
extern void xgpon_aes_key_tk4_handler(
    ca_uint32_t xgpn_ploam_int,
    ca_uint32_t xgpn_ploam_sts,
    ca_uint32_t key_ctrl_stat,
    void *cookie);
extern void xgpon_aes_key_tk5_handler(
    ca_uint32_t xgpn_ploam_int,
    ca_uint32_t xgpn_ploam_sts,
    ca_uint32_t key_ctrl_stat,
    void *cookie);
#if 0
extern void xgpon_ploam_rgst_handler(
    ca_uint32_t xgpn_ploam_int,
    ca_uint32_t xgpn_ploam_sts,
    ca_uint32_t stat,
    void *cookie);
#endif
extern ca_uint8_t olt_type[CFG_ID_PON_OLT_TYPE_LEN];


#define AAL_PON_KMSG_FIFO_SIZE  4800
#define AAL_PON_CHK_KMSG_CHN(chn)   do { \
                                        if((chn) >= AAL_PON_KMSG_CHN_NUM) { \
                                            ca_printf("%s:[%d] chn is %d!!\n", __func__, __LINE__, (chn)); \
                                            return CA_E_PARAM; \
                                        } \
                                    } while(0)

#define AAL_PON_CHK_KMSG_LEN(len)  do { \
                                        if((len) > AAL_PON_KMSG_FIFO_SIZE) { \
                                            ca_printf("%s:[%d] msg-len is %d!!\n", __func__, __LINE__, (len)); \
                                            return CA_E_PARAM; \
                                        } \
                                    } while(0)

#define XGPON_GLB_INTR_MASK_DSFP            (0x0001 << 0)
#define XGPON_GLB_INTR_MASK_USBG            (0x0001 << 1)
#define XGPON_GLB_INTR_MASK_PLMP            (0x0001 << 2)
#define XGPON_GLB_INTR_MASK_BWMP            (0x0001 << 3)
#define XGPON_GLB_INTR_MASK_ACTP            (0x0001 << 4)
#define XGPON_GLB_INTR_MASK_AES             (0x0001 << 5)
#define XGPON_GLB_INTR_MASK_AES_PLMP        (0x0001 << 6)
#define XGPON_GLB_INTR_MASK_XGFEC           (0x0001 << 7)
#define XGPON_GLB_INTR_MASK_RXCTL           (0x0001 << 8)
//#define XGPN_ACTP_ST_CHG_INT_MASK_LOS_SYNC  (0x0001 << 6)

#define XGPN_PLMP_INTR_MASK_RXBUF1_RDY       (0x0001 << 31)
#define XGPN_PLMP_INTR_MASK_RXBUF0_RDY       (0x0001 << 30)

#if 0
#define XGPN_PLMP_INTR_MASK_KEY_CTRL         (0x0001 << 7)
#define XGPN_PLMP_INTR_MASK_TK5_TIMEOUT      (0x0001 << 6)
#define XGPN_PLMP_INTR_MASK_TK4_TIMEOUT      (0x0001 << 5)
#endif

//#define HOST_GPIO_INTR_BIT            (0x4)   //GPIO2  for Saturn HGU FPGA board only

/*******************************************************************/
/*         Local Macros and Variables                              */
/*******************************************************************/
/* TOP XGPON interrupt */
#define XGPON_INTR_TOP_ENA_DEFAULT     0x16 // ACTP, PLOAM,USBG

/* XGPON ACTP interrupt enable flags */
#define XGPON_INTR_ACTP_ONU_STATE_CHANGE_EN    (1 << AAL_XGPON_INTR_ACTP_ONU_STATE_CHANGE_BIT )
#define XGPON_INTR_ACTP_HUNT_SYNC_EXPIRE_EN    (1 << AAL_XGPON_INTR_ACTP_HUNT_SYNC_EXPIRE_BIT )
#define XGPON_INTR_ACTP_PROFILE_LEARNING_EN    (1 << AAL_XGPON_INTR_ACTP_PROFILE_LEARNING_BIT )
#define XGPON_INTR_ACTP_TX_RESPONSE_CROSS_EN   (1 << AAL_XGPON_INTR_ACTP_TX_RESPONSE_CROSS_BIT)
#define XGPON_INTR_ACTP_REGST_EN               (1 << AAL_XGPON_INTR_ACTP_REGST_BIT)
#define XGPON_INTR_ACTP_DEREGST_EN             (1 << AAL_XGPON_INTR_ACTP_DEREGST_BIT)
#define XGPON_INTR_ACTP_DISABLE_SN_EN          (1 << AAL_XGPON_INTR_ACTP_DISABLE_SN_BIT)
#define XGPON_INTR_ACTP_DEACT_ONU_EN           (1 << AAL_XGPON_INTR_ACTP_DEACT_ONU_BIT)
#define XGPON_INTR_ACTP_LOSS_SYNC_EN           (1 << AAL_XGPON_INTR_ACTP_LOSS_SYNC_BIT)
#define XGPON_INTR_ACTP_TO1_EXPIRE_EN          (1 << AAL_XGPON_INTR_ACTP_TO1_EXPIRE_BIT)
#define XGPON_INTR_ACTP_TO2_EXPIRE_EN          (1 << AAL_XGPON_INTR_ACTP_TO2_EXPIRE_BIT)
#define XGPON_INTR_ACTP_ALL_EN                 (0x07F0000F)
/* XGPON PLOAM interrupt enable flags */
#define XGPON_INTR_PLMP_WRONG_ONUID_EN            (1 << AAL_XGPON_INTR_PLMP_WRONG_ONUID_BIT          )
#define XGPON_INTR_PLMP_BC_SEQ_MISMCH_EN          (1 << AAL_XGPON_INTR_PLMP_BC_SEQ_MISMCH_BIT        )
#define XGPON_INTR_PLMP_UC_SEQ_MISMCH_EN          (1 << AAL_XGPON_INTR_PLMP_UC_SEQ_MISMCH_BIT        )
#define XGPON_INTR_PLMP_MIC_CHK_FAIL_EN           (1 << AAL_XGPON_INTR_PLMP_MIC_CHK_FAIL_BIT         )
#define XGPON_INTR_PLMP_WRONG_PLOAM_EN            (1 << AAL_XGPON_INTR_PLMP_WRONG_PLOAM_BIT          )
#define XGPON_INTR_PLMP_KEY_EXCHG_TK4_TIMEOUT_EN  (1 << AAL_XGPON_INTR_PLMP_KEY_EXCHG_TK4_TIMEOUT_BIT)
#define XGPON_INTR_PLMP_KEY_EXCHG_TK5_TIMEOUT_EN  (1 << AAL_XGPON_INTR_PLMP_KEY_EXCHG_TK5_TIMEOUT_BIT)
#define XGPON_INTR_PLMP_KEY_CTRL_EN               (1 << AAL_XGPON_INTR_PLMP_KEY_CTRL_BIT             )
#define XGPON_INTR_PLMP_SLEEP_ALLOW_EN            (1 << AAL_XGPON_INTR_PLMP_SLEEP_ALLOW_BIT          )
#define XGPON_INTR_PLMP_SLEEP_REQ_DONE_EN         (1 << AAL_XGPON_INTR_PLMP_SLEEP_REQ_DONE_BIT       )
#define XGPON_INTR_PLMP_EQD_ERR_EN                (1 << AAL_XGPON_INTR_PLMP_EQD_ERR_BIT              )
#define XGPON_INTR_PLMP_BUSY_ERR_EN               (1 << AAL_XGPON_INTR_PLMP_BUSY_ERR_BIT             )
#define XGPON_INTR_PLMP_REGISTRATION_MSG_EN       (1 << AAL_XGPON_INTR_PLMP_REGISTRATION_MSG_BIT     )
#define XGPON_INTR_PLMP_BURST_PRFL_EN             (1 << AAL_XGPON_INTR_PLMP_BURST_PRFL_BIT           )
#define XGPON_INTR_PLMP_RUNING_REQ_EN             (1 << AAL_XGPON_INTR_PLMP_TUNING_REQ_BIT           )
#define XGPON_INTR_PLMP_RXBUF0_RDY_EN             (1 << AAL_XGPON_INTR_PLMP_RXBUF0_RDY_BIT           )
#define XGPON_INTR_PLMP_RXBUF1_RDY_EN             (1 << AAL_XGPON_INTR_PLMP_RXBUF1_RDY_BIT           )
#define XGPON_INTR_PLMP_ALL_EN                    (0xC00FFFFF)

/*
 * Default Module based XGPON interrupt enable values defined here!
 */
#define __XGPON_INTR_ACTP_ENA_DEFAULT     0x0000000F //XGPON_INTR_ACTP_ALL_EN
#define __XGPON_INTR_PLOAM_ENA_DEFAULT    0xC00000E0 //0xC00010E0
#define __XGPON_INTR_AES_PLMP_ENA_DEFAULT 0
#define __XGPON_INTR_AES_ENA_DEFAULT      0
#define __XGPON_INTR_RXCTL_ENA_DEFAULT    0
#define __XGPON_INTR_BWMP_ENA_DEFAULT     0
#define __XGPON_INTR_USBG_ENA_DEFAULT     0x00000001 // buf_empty_pop_intIE
#define __XGPON_INTR_DSFP_ENA_DEFAULT     0
#define __XGPON_INTR_XGFEC_ENA_DEFAULT    0
#define __XGPON_INTR_XGPUC_ENA_DEFAULT    0
#define __PON_GLB_OPT_ENA_DEFAULT         0x00000001 //rx_losE

/* Interrupt managment table */
static struct _intr_tbl_s{
    char        *name;         /* interrupt name */
    //ca_uint32_t  hw_irq;       /* hardware irq id */
    ca_uint32_t  ena_reg_addr; /* address of INTR enable register */
    ca_uint32_t  src_reg_addr; /* address of INTR source register */
    ca_uint32_t  ena_bmp;      /* default enable sub-interrupt bitmap */
    ca_uint64_t  count;        /* count of INTR */
    ca_uint64_t  fail_count;   /* count of fail handled INTR */
    ca_uint64_t  handle_count; /* count of ok handled INTR */
}__xgpon_intr_tbl[] = {
    { "ACTP",     XGPN_ACTP_INTERRUPTE0,     XGPN_ACTP_INTERRUPT,     __XGPON_INTR_ACTP_ENA_DEFAULT, 0, 0, 0},
    { "PLMP",     XGPN_PLMP_INTERRUPTE0,     XGPN_PLMP_INTERRUPT,     __XGPON_INTR_PLOAM_ENA_DEFAULT, 0, 0, 0},
    { "AES_PLMP", XGPN_AES_PLMP_INTERRUPTE0, XGPN_AES_PLMP_INTERRUPT, __XGPON_INTR_AES_PLMP_ENA_DEFAULT, 0, 0, 0},
    { "AES",      XGPN_AES_INTERRUPTE0,      XGPN_AES_INTERRUPT,      __XGPON_INTR_AES_ENA_DEFAULT, 0, 0, 0},
    { "RXCTL",    XGPN_RXCTL_INTERRUPTE0,    XGPN_RXCTL_INTERRUPT,    __XGPON_INTR_RXCTL_ENA_DEFAULT, 0, 0, 0},
    { "BWMP",     XGPN_BWMP_INTERRUPTE0,     XGPN_BWMP_INTERRUPT,     __XGPON_INTR_BWMP_ENA_DEFAULT, 0, 0, 0},
    { "USBG",     XGPN_USBG_INTERRUPTE0,     XGPN_USBG_INTERRUPT,     __XGPON_INTR_USBG_ENA_DEFAULT, 0, 0, 0},
    { "DSFP",     XGPN_DSFP_INTERRUPTE0,     XGPN_DSFP_INTERRUPT,     __XGPON_INTR_DSFP_ENA_DEFAULT, 0, 0, 0},
    { "XGFEC",    XGPN_XGFEC_RXINTE0,        XGPN_XGFEC_RXINT,        __XGPON_INTR_XGFEC_ENA_DEFAULT, 0, 0, 0}
    //{ "GLB_OPT",  GLOBAL_OPT_INTENABLE,      GLOBAL_OPT_INTERRUPT,    __PON_GLB_OPT_ENA_DEFAULT, 0, 0, 0}
    //{ "XGPUC",    PUC_PONCNTL_INTENABLE,     PUC_PONCNTL_INTERRUPT,   __XGPON_INTR_XGPUC_ENA_DEFAULT, 0}
};

/* XGPON interrupt handlers */
static aal_intr_callback_t aal_xgpon_intr_isr[AAL_XGPON_INTR_NUM];
static aal_intr_msg_handler_t g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_NUM];

struct kfifo aal_pon_kfifo[AAL_PON_KMSG_CHN_NUM];
static void __ploam_tasklet_action(unsigned long t);
static void __actp_tasklet_action(unsigned long t);
static void __aes_key_ctrl_tasklet_action(unsigned long t);
static void __aes_key_tk4_tasklet_action(unsigned long t);
static void __aes_key_tk5_tasklet_action(unsigned long t);
#if 0
static void __ploam_rgst_tasklet_action(unsigned long t);
#endif
DECLARE_TASKLET(aal_ploam_tasklet, __ploam_tasklet_action, 0);
DECLARE_TASKLET(aal_actp_tasklet, __actp_tasklet_action, 0);
DECLARE_TASKLET(aal_aes_key_ctrl_tasklet, __aes_key_ctrl_tasklet_action, 0);
DECLARE_TASKLET(aal_aes_key_tk4_tasklet, __aes_key_tk4_tasklet_action, 0);
DECLARE_TASKLET(aal_aes_key_tk5_tasklet, __aes_key_tk5_tasklet_action, 0);
#if 0
DECLARE_TASKLET(aal_ploam_rgst_tasklet, __ploam_rgst_tasklet_action, 0);
#endif

/*******************************************************************/
/*                    Local Functions                              */
/*******************************************************************/
/**
 * @aal_xgpon_intr_enable_set
 * Set XGPON interrupt enable bitmap for a specific interrupt
 */
ca_status_t aal_xgpon_intr_enable_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx,
    CA_IN ca_uint32_t  enable_bmp)
{

    if (intr_idx >= AAL_XGPON_INTR_NUM)
        return CA_E_PARAM;

    AAL_PON_REG_WRITE(enable_bmp, __xgpon_intr_tbl[intr_idx].ena_reg_addr);

    if (intr_idx == AAL_XGPON_INTR_ACTP
        && (enable_bmp & XGPON_INTR_ACTP_ONU_STATE_CHANGE_EN)) {
        AAL_PON_REG_WRITE(0xfffffff, XGPN_ACTP_ST_CHG_INTE);
    }

    return CA_E_OK;
}

/**
 * @aal_xgpon_intr_enable
 * Enable XGPON interrupt based on XGPON interrupt index
 */
ca_status_t aal_xgpon_intr_enable(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx)
{
    return aal_xgpon_intr_enable_set(device_id, intr_idx, ~0U);
}

/**
 * @aal_xgpon_intr_disable
 * Disable XGPON interrupt based on XGPON interrupt index
 */
ca_status_t aal_xgpon_intr_disable(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx)
{
    return aal_xgpon_intr_enable_set(device_id, intr_idx, 0U);
}

/**
 * @aal_xgpon_intr_enable_get
 * Get XGPON interrupt enable bitmap of a specific interrupt
 */
ca_status_t aal_xgpon_intr_enable_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_idx,
    CA_OUT ca_uint32_t *enable_bmp)
{
    if (intr_idx >= AAL_XGPON_INTR_NUM || NULL == enable_bmp)
        return CA_E_PARAM;

    *enable_bmp = AAL_PON_REG_READ(__xgpon_intr_tbl[intr_idx].ena_reg_addr);

    return CA_E_OK;
}

/**
 * @aal_xgpon_intr_status_get
 * Get XGPON interrupt status of a specific interrupt
 */
ca_status_t aal_xgpon_intr_status_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t     intr_idx,
    CA_OUT ca_uint32_t    *status_bmp)
{
    ca_uint32_t ena_bmp = 0;

    if (intr_idx >= AAL_XGPON_INTR_NUM || NULL == status_bmp)
        return CA_E_PARAM;

    ena_bmp = AAL_PON_REG_READ(__xgpon_intr_tbl[intr_idx].ena_reg_addr);

    *status_bmp = AAL_PON_REG_READ(__xgpon_intr_tbl[intr_idx].src_reg_addr);

    *status_bmp &= ena_bmp; /* ONLY return the status of enabled intr */

    return CA_E_OK;
}

/**
 * @aal_xgpon_intr_clr
 * Clear XGPON interrupt to allow next interrupt incoming
 */
ca_status_t aal_xgpon_intr_clr(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_idx,
    CA_IN  ca_uint32_t  clr_bmp)
{
    ca_uint32_t ena_bmp = 0;

    if (intr_idx >= AAL_XGPON_INTR_NUM)
        return CA_E_PARAM;

    ena_bmp = AAL_PON_REG_READ(__xgpon_intr_tbl[intr_idx].src_reg_addr);
    ena_bmp &= clr_bmp;
    AAL_PON_REG_WRITE(ena_bmp, __xgpon_intr_tbl[intr_idx].src_reg_addr); /* Write ones to clear */

    if (intr_idx == AAL_XGPON_INTR_ACTP) {
        AAL_PON_REG_WRITE(0xfffffff, XGPN_ACTP_ST_CHG_INT);
    }

    return CA_E_OK;
}

ca_status_t aal_xgpon_intr_info_get(
    CA_IN  ca_device_id_t  device_id
)
{
    ca_uint32_t  intr_idx;

    ca_printf("\r\n");
    ca_printf("%-10s %-12s %-12s %-12s %s\r\n",
              "PON-INTR", "EnableBmp", "Counter", "Handled Count", "Fail Count");
    ca_printf("---------------------------------------------------------------\r\n");

    for (intr_idx = 0; intr_idx < AAL_XGPON_INTR_NUM; intr_idx++) {

        ca_printf("%-10s 0x%08x   %-12lld %-12lld %lld\r\n",
            __xgpon_intr_tbl[intr_idx].name,
            __xgpon_intr_tbl[intr_idx].ena_bmp,
            __xgpon_intr_tbl[intr_idx].count,
            __xgpon_intr_tbl[intr_idx].handle_count,
             __xgpon_intr_tbl[intr_idx].fail_count);
    }

    return CA_E_OK;
}

/**
 * @aal_xgpon_intr_name_get
 * Get XGPON interrupt name string, normally for debug display
 */
ca_status_t aal_xgpon_intr_name_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_idx,
    CA_IN  ca_uint32_t  name_len,
    CA_OUT char        *name_str)
{
    if (intr_idx >= AAL_XGPON_INTR_NUM || NULL == name_str || name_len == 0)
        return CA_E_PARAM;

    if (__xgpon_intr_tbl[intr_idx].name && name_len)
        strncpy(name_str, __xgpon_intr_tbl[intr_idx].name, name_len);

    return CA_E_OK;
}


/**
 * @aal_xgpon_intr_reg
 * Register XGPON interrupt handler, it will overwite the old handler
 */
ca_status_t aal_xgpon_intr_isr_reg(
    aal_xgpon_intr_t     xgpon_intr,
    aal_intr_callback_t  callback)
{
    if (xgpon_intr >= AAL_XGPON_INTR_NUM || callback == NULL)
        return CA_E_PARAM;

    aal_xgpon_intr_isr[xgpon_intr] = callback;

    return CA_E_OK;
}

/**
 * @aal_xgpon_intr_unreg
 * Unregister XGPON interrupt handler
 */
ca_status_t aal_xgpon_intr_isr_unreg(
    aal_xgpon_intr_t     xgpon_intr)
{
    if (xgpon_intr >= AAL_XGPON_INTR_NUM )
        return CA_E_PARAM;

    aal_xgpon_intr_isr[xgpon_intr] = NULL;

    return CA_E_OK;
}


ca_status_t aal_xgpon_intr_handler_reg(
    aal_xgpon_intr_msg_t   intr_msg,
    aal_intr_msg_handler_t handler)
{
    AAL_PON_CHK_KMSG_CHN(intr_msg);
    CA_NULL_PTR_CHECK(handler);

    g_xgpon_intr_handlers[intr_msg] = (aal_intr_msg_handler_t)handler;

    return CA_E_OK;
}

ca_status_t aal_xgpon_intr_handler_unreg(
    aal_xgpon_intr_msg_t   intr_msg,
    aal_intr_msg_handler_t handler)
{
    AAL_PON_CHK_KMSG_CHN(intr_msg);
    CA_NULL_PTR_CHECK(handler);

    if (handler == g_xgpon_intr_handlers[intr_msg]) {
        g_xgpon_intr_handlers[intr_msg] = NULL;
        return CA_E_OK;
    }

    return CA_E_NOT_FOUND;
}

/****************************************************************************/
/*  Tasklets to accelerate some interrupt handlers, they are optional       */
/****************************************************************************/

/**__actp_tasklet_action
 *@brief: XGPON activation process interrupt tasklet handler
 *@see: act_use_tasklet
 */
static void __actp_tasklet_action(unsigned long t)
{
    aal_xgpon_actp_kmsg_t  msg;
    ca_status_t            ret =  CA_E_OK;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_ACTP, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        PON_DBG("ACTP msg rx : INTR %#x, ST_CHG %#x, STS %#x\n",
            msg.intr, msg.st_chg, msg.sts);

        if  (!msg.intr) {
            PON_DBG("err: rx actp msg but with null intr \n");
            return;
        }

        /* handle the interrupt */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_ACTP]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_ACTP])(msg.intr, msg.st_chg, msg.sts, NULL);
        }
    }
}


/**__ploam_tasklet_action
 *@brief: XGPON PLOAM packet Rx interrupt tasklet handler
 *@see: ploam_use_tasklet
 */
static void __ploam_tasklet_action(unsigned long t)
{
    aal_xgpon_ploam_kmsg_t   msg;
    ca_status_t            ret =  CA_E_OK;

    memset(&msg, 0, sizeof(msg));

    while (1) {
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_PLOAM, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        if  (!msg.intr) {
            PON_DBG("err: rx ploam msg with null intr \n");
            return;
        }

        PON_DBG("PLOAM Tasklet rx kmsg intr %#x, len %d\n", msg.intr, msg.len);

        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_PLOAM]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_PLOAM])(msg.intr, msg.intr, msg.len, (void *)(&msg.ploam_pkt[0]));
        }
    }
}

/**__aes_key_exchg_tasklet_action
 *@brief: XGPON AES key exchange interrupt tasklet handler
 *@see: aes_use_tasklet
 */
static void __aes_key_ctrl_tasklet_action(unsigned long t)
{
    aal_xgpon_key_exchg_kmsg_t   msg;
    ca_status_t            ret =  CA_E_OK;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_AES_KEY_CTRL, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        PON_DBG("AES msg rx : INTR %#x, STS %#x, KEY_CTRL_STS %#x,\n",
            msg.intr, msg.sts, msg.key_ctrl_stat);

        if  (!msg.intr) {
            PON_DBG("err: rx aes key exchange msg with null intr \n");
            return;
        }

        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_CTRL]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_CTRL])(msg.intr, msg.sts, msg.key_ctrl_stat, NULL);
        }
    }
}
static void __aes_key_tk4_tasklet_action(unsigned long t)
{
    aal_xgpon_key_exchg_kmsg_t   msg;
    ca_status_t            ret =  CA_E_OK;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_AES_KEY_TK4, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }
        PON_DBG("AES msg rx : INTR %#x, STS %#x, KEY_CTRL_STS %#x,\n",
            msg.intr, msg.sts, msg.key_ctrl_stat);
        if  (!msg.intr) {
            PON_DBG("err: rx aes key exchange msg with null intr \n");
            return;
        }

        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_TK4]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_TK4])(msg.intr, msg.sts, msg.key_ctrl_stat, NULL);
        }
    }
}
static void __aes_key_tk5_tasklet_action(unsigned long t)
{
    aal_xgpon_key_exchg_kmsg_t   msg;
    ca_status_t            ret =  CA_E_OK;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_AES_KEY_TK5, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }
        PON_DBG("AES msg rx : INTR %#x, STS %#x, KEY_CTRL_STS %#x,\n",
            msg.intr, msg.sts, msg.key_ctrl_stat);
        if  (!msg.intr) {
            PON_DBG("err: rx aes key exchange msg with null intr \n");
            return;
        }
        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_TK5]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_TK5])(msg.intr, msg.sts, msg.key_ctrl_stat, NULL);
        }
    }
}

#if 0
static void __ploam_rgst_tasklet_action(unsigned long t)
{
    aal_xgpon_rgst_kmsg_t   msg;
    ca_status_t            ret =  CA_E_OK;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_RGST, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        PON_DBG("RGST msg rx : INTR %#x\n", msg.intr);

        if  (!msg.intr) {
            PON_DBG("err: rx plm rgst msg with null intr \n");
            return;
        }

        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_RGST]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_RGST])(msg.intr, 0, 0, NULL);
        }
    }
}
#endif


/** aal_pon_kmsg_recv
 *@brief: receive the interrupt message from KFIFO
 */
ca_status_t aal_pon_kmsg_recv(
    ca_uint32_t msg_chn,
    void *msg,
    ca_uint8_t msg_len)
{
    int ret = 0;

    AAL_PON_CHK_KMSG_CHN(msg_chn);
    //AAL_PON_CHK_KMSG_LEN(msg_len);

    if (kfifo_is_empty(&aal_pon_kfifo[msg_chn])) {
        return CA_E_EMPTY;
    }

    /* De-Queue */
    if (0 == (ret = kfifo_out(&aal_pon_kfifo[msg_chn], msg, msg_len))) {
        ca_printf("KFIFO %d out fail.\n", msg_chn);
        return CA_E_NOT_FOUND;
    }

    PON_DBG("%s: KFIFO %d receive len %d OK.\n", __func__, msg_chn, ret);

    return CA_E_OK;
}

/** aal_pon_kmsg_send
 *@brief: send the interrupt message to KFIFO
 */
ca_status_t aal_pon_kmsg_send(
    ca_uint32_t msg_chn,
    void *msg,
    ca_uint32_t msg_len)
{
    int ret = 0;

    AAL_PON_CHK_KMSG_CHN(msg_chn);
    AAL_PON_CHK_KMSG_LEN(msg_len);

    /* En-Queue */
    if (0 == (ret = kfifo_in(&aal_pon_kfifo[msg_chn], msg, msg_len))) {
        ca_printf("KFIFO %d is full.\n", msg_chn );
        return CA_E_FULL;
    }

    PON_DBG("%s: KFIFO %d send len %d OK.\n", __func__, msg_chn, ret);

    return CA_E_OK;
}


/** aal_pon_kmsg_init
 *@brief: init the KFIFO for interrupt message to handlers (tasklet or wq)
 */
static ca_status_t aal_pon_kmsg_init(ca_uint32_t fifo_num)
{
    int rv = 0;
    int i  = 0;
    static ca_boolean_t kmsg_init = 0;

    if (kmsg_init)
        return CA_E_OK;

    for (i=0; i<fifo_num && i < AAL_PON_KMSG_CHN_NUM; ++i) {
        rv = kfifo_alloc(&aal_pon_kfifo[i], AAL_PON_KMSG_FIFO_SIZE, GFP_KERNEL);
        if (rv) {
            ca_printf("kfifo_alloc KFIFO %d fail.\n", i);
            return CA_E_RESOURCE;
        }
    }

    kmsg_init = TRUE;

    return CA_E_OK;
}

static ca_status_t __xgpon_actp_isr(ca_uint32_t intr_id, ca_uint32_t intr_sts)
{
    aal_xgpon_actp_kmsg_t  actp_msg;
    ca_uint32_t            ret  = 0;

    actp_msg.intr   = intr_sts;
    actp_msg.sts    = AAL_PON_REG_READ(XGPN_ACTP_STS);
    actp_msg.st_chg = AAL_PON_REG_READ(XGPN_ACTP_ST_CHG_INT);
    actp_msg.st_chg &= AAL_PON_REG_READ(XGPN_ACTP_ST_CHG_INTE);

    if (actp_msg.st_chg) { /* ACTP state change intr */
        AAL_PON_REG_WRITE(actp_msg.st_chg, XGPN_ACTP_ST_CHG_INT);
        /* En-queuen */
        ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_ACTP, &actp_msg, sizeof(actp_msg));
        if (ret != CA_E_OK) {
            return CA_E_FULL;
        }

        /* wake the handler to process */
        if (act_use_tasklet) {
            tasklet_schedule(&aal_actp_tasklet);
        } else {
            ret = queue_work(pon_act_wq.pon_wq, &(pon_act_wq.work_info.work));
            if (ret) {
                PON_DBG("add work to pon_act_wq fail, already exist.\n");
                return CA_E_OK;
            }
        }
    }

    return ret;

}

extern ca_status_t __send_key_report_msg(ca_uint8_t rpt_type);
extern ca_status_t __generate_key_report_msg(ca_uint8_t rpt_type);
extern ca_status_t __send_rpt_ploam(ca_uint8_t rpt_type);
extern ca_status_t __send_key_report_msg_isr(ca_xgpon_key_report_ploam_msg_t key_rpt_msg);
extern ca_status_t __send_key_report_isr(ca_uint8_t rpt_type);

#if 0
ca_status_t __build_key_new_ploam(void)
{
	int i = 0;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl ;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA2;
	memset(&g_key_rpt_new_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
    g_key_rpt_new_msg.onu_id = g_onu_id;
	g_key_rpt_new_msg.msg_type_id = CA_KEY_REPORT;
	g_key_rpt_new_msg.seq_no = 0;
    g_key_rpt_new_msg.report_type = 0;
	g_key_rpt_new_msg.key_idx = 0;
	g_key_rpt_new_msg.frament_no = g_key_prt_fragment_num;
	memcpy(g_key_rpt_new_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);	//always use hard coded key 0 (16 x 1)

	//fill PLOAM TX buffer right after boot - seq no and key id need to be updated before sending
	for (i = 0; i < 4; i++){
    	reg_data.wrd = ((ca_uint32_t)(g_encryped_key0[i*4]) << 24 )
        	| ((ca_uint32_t)(g_encryped_key0[i*4 + 1]) << 16)
        	| ((ca_uint32_t)(g_encryped_key0[i*4 + 2]) << 8)
        	| ((ca_uint32_t)(g_encryped_key0[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}
	return CA_E_OK;
}
/*
 * fill out key confirm report PLOAM during boot
 * call this BEFORE activation
 */
ca_status_t __build_key_confirm_ploam(void)
{
	//always use hard coded key name 0
	int i = 0;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl ;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA2;

	memset(&g_key_rpt_confirm_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
	
    g_key_rpt_confirm_msg.onu_id = g_onu_id;
	g_key_rpt_confirm_msg.msg_type_id = CA_KEY_REPORT;
	g_key_rpt_confirm_msg.seq_no = 0;
    g_key_rpt_confirm_msg.report_type = 1;
	g_key_rpt_confirm_msg.key_idx = 0;
	g_key_rpt_confirm_msg.frament_no = g_key_prt_fragment_num;
	
	//always use hard coded key 0 (16 x 1)
	memcpy(g_key_rpt_confirm_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);	

	return CA_E_OK;
}


/*
 * new key PLOAM should be already in PLOAM TX buffer, just update seq no and key id and trigger send
 * then put key name in PLOAM_TX_BUF
 */
ca_status_t __send_new_key_fill_key_name(aal_xgpon_key_exchg_kmsg_t *aes_msg)
{
	ca_status_t ret = 0;
	int i = 0;
	ca_uint32_t ploam_retry = 0;
	aal_xgpn_plmp_txbuf_t *txbuf;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;

	//update seq no and key index only, the rest of the fields are always the same
    g_key_rpt_new_msg.seq_no = aes_msg->key_ctrl_stat&0xFF;
    g_key_rpt_new_msg.key_idx = (aes_msg->key_ctrl_stat>>16)&0xFF;

    txbuf = (aal_xgpn_plmp_txbuf_t *)&g_key_rpt_new_msg;
	//update PLOAM TX buffer (bytes 1-8)
	for (i = 0; i < 2; i++){
		reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
			| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
			| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
			| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}        
	//check TX buffer ready
	memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    if(reg_ctrl.bf.pkt_rdy) {
		printk("PLOAM TX buffer not ready\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }

	//key new is already in TX buffer, just trigger PLOAM send
	reg_ctrl.bf.pkt_rdy = 1;
	AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);

	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    }
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		printk("PLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }



	//fill PLOAM TX buffer with key name - key name is only 16 bytes
	reg_addr = XGPN_PLMP_TXBUF_DATA0;
	txbuf = (aal_xgpn_plmp_txbuf_t *)&g_key_rpt_confirm_msg;
	for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4-4); i++){
    	reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
        	| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
        	| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
        	| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
    	AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
    	reg_addr += 4;
	}

	return CA_E_OK;
}
/*
 * Key Name PLOAM should be already in PLOAM TX buffer, just update seq no and key id and trigger send
 * then put new key in PLOAM_TX_BUF
 */
ca_status_t __send_key_name_fill_new_key(aal_xgpon_key_exchg_kmsg_t *aes_msg)
{
	int i = 0;
	ca_uint32_t ploam_retry = 0;
	ca_status_t ret = 0;
	aal_xgpn_plmp_txbuf_t *txbuf;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;

	//update seq no and key index only, the rest of the fields are always the same
    g_key_rpt_confirm_msg.seq_no = aes_msg->key_ctrl_stat&0xFF;
    g_key_rpt_confirm_msg.key_idx = (aes_msg->key_ctrl_stat>>16)&0xFF;
	
	txbuf = (aal_xgpn_plmp_txbuf_t *)&g_key_rpt_confirm_msg;
	//update PLOAM TX buffer (bytes 1-8)
	for (i = 0; i < 2; i++){
		reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}
                
	//check TX buffer ready
	memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    if(reg_ctrl.bf.pkt_rdy) {
		PLOAM_DEBUG("PLOAM TX buffer not ready\n"); /* something wrong H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }

	//key name is already in TX buffer, just trigger PLOAM send
    reg_ctrl.bf.pkt_rdy = 1;
	AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);


	//wait till previous PLOAM is sent
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    }
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		PLOAM_DEBUG("PLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }



	//fill PLOAM TX buffer with new key 
	reg_addr = XGPN_PLMP_TXBUF_DATA0;
	txbuf = (aal_xgpn_plmp_txbuf_t *)&g_key_rpt_new_msg;
	for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4 - 4); i++){
    	reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
        	| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
        	| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
        	| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
    	AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
    	reg_addr += 4;
	}

	return CA_E_OK;
}

#endif

#ifdef TITAN
#if 0
ca_status_t __build_key_new_ploam(void)
{
	int i = 0;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl ;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA2;
	memset(&g_key_rpt_new_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
    g_key_rpt_new_msg.onu_id = g_onu_id;
	g_key_rpt_new_msg.msg_type_id = CA_KEY_REPORT;
	g_key_rpt_new_msg.seq_no = 0;
    g_key_rpt_new_msg.report_type = 0;
	g_key_rpt_new_msg.key_idx = 0;
	g_key_rpt_new_msg.frament_no = g_key_prt_fragment_num;
	memcpy(g_key_rpt_new_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);	//always use hard coded key 0 (16 x 1)

	//fill PLOAM TX buffer right after boot - seq no and key id need to be updated before sending
	for (i = 0; i < 4; i++){
    	reg_data.wrd = ((ca_uint32_t)(g_encryped_key0[i*4]) << 24 )
        	| ((ca_uint32_t)(g_encryped_key0[i*4 + 1]) << 16)
        	| ((ca_uint32_t)(g_encryped_key0[i*4 + 2]) << 8)
        	| ((ca_uint32_t)(g_encryped_key0[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}
	return CA_E_OK;
}
/*
 * fill out key confirm report PLOAM during boot
 * call this BEFORE activation
 */
ca_status_t __build_key_confirm_ploam(void)
{
	//always use hard coded key name 0
	
	int i = 0;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl ;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA2;

	aal_key_report_frag_t key_rpt_frag;

	memset(&key_rpt_frag,0,sizeof(key_rpt_frag));
	memset(&g_key_rpt_confirm_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
	
    g_key_rpt_confirm_msg.onu_id = g_onu_id;
	g_key_rpt_confirm_msg.msg_type_id = CA_KEY_REPORT;
	g_key_rpt_confirm_msg.seq_no = 0;
    g_key_rpt_confirm_msg.report_type = 1;
	g_key_rpt_confirm_msg.key_idx = 0;
	g_key_rpt_confirm_msg.frament_no = g_key_prt_fragment_num;
	
	//always use hard coded key 0 (16 x 1)
	memcpy(g_key_rpt_confirm_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);	

	memcpy(key_rpt_frag.data, g_kek_name0, XGPN_AES_KEY_LEN);
	ret = aal_ploam_key_report_frag_set(0,&key_rpt_frag);
    CA_XGPON_AES_RET_VALUE_CHECK(ret);

	return CA_E_OK;
}

/*
 * new key PLOAM should be already in PLOAM TX buffer, just update seq no and key id and trigger send
 * then put key name in PLOAM_TX_BUF
 */
ca_status_t __send_new_key(aal_xgpon_key_exchg_kmsg_t *aes_msg)
{
	ca_status_t ret = 0;
	int i = 0;
	ca_uint32_t ploam_retry = 0;
	aal_xgpn_plmp_txbuf_t *txbuf;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;

	//update seq no and key index only, the rest of the fields are always the same
    g_key_rpt_new_msg.seq_no = aes_msg->key_ctrl_stat&0xFF;
    g_key_rpt_new_msg.key_idx = (aes_msg->key_ctrl_stat>>16)&0xFF;

    txbuf = (aal_xgpn_plmp_txbuf_t *)&g_key_rpt_new_msg;
	//update PLOAM TX buffer (bytes 1-8)
	for (i = 0; i < 2; i++){
		reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
			| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
			| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
			| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}        
	//check TX buffer ready
	memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    if(reg_ctrl.bf.pkt_rdy) {
		printk("PLOAM TX buffer not ready\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }

	//key new is already in TX buffer, just trigger PLOAM send
	reg_ctrl.bf.pkt_rdy = 1;
	AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);

	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    }
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		printk("PLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }
	
	return CA_E_OK;
}
/*
 * Key Name PLOAM should be already in PLOAM TX buffer, just update seq no and key id and trigger send
 * then put new key in PLOAM_TX_BUF
 */
ca_status_t __send_key_name(aal_xgpon_key_exchg_kmsg_t *aes_msg)
{
	int i = 0;
	ca_int32_t wait_timeout =  __KEY_REPORT_TIMEOUT;
	aal_xgpn_plmp_key_report_mask_t key_rpt_mask;
	aal_xgpn_plmp_key_report_t key_rpt;
	key_rpt_mask.wrd =  ~0U;

	g_key_rpt_new_msg.seq_no = aes_msg->key_ctrl_stat&0xFF;
    g_key_rpt_new_msg.key_idx = (aes_msg->key_ctrl_stat>>16)&0xFF;

	key_rpt.seq = g_key_rpt_confirm_msg.seq_no;
	key_rpt.type = g_key_rpt_confirm_msg.report_type;	 /*0: New key. 1: Report on existing key.*/
	key_rpt.index = g_key_rpt_confirm_msg.key_idx;
	key_rpt.fragment_num= g_key_rpt_confirm_msg.frament_no ;/*Here always 0,refer to 989.3 11.3.4.3*/
	key_rpt.rdy=  TRUE;
	
    ret = aal_ploam_key_report_set(0,key_rpt_mask,&key_rpt);
    CA_XGPON_AES_RET_VALUE_CHECK(ret);

    wait_timeout = __KEY_REPORT_TIMEOUT;
    /* Wait for send finish */
    key_rpt.rdy = 1;
    do {
        aal_ploam_key_report_get(0, &key_rpt);
    }while (key_rpt.rdy && wait_timeout--);

    if (wait_timeout <= 0) {
        XGPN_AES_SM_DEBUG("%s::%d:Send key report msg %d fail, Hardware timeout.\n",
            __FUNCTION__, __LINE__, key_rpt.type);
        __XGPN_AES_UNLOCK(0);
        return CA_E_TIMEOUT;
    }
	return CA_E_OK;
}
#endif

ca_status_t __send_key_report_msg_titan(ca_uint8_t rpt_type){

    ca_status_t ret = CA_E_OK;
    aal_key_report_frag_t key_rpt_frag;
    aal_xgpn_plmp_key_report_t    key_rpt;
    aal_xgpn_plmp_key_report_mask_t  key_rpt_mask;
    ca_int32_t wait_timeout =  __KEY_REPORT_TIMEOUT;

    key_rpt_mask.wrd =  ~0U;

    memset(&key_rpt_frag,0,sizeof(key_rpt_frag));
    memset(&key_rpt,0,sizeof(key_rpt));

    if(rpt_type == CA_KEY_CONTROL_NEW){
        key_rpt.seq = g_key_ctrl_seqNo;
        key_rpt.type = rpt_type;    /*0: New key. 1: Report on existing key.*/
        key_rpt.index = g_key_idx;
        //key_rpt.index = g_key_idx-1;//for ZTE only
        key_rpt.fragment_num= g_key_prt_fragment_num ;/*Here always 0,refer to 989.3 11.3.4.3*/
        key_rpt.rdy=  TRUE;

        if(g_key_idx == 1){
            memcpy(key_rpt_frag.data, g_encryped_key0, XGPN_AES_KEY_LEN);
        }
        if(g_key_idx == 2){
            memcpy(key_rpt_frag.data, g_encryped_key1, XGPN_AES_KEY_LEN);
        }
        
        ret = aal_ploam_key_report_frag_set(0,&key_rpt_frag);
        CA_RET_VALUE_CHECK(ret);

        ret = aal_ploam_key_report_set(0,key_rpt_mask,&key_rpt);
        CA_RET_VALUE_CHECK(ret);

        wait_timeout = __KEY_REPORT_TIMEOUT;
        /* Wait for send finish */
        key_rpt.rdy = 1;
        do {
            aal_ploam_key_report_get(0, &key_rpt);
        }while (key_rpt.rdy && wait_timeout--);

        if (wait_timeout <= 0) {
            ca_printf("%s::%d:Send key report msg %d fail, Hardware timeout.\n",
                __FUNCTION__, __LINE__, rpt_type);
            return CA_E_TIMEOUT;
        }
		g_rpt_new_key_cnt++ ;
    }else {
		int i = 0;
        ca_xgpon_key_report_ploam_msg_t  key_rpt_msg;
		ca_uint32_t ploam_retry = 0;
		aal_xgpn_plmp_txbuf_t *txbuf;
		XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
		aal_reg_xgpn_plmp_txbuf_data_t reg_data;
		ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;
		
		memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));
        memset(&key_rpt_msg, 0, sizeof(key_rpt_msg));
		
        key_rpt_msg.onu_id = g_onu_id;
        key_rpt_msg.msg_type_id = CA_KEY_REPORT;
        key_rpt_msg.seq_no = g_key_ctrl_seqNo;
        key_rpt_msg.report_type = rpt_type;
        key_rpt_msg.key_idx = g_key_idx;
        key_rpt_msg.frament_no = g_key_prt_fragment_num;
		
        if(g_key_idx == 1){
            memcpy(key_rpt_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
        }
        if(g_key_idx == 2){
            memcpy(key_rpt_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
        }
        
		//check TX buffer ready
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
			reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	    }
		if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
			printk("PLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
			return CA_E_TIMEOUT;
	    }
		
		//update PLOAM TX buffer (bytes 1-8)
        txbuf = (aal_xgpn_plmp_txbuf_t *)&key_rpt_msg;
		for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4 - 4); i++){
			reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
		    	| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
		    	| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
		    	| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
			AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
			reg_addr += 4;
		}
	                
		//trigger PLOAM send
		reg_ctrl.bf.pkt_rdy = 1;
		AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);
		
		//make sure key rpt went out
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
		while(reg_ctrl.bf.pkt_rdy) {
			reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
		}
		
		g_rpt_ext_key_cnt++ ;
    }
	return CA_E_OK;
}

static ca_status_t __ploam_key_ctrl_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
	ca_uint8_t key_rpt_type = 0;
	ca_uint8_t key_ctrl_flag = 0;
    aal_xgpon_key_exchg_kmsg_t aes_msg;
	XGPN_PLMP_SW_ACK_t ack_reg;

    memset(&aes_msg, 0, sizeof(aal_xgpon_key_exchg_kmsg_t));
    aes_msg.intr = xgpn_ploam_int;
    aes_msg.sts  = xgpn_ploam_int;
    aes_msg.key_ctrl_stat = AAL_PON_REG_READ(XGPN_PLMP_KEY_CTRL_STAT);

	#if 0 
	ack_reg.wrd = AAL_PON_REG_READ(XGPN_PLMP_SW_ACK);
	if(ack_reg.bf.chng_pwr_level == 0){
		ack_reg.bf.seqno = (aes_msg.key_ctrl_stat)&0xFF;
		ack_reg.bf.complt_code = 0x2;
		ack_reg.bf.chng_pwr_level = 0x1;
		AAL_PON_REG_WRITE(ack_reg.wrd, XGPN_PLMP_SW_ACK);
	}
	#endif
	
	g_key_ctrl_seqNo = aes_msg.key_ctrl_stat&0xFF;
	g_key_ctrl_flag = (aes_msg.key_ctrl_stat>>8)&0xFF;
	g_key_idx = (aes_msg.key_ctrl_stat>>16)&0xFF;
	g_key_len = (aes_msg.key_ctrl_stat>>24)&0xFF;

	if (g_key_ctrl_flag == CA_KEY_CONTROL_NEW){
        g_rpt_type = 0;
		ret = __send_key_report_msg_titan(g_rpt_type);
		CA_RET_VALUE_CHECK(ret);
    }else if(g_key_ctrl_flag == CA_KEY_CONTROL_CONFIRM){
        g_rpt_type = 1;
		ret = __send_key_report_msg_titan(g_rpt_type);
		CA_RET_VALUE_CHECK(ret);
    }

    ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_AES_KEY_CTRL, &aes_msg, sizeof(aes_msg));
    if (ret != CA_E_OK) {
        return ret;
    }

    /* wake the handler to process */
    if (aes_ctrl_use_tasklet) {
        tasklet_schedule(&aal_aes_key_ctrl_tasklet);
    } else {
        ret = queue_work(pon_ploam_key_ctrl_wq.pon_wq, &(pon_ploam_key_ctrl_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to key_exchg_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;

}
#endif

#if 0
ca_status_t __build_key_new_ploam(void)
{
	int i = 0;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl ;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA2;

	memset(&g_key_rpt_new_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
	
    g_key_rpt_new_msg.onu_id = g_onu_id;
	g_key_rpt_new_msg.msg_type_id = CA_KEY_REPORT;
    g_key_rpt_new_msg.report_type = CA_KEY_CONTROL_NEW;
	g_key_rpt_new_msg.frament_no = g_key_prt_fragment_num;
	
	g_key_rpt_new_msg.seq_no = CA_PON_AES_INVAILD_VALUE;
	g_key_rpt_new_msg.key_idx = CA_PON_AES_INVAILD_VALUE;
	memcpy(g_key_rpt_new_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);	//always use hard coded key 0 (16 x 1)

	return CA_E_OK;
}

/*
 * fill out key confirm report PLOAM during boot
 * call this BEFORE activation
 */
ca_status_t __build_key_confirm_ploam(void)
{
	//always use hard coded key name 0
	int i = 0;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl ;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA2;

	memset(&g_key_rpt_confirm_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
	
    g_key_rpt_confirm_msg.onu_id = g_onu_id;
	g_key_rpt_confirm_msg.msg_type_id = CA_KEY_REPORT;
    g_key_rpt_confirm_msg.report_type = CA_KEY_CONTROL_CONFIRM;
	g_key_rpt_confirm_msg.frament_no = g_key_prt_fragment_num;
	
	g_key_rpt_confirm_msg.seq_no = CA_PON_AES_INVAILD_VALUE;
	g_key_rpt_confirm_msg.key_idx = CA_PON_AES_INVAILD_VALUE;
	//always use hard coded key 0 (16 x 1)
	memcpy(g_key_rpt_confirm_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);	

	return CA_E_OK;
}

ca_status_t __send_key_rpt_msg_fixed_key(aal_xgpon_key_exchg_kmsg_t *aes_msg)
{
	ca_status_t ret = 0;
	int i = 0;
	ca_uint32_t ploam_retry = 0;
	aal_xgpn_plmp_txbuf_t *txbuf;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;
	XGPN_PLMP_INTERRUPTE0_t reg_cfg;
	XGPN_PLMP_INTERRUPT_t reg_cfg_intl;
	ca_xgpon_key_report_ploam_msg_t key_rpt_msg;
	XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG_t msg_cfg;

#ifdef AES_TIMER
	ca_uint32_t  sfc_lsb = 0, sfc_msb = 0, sfc_new_lsb = 0, sfc_new_msb = 0;
	aal_xgpon_dsfp_super_frame_get(0, &sfc_lsb, &sfc_msb);
#endif
	//printk("func %s:aes_msg.key_ctrl_stat :0x%x\n",__FUNCTION__,aes_msg->key_ctrl_stat);

	memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));
    memset(&key_rpt_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
	
	//send busy msg
	#if 0 
	msg_cfg.wrd = 0;
	msg_cfg.bf.seq = aes_msg->key_ctrl_stat&0xFF;
	msg_cfg.bf.complt_code = 0x2;
	msg_cfg.bf.ok = 0 ;
	msg_cfg.bf.start = 1 ;
    AAL_PON_REG_WRITE(msg_cfg.wrd, XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG);
	#endif
	
	key_rpt_msg.onu_id = g_onu_id;
	key_rpt_msg.msg_type_id = CA_KEY_REPORT;
	key_rpt_msg.seq_no = aes_msg->key_ctrl_stat&0xFF;
	key_rpt_msg.key_idx = (aes_msg->key_ctrl_stat>>16)&0xFF;
    key_rpt_msg.frament_no = g_key_prt_fragment_num;
	key_rpt_msg.report_type = (aes_msg->key_ctrl_stat>>8)&0xFF;
	
	//check TX buffer ready
	ploam_retry = 0;
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    }
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		printk("\r\nPLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }
	
	//fill PLOAM TX buffer 
	reg_addr = XGPN_PLMP_TXBUF_DATA0;
	if (key_rpt_msg.report_type == CA_KEY_CONTROL_NEW)
	{
	    g_key_rpt_new_msg.seq_no = aes_msg->key_ctrl_stat&0xFF;
	    g_key_rpt_new_msg.key_idx = (aes_msg->key_ctrl_stat>>16)&0xFF;
		
		//fill PLOAM TX buffer (kek)
		txbuf = (aal_xgpn_plmp_txbuf_t *)&g_key_rpt_new_msg;
		for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4 - 4); i++){
			reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
				| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
				| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
				| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
			AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
			reg_addr += 4;
		}
	}else if (key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
	{
	    g_key_rpt_confirm_msg.seq_no = aes_msg->key_ctrl_stat&0xFF;
	    g_key_rpt_confirm_msg.key_idx = (aes_msg->key_ctrl_stat>>16)&0xFF;

		//fill PLOAM TX buffer (kek_name)
		txbuf = (aal_xgpn_plmp_txbuf_t *)&g_key_rpt_confirm_msg;
		for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4 - 4); i++){
			reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
			| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
			| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
			| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
			AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
			reg_addr += 4;
		}
	}
	
	//trigger PLOAM send
	reg_ctrl.bf.pkt_rdy = 1;
	AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);
	
	//make sure kek went out
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	while(reg_ctrl.bf.pkt_rdy) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	}
	
#ifdef AES_TIMER
	aal_xgpon_dsfp_super_frame_get(0, &sfc_new_lsb, &sfc_new_msb);

	if (sfc_new_lsb >= sfc_lsb) {
		ca_printf("\r\nfunc %s line %d:From receive key control confirm to send key report confirm PLOAM Use about 0x%08x %08x XGPON cycles( %u us).\r\n",__FUNCTION__,__LINE__,
			sfc_new_msb - sfc_msb, sfc_new_lsb - sfc_lsb, (sfc_new_lsb - sfc_lsb) * 125);
	} else {
		ca_printf("\r\nfunc %s line %d:From receive key control confirm to send key report confirm PLOAM Use about 0x%08x %08x XGPON cycles( %u us).\r\n",__FUNCTION__,__LINE__,
			sfc_new_msb - sfc_msb - 1,	0xffffffff - (sfc_lsb - sfc_new_lsb), (0xffffffff - (sfc_lsb - sfc_new_lsb)) * 125);
	}
#endif
	if(key_rpt_msg.report_type == CA_KEY_CONTROL_NEW)
	{
    	g_rpt_new_key_cnt++ ;
    }else if(key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
	{
        g_rpt_ext_key_cnt++ ;
    }
	return CA_E_OK;
}

static ca_status_t __ploam_key_ctrl_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t key_rpt_type = 0;
    ca_uint8_t key_ctrl_flag = 0;
    aal_xgpon_key_exchg_kmsg_t aes_msg;
    XGPN_PLMP_SW_ACK_t ack_reg;
    ca_xgpon_key_report_ploam_msg_t key_rpt_msg;

    memset(&key_rpt_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
    aes_msg.intr = xgpn_ploam_int;
    aes_msg.sts  = xgpn_ploam_int;
    aes_msg.key_ctrl_stat = AAL_PON_REG_READ(XGPN_PLMP_KEY_CTRL_STAT);
	
	//printk("aes_msg.key_ctrl_stat :0x%x\r\n",aes_msg.key_ctrl_stat);
	ret = __send_key_rpt_msg_fixed_key(&aes_msg);
    CA_RET_VALUE_CHECK(ret);
	
    //notify tasklet to udpate state-machine
    ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_AES_KEY_CTRL, &aes_msg, sizeof(aes_msg));
    if (ret != CA_E_OK) {
		printk("ERROR:func %s:send aes_msg to work queue fail!\n",__FUNCTION__);
        return ret;
    }

    /* wake the handler to process */
    if (aes_use_tasklet) {
        tasklet_schedule(&aal_aes_tasklet);
    } else {
        ret = queue_work(pon_ploam_key_ctrl_wq.pon_wq, &(pon_ploam_key_ctrl_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to key_exchg_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;
}

#endif

#if 0
ca_status_t __send_key_rpt_msg_rand_key(aal_xgpon_key_exchg_kmsg_t *aes_msg)
{
	ca_status_t ret = 0;
	int i = 0;
	ca_uint32_t ploam_retry = 0;
	aal_xgpn_plmp_txbuf_t *txbuf;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;
	XGPN_PLMP_INTERRUPTE0_t reg_cfg;
	XGPN_PLMP_INTERRUPT_t reg_cfg_intl;
	ca_uint8_t key_ctrl_flag = 0;
	XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG_t msg_cfg;
	ca_xgpon_key_report_ploam_msg_t key_rpt_msg;

#ifdef AES_TIMER
	ca_uint32_t  sfc_lsb = 0, sfc_msb = 0, sfc_new_lsb = 0, sfc_new_msb = 0;
	aal_xgpon_dsfp_super_frame_get(0, &sfc_lsb, &sfc_msb);
#endif
	//printk("func %s:aes_msg.key_ctrl_stat :0x%x\n",__FUNCTION__,aes_msg->key_ctrl_stat);

	memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));
	memset(&key_rpt_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
	//send busy msg
	#if 0 
	msg_cfg.wrd = 0;
	msg_cfg.bf.seq = aes_msg->key_ctrl_stat&0xFF;
	msg_cfg.bf.complt_code = 0x2;
	msg_cfg.bf.ok = 0 ;
	msg_cfg.bf.start = 1 ;
    AAL_PON_REG_WRITE(msg_cfg.wrd, XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG);
	#endif
	
	key_rpt_msg.onu_id = g_onu_id;
	key_rpt_msg.msg_type_id = CA_KEY_REPORT;
	key_rpt_msg.seq_no = aes_msg->key_ctrl_stat&0xFF;
	key_rpt_msg.key_idx = (aes_msg->key_ctrl_stat>>16)&0xFF;
    key_rpt_msg.frament_no = g_key_prt_fragment_num;
	key_rpt_msg.report_type = (aes_msg->key_ctrl_stat>>8)&0xFF;
	
	if (key_rpt_msg.report_type == CA_KEY_CONTROL_NEW)
	{
		if(key_rpt_msg.key_idx == 1){
			memcpy(key_rpt_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);
		}
		if(key_rpt_msg.key_idx == 2){
			memcpy(key_rpt_msg.key_fragment, g_encryped_key1, XGPN_AES_KEY_LEN);
		}
		g_key_ctl_new_cnt++;
	}else if (key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
	{
	    if(key_rpt_msg.key_idx == 1){
			memcpy(key_rpt_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
		}
		if(key_rpt_msg.key_idx == 2){
			memcpy(key_rpt_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
		}
		g_key_ctl_cfm_cnt++;
	}
	
	//check TX buffer ready
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    }
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		printk("PLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }
	
	//fill PLOAM TX buffer 
	reg_addr = XGPN_PLMP_TXBUF_DATA0;
    txbuf = (aal_xgpn_plmp_txbuf_t *)&key_rpt_msg;
	for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4 - 4); i++){
		reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
			| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
			| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
			| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}
	
	//trigger PLOAM send
	reg_ctrl.bf.pkt_rdy = 1;
	AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);
	
	//make sure key rpt went out
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	while(reg_ctrl.bf.pkt_rdy) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	}
	
#ifdef AES_TIMER
	aal_xgpon_dsfp_super_frame_get(0, &sfc_new_lsb, &sfc_new_msb);

	if (sfc_new_lsb >= sfc_lsb) {
		ca_printf("\r\nfunc %s line %d:From receive key control confirm to send key report confirm PLOAM Use about 0x%08x %08x XGPON cycles( %u us).\r\n",__FUNCTION__,__LINE__,
			sfc_new_msb - sfc_msb, sfc_new_lsb - sfc_lsb, (sfc_new_lsb - sfc_lsb) * 125);
	} else {
		ca_printf("\r\nfunc %s line %d:From receive key control confirm to send key report confirm PLOAM Use about 0x%08x %08x XGPON cycles( %u us).\r\n",__FUNCTION__,__LINE__,
			sfc_new_msb - sfc_msb - 1,	0xffffffff - (sfc_lsb - sfc_new_lsb), (0xffffffff - (sfc_lsb - sfc_new_lsb)) * 125);
	}
#endif
	if(key_rpt_msg.report_type == CA_KEY_CONTROL_NEW)
	{
		g_rpt_new_key_cnt++ ;
	}else if(key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
	{
		g_rpt_ext_key_cnt++ ;
	}

	return CA_E_OK;
}

static ca_status_t __ploam_key_ctrl_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t key_rpt_type = 0;
    ca_uint8_t key_ctrl_flag = 0;
    aal_xgpon_key_exchg_kmsg_t aes_msg;
    XGPN_PLMP_SW_ACK_t ack_reg;
    ca_xgpon_key_report_ploam_msg_t key_rpt_msg;

    memset(&key_rpt_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
    aes_msg.intr = xgpn_ploam_int;
    aes_msg.sts  = xgpn_ploam_int;
    aes_msg.key_ctrl_stat = AAL_PON_REG_READ(XGPN_PLMP_KEY_CTRL_STAT);
	
	//printk("aes_msg.key_ctrl_stat :0x%x\r\n",aes_msg.key_ctrl_stat);
	ret = __send_key_rpt_msg_rand_key(&aes_msg);
    CA_RET_VALUE_CHECK(ret);
	
    //notify tasklet to udpate state-machine
    ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_AES_KEY_CTRL, &aes_msg, sizeof(aes_msg));
    if (ret != CA_E_OK) {
		printk("ERROR:func %s:send aes_msg to work queue fail!\n",__FUNCTION__);
        return ret;
    }

    /* wake the handler to process */
    if (aes_use_tasklet) {
        tasklet_schedule(&aal_aes_tasklet);
    } else {
        ret = queue_work(pon_ploam_key_ctrl_wq.pon_wq, &(pon_ploam_key_ctrl_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to key_exchg_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;
}

#endif

#if 1
extern ca_uint32_t ptp_timer1;
ca_status_t __send_key_report_msg_isr(ca_xgpon_key_report_ploam_msg_t key_rpt_msg)
{
   // ca_status_t ret = CA_E_OK;
    aal_key_report_frag_t key_rpt_frag;
    aal_xgpn_plmp_key_report_t    key_rpt;
    aal_xgpn_plmp_key_report_mask_t  key_rpt_mask;
//    ca_int32_t wait_timeout =  __KEY_REPORT_TIMEOUT;
	int i = 0;
	ca_uint32_t ploam_retry = 0;
	aal_xgpn_plmp_txbuf_t *txbuf;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;
	XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG_t busy_msg;
	//memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));
	reg_ctrl.wrd = 0;
    key_rpt_mask.wrd =  ~0U;
    memset(&key_rpt_frag,0,sizeof(key_rpt_frag));
    memset(&key_rpt,0,sizeof(key_rpt));
	busy_msg.wrd = 0;
    if(g_wrong_key_index == 0){
        key_rpt_msg.key_idx = key_rpt_msg.key_idx - 1;
    }
	if (key_rpt_msg.report_type == CA_KEY_CONTROL_NEW){
        if(key_rpt_msg.key_idx == 1){
            memcpy(key_rpt_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);
        }
        if(key_rpt_msg.key_idx == 2){
            memcpy(key_rpt_msg.key_fragment, g_encryped_key1, XGPN_AES_KEY_LEN);
        }
    }
    if (key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
    {
        if(key_rpt_msg.key_idx == 1){
            memcpy(key_rpt_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
        }
        if(key_rpt_msg.key_idx == 2){
            memcpy(key_rpt_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
        }
    }
	//check TX buffer ready
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    }
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		//printk("PLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }
	
	//fill PLOAM TX buffer 
	reg_addr = XGPN_PLMP_TXBUF_DATA0;
    txbuf = (aal_xgpn_plmp_txbuf_t *)&key_rpt_msg;
	for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4); i++){
		reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}
	
	//trigger PLOAM send
    reg_ctrl.bf.pkt_rdy = 1;
	AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);
	
	//make sure key rpt went out
	//sw don't know how long hw will send out ploam, so comment below code.
#if 0
    ploam_retry = 0;
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	}
	
    if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		return CA_E_TIMEOUT;
    }
#endif

	if(key_rpt_msg.report_type == CA_KEY_CONTROL_NEW)
	{
		g_rpt_new_key_cnt++ ;
		//ptp_timer1 = AAL_PON_REG_READ(PTP_TIMER);
		//ca_printf("\r\nfunc %s line %d:Send key report new PLOAM. Current PTP_TIMER is 0x%x.\r\n",__FUNCTION__,__LINE__,ptp_timer1);
	}else if(key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
	{
		g_rpt_ext_key_cnt++ ;
		//ptp_timer1 = AAL_PON_REG_READ(PTP_TIMER);
		//ca_printf("\r\nfunc %s line %d:Send key report confirm PLOAM. Current PTP_TIMER is 0x%x.\r\n",__FUNCTION__,__LINE__,ptp_timer1);
	}
    return CA_E_OK;
}

extern ca_uint8_t rand_key[XGPN_AES_KEY_LEN];

static ca_status_t __ploam_key_ctrl_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
	ca_uint8_t key_idx = 0;
	aal_xgpon_dsfp_xgem_key_ctrl_config_t key_ctrl_cfg;
    aal_xgpon_dsfp_xgem_key_ctrl_config_msk_t key_ctrl_mask;
	aal_uc_enc_key_config_msk_t uc_enc_key_msk;
    aal_uc_enc_key_config_t uc_enc_key_cfg;
//	ca_uint8_t key_rpt_type = 0;
    aal_xgpon_key_exchg_kmsg_t aes_msg;
//	XGPN_PLMP_SW_ACK_t ack_reg;
	ca_xgpon_key_report_ploam_msg_t key_rpt_msg;
	memset(&key_rpt_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
	memset(&aes_msg, 0, sizeof(aal_xgpon_key_exchg_kmsg_t));
    aes_msg.intr = xgpn_ploam_int;
    aes_msg.sts  = xgpn_ploam_int;
	
	aes_msg.key_ctrl_stat = AAL_PON_REG_READ(XGPN_PLMP_KEY_CTRL_STAT);
	
	key_rpt_msg.report_type = (aes_msg.key_ctrl_stat>>8)&0xFF;
	if (key_rpt_msg.report_type == CA_KEY_CONTROL_NEW){	
		g_key_ctl_new_cnt ++;
		//ptp_timer1 = AAL_PON_REG_READ(PTP_TIMER);
		//ca_printf("\r\nfunc %s line %d:Receive key control new interrupt. Current key_ctrl_stat is 0x%x. Current PTP_TIMER is 0x%x.\r\n",__FUNCTION__,__LINE__,aes_msg.key_ctrl_stat,ptp_timer1);
    }else if(key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM){
		g_key_ctl_cfm_cnt ++;
		//ptp_timer1 = AAL_PON_REG_READ(PTP_TIMER);
		//ca_printf("\r\nfunc %s line %d:Receive key control confirm interrupt. Current key_ctrl_stat is 0x%x. Current PTP_TIMER is 0x%x.\r\n",__FUNCTION__,__LINE__,aes_msg.key_ctrl_stat,ptp_timer1);
    }
	
	#if 0 
	ack_reg.wrd = AAL_PON_REG_READ(XGPN_PLMP_SW_ACK);
	if(ack_reg.bf.chng_pwr_level == 0){
		ack_reg.bf.seqno = (aes_msg.key_ctrl_stat)&0xFF;
		ack_reg.bf.complt_code = 0x2;
		ack_reg.bf.chng_pwr_level = 0x1;
		AAL_PON_REG_WRITE(ack_reg.wrd, XGPN_PLMP_SW_ACK);
	}
	#endif
	key_rpt_msg.onu_id = g_onu_id;
	key_rpt_msg.msg_type_id = CA_KEY_REPORT;
	key_rpt_msg.seq_no = aes_msg.key_ctrl_stat&0xFF;
	key_rpt_msg.key_idx = (aes_msg.key_ctrl_stat>>16)&0xFF;
    key_rpt_msg.frament_no = g_key_prt_fragment_num;
	key_idx =key_rpt_msg.key_idx;

	uc_enc_key_msk.u32 = ~0U;
	memset(&uc_enc_key_cfg,0,sizeof(uc_enc_key_cfg));

	uc_enc_key_cfg.p_key_active = TRUE;
	uc_enc_key_cfg.p_effective_key_length = XGPN_AES_KEY_LEN;

	key_ctrl_mask.u32 = 0;
	memset(&key_ctrl_cfg,0,sizeof(aal_xgpon_dsfp_xgem_key_ctrl_config_t));

	if (key_rpt_msg.report_type == CA_KEY_CONTROL_NEW)
	{
		//generate new key and set key to HW
		if(key_idx == 1){
            ca_generate_new_kek_and_kek_name(0);
			memcpy(g_key_rpt_new_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);
			memcpy(g_key_rpt_confirm_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
			key_ctrl_mask.s.key0_idx_vld = 1;
			key_ctrl_cfg.key0_idx_vld = 1;
		}
        if(key_idx == 2){
            ca_generate_new_kek_and_kek_name(1);
			memcpy(g_key_rpt_new_msg.key_fragment, g_encryped_key1, XGPN_AES_KEY_LEN);
			memcpy(g_key_rpt_confirm_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
			key_ctrl_mask.s.key1_idx_vld = 1;
			key_ctrl_cfg.key1_idx_vld = 1;
        }
		//enable key for downstream decryption
		ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);
	    CA_RET_VALUE_CHECK(ret);
			
		if(0 != strcmp(olt_type, "ZTE")){//enable us key encryption to match other OLT 
			ret = aal_xgpon_usbg_aes_uni_key_idx_set(0,(key_idx-1));
			CA_RET_VALUE_CHECK(ret);
		}
		
		ret = __send_key_report_msg_isr(key_rpt_msg);
		CA_RET_VALUE_CHECK(ret);
		
		#if 0
		if(0 != strcmp(olt_type, "ZTE")){
			//set new key to hw
			if(key_idx == 1){	
				memcpy(uc_enc_key_cfg.uc_key,g_uc_key0,XGPN_AES_KEY_LEN);
				ret = aal_xgpon_aes_uc_enc_key_set(0,(key_idx-1),uc_enc_key_msk,&uc_enc_key_cfg);
				CA_RET_VALUE_CHECK(ret);
			
				key_ctrl_mask.s.key0_idx_vld = 1;
				key_ctrl_cfg.key0_idx_vld = 1;
				
			} else if(key_idx == 2){
				memcpy(uc_enc_key_cfg.uc_key,g_uc_key1,XGPN_AES_KEY_LEN);
				ret = aal_xgpon_aes_uc_enc_key_set(0,(key_idx-1),uc_enc_key_msk,&uc_enc_key_cfg);
				CA_RET_VALUE_CHECK(ret);
				
				key_ctrl_mask.s.key1_idx_vld = 1;
				key_ctrl_cfg.key1_idx_vld = 1;
			}
			//valid dn direction key
			ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);
		    CA_RET_VALUE_CHECK(ret);
		}
		#endif
		
	}else if(key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM){
		ret = __send_key_report_msg_isr(key_rpt_msg);
		CA_RET_VALUE_CHECK(ret);
	}
	
	//printk("func %s :Send aes_msg.intr :0x%x and key_ctrl_stat :0x%x to workqueue\r\n",
	//	__FUNCTION__,aes_msg.intr,aes_msg.key_ctrl_stat);
	//notify tasklet to udpate state-machine
    ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_AES_KEY_CTRL, &aes_msg, sizeof(aes_msg));
    if (ret != CA_E_OK) {
		printk("ERROR:func %s:send aes_msg to work queue fail!\n",__FUNCTION__);
        return ret;
    }

    /* wake the handler to process */
    if (aes_ctrl_use_tasklet) {
        tasklet_schedule(&aal_aes_key_ctrl_tasklet);
    } else {
        ret = queue_work(pon_ploam_key_ctrl_wq.pon_wq, &(pon_ploam_key_ctrl_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to key_exchg_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;
}

static ca_status_t __ploam_key_tk4_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
//	ca_uint8_t key_rpt_type = 0;
    aal_xgpon_key_exchg_kmsg_t aes_msg;
//	XGPN_PLMP_SW_ACK_t ack_reg;
	ca_xgpon_key_report_ploam_msg_t key_rpt_msg;
	memset(&key_rpt_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
    aes_msg.intr = xgpn_ploam_int;
    aes_msg.sts  = xgpn_ploam_int;
		
	//printk("func %s :Send aes_msg.intr :0x%x to workqueue\r\n",__FUNCTION__,aes_msg.intr);
	//printk("func %s :Send aes_msg.key_ctrl_stat :0x%x to workqueue\r\n",__FUNCTION__,aes_msg.key_ctrl_stat);
	//notify tasklet to udpate state-machine
    ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_AES_KEY_TK4, &aes_msg, sizeof(aes_msg));
    if (ret != CA_E_OK) {
		printk("ERROR:func %s:send aes_msg to work queue fail!\n",__FUNCTION__);
        return ret;
    }

    /* wake the handler to process */
    if (aes_tk4_use_tasklet) {
        tasklet_schedule(&aal_aes_key_tk4_tasklet);
    } else {
        ret = queue_work(pon_ploam_key_tk4_wq.pon_wq, &(pon_ploam_key_tk4_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to key_tk4_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;
}

static ca_status_t __ploam_key_tk5_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
	//ca_uint8_t key_rpt_type = 0;
    aal_xgpon_key_exchg_kmsg_t aes_msg;
	//XGPN_PLMP_SW_ACK_t ack_reg;
	ca_xgpon_key_report_ploam_msg_t key_rpt_msg;
	memset(&key_rpt_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
    aes_msg.intr = xgpn_ploam_int;
    aes_msg.sts  = xgpn_ploam_int;
	
	//printk("func %s :Send aes_msg.intr :0x%x to workqueue\r\n",__FUNCTION__,aes_msg.intr);
	//printk("func %s :Send aes_msg.key_ctrl_stat :0x%x to workqueue\r\n",__FUNCTION__,aes_msg.key_ctrl_stat);
	//notify tasklet to udpate state-machine
    ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_AES_KEY_TK5, &aes_msg, sizeof(aes_msg));
    if (ret != CA_E_OK) {
		printk("ERROR:func %s:send aes_msg to work queue fail!\n",__FUNCTION__);
        return ret;
    }

    /* wake the handler to process */
    if (aes_tk5_use_tasklet) {
        tasklet_schedule(&aal_aes_key_tk4_tasklet);
    } else {
        ret = queue_work(pon_ploam_key_tk5_wq.pon_wq, &(pon_ploam_key_tk5_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to key_tk5_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;
}

#endif

#if 0

ca_status_t __send_key_report_msg_isr(ca_xgpon_key_report_ploam_msg_t key_rpt_msg)
{
    ca_status_t ret = CA_E_OK;
    aal_key_report_frag_t key_rpt_frag;
    aal_xgpn_plmp_key_report_t    key_rpt;
    aal_xgpn_plmp_key_report_mask_t  key_rpt_mask;
    ca_int32_t wait_timeout =  __KEY_REPORT_TIMEOUT;
	int i = 0;
	ca_uint32_t ploam_retry = 0;
	aal_xgpn_plmp_txbuf_t *txbuf;
	XGPN_PLMP_TXBUF_CTRL_t reg_ctrl;
	aal_reg_xgpn_plmp_txbuf_data_t reg_data;
	ca_uint32_t reg_addr = XGPN_PLMP_TXBUF_DATA0;
	XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG_t busy_msg;
	
	memset(&reg_ctrl, 0, sizeof(XGPN_PLMP_TXBUF_CTRL_t));
    key_rpt_mask.wrd =  ~0U;
    memset(&key_rpt_frag,0,sizeof(key_rpt_frag));
    memset(&key_rpt,0,sizeof(key_rpt));
	busy_msg.wrd = 0;

    if(g_wrong_key_index == 0){
        key_rpt_msg.key_idx = key_rpt_msg.key_idx - 1;
    }
	if (key_rpt_msg.report_type == CA_KEY_CONTROL_NEW){
        if(key_rpt_msg.key_idx == 1){
            memcpy(key_rpt_msg.key_fragment, g_encryped_key0, XGPN_AES_KEY_LEN);
        }
        if(key_rpt_msg.key_idx == 2){
            memcpy(key_rpt_msg.key_fragment, g_encryped_key1, XGPN_AES_KEY_LEN);
        }
    }
    if (key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
    {
        if(key_rpt_msg.key_idx == 1){
            memcpy(key_rpt_msg.key_fragment, g_kek_name0, XGPN_AES_KEY_LEN);
        }
        if(key_rpt_msg.key_idx == 2){
            memcpy(key_rpt_msg.key_fragment, g_kek_name1, XGPN_AES_KEY_LEN);
        }
    }
	
	//check TX buffer ready
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    while(reg_ctrl.bf.pkt_rdy && ploam_retry++ < PLOAM_TX_RETRY_MAX) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
    }
	if(ploam_retry >= PLOAM_TX_RETRY_MAX) {
		printk("PLOAM TX buffer not ready for refill\n"); /* H/W should always be ready for Tx */
		return CA_E_TIMEOUT;
    }
	
	//fill PLOAM TX buffer 
	reg_addr = XGPN_PLMP_TXBUF_DATA0;
    txbuf = (aal_xgpn_plmp_txbuf_t *)&key_rpt_msg;
	for (i = 0; i < (XGPN_PLMP_TXBUF_SIZE/4 - 4); i++){
		reg_data.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
	    	| ((ca_uint32_t)(txbuf->data[i*4 + 3]));
		AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);
		reg_addr += 4;
	}
	
	//trigger PLOAM send
    reg_ctrl.bf.pkt_rdy = 1;
	AAL_PON_REG_WRITE(reg_ctrl.wrd, XGPN_PLMP_TXBUF_CTRL);
	
	//make sure key rpt went out
	reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	while(reg_ctrl.bf.pkt_rdy) {
		reg_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);
	}
	
	if(key_rpt_msg.report_type == CA_KEY_CONTROL_NEW)
	{
    	g_rpt_new_key_cnt++ ;
    }else if(key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM)
	{
        g_rpt_ext_key_cnt++ ;
    }
    return CA_E_OK;
}

static ca_status_t __ploam_key_ctrl_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
	ca_uint8_t key_rpt_type = 0;
    aal_xgpon_key_exchg_kmsg_t aes_msg;
	XGPN_PLMP_SW_ACK_t ack_reg;
	ca_xgpon_key_report_ploam_msg_t key_rpt_msg;
	memset(&key_rpt_msg, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
    aes_msg.intr = xgpn_ploam_int;
    aes_msg.sts  = xgpn_ploam_int;
    aes_msg.key_ctrl_stat = AAL_PON_REG_READ(XGPN_PLMP_KEY_CTRL_STAT);

	#if 0 
	ack_reg.wrd = AAL_PON_REG_READ(XGPN_PLMP_SW_ACK);
	if(ack_reg.bf.chng_pwr_level == 0){
		ack_reg.bf.seqno = (aes_msg.key_ctrl_stat)&0xFF;
		ack_reg.bf.complt_code = 0x2;
		ack_reg.bf.chng_pwr_level = 0x1;
		AAL_PON_REG_WRITE(ack_reg.wrd, XGPN_PLMP_SW_ACK);
	}
	#endif
	
	key_rpt_msg.onu_id = g_onu_id;
	key_rpt_msg.msg_type_id = CA_KEY_REPORT;
	key_rpt_msg.seq_no = aes_msg.key_ctrl_stat&0xFF;
	key_rpt_msg.key_idx = (aes_msg.key_ctrl_stat>>16)&0xFF;
    key_rpt_msg.frament_no = g_key_prt_fragment_num;
	key_rpt_msg.report_type = (aes_msg.key_ctrl_stat>>8)&0xFF;

	if (key_rpt_msg.report_type == CA_KEY_CONTROL_NEW){	
		g_key_ctl_new_cnt ++;
    }else if(key_rpt_msg.report_type == CA_KEY_CONTROL_CONFIRM){
		g_key_ctl_cfm_cnt ++;
    }
	
	ret = __send_key_report_msg_isr(key_rpt_msg);
	CA_RET_VALUE_CHECK(ret);
	
	//notify tasklet to udpate state-machine
    ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_AES_KEY_CTRL, &aes_msg, sizeof(aes_msg));
    if (ret != CA_E_OK) {
		printk("ERROR:func %s:send aes_msg to work queue fail!\n",__FUNCTION__);
        return ret;
    }

    /* wake the handler to process */
    if (aes_use_tasklet) {
        tasklet_schedule(&aal_aes_tasklet);
    } else {
        ret = queue_work(pon_ploam_key_ctrl_wq.pon_wq, &(pon_ploam_key_ctrl_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to key_exchg_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;
}



static ca_status_t __ploam_registration_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_rgst_kmsg_t rgst_msg;
	
    rgst_msg.intr = xgpn_ploam_int;
		
    ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_RGST, &rgst_msg, sizeof(rgst_msg));
    if (ret != CA_E_OK) {
        return ret;
    }

    /* wake the handler to process */
	if (ploam_rgst_use_tasklet) {
		tasklet_schedule(&aal_ploam_rgst_tasklet);
	} else {
		ret = queue_work(pon_ploam_rgst_wq.pon_wq, &(pon_ploam_rgst_wq.work_info.work));
		if (ret) {
			PON_DBG("add work to pon_ploam_wq fail, already exist.\n");
			return CA_E_OK;
		}
	}

    return ret;
}
#endif

static ca_status_t __ploam_rx_isr(ca_uint32_t xgpn_ploam_int)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t read_buf_idx[2] = {0};
    ca_uint32_t read_buf_num = 0;
    ca_uint32_t i = 0;
    XGPN_PLMP_RXBUF0_STAT_t  rx_stat[2];
    aal_xgpon_ploam_kmsg_t   msg;

    rx_stat[0].wrd  = AAL_PON_REG_READ(XGPN_PLMP_RXBUF0_STAT);
    rx_stat[1].wrd  = AAL_PON_REG_READ(XGPN_PLMP_RXBUF1_STAT);

    if (rx_stat[0].bf.pkt_rdy) {
        read_buf_idx[read_buf_num] = 0;
        read_buf_num ++;
    }

    if (rx_stat[1].bf.pkt_rdy) {
        read_buf_idx[read_buf_num] = 1;
        read_buf_num ++;
    }

    if (2 == read_buf_num) { /* coming two pkt */
        if (!rx_stat[0].bf.turn && rx_stat[1].bf.turn) {/* Read buf#0 firstly */
            read_buf_idx[0] = 1;
            read_buf_idx[1] = 0;
        }
    }

    /* Read the Rx buffers and then send to handler */
    for (i = 0; i < read_buf_num; i++) {
        if(rx_stat[read_buf_idx[i]].bf.mic_err) {
            aal_ploam_err_cnt++;
            /* Clear buf ready to let next pkt incoming */
            AAL_PON_REG_WRITE(0,
                read_buf_idx[i] ? XGPN_PLMP_RXBUF1_STAT : XGPN_PLMP_RXBUF0_STAT);
            
#ifdef CONFIG_PROC_FS
            if (PON_DBG_GPON_PLOAM_RX & ca_pon_debug) {
                ca_printf("Rx PLOAM with MIC error in buf %d, stats = %08x, data = %08X %08X %08X, MIC = %08X %08X\n", 
                    read_buf_idx[i],
                    rx_stat[read_buf_idx[i]],
                    AAL_PON_REG_READ(XGPN_PLMP_RXBUF + 48 * read_buf_idx[i]),
                    AAL_PON_REG_READ(XGPN_PLMP_RXBUF + 48 * read_buf_idx[i] + 4),
                    AAL_PON_REG_READ(XGPN_PLMP_RXBUF + 48 * read_buf_idx[i] + 8),
                    AAL_PON_REG_READ(XGPN_PLMP_RXBUF + 48 * read_buf_idx[i] + 40),
                    AAL_PON_REG_READ(XGPN_PLMP_RXBUF + 48 * read_buf_idx[i] + 44));
            }
#endif
            continue;
        }

        memset(&msg, 0, sizeof(msg));

        /* Read buffer */
        ret = aal_ploam_rxbuf_get(AAL_DEFAUL_DEV_ID, read_buf_idx[i], FALSE,
            (aal_xgpn_plmp_rxbuf_t *)&msg.ploam_pkt);

        /* Clear buf ready to let next pkt incoming */
        AAL_PON_REG_WRITE(0,
            read_buf_idx[i] ? XGPN_PLMP_RXBUF1_STAT : XGPN_PLMP_RXBUF0_STAT);

        msg.intr = xgpn_ploam_int;
        msg.len  = XGPN_PLMP_RXBUF_SIZE;
        /* Send to handler */
        ret = aal_pon_kmsg_send(AAL_PON_KMSG_CHN_PLOAM, &msg, sizeof(msg));
        if (ret != CA_E_OK) {
            return CA_E_FULL;
        }
    }

    /* wake the handler to process */
    if (ploam_use_tasklet) {
        tasklet_schedule(&aal_ploam_tasklet);
    } else {
        ret = queue_work(pon_ploam_wq.pon_wq, &(pon_ploam_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to pon_ploam_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;

}


/** __xgpon_ploam_isr
 *@brief: handle PLOAM
 *@param [in] intr_id: refer to aal_xgpon_intr_t
 */
static ca_status_t __xgpon_ploam_isr(ca_uint32_t intr_id, ca_uint32_t intr_sts)
{
    ca_status_t  ret = CA_E_OK;

    PON_DBG("intr_id %d, intr_sts %#x\n", intr_id, intr_sts);

    /* ONLY handle the enabled PLOAM interrupt */
    if (intr_sts & 0x80) { //XGPN_PLMP_INTR_MASK_KEY_CTRL | XGPN_PLMP_INTR_MASK_TK5_TIMEOUT | XGPN_PLMP_INTR_MASK_TK4_TIMEOUT)
    /* AES Key exchange PLOAM interrupts */
        ret = __ploam_key_ctrl_isr(intr_sts & 0x80);
    }
	/* ONLY handle the enabled PLOAM interrupt */
    if (intr_sts & 0x20) { //XGPN_PLMP_INTR_MASK_KEY_CTRL | XGPN_PLMP_INTR_MASK_TK5_TIMEOUT | XGPN_PLMP_INTR_MASK_TK4_TIMEOUT)
    /* AES Key exchange PLOAM interrupts */
        ret = __ploam_key_tk4_isr(intr_sts & 0x20);
    }
	/* ONLY handle the enabled PLOAM interrupt */
    if (intr_sts & 0x40) { //XGPN_PLMP_INTR_MASK_KEY_CTRL | XGPN_PLMP_INTR_MASK_TK5_TIMEOUT | XGPN_PLMP_INTR_MASK_TK4_TIMEOUT)
    /* AES Key exchange PLOAM interrupts */
        ret = __ploam_key_tk5_isr(intr_sts & 0x40);
    }
#if 0
	if (intr_sts & 0x1000) { //XGPN_PLMP_INTR_MASK_KEY_CTRL | XGPN_PLMP_INTR_MASK_TK5_TIMEOUT | XGPN_PLMP_INTR_MASK_TK4_TIMEOUT)
    /* AES Key exchange PLOAM interrupts */
        ret = __ploam_registration_isr(intr_sts & 0x1000);
    }
#endif
    if (intr_sts & 0xC0000000) {
    /* PLOAM Rx packets interrupts */
        ret = __ploam_rx_isr(intr_sts & 0xC0000000);
    } else {
        /* others, now no other PLOAMs enabled */
    }

    return ret;
}

#if 0
static ca_status_t __pon_glb_opt_isr(ca_uint32_t intr_id, ca_uint32_t intr_sts)
{
    ca_status_t  ret = CA_E_OK;
	GLOBAL_OPT_MODULE_STATUS_t opt_stas;

    PON_DBG("intr_id %d, intr_sts %#x\n", intr_id, intr_sts);
	printk("GLOBAL_OPT_INTERRUPT!!\r\n");

    if (intr_sts & 0x1) {
		opt_stas.wrd = AAL_GLB_REG_READ(GLOBAL_OPT_MODULE_STATUS);
		printk("opt_stas.bf.opt_rx_los :0x%x\r\n",opt_stas.bf.opt_rx_los);
		if(!opt_stas.bf.opt_rx_los)
		{
        	ret = aal_xgpon_mac_reset(0, AAL_XGPON_RESET_FLAG_ALL);
		}
    }
    return ret;
}
#endif

static ca_status_t __pon_misc_isr(ca_uint32_t intr_id, ca_uint32_t intr_sts)
{
    ca_status_t            ret = CA_E_OK;
    aal_xgpon_misc_kmsg_t  msg;

    msg.intr_id = intr_id;
    msg.sts     = intr_sts;

    switch (intr_id) {

        case AAL_XGPON_INTR_AES_PLOAM:

            break;

        case AAL_XGPON_INTR_AES:

            break;

        case AAL_XGPON_INTR_RXCTL:

            break;

        case AAL_XGPON_INTR_BWMP:

            break;

        case AAL_XGPON_INTR_USBG:
            break;

        case AAL_XGPON_INTR_DSFP:

            break;

        case AAL_XGPON_INTR_FEC:

            break;

        default:
            break;
    }

    /* Send to handler */
    ret = aal_pon_kmsg_send(AAL_PON_KSMG_CHN_MISC, &msg, sizeof(msg));
    if (ret != CA_E_OK) {
        return CA_E_FULL;
    }

    /* wake the handler to process */
    ret = queue_work(pon_misc_wq.pon_wq, &(pon_misc_wq.work_info.work));
    if (ret) {
        return CA_E_CONFLICT;
    }

    return ret;

}

void aal_misc_wq_proc(struct work_struct *wq)
{
    aal_xgpon_misc_kmsg_t  msg;
    ca_status_t            ret =  CA_E_OK;

    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KSMG_CHN_MISC, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        if  (!msg.sts) {
            PON_DBG("err: rx actp msg but with null intr \n");
            continue;
        }

        /* handle the interrupt */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KSMG_CHN_MISC]) {
            (*g_xgpon_intr_handlers[AAL_PON_KSMG_CHN_MISC])(msg.intr_id, msg.sts, 0, NULL);
        }

    }

}

void aal_ploam_wq_proc(struct work_struct *wq)
{
    aal_xgpon_ploam_kmsg_t   msg;
    ca_status_t              ret =  CA_E_OK;
    //pon_work_info_t         *qwk = container_of(wq, pon_work_info_t, work);

    memset(&msg, 0, sizeof(msg));

    while (1) {
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_PLOAM, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        if  (!msg.intr) {
            PON_DBG("err: rx ploam msg with null intr \n");
            continue;
        }

        PON_DBG("Rx PLOAM kmsg sts %#x len %d\n",
            msg.intr, msg.len);

        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_PLOAM]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_PLOAM])(msg.intr, msg.intr, msg.len, (void *)(&msg.ploam_pkt[0]));
        }
    }

}

void aal_ploam_key_exchg_wq_proc(struct work_struct *wq)
{
    aal_xgpon_key_exchg_kmsg_t  msg;
    ca_status_t                 ret =  CA_E_OK;
    //pon_work_info_t            *qwk = container_of(wq, pon_work_info_t, work);


    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_AES_KEY_CTRL, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        PON_DBG("AES msg rx : INTR %#x, STS %#x, KEY_CTRL_STS %#x,\n",
            msg.intr, msg.sts, msg.key_ctrl_stat);

        if  (!msg.intr) {
            PON_DBG("err: rx aes key exchange msg with null intr \n");
            continue;
        }
		if  (!msg.key_ctrl_stat) {
            printk("Warning: rx key_ctrl_stat is 0x%x \n",msg.key_ctrl_stat);
        }
        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_CTRL]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_CTRL])(msg.intr, msg.sts, msg.key_ctrl_stat, NULL);
        }
    }

}
void aal_ploam_key_tk4_wq_proc(struct work_struct *wq)
{
    aal_xgpon_key_exchg_kmsg_t  msg;
    ca_status_t                 ret =  CA_E_OK;
    //pon_work_info_t            *qwk = container_of(wq, pon_work_info_t, work);


    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_AES_KEY_TK4, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        PON_DBG("AES msg rx : INTR %#x, STS %#x, KEY_CTRL_STS %#x,\n",
            msg.intr, msg.sts, msg.key_ctrl_stat);

        if  (!msg.intr) {
            PON_DBG("err: rx aes key exchange msg with null intr \n");
            continue;
        }
		if  (!msg.key_ctrl_stat) {
            printk("Warning: rx key_ctrl_stat is 0x%x \n",msg.key_ctrl_stat);
        }
        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_TK4]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_TK4])(msg.intr, msg.sts, msg.key_ctrl_stat, NULL);
        }
    }

}

void aal_ploam_key_tk5_wq_proc(struct work_struct *wq)
{
    aal_xgpon_key_exchg_kmsg_t  msg;
    ca_status_t                 ret =  CA_E_OK;
    //pon_work_info_t            *qwk = container_of(wq, pon_work_info_t, work);


    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_AES_KEY_TK5, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        PON_DBG("AES msg rx : INTR %#x, STS %#x, KEY_CTRL_STS %#x,\n",
            msg.intr, msg.sts, msg.key_ctrl_stat);

        if  (!msg.intr) {
            PON_DBG("err: rx aes key exchange msg with null intr \n");
            continue;
        }
		if  (!msg.key_ctrl_stat) {
            printk("Warning: rx key_ctrl_stat is 0x%x \n",msg.key_ctrl_stat);
        }
        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_TK5]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_AES_KEY_TK5])(msg.intr, msg.sts, msg.key_ctrl_stat, NULL);
        }
    }

}

#if 0
void aal_ploam_rgst_wq_proc(struct work_struct *wq)
{
    aal_xgpon_rgst_kmsg_t  msg;
    ca_status_t                 ret =  CA_E_OK;
    //pon_work_info_t            *qwk = container_of(wq, pon_work_info_t, work);


    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_RGST, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        PON_DBG("RGST msg rx : INTR %#x\n",msg.intr);

        if  (!msg.intr) {
            PON_DBG("err: rx rgst msg with null intr \n");
            continue;
        }

        /* handle the interrupt
         * NOTE: we re-use AES_PLOAM interrupt for AES key exchange PLOAM handle,
         *       the INTERRUPT is really PLOAM interrupt
         */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_RGST]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_RGST])(msg.intr, 0, 0, NULL);
        }
    }
}
#endif

void aal_act_wq_proc(struct work_struct *wq)
{
    aal_xgpon_actp_kmsg_t  msg;
    ca_status_t            ret =  CA_E_OK;
    //pon_work_info_t       *qwk = container_of(wq, pon_work_info_t, work);

    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = aal_pon_kmsg_recv(AAL_PON_KMSG_CHN_ACTP, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        PON_DBG("ACTP msg rx : INTR %#x, ST_CHG %#x, STS %#x\n",
            msg.intr, msg.st_chg, msg.sts);

        if  (!msg.intr) {
            PON_DBG("err: rx actp msg but with null intr \n");
            continue;
        }

        /* handle the interrupt */
        if (NULL != g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_ACTP]) {
            (*g_xgpon_intr_handlers[AAL_PON_KMSG_CHN_ACTP])(msg.intr, msg.st_chg, msg.sts, NULL);
        }
    }

}

void __misc_intr_handler(ca_uint32_t intr_id, ca_uint32_t intr_sts, ca_uint32_t param, void *cookie)
{
    //ca_status_t ret = CA_E_OK;

    switch (intr_id) {

        case AAL_XGPON_INTR_AES_PLOAM:

            break;

        case AAL_XGPON_INTR_AES:

            break;

        case AAL_XGPON_INTR_RXCTL:

            break;

        case AAL_XGPON_INTR_BWMP:

            break;

        case AAL_XGPON_INTR_USBG:
            break;

        case AAL_XGPON_INTR_DSFP:

            break;

        case AAL_XGPON_INTR_FEC:

            break;

        default:
            break;
    }
	//return ret;
}

/* Framework to handle the PON interrupt */
#define DO_XGPON_ISR(intr_id)  do {  \
        ca_uint32_t intre = 0;       \
        ca_uint32_t intrs = 0;       \
        ca_status_t ret   = CA_E_OK; \
        intre = AAL_PON_REG_READ(__xgpon_intr_tbl[intr_id].ena_reg_addr);  \
        intrs = AAL_PON_REG_READ(__xgpon_intr_tbl[intr_id].src_reg_addr);  \
        AAL_PON_REG_WRITE(0, __xgpon_intr_tbl[intr_id].ena_reg_addr);      \
        AAL_PON_REG_WRITE(intrs & intre, __xgpon_intr_tbl[intr_id].src_reg_addr);  \
        __xgpon_intr_tbl[intr_id].count ++;                                \
        if (NULL != aal_xgpon_intr_isr[intr_id]) {                         \
            ret =(*aal_xgpon_intr_isr[intr_id])(intr_id, intrs & intre);\
            if (CA_E_OK != ret) \
                __xgpon_intr_tbl[intr_id].fail_count ++;   \
            else \
                __xgpon_intr_tbl[intr_id].handle_count ++;   \
        }                                                                  \
        AAL_PON_REG_WRITE(intre, __xgpon_intr_tbl[intr_id].ena_reg_addr);  \
    }while(0)

void aal_xgpon_intr_handler(ca_device_id_t device_id, ca_uint32_t irq, ca_uint32_t xgpon_int)
{
    /* handler all XGPON interrupt here */
    /* TODO: add intr type here */

    /* Handle PLOAM interrupt 1stly */
    if (xgpon_int & XGPON_GLB_INTR_MASK_PLMP) {
        DO_XGPON_ISR(AAL_XGPON_INTR_PLOAM);
    }

    /* Handle ACTP interrupt 2ndly */
    if (xgpon_int & XGPON_GLB_INTR_MASK_ACTP){
        DO_XGPON_ISR(AAL_XGPON_INTR_ACTP);
    }

    /* Handle USBG interrupt */
    if (xgpon_int & XGPON_GLB_INTR_MASK_USBG){
        DO_XGPON_ISR(AAL_XGPON_INTR_USBG);
    }

    /* Handle DSFP interrupt */
    if (xgpon_int & XGPON_GLB_INTR_MASK_DSFP){
        DO_XGPON_ISR(AAL_XGPON_INTR_DSFP);
    }

    if (xgpon_int & XGPON_GLB_INTR_MASK_BWMP){
        DO_XGPON_ISR(AAL_XGPON_INTR_BWMP);
    }

    if (xgpon_int & XGPON_GLB_INTR_MASK_AES){
        DO_XGPON_ISR(AAL_XGPON_INTR_AES);
    }

    if (xgpon_int & XGPON_GLB_INTR_MASK_RXCTL){
        DO_XGPON_ISR(AAL_XGPON_INTR_RXCTL);
    }

    if (xgpon_int & XGPON_GLB_INTR_MASK_XGFEC){
        DO_XGPON_ISR(AAL_XGPON_INTR_FEC);
    }

    if (xgpon_int & XGPON_GLB_INTR_MASK_AES_PLMP){
        DO_XGPON_ISR(AAL_XGPON_INTR_AES_PLOAM);
    }
}

/**
 * @aal_xgpon_intr_init
 * Initialize XGPON interrupt config and handler
 */
ca_status_t aal_xgpon_intr_init(ca_device_id_t  device_id)
{
    ca_uint32_t  idx  = 0;
    ca_status_t  ret  = CA_E_OK;

    /* Disable Top interrupts  */
    AAL_PON_REG_WRITE (0, XGPN_XGPN_GLB_INTERRUPT0E);
    idx = AAL_PON_REG_READ(XGPN_XGPN_GLB_INTERRUPT0); /* read clear */

    /* Global var init */
    memset(&aal_xgpon_intr_isr[0], 0, sizeof(aal_xgpon_intr_isr));
    ret = aal_pon_kmsg_init(AAL_PON_KMSG_CHN_NUM);
    if (CA_E_OK != ret) {
        ca_printf("aal_pon_kmsg_init return %d.\n", ret);
        return ret;
    } else {
        PON_DBG("ca-pon: init kmsg return %d.\n", ret);
    }

    /* Hook the interrupt by Platform interfaces */
    (void)aal_xgpon_intr_isr_reg(AAL_XGPON_INTR_ACTP,  __xgpon_actp_isr);
    (void)aal_xgpon_intr_isr_reg(AAL_XGPON_INTR_PLOAM, __xgpon_ploam_isr);
    (void)aal_xgpon_intr_isr_reg(AAL_XGPON_INTR_USBG,  __pon_misc_isr);
//	(void)aal_xgpon_intr_isr_reg(AAL_PON_GLOBAL_INTR_OPT, __pon_glb_opt_isr);

    (void)aal_xgpon_intr_handler_reg(AAL_PON_KMSG_CHN_ACTP, (aal_intr_msg_handler_t)aal_xgpn_act_intr_handler);
    (void)aal_xgpon_intr_handler_reg(AAL_PON_KMSG_CHN_AES_KEY_CTRL, (aal_intr_msg_handler_t)xgpon_aes_key_ctrl_handler);
	(void)aal_xgpon_intr_handler_reg(AAL_PON_KMSG_CHN_AES_KEY_TK4, (aal_intr_msg_handler_t)xgpon_aes_key_tk4_handler);
	(void)aal_xgpon_intr_handler_reg(AAL_PON_KMSG_CHN_AES_KEY_TK5, (aal_intr_msg_handler_t)xgpon_aes_key_tk5_handler);
	//(void)aal_xgpon_intr_handler_reg(AAL_PON_KMSG_CHN_RGST, (aal_intr_msg_handler_t)xgpon_ploam_rgst_handler);
    (void)aal_xgpon_intr_handler_reg(AAL_PON_KMSG_CHN_PLOAM, (aal_intr_msg_handler_t)xgpon_ploam_rx_handler);
    (void)aal_xgpon_intr_handler_reg(AAL_PON_KSMG_CHN_MISC, (aal_intr_msg_handler_t)__misc_intr_handler);

    /* Enable used interrupts and clear interrupt status  */
    for (idx = 0; idx < AAL_XGPON_INTR_NUM; idx++) {
        aal_xgpon_intr_enable_set(device_id, idx, 0); /* disable firstly */
        aal_xgpon_intr_clr(device_id, idx, __xgpon_intr_tbl[idx].ena_bmp); /* clear status */
        aal_xgpon_intr_enable_set(device_id, idx, __xgpon_intr_tbl[idx].ena_bmp); /* enable MUST interrupt */
        if (__xgpon_intr_tbl[idx].ena_bmp) {
            PON_DBG("ca-pon: Enable XGPON %s interrupt : 0x%08x\n",
                __xgpon_intr_tbl[idx].name, __xgpon_intr_tbl[idx].ena_bmp);
        }
    }

    /* Enable TOP interrupt */
    AAL_PON_REG_WRITE (XGPON_INTR_TOP_ENA_DEFAULT, XGPN_XGPN_GLB_INTERRUPT0E);

    return CA_E_OK;

}


#ifdef __KERNEL__
EXPORT_SYMBOL(aal_xgpon_intr_name_get);
EXPORT_SYMBOL(aal_xgpon_intr_status_get);
EXPORT_SYMBOL(aal_xgpon_intr_enable_get);
EXPORT_SYMBOL(aal_xgpon_intr_enable_set);
EXPORT_SYMBOL(aal_xgpon_intr_clr);
EXPORT_SYMBOL(aal_xgpon_intr_init);
EXPORT_SYMBOL(aal_xgpon_intr_handler_reg);
EXPORT_SYMBOL(aal_xgpon_intr_handler_unreg);
EXPORT_SYMBOL(aal_xgpon_intr_info_get);
EXPORT_SYMBOL(aal_xgpon_intr_handler);
#endif

