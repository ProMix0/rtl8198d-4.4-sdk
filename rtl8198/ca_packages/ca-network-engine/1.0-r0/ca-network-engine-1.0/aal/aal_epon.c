
#include "osal_common.h"
#include <soc/cortina/registers.h>
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
#include "ca-scfg/scfg.h"
#include "osal_timer.h"
#include "osal_spinlock.h"
#ifdef CONFIG_LUNA_G3_SERIES
#include "aal_l2_vlan.h"
aal_l2_vlan_fib_map_t tmp_epon_l2_vlan_fib_map[4096];
#endif

ca_int32_t  aal_epon_debug = 1;
ca_uint32_t epon_link_fail_times   = 0;         /* counter for epon link fail event */
ca_uint32_t epon_link_reset_times  = 0;
ca_uint8_t  epon_forward_bcmc_flag = CA_TRUE;   /******
                                                ** KTBMT requirement,
                                                ** if laser is turned off by oam, ONU should not forward BC and MC any longer
                                                */


#define __EPON_LASER_TX_DELAY_MAX            0x3f
#if defined(CONFIG_LUNA_G3_SERIES)
#define __EPON_SERDES_FAIL_THRESHOLD         30
#else
#define __EPON_SERDES_FAIL_THRESHOLD         3
#endif
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
    static ca_boolean_t scfg_read_once_flag = CA_FALSE;
    ca_uint32_t rtn = CA_E_ERROR;

    if (!scfg_read_once_flag)
    {
        rtn = scfg_read(0, CFG_ID_PON_DS_FORWARD_BCMC, sizeof(epon_forward_bcmc_flag), &epon_forward_bcmc_flag);
        if (rtn != CA_E_OK)
            epon_forward_bcmc_flag = CA_TRUE;

        scfg_read_once_flag = CA_TRUE;
    }

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
                if (!epon_forward_bcmc_flag)
                    AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_RX_RXCNTRL, descram_rx_dis, 0);

                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, dis_opt_power, 0);
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, laser_always_on, 0);
                break;
            case AAL_EPON_LASER_MODE_ON:
                if (!epon_forward_bcmc_flag)
                    AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_RX_RXCNTRL, descram_rx_dis, 0);

                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, dis_opt_power, 0);
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, laser_always_on, 1);
                break;
            case AAL_EPON_LASER_MODE_OFF:
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_MISC_CTRL, dis_opt_power, 1);
                AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_STATIC_CFG, laser_always_on, 0);

                if (!epon_forward_bcmc_flag)
                    AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_RX_RXCNTRL, descram_rx_dis, 1);
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
    CA_IN  ca_device_id_t       device_id,
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
ca_status_t aal_epon_dying_gasp_get
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_OUT ca_uint8_t        *send_count,
    CA_OUT ca_uint8          *pkt_buff,
    CA_OUT ca_int32          *pkt_len
)
{
  	ca_device_config_tlv_t scfg;
    ca_status_t ret;
    ca_uint32_t i = 0;

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
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        send_count,
    CA_IN ca_uint8          *pkt_buff,
    CA_IN ca_int32          pkt_len
)
{
    ca_pkt_t *pkt = (ca_pkt_t *)dying_gasp_pkt_buf;
    ca_pkt_block_t  *pkt_data;
    ca_int32_t pkt_hdr_size, pkt_blk_hdr_size;
    ca_port_id_t lpid;

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
    pkt->dst_sub_port_id = 0;   /* llid */
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
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    GLOBAL_PON_CNTL_t pon_cfg;
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
    pon_cfg.wrd = 0;
    pon_cfg.bf.puc_reset = 1;
    pon_cfg.bf.pdc_reset = 1;
    pon_cfg.bf.ptp_rst_n = 1;
    pon_cfg.bf.psds_reg_rst_n   = 1;
    pon_cfg.bf.pon_serdes_rst_n = 1;
    pon_cfg.bf.epon_mode        = 1;    /* EPON mode */
    AAL_GLB_REG_WRITE(pon_cfg.wrd, GLOBAL_PON_CNTL);

    ca_mdelay(100);
#endif

    return CA_E_OK;
}

#ifdef CONFIG_LUNA_G3_SERIES
ca_status_t aal_epon_backup_ponmac(ca_device_id_t device_id)
{
    ca_uint32_t vlan_idx;
    aal_l2_vlan_fib_map_mask_t l2_vlan_fib_map_mask;
    aal_l2_vlan_fib_map_t l2_vlan_fib_map;

    for(vlan_idx=0;vlan_idx<4096;vlan_idx++)
    {
        aal_l2_vlan_fib_map_get(1,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_idx,&tmp_epon_l2_vlan_fib_map[vlan_idx]);

        l2_vlan_fib_map_mask.u32 = 0;
        l2_vlan_fib_map_mask.s.valid = 1;
        l2_vlan_fib_map.valid = 0;
        aal_l2_vlan_fib_map_set(1,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_idx,l2_vlan_fib_map_mask,&l2_vlan_fib_map);

        if(tmp_epon_l2_vlan_fib_map[vlan_idx].valid != 1)
            continue;

//        EPON_DEBUG("vlan_idx=%d valid=%d fib_id=%d\n",vlan_idx
//            ,tmp_epon_l2_vlan_fib_map[vlan_idx].valid
//            ,tmp_epon_l2_vlan_fib_map[vlan_idx].fib_id);
    }

    return CA_E_OK;
}

ca_status_t aal_epon_restore_ponmac(ca_device_id_t device_id)
{
    ca_uint32_t vlan_idx;
    aal_l2_vlan_fib_map_mask_t l2_vlan_fib_map_mask;
    aal_l2_vlan_fib_map_t l2_vlan_fib_map;

    for(vlan_idx=0;vlan_idx<4096;vlan_idx++)
    {
        if(tmp_epon_l2_vlan_fib_map[vlan_idx].valid != 1)
            continue;

//        EPON_DEBUG("vlan_idx=%d valid=%d fib_id=%d\n",vlan_idx
//            ,tmp_epon_l2_vlan_fib_map[vlan_idx].valid
//            ,tmp_epon_l2_vlan_fib_map[vlan_idx].fib_id);

        l2_vlan_fib_map_mask.u32 = 0;
        l2_vlan_fib_map_mask.s.valid = 1;
        l2_vlan_fib_map.valid = tmp_epon_l2_vlan_fib_map[vlan_idx].valid;
        aal_l2_vlan_fib_map_set(1,AAL_L2_VLAN_TYPE_LAN_VLAN,vlan_idx,l2_vlan_fib_map_mask,&l2_vlan_fib_map);
    }

    return CA_E_OK;
}
#endif

/* Reset SerDes */
ca_status_t aal_epon_reset(ca_device_id_t device_id)
{
    EPON_EPON_GLB_INTERRUPTE_t epon_glb;
    ca_uint32_t i = 0;
    ca_uint32_t intr_en[AAL_EPON_INTR_INDEX_MAX+1] = {0};

    ca_printf("ca-epon: epon link-fail was detected for (%d) times - attempt to reset SEDRES again.\n", epon_link_reset_times);

    __PON_LOCK();

#ifdef CONFIG_LUNA_G3_SERIES
    aal_epon_backup_ponmac(device_id);
#endif

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

#ifdef CONFIG_LUNA_G3_SERIES
    aal_epon_restore_ponmac(device_id);
#endif

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
    aal_psds_status_t status;

    aal_psds_status_get(device_id, &status);
    if (status.ckrdy_mpcp != 1 || status.ckrdy_rx != 1 || status.ckrdy_tx != 1)
    {
        ca_printf("EPON: serdes clock is not ready. ckrdy_mpcp = %d, ckrdy_rx = %d, ckrdy_tx = %d\n",
            status.ckrdy_mpcp, status.ckrdy_rx, status.ckrdy_tx);
        return CA_E_ERROR;
    }

    aal_epon_csr_reset(device_id);
    aal_epon_datapath_csr_reset(device_id);

    aal_epon_mac_init(device_id, 0, mode);
    aal_epon_fec_init(device_id, 0, mode);

    aal_psds_burst_enable(device_id, TRUE); // enable PON SerDES laser burst, it should be behind the PCS initialization, refer to bug 50430.

    aal_puc_init(device_id, mode);
    aal_pdc_init(device_id, mode);
#if defined(CONFIG_CORTINA_BOARD_FPGA)
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







