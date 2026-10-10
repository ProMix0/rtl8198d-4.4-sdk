#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "ca_types.h"
#include "ploam_main.h"
#include "ploam_cmn.h"
#include "ploam_pkt.h"
#include "ploam_ngpon2.h"
#include "aal_ploam.h"
#include "xgpon_aes.h"
#include "xgpon.h"
#include "aal_xgpon.h"
#include "aal_xgpon_fec.h"
#include "util_aes.h"
#include "util_misc.h"
#include "aal_ngpon2_chmgmt.h"
#include "aal_puc.h"
#if defined(CONFIG_LUNA_G3_SERIES)
#include "pon.h"
#endif
//extern ca_ngpon2_channel_mgmt_t g_ngpon2_channel_mgmt_cb;
extern xgpon_basic_info_t g_xgpn_basic_info;
ca_uint32_t onu_ploam_seq_no = 0;
ca_uint32_t ploam_channel_method = 0;   //0: through EAXI;  1: through RGMII data path
ca_uint32_t g_burst_profile_config_flag = 0;
ca_uint16_t g_scheduled_sfc = 0;
ca_uint8_t  g_tun_ctr_seqno = 0;
ca_uint8_t  g_tun_rollback_flag = 0; /* 0 means no rollback when tun fail */
ca_uint32_t  g_ds_pon_id = 0; // target downstream channel
ca_uint32_t  g_us_pon_id = 0; // target upstream cahnnel
ca_uint16_t  g_alloc_id_scope_bmp = 0;
extern ca_status_t aal_ngpon2_ploam_calibration_req_handler(ca_uint8_t *source);
extern ca_status_t aal_ngpon2_ploam_tuning_ctrl_handler(ca_uint8_t *source);
extern ca_status_t aal_ngpon2_ploam_sys_profile_handler(ca_uint8_t *source);
extern ca_status_t aal_ngpon2_ploam_burst_profile_handler(ca_uint8_t *source);
extern ca_status_t aal_ngpon2_ploam_channel_profile_handler(ca_uint8_t *source);
extern ca_status_t aal_ngpon2_ploam_protection_ctrl_handler(void *data_p);
extern ca_status_t aal_ngpon2_ploam_assign_onuid_handler(ca_uint8_t *source);
extern ca_status_t aal_ngpon2_ploam_allocid_handler(ca_uint8_t *source);
extern ca_status_t aal_ngpon2_ploam_ranging_handler(ca_uint8_t *source);


#define CA__NGPON2_ERR_GOTO_FUNC_EXIT(status)            \
   {                                                              \
      if (CA_E_OK != (status)) {                                    \
         goto FUNC_EXIT;                                          \
      }                                                           \
   }

ca_boolean_t __pon_tag_is_changed(ca_uint8_t *g_pon_tag,ca_uint8_t *pon_tag){
    int loopcnt;
    for(loopcnt = 0; loopcnt< PON_TAG_SIZE; loopcnt++){
        if(g_pon_tag[loopcnt] != pon_tag[loopcnt]){
            return TRUE;
        }
    }

    return FALSE;
}

ploam_ngpon2_twdm_burst_profile_t ds_burst_profile_ploam = {
    0,        /*  burst_profile_ctrl;        5 */
    0,        /*  us_fec_ind;                6 */
    0,        /*  delimiter_len;             7 */
    {0},      /*  delimiter[8];              8-15 */
    0,        /*  preamble_len;              16 */
    0,        /*  preamble_repeat_cnt;       17 */
    {0},      /*  preamble_pattern[8];       18-25 */
    {0},      /*  pon_tag[8];                26-33 */
    {0},      /*  padding[7];                34-40 */
    {0},      /*  mic[8];                    41-48 */
};

ca_status_t ploam_ngpon2_proc_burst_profile(void *data_p)
{
    ca_status_t ret = CA_E_OK;
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_twdm_burst_profile_t *burst_profile = (ploam_ngpon2_twdm_burst_profile_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);
    aal_xgpon_fec_ctrl_mask_t  ctrl_mask;
    aal_xgpon_fec_ctrl_t      ctrl;

    ctrl_mask.u32_data = 0;
    memset(&ctrl,0,sizeof(aal_xgpon_fec_ctrl_t));

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("burst_profile_ctrl:%d, us_fec_ind:%d, delimiter_len:%d, preamble_len:%d, preamble_repeat_cnt:%d\r\n",
            burst_profile->burst_profile_ctrl, burst_profile->us_fec_ind, burst_profile->delimiter_len,
            burst_profile->preamble_len, burst_profile->preamble_repeat_cnt);

    ploam_dump_octet("delimiter", burst_profile->delimiter, sizeof(burst_profile->delimiter));
    ploam_dump_octet("preamble_pattern", burst_profile->preamble_pattern, sizeof(burst_profile->preamble_pattern));
    ploam_dump_octet("pon_tag", burst_profile->pon_tag, sizeof(burst_profile->pon_tag));


    // update aes key if necessary
    if(__pon_tag_is_changed(g_pon_tag, burst_profile->pon_tag)){
        memcpy(g_pon_tag,burst_profile->pon_tag,PON_TAG_SIZE);
        ret = xgpn_aes_key_generate(0);

        ca_printf("!EVENT: PON-TAG chanaged to be 0x%02x%02x%02x%02x%02x%02x%02x%02x!\r\n",
            burst_profile->pon_tag[0], burst_profile->pon_tag[1],
            burst_profile->pon_tag[2], burst_profile->pon_tag[3],
            burst_profile->pon_tag[4], burst_profile->pon_tag[5],
            burst_profile->pon_tag[6], burst_profile->pon_tag[7]);
        ploam_dump_octet("g_pon_tag", g_pon_tag, PON_TAG_SIZE);
        ploam_dump_octet("burst_profile->pon_tag", burst_profile->pon_tag, sizeof(burst_profile->pon_tag));

    }

    // config burst profile parameters to HW if necessary
    if (g_burst_profile_config_flag)
    {
        aal_xgpon_usbg_burst_prof_cfg_t  prof_cfg;
        aal_xgpon_usbg_burst_prof_cfg_msk_t mask;
        ca_uint8_t bp_idx = 0;
        ca_uint8_t bp_ver = 0;
        ca_uint8_t plen = 0;
        ca_uint8_t dlen = 0;
        ca_uint8_t pcnt = 0;
        ca_uint32_t pp_l = 0;
        ca_uint32_t pp_h = 0;
        ca_uint32_t dp_l = 0;
        ca_uint32_t dp_h = 0;
        ca_uint32_t pon_mode;
        bp_idx = burst_profile->burst_profile_ctrl &0x3;
        bp_ver = burst_profile->burst_profile_ctrl>>4;
        plen = burst_profile->delimiter_len;
        dlen = burst_profile->preamble_len;
        pcnt = burst_profile->preamble_repeat_cnt;
        memcpy(&pp_l,burst_profile->preamble_pattern,4);
        memcpy(&pp_h,burst_profile->preamble_pattern+4,4);
        memcpy(&dp_l,burst_profile->delimiter,4);
        memcpy(&dp_h,burst_profile->delimiter+4,4);

        memset(&prof_cfg, 0, sizeof(prof_cfg));
        ret = aal_xgpon_usbg_burst_prof_cfg_get(AAL_DEFAUL_DEV_ID, bp_idx, &prof_cfg);
        CA_RET_VALUE_CHECK(ret);

        mask.u32 = ~0;
        prof_cfg.pp_h = pp_h;
        prof_cfg.pp_l = pp_l;
        prof_cfg.dp_h = dp_h;
        prof_cfg.dp_l = dp_l;
        prof_cfg.pcnt= pcnt;
        prof_cfg.plen = plen;
        prof_cfg.dlen = dlen;
        prof_cfg.ver = bp_ver;
        prof_cfg.vld = 1;
        ret = aal_xgpon_usbg_burst_prof_cfg_set(AAL_DEFAUL_DEV_ID, bp_idx, mask, &prof_cfg);
        CA_RET_VALUE_CHECK(ret);
#if 1   // SW should not config global FEC on/off(unless FEC on/off config register is per burst profile defined), as SW does not know which burst profile to be used on-time
		// HW confirmed ASIC has covered FEC config automatically, 10/17/2016
		/*configure FEC on or off*/
		/*For 10G FEC (US & DS), HW update based on burst profile.
		Only for 2.5G FEC (XPON1 US), software is required to program XGPN_XGFEC_TXDISFEC for FEC on/off.
		*/
		pon_mode = aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID);

		if((pon_mode == ONU_PON_MAC_XGPON1)||(pon_mode == ONU_PON_MAC_GPON)){
			ctrl_mask.s.disableFEC = 1;
			ctrl.disableFEC = !burst_profile->us_fec_ind;
			ret = aal_xgpon_fec_ctrl_set(CA_XGPN_DEV_ID,ctrl_mask,&ctrl);
			CA_RET_VALUE_CHECK(ret);
		}
#endif
		aal_ngpon2_ploam_burst_profile_handler(data_p);
    }


    return ret;
}

ploam_ngpon2_assign_onuid_t ds_assign_onuid_ploam = {
    0,                      /*  assign_onu_id;           5-6 */
    PLOAM_VENDOR_ID,        /*  vendor_id;               7-10 */
    PLOAM_VSSN,             /*  vssn;                    11-14 */
    {0},                    /*  padding[26];             15-40 */
    {0},                    /*  mic[8];                  41-48 */
};

extern void
ploam_octet_dump( char *tp, unsigned char *dp, int len );

ca_status_t ploam_ngpon2_proc_assign_onuid(void *data_p)
{
    ca_status_t ret = CA_E_OK;
    int i = 0;
    aal_xgpn_plmp_cfg_t plmp_cfg;
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_assign_onuid_t *pkt = (ploam_ngpon2_assign_onuid_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);
    aal_xgpon_usbg_gem_config_msk_t usbg_xgem_cfg_msk;
    aal_xgpon_usbg_gem_config_t usbg_xgem_cfg;
    aal_xgpon_dsfp_xgem_cfg_msk_t dsfp_xgem_cfg_msk;
    aal_xgpon_dsfp_xgem_cfg_t dsfp_xgem_cfg;
	ploam_ngpon2_header_t ack_header;
	ploam_ngpon2_us_ack_t us_ack;
    aal_xgpon_bwmp_alloc_id_cfg_t bwmp_alloc_id;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("assign_onu_id:0x%x, vendor_id:0x%x, vssn:0x%x\r\n",
            pkt->assign_onu_id, pkt->vendor_id, pkt->vssn);

    memset(&plmp_cfg, 0,  sizeof(aal_xgpn_plmp_cfg_t));
	memset(&ack_header, 0,  sizeof(ploam_ngpon2_header_t));
	memset(&us_ack, 0,	sizeof(ploam_ngpon2_us_ack_t));

    usbg_xgem_cfg_msk.u32 = ~0U;
    memset(&usbg_xgem_cfg, 0, sizeof(aal_xgpon_usbg_gem_config_t));

    dsfp_xgem_cfg_msk.u32 = ~0U;
    memset(&dsfp_xgem_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_cfg_t));
	
    if ((g_xgpn_basic_info.vendor_id == ntohl(pkt->vendor_id)) &&
        (g_xgpn_basic_info.vssn == ntohl(pkt->vssn)))
    {
        g_xgpn_basic_info.onu_id = ntohs(pkt->assign_onu_id);

        memset(&bwmp_alloc_id, 0, sizeof(aal_xgpon_bwmp_alloc_id_cfg_t));
        ret = aal_xgpon_bwmp_tcont_get(0, 0, 0, &bwmp_alloc_id);
        CA_RET_VALUE_CHECK(ret);
        if(bwmp_alloc_id.alloc_id != g_xgpn_basic_info.onu_id){
            ca_printf("ERROR!! %s->%d:bwmp_alloc_id.alloc_id 0x%x and g_xgpn_basic_info.onu_id 0x%x\r\n",__func__,__LINE__,bwmp_alloc_id.alloc_id,g_xgpn_basic_info.onu_id);
        }
        
        usbg_xgem_cfg.aes_mode = 0;
        usbg_xgem_cfg.mc_flag = 0;
        usbg_xgem_cfg.port_en = 1;
        usbg_xgem_cfg.xgem_id = g_xgpn_basic_info.onu_id;

        dsfp_xgem_cfg.gem_id =  g_xgpn_basic_info.onu_id;
        dsfp_xgem_cfg.gem_mc = 0;
        dsfp_xgem_cfg.gem_vld = 1; /* should disable by default */
        for(i = 1 ;i< queue_num_per_tcont; i++){
            ret = aal_xgpon_usbg_xgem_cfg_set(0, i, usbg_xgem_cfg_msk, &usbg_xgem_cfg);
            CA_RET_VALUE_CHECK(ret);
            ret = aal_xgpon_dsfp_xgem_set(0, i, dsfp_xgem_cfg_msk, &dsfp_xgem_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
        if((g_xgpn_basic_info.onu_id > 0x3FC)&&(g_xgpn_basic_info.onu_id != 0x3FF))
        {
#if 0   // delete for ADTRAN XGSPON OLT, code should be moved to AAL layer
            PLOAM_DEBUG("Invalid onu_id, onu_id is 0x%x\r\n!! ",pkt->assign_onu_id);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 1);
            plmp_cfg.msg_type = PLOAM_NGPON2_DS_DEACT_ONUID;
            plmp_cfg.done = 0;
            ret = aal_ploam_cfg_set(0, &plmp_cfg);
            CA_RET_VALUE_CHECK(ret);
            plmp_cfg.done = 1;
            ret = aal_ploam_cfg_set(0, &plmp_cfg);
            CA_RET_VALUE_CHECK(ret);
            AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 0);
#endif
        }
		#if 0
		ack_header.onu_id = ntohs(g_xgpn_basic_info.onu_id);
		ack_header.msg_id = PLOAM_NGPON2_US_ACK;
		ack_header.seq_no = 0;
		us_ack.complete_code = AAL_PLOAM_ACK_NO_MSG_TO_SEND;
		us_ack.attenuation = 0;
		us_ack.power_level_cap = 0;
		memcpy(ack_content, &ack_header, sizeof(ploam_ngpon2_header_t));
		memcpy(ack_content + sizeof(ploam_ngpon2_header_t), &us_ack, 36);

		ret = ca_generate_ploam_mic(0,0x02,40,ack_content,8,ack_mic);
		CA_RET_VALUE_CHECK(ret);

		ret = aal_ploam_ack_ploam_no_msg_set(0,ack_mic);
		CA_RET_VALUE_CHECK(ret);
		if (aal_ploam_debug){
		//if (1){
			ploam_octet_dump("ack_content", ack_content, 40);
			ploam_octet_dump("ack_mic", ack_mic, XGPN_PLMP_MIC_SIZE);
		}
		#endif

		PLOAM_DEBUG("g_xgpn_basic_info.onu_id:0x%x, g_xgpn_basic_info.vendor_id:0x%x, g_xgpn_basic_info.vssn:0x%x\r\n",
				g_xgpn_basic_info.onu_id, g_xgpn_basic_info.vendor_id, g_xgpn_basic_info.vssn);
    }
	aal_ngpon2_ploam_assign_onuid_handler(data_p);
    return ret;
}

ploam_ngpon2_ranging_time_t ds_ranging_time = {
    0,                /*  control;                 5 */
    0,                /*  equ_delay;               6-9  */
    0,                /*  ds_pon_id;               10-13 */
    0,                /*  us_pon_id;               14-17 */
    {0},              /*  padding[23];             18-40 */
    {0},              /*  mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_ranging_time(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_ranging_time_t *pkt = (ploam_ngpon2_ranging_time_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("control:0x%x, equ_delay:0x%x, ds_pon_id:0x%x, us_pon_id:0x%x\r\n",
            pkt->control, pkt->equ_delay, pkt->ds_pon_id, pkt->us_pon_id);

    aal_ngpon2_ploam_ranging_handler(data_p);

    return CA_E_OK;
}

ploam_ngpon2_deact_onuid_t ds_deact_onu_id_ploam = {
    {0},                /*  padding[36];             5-40 */
    {0},                /*  mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_deact_onuid(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    //ploam_ngpon2_deact_onuid_t *pkt = (ploam_ngpon2_deact_onuid_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    g_xgpn_basic_info.onu_id = DEFAULT_XGPN_ONU_ID;

    return CA_E_OK;
}

ploam_ngpon2_disable_sn_t ds_disable_sn_ploam = {
    0,                    /* enable;                  5 */
    PLOAM_VENDOR_ID,      /* vendor_id;               6-9 */
    PLOAM_VSSN,           /* vssn;                    10-13 */
    {0},                  /* padding[27];             14-40 */
    {0},                  /* mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_disable_sn(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_disable_sn_t *pkt = (ploam_ngpon2_disable_sn_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("enable:0x%x, vendor_id:0x%x, vssn:0x%x\r\n",
            pkt->enable, pkt->vendor_id, pkt->vssn);

    if ((g_xgpn_basic_info.vendor_id == ntohl(pkt->vendor_id)) &&
        (g_xgpn_basic_info.vssn == ntohl(pkt->vssn)))
    {   // TODO: handler to be added further
        PLOAM_DEBUG("SN match for DISABLE_SN PLOAM.\r\n");

#if defined(CONFIG_LUNA_G3_SERIES)
        if (0 == pkt->enable) {
            PLOAM_DEBUG("The ONU with this SN is allowed upstream access.\r\n");
#if defined(CONFIG_LUNA_G3_SERIES)&&defined(CONFIG_BOARD_REALTEK_CA8277_PON_GPIO)
            ca_pon_laser_tx_disable_set(0,0);
#endif
        }
        else {
            PLOAM_DEBUG("The ONU with this SN is disallowed upstream access.\r\n");
#if defined(CONFIG_LUNA_G3_SERIES)&&defined(CONFIG_BOARD_REALTEK_CA8277_PON_GPIO)
            ca_pon_laser_tx_disable_set(0,1);
#endif
        }
#else
        if (0 == pkt->enable)
            PLOAM_DEBUG("The ONU with this SN is allowed upstream access.\r\n");
#endif
    }

    return CA_E_OK;
}

ploam_ngpon2_request_reg_t ds_request_reg_ploam = {
    {0},                    /*  padding[36];             5-40 */
    {0},                    /*  mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_request_reg(void *data_p)
{
    ca_status_t ret = CA_E_OK;
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    //ploam_ngpon2_request_reg_t *pkt = (ploam_ngpon2_request_reg_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);
    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

#if 0       // HW would handle it automatically
    aal_xgpn_plmp_rgst_id_cfg_t rgst_id;
    memset(&rgst_id, 0, sizeof(aal_xgpn_plmp_rgst_id_cfg_t));

    // get registration id from CB: memcpy(rgst_id.data,pkt->padding,XGPN_PLMP_RGST_ID_SIZE);
    // Send PLOAM message: ret = ;
    CA_RET_VALUE_CHECK(ret);
#endif
    return ret;
}

ploam_ngpon2_assign_allocid_t ds_assign_allocid_ploam = {
    0,                    /*  alloc_id_value;          5-6 */
    0,                    /*  alloc_id_type;           7 */
    0,                    /*  alloc_id_scope;          8-9 */
    {0},                  /*  padding[31];             10-40 */
    {0},                  /*  mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_assign_allocid(void *data_p)
{
    //ca_status_t ret = CA_E_OK;
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_assign_allocid_t *pkt = (ploam_ngpon2_assign_allocid_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);
    //aal_xgpn_plmp_cfg_t plmp_cfg;
    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("alloc_id_value:0x%x, alloc_id_type:0x%x, alloc_id_scope:0x%x\r\n",
            pkt->alloc_id_value, pkt->alloc_id_type, pkt->alloc_id_scope);

    g_alloc_id_scope_bmp = ca_ntohs(pkt->alloc_id_scope);
    //ca_printf("alloc_id_scope = 0x%04x.\r\n", g_alloc_id_scope_bmp);
    // TODO: Add alloc-ID into HW and SW mapping table
    aal_ngpon2_ploam_allocid_handler(data_p);
#if 0
    memset(&plmp_cfg, 0, sizeof(aal_xgpn_plmp_cfg_t));
    if(ntohs(pkt->alloc_id_value) < 0x400)
    {
#if 0   // delete for ADTRAN XGSPON OLT, code should be moved to AAL layer
        PLOAM_DEBUG("Invalid alloc id, alloc id is 0x%x\r\n!! ",ntohs(pkt->alloc_id_value));
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 1);
        plmp_cfg.msg_type = PLOAM_NGPON2_DS_ASSIGN_ALLOCID;
        plmp_cfg.done = 0;
        pkt->alloc_id_type = 0xFF;
        memcpy(&plmp_cfg.data[0], &pkt->alloc_id_value, XGPN_PLMP_MSG_BODY_SIZE);
        ret = aal_ploam_cfg_set(0, &plmp_cfg);
        CA_RET_VALUE_CHECK(ret);
        plmp_cfg.done = 1;
        ret = aal_ploam_cfg_set(0, &plmp_cfg);
        CA_RET_VALUE_CHECK(ret);
        AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_CTRL, sw_mode_en, 0);
#endif
    }
#endif
    return CA_E_OK;
}

ploam_ngpon2_key_ctrl_t ds_key_ctrl_ploam = {
    0,                    /* resv;                    5 */
    0,                    /* ctrl_flag;               6 */
    0,                    /* key_idx;                 7 */
    0,                    /* key_len;                 8 */
    {0},                    /* padding[32];             9-40 */
    {0},                    /* mic[8];                  41-48 */
};

ca_status_t ca_generate_kek_and_kek_name(ca_uint8_t key_idx)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t rand_key[XGPN_AES_KEY_LEN] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
                                                 0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
    aal_uc_enc_key_config_msk_t uc_enc_key_msk;
    aal_uc_enc_key_config_t uc_enc_key_cfg;

    ca_uint8_t encryped_key[XGPN_AES_KEY_LEN] = {0};
    ca_uint8_t kek_name[XGPN_AES_KEY_LEN] = {0};
    memset(&uc_enc_key_cfg,0,sizeof(uc_enc_key_cfg));

    /*generate new key*/
    random_key_generate(rand_key);

    uc_enc_key_msk.s.p_key_active = 1;
    uc_enc_key_msk.s.p_effective_key_length = 1;
    uc_enc_key_msk.s.uc_key16 = 1;

    uc_enc_key_cfg.p_key_active = 0;
    uc_enc_key_cfg.p_effective_key_length = XGPN_AES_KEY_LEN;
    memcpy(uc_enc_key_cfg.uc_key,rand_key,XGPN_AES_KEY_LEN);
    memcpy(g_uc_key,rand_key,XGPN_AES_KEY_LEN);//for debug
    /*program hw*/
    ret = aal_xgpon_aes_uc_enc_key_set(0,key_idx,uc_enc_key_msk,&uc_enc_key_cfg);
    CA_RET_VALUE_CHECK(ret);

    /*generate enrypted key*/
    ret = aes128_ecb_encrypt(kek,rand_key, encryped_key);
    CA_RET_VALUE_CHECK(ret);
    //memcpy(encryped_key_tmp,encryped_key,XGPN_AES_KEY_LEN);
    memcpy(g_encryped_key,encryped_key,XGPN_AES_KEY_LEN);

    #if 0
    for (i= 0 ;i<XGPN_AES_KEY_LEN/4; i++){
        encryped_key_tmp [i] = htonl(encryped_key_tmp [i]);
    }
    #endif
    /*generate kek name*/
    ret = ca_generate_kek_name(0, kek,rand_key,XGPN_AES_KEY_LEN,XGPN_AES_KEY_LEN,kek_name);
    CA_RET_VALUE_CHECK(ret);
    //memcpy(kek_name_tmp,kek_name,XGPN_AES_KEY_LEN);
    memcpy(g_kek_name,kek_name,XGPN_AES_KEY_LEN);
    #if 0
    for (i= 0 ;i<XGPN_AES_KEY_LEN/4; i++){
        kek_name_tmp [i] = htonl(kek_name_tmp [i]);
    }
    #endif
    return ret;
}
ca_status_t ploam_ngpon2_proc_key_ctrl(void *data_p)
{
    ca_status_t ret = CA_E_OK;
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_key_ctrl_t *pkt = (ploam_ngpon2_key_ctrl_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);
    //aal_key_report_frag_t key_rpt_frag;

    //ca_uint8_t rand_key[XGPN_AES_KEY_LEN] = {0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
    //aal_uc_enc_key_config_msk_t uc_enc_key_msk;
    aal_uc_enc_key_config_t uc_enc_key_cfg;

    //ca_uint8_t encryped_key[XGPN_AES_KEY_LEN] = {0};
    //ca_uint8_t kek_name[XGPN_AES_KEY_LEN] = {0};
    memset(&uc_enc_key_cfg,0,sizeof(uc_enc_key_cfg));
    //memset(&key_rpt_frag,0,sizeof(key_rpt_frag));

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("ctrl_flag:0x%x, key_idx:0x%x, key_len:0x%x\r\n",
            pkt->ctrl_flag, pkt->key_idx, pkt->key_len);
    if(pkt->ctrl_flag == 0){
        //g_key_ctl_new_cnt ++;
    }
    if(pkt->ctrl_flag == 1){
        //g_key_ctl_cfm_cnt ++;
    }
#if 0
    if(pkt->ctrl_flag == 0){
        g_key_ctl_new_cnt ++;
        memcpy(key_rpt_frag.data,encryped_key_tmp,XGPN_AES_KEY_LEN);
    }
    if(pkt->ctrl_flag == 1){
        g_key_ctl_cfm_cnt ++;
        memcpy(key_rpt_frag.data,kek_name_tmp,XGPN_AES_KEY_LEN);
    }
    ret = aal_ploam_key_report_frag_set(0,&key_rpt_frag);
    CA_RET_VALUE_CHECK(ret);
#endif
    //xgpon_ploam_key_exchange_handler(0, (ca_uint8_t *)data_p); // handle in interrupt module
    return ret;
}

ploam_ngpon2_sleep_allow_t ds_sleep_allow_ploam = {
    0,                     /* ctrl_flag;               5 */
    {0},                   /* padding[35];             6-40 */
    {0},                   /* mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_sleep_allow(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_sleep_allow_t *pkt = (ploam_ngpon2_sleep_allow_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("ctrl_flag:0x%x\r\n", pkt->ctrl_flag);

    return CA_E_OK;
}

ploam_ngpon2_calibration_req_t ds_calibration_req_ploam = {
    PLOAM_VENDOR_ID,                    /*  vendor_id;               5-8 */
    PLOAM_VSSN,                    /*  vssn;                    9-12 */
    0,                    /*  current_pon_id;          13-16 */
    0,                    /*  correlation_tag;         17-18 */
    0,                    /*  target_ds_pon_id;        19-22 */
    0,                    /*  target_us_pon_id;        23-26 */
    {0},                    /*  padding[4];              27-40 */
    {0},                    /*  mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_calibration_req(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_calibration_req_t *pkt = (ploam_ngpon2_calibration_req_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);
    #if 0
    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("vendor_id:0x%x, vssn:0x%x, current_pon_id:0x%x, correlation_tag:%d, target_ds_pon_id:%d, target_us_pon_id:%d\r\n",
            pkt->vendor_id, pkt->vssn, pkt->current_pon_id, pkt->correlation_tag, pkt->target_ds_pon_id, pkt->target_us_pon_id);
    #endif

    ca_printf("%s:%d::onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n", __FUNCTION__,__LINE__,
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    ca_printf("%s:%d::vendor_id:0x%x, vssn:0x%x, current_pon_id:0x%x, correlation_tag:%d, target_ds_pon_id:%d, target_us_pon_id:%d\r\n", __FUNCTION__,__LINE__,
            pkt->vendor_id, pkt->vssn, pkt->current_pon_id, pkt->correlation_tag, pkt->target_ds_pon_id, pkt->target_us_pon_id);

    aal_ngpon2_ploam_calibration_req_handler((ca_uint8_t *)&pkt->vendor_id);

    return CA_E_OK;
}
ploam_ngpon2_adjust_tx_wave_t ds_adjust_tx_wave_ploam = {
    PLOAM_VENDOR_ID,               /* vendor_id;               5-8 */
    PLOAM_VSSN,                    /* vssn;                    9-12 */
    0,                    /* current_pon_id;          13-16 */
    0,                    /* correlation_tag;         17-18 */
    0,                    /* freq_adj_direct;         19 */
    0,                    /* freq_adj_size;            20-21 */
    {0},                    /* padding[19];             22-40 */
    {0},                    /* mic[8];                  41-48 */
};
ca_status_t ploam_ngpon2_proc_adjust_tx_wave(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_adjust_tx_wave_t *pkt = (ploam_ngpon2_adjust_tx_wave_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("vendor_id:0x%x, vssn:0x%x, current_pon_id:0x%x\r\n",
            pkt->vendor_id, pkt->vssn, pkt->current_pon_id);
    PLOAM_DEBUG("correlation_tag:0x%x, freq_adj_direct:0x%x, freq_adj_size:0x%x\r\n",
            pkt->correlation_tag, pkt->freq_adj_direct, pkt->freq_adj_size);


    return CA_E_OK;
}

ploam_ngpon2_tuning_ctrl_t ds_tuning_ctrl_ploam = {
    0,                    /*  op_code;                 5 */
    0,                    /*  scheduled_sfc;           6-7 */
    0,                    /*  rollback_flag;           8 */
    0,                    /*  target_ds_pon_id;        9-12 */
    0,                    /*  target_us_pon_id;        12-16 */
    0,                    /*  calibration_flag;        17 */
    {0},                    /*  padding[23];             18-40 */
    {0},                    /*  mic[8];                  41-48 */
};

extern void aal_xgpn_act_fsm_restore_to_o5(ca_device_id_t device_id);

static ca_status_t __ngpon2_tuning_reject(
    ca_uint8_t   resp_code,
    ca_boolean_t restore_to_o5)
{
    ca_status_t ret = CA_E_OK;
    aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;


    memset(&tunning_sw_resp, 0, sizeof(tunning_sw_resp));
    tunning_sw_resp_mask.wrd = 0;

    tunning_sw_resp_mask.s.tuning_request        = 1;
    tunning_sw_resp_mask.s.tuning_operation_code = 1;
    tunning_sw_resp_mask.s.seqno                 = 1;
    tunning_sw_resp_mask.s.tuning_response_code  = 1;

    tunning_sw_resp.tuning_request        = 1;
    tunning_sw_resp.tuning_operation_code = __PLOAM_OP_NACK;
    tunning_sw_resp.seqno                 = g_tun_ctr_seqno;
    tunning_sw_resp.tuning_response_code  = resp_code;

    //need to go back to state O5.1
    if (restore_to_o5) {
        aal_xgpn_act_fsm_restore_to_o5(0);
    }

    ret = aal_ploam_tuning_sw_resp_set(0, tunning_sw_resp_mask, &tunning_sw_resp);
    NGPON2_TUN_DEBUG("send N-ACK success!\r\n");

    return ret;

}

static ca_status_t __ngpon2_tuning_accept(void)
{
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_ACK);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
    ca_printf("%s::%d:sending ACK success. seqno:0x%x\r\n", __FUNCTION__,__LINE__,g_tun_ctr_seqno);

    return CA_E_OK;

}

ca_status_t ploam_ngpon2_proc_tuning_ctrl(void *data_p)
{
    ca_status_t ret = CA_E_OK;
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_tuning_ctrl_t *pkt = (ploam_ngpon2_tuning_ctrl_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);
    aal_ploam_tuning_cfg_mask_t tuning_cfg_mask;
    aal_ploam_tuning_cfg_t      tuning_cfg;
    //aal_ploam_tuning_sw_resp_mask_t tunning_sw_resp_mask;
    aal_ploam_tuning_sw_resp_t tunning_sw_resp;
    ca_uint8_t   us_chn = 0, ds_chn = 0;
    ca_uint8_t us_support = 0, ds_support = 0;
    ca_uint8_t calibration_record = 0;
    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("op_code:0x%x, scheduled_sfc:0x%x, rollback_flag:0x%x\r\n",
            pkt->op_code, pkt->scheduled_sfc, pkt->rollback_flag);
    PLOAM_DEBUG("target_ds_pon_id:0x%x, target_us_pon_id:0x%x, calibration_flag:0x%x\r\n",
            pkt->target_ds_pon_id, pkt->target_us_pon_id, pkt->calibration_flag);

    ca_printf("%s::%d:onu_id:0x%x, msg_id:0x%x, seq_no:0x%x, rollback_flag:0x%x.\r\n", __FUNCTION__,__LINE__,
            head_p->onu_id, head_p->msg_id, head_p->seq_no, pkt->rollback_flag);
    ca_printf("%s::%d:target_ds_pon_id:0x%x, target_us_pon_id:0x%x.\r\n", __FUNCTION__,__LINE__,
            pkt->target_ds_pon_id, pkt->target_us_pon_id);
    ca_printf("%s::%d:calibration_flag:0x%x,scheduled_sfc is :0x%x. \r\n", __FUNCTION__,__LINE__,
            pkt->calibration_flag,pkt->scheduled_sfc);


#if (defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA))
    /*disable RGMII TX and RX*/
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_TXMAC_CFG, tx_en, 0);
    AAL_PON_REG_FIELD_WRITE(FPGA_RGMII_RXMAC_CFG, rx_en, 0);
#endif

    aal_puc_enable_voq(0,FALSE);

    head_p->onu_id = ntohs(head_p->onu_id);
    pkt->target_ds_pon_id = ntohl(pkt->target_ds_pon_id);
    pkt->target_us_pon_id = ntohl(pkt->target_us_pon_id);
    pkt->scheduled_sfc  = ntohs(pkt->scheduled_sfc);


    g_tun_ctr_seqno = head_p->seq_no ;
    g_ds_pon_id = pkt->target_ds_pon_id;
    g_us_pon_id = pkt->target_us_pon_id;
    g_scheduled_sfc = pkt->scheduled_sfc;
    g_tun_rollback_flag = pkt->rollback_flag;

    us_chn = pkt->target_us_pon_id & 0xF;
    ds_chn = pkt->target_us_pon_id & 0xF;

    ca_printf("%s::%d:target_ds_pon_id:0x%x, target_us_pon_id:0x%x.\r\n", __FUNCTION__,__LINE__,
            pkt->target_ds_pon_id, pkt->target_us_pon_id);
    ca_printf("%s::%d:target_ds_pon_id:0x%x, target_us_pon_id:0x%x,scheduled_sfc is :0x%x. \r\n", __FUNCTION__,__LINE__,
            g_ds_pon_id, g_us_pon_id, g_scheduled_sfc );

    aal_ngpon2_ploam_tuning_ctrl_handler(&pkt->op_code);

    #if 0
    g_ngpon2_channel_mgmt_cb.tuning_control.operation_code = pkt->op_code;
    g_ngpon2_channel_mgmt_cb.tuning_control.scheduled_sfc = pkt->scheduled_sfc;
    g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag = pkt->rollback_flag;
    g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id = pkt->target_ds_pon_id;
    g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id = pkt->target_us_pon_id;
    g_ngpon2_channel_mgmt_cb.tuning_control.calibration_flag = pkt->calibration_flag;

    ca_printf("%s::%d==============================================================\r\n",__FUNCTION__,__LINE__);
    ca_printf("operation_code   = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.operation_code   );
    ca_printf("scheduled_sfc    = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.scheduled_sfc    );
    ca_printf("rollback_flag    = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.rollback_flag    );
    ca_printf("target_ds_pon_id = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.target_ds_pon_id );
    ca_printf("target_us_pon_id = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.target_us_pon_id );
    ca_printf("calibration_flag = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.calibration_flag );
    ca_printf("tuning_ctrl_flag = 0x%x\r\n",    g_ngpon2_channel_mgmt_cb.tuning_control.tuning_ctrl_flag );
    ca_printf("===================================================================\r\n");

    #endif


    if(pkt->op_code) {

        ca_printf("channel switch completed\r\n");
        return CA_E_OK;
    }

    memset(&tuning_cfg,0,sizeof(tuning_cfg));
    memset(&tunning_sw_resp,0,sizeof(tunning_sw_resp));
    tuning_cfg_mask.wrd = ~0;
    tuning_cfg.request_schedule_sfc = pkt->scheduled_sfc;
    tuning_cfg.skip_rx = 0 ;
    tuning_cfg.rollback_allow = pkt->rollback_flag;
    tuning_cfg.request_lead_sfc = 2; // TODO: lead time is enough?

#if 1 // ldchen
    ret = aal_xgpon_xfp_cap2_get(0, us_chn,
        &us_support, &ds_support, &calibration_record);
    if (CA_E_OK != ret || !us_support) {
        NGPON2_TUN_DEBUG("The UP channel %d is not support!\r\n", us_chn);
        ret = __ngpon2_tuning_reject(__NACK_US_TUNR, TRUE);
        if (CA_E_OK != ret) {
            ca_printf("__ngpon2_tuning_reject return error %d.\r\n", ret);
        }
        return CA_E_OK;
    }

    ret = aal_xgpon_xfp_cap2_get(0, ds_chn,
        &us_support, &ds_support, &calibration_record);
    if (CA_E_OK != ret || !ds_support) {
        NGPON2_TUN_DEBUG("The DN channel %d is not support!\r\n", ds_chn);
        tuning_cfg.skip_rx = 1;
    }

    if ((g_aal_xgpon_actfsm_debug & XGPON_TUNING_DEBUG_SFC_DROP_SFC0)
        && pkt->scheduled_sfc  == 0){

        ret = __ngpon2_tuning_reject(TUN_RESP_CODE_INT_SFC, TRUE);
        if (CA_E_OK != ret) {
            ca_printf("__ngpon2_tuning_reject return error %d.\r\n", ret);
        }

        return CA_E_OK;
    }

    /* Accept tuning */
    ret = aal_ploam_tuning_cfg_set(0, tuning_cfg_mask, &tuning_cfg);
    CA_RET_VALUE_CHECK(ret);

    __ngpon2_tuning_accept();
#else
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_operation_code, __PLOAM_OP_ACK);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_response_code, 0);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, seqno, g_tun_ctr_seqno);
    AAL_PON_REG_FIELD_WRITE(XGPN_PLMP_SW_RESPONSE, tuning_request, 1);
    ca_printf("%s::%d:sending ACK success. seqno:0x%x\r\n", __FUNCTION__,__LINE__,g_tun_ctr_seqno);
#endif

    /*change to packet mode scheduling before switching*/
    AAL_PON_REG_FIELD_WRITE(PUC_BTCCFG, schmode, 1);
    #if 0
    tunning_sw_resp_mask.wrd = 0;
    tunning_sw_resp_mask.s.tuning_request = 1;
    tunning_sw_resp_mask.s.tuning_operation_code = 1;
    tunning_sw_resp_mask.s.seqno = 1;
    tunning_sw_resp_mask.s.tuning_response_code = 1;

    if(( pkt->target_ds_pon_id & 0xFF) < 0xF){

        tunning_sw_resp.tuning_operation_code = __PLOAM_OP_ACK;
        tunning_sw_resp.tuning_response_code = 0;
        tunning_sw_resp.seqno = g_tun_ctr_seqno;
        tunning_sw_resp.tuning_request = 1;
        ca_printf(":%s:%d::send ACK success!!\r\n",__FUNCTION__,__LINE__);
    }else{
        tunning_sw_resp.tuning_request = 1;
        tunning_sw_resp.tuning_operation_code = __PLOAM_OP_NACK;
        tunning_sw_resp.seqno = g_tun_ctr_seqno;
        tunning_sw_resp.tuning_response_code = __NACK_DS_ALBL;
        ca_printf(":%s:%d::send nACK success!!\r\n",__FUNCTION__,__LINE__);
    }

    ret = aal_ploam_tuning_sw_resp_set(0,tunning_sw_resp_mask,&tunning_sw_resp);
    CA_RET_VALUE_CHECK(ret);
    #endif

    return CA_E_OK;
}

ploam_ngpon2_sys_profile_t ds_sys_profile_ploam = {
    {0},                    /* ng2sys_id;               5-7 */
    0,                    /* sys_profile_ver;         8 */
    0,                    /* us_op_wave_len_bands;    9 */
    0,                    /* twdm_channel_cnt;        10 */
    0,                    /* channel_spacing;         11 */
    0,                    /* us_mse;                  12 */
    0,                    /* fsr;                     13-14 */
    0,                    /* amcc_ctrl;               15 */
    0,                    /* loose_calibration;       16 */
    0,                    /* channel_cnt;             17-18 */
    0,                    /* channel_spacing_ptp;     19 */
    0,                    /* us_mse_ptp;              20 */
    0,                    /* fsr_ptp;                 21-22 */
    0,                    /* calibration_accuracy;    23 */
    0,                    /* loose_calibration_bound; 24 */
    {0},                    /* pading[16];              25-40 */
    {0},                    /* mic[8];                  41-48 */
};


ca_status_t ploam_ngpon2_proc_sys_profile(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_sys_profile_t *pkt = (ploam_ngpon2_sys_profile_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("ng2sys_id:0x%08x - 0x%08x - 0x%08x, sys_profile_ver:0x%x, us_op_wave_len_bands:0x%x\r\n",
            pkt->ng2sys_id[0], pkt->ng2sys_id[1], pkt->ng2sys_id[2], pkt->sys_profile_ver, pkt->us_op_wave_len_bands);
    PLOAM_DEBUG("twdm_channel_cnt:0x%x, channel_spacing:0x%x, us_mse:0x%x\r\n",
            pkt->twdm_channel_cnt, pkt->channel_spacing, pkt->us_mse);
    PLOAM_DEBUG("fsr:0x%x, amcc_ctrl:0x%x, loose_calibration:0x%x\r\n",
            pkt->fsr, pkt->amcc_ctrl, pkt->loose_calibration);
    PLOAM_DEBUG("channel_cnt:0x%x, channel_spacing_ptp:0x%x, us_mse_ptp:0x%x\r\n",
            pkt->channel_cnt, pkt->channel_spacing_ptp, pkt->us_mse_ptp);
    PLOAM_DEBUG("fsr_ptp:0x%x, calibration_accuracy:0x%x, loose_calibration_bound:0x%x\r\n",
            pkt->fsr_ptp, pkt->calibration_accuracy, pkt->loose_calibration_bound);

    aal_ngpon2_ploam_sys_profile_handler((void *)pkt);

    return CA_E_OK;
}

ploam_ngpon2_channel_profile_t ds_channel_profile_ploam = {
    0,                    /*  control;                 5 */
    0,                    /*  version;                 6 */
    0,                    /*  pon_id;                  7-10 */
    0,                    /*  ds_freq_offset;          11 */
    0,                    /*  ds_rate;                 12 */
    0,                    /*  channel_partition;       13 */
    0,                    /*  def_resp_channel;        14-17 */
    0,                    /*  sn_grant_type_ind;       18 */
    0,                    /*  amcc_window;             19-20 */
    0,                    /*  uwlch_id;                21 */
    0,                    /*  us_freq;                 22-25 */
    0,                    /*  opt_link_type;           26 */
    0,                    /*  us_rate;                 27 */
    0,                    /*  def_onu_attenuation;     28 */
    0,                    /*  resp_threshold;          29 */
    {0},                    /*  padding[11];             30-40 */
    {0},                    /*  mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_channel_profile(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_channel_profile_t *pkt = (ploam_ngpon2_channel_profile_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("control:0x%x, version:0x%x, pon_id:0x%x\r\n",
            pkt->control, pkt->version, pkt->pon_id);
    PLOAM_DEBUG("ds_freq_offset:0x%x, ds_rate:0x%x, channel_partition:0x%x\r\n",
            pkt->ds_freq_offset, pkt->ds_rate, pkt->channel_partition);
    PLOAM_DEBUG("def_resp_channel:0x%x, sn_grant_type_ind:0x%x, amcc_window:0x%x\r\n",
            pkt->def_resp_channel, pkt->sn_grant_type_ind, pkt->amcc_window);
    PLOAM_DEBUG("uwlch_id:0x%x, us_freq:0x%x, opt_link_type:0x%x\r\n",
            pkt->uwlch_id, pkt->us_freq, pkt->opt_link_type);
    PLOAM_DEBUG("us_rate:0x%x, def_onu_attenuation:0x%x, resp_threshold:0x%x\r\n",
            pkt->us_rate, pkt->def_onu_attenuation, pkt->resp_threshold);

    aal_ngpon2_ploam_channel_profile_handler((ca_uint8_t*)&pkt);

    return CA_E_OK;
}


ploam_ngpon2_protection_ctrl_t ds_protection_crtl_ploam = {
    0,                    /*  protect_ds_pon_id;       5-8 */
    0,
    0,/*  protect_us_pon_id;       9-12 */
    {0},                    /*  padding[28];             13-40 */
    {0}                    /*  mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_protection_ctrl(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_protection_ctrl_t *pkt = (ploam_ngpon2_protection_ctrl_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);
    ca_status_t rc;
    ca_uint8_t msg[PLOAM_NGPON2_MSG_SIZE] = {0};
    ploam_ngpon2_header_t *header_p_send;
    ploam_ngpon2_us_ack_t *us_ack;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("protect_ds_pon_id:0x%x, protect_us_pon_id:0x%x\r\n",
            pkt->protect_ds_pon_id, pkt->protect_us_pon_id);

    rc = aal_ngpon2_ploam_protection_ctrl_handler(data_p);

     /* fill common header */
    header_p_send = (ploam_ngpon2_header_t *)msg;
    header_p_send->onu_id = head_p->onu_id;
    header_p_send->seq_no = head_p->seq_no;
    header_p_send->msg_id = PLOAM_NGPON2_US_ACK;

    us_ack = (ploam_ngpon2_us_ack_t *)&(msg[PLOAM_NGPON2_HEADER_SIZE]);

    if (CA_E_OK == rc)  // Send ACK
    {
        us_ack->complete_code = AAL_PLOAM_ACK_COMPLETE_OK;
        us_ack->attenuation = 0;            // TODO: value to be updated
        us_ack->power_level_cap = FALSE;    // TODO: value to be updated
    }
    else    // Send NACK
    {
        us_ack->complete_code = AAL_PLOAM_ACK_PARAMETER_ERROR;
    }

    rc = ploam_ngpon2_send_ploam(msg, PLOAM_NGPON2_MSG_SIZE, FALSE);

    return rc;
}

ploam_ngpon2_change_power_t ds_change_power_ploam = {
    0,                    /* op_type;                 5 */
    0,                    /* attenuation;             6 */
    {0},                    /* padding[34];             7-40 */
    {0},                    /* mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_change_power(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_change_power_t *pkt = (ploam_ngpon2_change_power_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("op_type:0x%x, attenuation:0x%x\r\n",
            pkt->op_type, pkt->attenuation);

    return CA_E_OK;
}

ploam_ngpon2_power_consume_t ds_power_consume_ploam = {
    {0},                    /*  padding[36];             5-40 */
    {0},                    /*  mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_power_consume(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    //ploam_ngpon2_power_consume_t *pkt = (ploam_ngpon2_power_consume_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    return CA_E_OK;
}

ploam_ngpon2_rate_ctrl_t ds_rate_ctrl_ploam = {
    0,                    /* op_code;                 5 */
    0,                    /* scheduled_sfc;           6-7 */
    0,                    /* rollback_flag;           8 */
    0,                    /* norinal_line_rate;       9 */
    {0},                    /* padding[31];             10-40 */
    {0},                    /* mic[8];                  41-48 */
};


ca_status_t ploam_ngpon2_proc_rate_ctrl(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;
    ploam_ngpon2_rate_ctrl_t *pkt = (ploam_ngpon2_rate_ctrl_t *)(data_p + PLOAM_NGPON2_HEADER_SIZE);

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    PLOAM_DEBUG("op_code:0x%x, scheduled_sfc:0x%x, rollback_flag:0x%x, norinal_line_rate:%d\r\n",
            pkt->op_code, pkt->scheduled_sfc, pkt->rollback_flag, pkt->norinal_line_rate);

    return CA_E_OK;
}

static ploam_msg_proc_t  ploam_ngpon2_msg_table[] =
{
    {"BURST_PROFILE",   PLOAM_NGPON2_DS_BURST_PROFILE,              ploam_ngpon2_proc_burst_profile,    &ds_burst_profile_ploam},
    {"ASSIGN_ONUID",    PLOAM_NGPON2_DS_ASSIGN_ONUID,               ploam_ngpon2_proc_assign_onuid,     &ds_assign_onuid_ploam},
    {"RANGING_TIME",    PLOAM_NGPON2_DS_RANGING_TIME,               ploam_ngpon2_proc_ranging_time,     &ds_ranging_time},
    {"DEACT_ONUID",     PLOAM_NGPON2_DS_DEACT_ONUID,                ploam_ngpon2_proc_deact_onuid,      &ds_deact_onu_id_ploam},
    {"DISABLE_SN",      PLOAM_NGPON2_DS_DISABLE_SN,                 ploam_ngpon2_proc_disable_sn,       &ds_disable_sn_ploam},
    {"REQUEST_REG",     PLOAM_NGPON2_DS_REQUEST_REGISTRATION,       ploam_ngpon2_proc_request_reg,      &ds_request_reg_ploam},
    {"ASSIGN_ALLOCID",  PLOAM_NGPON2_DS_ASSIGN_ALLOCID,             ploam_ngpon2_proc_assign_allocid,   &ds_assign_allocid_ploam},
    {"KEY_CTRL",        PLOAM_NGPON2_DS_KEY_CTRL,                   ploam_ngpon2_proc_key_ctrl,         &ds_key_ctrl_ploam},
    {"SLEEP_ALLOW",     PLOAM_NGPON2_DS_SLEEP_ALLOW,                ploam_ngpon2_proc_sleep_allow,      &ds_sleep_allow_ploam},
    {"CALIBRATION_REQ", PLOAM_NGPON2_DS_CALIBRATION_REQ,            ploam_ngpon2_proc_calibration_req,  &ds_calibration_req_ploam},
    {"ADJUST_TX_WAVE",  PLOAM_NGPON2_DS_ADJUST_TX_WAVELEN,          ploam_ngpon2_proc_adjust_tx_wave,   &ds_adjust_tx_wave_ploam},
    {"TUNING_CTRL",     PLOAM_NGPON2_DS_TUNING_CTRL,                ploam_ngpon2_proc_tuning_ctrl,      &ds_tuning_ctrl_ploam},
    {"SYS_PROFILE",     PLOAM_NGPON2_DS_SYS_PROFILE,                ploam_ngpon2_proc_sys_profile,      &ds_sys_profile_ploam},
    {"CHANNEL_PROFILE", PLOAM_NGPON2_DS_CHANNEL_PROFILE,            ploam_ngpon2_proc_channel_profile,  &ds_channel_profile_ploam},
    {"PROTECTION_CTRL", PLOAM_NGPON2_DS_PROTECTION_CTRL,            ploam_ngpon2_proc_protection_ctrl,  &ds_protection_crtl_ploam},
    {"CHANGE_POWER",    PLOAM_NGPON2_DS_CHANGE_POWER_LEVEL,         ploam_ngpon2_proc_change_power,     &ds_change_power_ploam},
    {"POWER_CONSUME",   PLOAM_NGPON2_DS_POWER_CONSUMPTION_INQUIRE,  ploam_ngpon2_proc_power_consume,    &ds_power_consume_ploam},
    {"RATE_CTRL",       PLOAM_NGPON2_DS_RATE_CTRL,                  ploam_ngpon2_proc_rate_ctrl,        &ds_rate_ctrl_ploam},
};

#define NUMOF_PLOAM_NGPON2_MSG_ENT  (sizeof(ploam_ngpon2_msg_table)/sizeof(ploam_msg_proc_t))


ca_status_t ploam_pkt_ngpon2_proc(ca_uint8 *frame_ptr, ca_uint32_t len)
{
    ploam_pkt_ngpon2_t *pkt_p;
    ploam_msg_proc_t *entry_p = &ploam_ngpon2_msg_table[0];
    ca_int32_t i;
    ca_uint32_t ret = CA_E_ERROR;

    PLOAM_ASSERT_RET((frame_ptr != NULL) && (len >= PLOAM_NGPON2_PKT_SIZE), CA_E_PARAM);

    ploam_dump_pkt("ngpon2 ploam", frame_ptr, len);

    if (ploam_channel_method)   /* offset da/sa/eth-type 14 bytes */
        pkt_p = (ploam_pkt_ngpon2_t *)(frame_ptr + PLOAM_OVERHEAD_SIZE);
    else
        pkt_p = (ploam_pkt_ngpon2_t *)(frame_ptr);

    for (i = 0; i < NUMOF_PLOAM_NGPON2_MSG_ENT; i++, entry_p++){
        if (pkt_p->msg_id == entry_p->msg_id){
            PLOAM_DEBUG("msg type:%s \r\n", entry_p->name);
            if (NULL != entry_p->func) {
                ret = entry_p->func(pkt_p);
            }

            if (NULL != entry_p->value_p) {
                /* copy ploam data, not include MIC */
                memcpy(entry_p->value_p, pkt_p->data, XGPN_PLMP_MSG_BODY_SIZE);
            }

            break;
        }
    }

    return ret;
}


ca_status_t aal_ploam_ds_handler_reg(
    ca_uint8_t      msg_id,
    ploam_handler_t pfunc)
{
    ploam_msg_proc_t *entry_p = &ploam_ngpon2_msg_table[0];
    ca_uint32_t i = 0;

    CA_NULL_PTR_CHECK(pfunc);
    CA_ASSERT_RET(msg_id < PLOAM_NGPON2_DS_MAX, CA_E_PARAM);

    for (i = 0; i < NUMOF_PLOAM_NGPON2_MSG_ENT; i++, entry_p++){
        if (msg_id == entry_p->msg_id){
            PLOAM_DEBUG("msg type:%s \r\n", entry_p->name);
            entry_p->func = pfunc;
            return CA_E_OK;
        }
    }

    return CA_E_ERROR;
}

ca_status_t aal_ploam_ds_handler_unreg(
    ca_uint8_t      msg_id,
    ploam_handler_t pfunc)
{
    ploam_msg_proc_t *entry_p = &ploam_ngpon2_msg_table[0];
    ca_uint32_t i = 0;

    CA_NULL_PTR_CHECK(pfunc);
    CA_ASSERT_RET(msg_id < PLOAM_NGPON2_DS_MAX, CA_E_PARAM);

    for (i = 0; i < NUMOF_PLOAM_NGPON2_MSG_ENT; i++, entry_p++){
        if (msg_id == entry_p->msg_id && entry_p->func != NULL){
            PLOAM_DEBUG("msg type:%s \r\n", entry_p->name);
            entry_p->func = NULL;
            return CA_E_OK;
        }
    }

    return CA_E_OK;
}


ca_status_t ploam_ngpon2_ds_ploam_def_val_get(
    ca_uint8_t msg_id,
    void *data)
{
    ploam_msg_proc_t *entry_p = &ploam_ngpon2_msg_table[0];
    ca_int32_t i;
    ca_uint32_t ret = CA_E_ERROR;

    PLOAM_ASSERT_RET(data, CA_E_PARAM);

    for (i = 0; i < NUMOF_PLOAM_NGPON2_MSG_ENT; i++, entry_p++){
        if (msg_id == entry_p->msg_id){
            PLOAM_DEBUG("msg type:%s \r\n", entry_p->name);
            memcpy(data, entry_p->value_p, XGPN_PLMP_MSG_BODY_SIZE);
            ret = CA_E_OK;
            break;
        }
    }

    //ploam_dump_pkt("ds ploam", data, PLOAM_NGPON2_MSG_SIZE);

    return ret;
}

ca_status_t ploam_ngpon2_ds_ploam_def_val_set(
    ca_uint8_t msg_id,
    void *data)
{
    ploam_msg_proc_t *entry_p = &ploam_ngpon2_msg_table[0];
    ca_int32_t i;
    ca_uint32_t ret = CA_E_ERROR;

    PLOAM_ASSERT_RET(data, CA_E_PARAM);

    for (i = 0; i < NUMOF_PLOAM_NGPON2_MSG_ENT; i++, entry_p++){
        if (msg_id == entry_p->msg_id){
            PLOAM_DEBUG("msg type:%s \r\n", entry_p->name);
            memcpy(entry_p->value_p, data, XGPN_PLMP_MSG_BODY_SIZE);
            ret = CA_E_OK;
            break;
        }
    }

    //ploam_dump_pkt("ds ploam", data, PLOAM_NGPON2_MSG_SIZE);
    return ret;
}



ploam_ngpon2_us_serial_number_t us_sn_ploam = {
    PLOAM_VENDOR_ID,          /* ca_uint32_t     vendor_id;               5-8 */
    PLOAM_VSSN,               /* ca_uint32_t     vssn;                    9-12 */
    0x11112222,               /* ca_uint32_t     random_delay;            13-16 twdm, ptp wdm reserved */
    0xa,                      /* ca_uint16_t     correlation_tag;         17-18 */
    0xb,                      /* ca_uint32_t     current_ds_pon_id;       19-22 */
    1,                        /* ca_uint32_t     current_us_pon_id;       23-26 */
    {1},                      /* ca_uint8_t      calibration_rec_status[8];  27-34 twdm, ptp wdm named sn_digest */
    2,                        /* ca_uint8_t      tuning_granularity;      35 */
    2,                        /* ca_uint8_t      step_tuning_time;        36 */
    3,                        /* ca_uint8_t      us_line_rate_cap;        37 */
    3,                        /* ca_uint8_t      attenuation;             38 */
    10,                       /* ca_uint8_t      power_level_cap;         39 */
    0,                        /* ca_uint8_t      padding;                 40 */
    {0}                       /* ca_uint8_t      mic[8];                  41-48 */
};


ca_status_t ploam_ngpon2_proc_us_sn(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;


    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    memcpy(data_p + PLOAM_NGPON2_HEADER_SIZE, &us_sn_ploam, sizeof(ploam_ngpon2_us_serial_number_t));

    return CA_E_OK;
}

ploam_ngpon2_us_registration_t us_reg_ploam = {
    {0x11, 0x12},    /* ca_uint8_t      registration_id[36];     5-40 */
    {0}              /* ca_uint8_t      mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_us_registration(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    memcpy(data_p + PLOAM_NGPON2_HEADER_SIZE, &us_reg_ploam, sizeof(ploam_ngpon2_us_registration_t));

    return CA_E_OK;
}

ploam_ngpon2_us_key_report_t us_key_report_ploam = {
    10,          /* ca_uint8_t      report_type;             5 */
    11,          /* ca_uint8_t      key_idx;                 6 */
    12,          /* ca_uint8_t      fragment_no;             7 */
    0,           /* ca_uint8_t      resv;                    8 */
    {0x1, 0x2},  /* ca_uint8_t      key_fragment[32];        9-40 */
    {0}          /* ca_uint8_t      mic[8];                  41-48 */
};

ca_status_t ploam_ngpon2_proc_us_key_report(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    memcpy(data_p + PLOAM_NGPON2_HEADER_SIZE, &us_key_report_ploam, sizeof(ploam_ngpon2_us_key_report_t));

    return CA_E_OK;
}

ploam_ngpon2_us_ack_t us_ack_ploam = {
    0,          /* ca_uint8_t      complete_code;          5 */
    10,         /* ca_uint8_t      attenuation;            6 */
    20,         /* ca_uint8_t      power_level_cap;        7 */
    {0},        /* ca_uint8_t      padding[33];            8-40 */
    {0}         /* ca_uint8_t      mic[8];                 41-48 */
};

ca_status_t ploam_ngpon2_proc_us_ack(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    memcpy(data_p + PLOAM_NGPON2_HEADER_SIZE, &us_ack_ploam, sizeof(ploam_ngpon2_us_ack_t));

    return CA_E_OK;
}

ploam_ngpon2_us_sleep_req_t us_sleep_req_ploam = {
    0,       /* ca_uint8_t      activity_level;         5 */
    {0},     /* ca_uint8_t      padding[35];            6-40 */
    {0},     /* ca_uint8_t      mic[8];                 41-48 */
};

ca_status_t ploam_ngpon2_proc_us_sleep_req(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    memcpy(data_p + PLOAM_NGPON2_HEADER_SIZE, &us_sleep_req_ploam, sizeof(ploam_ngpon2_us_sleep_req_t));

    return CA_E_OK;
}

ploam_ngpon2_us_tuning_resp_t us_tuning_ploam = {
    0,      /* ca_uint8_t      op_code;                5 */
    1,      /* ca_uint16_t     resp_code;              6-7 */
    PLOAM_VENDOR_ID,       /* ca_uint32_t     vendor_id;              7-10 */
    PLOAM_VSSN,       /* ca_uint32_t     vssn;                   11-14 */
    10,     /* ca_uint16_t     correlation_tag;        15-16 */
    1,      /* ca_uint32_t     pon_id;                 17-20 */
    2,      /* ca_uint8_t      uwlch_id;               21 */
    4,      /* ca_uint8_t      calibration_rec_status; 22-29 sn digest for ptp wdm */
    5,      /* ca_uint8_t      tuning_granularity;     30 */
    6,      /* ca_uint8_t      one_step_tuning_time;   31 */
    7,      /* ca_uint8_t      us_line_rate_cap;       32 */
    8,      /* ca_uint8_t      attenuation;            33 */
    9,      /* ca_uint8_t      power_level_cap;        34 */
    {0},    /* ca_uint8_t      padding[6];             25-40 */
    {0},    /* ca_uint8_t      mic[8];                 41-48 */
};


ca_status_t ploam_ngpon2_proc_us_tuning_resp(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    memcpy(data_p + PLOAM_NGPON2_HEADER_SIZE, &us_tuning_ploam, sizeof(ploam_ngpon2_us_tuning_resp_t));

    return CA_E_OK;
}

ploam_ngpon2_us_power_consumption_report_t us_power_consum_ploam = {
    {0xa, 0xb},     /* ca_uint8_t      power_consumption[24];  5-28 */
    {0},            /* ca_uint8_t      padding[12];            29-40 */
    {0},            /* ca_uint8_t      mic[8];                 41-48 */
};

ca_status_t ploam_ngpon2_proc_us_power_consume_report(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    memcpy(data_p + PLOAM_NGPON2_HEADER_SIZE, &us_power_consum_ploam, sizeof(ploam_ngpon2_us_power_consumption_report_t));

    return CA_E_OK;
}

ploam_ngpon2_us_rate_resp_t us_rate_resp_ploam = {
    0,          /* ca_uint8_t      op_code;                5 */
    0,          /* ca_uint8_t      resp_code;              6 */
    {0},        /* ca_uint8_t      padding[34];            34 */
    {0},        /* ca_uint8_t      mic[8];                 41-48 */
};

ca_status_t ploam_ngpon2_proc_us_rate_resp(void *data_p)
{
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)data_p;

    PLOAM_ASSERT_RET(data_p, CA_E_PARAM);

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            head_p->onu_id, head_p->msg_id, head_p->seq_no);

    memcpy(data_p + PLOAM_NGPON2_HEADER_SIZE, &us_rate_resp_ploam, sizeof(ploam_ngpon2_us_rate_resp_t));

    return CA_E_OK;
}


static ploam_msg_proc_t  ploam_ngpon2_us_msg_table[] =
{
    {"US_SERIAL_NUM",      PLOAM_NGPON2_US_SERIAL_NUMBER,              ploam_ngpon2_proc_us_sn,             &us_sn_ploam},
    {"US_REG",             PLOAM_NGPON2_US_REGISTRATION,               ploam_ngpon2_proc_us_registration,   &us_reg_ploam},
    {"US_KEY_REPORT",      PLOAM_NGPON2_US_KEY_REPORT,                 ploam_ngpon2_proc_us_key_report,     &us_key_report_ploam},
    {"US_ACK",             PLOAM_NGPON2_US_ACK,                        ploam_ngpon2_proc_us_ack,            &us_ack_ploam},
    {"US_SLEEP_REQ",       PLOAM_NGPON2_US_SLEEP_REQ,                  ploam_ngpon2_proc_us_sleep_req,      &us_sleep_req_ploam},
    {"US_TUNING_RESP",     PLOAM_NGPON2_US_TUNING_RESP,                ploam_ngpon2_proc_us_tuning_resp,    &us_tuning_ploam},
    {"US_POWER_REPORT",    PLOAM_NGPON2_US_POWER_CONSUMPTION_REPORT,   ploam_ngpon2_proc_us_power_consume_report, &us_power_consum_ploam},
    {"US_RATE_RESP",       PLOAM_NGPON2_US_RATE_RESP,                  ploam_ngpon2_proc_us_rate_resp,      &us_rate_resp_ploam}
};

#define NUMOF_PLOAM_NGPON2_US_MSG_ENT  (sizeof(ploam_ngpon2_us_msg_table)/sizeof(ploam_msg_proc_t))


ca_status_t ploam_ngpon2_us_msg_proc(
    void *msg)
{
    ploam_msg_proc_t *entry_p = &ploam_ngpon2_us_msg_table[0];
    ploam_ngpon2_header_t *head_p = (ploam_ngpon2_header_t *)msg;
    ca_int32_t i;
    ca_uint8_t msg_id;
    ca_uint32_t ret = CA_E_ERROR;

    PLOAM_ASSERT_RET(msg, CA_E_PARAM);

    ploam_dump_pkt("us ploam", msg, PLOAM_NGPON2_MSG_SIZE);


    msg_id = head_p->msg_id;

    for (i = 0; i < NUMOF_PLOAM_NGPON2_US_MSG_ENT; i++, entry_p++){
        if (msg_id == entry_p->msg_id){
            PLOAM_DEBUG("msg type:%s \r\n", entry_p->name);
            ret = entry_p->func(msg);
            break;
        }
    }

    return ret;
}


ca_status_t ploam_ngpon2_us_ploam_def_val_get(
    ca_uint8_t msg_id,
    void *data)
{
    ploam_msg_proc_t *entry_p = &ploam_ngpon2_us_msg_table[0];
    ca_int32_t i;
    ca_uint32_t ret = CA_E_ERROR;

    PLOAM_ASSERT_RET(data, CA_E_PARAM);

    for (i = 0; i < NUMOF_PLOAM_NGPON2_US_MSG_ENT; i++, entry_p++){
        if (msg_id == entry_p->msg_id){
            PLOAM_DEBUG("msg type:%s \r\n", entry_p->name);
            memcpy(data, entry_p->value_p, XGPN_PLMP_MSG_BODY_SIZE);
            ret = CA_E_OK;
            break;
        }
    }

    //ploam_dump_pkt("us ploam", data, PLOAM_NGPON2_MSG_SIZE);

    return ret;
}

ca_status_t ploam_ngpon2_us_ploam_def_val_set(
    ca_uint8_t msg_id,
    void *data)
{
    ploam_msg_proc_t *entry_p = &ploam_ngpon2_us_msg_table[0];
    ca_int32_t i;
    ca_uint32_t ret = CA_E_ERROR;

    PLOAM_ASSERT_RET(data, CA_E_PARAM);

    for (i = 0; i < NUMOF_PLOAM_NGPON2_US_MSG_ENT; i++, entry_p++){
        if (msg_id == entry_p->msg_id){
            PLOAM_DEBUG("msg type:%s \r\n", entry_p->name);
            memcpy(entry_p->value_p, data, XGPN_PLMP_MSG_BODY_SIZE);
            ret = CA_E_OK;
            break;
        }
    }

    //ploam_dump_pkt("us ploam", data, PLOAM_NGPON2_MSG_SIZE);
    return ret;
}

ca_status_t ploam_ngpon2_build_ploam(
    ca_uint16_t onu_id,
    ca_uint8_t  msg_id,
    ca_uint8_t  seq_no,
    ca_uint8    ploam_buffer[]
)
{
    ca_status_t ret = CA_E_OK;
    ploam_ngpon2_header_t *header_p;

    PLOAM_DEBUG("onu_id:0x%x, msg_id:0x%x, seq_no:0x%x\r\n",
            onu_id, msg_id, seq_no);

    PLOAM_ASSERT_RET(ploam_buffer != NULL, CA_E_PARAM);

    /* fill common header */
    header_p = (ploam_ngpon2_header_t *)ploam_buffer;
    header_p->onu_id = onu_id;
    header_p->msg_id = msg_id;

    if(!seq_no)
        header_p->seq_no = onu_ploam_seq_no++;
    else
        header_p->seq_no = seq_no;

    ret = ploam_ngpon2_us_msg_proc((void*)ploam_buffer);

    return ret;
}


//this function can be called from ACTIVATION statemachine or CLI
ca_status_t ploam_ngpon2_send_ploam(ca_uint8_t ploam_buffer[], ca_uint8_t frame_size, ca_uint8_t use_def_key)
{
    ca_uint32_t ploam_retry = 0;
    aal_xgpn_plmp_txbuf_ctrl_t ctrl;
    aal_xgpn_plmp_txbuf_ctrl_mask_t  ctrl_mask;
    aal_xgpn_plmp_txbuf_t   txbuf;
    ca_status_t ret;

    memset(&ctrl, 0, sizeof(aal_xgpn_plmp_txbuf_ctrl_t));
    //memset(&ctrl_mask, 0, sizeof(aal_xgpn_plmp_txbuf_ctrl_mask_t));

    do { /* Check H/W ready for Tx PLOAM */
    ret =  aal_ploam_txbuf_ctrl_get(0, &ctrl);
        if (!ctrl.pkt_rdy) { /* H/W is ready for Tx */
            break;
        }
    } while (ploam_retry ++ < PLOAM_TX_RETRY_MAX);

    if(ploam_retry >= PLOAM_TX_RETRY_MAX) {

        PLOAM_DEBUG("PLOAM TX retry timed out\n");

        return CA_E_TIMEOUT;
    }

    /* copy PLOAM into TX buffer - HW will add 8 byte MIC before sending */
    memset(&txbuf, 0, sizeof(aal_xgpn_plmp_txbuf_t));
    memcpy(txbuf.data, ploam_buffer, (frame_size <= XGPN_PLMP_TXBUF_SIZE) ? frame_size : XGPN_PLMP_TXBUF_SIZE);
    ret = aal_ploam_txbuf_data_set(0, &txbuf);

    ctrl_mask.wrd        = 0;
    ctrl_mask.bf.pkt_rdy = 1;
    ctrl.pkt_rdy         = 1;
    if(use_def_key){
        /*  MIC calculation use default PLOAM integrity key. */
        ctrl_mask.bf.defkey = 1;
        ctrl.defkey         = 1;
    }

    ret = aal_ploam_txbuf_ctrl_set(0, ctrl_mask, &ctrl);
    if (CA_E_OK != ret) {
        PLOAM_DEBUG("ret:%d\r\n", ret);
        return ret;
    }

    memset(&ctrl, 0, sizeof(aal_xgpn_plmp_txbuf_ctrl_t));
    ploam_retry = 0;
    do { /* Check H/W Tx done */
    ret =  aal_ploam_txbuf_ctrl_get(0, &ctrl);
        if (!ctrl.pkt_rdy) { /* H/W Tx done */
            break;
        }
    } while (ploam_retry ++ < PLOAM_TX_RETRY_MAX);

    if(ploam_retry >= PLOAM_TX_RETRY_MAX) {

        PLOAM_DEBUG("PLOAM TX timed out\n");
        return CA_E_TIMEOUT;
    }

    return CA_E_OK;
}




