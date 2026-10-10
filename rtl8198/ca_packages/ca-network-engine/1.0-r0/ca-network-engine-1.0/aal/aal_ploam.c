
#include "ca_types.h"
#include "osal_common.h"
#include "aal_pon.h"
#include "aal_ploam.h"
#include "ploam_api.h"
#include "ploam_main.h"
#include "ploam_ngpon2.h"
#include "ploam_pkt.h"
#include "xgpon_aes.h"
#include "ca_event.h"

#include <soc/cortina/registers.h>

ca_int32_t aal_ploam_debug = 0;
ca_uint32_t aal_ploam_err_cnt = 0;
ca_int32_t aal_ploam_key_debug = 0;
ca_uint g_ploam_lock = 0;
aal_ploam_dbg_t  g_aal_ploam_dbg;
ca_list_t *g_ploam_pkt_lst = NULL;

#define __PLOAM_CAPTURE_BUF_NUM  256
#define __PLOAM_LOCK()    \
    do{ \
        if (g_ploam_lock) ca_spin_lock(g_ploam_lock);\
    }while(0)

#define __PLOAM_UNLOCK()    \
    do{ \
        if (g_ploam_lock) ca_spin_unlock(g_ploam_lock);\
    }while(0)

void ploam_octet_dump( char *tp, unsigned char *dp, int len );
extern void ploam_set_pkt_dump(ca_uint32_t type);
extern void aes_ploam_set_pkt_dump(ca_uint32_t type);
extern void ploam_buf_print(ca_uint8_t *pbuf , ca_int32_t len);

/*****************************************************************************/
/*                          Local functions                                  */
/*****************************************************************************/
void
ploam_octet_dump( char *tp, unsigned char *dp, int len )
{
    int  ii = 0;

    ca_printf("\r\n%s\n", tp);

    for (ii = 0; ii < len; ii++) {
        ca_printf("%02x ", dp[ii]);
        if (ii % 16 == 15) {
            ca_printf("\n");
        }
    }

    ca_printf("\n");
}

ca_status_t aal_ploam_init(
    ca_device_id_t     device_id
)
{
    ca_int32 ret = CA_E_ERROR;

    memset(&g_aal_ploam_dbg, 0, sizeof(g_aal_ploam_dbg));

    if (!g_ploam_lock) {
        ret = ca_spin_lock_init(&g_ploam_lock, SPINLOCK_BH);
        if (ret != 0) {
            ca_printf("create ploam spin lock fail\r\n");
			return ret;
        }
    }

    if (!g_ploam_pkt_lst) {
        g_ploam_pkt_lst = ca_list_create(sizeof(aal_xgpn_plmp_rxbuf_t), NULL);
		if(!g_ploam_pkt_lst){
			ca_printf("create ploam pkt list fail\r\n");
			return CA_E_ERROR;
		}
    }

    PON_DBG("ca-pon: AAL PLOAM init OK\n");

    return CA_E_OK;
}

ca_status_t aal_ploam_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
)
{

    switch (value1){
        case 0:
            ploam_set_pkt_dump(value2);
            break;

        case 1:
            aal_ploam_debug = value2;
            ca_printf("ploam debug %d\r\n", aal_ploam_debug);
            break;
        case 2:
            break;
        case 3:
            {
                ca_uint8_t pkt[48] = "123456789abcdef";
                ploam_pkt_ngpon2_t *dp = (ploam_pkt_ngpon2_t*)pkt;
                printk("<%s, %d> msg_id:0x%x\r\n", __FUNCTION__, __LINE__, value2);
                dp->msg_id = value2;
            }
            break;
        case 4:
            {
                int i, j;
                aal_xgpn_plmp_rxbuf_t rx_buf;
                aal_xgpn_plmp_rxbuf_stat_mask_t stat_mask;
                aal_xgpn_plmp_rxbuf_stat_t stat;

                memset(&stat_mask, 0, sizeof(aal_xgpn_plmp_rxbuf_stat_mask_t));
                memset(&stat, 0, sizeof(aal_xgpn_plmp_rxbuf_stat_t));

                stat_mask.bf.pkt_rdy = 1;
                stat.pkt_rdy = 1;
                for (i = 0; i < XGPN_PLMP_RXBUF_BLOCK; i++){
                    for (j = 0; j < XGPN_PLMP_RXBUF_SIZE; j++){
                        rx_buf.data[j] = (i << 8) + j + 1;
                    }
                    rx_buf.data[2] = value2 + i;
                    aal_ploam_rxbuf_set(0, i, 1, &rx_buf);
                    aal_ploam_rxbuf_stat_set(0, i, stat_mask, &stat);
                }

            }
            break;
        case 5:
            {
            }
            break;
        case 6:
            break;
        case 7:
            aes_ploam_set_pkt_dump(value2);
            break;
        case 9:
            {
                ca_int8_t buffer[68] = {
0x00, 0xcc, 0xcc, 0x00, 0x00, 0x01, 0x00, 0xcc, 0xcc, 0x00, 0x00, 0x02, 0xff, 0xf1, 0x00, 0x00,
0x22, 0x08, 0x49, 0x0a, 0xff, 0xfa, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x18, 0x4e, 0x1e, 0x7d};
                ca_uint32_t len;
                len = sizeof(buffer);

                ploam_octet_dump("buf", buffer, len);

                ca_ploam_tx(buffer, len, value2);

            }
            break;
        case 10:
            {
                ca_int8_t buffer[68] = {
0x00, 0xcc, 0xcc, 0x00, 0x00, 0x01, 0x00, 0xcc, 0xcc, 0x00, 0x00, 0x02, 0xff, 0xf0, 0x00, 0x00,
0x22, 0x08, 0x49, 0x0a, 0xff, 0xfa, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x18, 0x4e, 0x1e, 0x7d};
                ca_uint32_t len;
                //ca_uint32_t value = ntohl(AAL_PON_REG_READ(PUC_GLOBAL_MAC_TYPE));
                len = sizeof(buffer);

                ploam_octet_dump("buf", buffer, len);

                ca_ploam_tx(buffer, len, value2);

            }
            break;
        case 11:
            {

            }
            break;

        case 12:
            {
				ca_status_t ret = CA_E_OK;
                ca_uint8 adtran_pon_tag[PON_TAG_SIZE] = {0x01,0x02,0x03,0x04,0x05,0x06,0x00,0x00};
                ca_uint8 calix_pon_tag[PON_TAG_SIZE] = {0};
                ca_uint8 zte_pon_tag[PON_TAG_SIZE] = {0x5a,0x54,0x45,0x47,0x00,0x00,0x00,0x00};
                ca_uint8 mitra_pon_tag[PON_TAG_SIZE] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
				ca_int8_t tx_omci_mic[4] = {0};
				ca_int8_t ploam_mic[8] = {0};
				ca_int8_t ploam_mic1[8] = {0};
				ca_int8_t ploam_mic2[8] = {0};
               	ca_uint8_t saturn_tx_omci_content[]= {
                0xa5 ,0xe0 ,0x29 ,0x0a ,0x00 ,0x83 ,0x00 ,0x00 ,0x00 ,0x80 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
                0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
                0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x28
               };

                ca_int8_t zte_omci_content[] ={ 0x22, 0x09, 0x49, 0x0a, 0x01, 0x00, 0x00, 0x00,
                                            0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x28};
                ca_int8_t calix_omci_content[] = {
                    0x36, 0x41, 0x49, 0x0a, 0x00, 0x02, 0x00, 0x00,  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x28, 0x94, 0x45, 0xab, 0xc5,
                };
				ca_int8_t mitr_plm_content[] ={ 0x00,0x01,0x09,0x00,0x01,0x00,0x00,0x00,
												0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
												0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
												0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
												0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
				ca_int8_t alu_plm_content1[] ={  0x00, 0x00, 0x05, 0x06, 0x00, 0x02, 0x00, 0x00,
												0x34, 0xd7, 0x92, 0x44, 0x58, 0x18, 0xae, 0x99,
												0x0d, 0x3f, 0x63, 0x54, 0xf0, 0x26, 0x72, 0xd1,
												0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
												0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
				ca_int8_t alu_plm_content2[] ={  0x00, 0x00, 0x05, 0x06, 0x00, 0x02, 0x00, 0x00,
												0x34, 0xd7, 0x92, 0x44, 0x58, 0x18, 0xae, 0x99,
												0x0d, 0x3f, 0x63, 0x54, 0xf0, 0x26, 0x72, 0xd1,
												0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00,
												0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
				ca_uint8_t alu_ploam_ik[] = {0xbd, 0xc5, 0x19, 0xe8, 0x65, 0x4f, 0xed, 0x8a,
										 	 0x9b, 0xa7, 0x08, 0xdd, 0x76, 0x80, 0x79, 0x54};
                if (value2 == 1){
                    memcpy(g_pon_tag, adtran_pon_tag,PON_TAG_SIZE);
                    xgpn_aes_key_generate(0);
                    ca_generate_omci_mic(0, 2, saturn_tx_omci_content, 44, 4, tx_omci_mic);
                    ploam_octet_dump("saturn_tx_omci_content-tx_omci_mic", tx_omci_mic, 4);
                }
				if (value2 == 2){
                    memcpy(g_pon_tag, zte_pon_tag,PON_TAG_SIZE);
                    xgpn_aes_key_generate(0);

                    ca_generate_omci_mic(0,2, zte_omci_content, 44, 4, tx_omci_mic);
                    ploam_octet_dump("zte_omci_mic", tx_omci_mic, 4);
                }
                if (value2 == 3){
                    memcpy(g_pon_tag, calix_pon_tag,PON_TAG_SIZE);
                    xgpn_aes_key_generate(0);
                    ca_generate_omci_mic(0, 1, calix_omci_content, 44, 4, tx_omci_mic);
                    ploam_octet_dump("calix_omci_content-tx_omci_mic", tx_omci_mic, 4);

                    ca_generate_omci_mic(0, 2, calix_omci_content, 44, 4, tx_omci_mic);
                    ploam_octet_dump("calix_omci_content-tx_omci_mic", tx_omci_mic, 4);
                }
                
				if (value2 == 4){
                    memcpy(g_pon_tag, mitra_pon_tag,PON_TAG_SIZE);
                    xgpn_aes_key_generate(0);
                    ret = ca_generate_ploam_mic(0,0x02, 40,mitr_plm_content, 8, ploam_mic);
					CA_RET_VALUE_CHECK(ret);
					ret = aal_ploam_ack_ploam_no_msg_set(0,(aal_xgpn_plmp_ack_ploam_no_msg_t *)ploam_mic);
					CA_RET_VALUE_CHECK(ret);
                    ploam_octet_dump("mitr_plm_content", mitr_plm_content, 40);
					ploam_octet_dump("ploam_mic", ploam_mic, 8);
                }
				if (value2 == 5){
                    memcpy(ploam_ik, alu_ploam_ik,16);
                    
                    ret = ca_generate_ploam_mic(0,0x02, 40,alu_plm_content1, 8, ploam_mic1);
					CA_RET_VALUE_CHECK(ret);
					ploam_octet_dump("ploam_ik", ploam_ik, 16);
					ploam_octet_dump("alu_plm_content1", alu_plm_content1, 40);
					ploam_octet_dump("ploam_mic1", ploam_mic1, 8);
					ret = ca_generate_ploam_mic(0,0x02, 40,alu_plm_content2, 8, ploam_mic2);
					CA_RET_VALUE_CHECK(ret);
					ploam_octet_dump("alu_plm_content2", alu_plm_content2, 40);
					ploam_octet_dump("ploam_mic2", ploam_mic2, 8);
                    
                }
                if (value2 == 10)
                {
                    memcpy(g_pon_tag, adtran_pon_tag,PON_TAG_SIZE);
                    xgpn_aes_key_generate(0);

                    saturn_tx_omci_content[0] = (value2 >> 24) & 0xFF;
                    saturn_tx_omci_content[1] = (value2 >> 16) & 0xFF;
                    saturn_tx_omci_content[2] = (value2 >> 8) & 0xFF;
                    saturn_tx_omci_content[3] = (value2 >> 0) & 0xFF;

                    ca_generate_omci_mic(0, 2, saturn_tx_omci_content, 44, 4, tx_omci_mic);
                    ploam_octet_dump("saturn_tx_omci_content-tx_omci_mic", tx_omci_mic, 4);
                }
            }
            break;

        case 13:
            aal_ploam_key_debug = value2;
            break;

        case 20:
            break;
        case 22:
            break;
        default:
            ca_printf("\r\nvalue1 for debug selection, valu2 for debug config\r\n");
            ca_printf("      0: Set valu2 to ploam_pkt_dump_debug\r\n ");
            ca_printf("      1: Set value2 to aal_ploam_debug\r\n ");
            ca_printf("      2: Ploam init, create WQ\r\n ");
            ca_printf("      3: Debug send msg\r\n ");
            ca_printf("      6: Interrupt debug flag\r\n ");
            ca_printf("      7: trigger GPIO interrupt\r\n ");
            ca_printf("      8: RGMII path test\r\n ");
            ca_printf("        value2: 1. enable loopback\r\n ");
            ca_printf("                2. remove loopback\r\n ");
            ca_printf("                3. read mib count\r\n ");
            ca_printf("      9: ca_tx OMCI\r\n ");
            ca_printf("      10: set da sa, default, 0x0013250000'xx', value2 is 'xx'\r\n ");
            ca_printf("      13: set ploam key debug, swap byte order\r\n ");
            ca_printf("      20: set ploam capture enable control\r\n ");
            ca_printf("      22: send event, event id = value2\r\n ");

            break;
    }

    return CA_E_OK;
}

ca_status_t aal_ploam_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
)
{

    if (value2) *value2 = aal_ploam_debug;

    ca_printf("ploam debug %d\r\n", aal_ploam_debug);

    return CA_E_OK;
}

ca_status_t aal_ploam_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_ctrl_t       *ctrl
)
{
    XGPN_PLMP_CTRL_t plmp_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    plmp_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_CTRL);

    if (ctrl_mask.bf.sw_mode_en)
        plmp_ctrl.bf.sw_mode_en = ctrl->sw_mode_en;
    if (ctrl_mask.bf.dereg_rst_en)
        plmp_ctrl.bf.dereg_rst_en = ctrl->dereg_rst_en;
    if (ctrl_mask.bf.resrv_bc_en)
        plmp_ctrl.bf.resrv_bc_en = ctrl->resrv_bc_en;
    if (ctrl_mask.bf.resrv_id_sel)
        plmp_ctrl.bf.resrv_id_sel = ctrl->resrv_id_sel;

    AAL_PON_REG_WRITE(plmp_ctrl.wrd, XGPN_PLMP_CTRL);
    PLOAM_DEBUG("write XGPN_PLMP_CTRL:0x%x\r\n", plmp_ctrl.wrd);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_ctrl_t  *ctrl
)
{
    XGPN_PLMP_CTRL_t  plmp_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    plmp_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_CTRL);

    PLOAM_DEBUG("read XGPN_PLMP_CTRL:0x%x\r\n", plmp_ctrl.wrd);

    ctrl->sw_mode_en   = plmp_ctrl.bf.sw_mode_en;
    ctrl->dereg_rst_en = plmp_ctrl.bf.dereg_rst_en;
    ctrl->resrv_bc_en  = plmp_ctrl.bf.resrv_bc_en;
    ctrl->resrv_id_sel = plmp_ctrl.bf.resrv_id_sel ;

    return CA_E_OK;
}


ca_status_t aal_ploam_ds_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_ds_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_ds_ctrl_t       *ctrl
)
{
    XGPN_PLMP_DS_CTRL_t plmp_ds_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    plmp_ds_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_DS_CTRL);

    if (ctrl_mask.bf.mic_chk_dis)
        plmp_ds_ctrl.bf.mic_chk_dis = ctrl->mic_chk_dis;
    if (ctrl_mask.bf.seq_chk_dis)
        plmp_ds_ctrl.bf.seq_chk_dis = ctrl->seq_chk_dis;
    if (ctrl_mask.bf.onuid_flt_dis)
        plmp_ds_ctrl.bf.onuid_flt_dis = ctrl->onuid_flt_dis;
    if (ctrl_mask.bf.capt_msg_en)
        plmp_ds_ctrl.bf.capt_msg_en = ctrl->capt_msg_en;
    if (ctrl_mask.bf.capt_msg_type)
        plmp_ds_ctrl.bf.capt_msg_type = ctrl->capt_msg_type;
    if (ctrl_mask.bf.capt_onu_id)
        plmp_ds_ctrl.bf.capt_onu_id = ctrl->capt_onu_id;
    if (ctrl_mask.bf.extension_en)
        plmp_ds_ctrl.bf.extension_en = ctrl->extension_en;
    if (ctrl_mask.bf.unkwn_msg_type_en)
        plmp_ds_ctrl.bf.unkwn_msg_type_en = ctrl->unkwn_msg_type_en;
    if (ctrl_mask.bf.dozing_filter_en)
        plmp_ds_ctrl.bf.dozing_filter_en = ctrl->dozing_filter_en;
    if (ctrl_mask.bf.o7_filter_en)
        plmp_ds_ctrl.bf.o7_filter_en = ctrl->o7_filter_en;

    PLOAM_DEBUG("write plmp_ctrl:0x%x\r\n", plmp_ds_ctrl.wrd);
    AAL_PON_REG_WRITE(plmp_ds_ctrl.wrd, XGPN_PLMP_DS_CTRL);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_ds_ctrl_t       *ctrl
)
{
    XGPN_PLMP_DS_CTRL_t  plmp_ds_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    plmp_ds_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_DS_CTRL);

    PLOAM_DEBUG("read plmp_ctrl:0x%x\r\n", plmp_ds_ctrl.wrd);

    ctrl->mic_chk_dis = plmp_ds_ctrl.bf.mic_chk_dis;
    ctrl->seq_chk_dis = plmp_ds_ctrl.bf.seq_chk_dis;
    ctrl->onuid_flt_dis = plmp_ds_ctrl.bf.onuid_flt_dis;
    ctrl->capt_msg_en = plmp_ds_ctrl.bf.capt_msg_en;
    ctrl->capt_msg_type = plmp_ds_ctrl.bf.capt_msg_type;
    ctrl->capt_onu_id = plmp_ds_ctrl.bf.capt_onu_id;
    ctrl->extension_en = plmp_ds_ctrl.bf.extension_en;
    ctrl->unkwn_msg_type_en = plmp_ds_ctrl.bf.unkwn_msg_type_en;
    ctrl->dozing_filter_en = plmp_ds_ctrl.bf.dozing_filter_en;
    ctrl->o7_filter_en = plmp_ds_ctrl.bf.o7_filter_en;

    return CA_E_OK;
}

ca_status_t aal_ploam_rx_capture_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          msg_en
)
{
    XGPN_PLMP_RX_CAPTURE_t cfg;

    PLOAM_DEBUG("msg_en:0x%08x\r\n", msg_en);

    __PLOAM_LOCK();

    cfg.bf.msg_en = msg_en;

    AAL_PON_REG_WRITE(cfg.wrd, XGPN_PLMP_RX_CAPTURE);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_rx_capture_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint32_t      *msg_en

)
{
    XGPN_PLMP_RX_CAPTURE_t cfg;

    AAL_ASSERT_RET(msg_en, CA_E_PARAM);

    cfg.wrd =  AAL_PON_REG_READ(XGPN_PLMP_RX_CAPTURE);

    *msg_en = cfg.wrd;

    PLOAM_DEBUG("msg_en:0x%08x\r\n", *msg_en);

    return CA_E_OK;
}



ca_status_t aal_ploam_us_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_boolean_t         mic_gen_dis
)
{
    XGPN_PLMP_US_CTRL_t plmp_us_ctrl;

    __PLOAM_LOCK();

    plmp_us_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_US_CTRL);

    plmp_us_ctrl.bf.mic_gen_dis = mic_gen_dis;

    PLOAM_DEBUG("write plmp_us_ctrl:0x%x\r\n", plmp_us_ctrl.wrd);
    AAL_PON_REG_WRITE(plmp_us_ctrl.wrd, XGPN_PLMP_US_CTRL);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_boolean_t         *mic_gen_dis
)
{
    XGPN_PLMP_US_CTRL_t  plmp_us_ctrl;

    AAL_ASSERT_RET(mic_gen_dis != NULL, CA_E_PARAM);

    plmp_us_ctrl.wrd  = AAL_PON_REG_READ(XGPN_PLMP_US_CTRL);

    PLOAM_DEBUG("read plmp_us_ctrl:0x%x\r\n", plmp_us_ctrl.wrd);

    *mic_gen_dis = plmp_us_ctrl.bf.mic_gen_dis;

    return CA_E_OK;
}


ca_status_t aal_ploam_vendor_id_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          vendor_id
)
{
    XGPN_PLMP_VENDOR_ID_CFG_t cfg;
    ca_uint8_t *vendor_p = (ca_uint8_t *)&vendor_id;

    PLOAM_DEBUG("vendor_id:0x%08x, [%c%c%c%c]\r\n", vendor_id, vendor_p[3], vendor_p[2], vendor_p[1], vendor_p[0]);

    __PLOAM_LOCK();

    cfg.bf.value = vendor_id;

    AAL_PON_REG_WRITE(cfg.wrd, XGPN_PLMP_VENDOR_ID_CFG);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_vendor_id_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *vendor_id
)
{
    XGPN_PLMP_VENDOR_ID_CFG_t cfg;
    ca_uint8_t *vendor_p = (ca_uint8_t *)vendor_id;

    AAL_ASSERT_RET(vendor_id, CA_E_PARAM);

    cfg.wrd =  AAL_PON_REG_READ(XGPN_PLMP_VENDOR_ID_CFG);

    *vendor_id = cfg.wrd;

    PLOAM_DEBUG("vendor_id:0x%08x, [%c%c%c%c]\r\n", *vendor_id, vendor_p[3], vendor_p[2], vendor_p[1], vendor_p[0]);

    return CA_E_OK;
}


ca_status_t aal_ploam_vssn_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          vssn
)
{
    XGPN_PLMP_VSSN_CFG_t cfg;

    PLOAM_DEBUG("vendor_id:0x%08x\r\n", vssn);

    __PLOAM_LOCK();

    cfg.bf.value = vssn;

    AAL_PON_REG_WRITE(cfg.wrd, XGPN_PLMP_VSSN_CFG);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_vssn_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *vssn
)
{
    XGPN_PLMP_VSSN_CFG_t cfg;

    AAL_ASSERT_RET(vssn, CA_E_PARAM);

    cfg.wrd =  AAL_PON_REG_READ(XGPN_PLMP_VSSN_CFG);

    *vssn = cfg.wrd;

    PLOAM_DEBUG("vssn:0x%08x\r\n", *vssn);

    return CA_E_OK;
}

ca_status_t aal_ploam_correlation_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          correlation
)
{
    XGPN_PLMP_CORRELATION_t cfg;

    PLOAM_DEBUG("correlation:0x%08x\r\n", correlation);

    cfg.wrd = 0;

    __PLOAM_LOCK();

    cfg.bf.tag = correlation;

    AAL_PON_REG_WRITE(cfg.wrd, XGPN_PLMP_CORRELATION);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_correlation_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint16_t         *correlation
)
{
    XGPN_PLMP_CORRELATION_t cfg;

    AAL_ASSERT_RET(correlation, CA_E_PARAM);

    cfg.wrd =  AAL_PON_REG_READ(XGPN_PLMP_CORRELATION);

    *correlation = cfg.bf.tag;

    PLOAM_DEBUG("correlation:0x%08x\r\n", *correlation);

    return CA_E_OK;
}

ca_status_t aal_ploam_us_pon_id_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          value
)
{
    XGPN_PLMP_US_PON_ID_t cfg;

    PLOAM_DEBUG("value:0x%08x\r\n", value);

    cfg.wrd = 0;

    __PLOAM_LOCK();

    cfg.bf.value = value;

    AAL_PON_REG_WRITE(cfg.wrd, XGPN_PLMP_US_PON_ID);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_us_pon_id_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *value
)
{
    XGPN_PLMP_US_PON_ID_t cfg;

    AAL_ASSERT_RET(value, CA_E_PARAM);

    cfg.wrd =  AAL_PON_REG_READ(XGPN_PLMP_US_PON_ID);

    *value = cfg.bf.value;

    PLOAM_DEBUG("value:0x%08x\r\n", *value);

    return CA_E_OK;
}

ca_status_t aal_ploam_calibration_even_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          value
)
{
    XGPN_PLMP_CALIBRATION_EVEN_t cfg;

    PLOAM_DEBUG("value:0x%08x\r\n", value);

    cfg.wrd = 0;

    __PLOAM_LOCK();

    cfg.bf.record = value;

    AAL_PON_REG_WRITE(cfg.wrd, XGPN_PLMP_CALIBRATION_EVEN);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_calibration_even_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *value
)
{
    XGPN_PLMP_CALIBRATION_EVEN_t cfg;

    AAL_ASSERT_RET(value, CA_E_PARAM);

    cfg.wrd =  AAL_PON_REG_READ(XGPN_PLMP_CALIBRATION_EVEN);

    *value = cfg.bf.record;

    PLOAM_DEBUG("value:0x%08x\r\n", *value);

    return CA_E_OK;
}

ca_status_t aal_ploam_calibration_odd_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          value
)
{
    XGPN_PLMP_CALIBRATION_ODD_t cfg;

    PLOAM_DEBUG("value:0x%08x\r\n", value);

    cfg.wrd = 0;

    __PLOAM_LOCK();

    cfg.bf.record = value;

    AAL_PON_REG_WRITE(cfg.wrd, XGPN_PLMP_CALIBRATION_ODD);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_calibration_odd_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *value
)
{
    XGPN_PLMP_CALIBRATION_ODD_t cfg;

    AAL_ASSERT_RET(value, CA_E_PARAM);

    cfg.wrd =  AAL_PON_REG_READ(XGPN_PLMP_CALIBRATION_ODD);

    *value = cfg.bf.record;

    PLOAM_DEBUG("value:0x%08x\r\n", *value);

    return CA_E_OK;
}


ca_status_t aal_ploam_tuning_parameter_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_tuning_parameter_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_tuning_parameter_t       *ctrl
)
{
    XGPN_PLMP_TUNING_PARAMETER_t reg_data;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_data.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TUNING_PARAMETER);

    if (ctrl_mask.bf.granularity)
        reg_data.bf.granularity = ctrl->granularity;
    if (ctrl_mask.bf.time)
        reg_data.bf.time = ctrl->time;

    PLOAM_DEBUG("write plmp reg, granularity:%d, time:%d, ret:0x%x\r\n",
        reg_data.bf.granularity, reg_data.bf.time, reg_data.wrd);

    AAL_PON_REG_WRITE(reg_data.wrd, XGPN_PLMP_TUNING_PARAMETER);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_tuning_parameter_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_tuning_parameter_t       *ctrl
)
{
    XGPN_PLMP_TUNING_PARAMETER_t  reg_data;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_data.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TUNING_PARAMETER);

    ctrl->granularity = reg_data.bf.granularity;
    ctrl->time = reg_data.bf.time;

    PLOAM_DEBUG("read plmp reg:0x%x, granularity:%d, time:%d\r\n",
        reg_data.wrd, ctrl->granularity, ctrl->time);

    return CA_E_OK;
}

ca_status_t aal_ploam_attenuation_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_attenuation_mask_t  mask,
    CA_IN aal_xgpn_plmp_attenuation_t       *data
)
{
    XGPN_PLMP_ATTENUATION_t reg_data;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_data.wrd  = AAL_PON_REG_READ(XGPN_PLMP_ATTENUATION);

    if (mask.bf.level)
        reg_data.bf.level = data->level;
    if (mask.bf.capability)
        reg_data.bf.capability = data->capability;

    PLOAM_DEBUG("write plmp reg ret:0x%x, level:%d, capability:%d\r\n", reg_data.wrd,
            reg_data.bf.level, reg_data.bf.capability);
    AAL_PON_REG_WRITE(reg_data.wrd, XGPN_PLMP_ATTENUATION);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_attenuation_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_attenuation_t       *data
)
{
    XGPN_PLMP_ATTENUATION_t  reg_data;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    reg_data.wrd  = AAL_PON_REG_READ(XGPN_PLMP_ATTENUATION);

    PLOAM_DEBUG("read plmp reg ret:0x%x, level:%d, capability:%d\r\n", reg_data.wrd,
            reg_data.bf.level, reg_data.bf.capability);

    data->level = reg_data.bf.level;
    data->capability = reg_data.bf.capability;

    return CA_E_OK;
}

ca_status_t aal_ploam_rgst_id_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_rgst_id_cfg_t  *rgst_id
)
{
    aal_ploam_rgst_id_cfg_t reg_cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(rgst_id != NULL, CA_E_PARAM);

    if (aal_ploam_debug)
        ploam_octet_dump("rgst_id", rgst_id->data, XGPN_PLMP_RGST_ID_SIZE);

    __PLOAM_LOCK();

    reg_addr = XGPN_PLMP_RGST_ID_CFG8;

    for (i = 0; i < XGPN_PLMP_RGST_ID_SIZE/4; i++){
        reg_cfg.wrd = ((ca_uint32_t)(rgst_id->data[i*4])<< 24)
            | ((ca_uint32_t)(rgst_id->data[i*4 + 1]) << 16)
            | ((ca_uint32_t)(rgst_id->data[i*4 + 2]) << 8)
            | ((ca_uint32_t)(rgst_id->data[i*4 + 3]));
        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        reg_addr -= 4;
    }
    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_rgst_id_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_xgpn_plmp_rgst_id_cfg_t *rgst_id
)
{
    aal_ploam_rgst_id_cfg_t reg_cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(rgst_id != NULL, CA_E_PARAM);

    __PLOAM_LOCK();
    reg_addr = XGPN_PLMP_RGST_ID_CFG8;

    for (i = 0; i < XGPN_PLMP_RGST_ID_SIZE/4; i++){
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        rgst_id->data[i * 4] = (reg_cfg.wrd >> 24) & 0xFF;
        rgst_id->data[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        rgst_id->data[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        rgst_id->data[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
        reg_addr -= 4;
    }
    __PLOAM_UNLOCK();

    if (aal_ploam_debug)
        ploam_octet_dump("rgst_id", rgst_id->data, XGPN_PLMP_RGST_ID_SIZE);

    return CA_E_OK;
}


ca_status_t aal_ploam_eqd_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          eqd
)
{
    XGPN_PLMP_EQD_THRSHLD_t cfg;

    PLOAM_DEBUG("eqd:0x%08x\r\n", eqd);

    __PLOAM_LOCK();

    cfg.bf.value = eqd;

    AAL_PON_REG_WRITE(cfg.wrd, XGPN_PLMP_EQD_THRSHLD);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_eqd_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT ca_uint32_t         *eqd
)
{
    XGPN_PLMP_EQD_THRSHLD_t cfg;

    AAL_ASSERT_RET(eqd, CA_E_PARAM);

    cfg.wrd =  AAL_PON_REG_READ(XGPN_PLMP_EQD_THRSHLD);

    *eqd = cfg.wrd;

    PLOAM_DEBUG("eqd:0x%08x\r\n", *eqd);

    return CA_E_OK;
}

ca_status_t aal_ploam_ack_ploam_no_msg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_ack_ploam_no_msg_t  *mic
)
{
    aal_reg_ploam_ack_ploam_mic_t cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(mic != NULL, CA_E_PARAM);

    if (aal_ploam_debug)
        ploam_octet_dump("mic", mic->mic, XGPN_PLMP_MIC_SIZE);

    __PLOAM_LOCK();

    for (i = 0; i < 2; i++){
        reg_addr = XGPN_PLMP_ACK_PLOAM_NO_MSG0 + i * 4;
        cfg.bf.mic0 = mic->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 1];
        cfg.bf.mic1 = mic->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 2];
        cfg.bf.mic2 = mic->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 3];
        cfg.bf.mic3 = mic->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 4];
        AAL_PON_REG_WRITE(cfg.wrd, reg_addr);
    }

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ack_ploam_no_msg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_xgpn_plmp_ack_ploam_no_msg_t *mic
)
{
    aal_reg_ploam_ack_ploam_mic_t cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(mic != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    for (i = 0; i < 2; i++){
        reg_addr = XGPN_PLMP_ACK_PLOAM_NO_MSG0 + i * 4;
        cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        mic->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 4] = cfg.bf.mic0;
        mic->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 3] = cfg.bf.mic1;
        mic->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 2] = cfg.bf.mic2;
        mic->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 1] = cfg.bf.mic3;
    }

    __PLOAM_UNLOCK();

    if (aal_ploam_debug)
        ploam_octet_dump("mic", mic->mic, XGPN_PLMP_MIC_SIZE);

    return CA_E_OK;
}

ca_status_t aal_ploam_ack_ploam_busy_msg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_ack_ploam_busy_msg_mask_t mask,
    CA_IN aal_xgpn_plmp_ack_ploam_busy_msg_t  *busy_msg
)
{
    aal_reg_ploam_ack_ploam_mic_t mic;
    XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG_t msg_cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(busy_msg != NULL, CA_E_PARAM);

    if (aal_ploam_debug)
        ploam_octet_dump("mic", busy_msg->mic, XGPN_PLMP_MIC_SIZE);

    PLOAM_DEBUG("start:%d, seq:0x%x\r\n", busy_msg->start, busy_msg->seq);

    __PLOAM_LOCK();

    if (mask.bf.start)
        msg_cfg.bf.start = busy_msg->start;

    if (mask.bf.seq)
        msg_cfg.bf.seq = busy_msg->seq;

    AAL_PON_REG_WRITE(msg_cfg.wrd, XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG);

    if (mask.bf.mic){
        for (i = 0; i < 2; i++){
            reg_addr = XGPN_PLMP_ACK_PLOAM_BUSY_MSG0 + i * 4;
            mic.bf.mic0 = busy_msg->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 4];
            mic.bf.mic1 = busy_msg->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 3];
            mic.bf.mic2 = busy_msg->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 2];
            mic.bf.mic3 = busy_msg->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 1];
            AAL_PON_REG_WRITE(mic.wrd, reg_addr);
        }
    }

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ack_ploam_busy_msg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_xgpn_plmp_ack_ploam_busy_msg_t *busy_msg
)
{
    aal_reg_ploam_ack_ploam_mic_t mic;
    XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG_t msg_cfg;

    int i, reg_addr;

    AAL_ASSERT_RET(busy_msg != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    msg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_ACK_PLOAM_BUSY_MSG_CFG);
    busy_msg->ok = msg_cfg.bf.ok;
    busy_msg->seq = msg_cfg.bf.seq;
    busy_msg->start = msg_cfg.bf.start;

    for (i = 0; i < 2; i++){
        reg_addr = XGPN_PLMP_ACK_PLOAM_BUSY_MSG0 + i * 4;
        mic.wrd  = AAL_PON_REG_READ(reg_addr);
        busy_msg->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 4] = mic.bf.mic0;
        busy_msg->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 3] = mic.bf.mic1;
        busy_msg->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 2] = mic.bf.mic2;
        busy_msg->mic[XGPN_PLMP_MIC_SIZE - i * 4 - 1] = mic.bf.mic3;
    }

    __PLOAM_UNLOCK();

    if (aal_ploam_debug)
        ploam_octet_dump("mic", busy_msg->mic, XGPN_PLMP_MIC_SIZE);
    PLOAM_DEBUG("ok:%d, seq:%d, start:%d\r\n", busy_msg->ok, busy_msg->seq, busy_msg->start);

    return CA_E_OK;
}


ca_status_t aal_ploam_cfg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_cfg_t  *plmp_cfg
)
{
    aal_reg_ploam_cfg_t reg_cfg;
    XGPN_PLMP_PLOAM_CFG_t cfg_cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(plmp_cfg != NULL, CA_E_PARAM);

    if (aal_ploam_debug)
        ploam_octet_dump("data", plmp_cfg->data, XGPN_PLMP_MSG_BODY_SIZE);

    PLOAM_DEBUG("done:%d, msg_type:0x%x\r\n", plmp_cfg->done, plmp_cfg->msg_type);

    __PLOAM_LOCK();

    cfg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_PLOAM_CFG);

    cfg_cfg.bf.done = plmp_cfg->done;
    cfg_cfg.bf.msg_type = plmp_cfg->msg_type;

    for (i = 0; i < XGPN_PLMP_MSG_BODY_SIZE/4; i++){
        reg_addr = XGPN_PLMP_PLOAM_CFG_DATA0 + i * 4;
        reg_cfg.wrd = ((ca_uint32_t)(plmp_cfg->data[i * 4]) << 24 )
            + (plmp_cfg->data[i * 4 + 1] << 16)
            + (plmp_cfg->data[i * 4 + 2] << 8)
            + (plmp_cfg->data[i * 4 + 3] );

        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        PLOAM_DEBUG("XGPN_PLMP_PLOAM_CFG_DATA%d = 0x%08x\n", i, reg_cfg.wrd);
    }

    AAL_PON_REG_WRITE(cfg_cfg.wrd, XGPN_PLMP_PLOAM_CFG);
    PLOAM_DEBUG("XGPN_PLMP_PLOAM_CFG = 0x%08x\n", cfg_cfg.wrd);
    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_cfg_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_xgpn_plmp_cfg_t *plmp_cfg
)
{
    aal_reg_ploam_cfg_t reg_cfg;
    XGPN_PLMP_PLOAM_CFG_t cfg_cfg;

    int i, reg_addr;

    AAL_ASSERT_RET(plmp_cfg != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    cfg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_PLOAM_CFG);
    plmp_cfg->done = cfg_cfg.bf.done;
    plmp_cfg->msg_type = cfg_cfg.bf.msg_type;

    for (i = 0; i < XGPN_PLMP_MSG_BODY_SIZE/4; i++){
        reg_addr = XGPN_PLMP_PLOAM_CFG_DATA0 + i * 4;
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);

        plmp_cfg->data[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
        plmp_cfg->data[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        plmp_cfg->data[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        plmp_cfg->data[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
    }

    __PLOAM_UNLOCK();

    if (aal_ploam_debug)
        ploam_octet_dump("data", plmp_cfg->data, XGPN_PLMP_MSG_BODY_SIZE);
    PLOAM_DEBUG("done:%d, msg_type:%d\r\n", plmp_cfg->done, plmp_cfg->msg_type);

    return CA_E_OK;
}


ca_status_t aal_ploam_key_exchg_thrshd_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_key_exchg_thrshd_mask_t  thrshd_mask,
    CA_IN aal_xgpn_plmp_key_exchg_thrshd_t       *thrshd
)
{
    XGPN_PLMP_KEY_EXCHG_THRSHD_t reg_thrshd;

    AAL_ASSERT_RET(thrshd != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_thrshd.wrd  = AAL_PON_REG_READ(XGPN_PLMP_KEY_EXCHG_THRSHD);

    if (thrshd_mask.bf.tk5_timeout)
        reg_thrshd.bf.tk5_timeout = thrshd->tk5_timeout;
    if (thrshd_mask.bf.tk4_timeout)
        reg_thrshd.bf.tk4_timeout = thrshd->tk4_timeout;

    PLOAM_DEBUG("write:0x%x\r\n", reg_thrshd.wrd);
    AAL_PON_REG_WRITE(reg_thrshd.wrd, XGPN_PLMP_KEY_EXCHG_THRSHD);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_key_exchg_thrshd_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_key_exchg_thrshd_t  *thrshd
)
{
    XGPN_PLMP_KEY_EXCHG_THRSHD_t reg_thrshd;

    AAL_ASSERT_RET(thrshd != NULL, CA_E_PARAM);

    reg_thrshd.wrd  = AAL_PON_REG_READ(XGPN_PLMP_KEY_EXCHG_THRSHD);

    PLOAM_DEBUG("read :0x%x\r\n", reg_thrshd.wrd);

    thrshd->tk5_timeout = reg_thrshd.bf.tk5_timeout;
    thrshd->tk4_timeout = reg_thrshd.bf.tk4_timeout;

    return CA_E_OK;
}


ca_status_t aal_ploam_interrupt_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_interrupt_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_interrupt_t       *ctrl
)
{
    XGPN_PLMP_INTERRUPT_t reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_INTERRUPT);

    if (ctrl_mask.bf.wrong_onuid_inti)
        reg_cfg.bf.wrong_onuid_intI = ctrl->wrong_onuid_inti;
    if (ctrl_mask.bf.bc_seq_mismch_inti)
        reg_cfg.bf.bc_seq_mismch_intI = ctrl->bc_seq_mismch_inti;
    if (ctrl_mask.bf.uc_seq_mismch_inti)
        reg_cfg.bf.uc_seq_mismch_intI = ctrl->uc_seq_mismch_inti;
    if (ctrl_mask.bf.mic_chk_fail_inti)
        reg_cfg.bf.mic_chk_fail_intI = ctrl->mic_chk_fail_inti;
    if (ctrl_mask.bf.wrong_ploam_inti)
        reg_cfg.bf.wrong_ploam_intI = ctrl->wrong_ploam_inti;
    if (ctrl_mask.bf.key_exchg_tk4_timeout_inti)
        reg_cfg.bf.key_exchg_tk4_timeout_intI = ctrl->key_exchg_tk4_timeout_inti;
    if (ctrl_mask.bf.key_exchg_tk5_timeout_inti)
        reg_cfg.bf.key_exchg_tk5_timeout_intI = ctrl->key_exchg_tk5_timeout_inti;
    if (ctrl_mask.bf.key_ctrl_inti)
        reg_cfg.bf.key_ctrl_intI = ctrl->key_ctrl_inti;
    if (ctrl_mask.bf.sleep_allow_inti)
        reg_cfg.bf.sleep_allow_intI = ctrl->sleep_allow_inti;
    if (ctrl_mask.bf.sleep_req_done_inti)
        reg_cfg.bf.sleep_req_done_intI = ctrl->sleep_req_done_inti;
    if (ctrl_mask.bf.sleep_req_done_inti)
        reg_cfg.bf.sleep_req_done_intI = ctrl->sleep_req_done_inti;
    if (ctrl_mask.bf.eqd_err_inti)
        reg_cfg.bf.eqd_err_intI = ctrl->eqd_err_inti;
    if (ctrl_mask.bf.busy_err_inti)
        reg_cfg.bf.busy_err_intI = ctrl->busy_err_inti;
    if (ctrl_mask.bf.registration_msg_inti)
        reg_cfg.bf.registration_msg_intI = ctrl->registration_msg_inti;
    if (ctrl_mask.bf.rxbuf0_rdy_inti)
        reg_cfg.bf.rxbuf0_rdy_intI = ctrl->rxbuf0_rdy_inti;
    if (ctrl_mask.bf.rxbuf1_rdy_inti)
        reg_cfg.bf.rxbuf1_rdy_intI = ctrl->rxbuf1_rdy_inti;
#if 1
    if (ctrl_mask.bf.tuning_request)
        reg_cfg.bf.tuning_request = ctrl->tuning_request;
#endif

    PLOAM_DEBUG("write XGPN_PLMP_INTERRUPT:0x%x\r\n", reg_cfg.wrd);
    AAL_PON_REG_WRITE(reg_cfg.wrd, XGPN_PLMP_INTERRUPT);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_interrupt_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_interrupt_t       *ctrl
)
{
    XGPN_PLMP_INTERRUPT_t  reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_INTERRUPT);

    ctrl->wrong_onuid_inti = reg_cfg.bf.wrong_onuid_intI;
    ctrl->bc_seq_mismch_inti = reg_cfg.bf.bc_seq_mismch_intI;
    ctrl->uc_seq_mismch_inti = reg_cfg.bf.uc_seq_mismch_intI;
    ctrl->mic_chk_fail_inti = reg_cfg.bf.mic_chk_fail_intI;
    ctrl->wrong_ploam_inti = reg_cfg.bf.wrong_ploam_intI;
    ctrl->key_exchg_tk4_timeout_inti = reg_cfg.bf.key_exchg_tk4_timeout_intI;
    ctrl->key_exchg_tk5_timeout_inti = reg_cfg.bf.key_exchg_tk5_timeout_intI;
    ctrl->key_ctrl_inti = reg_cfg.bf.key_ctrl_intI;
    ctrl->sleep_allow_inti = reg_cfg.bf.sleep_allow_intI;
    ctrl->sleep_req_done_inti = reg_cfg.bf.sleep_req_done_intI;
    ctrl->eqd_err_inti = reg_cfg.bf.eqd_err_intI;
    ctrl->busy_err_inti = reg_cfg.bf.busy_err_intI;
    ctrl->registration_msg_inti = reg_cfg.bf.registration_msg_intI;
    ctrl->burst_prfl_inti = reg_cfg.bf.burst_prfl_intI;
    ctrl->rxbuf0_rdy_inti = reg_cfg.bf.rxbuf0_rdy_intI;
    ctrl->rxbuf1_rdy_inti = reg_cfg.bf.rxbuf1_rdy_intI;
    ctrl->tuning_request = reg_cfg.bf.tuning_request;

    return CA_E_OK;
}

ca_status_t aal_ploam_interrupt_enable0_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_interrupt_enable_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_interrupt_enable_t       *ctrl
)
{
    XGPN_PLMP_INTERRUPTE0_t reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_INTERRUPTE0);

    if (ctrl_mask.bf.wrong_onuid_intie)
        reg_cfg.bf.wrong_onuid_intIE = ctrl->wrong_onuid_intie;
    if (ctrl_mask.bf.bc_seq_mismch_intie)
        reg_cfg.bf.bc_seq_mismch_intIE = ctrl->bc_seq_mismch_intie;
    if (ctrl_mask.bf.uc_seq_mismch_intie)
        reg_cfg.bf.uc_seq_mismch_intIE = ctrl->uc_seq_mismch_intie;
    if (ctrl_mask.bf.mic_chk_fail_intie)
        reg_cfg.bf.mic_chk_fail_intIE = ctrl->mic_chk_fail_intie;
    if (ctrl_mask.bf.wrong_ploam_intie)
        reg_cfg.bf.wrong_ploam_intIE = ctrl->wrong_ploam_intie;
    if (ctrl_mask.bf.key_exchg_tk4_timeout_intie)
        reg_cfg.bf.key_exchg_tk4_timeout_intIE = ctrl->key_exchg_tk4_timeout_intie;
    if (ctrl_mask.bf.key_exchg_tk5_timeout_intie)
        reg_cfg.bf.key_exchg_tk5_timeout_intIE = ctrl->key_exchg_tk5_timeout_intie;
    if (ctrl_mask.bf.key_ctrl_intie)
        reg_cfg.bf.key_ctrl_intIE = ctrl->key_ctrl_intie;
    if (ctrl_mask.bf.sleep_allow_intie)
        reg_cfg.bf.sleep_allow_intIE = ctrl->sleep_allow_intie;
    if (ctrl_mask.bf.sleep_req_done_intie)
        reg_cfg.bf.sleep_req_done_intIE = ctrl->sleep_req_done_intie;
    if (ctrl_mask.bf.eqd_err_intie)
        reg_cfg.bf.eqd_err_intIE = ctrl->eqd_err_intie;
    if (ctrl_mask.bf.busy_err_intie)
        reg_cfg.bf.busy_err_intIE = ctrl->busy_err_intie;
    if (ctrl_mask.bf.registration_msg_intie)
        reg_cfg.bf.registration_msg_intIE = ctrl->registration_msg_intie;
    if (ctrl_mask.bf.burst_prfl_intie)
        reg_cfg.bf.burst_prfl_intIE = ctrl->burst_prfl_intie;
    if (ctrl_mask.bf.rxbuf0_rdy_intie)
        reg_cfg.bf.rxbuf0_rdy_intIE = ctrl->rxbuf0_rdy_intie;
    if (ctrl_mask.bf.rxbuf1_rdy_intie)
        reg_cfg.bf.rxbuf1_rdy_intIE = ctrl->rxbuf1_rdy_intie;


    PLOAM_DEBUG("write plmp_ctrl:0x%x\r\n", reg_cfg.wrd);
    AAL_PON_REG_WRITE(reg_cfg.wrd, XGPN_PLMP_INTERRUPTE0);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_interrupt_enable0_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_interrupt_enable_t       *ctrl
)
{
    XGPN_PLMP_INTERRUPTE0_t  reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_INTERRUPTE0);

    PLOAM_DEBUG("read XGPN_PLMP_INTERRUPTE0[0x%x]:0x%x\r\n", XGPN_PLMP_INTERRUPTE0, reg_cfg.wrd);

    ctrl->wrong_onuid_intie = reg_cfg.bf.wrong_onuid_intIE;
    ctrl->bc_seq_mismch_intie = reg_cfg.bf.bc_seq_mismch_intIE;
    ctrl->uc_seq_mismch_intie = reg_cfg.bf.uc_seq_mismch_intIE;
    ctrl->mic_chk_fail_intie = reg_cfg.bf.mic_chk_fail_intIE;
    ctrl->wrong_ploam_intie = reg_cfg.bf.wrong_ploam_intIE;
    ctrl->key_exchg_tk4_timeout_intie = reg_cfg.bf.key_exchg_tk4_timeout_intIE;
    ctrl->key_exchg_tk5_timeout_intie = reg_cfg.bf.key_exchg_tk5_timeout_intIE;
    ctrl->key_ctrl_intie = reg_cfg.bf.key_ctrl_intIE;
    ctrl->sleep_allow_intie = reg_cfg.bf.sleep_allow_intIE;
    ctrl->sleep_req_done_intie = reg_cfg.bf.sleep_req_done_intIE;
    ctrl->eqd_err_intie = reg_cfg.bf.eqd_err_intIE;
    ctrl->busy_err_intie = reg_cfg.bf.busy_err_intIE;
    ctrl->registration_msg_intie = reg_cfg.bf.registration_msg_intIE;
    ctrl->burst_prfl_intie = reg_cfg.bf.burst_prfl_intIE;
    ctrl->rxbuf0_rdy_intie = reg_cfg.bf.rxbuf0_rdy_intIE;
    ctrl->rxbuf1_rdy_intie = reg_cfg.bf.rxbuf1_rdy_intIE;

    return CA_E_OK;
}




ca_status_t aal_ploam_key_ctrl_stat_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_key_ctrl_stat_t       *ctrl
)
{
    XGPN_PLMP_KEY_CTRL_STAT_t  reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_KEY_CTRL_STAT);

    PLOAM_DEBUG("read plmp_ctrl:0x%x\r\n", reg_cfg.wrd);

    ctrl->seq = reg_cfg.bf.seq;
    ctrl->type = reg_cfg.bf.type;
    ctrl->index = reg_cfg.bf.index;
    ctrl->length = reg_cfg.bf.length;

    return CA_E_OK;
}


ca_status_t aal_ploam_key_report_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_key_report_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_key_report_t       *ctrl
)
{
    XGPN_PLMP_KEY_REPORT_t reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    //__PLOAM_LOCK();

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_KEY_REPORT);

    if (ctrl_mask.bf.seq)
        reg_cfg.bf.seq = ctrl->seq;
    if (ctrl_mask.bf.type)
        reg_cfg.bf.type = ctrl->type;
    if (ctrl_mask.bf.index)
        reg_cfg.bf.index = ctrl->index;
    if (ctrl_mask.bf.fragment_num)
        reg_cfg.bf.fragment_num = ctrl->fragment_num;
    if (ctrl_mask.bf.rdy)
        reg_cfg.bf.rdy = ctrl->rdy;

    PLOAM_DEBUG("write plmp_ctrl:0x%x\r\n", reg_cfg.wrd);
    AAL_PON_REG_WRITE(reg_cfg.wrd, XGPN_PLMP_KEY_REPORT);

    //__PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_key_report_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_key_report_t       *ctrl
)
{
    XGPN_PLMP_KEY_REPORT_t  reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_KEY_REPORT);

    PLOAM_DEBUG("read plmp_ctrl:0x%x\r\n", reg_cfg.wrd);

    ctrl->seq = reg_cfg.bf.seq;
    ctrl->type = reg_cfg.bf.type;
    ctrl->index = reg_cfg.bf.index;
    ctrl->fragment_num = reg_cfg.bf.fragment_num;
    ctrl->rdy = reg_cfg.bf.rdy;

    return CA_E_OK;
}


ca_status_t aal_ploam_key_report_frag_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_key_report_frag_t  *frag
)
{
    aal_reg_xgpn_plmp_key_report_frag_t msg_cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(frag != NULL, CA_E_PARAM);

    if (aal_ploam_debug)
        ploam_octet_dump("data", frag->data, XGPN_PLMP_KEY_REPORT_FRAG_SIZE);

    //__PLOAM_LOCK();

    for (i = 0; i < XGPN_PLMP_KEY_REPORT_FRAG_SIZE / 4; i++){
            reg_addr = XGPN_PLMP_KEY_REPORT_FRAG0 + i * 4;
            if (aal_ploam_key_debug & 0x1) {
                msg_cfg.bf.byte0 = frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 1];
                msg_cfg.bf.byte1 = frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 2];
                msg_cfg.bf.byte2 = frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 3];
                msg_cfg.bf.byte3 = frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 4];

            } else {
                msg_cfg.bf.byte0 = frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 4];
                msg_cfg.bf.byte1 = frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 3];
                msg_cfg.bf.byte2 = frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 2];
                msg_cfg.bf.byte3 = frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 1];
            }
            AAL_PON_REG_WRITE(msg_cfg.wrd, reg_addr);
    }

    //__PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_key_report_frag_get(
    CA_IN ca_device_id_t          device_id,
    CA_OUT aal_key_report_frag_t *frag
)
{
    aal_reg_xgpn_plmp_key_report_frag_t msg_cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(frag != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    for (i = 0; i < XGPN_PLMP_KEY_REPORT_FRAG_SIZE / 4; i++){
        reg_addr = XGPN_PLMP_KEY_REPORT_FRAG0 + i * 4;
        msg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 4] = msg_cfg.bf.byte0;
        frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 3] = msg_cfg.bf.byte1;
        frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 2] = msg_cfg.bf.byte2;
        frag->data[XGPN_PLMP_KEY_REPORT_FRAG_SIZE - i * 4 - 1] = msg_cfg.bf.byte3;
    }

    __PLOAM_UNLOCK();

    if (aal_ploam_debug)
        ploam_octet_dump("data", frag->data, XGPN_PLMP_KEY_REPORT_FRAG_SIZE);

    return CA_E_OK;
}

ca_status_t aal_ploam_key_sleep_allow_stat_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_sleep_allow_stat_t       *ctrl
)
{
    XGPN_PLMP_SLEEP_ALLOW_STAT_t  reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_SLEEP_ALLOW_STAT);

    PLOAM_DEBUG("read plmp_ctrl:0x%x\r\n", reg_cfg.wrd);

    ctrl->seq = reg_cfg.bf.seq;
    ctrl->type = reg_cfg.bf.type;

    return CA_E_OK;
}


ca_status_t aal_ploam_sleep_req_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_sleep_req_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_sleep_req_t       *ctrl
)
{
    XGPN_PLMP_SLEEP_REQ_t reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_SLEEP_REQ);

    if (ctrl_mask.bf.activity_level)
        reg_cfg.bf.activity_level = ctrl->activity_level;
    if (ctrl_mask.bf.rdy)
        reg_cfg.bf.rdy = ctrl->rdy;

    PLOAM_DEBUG("write plmp_ctrl:0x%x\r\n", reg_cfg.wrd);
    AAL_PON_REG_WRITE(reg_cfg.wrd, XGPN_PLMP_SLEEP_REQ);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_sleep_req_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_sleep_req_t       *ctrl
)
{
    XGPN_PLMP_SLEEP_REQ_t  reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_SLEEP_REQ);

    PLOAM_DEBUG("read plmp_ctrl:0x%x\r\n", reg_cfg.wrd);

    ctrl->activity_level = reg_cfg.bf.activity_level;
    ctrl->rdy = reg_cfg.bf.rdy;

    return CA_E_OK;
}


ca_status_t aal_ploam_txbuf_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_txbuf_ctrl_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_txbuf_ctrl_t       *ctrl
)
{
    XGPN_PLMP_TXBUF_CTRL_t reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);

    if (ctrl_mask.bf.defkey)
        reg_cfg.bf.defkey = ctrl->defkey;
    if (ctrl_mask.bf.pkt_rdy)
        reg_cfg.bf.pkt_rdy = ctrl->pkt_rdy;

    AAL_PON_REG_WRITE(reg_cfg.wrd, XGPN_PLMP_TXBUF_CTRL);
    PLOAM_DEBUG("write XGPN_PLMP_TXBUF_CTRL:0x%x\r\n", reg_cfg.wrd);

    return CA_E_OK;

}

ca_status_t aal_ploam_txbuf_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_txbuf_ctrl_t       *ctrl
)
{
    XGPN_PLMP_TXBUF_CTRL_t  reg_cfg;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(XGPN_PLMP_TXBUF_CTRL);

    PLOAM_DEBUG("read plmp_ctrl:0x%x\r\n", reg_cfg.wrd);

    ctrl->defkey = reg_cfg.bf.defkey;
    ctrl->pkt_rdy = reg_cfg.bf.pkt_rdy;

    return CA_E_OK;
}


ca_status_t aal_ploam_txbuf_data_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_xgpn_plmp_txbuf_t   *txbuf
)
{
    aal_reg_xgpn_plmp_txbuf_data_t reg_cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(txbuf != NULL, CA_E_PARAM);

    if (aal_ploam_debug)
        ploam_octet_dump("data", txbuf->data, XGPN_PLMP_TXBUF_SIZE);

    //__PLOAM_LOCK();

    reg_addr = XGPN_PLMP_TXBUF_DATA0;

    for (i = 0; i < XGPN_PLMP_TXBUF_SIZE/4; i++){
        reg_cfg.wrd = ((ca_uint32_t)(txbuf->data[i*4]) << 24 )
            | ((ca_uint32_t)(txbuf->data[i*4 + 1]) << 16)
            | ((ca_uint32_t)(txbuf->data[i*4 + 2]) << 8)
            | ((ca_uint32_t)(txbuf->data[i*4 + 3]));
        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        reg_addr += 4;
    }

    //__PLOAM_UNLOCK();
    return CA_E_OK;

}

ca_status_t aal_ploam_txbuf_data_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_xgpn_plmp_txbuf_t       *txbuf
)
{
    aal_reg_xgpn_plmp_txbuf_data_t  reg_cfg;
    int i, reg_addr;

    AAL_ASSERT_RET(txbuf != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_addr = XGPN_PLMP_TXBUF_DATA0;

    for (i = 0; i < XGPN_PLMP_TXBUF_SIZE; i += 4){
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        txbuf->data[i] = (reg_cfg.wrd >> 24 ) & 0xFF;
        txbuf->data[i + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        txbuf->data[i + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        txbuf->data[i + 3] = (reg_cfg.wrd) & 0xFF;
        reg_addr += 4;
    }

    __PLOAM_UNLOCK();

    if (aal_ploam_debug)
        ploam_octet_dump("data", txbuf->data, XGPN_PLMP_TXBUF_SIZE);

    return CA_E_OK;
}

ca_status_t aal_ploam_rxbuf_stat_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          buf_idx,   /* 0..1 */
    CA_IN aal_xgpn_plmp_rxbuf_stat_mask_t  ctrl_mask,
    CA_IN aal_xgpn_plmp_rxbuf_stat_t       *ctrl
)
{
    XGPN_PLMP_RXBUF0_STAT_t reg_cfg;
    ca_uint32_t reg_addr = XGPN_PLMP_RXBUF0_STAT + buf_idx * 4;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);

    if (ctrl_mask.bf.mic_err)
        reg_cfg.bf.mic_err = ctrl->mic_err;
    if (ctrl_mask.bf.rx_sfc)
        reg_cfg.bf.rx_sfc = ctrl->rx_sfc;
    if (ctrl_mask.bf.turn)
        reg_cfg.bf.turn = ctrl->turn;
    if (ctrl_mask.bf.pkt_rdy)
        reg_cfg.bf.pkt_rdy = ctrl->pkt_rdy;

    PLOAM_DEBUG("write [0x%08x]:0x%x\r\n", reg_addr, reg_cfg.wrd);
    AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_rxbuf_stat_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint32_t       buf_idx,   /* 0..1 */
    CA_OUT aal_xgpn_plmp_rxbuf_stat_t       *ctrl
)
{
    XGPN_PLMP_RXBUF0_STAT_t  reg_cfg;
    ca_uint32_t reg_addr = XGPN_PLMP_RXBUF0_STAT + buf_idx * 4;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);

    PLOAM_DEBUG("read [0x%08x]:0x%x\r\n", reg_addr, reg_cfg.wrd);

    ctrl->mic_err = reg_cfg.bf.mic_err;
    ctrl->rx_sfc = reg_cfg.bf.rx_sfc;
    ctrl->turn = reg_cfg.bf.turn;
    ctrl->pkt_rdy = reg_cfg.bf.pkt_rdy;

    return CA_E_OK;
}


ca_status_t aal_ploam_rxbuf_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint32_t          buf_idx,   /* 0..1 */
    CA_IN ca_boolean_t         has_mic,
    CA_IN aal_xgpn_plmp_rxbuf_t       *rxbuf
)
{
    aal_reg_xgpn_plmp_rxbuf_t reg_cfg;
    ca_uint32_t reg_addr, i;
    //ca_uint32_t reg_offset = buf_idx * XGPN_PLMP_RXBUF_COUNT /2;
    ca_uint32_t count = XGPN_PLMP_RXBUF_COUNT >> 1;

    AAL_ASSERT_RET(rxbuf != NULL, CA_E_PARAM);

    if (aal_ploam_debug)
        ploam_octet_dump("data", rxbuf->data, XGPN_PLMP_RXBUF_SIZE);

    reg_addr = XGPN_PLMP_RXBUF + (count << 2) * buf_idx;

    if (!has_mic)
        count -= 2;

    //__PLOAM_LOCK();

    for (i = 0; i < count; i++){
        #if 0 // fix byte order bug
        reg_addr = XGPN_PLMP_RXBUF + reg_offset + i * 4;
        reg_cfg.bf.byte0 = rxbuf->data[i * 4];
        reg_cfg.bf.byte1 = rxbuf->data[i * 4 + 1];
        reg_cfg.bf.byte2 = rxbuf->data[i * 4 + 2];
        reg_cfg.bf.byte3 = rxbuf->data[i * 4 + 3];
        #endif
        reg_cfg.wrd = (rxbuf->data[i * 4] << 24) |
            (rxbuf->data[i * 4 + 1] << 16) |
            (rxbuf->data[i * 4 + 2] << 8) |
            rxbuf->data[i * 4 + 3];
        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        PLOAM_DEBUG("[0x%08x]:0x%x\r\n", reg_addr, reg_cfg.wrd);
        reg_addr += 4;
    }

    //__PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_rxbuf_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint32_t       buf_idx,   /* 0..1 */
    CA_IN ca_boolean_t      has_mic,
    CA_OUT aal_xgpn_plmp_rxbuf_t       *rxbuf
)
{
    aal_reg_xgpn_plmp_rxbuf_t  reg_cfg;
    ca_uint32_t i, reg_addr;
    ca_uint32_t count = XGPN_PLMP_RXBUF_COUNT >> 1;

    AAL_ASSERT_RET(rxbuf != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(buf_idx < XGPN_PLMP_RXBUF_BLOCK, CA_E_PARAM);

    /* Get the start read register address */
    reg_addr = XGPN_PLMP_RXBUF + (count << 2) * buf_idx;

    if (!has_mic) {/* Not read MIC to save read time */
        count -= 2;
    }

    for (i = 0; i < count; i++){ /* Not read the PLOAM MIC */
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr); /* read buffer */
        rxbuf->data[i * 4]     = (reg_cfg.wrd >> 24) & 0xff;//reg_cfg.bf.byte0;
        rxbuf->data[i * 4 + 1] = (reg_cfg.wrd >> 16) & 0xff;//reg_cfg.bf.byte1;
        rxbuf->data[i * 4 + 2] = (reg_cfg.wrd >> 8) & 0xff; //reg_cfg.bf.byte2;
        rxbuf->data[i * 4 + 3] = reg_cfg.wrd & 0xff; //reg_cfg.bf.byte3;
        //PLOAM_DEBUG("[0x%08x]:0x%x\r\n", reg_addr, reg_cfg.wrd);
        reg_addr += 4;
    }

    return CA_E_OK;
}

ca_status_t aal_ploam_rx_mib_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT aal_ploam_rx_mib_t *rx_mib)
{
    AAL_ASSERT_RET(rx_mib != NULL, CA_E_PARAM);

    rx_mib->ds_rx_cnt   = AAL_PON_REG_READ(XGPN_PLMP_DS_RX_MIB);
    rx_mib->mir_err_cnt = AAL_PON_REG_READ(XGPN_PLMP_MIC_ERR_MIB);
    rx_mib->range_time_cnt = AAL_PON_REG_READ(XGPN_PLMP_RANGING_TIME_MIB);
    rx_mib->protection_ctrl_cnt = AAL_PON_REG_READ(XGPN_PLMP_PROTECT_CTRL_MIB);
    rx_mib->adjust_tx_wavelength_cnt = AAL_PON_REG_READ(XGPN_PLMP_ADJ_TX_WVLN_MIB);
    rx_mib->calibrt_request_cnt = AAL_PON_REG_READ(XGPN_PLMP_CALIBRT_REQ_MIB);
    rx_mib->sys_profile_cnt = AAL_PON_REG_READ(XGPN_PLMP_SYSTEM_PRFL_MIB);
    rx_mib->channel_profile_cnt = AAL_PON_REG_READ(XGPN_PLMP_CHANNEL_PRFL_MIB);
    rx_mib->burst_profile_cnt = AAL_PON_REG_READ(XGPN_PLMP_PRFL_MIB);
    rx_mib->assign_onu_id_cnt = AAL_PON_REG_READ(XGPN_PLMP_ASSGN_ONUID_MIB);
    //rx_mib->power_consump_inquire_cnt = AAL_PON_REG_READ(XGPN_PLMP_PWR_CNSMPT_RPT_MIB);
    rx_mib->deactive_onu_id_cnt = AAL_PON_REG_READ(XGPN_PLMP_DEACTV_ONU_MIB);
    rx_mib->disable_sn_cnt = AAL_PON_REG_READ(XGPN_PLMP_DISABLE_SN_MIB);
    rx_mib->request_regsitration_cnt = AAL_PON_REG_READ(XGPN_PLMP_REQ_REGST_MIB);
    rx_mib->assign_alloc_id_cnt = AAL_PON_REG_READ(XGPN_PLMP_ASSGN_ALLCTID_MIB);
    rx_mib->key_ctrl_cnt = AAL_PON_REG_READ(XGPN_PLMP_KEY_CTRL_MIB);
    rx_mib->sleep_allow_cnt = AAL_PON_REG_READ(XGPN_PLMP_SLEEP_ALLOW_STAT);

    return CA_E_OK;
}

ca_status_t aal_ploam_tx_mib_get(
    CA_IN  ca_device_id_t device_id,
    CA_OUT aal_ploam_tx_mib_t *tx_mib)
{
    AAL_ASSERT_RET(tx_mib != NULL, CA_E_PARAM);

    tx_mib->us_tx_cnt   = AAL_PON_REG_READ(XGPN_PLMP_US_TX_MIB);
    tx_mib->sn_onu_cnt   = AAL_PON_REG_READ(XGPN_PLMP_SN_ONU_MIB);
    tx_mib->registration_cnt = AAL_PON_REG_READ(XGPN_PLMP_REGST_MIB);
    tx_mib->key_report_cnt   = AAL_PON_REG_READ(XGPN_PLMP_KEY_REPORT_MIB);
    tx_mib->ack_cnt   = AAL_PON_REG_READ(XGPN_PLMP_ACK_MIB);
    tx_mib->sleep_request_cnt   = AAL_PON_REG_READ(XGPN_PLMP_SLEEP_REQ_MIB);
    tx_mib->tuning_resp_ack_nack_cnt   = AAL_PON_REG_READ(XGPN_PLMP_TUN_REQ_RSP_MIB);
    tx_mib->tuning_resp_complete_u_rollback_cnt = AAL_PON_REG_READ(XGPN_PLMP_TUN_CMPLTU_MIB);
    tx_mib->power_consumption_report_cnt = AAL_PON_REG_READ(XGPN_PLMP_PWR_CNSMPT_RPT_MIB);

    return CA_E_OK;

}

ca_status_t aal_ploam_debug_sts0_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint32_t *value
)
{
    aal_reg_xgpn_debug_sts_t debug_sts;

    AAL_ASSERT_RET(value != NULL, CA_E_PARAM);

    debug_sts.wrd  = AAL_PON_REG_READ(XGPN_PLMP_DEBUG_STS0);
    *value = debug_sts.bf.value;

    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", XGPN_PLMP_DEBUG_STS0, *value);

    return CA_E_OK;

}


ca_status_t aal_ploam_debug_sts1_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint32_t *value
)
{
    aal_reg_xgpn_debug_sts_t debug_sts;

    AAL_ASSERT_RET(value != NULL, CA_E_PARAM);

    debug_sts.wrd  = AAL_PON_REG_READ(XGPN_PLMP_DEBUG_STS1);
    *value = debug_sts.bf.value;

    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", XGPN_PLMP_DEBUG_STS1, *value);

    return CA_E_OK;

}


ca_status_t aal_ploam_debug_sts2_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint32_t *value
)
{
    aal_reg_xgpn_debug_sts_t debug_sts;

    AAL_ASSERT_RET(value != NULL, CA_E_PARAM);

    debug_sts.wrd  = AAL_PON_REG_READ(XGPN_PLMP_DEBUG_STS2);
    *value = debug_sts.bf.value;

    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", XGPN_PLMP_DEBUG_STS2, *value);

    return CA_E_OK;

}

ca_status_t aal_ploam_bc_ploam_seq_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint8_t *seq
)
{
    XGPN_PLMP_BC_PLOAM_SEQ_t reg_seq;

    AAL_ASSERT_RET(seq != NULL, CA_E_PARAM);
    __PLOAM_LOCK();

    reg_seq.wrd  = AAL_PON_REG_READ(XGPN_PLMP_BC_PLOAM_SEQ);
    *seq = reg_seq.bf.number;

    __PLOAM_UNLOCK();

    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", XGPN_PLMP_BC_PLOAM_SEQ, *seq);

    return CA_E_OK;

}

ca_status_t aal_ploam_uc_ploam_seq_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ca_uint8_t *seq
)
{
    XGPN_PLMP_UC_PLOAM_SEQ_t reg_seq;

    AAL_ASSERT_RET(seq != NULL, CA_E_PARAM);

    reg_seq.wrd  = AAL_PON_REG_READ(XGPN_PLMP_UC_PLOAM_SEQ);
    *seq = reg_seq.bf.number;

    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", XGPN_PLMP_UC_PLOAM_SEQ, *seq);

    return CA_E_OK;

}

ca_status_t aal_ploam_msg_cnt_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_ploam_msg_cnt_t *cnt)
{
    XGPN_PLMP_BC_MSG_CNT_t reg_cnt;
    XGPN_PLMP_DS_DROP_CNT_t ds_drop_cnt;

    AAL_ASSERT_RET(cnt != NULL, CA_E_PARAM);
    reg_cnt.wrd  = AAL_PON_REG_READ(XGPN_PLMP_BC_MSG_CNT);
    cnt->deact_onu_pkt = reg_cnt.bf.deact_onu_pkt;
    cnt->assgn_eqd_pkt = reg_cnt.bf.assgn_eqd_pkt;
    cnt->enable_onu_cnt= AAL_PON_REG_READ(XGPN_PLMP_ENABLE_ONU_CNT);
    cnt->uc_updt_prfl_cnt= AAL_PON_REG_READ(XGPN_PLMP_UC_UPDT_PRFL_CNT);
    cnt->extension_cnt= AAL_PON_REG_READ(XGPN_PLMP_EXTENSION_CNT);
    cnt->regst_cnt= AAL_PON_REG_READ(XGPN_PLMP_REGST_CNT);
    cnt->ack_ok_cnt = AAL_PON_REG_READ(XGPN_PLMP_ACK_OK_CNT);
    cnt->ploamd_err_cnt = AAL_PON_REG_READ(XGPN_PLMP_PLOAMD_ERR_CNT);
    cnt->ds_tx_cnt  = AAL_PON_REG_READ(XGPN_PLMP_DS_TX_CNT);
    ds_drop_cnt.wrd  = AAL_PON_REG_READ(XGPN_PLMP_DS_DROP_CNT);
    cnt->ds_drop_bc_cnt  = ds_drop_cnt.bf.bc_pkt;
    cnt->ds_drop_uc_cnt  = ds_drop_cnt.bf.uc_pkt;
    cnt->rx_ff_drop_cnt  = AAL_PON_REG_READ(XGPN_PLMP_RX_FF_DROP_CNT);
    cnt->exten_drop_cnt  = AAL_PON_REG_READ(XGPN_PLMP_EXTEN_DROP_CNT);
    cnt->us_rx_cnt  = AAL_PON_REG_READ(XGPN_PLMP_US_RX_CNT);
    cnt->us_sw_tx_cnt  = AAL_PON_REG_READ(XGPN_PLMP_US_SW_TX_CNT);

    return CA_E_OK;
}


/*--------------------------------------------------------------------------------------*/
ca_status_t aal_ploam_ds_burst_profile_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_twdm_burst_profile_mask_t  mask,
    CA_IN ploam_ngpon2_twdm_burst_profile_t       *data
)
{
    ploam_ngpon2_twdm_burst_profile_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_twdm_burst_profile_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_BURST_PROFILE, (void*)&cfg_data);

    if (mask.bf.burst_profile_ctrl)
        cfg_data.burst_profile_ctrl = data->burst_profile_ctrl;
    if (mask.bf.us_fec_ind)
        cfg_data.us_fec_ind = data->us_fec_ind;
    if (mask.bf.delimiter_len)
        cfg_data.delimiter_len = data->delimiter_len;
    if (mask.bf.delimiter)
        memcpy(cfg_data.delimiter, data->delimiter, sizeof(cfg_data.delimiter));
    if (mask.bf.preamble_len)
        cfg_data.preamble_len = data->preamble_len;
    if (mask.bf.preamble_repeat_cnt)
        cfg_data.preamble_repeat_cnt = data->preamble_repeat_cnt;
    if (mask.bf.preamble_pattern)
        memcpy(cfg_data.preamble_pattern, data->preamble_pattern, sizeof(cfg_data.preamble_pattern));
    if (mask.bf.pon_tag)
        memcpy(cfg_data.pon_tag, data->pon_tag, sizeof(cfg_data.pon_tag));

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_BURST_PROFILE, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_burst_profile_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_twdm_burst_profile_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_BURST_PROFILE, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_assign_onuid_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_assign_onuid_mask_t  mask,
    CA_IN ploam_ngpon2_assign_onuid_t       *data
)
{
    ploam_ngpon2_assign_onuid_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_assign_onuid_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_ASSIGN_ONUID, (void*)&cfg_data);

    if (mask.bf.assign_onu_id)
        cfg_data.assign_onu_id = data->assign_onu_id;
    if (mask.bf.vendor_id)
        cfg_data.vendor_id = data->vendor_id;
    if (mask.bf.vssn)
        cfg_data.vssn = data->vssn;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_ASSIGN_ONUID, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_assign_onuid_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_assign_onuid_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_ASSIGN_ONUID, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_ranging_time_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_ranging_time_mask_t  mask,
    CA_IN ploam_ngpon2_ranging_time_t       *data
)
{
    ploam_ngpon2_ranging_time_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_ranging_time_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_RANGING_TIME, (void*)&cfg_data);

    if (mask.bf.control)
        cfg_data.control = data->control;
    if (mask.bf.equ_delay)
        cfg_data.equ_delay = data->equ_delay;
    if (mask.bf.ds_pon_id)
        cfg_data.ds_pon_id = data->ds_pon_id;
    if (mask.bf.us_pon_id)
        cfg_data.us_pon_id = data->us_pon_id;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_RANGING_TIME, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_ranging_time_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_ranging_time_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_RANGING_TIME, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_deact_onuid_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_deact_onuid_mask_t  mask,
    CA_IN ploam_ngpon2_deact_onuid_t       *data
)
{
    /*ploam_ngpon2_deact_onuid_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_deact_onuid_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_DEACT_ONUID, (void*)&cfg_data);


    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_DEACT_ONUID, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();
    */
    return CA_E_OK;

}

ca_status_t aal_ploam_ds_deact_onuid_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_deact_onuid_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_DEACT_ONUID, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_disable_sn_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_disable_sn_mask_t  mask,
    CA_IN ploam_ngpon2_disable_sn_t       *data
)
{
    ploam_ngpon2_disable_sn_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_disable_sn_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_DISABLE_SN, (void*)&cfg_data);

    if (mask.bf.enable)
        cfg_data.enable = data->enable;
    if (mask.bf.vendor_id)
        cfg_data.vendor_id = data->vendor_id;
    if (mask.bf.vssn)
        cfg_data.vssn = data->vssn;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_DISABLE_SN, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_disable_sn_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_disable_sn_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_DISABLE_SN, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_request_reg_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_request_reg_mask_t  mask,
    CA_IN ploam_ngpon2_request_reg_t       *data
)
{
/*    ploam_ngpon2_request_reg_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_request_reg_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_REQUEST_REGISTRATION, (void*)&cfg_data);

    if (mask.bf.vendor_xxx_id)
        cfg_data.vendor_xxx_id = data->vendor_xxx_id;


    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_REQUEST_REGISTRATION, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();
*/
    return CA_E_OK;

}

ca_status_t aal_ploam_ds_request_reg_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_request_reg_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_REQUEST_REGISTRATION, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_assign_allocid_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_assign_allocid_mask_t  mask,
    CA_IN ploam_ngpon2_assign_allocid_t       *data
)
{
    ploam_ngpon2_assign_allocid_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_assign_allocid_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_ASSIGN_ALLOCID, (void*)&cfg_data);

    if (mask.bf.alloc_id_value)
        cfg_data.alloc_id_value = data->alloc_id_value;
    if (mask.bf.alloc_id_type)
        cfg_data.alloc_id_type = data->alloc_id_type;
    if (mask.bf.alloc_id_scope)
        cfg_data.alloc_id_scope = data->alloc_id_scope;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_ASSIGN_ALLOCID, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_assign_allocid_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_assign_allocid_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_ASSIGN_ALLOCID, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_key_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_key_ctrl_mask_t  mask,
    CA_IN ploam_ngpon2_key_ctrl_t       *data
)
{
    ploam_ngpon2_key_ctrl_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_key_ctrl_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_KEY_CTRL, (void*)&cfg_data);

    if (mask.bf.ctrl_flag)
        cfg_data.ctrl_flag = data->ctrl_flag;
    if (mask.bf.key_idx)
        cfg_data.key_idx = data->key_idx;
    if (mask.bf.key_len)
        cfg_data.key_len = data->key_len;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_KEY_CTRL, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_key_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_key_ctrl_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_KEY_CTRL, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_sleep_allow_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_sleep_allow_mask_t  mask,
    CA_IN ploam_ngpon2_sleep_allow_t       *data
)
{
    ploam_ngpon2_sleep_allow_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_sleep_allow_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_SLEEP_ALLOW, (void*)&cfg_data);

    if (mask.bf.ctrl_flag)
        cfg_data.ctrl_flag = data->ctrl_flag;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_SLEEP_ALLOW, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_sleep_allow_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_sleep_allow_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_SLEEP_ALLOW, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_calibration_req_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_calibration_req_mask_t  mask,
    CA_IN ploam_ngpon2_calibration_req_t       *data
)
{
    ploam_ngpon2_calibration_req_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_calibration_req_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_CALIBRATION_REQ, (void*)&cfg_data);

    if (mask.bf.vendor_id)
        cfg_data.vendor_id = data->vendor_id;
    if (mask.bf.vssn)
        cfg_data.vssn = data->vssn;
    if (mask.bf.current_pon_id)
        cfg_data.current_pon_id = data->current_pon_id;
    if (mask.bf.correlation_tag)
        cfg_data.correlation_tag = data->correlation_tag;
    if (mask.bf.target_ds_pon_id)
        cfg_data.target_ds_pon_id = data->target_ds_pon_id;
    if (mask.bf.target_us_pon_id)
        cfg_data.target_us_pon_id = data->target_us_pon_id;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_CALIBRATION_REQ, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_calibration_req_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_calibration_req_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_CALIBRATION_REQ, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_adjust_tx_wave_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_adjust_tx_wave_mask_t  mask,
    CA_IN ploam_ngpon2_adjust_tx_wave_t       *data
)
{
    ploam_ngpon2_adjust_tx_wave_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_adjust_tx_wave_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_ADJUST_TX_WAVELEN, (void*)&cfg_data);

    if (mask.bf.vendor_id)
        cfg_data.vendor_id = data->vendor_id;
    if (mask.bf.vssn)
        cfg_data.vssn = data->vssn;
    if (mask.bf.current_pon_id)
        cfg_data.current_pon_id = data->current_pon_id;
    if (mask.bf.correlation_tag)
        cfg_data.correlation_tag = data->correlation_tag;
    if (mask.bf.freq_adj_direct)
        cfg_data.freq_adj_direct = data->freq_adj_direct;
    if (mask.bf.freq_adj_size)
        cfg_data.freq_adj_size = data->freq_adj_size;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_ADJUST_TX_WAVELEN, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_adjust_tx_wave_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_adjust_tx_wave_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_ADJUST_TX_WAVELEN, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_tuning_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_tuning_ctrl_mask_t  mask,
    CA_IN ploam_ngpon2_tuning_ctrl_t       *data
)
{
    ploam_ngpon2_tuning_ctrl_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_tuning_ctrl_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_TUNING_CTRL, (void*)&cfg_data);

    if (mask.bf.op_code)
        cfg_data.op_code = data->op_code;
    if (mask.bf.scheduled_sfc)
        cfg_data.scheduled_sfc = data->scheduled_sfc;
    if (mask.bf.rollback_flag)
        cfg_data.rollback_flag = data->rollback_flag;
    if (mask.bf.target_ds_pon_id)
        cfg_data.target_ds_pon_id = data->target_ds_pon_id;
    if (mask.bf.target_us_pon_id)
        cfg_data.target_us_pon_id = data->target_us_pon_id;
    if (mask.bf.calibration_flag)
        cfg_data.calibration_flag = data->calibration_flag;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_TUNING_CTRL, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_tuning_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_tuning_ctrl_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_TUNING_CTRL, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_sys_profile_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_sys_profile_mask_t  mask,
    CA_IN ploam_ngpon2_sys_profile_t       *data
)
{
    ploam_ngpon2_sys_profile_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_sys_profile_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_SYS_PROFILE, (void*)&cfg_data);

    if (mask.bf.ng2sys_id)
    {
        cfg_data.ng2sys_id[0] = data->ng2sys_id[0];
        cfg_data.ng2sys_id[1] = data->ng2sys_id[1];
        cfg_data.ng2sys_id[2] = data->ng2sys_id[2];
    }
    if (mask.bf.sys_profile_ver)
        cfg_data.sys_profile_ver = data->sys_profile_ver;
    if (mask.bf.us_op_wave_len_bands)
        cfg_data.us_op_wave_len_bands = data->us_op_wave_len_bands;
    if (mask.bf.twdm_channel_cnt)
        cfg_data.twdm_channel_cnt = data->twdm_channel_cnt;
    if (mask.bf.channel_spacing)
        cfg_data.channel_spacing = data->channel_spacing;
    if (mask.bf.us_mse)
        cfg_data.us_mse = data->us_mse;
    if (mask.bf.fsr)
        cfg_data.fsr = data->fsr;
    if (mask.bf.amcc_ctrl)
        cfg_data.amcc_ctrl = data->amcc_ctrl;
    if (mask.bf.loose_calibration)
        cfg_data.loose_calibration = data->loose_calibration;
    if (mask.bf.channel_cnt)
        cfg_data.channel_cnt = data->channel_cnt;
    if (mask.bf.channel_spacing_ptp)
        cfg_data.channel_spacing_ptp = data->channel_spacing_ptp;
    if (mask.bf.us_mse_ptp)
        cfg_data.us_mse_ptp = data->us_mse_ptp;
    if (mask.bf.fsr_ptp)
        cfg_data.fsr_ptp = data->fsr_ptp;
    if (mask.bf.calibration_accuracy)
        cfg_data.calibration_accuracy = data->calibration_accuracy;
    if (mask.bf.loose_calibration_bound)
        cfg_data.loose_calibration_bound = data->loose_calibration_bound;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_SYS_PROFILE, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_sys_profile_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_sys_profile_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_SYS_PROFILE, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_channel_profile_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_channel_profile_mask_t  mask,
    CA_IN ploam_ngpon2_channel_profile_t       *data
)
{
    ploam_ngpon2_channel_profile_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_channel_profile_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_CHANNEL_PROFILE, (void*)&cfg_data);

    if (mask.bf.control)
        cfg_data.control = data->control;
    if (mask.bf.version)
        cfg_data.version = data->version;
    if (mask.bf.pon_id)
        cfg_data.pon_id = data->pon_id;
    if (mask.bf.ds_freq_offset)
        cfg_data.ds_freq_offset = data->ds_freq_offset;
    if (mask.bf.ds_rate)
        cfg_data.ds_rate = data->ds_rate;
    if (mask.bf.channel_partition)
        cfg_data.channel_partition = data->channel_partition;
    if (mask.bf.def_resp_channel)
        cfg_data.def_resp_channel = data->def_resp_channel;
    if (mask.bf.sn_grant_type_ind)
        cfg_data.sn_grant_type_ind = data->sn_grant_type_ind;
    if (mask.bf.amcc_window)
        cfg_data.amcc_window = data->amcc_window;
    if (mask.bf.uwlch_id)
        cfg_data.uwlch_id = data->uwlch_id;
    if (mask.bf.us_freq)
        cfg_data.us_freq = data->us_freq;
    if (mask.bf.opt_link_type)
        cfg_data.opt_link_type = data->opt_link_type;
    if (mask.bf.us_rate)
        cfg_data.us_rate = data->us_rate;
    if (mask.bf.def_onu_attenuation)
        cfg_data.def_onu_attenuation = data->def_onu_attenuation;
    if (mask.bf.resp_threshold)
        cfg_data.resp_threshold = data->resp_threshold;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_CHANNEL_PROFILE, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_channel_profile_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_channel_profile_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_CHANNEL_PROFILE, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_protection_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_protection_ctrl_mask_t  mask,
    CA_IN ploam_ngpon2_protection_ctrl_t       *data
)
{
    ploam_ngpon2_protection_ctrl_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_protection_ctrl_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_PROTECTION_CTRL, (void*)&cfg_data);

    if (mask.bf.protect_ds_pon_id)
        cfg_data.protect_ds_pon_id = data->protect_ds_pon_id;
    if (mask.bf.protect_us_pon_id)
        cfg_data.protect_us_pon_id = data->protect_us_pon_id;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_PROTECTION_CTRL, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_protection_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_protection_ctrl_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_PROTECTION_CTRL, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_change_power_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_change_power_mask_t  mask,
    CA_IN ploam_ngpon2_change_power_t       *data
)
{
    ploam_ngpon2_change_power_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_change_power_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_CHANGE_POWER_LEVEL, (void*)&cfg_data);

    if (mask.bf.op_type)
        cfg_data.op_type = data->op_type;
    if (mask.bf.attenuation)
        cfg_data.attenuation = data->attenuation;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_CHANGE_POWER_LEVEL, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_change_power_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_change_power_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_CHANGE_POWER_LEVEL, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_power_consume_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_power_consume_mask_t  mask,
    CA_IN ploam_ngpon2_power_consume_t       *data
)
{
/*    ploam_ngpon2_power_consume_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_power_consume_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_POWER_CONSUMPTION_INQUIRE, (void*)&cfg_data);

    if (mask.bf.vendor_xxx_id)
        cfg_data.vendor_xxx_id = data->vendor_xxx_id;


    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_POWER_CONSUMPTION_INQUIRE, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();
*/
    return CA_E_OK;

}

ca_status_t aal_ploam_ds_power_consume_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_power_consume_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_POWER_CONSUMPTION_INQUIRE, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_ds_rate_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_rate_ctrl_mask_t  mask,
    CA_IN ploam_ngpon2_rate_ctrl_t       *data
)
{
    ploam_ngpon2_rate_ctrl_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_rate_ctrl_t));
    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_RATE_CTRL, (void*)&cfg_data);

    if (mask.bf.op_code)
        cfg_data.op_code = data->op_code;
    if (mask.bf.scheduled_sfc)
        cfg_data.scheduled_sfc = data->scheduled_sfc;
    if (mask.bf.rollback_flag)
        cfg_data.rollback_flag = data->rollback_flag;
    if (mask.bf.norinal_line_rate)
        cfg_data.norinal_line_rate = data->norinal_line_rate;

    ret = ploam_ngpon2_ds_ploam_def_val_set(PLOAM_NGPON2_DS_RATE_CTRL, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_ds_rate_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_rate_ctrl_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_ds_ploam_def_val_get(PLOAM_NGPON2_DS_RATE_CTRL, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}


/*--------------------------------------------------------------------------------------*/
ca_status_t aal_ploam_us_serial_number_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_serial_number_mask_t  mask,
    CA_IN ploam_ngpon2_us_serial_number_t       *data
)
{
    ploam_ngpon2_us_serial_number_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_us_serial_number_t));
    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_SERIAL_NUMBER, (void*)&cfg_data);

    if (mask.bf.vendor_id)
        cfg_data.vendor_id = data->vendor_id;
    if (mask.bf.vssn)
        cfg_data.vssn = data->vssn;
    if (mask.bf.random_delay)
        cfg_data.random_delay = data->random_delay;
    if (mask.bf.correlation_tag)
        cfg_data.correlation_tag = data->correlation_tag;
    if (mask.bf.current_ds_pon_id)
        cfg_data.current_ds_pon_id = data->current_ds_pon_id;
    if (mask.bf.current_us_pon_id)
        cfg_data.current_us_pon_id = data->current_us_pon_id;
    if (mask.bf.calibration_rec_status)
        memcpy(cfg_data.calibration_rec_status, data->calibration_rec_status, sizeof(data->calibration_rec_status));
    if (mask.bf.tuning_granularity)
        cfg_data.tuning_granularity = data->tuning_granularity;
    if (mask.bf.step_tuning_time)
        cfg_data.step_tuning_time = data->step_tuning_time;
    if (mask.bf.us_line_rate_cap)
        cfg_data.us_line_rate_cap = data->us_line_rate_cap;
    if (mask.bf.attenuation)
        cfg_data.attenuation = data->attenuation;
    if (mask.bf.power_level_cap)
        cfg_data.power_level_cap = data->power_level_cap;
    if (mask.bf.padding)
        cfg_data.padding = data->padding;

    ret = ploam_ngpon2_us_ploam_def_val_set(PLOAM_NGPON2_US_SERIAL_NUMBER, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_serial_number_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_serial_number_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_REGISTRATION, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_us_registration_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_registration_mask_t  mask,
    CA_IN ploam_ngpon2_us_registration_t       *data
)
{
    ploam_ngpon2_us_registration_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_us_registration_t));
    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_REGISTRATION, (void*)&cfg_data);

    if (mask.bf.registration_id)
        memcpy(cfg_data.registration_id, data->registration_id, sizeof(cfg_data.registration_id));

    ret = ploam_ngpon2_us_ploam_def_val_set(PLOAM_NGPON2_US_REGISTRATION, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_registration_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_registration_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_REGISTRATION, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_us_key_report_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_key_report_mask_t  mask,
    CA_IN ploam_ngpon2_us_key_report_t       *data
)
{
    ploam_ngpon2_us_key_report_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_us_key_report_t));
    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_KEY_REPORT, (void*)&cfg_data);

    if (mask.bf.report_type)
        cfg_data.report_type = data->report_type;
    if (mask.bf.key_idx)
        cfg_data.key_idx = data->key_idx;
    if (mask.bf.fragment_no)
        cfg_data.fragment_no = data->fragment_no;
    if (mask.bf.resv)
        cfg_data.resv = data->resv;
    if (mask.bf.key_fragment)
        memcpy(cfg_data.key_fragment, data->key_fragment, sizeof(cfg_data.key_fragment));

    ret = ploam_ngpon2_us_ploam_def_val_set(PLOAM_NGPON2_US_KEY_REPORT, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_key_report_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_key_report_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_KEY_REPORT, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_us_ack_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_ack_mask_t  mask,
    CA_IN ploam_ngpon2_us_ack_t       *data
)
{
    ploam_ngpon2_us_ack_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_us_ack_t));
    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_ACK, (void*)&cfg_data);

    if (mask.bf.complete_code)
        cfg_data.complete_code = data->complete_code;
    if (mask.bf.attenuation)
        cfg_data.attenuation = data->attenuation;
    if (mask.bf.power_level_cap)
        cfg_data.power_level_cap = data->power_level_cap;
    if (mask.bf.padding)
        memcpy(cfg_data.padding, data->padding, sizeof(cfg_data.padding));

    ret = ploam_ngpon2_us_ploam_def_val_set(PLOAM_NGPON2_US_ACK, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_ack_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_ack_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_ACK, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_us_sleep_req_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_sleep_req_mask_t  mask,
    CA_IN ploam_ngpon2_us_sleep_req_t       *data
)
{
    ploam_ngpon2_us_sleep_req_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_us_sleep_req_t));
    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_SLEEP_REQ, (void*)&cfg_data);

    if (mask.bf.activity_level)
        cfg_data.activity_level = data->activity_level;
    if (mask.bf.padding)
        memcpy(cfg_data.padding, data->padding, sizeof(cfg_data.padding));

    ret = ploam_ngpon2_us_ploam_def_val_set(PLOAM_NGPON2_US_SLEEP_REQ, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_sleep_req_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_sleep_req_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_SLEEP_REQ, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_us_tuning_resp_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_tuning_resp_mask_t  mask,
    CA_IN ploam_ngpon2_us_tuning_resp_t       *data
)
{
    ploam_ngpon2_us_tuning_resp_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_us_tuning_resp_t));
    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_TUNING_RESP, (void*)&cfg_data);

    if (mask.bf.op_code)
        cfg_data.op_code = data->op_code;
    if (mask.bf.resp_code)
        cfg_data.resp_code = data->resp_code;
    if (mask.bf.vendor_id)
        cfg_data.vendor_id = data->vendor_id;
    if (mask.bf.vssn)
        cfg_data.vssn = data->vssn;
    if (mask.bf.correlation_tag)
        cfg_data.correlation_tag = data->correlation_tag;
    if (mask.bf.pon_id)
        cfg_data.pon_id = data->pon_id;
    if (mask.bf.uwlch_id)
        cfg_data.uwlch_id = data->uwlch_id;
    if (mask.bf.calibration_rec_status)
        cfg_data.calibration_rec_status = data->calibration_rec_status;
    if (mask.bf.tuning_granularity)
        cfg_data.tuning_granularity = data->tuning_granularity;
    if (mask.bf.one_step_tuning_time)
        cfg_data.one_step_tuning_time = data->one_step_tuning_time;
    if (mask.bf.us_line_rate_cap)
        cfg_data.us_line_rate_cap = data->us_line_rate_cap;
    if (mask.bf.attenuation)
        cfg_data.attenuation = data->attenuation;
    if (mask.bf.power_level_cap)
        cfg_data.power_level_cap = data->power_level_cap;
    if (mask.bf.padding)
        memcpy(cfg_data.padding, data->padding, sizeof(cfg_data.padding));

    ret = ploam_ngpon2_us_ploam_def_val_set(PLOAM_NGPON2_US_TUNING_RESP, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_tuning_resp_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_tuning_resp_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_TUNING_RESP, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_ploam_us_power_consumption_report_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_power_consumption_report_mask_t  mask,
    CA_IN ploam_ngpon2_us_power_consumption_report_t       *data
)
{
    ploam_ngpon2_us_power_consumption_report_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_us_power_consumption_report_t));
    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_POWER_CONSUMPTION_REPORT, (void*)&cfg_data);

    if (mask.bf.power_consumption)
        memcpy(cfg_data.power_consumption, data->power_consumption, sizeof(cfg_data.power_consumption));
    if (mask.bf.padding)
        memcpy(cfg_data.padding, data->padding, sizeof(cfg_data.padding));

    ret = ploam_ngpon2_us_ploam_def_val_set(PLOAM_NGPON2_US_POWER_CONSUMPTION_REPORT, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_power_consumption_report_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_power_consumption_report_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_POWER_CONSUMPTION_REPORT, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_us_rate_resp_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ploam_ngpon2_us_rate_resp_mask_t  mask,
    CA_IN ploam_ngpon2_us_rate_resp_t       *data
)
{
    ploam_ngpon2_us_rate_resp_t cfg_data;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    memset(&cfg_data, 0, sizeof(ploam_ngpon2_us_rate_resp_t));
    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_RATE_RESP, (void*)&cfg_data);

    if (mask.bf.op_code)
        cfg_data.op_code = data->op_code;
    if (mask.bf.resp_code)
        cfg_data.resp_code = data->resp_code;
    if (mask.bf.padding)
        memcpy(cfg_data.padding, data->padding, sizeof(cfg_data.padding));

    ret = ploam_ngpon2_us_ploam_def_val_set(PLOAM_NGPON2_US_RATE_RESP, (void*)&cfg_data);

    PLOAM_DEBUG("\r\n");

    __PLOAM_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_ploam_us_rate_resp_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT ploam_ngpon2_us_rate_resp_t       *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    PLOAM_DEBUG("\r\n");

    __PLOAM_LOCK();

    ret = ploam_ngpon2_us_ploam_def_val_get(PLOAM_NGPON2_US_RATE_RESP, (void*)data);

    __PLOAM_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_ploam_us_pkt_send_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint16_t          onu_id,
    CA_IN ca_uint8_t           msg_id,
    CA_IN ca_uint8_t           seq_no,
    CA_IN ca_boolean_t         use_def_key
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t msg[PLOAM_NGPON2_MSG_SIZE] = {0};

    ca_printf("%-16s, %d/0x%x\r\n", "US_SERIAL_NUM",     PLOAM_NGPON2_US_SERIAL_NUMBER,           PLOAM_NGPON2_US_SERIAL_NUMBER);
    ca_printf("%-16s, %d/0x%x\r\n", "US_REG",            PLOAM_NGPON2_US_REGISTRATION,            PLOAM_NGPON2_US_REGISTRATION);
    ca_printf("%-16s, %d/0x%x\r\n", "US_KEY_REPORT",     PLOAM_NGPON2_US_KEY_REPORT,              PLOAM_NGPON2_US_KEY_REPORT);
    ca_printf("%-16s, %d/0x%x\r\n", "US_ACK",            PLOAM_NGPON2_US_ACK,                     PLOAM_NGPON2_US_ACK);
    ca_printf("%-16s, %d/0x%x\r\n", "US_SLEEP_REQ",      PLOAM_NGPON2_US_SLEEP_REQ,               PLOAM_NGPON2_US_SLEEP_REQ);
    ca_printf("%-16s, %d/0x%x\r\n", "US_TUNING_RESP",    PLOAM_NGPON2_US_TUNING_RESP,             PLOAM_NGPON2_US_TUNING_RESP);
    ca_printf("%-16s, %d/0x%x\r\n", "US_POWER_REPORT",   PLOAM_NGPON2_US_POWER_CONSUMPTION_REPORT,PLOAM_NGPON2_US_POWER_CONSUMPTION_REPORT);
    ca_printf("%-16s, %d/0x%x\r\n", "US_RATE_RESP",      PLOAM_NGPON2_US_RATE_RESP,               PLOAM_NGPON2_US_RATE_RESP);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            onu_id, msg_id, seq_no);

    ret = ploam_ngpon2_us_ploam_def_val_get(msg_id, msg);

    if (ret != CA_E_OK){
        ca_printf("invalid msg id\r\n");
        return ret;
    }

    ret = ploam_ngpon2_build_ploam(onu_id, msg_id, seq_no, msg);

    ret = ploam_ngpon2_send_ploam(msg, PLOAM_NGPON2_MSG_SIZE, use_def_key);

    ca_printf("ret:%d\r\n", ret);

    //ploam_dump_pkt("us ploam", msg, PLOAM_NGPON2_MSG_SIZE);

    return CA_E_OK;

}

ca_status_t aal_ploam_us_pkt_send_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint16_t       onu_id,
    CA_IN ca_uint8_t        msg_id,
    CA_IN ca_uint8_t        seq_no,
    CA_IN ca_boolean_t      use_def_key,
    CA_OUT ca_uint32_t      *value
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(value != NULL, CA_E_PARAM);

    *value = 1;
    PLOAM_DEBUG("Empty function\r\n");

    return ret;
}


ca_status_t aal_ploam_loopback_ploam_send_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_uint8_t           msg_id
)
{
    int i = 0;
    aal_xgpn_plmp_cfg_t cfg_data;
    ca_status_t ret = CA_E_OK;
//    ca_uint8_t msg[PLOAM_NGPON2_MSG_SIZE] = {0};


    //set sw_mode = 1
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 1);

    //fill burst profile header

    //fill burst profile body

    for (i = 0; i < XGPN_PLMP_MSG_BODY_SIZE; i++) {
        cfg_data.data[i] = i;
    }

    cfg_data.msg_type = msg_id;
    cfg_data.done = 1;

    //memset(&cfg_data, 0, sizeof(aal_xgpn_plmp_cfg_t));
    ret = aal_ploam_cfg_set(0, &cfg_data);

    //cfg_data.done = 0;
    //ret = aal_ploam_cfg_set(0, &cfg_data);

    //set sw_mode = 0
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 0);
    PLOAM_DEBUG("ret:%d\r\n", ret);

    return CA_E_OK;

}

ca_status_t aal_ploam_loopback_ploam_send_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint8_t        msg_id,
    CA_OUT aal_xgpn_plmp_cfg_t      *data
)
{
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(data != NULL, CA_E_PARAM);

    ret = aal_ploam_cfg_get(0, data);

    PLOAM_DEBUG("Empty function\r\n");
    return CA_E_OK;
}

ca_status_t aal_ploam_tuning_sw_ack_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_ploam_tuning_sw_ack_mask_t mask,
    CA_IN aal_ploam_tuning_sw_ack_t     *cfg
)
{
    XGPN_PLMP_SW_ACK_t reg_data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_data.wrd = AAL_PON_REG_READ(XGPN_PLMP_SW_ACK);

    if(mask.s.complt_code) reg_data.bf.complt_code    = cfg->complt_code   ;
    if(mask.s.seqno) reg_data.bf.seqno = cfg->seqno;
    if(mask.s.chng_pwr_level) reg_data.bf.chng_pwr_level = cfg->chng_pwr_level;
    if(mask.s.protect_ctrl) reg_data.bf.protect_ctrl = cfg->protect_ctrl;

    if (mask.wrd) {
        AAL_PON_REG_WRITE(reg_data.wrd, XGPN_PLMP_SW_ACK);
    }

    __PLOAM_UNLOCK();

    PLOAM_DEBUG("set value[0x%x]:0x%x\r\n", XGPN_PLMP_SW_ACK, reg_data.wrd );

    return CA_E_OK;

}

ca_status_t aal_ploam_tuning_sw_ack_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_ploam_tuning_sw_ack_t     *cfg
)
{
    XGPN_PLMP_SW_ACK_t reg_data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    reg_data.wrd = AAL_PON_REG_READ(XGPN_PLMP_SW_ACK);
    cfg->complt_code    = reg_data.bf.complt_code   ;
    cfg->seqno    = reg_data.bf.seqno   ;
    cfg->chng_pwr_level          = reg_data.bf.chng_pwr_level         ;
    cfg->protect_ctrl   = reg_data.bf.protect_ctrl  ;

    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", XGPN_PLMP_SW_ACK, reg_data.wrd);

    return CA_E_OK;
}

ca_status_t aal_ploam_tuning_sw_resp_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_ploam_tuning_sw_resp_mask_t mask,
    CA_IN aal_ploam_tuning_sw_resp_t     *cfg
)
{
    XGPN_PLMP_SW_RESPONSE_t reg_data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_data.wrd = AAL_PON_REG_READ(XGPN_PLMP_SW_RESPONSE);

    if(mask.s.tuning_request) reg_data.bf.tuning_request    = cfg->tuning_request   ;
    if(mask.s.tuning_operation_code) reg_data.bf.tuning_operation_code = cfg->tuning_operation_code;
    if(mask.s.seqno) reg_data.bf.seqno = cfg->seqno;
    if(mask.s.tuning_response_code) reg_data.bf.tuning_response_code = cfg->tuning_response_code;

    if (mask.wrd) {
        AAL_PON_REG_WRITE(reg_data.wrd, XGPN_PLMP_SW_RESPONSE);
    }

    __PLOAM_UNLOCK();

    PLOAM_DEBUG("set value[0x%x]:0x%x\r\n", XGPN_PLMP_SW_RESPONSE, reg_data.wrd );
    return CA_E_OK;

}

ca_status_t aal_ploam_tuning_sw_resp_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_ploam_tuning_sw_resp_t     *cfg
)
{
    XGPN_PLMP_SW_RESPONSE_t reg_data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_data.wrd = AAL_PON_REG_READ(XGPN_PLMP_SW_RESPONSE);
    cfg->tuning_request    = reg_data.bf.tuning_request   ;
    cfg->seqno  = reg_data.bf.seqno         ;
    cfg->tuning_operation_code   = reg_data.bf.tuning_operation_code  ;
    cfg->tuning_response_code = reg_data.bf.tuning_response_code;


    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", XGPN_PLMP_SW_RESPONSE, reg_data.wrd);

    return CA_E_OK;
}

ca_status_t aal_ploam_tuning_cfg_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_ploam_tuning_cfg_mask_t mask,
    CA_IN aal_ploam_tuning_cfg_t     *cfg
)
{
    XGPN_PLMP_TUNING_CONFIG_t reg_data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_data.wrd = AAL_PON_REG_READ(XGPN_PLMP_TUNING_CONFIG);

    if(mask.s.request_schedule_sfc   ) reg_data.bf.request_schedule_sfc    = cfg->request_schedule_sfc   ;
    if(mask.s.skip_rx         ) reg_data.bf.skip_rx          = cfg->skip_rx         ;
    if(mask.s.rollback_allow  ) reg_data.bf.rollback_allow   = cfg->rollback_allow  ;
    if(mask.s.request_lead_sfc) reg_data.bf.request_lead_sfc = cfg->request_lead_sfc;

    if (mask.wrd) {
        AAL_PON_REG_WRITE(reg_data.wrd, XGPN_PLMP_TUNING_CONFIG);
    }

    __PLOAM_UNLOCK();

    PLOAM_DEBUG("set value[0x%x]:0x%x\r\n", XGPN_PLMP_TUNING_CONFIG, reg_data.wrd );

    return CA_E_OK;

}

ca_status_t aal_ploam_tuning_cfg_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_OUT aal_ploam_tuning_cfg_t     *cfg
)
{
    XGPN_PLMP_TUNING_CONFIG_t reg_data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    reg_data.wrd = AAL_PON_REG_READ(XGPN_PLMP_TUNING_CONFIG);
    cfg->request_schedule_sfc    = reg_data.bf.request_schedule_sfc   ;
    cfg->skip_rx          = reg_data.bf.skip_rx         ;
    cfg->rollback_allow   = reg_data.bf.rollback_allow  ;
    cfg->request_lead_sfc = reg_data.bf.request_lead_sfc;

    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", XGPN_PLMP_TUNING_CONFIG, reg_data.wrd);

    return CA_E_OK;

}

ca_status_t aal_ploam_power_consumption_set(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  ca_uint32_t                 channel,
    CA_IN  ca_uint32_t                 consumption
)
{
    XGPN_PLMP_CH0_POWER_CONSUMPTION_t reg_data;
    ca_uint32_t reg_addr = XGPN_PLMP_CH0_POWER_CONSUMPTION + (channel * 4);


    AAL_ASSERT_RET(consumption <= AAL_PLAOM_POWER_CONSUMPTION_MAX, CA_E_PARAM);
    AAL_ASSERT_RET(channel < AAL_PLAOM_CHANNEL_NUM, CA_E_PARAM);

    __PLOAM_LOCK();

    reg_data.wrd = AAL_PON_REG_READ(reg_addr);
    reg_data.bf.indicate = consumption;
    AAL_PON_REG_WRITE(reg_data.wrd, reg_addr);

    __PLOAM_UNLOCK();


    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", reg_addr, reg_data.wrd);

    return CA_E_OK;

}


ca_status_t aal_ploam_power_consumption_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  ca_uint32_t                 channel,
    CA_OUT ca_uint32_t                *consumption
)
{

    XGPN_PLMP_CH0_POWER_CONSUMPTION_t reg_data;
    ca_uint32_t reg_addr = XGPN_PLMP_CH0_POWER_CONSUMPTION + (channel * 4);


    AAL_ASSERT_RET(consumption != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(channel < AAL_PLAOM_CHANNEL_NUM, CA_E_PARAM);

    reg_data.wrd = AAL_PON_REG_READ(reg_addr);

    *consumption = reg_data.bf.indicate;

    PLOAM_DEBUG("read value[0x%x]:0x%x\r\n", reg_addr, reg_data.wrd);

    return CA_E_OK;

}

ca_status_t aal_ploam_dbg_get(
    CA_IN  ca_device_id_t   device_id,
    CA_OUT aal_ploam_dbg_t *dbg
)
{
    AAL_ASSERT_RET(dbg != NULL, CA_E_PARAM);

    memcpy(dbg, &g_aal_ploam_dbg, sizeof(aal_ploam_dbg_t));
    return CA_E_OK;
}

ca_boolean_t aal_ploam_dbg_capture_get(CA_IN  ca_device_id_t   device_id)
{
    return g_aal_ploam_dbg.capture_start;
}

ca_status_t aal_ploam_dbg_capture_add(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  ca_uint8_t      *pkt,
    CA_IN  ca_uint32_t      len)
{
    if (pkt == NULL || len == 0)
        return CA_E_PARAM;
    if (ca_list_count(g_ploam_pkt_lst) >= __PLOAM_CAPTURE_BUF_NUM) {
        return CA_E_RESOURCE;
    }

    return ca_list_add(g_ploam_pkt_lst, pkt);
}


static void __dump_ploam_rx(void *buf)
{
    aal_xgpn_plmp_rxbuf_t *p = (aal_xgpn_plmp_rxbuf_t *)buf;
    ploam_ngpon2_header_t *h = (ploam_ngpon2_header_t *)buf;
    char *msg_str[] = {
        " ", // = 0
        "BURST_PROFILE", // =0x1,
        " ", // = 0x2
        "ASSIGN_ONUID", // =0x3,
        "RANGING_TIME", // =0x4,
        "DEACT_ONUID", // =0x5,
        "DISABLE_SN", // =0x6,
        " ",// =0x7,
        " ",// =0x8,
        "REQUEST_REGISTRATION", // =0x9,
        "ASSIGN_ALLOCID", // =0xA,
        " ",// =0xB,
        " ",// =0xC,
        "KEY_CTRL", // =0xD,
        " ",// =0xE,
        " ",// =0xF,
        " ",// =0x10,
        " ",// =0x11,
        "SLEEP_ALLOW", // =0x12,
        "CALIBRATION_REQ", // =0x13,
        "ADJUST_TX_WAVELEN", // =0x14,
        "TUNING_CTRL", // =0x15,
        " ",// =0x16,
        "SYS_PROFILE", // =0x17,
        "CHANNEL_PROFILE", // =0x18,
        "PROTECTION_CTRL", // =0x19,
        "CHANGE_POWER_LEVEL", // =0x1A,
        "POWER_CONSUMPTION_INQUIRE", // =0x1B,
        "RATE_CTRL" // =0x1C,
    };

    ca_printf("\r\nONU-ID:0x%x, MSG-ID:0x%x %s, SEQ-NUM:0x%x\r\n",
            ca_ntohs(h->onu_id), h->msg_id,
            (h->msg_id <= 0x1C) ? msg_str[h->msg_id] : " ", h->seq_no);

    ploam_buf_print(p->data, XGPN_PLMP_RXBUF_SIZE);
}

ca_status_t aal_ploam_dbg_set(
    CA_IN  ca_device_id_t   device_id,
    CA_IN  aal_ploam_dbg_mask_t mask,
    CA_IN  aal_ploam_dbg_t *dbg
)
{
    AAL_ASSERT_RET(dbg != NULL, CA_E_PARAM);


    if (mask.s.capture_start) {
        g_aal_ploam_dbg.capture_start = dbg->capture_start;
    }

    if (mask.s.capture_dump && g_aal_ploam_dbg.capture_dump != dbg->capture_dump) {
        g_aal_ploam_dbg.capture_dump = dbg->capture_dump;

        if (g_aal_ploam_dbg.capture_start) {
            ca_printf("Please stop capture firstly.\n");
        } else {
            ca_printf("\r\nTotal Rx %u PLOAM packets.\n", ca_list_count(g_ploam_pkt_lst));
            ca_list_traverse(g_ploam_pkt_lst, __dump_ploam_rx);
        }
    }

    if (mask.s.capture_flush && g_aal_ploam_dbg.capture_flush != dbg->capture_flush) {
        g_aal_ploam_dbg.capture_flush = dbg->capture_flush;
        ca_list_delete_all(g_ploam_pkt_lst);
    }

    if (mask.s.print_level) {
        g_aal_ploam_dbg.print_level = dbg->print_level;
        aal_ploam_debug = g_aal_ploam_dbg.print_level;
    }

    return CA_E_OK;

}

EXPORT_SYMBOL(aal_ploam_debug_set);
EXPORT_SYMBOL(aal_ploam_debug_get);
EXPORT_SYMBOL(aal_ploam_ctrl_set);
EXPORT_SYMBOL(aal_ploam_ctrl_get);
EXPORT_SYMBOL(aal_ploam_ds_ctrl_set);
EXPORT_SYMBOL(aal_ploam_ds_ctrl_get);
EXPORT_SYMBOL(aal_ploam_rx_capture_set);
EXPORT_SYMBOL(aal_ploam_rx_capture_get);
EXPORT_SYMBOL(aal_ploam_us_ctrl_set);
EXPORT_SYMBOL(aal_ploam_us_ctrl_get);
EXPORT_SYMBOL(aal_ploam_vendor_id_cfg_set);
EXPORT_SYMBOL(aal_ploam_vendor_id_cfg_get);
EXPORT_SYMBOL(aal_ploam_vssn_cfg_set);
EXPORT_SYMBOL(aal_ploam_vssn_cfg_get);
EXPORT_SYMBOL(aal_ploam_correlation_set);
EXPORT_SYMBOL(aal_ploam_correlation_get);
EXPORT_SYMBOL(aal_ploam_us_pon_id_set);
EXPORT_SYMBOL(aal_ploam_us_pon_id_get);
EXPORT_SYMBOL(aal_ploam_calibration_even_set);
EXPORT_SYMBOL(aal_ploam_calibration_even_get);
EXPORT_SYMBOL(aal_ploam_calibration_odd_set);
EXPORT_SYMBOL(aal_ploam_calibration_odd_get);
EXPORT_SYMBOL(aal_ploam_tuning_parameter_set);
EXPORT_SYMBOL(aal_ploam_tuning_parameter_get);
EXPORT_SYMBOL(aal_ploam_attenuation_set);
EXPORT_SYMBOL(aal_ploam_attenuation_get);
EXPORT_SYMBOL(aal_ploam_rgst_id_cfg_set);
EXPORT_SYMBOL(aal_ploam_rgst_id_cfg_get);
EXPORT_SYMBOL(aal_ploam_eqd_cfg_set);
EXPORT_SYMBOL(aal_ploam_eqd_cfg_get);
EXPORT_SYMBOL(aal_ploam_ack_ploam_no_msg_set);
EXPORT_SYMBOL(aal_ploam_ack_ploam_no_msg_get);
EXPORT_SYMBOL(aal_ploam_ack_ploam_busy_msg_set);
EXPORT_SYMBOL(aal_ploam_ack_ploam_busy_msg_get);
EXPORT_SYMBOL(aal_ploam_cfg_set);
EXPORT_SYMBOL(aal_ploam_cfg_get);
EXPORT_SYMBOL(aal_ploam_key_exchg_thrshd_set);
EXPORT_SYMBOL(aal_ploam_key_exchg_thrshd_get);
EXPORT_SYMBOL(aal_ploam_interrupt_set);
EXPORT_SYMBOL(aal_ploam_interrupt_get);
EXPORT_SYMBOL(aal_ploam_interrupt_enable0_set);
EXPORT_SYMBOL(aal_ploam_interrupt_enable0_get);
EXPORT_SYMBOL(aal_ploam_key_ctrl_stat_get);
EXPORT_SYMBOL(aal_ploam_key_report_set);
EXPORT_SYMBOL(aal_ploam_key_report_get);
EXPORT_SYMBOL(aal_ploam_key_report_frag_set);
EXPORT_SYMBOL(aal_ploam_key_report_frag_get);
EXPORT_SYMBOL(aal_ploam_key_sleep_allow_stat_get);
EXPORT_SYMBOL(aal_ploam_sleep_req_set);
EXPORT_SYMBOL(aal_ploam_sleep_req_get);
EXPORT_SYMBOL(aal_ploam_txbuf_ctrl_set);
EXPORT_SYMBOL(aal_ploam_txbuf_ctrl_get);
EXPORT_SYMBOL(aal_ploam_rxbuf_set);
EXPORT_SYMBOL(aal_ploam_rxbuf_get);
EXPORT_SYMBOL(aal_ploam_rx_mib_get);
EXPORT_SYMBOL(aal_ploam_tx_mib_get);
EXPORT_SYMBOL(aal_ploam_debug_sts0_get);
EXPORT_SYMBOL(aal_ploam_debug_sts1_get);
EXPORT_SYMBOL(aal_ploam_debug_sts2_get);
EXPORT_SYMBOL(aal_ploam_bc_ploam_seq_get);
EXPORT_SYMBOL(aal_ploam_uc_ploam_seq_get);
EXPORT_SYMBOL(aal_ploam_ds_burst_profile_set);
EXPORT_SYMBOL(aal_ploam_ds_burst_profile_get);
EXPORT_SYMBOL(aal_ploam_ds_assign_onuid_set);
EXPORT_SYMBOL(aal_ploam_ds_assign_onuid_get);
EXPORT_SYMBOL(aal_ploam_ds_ranging_time_set);
EXPORT_SYMBOL(aal_ploam_ds_ranging_time_get);
EXPORT_SYMBOL(aal_ploam_ds_deact_onuid_set);
EXPORT_SYMBOL(aal_ploam_ds_deact_onuid_get);
EXPORT_SYMBOL(aal_ploam_ds_disable_sn_set);
EXPORT_SYMBOL(aal_ploam_ds_disable_sn_get);
EXPORT_SYMBOL(aal_ploam_ds_request_reg_set);
EXPORT_SYMBOL(aal_ploam_ds_request_reg_get);
EXPORT_SYMBOL(aal_ploam_ds_assign_allocid_set);
EXPORT_SYMBOL(aal_ploam_ds_assign_allocid_get);
EXPORT_SYMBOL(aal_ploam_ds_key_ctrl_set);
EXPORT_SYMBOL(aal_ploam_ds_key_ctrl_get);
EXPORT_SYMBOL(aal_ploam_ds_sleep_allow_set);
EXPORT_SYMBOL(aal_ploam_ds_sleep_allow_get);
EXPORT_SYMBOL(aal_ploam_ds_calibration_req_set);
EXPORT_SYMBOL(aal_ploam_ds_calibration_req_get);
EXPORT_SYMBOL(aal_ploam_ds_adjust_tx_wave_set);
EXPORT_SYMBOL(aal_ploam_ds_adjust_tx_wave_get);
EXPORT_SYMBOL(aal_ploam_ds_tuning_ctrl_set);
EXPORT_SYMBOL(aal_ploam_ds_tuning_ctrl_get);
EXPORT_SYMBOL(aal_ploam_ds_sys_profile_set);
EXPORT_SYMBOL(aal_ploam_ds_sys_profile_get);
EXPORT_SYMBOL(aal_ploam_ds_channel_profile_set);
EXPORT_SYMBOL(aal_ploam_ds_channel_profile_get);
EXPORT_SYMBOL(aal_ploam_ds_protection_ctrl_set);
EXPORT_SYMBOL(aal_ploam_ds_protection_ctrl_get);
EXPORT_SYMBOL(aal_ploam_ds_change_power_set);
EXPORT_SYMBOL(aal_ploam_ds_change_power_get);
EXPORT_SYMBOL(aal_ploam_ds_power_consume_set);
EXPORT_SYMBOL(aal_ploam_ds_power_consume_get);
EXPORT_SYMBOL(aal_ploam_ds_rate_ctrl_set);
EXPORT_SYMBOL(aal_ploam_ds_rate_ctrl_get);
EXPORT_SYMBOL(aal_ploam_us_serial_number_set);
EXPORT_SYMBOL(aal_ploam_us_serial_number_get);
EXPORT_SYMBOL(aal_ploam_us_registration_set);
EXPORT_SYMBOL(aal_ploam_us_registration_get);
EXPORT_SYMBOL(aal_ploam_us_key_report_set);
EXPORT_SYMBOL(aal_ploam_us_key_report_get);
EXPORT_SYMBOL(aal_ploam_us_ack_set);
EXPORT_SYMBOL(aal_ploam_us_ack_get);
EXPORT_SYMBOL(aal_ploam_us_sleep_req_set);
EXPORT_SYMBOL(aal_ploam_us_sleep_req_get);
EXPORT_SYMBOL(aal_ploam_us_tuning_resp_set);
EXPORT_SYMBOL(aal_ploam_us_tuning_resp_get);
EXPORT_SYMBOL(aal_ploam_us_power_consumption_report_set);
EXPORT_SYMBOL(aal_ploam_us_power_consumption_report_get);
EXPORT_SYMBOL(aal_ploam_us_rate_resp_set);
EXPORT_SYMBOL(aal_ploam_us_rate_resp_get);
EXPORT_SYMBOL(aal_ploam_us_pkt_send_set);
EXPORT_SYMBOL(aal_ploam_us_pkt_send_get);
EXPORT_SYMBOL(aal_ploam_loopback_ploam_send_set);
EXPORT_SYMBOL(aal_ploam_loopback_ploam_send_get);
EXPORT_SYMBOL(aal_ploam_tuning_cfg_set);
EXPORT_SYMBOL(aal_ploam_tuning_cfg_get);
EXPORT_SYMBOL(aal_ploam_tuning_sw_ack_set);
EXPORT_SYMBOL(aal_ploam_tuning_sw_ack_get);
EXPORT_SYMBOL(aal_ploam_tuning_sw_resp_set);
EXPORT_SYMBOL(aal_ploam_tuning_sw_resp_get);
EXPORT_SYMBOL(aal_ploam_power_consumption_set);
EXPORT_SYMBOL(aal_ploam_power_consumption_get);
EXPORT_SYMBOL(aal_ploam_msg_cnt_get);
EXPORT_SYMBOL(aal_ploam_dbg_capture_get);
EXPORT_SYMBOL(aal_ploam_dbg_get);
EXPORT_SYMBOL(aal_ploam_dbg_set);
EXPORT_SYMBOL(aal_ploam_dbg_capture_add);
