#include "ca-scfg/scfg.h"
#include "osal_common.h"
#include "aal_pon.h"
#include <soc/cortina/registers.h>
#include "aal_epon_crypto.h"
#include "aal_mpcp.h"
#include "ca_event.h"
#include "port.h"
/*Init values*/
#define __AAL_ENCRYPTION_BASE_TIMER                 0x5
#define __AAL_ENCRYPTION_RETRY_TIMER                0x8
#define __AAL_ENCRYPTION_PASUE_DLY                  0x3
#define __AAL_ENCRYPTION_BASE_TIMER_MAX             0x7F
#define __AAL_ENCRYPTION_BASE_TIMER_MIN             0x2
#define __AAL_ENCRYPTION_XG_BASE_TIMER_MIN          0x1
#define __AAL_ENCRYPTION_RETRY_TIMER_MAX            0x7F
#define __AAL_ENCRYPTION_PASUE_DLY_MAX              0xFFF
#define __AAL_ENCRYPTION_CYCLE_TIMER_MAX            0x1FFFF
#define __AAL_ENCRYPTION_CYCLE_TIMER_MIN            0x2
#define __AAL_ENCRYPTION_XG_CYCLE_TIMER_MIN         0xc /* 3 + 1 + 8 = 12 */
#define __AAL_ENCRYPTION_REKEYING_TIMER             10000      /*10s*/
#define __AAL_ENCRYPTION_CHN_OAM_OUI                0x111111
#define __AAL_ENCRYPTION_CHN_OAM_OUI_MAX            0xFFFFFF
#define __AAL_ENCRYPTION_OLT_CFG_FLAG               0x50
#define __AAL_ENCRYPTION_KEY_INDEX_MAX              1
#define __AAL_ENCRYPTION_AES_CLK_COMP_VALUE         12
#define __AAL_ENCRYPTION_TBL_ACCESS_RETRY_MAX       1000
#define __AAL_ENCRYPTION_DEBUG_INDEX_MAX            0x1FF
#define __AAL_ENCRYPTION_XG_TBL_AN_MAX              0x1
#define __AAL_ENCRYPTION_XG_TBL_SC_MAX              0x1
#define __AAL_ENCRYPTION_XG_TBL_SCB_MAX             0x1
#define __AAL_ENCRYPTION_XG_TBL_ES_MAX              0x1
#define __AAL_ENCRYPTION_XG_TBL_CO_MAX              0x3F
#define __AAL_ENCRYPTION_XG_TBL_RLYEN_MAX           0x1
#define __AAL_ENCRYPTION_XG_TBL_SEC_MODE_MAX        0x3
#define __AAL_ENCRYPTION_XG_TBL_V_CONFIG_MAX        0x3

#define __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET       0x00000140

#define AAL_EPON_CRYPTO_DIRECTION_CHECK(direction) \
    if((direction < AAL_EPON_CRYPTO_DIRECTION_RX) || (direction > AAL_EPON_CRYPTO_DIRECTION_MAX)){  \
        return CA_E_PARAM; \
    }

/* mpcp trace level */
typedef enum
{
    AAL_EPON_CRYPTO_TRACE_LOG = 0x1,
    AAL_EPON_CRYPTO_TRACE_MIN = 0x2,
    AAL_EPON_CRYPTO_TRACE_MAJ = 0x4,
    AAL_EPON_CRYPTO_TRACE_CRI = 0x8,
    AAL_EPON_CRYPTO_TRACE_ALL = 0xf
}aal_epon_crypto_trace_level_t;

/* Private structure */

aal_epon_cryp_macsec_misc_cfg_t epon_cryp_macsec_misc_cfg;

typedef struct{
    ca_uint32_t debug_level;
    //ca_uint32_t pon_mode;
    aal_epon_crypto_1g_mode_t rx_1g_mode;
    aal_epon_crypto_1g_mode_t tx_1g_mode;
    ca_boolean_t              rx_1g_ena;
    ca_boolean_t              tx_1g_ena;
    
    aal_epon_crypto_xg_mode_t rx_xg_mode;
    aal_epon_crypto_xg_mode_t tx_xg_mode;
    ca_boolean_t              rx_xg_ena;
    ca_boolean_t              tx_xg_ena;
}aal_epon_crypto_mgmt_t;

aal_epon_crypto_mgmt_t __crypto_mgmt;

#define __EPON_CRYPTO_LOG(level, args...) do { if (__crypto_mgmt.debug_level & level) ca_printf(args); }while(0)

ca_status_t aal_epon_crypto_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_uint32_t    level)
{
    __crypto_mgmt.debug_level = level;

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_debug_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_port_id_t   port_id,
    CA_OUT ca_uint32_t    *level)
{
    AAL_ASSERT_RET(level, CA_E_PARAM);

    *level = __crypto_mgmt.debug_level;

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_xg_mode_get(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_port_id_t                port_id,
    CA_OUT aal_epon_crypto_xg_mode_t  *tx_mode,
    CA_OUT aal_epon_crypto_xg_mode_t  *rx_mode)
{
    ca_status_t ret = CA_E_OK;
    EPON_EPON_GLB_CORESEC10_CFG_t glb_cfg;
    EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t tx_cfg;

    CA_DEV_ID_CHECK(device_id);
    //CA_PORT_EPON_CHECK(port_id);
    CA_NULL_PTR_CHECK(tx_mode);
    CA_NULL_PTR_CHECK(rx_mode);

    glb_cfg.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_CORESEC10_CFG);

    *tx_mode = glb_cfg.bf.tx_algo_sel;
    *rx_mode = glb_cfg.bf.algo_sel;

    return CA_E_OK;
}

/********************* Following APIs are for 1G EPON crypto **********************/
ca_status_t aal_epon_crypto_1g_cfg_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN aal_epon_crypto_cfg_msk_t msk,
    CA_IN aal_epon_crypto_cfg_t *cfg)
{
    EPON_ONU_EPON_AES_MODE_CTL_t glb_cfg;
    EPON_ONU_EPON_AES_CHURNING_CTRL_t ctrl;
    EPON_ONU_EPON_DATAPATH_BYPASS_t bypass;
    EPON_ONU_EPON_CHURN_CONFIG_CONTROL_t chrunCtr;

    if (NULL == cfg)
        return CA_E_ERROR;

    glb_cfg.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_MODE_CTL);
    ctrl.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_CHURNING_CTRL);
    
    /*for ctc churning key*/
    chrunCtr.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_CHURN_CONFIG_CONTROL);

    /* rxpath_bypass[3:0]    
    bit 3: bypass preamble compensation;
    bit 2: bypass DEC;
    bit 1: bypass RxFEC;
    bit 0: not used */
    bypass.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_DATAPATH_BYPASS);  

    /* For Churning and DPOE mode, support decryption only, see EPON_ONU_EPON_AES_MODE_CTL */
    if (msk.s.tx_mode) {

    }

    if (msk.s.rx_mode) {
        if(cfg->rx_mode == AAL_EPON_CRYPTO_1G_MODE_DISABLE){
            ctrl.bf.enable_decryption_function_churning_or_AES = 0;
            bypass.bf.rxpath_bypass |= (1<<2);
        }else
        {
            glb_cfg.bf.AesModeSel = cfg->rx_mode;
            ctrl.bf.enable_decryption_function_churning_or_AES = 1;
            bypass.bf.rxpath_bypass &= ~(1<<2);
        }
    }

    if (msk.s.key_exchange_mode) {
        ctrl.bf.HW_key_mode = cfg->key_exchange_mode;
        if(cfg->key_exchange_mode == AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW)
            chrunCtr.bf.select_sw_key_as_decryption_key = 1;
        else
            chrunCtr.bf.select_sw_key_as_decryption_key = 0;
    }
    AAL_PON_REG_WRITE(glb_cfg.wrd, EPON_ONU_EPON_AES_MODE_CTL);
    AAL_PON_REG_WRITE(ctrl.wrd, EPON_ONU_EPON_AES_CHURNING_CTRL);
    AAL_PON_REG_WRITE(bypass.wrd, EPON_ONU_EPON_DATAPATH_BYPASS);
    AAL_PON_REG_WRITE(chrunCtr.wrd, EPON_ONU_EPON_CHURN_CONFIG_CONTROL);


    __crypto_mgmt.rx_1g_mode = cfg->rx_mode;
    __crypto_mgmt.tx_1g_mode = cfg->tx_mode;

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_1g_cfg_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_OUT aal_epon_crypto_cfg_t *cfg)
{
    EPON_ONU_EPON_AES_MODE_CTL_t glb_cfg;
    EPON_ONU_EPON_AES_CHURNING_CTRL_t ctrl;

    if (NULL == cfg)
        return CA_E_ERROR;

    glb_cfg.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_MODE_CTL);
    ctrl.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_CHURNING_CTRL);

    cfg->tx_mode = __crypto_mgmt.tx_1g_mode;
    cfg->rx_mode              = glb_cfg.bf.AesModeSel;
    cfg->key_exchange_mode = ctrl.bf.HW_key_mode;
    if(ctrl.bf.enable_decryption_function_churning_or_AES == 0){
        cfg->rx_mode = AAL_EPON_CRYPTO_1G_MODE_DISABLE;
    }

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_1g_llid_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid_idx,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_IN aal_epon_crypto_llid_t *llid
)
{
    EPON_ONU_EPON_AES_DEC_LLID_INDEX_0_t llid_entry;
    ca_uint8_t key_tbl_idx;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(llid, CA_E_PARAM);

    if(direction != AAL_EPON_CRYPTO_DIRECTION_RX ){ /* LLID table is used for decryption only */
        return CA_E_PARAM;
    }
    memset(&llid_entry, 0, sizeof(EPON_ONU_EPON_AES_DEC_LLID_INDEX_0_t));
    key_tbl_idx = (llid_idx & 0x7)*2; /* only support 8 instance */
    
    if(llid->ena)
    {
        llid_entry.bf.entry |= (1<<19); /* bit 19: entry valid indication. 1 is valid. */
    }else{
        llid_entry.bf.entry &= ~(1<<19);
    }
    llid_entry.bf.entry |= (key_tbl_idx<<16);   /* bit 18~16: llid key table index. Valid when bit 19 is 1. */
    llid_entry.bf.entry |= (llid->llid) & 0xffff;   /* bit 15~0: llid.  Valid when bit 19 is 1. */
    AAL_PON_REG_WRITE(llid_entry.wrd, EPON_ONU_EPON_AES_DEC_LLID_INDEX_0 + llid_idx * 4);

    return CA_E_OK;

}

ca_status_t aal_epon_crypto_1g_llid_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid_idx,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_OUT aal_epon_crypto_llid_t  *llid
)
{
    EPON_ONU_EPON_AES_DEC_LLID_INDEX_0_t llid_entry;

    if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
    llid_entry.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DEC_LLID_INDEX_0 + llid_idx * 4);
        if(llid_entry.bf.entry & (1<<19)){
        llid->ena = 1;
        }else{
            llid->ena = 0;
        }
        llid->llid = llid_entry.bf.entry & 0xffff;
    }else{
        memset(llid, 0, sizeof(aal_epon_crypto_llid_t));
    }
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_1g_llid_enable_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid_idx,
    CA_IN ca_uint32_t                   llid_value,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_boolean_t                  enable)
{
    ca_status_t ret;
    aal_epon_crypto_llid_t llid_cfg;

    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);

    memset(&llid_cfg, 0, sizeof(aal_epon_crypto_llid_t));
    llid_cfg.ena = enable;
    llid_cfg.llid = llid_value;
    ret = aal_epon_crypto_1g_llid_set(device_id, port_id, llid_idx, direction, &llid_cfg);   

    return CA_E_OK;
    }

ca_status_t aal_epon_crypto_1g_llid_enable_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid_idx,
    CA_IN ca_uint32_t                   llid_value,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_OUT ca_boolean_t                 *enable)
{
    ca_status_t ret;
    aal_epon_crypto_llid_t llid_cfg;
    *enable = CA_FALSE;
    
    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);

    memset(&llid_cfg, 0, sizeof(aal_epon_crypto_llid_t));
    ret = aal_epon_crypto_1g_llid_get(device_id, port_id, llid_idx, direction, &llid_cfg);
    CA_RET_VALUE_CHECK(ret);
    *enable = llid_cfg.ena;
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_1g_enable_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_boolean_t                  enable)
{
    ca_status_t ret;
    ca_uint32_t i = 0;
    ca_uint16_t llid_value;
    ca_uint8_t llid_num;
    ca_boolean_t reg_flag = CA_TRUE;

    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);
    
    llid_num = aal_mpcp_llid_num_get(0);
    for(i = 0; i < llid_num; i++)
    {
        aal_mpcp_reg_status_get(device_id, port_id, i, &reg_flag);
        if(reg_flag) /* encryption/de-cryption can only be enabled when LLID registered */
        {
            ret = aal_mpcp_reg_llid_get(device_id, port_id, i, &llid_value);
            CA_RET_VALUE_CHECK(ret);
            __EPON_CRYPTO_LOG(AAL_EPON_CRYPTO_TRACE_MAJ, "start to %s llid_idx %d, llid_value %d %s\r\n", (enable)?"enable":"disable", i, 
                llid_value, (direction == AAL_EPON_CRYPTO_DIRECTION_RX)?"decryption":"encryption");
            ret = aal_epon_crypto_1g_llid_enable_set(0, 0, i, llid_value, direction, enable); 
        }
    }
    if(direction == AAL_EPON_CRYPTO_DIRECTION_RX)
        __crypto_mgmt.rx_1g_ena = enable;
    else
        __crypto_mgmt.tx_1g_ena = enable;
    
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_1g_enable_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_OUT ca_boolean_t                 *enable)
{
    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        *enable = __crypto_mgmt.rx_1g_ena;
        
    }else{
        *enable = __crypto_mgmt.tx_1g_ena;
    }
    
    return CA_E_OK;
}

/* 32*4=128bit key */
ca_status_t aal_epon_crypto_1g_key_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_uint8_t                    llid_idx,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_uint32_t                   key_id,
    CA_IN aal_epon_crypto_key_t         *key_data)
{
    ca_status_t ret;
    ca_uint32_t addr;
    ca_uint32_t mode;
    
    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);

    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_AES_MODE_CTL, AesModeSel, mode);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_RX)
    {
        if(mode == AAL_EPON_CRYPTO_1G_MODE_CHURNING)
        {
            /* 1G churning decryption */
            if(key_id == 0){
                AAL_PON_REG_WRITE(key_data->key[3], EPON_ONU_EPON_UC_KEY03);
                AAL_PON_REG_WRITE(key_data->key[2], EPON_ONU_EPON_UC_KEY02);
                AAL_PON_REG_WRITE(key_data->key[1], EPON_ONU_EPON_UC_KEY01);
                AAL_PON_REG_WRITE(key_data->key[0], EPON_ONU_EPON_UC_KEY00);
                
            }else if(key_id == 1){
                AAL_PON_REG_WRITE(key_data->key[3], EPON_ONU_EPON_UC_KEY13);
                AAL_PON_REG_WRITE(key_data->key[2], EPON_ONU_EPON_UC_KEY12);
                AAL_PON_REG_WRITE(key_data->key[1], EPON_ONU_EPON_UC_KEY11);
                AAL_PON_REG_WRITE(key_data->key[0], EPON_ONU_EPON_UC_KEY10);  
            }
        }else if(mode == AAL_EPON_CRYPTO_1G_MODE_AES_DPOE)
        {
            /* multiple LLID DPoE decryption */
            addr = (llid_idx<<1)|key_id;
            AAL_PON_REG_INDIRECT_READ(addr, EPON_ONU_EPON_AES_DEC_KEY_TBL_ACCESS);

            AAL_PON_REG_WRITE(key_data->key[3], EPON_ONU_EPON_AES_DEC_KEY_TBL_DATA3);
            AAL_PON_REG_WRITE(key_data->key[2], EPON_ONU_EPON_AES_DEC_KEY_TBL_DATA2);
            AAL_PON_REG_WRITE(key_data->key[1], EPON_ONU_EPON_AES_DEC_KEY_TBL_DATA1);
            AAL_PON_REG_WRITE(key_data->key[0], EPON_ONU_EPON_AES_DEC_KEY_TBL_DATA0);
            AAL_PON_REG_INDIRECT_WRITE(addr, EPON_ONU_EPON_AES_DEC_KEY_TBL_ACCESS);
        }

    }else
    {
        // not support yet
    }
   
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_1g_key_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_uint8_t                    llid_idx,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_uint32_t                   key_id,
    CA_OUT aal_epon_crypto_key_t        *key_data)
{
    ca_status_t ret;
    ca_uint32_t addr;
    ca_uint32_t mode;
    
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_AES_MODE_CTL, AesModeSel, mode);
    
    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_RX)
    {
        if(mode == AAL_EPON_CRYPTO_1G_MODE_CHURNING)
        {
            /* 1G churning decryption */
            if(key_id == 0){
                key_data->key[3] = AAL_PON_REG_READ(EPON_ONU_EPON_UC_KEY03);
                key_data->key[2] = AAL_PON_REG_READ(EPON_ONU_EPON_UC_KEY02);
                key_data->key[1] = AAL_PON_REG_READ(EPON_ONU_EPON_UC_KEY01);
                key_data->key[0] = AAL_PON_REG_READ(EPON_ONU_EPON_UC_KEY00);
                
            }else if(key_id == 1){
                key_data->key[3] = AAL_PON_REG_READ(EPON_ONU_EPON_UC_KEY13);
                key_data->key[2] = AAL_PON_REG_READ(EPON_ONU_EPON_UC_KEY12);
                key_data->key[1] = AAL_PON_REG_READ(EPON_ONU_EPON_UC_KEY11);
                key_data->key[0] = AAL_PON_REG_READ(EPON_ONU_EPON_UC_KEY10);  
            }
        }

        else if(mode == AAL_EPON_CRYPTO_1G_MODE_AES_DPOE)
        {

            addr = (llid_idx<<1)|key_id;
            AAL_PON_REG_INDIRECT_READ(addr, EPON_ONU_EPON_AES_DEC_KEY_TBL_ACCESS);
            key_data->key[3] = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DEC_KEY_TBL_DATA3);
            key_data->key[2] = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DEC_KEY_TBL_DATA2);
            key_data->key[1] = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DEC_KEY_TBL_DATA1);
            key_data->key[0] = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DEC_KEY_TBL_DATA0);
        }      
    }else
    {
        // not support yet
    }
    
    return CA_E_OK;    
}


ca_status_t aal_epon_crypto_1g_stats_get(
    CA_IN ca_device_id_t        device_id,
    CA_IN ca_port_id_t          port_id,
    CA_IN ca_uint8_t            llid_idx,
    CA_OUT aal_epon_crypto_1g_stats_t *stats)
{
    EPON_ONU_EPON_AES_DBG_CNT0_t cnt0;
    EPON_ONU_EPON_AES_DBG_GOOD_CNT0_t good_cnt0;
    EPON_ONU_EPON_AES_DBG_CNT1_t cnt1;
    EPON_ONU_EPON_AES_DBG_GOOD_CNT1_t good_cnt1;

    cnt0.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DBG_CNT0);
    good_cnt0.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DBG_GOOD_CNT0);
    cnt1.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DBG_CNT1);
    good_cnt1.wrd = AAL_PON_REG_READ(EPON_ONU_EPON_AES_DBG_GOOD_CNT1);

    stats->rx_pkt_encrypted_crcerr_0 = cnt0.wrd;
    stats->rx_pkt_encrypted_good_0 = good_cnt0.wrd;
    stats->rx_pkt_encrypted_crcerr_1 = cnt1.wrd;
    stats->rx_pkt_encrypted_good_1 = good_cnt1.wrd;

    return CA_E_OK;
}

/********************* Following APIs are for 10G EPON crypto **********************/
ca_status_t aal_epon_crypto_xg_cfg_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN aal_epon_crypto_cfg_msk_t msk,
    CA_IN aal_epon_crypto_cfg_t *cfg)
{
    EPON_EPON_GLB_CORESEC10_CFG_t glb_cfg;
    EPON_XGEPN_XMAC_CTRL_XGEPN_RX_CTL_t rx_ctrl;
    EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t tx_cfg, rx_cfg;
	
    if (NULL == cfg)
        return CA_E_ERROR;

    if (msk.s.tx_mode){
        if (cfg->tx_mode > AAL_EPON_CRYPTO_XG_MODE_MAX)
            return CA_E_ERROR;
    }
    if (msk.s.rx_mode){
        if (cfg->rx_mode > AAL_EPON_CRYPTO_XG_MODE_MAX)
            return CA_E_ERROR;
    }

    glb_cfg.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_CORESEC10_CFG);
    rx_ctrl.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_CTRL_XGEPN_RX_CTL);
    tx_cfg.wrd = AAL_PON_REG_READ((EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET ));
    rx_cfg.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG);

    if (msk.s.tx_mode) {
        glb_cfg.bf.tx_algo_sel = cfg->tx_mode;
    }
    if (msk.s.rx_mode) {
        glb_cfg.bf.algo_sel = cfg->rx_mode;
    }
	if (msk.s.tx_ivsa_sel) {
        glb_cfg.bf.tx_IvSa_sel = cfg->tx_ivsa_sel;
    }

    if (msk.s.key_exchange_mode) {
        tx_cfg.bf.key_exhange_mode = cfg->key_exchange_mode;
        rx_cfg.bf.key_exhange_mode = cfg->key_exchange_mode;
        rx_ctrl.bf.frame_keyex_oam_drop_en = cfg->key_exchange_mode; /* For HW key exchange, drop enable */
    }
    AAL_PON_REG_WRITE(glb_cfg.wrd, EPON_EPON_GLB_CORESEC10_CFG);
    AAL_PON_REG_WRITE(rx_ctrl.wrd, EPON_XGEPN_XMAC_CTRL_XGEPN_RX_CTL);
    AAL_PON_REG_WRITE(tx_cfg.wrd, (EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET ));
    AAL_PON_REG_WRITE(rx_cfg.wrd, EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG);

    __crypto_mgmt.rx_xg_mode = cfg->rx_mode;
    __crypto_mgmt.tx_xg_mode = cfg->tx_mode;
    
	return CA_E_OK;
}

ca_status_t aal_epon_crypto_xg_cfg_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_OUT aal_epon_crypto_cfg_t *cfg)
{
    EPON_EPON_GLB_CORESEC10_CFG_t glb_cfg;
    EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t tx_cfg;

    if (NULL == cfg)
        return CA_E_ERROR;

    glb_cfg.wrd = AAL_PON_REG_READ(EPON_EPON_GLB_CORESEC10_CFG);
    tx_cfg.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG);

    cfg->tx_mode = glb_cfg.bf.tx_algo_sel;
    cfg->rx_mode = glb_cfg.bf.algo_sel;
    cfg->key_exchange_mode = tx_cfg.bf.key_exhange_mode;
	cfg->tx_ivsa_sel = glb_cfg.bf.tx_IvSa_sel;
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_coresec_glb_cfg_set(
	CA_IN ca_device_id_t	   device_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_IN aal_epon_coresec_glb_cfg_msk_t msk,
	CA_IN aal_epon_coresec_glb_cfg_t *cfg)
{
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t coresec_glb_cfg;
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_t no_drop_eth_type;
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1_t no_drop_eth_type1;
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2_t no_drop_eth_type2;
	ca_uint32_t coresec_glb_addr, no_drop_eth_type_addr,no_drop_eth_type1_addr,no_drop_eth_type2_addr;
	

	if (NULL == cfg)
		return CA_E_ERROR;

	if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
		coresec_glb_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		no_drop_eth_type_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		no_drop_eth_type1_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		no_drop_eth_type2_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
	}else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
		coresec_glb_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG ;
		no_drop_eth_type_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE ;
		no_drop_eth_type1_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1 ;
		no_drop_eth_type2_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2 ;
	}else{
        return CA_E_PARAM;
	}
	coresec_glb_cfg.wrd = AAL_PON_REG_READ(coresec_glb_addr);
	no_drop_eth_type.wrd = AAL_PON_REG_READ(no_drop_eth_type_addr);
	no_drop_eth_type1.wrd = AAL_PON_REG_READ(no_drop_eth_type1_addr);
	no_drop_eth_type2.wrd = AAL_PON_REG_READ(no_drop_eth_type2_addr);

	if (msk.s.dis_ver0_macsec_err) {
		coresec_glb_cfg.bf.dis_ver0_macsec_err = cfg->dis_ver0_macsec_err;
	}
	if (msk.s.debug_bus_sel) {
		coresec_glb_cfg.bf.debug_bus_sel = cfg->debug_bus_sel;
	}
	if (msk.s.drop_untag_pkt_mode) {
        coresec_glb_cfg.bf.drop_untag_pkt_mode = cfg->drop_untag_pkt_mode;
	}
	if (msk.s.en_co_error) {
        coresec_glb_cfg.bf.en_co_error = cfg->en_co_error;
	}
	if (msk.s.soft_reset) {
        coresec_glb_cfg.bf.soft_reset = cfg->soft_reset;
	}
	if (msk.s.cfg_mask_dec_err) {
        coresec_glb_cfg.bf.cfg_mask_dec_err = cfg->cfg_mask_dec_err;
	}
	if (msk.s.cfg_dont_modify_crc32) {
        coresec_glb_cfg.bf.cfg_dont_modify_crc32 = cfg->cfg_dont_modify_crc32;
	}
	if (msk.s.dis_crc32_chk) {
        coresec_glb_cfg.bf.dis_crc32_chk = cfg->dis_crc32_chk;
	}
	if (msk.s.dis_preamble_chk) {
        coresec_glb_cfg.bf.dis_preamble_chk = cfg->dis_preamble_chk;
	}
	if (msk.s.dis_drop_macsec_err_ptk) {
        coresec_glb_cfg.bf.dis_drop_macsec_err_ptk = cfg->dis_drop_macsec_err_ptk;
	}
	
	if (msk.s.key_exchange_mode) {
		coresec_glb_cfg.bf.key_exhange_mode = cfg->key_exchange_mode;
	}
	if (msk.s.disable_crc8_chk) {
        coresec_glb_cfg.bf.disable_crc8_chk = cfg->disable_crc8_chk;
	}

	if(msk.s.drop_8902_ena){
		no_drop_eth_type.bf.no_drop_8902 = (cfg->drop_8902_ena == 0);
	}

	if(msk.s.drop_8021x_ena){
		no_drop_eth_type.bf.no_drop_802_1x = (cfg->drop_8021x_ena == 0);
	}

	if(msk.s.drop_oam_ena){
		no_drop_eth_type.bf.no_drop_oam = (cfg->drop_oam_ena == 0);
	}

	if(msk.s.drop_mpcp_ena){
		no_drop_eth_type.bf.no_drop_mpcp = (cfg->drop_mpcp_ena == 0);
	}

	if(msk.s.drop_usrdef_ena){
		no_drop_eth_type.bf.no_drop_user_def = (cfg->drop_usrdef_ena == 0);
	}
	if(msk.s.drop_usrdef10_ena){
		no_drop_eth_type.bf.no_drop_user_def10 = (cfg->drop_usrdef10_ena == 0);
	}
	if(msk.s.drop_usrdef11_ena){
		no_drop_eth_type.bf.no_drop_user_def11 = (cfg->drop_usrdef11_ena == 0);
	}
	if(msk.s.drop_usrdef20_ena){
		no_drop_eth_type.bf.no_drop_user_def20 = (cfg->drop_usrdef20_ena == 0);
	}
	if(msk.s.drop_usrdef21_ena){
		no_drop_eth_type.bf.no_drop_user_def21 = (cfg->drop_usrdef21_ena == 0);
	}
	if(msk.s.drop_usrdef){
		no_drop_eth_type.bf.user_defined = cfg->drop_usrdef;
	}
	if(msk.s.drop_usrdef10){
		no_drop_eth_type1.bf.user_defined10 = cfg->drop_usrdef10;
	}
	if(msk.s.drop_usrdef11){
		no_drop_eth_type1.bf.user_defined11 = cfg->drop_usrdef11;
	}
	if(msk.s.drop_usrdef20){
		no_drop_eth_type2.bf.user_defined20 = cfg->drop_usrdef20;
	}
	if(msk.s.drop_usrdef21){
		no_drop_eth_type2.bf.user_defined21 = cfg->drop_usrdef21;
	}
	
	AAL_PON_REG_WRITE(coresec_glb_cfg.wrd, coresec_glb_addr);
	AAL_PON_REG_WRITE(no_drop_eth_type.wrd, no_drop_eth_type_addr);
	AAL_PON_REG_WRITE(no_drop_eth_type1.wrd, no_drop_eth_type1_addr);
	AAL_PON_REG_WRITE(no_drop_eth_type2.wrd, no_drop_eth_type2_addr);
	return CA_E_OK;
}

ca_status_t aal_epon_crypto_coresec_glb_cfg_get(
	CA_IN ca_device_id_t	   device_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_OUT aal_epon_coresec_glb_cfg_t *cfg)
{
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t coresec_glb_cfg;
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_t no_drop_eth_type;
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1_t no_drop_eth_type1;
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2_t no_drop_eth_type2;
	ca_uint32_t coresec_glb_addr, no_drop_eth_type_addr,no_drop_eth_type1_addr,no_drop_eth_type2_addr;
	

	if (NULL == cfg)
		return CA_E_ERROR;

	if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
		coresec_glb_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		no_drop_eth_type_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		no_drop_eth_type1_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		no_drop_eth_type2_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
	}else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
		coresec_glb_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG ;
		no_drop_eth_type_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE ;
		no_drop_eth_type1_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1 ;
		no_drop_eth_type2_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2 ;
	}else{
        return CA_E_PARAM;
	}
	coresec_glb_cfg.wrd = AAL_PON_REG_READ(coresec_glb_addr);
	no_drop_eth_type.wrd = AAL_PON_REG_READ(no_drop_eth_type_addr);
	no_drop_eth_type1.wrd = AAL_PON_REG_READ(no_drop_eth_type1_addr);
	no_drop_eth_type2.wrd = AAL_PON_REG_READ(no_drop_eth_type2_addr);
	
	cfg->dis_ver0_macsec_err = coresec_glb_cfg.bf.dis_ver0_macsec_err;
	cfg->debug_bus_sel 		 = coresec_glb_cfg.bf.debug_bus_sel;
	cfg->drop_untag_pkt_mode = coresec_glb_cfg.bf.drop_untag_pkt_mode;
	cfg->en_co_error 		 = coresec_glb_cfg.bf.en_co_error;
	cfg->soft_reset          = coresec_glb_cfg.bf.soft_reset;
	cfg->cfg_mask_dec_err 	 = coresec_glb_cfg.bf.cfg_mask_dec_err;
	cfg->cfg_dont_modify_crc32 = coresec_glb_cfg.bf.cfg_dont_modify_crc32;
	cfg->dis_crc32_chk 		 = coresec_glb_cfg.bf.dis_crc32_chk;
	cfg->dis_preamble_chk 	 = coresec_glb_cfg.bf.dis_preamble_chk;
	cfg->dis_drop_macsec_err_ptk = coresec_glb_cfg.bf.dis_drop_macsec_err_ptk;
	cfg->key_exchange_mode   = coresec_glb_cfg.bf.key_exhange_mode;
	cfg->disable_crc8_chk    = coresec_glb_cfg.bf.disable_crc8_chk;
	cfg->disable_crc8_chk    = coresec_glb_cfg.bf.disable_crc8_chk;
	cfg->drop_8902_ena		 = 	(no_drop_eth_type.bf.no_drop_8902 ==0)	   		;
	cfg->drop_usrdef21_ena 	 =	(no_drop_eth_type.bf.no_drop_user_def21 ==0)	;
	cfg->drop_usrdef20_ena 	 =	(no_drop_eth_type.bf.no_drop_user_def20 ==0)	;
	cfg->drop_usrdef11_ena   =	(no_drop_eth_type.bf.no_drop_user_def11 ==0)	;
	cfg->drop_usrdef10_ena   =	(no_drop_eth_type.bf.no_drop_user_def11 ==0)	;
 	cfg->drop_usrdef_ena	 =	(no_drop_eth_type.bf.no_drop_user_def   ==0)	;
 	cfg->drop_8021x_ena 	 =	(no_drop_eth_type.bf.no_drop_802_1x     ==0)	;
	cfg->drop_mpcp_ena		 =	(no_drop_eth_type.bf.no_drop_mpcp       ==0)	;
	cfg->drop_oam_ena		 =	(no_drop_eth_type.bf.no_drop_oam       	==0)	;
	cfg->drop_usrdef		 =	 no_drop_eth_type.bf.user_defined 				;
	cfg->drop_usrdef10		 =	 no_drop_eth_type1.bf.user_defined10	   		;
	cfg->drop_usrdef11		 =	 no_drop_eth_type1.bf.user_defined11	   	   	;
	cfg->drop_usrdef20		 =	 no_drop_eth_type2.bf.user_defined20	        ;
	cfg->drop_usrdef21		 =	 no_drop_eth_type2.bf.user_defined21	        ;
	return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_enable_set(
    CA_IN ca_device_id_t        device_id,
    CA_IN ca_port_id_t          port_id,
    CA_IN ca_uint8_t            llid,
    CA_IN ca_boolean_t          enable)
{
    ca_status_t ret = CA_E_OK;
	EPON_XGEPN_XMAC_TXMAC_MACSEC_LLIDEN0_t sec_en;
    ca_uint8_t sectag_type = 0;
	sec_en.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_TXMAC_MACSEC_LLIDEN0);
    ret = aal_epon_crypto_macsec_sectag_type_get(device_id, port_id, llid, &sectag_type);
    CA_RET_VALUE_CHECK(ret);
	if(enable){
	    sec_en.bf.llid_sec_en |= (1<<(llid&0x1f));
        if(sectag_type){
            AAL_PON_REG_FIELD_WRITE(PUC_RPTCFG,rptpreamblovhd,(8+16+16));
            AAL_PON_REG_FIELD_WRITE(PUC_BTCCFG,pfovrhd,(8+12+16+16));
        }else{
            AAL_PON_REG_FIELD_WRITE(PUC_RPTCFG,rptpreamblovhd,(8+8+16));
            AAL_PON_REG_FIELD_WRITE(PUC_BTCCFG,pfovrhd,(8+12+8+16));
        }
    }else{
        sec_en.bf.llid_sec_en &= (~(1<<(llid&0x1f)));
        AAL_PON_REG_FIELD_WRITE(PUC_RPTCFG,rptpreamblovhd,8);
        AAL_PON_REG_FIELD_WRITE(PUC_BTCCFG,pfovrhd,(8+12));
    }
	AAL_PON_REG_WRITE(sec_en.wrd, EPON_XGEPN_XMAC_TXMAC_MACSEC_LLIDEN0);
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_enable_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid,
    CA_OUT ca_boolean_t     *enable)
{
    EPON_XGEPN_XMAC_TXMAC_MACSEC_LLIDEN0_t sec_en;

    sec_en.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_TXMAC_MACSEC_LLIDEN0);

    if(sec_en.bf.llid_sec_en & (1<<(llid&0x1f))){
        *enable = 1;
    }else{
        *enable = 0;
    }
    return CA_E_OK;
}
	
ca_status_t aal_epon_crypto_macsec_sectag_type_set(
	CA_IN ca_device_id_t	device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN ca_uint8_t		llid,
	CA_IN ca_uint8_t		type)
{
	EPON_XGEPN_XMAC_TXMAC_SECTAG_LLIDEN0_t sectag_type;

	sectag_type.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_TXMAC_SECTAG_LLIDEN0);
	if(type != 0){
		sectag_type.bf.sectag_type |= (1<<(llid&0x1f));
	}else{
		sectag_type.bf.sectag_type &= (~(1<<(llid&0x1f)));
	}
	AAL_PON_REG_WRITE(sectag_type.wrd, EPON_XGEPN_XMAC_TXMAC_SECTAG_LLIDEN0);
	return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_sectag_type_get(
	CA_IN ca_device_id_t	device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN ca_uint8_t		llid,
	CA_OUT ca_uint8_t 		*type)
{
	EPON_XGEPN_XMAC_TXMAC_SECTAG_LLIDEN0_t sectag_type;

	sectag_type.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_TXMAC_SECTAG_LLIDEN0);

	if(sectag_type.bf.sectag_type & (1<<(llid&0x1f))){
		*type = 1;
	}else{
		*type = 0;
	}
	return CA_E_OK;
}
    
ca_status_t aal_epon_crypto_xg_llid_set(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid_idx,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_IN aal_epon_crypto_llid_t *llid
)
{
    ca_status_t ret = CA_E_OK;
    EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA_t llid_prog_mem_data;
    ca_uint32_t data_addr, access_addr;
    aal_epon_crypto_macsec_said_t macsec_said;
    memset(&macsec_said, 0, sizeof(aal_epon_crypto_macsec_said_t));

    ret = aal_epon_crypto_macsec_said_get(device_id, port_id, llid_idx, direction, &macsec_said);
    CA_RET_VALUE_CHECK(ret);

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(llid, CA_E_PARAM);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
        access_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        data_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
    }else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        access_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS;
        data_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA;
    }else{
        return CA_E_PARAM;
    }

    AAL_PON_REG_INDIRECT_READ(llid_idx, access_addr);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
        
        if(__crypto_mgmt.tx_xg_mode == AAL_EPON_CRYPTO_XG_MODE_DISABLE){
            llid_prog_mem_data.bf.LLID_value = macsec_said.llid;
            llid_prog_mem_data.bf.SA_ID = macsec_said.sa_id;
            llid_prog_mem_data.bf.valid = 0;
        }else if(__crypto_mgmt.tx_xg_mode == AAL_EPON_CRYPTO_XG_MODE_MACSEC){
            llid_prog_mem_data.bf.LLID_value = macsec_said.llid;
            llid_prog_mem_data.bf.SA_ID = macsec_said.sa_id;
            llid_prog_mem_data.bf.valid = 1;
        }else{
            llid_prog_mem_data.bf.LLID_value = llid->llid;
            llid_prog_mem_data.bf.SA_ID = llid_idx;
            llid_prog_mem_data.bf.valid = (llid->ena!=0);
        }
    }else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        if(__crypto_mgmt.rx_xg_mode == AAL_EPON_CRYPTO_XG_MODE_DISABLE){
            llid_prog_mem_data.bf.LLID_value = macsec_said.llid;
            llid_prog_mem_data.bf.SA_ID = macsec_said.sa_id;
            llid_prog_mem_data.bf.valid = 0;
        }else if(__crypto_mgmt.rx_xg_mode == AAL_EPON_CRYPTO_XG_MODE_MACSEC){
            llid_prog_mem_data.bf.LLID_value = macsec_said.llid;
            llid_prog_mem_data.bf.SA_ID = macsec_said.sa_id;
            llid_prog_mem_data.bf.valid = 1;
        }else{
            llid_prog_mem_data.bf.LLID_value = llid->llid;
            llid_prog_mem_data.bf.SA_ID = llid_idx;
            llid_prog_mem_data.bf.valid = (llid->ena!=0);
        } 
    }else{
        return CA_E_PARAM;
    }
    AAL_PON_REG_WRITE(llid_prog_mem_data.wrd, data_addr);
    AAL_PON_REG_INDIRECT_WRITE(llid_idx, access_addr);

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_xg_llid_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid_idx,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_OUT aal_epon_crypto_llid_t  *llid
)
{
    EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA_t llid_prog_mem_data;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(llid, CA_E_PARAM);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
        AAL_PON_REG_INDIRECT_READ(llid_idx, (EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));
        llid_prog_mem_data.wrd = AAL_PON_REG_READ((EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));

    }else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        AAL_PON_REG_INDIRECT_READ(llid_idx, EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS);
        llid_prog_mem_data.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA);
    }else{
        return CA_E_PARAM;
    }

    llid->llid = llid_prog_mem_data.bf.LLID_value;
    llid->ena = llid_prog_mem_data.bf.valid;

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_said_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_uint8_t                    llid_idx,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN aal_epon_crypto_macsec_said_t *said
)
{
    EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA_t llid_prog_mem_data;
    ca_uint32_t data_addr, access_addr;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(llid, CA_E_PARAM);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
        access_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        data_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
    }else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        access_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS;
        data_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA;
    }else{
        return CA_E_PARAM;
    }

    AAL_PON_REG_INDIRECT_READ(llid_idx, access_addr);

    llid_prog_mem_data.bf.LLID_value = said->llid;
    llid_prog_mem_data.bf.SA_ID = said->sa_id;
    llid_prog_mem_data.bf.valid = said->valid;

    AAL_PON_REG_WRITE(llid_prog_mem_data.wrd, data_addr);
    AAL_PON_REG_INDIRECT_WRITE(llid_idx, access_addr);

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_said_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8_t        llid_idx,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_OUT aal_epon_crypto_macsec_said_t  *said
)
{
    EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA_t llid_prog_mem_data;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(llid, CA_E_PARAM);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
        AAL_PON_REG_INDIRECT_READ(llid_idx, (EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));
        llid_prog_mem_data.wrd = AAL_PON_REG_READ((EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));

    }else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        AAL_PON_REG_INDIRECT_READ(llid_idx, EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS);
        llid_prog_mem_data.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA);
    }else{
        return CA_E_PARAM;
    }
	
	said->sa_id = llid_prog_mem_data.bf.SA_ID;
	said->valid = llid_prog_mem_data.bf.valid;
    said->llid 	= llid_prog_mem_data.bf.LLID_value;
    

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_xg_sa_tbl_set(
    CA_IN ca_device_id_t                    device_id,
    CA_IN ca_port_id_t                      port_id,
    CA_IN ca_uint8_t                        said,
    CA_IN aal_epon_crypto_direction_t       direction,
    CA_IN aal_epon_crypto_xg_tbl_msk_t      msk,
    CA_IN aal_epon_crypto_xg_tbl_field_t    *cfg)
{
    aal_epon_crypto_xg_sadb_ctrl_t SADB_Control;
    EPON_XGEPN_CORESEC10_TOP_NANDI_DATAFIELD_VALID_t DATAFIELD_VALID;
    EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t COMMAND_CONTROL;
    ca_uint32_t key0_addr, key1_addr, hash0_addr, hash1_addr;
    ca_uint32_t sadb_ctrl_addr, valid_addr, command_addr;
    ca_uint32_t pkt_addr, next_pkt_addr;
	ca_uint32 init_done, i = 0;
    ca_status_t rc = CA_E_OK;

    if (NULL == cfg)
        return CA_E_ERROR;
    if (msk.s.an)
        if (cfg->an > __AAL_ENCRYPTION_XG_TBL_AN_MAX)
            return CA_E_PARAM;
    if (msk.s.sc)
        if (cfg->sc > __AAL_ENCRYPTION_XG_TBL_SC_MAX)
            return CA_E_PARAM;
    if (msk.s.scb)
        if (cfg->scb > __AAL_ENCRYPTION_XG_TBL_SCB_MAX)
            return CA_E_PARAM;
    if (msk.s.es)
        if (cfg->es > __AAL_ENCRYPTION_XG_TBL_ES_MAX)
            return CA_E_PARAM;
    if (msk.s.co)
        if (cfg->co > __AAL_ENCRYPTION_XG_TBL_CO_MAX)
            return CA_E_PARAM;
    if (msk.s.rlyen)
        if (cfg->rlyen > __AAL_ENCRYPTION_XG_TBL_RLYEN_MAX)
            return CA_E_PARAM;
    if (msk.s.sec_mode)
        if (cfg->sec_mode > __AAL_ENCRYPTION_XG_TBL_SEC_MODE_MAX)
            return CA_E_PARAM;
    if (msk.s.v_config)
        if (cfg->v_config > __AAL_ENCRYPTION_XG_TBL_V_CONFIG_MAX)
            return CA_E_PARAM;

    if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
        key0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_KEY0_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        key1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_KEY1_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;

        hash0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_HASH0_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        hash1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_HASH1_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;

        sadb_ctrl_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_Control + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        valid_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_DATAFIELD_VALID + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        command_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;

        pkt_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_PacketNumber + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        next_pkt_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_NextPacketNumber + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;

    }else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        key0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_KEY0_0;
        key1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_KEY1_0;

        hash0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_HASH0_0;
        hash1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_HASH1_0;

        sadb_ctrl_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_Control;
        valid_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_DATAFIELD_VALID;
        command_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL;

        pkt_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_PacketNumber;
        next_pkt_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_NextPacketNumber;
    }else{
        return CA_E_PARAM;
    }

	AAL_PON_REG_FIELD_READ(EPON_XGEPN_CORESEC10_TOP_NANDI_STATUS, mem_init_done, init_done);
    if(init_done == 0){
        return CA_E_NOT_SUPPORT;
    }

	/* Start to read */
    COMMAND_CONTROL.bf.said = said;
    COMMAND_CONTROL.bf.rwn = 0;
    COMMAND_CONTROL.bf.start_write = 1;
    AAL_PON_REG_WRITE(COMMAND_CONTROL.wrd, command_addr);

    /* Polling for read done */
    while (COMMAND_CONTROL.bf.start_write)
    {
        i++;
        if (i >= __AAL_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
        {
            return CA_E_TIMEOUT;
        }
        COMMAND_CONTROL.wrd = AAL_PON_REG_READ(command_addr);
    }

    /* Init data & msk */
    //AAL_PON_REG_INDIRECT_READ(llid_idx, command_addr);
    DATAFIELD_VALID.wrd = AAL_PON_REG_READ(valid_addr);
    SADB_Control.wrd = AAL_PON_REG_READ(sadb_ctrl_addr);

    /* STURAN (API key0_0...key0_3) -> (ASIC key0_0...key0_3) (high addr ... low addr) */
    if (msk.s.key0) {
        DATAFIELD_VALID.bf.key0_vld = 1;
        AAL_PON_REG_WRITE(cfg->key0[0], key0_addr);         /* key0_0 */
        AAL_PON_REG_WRITE(cfg->key0[1], (key0_addr - 4));   /* key0_1 */
        AAL_PON_REG_WRITE(cfg->key0[2], (key0_addr - 8));   /* key0_2 */
        AAL_PON_REG_WRITE(cfg->key0[3], (key0_addr - 12));  /* key0_3 */
    }
    if (msk.s.key1) {
        DATAFIELD_VALID.bf.key1_vld = 1;
        AAL_PON_REG_WRITE(cfg->key1[0], key1_addr);
        AAL_PON_REG_WRITE(cfg->key1[1], (key1_addr - 4));
        AAL_PON_REG_WRITE(cfg->key1[2], (key1_addr - 8));
        AAL_PON_REG_WRITE(cfg->key1[3], (key1_addr - 12));
    }
    if (msk.s.hash0) {
        DATAFIELD_VALID.bf.hash0_vld = 1;
        AAL_PON_REG_WRITE(cfg->hash0[0], hash0_addr);
        AAL_PON_REG_WRITE(cfg->hash0[1], (hash0_addr - 4));
        AAL_PON_REG_WRITE(cfg->hash0[2], (hash0_addr - 8));
        AAL_PON_REG_WRITE(cfg->hash0[3], (hash0_addr - 12));
    }
    if (msk.s.hash1) {
        DATAFIELD_VALID.bf.hash1_vld = 1;
        AAL_PON_REG_WRITE(cfg->hash1[0], hash1_addr);
        AAL_PON_REG_WRITE(cfg->hash1[1], (hash1_addr - 4));
        AAL_PON_REG_WRITE(cfg->hash1[2], (hash1_addr - 8));
        AAL_PON_REG_WRITE(cfg->hash1[3], (hash1_addr - 12));
    }
    if (msk.s.pn) {
        DATAFIELD_VALID.bf.pn_vld = 1;
        AAL_PON_REG_WRITE(cfg->pn, pkt_addr);
    }
    if (msk.s.npn) {
        DATAFIELD_VALID.bf.npn_vld = 1;
        AAL_PON_REG_WRITE(cfg->npn, next_pkt_addr);
    }
    if (msk.s.an) {
        DATAFIELD_VALID.bf.an_vld = 1;
        SADB_Control.bf.an = cfg->an;
    }
    if (msk.s.sc) {
        DATAFIELD_VALID.bf.sc_vld = 1;
        SADB_Control.bf.sc = cfg->sc;
    }
    if (msk.s.scb) {
        DATAFIELD_VALID.bf.scb_vld = 1;
        SADB_Control.bf.scb = cfg->scb;
    }
    if (msk.s.es) {
        DATAFIELD_VALID.bf.es_vld = 1;
        SADB_Control.bf.es = cfg->es;
    }
    if (msk.s.co) {
        DATAFIELD_VALID.bf.co_vld = 1;
        SADB_Control.bf.co = cfg->co;
    }
    if (msk.s.rlyen) {
        DATAFIELD_VALID.bf.rlyen_vld = 1;
        SADB_Control.bf.rlyen = cfg->rlyen;
    }
    if (msk.s.sec_mode) {
        DATAFIELD_VALID.bf.sec_mode_vld = 1;
        SADB_Control.bf.sec_mode = cfg->sec_mode;
    }
    if (msk.s.v_config) {
        DATAFIELD_VALID.bf.v_config_vld = 1;
        SADB_Control.bf.v_config = cfg->v_config;
    }

    //ca_printf("set: DATAFIELD_VALID.wrd 0x%x, SADB_Control.wrd addr 0x%x, value 0x%x\r\n", DATAFIELD_VALID.wrd, sadb_ctrl_addr, SADB_Control.wrd);

    AAL_PON_REG_WRITE(SADB_Control.wrd, sadb_ctrl_addr);
    AAL_PON_REG_WRITE(DATAFIELD_VALID.wrd, valid_addr);

	/* Start to write */
    COMMAND_CONTROL.bf.said = said;
    COMMAND_CONTROL.bf.rwn = 1;
    COMMAND_CONTROL.bf.start_write = 1;
    AAL_PON_REG_WRITE(COMMAND_CONTROL.wrd, command_addr);

    /* Polling for written done */
    while (COMMAND_CONTROL.bf.start_write)
    {
        i++;
        if (i >= __AAL_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
        {
            return CA_E_TIMEOUT;
        }
        COMMAND_CONTROL.wrd = AAL_PON_REG_READ(command_addr);
    }

    //AAL_PON_REG_INDIRECT_WRITE(llid_idx, command_addr);

    return rc;

}


ca_status_t aal_epon_crypto_xg_sa_tbl_get(
    CA_IN ca_device_id_t                    device_id,
    CA_IN ca_port_id_t                      port_id,
    CA_IN ca_uint8_t                        said,
    CA_IN aal_epon_crypto_direction_t       direction,
    CA_OUT aal_epon_crypto_xg_tbl_field_t   *cfg)
{
    aal_epon_crypto_xg_sadb_ctrl_t SADB_Control;
    EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t COMMAND_CONTROL;

    ca_uint32_t key0_addr, key1_addr, hash0_addr, hash1_addr;
    ca_uint32_t sadb_ctrl_addr, command_addr;
    ca_uint32_t pkt_addr, next_pkt_addr;
	ca_uint32_t i = 0;
    ca_status_t rc = CA_E_OK;

    if (NULL == cfg)
        return CA_E_ERROR;
    if (said >= AAL_EPON_MACSEC_SAID_NUM_MAX)
        return CA_E_PARAM;

    if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
        key0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_KEY0_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        key1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_KEY1_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;

        hash0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_HASH0_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        hash1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_HASH1_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;

        sadb_ctrl_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_Control + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        command_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;

        pkt_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_PacketNumber + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
        next_pkt_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_NextPacketNumber + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;

    }else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        key0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_KEY0_0;
        key1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_KEY1_0;

        hash0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_HASH0_0;
        hash1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_HASH1_0;

        sadb_ctrl_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_Control;
        command_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL;

        pkt_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_PacketNumber;
        next_pkt_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_NextPacketNumber;
    }else{
        return CA_E_PARAM;
    }

    //AAL_PON_REG_INDIRECT_READ(llid_idx, command_addr);

	/* Start to read */
    COMMAND_CONTROL.bf.said = said;
    COMMAND_CONTROL.bf.rwn = 0;
    COMMAND_CONTROL.bf.start_write = 1;
    AAL_PON_REG_WRITE(COMMAND_CONTROL.wrd, command_addr);

    /* Polling for read done */
    while (COMMAND_CONTROL.bf.start_write)
    {
        i++;
        if (i >= __AAL_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
        {
            return CA_E_TIMEOUT;
        }
        COMMAND_CONTROL.wrd = AAL_PON_REG_READ(command_addr);
    }
    /* read back data */
    cfg->key0[0] = AAL_PON_REG_READ(key0_addr);
    cfg->key0[1] = AAL_PON_REG_READ((key0_addr - 4));
    cfg->key0[2] = AAL_PON_REG_READ((key0_addr - 8));
    cfg->key0[3] = AAL_PON_REG_READ((key0_addr - 12));
    cfg->key1[0] = AAL_PON_REG_READ(key1_addr);
    cfg->key1[1] = AAL_PON_REG_READ((key1_addr - 4));
    cfg->key1[2] = AAL_PON_REG_READ((key1_addr - 8));
    cfg->key1[3] = AAL_PON_REG_READ((key1_addr - 12));
    cfg->hash0[0] = AAL_PON_REG_READ(hash0_addr);
    cfg->hash0[1] = AAL_PON_REG_READ((hash0_addr - 4));
    cfg->hash0[2] = AAL_PON_REG_READ((hash0_addr - 8));
    cfg->hash0[3] = AAL_PON_REG_READ((hash0_addr - 12));
    cfg->hash1[0] = AAL_PON_REG_READ(hash1_addr);
    cfg->hash1[1] = AAL_PON_REG_READ((hash1_addr - 4));
    cfg->hash1[2] = AAL_PON_REG_READ((hash1_addr - 8));
    cfg->hash1[3] = AAL_PON_REG_READ((hash1_addr - 12));

    cfg->pn = AAL_PON_REG_READ(pkt_addr);
    cfg->npn = AAL_PON_REG_READ(next_pkt_addr);

    SADB_Control.wrd = AAL_PON_REG_READ(sadb_ctrl_addr);
    ca_printf("get: SADB_Control.wrd addr 0x%x, value 0x%x\r\n", sadb_ctrl_addr, SADB_Control.wrd);

    cfg->an        = SADB_Control.bf.an      ;
    cfg->sc        = SADB_Control.bf.sc      ;
    cfg->scb       = SADB_Control.bf.scb     ;
    cfg->es        = SADB_Control.bf.es      ;
    cfg->co        = SADB_Control.bf.co      ;
    cfg->rlyen     = SADB_Control.bf.rlyen   ;
    cfg->sec_mode  = SADB_Control.bf.sec_mode;
    cfg->v_config  = SADB_Control.bf.v_config;

    return rc;
}


/* Since those MIB are read-clear, read them all in one routine.
 * Before Bala confirm, only read one counter. Maybe all of them are for MACSEC */
ca_status_t aal_epon_crypto_xg_stats_get(
    CA_IN ca_device_id_t        device_id,
    CA_IN ca_port_id_t          port_id,
    CA_IN ca_uint8_t            llid_idx,
    CA_OUT aal_epon_crypto_xg_stats_t *stats)
{
    if (NULL == stats)
        return CA_E_ERROR;
    if (llid_idx >= AAL_EPON_LLID_NUM_MAX)
        return CA_E_PARAM;

    /* TX counter */
    AAL_PON_REG_INDIRECT_READ(llid_idx, (EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));

    stats->tx_pkt_unencrypted = AAL_PON_REG_READ((EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));
    stats->tx_pkt_encrypted = AAL_PON_REG_READ((EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA1 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));
    stats->tx_byte_unencrypted = AAL_PON_REG_READ((EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA2 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));
    stats->tx_byte_encrypted = AAL_PON_REG_READ((EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));

    /* RX counter */
    AAL_PON_REG_INDIRECT_READ(llid_idx, EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS);

    stats->rx_pkt_unencrypted = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA0);
    stats->rx_pkt_encrypted = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA1);
    stats->rx_byte_unencrypted = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA2);
    stats->rx_byte_encrypted = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3);

    return CA_E_OK;
}

/*  This API is only valid when LLID is registered.
    Then software has to program registered LLIDs here. 
    LLID value can be arbitrary value or continuous value.
    Coresec_10 compares the received LLID value with this programed LLID value. 
    If the LLID matches, then hit index is used to read the Key Table. 
    Software has to program the key table in Nandi IP based on the hit index. */
ca_status_t aal_epon_crypto_xg_llid_enable_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid_idx,
    CA_IN ca_uint32_t                   llid_value,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_boolean_t                  enable)
{
    ca_status_t ret;
    aal_epon_crypto_llid_t llid_cfg;

    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);

    memset(&llid_cfg, 0, sizeof(aal_epon_crypto_llid_t));
    llid_cfg.ena = enable;
    llid_cfg.llid = llid_value;
    ret = aal_epon_crypto_xg_llid_set(device_id, port_id, llid_idx, direction, &llid_cfg);   

}

ca_status_t aal_epon_crypto_xg_llid_enable_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid_idx,
    CA_IN ca_uint32_t                   llid_value,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_OUT ca_boolean_t                 *enable)
{
    ca_status_t ret;
    aal_epon_crypto_llid_t llid_cfg;
    *enable = CA_FALSE;
    
    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);

    memset(&llid_cfg, 0, sizeof(aal_epon_crypto_llid_t));
    ret = aal_epon_crypto_xg_llid_get(device_id, port_id, llid_idx, direction, &llid_cfg);
    CA_RET_VALUE_CHECK(ret);
    *enable = llid_cfg.ena;
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_xg_enable_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_IN ca_boolean_t                  enable)
{
    ca_status_t ret;
    ca_uint32_t i = 0;
    ca_uint16_t llid_value;
    ca_uint8_t llid_num;
    ca_boolean_t reg_flag = CA_TRUE;

    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);
    
    llid_num = aal_mpcp_llid_num_get(0);
    for(i = 0; i < llid_num; i++)
    {
        aal_mpcp_reg_status_get(device_id, port_id, i, &reg_flag);
        if(reg_flag) /* encryption/de-cryption can only be enabled when LLID registered */
        {
            ret = aal_mpcp_reg_llid_get(device_id, port_id, i, &llid_value);
            CA_RET_VALUE_CHECK(ret);
            __EPON_CRYPTO_LOG(AAL_EPON_CRYPTO_TRACE_MAJ, "start to %s llid_idx %d, llid_value %d %s\r\n", (enable)?"enable":"disable", i, 
                llid_value, (direction == AAL_EPON_CRYPTO_DIRECTION_RX)?"decryption":"encryption");
            ret = aal_epon_crypto_xg_llid_enable_set(0, 0, i, llid_value, direction, enable);
            CA_RET_VALUE_CHECK(ret);            
            if(direction == AAL_EPON_CRYPTO_DIRECTION_TX)
            {
                ret = aal_epon_crypto_macsec_enable_set(device_id, port_id, i, enable);
                CA_RET_VALUE_CHECK(ret);
            }    
        }
    }
    
    if(direction == AAL_EPON_CRYPTO_DIRECTION_RX)
        __crypto_mgmt.rx_xg_ena = enable;
    else
        __crypto_mgmt.tx_xg_ena = enable;
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_xg_enable_get(
    CA_IN ca_device_id_t                device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_crypto_direction_t   direction,
    CA_OUT ca_boolean_t                 *enable)
{
    AAL_EPON_CRYPTO_DIRECTION_CHECK(direction);

    if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        *enable = __crypto_mgmt.rx_xg_ena;
        
    }else{
        *enable = __crypto_mgmt.tx_xg_ena;
    }
    
    return CA_E_OK;
}

ca_uint32_t aal_epon_crypto_reg_status_event_handler(
    ca_device_id_t      device_id,
    ca_uint32_t         event_id,
    ca_void_t           *event,
    ca_uint32_t         len,
    ca_void_t           *cookie)
{
    ca_event_reg_stat_t *pEvent;
    ca_boolean_t reg_status;
    ca_llid_t llid_idx;
    ca_uint32_t llid_value;
    ca_boolean_t ena;

    if(event_id != CA_EVENT_EPON_REG_STAT_CHNG)
    {
        return CA_E_OK;
    }
    
    if(event == NULL){
        return CA_E_PARAM;
    }
    pEvent = (ca_event_reg_stat_t *)event;
    llid_idx = pEvent->llid_idx;
    llid_value = pEvent->llid_value;
    reg_status = pEvent->status;
    
    __EPON_CRYPTO_LOG(AAL_EPON_CRYPTO_TRACE_MAJ, "REG STAT change event received: index %d, llid 0x%x, status %d\r\n", 
            llid_idx, llid_value, reg_status);

    ena = reg_status; /* If the particular LLID is de-registered, software has to disable cryption. */

    if(__crypto_mgmt.rx_1g_ena != 0)
        {
        __EPON_CRYPTO_LOG(AAL_EPON_CRYPTO_TRACE_MAJ, "llid_idx %d, llid_value %d %s, start to %s decryption\r\n", 
                llid_idx, llid_value, (ena)?"register":"de-register", (ena)?"enable":"disable");
        aal_epon_crypto_1g_llid_enable_set(0, 0, llid_idx, llid_value, AAL_EPON_CRYPTO_DIRECTION_RX, ena);
        }

    if(__crypto_mgmt.tx_1g_ena != 0)
        {
        __EPON_CRYPTO_LOG(AAL_EPON_CRYPTO_TRACE_MAJ, "llid_idx %d, llid_value %d %s, start to %s encryption\r\n", 
                llid_idx, llid_value, (ena)?"register":"de-register", (ena)?"enable":"disable");
        aal_epon_crypto_1g_llid_enable_set(0, 0, llid_idx, llid_value, AAL_EPON_CRYPTO_DIRECTION_TX, ena);
    }
    
    if(__crypto_mgmt.rx_xg_ena != 0)
        {
        __EPON_CRYPTO_LOG(AAL_EPON_CRYPTO_TRACE_MAJ, "llid_idx %d, llid_value %d %s, start to %s decryption\r\n", 
                llid_idx, llid_value, (ena)?"register":"de-register", (ena)?"enable":"disable");
        aal_epon_crypto_xg_llid_enable_set(0, 0, llid_idx, llid_value, AAL_EPON_CRYPTO_DIRECTION_RX, ena);
        }

    if(__crypto_mgmt.tx_xg_ena != 0)
        {
        __EPON_CRYPTO_LOG(AAL_EPON_CRYPTO_TRACE_MAJ, "llid_idx %d, llid_value %d %s, start to %s encryption\r\n", 
                llid_idx, llid_value, (ena)?"register":"de-register", (ena)?"enable":"disable");
        aal_epon_crypto_xg_llid_enable_set(0, 0, llid_idx, llid_value, AAL_EPON_CRYPTO_DIRECTION_TX, ena);
    }


    return CA_E_OK;
}

#if 0
ca_status_t aal_epon_crypto_mpcp_time_adjust_set(ca_device_id_t device_id, ca_port_id_t port_id, ca_uint8_t mpcp_time)
{

    //AAL_CHIP_CORE_CHECK(device_id, port_id);

    if(mpcp_time > 0x1f){
        return CA_E_PARAM;
    }
    AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_MPCP_TIMER_ADJUST, mpcp_timer_comp_mac_coresec_10, mpcp_time);

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_mpcp_time_adjust_get(ca_device_id_t device_id, ca_port_id_t port_id, ca_uint8_t *mpcp_time)
{

    //AAL_CHIP_CORE_CHECK(device_id, port_id);
    //AAL_ASSERT_RET(mpcp_time, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_MPCP_TIMER_ADJUST, mpcp_timer_comp_mac_coresec_10, *mpcp_time);

    return CA_E_OK;
}

ca_status_t __aal_epon_crypto_xg_mem_ready(CA_IN ca_device_id_t  device_id, CA_IN ca_port_id_t port_id)
{
    ca_uint32_t ii = 0;
    EPON_XGEPN_CORESEC10_TOP_NANDI_STATUS_t STATUS;
    for (ii = 0; ii < __AAL_ENCRYPTION_TBL_ACCESS_RETRY_MAX; ii++) {
        STATUS.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_NANDI_STATUS);
        if (STATUS.bf.mem_init_done) {
            return CA_E_OK;
        }
    }

    return CA_E_TIMEOUT;
}
#endif

ca_status_t __enc_mode_to_1g_crypto_mode(
    ca_port_encryption_mode_t enc_mode,
    aal_epon_crypto_1g_mode_t *crypto_mode,
    aal_epon_crypto_key_exchange_mode_t *key_exchange)
{
    switch(enc_mode){
        case CA_PORT_ENCRYPTION_MODE_DISABLE:    /* 0 */
            *crypto_mode = AAL_EPON_CRYPTO_1G_MODE_DISABLE;     /* 1 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case CA_PORT_ENCRYPTION_MODE_AES_CFB:    /* 2 */
            *crypto_mode = AAL_EPON_CRYPTO_1G_MODE_AES_DPOE;  /* 1 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case CA_PORT_ENCRYPTION_MODE_SW_CTC_CHURNING:    /* 4 */
            *crypto_mode = AAL_EPON_CRYPTO_1G_MODE_CHURNING;  /* 0 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case CA_PORT_ENCRYPTION_MODE_HW_CTC_CHURNING:    /* 5 */
            *crypto_mode = AAL_EPON_CRYPTO_1G_MODE_CHURNING;  /* 0 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_HW;
            break;

        default:
            ca_printf("not support this mode %d\r\n", enc_mode);
            return CA_E_NOT_SUPPORT;
    }
    return CA_E_OK;
}

ca_status_t __enc_mode_to_xg_crypto_mode(
    ca_port_encryption_mode_t enc_mode,
    aal_epon_crypto_xg_mode_t *crypto_mode,
    aal_epon_crypto_key_exchange_mode_t *key_exchange)
{
    switch(enc_mode){
        case CA_PORT_ENCRYPTION_MODE_DISABLE:    /* 0 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_DISABLE;     /* 3 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case CA_PORT_ENCRYPTION_MODE_AES_CTR:    /* 1 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_AES_DPOE;  /* 1 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case CA_PORT_ENCRYPTION_MODE_MACSEC:    /* 3 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_MACSEC;     /* 0 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case CA_PORT_ENCRYPTION_MODE_SW_CTC_CHURNING:    /* 4 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_CHURNING;  /* 2 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case CA_PORT_ENCRYPTION_MODE_HW_CTC_CHURNING:    /* 5 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_CHURNING;  /* 2 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_HW;
            break;

        default:
            ca_printf("not support this mode %d\r\n", enc_mode);
            return CA_E_NOT_SUPPORT;
    }
    return CA_E_OK;
}

ca_status_t aal_epon_crypto_init(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t mode
)
{
    ca_status_t ret;
    ca_uint32_t enc_mode;
    ca_uint32_t crypto_mode;
    aal_epon_crypto_key_exchange_mode_t key_exchange;
    ca_uint32_t pon_mode;
    
    aal_epon_crypto_cfg_msk_t msk;
    aal_epon_crypto_cfg_t cfg;

    memset(&__crypto_mgmt, 0, sizeof(aal_epon_crypto_mgmt_t));
    __crypto_mgmt.debug_level = 0xf;

    /* get epon encryption mode from SCFG, 
       the value of CFG_ID_EPON_ENCRYPTION_MODE: refer to ca_port_encryption_mode_t */
    /* ASIC limitation: encryption mode can't be updated during run-time. */
    ret = scfg_read(0, CFG_ID_EPON_ENCRYPTION_MODE, sizeof(ca_uint32), &enc_mode);
    if (ret != CA_E_OK)
    {
        ca_printf("ca-mpcp: failed to get CFG_ID_EPON_ENCRYPTION_MODE.\n");
        enc_mode = 0; /* disabled, bypass encryption module */
    }

    memset(&msk, 0, sizeof(aal_epon_crypto_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_epon_crypto_cfg_t));

    pon_mode = aal_pon_mac_mode_get(device_id);
    if(pon_mode == ONU_PON_MAC_EPON_1G)
    {
        ret = __enc_mode_to_1g_crypto_mode(enc_mode, &crypto_mode, &key_exchange);
        if(ret != CA_E_OK){
            return ret;
        }
        msk.s.tx_mode = 1;
        msk.s.rx_mode = 1;
        msk.s.key_exchange_mode = 1;
        cfg.tx_mode = crypto_mode;
        cfg.rx_mode = crypto_mode;
        cfg.key_exchange_mode = key_exchange;
        aal_epon_crypto_1g_cfg_set(device_id, port_id, msk, &cfg);
        
    }else if(pon_mode == ONU_PON_MAC_EPON_BI10G)
    {
        ret = __enc_mode_to_xg_crypto_mode(enc_mode, &crypto_mode, &key_exchange);
        if(ret != CA_E_OK){
            return ret;
        }
        msk.s.tx_mode = 1;
        msk.s.rx_mode = 1;
        msk.s.key_exchange_mode = 1;
        cfg.tx_mode = crypto_mode;
        cfg.rx_mode = crypto_mode;
        cfg.key_exchange_mode = key_exchange;
        aal_epon_crypto_xg_cfg_set(device_id, port_id, msk, &cfg);
    }
    else if(pon_mode == ONU_PON_MAC_EPON_D10G)
    {
        ret = __enc_mode_to_xg_crypto_mode(enc_mode, &crypto_mode, &key_exchange);
        if(ret != CA_E_OK){
            return ret;
        }
        msk.s.tx_mode = 1;
        msk.s.rx_mode = 1;
        msk.s.key_exchange_mode = 1;
        cfg.tx_mode = crypto_mode;
        cfg.rx_mode = crypto_mode;
        cfg.key_exchange_mode = key_exchange;
    aal_epon_crypto_xg_cfg_set(device_id, port_id, msk, &cfg);
    }

    ret = ca_event_register(0, CA_EVENT_EPON_REG_STAT_CHNG, aal_epon_crypto_reg_status_event_handler, NULL);
    
    if (CA_E_OK != ret) 
    {
        __EPON_CRYPTO_LOG(AAL_EPON_CRYPTO_TRACE_CRI, "reg state change Event reg failed. ret=%d\n", ret);
        return CA_E_ERROR;
    }
    return CA_E_OK;
}

/********************* Following APIs are for 10G EPON MACSEC **********************/
ca_status_t aal_epon_crypto_macsec_local_sa_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN aal_epon_crypto_direction_t direction,
    CA_IN aal_epon_crypto_macsec_sa_cfg_msk_t msk,
    CA_IN aal_epon_crypto_macsec_sa_cfg_t *cfg)
{
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_0_t sa0;
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_1_t sa1;
	ca_uint32_t sa0_addr=0, sa1_addr=0;
	if (NULL == cfg){
        return CA_E_ERROR;
    }
	if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
		sa0_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		sa1_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_1 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
	}else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
		sa0_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_0 ;
		sa1_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_1 ;
	}else{
		return CA_E_PARAM;
	}
	sa0.wrd = AAL_PON_REG_READ(sa0_addr);
	sa1.wrd = AAL_PON_REG_READ(sa1_addr);
	if (msk.s.local_sa_reg0) {
        sa0.bf.local_sa_reg0 = cfg->local_sa_reg0;
    }
	if (msk.s.local_sa_reg1) {
        sa1.bf.local_sa_reg1 = cfg->local_sa_reg1;
    }
	if (msk.s.sel_local_sa_reg) {
        sa1.bf.sel_local_sa_reg = cfg->sel_local_sa_reg;
    }
	AAL_PON_REG_WRITE(sa0.wrd, sa0_addr);
	AAL_PON_REG_WRITE(sa1.wrd, sa1_addr);
	return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_local_sa_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_OUT aal_epon_crypto_macsec_sa_cfg_t *cfg)
{
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_0_t sa0;
	EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_1_t sa1;
	ca_uint32_t sa0_addr = 0, sa1_addr = 0;
	if (NULL == cfg){
        return CA_E_ERROR;
    }
	if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
		sa0_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		sa1_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_1 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
	}else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
		sa0_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_0 ;
		sa1_addr = EPON_XGEPN_CORESEC10_TOP_CORESEC10_SA_1 ;
	}else{
		return CA_E_PARAM;
	}
	sa0.wrd = AAL_PON_REG_READ(sa0_addr);
	sa1.wrd = AAL_PON_REG_READ(sa1_addr);
	cfg->local_sa_reg0 = sa0.bf.local_sa_reg0;
	cfg->local_sa_reg1 = sa1.bf.local_sa_reg1;
	cfg->sel_local_sa_reg = sa1.bf.sel_local_sa_reg;
	return CA_E_OK;
}

ca_status_t aal_epon_txmac_pkt_type_enable_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_epon_txmac_pkt_type_ena_cfg_msk_t msk,
	CA_IN aal_epon_txmac_pkt_type_ena_cfg_t *cfg)
{
	EPON_XGEPN_XMAC_TXMAC_PKT_TYPE_EN_t txmac_pkt_ena;
	if (NULL == cfg){
        return CA_E_ERROR;
    }
	txmac_pkt_ena.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_TXMAC_PKT_TYPE_EN);
	if(msk.s.pkt_type){
		txmac_pkt_ena.bf.pkt_type = cfg->pkt_type;
	}
	if(msk.s.mpcp_sec_en){
		txmac_pkt_ena.bf.mpcp_sec_en = cfg->mpcp_sec_en;
	}
	if(msk.s.oam_sec_en){
		txmac_pkt_ena.bf.oam_sec_en = cfg->oam_sec_en;
	}
	if(msk.s.ethernettype_sec_en){
		txmac_pkt_ena.bf.ethernettype_sec_en = cfg->ethernettype_sec_en;
	}
	AAL_PON_REG_WRITE(txmac_pkt_ena.wrd, EPON_XGEPN_XMAC_TXMAC_PKT_TYPE_EN);
	return CA_E_OK;
}
ca_status_t aal_epon_txmac_pkt_type_enable_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_epon_txmac_pkt_type_ena_cfg_t *cfg)
{
	EPON_XGEPN_XMAC_TXMAC_PKT_TYPE_EN_t txmac_pkt_ena;
	if (NULL == cfg){
        return CA_E_ERROR;
    }
	txmac_pkt_ena.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_TXMAC_PKT_TYPE_EN);
	cfg->pkt_type = txmac_pkt_ena.bf.pkt_type;
	cfg->mpcp_sec_en = txmac_pkt_ena.bf.mpcp_sec_en;
	cfg->oam_sec_en = txmac_pkt_ena.bf.oam_sec_en;
	cfg->ethernettype_sec_en = txmac_pkt_ena.bf.ethernettype_sec_en;
	
        return CA_E_OK;
}

ca_status_t aal_epon_crypto_nandi_cfg_set(
	CA_IN ca_device_id_t	   device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_IN aal_epon_crypto_nandi_cfg_msk_t msk,
	CA_IN aal_epon_crypto_nandi_cfg_t *cfg
)
{
	EPON_XGEPN_CORESEC10_TOP_NANDI_SCI1_t                        crypto_sci1;
    EPON_XGEPN_CORESEC10_TOP_NANDI_SCI0_t                        crypto_sci0;
    EPON_XGEPN_CORESEC10_TOP_NANDI_PNTHLD_t                      crypto_pn_thred;
    EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW_t                crypto_reply_window;
	EPON_XGEPN_CORESEC10_TOP_NANDI_CONFIG_t nandi_cfg;
	ca_uint32_t crypto_sci1_addr = 0;
	ca_uint32_t crypto_sci0_addr = 0;
	ca_uint32_t crypto_pn_thred_addr = 0;
	ca_uint32_t crypto_reply_window_addr = 0;
	ca_uint32_t nandi_cfg_addr = 0;
	
	if (NULL == cfg){
        return CA_E_ERROR;
    }
	if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
		crypto_sci1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SCI1 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		crypto_sci0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SCI0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		crypto_pn_thred_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_PNTHLD + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		crypto_reply_window_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		nandi_cfg_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_CONFIG + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
	}else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
		crypto_sci1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SCI1 ;
		crypto_sci0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SCI0 ;
		crypto_pn_thred_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_PNTHLD;
		crypto_reply_window_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW ;
		nandi_cfg_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_CONFIG ;
	}else{
		return CA_E_PARAM;
	}
	crypto_sci1.wrd                 = AAL_PON_REG_READ(crypto_sci1_addr);
    crypto_sci0.wrd                 = AAL_PON_REG_READ(crypto_sci0_addr);
    crypto_pn_thred.wrd             = AAL_PON_REG_READ(crypto_pn_thred_addr);
    crypto_reply_window.wrd         = AAL_PON_REG_READ(crypto_reply_window_addr);
	nandi_cfg.wrd 					= AAL_PON_REG_READ(nandi_cfg_addr);

	if(msk.s.sci1){
		crypto_sci1.wrd = ((ca_uint32_t)(cfg->sci1[0]) << 24 )
			| ((ca_uint32_t)(cfg->sci1[1]) << 16)
			| ((ca_uint32_t)(cfg->sci1[2]) << 8)
			| ((ca_uint32_t)(cfg->sci1[3]));
	}
	if(msk.s.sci0){
		crypto_sci0.wrd = ((ca_uint32_t)(cfg->sci0[0]) << 24 )
			| ((ca_uint32_t)(cfg->sci0[1]) << 16)
			| ((ca_uint32_t)(cfg->sci0[2]) << 8)
			| ((ca_uint32_t)(cfg->sci0[3]));
	}
	if(msk.s.pn_threshold){
		crypto_pn_thred.wrd = cfg->pn_threshold;
	}
	if(msk.s.replay_window){
		crypto_reply_window.wrd = cfg->replay_window;
	}
	
	if (msk.s.sci_llid_disable) {
        nandi_cfg.bf.sci_llid_disable = cfg->sci_llid_disable;
    }
	if (msk.s.sec_tag_icv_send_en) {
        nandi_cfg.bf.sec_tag_icv_send_en = cfg->sec_tag_icv_send_en;
    }
	if (msk.s.sec_tag_mem_cmd) {
        nandi_cfg.bf.sec_tag_mem_cmd = cfg->sec_tag_mem_cmd;
    }
	AAL_PON_REG_WRITE(crypto_sci1.wrd,EPON_XGEPN_CORESEC10_TOP_NANDI_SCI1);
	AAL_PON_REG_WRITE(crypto_sci0.wrd,EPON_XGEPN_CORESEC10_TOP_NANDI_SCI0);
	AAL_PON_REG_WRITE(crypto_pn_thred.wrd,EPON_XGEPN_CORESEC10_TOP_NANDI_PNTHLD);
	AAL_PON_REG_WRITE(crypto_reply_window.wrd,EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW);
	AAL_PON_REG_WRITE(nandi_cfg.wrd, nandi_cfg_addr);
	return CA_E_OK;
}

ca_status_t aal_epon_crypto_nandi_cfg_get(
	CA_IN ca_device_id_t	   device_id,
	CA_IN ca_port_id_t		port_id,
	CA_IN aal_epon_crypto_direction_t direction,
	CA_OUT aal_epon_crypto_nandi_cfg_t  *cfg
)
{
	EPON_XGEPN_CORESEC10_TOP_NANDI_SCI1_t                        crypto_sci1;
    EPON_XGEPN_CORESEC10_TOP_NANDI_SCI0_t                        crypto_sci0;
    EPON_XGEPN_CORESEC10_TOP_NANDI_PNTHLD_t                      crypto_pn_thred;
    EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW_t                crypto_reply_window;
	EPON_XGEPN_CORESEC10_TOP_NANDI_CONFIG_t nandi_cfg;
	ca_uint32_t crypto_sci1_addr = 0;
	ca_uint32_t crypto_sci0_addr = 0;
	ca_uint32_t crypto_pn_thred_addr = 0;
	ca_uint32_t crypto_reply_window_addr = 0;
	ca_uint32_t nandi_cfg_addr = 0;
	
	if (NULL == cfg){
        return CA_E_ERROR;
    }
	if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
		crypto_sci1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SCI1 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		crypto_sci0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SCI0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		crypto_pn_thred_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_PNTHLD + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		crypto_reply_window_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
		nandi_cfg_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_CONFIG + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET;
	}else if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
		crypto_sci1_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SCI1 ;
		crypto_sci0_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_SCI0 ;
		crypto_pn_thred_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_PNTHLD;
		crypto_reply_window_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW ;
		nandi_cfg_addr = EPON_XGEPN_CORESEC10_TOP_NANDI_CONFIG ;
	}else{
		return CA_E_PARAM;
	}
	crypto_sci1.wrd                 = AAL_PON_REG_READ(crypto_sci1_addr);
    crypto_sci0.wrd                 = AAL_PON_REG_READ(crypto_sci0_addr);
    crypto_pn_thred.wrd             = AAL_PON_REG_READ(crypto_pn_thred_addr);
    crypto_reply_window.wrd         = AAL_PON_REG_READ(crypto_reply_window_addr);
	nandi_cfg.wrd = AAL_PON_REG_READ(nandi_cfg_addr);

	cfg->sci1[0] = (crypto_sci1.wrd >> 24 ) & 0xFF;
	cfg->sci1[1] = (crypto_sci1.wrd >> 16 ) & 0xFF;
	cfg->sci1[2] = (crypto_sci1.wrd >> 8 ) & 0xFF;
	cfg->sci1[3] = (crypto_sci1.wrd) & 0xFF;

	cfg->sci0[0] = (crypto_sci0.wrd >> 24 ) & 0xFF;
	cfg->sci0[1] = (crypto_sci0.wrd >> 16 ) & 0xFF;
	cfg->sci0[2] = (crypto_sci0.wrd >> 8 ) & 0xFF;
	cfg->sci0[3] = (crypto_sci0.wrd) & 0xFF;
		
	cfg->pn_threshold   = crypto_pn_thred.wrd;
    cfg->replay_window  = crypto_reply_window.wrd ;
	
	cfg->sci_llid_disable = nandi_cfg.bf.sci_llid_disable;
	cfg->sec_tag_icv_send_en = nandi_cfg.bf.sec_tag_icv_send_en;
	cfg->sec_tag_mem_cmd = nandi_cfg.bf.sec_tag_mem_cmd;
	return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_rx_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t said,
    CA_OUT aal_epon_crypto_macsec_rx_stats_t *stats)
{
    ca_status_t ret = CA_E_OK;
    if (NULL == stats){
        return CA_E_ERROR;
    }
    if (said > 64){
        return CA_E_PARAM;
    }

	AAL_PON_REG_INDIRECT_READ(said,EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS);

    stats->rx_pkt_is_invalid                = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA0);
    stats->rx_pkt_number_less_than_lowest_pn= AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA1);
    stats->rx_oct_protected_unencrypted = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA2)|
   										 ((ca_uint64_t)(AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3)&0x7ff) <<32);
	
    stats->rx_oct_protected_encrypted       = ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3)>>11)|
                                    		  ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA4)&0x3FFFFF)<<21)) ;
	
    stats->rx_rplen_less_than_lowest_pn     = ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA4)>>22)|
        									  ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA5)&0x3FFFFF)<<10));
	
	
    stats->rx_pkts_ok = ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA5)>>22)|
        				((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA6)&0x3FFFFF)<<10));

    stats->rx_pkt_invalid     = ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA6)>>22)|
        						((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA7)&0x3FFFFF)<<10));
	
    stats->rx_pkt_not_valid   = ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA7)>>22)|
        						((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA8)&0x3FFFFF)<<10));
	
    stats->rx_pkt_not_using_sa = ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA8)>>22)|
        						 ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA9)&0x3FFFFF)<<10));

    stats->rx_pkt_unused_sa  = ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA9)>>22)|
        					   ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA10)&0x3FFFFF)<<10));

	return ret;
}

ca_status_t aal_epon_crypto_macsec_rx_error_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT aal_epon_crypto_macsec_rx_error_stats_t *stats)
{
    ca_status_t ret = CA_E_OK;
    if (NULL == stats){
        return CA_E_ERROR;
    }
   
    stats->rx_pkt_untagged         = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsUnTagged);
    stats->rx_pkt_no_tag           = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsNoTag);
    stats->rx_pkt_bad_tag          = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsBadTag);
    stats->rx_pkt_unknow_sci       = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsUnKnownSCI);
    stats->rx_pkt_overrun          = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsOverrun);
    stats->rx_pkt_dec_invalid_co_err= AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_DEC_INVALID_CO_ERR_CNT);
    stats->rx_pkt_dec_auth_fail_err = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_DEC_AUTH_FAIL_ERR_CNT);
    stats->rx_pkt_dec_ver_num_err   = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_DEC_VER_NUM_ERR_CNT);
    stats->rx_pkt_dec_sl_msb_err    = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_MACSEC_DEC_SL_MSB_ERR_CNT);
    stats->rx_pkt_unknown_llid     = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_PKTS_UN_REG_LLID_CNT);
    stats->rx_pkt_dec_sl_msb_err    = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_RUNT_PKTS_CNT);

    return ret;
}

ca_status_t aal_epon_crypto_macsec_tx_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN  ca_uint8_t said,
    CA_OUT aal_epon_crypto_macsec_tx_stats_t *stats)
{
    ca_status_t ret = CA_E_OK;

    if (NULL == stats){
        return CA_E_ERROR;
    }
    if (said >= 64){
        return CA_E_PARAM;
    }

	AAL_PON_REG_INDIRECT_READ(said,(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET));

    stats->tx_pkt_protected = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA0 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET);
    stats->tx_pkt_encrypted = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA1 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET);
	
    stats->tx_oct_protected = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA2 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET)|
                             ((ca_uint64_t)(AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET)&0x7ff) <<32);
	
    stats->tx_oct_encrypted = ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET)>>11)|
                              ((AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA4 + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET)&0x3FFFFF)<<21)) ;

	return ret;
}

ca_status_t aal_epon_crypto_macsec_tx_error_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT aal_epon_crypto_macsec_tx_error_stats_t *stats)
{
    ca_status_t ret = CA_E_OK;

    if (NULL == stats){
        return CA_E_ERROR;
    }

    stats->tx_pkt_untagged          = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_MACSEC_MIB_OutPktsUnTagged + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET);
    stats->tx_pkt_too_long          = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_MACSEC_MIB_OutPktsTooLong + __AAL_ENCRYPTION_XG_TX_RX_ADDR_OFFSET);
    
    return ret;
}

ca_status_t aal_epon_crypto_macsec_misc_set(
    CA_IN ca_device_id_t  device_id)
{

    AAL_PON_REG_WRITE(0x0,          PUC_PVTBL_DATA4);
    AAL_PON_REG_WRITE(0x0,          PUC_PVTBL_DATA3);
    AAL_PON_REG_WRITE(0x10,         PUC_PVTBL_DATA2);
    AAL_PON_REG_WRITE(0x87868584,   PUC_PVTBL_DATA1);
    AAL_PON_REG_WRITE(0x838281b8,   PUC_PVTBL_DATA0);
    AAL_PON_REG_INDIRECT_WRITE(0,   PUC_PVTBL_ACCESS);

    AAL_PON_REG_WRITE(0x0,          PDC_PDC_MAP_MEM_DATA1);
    AAL_PON_REG_WRITE(0x8e28,       PDC_PDC_MAP_MEM_DATA0);
    AAL_PON_REG_INDIRECT_WRITE(0,   PDC_PDC_MAP_MEM_ACCESS);

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_misc_get(
    CA_IN ca_device_id_t  device_id)
{
    memset(&epon_cryp_macsec_misc_cfg, 0, sizeof(epon_cryp_macsec_misc_cfg));

    
    AAL_PON_REG_INDIRECT_READ(0, PUC_PVTBL_ACCESS);

    epon_cryp_macsec_misc_cfg.pvtbl_data0_old = AAL_PON_REG_READ (PUC_PVTBL_DATA0);
    epon_cryp_macsec_misc_cfg.pvtbl_data1_old = AAL_PON_REG_READ (PUC_PVTBL_DATA1);
    epon_cryp_macsec_misc_cfg.pvtbl_data2_old = AAL_PON_REG_READ (PUC_PVTBL_DATA2);
    epon_cryp_macsec_misc_cfg.pvtbl_data3_old = AAL_PON_REG_READ (PUC_PVTBL_DATA3);
    epon_cryp_macsec_misc_cfg.pvtbl_data4_old = AAL_PON_REG_READ (PUC_PVTBL_DATA4);
    
    AAL_PON_REG_INDIRECT_READ(0, PDC_PDC_MAP_MEM_ACCESS);
    epon_cryp_macsec_misc_cfg.pdc_mem_map_data0_old = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA0);
    epon_cryp_macsec_misc_cfg.pdc_mem_map_data1_old = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA1);

    return CA_E_OK;
}

ca_status_t aal_epon_crypto_macsec_misc_recover(
    CA_IN ca_device_id_t  device_id,
    CA_IN aal_epon_crypto_direction_t direction)
{
    if(direction == AAL_EPON_CRYPTO_DIRECTION_TX){
        AAL_PON_REG_WRITE(epon_cryp_macsec_misc_cfg.pvtbl_data4_old,   PUC_PVTBL_DATA4);
        AAL_PON_REG_WRITE(epon_cryp_macsec_misc_cfg.pvtbl_data3_old,   PUC_PVTBL_DATA3);
        AAL_PON_REG_WRITE(epon_cryp_macsec_misc_cfg.pvtbl_data2_old,   PUC_PVTBL_DATA2);
        AAL_PON_REG_WRITE(epon_cryp_macsec_misc_cfg.pvtbl_data1_old,   PUC_PVTBL_DATA1);
        AAL_PON_REG_WRITE(epon_cryp_macsec_misc_cfg.pvtbl_data0_old,   PUC_PVTBL_DATA0);
        AAL_PON_REG_INDIRECT_WRITE(0,   PUC_PVTBL_ACCESS);
    }
    
    if(direction == AAL_EPON_CRYPTO_DIRECTION_RX){
        AAL_PON_REG_WRITE(epon_cryp_macsec_misc_cfg.pdc_mem_map_data1_old,  PDC_PDC_MAP_MEM_DATA1);
        AAL_PON_REG_WRITE(epon_cryp_macsec_misc_cfg.pdc_mem_map_data0_old,  PDC_PDC_MAP_MEM_DATA0);
        AAL_PON_REG_INDIRECT_WRITE(0,   PDC_PDC_MAP_MEM_ACCESS);
    }
    return CA_E_OK;
}

EXPORT_SYMBOL(aal_epon_crypto_debug_set);
EXPORT_SYMBOL(aal_epon_crypto_debug_get);
EXPORT_SYMBOL(aal_epon_crypto_1g_cfg_set);
EXPORT_SYMBOL(aal_epon_crypto_1g_cfg_get);
EXPORT_SYMBOL(aal_epon_crypto_1g_llid_set);
EXPORT_SYMBOL(aal_epon_crypto_1g_llid_get);
EXPORT_SYMBOL(aal_epon_crypto_1g_llid_enable_set);
EXPORT_SYMBOL(aal_epon_crypto_1g_llid_enable_get);
EXPORT_SYMBOL(aal_epon_crypto_1g_enable_set);
EXPORT_SYMBOL(aal_epon_crypto_1g_enable_get);
EXPORT_SYMBOL(aal_epon_crypto_1g_key_set);
EXPORT_SYMBOL(aal_epon_crypto_1g_key_get);
EXPORT_SYMBOL(aal_epon_crypto_1g_stats_get);
EXPORT_SYMBOL(aal_epon_crypto_xg_mode_get);
EXPORT_SYMBOL(aal_epon_crypto_xg_cfg_set);
EXPORT_SYMBOL(aal_epon_crypto_xg_cfg_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_enable_set);
EXPORT_SYMBOL(aal_epon_crypto_macsec_enable_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_said_set);
EXPORT_SYMBOL(aal_epon_crypto_macsec_said_get);
EXPORT_SYMBOL(aal_epon_crypto_xg_sa_tbl_set);
EXPORT_SYMBOL(aal_epon_crypto_xg_sa_tbl_get);
EXPORT_SYMBOL(aal_epon_crypto_xg_llid_set);
EXPORT_SYMBOL(aal_epon_crypto_xg_llid_get);
EXPORT_SYMBOL(aal_epon_crypto_xg_llid_enable_set);
EXPORT_SYMBOL(aal_epon_crypto_xg_llid_enable_get);
EXPORT_SYMBOL(aal_epon_crypto_xg_enable_get);
EXPORT_SYMBOL(aal_epon_crypto_xg_enable_set);
EXPORT_SYMBOL(aal_epon_crypto_xg_stats_get);
EXPORT_SYMBOL(aal_epon_crypto_coresec_glb_cfg_set);
EXPORT_SYMBOL(aal_epon_crypto_coresec_glb_cfg_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_local_sa_set);
EXPORT_SYMBOL(aal_epon_crypto_macsec_local_sa_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_sectag_type_set);
EXPORT_SYMBOL(aal_epon_crypto_macsec_sectag_type_get);
EXPORT_SYMBOL(aal_epon_txmac_pkt_type_enable_set);
EXPORT_SYMBOL(aal_epon_txmac_pkt_type_enable_get);
EXPORT_SYMBOL(aal_epon_crypto_nandi_cfg_set);
EXPORT_SYMBOL(aal_epon_crypto_nandi_cfg_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_rx_stats_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_tx_stats_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_rx_error_stats_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_tx_error_stats_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_misc_set);
EXPORT_SYMBOL(aal_epon_crypto_macsec_misc_get);
EXPORT_SYMBOL(aal_epon_crypto_macsec_misc_recover);

