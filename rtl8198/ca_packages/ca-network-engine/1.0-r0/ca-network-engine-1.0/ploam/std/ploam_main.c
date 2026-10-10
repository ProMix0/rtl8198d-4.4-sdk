#ifdef CONFIG_NE_PON

#include "osal_common.h"
#include "ca-scfg/scfg.h"

#include "ploam_main.h"
#include "ploam_cmn.h"
#include "ploam_pkt.h"
#include "ploam_gpon.h"
#include "ploam_ngpon2.h"

#include "aal_ploam.h"
#include "aal_pon.h"

#include "xgpon_aes.h"

#define PLOAM_ONCE_PROC_MAX_COUNT  8

#if 0
extern ca_uint32_t aal_pon_mac_mode_get(ca_device_id_t device_id);
extern ca_uint8_t omci_msk_flag;
extern ca_uint8_t omci_msk[];
#endif

ca_ploam_t g_ploam_cb;
typedef ca_uint32_t (*ca_tx_cb)(
    ca_device_id_t device_id,
    void *pkt);

ca_tx_cb ca_ploam_tx_handle = NULL;

ca_status_t ca_ploam_tx_register(
    ca_uint32_t (*handler)(ca_device_id_t device_id, void *pkt))
{
    ca_ploam_tx_handle = handler;

    printk("<%s, %d>ca_ploam_tx_handle:%p\r\n", __FUNCTION__, __LINE__, ca_ploam_tx_handle);

    return CA_E_OK;
}


ca_status_t ca_ploam_tx_unregister(void)
{
    ca_ploam_tx_handle = NULL;

    printk("<%s, %d>ca_ploam_tx_handle:%p\r\n", __FUNCTION__, __LINE__, ca_ploam_tx_handle);

    return CA_E_OK;
}

ca_status_t ca_ploam_tx(ca_int8_t *buffer, ca_uint32_t len, ca_uint16_t port_id)
{
    ca_pkt_t pkt;
    //ca_uint32_t pid = CA_PORT_TYPE_GPON << 8;

    memset(&pkt, 0, sizeof(ca_pkt_t));

    pkt.block_count = 1;
    pkt.device_id = 0;
    pkt.dst_port_id = port_id;
    pkt.pkt_len = len;

    pkt.pkt_data = ca_malloc(sizeof(ca_pkt_block_t));
    memset(pkt.pkt_data, 0, sizeof(ca_pkt_block_t));

    pkt.pkt_data->data = ca_malloc(len);
    memcpy(pkt.pkt_data->data, buffer, len);
    pkt.pkt_data->len = len;

    ca_ploam_tx_handle(0, &pkt);
#ifdef CONFIG_PROC_FS
    if (PON_DBG_GPON_PLOAM_TX & ca_pon_debug) {
        ca_buffer_dump("Tx PLOAM:", buffer, len);
    }
#endif
    ca_free(pkt.pkt_data->data);
    ca_free(pkt.pkt_data);
    return CA_E_OK;
}


/* emergency stop */
/*
#define PLOAM_ES_FILE   "/overlay/etc/es"

void cs_ploam_onu_state_proc(void *data, cs_uint32 data_len);
void cs_ploam_alarm_mem_proc(void *data, cs_uint32 length);
*/

void ploam_lock(void)
{

    ca_spin_lock(g_ploam_cb.lock);
}

void ploam_unlock(void)
{
    ca_spin_lock(g_ploam_cb.lock);
}

#if 0
ca_status_t cs_ploam_onu_es_read(cs_uint8 *es_state)
{
    FILE *fp = NULL;
    cs_uint8 ch = 0;
    size_t size = 0;
    ca_status_t ret = CS_PLOAM_OK;

    fp = fopen(PLOAM_ES_FILE, "rb");
    if (NULL == fp)
    {
        PLOAM_ERROR("Open %s fail\r\n", PLOAM_ES_FILE);
        return CS_PLOAM_E_ERROR;
    }

    size = fread(&ch, 1, 1, fp);
    if (size != 0)
    {
        *es_state = ch;
    }
    else
    {
        ret = CS_PLOAM_E_ERROR;
    }

    fclose(fp);

    return ret;
}

ca_status_t cs_ploam_onu_es_write(cs_uint8 es_state)
{
    FILE *fp = NULL;
    cs_uint8 ch = 0;

    PLOAM_DEBUG("ES state %d\r\n", es_state);

    fp = fopen(PLOAM_ES_FILE, "wb");
    if (NULL == fp)
    {
        PLOAM_ERROR("Open %s fail\r\n", PLOAM_ES_FILE);
        return CS_PLOAM_E_ERROR;
    }

    if (es_state)
    {
        ch = 1;
    }
    else
    {
        ch = 0;
    }

    fwrite(&ch, 1, 1, fp);

    fclose(fp);

    return CS_PLOAM_OK;
}
#endif


static void ploam_pkt_proc(ca_uint8 *frame_ptr, ca_uint32_t len)
{
    ca_uint32_t pon_mac = aal_pon_mac_mode_get(0);
    PLOAM_ASSERT(frame_ptr != NULL && len > 0);

#ifdef CONFIG_PROC_FS
    if (ca_pon_debug & PON_DBG_GPON_PLOAM_RX) {
        ca_buffer_dump("Rx PLOAM:", frame_ptr, len);
    }
#endif

    switch (pon_mac){
        case ONU_PON_MAC_GPON:
            ploam_pkt_gpon_proc(frame_ptr, len);
            break;
        case ONU_PON_MAC_NGPON2:
        case ONU_PON_MAC_XGPON1:
        case ONU_PON_MAC_XGSPON:
            ploam_pkt_ngpon2_proc(frame_ptr, len);
            break;
        default:
            PLOAM_ERROR("no support pon mac\r\n");
            break;
    }

    return;
}
#if 0
void xgpon_ploam_rgst_handler(
    ca_uint32_t xgpn_ploam_int,
    ca_uint32_t xgpn_ploam_sts,
    ca_uint32_t stat,
    void *cookie)
{
    if (xgpn_ploam_int & XGPN_PLMP_INTR_MASK_REGST_MSG) {
		if(omci_msk_flag){
			//ca_buffer_dump("omci_msk:", omci_msk, 16);
			aal_xgpon_aes_plmp_msk_set(AAL_DEFAUL_DEV_ID,omci_msk);
			omci_msk_flag = FALSE;
		}
    }
	
}
EXPORT_SYMBOL(xgpon_ploam_rgst_handler);
#endif
void xgpon_ploam_rx_handler(ca_uint32_t intr, ca_uint32_t sts, ca_uint32_t len, void *pkt)
{
    /* process ploam pkt */
    ploam_pkt_proc(pkt, len);

    /* capture into DEBUG buffer */
    if (aal_ploam_dbg_capture_get(AAL_DEFAUL_DEV_ID)) {
        aal_ploam_dbg_capture_add(AAL_DEFAUL_DEV_ID, (ca_uint8_t *)pkt, len);
    }
}
EXPORT_SYMBOL(xgpon_ploam_rx_handler);
#if 0

static void ploam_msg_proc(void *msg)
{
    ploam_msg_t *ploam_msg = (ploam_msg_t *)msg;

    PLOAM_ASSERT(msg != NULL);

    PLOAM_DEBUG("rcv msg\r\n");

    switch (ploam_msg->hdr.type) {
        case PLOAM_PACKET:
            ploam_pkt_proc(ploam_msg);
            break;

        default:
            PLOAM_ERROR("Invalid ploam message type %d\r\n", ploam_msg->hdr.type);
            break;
    }

    ca_free(msg);
}

void ploam_rxbuf_proc(void)
{
    //ca_uint8_t *ploam_msg = OS_MALLOC_API(PLOAM_MSG_HDR_LEN + XGPN_PLMP_RXBUF_SIZE);
    ploam_msg_t ploam_msg;
    ca_uint8_t  buf_sz = PLOAM_MQ_MSG_MAX_LEN;
    ca_uint16_t i = 0;
    ca_status_t ret = CA_E_OK;

    memset(ploam_msg.data, 0, PLOAM_MQ_MSG_MAX_LEN);
    ploam_msg.hdr.type  = 0;
    //ploam_msg.hdr.resv1 = 0;

    for (i = 0; i < PLOAM_ONCE_PROC_MAX_COUNT; i++) {
        ret = aal_ploam_queue_out(ploam_msg.data, &buf_sz);
        if (CA_E_OK != ret) {
            break;
        }

        ploam_msg.hdr.len = buf_sz;

        aal_ploam_rx_stats_update(0, ploam_msg.data);

        ploam_pkt_proc(&ploam_msg);

        if (!aal_ploam_queue_exist_pkt()) {
            break;
        }
    }

}

{
    aal_xgpn_plmp_rxbuf_stat_t rx_stat[XGPN_PLMP_RXBUF_BLOCK];
    ca_uint32_t   read_buf_idx[XGPN_PLMP_RXBUF_BLOCK] = {0};
    ca_uint32_t   read_buf_num = 0, buf_id = 0;
    //aal_xgpn_plmp_rxbuf_t  rx_buf;
    aal_xgpn_plmp_rxbuf_stat_mask_t  ctrl_mask;
    //aal_xgpn_plmp_interrupt_mask_t  inti_mask;
    //aal_xgpn_plmp_interrupt_enable_mask_t  intie_mask;
    aal_xgpn_plmp_interrupt_enable_t intie;
    //int pkt_flag[XGPN_PLMP_RXBUF_BLOCK] = {0};
    ploam_msg_t ploam_msg;
    int i;
    aal_xgpn_plmp_interrupt_t inti;
    //ca_status_t ret;

#if 0 /* needn't disable interrupt by S/W, just trigger with clear INTR status */
    /* turn off interrupt */
    intie_mask.wrd = (ca_uint32_t)-1;
    memset(&intie, 0, sizeof(intie));
    ret = aal_ploam_interrupt_enable0_set(0, intie_mask, &intie);
#endif

#if 0   //intr cleared in IRQ handler before entering wq process
    memset(&intie, 0, sizeof(intie));
    (void)aal_ploam_interrupt_enable0_get(0, &intie);
    memset(&inti, 0, sizeof(inti));
    (void)aal_ploam_interrupt_get(0, &inti);
#endif
#if 0 // ASIC has bug that not report BUF Ready interrupt correctly
    if ((intie.rxbuf0_rdy_intie && inti.rxbuf0_rdy_inti)
        || (intie.rxbuf1_rdy_intie && inti.rxbuf1_rdy_inti)) {

        memset(&rx_stat, 0, sizeof(rx_stat));

        if (intie.rxbuf0_rdy_intie && inti.rxbuf0_rdy_inti) {
            aal_ploam_rxbuf_stat_get(0, 0, &rx_stat[0]);
        }

        if (intie.rxbuf1_rdy_intie && inti.rxbuf1_rdy_inti){
            aal_ploam_rxbuf_stat_get(0, 1, &rx_stat[1]);
        }
#else
    if (1) {
            aal_ploam_rxbuf_stat_get(0, 0, &rx_stat[0]);
            aal_ploam_rxbuf_stat_get(0, 1, &rx_stat[1]);

#endif

        if (rx_stat[0].pkt_rdy && rx_stat[1].pkt_rdy) { /* coming two pkt */

            read_buf_num = 2;
            if (rx_stat[0].turn) {/* Read buf#0 firstly */
                read_buf_idx[0] = 0;
                read_buf_idx[1] = 1;
            }
            else if (rx_stat[1].turn) {/* Read buf#1 firstly */
                read_buf_idx[0] = 1;
                read_buf_idx[1] = 0;
            }

        } else if (rx_stat[0].pkt_rdy) {
            read_buf_num = 1;
            read_buf_idx[0] = 0;
            //read_buf_idx[1] = 0;
        } else if (rx_stat[1].pkt_rdy) {
            read_buf_num = 1;
            read_buf_idx[0] = 1;
            //read_buf_idx[1] = 1;
        }

        for (i = 0; i < read_buf_num; i++) {
            buf_id = read_buf_idx[i];

            if(rx_stat[buf_id].mic_err) {
                if (aal_ploam_debug > 1) {
                    ca_printf("err: rx_buf %d mic_err !\r\n", buf_id);
                }
                //continue;
            }

            memset(&ploam_msg, 0, sizeof(ploam_msg_t));

            aal_ploam_rxbuf_get(0, buf_id, TRUE, (aal_xgpn_plmp_rxbuf_t *)&ploam_msg.data);

            if (aal_ploam_debug > 2){
                ca_printf("Rx PLOAM from rx_buf %d\r\n", buf_id);
                //ploam_dump_pkt("rxbuf", ploam_msg.data, XGPN_PLMP_RXBUF_SIZE);
            }

            ploam_msg.hdr.len = XGPN_PLMP_RXBUF_SIZE;
            //ploam_msg.hdr.type = 0;
            /* process pkt */
            ploam_pkt_proc(&ploam_msg);

            /*trigger key exchange sm*/
            // TODO: SHOULD move the KEY SM to ploam_pkt_proc
            //xgpon_ploam_key_exchange_handler((ca_uint8_t*)(&ploam_msg));

            /* Set to 0 for release buffer */
            ctrl_mask.wrd = ~0U;
            memset(&rx_stat[buf_id], 0, sizeof(aal_xgpn_plmp_rxbuf_stat_t));
            //ctrl_mask.bf.pkt_rdy = 1;
            //ctrl_mask.bf.turn    = 1;
            //rx_stat[buf_id].pkt_rdy = 0;
            //rx_stat[buf_id].turn    = 0;
            aal_ploam_rxbuf_stat_set(0, buf_id, ctrl_mask, &rx_stat[buf_id]);

        }

    }

    /*Corey-2016-9-21:hook tk4 and tk5 interrupt handler*/
    if(intie.key_exchg_tk4_timeout_intie && inti.key_exchg_tk4_timeout_inti){
        xgpon_key_exchange_tk4_timeout_handler();
    }

    if(intie.key_exchg_tk5_timeout_intie && inti.key_exchg_tk5_timeout_inti){
        xgpon_key_exchange_tk5_timeout_handler();
    }

__end:

    /* clean interrupt */
    //inti_mask.wrd = (ca_uint32_t)-1;
    //(void)aal_ploam_interrupt_set(0, inti_mask, &inti);

#if 0
    /* enable interrupt */
    intie_mask.wrd = (ca_uint32_t)-1;
    memset(&intie, 0xff, sizeof(intie));
    ret = aal_ploam_interrupt_enable0_set(0, intie_mask, &intie);
#endif
    return;
}
#endif

/* receive ploam packet from NI driver */
/*
ca_status_t ca_ploam_pkt_rcv(
    ca_uint8_t       *pkt,
    ca_uint32_t      len)
{
    ploam_msg_t msg;

    PLOAM_ASSERT_RET(pkt != NULL, CA_E_ERROR);

    memset(&msg, 0, sizeof(msg));

    if (len > PLOAM_MQ_MSG_MAX_LEN)
        len = PLOAM_MQ_MSG_MAX_LEN;

    msg.hdr.type = PLOAM_PACKET;
    msg.hdr.len  = len;

    PLOAM_DEBUG("len:%d\r\n", len);

    if (len > 0)
        memcpy(&msg.data[0], pkt, len);
    else {
        PLOAM_ERROR("Invalid len %d\r\n", len);
        return CA_E_ERROR;
    }

    if (ca_queue_put(g_ploam_cb.mq_id, &msg, sizeof(ploam_msg_hdr_t) + len,
                     OSAL_NO_WAIT, 0) < 0) {
        PLOAM_ERROR("Send message failed.\r\n");
        return CA_E_ERROR;
    }

    return CA_E_OK;

}
*/
/*
static void OS_WORK_PROCESS_INTERNAL(void *data)
{
    OS_WORK_t *my_work = (OS_WORK_t *)data;
printk("<%s, %d>\r\n", __FUNCTION__, __LINE__);
    if(my_work == NULL)
        return;
printk("<%s, %d>\r\n", __FUNCTION__, __LINE__);
    if(my_work->callback)
        my_work->callback(my_work->data);
printk("<%s, %d>\r\n", __FUNCTION__, __LINE__);
    kfree(my_work);
    return;
}
*/
#if 0
ca_status_t ca_ploam_os_wq_init(const ca_int8_t *name)
{
    ca_ploam_wq_t *ploam_wq = &g_ploam_cb.ploam_wq;
    struct workqueue_struct *wq = &ploam_wq.wq;

    wq = create_workqueue(name);
    if(wq == NULL){
        PLOAM_DEBUG("create work queue fail\r\n");
        return CA_E_ERROR;
    }

    INIT_WORK(&work->work, (void *)ploam_msg_proc);
    return CA_E_OK;
}

ca_status_t ca_ploam_os_work_add(void *data)
{
    ca_ploam_wq_t *ploam_wq = &g_ploam_cb.ploam_wq;
    struct work_struct *work = &ploam_wq.work_data.work;
    struct workqueue_struct *wq = &ploam_wq.wq;
    struct demo_struct *demop = container_of(memp, struct demo_struct, member3);

    PLOAM_ASSERT_RET(ploam_wq != NULL && work != NULL && wq != NULL, CA_E_EMPTY);
    PLOAM_ASSERT_RET(data != NULL, CA_E_PARAM);

    work->callback = callback;
    work->data = data;
    queue_work(wq, &work->work);
    return 0;
}

void OS_WORKQUEUE_DESTROY(ca_uint wq_id)
{
    if(wq_id == 0)
        return;

    destroy_workqueue((struct workqueue_struct *)wq_id);
    return;
}


/* interrupt handler */
void ca_ploam_wq_interrupt(ca_uint32_t info)
{
    ca_status_t ret = CA_E_OK;

    ret = ca_work_add(g_ploam_cb.wq_id, ploam_rxbuf_proc, NULL);

    if (ret == -1){
        PLOAM_ERROR("process message failed.\r\n");
        return;
    }

    return;

}


ca_status_t ca_ploam_wq_add(
    ca_uint8_t       *pkt,
    ca_uint32_t      len)
{
    ploam_msg_t *msg = ca_malloc(sizeof(ploam_msg_t));
    int ret;

    PLOAM_ASSERT_RET(pkt != NULL && msg != NULL, CA_E_ERROR);

    memset(msg, 0, sizeof(ploam_msg_t));

    if (len > PLOAM_MQ_MSG_MAX_LEN)
        len = PLOAM_MQ_MSG_MAX_LEN;

    msg->hdr.type = PLOAM_PACKET;
    msg->hdr.len  = len;

    PLOAM_DEBUG("len:%d\r\n", len);

    if (len > 0)
        memcpy(msg->data, pkt, len);
    else {
        PLOAM_ERROR("Invalid len %d\r\n", len);
        return CA_E_ERROR;
    }

    ret = ca_work_add(g_ploam_cb.wq_id, ploam_msg_proc, (void *)msg);

    if (ret == -1){
        PLOAM_ERROR("process message failed.\r\n");
        return CA_E_ERROR;
    }

    /*
    if (ca_queue_put(g_ploam_cb.mq_id, &msg, sizeof(ploam_msg_hdr_t) + len,
                     OSAL_NO_WAIT, 0) < 0) {
        PLOAM_ERROR("Send message failed.\r\n");
        return CA_E_ERROR;
    }
    */
    return CA_E_OK;

}

#endif

ca_status_t ca_ploam_init(
    CA_IN  ca_device_id_t device_id
)
{
//    ca_uint8_t es_state = 0;
    ca_status_t ret = CA_E_OK;
    ca_uint8_t vendor_id[] = { 'C', 'O', 'R', 'T' };
    ca_uint32_t serial_num = 0x1234;
    ca_uint32_t mode = aal_pon_mac_mode_get(0);

    if (mode < ONU_PON_MAC_GPON || mode > ONU_PON_MAC_NGPON2){
        printk("unmatched pon mode:%d, quit\r\n", mode);
        return ret;
    }

    ca_printf("CA ploam init \n");


    /* TODO */
    /* get vendor_id and serial num from startup cfg */

    memset(&g_ploam_cb, 0, sizeof(g_ploam_cb));

    /* save serial number */
    memcpy(g_ploam_cb.sn.vendor_id, vendor_id, CS_PLOAM_VENDOR_ID_LEN);

    /* save as network endian */
    g_ploam_cb.sn.serial_num = ca_htonl(serial_num);

#if 0
    ret = cs_ploam_onu_es_read(&es_state);
    if (CA_E_OK == ret)
    {
        PLOAM_DEBUG("es_state %d\r\n", es_state);
        g_ploam_cb.es_state = es_state ? TRUE : FALSE;
        if (g_ploam_cb.es_state)
        {
          //  cs_ploam_set_onu_es_state(PLOAM_DEV_ID, PLOAM_PORT_ID);
#ifdef ZTE_SPECIFIC
            cs_ploam_onu_laser_set(PLOAM_DEV_ID, PLOAM_PORT_ID, PLOAM_LASER_OFF_BY_SN);
#endif/*END_ZTE_SPECIFIC*/
        }
    }
    else
    {
        PLOAM_DEBUG("error read file\r\n");
    }

#endif
    ret = ca_spin_lock_init(&g_ploam_cb.lock,SPINLOCK_BH);

    if (ret) {
        ca_printf("Init ploam lock failed!\r\n");
        return CA_E_ERROR;
    }

#if 0
    g_ploam_cb.wq_id = ca_workqueue_init(PLOAM_QUEUE_NAME, PLOAM_QUEUE_PRIORITY);

    PLOAM_DEBUG("wq_id:0x%x\r\n", g_ploam_cb.wq_id);

    if (g_ploam_cb.wq_id == 0) {
        ca_printf("Create %s walk queue failed!\r\n", PLOAM_QUEUE_NAME);
        return CA_E_ERROR;
    }

    ca_printf("Ploam init \n");
    //ret = aal_ploam_init(device_id);
#endif

    return CA_E_OK;
}



#if 0
#ifdef ZTE_SPECIFIC
static void ploam_trcvr_tx_ctrl_proc(cs_uint8 *buff)
{
    cs_ploam_trcvr_tx_ctrl_t *msg = (cs_ploam_trcvr_tx_ctrl_t *)buff;

    if (!buff)
        return;

    PLOAM_DEBUG("msg action 0x%x\r\n", msg->action);

    switch(msg->action)
    {
        case PLOAM_LASER_ON_BY_SN:
        case PLOAM_LASER_OFF_BY_SN:
        case PLOAM_LASER_BRST_LOCKED_BY_SN:
        case PLOAM_LASER_BRST_NORMAL_BY_SN:
            PLOAM_DEBUG("SN %c%c%c%c-%02x%02x%02x%02x\r\n",
                msg->sn[0], msg->sn[1], msg->sn[2], msg->sn[3],
                msg->sn[4], msg->sn[5], msg->sn[6], msg->sn[7]);
            if (memcmp(&g_ploam_cb.sn, msg->sn, CS_SN_LEN) != 0)
            {
                PLOAM_DEBUG("SN not match, ignore!\r\n");
                return;
            }
            break;
        default:
            PLOAM_DEBUG("Invalid tx control action %d!\r\n", msg->action);
            return;
    }

    cs_ploam_onu_laser_set(PLOAM_DEV_ID, PLOAM_PORT_ID, msg->action);
}

static void ploam_onu_reboot_proc(cs_uint8 *buff)
{
    cs_ploam_onu_reboot_t *msg = (cs_ploam_onu_reboot_t *)buff;

    if (!buff)
        return;

    switch(msg->action)
    {
        case PLOAM_ONU_REBOOT:
            PLOAM_DEBUG("ONU_REBOOT\r\n");
            break;
        case PLOAM_ONU_REBOOT_BY_SN:
            PLOAM_DEBUG("ONU_REBOOT_BY_SN %c%c%c%c-%02x%02x%02x%02x\r\n",
                msg->sn[0], msg->sn[1], msg->sn[2], msg->sn[3],
                msg->sn[4], msg->sn[5], msg->sn[6], msg->sn[7]);
            if (memcmp(&g_ploam_cb.sn, msg->sn, CS_SN_LEN) != 0)
            {
                PLOAM_DEBUG("SN not match, ignore!\r\n");
                return;
            }
            break;
        default:
            PLOAM_DEBUG("Invalid reboot action %d!\r\n", msg->action);
            return;
    }

    cs_ploam_onu_reboot(PLOAM_DEV_ID);
}
#endif/*END_ZTE_SPECIFIC*/

void cs_ploam_onu_state_proc(void *data, cs_uint32 data_len)
{
    cs_uint32   state = 0;
    cs_boolean  es_state = FALSE;
    ca_status_t ret = CA_E_OK;

    if (!data)
    {
        PLOAM_ERROR("data pointer NULL\r\n");
        return;
    }

    if (data_len != sizeof(cs_uint32))
    {
        PLOAM_ERROR("Invalid data len %d\r\n", data_len);
        return;
    }

    state = *(cs_uint32 *)data;

    es_state = cs_ploam_is_state_es(state);

    /* ignore this state change */
    if (es_state == g_ploam_cb.es_state)
    {
        return;
    }

#ifdef ZTE_SPECIFIC
    {
        cs_uint32   laser_action = 0;
        if (es_state)
            laser_action = PLOAM_LASER_OFF_BY_SN;
        else
            laser_action = PLOAM_LASER_BRST_NORMAL;

        cs_ploam_onu_laser_set(PLOAM_DEV_ID, PLOAM_PORT_ID, laser_action);
    }
#endif/*END_ZTE_SPECIFIC*/

    ret = cs_ploam_onu_es_write(es_state);
    if (ret != CS_PLOAM_OK)
    {
        PLOAM_ERROR("ONU es state write to flash fail. ret = %d\r\n", ret);
        return;
    }

    g_ploam_cb.es_state = es_state;
}

void cs_ploam_alarm_mem_proc(void *data, cs_uint32 length)
{
    ca_ploam_msg_t ploam_msg;
    ca_status_t ret = CA_E_OK;

    do
    {
        memset(&ploam_msg, 0, sizeof(ploam_msg));

        ret = cs_priv_ploam_ds_get(PLOAM_DEV_ID, (cs_uint8 *)&ploam_msg);
        if (ret != CS_PLOAM_OK)
        {
            return;
        }

        PLOAM_DEBUG("msg_id 0x%x\r\n", ploam_msg.pkt.msg_id);

        switch (ploam_msg.pkt.msg_id)
        {
#ifdef ZTE_SPECIFIC
            case PLOAM_MSG_ID_TRCVR_TX_CTRL:
                ploam_trcvr_tx_ctrl_proc(ploam_msg.pkt.data);
                break;
            case PLOAM_MSG_ID_ONU_REBOOT:
                ploam_onu_reboot_proc(ploam_msg.pkt.data);
                break;
#endif/*END_ZTE_SPECIFIC*/
            default:
                break;
        }
    } while(ploam_msg.left_entry_num);
}

void cs_ploam_sn_get(cs_uint8 *sn)
{
    ploam_lock();

    if (sn)
    {
        memcpy(sn, &g_ploam_cb.sn, CS_SN_LEN);
    }

    ploam_unlock();
}

cs_boolean cs_ploam_es_state_get()
{
    cs_boolean es_state;

    ploam_lock();

    es_state = g_ploam_cb.es_state;

    ploam_unlock();

    return es_state;
}
#endif

//EXPORT_SYMBOL(ca_ploam_init);
//EXPORT_SYMBOL(ca_ploam_wq_add);
EXPORT_SYMBOL(ca_ploam_tx_register);
EXPORT_SYMBOL(ca_ploam_tx_unregister);
#endif/*END_CONFIG_NE_PON*/

