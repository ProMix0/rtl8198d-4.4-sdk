#include <soc/cortina/registers.h>
#include "aal_xgpon.h"
#include "osal_common.h"
#include "scfg.h"
#include "aal_pon.h"
#include "aal_ploam.h"
#include "aal_xgpon_fec.h"
#include "aal_xgpon_intr.h"
#include "aal_pdc.h"
#include "aal_puc.h"
#include "aal_idp.h"
#include "aal_xgpon_actp.h"
#include "ploam_ngpon2.h"
#include "ploam_pkt.h"
#include "gpon.h"
#include "xgpon_aes.h"
#include "peri.h"
#include "aal_psds.h"

#ifdef CONFIG_LUNA_G3_SERIES
#include "aal_l2_vlan.h"
aal_xgpon_bwmp_alloc_id_cfg_t tmp_alloc_id_cfg[AAL_XGPON_MAX_TCONT_NUM];
aal_puc_queue_cfg_t tmp_puc_queue_cfg[AAL_XGPON_MAX_TCONT_NUM][8];
aal_xgpon_usbg_gem_config_t tmp_usbg_xgem_cfg[AAL_XGPON_MAX_GEM_NUM];
aal_xgpon_dsfp_xgem_cfg_t tmp_dsfp_xgem_cfg[AAL_XGPON_MAX_GEM_NUM];
aal_puc_ctrl_cfg_t tmp_puc_ctrl_config;
aal_puc_btc_cfg_t tmp_puc_btc_config;
aal_puc_voq_tbc_mem_cfg_t tmp_puc_voq_tbc_mem_cfg[AAL_PUC_MAX_QUEUE_NUM];
aal_puc_voq_tbc_mem_cfg_t tmp_puc_rl_voq_tbc_mem_cfg[AAL_PUC_MAX_QUEUE_NUM];
ca_uint32_t tmp_puc_bp_tqmvoqid[64];//bp only has 64 entries
ca_uint32_t tmp_puc_voq_buf_lmt_sel1500_cfg;
ca_uint32_t tmp_puc_voq_buf_lmt_sel3116_cfg;
ca_uint32_t tmp_puc_voq_buf_lmt_sel4732_cfg;
ca_uint32_t tmp_puc_voq_buf_lmt_sel6348_cfg;
ca_uint32_t tmp_puc_voq_buf_limit_a;
ca_uint32_t tmp_puc_voq_buf_limit_b;
ca_uint32_t tmp_puc_voq_buf_limit_c;

extern ca_uint32_t tcont_num_per_chip;
extern ca_uint32_t queue_num_per_tcont;
#endif

extern void aal_clear_actp_rx_tx_optical_tuning
(
    CA_IN ca_uint8 clear_rx,
    CA_IN ca_uint8 clear_tx
);

#define CA_AAL_XGPON_KEY_IDX_CHECK(val)         if(((val) != 0) && ((val) != 1)) { \
                                                    ca_printf("%s:[%d] val %d!!\n", __FUNCTION__, __LINE__, val); \
                                                    return CA_E_PARAM; \
                                                }

#define CA_AAL_XGPON_KEY_LEN_CHECK(val)         if(((val) != 16)){ \
                                                    ca_printf("%s:[%d] val %d!!\n", __FUNCTION__, __LINE__, val); \
                                                    return CA_E_PARAM; \
                                                }
#define CA_AAL_XGON_BOOL_VAL_CHECK(val)         if(((val) != FALSE) && ((val) != TRUE)) { \
                                                    ca_printf("%s:[%d] val %d!!\n", __FUNCTION__, __LINE__, val); \
                                                    return CA_E_PARAM; \
                                                }


#define XGPON_ASSERT(x)      CA_ASSERT_RET(x, CA_E_PARAM)

#define __XGPN_FIND_INDIRCT_ADDRESS(rbw,select,index)   (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|((ca_uint32_t)(select)<<16)|(ca_uint32_t)(index))
#define __XGPN_DO_INDIRCT_OP(rbw,select, index, reg) AAL_PON_REG_WRITE(__XGPN_FIND_INDIRCT_ADDRESS(rbw,select, index), (void *)reg); \
                                  __CHECK_INDIRCT_OPERATE_STATE(reg)

#define AAL_XGPN_REG_INDIRECT_READ(select,index, reg) __XGPN_DO_INDIRCT_OP(0, select,index, reg)
#define AAL_XGPN_REG_INDIRECT_WRITE(select,index, reg) __XGPN_DO_INDIRCT_OP(1, select,index, reg)


#define __XGPN_BWMP_FIND_INDIRCT_ADDRESS(grant_ff_sel, index)    (((ca_uint32_t)1<<31)|((ca_uint32_t)(grant_ff_sel)<<29)|(ca_uint32_t)(index))

#define __XGPN_BWMP_DO_INDIRCT_OP(grant_ff_sel, index, reg)  AAL_PON_REG_WRITE(__XGPN_BWMP_FIND_INDIRCT_ADDRESS(grant_ff_sel, index), (void *)reg); \
                                  __CHECK_INDIRCT_OPERATE_STATE(reg)

#define AAL_XGPN_BWMP_REG_INDIRECT_READ(grant_ff_sel ,index,reg) __XGPN_BWMP_DO_INDIRCT_OP(grant_ff_sel, index,  reg)
//#define AAL_XGPN_BWMP_REG_INDIRECT_WRITE(grant_ff_sel, index, reg) __XGPN_BWMP_DO_INDIRCT_OP(grant_ff_sel,index,  reg)



#define __XGPN_MSG_CTL_FIND_INDIRCT_ADDRESS(rbw, index) (((ca_uint32_t)(rbw)<<31)|(ca_uint32_t)(index))
#define __XGPN_MSG_CTL_DO_INDIRCT_OP(rbw, index, reg) AAL_PON_REG_WRITE(__XGPN_MSG_CTL_FIND_INDIRCT_ADDRESS(rbw, index), (void *)reg); \
                                  __CHECK_INDIRCT_OPERATE_STATE(reg)

#define AAL_XGPN_MSG_CTL_REG_INDIRECT_READ(index,reg) __XGPN_MSG_CTL_DO_INDIRCT_OP(0, index, reg)
#define AAL_XGPN_MSG_CTL_REG_INDIRECT_WRITE(index,reg) __XGPN_MSG_CTL_DO_INDIRCT_OP(1, index, reg)


#define __XGPN_BWMP_ALLOC_FIND_INDIRCT_ADDRESS(rbw, uwlchid, index) (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|((ca_uint32_t)(uwlchid)<<8)|(ca_uint32_t)((index)&0x1F))
#define __XGPN_BWMP_ALLOC_DO_INDIRCT_OP(rbw, uwlchid, index, reg) AAL_PON_REG_WRITE(__XGPN_BWMP_ALLOC_FIND_INDIRCT_ADDRESS(rbw, uwlchid, index), reg); \
                                  __CHECK_INDIRCT_OPERATE_STATE(reg)

#define AAL_XGPN_BWMP_ALLOC_REG_INDIRECT_READ(uwlchid, index, reg) __XGPN_BWMP_ALLOC_DO_INDIRCT_OP(0, uwlchid, index, reg)
#define AAL_XGPN_BWMP_ALLOC_REG_INDIRECT_WRITE(uwlchid, index, reg) __XGPN_BWMP_ALLOC_DO_INDIRCT_OP(1, uwlchid, index, reg)

#define __TCONT_FLUSH_TIMEOUT  5000
#define __XGPON_MAX_FIFOS  8
#define __XGPON_MAX_GRANTS 32

ca_int32_t aal_xgpn_mac_debug = 0;
ca_int32_t aal_xgpn_simulation_flag = 0;
ca_int32_t aal_xgpn_protect_switch_sim_flag = 0;
ca_uint32_t aal_xgpn_auto_reset = 1;

ca_uint32_t aal_xgpn_aes_ukey_chg_flag = 0;
ca_int32_t aal_xgpon_aes_debug = 0;
ca_int32_t aal_xgpon_aes_sm_debug = 0;
ca_int32_t aal_xgpon_aes_key_type = 0;
ca_int32_t aal_xgpn_aes_rpt_hw_accelerate = 0;
ca_int32_t aal_xgpon_aes_key_hw_mode = 0;

#define AAL_FOR_ALL_XGEM_DO(gem) \
        for ((gem) = 0; (gem) <= AAL_XGPON_MAX_GEM_PORT_IDX; (gem)++)

#define AAL_FOR_ALL_USR_XGEM_DO(gem) \
        for ((gem) = 8; (gem) <= AAL_XGPON_MAX_GEM_PORT_IDX; (gem)++)

#define AAL_FOR_ALL_OMCI_XGEM_DO(gem) \
        for ((gem) = 0; (gem) <= 7; (gem)++)


ca_uint g_aal_xgpon_lock = 0;
#define AAL_XGPON_LOCK()           ca_spin_lock(g_aal_xgpon_lock)
#define AAL_XGPON_UNLOCK()         ca_spin_unlock(g_aal_xgpon_lock)

static ca_status_t __aal_xgpon_gem_enable(ca_device_id_t device_id,
    ca_uint32_t gem_start, ca_uint32_t gem_end,ca_boolean_t ena)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t dsfp_gem_idx = 0;
    aal_xgpon_dsfp_xgem_cfg_msk_t dsfp_xgem_mask;
    aal_xgpon_dsfp_xgem_cfg_t  dsfp_xgem_cfg;

    ca_uint32_t usbg_gem_idx = 0;
    aal_xgpon_usbg_gem_config_msk_t usbg_xgem_mask;
    aal_xgpon_usbg_gem_config_t  usbg_xgem_cfg;

    if((gem_start > AAL_XGPON_MAX_GEM_PORT_IDX) ||(gem_end > AAL_XGPON_MAX_GEM_PORT_IDX)){
        ca_printf("ERROR:%s:%d:gem_start or gem_end out of range:0-127!!\r\n",__FUNCTION__,__LINE__);
        return CA_E_ERROR;
    }
    if(gem_start > gem_end){
        ca_printf("ERROR:%s:%d:gem_start > gem_end !!\r\n",__FUNCTION__,__LINE__);
        return CA_E_ERROR;
    }
    memset(&dsfp_xgem_cfg, 0 ,sizeof(aal_xgpon_dsfp_xgem_cfg_t));
    memset(&usbg_xgem_cfg, 0 ,sizeof(aal_xgpon_usbg_gem_config_t));

    dsfp_xgem_mask.u32 = 0;
    dsfp_xgem_mask.s.gem_vld = 1;
    //dsfp_xgem_mask.s.gem_id = 1;
    //dsfp_xgem_mask.s.gem_mc = 1;

    for(dsfp_gem_idx = gem_start ;dsfp_gem_idx <= gem_end; dsfp_gem_idx++){
        dsfp_xgem_cfg.gem_vld = ena;
        ret = aal_xgpon_dsfp_xgem_set(device_id,dsfp_gem_idx,dsfp_xgem_mask,&dsfp_xgem_cfg);
        CA_RET_VALUE_CHECK(ret);
    }

    usbg_xgem_mask.u32 = 0;
    usbg_xgem_mask.s.port_en = 1;

    for(usbg_gem_idx = gem_start;usbg_gem_idx <= gem_end; usbg_gem_idx++){

        usbg_xgem_cfg.port_en = ena;
        ret = aal_xgpon_usbg_xgem_cfg_set(device_id,usbg_gem_idx,usbg_xgem_mask,&usbg_xgem_cfg);
        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}

#ifdef AAL_XGPN_DEBUG
ca_status_t __aal_xgpon_api_test(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_dsfp_xgem_cfg_msk_t mask;
    aal_xgpon_dsfp_xgem_cfg_t xgem_cfg;
    aal_uc_enc_key_config_msk_t key_mask;
    aal_uc_enc_key_config_t key_config;
    aal_uc_enc_key_config_t key_config_output;
    ca_uint8_t key1[16] = {0x22,0x33,0x44,0x55,0x11,0x12,0x13,0x14,
                           0x15,0x15,0x77,0x88,0x99,0x20,0x30,0x10};

    mask.s.gem_id = 1;
    mask.s.gem_vld =1;
    xgem_cfg.gem_id = 0x1234;
    xgem_cfg.gem_vld =1;
    ret = aal_xgpon_dsfp_xgem_set( device_id,1,mask,&xgem_cfg);
    CA_RET_VALUE_CHECK(ret);
    ret = aal_xgpon_dsfp_xgem_get( device_id,1,&xgem_cfg);
    CA_RET_VALUE_CHECK(ret);

    key_mask.s.p_key_active = 1;
    key_mask.s.p_effective_key_length = 1;
    key_mask.s.uc_key16 = 1;

    key_config.p_key_active = 1;
    key_config.p_effective_key_length = 16;
    memcpy(key_config.uc_key,key1,XGPN_AES_KEY_LEN);
    if (aal_xgpn_mac_debug){
        ca_buffer_dump("key_config.uc_key", key_config.uc_key, XGPN_AES_KEY_LEN);
    }

    ret = aal_xgpon_aes_uc_enc_key_set(device_id,CA_AAL_XGPON_KEY_IDX_0,key_mask,&key_config);
    CA_RET_VALUE_CHECK(ret);
    ret = aal_xgpon_aes_uc_enc_key_get(device_id,CA_AAL_XGPON_KEY_IDX_0,&key_config_output);
    CA_RET_VALUE_CHECK(ret);
    if (aal_xgpn_mac_debug){
        ca_buffer_dump("key_config_output.uc_key", key_config_output.uc_key, XGPN_AES_KEY_LEN);
    }

    ca_printf("%s:%d::gem_id is 0x%x\r\n",__FUNCTION__,__LINE__,xgem_cfg.gem_id);
    return ret;
}

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
#endif
static ca_status_t aal_xgpon_reset_puc(ca_device_id_t  device_id)
{
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, puc_reset, 0);
    ca_mdelay(5);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, puc_reset, 1);


    aal_puc_init(device_id, aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID));

    ca_printf("%s(): done, should enable GEM,VoQ with OMCI or manually.\r\n",
        __FUNCTION__);

    return CA_E_OK;
}
#define AES_BLOCK_SIZE		16

static void joint_two_mac(const ca_uint8_t *mac1,const ca_uint8_t *mac2, ca_uint8_t *out)
{
	if (memcmp(mac1, mac2, ETH_ALEN) < 0) {
		memcpy(out, mac1, ETH_ALEN);
		memcpy(out + ETH_ALEN, mac2, ETH_ALEN);
	} else {
		memcpy(out, mac2, ETH_ALEN);
		memcpy(out + ETH_ALEN, mac1, ETH_ALEN);
	}
}

static inline void WPA_PUT_BE16(ca_uint8_t *a, ca_uint16_t val)
{
	a[0] = val >> 8;
	a[1] = val & 0xff;
}

/* IEEE Std 802.1X-2010, 6.2.1 KDF */
static ca_uint32_t aes_kdf_128(
	const ca_uint8_t *kdk, 
	const ca_uint8_t *label, 
	const ca_uint8_t *context,
	int ctx_bits, 
	int ret_bits, 
	ca_uint8_t *ret)
{
	const int h = 128;
	const int r = 8;
	int i, n;
	int lab_len, ctx_len, ret_len, buf_len;
	ca_uint8_t *buf;

	lab_len = strlen(label);
	ctx_len = (ctx_bits + 7) / 8;
	ret_len = ((ret_bits & 0xffff) + 7) / 8;
	buf_len = lab_len + ctx_len + 4;

	memset(ret, 0, ret_len);

	n = (ret_bits + h - 1) / h;
	if (n > ((0x1 << r) - 1))
		return -1;

	buf = ca_malloc(buf_len);
	if (buf == NULL)
		return -1;

	memcpy(buf + 1, label, lab_len);
	memcpy(buf + lab_len + 2, context, ctx_len);
	WPA_PUT_BE16(&buf[buf_len - 2], ret_bits);

	for (i = 0; i < n; i++) {
		buf[0] = (ca_uint8_t) (i + 1);
		if (aes128_cmac_compute((ca_uint8_t *)kdk, buf, buf_len, 16, ret)) {
			ca_free(buf);
			return -1;
		}
		ret = ret + h / 8;
	}
	ca_free(buf);
	return 0;
}

int ieee802_1x_cak_128bits_aes_cmac(
	const ca_uint8_t *msk, 
	const ca_uint8_t *mac1,
	const ca_uint8_t *mac2, 
	ca_uint8_t *cak)
{
	ca_uint8_t context[2 * ETH_ALEN];

	joint_two_mac(mac1, mac2, context);
	return aes_kdf_128(msk, "IEEE8021 EAP CAK",
			   context, sizeof(context) * 8, 128, cak);
}

/**
 * ieee802_1x_ckn_128bits_aes_cmac
 *
 * IEEE Std 802.1X-2010, 6.2.2
 * CKN = KDF(Key, Label, ID | mac1 | mac2, CKNlength)
 */
int ieee802_1x_ckn_128bits_aes_cmac(const ca_uint8_t *msk, const ca_uint8_t *mac1,
					const ca_uint8_t *mac2, const ca_uint8_t *sid,
					ca_uint32_t sid_bytes, ca_uint8_t *ckn)
{
	int res;
	ca_uint8_t *context;
	ca_uint32_t ctx_len = sid_bytes + ETH_ALEN * 2;

	context = ca_malloc(ctx_len);
	if (!context) {
		//wpa_printf(MSG_ERROR, "MKA-%s: out of memory", __func__);
		return -1;
	}
	memcpy(context, sid, sid_bytes);
	joint_two_mac(mac1, mac2, context + sid_bytes);

	res = aes_kdf_128(msk, "IEEE8021 EAP CKN", context, ctx_len * 8,
			  128, ckn);
	ca_free(context);
	return res;
}


/**
 * ieee802_1x_kek_128bits_aes_cmac
 *
 * IEEE Std 802.1X-2010, 9.3.3
 * KEK = KDF(Key, Label, Keyid, KEKLength)
 */
int ieee802_1x_kek_128bits_aes_cmac(const ca_uint8_t *cak, const ca_uint8_t *ckn,
					ca_uint32_t ckn_bytes, ca_uint8_t *kek)
{
	ca_uint8_t context[16];

	/* First 16 octets of CKN, with null octets appended to pad if needed */
	memset(context, 0, sizeof(context));
	memcpy(context, ckn, (ckn_bytes < 16) ? ckn_bytes : 16);

	return aes_kdf_128(cak, "IEEE8021 KEK", context, sizeof(context) * 8,
			   128, kek);
}


/**
 * ieee802_1x_ick_128bits_aes_cmac
 *
 * IEEE Std 802.1X-2010, 9.3.3
 * ICK = KDF(Key, Label, Keyid, ICKLength)
 */
int ieee802_1x_ick_128bits_aes_cmac(const ca_uint8_t *cak, const ca_uint8_t *ckn,
					ca_uint32_t ckn_bytes, ca_uint8_t *ick)
{
	ca_uint8_t context[16];

	/* First 16 octets of CKN, with null octets appended to pad if needed */
	memset(context, 0, sizeof(context));
	memcpy(context, ckn, (ckn_bytes < 16) ? ckn_bytes : 16);

	return aes_kdf_128(cak, "IEEE8021 ICK", context, sizeof(context) * 8,
			   128, ick);
}


/**
 * ieee802_1x_icv_128bits_aes_cmac
 *
 * IEEE Std 802.1X-2010, 9.4.1
 * ICV = AES-CMAC(ICK, M, 128)
 */
int ieee802_1x_icv_128bits_aes_cmac(const ca_uint8_t *ick, const ca_uint8_t *msg,
					ca_uint32_t msg_bytes, ca_uint8_t *icv)
{
	if (aes128_cmac_compute((ca_uint8_t *)ick, (ca_uint8_t *)msg, msg_bytes, 16,icv)) {
		//wpa_printf(MSG_ERROR, "MKA: omac1_aes_128 failed");
		return -1;
	}
	return 0;
}


/**
 * ieee802_1x_sak_128bits_aes_cmac
 *
 * IEEE Std 802.1X-2010, 9.8.1
 * SAK = KDF(Key, Label, KS-nonce | MI-value list | KN, SAKLength)
 */
int ieee802_1x_sak_128bits_aes_cmac(const ca_uint8_t *cak, const ca_uint8_t *ctx,
					ca_uint32_t ctx_bytes, ca_uint8_t *sak)
{
	return aes_kdf_128(cak, "IEEE8021 SAK", ctx, ctx_bytes * 8, 128, sak);
}

static inline void WPA_PUT_BE32(ca_uint8_t *a, ca_uint32_t val)
{
	a[0] = (val >> 24) & 0xff;
	a[1] = (val >> 16) & 0xff;
	a[2] = (val >> 8) & 0xff;
	a[3] = val & 0xff;
}

static inline ca_uint32_t WPA_GET_BE32(const ca_uint8_t *a)
{
	return ((ca_uint32_t) a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
}

static void inc32(ca_uint8_t *block)
{
	ca_uint32_t val;
	val = WPA_GET_BE32(block + AES_BLOCK_SIZE - 4);
	val++;
	WPA_PUT_BE32(block + AES_BLOCK_SIZE - 4, val);
}


static void xor_block(ca_uint8_t *dst, const ca_uint8_t *src)
{
	ca_uint32_t *d = (ca_uint32_t *) dst;
	ca_uint32_t *s = (ca_uint32_t *) src;
	*d++ ^= *s++;
	*d++ ^= *s++;
	*d++ ^= *s++;
	*d++ ^= *s++;
}

static void shift_right_block(ca_uint8_t *v)
{
	ca_uint32_t val;

	val = WPA_GET_BE32(v + 12);
	val >>= 1;
	if (v[11] & 0x01)
		val |= 0x80000000;
	WPA_PUT_BE32(v + 12, val);

	val = WPA_GET_BE32(v + 8);
	val >>= 1;
	if (v[7] & 0x01)
		val |= 0x80000000;
	WPA_PUT_BE32(v + 8, val);

	val = WPA_GET_BE32(v + 4);
	val >>= 1;
	if (v[3] & 0x01)
		val |= 0x80000000;
	WPA_PUT_BE32(v + 4, val);

	val = WPA_GET_BE32(v);
	val >>= 1;
	WPA_PUT_BE32(v, val);
}
#ifndef BIT
#define BIT(x) (1U << (x))
#endif

static inline void WPA_PUT_BE64(ca_uint8_t *a, ca_uint64_t val)
{
	a[0] = val >> 56;
	a[1] = val >> 48;
	a[2] = val >> 40;
	a[3] = val >> 32;
	a[4] = val >> 24;
	a[5] = val >> 16;
	a[6] = val >> 8;
	a[7] = val & 0xff;
}

/* Multiplication in GF(2^128) */
static void gf_mult(const ca_uint8_t *x, const ca_uint8_t *y, ca_uint8_t *z)
{
	ca_uint8_t v[16];
	int i, j;

	memset(z, 0, 16); /* Z_0 = 0^128 */
	memcpy(v, y, 16); /* V_0 = Y */

	for (i = 0; i < 16; i++) {
		for (j = 0; j < 8; j++) {
			if (x[i] & BIT(7 - j)) {
				/* Z_(i + 1) = Z_i XOR V_i */
				xor_block(z, v);
			} else {
				/* Z_(i + 1) = Z_i */
			}

			if (v[15] & 0x01) {
				/* V_(i + 1) = (V_i >> 1) XOR R */
				shift_right_block(v);
				/* R = 11100001 || 0^120 */
				v[0] ^= 0xe1;
			} else {
				/* V_(i + 1) = V_i >> 1 */
				shift_right_block(v);
			}
		}
	}
}

static void ghash_start(ca_uint8_t *y)
{
	/* Y_0 = 0^128 */
	memset(y, 0, 16);
}


static void ghash(
	const ca_uint8_t *h, 
	const ca_uint8_t *x, 
	ca_uint32_t xlen, 
	ca_uint8_t *y)
{
	ca_uint32_t m, i;
	const ca_uint8_t *xpos = x;
	ca_uint8_t tmp[16];

	m = xlen / 16;

	for (i = 0; i < m; i++) {
		/* Y_i = (Y^(i-1) XOR X_i) dot H */
		xor_block(y, xpos);
		xpos += 16;

		/* dot operation:
		 * multiplication operation for binary Galois (finite) field of
		 * 2^128 elements */
		gf_mult(y, h, tmp);
		memcpy(y, tmp, 16);
	}

	if (x + xlen > xpos) {
		/* Add zero padded last block */
		ca_uint8_t last = x + xlen - xpos;
		memcpy(tmp, xpos, last);
		memset(tmp + last, 0, sizeof(tmp) - last);

		/* Y_i = (Y^(i-1) XOR X_i) dot H */
		xor_block(y, tmp);

		/* dot operation:
		 * multiplication operation for binary Galois (finite) field of
		 * 2^128 elements */
		gf_mult(y, h, tmp);
		memcpy(y, tmp, 16);
	}

	/* Return Y_m */
}

static void aes_gctr(void *aes, const ca_uint8_t *icb, const ca_uint8_t *x, ca_uint32_t xlen, ca_uint8_t *y)
{
	ca_uint32_t i, n, last;
	ca_uint8_t cb[AES_BLOCK_SIZE], tmp[AES_BLOCK_SIZE];
	const ca_uint8_t *xpos = x;
	ca_uint8_t *ypos = y;
	ca_status_t ret=  CA_E_ERROR;
	if (xlen == 0)
		return;

	n = xlen / 16;

	memcpy(cb, icb, AES_BLOCK_SIZE);
	/* Full blocks */
	for (i = 0; i < n; i++) {
		ret = aes128_ecb_encrypt(aes, cb, ypos);
 		CA_RET_VALUE_CHECK(ret);
		xor_block(ypos, xpos);
		xpos += AES_BLOCK_SIZE;
		ypos += AES_BLOCK_SIZE;
		inc32(cb);
	}

	last = x + xlen - xpos;
	if (last) {
		/* Last, partial block */
		ret = aes128_ecb_encrypt(aes, cb, tmp);
		CA_RET_VALUE_CHECK(ret);
		for (i = 0; i < last; i++)
			*ypos++ = *xpos++ ^ tmp[i];
	}
}

#ifdef AAL_XGPN_DEBUG
static void * aes_gcm_init_hash_subkey(const u8 *key, size_t key_len, u8 *H)
{
	void *aes;

	aes = aes_encrypt_init(key, key_len);
	if (aes == NULL)
		return NULL;

	/* Generate hash subkey H = AES_K(0^128) */
	os_memset(H, 0, AES_BLOCK_SIZE);
	aes_encrypt(aes, H, H);
	wpa_hexdump_key(MSG_EXCESSIVE, "Hash subkey H for GHASH",
			H, AES_BLOCK_SIZE);
	return aes;
}
#endif

static void aes_gcm_prepare_j0(const ca_uint8_t *iv, ca_uint32_t iv_len, const ca_uint8_t *H, ca_uint8_t *J0)
{
	ca_uint8_t len_buf[16];

	if (iv_len == 12) {
		/* Prepare block J_0 = IV || 0^31 || 1 [len(IV) = 96] */
		memcpy(J0, iv, iv_len);
		memset(J0 + iv_len, 0, AES_BLOCK_SIZE - iv_len);
		J0[AES_BLOCK_SIZE - 1] = 0x01;
	} else {
		/*
		 * s = 128 * ceil(len(IV)/128) - len(IV)
		 * J_0 = GHASH_H(IV || 0^(s+64) || [len(IV)]_64)
		 */
		ghash_start(J0);
		ghash(H, iv, iv_len, J0);
		WPA_PUT_BE64(len_buf, 0);
		WPA_PUT_BE64(len_buf + 8, iv_len * 8);
		ghash(H, len_buf, sizeof(len_buf), J0);
	}
}


static void aes_gcm_gctr(void *aes, const ca_uint8_t *J0, const ca_uint8_t *in, ca_uint32_t len,
			 ca_uint8_t *out)
{
	ca_uint8_t J0inc[AES_BLOCK_SIZE];

	if (len == 0)
		return;

	memcpy(J0inc, J0, AES_BLOCK_SIZE);
	inc32(J0inc);
	aes_gctr(aes, J0inc, in, len, out);
}


static void aes_gcm_ghash(const ca_uint8_t *H, const ca_uint8_t *aad, ca_uint32_t aad_len,
			  const ca_uint8_t *crypt, ca_uint32_t crypt_len, ca_uint8_t *S)
{
	ca_uint8_t len_buf[16];

	/*
	 * u = 128 * ceil[len(C)/128] - len(C)
	 * v = 128 * ceil[len(A)/128] - len(A)
	 * S = GHASH_H(A || 0^v || C || 0^u || [len(A)]64 || [len(C)]64)
	 * (i.e., zero padded to block size A || C and lengths of each in bits)
	 */
	ghash_start(S);
	ghash(H, aad, aad_len, S);
	ghash(H, crypt, crypt_len, S);
	WPA_PUT_BE64(len_buf, aad_len * 8);
	WPA_PUT_BE64(len_buf + 8, crypt_len * 8);
	ghash(H, len_buf, sizeof(len_buf), S);

	//wpa_hexdump_key(MSG_EXCESSIVE, "S = GHASH_H(...)", S, 16);
}


/**
 * aes_gcm_ae - GCM-AE_K(IV, P, A)
 */
int aes_gcm_ae(const ca_uint8_t *key, ca_uint32_t key_len, const ca_uint8_t *iv, ca_uint32_t iv_len,
		   const ca_uint8_t *plain, ca_uint32_t plain_len,
		   const ca_uint8_t *aad, ca_uint32_t aad_len, ca_uint8_t *crypt, ca_uint8_t *tag)
{
	ca_uint8_t H[AES_BLOCK_SIZE];
	ca_uint8_t J0[AES_BLOCK_SIZE];
	ca_uint8_t S[16];
	ca_uint8_t data[16] ={0}; 
	ca_status_t ret = CA_E_ERROR;

	ret = aes128_ecb_encrypt( (ca_uint8_t *)key, data, H);
	CA_RET_VALUE_CHECK(ret);

	aes_gcm_prepare_j0(iv, iv_len, H, J0);

	/* C = GCTR_K(inc_32(J_0), P) */
	aes_gcm_gctr((ca_uint8_t *)key, J0, plain, plain_len, crypt);

	aes_gcm_ghash(H, aad, aad_len, crypt, plain_len, S);

	/* T = MSB_t(GCTR_K(J_0, S)) */
	aes_gctr((ca_uint8_t *)key, J0, S, sizeof(S), tag);

	/* Return (C, T) */

	//aes_encrypt_deinit(aes);

	return 0;
}


/**
 * aes_gcm_ad - GCM-AD_K(IV, C, A, T)
 */
int aes_gcm_ad(const ca_uint8_t *key, ca_uint32_t key_len, const ca_uint8_t *iv, ca_uint32_t iv_len,
		   const ca_uint8_t *crypt, ca_uint32_t crypt_len,
		   const ca_uint8_t *aad, ca_uint32_t aad_len, const ca_uint8_t *tag, ca_uint8_t *plain)
{
	ca_uint8_t H[AES_BLOCK_SIZE];
	const ca_uint8_t J0[AES_BLOCK_SIZE];
	ca_uint8_t S[16], T[16];
	ca_uint8_t data[16] ={0}; 
	ca_status_t ret = CA_E_ERROR;

	ret = aes128_ecb_encrypt( (ca_uint8_t *)key, data, H);
	CA_RET_VALUE_CHECK(ret);
	aes_gcm_prepare_j0(iv, iv_len, H, (ca_uint8_t *)J0);

	/* P = GCTR_K(inc_32(J_0), C) */
	aes_gcm_gctr((void*)key, J0, crypt, crypt_len, plain);

	aes_gcm_ghash(H, aad, aad_len, crypt, crypt_len, S);

	/* T' = MSB_t(GCTR_K(J_0, S)) */
	aes_gctr((void *)key, J0, S, sizeof(S), T);

	//aes_encrypt_deinit(aes);

	if (memcmp(tag, T, 16) != 0) {
		//wpa_printf(MSG_EXCESSIVE, "GCM: Tag mismatch");
		return -1;
	}

	return 0;
}


int aes_gmac(const ca_uint8_t *key, ca_uint32_t key_len, const ca_uint8_t *iv, ca_uint32_t iv_len,
		 const ca_uint8_t *aad, ca_uint32_t aad_len, ca_uint8_t *tag)
{
	return aes_gcm_ae(key, key_len, iv, iv_len, NULL, 0, aad, aad_len, NULL,
			  tag);
}


ca_status_t aal_xgpon_mac_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
)
{
    ca_status_t ret = CA_E_OK;
	//ca_uint8_t def_key[16] ={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	//						 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    ca_uint8_t def_key[16] ={0x04,0x6b,0x5f,0x5d,0xae,0x80,0x73,0x1d,
                            0x3a,0x9b,0x08,0x1c,0x70,0x3a,0xc9,0xb3};
	ca_uint8_t def_key1[16] ={0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
							 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22};
	ca_uint8_t hash_key[16] ={0}; 
	ca_uint8_t data[16] ={0}; 
	ca_uint8_t msk[16] =	   {0xe6,0x8a,0x1a,0xb9,0x03,0x13,0x02,0x4f,
								0xda,0x7a,0x04,0xa0,0x3f,0xea,0x01,0x0f};
	//ca_uint8_t mac1[6] = {0x00,0x1b,0x63,0x93,0xfc,0xbc};
	//ca_uint8_t mac2[6] = {0x00,0xd0,0xb7,0x1a,0x77,0x17};
	ca_uint8_t mac1[6] = {0x00,0x13,0x25,0x00,0x00,0x02};
	ca_uint8_t mac2[6] = {0x00,0x0c,0x29,0x7d,0xf4,0x7f};
	/* ca_uint8_t cak_msg[29] = {0x49,0x45,0x45,0x45,0x38,0x30,0x32,0x31,
							  0x20,0x45,0x41,0x50,0x20,0x43,0x41,0x4b,0x00,
							  0x00,0x1b,0x63,0x93,0xfc,0xbc,
							  0x00,0xd0,0xb7,0x1a,0x77,0x17}; */
	ca_uint8_t cak[16] = {0};
	ca_uint8_t ckn[16] = {0};
	ca_uint8_t macsec_kek[16] = {0};
	ca_uint8_t ick[16] = {0};
	ca_uint8_t sak[16] = {0};
	ca_uint8_t ckn_session_id[65] = {0x0d,
									 0xd0,0x75,0x69,0x3f,0x54,0xb2,0xb2,0xeb,
									 0x01,0xda,0x61,0xf0,0xaf,0x5d,0x42,0x9b,
									 0x65,0xb1,0xeb,0xca,0xf5,0x36,0xfb,0xa3,
									 0x50,0x77,0x75,0x98,0x57,0x17,0x28,0xf6,
									 0x30,0xc5,0xc8,0xda,0x54,0x75,0x48,0x9a,
									 0xd4,0x7b,0x6e,0x34,0x89,0xa9,0x73,0x72,
									 0xe5,0xa8,0xfd,0x55,0x06,0x17,0x97,0x2c,
									 0x02,0x0d,0x42,0xa3,0xb1,0x3a,0x4e,0xae};
	ca_uint8_t ks_nonce[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
						       0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10};
	ca_uint8_t mi_value_lst[24]= {0xcd,0x42,0x1c,0xf8,0x6b,0xa4,0x57,0x93,
							      0x86,0x57,0x67,0x5b,0x01,0x02,0x03,0x04,
							      0x05,0x06,0x07,0x08,0x0d,0x1f,0x36,0xcf};
	ca_uint8_t kn[4] = {0x00,0x00,0x00,0x01};
	ca_uint8_t sak_ctx[44] = {0};
	ca_uint8_t iv[12] = {0x00,0x0c,0x29,0x7d,0xf4,0x7f,0x00,0x01,0x00,0x00,0x00,0x0b };//sci+pn
	ca_uint8_t aad[90]={
			0x00,0x13,0x25,0x00,0x00,0x02,0x00,0x0c,0x29,0x7d,0xf4,0x7f,0x88,0xe5,0x2c,0x00,
			0x00,0x00,0x00,0x0b,0x00,0x0c,0x29,0x7d,0xf4,0x7f,0x00,0x01,0x4a,0xcb,0x26,0xca,
			0x8a,0x54,0x1b,0x93,0xab,0x22,0xa5,0x9a,0x3d,0x28,0xfe,0xe2,0xcf,0xc1,0x96,0x7b,
			0x5f,0x75,0x55,0x45,0x59,0x18,0x2d,0xe3,0x84,0x2b,0x67,0x0f,0x04,0xc7,0xa6,0xac,
			0xbf,0xb2,0x08,0x2f,0x9f,0xb6,0xb4,0xcd,0xe1,0x4f,0xf0,0x14,0x96,0x76,0x10,0xa1,
			0x46,0x29,0x70,0x91,0x5a,0x08,0xe2,0x39,0x71,0x34
	};
			
	ca_uint8_t tag[16] = {0};
    switch (value1){

        case 1:
            aal_xgpn_mac_debug = value2;
            ca_printf("xgpon mac debug set: %d\r\n", aal_xgpn_mac_debug);
            break;
        case 2:
            switch (value2){
#if 0
                case 1:
#ifdef PON_DEBUG
                    ret = ca_gpon_api_test(device_id);
                    CA_RET_VALUE_CHECK(ret);
#endif
                    break;
                case 2:
                    ret = ca_gpon_cb_dump(device_id);
                    CA_RET_VALUE_CHECK(ret);
                    break;
                case 3:
#ifdef PON_DEBUG
                    ret = __aal_xgpon_api_test(device_id);
                    CA_RET_VALUE_CHECK(ret);
#endif
                    break;
#endif
                case 4:
					//ghash_start(hash_key);
					//ghash(def_key, data, 16,hash_key);
					
					ret = aes128_ecb_encrypt(def_key, data, hash_key);
					CA_RET_VALUE_CHECK(ret);
					ca_buffer_dump("def_key:",def_key, 16);
					ca_buffer_dump("data:",data, 16);
					ca_buffer_dump("hash key:",hash_key, 16);

					ret = aes128_ecb_encrypt(def_key1, data, hash_key);
					CA_RET_VALUE_CHECK(ret);
					ca_buffer_dump("def_key1:",def_key1, 16);
					ca_buffer_dump("data:",data, 16);
					ca_buffer_dump("hash key1:",hash_key, 16);

					ret = aes_gmac(def_key, 16, iv, 12,aad,90,tag);
					CA_RET_VALUE_CHECK(ret);
					ca_buffer_dump("def_key :",def_key, 16);
					ca_buffer_dump("iv :",iv, 12);
					ca_buffer_dump("tag:",tag, 16);
					
					ret = ieee802_1x_cak_128bits_aes_cmac(msk, mac1, mac2,cak);
					CA_RET_VALUE_CHECK(ret);
					ca_buffer_dump("msk :",msk, 16);
					//ca_buffer_dump("cak_msg :",cak_msg, 29);
					ca_buffer_dump("cak key:",cak, 16);
					
					ret = ieee802_1x_ckn_128bits_aes_cmac(msk, mac1,mac2, ckn_session_id,65, ckn);
					ca_buffer_dump("ckn:",ckn, 16);

					ret = ieee802_1x_kek_128bits_aes_cmac(cak, ckn,16, macsec_kek);
					ca_buffer_dump("macsec_kek:",macsec_kek, 16);

					ret = ieee802_1x_ick_128bits_aes_cmac(cak, ckn,16, ick);
					ca_buffer_dump("ick:",ick, 16);

					memcpy(sak_ctx,ks_nonce,16);
					memcpy(sak_ctx + 16,mi_value_lst,24);
					memcpy(sak_ctx + 40,kn,4);
					ret = ieee802_1x_sak_128bits_aes_cmac(cak, sak_ctx,44, sak);
					ca_buffer_dump("sak_ctx:",sak_ctx, 44);
					ca_buffer_dump("sak:",sak, 16);
					
					break;

                case 5:
                    //ca_printf("Start NGPON2 activation state machine!!\r\n ");
                    //ret = aal_xgpon_sm_simulation(0);
                    //CA_RET_VALUE_CHECK(ret);
                    break;

                default:
                    break;
            }
            break;

        case 3:
            aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(device_id, value2);
            break;

        case 4:
            aal_xgpon_reset_puc(device_id);
            break;

        case 5:
            aal_xgpon_clk_seq_reset(device_id);
            break;

        case 6:
           //aal_xgpon_mac_reset(device_id, AAL_XGPON_RESET_FLAG_ALL);
            //break;
        case 7:
            aal_xgpon_datapath_reset(device_id);
            break;
        case 8:
            aal_xgpn_auto_reset = value2;
            break;
        case 9:
            ca_printf("Enable aal_xgpn_simulation_flag for activation sm simulation.\r\n");
            aal_xgpn_simulation_flag = value2;
            break;
        case 10:
            ca_printf("Enable aal_xgpn_protect_switch_sim_flag for activation sm simulation.\r\n");
            aal_xgpn_protect_switch_sim_flag = value2;
            break;

        default:
            ca_printf("\r\nvalue1 for debug selection, valu2 for debug config\r\n");
            ca_printf("      1: Set value2 to aal_xgpn_mac_debug\r\n ");
            ca_printf("      2: cortina_api test or p_gpon_cb dump \r\n ");
            ca_printf("        value2: 1. cortina_api test\r\n ");
            ca_printf("                2. p_gpon_cb dump\r\n ");
            ca_printf("                2. aal xgpon test\r\n ");
            ca_printf("                4. ngpon2 state machine simulation\r\n ");
            ca_printf("      3: reset clock and clear optical tuning \r\n ");
            ca_printf("      4: reset PUC \r\n ");
            ca_printf("      5: reset clock sequence \r\n ");
            ca_printf("      6: reset pon mac and PUC\r\n ");
            ca_printf("      7: reset datapath modules\r\n ");
            ca_printf("      8: configure the auto-reset PON MAC function when ONU de-active\r\n ");
            ca_printf("      9: Enable aal_xgpn_simulation_flag.\r\n ");
            ca_printf("      10: Enable aal_xgpn_protect_switch_sim_flag.\r\n ");

            break;
    }

    return CA_E_OK;
}


extern ca_uint32_t xfp_op_delay;
ca_status_t aal_xgpon_us_ds_chanl_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  value2
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t  data[4] = {0};
    ret = aal_xgpon_xfp_chanl_set(0, value2&0xFF, (value2&0xFF00)>>8);
    if (CA_E_OK != ret){
        ca_printf("channel set fail, return %d\r\n", ret);
        return ret;
    }

    ret = cap_i2c_read(0, 0xA0, 0x70, 2, &(data[0]));
    if (CA_E_OK == ret){
        ca_printf("channel set done, read back: TX=%02d, RX=%02d\r\n",
            data[0], data[1]);
    }

    return ret;
}

ca_status_t aal_xgpon_us_ds_chanl_get(
    CA_IN ca_device_id_t  device_id,
    CA_OUT ca_uint32_t  value2
)
{
    ca_status_t ret= CA_E_OK;
    ca_uint8_t xfp_reg;

    ret = cap_i2c_read(0, 0xA0, 0x70, 1, &xfp_reg);
    if (CA_E_OK == ret){
        ca_printf("TX ID %d\r\n", xfp_reg);
    }
    else {
        ca_printf("TX get error 0x%x\r\n", ret);
    }
    ca_mdelay(xfp_op_delay);
    ret = cap_i2c_read(0, 0xA0, 0x71, 1, &xfp_reg);
    if (CA_E_OK == ret) {
        ca_printf("RX ID %d\r\n", xfp_reg);
    }
    else {
        ca_printf("RX get error 0x%x\r\n", ret);
    }
    ca_mdelay(xfp_op_delay);
    ret = cap_i2c_read(0, 0xA0, 110, 1, &xfp_reg);//General Control/Status Bits
    if (CA_E_OK == ret){
        ca_printf("General Control/Status Bits 0x%x\r\n", xfp_reg);
    } else {
        ca_printf("General Control/Status Bits read error 0x%x\r\n", ret);
    }
    return ret;
}


ca_status_t aal_xgpon_tcont_bwmp_grant_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t grant_ff_sel,
    CA_IN ca_uint8_t tcont_id
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t fifo_id = 0;
    ca_uint32_t grant_id = 0;
    ca_uint32_t addr = 0;
    aal_xgpon_bwmp_grt_ff_cfg_t grf_ff_cfg;
    aal_xgpon_bwmp_bwmap_grt_ff_cfg_t grt_ff_sts;
    char *ff_sel_str[4] = {
        "BWMAP Grant",
        "XGTC Grant",
        "BM Grant",
        "Reserved"
    };
        
    memset(&grf_ff_cfg, 0, sizeof(aal_xgpon_bwmp_grt_ff_cfg_t));
    memset(&grt_ff_sts, 0, sizeof(aal_xgpon_bwmp_bwmap_grt_ff_cfg_t));

    /*if(tcont_id > 31) {
        ca_printf("Invalid parameter tcont_id :%d, should be 0~31\r\n",tcont_id);
        return CA_E_PARAM;
    }*/

    if(grant_ff_sel > 3) {
        ca_printf("Invalid parameter grant_ff_sel :%d, should be 0~3\r\n",grant_ff_sel);
        return CA_E_PARAM;
    }

    ca_printf("%s information:\r\n", ff_sel_str[grant_ff_sel]);
    ca_printf("%-4s %-6s %-5s(%-6s) %-10s %-10s %-9s %-4s %-6s %-4s\r\n",
        "FIFO", "GrantID", "TCONT", "enable", "GrantSize", "StartTime", "BurstProf", "FECu", "PLOAMu", "DBRu");
    ca_printf("------------------------------------------------------------------------------\r\n");

    if(tcont_id == 32){

        for(fifo_id = 0; fifo_id < __XGPON_MAX_FIFOS; fifo_id++) {
            for(grant_id = 0; grant_id < __XGPON_MAX_GRANTS; grant_id++) {
                addr = (fifo_id << 5) | grant_id;
                ret = aal_xgpon_bwmp_grt_ff_ctrl_cfg_get(device_id,addr,grant_ff_sel,&grf_ff_cfg);
                CA_RET_VALUE_CHECK(ret);
                //ca_printf("%d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t%d\t%d\t%d\t%d\r\n",
                //ca_printf("%d\t%d\t\t%x\t\t%x\t\t%x\t\t%x\t%x\t%x\t%x\t%x\r\n",
                ca_printf("%-4u %-7u %-5u(%-6s) 0x%04x     0x%04x      %-8u  %-3u  %-5u  %-3u\r\n",
                    fifo_id, grant_id, grf_ff_cfg.tcont_id,  grf_ff_cfg.tcont_en ? "Yes" : "No",
                    grf_ff_cfg.grant_size,  grf_ff_cfg.st_tm, grf_ff_cfg.bp_ind,
                    grf_ff_cfg.fecu,  grf_ff_cfg.ploamu,  grf_ff_cfg.dbru);
            }
        }
    } else {
    
        for(fifo_id = 0; fifo_id < __XGPON_MAX_FIFOS; fifo_id++) {
            for(grant_id = 0; grant_id < __XGPON_MAX_GRANTS; grant_id++) {
                addr = (fifo_id << 5) | grant_id;
                ret = aal_xgpon_bwmp_grt_ff_ctrl_cfg_get(device_id,addr,grant_ff_sel,&grf_ff_cfg);
                CA_RET_VALUE_CHECK(ret);
                if(grf_ff_cfg.tcont_id == tcont_id){
                    ca_printf("%-4u %-7u %-5u(%-6s) 0x%04x     0x%04x      %-8u  %-3u  %-5u  %-3u\r\n",
                        fifo_id, grant_id, grf_ff_cfg.tcont_id,  grf_ff_cfg.tcont_en ? "Yes" : "No",
                        grf_ff_cfg.grant_size,  grf_ff_cfg.st_tm, grf_ff_cfg.bp_ind,
                        grf_ff_cfg.fecu,  grf_ff_cfg.ploamu,  grf_ff_cfg.dbru);

                }
            }
        }
    }
    ca_printf("------------------------------------------------------------------------------\r\n");
    ret = aal_xgpon_bwmp_bwmap_grant_ff_sts_get(device_id, &grt_ff_sts);
    CA_RET_VALUE_CHECK(ret);
    ca_printf("FIFO pointer status:\r\n");
    ca_printf(" rd_sel: 0x%04x\r\n rd_ptr: 0x%04x\r\n wr_sel: 0x%04x\r\n wr_ptr: 0x%04x\r\n", grt_ff_sts.rd_sel, grt_ff_sts.rd_ptr, grt_ff_sts.wr_sel, grt_ff_sts.wr_ptr);
    return ret;

}

ca_status_t aal_xgpon_aes_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t origin_key[16] = {0};
    aal_xgpon_dsfp_xgem_key_ctrl_config_msk_t key_ctrl_mask;
    aal_xgpon_dsfp_xgem_key_ctrl_config_t key_ctrl_cfg;

    aal_xgpon_usbg_gem_config_msk_t usbg_xgem_mask;
    aal_xgpon_usbg_gem_config_t usbg_xgem_cfg;
    key_ctrl_mask.u32 = 0;
    usbg_xgem_mask.u32 = 0;

    memset(&key_ctrl_cfg,0,sizeof(aal_xgpon_dsfp_xgem_key_ctrl_config_t));
    memset(&usbg_xgem_cfg, 0 , sizeof(aal_xgpon_usbg_gem_config_t));
    //ca_printf("\r\nvalue1 1: Set value2 to aal_aes_debug\r\n ");

    switch (value1){

        case 1:
            aal_xgpon_aes_debug = value2;
            ca_printf("xgpon aes debug set: %d\r\n", aal_xgpon_aes_debug);
            break;
        case 2:
            switch (value2){
                case 1:
                    ca_printf("\r\nxgpon aes debug set: clear g_key_ctl_new_cnt and g_key_ctl_cfm_cnt!\r\n");
                    g_key_ctl_new_cnt = 0;
                    g_key_ctl_cfm_cnt = 0;
                    g_rpt_new_key_cnt = 0;
                    g_rpt_ext_key_cnt = 0;

                    ca_printf("xgpon aes debug set: set aes state to default KN0 and disable both keys!\r\n");
                    previous_key_exchange_state = CA_XGPON_ONU_KN0;
                    current_key_exchange_state = CA_XGPON_ONU_KN0;
                    key_ctrl_mask.s.key0_idx_vld = 1;
                    key_ctrl_mask.s.key1_idx_vld = 1;
                    key_ctrl_cfg.key0_idx_vld = 0;
                    key_ctrl_cfg.key1_idx_vld = 0;
                    ret = aal_xgpon_dsfp_xgem_key_ctrl_set(0,key_ctrl_mask,&key_ctrl_cfg);
                    CA_RET_VALUE_CHECK(ret);

                    ret = aal_xgpon_usbg_xgem_cfg_get(0,1,&usbg_xgem_cfg);
                    CA_RET_VALUE_CHECK(ret);

                    if(usbg_xgem_cfg.aes_mode){

                        usbg_xgem_mask.s.aes_mode = 1;
                        usbg_xgem_cfg.aes_mode = 0;

                        /*config usbg xgem aes mode enable*/

                        ret = aal_xgpon_usbg_xgem_cfg_set(0,1,usbg_xgem_mask,&usbg_xgem_cfg);
                        CA_RET_VALUE_CHECK(ret);
                    }
                    break;
                case 3:
                    ca_printf("xgpon aes debug set: decrypt kek and kek name!\r\n");
                    ca_buffer_dump("AES_kek", kek, XGPN_AES_KEY_LEN);
                    ca_buffer_dump("AES_encryped_key", g_encryped_key, XGPN_AES_KEY_LEN);
                    aes128_ecb_decrypt(kek,g_encryped_key, origin_key);
                    ca_buffer_dump("AES_origin_key", origin_key, XGPN_AES_KEY_LEN);
                   break;
                default:
                    break;
            }
            break;
        case 3:
            ca_printf("Enable aal_xgpn_aes_ukey_chg_flag for changing uc key manually.\r\n");
            aal_xgpn_aes_ukey_chg_flag = value2;
            break;

        case 4:
            aal_xgpn_aes_rpt_hw_accelerate = value2;
            ca_printf("set aal_xgpn_aes_rpt_hw_accelerate to be %d.\r\n", aal_xgpn_aes_rpt_hw_accelerate);
            break;

        case 5:
            aal_xgpon_aes_sm_debug = value2;
            ca_printf("aal_xgpon_aes_sm_debug: %d\r\n", aal_xgpon_aes_sm_debug);
            break;

        default:
            ca_printf("Usage:\r\n");
            ca_printf("value1 = 0 : show help \r\n");
            ca_printf("value1 = 1 : set AES debug flag \r\n");
            ca_printf("value1 = 2 : set AES key configurations \r\n");
            ca_printf("value1 = 3 : set AES UC Key change manually flag \r\n");
            ca_printf("value1 = 4 : set AES report hardware accelerate enable \r\n");
            ca_printf("value1 = 5 : set AES StateMachine debug flag \r\n");

            break;
    }

    return CA_E_OK;
}
extern void xgpon_aes_dbg_show(void);

ca_status_t aal_xgpon_aes_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
)
{
    *value2 = aal_xgpon_aes_debug;
    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_state_get(
    CA_IN ca_device_id_t device_id
)
{
    //ca_printf("xgpon aes debug get   : %u\r\n", aal_xgpon_aes_debug);
    xgpon_aes_dbg_show();
    g_key_ctl_new_cnt = 0;
    g_key_ctl_cfm_cnt = 0;
	g_rpt_new_key_cnt = 0;
	g_rpt_ext_key_cnt = 0;

    return CA_E_OK;
}

ca_status_t aal_gpon_glb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_gpon_glb_ctrl_config_t *glb_ctrl_cfg)
{
    GLOBAL_GPON_CNTL_t xgpn_glb_ctrl;

    XGPON_ASSERT(glb_ctrl_cfg);

    xgpn_glb_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_GPON_CNTL);

    glb_ctrl_cfg->gpon_rst_n = xgpn_glb_ctrl.bf.gpon_rst_n;
    glb_ctrl_cfg->us10g_mode = xgpn_glb_ctrl.bf.us10g_mode;
    glb_ctrl_cfg->ani_rst_n  = xgpn_glb_ctrl.bf.ani_rst_n;
    glb_ctrl_cfg->xgpn_rst_n = xgpn_glb_ctrl.bf.xgpn_rst_n;
    glb_ctrl_cfg->nxgs_mode  = xgpn_glb_ctrl.bf.nxgs_mode;

    return CA_E_OK;
}

ca_status_t aal_gpon_glb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_gpon_glb_ctrl_config_msk_t mask,
    CA_IN aal_gpon_glb_ctrl_config_t *glb_ctrl_cfg)
{
    GLOBAL_GPON_CNTL_t xgpn_glb_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }

    XGPON_ASSERT(glb_ctrl_cfg);

    xgpn_glb_ctrl.wrd = AAL_GLB_REG_READ(GLOBAL_GPON_CNTL);

    if (mask.s.gpon_rst_n) {
        xgpn_glb_ctrl.bf.gpon_rst_n = glb_ctrl_cfg->gpon_rst_n;
    }
    if (mask.s.us10g_mode) {
        xgpn_glb_ctrl.bf.us10g_mode = glb_ctrl_cfg->us10g_mode ;
    }
    if (mask.s.ani_rst_n) {
        xgpn_glb_ctrl.bf.ani_rst_n = glb_ctrl_cfg->ani_rst_n;
    }
    if (mask.s.xgpn_rst_n) {
        xgpn_glb_ctrl.bf.xgpn_rst_n = glb_ctrl_cfg->xgpn_rst_n;
    }
    if (mask.s.nxgs_mode) {
        xgpn_glb_ctrl.bf.nxgs_mode = glb_ctrl_cfg->nxgs_mode;
    }

    AAL_GLB_REG_WRITE(xgpn_glb_ctrl.wrd, GLOBAL_GPON_CNTL);

    return CA_E_OK;
}


ca_status_t aal_xgpon_aes_plmp_glb_ctrl_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_OUT      aal_xgpon_aes_plmp_gbl_ctrl_t *aes_plmp_gbl_ctrl)
{
    XGPN_AES_PLMP_GBL_CTRL_t aes_plmp_gbl_cfg;

    XGPON_ASSERT(aes_plmp_gbl_ctrl);

    aes_plmp_gbl_cfg.wrd = AAL_PON_REG_READ(XGPN_AES_PLMP_GBL_CTRL);

    aes_plmp_gbl_ctrl->sma_mode = aes_plmp_gbl_cfg.bf.sma_mode;
    aes_plmp_gbl_ctrl->key_hw_mode = aes_plmp_gbl_cfg.bf.key_hw_mode;
    aes_plmp_gbl_ctrl->msk_reg_id_src_sel = aes_plmp_gbl_cfg.bf.msk_reg_id_src_sel;
    aes_plmp_gbl_ctrl->key_len = aes_plmp_gbl_cfg.bf.key_len;

    return CA_E_OK;

}

ca_status_t aal_xgpon_aes_plmp_glb_ctrl_set(
    CA_IN       ca_device_id_t                 device_id,
    CA_IN       aal_xgpon_aes_plmp_gbl_ctrl_msk_t   mask,
    CA_IN       aal_xgpon_aes_plmp_gbl_ctrl_t *aes_plmp_gbl_ctrl)
{
    XGPN_AES_PLMP_GBL_CTRL_t aes_plmp_gbl_cfg;

    if (!mask.u32) {
        return CA_E_OK;
    }

    XGPON_ASSERT(aes_plmp_gbl_ctrl);

    AAL_XGPON_LOCK();

    aes_plmp_gbl_cfg.wrd = AAL_PON_REG_READ(XGPN_AES_PLMP_GBL_CTRL);

    if (mask.s.sma_mode) {
        aes_plmp_gbl_cfg.bf.sma_mode = aes_plmp_gbl_ctrl->sma_mode;
    }

    if (mask.s.key_hw_mode) {
        aes_plmp_gbl_cfg.bf.key_hw_mode = aes_plmp_gbl_ctrl->key_hw_mode;
    }

    if (mask.s.msk_reg_id_src_sel) {
        aes_plmp_gbl_cfg.bf.msk_reg_id_src_sel = aes_plmp_gbl_ctrl->msk_reg_id_src_sel;
    }

    if (mask.s.key_len) {
        aes_plmp_gbl_cfg.bf.key_len = aes_plmp_gbl_ctrl->key_len;
    }

    AAL_PON_REG_WRITE(aes_plmp_gbl_cfg.wrd, XGPN_AES_PLMP_GBL_CTRL);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_xgpon_aes_plmp_subkey_ctrl_get(
    CA_IN       ca_device_id_t                   device_id,
    CA_OUT      aal_xgpon_aes_plmp_subkey_ctrl_t *ctrl)
{
    XGPN_AES_PLMP_SUBKEY_CTRL_t regdata;

    CA_NULL_PTR_CHECK(ctrl);

    regdata.wrd = AAL_PON_REG_READ(XGPN_AES_PLMP_SUBKEY_CTRL);

    ctrl->gene_st       = regdata.bf.gene_st      ;
    ctrl->sk_sw_st      = regdata.bf.sk_sw_st     ;
    ctrl->ik_sw_st      = regdata.bf.ik_sw_st     ;
    ctrl->mic_k12_sw_st = regdata.bf.mic_k12_sw_st;

    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_plmp_subkey_ctrl_set(
    CA_IN      ca_device_id_t                   device_id,
    CA_IN      aal_xgpon_aes_plmp_subkey_ctrl_t *ctrl)
{
    XGPN_AES_PLMP_SUBKEY_CTRL_t regdata;

    CA_NULL_PTR_CHECK(ctrl);

    regdata.wrd = AAL_PON_REG_READ(XGPN_AES_PLMP_SUBKEY_CTRL);

    regdata.bf.gene_st       = ctrl->gene_st      ;
    regdata.bf.sk_sw_st      = ctrl->sk_sw_st     ;
    regdata.bf.ik_sw_st      = ctrl->ik_sw_st     ;
    regdata.bf.mic_k12_sw_st = ctrl->mic_k12_sw_st;
    
    AAL_PON_REG_WRITE(regdata.wrd, XGPN_AES_PLMP_SUBKEY_CTRL);
    return CA_E_OK;
}


ca_status_t aal_xgpon_aes_plmp_msk_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_OUT      ca_uint8_t              plmp_msk[])
{
    int i = 0;
    ca_uint32_t reg_addr = XGPN_AES_PLMP_MSK3_STS;
    XGPN_AES_PLMP_MSK3_STS_t reg_cfg;

    CA_NULL_PTR_CHECK(plmp_msk);

    reg_addr = XGPN_AES_PLMP_MSK3_STS;

	for (i = 0; i < (XGPN_AES_KEY_LEN >> 2); i++){
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        plmp_msk[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
        plmp_msk[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        plmp_msk[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        plmp_msk[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
        reg_addr += 4;
    }

    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_plmp_msk_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t plmp_msk[])
{
    XGPN_AES_PLMP_MSK3_STS_t plmp_msk3;
    XGPN_AES_PLMP_MSK2_STS_t plmp_msk2;
    XGPN_AES_PLMP_MSK1_STS_t plmp_msk1;
    XGPN_AES_PLMP_MSK0_STS_t plmp_msk0;

    CA_NULL_PTR_CHECK(plmp_msk);

    plmp_msk3.wrd = ((ca_uint32_t)(plmp_msk[0]) << 24)
            + ((ca_uint32_t)(plmp_msk[1]) << 16)
            + ((ca_uint32_t)(plmp_msk[2]) << 8)
            + ((ca_uint32_t)(plmp_msk[3]));
    plmp_msk2.wrd = ((ca_uint32_t)(plmp_msk[4]) << 24)
            + ((ca_uint32_t)(plmp_msk[5]) << 16)
            + ((ca_uint32_t)(plmp_msk[6]) << 8)
            + ((ca_uint32_t)(plmp_msk[7]));
    plmp_msk1.wrd = ((ca_uint32_t)(plmp_msk[8]) << 24)
            + ((ca_uint32_t)(plmp_msk[9]) << 16)
            + ((ca_uint32_t)(plmp_msk[10]) << 8)
            + ((ca_uint32_t)(plmp_msk[11]));
    plmp_msk0.wrd = ((ca_uint32_t)(plmp_msk[12]) << 24)
            + ((ca_uint32_t)(plmp_msk[13]) << 16)
            + ((ca_uint32_t)(plmp_msk[14]) << 8)
            + ((ca_uint32_t)(plmp_msk[15]));


    AAL_PON_REG_WRITE(plmp_msk3.wrd, XGPN_AES_PLMP_MSK3_STS);
    AAL_PON_REG_WRITE(plmp_msk2.wrd, XGPN_AES_PLMP_MSK2_STS);
    AAL_PON_REG_WRITE(plmp_msk1.wrd, XGPN_AES_PLMP_MSK1_STS);
    AAL_PON_REG_WRITE(plmp_msk0.wrd, XGPN_AES_PLMP_MSK0_STS);

    //regdata2.wrd = AAL_PON_REG_READ(XGPN_AES_PLMP_SUBKEY_CTRL);
    //regdata2.bf.sk_sw_st = 1;
    // AAL_PON_REG_WRITE( regdata2.wrd, XGPN_AES_PLMP_SUBKEY_CTRL);

    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_plmp_sk_get(
    CA_IN       ca_device_id_t                 device_id,
    CA_OUT      ca_uint8_t              plmp_sk[])
{
    int i, reg_addr;
    aal_xgpn_aes_key_cfg_t reg_cfg;

    CA_NULL_PTR_CHECK(plmp_sk);

    //AAL_XGPON_LOCK();

    reg_addr = XGPN_AES_PLMP_SK3_STS;

    for (i = 0; i < XGPN_AES_KEY_LEN/4; i++){
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        plmp_sk[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
        plmp_sk[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        plmp_sk[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        plmp_sk[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
        reg_addr += 4;
    }
    //AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_plmp_sk_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t plmp_sk[])
{
    int i, reg_addr;
    aal_xgpn_aes_key_cfg_t reg_cfg;
    XGPN_AES_PLMP_SUBKEY_CTRL_t   regdata2;

    XGPON_ASSERT(plmp_sk);
    AAL_XGPON_LOCK();
	
    reg_addr = XGPN_AES_PLMP_SK3_STS;

    for (i = 0; i < XGPN_AES_KEY_LEN/4; i++){
		reg_cfg.wrd = ((ca_uint32_t)(plmp_sk[i*4]) << 24 )
            | ((ca_uint32_t)(plmp_sk[i*4 + 1]) << 16)
            | ((ca_uint32_t)(plmp_sk[i*4 + 2]) << 8)
            | ((ca_uint32_t)(plmp_sk[i*4 + 3]));
        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        reg_addr += 4;
    }
	
	regdata2.wrd = AAL_PON_REG_READ(XGPN_AES_PLMP_SUBKEY_CTRL);
	regdata2.bf.ik_sw_st = 1;
    AAL_PON_REG_WRITE( regdata2.wrd, XGPN_AES_PLMP_SUBKEY_CTRL);
	
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_aes_plmp_mic_key_get(
    CA_IN       ca_device_id_t          device_id,
    CA_IN       ca_uint8_t              key_len,
    CA_OUT      ca_uint8_t              plmp_mic_key[])
{

    int i, reg_addr;
    aal_xgpn_aes_key_cfg_t reg_cfg;

    XGPON_ASSERT(plmp_mic_key);
    
    if(key_len != XGPN_AES_KEY_LEN){
        return CA_E_ERROR;
    }

    AAL_XGPON_LOCK();

    reg_addr = XGPN_AES_PLMP_MIC3_K1_STS;

    for (i = 0; i < XGPN_AES_KEY_LEN/4; i++){
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        plmp_mic_key[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
        plmp_mic_key[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        plmp_mic_key[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        plmp_mic_key[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
        reg_addr += 4;
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_plmp_mic_key_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t key_len,
    CA_IN ca_uint8_t plmp_mic_key[])
{
    int i, reg_addr;
    aal_xgpn_aes_key_cfg_t reg_cfg;
    XGPON_ASSERT(plmp_mic_key);

    if(key_len != XGPN_AES_KEY_LEN){
        return CA_E_ERROR;
    }
    AAL_XGPON_LOCK();

    reg_addr = XGPN_AES_PLMP_MIC3_K1_STS;

    for (i = 0; i < XGPN_AES_KEY_LEN/4; i++){
		reg_cfg.wrd = ((ca_uint32_t)(plmp_mic_key[i*4]) << 24 )
            | ((ca_uint32_t)(plmp_mic_key[i*4 + 1]) << 16)
            | ((ca_uint32_t)(plmp_mic_key[i*4 + 2]) << 8)
            | ((ca_uint32_t)(plmp_mic_key[i*4 + 3]));
        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        reg_addr += 4;
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_plmp_ik_get(
    CA_IN       ca_device_id_t          device_id,
    CA_OUT      ca_uint8_t              plmp_ik[])
{
    int i, reg_addr;
    aal_xgpn_aes_key_cfg_t reg_cfg;

    XGPON_ASSERT(plmp_ik);

    AAL_XGPON_LOCK();

    reg_addr = XGPN_AES_PLMP_IK3_STS;

    for (i = 0; i < XGPN_AES_KEY_LEN/4; i++){
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        plmp_ik[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
        plmp_ik[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        plmp_ik[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        plmp_ik[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
        reg_addr += 4;
    }
    AAL_XGPON_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_plmp_ik_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t plmp_ik[])
{
    int i, reg_addr;
    aal_xgpn_aes_key_cfg_t reg_cfg;
    XGPN_AES_PLMP_SUBKEY_CTRL_t   regdata2;

    CA_DEV_ID_CHECK(device_id);
    XGPON_ASSERT(plmp_ik);
	
	AAL_XGPON_LOCK();

    reg_addr = XGPN_AES_PLMP_IK3_STS;
    for (i = 0; i < XGPN_AES_KEY_LEN/4; i++){
		reg_cfg.wrd = ((ca_uint32_t)(plmp_ik[i*4]) << 24 )
            | ((ca_uint32_t)(plmp_ik[i*4 + 1]) << 16)
            | ((ca_uint32_t)(plmp_ik[i*4 + 2]) << 8)
            | ((ca_uint32_t)(plmp_ik[i*4 + 3]));
        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        reg_addr += 4;
    }
	regdata2.wrd = AAL_PON_REG_READ(XGPN_AES_PLMP_SUBKEY_CTRL);
	regdata2.bf.mic_k12_sw_st = 1;
    AAL_PON_REG_WRITE( regdata2.wrd, XGPN_AES_PLMP_SUBKEY_CTRL);
	
	AAL_XGPON_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_uc_enc_key_get(
    CA_IN  ca_device_id_t                 device_id,
    CA_IN  aal_xgpon_enc_key_id_t   key_idx,
    CA_OUT aal_uc_enc_key_config_t *config)
{
    int i, reg_addr;
    XGPN_AES_UC_KEY03_t reg_cfg;
    ca_uint8_t key_idx_tmp = 0;
    XGPN_USBG_AES_UNI_KEY_IDX_CTRL_t regdata0;
    XGPN_AES_PLMP_GBL_CTRL_t         regdata1;

    CA_DEV_ID_CHECK(device_id);
    CA_AAL_XGPON_KEY_IDX_CHECK(key_idx);
    XGPON_ASSERT(config);

    AAL_XGPON_LOCK();

    regdata0.wrd = AAL_PON_REG_READ( XGPN_USBG_AES_UNI_KEY_IDX_CTRL);
    regdata1.wrd = AAL_PON_REG_READ( XGPN_AES_PLMP_GBL_CTRL);

    key_idx_tmp = regdata0.bf.key_idx;

    /* *p_effective_key_length = regdata1.bf.key_len;*/

    config->p_effective_key_length = 16;
    if (key_idx_tmp == key_idx) {
        config->p_key_active = 1;
    } else {
        config->p_key_active = 0;
    }

    reg_addr = (0 == key_idx) ? XGPN_AES_UC_KEY03 : XGPN_AES_UC_KEY13;

    for (i = 0; i < 4; i++){
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        config->uc_key[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
        config->uc_key[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        config->uc_key[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        config->uc_key[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
        reg_addr += 4;
    }

    AAL_XGPON_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_uc_enc_key_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_enc_key_id_t    key_idx,
    CA_IN aal_uc_enc_key_config_msk_t mask,
    CA_IN aal_uc_enc_key_config_t *config)
{
    int i, reg_addr;
    XGPN_AES_UC_KEY03_t reg_cfg;
    XGPN_USBG_AES_UNI_KEY_IDX_CTRL_t regdata0;
    XGPN_AES_PLMP_GBL_CTRL_t         regdata1;

    CA_DEV_ID_CHECK(device_id);
    XGPON_ASSERT(config);

    CA_AAL_XGPON_KEY_IDX_CHECK(key_idx);
    CA_AAL_XGON_BOOL_VAL_CHECK(config->p_key_active);
    CA_AAL_XGPON_KEY_LEN_CHECK(config->p_effective_key_length);

    if (!mask.u32) {
        return CA_E_OK;
    }

    //AAL_XGPON_LOCK();
    regdata0.wrd = AAL_PON_REG_READ( XGPN_USBG_AES_UNI_KEY_IDX_CTRL);
    regdata1.wrd = AAL_PON_REG_READ( XGPN_AES_PLMP_GBL_CTRL);


    if (mask.s.p_key_active) {
        if (config->p_key_active)
            regdata0.bf.key_idx = key_idx;
    }
    if (mask.s.p_effective_key_length)
        regdata1.bf.key_len = config->p_effective_key_length;


    AAL_PON_REG_WRITE(regdata0.wrd, XGPN_USBG_AES_UNI_KEY_IDX_CTRL);
    AAL_PON_REG_WRITE(regdata1.wrd, XGPN_AES_PLMP_GBL_CTRL);

    reg_addr = (0 == key_idx) ? XGPN_AES_UC_KEY03 : XGPN_AES_UC_KEY13;

    for (i = 0; i < 4; i++){
        reg_cfg.wrd = ((ca_uint32_t)(config->uc_key[i*4]) << 24 )
            | ((ca_uint32_t)(config->uc_key[i*4 + 1]) << 16)
            | ((ca_uint32_t)(config->uc_key[i*4 + 2]) << 8)
            | ((ca_uint32_t)(config->uc_key[i*4 + 3]));
        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        reg_addr += 4;
    }

    //AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_mc_enc_key_get(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  aal_xgpon_enc_key_id_t   key_idx,
    CA_OUT aal_mc_enc_key_config_t *config)

{
    int i, reg_addr;
    aal_xgpn_aes_key_cfg_t reg_cfg;
    XGPN_DSFP_xgem_mc_key_ctl_t regdata0;
    XGPN_AES_PLMP_GBL_CTRL_t regdata1;

    CA_DEV_ID_CHECK(device_id);
    CA_AAL_XGPON_KEY_IDX_CHECK(key_idx);
    XGPON_ASSERT(config);

    AAL_XGPON_LOCK();
    regdata0.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgem_mc_key_ctl); 
    regdata1.wrd = AAL_PON_REG_READ( XGPN_AES_PLMP_GBL_CTRL);
    if(key_idx){
        config->p_key_active = regdata0.bf.key1_idx_vld;
    }else{
        config->p_key_active = regdata0.bf.key0_idx_vld;
    }
    /* *p_effective_key_length = regdata1.bf.key_len;*/

    config->p_effective_key_length = 16;

    reg_addr = (0 == key_idx) ? XGPN_AES_MC_KEY03 : XGPN_AES_MC_KEY13;

    for (i = 0; i < XGPN_AES_KEY_LEN/4; i++){
        reg_cfg.wrd  = AAL_PON_REG_READ(reg_addr);
        config->mc_key[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
        config->mc_key[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
        config->mc_key[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
        config->mc_key[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
        reg_addr += 4;
    }

    AAL_XGPON_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_xgpon_aes_mc_enc_key_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_enc_key_id_t    key_idx,
    CA_IN aal_mc_enc_key_config_msk_t mask,
    CA_IN aal_mc_enc_key_config_t *config)
{
    int i, reg_addr;
    aal_xgpn_aes_key_cfg_t reg_cfg;
    XGPN_DSFP_xgem_mc_key_ctl_t regdata0;
    XGPN_AES_PLMP_GBL_CTRL_t regdata1;
    
    CA_DEV_ID_CHECK(device_id);
    XGPON_ASSERT(config);

    CA_AAL_XGPON_KEY_IDX_CHECK(key_idx);
    CA_AAL_XGPON_KEY_LEN_CHECK(config->p_effective_key_length);

    if (!mask.u32) {
        return CA_E_OK;
    }
    AAL_XGPON_LOCK();
    regdata0.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgem_mc_key_ctl);
    regdata1.wrd = AAL_PON_REG_READ( XGPN_AES_PLMP_GBL_CTRL);
    
    if (mask.s.p_effective_key_length){
        regdata1.bf.key_len = config->p_effective_key_length;
    }
    if (mask.s.p_key_active){
        if(key_idx){
            regdata0.bf.key0_idx_vld = 0;
            regdata0.bf.key1_idx_vld = config->p_key_active;
        }else{
            regdata0.bf.key0_idx_vld = config->p_key_active;
            regdata0.bf.key1_idx_vld = 0;
        }
    }
    
    reg_addr = (0 == key_idx) ? XGPN_AES_MC_KEY03 : XGPN_AES_MC_KEY13;

    for (i = 0; i < XGPN_AES_KEY_LEN/4; i++){
        reg_cfg.wrd = ((ca_uint32_t)(config->mc_key[i*4]) << 24 )
            | ((ca_uint32_t)(config->mc_key[i*4 + 1]) << 16)
            | ((ca_uint32_t)(config->mc_key[i*4 + 2]) << 8)
            | ((ca_uint32_t)(config->mc_key[i*4 + 3]));
        AAL_PON_REG_WRITE(reg_cfg.wrd, reg_addr);
        reg_addr += 4;
    }
    
    AAL_PON_REG_WRITE( regdata0.wrd,XGPN_DSFP_xgem_mc_key_ctl);
    AAL_PON_REG_WRITE( regdata1.wrd,XGPN_AES_PLMP_GBL_CTRL);
    
    AAL_XGPON_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_xgpon_xgpn_glb_ver_id_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *revid)
{
    XGPN_XGPN_GLB_VERSION_ID_t xgpn_ver_id;
    XGPON_ASSERT(revid);

    xgpn_ver_id.wrd = AAL_PON_REG_READ (XGPN_XGPN_GLB_VERSION_ID);
    *revid = xgpn_ver_id.bf.revid;

    return CA_E_OK;
}

ca_status_t aal_xgpon_xgpn_glb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_xgpn_glb_ctrl_config_t *glb_ctrl_cfg)
{
    XGPN_XGPN_GLB_CTRL_t xgpn_glb_ctrl;

    XGPON_ASSERT(glb_ctrl_cfg);

    xgpn_glb_ctrl.wrd = AAL_PON_REG_READ( XGPN_XGPN_GLB_CTRL);

    glb_ctrl_cfg->mac_en = xgpn_glb_ctrl.bf.mac_en;
    glb_ctrl_cfg->power_saving_mode = xgpn_glb_ctrl.bf.power_saving_mode;
    glb_ctrl_cfg->pm_mode = xgpn_glb_ctrl.bf.pm_mode;
    glb_ctrl_cfg->us_line_rate = xgpn_glb_ctrl.bf.us_line_rate;
    glb_ctrl_cfg->uwlch_id = xgpn_glb_ctrl.bf.uwlch_id;
    glb_ctrl_cfg->flush_id = xgpn_glb_ctrl.bf.flush_id;
    glb_ctrl_cfg->flush_en = xgpn_glb_ctrl.bf.flush_en;

    return CA_E_OK;
}

ca_status_t aal_xgpon_xgpn_glb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_xgpn_glb_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_xgpn_glb_ctrl_config_t *glb_ctrl_cfg)
{
    XGPN_XGPN_GLB_CTRL_t xgpn_glb_ctrl, xgpn_glb_ctrl_old;

    if (!mask.u32) {
        return CA_E_OK;
    }

    CA_NULL_PTR_CHECK(glb_ctrl_cfg);

    //AAL_XGPON_LOCK();

    xgpn_glb_ctrl.wrd = AAL_PON_REG_READ( XGPN_XGPN_GLB_CTRL);
    xgpn_glb_ctrl_old.wrd = xgpn_glb_ctrl.wrd;
    if (mask.s.mac_en) {
        xgpn_glb_ctrl.bf.mac_en = glb_ctrl_cfg->mac_en;
    }
    if (mask.s.power_saving_mode) {
        xgpn_glb_ctrl.bf.power_saving_mode = glb_ctrl_cfg->power_saving_mode ;
    }
    if (mask.s.pm_mode) {
        xgpn_glb_ctrl.bf.pm_mode = glb_ctrl_cfg->pm_mode  ;
    }
    if (mask.s.us_line_rate) {
        xgpn_glb_ctrl.bf.us_line_rate = glb_ctrl_cfg->us_line_rate  ;
    }

    if (mask.s.uwlch_id) {
        xgpn_glb_ctrl.bf.uwlch_id = glb_ctrl_cfg->uwlch_id  ;
    }
    if (mask.s.flush_id) {
        xgpn_glb_ctrl.bf.flush_id = glb_ctrl_cfg->flush_id  ;
    }
    if (mask.s.flush_en) {
        xgpn_glb_ctrl.bf.flush_en = glb_ctrl_cfg->flush_en  ;
    }

    if (xgpn_glb_ctrl_old.wrd != xgpn_glb_ctrl.wrd) {
        AAL_PON_REG_WRITE (xgpn_glb_ctrl.wrd, XGPN_XGPN_GLB_CTRL);
    }
    //AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_xgpn_glb_dbg_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t dbg_type,
    CA_OUT ca_uint64_t *val)
{
    ca_status_t ret = CA_E_OK;
    XGPN_XGPN_GLB_DEBUG_CTRL_t xgpn_glb_dbg_ctrl;
    XGPN_XGPN_GLB_DEBUG_STATUS0_t dbg_sts0;
    XGPN_XGPN_GLB_DEBUG_STATUS0_t dbg_sts1;
    ca_uint64_t val_tmp = 0;

    xgpn_glb_dbg_ctrl.wrd = 0;
    dbg_sts0.wrd = 0;
    dbg_sts1.wrd = 0;
    if (dbg_type > 12)
        return CA_E_PARAM;

    XGPON_ASSERT(val);

    AAL_XGPON_LOCK();

    xgpn_glb_dbg_ctrl.bf.sel = dbg_type;

    AAL_PON_REG_WRITE (xgpn_glb_dbg_ctrl.wrd, XGPN_XGPN_GLB_DEBUG_CTRL);

    dbg_sts0.wrd = AAL_PON_REG_READ( XGPN_XGPN_GLB_DEBUG_STATUS0);
    dbg_sts1.wrd = AAL_PON_REG_READ( XGPN_XGPN_GLB_DEBUG_STATUS1);
    val_tmp = dbg_sts1.bf.value;
    *val = dbg_sts0.bf.value | (val_tmp << 32);
    AAL_XGPON_UNLOCK();

    return ret;
}

ca_status_t aal_xgpon_dsfp_hec_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_hec_ctrl_config_t *hec_ctrl_cfg)
{
    XGPN_DSFP_hec_ctl_t hec_ctrl;

    XGPON_ASSERT(hec_ctrl_cfg);

    hec_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_hec_ctl);

    hec_ctrl_cfg->hec0_en = hec_ctrl.bf.hec0_en;
    hec_ctrl_cfg->hec1_en = hec_ctrl.bf.hec1_en;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_hec_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_hec_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_hec_ctrl_config_t *hec_ctrl_cfg)
{
    XGPN_DSFP_hec_ctl_t hec_ctrl;


    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(hec_ctrl_cfg);

    AAL_XGPON_LOCK();

    hec_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_hec_ctl);
    if (mask.s.hec0_en) {
        if (hec_ctrl.bf.hec0_en != hec_ctrl_cfg->hec0_en)
            hec_ctrl.bf.hec0_en = hec_ctrl_cfg->hec0_en;
    }
    if (mask.s.hec1_en) {
        if (hec_ctrl.bf.hec1_en != hec_ctrl_cfg->hec1_en)
            hec_ctrl.bf.hec1_en = hec_ctrl_cfg->hec1_en ;
    }
    AAL_PON_REG_WRITE (hec_ctrl.wrd, XGPN_DSFP_hec_ctl);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}



ca_status_t aal_xgpon_dsfp_sync_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_sync_ctrl_config_t *sync_ctrl_cfg)
{
    XGPN_DSFP_sync_ctl_t sync_ctrl;

    XGPON_ASSERT(sync_ctrl_cfg);

    sync_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_sync_ctl);

    sync_ctrl_cfg->ds_descramble_en = sync_ctrl.bf.ds_descramble_en;

    sync_ctrl_cfg->sfc_hecchk_en = sync_ctrl.bf.sfc_hecchk_en;

    sync_ctrl_cfg->ponid_hecchk_en = sync_ctrl.bf.ponid_hecchk_en;

    sync_ctrl_cfg->psbd_err_allow = sync_ctrl.bf.psbd_err_allow;

    sync_ctrl_cfg->resync_loss_frame_num = sync_ctrl.bf.resync_loss_frame_num;

    sync_ctrl_cfg->byte_swap_en = sync_ctrl.bf.byte_swap_en;

    sync_ctrl_cfg->bit_swap_en = sync_ctrl.bf.bit_swap_en;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_sync_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_sync_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_sync_ctrl_config_t *sync_ctrl_cfg)
{
    XGPN_DSFP_sync_ctl_t sync_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(sync_ctrl_cfg);

    AAL_XGPON_LOCK();
    sync_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_sync_ctl);
    if (mask.s.ds_descramble_en) {
        if (sync_ctrl.bf.ds_descramble_en != sync_ctrl_cfg->ds_descramble_en)
            sync_ctrl.bf.ds_descramble_en = sync_ctrl_cfg->ds_descramble_en;
    }
    if (mask.s.sfc_hecchk_en) {
        if (sync_ctrl.bf.sfc_hecchk_en != sync_ctrl_cfg->sfc_hecchk_en)
            sync_ctrl.bf.sfc_hecchk_en = sync_ctrl_cfg->sfc_hecchk_en ;
    }
    if (mask.s.ponid_hecchk_en) {
        if (sync_ctrl.bf.ponid_hecchk_en != sync_ctrl_cfg->ponid_hecchk_en)
            sync_ctrl.bf.ponid_hecchk_en = sync_ctrl_cfg->ponid_hecchk_en ;
    }
    if (mask.s.psbd_err_allow) {
        if (sync_ctrl.bf.psbd_err_allow != sync_ctrl_cfg->psbd_err_allow)
            sync_ctrl.bf.psbd_err_allow = sync_ctrl_cfg->psbd_err_allow  ;
    }
    if (mask.s.resync_loss_frame_num) {
        if (sync_ctrl.bf.resync_loss_frame_num != sync_ctrl_cfg->resync_loss_frame_num)
            sync_ctrl.bf.resync_loss_frame_num = sync_ctrl_cfg->resync_loss_frame_num ;
    }
    if (mask.s.byte_swap_en) {
        if (sync_ctrl.bf.byte_swap_en != sync_ctrl_cfg->byte_swap_en)
            sync_ctrl.bf.byte_swap_en = sync_ctrl_cfg->byte_swap_en  ;
    }
    if (mask.s.bit_swap_en) {
        if (sync_ctrl.bf.bit_swap_en != sync_ctrl_cfg->bit_swap_en)
            sync_ctrl.bf.bit_swap_en = sync_ctrl_cfg->bit_swap_en ;
    }

    AAL_PON_REG_WRITE (sync_ctrl.wrd, XGPN_DSFP_sync_ctl);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_dsfp_xgtc_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_xgtc_ctrl_config_t *xgtc_ctrl_cfg)
{
    XGPN_DSFP_xgtc_ctl_t xgtc_ctrl;

    XGPON_ASSERT(xgtc_ctrl_cfg);

    xgtc_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgtc_ctl);
    xgtc_ctrl_cfg->hec_chk_en = xgtc_ctrl.bf.hec_chk_en;
    xgtc_ctrl_cfg->bwmp_en = xgtc_ctrl.bf.bwmp_en;
    xgtc_ctrl_cfg->ploam_en = xgtc_ctrl.bf.ploam_en;
    xgtc_ctrl_cfg->ds_en = xgtc_ctrl.bf.ds_en;
    xgtc_ctrl_cfg->drop_xgtc_frame_en = xgtc_ctrl.bf.drop_xgtc_frame_en;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_xgtc_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_xgtc_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgtc_ctrl_config_t *xgtc_ctrl_cfg)
{
    XGPN_DSFP_xgtc_ctl_t xgtc_ctrl, xgtc_ctrl_old;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(xgtc_ctrl_cfg);

    xgtc_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgtc_ctl);
    xgtc_ctrl_old.wrd = xgtc_ctrl.wrd;

    if (mask.s.hec_chk_en) {
        xgtc_ctrl.bf.hec_chk_en = xgtc_ctrl_cfg->hec_chk_en;
    }
    if (mask.s.bwmp_en) {
        xgtc_ctrl.bf.bwmp_en = xgtc_ctrl_cfg->bwmp_en ;
    }
    if (mask.s.ploam_en) {
        xgtc_ctrl.bf.ploam_en = xgtc_ctrl_cfg->ploam_en ;
    }
    if (mask.s.ds_en) {
        xgtc_ctrl.bf.ds_en = xgtc_ctrl_cfg->ds_en  ;
    }
    if (mask.s.drop_xgtc_frame_en) {
        xgtc_ctrl.bf.drop_xgtc_frame_en = xgtc_ctrl_cfg->drop_xgtc_frame_en ;
    }
    if (xgtc_ctrl_old.wrd != xgtc_ctrl.wrd) {
        AAL_PON_REG_WRITE (xgtc_ctrl.wrd, XGPN_DSFP_xgtc_ctl);
    }

    return CA_E_OK;
}


ca_status_t aal_xgpon_dsfp_xgem_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_xgem_ctrl_config_t *xgem_ctrl_cfg)
{
    XGPN_DSFP_xgem_ctl_t xgem_ctrl;

    XGPON_ASSERT(xgem_ctrl_cfg);

    xgem_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgem_ctl);

    xgem_ctrl_cfg->hec_chk_en = xgem_ctrl.bf.hec_chk_en;
    xgem_ctrl_cfg->invld_key_xgem_drop_en = xgem_ctrl.bf.invld_key_xgem_drop_en;
    xgem_ctrl_cfg->old_key_xgem_drop_en = xgem_ctrl.bf.old_key_xgem_drop_en;
    xgem_ctrl_cfg->los_word_cnt_clr_en = xgem_ctrl.bf.los_word_cnt_clr_en;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_xgem_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_xgem_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgem_ctrl_config_t *xgem_ctrl_cfg)
{
    ca_status_t ret = CA_E_OK;
    XGPN_DSFP_xgem_ctl_t xgem_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(xgem_ctrl_cfg);

    AAL_XGPON_LOCK();

    xgem_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgem_ctl);
    if (mask.s.hec_chk_en) {
        xgem_ctrl.bf.hec_chk_en = xgem_ctrl_cfg->hec_chk_en;
    }
    if (mask.s.invld_key_xgem_drop_en) {
        xgem_ctrl.bf.invld_key_xgem_drop_en = xgem_ctrl_cfg->invld_key_xgem_drop_en ;
    }
    if (mask.s.old_key_xgem_drop_en) {
        xgem_ctrl.bf.old_key_xgem_drop_en = xgem_ctrl_cfg->old_key_xgem_drop_en ;
    }
    if (mask.s.los_word_cnt_clr_en) {
        xgem_ctrl.bf.los_word_cnt_clr_en = xgem_ctrl_cfg->los_word_cnt_clr_en  ;
    }

    AAL_PON_REG_WRITE (xgem_ctrl.wrd, XGPN_DSFP_xgem_ctl);
    AAL_XGPON_UNLOCK();

    return ret;
}


ca_status_t aal_xgpon_dsfp_rxmac_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_rxmac_ctrl_config_t *rxmac_ctrl_cfg)
{
    XGPN_DSFP_rxmac_ctl_t rxmac_ctrl;
    XGPN_DSFP_rxmac_ctl1_t rxmac_ctrl1;

    XGPON_ASSERT(rxmac_ctrl_cfg);

    rxmac_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_rxmac_ctl);
    rxmac_ctrl1.wrd = AAL_PON_REG_READ( XGPN_DSFP_rxmac_ctl1);

    rxmac_ctrl_cfg->crc_chk_en = rxmac_ctrl.bf.crc_chk_en;
    rxmac_ctrl_cfg->omci_min_pkt_size = rxmac_ctrl.bf.omci_min_pkt_size;
    rxmac_ctrl_cfg->max_pkt_size = rxmac_ctrl.bf.max_pkt_size;
    rxmac_ctrl_cfg->omci_max_pkt_size = rxmac_ctrl1.bf.max_pkt_size;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_rxmac_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_rxmac_ctrl_config_msk_t mask,
    CA_IN  aal_xgpon_dsfp_rxmac_ctrl_config_t *rxmac_ctrl_cfg)
{
    ca_status_t ret = CA_E_OK;
    XGPN_DSFP_rxmac_ctl_t rxmac_ctrl, rxmac_ctrl_old;
    XGPN_DSFP_rxmac_ctl1_t rxmac_ctrl1;

    if (!mask.u32) {
        return CA_E_OK;
    }

    rxmac_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_rxmac_ctl);
    rxmac_ctrl_old.wrd = rxmac_ctrl.wrd;

    if (mask.s.crc_chk_en) {
        rxmac_ctrl.bf.crc_chk_en = rxmac_ctrl_cfg->crc_chk_en ? 1 : 0;
    }

    if (mask.s.omci_min_pkt_size) {
        if (rxmac_ctrl_cfg->omci_min_pkt_size > 63 || rxmac_ctrl_cfg->omci_min_pkt_size < AAL_XGPON_MIN_OMCI_PACKET_LEN) {
            ca_printf("%s(): invalid omci_min_pkt_size %u\n", __func__, rxmac_ctrl_cfg->omci_min_pkt_size);
            return CA_E_PARAM;
        } else {
            rxmac_ctrl.bf.omci_min_pkt_size = rxmac_ctrl_cfg->omci_min_pkt_size ;
        }
    }

    if (mask.s.max_pkt_size) {
        if (rxmac_ctrl_cfg->max_pkt_size > AAL_XGPON_RX_MAX_FRAME_SIZE || 
            rxmac_ctrl_cfg->max_pkt_size < AAL_XGPON_RX_MIN_FRAME_SIZE) 
        {
            ca_printf("%s(): invalid max_pkt_size %u\n", __func__, rxmac_ctrl_cfg->max_pkt_size);
            return CA_E_PARAM;
        } else {
            rxmac_ctrl.bf.max_pkt_size = rxmac_ctrl_cfg->max_pkt_size;
        }
    }

    if (rxmac_ctrl_old.wrd != rxmac_ctrl.wrd) {
        AAL_PON_REG_WRITE (rxmac_ctrl.wrd, XGPN_DSFP_rxmac_ctl);
    }

    if (mask.s.omci_max_pkt_size) {
        if (rxmac_ctrl_cfg->omci_max_pkt_size > 2047 
            || rxmac_ctrl_cfg->omci_max_pkt_size < 40
            || rxmac_ctrl_cfg->omci_max_pkt_size < rxmac_ctrl.bf.omci_min_pkt_size) {
            ca_printf("%s(): invalid omci_max_pkt_size %u\n", __func__, rxmac_ctrl_cfg->omci_max_pkt_size);
            return CA_E_PARAM;
        }
        rxmac_ctrl1.wrd = AAL_PON_REG_READ( XGPN_DSFP_rxmac_ctl1);
        rxmac_ctrl1.bf.max_pkt_size = rxmac_ctrl_cfg->omci_max_pkt_size;
        AAL_PON_REG_WRITE (rxmac_ctrl1.wrd, XGPN_DSFP_rxmac_ctl1);
    }

    return ret;
}

ca_status_t aal_xgpon_dsfp_rxmac_ctrl1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_rxmac_ctrl1_config_t *rxmac_ctrl_cfg)
{
    XGPN_DSFP_rxmac_ctl1_t rxmac_ctrl;

    XGPON_ASSERT(rxmac_ctrl_cfg);

    rxmac_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_rxmac_ctl1);
    rxmac_ctrl_cfg->max_pkt_size = rxmac_ctrl.bf.max_pkt_size;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_rxmac_ctrl1_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_rxmac_ctrl1_config_msk_t mask,
    CA_IN  aal_xgpon_dsfp_rxmac_ctrl1_config_t *rxmac_ctrl_cfg)
{
    ca_status_t ret = CA_E_OK;
    XGPN_DSFP_rxmac_ctl1_t rxmac_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }

    XGPON_ASSERT(rxmac_ctrl_cfg);

    AAL_XGPON_LOCK();
    rxmac_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_rxmac_ctl1);

    if (mask.s.max_pkt_size) {
        if (rxmac_ctrl.bf.max_pkt_size != rxmac_ctrl_cfg->max_pkt_size)
            rxmac_ctrl.bf.max_pkt_size = rxmac_ctrl_cfg->max_pkt_size ;
    }

    AAL_PON_REG_WRITE (rxmac_ctrl.wrd, XGPN_DSFP_rxmac_ctl1);
    AAL_XGPON_UNLOCK();

    return ret;
}

ca_status_t aal_xgpon_dsfp_xgem_key_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_xgem_key_ctrl_config_t *key_ctrl_cfg)
{
    XGPN_DSFP_xgem_key_ctl_t key_ctrl;

    XGPON_ASSERT(key_ctrl_cfg);

    key_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgem_key_ctl);

    key_ctrl_cfg->key0_idx_vld = key_ctrl.bf.key0_idx_vld;
    key_ctrl_cfg->key1_idx_vld = key_ctrl.bf.key1_idx_vld;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_xgem_key_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_xgem_key_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgem_key_ctrl_config_t *key_ctrl_cfg)
{
    ca_status_t ret = CA_E_OK;
    XGPN_DSFP_xgem_key_ctl_t key_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }

    XGPON_ASSERT(key_ctrl_cfg);

    //AAL_XGPON_LOCK();

    key_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgem_key_ctl);
    if (mask.s.key0_idx_vld) {
        key_ctrl.bf.key0_idx_vld = key_ctrl_cfg->key0_idx_vld;
    }
    if (mask.s.key1_idx_vld) {
        key_ctrl.bf.key1_idx_vld = key_ctrl_cfg->key1_idx_vld ;
    }

    AAL_PON_REG_WRITE (key_ctrl.wrd, XGPN_DSFP_xgem_key_ctl);
    //AAL_XGPON_UNLOCK();

    return ret;
}



ca_status_t aal_xgpon_dsfp_xgem_mc_key_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t *key_ctrl_cfg)
{
    XGPN_DSFP_xgem_mc_key_ctl_t xgem_mc_key_ctrl;

    XGPON_ASSERT(key_ctrl_cfg);

    xgem_mc_key_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgem_mc_key_ctl);
    key_ctrl_cfg->key0_idx_vld = xgem_mc_key_ctrl.bf.key0_idx_vld;
    key_ctrl_cfg->key1_idx_vld = xgem_mc_key_ctrl.bf.key1_idx_vld;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_xgem_mc_key_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_dsfp_xgem_mc_key_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgem_mc_key_ctrl_config_t *key_ctrl_cfg)
{
    ca_status_t ret = CA_E_OK;
    XGPN_DSFP_xgem_mc_key_ctl_t xgem_mc_key_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }

    XGPON_ASSERT(key_ctrl_cfg);

    AAL_XGPON_LOCK();

    xgem_mc_key_ctrl.wrd = AAL_PON_REG_READ( XGPN_DSFP_xgem_mc_key_ctl);
    if (mask.s.key0_idx_vld) {
        if (xgem_mc_key_ctrl.bf.key0_idx_vld != key_ctrl_cfg->key0_idx_vld)
            xgem_mc_key_ctrl.bf.key0_idx_vld = key_ctrl_cfg->key0_idx_vld;
    }
    if (mask.s.key1_idx_vld) {
        if (xgem_mc_key_ctrl.bf.key1_idx_vld != key_ctrl_cfg->key1_idx_vld)
            xgem_mc_key_ctrl.bf.key1_idx_vld = key_ctrl_cfg->key1_idx_vld ;
    }
    AAL_PON_REG_WRITE (xgem_mc_key_ctrl.wrd, XGPN_DSFP_xgem_mc_key_ctl);
    AAL_XGPON_UNLOCK();

    return ret;
}

ca_status_t aal_xgpon_dsfp_xgems_cfg_get(
	    CA_IN ca_device_id_t device_id,
	    CA_IN ca_uint32_t xgem_start_index,
	    CA_IN ca_uint32_t xgem_end_index)
{
	ca_status_t ret = CA_E_OK;
	int i = 0;
	aal_xgpon_dsfp_xgem_cfg_t xgem_cfg;
	memset(&xgem_cfg,0, sizeof(aal_xgpon_dsfp_xgem_cfg_t));
	if(xgem_start_index >= AAL_XGPON_MAX_GEM_NUM)
	{
		ca_printf("ERROR:xgem_start_index should less than 128 !!\r\n");
		return CA_E_PARAM;
	}
	if(xgem_end_index >= AAL_XGPON_MAX_GEM_NUM)
	{
		ca_printf("ERROR:xgem_end_index should less than 128 !!\r\n");
		return CA_E_PARAM;
	}
	if(xgem_start_index > xgem_end_index)
	{
		ca_printf("ERROR:xgem_start_index should less than xgem_end_index !!\r\n");
		return CA_E_PARAM;
	}
	for(i = xgem_start_index; i<=xgem_end_index; i++)
	{
		ret = aal_xgpon_dsfp_xgem_get(device_id, i ,&xgem_cfg );
		CA_RET_VALUE_CHECK(ret);
		ca_printf("gem_idex: %d, gem_id:%d,gem_vld:%d,gem_mc_flag:%d\r\n",i,xgem_cfg.gem_id,xgem_cfg.gem_vld,xgem_cfg.gem_mc);
	}
	return ret;
}

ca_status_t aal_xgpon_xgems_get(
        CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    int i = 0;
    aal_xgpon_dsfp_xgem_cfg_t xgem_cfg;
    aal_xgpon_usbg_gem_config_t usbg_xgem_cfg;

    ca_printf("\r\n");
    ca_printf("Internal  GEM-ID      Valid     MC-Flag    AES-Mode\r\n");
    ca_printf("GEM-Idx   (UP/DN)    (UP/DN)    (UP/DN)       UP   \r\n");
    ca_printf("---------------------------------------------------\r\n");

    for(i = 0; i< AAL_XGPON_MAX_GEM_NUM; i++){

        memset(&xgem_cfg,0, sizeof(aal_xgpon_dsfp_xgem_cfg_t));
        ret = aal_xgpon_dsfp_xgem_get(device_id, i ,&xgem_cfg );
        CA_RET_VALUE_CHECK(ret);

        memset(&usbg_xgem_cfg,0, sizeof(usbg_xgem_cfg));
        ret = aal_xgpon_usbg_xgem_cfg_get(device_id, i ,&usbg_xgem_cfg );
        CA_RET_VALUE_CHECK(ret);

        if (xgem_cfg.gem_vld || usbg_xgem_cfg.port_en) {

            if (usbg_xgem_cfg.xgem_id != xgem_cfg.gem_id ||
                xgem_cfg.gem_vld != usbg_xgem_cfg.port_en ||
                usbg_xgem_cfg.mc_flag != xgem_cfg.gem_mc) {
                ca_printf("%-9u (%-4u/%-4u)   (%-2u/%-2u)   (%-2u/%-2u)     %-2u\r\n",
                    i, usbg_xgem_cfg.xgem_id, xgem_cfg.gem_id,
                    usbg_xgem_cfg.port_en, xgem_cfg.gem_vld,
                    usbg_xgem_cfg.mc_flag, xgem_cfg.gem_mc,
                    usbg_xgem_cfg.aes_mode);
            } else {
                ca_printf("%-9u %-10u    %-6u    %-6u      %-2u\r\n", i,
                    xgem_cfg.gem_id, xgem_cfg.gem_vld,
                    xgem_cfg.gem_mc,usbg_xgem_cfg.aes_mode);
            }

        }

    }

    return ret;
}

ca_status_t aal_xgpon_xgem_mibs_get(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    aal_xgpon_usbg_xgem_mib_config_t  up_mib;
    aal_xgpon_dsfp_xgem_mib_config_t  dn_mib;
    aal_xgpon_usbg_gem_config_t       us_gem_cfg;
	aal_xgpon_dsfp_xgem_cfg_t		  dn_gem_cfg;

    ca_printf("\r\nGEM-Idx  US_XGEM-ID  DN_XGEM-ID   UP-pkt-cnt    UP-byte-cnt    DN-pkt-cnt    DN-byte-cnt\r\n");
    ca_printf("-----------------------------------------------------------------------------\r\n");

    for (i = 0; i <  AAL_XGPON_GEM_PORT_NUM; i++) {
        memset(&up_mib, 0 , sizeof(up_mib));
        ret = aal_xgpon_usbg_xgem_mib_get(device_id, i ,&up_mib);
        CA_RET_VALUE_CHECK(ret);

        memset(&dn_mib, 0 , sizeof(dn_mib));
        ret = aal_xgpon_dsfp_xgem_mib_get(device_id, i ,&dn_mib);
        CA_RET_VALUE_CHECK(ret);

        if(up_mib.byte_cnt || dn_mib.byte_cnt) {
			
			memset(&us_gem_cfg, 0 , sizeof(us_gem_cfg));
            ret = aal_xgpon_usbg_xgem_cfg_get(device_id, i, &us_gem_cfg);
            CA_RET_VALUE_CHECK(ret);
			
            memset(&dn_gem_cfg, 0 , sizeof(dn_gem_cfg));
            ret = aal_xgpon_dsfp_xgem_get(device_id, i, &dn_gem_cfg);
            CA_RET_VALUE_CHECK(ret);
			
            ca_printf("%7d  %10d  %10d   %10d    %11lld    %10d    %11lld\n",
                i, us_gem_cfg.xgem_id,	dn_gem_cfg.gem_id,
                up_mib.frame_cnt, up_mib.byte_cnt,
                dn_mib.frame_cnt, dn_mib.byte_cnt);
        }
    }

    return ret;

}

ca_status_t aal_xgpon_dsfp_xgem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_idx,
    CA_OUT aal_xgpon_dsfp_xgem_cfg_t *xgem_cfg )
{
    XGPN_DSFP_portid0_ctl_xgem_t  xgem_id0;
    ca_uint32_t reg_addr = XGPN_DSFP_portid0_ctl_xgem + (gem_idx * 4);

    XGPON_ASSERT(gem_idx < AAL_XGPON_MAX_GEM_NUM);
    XGPON_ASSERT(xgem_cfg);

    xgem_id0.wrd = AAL_PON_REG_READ(reg_addr);

    xgem_cfg->gem_id = xgem_id0.bf.portid;
    xgem_cfg->gem_vld = xgem_id0.bf.portid_vld;
    xgem_cfg->gem_mc = xgem_id0.bf.portid_mc;

    return CA_E_OK;
}


ca_status_t aal_xgpon_dsfp_xgem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_idx,
    CA_IN aal_xgpon_dsfp_xgem_cfg_msk_t mask,
    CA_IN aal_xgpon_dsfp_xgem_cfg_t *xgem_cfg)
{
    ca_status_t ret = CA_E_OK;
    XGPN_DSFP_portid0_ctl_xgem_t  xgem_id0;
    ca_uint32_t reg_addr = XGPN_DSFP_portid0_ctl_xgem + (gem_idx * 4);

    if (gem_idx >= AAL_XGPON_MAX_GEM_NUM) {
        ca_printf("%s:%d::gem_idx :%d \r\n",__FUNCTION__,__LINE__,gem_idx);
        return CA_E_PARAM;
    }
    if (!mask.u32) {
        return CA_E_OK;
    }

    XGPON_ASSERT(xgem_cfg);

    AAL_XGPON_LOCK();
    xgem_id0.wrd = AAL_PON_REG_READ(reg_addr);

    if (mask.s.gem_id)
        xgem_id0.bf.portid = xgem_cfg->gem_id;
    if (mask.s.gem_vld)
        xgem_id0.bf.portid_vld = xgem_cfg->gem_vld;
    if (mask.s.gem_mc)
        xgem_id0.bf.portid_mc = xgem_cfg->gem_mc;

    AAL_PON_REG_WRITE(xgem_id0.wrd, reg_addr);
    AAL_XGPON_UNLOCK();

    return ret;
}


ca_status_t aal_xgpon_dsfp_sync_state_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *state)
{
    XGPN_DSFP_ds_sync_t sync_ds_state;

    XGPON_ASSERT(state);

    sync_ds_state.wrd = AAL_PON_REG_READ (XGPN_DSFP_ds_sync);
    *state = sync_ds_state.bf.state;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_super_frame_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *sfc0,
    CA_OUT ca_uint32_t *sfc1)
{
    AAL_ASSERT_RET(sfc0 != NULL && sfc1 != NULL, CA_E_PARAM);

    *sfc0 = AAL_PON_REG_READ (XGPN_DSFP_superframe0);
    *sfc1 = AAL_PON_REG_READ (XGPN_DSFP_superframe1);

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_oc_body_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_oc_body_config_t *oc_body)
{
    XGPN_DSFP_OC_body1_t dsfp_oc_body1;
    XGPN_DSFP_OC_body0_t dsfp_ponid0;

    XGPON_ASSERT(oc_body);

    dsfp_ponid0.wrd = AAL_PON_REG_READ (XGPN_DSFP_OC_body0);
    dsfp_oc_body1.wrd = AAL_PON_REG_READ (XGPN_DSFP_OC_body1);

    oc_body->TOL1  = dsfp_oc_body1.bf.TOL1;
    oc_body->C_flag1  = dsfp_oc_body1.bf.C_flag1;
    oc_body->link_type1  = dsfp_oc_body1.bf.link_type1;
    oc_body->P_flag1  = dsfp_oc_body1.bf.P_flag1;
    oc_body->DS_FEC_flag1  = dsfp_oc_body1.bf.DS_FEC_flag1;
    oc_body->ODN_class1  = dsfp_oc_body1.bf.ODN_class1;
    oc_body->RE_flag1  = dsfp_oc_body1.bf.RE_flag1;
    oc_body->ponid0 = dsfp_ponid0.bf.pon_id0;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_bip_err_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data)
{
    XGPN_DSFP_bip_32_err_t bip_32_err;

    XGPON_ASSERT(data);

    bip_32_err.wrd = AAL_PON_REG_READ( XGPN_DSFP_bip_32_err);

    *data = bip_32_err.bf.cnt;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_psbd_hec_err_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data)
{
    XGPN_DSFP_psbd_hec_err_t psbd_hec_err;

    XGPON_ASSERT(data);

    psbd_hec_err.wrd = AAL_PON_REG_READ( XGPN_DSFP_psbd_hec_err);

    *data = psbd_hec_err.bf.cnt;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_bip_dw0_get(/*The number of double words that BIP-32 checker received*/
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data)
{
    XGPN_DSFP_bip_32_dw0_t bip_32_dw0;

    XGPON_ASSERT(data);

    bip_32_dw0.wrd = AAL_PON_REG_READ( XGPN_DSFP_bip_32_dw0);

    *data = bip_32_dw0.bf.cnt0;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_bip_dw1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data)
{
    XGPN_DSFP_bip_32_dw1_t bip_32_dw1;

    XGPON_ASSERT(data);

    bip_32_dw1.wrd = AAL_PON_REG_READ( XGPN_DSFP_bip_32_dw1);

    *data = bip_32_dw1.bf.cnt1;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_psbd_hec_uncorrect_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_DSFP_psbd_hec_uncorrected_mib_t psbd_hec_uncrt_cnt;

    XGPON_ASSERT(cnt);

    psbd_hec_uncrt_cnt.wrd = AAL_PON_REG_READ (XGPN_DSFP_psbd_hec_uncorrected_mib);
    *cnt = psbd_hec_uncrt_cnt.bf.cnt;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_dsfp_stats_t *stats)
{
    XGPON_ASSERT(stats);

    stats->xgtc_frame = AAL_PON_REG_READ (XGPN_DSFP_xgtc_frame);
    stats->xgtc_err = AAL_PON_REG_READ (XGPN_DSFP_xgtc_err);
    stats->xgtc_hec_corrected = AAL_PON_REG_READ (XGPN_DSFP_xgtc_hec_corrected);
    stats->xgtc_hec_uncorrected = AAL_PON_REG_READ (XGPN_DSFP_xgtc_hec_uncorrected_mib);
    stats->xgem_frame = AAL_PON_REG_READ (XGPN_DSFP_xgem_frame);
    stats->xgem_err0 = AAL_PON_REG_READ (XGPN_DSFP_xgem_err0);
    stats->xgem_err1 = AAL_PON_REG_READ (XGPN_DSFP_xgem_err1);
    stats->xgem_hec_corrected = AAL_PON_REG_READ (XGPN_DSFP_xgem_hec_corrected);
    stats->xgem_hec_uncorrected = AAL_PON_REG_READ (XGPN_DSFP_xgem_hec_uncorrected_mib);
    stats->xgem_nolf = AAL_PON_REG_READ (XGPN_DSFP_xgem_nolf_mib);
    stats->xgem_los_word = AAL_PON_REG_READ (XGPN_DSFP_xgem_los_word_mib);
    stats->xgem_err_key = AAL_PON_REG_READ (XGPN_DSFP_xgem_err_key_mib);
    stats->rxmac_xgem_frame = AAL_PON_REG_READ (XGPN_DSFP_rxmac_xgem_frame);
    stats->rxmac_pkt_frame = AAL_PON_REG_READ (XGPN_DSFP_rxmac_pkt_frame);
    stats->rxmac_omci_frame = AAL_PON_REG_READ (XGPN_DSFP_rxmac_omci_frame);
    stats->rxmac_pkt_crc_err = AAL_PON_REG_READ (XGPN_DSFP_rxmac_pkt_crc_err);
    stats->bwmap_hec_corrected = AAL_PON_REG_READ (XGPN_DSFP_bwmap_hec_corrected);
    stats->bwmap_hec_uncorrected = AAL_PON_REG_READ (XGPN_DSFP_bwmap_hec_uncorrected_mib);
    stats->psbd_hec_uncorrected = AAL_PON_REG_READ (XGPN_DSFP_psbd_hec_uncorrected_mib);
    stats->ponid_hec_corrected = AAL_PON_REG_READ (XGPN_DSFP_ponid_hec_corrected);
    stats->ponid_hec_uncorrected = AAL_PON_REG_READ (XGPN_DSFP_ponid_hec_uncorrected_mib);
    stats->sfc_hec_corrected = AAL_PON_REG_READ (XGPN_DSFP_sfc_hec_corrected);
    stats->sfc_hec_uncorrected = AAL_PON_REG_READ (XGPN_DSFP_sfc_hec_uncorrected_mib);
    stats->bip_32_err = AAL_PON_REG_READ (XGPN_DSFP_bip_32_err);
    stats->bip_32_dw0  = AAL_PON_REG_READ (XGPN_DSFP_bip_32_dw0);
    stats->bip_32_dw1  = AAL_PON_REG_READ (XGPN_DSFP_bip_32_dw1);
    stats->psbd_hec_err = AAL_PON_REG_READ( XGPN_DSFP_psbd_hec_err); /*Bit errors that PSBd HEC decoder detected*/

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_xgtc_hec_correct_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_DSFP_xgtc_hec_corrected_t xgtc_hec_correct_cnt;

    XGPON_ASSERT(cnt);

    xgtc_hec_correct_cnt.wrd = AAL_PON_REG_READ (XGPN_DSFP_xgtc_hec_corrected);
    *cnt = xgtc_hec_correct_cnt.bf.cnt;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_xgtc_hec_uncorrect_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_DSFP_xgtc_hec_uncorrected_mib_t xgtc_hec_uncorrect_cnt;

    XGPON_ASSERT(cnt);

    xgtc_hec_uncorrect_cnt.wrd = AAL_PON_REG_READ (XGPN_DSFP_xgtc_hec_uncorrected_mib);
    *cnt = xgtc_hec_uncorrect_cnt.bf.cnt;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_xgem_err0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *hd)
{
    XGPN_DSFP_xgem_err0_t xgem_err;

    XGPON_ASSERT(hd);

    xgem_err.wrd = AAL_PON_REG_READ (XGPN_DSFP_xgem_err0);
    *hd = xgem_err.bf.hd;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_xgem_err1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *hd)
{
    XGPN_DSFP_xgem_err1_t xgem_err;

    XGPON_ASSERT(hd);

    xgem_err.wrd = AAL_PON_REG_READ (XGPN_DSFP_xgem_err1);
    *hd = xgem_err.bf.hd;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_xgem_los_word_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_DSFP_xgem_los_word_mib_t xgem_los_word_cnt;

    XGPON_ASSERT(cnt);

    xgem_los_word_cnt.wrd = AAL_PON_REG_READ (XGPN_DSFP_xgem_los_word_mib);
    *cnt = xgem_los_word_cnt.bf.cnt;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_xgem_err_key_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_DSFP_xgem_err_key_mib_t xgem_err_key_cnt;

    XGPON_ASSERT(cnt);

    xgem_err_key_cnt.wrd = AAL_PON_REG_READ (XGPN_DSFP_xgem_err_key_mib);
    *cnt = xgem_err_key_cnt.bf.cnt;

    return CA_E_OK;

}

ca_status_t aal_xgpon_dsfp_rxmac_omci_frame_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_DSFP_rxmac_omci_frame_t omci_frame_cnt;

    XGPON_ASSERT(cnt);

    omci_frame_cnt.wrd = AAL_PON_REG_READ (XGPN_DSFP_rxmac_omci_frame);
    *cnt = omci_frame_cnt.bf.cnt;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_rxmac_xgem_frame_status_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *fg_xgem_err)
{
    XGPN_DSFP_rxmac_xgem_frame_status_t rxmac_xgem_frm_status;

    XGPON_ASSERT(fg_xgem_err);

    rxmac_xgem_frm_status.wrd = AAL_PON_REG_READ (XGPN_DSFP_rxmac_xgem_frame_status);
    *fg_xgem_err = rxmac_xgem_frm_status.bf.fg_xgem_err;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_async_ff_status_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *overflow_status)
{
    XGPN_DSFP_async_ff_status_t async_ff_status;

    XGPON_ASSERT(overflow_status);

    async_ff_status.wrd = AAL_PON_REG_READ (XGPN_DSFP_async_ff_status);
    *overflow_status = async_ff_status.bf.overflow;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_mib_ram_status_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *status)
{
    XGPN_DSFP_MIB_RAM_STATUS_REG_t mib_ram_status;

    XGPON_ASSERT(status);

    mib_ram_status.wrd = AAL_PON_REG_READ (XGPN_DSFP_MIB_RAM_STATUS_REG);
    *status = mib_ram_status.bf.ram_initdone;

    return CA_E_OK;
}

ca_status_t aal_xgpon_dsfp_xgem_mib_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_OUT aal_xgpon_dsfp_xgem_mib_config_t *xgem_cnt)
{
    ca_status_t ret = CA_E_OK;
    ca_uint64_t tmp = 0;
    XGPN_DSFP_PORTID_XGEM_MIB_DATA0_t xgem_mib_data0;
    XGPN_DSFP_PORTID_XGEM_MIB_DATA1_t xgem_mib_data1;
    XGPN_DSFP_PORTID_XGEM_MIB_DATA2_t xgem_mib_data2;

    XGPON_ASSERT(xgem_idx < AAL_XGPON_MAX_GEM_NUM);
    XGPON_ASSERT(xgem_cnt);

    AAL_XGPON_LOCK();
    AAL_PON_REG_INDIRECT_READ(xgem_idx, XGPN_DSFP_PORTID_XGEM_MIB_ACCESS);
    xgem_mib_data0.wrd = AAL_PON_REG_READ (XGPN_DSFP_PORTID_XGEM_MIB_DATA0);
    xgem_mib_data1.wrd = AAL_PON_REG_READ (XGPN_DSFP_PORTID_XGEM_MIB_DATA1);
    xgem_mib_data2.wrd = AAL_PON_REG_READ (XGPN_DSFP_PORTID_XGEM_MIB_DATA2);
    AAL_XGPON_UNLOCK();
    tmp = xgem_mib_data2.bf.byte_cnt;
    xgem_cnt->frame_cnt = xgem_mib_data0.bf.frame_cnt;
    xgem_cnt->byte_cnt = ca_inter_to64(xgem_mib_data2.wrd, xgem_mib_data1.wrd);
    //xgem_cnt->byte_cnt = (tmp<<32)|xgem_mib_data1.bf.byte_cnt;
    return ret;
}

ca_status_t aal_xgpon_dsfp_xgem_mib_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_OUT aal_xgpon_dsfp_xgem_mib_config_t *xgem_cnt)
{
    ca_status_t ret = CA_E_OK;
    XGPN_DSFP_PORTID_XGEM_MIB_DATA0_t xgem_mib_data0;
    XGPN_DSFP_PORTID_XGEM_MIB_DATA1_t xgem_mib_data1;
    XGPN_DSFP_PORTID_XGEM_MIB_DATA2_t xgem_mib_data2;


    XGPON_ASSERT(xgem_cnt);
    XGPON_ASSERT(xgem_idx < AAL_XGPON_MAX_GEM_NUM);

    AAL_XGPON_LOCK();
    AAL_PON_REG_INDIRECT_READ(xgem_idx, XGPN_DSFP_PORTID_XGEM_MIB_ACCESS);
    xgem_mib_data0.wrd = AAL_PON_REG_READ (XGPN_DSFP_PORTID_XGEM_MIB_DATA0);
    xgem_mib_data1.wrd = AAL_PON_REG_READ (XGPN_DSFP_PORTID_XGEM_MIB_DATA1);
    xgem_mib_data2.wrd = AAL_PON_REG_READ (XGPN_DSFP_PORTID_XGEM_MIB_DATA2);

    xgem_mib_data0.bf.frame_cnt =  xgem_cnt->frame_cnt;
    xgem_mib_data1.bf.byte_cnt  = (ca_uint32_t)xgem_cnt->byte_cnt;
    xgem_mib_data2.bf.byte_cnt  = xgem_cnt->byte_cnt>>32;

    AAL_PON_REG_WRITE (xgem_mib_data0.wrd, XGPN_DSFP_PORTID_XGEM_MIB_DATA0);
    AAL_PON_REG_WRITE (xgem_mib_data1.wrd, XGPN_DSFP_PORTID_XGEM_MIB_DATA1);
    AAL_PON_REG_WRITE (xgem_mib_data2.wrd, XGPN_DSFP_PORTID_XGEM_MIB_DATA2);
    AAL_PON_REG_INDIRECT_WRITE(xgem_idx, XGPN_DSFP_PORTID_XGEM_MIB_ACCESS);
    AAL_XGPON_UNLOCK();
    return ret;
}


ca_status_t aal_xgpon_usbg_glb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_glb_ctrl_config_t *usbg_glb_cfg)
{
    XGPN_USBG_GBL_CTRL_t usbg_glb_ctrl;

    XGPON_ASSERT(usbg_glb_cfg);

    usbg_glb_ctrl.wrd = AAL_PON_REG_READ( XGPN_USBG_GBL_CTRL);
    usbg_glb_cfg->xgtc_en = usbg_glb_ctrl.bf.xgtc_en;
    usbg_glb_cfg->scramble_en = usbg_glb_ctrl.bf.scramble_en;
    usbg_glb_cfg->laser_on = usbg_glb_ctrl.bf.laser_on;
    usbg_glb_cfg->laser_polarity = usbg_glb_ctrl.bf.laser_polarity;
    usbg_glb_cfg->swap_tx_bus = usbg_glb_ctrl.bf.swap_tx_bus;

    usbg_glb_cfg->pf_align_mode = usbg_glb_ctrl.bf.pf_align_mode;
    usbg_glb_cfg->aes_encrypt_en = usbg_glb_ctrl.bf.aes_encrypt_en;
    usbg_glb_cfg->laser_on_align = usbg_glb_ctrl.bf.laser_on_align;


    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_glb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_glb_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_usbg_glb_ctrl_config_t *usbg_glb_cfg)
{
    XGPN_USBG_GBL_CTRL_t usbg_glb_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }
    AAL_XGPON_LOCK();

    usbg_glb_ctrl.wrd = AAL_PON_REG_READ( XGPN_USBG_GBL_CTRL);
    if (mask.s.xgtc_en) {
        if (usbg_glb_ctrl.bf.xgtc_en != usbg_glb_cfg->xgtc_en)
            usbg_glb_ctrl.bf.xgtc_en = usbg_glb_cfg->xgtc_en;
    }
    if (mask.s.scramble_en) {
        if (usbg_glb_ctrl.bf.scramble_en != usbg_glb_cfg->scramble_en)
            usbg_glb_ctrl.bf.scramble_en = usbg_glb_cfg->scramble_en ;
    }
    if (mask.s.laser_on) {
        if (usbg_glb_ctrl.bf.laser_on != usbg_glb_cfg->laser_on)
            usbg_glb_ctrl.bf.laser_on = usbg_glb_cfg->laser_on ;
    }
    if (mask.s.laser_polarity) {
        if (usbg_glb_ctrl.bf.laser_polarity != usbg_glb_cfg->laser_polarity)
            usbg_glb_ctrl.bf.laser_polarity = usbg_glb_cfg->laser_polarity  ;
    }
    if (mask.s.swap_tx_bus) {
        if (usbg_glb_ctrl.bf.swap_tx_bus != usbg_glb_cfg->swap_tx_bus)
            usbg_glb_ctrl.bf.swap_tx_bus = usbg_glb_cfg->swap_tx_bus ;
    }

    if (mask.s.pf_align_mode) {
        if (usbg_glb_ctrl.bf.pf_align_mode != usbg_glb_cfg->pf_align_mode)
            usbg_glb_ctrl.bf.pf_align_mode = usbg_glb_cfg->pf_align_mode ;
    }
    if (mask.s.aes_encrypt_en) {
        if (usbg_glb_ctrl.bf.aes_encrypt_en != usbg_glb_cfg->aes_encrypt_en)
            usbg_glb_ctrl.bf.aes_encrypt_en = usbg_glb_cfg->aes_encrypt_en  ;
    }
    if (mask.s.laser_on_align) {
        if (usbg_glb_ctrl.bf.laser_on_align != usbg_glb_cfg->laser_on_align)
            usbg_glb_ctrl.bf.laser_on_align = usbg_glb_cfg->laser_on_align ;
    }
    AAL_PON_REG_WRITE (usbg_glb_ctrl.wrd, XGPN_USBG_GBL_CTRL);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}



ca_status_t aal_xgpon_usbg_xgtc_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_xgtc_ctrl_config_t *xgtc_ctrl_cfg)
{
    XGPN_USBG_XGTC_CTRL_t xgtc_ctrl;

    XGPON_ASSERT(xgtc_ctrl_cfg);

    xgtc_ctrl.wrd = AAL_PON_REG_READ( XGPN_USBG_XGTC_CTRL);
    xgtc_ctrl_cfg->xgem_en = xgtc_ctrl.bf.xgem_en;
    xgtc_ctrl_cfg->dbru_en = xgtc_ctrl.bf.dbru_en;
    xgtc_ctrl_cfg->ploam_en = xgtc_ctrl.bf.ploam_en;
    xgtc_ctrl_cfg->O7_plmp_off = xgtc_ctrl.bf.O7_plmp_off;

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_xgtc_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_xgtc_ctrl_config_msk_t mask,
    CA_IN aal_xgpon_usbg_xgtc_ctrl_config_t *xgtc_ctrl_cfg)
{
    XGPN_USBG_XGTC_CTRL_t xgtc_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(xgtc_ctrl_cfg);

    AAL_XGPON_LOCK();
    xgtc_ctrl.wrd = AAL_PON_REG_READ( XGPN_USBG_XGTC_CTRL);
    if (mask.s.xgem_en) {
        if (xgtc_ctrl.bf.xgem_en != xgtc_ctrl_cfg->xgem_en)
            xgtc_ctrl.bf.xgem_en = xgtc_ctrl_cfg->xgem_en;
    }
    if (mask.s.dbru_en) {
        if (xgtc_ctrl.bf.dbru_en != xgtc_ctrl_cfg->dbru_en)
            xgtc_ctrl.bf.dbru_en = xgtc_ctrl_cfg->dbru_en ;
    }
    if (mask.s.ploam_en) {
        if (xgtc_ctrl.bf.ploam_en != xgtc_ctrl_cfg->ploam_en)
            xgtc_ctrl.bf.ploam_en = xgtc_ctrl_cfg->ploam_en ;
    }
    if (mask.s.O7_plmp_off) {
        if (xgtc_ctrl.bf.O7_plmp_off != xgtc_ctrl_cfg->O7_plmp_off)
            xgtc_ctrl.bf.O7_plmp_off = xgtc_ctrl_cfg->O7_plmp_off  ;
    }

    AAL_PON_REG_WRITE (xgtc_ctrl.wrd, XGPN_USBG_XGTC_CTRL);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}



ca_status_t aal_xgpon_usbg_guard_time_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_guard_time_config_t *guard_time_cfg)
{
    XGPN_USBG_GUARD_TM_CFG_t guard_time;

    XGPON_ASSERT(guard_time_cfg);

    guard_time.wrd = AAL_PON_REG_READ( XGPN_USBG_GUARD_TM_CFG);
    guard_time_cfg->time = guard_time.bf.time;

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_guard_time_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_guard_time_config_msk_t mask,
    CA_IN aal_xgpon_usbg_guard_time_config_t *guard_time_cfg)
{
    XGPN_USBG_GUARD_TM_CFG_t guard_time;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(guard_time_cfg);

    AAL_XGPON_LOCK();

    guard_time.wrd = AAL_PON_REG_READ( XGPN_USBG_GUARD_TM_CFG);

    if (mask.s.time) {
        if (guard_time.bf.time != guard_time_cfg->time)
            guard_time.bf.time = guard_time_cfg->time ;
    }

    AAL_PON_REG_WRITE (guard_time.wrd, XGPN_USBG_GUARD_TM_CFG);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}




ca_status_t aal_xgpon_usbg_idl_frame_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_idl_frm_config_t *idl_frm_cfg)
{
    XGPN_USBG_IDLE_FM_CFG_t idl_frm_config;

    XGPON_ASSERT(idl_frm_cfg);

    idl_frm_config.wrd = AAL_PON_REG_READ( XGPN_USBG_IDLE_FM_CFG);
    idl_frm_cfg->size = idl_frm_config.bf.size;
    idl_frm_cfg->mode = idl_frm_config.bf.mode;
    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_idl_frame_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_idl_frm_config_msk_t mask,
    CA_IN aal_xgpon_usbg_idl_frm_config_t *idl_frm_cfg)
{
    XGPN_USBG_IDLE_FM_CFG_t idl_frm_config;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(idl_frm_cfg);

    AAL_XGPON_LOCK();

    idl_frm_config.wrd = AAL_PON_REG_READ( XGPN_USBG_IDLE_FM_CFG);

    if (mask.s.size) {
        if (idl_frm_config.bf.size != idl_frm_cfg->size)
            idl_frm_config.bf.size = idl_frm_cfg->size ;
    }
    if (mask.s.mode) {
        if (idl_frm_config.bf.mode != idl_frm_cfg->mode)
            idl_frm_config.bf.mode = idl_frm_cfg->mode ;
    }

    AAL_PON_REG_WRITE (idl_frm_config.wrd, XGPN_USBG_IDLE_FM_CFG);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}



ca_status_t aal_xgpon_usbg_xgtc_hd_ind_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_xgtc_hd_ind_config_t *hd_ind_cfg)
{
    XGPN_USBG_XGTC_HD_IND_CFG_t hd_ind_config;

    XGPON_ASSERT(hd_ind_cfg);

    hd_ind_config.wrd = AAL_PON_REG_READ( XGPN_USBG_XGTC_HD_IND_CFG);
    hd_ind_cfg->dg = hd_ind_config.bf.dg;

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_xgtc_hd_ind_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_xgtc_hd_ind_config_msk_t mask,
    CA_OUT aal_xgpon_usbg_xgtc_hd_ind_config_t *hd_ind_cfg)
{
    XGPN_USBG_XGTC_HD_IND_CFG_t hd_ind_config;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(hd_ind_cfg);

//    AAL_XGPON_LOCK();

    hd_ind_config.wrd = AAL_PON_REG_READ( XGPN_USBG_XGTC_HD_IND_CFG);

    if (mask.s.dg) {
        hd_ind_config.bf.dg = hd_ind_cfg->dg ;
    }

    AAL_PON_REG_WRITE (hd_ind_config.wrd, XGPN_USBG_XGTC_HD_IND_CFG);
//    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_usbg_aes_uni_key_idx_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint8_t  *active_key_idx)
{
    XGPN_USBG_AES_UNI_KEY_IDX_CTRL_t aes_uni_key_cfg;

    XGPON_ASSERT(active_key_idx);

    aes_uni_key_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_AES_UNI_KEY_IDX_CTRL);
    *active_key_idx = aes_uni_key_cfg.bf.key_idx;

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_aes_uni_key_idx_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  active_key_idx)
{
    XGPN_USBG_AES_UNI_KEY_IDX_CTRL_t aes_uni_key_cfg;

    //AAL_XGPON_LOCK();
    aes_uni_key_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_AES_UNI_KEY_IDX_CTRL);

    if (aes_uni_key_cfg.bf.key_idx != active_key_idx) {
        aes_uni_key_cfg.bf.key_idx = active_key_idx ;
        AAL_PON_REG_WRITE (aes_uni_key_cfg.wrd, XGPN_USBG_AES_UNI_KEY_IDX_CTRL);
    }
    //AAL_XGPON_UNLOCK();

    return CA_E_OK;
}
    
ca_status_t aal_xgpon_usbg_xgems_cfg_get(
        CA_IN ca_device_id_t device_id,
        CA_IN ca_uint32_t xgem_start_index,
        CA_IN ca_uint32_t xgem_end_index)
{
    ca_status_t ret = CA_E_OK;
    int i = 0;
    aal_xgpon_usbg_gem_config_t xgem_cfg;
    memset(&xgem_cfg,0, sizeof(aal_xgpon_usbg_gem_config_t));
    if(xgem_start_index >= AAL_XGPON_MAX_GEM_NUM)
    {
        ca_printf("ERROR:xgem_start_index should less than 128 !!\r\n");
        return CA_E_PARAM;
    }
    if(xgem_end_index >= AAL_XGPON_MAX_GEM_NUM)
    {
        ca_printf("ERROR:xgem_end_index should less than 128 !!\r\n");
        return CA_E_PARAM;
    }
    if(xgem_start_index > xgem_end_index)
    {
        ca_printf("ERROR:xgem_start_index should less than xgem_end_index !!\r\n");
        return CA_E_PARAM;
    }
    for(i = xgem_start_index; i<= xgem_end_index; i++)
    {
        ret = aal_xgpon_usbg_xgem_cfg_get(device_id, i ,&xgem_cfg );
        CA_RET_VALUE_CHECK(ret);
        ca_printf("gem_idex: %d, gem_id:%d,port_en:%d,aes_mode:%d,gem_mc_flag:%d\r\n",i,xgem_cfg.xgem_id,xgem_cfg.port_en,xgem_cfg.aes_mode,xgem_cfg.mc_flag);
    
    }

    return ret;
}

ca_status_t aal_xgpon_usbg_xgem_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_OUT aal_xgpon_usbg_gem_config_t *config)
{
    XGPN_USBG_XGEM_PID_TBL_DATA_t usbg_xgem_data;


    XGPON_ASSERT(xgem_idx < AAL_XGPON_MAX_GEM_NUM);
    XGPON_ASSERT(config);

    AAL_XGPON_LOCK();

    AAL_PON_REG_INDIRECT_READ(xgem_idx, XGPN_USBG_XGEM_PID_TBL_ACCESS);
    usbg_xgem_data.wrd = AAL_PON_REG_READ (XGPN_USBG_XGEM_PID_TBL_DATA);


    config->xgem_id = usbg_xgem_data.bf.XGEM_PortID;
    config->port_en = usbg_xgem_data.bf.port_en;
    config->aes_mode = usbg_xgem_data.bf.aes_mode;
    config->mc_flag = usbg_xgem_data.bf.MC_flag;
    AAL_XGPON_UNLOCK();
    return CA_E_OK;

}
ca_status_t aal_xgpon_usbg_xgem_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_IN aal_xgpon_usbg_gem_config_msk_t mask,
    CA_IN aal_xgpon_usbg_gem_config_t *config)
{
    ca_status_t ret = CA_E_OK;
    XGPN_USBG_XGEM_PID_TBL_DATA_t usbg_xgem_data;

    XGPON_ASSERT(xgem_idx < AAL_XGPON_MAX_GEM_NUM);
    XGPON_ASSERT(config);

    if (!mask.u32) {
        return CA_E_OK;
    }
    AAL_XGPON_LOCK();

    AAL_PON_REG_INDIRECT_READ(xgem_idx, XGPN_USBG_XGEM_PID_TBL_ACCESS);
    usbg_xgem_data.wrd = AAL_PON_REG_READ (XGPN_USBG_XGEM_PID_TBL_DATA);


    if (mask.s.xgem_id) {
        usbg_xgem_data.bf.XGEM_PortID = config->xgem_id;
    }
    if (mask.s.port_en) {
        usbg_xgem_data.bf.port_en = config->port_en;
    }
    if (mask.s.aes_mode) {
        usbg_xgem_data.bf.aes_mode = config->aes_mode;
    }
    if (mask.s.mc_flag) {
        usbg_xgem_data.bf.MC_flag = config->mc_flag;
    }
    AAL_PON_REG_WRITE (usbg_xgem_data.wrd, XGPN_USBG_XGEM_PID_TBL_DATA);

    AAL_PON_REG_INDIRECT_WRITE(xgem_idx, XGPN_USBG_XGEM_PID_TBL_ACCESS);
    AAL_XGPON_UNLOCK();
    return ret;

}

ca_status_t aal_xgpon_usbg_xgem_buf_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_xgem_buf_config_t *xgem_buf_cfg)
{
    XGPN_USBG_XGEM_BUF_CFG_t xgem_buf_config;

    XGPON_ASSERT(xgem_buf_cfg);
    xgem_buf_config.wrd = AAL_PON_REG_READ( XGPN_USBG_XGEM_BUF_CFG);
    xgem_buf_cfg->lo_threshold = xgem_buf_config.bf.lo_threshold;
    xgem_buf_cfg->hi_threshold = xgem_buf_config.bf.hi_threshold;

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_xgem_buf_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_xgem_buf_config_msk_t mask,
    CA_IN aal_xgpon_usbg_xgem_buf_config_t *xgem_buf_cfg)
{
    XGPN_USBG_XGEM_BUF_CFG_t xgem_buf_config;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(xgem_buf_cfg);

    AAL_XGPON_LOCK();

    xgem_buf_config.wrd = AAL_PON_REG_READ( XGPN_USBG_XGEM_BUF_CFG);
    if (mask.s.lo_threshold) {
        if (xgem_buf_config.bf.lo_threshold != xgem_buf_cfg->lo_threshold)
            xgem_buf_config.bf.lo_threshold = xgem_buf_cfg->lo_threshold;
    }
    if (mask.s.hi_threshold) {
        if (xgem_buf_config.bf.hi_threshold != xgem_buf_cfg->hi_threshold)
            xgem_buf_config.bf.hi_threshold = xgem_buf_cfg->hi_threshold ;
    }
    AAL_PON_REG_WRITE (xgem_buf_config.wrd, XGPN_USBG_XGEM_BUF_CFG);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_usbg_dbru_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint8_t  *mode)
{
    XGPN_USBG_DBRU_CFG_t dbru_cfg;

    XGPON_ASSERT(mode);
    dbru_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_DBRU_CFG);
    *mode = dbru_cfg.bf.mode;

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_dbru_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t  mode)
{
    XGPN_USBG_DBRU_CFG_t dbru_cfg;

    AAL_XGPON_LOCK();
    dbru_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_DBRU_CFG);

    if (dbru_cfg.bf.mode != mode) {
        dbru_cfg.bf.mode = mode ;
        AAL_PON_REG_WRITE (dbru_cfg.wrd, XGPN_USBG_DBRU_CFG);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_usbg_ploamu_queue_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_ploamu_qu_config_t *ploamu_qu_config)
{
    XGPN_USBG_PLOAMU_QU_CFG_t ploamu_qu_cfg;

    XGPON_ASSERT(ploamu_qu_config);

    ploamu_qu_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_PLOAMU_QU_CFG);
    ploamu_qu_config->no_empty = ploamu_qu_cfg.bf.no_empty;
    ploamu_qu_config->mode = ploamu_qu_cfg.bf.mode;
    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_ploamu_queue_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_ploamu_qu_config_msk_t mask,
    CA_IN aal_xgpon_usbg_ploamu_qu_config_t *ploamu_qu_config)
{
    XGPN_USBG_PLOAMU_QU_CFG_t ploamu_qu_cfg;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(ploamu_qu_config);

    AAL_XGPON_LOCK();

    ploamu_qu_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_PLOAMU_QU_CFG);
    if (mask.s.no_empty) {
        if (ploamu_qu_cfg.bf.no_empty != ploamu_qu_config->no_empty)
            ploamu_qu_cfg.bf.no_empty = ploamu_qu_config->no_empty;
    }
    if (mask.s.mode) {
        if (ploamu_qu_cfg.bf.mode != ploamu_qu_config->mode)
            ploamu_qu_cfg.bf.mode = ploamu_qu_config->mode ;
    }
    AAL_PON_REG_WRITE (ploamu_qu_cfg.wrd, XGPN_USBG_PLOAMU_QU_CFG);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}



ca_status_t aal_xgpon_usbg_frag_ff_dump_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_frag_ff_dump_config_t *frag_ff_dump_config)
{
    XGPN_USBG_FRAG_FF_DUMP_CFG_t frag_ff_dump_cfg;

    XGPON_ASSERT(frag_ff_dump_config);

    frag_ff_dump_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_FRAG_FF_DUMP_CFG);
    frag_ff_dump_config->dump_time = frag_ff_dump_cfg.bf.dump_time;
    frag_ff_dump_config->dump_en = frag_ff_dump_cfg.bf.dump_en;
    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_frag_ff_dump_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_usbg_frag_ff_dump_config_msk_t mask,
    CA_IN aal_xgpon_usbg_frag_ff_dump_config_t *frag_ff_dump_config)
{
    XGPN_USBG_FRAG_FF_DUMP_CFG_t frag_ff_dump_cfg;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(frag_ff_dump_config);

    AAL_XGPON_LOCK();

    frag_ff_dump_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_FRAG_FF_DUMP_CFG);
    if (mask.s.dump_time) {
        if (frag_ff_dump_cfg.bf.dump_time != frag_ff_dump_config->dump_time)
            frag_ff_dump_cfg.bf.dump_time = frag_ff_dump_config->dump_time;
    }
    if (mask.s.dump_en) {
        if (frag_ff_dump_cfg.bf.dump_en != frag_ff_dump_config->dump_en)
            frag_ff_dump_cfg.bf.dump_en = frag_ff_dump_config->dump_en ;
    }
    AAL_PON_REG_WRITE (frag_ff_dump_cfg.wrd, XGPN_USBG_FRAG_FF_DUMP_CFG);
    AAL_XGPON_UNLOCK();
    return CA_E_OK;
}


ca_status_t aal_xgpon_usbg_xgtc_err_dump_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *dly_time)
{
    XGPN_USBG_XGTC_ERR_DUMP_CFG_t xgtc_err_dmp_cfg;

    XGPON_ASSERT(dly_time);

    xgtc_err_dmp_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_XGTC_ERR_DUMP_CFG);
    *dly_time = xgtc_err_dmp_cfg.bf.dly_time;
    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_xgtc_err_dump_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  dly_time)
{
    XGPN_USBG_XGTC_ERR_DUMP_CFG_t xgtc_err_dmp_cfg;
    AAL_XGPON_LOCK();

    xgtc_err_dmp_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_XGTC_ERR_DUMP_CFG);

    if (xgtc_err_dmp_cfg.bf.dly_time != dly_time) {
        xgtc_err_dmp_cfg.bf.dly_time = dly_time ;
        AAL_PON_REG_WRITE (xgtc_err_dmp_cfg.wrd, XGPN_USBG_XGTC_ERR_DUMP_CFG);
    }

    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_usbg_dbru_rpt_max_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *val)
{
    XGPN_USBG_DBRU_RPT_MAX_CFG_t dbru_rpt_cfg;

    XGPON_ASSERT(val);

    dbru_rpt_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_DBRU_RPT_MAX_CFG);
    *val = dbru_rpt_cfg.bf.val;

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_dbru_rpt_max_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  val)
{
    XGPN_USBG_DBRU_RPT_MAX_CFG_t dbru_rpt_cfg;
    AAL_XGPON_LOCK();

    dbru_rpt_cfg.wrd = AAL_PON_REG_READ( XGPN_USBG_DBRU_RPT_MAX_CFG);

    if (dbru_rpt_cfg.bf.val != val)
        dbru_rpt_cfg.bf.val = val ;

    AAL_PON_REG_WRITE (dbru_rpt_cfg.wrd, XGPN_USBG_DBRU_RPT_MAX_CFG);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_usbg_dbru_bufocc_tbl_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t alloc_idx,
    CA_IN ca_uint32_t select,
    CA_OUT ca_uint32_t *buf_occ)
{
    XGPN_USBG_DBRU_BUFOCC_TBL_DATA_t dbru_bufocc;

    XGPON_ASSERT(alloc_idx <= AAL_MAX_LLID_TCONT_IDX);
    XGPON_ASSERT(buf_occ);
    XGPON_ASSERT(select <= 1);

    AAL_XGPON_LOCK();

    AAL_XGPN_REG_INDIRECT_READ(select, alloc_idx, XGPN_USBG_DBRU_BUFOCC_TBL_ACCESS);
    dbru_bufocc.wrd = AAL_PON_REG_READ (XGPN_USBG_DBRU_BUFOCC_TBL_DATA);
    *buf_occ = dbru_bufocc.bf.BufOcc;
    AAL_XGPON_UNLOCK();

    return CA_E_OK;

}
ca_status_t aal_xgpon_usbg_dbru_bufocc_tbl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t alloc_idx,
    CA_IN ca_uint32_t select,
    CA_IN ca_uint32_t buf_occ)
{
    XGPN_USBG_DBRU_BUFOCC_TBL_DATA_t dbru_bufocc;

    XGPON_ASSERT(alloc_idx <= AAL_MAX_LLID_TCONT_IDX);
    XGPON_ASSERT(select <= 1);

    AAL_XGPON_LOCK();
    AAL_XGPN_REG_INDIRECT_READ(select, alloc_idx, XGPN_USBG_DBRU_BUFOCC_TBL_ACCESS);
    dbru_bufocc.wrd = AAL_PON_REG_READ (XGPN_USBG_DBRU_BUFOCC_TBL_DATA);

    dbru_bufocc.bf.BufOcc = buf_occ;

    AAL_PON_REG_WRITE (dbru_bufocc.wrd, XGPN_USBG_DBRU_BUFOCC_TBL_DATA);
    AAL_XGPN_REG_INDIRECT_WRITE(select, alloc_idx, XGPN_USBG_DBRU_BUFOCC_TBL_ACCESS);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_xgpon_usbg_burst_prof_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    prof_idx,
    CA_IN aal_xgpon_usbg_burst_prof_cfg_msk_t mask,
    CA_IN aal_xgpon_usbg_burst_prof_cfg_t *prof_cfg)
{
    XGPN_USBG_BURST_PF0_INFOR_STS_t  pf_info;
    ca_uint32_t  reg_addr = XGPN_USBG_BURST_PF0_INFOR_STS + (prof_idx * 20);

    XGPON_ASSERT(prof_idx < AAL_XGPON_BURST_PROF_NUM);
    XGPON_ASSERT(prof_cfg);

    AAL_XGPON_LOCK();

    pf_info.wrd = AAL_PON_REG_READ(reg_addr);
    if (mask.s.pcnt) {
        if (pf_info.bf.pcnt != prof_cfg->pcnt)
            pf_info.bf.pcnt = prof_cfg->pcnt;
    }
    if (mask.s.plen) {
        if (pf_info.bf.plen != prof_cfg->plen)
            pf_info.bf.plen = prof_cfg->plen ;
    }
    if (mask.s.dlen) {
        if (pf_info.bf.dlen != prof_cfg->dlen)
            pf_info.bf.dlen = prof_cfg->dlen ;
    }
    if (mask.s.oh_len) {
        if (pf_info.bf.oh_len != prof_cfg->oh_len)
            pf_info.bf.oh_len = prof_cfg->oh_len  ;
    }
    if (mask.s.fecu) {
        if (pf_info.bf.fecu != prof_cfg->fecu)
            pf_info.bf.fecu = prof_cfg->fecu ;
    }
    if (mask.s.ver) {
        if (pf_info.bf.ver != prof_cfg->ver)
            pf_info.bf.ver = prof_cfg->ver  ;
    }
    if (mask.s.vld) {
        if (pf_info.bf.vld != prof_cfg->vld)
            pf_info.bf.vld = prof_cfg->vld ;
    }

    if ((mask.s.pcnt) ||(mask.s.plen) || (mask.s.dlen) || (mask.s.oh_len)
        || (mask.s.fecu) || (mask.s.ver) || (mask.s.vld)) {
        AAL_PON_REG_WRITE (pf_info.wrd, reg_addr);
    }

    if (mask.s.dp_h ) {
        AAL_PON_REG_WRITE (prof_cfg->dp_h, reg_addr + 4);
    }
    if (mask.s.dp_l ) {
        AAL_PON_REG_WRITE (prof_cfg->dp_l, reg_addr + 8);
    }

    if (mask.s.pp_h ) {
        AAL_PON_REG_WRITE (prof_cfg->pp_h, reg_addr + 12);
    }
    if (mask.s.pp_l ) {
        AAL_PON_REG_WRITE (prof_cfg->pp_l, reg_addr + 16);
    }

    AAL_XGPON_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_xgpon_usbg_burst_prof_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    prof_idx,
    CA_OUT aal_xgpon_usbg_burst_prof_cfg_t *prof_cfg)
{
    XGPN_USBG_BURST_PF0_INFOR_STS_t  pf_info;
    XGPN_USBG_BURST_PF0_DP_H32_STS_t dp_h32;
    XGPN_USBG_BURST_PF0_DP_L32_STS_t dp_l32;
    XGPN_USBG_BURST_PF0_PP_H32_STS_t pp_h32;
    XGPN_USBG_BURST_PF0_PP_L32_STS_t pp_l32;
    ca_uint32_t  reg_addr = XGPN_USBG_BURST_PF0_INFOR_STS + (prof_idx * 20);

    XGPON_ASSERT(prof_idx < AAL_XGPON_BURST_PROF_NUM);
    XGPON_ASSERT(prof_cfg);

    pf_info.wrd = AAL_PON_REG_READ(reg_addr);
    dp_h32.wrd  = AAL_PON_REG_READ(reg_addr + 4);
    dp_l32.wrd  = AAL_PON_REG_READ(reg_addr + 8);
    pp_h32.wrd  = AAL_PON_REG_READ(reg_addr + 12);
    pp_l32.wrd  = AAL_PON_REG_READ(reg_addr + 16);

    prof_cfg->pcnt = pf_info.bf.pcnt;
    prof_cfg->plen = pf_info.bf.plen;
    prof_cfg->dlen = pf_info.bf.dlen;
    prof_cfg->oh_len = pf_info.bf.oh_len;
    prof_cfg->fecu = pf_info.bf.fecu;
    prof_cfg->ver  = pf_info.bf.ver;
    prof_cfg->vld  = pf_info.bf.vld;
    prof_cfg->dp_h = dp_h32.bf.dp;
    prof_cfg->dp_l = dp_l32.bf.dp;
    prof_cfg->pp_h = pp_h32.bf.pp;
    prof_cfg->pp_l = pp_l32.bf.pp;

    return CA_E_OK;

}

ca_status_t aal_xgpon_usbg_bp_ver_ol_status_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_bp_ver_ol_t *ind)
{
    XGPN_USBG_BP_VER_OL_STS_t bp_ver_ol;

    XGPON_ASSERT(ind);

    bp_ver_ol.wrd = AAL_PON_REG_READ (XGPN_USBG_BP_VER_OL_STS);
    ind->ind0 = bp_ver_ol.bf.ind0;
    ind->ind1 = bp_ver_ol.bf.ind1;
    ind->ind2 = bp_ver_ol.bf.ind2;
    ind->ind3 = bp_ver_ol.bf.ind3;

    return CA_E_OK;

}

ca_status_t aal_xgpon_usbg_mib_ram_status_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *ram_init)
{
    ca_status_t ret = CA_E_OK;
    XGPN_USBG_MIB_RAM_STATUS_REG_t mib_ram_status;

    XGPON_ASSERT(ram_init);

    mib_ram_status.wrd = AAL_PON_REG_READ( XGPN_USBG_MIB_RAM_STATUS_REG);
    *ram_init = mib_ram_status.bf.ram_initdone;

    return ret;
}

ca_status_t aal_xgpon_usbg_mib_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *ram_clr_en)
{
    XGPN_USBG_MIB_CTL_t mib_ctrl;

    XGPON_ASSERT(ram_clr_en);

    mib_ctrl.wrd = AAL_PON_REG_READ( XGPN_USBG_MIB_CTL);
    *ram_clr_en = mib_ctrl.bf.ram_clr_en;
    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_mib_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  ram_clr_en)
{
    XGPN_USBG_MIB_CTL_t mib_ctrl;

    AAL_XGPON_LOCK();

    mib_ctrl.wrd = AAL_PON_REG_READ( XGPN_USBG_MIB_CTL);

    if (mib_ctrl.bf.ram_clr_en != ram_clr_en) {
        mib_ctrl.bf.ram_clr_en = ram_clr_en ;
        AAL_PON_REG_WRITE (mib_ctrl.wrd, XGPN_USBG_MIB_CTL);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_usbg_xgem_mib_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_OUT aal_xgpon_usbg_xgem_mib_config_t *xgem_cnt)
{
    ca_uint64_t tmp = 0 ;
    XGPN_USBG_PORTID_XGEM_MIB_DATA0_t xgem_mib_data0;
    XGPN_USBG_PORTID_XGEM_MIB_DATA1_t xgem_mib_data1;
    XGPN_USBG_PORTID_XGEM_MIB_DATA2_t xgem_mib_data2;

    XGPON_ASSERT(xgem_cnt);
    XGPON_ASSERT(xgem_idx < AAL_XGPON_MAX_GEM_NUM);
    AAL_XGPON_LOCK();
    AAL_PON_REG_INDIRECT_READ(xgem_idx, XGPN_USBG_PORTID_XGEM_MIB_ACCESS);
    xgem_mib_data0.wrd = AAL_PON_REG_READ (XGPN_USBG_PORTID_XGEM_MIB_DATA0);
    xgem_mib_data1.wrd = AAL_PON_REG_READ (XGPN_USBG_PORTID_XGEM_MIB_DATA1);
    xgem_mib_data2.wrd = AAL_PON_REG_READ (XGPN_USBG_PORTID_XGEM_MIB_DATA2);
    
    tmp = xgem_mib_data2.bf.byte_cnt;
    xgem_cnt->frame_cnt = xgem_mib_data0.bf.frame_cnt;
    xgem_cnt->byte_cnt = ca_inter_to64(xgem_mib_data2.wrd,xgem_mib_data1.wrd);
    
    //xgem_cnt->byte_cnt = (tmp<<32)|xgem_mib_data1.bf.byte_cnt;
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_xgem_mib_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t xgem_idx,
    CA_IN aal_xgpon_usbg_xgem_mib_config_t *xgem_cnt)
{
    XGPN_USBG_PORTID_XGEM_MIB_DATA0_t xgem_mib_data0;
    XGPN_USBG_PORTID_XGEM_MIB_DATA1_t xgem_mib_data1;
    XGPN_USBG_PORTID_XGEM_MIB_DATA2_t xgem_mib_data2;

    XGPN_USBG_IDLE_XGEM_FRAME_CNT_t   idle_frame_cnt;
    XGPN_USBG_IDLE_XGEM_BYTE_CNT0_t   idle_byte_cnt0;
    XGPN_USBG_IDLE_XGEM_BYTE_CNT1_t   idle_byte_cnt1;
    XGPON_ASSERT(xgem_cnt);
    XGPON_ASSERT(xgem_idx < AAL_XGPON_MAX_GEM_NUM);

    AAL_XGPON_LOCK();
    AAL_PON_REG_INDIRECT_READ(xgem_idx, XGPN_USBG_PORTID_XGEM_MIB_ACCESS);
    xgem_mib_data0.wrd = AAL_PON_REG_READ (XGPN_USBG_PORTID_XGEM_MIB_DATA0);
    xgem_mib_data1.wrd = AAL_PON_REG_READ (XGPN_USBG_PORTID_XGEM_MIB_DATA1);
    xgem_mib_data2.wrd = AAL_PON_REG_READ (XGPN_USBG_PORTID_XGEM_MIB_DATA2);

    xgem_mib_data0.bf.frame_cnt =  xgem_cnt->frame_cnt;
    xgem_mib_data1.bf.byte_cnt  = (ca_uint32_t)xgem_cnt->byte_cnt;
    xgem_mib_data2.bf.byte_cnt  = xgem_cnt->byte_cnt>>32;

    AAL_PON_REG_WRITE (xgem_mib_data0.wrd, XGPN_USBG_PORTID_XGEM_MIB_DATA0);
    AAL_PON_REG_WRITE (xgem_mib_data1.wrd, XGPN_USBG_PORTID_XGEM_MIB_DATA1);
    AAL_PON_REG_WRITE (xgem_mib_data2.wrd, XGPN_USBG_PORTID_XGEM_MIB_DATA2);
    AAL_PON_REG_INDIRECT_WRITE(xgem_idx, XGPN_USBG_PORTID_XGEM_MIB_ACCESS);
    idle_frame_cnt.wrd = AAL_PON_REG_READ (XGPN_USBG_IDLE_XGEM_FRAME_CNT);
    idle_byte_cnt0.wrd = AAL_PON_REG_READ (XGPN_USBG_IDLE_XGEM_BYTE_CNT0);
    idle_byte_cnt1.wrd = AAL_PON_REG_READ (XGPN_USBG_IDLE_XGEM_BYTE_CNT1);

    idle_frame_cnt.bf.cnt =  xgem_cnt->frame_cnt;
    idle_byte_cnt0.bf.cnt0  = (ca_uint32_t)xgem_cnt->byte_cnt;
    idle_byte_cnt1.bf.cnt1  = xgem_cnt->byte_cnt>>32;

    AAL_PON_REG_WRITE (idle_frame_cnt.wrd, XGPN_USBG_IDLE_XGEM_FRAME_CNT);
    AAL_PON_REG_WRITE (idle_byte_cnt0.wrd, XGPN_USBG_IDLE_XGEM_BYTE_CNT0);
    AAL_PON_REG_WRITE (idle_byte_cnt1.wrd, XGPN_USBG_IDLE_XGEM_BYTE_CNT1);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_usbg_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_usbg_stats_t *stats)
{
    XGPON_ASSERT(stats);

    stats->xgem_lf0_cnt     = AAL_PON_REG_READ (XGPN_USBG_XGEM_LF0);
    stats->xgtc_frm_cnt     = AAL_PON_REG_READ (XGPN_USBG_XGTC_FM_CNT);
    stats->xgtc_fec_blk_cnt = AAL_PON_REG_READ (XGPN_USBG_XGTC_FEC_BLK_CNT);
    stats->ploam_frm_cnt    = AAL_PON_REG_READ (XGPN_USBG_PLM_FM_CNT);

    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_glb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_glb_ctrl_t *bwmp_glb_cfg)
{
    XGPN_BWMP_GBL_CTRL_t bwmp_glb_ctrl;

    XGPON_ASSERT(bwmp_glb_cfg);

    bwmp_glb_ctrl.wrd = AAL_PON_REG_READ( XGPN_BWMP_GBL_CTRL);
    bwmp_glb_cfg->rx_fltr_en = bwmp_glb_ctrl.bf.rx_fltr_en;
    bwmp_glb_cfg->grant_src_mode = bwmp_glb_ctrl.bf.grant_src_mode;
    bwmp_glb_cfg->st_out_chk_en = bwmp_glb_ctrl.bf.st_out_chk_en;
    bwmp_glb_cfg->pre_st_out_chk_en = bwmp_glb_ctrl.bf.pre_st_out_chk_en;
    bwmp_glb_cfg->grt_always_drop_num = bwmp_glb_ctrl.bf.grt_always_drop_num;
    bwmp_glb_cfg->grt_always_drop_en = bwmp_glb_ctrl.bf.grt_always_drop_en;

    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_glb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_bwmp_glb_ctrl_msk_t mask,
    CA_IN aal_xgpon_bwmp_glb_ctrl_t *bwmp_glb_cfg)
{
    XGPN_BWMP_GBL_CTRL_t bwmp_glb_ctrl;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(bwmp_glb_cfg);

    AAL_XGPON_LOCK();

    bwmp_glb_ctrl.wrd = AAL_PON_REG_READ( XGPN_BWMP_GBL_CTRL);
    if (mask.s.rx_fltr_en) {
        if (bwmp_glb_ctrl.bf.rx_fltr_en != bwmp_glb_cfg->rx_fltr_en)
            bwmp_glb_ctrl.bf.rx_fltr_en = bwmp_glb_cfg->rx_fltr_en;
    }
    if (mask.s.grant_src_mode) {
        if (bwmp_glb_ctrl.bf.grant_src_mode != bwmp_glb_cfg->grant_src_mode)
            bwmp_glb_ctrl.bf.grant_src_mode = bwmp_glb_cfg->grant_src_mode ;
    }
    if (mask.s.st_out_chk_en) {
        if (bwmp_glb_ctrl.bf.st_out_chk_en != bwmp_glb_cfg->st_out_chk_en)
            bwmp_glb_ctrl.bf.st_out_chk_en = bwmp_glb_cfg->st_out_chk_en ;
    }
    if (mask.s.pre_st_out_chk_en) {
        if (bwmp_glb_ctrl.bf.pre_st_out_chk_en != bwmp_glb_cfg->pre_st_out_chk_en)
            bwmp_glb_ctrl.bf.pre_st_out_chk_en = bwmp_glb_cfg->pre_st_out_chk_en  ;
    }

    if (mask.s.grt_always_drop_num) {
        if (bwmp_glb_ctrl.bf.grt_always_drop_num != bwmp_glb_cfg->grt_always_drop_num)
            bwmp_glb_ctrl.bf.grt_always_drop_num = bwmp_glb_cfg->grt_always_drop_num ;
    }
    if (mask.s.grt_always_drop_en) {
        if (bwmp_glb_ctrl.bf.grt_always_drop_en != bwmp_glb_cfg->grt_always_drop_en)
            bwmp_glb_ctrl.bf.grt_always_drop_en = bwmp_glb_cfg->grt_always_drop_en  ;
    }
    AAL_PON_REG_WRITE (bwmp_glb_ctrl.wrd, XGPN_BWMP_GBL_CTRL);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

/*@aal_xgpon_bwmp_tcont_get
 *@brief: when tcnt_idx is 0, ONU-ID will be read from hardware
 */
ca_status_t aal_xgpon_bwmp_tcont_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t wave_channel,
    CA_IN ca_uint32_t tcont_idx,
    CA_OUT aal_xgpon_bwmp_alloc_id_cfg_t *alloc_id_cfg)
{
    XGPN_BWMP_ALLOC_LUT_DATA_t bwmp_alloc;

    CA_ASSERT_RET(wave_channel < AAL_XGPON_WAVE_CHANNEL_MAX_NUM, CA_E_PARAM);
    CA_ASSERT_RET(tcont_idx < AAL_XGPON_MAX_TCONT_NUM, CA_E_PARAM);
    CA_ASSERT_RET(alloc_id_cfg != NULL, CA_E_PARAM);
    AAL_XGPON_LOCK();

    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_XGPN_BWMP_ALLOC_REG_INDIRECT_READ(wave_channel, tcont_idx,
        XGPN_BWMP_ALLOC_LUT_ACCESS);

    bwmp_alloc.wrd = AAL_PON_REG_READ(XGPN_BWMP_ALLOC_LUT_DATA);
    alloc_id_cfg->alloc_id = bwmp_alloc.bf.allocation_ID;
    alloc_id_cfg->omci_en  = bwmp_alloc.bf.omci_en;
    alloc_id_cfg->ploam_en = bwmp_alloc.bf.ploam_en;
    AAL_XGPON_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_xgpon_tconts_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t wave_channel)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t tcont_idx = 0;
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;


    ca_printf("\r\n");
    ca_printf("TCONT  Alloc-ID    OMCI-ena    PLOAM-ena\r\n");
    ca_printf("-----------------------------------------\r\n");

    for (tcont_idx = 0; tcont_idx < AAL_XGPON_MAX_TCONT_NUM; tcont_idx++) {

        memset(&alloc_id_cfg, 0, sizeof(alloc_id_cfg));
        ret = aal_xgpon_bwmp_tcont_get(device_id, wave_channel,
            tcont_idx, &alloc_id_cfg);
        if (CA_E_OK != ret) {
            return ret;
        }

        if (!alloc_id_cfg.omci_en && !alloc_id_cfg.ploam_en) {
            continue;
        }

        ca_printf("%-5d  %-8d    %-8d    %-8d\r\n",
            tcont_idx, alloc_id_cfg.alloc_id,
            alloc_id_cfg.omci_en, alloc_id_cfg.ploam_en);
    }

    return CA_E_OK;
}


/*need modify code for register access changed!*/
ca_status_t aal_xgpon_bwmp_tcont_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t wave_channel,
    CA_IN ca_uint32_t tcont_idx,
    CA_IN aal_xgpon_bwmp_alloc_id_cfg_msk_t mask,
    CA_IN aal_xgpon_bwmp_alloc_id_cfg_t *alloc_id_cfg)
{
    XGPN_BWMP_ALLOC_LUT_DATA_t bwmp_alloc;
    CA_ASSERT_RET(wave_channel < AAL_XGPON_WAVE_CHANNEL_MAX_NUM, CA_E_PARAM);
    CA_ASSERT_RET(tcont_idx < AAL_XGPON_MAX_TCONT_NUM, CA_E_PARAM);
    CA_ASSERT_RET(alloc_id_cfg != NULL, CA_E_PARAM);

    AAL_XGPON_LOCK();
    /* uwlchid is always 0 when pon mac mode is not ngpon2 mode */
    AAL_XGPN_BWMP_ALLOC_REG_INDIRECT_READ(wave_channel, tcont_idx, XGPN_BWMP_ALLOC_LUT_ACCESS);
    bwmp_alloc.wrd = AAL_PON_REG_READ(XGPN_BWMP_ALLOC_LUT_DATA);
    AAL_XGPON_UNLOCK();

    if (mask.s.alloc_id) {
        bwmp_alloc.bf.allocation_ID = alloc_id_cfg->alloc_id;
#ifdef CONFIG_LUNA_G3_SERIES
        tmp_alloc_id_cfg[tcont_idx].alloc_id = alloc_id_cfg->alloc_id;
#endif
    }
    if (mask.s.omci_en) {
        bwmp_alloc.bf.omci_en = alloc_id_cfg->omci_en;
#ifdef CONFIG_LUNA_G3_SERIES
        tmp_alloc_id_cfg[tcont_idx].omci_en = alloc_id_cfg->omci_en;
#endif
    }
    if (mask.s.ploam_en) {
        bwmp_alloc.bf.ploam_en      = alloc_id_cfg->ploam_en;
#ifdef CONFIG_LUNA_G3_SERIES
        tmp_alloc_id_cfg[tcont_idx].ploam_en = alloc_id_cfg->ploam_en;
#endif
    }
    AAL_XGPON_LOCK();
    AAL_PON_REG_WRITE(bwmp_alloc.wrd, XGPN_BWMP_ALLOC_LUT_DATA);
    AAL_XGPN_BWMP_ALLOC_REG_INDIRECT_WRITE(wave_channel, tcont_idx, XGPN_BWMP_ALLOC_LUT_ACCESS);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_tcont_clr(
    ca_device_id_t device_id,
    ca_uint32_t tcont_start,
    ca_uint32_t tcont_end)
{
    ca_status_t ret   = CA_E_OK;
    ca_uint32_t chn   = 0;
    ca_uint32_t tcont = 0;
    aal_xgpon_bwmp_alloc_id_cfg_msk_t alloc_id_mask;
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;

    alloc_id_mask.u32        = 0;
    alloc_id_mask.s.alloc_id = 1;
    alloc_id_mask.s.omci_en  = 1;
    alloc_id_mask.s.ploam_en = 1;

    alloc_id_cfg.alloc_id = AAL_XGPON_ALLOC_ID_INVALID;
    alloc_id_cfg.omci_en  = 1;
    alloc_id_cfg.ploam_en = 0;

    /* tcont_num_per_chip */
    for(chn = 0 ;chn < AAL_XGPON_WAVE_CHANNEL_MAX_NUM; chn++){
        for(tcont = tcont_start; tcont < tcont_end; tcont++){
            ret = aal_xgpon_bwmp_tcont_set(device_id, chn, tcont, alloc_id_mask, &alloc_id_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
    }

    return CA_E_OK;

}
ca_status_t aal_xgpon_bwmp_rsp_tm_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *time)
{
    XGPN_BWMP_RSP_TM_CFG_t rsp_tm_cfg;

    XGPON_ASSERT(time);

    rsp_tm_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_RSP_TM_CFG);
    *time = rsp_tm_cfg.bf.time;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_rsp_tm_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  time)
{
    XGPN_BWMP_RSP_TM_CFG_t rsp_tm_cfg;
    AAL_XGPON_LOCK();

    rsp_tm_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_RSP_TM_CFG);

    if (rsp_tm_cfg.bf.time != time) {
        rsp_tm_cfg.bf.time = time ;
        AAL_PON_REG_WRITE (rsp_tm_cfg.wrd, XGPN_BWMP_RSP_TM_CFG);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_bwmp_bst_mg_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *time)
{
    XGPN_BWMP_BST_MG_CFG_t bst_mg_cfg;

    XGPON_ASSERT(time);

    bst_mg_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_BST_MG_CFG);
    *time = bst_mg_cfg.bf.time;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_bst_mg_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  time)
{
    XGPN_BWMP_BST_MG_CFG_t bst_mg_cfg;
    AAL_XGPON_LOCK();

    bst_mg_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_BST_MG_CFG);

    if (bst_mg_cfg.bf.time != time) {
        bst_mg_cfg.bf.time = time ;
        AAL_PON_REG_WRITE (bst_mg_cfg.wrd, XGPN_BWMP_BST_MG_CFG);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_bwmp_seq_tm_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *map_lead_tm)
{
    XGPN_BWMP_SEQ_TM_CTRL_t seq_tm_cfg;

    XGPON_ASSERT(map_lead_tm);

    seq_tm_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_SEQ_TM_CTRL);
    *map_lead_tm = seq_tm_cfg.bf.map_lead_tm;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_seq_tm_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  map_lead_tm)
{
    XGPN_BWMP_SEQ_TM_CTRL_t seq_tm_cfg;
    AAL_XGPON_LOCK();

    seq_tm_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_SEQ_TM_CTRL);

    if (seq_tm_cfg.bf.map_lead_tm != map_lead_tm) {
        seq_tm_cfg.bf.map_lead_tm = map_lead_tm ;
        AAL_PON_REG_WRITE (seq_tm_cfg.wrd, XGPN_BWMP_SEQ_TM_CTRL);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_bwmp_msg_ctl_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t idx,
    CA_OUT aal_xgpon_bwmp_grt_msg_ctrl_cfg_t *config)
{
    ca_status_t ret = CA_E_OK;
    XGPN_BWMP_GRT_MSG_DATA0_t data0;
    XGPN_BWMP_GRT_MSG_DATA1_t data1;

    XGPON_ASSERT(idx == 0);
    XGPON_ASSERT(config);

    AAL_XGPON_LOCK();

    AAL_XGPN_MSG_CTL_REG_INDIRECT_READ(idx, XGPN_BWMP_GRT_MSG_CTL);
    data0.wrd = AAL_PON_REG_READ (XGPN_BWMP_GRT_MSG_DATA0);
    data1.wrd = AAL_PON_REG_READ (XGPN_BWMP_GRT_MSG_DATA1);
    config->infor0 = data0.bf.infor;
    config->infor1 = data1.bf.infor;
    AAL_XGPON_UNLOCK();

    return ret;

}
ca_status_t aal_xgpon_bwmp_msg_ctl_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t idx,
    CA_IN aal_xgpon_bwmp_grt_msg_ctrl_cfg_msk_t mask,
    CA_IN aal_xgpon_bwmp_grt_msg_ctrl_cfg_t *config)
{
    ca_status_t ret = CA_E_OK;

    XGPN_BWMP_GRT_MSG_DATA0_t data0;
    XGPN_BWMP_GRT_MSG_DATA1_t data1;

    if (!mask.u32) {
        return CA_E_OK;
    }

    XGPON_ASSERT(idx == 0);
    XGPON_ASSERT(config);

    AAL_XGPON_LOCK();

    AAL_XGPN_MSG_CTL_REG_INDIRECT_READ(idx, XGPN_BWMP_GRT_MSG_CTL);
    data0.wrd = AAL_PON_REG_READ (XGPN_BWMP_GRT_MSG_DATA0);
    data1.wrd = AAL_PON_REG_READ (XGPN_BWMP_GRT_MSG_DATA1);

    if (mask.s.infor0) {
        data0.bf.infor = config->infor0;
    }
    if (mask.s.infor1) {
        data1.bf.infor = config->infor1;
    }


    AAL_PON_REG_WRITE (data0.wrd, XGPN_BWMP_GRT_MSG_DATA0);
    AAL_PON_REG_WRITE (data1.wrd, XGPN_BWMP_GRT_MSG_DATA1);

    AAL_XGPN_MSG_CTL_REG_INDIRECT_WRITE(idx, XGPN_BWMP_GRT_MSG_CTL);
    AAL_XGPON_UNLOCK();
    return ret;

}

ca_status_t aal_xgpon_bwmp_grt_ff_ctrl_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t ff_idx,
    CA_IN ca_uint32_t grant_ff_sel, /*0: BWmap grant fifo1: XGTC grant fifo2: BM grant fifo3: reserve */
    CA_OUT aal_xgpon_bwmp_grt_ff_cfg_t *grf_ff_cfg)
{
    ca_status_t ret = CA_E_OK;
    XGPN_BWMP_GRT_FF_DATA0_t data0;
    XGPN_BWMP_GRT_FF_DATA1_t data1;

    XGPON_ASSERT(grf_ff_cfg);

    if (ff_idx >= AAL_BWMP_GRANT_FIFO_MAX_IDX) {
        ca_printf("ERROR ff_idx:%d\r\n", ff_idx);
        return CA_E_PARAM;
    }
    if (grant_ff_sel > 3) {
        ca_printf("ERROR grant_ff_sel:%d\r\n", grant_ff_sel);
        return CA_E_PARAM;
    }

    AAL_XGPON_LOCK();
    AAL_XGPN_BWMP_REG_INDIRECT_READ(grant_ff_sel, ff_idx, XGPN_BWMP_GRT_FF_CTL);
    data0.wrd = AAL_PON_REG_READ (XGPN_BWMP_GRT_FF_DATA0);
    data1.wrd = AAL_PON_REG_READ (XGPN_BWMP_GRT_FF_DATA1);
    AAL_XGPON_UNLOCK();

    grf_ff_cfg->grant_size = data0.bf.grant_size;
    grf_ff_cfg->st_tm = data0.bf.st_tm;
    grf_ff_cfg->bp_ind = data1.bf.bp_ind;
    grf_ff_cfg->fecu = data1.bf.fecu;
    grf_ff_cfg->ploamu = data1.bf.ploamu;
    grf_ff_cfg->dbru = data1.bf.dbru;
    grf_ff_cfg->tcont_id = data1.bf.tcont_id;
    grf_ff_cfg->tcont_en = data1.bf.tcont_en;

    return ret;

}


ca_status_t aal_xgpon_bwmp_bm_buf_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *wr_threshold)
{
    XGPN_BWMP_BM_BUF_CFG_t bm_buf_cfg;

    XGPON_ASSERT(wr_threshold);

    bm_buf_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_BM_BUF_CFG);
    *wr_threshold = bm_buf_cfg.bf.wr_threshold;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_bm_buf_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  wr_threshold)
{
    XGPN_BWMP_BM_BUF_CFG_t bm_buf_cfg;
    AAL_XGPON_LOCK();

    bm_buf_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_BM_BUF_CFG);

    if (bm_buf_cfg.bf.wr_threshold != wr_threshold) {
        bm_buf_cfg.bf.wr_threshold = wr_threshold ;
        AAL_PON_REG_WRITE (bm_buf_cfg.wrd, XGPN_BWMP_BM_BUF_CFG);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_eqd_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data)
{
    XGPN_BWMP_EQD_STS_t eqd_sts;

    XGPON_ASSERT(data);

    eqd_sts.wrd = AAL_PON_REG_READ( XGPN_BWMP_EQD_STS);
    *data = eqd_sts.bf.data;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_eqd_sts_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  data)
{
    XGPN_BWMP_EQD_STS_t eqd_sts;
    AAL_XGPON_LOCK();

    eqd_sts.wrd = AAL_PON_REG_READ( XGPN_BWMP_EQD_STS);

    if (eqd_sts.bf.data != data) {
        eqd_sts.bf.data = data ;
        AAL_PON_REG_WRITE (eqd_sts.wrd, XGPN_BWMP_EQD_STS);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_bwmp_random_dly_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *data)
{
    XGPN_BWMP_RANDOM_DLY_STS_t rand_dly;

    XGPON_ASSERT(data);
    rand_dly.wrd = AAL_PON_REG_READ( XGPN_BWMP_RANDOM_DLY_STS);
    *data = rand_dly.bf.data;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_lmt_drop_src_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_lmt_drop_src_cfg_t *lmt_drp_src_config)
{
    XGPN_BWMP_LMT_DROP_SRC_STS_t lmt_drp_src_cfg;

    XGPON_ASSERT(lmt_drp_src_config);

    AAL_XGPON_LOCK();

    lmt_drp_src_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_LMT_DROP_SRC_STS);

#if defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA)
    lmt_drp_src_config->all_onu_num_os = lmt_drp_src_cfg.bf.all_onu_num_os;
    lmt_drp_src_config->alloc_peronu_num_os = lmt_drp_src_cfg.bf.alloc_peronu_num_os;
#endif
    lmt_drp_src_config->bst_series_num_os = lmt_drp_src_cfg.bf.bst_series_num_os;
    lmt_drp_src_config->bst_sz_sum_os = lmt_drp_src_cfg.bf.bst_sz_sum_os;
    lmt_drp_src_config->alloc_sz_chk_vld_os = lmt_drp_src_cfg.bf.alloc_sz_chk_vld_os;
    lmt_drp_src_config->st_chk_fail = lmt_drp_src_cfg.bf.st_chk_fail;
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_lmt_drop_src_sts_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_bwmp_lmt_drop_src_cfg_msk_t mask,
    CA_IN aal_xgpon_bwmp_lmt_drop_src_cfg_t *lmt_drp_src_config)
{
    ca_status_t ret = CA_E_OK;
    XGPN_BWMP_LMT_DROP_SRC_STS_t lmt_drp_src_cfg;

    if (!mask.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(lmt_drp_src_config);

    AAL_XGPON_LOCK();

    lmt_drp_src_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_LMT_DROP_SRC_STS);

    if (mask.s.all_onu_num_os) {
        if (lmt_drp_src_cfg.bf.total_alloc_num_os != lmt_drp_src_config->all_onu_num_os)
            lmt_drp_src_cfg.bf.total_alloc_num_os = lmt_drp_src_config->all_onu_num_os;
    }
    if (mask.s.alloc_peronu_num_os) {
        if (lmt_drp_src_cfg.bf.per_bst_alloc_num_os != lmt_drp_src_config->alloc_peronu_num_os)
            lmt_drp_src_cfg.bf.per_bst_alloc_num_os = lmt_drp_src_config->alloc_peronu_num_os ;
    }



    if (mask.s.bst_series_num_os) {
        if (lmt_drp_src_cfg.bf.bst_series_num_os != lmt_drp_src_config->bst_series_num_os)
            lmt_drp_src_cfg.bf.bst_series_num_os = lmt_drp_src_config->bst_series_num_os ;
    }
    if (mask.s.bst_sz_sum_os) {
        if (lmt_drp_src_cfg.bf.bst_sz_sum_os != lmt_drp_src_config->bst_sz_sum_os)
            lmt_drp_src_cfg.bf.bst_sz_sum_os = lmt_drp_src_config->bst_sz_sum_os  ;
    }
    if (mask.s.alloc_sz_chk_vld_os) {
        if (lmt_drp_src_cfg.bf.alloc_sz_chk_vld_os != lmt_drp_src_config->alloc_sz_chk_vld_os)
            lmt_drp_src_cfg.bf.alloc_sz_chk_vld_os = lmt_drp_src_config->alloc_sz_chk_vld_os ;
    }
    if (mask.s.st_chk_fail) {
        if (lmt_drp_src_cfg.bf.st_chk_fail != lmt_drp_src_config->st_chk_fail)
            lmt_drp_src_cfg.bf.st_chk_fail = lmt_drp_src_config->st_chk_fail  ;
    }
    AAL_PON_REG_WRITE (lmt_drp_src_cfg.wrd, XGPN_BWMP_LMT_DROP_SRC_STS);
    AAL_XGPON_UNLOCK();

    return ret;
}


ca_status_t aal_xgpon_bwmp_bwmap_grant_ff_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_bwmap_grt_ff_cfg_t *grt_ff_config)
{
    XGPN_BWMP_BWMAP_GRT_FF_STS_t grt_ff_cfg;
    XGPON_ASSERT(grt_ff_config);

    grt_ff_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_BWMAP_GRT_FF_STS);
    grt_ff_config->wr_ptr = grt_ff_cfg.bf.wr_ptr;
    grt_ff_config->wr_sel = grt_ff_cfg.bf.wr_sel;
    grt_ff_config->rd_ptr = grt_ff_cfg.bf.rd_ptr;
    grt_ff_config->rd_sel = grt_ff_cfg.bf.rd_sel;

    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_xgtc_grt_ff_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_xgtc_grt_ff_cfg_t *xgtc_grt_ff_config)
{
    XGPN_BWMP_XGTC_GRT_FF_STS_t xgtc_grt_ff_cfg;

    XGPON_ASSERT(xgtc_grt_ff_config);

    xgtc_grt_ff_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_XGTC_GRT_FF_STS);
    xgtc_grt_ff_config->wr_ptr = xgtc_grt_ff_cfg.bf.wr_ptr;
    xgtc_grt_ff_config->wr_sel = xgtc_grt_ff_cfg.bf.wr_sel;
    xgtc_grt_ff_config->rd_ptr = xgtc_grt_ff_cfg.bf.rd_ptr;
    xgtc_grt_ff_config->rd_sel = xgtc_grt_ff_cfg.bf.rd_sel;

    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_bm_grant_ff_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_bwmp_bm_grt_ff_cfg_t *bm_grt_ff_config)
{
    XGPN_BWMP_BM_GRT_FF_STS_t bm_grt_ff_cfg;

    XGPON_ASSERT(bm_grt_ff_config);

    bm_grt_ff_cfg.wrd = AAL_PON_REG_READ( XGPN_BWMP_BM_GRT_FF_STS);
    bm_grt_ff_config->wr_ptr = bm_grt_ff_cfg.bf.wr_ptr;
    bm_grt_ff_config->rd_ptr = bm_grt_ff_cfg.bf.rd_ptr;
    return CA_E_OK;
}


ca_status_t aal_xgpon_bwmp_unk_bf_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_BWMP_UNK_PF_MIB_t unk_bf;

    XGPON_ASSERT(cnt);

    unk_bf.wrd = AAL_PON_REG_READ (XGPN_BWMP_UNK_PF_MIB);
    *cnt = unk_bf.bf.cntr;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_as_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_BWMP_AS_CNT_t as_cnt;

    XGPON_ASSERT(cnt);

    as_cnt.wrd = AAL_PON_REG_READ (XGPN_BWMP_AS_CNT);
    *cnt = as_cnt.bf.entry;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_as_hec_err_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_BWMP_AS_HEC_ERR_CNT_t as_hec_err_cnt;

    XGPON_ASSERT(cnt);

    as_hec_err_cnt.wrd = AAL_PON_REG_READ (XGPN_BWMP_AS_HEC_ERR_CNT);
    *cnt = as_hec_err_cnt.bf.entry;

    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_cnt_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *cnt_ctl)
{
    XGPN_BWMP_BWMAP_CNT_CTRL_t cnt_ctrl;

    XGPON_ASSERT(cnt_ctl);

    cnt_ctrl.wrd = AAL_PON_REG_READ( XGPN_BWMP_BWMAP_CNT_CTRL);
    *cnt_ctl = cnt_ctrl.bf.latch;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_cnt_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t  cnt_ctl)
{
    XGPN_BWMP_BWMAP_CNT_CTRL_t cnt_ctrl;

    AAL_XGPON_LOCK();
    cnt_ctrl.wrd = AAL_PON_REG_READ( XGPN_BWMP_BWMAP_CNT_CTRL);

    if (cnt_ctrl.bf.latch != cnt_ctl) {
        cnt_ctrl.bf.latch = cnt_ctl ;
        AAL_PON_REG_WRITE (cnt_ctrl.wrd, XGPN_BWMP_BWMAP_CNT_CTRL);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_xgpon_bwmp_ds_fm_sync_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_BWMP_DS_FM_SYNC_CNT_t fm_sync_cnt;

    XGPON_ASSERT(cnt);

    fm_sync_cnt.wrd = AAL_PON_REG_READ (XGPN_BWMP_DS_FM_SYNC_CNT);
    *cnt = fm_sync_cnt.bf.cnt;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_us_map_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_BWMP_US_MAP_CNT_t us_map_cnt;
    XGPON_ASSERT(cnt);

    us_map_cnt.wrd = AAL_PON_REG_READ (XGPN_BWMP_US_MAP_CNT);
    *cnt = us_map_cnt.bf.cnt;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_us_xgem_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_BWMP_US_XGEM_CNT_t us_xgem_cnt;

    XGPON_ASSERT(cnt);

    us_xgem_cnt.wrd = AAL_PON_REG_READ (XGPN_BWMP_US_XGEM_CNT);
    *cnt = us_xgem_cnt.bf.cnt;
    return CA_E_OK;
}

ca_status_t aal_xgpon_bwmp_us_xgtc_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_BWMP_US_XGTC_CNT_t us_xgtc_cnt;

    XGPON_ASSERT(cnt);

    us_xgtc_cnt.wrd = AAL_PON_REG_READ (XGPN_BWMP_US_XGTC_CNT);
    *cnt = us_xgtc_cnt.bf.cnt;
    return CA_E_OK;
}


ca_status_t aal_xgpon_rxctl_rx_port_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *spid)
{
    XGPN_RXCTL_rx_port_t rx_port_cfg;

    XGPON_ASSERT(spid);

    rx_port_cfg.wrd = AAL_PON_REG_READ( XGPN_RXCTL_rx_port);
    *spid = rx_port_cfg.bf.spid;
    return CA_E_OK;
}

ca_status_t aal_xgpon_rxctl_rx_port_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t spid)
{
    XGPN_RXCTL_rx_port_t rx_port_cfg;

    AAL_XGPON_LOCK();

    rx_port_cfg.wrd = AAL_PON_REG_READ( XGPN_RXCTL_rx_port);
    if (rx_port_cfg.bf.spid != spid) {
        rx_port_cfg.bf.spid = spid ;
        AAL_PON_REG_WRITE (rx_port_cfg.wrd, XGPN_RXCTL_rx_port);
    }
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_rxctl_rx_pkt_ctl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpon_rxctl_rx_pkt_ctrl_cfg_t  *rx_pkt_ctrl_cfg)
{
    XGPN_RXCTL_rx_pkt_ctl_t rx_pkt_ctrl_config;

    XGPON_ASSERT(rx_pkt_ctrl_cfg);

    rx_pkt_ctrl_config.wrd = AAL_PON_REG_READ( XGPN_RXCTL_rx_pkt_ctl);
    rx_pkt_ctrl_cfg->frame_oversize_drop_en = rx_pkt_ctrl_config.bf.frame_oversize_drop_en;
    rx_pkt_ctrl_cfg->frame_runt_drop_en = rx_pkt_ctrl_config.bf.frame_runt_drop_en;
    rx_pkt_ctrl_cfg->frame_length_mismatch_drop_en = rx_pkt_ctrl_config.bf.frame_length_mismatch_drop_en;
    rx_pkt_ctrl_cfg->frame_fcs_error_drop_en = rx_pkt_ctrl_config.bf.frame_fcs_error_drop_en;
    rx_pkt_ctrl_cfg->frame_ffoverflow_drop_en = rx_pkt_ctrl_config.bf.frame_ffoverflow_drop_en;
    rx_pkt_ctrl_cfg->fragment_drop_en = rx_pkt_ctrl_config.bf.fragment_drop_en;
    rx_pkt_ctrl_cfg->omci_runt_drop_en = rx_pkt_ctrl_config.bf.omci_runt_drop_en;
    rx_pkt_ctrl_cfg->drop_ptp_pkt_type = rx_pkt_ctrl_config.bf.drop_ptp_pkt_type;
    rx_pkt_ctrl_cfg->ptp_da_en = rx_pkt_ctrl_config.bf.ptp_da_en;
    rx_pkt_ctrl_cfg->ptp_mode_sel = rx_pkt_ctrl_config.bf.ptp_mode_sel;
    rx_pkt_ctrl_cfg->ptp_ing_ts_cmp = rx_pkt_ctrl_config.bf.ptp_ing_ts_cmp;
    rx_pkt_ctrl_cfg->omci_pad_en = rx_pkt_ctrl_config.bf.omci_pad_en;
    rx_pkt_ctrl_cfg->cripple_frame_drop_en = rx_pkt_ctrl_config.bf.cripple_frame_drop_en;

    return CA_E_OK;
}

ca_status_t aal_xgpon_rxctl_rx_pkt_ctl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpon_rxctl_rx_pkt_ctrl_cfg_msk_t  mask,
    CA_IN aal_xgpon_rxctl_rx_pkt_ctrl_cfg_t  *rx_pkt_ctrl_cfg)
{
    XGPN_RXCTL_rx_pkt_ctl_t rx_pkt_ctrl_config;

    XGPON_ASSERT(rx_pkt_ctrl_cfg);

    AAL_XGPON_LOCK();
    rx_pkt_ctrl_config.wrd = AAL_PON_REG_READ( XGPN_RXCTL_rx_pkt_ctl);
    if (mask.s.frame_oversize_drop_en) {
        rx_pkt_ctrl_config.bf.frame_oversize_drop_en = rx_pkt_ctrl_cfg->frame_oversize_drop_en ;
    }

    if (mask.s.frame_runt_drop_en) {
        rx_pkt_ctrl_config.bf.frame_runt_drop_en = rx_pkt_ctrl_cfg->frame_runt_drop_en ;
    }

    if (mask.s.frame_length_mismatch_drop_en) {
        rx_pkt_ctrl_config.bf.frame_length_mismatch_drop_en = rx_pkt_ctrl_cfg->frame_length_mismatch_drop_en ;
    }

    if (mask.s.frame_fcs_error_drop_en) {
        rx_pkt_ctrl_config.bf.frame_fcs_error_drop_en = rx_pkt_ctrl_cfg->frame_fcs_error_drop_en ;
    }

    if (mask.s.frame_ffoverflow_drop_en) {
        rx_pkt_ctrl_config.bf.frame_ffoverflow_drop_en = rx_pkt_ctrl_cfg->frame_ffoverflow_drop_en ;
    }

    if (mask.s.fragment_drop_en) {
        rx_pkt_ctrl_config.bf.fragment_drop_en = rx_pkt_ctrl_cfg->fragment_drop_en ;
    }

    if (mask.s.omci_runt_drop_en) {
        rx_pkt_ctrl_config.bf.omci_runt_drop_en = rx_pkt_ctrl_cfg->omci_runt_drop_en ;
    }

    if (mask.s.drop_ptp_pkt_type) {
        rx_pkt_ctrl_config.bf.drop_ptp_pkt_type = rx_pkt_ctrl_cfg->drop_ptp_pkt_type ;
    }

    if (mask.s.ptp_da_en) {
        rx_pkt_ctrl_config.bf.ptp_da_en = rx_pkt_ctrl_cfg->ptp_da_en ;
    }

    if (mask.s.ptp_mode_sel) {
        rx_pkt_ctrl_config.bf.ptp_mode_sel = rx_pkt_ctrl_cfg->ptp_mode_sel ;
    }

    if (mask.s.ptp_ing_ts_cmp) {
        rx_pkt_ctrl_config.bf.ptp_ing_ts_cmp = rx_pkt_ctrl_cfg->ptp_ing_ts_cmp ;
    }

    if (mask.s.cripple_frame_drop_en) {
        rx_pkt_ctrl_config.bf.cripple_frame_drop_en = rx_pkt_ctrl_cfg->cripple_frame_drop_en ;
    }

    if (mask.s.omci_pad_en) {
        rx_pkt_ctrl_config.bf.omci_pad_en = rx_pkt_ctrl_cfg->omci_pad_en ;
    }

    AAL_PON_REG_WRITE (rx_pkt_ctrl_config.wrd, XGPN_RXCTL_rx_pkt_ctl);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}



ca_status_t aal_xgpon_rxctl_ds_gem_drop_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t  *cnt)
{
    XGPN_RXCTL_discarded_frames_t drop_cnt;

    XGPON_ASSERT(cnt);
    drop_cnt.wrd = AAL_PON_REG_READ( XGPN_RXCTL_discarded_frames);
    *cnt = drop_cnt.bf.cnt;

    return CA_E_OK;
}

ca_status_t aal_xgpon_rxctl_ds_gem_drop_cnt_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cnt)
{
    ca_status_t ret = CA_E_OK;
    XGPN_RXCTL_discarded_frames_t drop_cnt;

    AAL_XGPON_LOCK();

    drop_cnt.wrd = AAL_PON_REG_READ( XGPN_RXCTL_discarded_frames);

    if (drop_cnt.bf.cnt != cnt) {
        drop_cnt.bf.cnt = cnt ;
        AAL_PON_REG_WRITE (drop_cnt.wrd, XGPN_RXCTL_discarded_frames);
    }
    AAL_XGPON_UNLOCK();

    return ret;
}

ca_status_t aal_xgpon_rxctl_rc1_ff_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_RXCTL_rc_status1_t rc_ff_cnt;

    XGPON_ASSERT(cnt);

    rc_ff_cnt.wrd = AAL_PON_REG_READ (XGPN_RXCTL_rc_status1);
    *cnt = rc_ff_cnt.bf.rcff_pktnum;
    return CA_E_OK;
}

ca_status_t aal_xgpon_rxctl_rc2_ff_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    XGPN_RXCTL_rc_status2_t rc_ff_cnt;

    XGPON_ASSERT(cnt);

    rc_ff_cnt.wrd = AAL_PON_REG_READ (XGPN_RXCTL_rc_status2);
    *cnt = rc_ff_cnt.bf.ff;
    return CA_E_OK;
}

ca_status_t aal_xgpon_rxctl_usbg_dbg_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *state)
{
    XGPN_RXCTL_usbg_dbg_t usbg_dbg_sts;

    XGPON_ASSERT(state);

    usbg_dbg_sts.wrd = AAL_PON_REG_READ (XGPN_RXCTL_usbg_dbg);
    *state = usbg_dbg_sts.bf.state;
    return CA_E_OK;
}

/* @aal_xgpon_tcont_flush
 * Flush TCONT in XGPON MAC
 */
ca_status_t aal_xgpon_tcont_flush(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint8_t   uwlch_id,
    CA_IN ca_uint8_t   tcont_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t timeout = __TCONT_FLUSH_TIMEOUT;
    aal_xgpon_xgpn_glb_ctrl_config_msk_t xgpn_glb_ctrl_msk;
    aal_xgpon_xgpn_glb_ctrl_config_t xgpn_glb_ctrl;

    AAL_ASSERT_RET(uwlch_id < AAL_XGPON_WAVE_CHANNEL_MAX_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(tcont_id < AAL_XGPON_MAX_TCONT_NUM, CA_E_PARAM);

    xgpn_glb_ctrl_msk.u32 = 0;
    memset(&xgpn_glb_ctrl, 0, sizeof(aal_xgpon_xgpn_glb_ctrl_config_t));

    xgpn_glb_ctrl_msk.s.flush_id = 1;
    xgpn_glb_ctrl_msk.s.flush_en = 1;
    xgpn_glb_ctrl_msk.s.uwlch_id = 1;
    xgpn_glb_ctrl.uwlch_id = uwlch_id;
    xgpn_glb_ctrl.flush_id = tcont_id;
    xgpn_glb_ctrl.flush_en = 1;

    ret = aal_xgpon_xgpn_glb_ctrl_set(device_id,xgpn_glb_ctrl_msk, &xgpn_glb_ctrl);
    if (CA_E_OK != ret) {
        return ret;
    }
#ifdef CONFIG_LUNA_G3_SERIES
    ca_mdelay(1);
#else
    do { /* wait FLUSH done */
        (void)aal_xgpon_xgpn_glb_ctrl_get(device_id, &xgpn_glb_ctrl);
        if (!xgpn_glb_ctrl.flush_en)
            break;
    }while(timeout-- > 0);
#endif
    if (timeout == 0) {
        ca_printf("flush chn %u tcont %u timeout.\r\n", uwlch_id, tcont_id);
        return CA_E_OK;//CA_E_TIMEOUT;
    }

    xgpn_glb_ctrl.flush_en = 0;
    (void)aal_xgpon_xgpn_glb_ctrl_set(device_id,xgpn_glb_ctrl_msk, &xgpn_glb_ctrl);

    return CA_E_OK;
}

static ca_status_t __glb_gpon_ctrl_init(ca_device_id_t device_id,ca_uint32_t pon_mac_mode)
{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) ||defined(CONFIG_ARCH_CORTINA_G3HGU)
    AAL_GLB_REG_WRITE(0x00E, GLOBAL_PON_CNTL);  /* reset PDC,PUC,PSDS */
    AAL_GLB_REG_WRITE(0x300, GLOBAL_GPON_CNTL); /* reset GPON */
    ca_mdelay(AAL_XGPON_INIT_DELAY);
    AAL_GLB_REG_WRITE(0x30E, GLOBAL_PON_CNTL); /* Enable PDC,PUC,PSDS */
    AAL_GLB_REG_WRITE(0x307, GLOBAL_GPON_CNTL); /* Enable GPON */
    ca_mdelay(AAL_XGPON_INIT_DELAY);
#endif
    ca_printf("%s::%d:Reset global register!!\r\n",__FUNCTION__,__LINE__);
    AAL_GLB_REG_WRITE(0x00E, GLOBAL_PON_CNTL);  /* reset PDC,PUC,PSDS */
    AAL_GLB_REG_WRITE(0x300, GLOBAL_GPON_CNTL); /* reset GPON */
    ca_mdelay(AAL_XGPON_INIT_DELAY);
    AAL_GLB_REG_WRITE(0x30E, GLOBAL_PON_CNTL); /* Enable PDC,PUC,PSDS */
    AAL_GLB_REG_WRITE(0x307, GLOBAL_GPON_CNTL); /* Enable GPON */
    ca_mdelay(AAL_XGPON_INIT_DELAY);

    /* Enable PON H/W blocks */
    switch (pon_mac_mode){
        case ONU_PON_MAC_GPON:
            AAL_GLB_REG_WRITE(0x003, GLOBAL_GPON_CNTL); /* Enable GPON */
            break;
        case ONU_PON_MAC_XGPON1:
            AAL_GLB_REG_WRITE(0x205, GLOBAL_GPON_CNTL); /* Enable GPON */
            break;
        case ONU_PON_MAC_XGSPON:
        case ONU_PON_MAC_NGPON2:
            AAL_GLB_REG_WRITE(0x305, GLOBAL_GPON_CNTL); /* Enable GPON */
            break;
        default:
            ca_printf("%s::%d:Invalid gpon mode!!\r\n",__FUNCTION__,__LINE__);
            break;
    }

    //AAL_GLB_REG_WRITE(0x00, GLOBAL_OPT_MODULE_CNTL); /* Enable Optical transceiver */
    ca_mdelay(AAL_XGPON_INIT_DELAY);

    //ca_printf("ca-pon: optical transceiver status = %#x!\r\n",
    //    AAL_GLB_REG_READ(GLOBAL_OPT_MODULE_STATUS));

    return CA_E_OK;

}
static ca_status_t __xgpon_bwmp_init(ca_device_id_t device_id,ca_uint32_t pon_mac_mode)
{
    ca_status_t ret = CA_E_OK;

    switch (pon_mac_mode){
        case ONU_PON_MAC_GPON:

            break;

        case ONU_PON_MAC_XGPON1:
        case ONU_PON_MAC_XGSPON:
        case ONU_PON_MAC_NGPON2:
            ret = aal_xgpon_bwmp_bst_mg_cfg_set(device_id, AAL_XGPON_BWMP_BURST_MARGIN_TIME_DEFAULT);
            CA_RET_VALUE_CHECK(ret);
			ret = aal_xgpon_bwmp_rsp_tm_set(device_id,g_xgpn_basic_info.bwmp_rsp_tm);
			CA_RET_VALUE_CHECK(ret);

            /*
             * US burst window request ahead time before the actual burst
             * transferred in 2.5G 2byte unit(6.43ns). The minimal value
             * should satisfy data scheduling latency +XGEM frame mapping
             * latency +  AES data path delay (about 20 cycle).
             * The maximalvalue is 0x1500(<35us). The suggested value is 0x400
             * which is the depth ofXGEM buffer 1024 x 64b.
             */
            ret = aal_xgpon_bwmp_seq_tm_cfg_set(device_id, AAL_XGPON_BWMP_SEQ_AHEAD_TIME_DEFAULT);
            CA_RET_VALUE_CHECK(ret);
            break;

        default:
            ca_printf("%s::%d:Invalid gpon mode!!\r\n",__FUNCTION__,__LINE__);
            break;

    }

    return ret;
}

static ca_status_t __xgpon_dsfp_init(ca_device_id_t device_id, ca_uint32_t pon_mac_mode)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_dsfp_xgtc_ctrl_config_msk_t   dsfp_cfg_msk;
    aal_xgpon_dsfp_xgtc_ctrl_config_t       dsfp_cfg;
    aal_xgpon_dsfp_rxmac_ctrl_config_msk_t  rxmac_cfg_mask; 
    aal_xgpon_dsfp_rxmac_ctrl_config_t      rxmac_cfg;

    dsfp_cfg_msk.u32 = 0;
    memset(&dsfp_cfg, 0, sizeof(dsfp_cfg));

    dsfp_cfg_msk.s.ds_en = 1;
    dsfp_cfg_msk.s.ploam_en = 1;
    dsfp_cfg_msk.s.bwmp_en = 1;
    dsfp_cfg_msk.s.hec_chk_en = 1;
    dsfp_cfg_msk.s.drop_xgtc_frame_en = 1;
    dsfp_cfg.ds_en = 1;
    dsfp_cfg.ploam_en = 1;
    dsfp_cfg.bwmp_en = 1;
    dsfp_cfg.hec_chk_en = 1;
    dsfp_cfg.drop_xgtc_frame_en = 1; /* enable to drop the whole XGTC frame when XGTC HEC error isun-correctable*/
    ret = aal_xgpon_dsfp_xgtc_ctrl_set(device_id, dsfp_cfg_msk, &dsfp_cfg);
    if (CA_E_OK != ret) {
        ca_printf("Err: aal_xgpon_dsfp_xgtc_ctrl_set FAIL! \r\n");
        return ret;
    }

    //support OMCI extern message
    rxmac_cfg_mask.u32 = 0;
    memset(&rxmac_cfg, 0, sizeof(rxmac_cfg));
    ret = aal_xgpon_dsfp_rxmac_ctrl_get(device_id, &rxmac_cfg);
    if (CA_E_OK != ret) {
        ca_printf("Err: aal_xgpon_dsfp_rxmac_ctrl_get FAIL! \r\n");
    }

    rxmac_cfg_mask.s.omci_min_pkt_size = 1;
    rxmac_cfg.omci_min_pkt_size        = AAL_XGPON_MIN_OMCI_PACKET_LEN;
    ret = aal_xgpon_dsfp_rxmac_ctrl_set(device_id, rxmac_cfg_mask, &rxmac_cfg);
    return ret;
}

static ca_status_t __xgpon_common_init(ca_device_id_t device_id,ca_uint32_t pon_mac_mode)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_xgpn_glb_ctrl_config_t xgpn_glb_cfg;
    aal_xgpon_xgpn_glb_ctrl_config_msk_t xgpn_glb_cfg_msk;

    memset(&xgpn_glb_cfg, 0, sizeof(xgpn_glb_cfg));
    xgpn_glb_cfg_msk.u32 = 0;
    xgpn_glb_cfg_msk.s.mac_en  = 1;
    xgpn_glb_cfg_msk.s.pm_mode = 1;
    xgpn_glb_cfg_msk.s.us_line_rate  = 1;
    xgpn_glb_cfg.mac_en        = 0; /* disable MAC */
    xgpn_glb_cfg.pm_mode       = 2; /* read clear mode: Performance monitor */

    switch (pon_mac_mode) {/* Case different PON mode to set MAC mode and upstream line rate */
        case ONU_PON_MAC_XGPON1:
            xgpn_glb_cfg.us_line_rate = AAL_XGPON_US_LINE_RATE_2G;
            break;
        case ONU_PON_MAC_XGSPON:
            xgpn_glb_cfg.us_line_rate = AAL_XGPON_US_LINE_RATE_XG;
            break;
        case ONU_PON_MAC_NGPON2:
            xgpn_glb_cfg.us_line_rate = AAL_XGPON_US_LINE_RATE_BOTH_2G_XG;
            break;
        default:
            xgpn_glb_cfg.us_line_rate = AAL_XGPON_US_LINE_RATE_RSVD;
            break;
    }

    ret = aal_xgpon_xgpn_glb_ctrl_set(device_id,xgpn_glb_cfg_msk,&xgpn_glb_cfg);
    if (CA_E_OK != ret) {
        ca_printf("Err: aal_xgpon_xgpn_glb_ctrl_set FAIL! \r\n");
    }
    //ca_printf("aal_xgpon_xgpn_glb_ctrl_set done\n");

    return ret;

}


static ca_status_t __xgpon_ploam_init(ca_device_id_t device_id,ca_uint32_t pon_mac_mode)
{
/*
4.  Set US PLOAM config
"   PLMP_CTRL
"   PLMP_DS_CTRL
"   PLMP_US_CTRL (default value is good)
"   Fill SN: PLMP_VENDOR_ID_CFG/PLMP_VSS_CFG; //fill serial number
"   Fill REGISTRATION_ID: PLMP_RGST_ID_CFG0-8
"   XGPN_PLMP_CALIBRATION_EVEN/ODD = 0xffff;        //sufficient
"   XGPN_PLMP_TUNING_PARAMETER. granularity = 5
"   XGPN_PLMP_TUNING_PARAMETER. time = 0xC8; //class 2 - 25 msec
"   PLMP_ATTENUATION.level = 0; //un-attent
"   PLMP_ATTENUATION.capability = 1;    //3db
"   PLMP_RGST_ID0-8 - CLI to set?
"   PLMP_ACK_PLOAM_XXX - maybe not needed
"   PLMP_INTERRUPTE0/1 = 0xc00003ff; //enable all
"   PLMP_PKT_GEN_HEADER/DA_SA=>L2 wrapper for PLOAM/CPU TX/RX
"   PLMP ICK length: AES_PLMP_GBL_CTRL

6.  Setup US MAC:
"   US PLOAM: USBG_PLOAM_QU_CFG
10. Set PLOAM capture to CPU:
"   PLMP_DS_CTRL
o   onuid_flt_dis =0
o   capt_msg_type[7:0]=0
o   capt_msg_en=1
o   extension_en=1
o   unkwn_msg_type_en= 1.

*/

    ca_status_t ret = CA_E_OK;
    aal_xgpon_aes_plmp_gbl_ctrl_t aes_plmp_gbl_ctrl;
    aal_xgpon_aes_plmp_gbl_ctrl_msk_t aes_plmp_gbl_ctrl_mask;
    aal_xgpon_aes_plmp_subkey_ctrl_t  aes_plmp_subkey_ctrl;

    aal_xgpn_plmp_rgst_id_cfg_t rgst_id;
    aal_xgpn_plmp_ds_ctrl_mask_t  ds_ctrl_mask;
    aal_xgpn_plmp_ds_ctrl_t       ds_ctrl;
    aal_xgpn_plmp_tuning_parameter_mask_t  tuning_mask;
    aal_xgpn_plmp_tuning_parameter_t       tuning;
    aal_ploam_tuning_cfg_mask_t tuning_cfg_mask;
    aal_ploam_tuning_cfg_t      tuning_cfg;
    ca_uint32_t i;

    aal_ploam_init(device_id);
    aes_plmp_gbl_ctrl_mask.u32 = ~0U;
    memset(&aes_plmp_gbl_ctrl, 0, sizeof(aal_xgpon_aes_plmp_gbl_ctrl_t)); 
    memset(&aes_plmp_subkey_ctrl, 0, sizeof(aal_xgpon_aes_plmp_subkey_ctrl_t));
    
    memset(&rgst_id, 0, sizeof(aal_xgpn_plmp_rgst_id_cfg_t));

    aes_plmp_gbl_ctrl.key_hw_mode = CA_AAL_XGPON_KEY_HW_MODE;
    aes_plmp_gbl_ctrl.key_len = 16;
    aes_plmp_gbl_ctrl.msk_reg_id_src_sel = CA_AAL_XGPON_REGID_FROM_REGST_PLOAM;
    aes_plmp_gbl_ctrl.sma_mode = CA_AAL_XGPON_AES_SMA_NOT_EXISIT;
    ret = aal_xgpon_aes_plmp_glb_ctrl_set(device_id, aes_plmp_gbl_ctrl_mask, &aes_plmp_gbl_ctrl);
	CA_RET_VALUE_CHECK(ret);
    aal_xgpon_aes_key_hw_mode = aes_plmp_gbl_ctrl.key_hw_mode;

    if(aes_plmp_gbl_ctrl.key_hw_mode == CA_AAL_XGPON_KEY_SW_MODE){
        aes_plmp_subkey_ctrl.gene_st        = TRUE;
        aes_plmp_subkey_ctrl.ik_sw_st       = TRUE;
        aes_plmp_subkey_ctrl.mic_k12_sw_st  = TRUE;
        aes_plmp_subkey_ctrl.sk_sw_st       = TRUE;
        
    }else{
        aes_plmp_subkey_ctrl.gene_st        = FALSE;
        aes_plmp_subkey_ctrl.ik_sw_st       = FALSE;
        aes_plmp_subkey_ctrl.mic_k12_sw_st  = FALSE;
        aes_plmp_subkey_ctrl.sk_sw_st       = FALSE;
    }
    ret = aal_xgpon_aes_plmp_subkey_ctrl_set(device_id, &aes_plmp_subkey_ctrl);
    CA_RET_VALUE_CHECK(ret);
    
    for (i = 0; i < XGPN_PLMP_RGST_ID_SIZE; i++)
        rgst_id.data[i] = g_xgpn_basic_info.registration_id[i];

    ret = aal_ploam_vendor_id_cfg_set(device_id, g_xgpn_basic_info.vendor_id);
    CA_RET_VALUE_CHECK(ret);
    ret = aal_ploam_vssn_cfg_set(device_id, g_xgpn_basic_info.vssn);
    CA_RET_VALUE_CHECK(ret);
    ret = aal_ploam_rgst_id_cfg_set(device_id, &rgst_id);
    CA_RET_VALUE_CHECK(ret);

    /* XGPN_PLMP_DS_CTRL */
    ds_ctrl_mask.wrd = 0;
    memset(&ds_ctrl, 0, sizeof(ds_ctrl));
		
    ds_ctrl_mask.bf.capt_msg_en = TRUE;
	ds_ctrl_mask.bf.onuid_flt_dis = TRUE;
	ds_ctrl_mask.bf.extension_en = TRUE;
	ds_ctrl_mask.bf.unkwn_msg_type_en = TRUE;
    ds_ctrl.capt_msg_en = TRUE;
    ds_ctrl.onuid_flt_dis = FALSE;  // ONU only receives broadcast or directed(ONU-ID matches assigned one) PLOAMd message
    ds_ctrl.extension_en = TRUE;
	ds_ctrl.unkwn_msg_type_en = TRUE;    
#if 0   //the following bits not work further according to ASIC, 10/13/2016
    ds_ctrl_mask.bf.capt_msg_type = TRUE;
    ds_ctrl.capt_msg_type = FALSE;  // when capture_msg_type=0, capture all known message type
    ds_ctrl_mask.bf.extension_en = 1;
    ds_ctrl.extension_en = 0;
    ds_ctrl_mask.bf.unkwn_msg_type_en = 1;
    ds_ctrl.unkwn_msg_type_en = 0;
#endif

    ret = aal_ploam_ds_ctrl_set(0, ds_ctrl_mask, &ds_ctrl);
    PLOAM_DEBUG("aal_ploam_ds_ctrl_set, ret:0x%x\r\n", ret);

    /* XGPN_PLMP_CTRL */
    AAL_PON_REG_WRITE(0x7e, XGPN_PLMP_CTRL);
    #if 0
    memset(&plmp_ctrl, 0, sizeof(plmp_ctrl));

    plmp_ctrl_mask.wrd =0;
    plmp_ctrl_mask.bf.resrv_bc_en = TRUE;
    plmp_ctrl.resrv_bc_en = TRUE; // ONU receives broadcast PLOAMd including ONU-ID 0x3FD, 0x3FE, and 0x3FF
    ret = aal_ploam_ctrl_set(0, plmp_ctrl_mask, &plmp_ctrl);
    PLOAM_DEBUG("aal_ploam_ctrl_set, ret:0x%x\r\n", ret);
    #endif
    /* XGPN_PLMP_RX_CAPTURE */
    AAL_PON_REG_WRITE(0xFFFFFFFF, XGPN_PLMP_RX_CAPTURE);    // capture all PLOAM types

    AAL_PON_REG_WRITE(0xFFFFFF, XGPN_PLMP_EQD_THRSHLD); // set the EQD threshould to support MAX distance 500KM

    if(pon_mac_mode == ONU_PON_MAC_NGPON2)
    {
#if 0    // this should not be set here, keep HW default
            /* set XGPN_PLMP_CALIBRATION_EVEN/ODD to 0xffff */
            ret = aal_ploam_calibration_even_set(0, __NGPON2_EVEN_CALIBRATION_DEFAULT);
            CA_RET_VALUE_CHECK(ret);
        
            ret = aal_ploam_calibration_odd_set(0, __NGPON2_ODD_CALIBRATION_DEFAULT);
            CA_RET_VALUE_CHECK(ret);
#endif

        tuning_mask.wrd            = 0;
        tuning_mask.bf.granularity = 1;
        tuning_mask.bf.time        = 1;

        tuning.granularity = AAL_XGPON_TUNING_PARAM_GRANULARITY_DEFAULT;
        tuning.time        = AAL_XGPON_TUNING_PARAM_TIME_DEFAULT;

        ret = aal_ploam_tuning_parameter_set(0, tuning_mask, &tuning);
        CA_RET_VALUE_CHECK(ret);

        tuning_cfg_mask.wrd = 0;
        tuning_cfg_mask.s.request_lead_sfc = 1;
        tuning_cfg.request_lead_sfc = AAL_XGPON_TUNING_LEAD_SFC_DEFAULT;
        ret = aal_ploam_tuning_cfg_set(device_id, tuning_cfg_mask, &tuning_cfg);
        CA_RET_VALUE_CHECK(ret);

    }
    return ret;
}

static ca_status_t __xgpon_actp_init(ca_device_id_t device_id,ca_uint32_t pon_mac_mode)
{
/*
7.  Activation state machine:
"   XGPN_ACTP_TOZ, TO1-5, XGPN_ACTP_CTRL, XGPN_ACTPSYNC_HUNT

*/
    ca_status_t ret = CA_E_OK;

    ret = aal_xgpon_actp_init(device_id);

    // TODO: add actp_fsm_init here
    return ret;
}
static ca_status_t __xgpon_usbg_init(ca_device_id_t device_id,ca_uint32_t pon_mac_mode)
{
/*
12. Setup upstream flow=>GEM mapping
"   XGPN_USBG_XGEM_PID_TBL_ACCESS/XGPN_USBG_XGEM_PID_TBL_DATA
"   XGPN_USBG_XGEM_PID_TBL_DATA. port_en/ aes_mode

*/
    ca_status_t ret = CA_E_OK;
    aal_xgpon_usbg_glb_ctrl_config_msk_t usbg_glb_ctrl_config_msk;
    aal_xgpon_usbg_glb_ctrl_config_t usbg_glb_ctrl_config;
    aal_xgpon_usbg_ploamu_qu_config_msk_t usbg_plmu_qu_cfg_msk;
    aal_xgpon_usbg_ploamu_qu_config_t usbg_plmu_qu_cfg;

    usbg_glb_ctrl_config_msk.u32 = 0;
    usbg_glb_ctrl_config_msk.s.laser_on_align = 1;
    usbg_glb_ctrl_config_msk.s.pf_align_mode = 1;

    memset(&usbg_glb_ctrl_config, 0, sizeof(usbg_glb_ctrl_config));

    usbg_glb_ctrl_config.laser_on_align = g_xgpn_basic_info.laser_on_align;
    usbg_glb_ctrl_config.pf_align_mode  = 1;

    ret = aal_xgpon_usbg_glb_ctrl_set(device_id, usbg_glb_ctrl_config_msk,
        &usbg_glb_ctrl_config);

    CA_RET_VALUE_CHECK(ret);

    usbg_plmu_qu_cfg_msk.u32 = 0;
    usbg_plmu_qu_cfg_msk.s.no_empty = 1;
    usbg_plmu_qu_cfg_msk.s.mode = 1;

    usbg_plmu_qu_cfg.no_empty = 0;
    usbg_plmu_qu_cfg.mode = 0; /* H/W mode */

    ret = aal_xgpon_usbg_ploamu_queue_cfg_set(device_id,usbg_plmu_qu_cfg_msk,&usbg_plmu_qu_cfg);
    CA_RET_VALUE_CHECK(ret);

    return ret;

}

ca_status_t __xgpon_datapath_init(ca_device_id_t device_id,ca_uint32_t pon_mac_mode)
{
/*
2.  Setup GEM=>UNI cross connect (if necessary: PDC_PDC_MAP_MEM_ACCESS/ PDC_PDC_MAP_MEM_DATA)
9.  Setup PUC upstream traffic mapping (see section 4)
*/
    ca_status_t ret = CA_E_OK;
    /* PDC Init */
    ret = aal_pdc_init(device_id,pon_mac_mode);
    CA_RET_VALUE_CHECK(ret);
    /* PUC Init */
    ret = aal_puc_init(device_id,pon_mac_mode);
    CA_RET_VALUE_CHECK(ret);

    return ret;
}

static ca_status_t __xgpon_intr_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    ret = aal_xgpon_intr_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("aal_xgpon_intr_init fail, return %d.\r\n", ret);
        return ret;
    }

    return ret;

}


/* @__xgpon_res_init
 * init software resource
 */
ca_status_t __xgpon_res_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    if (0 == g_aal_xgpon_lock)
        ret = ca_spin_lock_init(&g_aal_xgpon_lock, SPINLOCK_BH);

    aal_xgpn_mac_debug = 0;
    aal_xgpn_simulation_flag = 0;
    aal_xgpn_auto_reset = 1;

    return ret;
}

ca_status_t aal_xgpon_init(ca_device_id_t device_id,ca_uint32_t pon_mac_mode)
{
    ca_status_t ret = CA_E_OK;

    //ca_printf("ca-pon: %s(): start\n", __func__);

    ret = __xgpon_res_init(device_id);
    ca_printf("ca-pon: init XGPON global resource %s\n", ret ? "FAIL" : "OK");

#if defined(CONFIG_ARCH_CORTINA_VENUS) && !defined(CONFIG_CORTINA_BOARD_FPGA)
    ret = aal_psds_init(device_id, pon_mac_mode);
    ca_printf("ca-pon: init XGPON SERDES %s\n", ret ? "FAIL" : "OK");
#endif

    /* Before init, should disable PON MAC to prevent Rx any signal  */
    ret = __glb_gpon_ctrl_init(device_id,pon_mac_mode);
    ca_printf("ca-pon: init XGPON global control %s\n", ret ? "FAIL" : "OK");
    // TODO: merge __glb_gpon_ctrl_init  into __xgpon_common_init
    ret = __xgpon_common_init(device_id, pon_mac_mode);
    ca_printf("ca-pon: init XGPON common configuration %s\n", ret ? "FAIL" : "OK");

    ret = aal_xgpn_activation_fsm_init(device_id, pon_mac_mode);
    ca_printf("ca-pon: init XGPON activation FSM %s\n", ret ? "FAIL" : "OK");

    ret = xgpon_ploam_key_exchange_fsm_init(device_id);
    ca_printf("ca-pon: init XGPON Key exchange FSM %s\n", ret ? "FAIL" : "OK");

    ret = __xgpon_dsfp_init(device_id,pon_mac_mode);
    ca_printf("ca-pon: init XGPON DSFP  %s\n", ret ? "FAIL" : "OK");

    ret = __xgpon_ploam_init(device_id,pon_mac_mode);
    ca_printf("ca-pon: init XGPON PLOAM  %s\n", ret ? "FAIL" : "OK");

    ret = __xgpon_bwmp_init(device_id,pon_mac_mode);
    ca_printf("ca-pon: init XGPON BWMP  %s\n", ret ? "FAIL" : "OK");

    ret = __xgpon_actp_init(device_id,pon_mac_mode);
    ca_printf("ca-pon: init XGPON ACTP  %s\n", ret ? "FAIL" : "OK");

    ret = __xgpon_usbg_init(device_id,pon_mac_mode);
    ca_printf("ca-pon: init XGPON USBG  %s\n", ret ? "FAIL" : "OK");

    ret = __xgpon_datapath_init(device_id,pon_mac_mode);
    ca_printf("ca-pon: init XGPON DP %s\n", ret ? "FAIL" : "OK");

    ret = __xgpon_intr_init(device_id);
    ca_printf("ca-pon: init XGPON interrupt %s\n", ret ? "FAIL" : "OK");

    //ca_printf("ca-pon: %s(): end\n", __func__);

    return ret;
}

ca_boolean_t aal_xgpon_clk_is_locked_ok(ca_device_id_t device_id)
{
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)||defined(CONFIG_ARCH_CORTINA_G3HGU)
    aal_psds_status_t  psds_status;

    memset(&psds_status, 0, sizeof(aal_psds_status_t));
    (void)aal_psds_status_get(device_id, &psds_status);
    if (!psds_status.rx_los && psds_status.ckrdy_rx && psds_status.ckrdy_tx && psds_status.ber_notify)
        return TRUE;
    else
        return FALSE;
#endif

#endif
    return TRUE;

}

ca_status_t aal_xgpon_clk_seq_reset(ca_device_id_t device_id)
{
    return CA_E_OK;
}

ca_status_t aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_uint32_t  timeout_ms /* 0 means wait for dsync forever*/
    )
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t dsync_sts = 0;
    ca_boolean_t clk_locked = 0;
    aal_xgpon_actp_status_t   actp_sts;

    memset(&actp_sts, 0 , sizeof(aal_xgpon_actp_status_t));

    clk_locked = aal_xgpon_clk_is_locked_ok(device_id);

    ret = aal_xgpon_dsfp_sync_state_get(device_id, &dsync_sts);
    CA_RET_VALUE_CHECK(ret);
    
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)||defined(CONFIG_ARCH_CORTINA_G3HGU)
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
    (void)aal_psds_reset(device_id, aal_pon_mac_mode_get(device_id));
    ca_mdelay(1);
#endif
#endif

    if (ONU_PON_MAC_NGPON2 == aal_pon_mac_mode_get(device_id)) {
        aal_clear_actp_rx_tx_optical_tuning(TRUE,TRUE);
    }

    ret = aal_xgpon_actp_status_get(0,&actp_sts);
    CA_RET_VALUE_CHECK(ret);

    if ((actp_sts.onu_state == AAL_XGPON_ACTP_STATE_O1 || actp_sts.onu_state == AAL_XGPON_ACTP_STATE_O8)
        && actp_sts.onu_sub_state == AAL_XGPON_ACTP_SUB_STATE_OX2) {// O1.2, O8.2
        aal_xgpon_actp_profile_learning_set(device_id, 1);
    }

    AAL_XGPON_DBG("%s(): done.\r\n", __FUNCTION__);

    return ret;
}

extern void aal_xgpn_act_fsm_state_reset(ca_device_id_t device_id);
extern ca_status_t ca_gpon_cb_init (CA_IN ca_device_id_t device_id);
extern  ca_status_t ca_gpon_cb_reset(CA_IN ca_device_id_t device_id);
ca_status_t aal_xgpon_reset(ca_device_id_t device_id, ca_uint32_t reset_flag)
{
    XGPN_XGPN_GLB_CTRL_t  xgpn_glb_save;
    ca_uint32_t           sds_tx_enable = 0;
    GLOBAL_GPON_CNTL_t   glb_gpon_cntl;
    GLOBAL_PON_CNTL_t    glb_pon_cntl;
    ca_uint32_t   optic_status = 0;
    ca_uint32_t   los_status = 0;
    ca_uint32_t   temp = 0;
    ca_uint32_t   pon_mac_mode = aal_pon_mac_mode_get(device_id);
    ca_uint32_t   sniffer_stats = 0;
    /*
    1. DIS TX
    2. DIS INT
    3. RESET MAC
    4. CLKGEN_PSDS_INIT_CNTL.POW_PCIX=0
    5. RESET SDS SEQ
    6. PSDS_RG95[REG0_SEL_SD]=2'11  /400ns
    */
    if (!reset_flag)
        return CA_E_OK;

    if (!aal_xgpn_auto_reset)
        return CA_E_OK;

    //AAL_GLB_REG_FIELD_WRITE(GLOBAL_OPT_MODULE_CNTL, opt_tx_power_down, 1);
    AAL_GLB_REG_FIELD_READ(GLOBAL_PSDS_INIT_CNTL, ben_oen, sds_tx_enable);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PSDS_INIT_CNTL, ben_oen, 0);


    if (aal_xgpn_mac_debug) {
        optic_status = AAL_GLB_REG_READ(GLOBAL_OPT_MODULE_STATUS);
        los_status   = AAL_PON_REG_READ(PSDS_RGB8);
    }

    if (reset_flag & AAL_XGPON_RESET_FLAG_MAC) { /* PON MAC reset logic */
          
        AAL_PON_REG_WRITE (device_id, XGPN_XGPN_GLB_INTERRUPT0E); /* disable PON interrupts */
        for (temp = 0; temp < AAL_XGPON_INTR_NUM; ++temp) {
            aal_xgpon_intr_clr(device_id, temp, ~0);
            aal_xgpon_intr_disable(device_id, temp);
        }

#ifdef CONFIG_LUNA_G3_SERIES
        aal_xgpon_backup_ponmac(device_id);
#endif

        xgpn_glb_save.wrd = AAL_PON_REG_READ(XGPN_XGPN_GLB_CTRL); /* store old XGPON MAC global config */
        
        AAL_PON_REG_FIELD_WRITE(XGPN_XGPN_GLB_CTRL, mac_en, 0);
        ca_gpon_cb_reset(device_id); /* reset GPON API, prevent API invoking */
        aal_xgpn_act_fsm_state_reset(device_id);
        ca_xgpon_aes_sm_state_set(device_id, CA_XGPON_ONU_KN0, CA_XGPON_ONU_KN0);
        //__wait_puc_upstream_empty(device_id);

        /*delay 10s for reset glb pon ctrl register*/
        
        //ca_printf("ca-pon: begin to reset PON MAC!!\r\n");
        //ca_mdelay(7000);
        
        /* Set PON MAC to be reset state */
        glb_gpon_cntl.wrd = AAL_GLB_REG_READ(GLOBAL_GPON_CNTL);
        glb_gpon_cntl.bf.xgpn_rst_n = 0;
        glb_gpon_cntl.bf.gpon_rst_n = 0;
        glb_gpon_cntl.bf.ani_rst_n = 0;
        glb_pon_cntl.wrd  = AAL_GLB_REG_READ(GLOBAL_PON_CNTL);
        glb_pon_cntl.bf.puc_reset = 0;
        glb_pon_cntl.bf.pdc_reset = 0;
        AAL_GLB_REG_WRITE(glb_gpon_cntl.wrd, GLOBAL_GPON_CNTL);
        AAL_GLB_REG_WRITE(glb_pon_cntl.wrd, GLOBAL_PON_CNTL);
        ca_mdelay(1);
        ca_printf("ca-pon: reset PON MAC OK\r\n");
        
        /*read sniffer stats after pon mac reset and make sure it is 0*/
        sniffer_stats = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS);
        ca_printf("ca-pon: sniffer_stats is 0x%x after PON MAC reset\r\n",sniffer_stats);
    }

    if (aal_xgpn_mac_debug) {
        ca_printf("DEBUG INFO: GLOBAL_OPT_MODULE_STATUS = 0x%x, RX_LOS = %u\r\n", optic_status, los_status & 0x01);
    }

    if (reset_flag & AAL_XGPON_RESET_FLAG_SDS) { /* PON SERDES reset logic */
        //ca_printf("ca-pon: PON SERDES re-init start\r\n");
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
        /* Reset PON SERDES */
        aal_psds_reset(device_id, pon_mac_mode);
        //ca_mdelay(100); /* wait for SERDES clock stable */
#endif
    }

    if (reset_flag & AAL_XGPON_RESET_FLAG_MAC) { /* release PON MAC */

        /* Set PON MAC to be release state */
        glb_pon_cntl.bf.puc_reset = 1;
        glb_pon_cntl.bf.pdc_reset = 1;
        glb_gpon_cntl.bf.xgpn_rst_n = 1;
        glb_gpon_cntl.bf.ani_rst_n = 1;
        AAL_GLB_REG_WRITE(glb_gpon_cntl.wrd, GLOBAL_GPON_CNTL);
        AAL_GLB_REG_WRITE(glb_pon_cntl.wrd, GLOBAL_PON_CNTL);
        ca_mdelay(1);

        //ca_printf("ca-pon: PON MAC re-init start\r\n");
        /* Re-initialize PON MAC modules */
        __xgpon_dsfp_init(device_id,pon_mac_mode);
        __xgpon_actp_init(device_id,pon_mac_mode);
        __xgpon_ploam_init(device_id,pon_mac_mode);
        __xgpon_bwmp_init(device_id,pon_mac_mode);
        __xgpon_usbg_init(device_id,pon_mac_mode);
        __xgpon_datapath_init(device_id, pon_mac_mode);
        //aal_xgpn_act_fsm_state_reset(device_id);
        //ca_xgpon_aes_sm_state_set(device_id, CA_XGPON_ONU_KN0, CA_XGPON_ONU_KN0);
        __xgpon_intr_init(device_id);
        ca_gpon_cb_init(device_id);

#if defined(CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN)
        AAL_PON_REG_WRITE(0x30A, TITANGLB_FPGA_IDP_CNTL);
        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_CTRL, xge_voqbp_drop_en, 1); /* XGE voq backpressure drop enable */
#endif

#if defined(CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN) 
        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_DOWN_INTE, localcpu_rx_pkt_rdyE, 1);
#endif
        
        /*enable sniffer hw write*/
        AAL_PON_REG_WRITE(0x1007F, XGPN_XGPN_GLB_SNIFFER_CFG);

       // ca_udelay(500);
        /* Re-enable PON MAC */
        AAL_PON_REG_WRITE(xgpn_glb_save.wrd, XGPN_XGPN_GLB_CTRL); // recover PON MAC mode
        //ca_udelay(500);
        ca_printf("ca-pon: recover PON MAC OK\r\n");

#ifdef CONFIG_LUNA_G3_SERIES
        aal_xgpon_restore_ponmac(device_id);
#endif
    }

    if (pon_mac_mode == ONU_PON_MAC_NGPON2) {
        //ca_mdelay(1);
        //aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(device_id, AAL_CLK_SEQ_RESET_TRY_COUNT);
        aal_xgpon_actp_profile_learning_set(device_id, 1);
        aal_clear_actp_rx_tx_optical_tuning(TRUE, TRUE);
    }

    /* Re-enable SERDES Tx */
    //AAL_GLB_REG_FIELD_WRITE(CLKGEN_PSDS_INIT_CNTL, ben_oen, 1);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PSDS_INIT_CNTL, ben_oen, sds_tx_enable);
    //ca_udelay(500);

    //AAL_GLB_REG_FIELD_WRITE(GLOBAL_OPT_MODULE_CNTL, opt_tx_power_down, 0);
    if (reset_flag & AAL_XGPON_RESET_FLAG_MAC) {
        ca_printf("ca-pon: re-init PON MAC OK\r\n");
    }
    return CA_E_OK;
}

/*
 * @aal_xgpon_datapath_reset:
 *  reset datapath when IDP,PUC,USBG has hang problem
 */
void aal_xgpon_datapath_reset(ca_device_id_t device_id)
{
    XGPN_USBG_INTERRUPT_t usbg_intr;

    AAL_XGPON_DBG("Reset Datapath modules: PUC, IDP...\r\n");
    /* Flush VOQ and User XGEM ports */
    (void)aal_puc_voq_flush();
    (void)__aal_xgpon_gem_enable(AAL_DEFAUL_DEV_ID, 1, AAL_XGPON_MAX_GEM_PORT_IDX, FALSE);

    /* reset PUC and init, all TCONT/GEM cfg should be restored from OMCI */
    aal_puc_init(AAL_DEFAUL_DEV_ID, aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID));

    usbg_intr.wrd = AAL_PON_REG_READ(XGPN_USBG_INTERRUPT);
    if (usbg_intr.bf.xgem_port_disable_intI) {
        ca_printf("detect xgem_port_disable_intI, need reset IDP.\r\n");
        AAL_PON_REG_FIELD_WRITE(XGPN_USBG_INTERRUPT, xgem_port_disable_intI, 1);

    }

    /* reset clock sequence for FPGA serdes out of clock issue */
    /* neednt reset clk seq here, hw timer callback will do it */
    //aal_xgpon_clk_seq_reset_and_clear_rxtx_tuning(AAL_DEFAUL_DEV_ID, AAL_CLK_SEQ_RESET_TRY_COUNT);

    AAL_XGPON_DBG("Reset Datapath modules done.\r\n");

}

#define __UPS_MIB_PRINT(blk,cnt,str)    do { \
        if (sizeof(cnt) == 4) \
            ca_printf("\t %-4s : %u  %s\r\n", #blk, cnt, str[0] ? str : "(no error)"); \
        else \
            ca_printf("\t %-4s : %llu  %s\r\n", #blk, cnt, str[0] ? str : "(no error)"); \
        ca_printf("\t  ^\r\n"); \
        ca_printf("\t  |\r\n"); \
    }while(0)

#define __DNS_MIB_PRINT(blk,cnt,str)    do { \
        if (sizeof(cnt) == 4) \
            ca_printf("\t %-4s : %u  %s\r\n", #blk, cnt, str[0] ? str : "(no error)"); \
        else \
            ca_printf("\t %-4s : %llu  %s\r\n", #blk, cnt, str[0] ? str : "(no error)"); \
        ca_printf("\t  |\r\n"); \
        ca_printf("\t  v\r\n"); \
    }while(0)

#define __CPU_MIB_PROMPT()   \
        ca_printf("\t%-8s %-11s %-8s  \r\n", "PON-Rx", " ", "PON-Tx")

#define __CPU_MIB_PRINT(blk_dn, blk_up, dn_cnt, up_cnt)    do { \
        ca_printf("\t %-8s %-11s %-8s  \r\n", "|", " ", "^"); \
        ca_printf("\t %-8s %-11s %-8s  \r\n", "v", " ", "|"); \
        ca_printf("\t %-8u %-5s:%-5s %-8u  \r\n", dn_cnt, #blk_dn, #blk_up, up_cnt); \
    }while(0)


#define  __MIB_ERR_DETECT(reg,field,str)  do { \
        reg##_t reg_data;\
        reg_data.wrd = AAL_PON_REG_READ(reg); \
        str[127] = reg_data.bf.field;\
        if (str[127] && strlen(str) < __ERR_STR_LEN) { \
            strncat(str, #field, __ERR_STR_LEN-strlen(str)); \
            strncat(str, ",", __ERR_STR_LEN-strlen(str)); \
        } \
        str[127] = 0; \
    }while(0)

#define  __CNT_ERR_DETECT(reg,str)  do { \
        reg##_t reg_data;\
        reg_data.wrd = AAL_PON_REG_READ(reg); \
        if (reg_data.wrd && strlen(str) < __ERR_STR_LEN) { \
            strncat(str, #reg, __ERR_STR_LEN-strlen(str)); \
            strncat(str, ",", __ERR_STR_LEN-strlen(str)); \
        } \
        str[127] = 0; \
    }while(0)


#define __ERR_STR_LEN  200
ca_status_t aal_xgpon_ups_mib_get(
    CA_IN  ca_device_id_t  device_id)
{
/*******************************
   USBG : 12121212 (err:)
    ^
    |
   PUC : 121223 (err:)
    ^
    |
   IDP : 132323 (err:)
    ^
    |
   XGE : 12132323 (err:)
    ^
    |
***********************************/
    aal_xgpon_usbg_xgem_mib_config_t gem_mib;
    ca_uint32_t  gem = 0;
    ca_uint32_t  count32 = 0;
    ca_uint32_t  temp    = 0;
    char         err[__ERR_STR_LEN+1] = {0};


    ca_printf("\r\n");
    /* USBG */
    memset(err, 0, sizeof(err));
    count32 = 0;
    __MIB_ERR_DETECT(XGPN_USBG_INTERRUPT, xgem_port_disable_intI, err);
    __MIB_ERR_DETECT(XGPN_USBG_INTERRUPT, xgem_ff_uf_intI, err);
    __MIB_ERR_DETECT(XGPN_USBG_INTERRUPT, xgem_ff_of_intI, err);
    __MIB_ERR_DETECT(XGPN_USBG_INTERRUPT, st_chk_err_intI, err);
    __MIB_ERR_DETECT(XGPN_USBG_INTERRUPT, buf_empty_pop_intI, err);
    AAL_FOR_ALL_USR_XGEM_DO(gem) {
        memset(&gem_mib, 0, sizeof(gem_mib));
        aal_xgpon_usbg_xgem_mib_get(device_id, gem, &gem_mib);
        count32 += gem_mib.frame_cnt;
    }
    __UPS_MIB_PRINT(USBG, count32, err);

    /* PUC */
    memset(err, 0, sizeof(err));
    count32 = AAL_PON_REG_READ(PUC_BMC_RX_PKT);
    temp = AAL_PON_REG_READ(PUC_BMC_CONTROL_PKT_CNTR_mac);
    if (count32 >= temp)
        count32 -= temp;
    temp = AAL_PON_REG_READ(PUC_BMC_CONTROL_PKT_CNTR_lnk);
    if (count32 >= temp)
        count32 -= temp;
    temp = AAL_PON_REG_READ(PUC_BMC_LENGTH_ERROR);
    if (temp > 0) {
        strncat(err, "rx_lenth_err,", __ERR_STR_LEN-strlen(err));
    }

    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, gnttoosmall_intI, err     ); /* bits 2:2 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, opm_correctedeccerr_intI, err ); /* bits 3:3 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, lfram_correctedeccerr_intI, err ); /* bits 4:4 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, tqm_correctedeccerr_intI, err ); /* bits 5:5 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bcm_correctedeccerr_intI, err ); /* bits 6:6 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, pllm_correctedeccerr_intI, err ); /* bits 7:7 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bpmem_correctedeccerr_intI, err ); /* bits 8:8 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bmc_pe_fifo_overflow_intI, err ); /* bits 16:16 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bmc_data_fifo_overflow_intI, err ); /* bits 17:17 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bmc_front_fifo_overflow_intI, err ); /* bits 18:18 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bmc_rx_err_pkt_intI, err  ); /* bits 19:19 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bmc_length_err_pkt_intI, err ); /* bits 20:20 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, opm_eccerr_intI, err      ); /* bits 26:26 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, lfram_eccerr_intI, err    ); /* bits 27:27 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, tqm_eccerr_intI, err     ); /* bits 28:28 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bcm_eccerr_intI, err      ); /* bits 29:29 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, pllm_eccerr_intI, err     ); /* bits 30:30 */
    __MIB_ERR_DETECT(PUC_PONCNTL_INTERRUPT, bpmem_eccerr_intI, err    ); /* bits 31:31 */
    __CNT_ERR_DETECT(PUC_BMC_LENGTH_ERROR, err);
    __CNT_ERR_DETECT(PUC_BMC_FORCE_DROP, err);
    __UPS_MIB_PRINT(PUC, count32, err);


    return CA_E_OK;
}

ca_status_t aal_xgpon_dns_mib_get(
    CA_IN  ca_device_id_t device_id)
{
/*******************************
   DSFP : 12121212 (err:)
    |
    V
   PDC : 121223 (err:)
    |
    V
   IDP : 132323 (err:)
    |
    V
   XGE : 12132323 (err:)
    |
    V
***********************************/
    aal_xgpon_dsfp_xgem_mib_config_t gem_mib;
    ca_uint32_t  gem = 0;
    ca_uint32_t  count32 = 0;
    ca_uint32_t  temp = 0;
    char         err[__ERR_STR_LEN+1] = {0};

    ca_printf("\r\n");

    /* DSFP */
    memset(err, 0, sizeof(err));
    count32 = 0;
    __MIB_ERR_DETECT(XGPN_DSFP_INTERRUPT, xgem_fifo_overflowI, err ); /* bits 3:3 */
    __MIB_ERR_DETECT(XGPN_DSFP_INTERRUPT, xgem_portid_mismatchI, err); /* bits 7:7 */
    __MIB_ERR_DETECT(XGPN_DSFP_INTERRUPT, pkt_length_mismatchI, err); /* bits 10:10 */
    __MIB_ERR_DETECT(XGPN_DSFP_INTERRUPT, bip_32_errSat, err       ); /* bits 16:16 */
    __MIB_ERR_DETECT(XGPN_DSFP_INTERRUPT, psbd_hec_errSat, err     ); /* bits 18:18 */
    __MIB_ERR_DETECT(XGPN_DSFP_INTERRUPT, psbd_hec_uncorrected_mibSat, err); /* bits 19:19 */
    __MIB_ERR_DETECT(XGPN_DSFP_INTERRUPT, xgtc_hec_uncorrected_mibSat, err); /* bits 21:21 */
    __MIB_ERR_DETECT(XGPN_DSFP_INTERRUPT, bwmap_hec_uncorrected_mibSat, err); /* bits 23:23 */
    __CNT_ERR_DETECT(XGPN_DSFP_bip_32_err, err);
    __CNT_ERR_DETECT(XGPN_DSFP_xgtc_hec_uncorrected_mib, err);
    __CNT_ERR_DETECT(XGPN_DSFP_xgem_err_key_mib, err);
    __CNT_ERR_DETECT(XGPN_DSFP_rxmac_pkt_crc_err, err);

    AAL_FOR_ALL_USR_XGEM_DO(gem) {
        memset(&gem_mib, 0, sizeof(gem_mib));
        aal_xgpon_dsfp_xgem_mib_get(device_id, gem, &gem_mib);
        count32 += gem_mib.frame_cnt;
    }
    __DNS_MIB_PRINT(DSFP, count32, err);

    /* PDC */
    memset(err, 0, sizeof(err));
    count32 = AAL_PON_REG_READ(PDC_PKT_CNTR);
    temp = AAL_PON_REG_READ(PDC_CONTROL_PKT_CNTR_mac);
    if (temp > 0)
        count32 -= temp;
    temp = AAL_PON_REG_READ(PDC_CONTROL_PKT_CNTR_lnk);
    if (temp > 0)
        count32 -= temp;
    temp = AAL_PON_REG_READ(PDC_ERR_PKT_CNTR);
    if (temp > 0) {
        strncat(err, "rx_eop_error,", __ERR_STR_LEN-strlen(err));
    }

    __MIB_ERR_DETECT(PDC_FIFO_STATUS, map_fifo_over_flow, err);
    __MIB_ERR_DETECT(PDC_FIFO_STATUS, over_flow, err);
    __DNS_MIB_PRINT(PDC, count32, err);


    return CA_E_OK;
}


ca_status_t aal_xgpon_omci_mib_get(
    CA_IN  ca_device_id_t device_id)
{
/*************************************
  PON-Rx                        PON-Tx
    |                             ^
    v                             |
   123     DSFP/USBG            123
    |                             ^
    v                             |
   123      PDC/PUC             123
    |                             ^
    v                             |
   123    IDP_DN/IDP_UP          123

*****************************************/
    aal_xgpon_usbg_xgem_mib_config_t gem_mib;
    aal_xgpon_dsfp_xgem_mib_config_t gem_mib_dn;
    ca_uint32_t  count_up[3] = {0};
    ca_uint32_t  count_dn[3] = {0};
    ca_uint32_t  gem = 0;

    ca_printf("\r\n");

    /* DSFP/USBG */
    count_dn[0] = 0;
    AAL_FOR_ALL_OMCI_XGEM_DO(gem) {
        memset(&gem_mib_dn, 0, sizeof(gem_mib_dn));
        aal_xgpon_dsfp_xgem_mib_get(device_id, gem, &gem_mib_dn);
        count_dn[0] += gem_mib_dn.frame_cnt;
    }

    count_up[0] = 0;
    AAL_FOR_ALL_OMCI_XGEM_DO(gem) {
        memset(&gem_mib, 0, sizeof(gem_mib));
        aal_xgpon_usbg_xgem_mib_get(device_id, gem, &gem_mib);
        count_up[0] += gem_mib.frame_cnt;
    }

    /* PDC/PUC */
    count_dn[1] = AAL_PON_REG_READ(PDC_CONTROL_PKT_CNTR_lnk);
    count_up[1] = AAL_PON_REG_READ(PUC_BMC_CONTROL_PKT_CNTR_lnk);

    __CPU_MIB_PROMPT();
    __CPU_MIB_PRINT(DSFP, USBG, count_dn[0], count_up[0]);
    __CPU_MIB_PRINT(PDC, PUC, count_dn[1], count_up[1]);

    return CA_E_OK;
}

extern ca_pon_tx_cb ca_pon_tx_handle;
ca_uint8 dying_gasp_pkt_gpon[512];
ca_uint8 aal_gpon_dg_pdu[] = {
0x00, 0x00, 0x10, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00};

static int gasp_pdu_gpon = 0;

void aal_gpon_dying_gasp_handler(ca_int32 gpio_pin, ca_uint8 status)
{
    ca_pkt_t *pkt_p = (ca_pkt_t *)dying_gasp_pkt_gpon;
    ca_status_t   ret = CA_E_OK;

    if (!gasp_pdu_gpon) return;

    ret = ca_pon_tx_handle(0, pkt_p);
    if(CA_E_OK != ret){
        ca_printf("dying gasp tx failed:ret:%d\n", ret);
    }

    printk("dgg\r\n");
    return;
}
ca_status_t aal_gpon_dying_gasp_get
(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t       port_id,
    CA_OUT ca_uint8          *pkt_buff,
    CA_OUT ca_int32          *pkt_len
)
{
    aal_xgpon_usbg_xgtc_hd_ind_config_msk_t mask;
    aal_xgpon_usbg_xgtc_hd_ind_config_t hd_ind_cfg;

    ca_printf("<%s, %d>\r\n", __FUNCTION__, __LINE__);

    aal_gpon_dying_gasp_set(0, 0, aal_gpon_dg_pdu, sizeof(aal_gpon_dg_pdu));

    ca_buffer_dump("gasp111", dying_gasp_pkt_gpon, 192);

    mask.u32 = 0;
    mask.s.dg = 1;
    hd_ind_cfg.dg = 1;
    aal_xgpon_usbg_xgtc_hd_ind_cfg_set(0, mask, &hd_ind_cfg);

    aal_gpon_dying_gasp_handler(0, 0);

    return CA_E_OK;
}

    
ca_status_t aal_gpon_dying_gasp_set
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint8          *pkt_buff,
    CA_IN ca_int32          pkt_len
)
{
    ca_pkt_t *pkt = (ca_pkt_t *)dying_gasp_pkt_gpon;
    ca_pkt_block_t  *pkt_data;
    ca_int32_t pkt_hdr_size, pkt_blk_hdr_size;
    
    if(pkt_buff == NULL || pkt_len < 48)
        return CA_E_ERROR;

    pkt_hdr_size = sizeof(ca_pkt_t);
    pkt_blk_hdr_size = sizeof(ca_pkt_block_t);

    if(pkt_len + pkt_hdr_size + pkt_blk_hdr_size > sizeof(dying_gasp_pkt_gpon))
        return CA_E_PARAM;

    pkt_data = (ca_pkt_block_t *)(dying_gasp_pkt_gpon + pkt_hdr_size);

    pkt_data->data = dying_gasp_pkt_gpon + pkt_hdr_size + pkt_blk_hdr_size;
    pkt_data->len = pkt_len;
    /*
    ca_printf("<%s, %d>dying_gasp_pkt_gpon:%p, pkt:%p, pkt_data:%p, pkt_data->data:%p, size:%d, %d, len:%d\r\n", __FUNCTION__, __LINE__,
        dying_gasp_pkt_gpon, pkt, pkt_data, pkt_data->data, pkt_hdr_size, pkt_blk_hdr_size, pkt_len);
    */
    /* calc mic
    ca_generate_omci_mic(CA_IN ca_device_id_t device_id,
                         CA_IN ca_uint8_t  dir, //1 down, 2 up
                         CA_IN ca_uint8_t  *omci_content,
                         CA_IN ca_uint8_t  content_len,
                         CA_IN ca_uint8_t  mic_size,
                         CA_OUT ca_uint8_t *mic)
     */
    ca_generate_omci_mic(0, 2/* up stream */,
                         pkt_buff, 44, 4, pkt_buff + 44);

    memcpy(pkt_data->data, pkt_buff, pkt_len);

    //ca_buffer_dump("omci_dg", pkt_data->data, pkt_len);

    pkt->device_id = 0;
    pkt->flags = (0x1<<6);//CA_TX_BYPASS_FWD_ENGINE;
    pkt->pkt_data = pkt_data;
    pkt->block_count = 1;
    pkt->src_port_id = CA_PORT_ID(CA_PORT_TYPE_CPU, 0x10); // CPU0
    pkt->dst_port_id = 7; //CA_PORT_ID_NI7
    pkt->dst_sub_port_id = 0;   /* llid */
    pkt->pkt_len = pkt_data->len;
    pkt->cos = 8;
    pkt->pkt_type = CA_PKT_TYPE_OMCI;

    gasp_pdu_gpon = 1;
    //ca_printf("gpon dying gasp packet ready\r\n");
#ifdef CONFIG_LUNA_G3_SERIES
    AAL_PERI_REG_WRITE(0x105,PER_DYING_GASP);
    //AAL_PERI_REG_WRITE(1,PER_DG_IE_0); dg_int
#endif
    return CA_E_OK;
}

ca_status_t aal_xgpon_idle_xgem_frm_pli_get
(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t       port_id,
    CA_OUT aal_xgpon_usbg_idle_frm_cfg_t *pli
)
{
    XGPN_USBG_IDLE_FM_CFG_t pli_cfg;
    
    AAL_XGPON_LOCK();
    pli_cfg.wrd = AAL_PON_REG_READ(XGPN_USBG_IDLE_FM_CFG);
    pli->mode = pli_cfg.bf.mode;
    pli->size = pli_cfg.bf.size;
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_idle_xgem_frm_pli_set
(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN aal_xgpon_usbg_idle_frm_cfg_msk_t pli_msk,
    CA_IN aal_xgpon_usbg_idle_frm_cfg_t *pli
)
{
    XGPN_USBG_IDLE_FM_CFG_t pli_cfg;
    if (!pli_msk.u32) {
        return CA_E_OK;
    }
    AAL_XGPON_LOCK();
    pli_cfg.wrd = AAL_PON_REG_READ(XGPN_USBG_IDLE_FM_CFG);
    if(pli_msk.s.mode){
        pli_cfg.bf.mode = pli->mode;
    }
    if(pli_msk.s.size){
        pli_cfg.bf.size = pli->size;
    }
    AAL_PON_REG_WRITE (pli_cfg.wrd, XGPN_USBG_IDLE_FM_CFG);

    AAL_XGPON_UNLOCK();

    return CA_E_OK;

}

ca_status_t aal_xgpon_idle_xgem_frm_pli_stats_get
(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t       port_id,
    CA_OUT aal_xgpon_usbg_idle_frm_stats_t *stats
)
{
    XGPN_USBG_IDLE_XGEM_FRAME_CNT_t frm_cnt;
    XGPN_USBG_IDLE_XGEM_BYTE_CNT1_t byte_cnt1;
    XGPN_USBG_IDLE_XGEM_BYTE_CNT0_t byte_cnt0;
    AAL_XGPON_LOCK();
    frm_cnt.wrd = AAL_PON_REG_READ(XGPN_USBG_IDLE_XGEM_FRAME_CNT);
    byte_cnt1.wrd = AAL_PON_REG_READ(XGPN_USBG_IDLE_XGEM_BYTE_CNT1);
    byte_cnt0.wrd = AAL_PON_REG_READ(XGPN_USBG_IDLE_XGEM_BYTE_CNT0);
    stats->frame_cnt = frm_cnt.bf.cnt;
    stats->byte_cnt = ca_inter_to64(byte_cnt1.wrd,byte_cnt0.wrd);
    AAL_XGPON_UNLOCK();

    return CA_E_OK;
}

ca_status_t aal_xgpon_sniffer_glb_stats_get(
    CA_IN ca_device_id_t dev_id, 
	CA_OUT ca_uint32_t *sniffer_sts)
{
    XGPN_XGPN_GLB_SNIFFER_STS_t glb_sniffer_sts;

    XGPON_ASSERT(sniffer_sts);

    glb_sniffer_sts.wrd = AAL_PON_REG_READ( XGPN_XGPN_GLB_SNIFFER_STS);
    *sniffer_sts = glb_sniffer_sts.bf.sniffer_mem_ptr;
    return CA_E_OK;

}

ca_status_t aal_xgpon_sniffer_glb_cfg_get(
    CA_IN ca_device_id_t dev_id, 
	CA_OUT aal_xgpon_sniffer_glb_cfg_t *sniffer_cfg)
{
    XGPN_XGPN_GLB_SNIFFER_CFG_t sniffer_glb_cfg;

    XGPON_ASSERT(sniffer_cfg);

    sniffer_glb_cfg.wrd = AAL_PON_REG_READ( XGPN_XGPN_GLB_SNIFFER_CFG);
    
    sniffer_cfg->cpu_read_sniffer = sniffer_glb_cfg.bf.cpu_read_sniffer;
    sniffer_cfg->dis_int = sniffer_glb_cfg.bf.dis_int;
    sniffer_cfg->ploamd_uc_all = sniffer_glb_cfg.bf.ploamd_uc_all;
    sniffer_cfg->ploamd_bc = sniffer_glb_cfg.bf.ploamd_bc;
    sniffer_cfg->bwmp_bc = sniffer_glb_cfg.bf.bwmp_bc;
    sniffer_cfg->bwmp_ploamu = sniffer_glb_cfg.bf.bwmp_ploamu;
    sniffer_cfg->ploamu_en = sniffer_glb_cfg.bf.ploamu_en;
    sniffer_cfg->ploamd_en = sniffer_glb_cfg.bf.ploamd_en;
    sniffer_cfg->bwmp_en = sniffer_glb_cfg.bf.bwmp_en;
    return CA_E_OK;

}

ca_status_t aal_xgpon_sniffer_glb_cfg_set(
    CA_IN ca_device_id_t dev_id, 
    CA_OUT aal_xgpon_sniffer_glb_cfg_msk_t sniffer_cfg_msk,
    CA_OUT aal_xgpon_sniffer_glb_cfg_t *sniffer_cfg)
{
    ca_status_t ret = CA_E_OK;
    XGPN_XGPN_GLB_SNIFFER_CFG_t sniffer_glb_cfg;
    
    if (!sniffer_cfg_msk.u32) {
        return CA_E_OK;
    }
    XGPON_ASSERT(sniffer_cfg);

    AAL_XGPON_LOCK();
    
    sniffer_glb_cfg.wrd = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_CFG);
    if(sniffer_cfg_msk.s.cpu_read_sniffer){
        sniffer_glb_cfg.bf.cpu_read_sniffer = sniffer_cfg->cpu_read_sniffer;
    }
    if(sniffer_cfg_msk.s.dis_int){
        sniffer_glb_cfg.bf.dis_int = sniffer_cfg->dis_int;
    }
    if(sniffer_cfg_msk.s.ploamd_uc_all){
        sniffer_glb_cfg.bf.ploamd_uc_all = sniffer_cfg->ploamd_uc_all;
    }
    if(sniffer_cfg_msk.s.ploamd_bc){
        sniffer_glb_cfg.bf.ploamd_bc = sniffer_cfg->ploamd_bc;
    }
    
    if(sniffer_cfg_msk.s.bwmp_bc){
        sniffer_glb_cfg.bf.bwmp_bc = sniffer_cfg->bwmp_bc;
    }
    if(sniffer_cfg_msk.s.bwmp_ploamu){
        sniffer_glb_cfg.bf.bwmp_ploamu = sniffer_cfg->bwmp_ploamu;
    }

    if(sniffer_cfg_msk.s.ploamu_en){
        sniffer_glb_cfg.bf.ploamu_en = sniffer_cfg->ploamu_en;
    }
    if(sniffer_cfg_msk.s.ploamd_en){
        sniffer_glb_cfg.bf.ploamd_en = sniffer_cfg->ploamd_en;
    }
    if(sniffer_cfg_msk.s.bwmp_en){
        sniffer_glb_cfg.bf.bwmp_en = sniffer_cfg->bwmp_en;
    }
    AAL_PON_REG_WRITE (sniffer_glb_cfg.wrd, XGPN_XGPN_GLB_SNIFFER_CFG);

    AAL_XGPON_UNLOCK();
    return ret;

}

#define XGPN_GLB_SNIFFER_MEM_DUMP   0x80000000
#define XGPN_SNIFF_ENTRY_MAX        1024
#define XGPN_SNIFF_ENTRY_PLOAMD     0
#define XGPN_SNIFF_ENTRY_BWMAP      0x40000000
#define XGPN_SNIFF_ENTRY_PLOAMU     0x80000000

#define XGPN_SNIFF_BWMAP_ALL		0x00000001
#define XGPN_SNIFF_PLOAMD			0x00000002
#define XGPN_SNIFF_PLOAMU			0x00000004
#define XGPN_SNIFF_BWMAP_PLOAM		0x00000008

#define XGPN_SNIFFER_DOWNSTREAM     0
#define XGPN_SNIFFER_UPSTREAM       1
#define XGPN_SNIFF_BWMAP            1
#define XGPN_SNIFF_PLOAM            0

ca_status_t aal_xgpon_sniffer_buf_get(
	CA_IN ca_device_id_t dev_id, 
	CA_IN ca_uint32_t start_entry, 
	CA_IN ca_uint32_t entry_num)
{
    ca_uint32_t end = 0;
    ca_uint32_t i = 0;
    ca_uint32_t first_b = 0;
    ca_uint32_t second_b = 0;
    ca_uint32_t third_b = 0;
    ca_uint32_t fourth_b = 0;
    ca_uint32_t type = XGPN_SNIFF_ENTRY_PLOAMU + 1; 
    ca_uint32_t timeout = 0;
    
    aal_xgpon_sniffer_buffer_t sniffer_buffer;
    volatile ca_uint32_t reg_addr = XGPN_XGPN_GLB_SNIFFER_MEM_ACCESS;
    volatile ca_uint32_t reg_data = XGPN_GLB_SNIFFER_MEM_DUMP;
    XGPN_XGPN_GLB_SNIFFER_MEM_DATA0_t mem_data0;
    XGPN_XGPN_GLB_SNIFFER_MEM_DATA1_t mem_data1;
    XGPN_XGPN_GLB_SNIFFER_MEM_DATA2_t mem_data2;
    XGPN_XGPN_GLB_SNIFFER_MEM_DATA3_t mem_data3;
    
    aal_xgpon_bwmp_hdr_t bwmp_hdr;
	ploam_ngpon2_header_t plm_hdr;
    ca_uint32_t previous = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_CFG);
	//ploam_key_ctrl_t key_ctrl;
	ploam_ds_burst_pfl_t burst_prfl;
	//ploam_ngpon2_us_serial_number_t us_sn;
	//ploam_ngpon2_us_registration_t us_registration;
	//ploam_us_key_report_t key_rpt;
	ploam_us_ack_t        us_ack;
	ca_xgpon_key_control_ploam_msg_t plmd_key_ctrl;
	ca_xgpon_key_report_ploam_msg_t  plmu_key_rpt;
    
    memset(&sniffer_buffer, 0, sizeof(aal_xgpon_sniffer_buffer_t));

    memset(&bwmp_hdr, 0, sizeof(aal_xgpon_bwmp_hdr_t));
	memset(&plm_hdr, 0, sizeof(ploam_ngpon2_header_t));
	memset(&plmd_key_ctrl, 0, sizeof(ca_xgpon_key_control_ploam_msg_t));
	memset(&plmu_key_rpt, 0, sizeof(ca_xgpon_key_report_ploam_msg_t));
    
    //CPU read from sniffer memory.
    AAL_PON_REG_WRITE(0x8001007F, XGPN_XGPN_GLB_SNIFFER_CFG);

	end = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS);
	printk("%s(): sniffer end ptr %d\n", __func__, end);
    
    for (i = start_entry; i < (start_entry + entry_num) && (i < XGPN_SNIFF_ENTRY_MAX); i++) {

        //in-direct access sniffer memory
        timeout = 0;
        reg_data = XGPN_GLB_SNIFFER_MEM_DUMP + i;
        AAL_PON_REG_WRITE(reg_data, XGPN_XGPN_GLB_SNIFFER_MEM_ACCESS);

        do {
            ca_udelay(1);
            reg_data = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_MEM_ACCESS);
            if (0 == (reg_data & XGPN_GLB_SNIFFER_MEM_DUMP))
                break;
        }while(timeout ++ < 50000); /* wait no more than 50ms */

        if (timeout >= 50000) {
            ca_printf("error: read entry %d timeout!\n", i);
            continue;
        }

        //dump 4 DWORDs
        first_b     = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_MEM_DATA0);
        second_b    = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_MEM_DATA1);
        third_b     = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_MEM_DATA2);
        fourth_b    = AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_MEM_DATA3);
        type = (fourth_b & 0xc0000000);
        switch(type) {
        case XGPN_SNIFF_ENTRY_PLOAMD:
			plm_hdr.onu_id      = second_b>>16;
			plm_hdr.msg_id      = (second_b>>8)&0xff;
			plm_hdr.seq_no 	    = second_b&0xff;
			
			switch(plm_hdr.msg_id)
			{
				case PLOAM_NGPON2_DS_BURST_PROFILE:
					burst_prfl.burst_profile_ctrl = (first_b>>24)&0xff;
					burst_prfl.us_fec_ind         = (first_b>>16)&0xff;
					burst_prfl.delimiter_len      = (first_b>>8)&0xff;
					ca_printf("entry: %d type: PLOAMD SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
					ca_printf("HDR Details: ONUID=0x%08x %s SEQID=0x%08x Ctrl=0x%08x Us_fec_ind=0x%08x Delimiter_len=0x%08x \n", 
							plm_hdr.onu_id,"PROFILE",plm_hdr.seq_no, burst_prfl.burst_profile_ctrl,burst_prfl.us_fec_ind,burst_prfl.delimiter_len);
				
				break;
				case PLOAM_NGPON2_DS_ASSIGN_ONUID:
					ca_printf("entry: %d type: PLOAMD:ASSIGN_ONUID SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
				break;
				case PLOAM_NGPON2_DS_RANGING_TIME:
					ca_printf("entry: %d type: PLOAMD:RANG_TIME SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
				break;
				case PLOAM_NGPON2_DS_DEACT_ONUID:
					ca_printf("entry: %d type: PLOAMD:DEACT_ONUID SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
				break;
				case PLOAM_NGPON2_DS_DISABLE_SN:
					ca_printf("entry: %d type: PLOAMD:DISABLE_SN SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
				break;
				case PLOAM_NGPON2_DS_REQUEST_REGISTRATION:
					ca_printf("entry: %d type: PLOAMD:REQ_REG SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
				break;
				case PLOAM_NGPON2_DS_ASSIGN_ALLOCID:
					ca_printf("entry: %d type: PLOAMD:ASSIGN_ALLOC SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
				break;
				case PLOAM_NGPON2_DS_KEY_CTRL:
					plmd_key_ctrl.ctrl_flag   = (first_b>>16)&0xff;
					plmd_key_ctrl.key_idx     = (first_b>>8)&0xff;
					plmd_key_ctrl.key_length  = first_b&0xff;
					ca_printf("entry: %d type: PLOAMD SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
					ca_printf("HDR Details: ONUID=0x%08x %s SEQID=0x%08x %s KeyIndex=0x%08x keyLength=0x%08x \n", 
							plm_hdr.onu_id,"KEY_CTL",plm_hdr.seq_no, (plmd_key_ctrl.ctrl_flag == 0)?"NewKey":"Cfm",plmd_key_ctrl.key_idx,plmd_key_ctrl.key_length);
				break;
				
				default:
				ca_printf("entry: %d type: UNKNON SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i,  fourth_b, third_b, second_b, first_b);
				break;
			}
			break;
        case XGPN_SNIFF_ENTRY_BWMAP:
			bwmp_hdr.alloc_id     = second_b>>18;
			bwmp_hdr.flag         =(second_b>>16)&0x3;
			bwmp_hdr.start_time   =second_b&0xffff;
			bwmp_hdr.grant_size   =first_b>>16;
			bwmp_hdr.fwi          =(first_b>>15)&0x1;
			bwmp_hdr.bst_prfl     =(first_b>>13)&0x3;

			ca_printf("entry: %d type: BWMP SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
            ca_printf("HDR Details: AllocID=0x%08x Dbru=0x%08x PLOAMu=0x%08x StartTime=0x%08x GrantSize=0x%08x FWI=0x%08x BP=0x%08x\n", 
				bwmp_hdr.alloc_id,(bwmp_hdr.flag>>1), (bwmp_hdr.flag&0x1),bwmp_hdr.start_time,bwmp_hdr.grant_size,bwmp_hdr.fwi,bwmp_hdr.bst_prfl);
			break;
        case XGPN_SNIFF_ENTRY_PLOAMU:
			plm_hdr.onu_id      = second_b>>16;
			plm_hdr.msg_id = (second_b>>8)&0xff;
			plm_hdr.seq_no 	  = second_b&0xff;
			
			switch(plm_hdr.msg_id)
			{
				case PLOAM_NGPON2_US_SERIAL_NUMBER:
                    if(AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS)== 1023){
                        ca_printf("LAST-ENTRY>>>");
					    ca_printf("entry: %d type: PLOAMU:SERI_NUM SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
                    }else{
					    ca_printf("entry: %d type: PLOAMU:SERI_NUM SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
                    }
                break;
				case PLOAM_NGPON2_US_REGISTRATION:
                    if(AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS)== 1023){
                        ca_printf("LAST-ENTRY>>>");
					    ca_printf("entry: %d type: PLOAMU:REGIST SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
                    }else{
					    ca_printf("entry: %d type: PLOAMU:REGIST SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
                    }
                break;
				case PLOAM_NGPON2_US_KEY_REPORT:
					plmu_key_rpt.report_type = (first_b>>24)&0xff;
					plmu_key_rpt.key_idx     = (first_b>>16)&0xff;
					plmu_key_rpt.frament_no  = (first_b>>8)&0xff;
					if(AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS)== 1023){
                        ca_printf("LAST-ENTRY>>>");
    					ca_printf("entry: %d type: PLOAMU SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
    					ca_printf("HDR Details: ONUID=0x%08x %s SEQID=0x%08x %s KeyIndex=0x%08x keyFragNo=0x%08x \n", 
    						plm_hdr.onu_id,"KEY_RPT",plm_hdr.seq_no, (plmu_key_rpt.report_type == 0)?"NewKey":"Cfm",plmu_key_rpt.key_idx,plmu_key_rpt.frament_no);
                    }else{
    					ca_printf("entry: %d type: PLOAMU SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
    					ca_printf("HDR Details: ONUID=0x%08x %s SEQID=0x%08x %s KeyIndex=0x%08x keyFragNo=0x%08x \n", 
    						plm_hdr.onu_id,"KEY_RPT",plm_hdr.seq_no, (plmu_key_rpt.report_type == 0)?"NewKey":"Cfm",plmu_key_rpt.key_idx,plmu_key_rpt.frament_no);
                    }
                break;
				case PLOAM_NGPON2_US_ACK:
					us_ack.complete_code = (first_b>>24)&0xff;
					us_ack.attenuation     = (first_b>>16)&0xff;
					us_ack.power_level_cap  = (first_b>>8)&0xff;
					if(AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS)== 1023){
                        ca_printf("LAST-ENTRY>>>");
                        ca_printf("entry: %d type: PLOAMU SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
                        ca_printf("HDR Details: ONUID=0x%08x %s SEQID=0x%08x COMP_CODE=0x%08x attunate=0x%08x pwrlevel=0x%08x \n", 
                            plm_hdr.onu_id,"ACK",plm_hdr.seq_no, us_ack.complete_code,us_ack.attenuation,us_ack.power_level_cap);
    				}else{
    					ca_printf("entry: %d type: PLOAMU SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
    					ca_printf("HDR Details: ONUID=0x%08x %s SEQID=0x%08x COMP_CODE=0x%08x attunate=0x%08x pwrlevel=0x%08x \n", 
    						plm_hdr.onu_id,"ACK",plm_hdr.seq_no, us_ack.complete_code,us_ack.attenuation,us_ack.power_level_cap);
                    }
                break;
				case PLOAM_NGPON2_US_SLEEP_REQ:
                    if(AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS)== 1023){
                        ca_printf("LAST-ENTRY>>>");
					    ca_printf("entry: %d type: PLOAMU:SLEEP_REQ SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
                    }else {
					    ca_printf("entry: %d type: PLOAMU:SLEEP_REQ SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i, fourth_b, third_b, second_b, first_b);
                    }
                break;
				
				default:
                if(AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS)== 1023){
                    ca_printf("LAST-ENTRY>>>");
				    ca_printf("entry: %d type: UNKNON SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i,  fourth_b, third_b, second_b, first_b);
                }else {
				    ca_printf("entry: %d type: UNKNON SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i,  fourth_b, third_b, second_b, first_b);
                }
                break;
			}
			break;
        default:
            if(AAL_PON_REG_READ(XGPN_XGPN_GLB_SNIFFER_STS)== 1023){
                ca_printf("LAST-ENTRY>>>");
                ca_printf("entry: %d type: UNKNON SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i,  fourth_b, third_b, second_b, first_b);
            }else {
                ca_printf("entry: %d type: UNKNON SFC: 0x%08x 0x%08x HDR 0x%08x 0x%08x\n", i,  fourth_b, third_b, second_b, first_b);
            }
            break;
        }
    }

	//enable HW write
	AAL_PON_REG_WRITE(previous, XGPN_XGPN_GLB_SNIFFER_CFG);

    return CA_E_OK;

}

#ifdef AAL_XGPN_DEBUG
ca_status_t aal_xgpon_sniffer_stop_on_intr_set(
	CA_IN ca_device_id_t dev_id,
	CA_IN ca_uint32_t bwmp_intr_en,
	CA_IN ca_uint32_t xgpn_glb_intr_en)
{
	ca_uint32_t reg_addr = XGPN_GLB_SNIFFER_MEM_ACCESS;
	ca_uint32_t reg_data = 0;
	if(bwmp_intr_en|xgpn_glb_intr_en) {
		//enable stop on interrupt
		AAL_PON_REG_WRITE(XGPN_SNIFF_STOP_ON_INTR | XGPN_SNIFF_BWMAP_ALL, XGPN_GLB_SNIFFER_FILTER);

		//enable interrupt 1
		AAL_PON_REG_WRITE(bwmp_intr_en, XGPN_BWMP_INTERRUPTE1);
		AAL_PON_REG_WRITE(xgpn_glb_intr_en, XGPN_XGPN_GLB_INTERRUPT1E);
	}
	else {

		//turn off stop on interrupt 
		AAL_PON_REG_WRITE(XGPN_SNIFF_BWMAP_ALL | XGPN_SNIFF_PLOAMD | XGPN_SNIFF_PLOAMU | XGPN_SNIFF_PLOAMU, XGPN_GLB_SNIFFER_FILTER);
		//disable all interrupt
		AAL_PON_REG_WRITE(0, XGPN_BWMP_INTERRUPTE1);
		AAL_PON_REG_WRITE(0, XGPN_XGPN_GLB_INTERRUPT1E);
		//clear all interrupt
		AAL_PON_REG_WRITE(0xffffffff, XGPN_BWMP_INTERRUPT1);
		AAL_PON_REG_WRITE(0xffffffff, XGPN_XGPN_GLB_INTERRUPT1);
	}
	return CA_E_OK;
}
			
ca_status_t aal_xgpon_sniffer_stop_on_intr_get(
	CA_IN ca_device_id_t dev_id,
	CA_IN ca_uint32_t bwmp_intr_en,
	CA_OUT ca_uint32_t *xgpn_glb_intr_en)
{
	//*bwmp_intr_en = AAL_PON_REG_READ(XGPN_BWMP_INTERRUPTE1);
	*xgpn_glb_intr_en = AAL_PON_REG_READ(XGPN_XGPN_GLB_INTERRUPT1E);
	//*enable = g_enable1;
	return CA_E_OK;
}
#endif


ca_status_t aal_xgpon_software_reset_set(
    CA_IN ca_device_id_t dev_id,
    CA_IN ca_uint32_t    step_num)
{
#if defined(CONFIG_ARCH_CORTINA_VENUS) && defined(CONFIG_CORTINA_BOARD_FPGA)
    if (AAL_XGPON_SW_RESET_STEP1 == step_num)
    {
        ca_printf("First step\r\n");

        ca_printf("Disable intterupts\r\n");
        AAL_PON_REG_WRITE(0, XGPN_XGPN_GLB_INTERRUPT0E); /* reg_op w 0xf550400c 0 */
        AAL_PON_REG_WRITE(0, XGPN_ACTP_INTERRUPTE0); /* reg_op w 0xf5504d20 0 */
        AAL_PON_REG_WRITE(0, XGPN_ACTP_ST_CHG_INTE); /* reg_op w 0xf5504d2c 0 */

        ca_printf("Disable mac_en\r\n");
        AAL_PON_REG_FIELD_WRITE(XGPN_XGPN_GLB_CTRL, mac_en, 0);

        ca_printf("Please reset Titan and do other steps next\r\n");
        ca_printf("Switch Titan SW4 to 1/2/3/4 = ON, OFF, ON, ON\r\n");
        return CA_E_OK;
    }

    if (AAL_XGPON_SW_RESET_STEP2 == step_num)
    {
        ca_printf("Second step, please make sure reset Titan before this step\r\n");
        ca_printf("Switch Titan SW4 1/2/3/4 = OFF, ON, ON, ON\r\n");

        //clear API database before enable pon
        ca_gpon_cb_init(0);

        ca_printf("Enable intterupts\r\n");
        AAL_PON_REG_WRITE(0xf7, XGPN_XGPN_GLB_INTERRUPT0E); /* reg_op w 0xf550400c 0xf7*/
        AAL_PON_REG_WRITE(0xf, XGPN_ACTP_INTERRUPTE0); /* reg_op w 0xf5504d20 0xf */
        AAL_PON_REG_WRITE(0xfffffc2, XGPN_ACTP_ST_CHG_INTE); /* reg_op w 0xf5504d2c 0xfffffc2 */

        ca_printf("Set XGPN_XGPN_GLB_CTRL to 0xa2\r\n");
        AAL_PON_REG_WRITE(0xa2, XGPN_XGPN_GLB_CTRL);
        return CA_E_OK;
    }
#endif

    return CA_E_OK;
}


ca_status_t aal_xgpon_software_reset_get(
    CA_IN  ca_device_id_t dev_id,
    CA_OUT ca_uint32_t    *step_num)
{
    *step_num = 0;
#if defined(CONFIG_ARCH_CORTINA_VENUS) && defined(CONFIG_CORTINA_BOARD_FPGA)
    ca_printf("Debug venus software reset steps\r\n");
    ca_printf("Step1: ON Venus G3 board, Disable intterupts and mac_en\r\n");
    ca_printf("Step2: ON Venus Titan board, Switch Titan SW4 to 1/2/3/4 = ON, OFF, ON, ON \r\n");
    ca_printf("Step3: Reset Venus Titan board, Switch Titan SW4 1/2/3/4 = OFF, ON, ON, ON\r\n");
    ca_printf("Step4: ON Venus G3 board, Enable intterupts and mac_en\r\n");
#endif
    return CA_E_OK;
}

#ifdef CONFIG_LUNA_G3_SERIES
ca_status_t aal_xgpon_backup_ponmac(ca_device_id_t device_id)
{
    ca_uint32_t tcontId;
    ca_uint32_t priId;
    ca_uint32_t gem_idx;
    ca_uint32_t queue_idx;
    ca_uint32_t br_tcont_num = AAL_XGPON_MAX_TCONT_NUM;
    ca_uint32_t br_queue_num = 8;

    if(tcont_num_per_chip < AAL_XGPON_MAX_TCONT_NUM)
        br_tcont_num = tcont_num_per_chip;

    if(queue_num_per_tcont < 8)
        br_queue_num = queue_num_per_tcont;

//printk("%s %d br_tcont_num=%d br_queue_num=%d\n",__FUNCTION__,__LINE__,br_tcont_num,br_queue_num);

    for(tcontId=0;tcontId<br_tcont_num;tcontId++)
    {
        for(priId=0;priId<br_queue_num;priId++)
        {
            aal_puc_port_voq_map_get(device_id, tcontId, priId,&tmp_puc_queue_cfg[tcontId][priId]);
            AAL_XGPON_DBG("tcontId=%d priId=%d queue_id=%d weight=%d pri=%d enable=%d\n",tcontId,priId
                ,tmp_puc_queue_cfg[tcontId][priId].queue_id
                ,tmp_puc_queue_cfg[tcontId][priId].queue_weight
                ,tmp_puc_queue_cfg[tcontId][priId].queue_pri
                ,tmp_puc_queue_cfg[tcontId][priId].queue_ena);
        }
    }

    for(gem_idx=0;gem_idx<AAL_XGPON_MAX_GEM_NUM;gem_idx++)
    {
        aal_xgpon_usbg_xgem_cfg_get(device_id,gem_idx,&tmp_usbg_xgem_cfg[gem_idx]);

        if(tmp_usbg_xgem_cfg[gem_idx].port_en != 1)
            continue;

        AAL_XGPON_DBG("usbg gem_idx=%d port_en=%d xgem_id=%d aes_mode=%d mc_flag=%d\n",gem_idx
            ,tmp_usbg_xgem_cfg[gem_idx].port_en
            ,tmp_usbg_xgem_cfg[gem_idx].xgem_id
            ,tmp_usbg_xgem_cfg[gem_idx].aes_mode
            ,tmp_usbg_xgem_cfg[gem_idx].mc_flag);
    }

    for(gem_idx=0;gem_idx<AAL_XGPON_MAX_GEM_NUM;gem_idx++)
    {
        aal_xgpon_dsfp_xgem_get(device_id,gem_idx,&tmp_dsfp_xgem_cfg[gem_idx]);

        if(tmp_dsfp_xgem_cfg[gem_idx].gem_vld !=1)
            continue;

        AAL_XGPON_DBG("dsfp gem_idx=%d gem_vld=%d gem_id=%d gem_mc=%d\n",gem_idx
            ,tmp_dsfp_xgem_cfg[gem_idx].gem_vld
            ,tmp_dsfp_xgem_cfg[gem_idx].gem_id
            ,tmp_dsfp_xgem_cfg[gem_idx].gem_mc);
    }

    aal_puc_ctrl_cfg_get(device_id,&tmp_puc_ctrl_config);
    AAL_XGPON_DBG("puc_ctrl_cfg pkt_mode_class0_cnt=%d pkt_mode_class1_cnt=%d force_init_done=%d addtok_en=%d subtok_en=%d shp_en=%d\n"
            ,tmp_puc_ctrl_config.pkt_mode_class0_cnt
            ,tmp_puc_ctrl_config.pkt_mode_class1_cnt
            ,tmp_puc_ctrl_config.force_init_done
            ,tmp_puc_ctrl_config.addtok_en
            ,tmp_puc_ctrl_config.subtok_en
            ,tmp_puc_ctrl_config.shp_en);


    aal_puc_btc_cfg_get(device_id,&tmp_puc_btc_config);
    AAL_XGPON_DBG("pfovrhd=%d schmode=%d ethmode=%d wdaligned=%d lrgfrmfragen=%d pyld1gfecon=%d schen=%d minrmnwindowsz=%d\n" 
            ,tmp_puc_btc_config.pfovrhd
            ,tmp_puc_btc_config.schmode
            ,tmp_puc_btc_config.ethmode
            ,tmp_puc_btc_config.wdaligned
            ,tmp_puc_btc_config.lrgfrmfragen
            ,tmp_puc_btc_config.pyld1gfecon
            ,tmp_puc_btc_config.schen
            ,tmp_puc_btc_config.minrmnwindowsz);

    for(queue_idx=0;queue_idx<AAL_PUC_MAX_QUEUE_NUM;queue_idx++)
    {
        aal_puc_voq_tbc_mem_get(device_id,queue_idx,&(tmp_puc_voq_tbc_mem_cfg[queue_idx]));
        AAL_XGPON_DBG("puc_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
            ,queue_idx
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_k
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_m
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].bs
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].enb
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].mode
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].tbc
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);

        aal_puc_rl_voq_tbc_mem_get(device_id,queue_idx,&(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx]));
        AAL_XGPON_DBG("puc_rl_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
            ,queue_idx
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_k
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_m
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].bs
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].mode
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].tbc
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
    }

    for(queue_idx=0;queue_idx<64;queue_idx++)//bp only has 64 entries
    {
        aal_puc_voq_bp_remap_data_get(device_id, queue_idx, &(tmp_puc_bp_tqmvoqid[queue_idx]));
        AAL_XGPON_DBG("puc_voq_bp_remap_data idx=%d tqmvoqid=%d\n",queue_idx,tmp_puc_bp_tqmvoqid[queue_idx]);
    }

    aal_puc_voq_buf_lmt_sel1500_get(device_id, &tmp_puc_voq_buf_lmt_sel1500_cfg);
    AAL_XGPON_DBG("puc_voq_buf_lmt 1500=%d\n",tmp_puc_voq_buf_lmt_sel1500_cfg);

    aal_puc_voq_buf_lmt_sel3116_get(device_id, &tmp_puc_voq_buf_lmt_sel3116_cfg);
    AAL_XGPON_DBG("puc_voq_buf_lmt 3116=%d\n",tmp_puc_voq_buf_lmt_sel3116_cfg);

    aal_puc_voq_buf_lmt_sel4732_get(device_id, &tmp_puc_voq_buf_lmt_sel4732_cfg);
    AAL_XGPON_DBG("puc_voq_buf_lmt 3116=%d\n",tmp_puc_voq_buf_lmt_sel4732_cfg);

    aal_puc_voq_buf_lmt_sel6348_get(device_id, &tmp_puc_voq_buf_lmt_sel6348_cfg);
    AAL_XGPON_DBG("puc_voq_buf_lmt 6348=%d\n",tmp_puc_voq_buf_lmt_sel6348_cfg);
    
    aal_puc_voq_buf_lmt_a_get(device_id, &tmp_puc_voq_buf_limit_a);
    AAL_XGPON_DBG("puc_voq_buf_lmt a=%d\n",tmp_puc_voq_buf_limit_a);
    
    aal_puc_voq_buf_lmt_b_get(device_id, &tmp_puc_voq_buf_limit_b);
    AAL_XGPON_DBG("puc_voq_buf_lmt b=%d\n",tmp_puc_voq_buf_limit_b);
    
    aal_puc_voq_buf_lmt_c_get(device_id, &tmp_puc_voq_buf_limit_c);
    AAL_XGPON_DBG("puc_voq_buf_lmt c=%d\n",tmp_puc_voq_buf_limit_c);

    return CA_E_OK;
}

ca_status_t aal_xgpon_restore_ponmac(ca_device_id_t device_id)
{
    ca_uint32_t tcontId;

    ca_uint32_t priId;
    aal_puc_voq_drop_cfg_msk_t voq_drp_mask;
    aal_puc_voq_drop_cfg_t voq_drp;
    aal_puc_queue_cfg_msk_t queue_cfg_mask;
    aal_puc_queue_cfg_t queue_cfg;
    aal_puc_voq_sche_cfg_t voq_sche_config;
    ca_uint32_t tbc = 0;

    ca_uint32_t gem_idx;
    aal_xgpon_usbg_gem_config_msk_t us_mask;
    aal_xgpon_usbg_gem_config_t us_xgem_cfg;

    aal_xgpon_dsfp_xgem_cfg_msk_t ds_mask;
    aal_xgpon_dsfp_xgem_cfg_t ds_xgem_cfg;

    aal_puc_ctrl_cfg_msk_t puc_ctrl_config_mask; 
    aal_puc_btc_cfg_msk_t puc_btc_mask;

    ca_uint32_t queue_idx;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_voq_tbc_cfg_msk;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_rl_voq_tbc_cfg_msk;

    ca_uint32_t br_tcont_num = AAL_XGPON_MAX_TCONT_NUM;
    ca_uint32_t br_queue_num = 8;
       
    if(tcont_num_per_chip < AAL_XGPON_MAX_TCONT_NUM)
        br_tcont_num = tcont_num_per_chip;

    if(queue_num_per_tcont < 8)
        br_queue_num = queue_num_per_tcont;

//printk("%s %d br_tcont_num=%d br_queue_num=%d\n",__FUNCTION__,__LINE__,br_tcont_num,br_queue_num);

    for(tcontId=0;tcontId<br_tcont_num;tcontId++)
    {
        for(priId=0;priId<br_queue_num;priId++)
        {
            AAL_XGPON_DBG("tcontId=%d priId=%d queue_id=%d weight=%d pri=%d enable=%d\n",tcontId,priId
                ,tmp_puc_queue_cfg[tcontId][priId].queue_id
                ,tmp_puc_queue_cfg[tcontId][priId].queue_weight
                ,tmp_puc_queue_cfg[tcontId][priId].queue_pri
                ,tmp_puc_queue_cfg[tcontId][priId].queue_ena);

            voq_drp_mask.u32      = 0;
            voq_drp_mask.s.voqid  = 1;
            voq_drp_mask.s.enable = 1;

            memset(&voq_drp, 0, sizeof(voq_drp));
            voq_drp.enable = 0;
            voq_drp.voqid = tmp_puc_queue_cfg[tcontId][priId].queue_id;
            aal_puc_voq_drop_set(device_id, voq_drp_mask, &voq_drp);

            queue_cfg_mask.u32 = ~0U;    
            queue_cfg.queue_id = tmp_puc_queue_cfg[tcontId][priId].queue_id;
            queue_cfg.queue_weight = tmp_puc_queue_cfg[tcontId][priId].queue_weight;
            queue_cfg.queue_pri = tmp_puc_queue_cfg[tcontId][priId].queue_pri;
            queue_cfg.queue_ena = tmp_puc_queue_cfg[tcontId][priId].queue_ena;

            aal_puc_port_voq_map_set(device_id, tcontId, priId, queue_cfg_mask, &queue_cfg);
#ifdef RTK_VOQ_VALID_SUPPORT
            aal_puc_voq_valid_set(device_id, queue_cfg.queue_id, queue_cfg.queue_ena);
#endif
            memset(&voq_sche_config, 0, sizeof(aal_puc_voq_sche_cfg_t));
            aal_puc_voq_sche_cfg_get(device_id,&voq_sche_config);
            if(voq_sche_config.basesel == 0)
                tbc = tmp_puc_queue_cfg[tcontId][priId].queue_weight * 128;
            if(voq_sche_config.basesel == 1)
                tbc = tmp_puc_queue_cfg[tcontId][priId].queue_weight * 256;
            if(voq_sche_config.basesel == 2)
                tbc = tmp_puc_queue_cfg[tcontId][priId].queue_weight * 512;
            if(voq_sche_config.basesel == 3)
                tbc = tmp_puc_queue_cfg[tcontId][priId].queue_weight * 1024;
            aal_puc_port_voq_sche_info_tbl_set(device_id,tcontId,priId,tbc);
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
    for(gem_idx=0;gem_idx<AAL_XGPON_MAX_GEM_NUM;gem_idx++)
    {
        if(tmp_usbg_xgem_cfg[gem_idx].port_en != 1)
            continue;

        AAL_XGPON_DBG("usbg gem_idx=%d port_en=%d xgem_id=%d aes_mode=%d mc_flag=%d\n",gem_idx
            ,tmp_usbg_xgem_cfg[gem_idx].port_en
            ,tmp_usbg_xgem_cfg[gem_idx].xgem_id
            ,tmp_usbg_xgem_cfg[gem_idx].aes_mode
            ,tmp_usbg_xgem_cfg[gem_idx].mc_flag);

        us_mask.u32 = ~0U;
        us_xgem_cfg.port_en     = tmp_usbg_xgem_cfg[gem_idx].port_en;
        us_xgem_cfg.xgem_id     = tmp_usbg_xgem_cfg[gem_idx].xgem_id;
        us_xgem_cfg.aes_mode    = tmp_usbg_xgem_cfg[gem_idx].aes_mode;
        us_xgem_cfg.mc_flag     = tmp_usbg_xgem_cfg[gem_idx].mc_flag;
        
        aal_xgpon_usbg_xgem_cfg_set(device_id, gem_idx, us_mask, &us_xgem_cfg);
    }

    for(gem_idx=0;gem_idx<AAL_XGPON_MAX_GEM_NUM;gem_idx++)
    {
        if(tmp_dsfp_xgem_cfg[gem_idx].gem_vld !=1)
            continue;

        AAL_XGPON_DBG("dsfp gem_idx=%d gem_vld=%d gem_id=%d gem_mc=%d\n",gem_idx
            ,tmp_dsfp_xgem_cfg[gem_idx].gem_vld
            ,tmp_dsfp_xgem_cfg[gem_idx].gem_id
            ,tmp_dsfp_xgem_cfg[gem_idx].gem_mc);

        ds_mask.u32 = 0xffffffff;
        ds_xgem_cfg.gem_vld = tmp_dsfp_xgem_cfg[gem_idx].gem_vld;
        ds_xgem_cfg.gem_id = tmp_dsfp_xgem_cfg[gem_idx].gem_id;
        ds_xgem_cfg.gem_mc = tmp_dsfp_xgem_cfg[gem_idx].gem_mc;
        aal_xgpon_dsfp_xgem_set(device_id, gem_idx, ds_mask, &ds_xgem_cfg);
    }

    AAL_XGPON_DBG("puc_ctrl_cfg pkt_mode_class0_cnt=%d pkt_mode_class1_cnt=%d force_init_done=%d addtok_en=%d subtok_en=%d shp_en=%d\n"
            ,tmp_puc_ctrl_config.pkt_mode_class0_cnt
            ,tmp_puc_ctrl_config.pkt_mode_class1_cnt
            ,tmp_puc_ctrl_config.force_init_done
            ,tmp_puc_ctrl_config.addtok_en
            ,tmp_puc_ctrl_config.subtok_en
            ,tmp_puc_ctrl_config.shp_en);
    puc_ctrl_config_mask.u32 = ~0;
    aal_puc_ctrl_cfg_set(device_id, puc_ctrl_config_mask, &tmp_puc_ctrl_config);

    AAL_XGPON_DBG("pfovrhd=%d schmode=%d ethmode=%d wdaligned=%d lrgfrmfragen=%d pyld1gfecon=%d schen=%d minrmnwindowsz=%d\n" 
            ,tmp_puc_btc_config.pfovrhd
            ,tmp_puc_btc_config.schmode
            ,tmp_puc_btc_config.ethmode
            ,tmp_puc_btc_config.wdaligned
            ,tmp_puc_btc_config.lrgfrmfragen
            ,tmp_puc_btc_config.pyld1gfecon
            ,tmp_puc_btc_config.schen
            ,tmp_puc_btc_config.minrmnwindowsz);
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
        AAL_XGPON_DBG("puc_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
            ,queue_idx
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_k
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].rate_m
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].bs
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].enb
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].mode
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].tbc
            ,tmp_puc_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
        aal_puc_voq_tbc_mem_set(device_id,queue_idx,puc_voq_tbc_cfg_msk,&(tmp_puc_voq_tbc_mem_cfg[queue_idx]));
#ifdef RTK_VOQ_VALID_SUPPORT
        if(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb == 1) //workaround
            tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb = 0;
#endif
        AAL_XGPON_DBG("puc_rl_voq_tbc idx=%d rate_k=%d rate_m=%d bs=%d enb=%d mode=%d tbc=%d pkt_mode_class_sel=%d\n"
            ,queue_idx
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_k
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].rate_m
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].bs
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].enb
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].mode
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].tbc
            ,tmp_puc_rl_voq_tbc_mem_cfg[queue_idx].pkt_mode_class_sel);
        aal_puc_rl_voq_tbc_mem_set(device_id,queue_idx,puc_rl_voq_tbc_cfg_msk,&(tmp_puc_rl_voq_tbc_mem_cfg[queue_idx]));
    }

    for(queue_idx=0;queue_idx<64;queue_idx++)//bp only has 64 entries
    {
        AAL_XGPON_DBG("puc_voq_bp_remap_data idx=%d tqmvoqid=%d\n",queue_idx,tmp_puc_bp_tqmvoqid[queue_idx]);
        aal_puc_voq_bp_remap_data_set(device_id, queue_idx, tmp_puc_bp_tqmvoqid[queue_idx]);
    }

    AAL_XGPON_DBG("puc_voq_buf_lmt 1500=%d\n",tmp_puc_voq_buf_lmt_sel1500_cfg);
    aal_puc_voq_buf_lmt_sel1500_set(device_id, tmp_puc_voq_buf_lmt_sel1500_cfg);

    AAL_XGPON_DBG("puc_voq_buf_lmt 3116=%d\n",tmp_puc_voq_buf_lmt_sel3116_cfg);
    aal_puc_voq_buf_lmt_sel3116_set(device_id, tmp_puc_voq_buf_lmt_sel3116_cfg);

    AAL_XGPON_DBG("puc_voq_buf_lmt 4732=%d\n",tmp_puc_voq_buf_lmt_sel4732_cfg);
    aal_puc_voq_buf_lmt_sel4732_set(device_id, tmp_puc_voq_buf_lmt_sel4732_cfg);

    AAL_XGPON_DBG("puc_voq_buf_lmt 6348=%d\n",tmp_puc_voq_buf_lmt_sel6348_cfg);
    aal_puc_voq_buf_lmt_sel6348_set(device_id, tmp_puc_voq_buf_lmt_sel6348_cfg);

    AAL_XGPON_DBG("puc_voq_buf_lmt a=%d\n",tmp_puc_voq_buf_limit_a);
    aal_puc_voq_buf_lmt_a_set(device_id, tmp_puc_voq_buf_limit_a);

    AAL_XGPON_DBG("puc_voq_buf_lmt b=%d\n",tmp_puc_voq_buf_limit_b);
    aal_puc_voq_buf_lmt_b_set(device_id, tmp_puc_voq_buf_limit_b);
    
    AAL_XGPON_DBG("puc_voq_buf_lmt c=%d\n",tmp_puc_voq_buf_limit_c);
    aal_puc_voq_buf_lmt_c_set(device_id, tmp_puc_voq_buf_limit_c);

    return CA_E_OK;
}

ca_status_t aal_xgpon_restore_tcont(ca_device_id_t device_id)
{
    ca_uint32_t tcontId;
    aal_xgpon_bwmp_alloc_id_cfg_msk_t mask;
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;

    for(tcontId=1;tcontId<AAL_XGPON_MAX_TCONT_NUM;tcontId++)
    {
        if(tmp_alloc_id_cfg[tcontId].omci_en != 1)
            continue;

        AAL_XGPON_DBG("tcontId=%d omci_en=%d alloc_id=%d\n",tcontId
            ,tmp_alloc_id_cfg[tcontId].omci_en
            ,tmp_alloc_id_cfg[tcontId].alloc_id);

        mask.u32 = 0;
        mask.s.alloc_id =1;
        mask.s.omci_en =1;
        mask.s.ploam_en =1;
        alloc_id_cfg.alloc_id = tmp_alloc_id_cfg[tcontId].alloc_id;
        alloc_id_cfg.omci_en = tmp_alloc_id_cfg[tcontId].omci_en;
        alloc_id_cfg.ploam_en = 0;
        
        aal_xgpon_bwmp_tcont_set(device_id, AAL_XGPON_WAVE_CHANNEL_DEFAULT, tcontId, mask, &alloc_id_cfg);
    }
        
    return CA_E_OK;
}
#endif

EXPORT_SYMBOL(aal_xgpn_auto_reset);
EXPORT_SYMBOL(aal_xgpon_ups_mib_get);
EXPORT_SYMBOL(aal_xgpon_dns_mib_get);
EXPORT_SYMBOL(aal_xgpon_omci_mib_get);
EXPORT_SYMBOL(aal_xgpon_reset);
#ifdef CONFIG_LUNA_G3_SERIES
EXPORT_SYMBOL(aal_xgpon_backup_ponmac);
EXPORT_SYMBOL(aal_xgpon_restore_ponmac);
EXPORT_SYMBOL(aal_xgpon_restore_tcont);
#endif
//EXPORT_SYMBOL(aal_xgpon_mac_debug_set);
//EXPORT_SYMBOL(aal_xgpon_mac_debug_get);
//EXPORT_SYMBOL(aal_xgpon_us_ds_chanl_set);
//EXPORT_SYMBOL(aal_xgpon_us_ds_chanl_get);
EXPORT_SYMBOL(aal_xgpon_tcont_bwmp_grant_get);
EXPORT_SYMBOL(aal_xgpon_aes_debug_set);
EXPORT_SYMBOL(aal_xgpon_aes_debug_get);
EXPORT_SYMBOL(aal_xgpon_aes_state_get);
EXPORT_SYMBOL(aal_gpon_glb_ctrl_get);
EXPORT_SYMBOL(aal_gpon_glb_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_glb_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_glb_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_msk_get);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_msk_set);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_sk_get);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_sk_set);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_mic_key_get);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_mic_key_set);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_ik_get);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_ik_set);
EXPORT_SYMBOL(aal_xgpon_aes_uc_enc_key_get);
EXPORT_SYMBOL(aal_xgpon_aes_uc_enc_key_set);
EXPORT_SYMBOL(aal_xgpon_aes_mc_enc_key_get);
EXPORT_SYMBOL(aal_xgpon_aes_mc_enc_key_set);

EXPORT_SYMBOL(aal_xgpon_xgpn_glb_ver_id_get);
EXPORT_SYMBOL(aal_xgpon_xgpn_glb_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_xgpn_glb_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_xgpn_glb_dbg_ctrl_get);

EXPORT_SYMBOL(aal_xgpon_dsfp_hec_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_hec_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_sync_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_sync_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgtc_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgtc_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_rxmac_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_rxmac_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_rxmac_ctrl1_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_rxmac_ctrl1_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_key_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_key_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_mc_key_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_mc_key_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_set);
EXPORT_SYMBOL(aal_xgpon_dsfp_sync_state_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_super_frame_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_oc_body_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_bip_err_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_psbd_hec_err_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_bip_dw0_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_bip_dw1_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_psbd_hec_uncorrect_cnt_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgtc_hec_correct_cnt_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgtc_hec_uncorrect_cnt_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_err0_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_err1_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_los_word_cnt_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_err_key_cnt_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_rxmac_omci_frame_cnt_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_rxmac_xgem_frame_status_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_async_ff_status_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_mib_ram_status_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_mib_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgem_mib_set);
EXPORT_SYMBOL(aal_xgpon_usbg_glb_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_usbg_glb_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_usbg_xgtc_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_usbg_xgtc_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_usbg_guard_time_get);
EXPORT_SYMBOL(aal_xgpon_usbg_guard_time_set);
EXPORT_SYMBOL(aal_xgpon_usbg_idl_frame_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_idl_frame_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_xgtc_hd_ind_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_xgtc_hd_ind_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_aes_uni_key_idx_get);
EXPORT_SYMBOL(aal_xgpon_usbg_aes_uni_key_idx_set);
EXPORT_SYMBOL(aal_xgpon_usbg_xgems_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_xgem_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_xgem_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_xgem_buf_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_xgem_buf_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_dbru_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_dbru_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_ploamu_queue_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_ploamu_queue_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_frag_ff_dump_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_frag_ff_dump_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_xgtc_err_dump_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_xgtc_err_dump_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_dbru_rpt_max_cfg_get);
EXPORT_SYMBOL(aal_xgpon_usbg_dbru_rpt_max_cfg_set);
EXPORT_SYMBOL(aal_xgpon_usbg_dbru_bufocc_tbl_get);
EXPORT_SYMBOL(aal_xgpon_usbg_dbru_bufocc_tbl_set);
EXPORT_SYMBOL(aal_xgpon_usbg_bp_ver_ol_status_get );
EXPORT_SYMBOL(aal_xgpon_usbg_mib_ram_status_get);
EXPORT_SYMBOL(aal_xgpon_usbg_mib_ctrl_get );
EXPORT_SYMBOL(aal_xgpon_usbg_mib_ctrl_set );
EXPORT_SYMBOL(aal_xgpon_usbg_xgem_mib_get );
EXPORT_SYMBOL(aal_xgpon_usbg_xgem_mib_set );
EXPORT_SYMBOL(aal_xgpon_usbg_stats_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_glb_ctrl_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_glb_ctrl_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_tcont_get);
EXPORT_SYMBOL(aal_xgpon_bwmp_tcont_set);
EXPORT_SYMBOL(aal_xgpon_bwmp_rsp_tm_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_rsp_tm_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_bst_mg_cfg_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_bst_mg_cfg_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_seq_tm_cfg_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_seq_tm_cfg_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_msg_ctl_cfg_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_msg_ctl_cfg_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_grt_ff_ctrl_cfg_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_bm_buf_cfg_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_bm_buf_cfg_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_eqd_sts_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_eqd_sts_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_random_dly_sts_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_lmt_drop_src_sts_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_lmt_drop_src_sts_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_bwmap_grant_ff_sts_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_xgtc_grt_ff_sts_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_bm_grant_ff_sts_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_unk_bf_cnt_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_as_cnt_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_as_hec_err_cnt_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_cnt_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_bwmp_cnt_ctrl_set );
EXPORT_SYMBOL(aal_xgpon_bwmp_ds_fm_sync_cnt_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_us_map_cnt_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_us_xgem_cnt_get );
EXPORT_SYMBOL(aal_xgpon_bwmp_us_xgtc_cnt_get );
EXPORT_SYMBOL(aal_xgpon_rxctl_rx_port_get );
EXPORT_SYMBOL(aal_xgpon_rxctl_rx_port_set );
EXPORT_SYMBOL(aal_xgpon_rxctl_rx_pkt_ctl_get );
EXPORT_SYMBOL(aal_xgpon_rxctl_rx_pkt_ctl_set );
EXPORT_SYMBOL(aal_xgpon_rxctl_ds_gem_drop_cnt_get );
EXPORT_SYMBOL(aal_xgpon_rxctl_ds_gem_drop_cnt_set );
EXPORT_SYMBOL(aal_xgpon_rxctl_rc1_ff_cnt_get );
EXPORT_SYMBOL(aal_xgpon_rxctl_rc2_ff_cnt_get );
EXPORT_SYMBOL(aal_xgpon_rxctl_usbg_dbg_sts_get );
EXPORT_SYMBOL(aal_xgpon_clk_seq_reset);
EXPORT_SYMBOL(aal_xgpon_tconts_get);
EXPORT_SYMBOL(aal_xgpon_xgems_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_xgems_cfg_get);
EXPORT_SYMBOL(aal_xgpon_tcont_flush);
EXPORT_SYMBOL(aal_xgpon_xgem_mibs_get);
EXPORT_SYMBOL(aal_xgpon_dsfp_stats_get);
EXPORT_SYMBOL(aal_xgpon_usbg_burst_prof_cfg_set );
EXPORT_SYMBOL(aal_xgpon_usbg_burst_prof_cfg_get );
EXPORT_SYMBOL(aal_gpon_dying_gasp_set );
EXPORT_SYMBOL(aal_gpon_dying_gasp_get );
EXPORT_SYMBOL(aal_xgpon_aes_plmp_subkey_ctrl_get);
EXPORT_SYMBOL(aal_xgpon_aes_plmp_subkey_ctrl_set);
EXPORT_SYMBOL(aal_xgpon_bwmp_tcont_clr);
EXPORT_SYMBOL(aal_xgpon_idle_xgem_frm_pli_set);
EXPORT_SYMBOL(aal_xgpon_idle_xgem_frm_pli_get);
EXPORT_SYMBOL(aal_xgpon_idle_xgem_frm_pli_stats_get);

EXPORT_SYMBOL(aal_xgpon_sniffer_buf_get);
EXPORT_SYMBOL(aal_xgpon_sniffer_glb_cfg_set);
EXPORT_SYMBOL(aal_xgpon_sniffer_glb_cfg_get);
EXPORT_SYMBOL(aal_xgpon_sniffer_glb_stats_get);
EXPORT_SYMBOL(aal_xgpon_software_reset_set);
EXPORT_SYMBOL(aal_xgpon_software_reset_get);

