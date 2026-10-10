/******************************************************************************/
/* This file contains unpublished documentation and software                */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure,    */
/* in whole or in part, of the information in this file without a                */
/* written consent of an officer of Cortina Systems Incorporated is          */
/* strictly prohibited.                                                                        */
/* Copyright (c) 2018 by Cortina Systems Incorporated.                        */
/******************************************************************************/
/*
  * aal_gpon_intr.c
 *
 * Brief  : GPON Interrupt config and handler AAL driver
 * History:
 *          2018-11-23 initial @Corey
 *          2018-12-10 change file support gpon mode
 *
 * GPON Interrupt config and handler AAL driver
 */
#ifdef CONFIG_NE_PON
#include <soc/cortina/registers.h>
#include "osal_common.h"
#include <linux/types.h>
#include <linux/resource.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/init.h>
#include <linux/kfifo.h>
#include "ca_types.h"
#include "ca_event.h"

#include "aal_pon.h"
#include "aal_gpon.h"
#include "aal_gpon_intr.h"
#ifdef CONFIG_LUNA_G3_SERIES
#include "gpon.h"
#include <scfg.h>
#endif

ca_int8_t* gpon_act_state[AAL_GPON_STATE_MAX + 1] = 
    {"Initial", "Standby", "SerNumber", "Ranging", "Operation", "POPUP", "EmrgStop", "Unknow"};

#define AAL_GPON_KMSG_FIFO_SIZE  4800

/* GPON MAC top interrupt mask bit */
#define AAL_GPON_INTR_BIT   (0x0001 << 0)
#define AAL_GPON_INTR2_BIT  (0x0001 << 1)
#define AAL_GPON_INTR3_BIT  (0x0001 << 2)
#define AAL_GPON_INTR4_BIT  (0x0001 << 3)

/* GPON MAC INTERRUPT ENABLE DEFAULT MASK*/
#define GPON_MAC_GPON_INT_TOP_ENA_DEF   0xF //just enable GPON_MAC_GPON_interrupt
#define GPON_MAC_GPON_INT_ENA_DEF       0xC00A1FFF //above all bit
#define GPON_MAC_GPON_INT2_ENA_DEF      0
#define GPON_MAC_GPON_INT3_ENA_DEF      0
#define GPON_MAC_GPON_INT4_ENA_DEF      0


extern ca_uint32_t gpon_intr_tasklet;
extern pon_wq_t    gpon_intr_wq;


struct kfifo aal_gpon_kfifo[AAL_GPON_KMSG_CHN_NUM];


static aal_intr_callback_t    sg_aal_gpon_intr_isr[AAL_GPON_INTR_NUM];
static aal_intr_msg_handler_t sg_gpon_intr_handlers[AAL_GPON_KMSG_CHN_NUM];
static aal_gpon_intr_t        sg_gpon_intr_tbl[] = {
    { "INTR",  GPON_MAC_GPON_int_en,  GPON_MAC_GPON_interrupt,  GPON_MAC_GPON_INT_ENA_DEF,  0, 0, 0},
    { "INTR2", GPON_MAC_GPON_int2_en, GPON_MAC_GPON_interrupt2, GPON_MAC_GPON_INT2_ENA_DEF, 0, 0, 0},
    { "INTR3", GPON_MAC_GPON_int3_en, GPON_MAC_GPON_interrupt3, GPON_MAC_GPON_INT3_ENA_DEF, 0, 0, 0},
    { "INTR4", GPON_MAC_GPON_int4_en, GPON_MAC_GPON_interrupt4, GPON_MAC_GPON_INT4_ENA_DEF, 0, 0, 0},
};


static ca_status_t __kmsg_recv(ca_uint32_t msg_chn, void * msg, ca_uint32_t msg_len);
static ca_status_t __do_intr_isp(ca_uint32_t intr_id);
static void __intr_tasklet_action(unsigned long t);


DECLARE_TASKLET(aal_gpon_intr_tasklet, __intr_tasklet_action, 0);

void aal_gpon_intr_handler(
    ca_device_id_t device_id,
    ca_uint32_t    gpon_int)
{
    if (gpon_int & AAL_GPON_INTR_BIT)
    {
        __do_intr_isp(AAL_GPON_INTR);
    }

    if (gpon_int & AAL_GPON_INTR2_BIT)
    {
        __do_intr_isp(AAL_GPON_INTR2);
    }

    if (gpon_int & AAL_GPON_INTR3_BIT)
    {
        __do_intr_isp(AAL_GPON_INTR3);
    }

    if (gpon_int & AAL_GPON_INTR4_BIT)
    {
        __do_intr_isp(AAL_GPON_INTR4);
    }
}


void aal_gpon_intr_wq_proc(struct work_struct *wq)
{
    ca_status_t  ret =  CA_E_OK;
    aal_gpon_intr_kmsg_t  msg = {0};
   
    while (1) {
        memset(&msg, 0, sizeof(msg));
        ret = __kmsg_recv(AAL_GPON_KMSG_CHN_INTR, &msg, sizeof(msg));
        if (CA_E_OK != ret) {
            return;
        }

        if (0 == msg.intr) {
            PON_DBG("err: rx msg but with null intr \n");
            continue;
        }

        /* handle the interrupt */
        if (NULL != sg_gpon_intr_handlers[AAL_GPON_KMSG_CHN_INTR]) {
            (*sg_gpon_intr_handlers[AAL_GPON_KMSG_CHN_INTR])(msg.intr, msg.onu_state, msg.onu_id, NULL);
        }
    }
}


ca_status_t aal_gpon_intr_clr(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t  intr_idx,
    CA_IN  ca_uint32_t  clr_bmp)
{
    ca_uint32_t ena_bmp = 0;

    if (intr_idx >= AAL_GPON_INTR_NUM)
        return CA_E_PARAM;

    ena_bmp = AAL_PON_REG_READ(sg_gpon_intr_tbl[intr_idx].src_reg_addr);
    ena_bmp &= clr_bmp;
    AAL_PON_REG_WRITE(ena_bmp, sg_gpon_intr_tbl[intr_idx].src_reg_addr); /* Write ones to clear */
    return CA_E_OK;
}


ca_status_t aal_gpon_intr_enable_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  intr_idx,
    CA_IN ca_uint32_t  enable_bmp)
{
    if (intr_idx >= AAL_GPON_INTR_NUM)
        return CA_E_PARAM;

    AAL_PON_REG_WRITE(enable_bmp, sg_gpon_intr_tbl[intr_idx].ena_reg_addr);
    return CA_E_OK;
}


static void __intr_tasklet_action(unsigned long t)
{
    aal_gpon_intr_kmsg_t  msg;
    ca_status_t 		   ret =  CA_E_OK;

    while (1) {
    memset(&msg, 0, sizeof(msg));
    ret = __kmsg_recv(AAL_GPON_KMSG_CHN_INTR, &msg, sizeof(msg));
    if (CA_E_OK != ret) {
        return;
    }

    PON_DBG("intr msg rx : INTR %#x, ST_CHG %#x, STS %#x\n",
            msg.intr, msg.st_chg, msg.sts);

    if (!msg.intr) {
        PON_DBG("err: rx intr msg but with null intr \n");
        return;
    }

    /* handle the interrupt */
    if (NULL != sg_gpon_intr_handlers[AAL_GPON_KMSG_CHN_INTR]) {
        (*sg_gpon_intr_handlers[AAL_GPON_KMSG_CHN_INTR])(msg.intr, msg.onu_state, msg.onu_id, NULL);
    }
}
}


static ca_status_t __kmsg_recv(
    ca_uint32_t  msg_chn,
    void        *msg,
    ca_uint32_t   msg_len)
{
    ca_status_t ret = CA_E_OK;

    if (msg_chn >= AAL_GPON_KMSG_CHN_NUM || msg_len > AAL_GPON_KMSG_FIFO_SIZE)
    {
        ca_printf("%s:[%d] chn[%d] msgLen[%d] !!\n", __func__, __LINE__, msg_chn, msg_len);
        return CA_E_PARAM;
    }

    if (kfifo_is_empty(&aal_gpon_kfifo[msg_chn])) {
        return CA_E_EMPTY;
    }

    /* De-Queue */
    if (0 == (ret = kfifo_out(&aal_gpon_kfifo[msg_chn], msg, msg_len))) {
        ca_printf("KFIFO %d out fail.\n", msg_chn);
        return CA_E_NOT_FOUND;
    }

    PON_DBG("%s: KFIFO %d receive len %d OK.\n", __func__, msg_chn, ret);
    return CA_E_OK;
}


static ca_status_t __kmsg_send(
    ca_uint32_t  msg_chn,
    void        *msg,
    ca_uint32_t  msg_len)
{
    ca_status_t ret = CA_E_OK;

    if (msg_chn >= AAL_GPON_KMSG_CHN_NUM || msg_len > AAL_GPON_KMSG_FIFO_SIZE)
    {
        ca_printf("%s:[%d] chn[%d] msgLen[%d] !!\n", __func__, __LINE__, msg_chn, msg_len);
        return CA_E_PARAM;
    }

    /* En-Queue */
    if (0 == (ret = kfifo_in(&aal_gpon_kfifo[msg_chn], msg, msg_len))) {
        ca_printf("KFIFO %d is full.\n", msg_chn );
        return CA_E_FULL;
    }

    PON_DBG("%s: KFIFO %d send len %d OK.\n", __func__, msg_chn, msg_len);
    return CA_E_OK;
}


static ca_status_t __kmsg_init(ca_uint32_t fifo_num)
{
    static ca_boolean_t kmsg_init = FALSE;
    int rv = 0;
    int i  = 0;

    if (kmsg_init)
        return CA_E_OK;

    for (i = 0; i < fifo_num && i < AAL_GPON_KMSG_CHN_NUM; ++i) {
        rv = kfifo_alloc(&aal_gpon_kfifo[i], AAL_GPON_KMSG_FIFO_SIZE, GFP_KERNEL);
        if (rv) {
            ca_printf("kfifo_alloc KFIFO %d fail.\n", i);
            return CA_E_RESOURCE;
        }
    }

    kmsg_init = TRUE;
    return CA_E_OK;
}


static ca_status_t __do_intr_isp(ca_uint32_t intr_id)
{
    ca_status_t ret   = CA_E_OK;
    ca_uint32_t intre = 0;
    ca_uint32_t intrs = 0;

    intre = AAL_PON_REG_READ(sg_gpon_intr_tbl[intr_id].ena_reg_addr);
    intrs = AAL_PON_REG_READ(sg_gpon_intr_tbl[intr_id].src_reg_addr);
    AAL_PON_REG_WRITE(0, sg_gpon_intr_tbl[intr_id].ena_reg_addr);
    AAL_PON_REG_WRITE(intrs & intre, sg_gpon_intr_tbl[intr_id].src_reg_addr);
    sg_gpon_intr_tbl[intr_id].count++;

    if (NULL != sg_aal_gpon_intr_isr[intr_id]) {
        ret =(*sg_aal_gpon_intr_isr[intr_id])(intr_id, intrs & intre);
        if (CA_E_OK != ret)
            sg_gpon_intr_tbl[intr_id].fail_count++;
        else
            sg_gpon_intr_tbl[intr_id].handle_count++;
    }

    AAL_PON_REG_WRITE(intre, sg_gpon_intr_tbl[intr_id].ena_reg_addr);
    return ret;
}

static aal_gpon_act_event_t  __intr_to_evt(ca_uint32_t st_chg_int)
{
    GPON_MAC_GPON_interrupt_t   gpon_intr;

    gpon_intr.wrd = st_chg_int;

	if (gpon_intr.bf.LOSi){
		return	CA_EVENT_GPON_ALM_LOS;
	}

    if (gpon_intr.bf.LOFi){
        return  CA_EVENT_GPON_ALM_LOF;
    }

    if (gpon_intr.bf.SFi){
        return CA_EVENT_GPON_ALM_SF;
    }

    if (gpon_intr.bf.SDi){
        return CA_EVENT_GPON_ALM_SD;
    }

    if (gpon_intr.bf.LCDGi){
        return CA_EVENT_GPON_ALM_LCDG;
    }

    if (gpon_intr.bf.TFi){
        return  CA_EVENT_GPON_ALM_TF;
    }

    if (gpon_intr.bf.SUFi){
        return  CA_EVENT_GPON_ALM_SUF;
    }

    if (gpon_intr.bf.MEMi){
        return CA_EVENT_GPON_ALM_MEM;
    }

    if (gpon_intr.bf.DACTi){
        return CA_EVENT_GPON_ALM_DACT;
    }

    if (gpon_intr.bf.DISi){
        return CA_EVENT_GPON_ALM_DIS;
    }

    if (gpon_intr.bf.MISi){
        return CA_EVENT_GPON_ALM_MIS;
    }

    if (gpon_intr.bf.PEEi){
        return CA_EVENT_GPON_ALM_PEE;
    }

    if (gpon_intr.bf.RDIi){
        return CA_EVENT_GPON_ALM_RDI;
    }

    if (gpon_intr.bf.POPUPi	){
        //return AAL_XGPN_ACT_EVENT_TO3_EXPIRE;
    }

    if (gpon_intr.bf.DS_FEC_ONi){
        //return  AAL_XGPN_ACT_EVENT_TO4_EXPIRE;
    }

    if (gpon_intr.bf.DS_FEC_OFFi){
        //return AAL_XGPN_ACT_EVENT_TO5_EXPIRE;
    }

    if (gpon_intr.bf.ALLOCIDi){
        //return AAL_XGPN_ACT_EVENT_ASSIGN_ONUID_INCONSIST;
    }

    if (gpon_intr.bf.PORTIDi){
        //return AAL_XGPN_ACT_EVENT_LODS;
    }

    if (gpon_intr.bf.CPLi){
        //return AAL_XGPN_ACT_EVENT_DISABLE_SN_REQUEST;
    }
    if (gpon_intr.bf.KSWi){
        return CA_EVENT_GPON_PLOAM_KSW;
    }

    if (gpon_intr.bf.PDROPi){
        //return AAL_XGPN_ACT_EVENT_TUNING_REQ_ACK;
    }

    if (gpon_intr.bf.HPLOAMUi){
        return CA_EVENT_GPON_ERR_PLOAMU_OFLOW;
    }
	
	if (gpon_intr.bf.UNSYNCi){
		return	CA_EVENT_GPON_ERR_MAC_UNSYNC;
	}

	if (gpon_intr.bf.LF_UNDERi){
		return CA_EVENT_GPON_ERR_MAC_UNSYNC;
	}

	if (gpon_intr.bf.DS_PKTi){
		//return AAL_XGPN_ACT_EVENT_DWLCH_OK_TO_WORK;
	}

	if (gpon_intr.bf.DS_FRMi){
		//return AAL_XGPN_ACT_EVENT_DWLCH_NOT_APPROPRIATE;
	}

	if (gpon_intr.bf.ONU_IDi){
		return	CA_EVENT_GPON_ONU_ID;
	}

	if (gpon_intr.bf.ONU_ST_CHGi){
		return	CA_EVENT_GPON_ONU_STATE_CHNG;
	}

    return 0;
}

#ifdef CONFIG_LUNA_G3_SERIES
/* Check PON optic Rx signal ready or NOT */
static ca_boolean_t __gpon_chk_opt_rx(ca_device_id_t device_id)
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
static ca_boolean_t __gpon_chk_psds_rx_clock(ca_device_id_t device_id)
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
static ca_boolean_t __gpon_chk_no_rx_error(ca_device_id_t device_id)
{

    return TRUE;
}

static ca_uint32_t            g_gpon_reset_timer = 0;
#define __GPON_RESET_TIMER_INTERVAL_MS          1000

static void __gpon_reset_timer_callback(void *param)
{
    if (!__gpon_chk_opt_rx(AAL_DEFAUL_DEV_ID)) { /* RX LOS */
        return ;
    }

    aal_psds_reset(AAL_DEFAUL_DEV_ID, ONU_PON_MAC_GPON);

    if (!__gpon_chk_psds_rx_clock(AAL_DEFAUL_DEV_ID)) {/* wait CDR clock stable */
        return;
    }

    aal_gpon_backup_ponmac(AAL_DEFAUL_DEV_ID);

    aal_gpon_reinit(AAL_DEFAUL_DEV_ID);

    ca_gpon_cb_init(AAL_DEFAUL_DEV_ID);

    aal_gpon_restore_ponmac(AAL_DEFAUL_DEV_ID);

    if (!__gpon_chk_psds_rx_clock(AAL_DEFAUL_DEV_ID)) { /* check CDR again before enable PON MAC */
        ca_printf("%s(): %d PON SERDES clock loss again\r\n", __func__, __LINE__);
        return;
    }

    ca_timer_del(g_gpon_reset_timer);
    g_gpon_reset_timer = 0;
}

#endif

//data0 is interrupt id, data1 is interrupt value
void __intr_handler(
    ca_uint32_t intr,
    ca_uint32_t onu_state,
    ca_uint32_t onu_id,
    void *param)
{
    static ca_uint32_t        lastState = AAL_GPON_STATE_MAX;
    static ca_boolean_t       linkState = FALSE;
    GPON_MAC_GPON_omci_port_t omci_port = {0};
    ca_event_omcc_link_t      evt       = {0};
    ca_event_gpon_port_link_t portEvt   = {0};
    aal_gpon_aes_key_t        key       = {0};
#ifdef CONFIG_LUNA_G3_SERIES
    ca_event_gpon_stat_t gpon_evt;
#endif

#ifdef CONFIG_LUNA_G3_SERIES
    if(intr & AAL_GPON_INTR_POSEDGE_LOS){
        if (!g_gpon_reset_timer) {
            g_gpon_reset_timer = ca_circle_timer_add(__GPON_RESET_TIMER_INTERVAL_MS,
                __gpon_reset_timer_callback, NULL);

            if (g_gpon_reset_timer == 0) {
                ca_printf("%s(): error! fail to add gpon reset timer!\r\n", __func__);
            }
        }
    }
#endif

    if (intr & AAL_GPON_INTR_ONU_ST_CHG){
        if (lastState != onu_state)
        {
            ca_printf("onu new state: %s last state: %s\r\n", gpon_act_state[onu_state], gpon_act_state[lastState]);
        }

        //TODO:may send link down and handle ESTOP state
        if((onu_state == AAL_GPON_STATE_ESTOP)
        || (lastState == AAL_GPON_STATE_OPERATION && onu_state != AAL_GPON_STATE_POPUP) 
        || (lastState == AAL_GPON_STATE_POPUP && onu_state != AAL_GPON_STATE_OPERATION))
        {
            //send LINK DOWN to OMCI, need to check the last state and then send
            evt.port_id = CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7);
            evt.status  = CA_OMCC_LINK_DOWN;
            (void)ca_event_send(AAL_DEFAUL_DEV_ID, CA_EVENT_OMCC_LINK, &evt, sizeof(evt));
            
            portEvt.port_id = CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7);
            portEvt.status  = CA_OMCC_LINK_DOWN;
            (void)ca_event_send(AAL_DEFAUL_DEV_ID, CA_EVENT_GPON_PORT_LINK, &portEvt, sizeof(portEvt));

            linkState = FALSE;
#ifdef CONFIG_LUNA_G3_SERIES
            if (g_gpon_reset_timer) {
                /* RX LOS */
                //ca_printf("rx los\n");
            }
            else
            {
                if (!(intr & AAL_GPON_INTR_POSEDGE_DACT)) /* Not deactive*/
                {
                    //ca_printf("not deactive\n");
                    aal_gpon_reset(AAL_DEFAUL_DEV_ID);
                }
            }
#else
            //some reset sequence may call
            aal_gpon_reset(AAL_DEFAUL_DEV_ID);
#endif
        }
        else if (AAL_GPON_STATE_ESTOP == lastState)
        {
            //handle the ESTOP mode
        }

        lastState = onu_state;

#ifdef CONFIG_LUNA_G3_SERIES
        gpon_evt.port_id = CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7);
        gpon_evt.status  =  onu_state;

        (void)ca_event_send(AAL_DEFAUL_DEV_ID, CA_EVENT_GPON_REG_STAT_CHNG, &gpon_evt, sizeof(gpon_evt));
#endif
    }

    if (intr & AAL_GPON_INTR_ONU_ID){
        aal_gpon_omcc_tcont_enable(AAL_DEFAUL_DEV_ID, onu_id);
    }

    if (intr & AAL_GPON_INTR_POSEDGE_DACT){
        //TODO: need to add reset
        ca_printf("de-active onu \r\n");
        aal_gpon_reset(AAL_DEFAUL_DEV_ID);
    }

    if (intr & AAL_GPON_INTR_KSW){
        ca_printf("KSW intr \r\n");
        random_key_generate(key.key);
        aal_gpon_aes_key_set(AAL_DEFAUL_DEV_ID, &key);
    }

    if (intr & AAL_GPON_INTR_PORTID) {
        if (AAL_GPON_STATE_OPERATION == onu_state)
        {
            if (TRUE == linkState)
            {
                //don't need to set twice
                return;
            }

            omci_port.wrd = AAL_PON_REG_READ(GPON_MAC_GPON_omci_port);
            if (0 == omci_port.bf.en)
            {
                ca_printf("ONU stay in O5 but omci port is disable\r\n");
                return;
            }

            //enable OMCC channel
            aal_gpon_omcc_gem_enable(AAL_DEFAUL_DEV_ID, omci_port.bf.id);

            //send LINK UP to OMCI, need to check the last state and then send
            evt.port_id = CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7);
            evt.status  = CA_OMCC_LINK_UP;
            (void)ca_event_send(AAL_DEFAUL_DEV_ID, CA_EVENT_OMCC_LINK, &evt, sizeof(evt));

#ifdef CONFIG_LUNA_G3_SERIES
            portEvt.port_id = onu_id;
            aal_gpon_restore_gems(AAL_DEFAUL_DEV_ID);
#else
            portEvt.port_id = CA_PORT_ID(CA_PORT_TYPE_GPON, CA_PORT_ID_NI7);
#endif
            portEvt.status  = CA_OMCC_LINK_UP;
            (void)ca_event_send(AAL_DEFAUL_DEV_ID, CA_EVENT_GPON_PORT_LINK, &portEvt, sizeof(portEvt));

            linkState = TRUE;
        }
    }

}


//only used for GPON_MAC_GPON_interrupt
static ca_status_t __intr_isr(
    ca_uint32_t intr_id,
    ca_uint32_t intr_sts)
{
    ca_status_t ret = CA_E_OK;
    aal_gpon_intr_kmsg_t intr_msg = {0};
    GPON_MAC_GPON_onu_t  gpon_onu = {0};

    gpon_onu.wrd       = AAL_PON_REG_READ(GPON_MAC_GPON_onu);
    intr_msg.intr      = intr_sts;
    intr_msg.onu_state = gpon_onu.bf.state;
    intr_msg.onu_id    = gpon_onu.bf.id;

    ret = __kmsg_send(AAL_GPON_KMSG_CHN_INTR, &intr_msg, sizeof(intr_msg));
    if (ret != CA_E_OK) {
        return CA_E_FULL;
    }

    if (gpon_intr_tasklet) {
        tasklet_schedule(&aal_gpon_intr_tasklet);
    } else {
        ret = queue_work(gpon_intr_wq.pon_wq, &(gpon_intr_wq.work_info.work));
        if (ret) {
            PON_DBG("add work to gpon_intr_wq fail, already exist.\n");
            return CA_E_OK;
        }
    }

    return ret;
}


static ca_status_t __intr_handler_reg(
    aal_gpon_intr_msg_e    intr_msg,
    aal_intr_msg_handler_t handler)
{
    if (intr_msg >= AAL_GPON_KMSG_CHN_NUM || handler == NULL)
        return CA_E_PARAM;

    sg_gpon_intr_handlers[intr_msg] = handler;
    return CA_E_OK;
}


static ca_status_t __intr_isr_reg(
    aal_gpon_intr_e      intr_type,
    aal_intr_callback_t  callback)
{
    if (intr_type >= AAL_GPON_INTR_NUM || callback == NULL)
        return CA_E_PARAM;

    sg_aal_gpon_intr_isr[intr_type] = callback;
    return CA_E_OK;
}


ca_status_t aal_gpon_intr_init(CA_IN ca_device_id_t  device_id)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint32_t  idx = 0;

    /* Disable Top interrupts  */
    AAL_PON_REG_WRITE(0, GPON_MAC_GPON_int_top_en);
    idx = AAL_PON_REG_READ(GPON_MAC_GPON_interrupt_top); /* read clear */

    /* Global var init */
    memset(sg_aal_gpon_intr_isr, 0, sizeof(sg_aal_gpon_intr_isr));
    ret = __kmsg_init(AAL_GPON_KMSG_CHN_NUM);
    if (CA_E_OK != ret) {
        ca_printf("aal_gpon_kmsg_init return %d.\n", ret);
        return ret;
    } else {
        ca_printf("ca-pon: init GPON kmsg OK \n");
    }

    __intr_isr_reg(AAL_GPON_INTR, __intr_isr);
    __intr_handler_reg(AAL_GPON_KMSG_CHN_INTR, __intr_handler);

    /* Enable used interrupts and clear interrupt status  */
    for (idx = 0; idx < AAL_GPON_INTR_NUM; idx++) {
        aal_gpon_intr_enable_set(device_id, idx, 0); /* disable firstly */
        aal_gpon_intr_clr(device_id, idx, sg_gpon_intr_tbl[idx].ena_bmp); /* clear status */
        aal_gpon_intr_enable_set(device_id, idx, sg_gpon_intr_tbl[idx].ena_bmp); /* enable MUST interrupt */
        if (0 != sg_gpon_intr_tbl[idx].ena_bmp) {
            PON_DBG("ca-pon: Enable XGPON %s interrupt : 0x%08x\n",
                    sg_gpon_intr_tbl[idx].name, sg_gpon_intr_tbl[idx].ena_bmp);
        }
    }

    /* Enable interrupt */
    AAL_PON_REG_WRITE(GPON_MAC_GPON_INT_TOP_ENA_DEF, GPON_MAC_GPON_int_top_en);
    return CA_E_OK;
}
#endif /* CONFIG_NE_PON */
