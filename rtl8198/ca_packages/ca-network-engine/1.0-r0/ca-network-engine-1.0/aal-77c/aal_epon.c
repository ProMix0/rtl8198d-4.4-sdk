
#include "osal_common.h"
#include <soc/cortina/registers.h>
#include "aal_common.h"
#include "aal_pon.h"
#include "aal_epon.h"
#include "aal_epon_mac.h"
#include "aal_epon_fec.h"
#include "aal_epon_intr.h"
#include "aal_mpcp.h"
#include "aal_epon_crypto.h"
#include "aal_puc.h"
#include "aal_pdc.h"
#include "aal_psds.h"
#include "rx.h"
#include "epon.h"
#include "pon.h"
#include "scfg.h"
#include "osal_timer.h"

extern aal_puc_queue_cfg_t tmp_puc_queue_cfg[AAL_XGPON_MAX_TCONT_NUM][8];
extern aal_puc_ctrl_cfg_t tmp_puc_ctrl_config;
extern aal_puc_btc_cfg_t tmp_puc_btc_config;
extern aal_puc_voq_tbc_mem_cfg_t tmp_puc_voq_tbc_mem_cfg[AAL_PUC_MAX_QUEUE_NUM];
extern aal_puc_voq_tbc_mem_cfg_t tmp_puc_rl_voq_tbc_mem_cfg[AAL_PUC_MAX_QUEUE_NUM];
extern ca_uint32_t tmp_puc_bp_tqmvoqid[64];//bp only has 64 entries
extern ca_uint32_t tmp_puc_voq_buf_lmt_sel1500_cfg;
extern ca_uint32_t tmp_puc_voq_buf_lmt_sel3116_cfg;
extern ca_uint32_t tmp_puc_voq_buf_lmt_sel4732_cfg;
extern ca_uint32_t tmp_puc_voq_buf_lmt_sel6348_cfg;
extern ca_uint32_t tmp_puc_voq_buf_limit_a;
extern ca_uint32_t tmp_puc_voq_buf_limit_b;
extern ca_uint32_t tmp_puc_voq_buf_limit_c;

extern ca_uint32_t llid_num_per_chip;
extern ca_uint32_t queue_num_per_llid;

ca_int32_t aal_epon_debug = 1;
ca_uint32_t epon_link_fail_times  = 0;  /* counter for epon link fail event */
ca_uint32_t epon_link_reset_times = 0;

#define __EPON_LASER_TX_DELAY_MAX            0x3f
#define __EPON_SERDES_FAIL_THRESHOLD         30

ca_status_t aal_epon_lao_set
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_boolean_t flag
)
{
    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, laser_always_on, flag);

    AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_STATIC_CFG, laser_always_on, flag);

    return CA_E_OK;
}

ca_status_t aal_epon_lao_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_boolean_t *flag
)
{
    ca_status_t rtn = CA_E_PARAM;
    AAL_ASSERT_RET(flag, rtn);

    AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_STATIC_CFG, laser_always_on, *flag);

    return CA_E_OK;
}

ca_status_t aal_epon_laser_enable_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t flag
)
{
    aal_epon_laser_cfg_t cfg;
    aal_epon_laser_msk_t msk;

    memset((void *)&msk, 0, sizeof(msk));
    memset((void *)&cfg, 0, sizeof(cfg));

    msk.s.tx_ctrl_en = 1;
    cfg.tx_ctrl_en   = flag;

    return aal_epon_laser_cfg_set(device_id, port_id, msk, &cfg);
}

ca_status_t aal_epon_laser_enable_get
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t *flag
)
{
    aal_epon_laser_cfg_t cfg;

    if (!flag)
        return CA_E_PARAM;

    memset((void *)&cfg, 0, sizeof(cfg));

    if (aal_epon_laser_cfg_get(device_id, port_id, &cfg) != CA_E_OK)
        return CA_E_ERROR;

    *flag = cfg.tx_ctrl_en;

    return CA_E_OK;
}

ca_status_t aal_epon_laser_cfg_set
(
    CA_IN ca_device_id_t          device_id,
    CA_IN ca_port_id_t         port_id,
    CA_IN aal_epon_laser_msk_t msk,
    CA_IN aal_epon_laser_cfg_t *cfg
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    if(msk.s.tx_mode && (cfg->tx_mode > AAL_EPON_LASER_MODE_OFF)){
        return CA_E_PARAM;
    }

    if(msk.s.tx_delay_1g && ( cfg->tx_delay_1g > __EPON_LASER_TX_DELAY_MAX )){
        return CA_E_PARAM;
    }

    if(msk.s.tx_delay_xg && ( cfg->tx_delay_xg > __EPON_LASER_TX_DELAY_MAX )){
        return CA_E_PARAM;
    }

    if(msk.s.tx_mode){
        switch (cfg->tx_mode){
            case AAL_EPON_LASER_MODE_BURST:
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, dis_opt_power, 0);
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, laser_always_on, 0);
                break;
            case AAL_EPON_LASER_MODE_ON:
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, dis_opt_power, 0);
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, laser_always_on, 1);
                break;
            case AAL_EPON_LASER_MODE_OFF:
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, dis_opt_power, 1);
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, laser_always_on, 0);
                break;
            default:
                break;
        }
        //AAL_PON_REG_FIELD_WRITE(EPON_XGONU_MPCP_MPCP_CFG0, laser_ctrl, (ca_uint32_t)cfg->tx_mode);
    }

    if(msk.s.tx_ctrl_en){
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, tx_dis_oen, (cfg->tx_ctrl_en == 0));
    }

    if(msk.s.tx_power){
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, dis_opt_power, (cfg->tx_power == 0));
    }

    if(msk.s.tx_power_polarity){
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, dis_opt_power_polarity, (cfg->tx_power_polarity == 0));/*OLT only*/
    }

    if(msk.s.tx_burst_en){
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, tx_burst_oen, (cfg->tx_burst_en == 0)); /*???*/
    }

    if(msk.s.tx_burst_polarity_1g){
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, laser_burst_polarity, (cfg->tx_burst_polarity_1g!=0));
    }

    if(msk.s.tx_burst_polarity_xg){
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_TX_TXCNTRL, laserctrlpolarity, cfg->tx_burst_polarity_xg);
    }

    if(msk.s.tx_delay_1g){
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, laser_delay, cfg->tx_delay_1g);
    }

    if(msk.s.tx_delay_xg){
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_TX_TXCNTRL, laserctrldelay, cfg->tx_delay_xg);
    }

    if(msk.s.tx_idle_off){
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_TX_TXCNTRL, close_laser_advance_en, cfg->tx_idle_off);
    }

    if(msk.s.rssi_pulse_on){
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, rssi_pulse_on, (cfg->rssi_pulse_on != 0)); /*OLT only*/
    }

    if(msk.s.rssi_polarity){
        AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, rssi_polarity, (cfg->rssi_polarity != 0)); /*OLT only*/
    }

    return CA_E_OK;
}

ca_status_t aal_epon_laser_cfg_get
(
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_OUT aal_epon_laser_cfg_t *cfg
)
{
    EPON_EPON_GLB_STATIC_CFG_t static_cfg;
    EPON_EPON_GLB_MISC_CTRL_t  misc_ctrl;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    AAL_ASSERT_RET(cfg, CA_E_PARAM);

    misc_ctrl.wrd =  AAL_PON_REG_READ(EPON_EPON_GLB_MISC_CTRL);
    static_cfg.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_STATIC_CFG);

    if (misc_ctrl.bf.dis_opt_power == 1)
        cfg->tx_mode = AAL_EPON_LASER_MODE_OFF;
    else if (static_cfg.bf.laser_always_on)
        cfg->tx_mode = AAL_EPON_LASER_MODE_ON;
    else
        cfg->tx_mode = AAL_EPON_LASER_MODE_BURST;

    //AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, laser_ctrl, cfg->tx_mode);
    cfg->tx_ctrl_en = !misc_ctrl.bf.tx_dis_oen;
    cfg->tx_power = !misc_ctrl.bf.dis_opt_power;
    cfg->tx_power_polarity = !misc_ctrl.bf.dis_opt_power_polarity;
    cfg->tx_burst_en = !misc_ctrl.bf.tx_burst_oen;
    cfg->rssi_pulse_on = misc_ctrl.bf.rssi_pulse_on;
    cfg->rssi_polarity = misc_ctrl.bf.rssi_polarity;

    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, laser_burst_polarity, cfg->tx_burst_polarity_1g);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XGPCS_TX_TXCNTRL, laserctrlpolarity, cfg->tx_burst_polarity_xg);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, laser_delay, cfg->tx_delay_1g);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XGPCS_TX_TXCNTRL, laserctrldelay, cfg->tx_delay_xg);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XGPCS_TX_TXCNTRL, close_laser_advance_en, cfg->tx_idle_off);

    return CA_E_OK;
}

ca_status_t aal_epon_status_get
(
    CA_IN  ca_device_id_t    device_id,
    CA_IN  ca_port_id_t      port_id,
    CA_OUT aal_epon_status_t *status
)
{
    EPON_EPON_GLB_EPON_STATUS_t epon_status;
    ca_status_t rtn = CA_E_PARAM;

    AAL_ASSERT_RET(status, rtn);

    epon_status.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_EPON_STATUS);

    status->sig_loss    = epon_status.bf.sigloss;
    status->pcs_sync_xg = epon_status.bf.pcs_sync_xg;
    status->pcs_sync_1g = epon_status.bf.pcs_sync_1g;
    status->ether_mode  = epon_status.bf.ethernet_mode;
    status->speed       = epon_status.bf.datapath_speed;

    return CA_E_OK;
}

ca_status_t aal_epon_opt_status_get
(
    CA_IN  ca_device_id_t        device_id,
    CA_IN  ca_port_id_t          port_id,
    CA_OUT aal_epon_opt_status_t *status
)
{
    GLOBAL_OPT_MODULE_STATUS_t opt_status;
    ca_status_t rtn = CA_E_PARAM;

    AAL_ASSERT_RET(status, rtn);

    opt_status.wrd = AAL_GLB_REG_READ(GLOBAL_OPT_MODULE_STATUS);

    status->opt_rx_los    = opt_status.bf.opt_rx_los;
    status->opt_mod_abs   = opt_status.bf.opt_mod_abs;
    status->opt_tx_sd     = opt_status.bf.opt_tx_sd;
    status->opt_tx_fault  = opt_status.bf.opt_tx_fault;

    return CA_E_OK;
}

extern ca_pon_tx_cb ca_pon_tx_handle;
ca_uint8 dying_gasp_pkt_buf[1024];
ca_uint8 aal_epon_oam_pdu[] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02, 0x00, 0x13, 0x25, 0x00, 0x00, 0x01, 0x88, 0x09, 0x03, 0x00,
0x52, 0x01, 0x00, 0x11, 0xfe, 0x0e, 0x00, 0x0d, 0xb6, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static void
aal_epon_octet_dump( char *tp, unsigned char *dp, int len )
{
  unsigned char *cp = dp;
  int  ii;
  int  jj;

  printk("%s\n", tp);
  for ( ii=len; ii>16; ii-=16 )
  {
    for ( jj=0; jj<16; jj++, cp++ )
      printk("%02x ", *cp);
    printk("\n");
  }
  for ( jj=0; jj<ii; jj++, cp++ )
  printk("%02x ", *cp);
  printk("\n");
}

static int gasp_pdu_epon = 0, gasp_send_cnt = 1;

void aal_epon_dying_gasp_handler(ca_int32 gpio_pin, ca_uint8 status)
{
    ca_pkt_t *pkt_p = (ca_pkt_t *)dying_gasp_pkt_buf;
    ca_status_t   ret = CA_E_OK;
    ca_uint32_t i = 0;

    if (!gasp_pdu_epon) return;

    for (i = 0; i < gasp_send_cnt; i++){
        ret = ca_pon_tx_handle(0, pkt_p);
        if(CA_E_OK != ret){
            ca_printf("dying gasp tx [%d]failed:ret:%d\n", i, ret);
        }
        pkt_p->src_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, 0x10);
        pkt_p->pkt_data->len = pkt_p->pkt_len = sizeof(aal_epon_oam_pdu);

    }

    printk("dge%d\r\n", gasp_send_cnt);
    //gasp_pdu_epon = 0;
    return;
}

ca_status_t aal_epon_tx_sw_frame_data_set
(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_llid_t         llid,
    CA_IN ca_uint8_t        *pkt_buff,
    CA_IN ca_int32_t        pkt_len
)
{
    EPON_EPON_GLB_TX_SW_FRAME_DATA_ACCESS_t reg_access;
    EPON_EPON_GLB_TX_SW_START_t tx_start;
    aal_epon_tx_sw_frame_header_t   frame_hda;
    ca_int32_t reg_data1, reg_data0;
    int i, base_addr;
    ca_uint8_t  *tx_buf = NULL;
    ca_uint32_t buf_len = ALIGN(pkt_len, 8);
    ca_device_config_tlv_t scfg;
    ca_status_t ret = CA_E_OK;

    AAL_ASSERT_RET(pkt_buff != NULL, CA_E_PARAM);

    if (aal_epon_debug){
        ca_printf("llid:0x%x, pkt_len:%d, buf_len:%d\r\n", llid, pkt_len, buf_len);
    }

    tx_buf = ca_malloc(buf_len);
    if (tx_buf == NULL){
        ca_printf("malloc fail!\r\n");
        return CA_E_MEMORY;
    }

    memcpy(tx_buf, pkt_buff, pkt_len);

    memset(&scfg, 0, sizeof(scfg));
    scfg.type = CA_CFG_ID_MAC_ADDRESS;
    ret = ca_device_config_tlv_get(0, &scfg);
    if(ret == CA_E_OK && scfg.len == CA_ETH_ADDR_LEN){
        memcpy(tx_buf+ 6, scfg.value, CA_ETH_ADDR_LEN);
    }

    if (aal_epon_debug){
        ca_buffer_dump("data", tx_buf, buf_len);
    }

    reg_access.wrd = 0;
    frame_hda.wrd = 0;

    base_addr = 0xff - buf_len / 8 + 1;

    for (i = 0; i < buf_len/8; i++){
        reg_data1 = ((ca_uint32_t)(tx_buf[i*8]) << 24 )
                 | ((ca_uint32_t)(tx_buf[i*8 + 1]) << 16)
                 | ((ca_uint32_t)(tx_buf[i*8 + 2]) << 8)
                 | ((ca_uint32_t)(tx_buf[i*8 + 3]));
        AAL_PON_REG_WRITE(reg_data1, EPON_EPON_GLB_TX_SW_FRAME_DATA_DATA1);

        reg_data0 = ((ca_uint32_t)(tx_buf[i*8 + 4]) << 24 )
                 | ((ca_uint32_t)(tx_buf[i*8 + 5]) << 16)
                 | ((ca_uint32_t)(tx_buf[i*8 + 6]) << 8)
                 | ((ca_uint32_t)(tx_buf[i*8 + 7]));
        AAL_PON_REG_WRITE(reg_data0, EPON_EPON_GLB_TX_SW_FRAME_DATA_DATA0);
        AAL_PON_REG_INDIRECT_WRITE(base_addr, EPON_EPON_GLB_TX_SW_FRAME_DATA_ACCESS);
        ca_printf("base_addr:0x%x, reg_data1:0x%08x, reg_data0:0x%08x.\r\n", base_addr, reg_data1, reg_data0);

        base_addr++;
    }

    AAL_PERI_REG_WRITE(0x105,PER_DYING_GASP);
    AAL_PON_REG_WRITE(0x00F86040, EPON_EPON_GLB_TX_GASP_FRAME_HEADER);
    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, gasp_en, 1);
    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, tx_gasp_limit, 3);
    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, sw_gasp, 0);
    AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, sw_gasp_en, 0); 
    //AAL_PERI_REG_WRITE(1,PER_DG_IE_0); dg_int

    ca_free(tx_buf);
    tx_buf = NULL;

    ca_printf("base_addr:0x%x, write done.\r\n", base_addr);

    /* trigger transmit */
/*    tx_start.wrd = 0;
    tx_start.bf.ready = 1;

    AAL_PON_REG_WRITE(tx_start.wrd, EPON_EPON_GLB_TX_SW_START);
    ca_printf("tx start\r\n");
*/
    return CA_E_OK;

}
ca_status_t aal_epon_tx_sw_frame_data_get
(
    CA_IN ca_device_id_t    device_id,
    CA_OUT ca_llid_t        *llid,
    CA_OUT ca_uint8_t       *pkt_buff,
    CA_OUT ca_int32_t       *pkt_len
)
{
    ca_uint32_t  reg_data1, reg_data0;
    aal_epon_tx_sw_frame_header_t   frame_hda;
    int i, base_addr;
    ca_uint8_t  *tx_buf = NULL;
    ca_uint32_t buf_len = 0;

    //AAL_ASSERT_RET(pkt_buff && pkt_len, CA_E_PARAM);
    frame_hda.wrd = 0;
    frame_hda.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_TX_SW_FRAME_HEADER);
    buf_len = frame_hda.bf.pkt_sz;
    if (buf_len < 64) buf_len = 64;

    tx_buf = ca_malloc(buf_len);
    if (tx_buf == NULL){
        ca_printf("malloc fail!\r\n");
        return CA_E_MEMORY;
    }

    base_addr = 0xff - buf_len / 8 + 1;

    for (i = 0; i < buf_len/8; i++){
        AAL_PON_REG_INDIRECT_READ(base_addr, EPON_EPON_GLB_TX_SW_FRAME_DATA_ACCESS);

        reg_data1  = AAL_PON_REG_READ(EPON_EPON_GLB_TX_SW_FRAME_DATA_DATA1);
        tx_buf[i * 8]     = (reg_data1 >> 24 ) & 0xFF;
        tx_buf[i * 8 + 1] = (reg_data1 >> 16 ) & 0xFF;
        tx_buf[i * 8 + 2] = (reg_data1 >> 8 ) & 0xFF;
        tx_buf[i * 8 + 3] = (reg_data1) & 0xFF;

        reg_data0  = AAL_PON_REG_READ(EPON_EPON_GLB_TX_SW_FRAME_DATA_DATA0);
        tx_buf[i * 8 + 4] = (reg_data0 >> 24 ) & 0xFF;
        tx_buf[i * 8 + 5] = (reg_data0 >> 16 ) & 0xFF;
        tx_buf[i * 8 + 6] = (reg_data0 >> 8 ) & 0xFF;
        tx_buf[i * 8 + 7] = (reg_data0) & 0xFF;
        ca_printf("base_addr:0x%x, reg_data1:0x%08x, reg_data0:0x%08x.\r\n", base_addr, reg_data1, reg_data0);

        base_addr++;
    }

    ca_printf("llid:%d, frame_type:%d, pkt_sz:%d\r\n", frame_hda.bf.llid, frame_hda.bf.frame_type, frame_hda.bf.pkt_sz);

    ca_buffer_dump("data", tx_buf, buf_len);
    if (pkt_buff)
        memcpy(pkt_buff, tx_buf, buf_len);
    if (pkt_len)
        *pkt_len = buf_len;
    if (llid)
        *llid = frame_hda.bf.llid;

    ca_free(tx_buf);
    tx_buf = NULL;

    return CA_E_OK;
}


ca_status_t aal_epon_dying_gasp_get
(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t       port_id,
    CA_OUT ca_uint8_t        *send_count,
    CA_OUT ca_uint8          *pkt_buff,
    CA_OUT ca_int32          *pkt_len
)
{
  	ca_device_config_tlv_t scfg;
    ca_status_t ret;
    ca_uint32_t lpid = PORT_ID(port_id);

    if (lpid == 7){
        ret = aal_epon_tx_sw_frame_data_get(0, NULL, NULL, NULL);
        return ret;
    }

    memset(&scfg, 0, sizeof(scfg));
    scfg.type = CA_CFG_ID_MAC_ADDRESS;
    ret = ca_device_config_tlv_get(0, &scfg);
    if(ret == CA_E_OK && scfg.len == CA_ETH_ADDR_LEN){
        memcpy(aal_epon_oam_pdu+6, scfg.value, CA_ETH_ADDR_LEN);
    }

    ca_printf("<%s, %d>\r\n", __FUNCTION__, __LINE__);

    gasp_pdu_epon = 0;
    aal_epon_dying_gasp_set(0, port_id, gasp_send_cnt, aal_epon_oam_pdu, sizeof(aal_epon_oam_pdu));

    aal_epon_octet_dump("gasp111", dying_gasp_pkt_buf, 192);


    aal_epon_dying_gasp_handler(0, 0);

    return CA_E_OK;
}

ca_status_t aal_epon_dying_gasp_set
(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        send_count,
    CA_IN ca_uint8          *pkt_buff,
    CA_IN ca_int32          pkt_len
)
{
    ca_pkt_t *pkt = (ca_pkt_t *)dying_gasp_pkt_buf;
    ca_pkt_block_t  *pkt_data;
    ca_int32_t pkt_hdr_size, pkt_blk_hdr_size;
    ca_port_id_t lpid = PORT_ID(port_id);
    ca_status_t ret = CA_E_OK;

    if(pkt_buff == NULL || pkt_len == 0)
        return CA_E_ERROR;
    
    if (lpid == 7){
        ret = aal_epon_tx_sw_frame_data_set(0, lpid, pkt_buff, pkt_len);
        return ret;
    }
    if(lpid == 6){
        EPON_EPON_GLB_TX_SW_START_t tx_start;
        ca_uint32_t timeout = 0;

        /* trigger transmit */
        tx_start.wrd = 0;
        tx_start.bf.ready = 1;

        AAL_PON_REG_WRITE(tx_start.wrd, EPON_EPON_GLB_TX_SW_START);
        ca_printf("tx start\r\n");
        do {
            tx_start.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_TX_SW_START);

            if (timeout++ >= 0xfff) {
                ca_printf("timeout\r\n");
                return CA_E_TIMEOUT;
            }
        } while(tx_start.bf.ready);

        return CA_E_OK;
    }

    if(pkt_buff == NULL || pkt_len == 0)
        return CA_E_ERROR;

    if (gasp_pdu_epon)
        return CA_E_OK;

    gasp_send_cnt = send_count;
    lpid = port_id == 0 ? CA_PORT_ID(CA_PORT_TYPE_EPON, 7) : CA_PORT_ID(CA_PORT_TYPE_ETHERNET, port_id);

    pkt_hdr_size = sizeof(ca_pkt_t);
    pkt_blk_hdr_size = sizeof(ca_pkt_block_t);

    if(pkt_len + pkt_hdr_size + pkt_blk_hdr_size > sizeof(dying_gasp_pkt_buf))
        return CA_E_PARAM;

    pkt_data = (ca_pkt_block_t *)(dying_gasp_pkt_buf + pkt_hdr_size);

    pkt_data->data = dying_gasp_pkt_buf + pkt_hdr_size + pkt_blk_hdr_size;
    pkt_data->len = pkt_len;

    /*
    ca_printf("<%s, %d>dying_gasp_pkt_buf:%p, pkt:%p, pkt_data:%p, pkt_data->data:%p, size:%d, %d, len:%d\r\n", __FUNCTION__, __LINE__,
        dying_gasp_pkt_buf, pkt, pkt_data, pkt_data->data, pkt_hdr_size, pkt_blk_hdr_size, pkt_len);
    */

    memcpy(pkt_data->data, pkt_buff, pkt_len);

    //aal_epon_octet_dump("oam_dg", pkt_data->data, pkt_len);
    pkt->device_id = 0;
    pkt->flags = (0x1<<6);//CA_TX_BYPASS_FWD_ENGINE;
    pkt->pkt_data = pkt_data;
    pkt->block_count = 1;
    pkt->src_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, 0x10); // CPU0
    pkt->dst_port_id = lpid;//CA_PORT_ID(CA_PORT_TYPE_EPON, 7); //CA_PORT_ID_NI7

    //Using llid-idx 4 for registeration
    pkt->dst_sub_port_id = 4;   /* llid */

    pkt->pkt_len = pkt_data->len;
    pkt->cos = 7;
    pkt->pkt_type = CA_PKT_TYPE_OAM;

    gasp_pdu_epon = 1;
    ca_printf("dg oam set done, len:%d, gasp_send_cnt:%d\r\n", pkt_len, gasp_send_cnt);

    return CA_E_OK;
}

ca_status_t aal_epon_datapath_csr_reset
(
    CA_IN ca_device_id_t device_id
)
{
    GLOBAL_PON_CNTL_t pon_cfg;


    pon_cfg.wrd = 0;
    pon_cfg.bf.puc_reset = 1;
    pon_cfg.bf.pdc_reset = 1;
    pon_cfg.bf.ptp_rst_n = 1;
    pon_cfg.bf.psds_reg_rst_n   = 1;
    pon_cfg.bf.pon_serdes_rst_n = 1;
    pon_cfg.bf.epon_mode        = 1;    /* EPON mode */
    AAL_GLB_REG_WRITE(pon_cfg.wrd, GLOBAL_PON_CNTL);

    ca_mdelay(100);

    return CA_E_OK;
}

ca_status_t aal_epon_backup_ponmac(ca_device_id_t device_id)
{
    ca_uint32_t llidId;
    ca_uint32_t priId;
    ca_uint32_t queue_idx;

    ca_uint32_t br_llid_num = AAL_XGPON_MAX_TCONT_NUM;
    ca_uint32_t br_queue_num = 8;
       
    if(llid_num_per_chip < AAL_XGPON_MAX_TCONT_NUM)
        br_llid_num = llid_num_per_chip;

    if(queue_num_per_llid < 8)
        br_queue_num = queue_num_per_llid;
    

    for(llidId=0;llidId<br_llid_num;llidId++)
    {
        for(priId=0;priId<br_queue_num;priId++)
        {
            aal_puc_port_voq_map_get(device_id, llidId, priId,&tmp_puc_queue_cfg[llidId][priId]);
//            EPON_DEBUG("llidId=%d priId=%d queue_id=%d weight=%d pri=%d enable=%d\n",llidId,priId
//                ,tmp_puc_queue_cfg[llidId][priId].queue_id
//                ,tmp_puc_queue_cfg[llidId][priId].queue_weight
//                ,tmp_puc_queue_cfg[llidId][priId].queue_pri
//                ,tmp_puc_queue_cfg[llidId][priId].queue_ena);
        }
    }

    aal_puc_ctrl_cfg_get(device_id,&tmp_puc_ctrl_config);
//    EPON_DEBUG("puc_ctrl_cfg pkt_mode_class0_cnt=%d pkt_mode_class1_cnt=%d force_init_done=%d addtok_en=%d subtok_en=%d shp_en=%d\n"
//            ,tmp_puc_ctrl_config.pkt_mode_class0_cnt
//            ,tmp_puc_ctrl_config.pkt_mode_class1_cnt
//            ,tmp_puc_ctrl_config.force_init_done
//            ,tmp_puc_ctrl_config.addtok_en
//            ,tmp_puc_ctrl_config.subtok_en
//            ,tmp_puc_ctrl_config.shp_en);


    aal_puc_btc_cfg_get(device_id,&tmp_puc_btc_config);
//    EPON_DEBUG("pfovrhd=%d schmode=%d ethmode=%d wdaligned=%d lrgfrmfragen=%d pyld1gfecon=%d schen=%d minrmnwindowsz=%d\n" 
//            ,tmp_puc_btc_config.pfovrhd
//            ,tmp_puc_btc_config.schmode
//            ,tmp_puc_btc_config.ethmode
//            ,tmp_puc_btc_config.wdaligned
//            ,tmp_puc_btc_config.lrgfrmfragen
//            ,tmp_puc_btc_config.pyld1gfecon
//            ,tmp_puc_btc_config.schen
//            ,tmp_puc_btc_config.minrmnwindowsz);

    for(queue_idx=0;queue_idx<AAL_PUC_MAX_QUEUE_NUM;queue_idx++)
    {
        aal_puc_voq_tbc_mem_get(device_id,queue_idx,&(tmp_puc_voq_tbc_mem_cfg[queue_idx]));
//        EPON_DEBUG("puc_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
//            ,queue_idx
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_k
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_m
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].bs
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].enb
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].mode
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].tbc
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);

        aal_puc_rl_voq_tbc_mem_get(device_id,queue_idx,&(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx]));
//        EPON_DEBUG("puc_rl_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
//            ,queue_idx
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_k
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_m
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].bs
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].mode
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].tbc
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
    }

    for(queue_idx=0;queue_idx<64;queue_idx++)//bp only has 64 entries
    {
        aal_puc_voq_bp_remap_data_get(device_id, queue_idx, &(tmp_puc_bp_tqmvoqid[queue_idx]));
//        EPON_DEBUG("puc_voq_bp_remap_data idx=%d tqmvoqid=%d\n",queue_idx,tmp_puc_bp_tqmvoqid[queue_idx]);
    }

    aal_puc_voq_buf_lmt_sel1500_get(device_id, &tmp_puc_voq_buf_lmt_sel1500_cfg);
//    EPON_DEBUG("puc_voq_buf_lmt 1500=%d\n",tmp_puc_voq_buf_lmt_sel1500_cfg);

    aal_puc_voq_buf_lmt_sel3116_get(device_id, &tmp_puc_voq_buf_lmt_sel3116_cfg);
//    EPON_DEBUG("puc_voq_buf_lmt 3116=%d\n",tmp_puc_voq_buf_lmt_sel3116_cfg);

    aal_puc_voq_buf_lmt_sel4732_get(device_id, &tmp_puc_voq_buf_lmt_sel4732_cfg);
//    EPON_DEBUG("puc_voq_buf_lmt 3116=%d\n",tmp_puc_voq_buf_lmt_sel4732_cfg);

    aal_puc_voq_buf_lmt_sel6348_get(device_id, &tmp_puc_voq_buf_lmt_sel6348_cfg);
//    EPON_DEBUG("puc_voq_buf_lmt 6348=%d\n",puc_voq_buf_lmt_sel6348_cfg);

    aal_puc_voq_buf_lmt_a_get(device_id, &tmp_puc_voq_buf_limit_a);
//    EPON_DEBUG("puc_voq_buf_lmt a=%d\n",tmp_puc_voq_buf_limit_a);
    
    aal_puc_voq_buf_lmt_b_get(device_id, &tmp_puc_voq_buf_limit_b);
//    EPON_DEBUG("puc_voq_buf_lmt b=%d\n",tmp_puc_voq_buf_limit_b);
    
    aal_puc_voq_buf_lmt_c_get(device_id, &tmp_puc_voq_buf_limit_c);
//    EPON_DEBUG("puc_voq_buf_lmt c=%d\n",tmp_puc_voq_buf_limit_c);

    return CA_E_OK;
}

ca_status_t aal_epon_restore_ponmac(ca_device_id_t device_id)
{
    ca_uint32_t llidId;

    ca_uint32_t priId;
    aal_puc_voq_drop_cfg_msk_t voq_drp_mask;
    aal_puc_voq_drop_cfg_t voq_drp;
    aal_puc_queue_cfg_msk_t queue_cfg_mask;
    aal_puc_queue_cfg_t queue_cfg;
    aal_puc_voq_sche_cfg_t voq_sche_config;
    ca_uint32_t tbc = 0;

    aal_puc_ctrl_cfg_msk_t puc_ctrl_config_mask; 
    aal_puc_btc_cfg_msk_t puc_btc_mask;

    ca_uint32_t queue_idx;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_voq_tbc_cfg_msk;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_rl_voq_tbc_cfg_msk;

    ca_uint32_t br_llid_num = AAL_XGPON_MAX_TCONT_NUM;
    ca_uint32_t br_queue_num = 8;
       
    if(llid_num_per_chip < AAL_XGPON_MAX_TCONT_NUM)
        br_llid_num = llid_num_per_chip;

    if(queue_num_per_llid < 8)
        br_queue_num = queue_num_per_llid;

//printk("%s %d br_llid_num=%d br_queue_num=%d\n",__FUNCTION__,__LINE__,br_llid_num,br_queue_num);
    
    for(llidId=0;llidId<br_llid_num;llidId++)
    {
        for(priId=0;priId<br_queue_num;priId++)
        {
//            EPON_DEBUG("llidId=%d priId=%d queue_id=%d weight=%d pri=%d enable=%d\n",llidId,priId
//                ,tmp_puc_queue_cfg[llidId][priId].queue_id
//                ,tmp_puc_queue_cfg[llidId][priId].queue_weight
//                ,tmp_puc_queue_cfg[llidId][priId].queue_pri
//                ,tmp_puc_queue_cfg[llidId][priId].queue_ena);

            voq_drp_mask.u32      = 0;
            voq_drp_mask.s.voqid  = 1;
            voq_drp_mask.s.enable = 1;

            memset(&voq_drp, 0, sizeof(voq_drp));
            voq_drp.enable = 0;
            voq_drp.voqid = tmp_puc_queue_cfg[llidId][priId].queue_id;
            aal_puc_voq_drop_set(device_id, voq_drp_mask, &voq_drp);

            queue_cfg_mask.u32 = ~0U;    
            queue_cfg.queue_id = tmp_puc_queue_cfg[llidId][priId].queue_id;
            queue_cfg.queue_weight = tmp_puc_queue_cfg[llidId][priId].queue_weight;
            queue_cfg.queue_pri = tmp_puc_queue_cfg[llidId][priId].queue_pri;
            queue_cfg.queue_ena = tmp_puc_queue_cfg[llidId][priId].queue_ena;

            aal_puc_port_voq_map_set(device_id, llidId, priId, queue_cfg_mask, &queue_cfg);
#ifdef RTK_VOQ_VALID_SUPPORT
            aal_puc_voq_valid_set(device_id, queue_cfg.queue_id, queue_cfg.queue_ena);
#endif
            memset(&voq_sche_config, 0, sizeof(aal_puc_voq_sche_cfg_t));
            aal_puc_voq_sche_cfg_get(device_id,&voq_sche_config);
            if(voq_sche_config.basesel == 0)
                tbc = tmp_puc_queue_cfg[llidId][priId].queue_weight * 128;
            if(voq_sche_config.basesel == 1)
                tbc = tmp_puc_queue_cfg[llidId][priId].queue_weight * 256;
            if(voq_sche_config.basesel == 2)
                tbc = tmp_puc_queue_cfg[llidId][priId].queue_weight * 512;
            if(voq_sche_config.basesel == 3)
                tbc = tmp_puc_queue_cfg[llidId][priId].queue_weight * 1024;
            aal_puc_port_voq_sche_info_tbl_set(device_id,llidId,priId,tbc);
        }
    }
#ifdef RTK_VOQ_VALID_SUPPORT
    aal_puc_voq_valid_set(device_id, 127, TRUE);

    voq_drp_mask.u32 = 0;
    memset(&voq_drp, 0, sizeof(aal_puc_voq_drop_cfg_t));
    voq_drp_mask.s.invalid_voqdrop_enable = 1;
    voq_drp_mask.s.enable = 1;
    voq_drp.invalid_voqdrop_enable = 1;
    voq_drp.enable = 0;
    aal_puc_voq_drop_set(device_id, voq_drp_mask, &voq_drp);
#endif
//    EPON_DEBUG("puc_ctrl_cfg pkt_mode_class0_cnt=%d pkt_mode_class1_cnt=%d force_init_done=%d addtok_en=%d subtok_en=%d shp_en=%d\n"
//            ,tmp_puc_ctrl_config.pkt_mode_class0_cnt
//            ,tmp_puc_ctrl_config.pkt_mode_class1_cnt
//            ,tmp_puc_ctrl_config.force_init_done
//            ,tmp_puc_ctrl_config.addtok_en
//            ,tmp_puc_ctrl_config.subtok_en
//            ,tmp_puc_ctrl_config.shp_en);
    puc_ctrl_config_mask.u32 = ~0;
    aal_puc_ctrl_cfg_set(device_id, puc_ctrl_config_mask, &tmp_puc_ctrl_config);

//    EPON_DEBUG("pfovrhd=%d schmode=%d ethmode=%d wdaligned=%d lrgfrmfragen=%d pyld1gfecon=%d schen=%d minrmnwindowsz=%d\n" 
//            ,tmp_puc_btc_config.pfovrhd
//            ,tmp_puc_btc_config.schmode
//            ,tmp_puc_btc_config.ethmode
//            ,tmp_puc_btc_config.wdaligned
//            ,tmp_puc_btc_config.lrgfrmfragen
//            ,tmp_puc_btc_config.pyld1gfecon
//            ,tmp_puc_btc_config.schen
//            ,tmp_puc_btc_config.minrmnwindowsz);
    puc_btc_mask.u32 = ~0;
    aal_puc_btc_cfg_set(device_id, puc_btc_mask, &tmp_puc_btc_config);

    puc_voq_tbc_cfg_msk.u32=~0;
    puc_rl_voq_tbc_cfg_msk.u32=~0;
    for(queue_idx=0;queue_idx<AAL_PUC_MAX_QUEUE_NUM;queue_idx++)
    {
#ifdef RTK_VOQ_VALID_SUPPORT
        if(tmp_puc_voq_tbc_mem_cfg[queue_idx].enb == 1) //workaround
            tmp_puc_voq_tbc_mem_cfg[queue_idx].enb = 0;
#endif
        
//        EPON_DEBUG("puc_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
//            ,queue_idx
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_k
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_m
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].bs
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].enb
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].mode
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].tbc
//            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
        aal_puc_voq_tbc_mem_set(device_id,queue_idx,puc_voq_tbc_cfg_msk,&(tmp_puc_voq_tbc_mem_cfg[queue_idx]));
#ifdef RTK_VOQ_VALID_SUPPORT
        if(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb == 1) //workaround
            tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb = 0;
#endif

//        EPON_DEBUG("puc_rl_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
//            ,queue_idx
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_k
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_m
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].bs
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].mode
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].tbc
//            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
        aal_puc_rl_voq_tbc_mem_set(device_id,queue_idx,puc_rl_voq_tbc_cfg_msk,&(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx]));
    }

    for(queue_idx=0;queue_idx<64;queue_idx++)//bp only has 64 entries
    {
//        EPON_DEBUG("puc_voq_bp_remap_data idx=%d tqmvoqid=%d\n",queue_idx,tmp_puc_bp_tqmvoqid[queue_idx]);
        aal_puc_voq_bp_remap_data_set(device_id, queue_idx, tmp_puc_bp_tqmvoqid[queue_idx]);
    }

//    EPON_DEBUG("puc_voq_buf_lmt 1500=%d\n",puc_voq_buf_lmt_sel1500_cfg);
    aal_puc_voq_buf_lmt_sel1500_set(device_id, tmp_puc_voq_buf_lmt_sel1500_cfg);

//    EPON_DEBUG("puc_voq_buf_lmt 3116=%d\n",puc_voq_buf_lmt_sel3116_cfg);
    aal_puc_voq_buf_lmt_sel3116_set(device_id, tmp_puc_voq_buf_lmt_sel3116_cfg);

//    EPON_DEBUG("puc_voq_buf_lmt 4732=%d\n",puc_voq_buf_lmt_sel4732_cfg);
    aal_puc_voq_buf_lmt_sel4732_set(device_id, tmp_puc_voq_buf_lmt_sel4732_cfg);

//    EPON_DEBUG("puc_voq_buf_lmt 6348=%d\n",puc_voq_buf_lmt_sel6348_cfg);
    aal_puc_voq_buf_lmt_sel6348_set(device_id, tmp_puc_voq_buf_lmt_sel6348_cfg);

//    EPON_DEBUG("puc_voq_buf_lmt a=%d\n",tmp_puc_voq_buf_limit_a);
    aal_puc_voq_buf_lmt_a_set(device_id, tmp_puc_voq_buf_limit_a);

//    EPON_DEBUG("puc_voq_buf_lmt b=%d\n",tmp_puc_voq_buf_limit_b);
    aal_puc_voq_buf_lmt_b_set(device_id, tmp_puc_voq_buf_limit_b);
    
//    EPON_DEBUG("puc_voq_buf_lmt c=%d\n",tmp_puc_voq_buf_limit_c);
    aal_puc_voq_buf_lmt_c_set(device_id, tmp_puc_voq_buf_limit_c);
    return CA_E_OK;
}


/* Reset SerDes */
ca_status_t aal_epon_reset(ca_device_id_t device_id)
{
    EPON_EPON_GLB_INTERRUPTE_t epon_glb;
    ca_uint32_t i = 0;
    ca_uint32_t intr_en[AAL_EPON_INTR_INDEX_MAX+1] = {0};

    ca_printf("ca-epon: epon link-fail was detected for (%d) times - attempt to reset SEDRES again.\n", epon_link_reset_times);

    __PON_LOCK();

    aal_epon_backup_ponmac(device_id);

    (void)aal_psds_burst_enable(device_id, FALSE);

    epon_glb.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_INTERRUPTE);

    aal_pon_mac_enable_set(device_id, 0, 1, 0); /* disable PON MAC Rx */

    for (i=0; i<=AAL_EPON_INTR_INDEX_MAX; i++)
    {
        aal_epon_intr_ena_get(device_id, i, &intr_en[i]); /* disable at first   */
        aal_epon_intr_status_clr(device_id, i, ~0);       /* clear status then  */
        aal_epon_intr_ena_set(device_id, i, 0);           /* disable at first   */
        aal_epon_intr_status_clr(device_id, i, ~0);       /* clear status again */
    }

    AAL_PON_REG_WRITE(0, EPON_EPON_GLB_INTERRUPTE); /* disable top enable */
    AAL_PON_REG_READ(EPON_EPON_GLB_INTERRUPT);      /* read out */

    (void)aal_psds_reset(device_id, aal_pon_mac_mode_get(device_id));

    AAL_GLB_REG_FIELD_WRITE(GLOBAL_EPON_CNTL, rx_coresec_10_rst_n, 0);  /* reset coresec 10 */
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_EPON_CNTL, tx_coresec_10_rst_n, 0);

    ca_mdelay(1);

    AAL_GLB_REG_FIELD_WRITE(GLOBAL_EPON_CNTL, rx_coresec_10_rst_n, 1);  /* release reset */
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_EPON_CNTL, tx_coresec_10_rst_n, 1);

    for (i=0; i<=AAL_EPON_INTR_INDEX_MAX; i++)
    {
        aal_epon_intr_ena_set(device_id, i, intr_en[i]);/* recover enable interrupt  */
    }

    AAL_PON_REG_WRITE(epon_glb.wrd, EPON_EPON_GLB_INTERRUPTE);

    aal_pon_mac_enable_set(device_id, 0, 1, 1);     /* enable PON MAC Rx */

    (void)aal_psds_burst_enable(device_id, TRUE);

    aal_epon_restore_ponmac(device_id);

    __PON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_epon_reset_all_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    mode
)
{
    (void)aal_epon_reset(0);

    return CA_E_OK;
}

ca_status_t aal_epon_reset_all_get
(
    CA_IN  ca_device_id_t device_id,
    CA_OUT ca_uint32_t    *mode
)
{
    if (!mode)
        return CA_E_PARAM;

    *mode = 0;

    return CA_E_OK;
}

void aal_epon_link_monitor_handler(void *data)
{
    aal_epon_status_t status;
    aal_psds_status_t sds_status;

    memset(&sds_status, 0, sizeof(sds_status));

    __PON_LOCK();
    (void)aal_psds_status_get(0, &sds_status);
    __PON_UNLOCK();

    if (sds_status.rx_los) /* No Rx optic */
    {
        epon_link_fail_times = 0;
        return;
    }

    if (!sds_status.ckrdy_mpcp || !sds_status.ckrdy_rx || !sds_status.ckrdy_tx)
    {
        /* there is optical signal but SerDes clock is not ready */
        epon_link_fail_times++;
    }
    else
    {
        memset(&status, 0, sizeof(status));

        __PON_LOCK();
        (void)aal_epon_status_get(0, 0, &status);
        __PON_UNLOCK();

        if ((status.pcs_sync_xg && (status.speed > AAL_EPON_SPEED_MODE_DS_1G_US_1G))
            || (status.pcs_sync_1g && (status.speed <= AAL_EPON_SPEED_MODE_DS_1G_US_1G)))
        {
            /* PCS is synchronized and speed is ok, do nothing */
            epon_link_fail_times = 0;
        }
        else /* PCS isn't synchronized */
        {
            if (status.sig_loss)
            {
                /* no laser signal is detected and therefore do nothing */
                epon_link_fail_times = 0;
            }
            else
            {
                /* need to reset SerDes when both pcs isn't synchronized and there is laser singal */
                epon_link_fail_times++;
            }
        }
    }

    if (epon_link_fail_times >= __EPON_SERDES_FAIL_THRESHOLD)
    {
        epon_link_reset_times++;
        (void)aal_epon_reset(0);
        epon_link_fail_times = 0;
    }

}

ca_status_t aal_epon_init
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t mode
)
{
    ca_status_t ret = CA_E_OK;

aal_epon_csr_reset(device_id);
    aal_epon_datapath_csr_reset(device_id);

    aal_epon_mac_init(device_id, 0, mode);
    aal_epon_fec_init(device_id, 0, mode);

#ifndef CONFIG_CORTINA_BOARD_FPGA
    aal_psds_burst_enable(device_id, TRUE); // enable PON SerDES laser burst, it should be behind the PCS initialization, refer to bug 50430.
#endif

    aal_puc_init(device_id, mode);
    aal_pdc_init(device_id, mode);
#if 0//defined(CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN)
    aal_idp_init(device_id, mode);
#endif
    aal_mpcp_init(device_id, 0);

    aal_epon_crypto_init(device_id, 0, mode);

    /* enable MAC Rx & Tx */
    aal_pon_mac_enable_set(0, 0, TRUE, TRUE);

    /* enable laser Tx */
    aal_epon_laser_enable_set(0, 0, TRUE);

    /* initialize EPON interrupt */
    aal_epon_intr_init(device_id);

    /* check link status per second */
    ca_circle_timer_add(1000, aal_epon_link_monitor_handler, NULL);

    return ret;
}



ca_status_t aal_epon_init_debug_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t mode
)
{
    return aal_epon_init(device_id, port_id, mode);
}

ca_status_t aal_epon_init_debug_get
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *mode
)
{
    return CA_E_OK;
}


EXPORT_SYMBOL(aal_epon_lao_set);
EXPORT_SYMBOL(aal_epon_lao_get);
EXPORT_SYMBOL(aal_epon_laser_enable_set);
EXPORT_SYMBOL(aal_epon_laser_enable_get);
EXPORT_SYMBOL(aal_epon_laser_cfg_set);
EXPORT_SYMBOL(aal_epon_laser_cfg_get);
EXPORT_SYMBOL(aal_epon_status_get);
EXPORT_SYMBOL(aal_epon_opt_status_get);
EXPORT_SYMBOL(aal_epon_dying_gasp_set);
EXPORT_SYMBOL(aal_epon_dying_gasp_get);
EXPORT_SYMBOL(aal_epon_init);
EXPORT_SYMBOL(aal_epon_init_debug_set);
EXPORT_SYMBOL(aal_epon_init_debug_get);
EXPORT_SYMBOL(aal_epon_reset_all_set);
EXPORT_SYMBOL(aal_epon_reset_all_get);





