#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "mirror.h"
#include "classifier.h"
#include "port.h"
#include "vlan.h"

#include "aal_mir.h"
#include "aal_fdb.h"
#include "aal_common.h"
#include "aal_ni.h"
#include "aal_l2_vlan.h"
#if defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_SATURN_SFU) 
#include "aal_pon.h"
#endif
#include "ca-scfg/scfg.h"


ca_port_mirror_config_t  mirror_ssid[NUMOF_MIRE_SESSION];
ca_uint32_t   mirror_vlan = 100;
mirror_config_t mirror_cfg;

#define MIR_INVALID_INDEX 0xffffffff

#if 0
ca_status_t __ca_port_mirror_sanity_check(ca_port_mirror_config_t *mirror_config)
{
    ca_status_t ret = CA_E_OK;
    ca_int32_t i;
    ca_boolean_t has_free = 0;

    CA_ASSERT_RET(mirror_config != NULL, CA_E_PARAM);

    for (i = 0; i < NUMOF_MIRE_SESSION; i++){
        if (mirror_ssid[i].enable == 0)
            has_free = 1;

        if (mirror_config->mask.mirror_dest_port){
            if (mirror_config->destination_port == mirror_ssid[i].destination_port){
                L2_DEBUG("conflict destination_port\r\n");
                ret = CA_E_CONFLICT;
                break;
            }
        }
        if (mirror_config->mask.mirror_source_port){
            if (mirror_config->source_port == mirror_ssid[i].source_port){
                L2_DEBUG("conflict source_port\r\n");
                ret = CA_E_CONFLICT;
                break;
            }
        }
        if (mirror_config->mirror_port == mirror_ssid[i].mirror_port){
            L2_DEBUG("conflict mirror_port\r\n");
            ret = CA_E_CONFLICT;
            break;
        }
    }

    if (has_free == 0){
        L2_DEBUG("out of resource\r\n");
        ret = CA_E_RESOURCE;
    }
    return ret;
}

ca_status_t __ca_port_mirror_alloc_sid(ca_port_mirror_config_t *mirror_config)
{
    ca_status_t ret = CA_E_OK;
    ca_int32_t i;
    ca_boolean_t has_free = 0;

    CA_ASSERT_RET(mirror_config != NULL, CA_E_PARAM);

    return ret;
}
#endif


#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3)
extern ca_status_t l2_classifier_g3_mirror_add (
    ca_device_id_t device_id,
    ca_port_id_t   port_id,
    ca_port_id_t   dest_port_id,
    ca_uint32_t    priority
);

ca_status_t __port_mirror_cls_wan_to_l3fe(
    ca_device_id_t device_id,
    ca_boolean_t enable
)
{
    ca_classifier_key_t         key;
    ca_classifier_key_mask_t    key_mask;
    ca_classifier_action_t      action;
    ca_uint32_t                 index = 0;
    ca_status_t                 ret = CA_E_OK;

    memset(&key, 0, sizeof(ca_classifier_key_t));
    memset(&key_mask, 0, sizeof(ca_classifier_key_mask_t));
    memset(&action, 0, sizeof(ca_classifier_action_t));

    if (enable){
        key.src_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, AAL_LPORT_ETH_WAN);
        key_mask.src_port = 1;

        action.forward = CA_CLASSIFIER_FORWARD_PORT;
        action.dest.port = CA_PORT_ID(CA_PORT_TYPE_L3, AAL_LPORT_L3_WAN);

        ret = ca_classifier_rule_add (device_id,
                                      0,
                                      &key,
                                      &key_mask ,
                                      &action,
                                      &index);
        mirror_cfg.wan_to_l3fe_cls_idx = index;
    }
    else{
        ret = ca_classifier_rule_delete(device_id, mirror_cfg.wan_to_l3fe_cls_idx);
    }
    return ret;
}


ca_status_t __port_mirror_dp_redirect(
    ca_port_id_t src_port,
    ca_port_id_t dst_port,
    ca_boolean_t enable)
{
    ca_vlan_port_control_t port_config;
    NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;
    aal_ilpb_cfg_msk_t igr_msk;
    aal_ilpb_cfg_t igr_cfg;
    ca_uint8_t lspid, ldpid;
    ca_ni_rx_l3fe_demux_t demux, dpq_demux;
    aal_l2_vlan_default_cfg_mask_t def_cfg_msk;
    aal_l2_vlan_default_cfg_t def_cfg;
	L3FE_STG0_LDPID_MAP_t   ldpid_map;
	L3FE_STG0_CTRL_t stg0_ctrl;

    memset(&igr_msk, 0, sizeof(aal_ilpb_cfg_msk_t));
    memset(&igr_cfg, 0, sizeof(aal_ilpb_cfg_t));

    memset(&port_config, 0, sizeof(ca_vlan_port_control_t));

    printk("<%s, %d>\r\n", __FUNCTION__, __LINE__);
    if (enable){
        /*
          devmem 0xf4304208 w 0xAAAA8000 NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0
          devmem 0xf43041f8 w 0xAAAA8000 NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0
        */
        lspid = PORT_ID(src_port);
        ldpid = PORT_ID(dst_port);

#if defined(CONFIG_ARCH_CORTINA_G3)
        if (mirror_cfg.g3_l3_lan_2_wan == 0){
            mirror_cfg.g3_l3_lan_2_wan = 1;
            l2_classifier_g3_mirror_add(0, 0x70019, 0x30007, 0);
        }
#endif

        if (ldpid == AAL_LPORT_ETH_WAN){
            dpq_demux.wrd = CA_L2FE_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0);
            mirror_cfg.l3fe_dpq_demux = dpq_demux.wrd;
            demux.wrd = CA_L2FE_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0);
            mirror_cfg.l3fe_demux = demux.wrd;

            dpq_demux.bf.ldpid_7 = 2;
            demux.bf.ldpid_7 = 2;
            CA_L2FE_REG_WRITE(dpq_demux.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0);
            CA_L2FE_REG_WRITE(demux.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0);

        }

        if (lspid == AAL_LPORT_ETH_WAN){
            /* wan_rxsel */
            port_id_cfg.wrd = CA_L2FE_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
            mirror_cfg.wan_rxsel = port_id_cfg.bf.wan_rxsel;

            port_id_cfg.bf.wan_rxsel = 2;
            CA_L2FE_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);

            /* L3FE_STG0_CTRL */
            stg0_ctrl.wrd = CA_L2FE_REG_READ(L3FE_STG0_CTRL);
            mirror_cfg.l3fe_stg0_ctrl = stg0_ctrl.wrd;
            stg0_ctrl.bf.wan_lspid = AAL_LPORT_L3_WAN;
            CA_L2FE_REG_WRITE(stg0_ctrl.wrd, L3FE_STG0_CTRL);

            /* L3FE_STG0_LDPID_MAP */
            ldpid_map.wrd = CA_L2FE_REG_READ(L3FE_STG0_LDPID_MAP);
            mirror_cfg.l3fe_stg0_ldpid_map = ldpid_map.wrd;
            ldpid_map.bf.rule_0 = AAL_LPORT_L3_WAN;
            CA_L2FE_REG_WRITE(ldpid_map.wrd, L3FE_STG0_LDPID_MAP);

            /* disable l3_wan cls */
            igr_msk.s.igr_cls_lookup_en = 1;
            igr_cfg.igr_cls_lookup_en = 0;
            aal_port_ilpb_cfg_set(0, AAL_LPORT_L3_WAN, igr_msk, &igr_cfg);

            /* cls l2fe --> l3fe */
            __port_mirror_cls_wan_to_l3fe(0, TRUE);
        }

        if (ldpid == AAL_LPORT_ETH_WAN || lspid == AAL_LPORT_ETH_WAN){

            memset(&def_cfg_msk, 0, sizeof(aal_l2_vlan_default_cfg_mask_t));
            memset(&def_cfg, 0, sizeof(aal_l2_vlan_default_cfg_t));

            aal_l2_vlan_default_cfg_get(0, &def_cfg);

            mirror_cfg.rx_vlan_member = def_cfg.rx_unknown_vlan_memship_check_bmp;
            mirror_cfg.tx_vlan_member = def_cfg.tx_unknown_vlan_memship_check_bmp;

            def_cfg_msk.s.rx_unknown_vlan_memship_check_bmp = 1;
            def_cfg_msk.s.tx_unknown_vlan_memship_check_bmp = 1;

            def_cfg.tx_unknown_vlan_memship_check_bmp |= (1 << AAL_LPORT_ETH_WAN);
            def_cfg.rx_unknown_vlan_memship_check_bmp |= (1 << AAL_LPORT_ETH_WAN);
            aal_l2_vlan_default_cfg_set(0, def_cfg_msk, &def_cfg);
        }

    }
    else{
        lspid = PORT_ID(mirror_cfg.mirror_cfg[0].source_port);
        ldpid = PORT_ID(mirror_cfg.mirror_cfg[0].destination_port);

        if (ldpid == AAL_LPORT_ETH_WAN){
            CA_L2FE_REG_WRITE(mirror_cfg.l3fe_dpq_demux, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0);
            CA_L2FE_REG_WRITE(mirror_cfg.l3fe_demux, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0);
        }

        if (lspid == AAL_LPORT_ETH_WAN){
            port_id_cfg.wrd = CA_L2FE_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
            port_id_cfg.bf.wan_rxsel = mirror_cfg.wan_rxsel;
            CA_L2FE_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);

            /* L3FE_STG0_CTRL */
            stg0_ctrl.wrd = mirror_cfg.l3fe_stg0_ctrl;
            CA_L2FE_REG_WRITE(stg0_ctrl.wrd, L3FE_STG0_CTRL);

            /* L3FE_STG0_LDPID_MAP */
            ldpid_map.wrd = mirror_cfg.l3fe_stg0_ldpid_map;
            CA_L2FE_REG_WRITE(ldpid_map.wrd, L3FE_STG0_LDPID_MAP);

            /* en l3_wan cls */
            igr_msk.s.igr_cls_lookup_en = 1;
            igr_cfg.igr_cls_lookup_en = 1;
            aal_port_ilpb_cfg_set(0, AAL_LPORT_L3_WAN, igr_msk, &igr_cfg);

            __port_mirror_cls_wan_to_l3fe(0, FALSE);
        }

        if (ldpid == AAL_LPORT_ETH_WAN || lspid == AAL_LPORT_ETH_WAN){

            memset(&def_cfg_msk, 0, sizeof(aal_l2_vlan_default_cfg_mask_t));
            memset(&def_cfg, 0, sizeof(aal_l2_vlan_default_cfg_t));

            def_cfg_msk.s.rx_unknown_vlan_memship_check_bmp = 1;
            def_cfg_msk.s.tx_unknown_vlan_memship_check_bmp = 1;

            def_cfg.tx_unknown_vlan_memship_check_bmp = mirror_cfg.tx_vlan_member;
            def_cfg.rx_unknown_vlan_memship_check_bmp = mirror_cfg.rx_vlan_member;
            aal_l2_vlan_default_cfg_set(0, def_cfg_msk, &def_cfg);
        }

    }
}
ca_status_t __port_mirror_enable_set (
    ca_device_id_t             device_id,
    ca_port_mirror_config_t    * cfg)
{
    ca_status_t              ret = CA_E_OK;
    aal_mire_mir_rule_mask_t rule_mask;
    aal_mire_mir_rule_t      mire_rule;
    aal_mire_mir_act_mask_t  act_mask;
    aal_mire_mir_act_t       act;
    ca_int32                 i = 0;
    ca_boolean_t             find_flag = FALSE;
    ca_port_mirror_config_t * local_cfg = NULL;
    ca_int32                ssid = 0;
    ca_int32                free_entry_idx = MIR_INVALID_INDEX;
    ca_uint32_t             index = 0;
    ca_uint32               port_type = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    aal_pon_pdc_map_mem_data_t   pdc_map_cfg;
#endif
    ca_uint8_t wan_to_l3fe = 0;
    
    CA_ASSERT_RET( cfg  != NULL, CA_E_PARAM);

   // CA_PORT_ETHERNET_CHECK(cfg->mirror_port);

    L2_DEBUG("\r\nmask=0x%x,enable=0x%x,src port=0x%x,dst port=0x%x,mir port=0x%x \r\n",
                cfg->mask, cfg->enable, cfg->source_port, cfg->destination_port, cfg->mirror_port);

#if defined(CONFIG_ARCH_CORTINA_G3)
    if (cfg->enable == mirror_cfg.mirror_cfg[0].enable){
        ca_printf("Mirror Entry has already %s\r\n", cfg->enable ? "Enable" : "Disable");
        return CA_E_CONFLICT;
    }

    ret = aal_scfg_read(CFG_ID_WAN_TO_L3FE, 1, &wan_to_l3fe);
    if (ret != CA_E_OK) {
        printk("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT, ret=%d\n", __func__, ret);
    }

    if (wan_to_l3fe == 1)
        __port_mirror_dp_redirect(cfg->source_port, cfg->destination_port, cfg->enable);
#endif

    for (i = 0; i < 2 ; i++ ){
        local_cfg = &mirror_cfg.mirror_cfg[i];
        if ( (local_cfg->mask.mirror_source_port == 0 )
            && (local_cfg->mask.mirror_dest_port == 0)
            && (local_cfg->enable == FALSE )) {
            /*find a free entry*/
            if (free_entry_idx ==  MIR_INVALID_INDEX){
                free_entry_idx = i;
                L2_DEBUG ("\r\nget free_entry_idx=0x%x in %s %d \r\n", free_entry_idx, __FUNCTION__, __LINE__  );
            }
            continue;
        }
        
        if ( (0 == memcmp (&cfg->mask , &local_cfg->mask, sizeof(ca_port_mirror_mask_t)))
            && (cfg->source_port == local_cfg->source_port)
            && (cfg->destination_port == local_cfg->destination_port)
            && (cfg->mirror_port == local_cfg->mirror_port) ){
            find_flag = TRUE;

            /*clear existed mirror rule*/
            
            if( cfg->enable == FALSE ){
                memset (&rule_mask, 0, sizeof(aal_mire_mir_rule_mask_t));
                memset (&mire_rule, 0, sizeof(aal_mire_mir_rule_t));

                rule_mask.bf.mode = 1;
                rule_mask.bf.lspid = 1;
                rule_mask.bf.ldpid = 1;
                
                mire_rule.mode = MIR_DISABLE;
                mire_rule.lspid = 0;
                mire_rule.ldpid = 0;

                ssid = i*2 + 0;

                ret =  aal_mire_rule_set(0, ssid, rule_mask, &mire_rule);
                ret =  aal_mire_rule_set(0, ssid+1, rule_mask, &mire_rule); 

                memset (local_cfg, 0, sizeof(ca_port_mirror_config_t));
                L2_DEBUG ("\r\nclear mir rule, ssid=0x%x in %s %d \r\n", ssid, __FUNCTION__, __LINE__  );
                    
            }else {
                 /*same mir rule cfg , return */
                 
                L2_DEBUG ("\r\nsame mir rule cfg,  i=0x%x in %s %d \r\n", i, __FUNCTION__, __LINE__  );
            }

            goto END;
        }
        
    }

#if defined(CONFIG_ARCH_CORTINA_G3)
    free_entry_idx = 0;
#endif

    if (free_entry_idx == MIR_INVALID_INDEX){
        ret = CA_E_ERROR;
        goto END;
    }

    ssid = free_entry_idx*2;

    /*add a new MIR rule*/
    if ( (cfg->mask.mirror_source_port)
         && (CA_PORT_TYPE_VIRTUAL == PORT_TYPE(cfg->source_port)) ){

      //  cfg->mask.mirror_source_port = 0;
        memset (&rule_mask, 0, sizeof(aal_mire_mir_rule_mask_t));
        memset (&mire_rule, 0, sizeof(aal_mire_mir_rule_t));

        /* rule */
        rule_mask.bf.mode = 1;
        rule_mask.bf.flowid = 1;
        
        mire_rule.mode = MIR_FLOW_BASE;

        mire_rule.flowid = PORT_ID(cfg->source_port);
       
        L2_DEBUG("mire_rule.lspid=0x%x\r\n", mire_rule.flowid);

        ret =  aal_mire_rule_set(0, ssid, rule_mask, &mire_rule);

        /* action */
        memset (&act_mask, 0, sizeof(aal_mire_mir_act_mask_t));
        memset (&act, 0, sizeof(aal_mire_mir_act_t));
        act_mask.bf.ldpid = 1;
        
        act_mask.bf.lspid = 1;
        act_mask.bf.cos_vld = 1;
        act_mask.bf.cos = 1;

        act.lspid = 0;
        act.cos_vld = 1;
        act.cos = 6;
        

        act.ldpid = PORT_ID( cfg->mirror_port);
        ret = aal_mire_act_set(0, ssid, act_mask, &act);

        L2_DEBUG("downstream gem set mirror ssid %d, source_port:0x%x, rule lspid:%d\r\n", 
                                            ssid, cfg->source_port, mire_rule.lspid);
        goto SAVE_CFG;
    }

    /* set mirror, ingress, downsteam */
    if ( cfg->mask.mirror_source_port){
        memset (&rule_mask, 0, sizeof(aal_mire_mir_rule_mask_t));
        memset (&mire_rule, 0, sizeof(aal_mire_mir_rule_t));

        /* rule */
        rule_mask.bf.mode = 1;
        rule_mask.bf.lspid = 1;
        

        mire_rule.mode = MIR_INGRESS_PORT;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        memset(&pdc_map_cfg, 0, sizeof(aal_pon_pdc_map_mem_data_t));

        port_type = PORT_TYPE(cfg->source_port);
        if ((CA_PORT_TYPE_GPON == port_type)
            ||(CA_PORT_TYPE_EPON == port_type)
            ||(CA_PORT_TYPE_SUBPORT == port_type)
            ||(CA_PORT_TYPE_LLID == port_type)){
            ret = aal_pon_pdc_map_mem_get(0, PORT_ID(cfg->source_port), &pdc_map_cfg);
            /*mire_rule.ldpid = */
            mire_rule.lspid = pdc_map_cfg.lspid;
        }else
#endif
        {
            mire_rule.lspid =  PORT_ID(cfg->source_port);
        }

        ret =  aal_mire_rule_set(0, ssid, rule_mask, &mire_rule);

        /* action */
        memset (&act_mask, 0, sizeof(aal_mire_mir_act_mask_t));
        memset (&act, 0, sizeof(aal_mire_mir_act_t));
        act_mask.bf.ldpid = 1;
        
        act_mask.bf.lspid = 1;
 
        act.lspid = 0;

        act.ldpid = PORT_ID( cfg->mirror_port);
        ret = aal_mire_act_set(0, ssid, act_mask, &act);

        L2_DEBUG("ingress rule ssid:%d lspid:%d, mode:%d. act ldpid:%d\r\n", ssid, mire_rule.lspid, mire_rule.mode, act.ldpid);

    }

    /* egress, upstream  */
    if (cfg->mask.mirror_dest_port){
        /* rule */
        memset (&rule_mask, 0, sizeof(aal_mire_mir_rule_mask_t));
        memset (&mire_rule, 0, sizeof(aal_mire_mir_rule_t));

        rule_mask.bf.mode = 1;
        rule_mask.bf.ldpid = 1;
        rule_mask.bf.lspid = 1;
       
        mire_rule.mode = MIR_EGRESS_PORT;

        port_type = PORT_TYPE(cfg->destination_port);
          
        if ((CA_PORT_TYPE_GPON == port_type)
              ||(CA_PORT_TYPE_EPON == port_type)
              ||(CA_PORT_TYPE_SUBPORT == port_type)
              ||(CA_PORT_TYPE_LLID == port_type)){
            mire_rule.ldpid = PORT_ID(cfg->destination_port) + 0x20;
        }else {
            mire_rule.ldpid = PORT_ID(cfg->destination_port);
        }
        
        ret =  aal_mire_rule_set(0, ssid+1, rule_mask, &mire_rule);

        /* action */
        memset (&act_mask, 0, sizeof(aal_mire_mir_act_mask_t));
        memset (&act, 0, sizeof(aal_mire_mir_act_t));
        act_mask.bf.ldpid = 1;
        act_mask.bf.lspid = 1;

        act.lspid = 0;
        act.ldpid = PORT_ID( cfg->mirror_port);;

        ret =  aal_mire_act_set(0, ssid+1 , act_mask, &act);
        L2_DEBUG("egress mirror ssid %d, rule mode:%d, ldpid:%d, act ldpid:%d\r\n",
                    ssid + 1, mire_rule.mode, mire_rule.ldpid, act.ldpid);
        
    }
    
SAVE_CFG:    

    memcpy(&mirror_cfg.mirror_cfg[free_entry_idx], cfg, sizeof(ca_port_mirror_config_t));

END:
    return ret;
}

#endif


ca_status_t ca_port_mirror_enable_set (
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_port_mirror_config_t    *mirror_config)
{

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3)
     return __port_mirror_enable_set(device_id, mirror_config);
#endif

#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    ca_device_id_t dev_id = 1;
    aal_pon_pdc_map_mem_data_t pdc_map_cfg;
    ca_uint32_t gem_port_id;
    aal_mire_mir_act_mask_t  act_mask;
    aal_mire_mir_rule_t      mire_rule;
    ca_status_t ret = CA_E_OK;
    aal_mire_mir_rule_mask_t rule_mask;
    aal_mire_mir_act_t       act;
    ca_port_id_t member_ports[16];
    ca_uint8_t   member_count = 0;
    ca_uint8_t   untagged_count = 0;
    ca_port_id_t untagged_ports[16];
    ca_classifier_key_t key;
    ca_classifier_key_mask_t key_mask;
    ca_classifier_action_t action;
    ca_uint32_t index = 0;

    CA_ASSERT_RET(mirror_config != NULL, CA_E_PARAM);

    CA_PORT_ETHERNET_CHECK(mirror_config->mirror_port);

    if (mirror_config->enable == FALSE && mirror_cfg.mirror_cfg[0].enable == FALSE){
        L2_DEBUG("mirror has not enabled yet!\r\n");
        return ret;
    }

    L2_DEBUG("mask src:%d, dst:%d; enable:%d, src:0x%x, dst:0x%x, mirror_to:0x%x\r\n", 
        mirror_config->mask.mirror_source_port, mirror_config->mask.mirror_dest_port,
        mirror_config->enable, mirror_config->source_port, mirror_config->destination_port,
        mirror_config->mirror_port);

    memset (&rule_mask, 0, sizeof(aal_mire_mir_rule_mask_t));
    memset (&mire_rule, 0, sizeof(aal_mire_mir_rule_t));
    memset (&act_mask, 0, sizeof(aal_mire_mir_act_mask_t));
    memset (&act, 0, sizeof(aal_mire_mir_act_t));

    if (mirror_cfg.mirror_cfg[0].enable){
        /* clean up mirror */
        rule_mask.bf.mode = 1;
        mire_rule.mode = MIR_DISABLE;

        ret =  aal_mire_rule_set(dev_id, 0, rule_mask, &mire_rule);
        ret =  aal_mire_rule_set(dev_id, 1, rule_mask, &mire_rule);

        /* remove l2/l3 cls */
        ret = ca_classifier_rule_delete(0, mirror_cfg.l3_cls_idx);
        ret = ca_classifier_rule_delete(0, mirror_cfg.l2_cls_idx);

        /* remove vlan */
        ret = ca_l2_vlan_delete(0, mirror_cfg.vlan);
        L2_DEBUG("clean mirror\r\n");
    }

    if(mirror_config->enable == FALSE){
        mirror_cfg.mirror_cfg[0].enable = FALSE;
        L2_DEBUG("mark mirror cb to disable\r\n");
        return ret;
    }

    /* set mirror, ingress, downsteam */
    if (mirror_config->mask.mirror_source_port){
        memset (&rule_mask, 0, sizeof(aal_mire_mir_rule_mask_t));
        memset (&mire_rule, 0, sizeof(aal_mire_mir_rule_t));

        /* rule */
        rule_mask.bf.mode = 1;
        rule_mask.bf.ldpid = 1;
        rule_mask.bf.lspid = 1;
        
        if (0xffff == PORT_ID(mirror_config->source_port)){
            mire_rule.mode = MIR_EGRESS_PORT;
            mire_rule.ldpid = 6;
        }else{
            mire_rule.mode = MIR_INGRESS_PORT;
            mire_rule.lspid = PORT_ID(mirror_config->source_port);
        }

        memset(&pdc_map_cfg, 0, sizeof(aal_pon_pdc_map_mem_data_t));
        gem_port_id = (mirror_config->source_port) & 0xff;
        ret = aal_pon_pdc_map_mem_get(0, gem_port_id, &pdc_map_cfg);
        /*mire_rule.ldpid = */
        mire_rule.lspid = pdc_map_cfg.lspid;

        ret =  aal_mire_rule_set(dev_id, 0, rule_mask, &mire_rule);

        /* action */
        memset (&act_mask, 0, sizeof(aal_mire_mir_act_mask_t));
        memset (&act, 0, sizeof(aal_mire_mir_act_t));
        act_mask.bf.ldpid = 1;
        act_mask.bf.lspid = 1;
        act_mask.bf.vlan_cmd = 1;
        act_mask.bf.vid = 1;

        act.lspid = 0;
        act.vlan_cmd = 1;
        act.vid = mirror_vlan;
        act.ldpid = 6;
        ret = aal_mire_act_set(dev_id, 0, act_mask, &act);

        L2_DEBUG("downstream gem set mirror ssid 0, source_port:0x%x, rule lspid:%d\r\n", 
                mirror_config->source_port, mire_rule.lspid);
    }

    /* egress, upstream  */
    if (mirror_config->mask.mirror_dest_port){
        /* rule */
        memset (&rule_mask, 0, sizeof(aal_mire_mir_rule_mask_t));
        memset (&mire_rule, 0, sizeof(aal_mire_mir_rule_t));

        rule_mask.bf.mode = 1;
        rule_mask.bf.ldpid = 1;
        rule_mask.bf.lspid = 1;
        
        /* T-CONT */
        if (0xff == (mirror_config->destination_port & 0xff)){
            mire_rule.mode = MIR_INGRESS_PORT;
            mire_rule.lspid = 6;
        }else{
            mire_rule.mode = MIR_EGRESS_PORT;
            mire_rule.ldpid = PORT_ID(mirror_config->destination_port) + 0x20;
        }
        ret =  aal_mire_rule_set(dev_id, 1, rule_mask, &mire_rule);

        /* action */
        memset (&act_mask, 0, sizeof(aal_mire_mir_act_mask_t));
        memset (&act, 0, sizeof(aal_mire_mir_act_t));
        act_mask.bf.ldpid = 1;
        act_mask.bf.lspid = 1;
        act_mask.bf.vlan_cmd = 1;
        act_mask.bf.vid = 1;

        act.lspid = 0;
        act.vlan_cmd = 1;
        act.vid = mirror_vlan;
        act.ldpid = 6;

        ret =  aal_mire_act_set(dev_id, 1, act_mask, &act);
        L2_DEBUG("upstream T-CONT set mirror ssid 1, destination_port:%d, rule lspid:%d\r\n", 
                mirror_config->destination_port, mire_rule.lspid);

    }

    /* create vlan */
    ret = ca_l2_vlan_create(0, mirror_vlan);
    member_count = 2;
    member_ports[0] = mirror_config->mirror_port;
    member_ports[1] = 0x10007;
    untagged_count = 0;

    ret = ca_l2_vlan_port_set(0, mirror_vlan, member_count, member_ports, untagged_count, untagged_ports);
    L2_DEBUG("ret:%d, vlan:%d, mem_cnt:%d, port[0]:0x%x, port[1]:0x%x, untag_cnt:%d\r\n", ret, 
            mirror_vlan, member_count, member_ports[0], member_ports[1], untagged_count);

    /* l3 cls, forward mirror packet to l2fe 0xc0019 */
    memset(&key, 0, sizeof(key));
    memset(&key_mask, 0, sizeof(key_mask));
    memset(&action, 0, sizeof(action));
    key.l2.vlan_otag.vlan_min.vid          = mirror_vlan;
    key.l2.vlan_otag.vlan_max.vid          = mirror_vlan;
    key.src_port                           = CA_PORT_ID(CA_PORT_TYPE_L3, 0x7); //0xc0007
    key_mask.src_port                      = 1;
    key_mask.l2                            = 1;
    key_mask.l2_mask.vlan_otag             = 1;
    key_mask.l2_mask.vlan_otag_mask.vid    = 1;
    action.forward                         = CA_CLASSIFIER_FORWARD_PORT;
    action.dest.port                       = CA_PORT_ID(CA_PORT_TYPE_L3, 0x19); //0xc0019; 0x19=l2fe
    action.options.masks.action_handle     = 1;
    L2_DEBUG("add classifier key otag pri:%d, vid:%d, src port:0x%x, dest port:0x%x\r\n",
              key.l2.vlan_otag.vlan_min.pri, key.l2.vlan_otag.vlan_min.vid,
              key.src_port, action.dest.port);

    ret = ca_classifier_rule_add(0, 0, &key, &key_mask, &action, &index);

    if (ret != CA_E_OK){
        L2_DEBUG("create l3 cls fail, ret:%d\r\n", ret);
        return ret;
    }

    mirror_cfg.l3_cls_idx = index;

    /* l2 cls, forward mirror packet to mirror_to port */
    memset(&key, 0, sizeof(key));
    memset(&key_mask, 0, sizeof(key_mask));
    memset(&action, 0, sizeof(action));

    key.l2.vlan_otag.vlan_min.vid          = mirror_vlan;
    key.l2.vlan_otag.vlan_max.vid          = mirror_vlan;
    key.key_handle.gem_index               = 0xffff;
    key.src_port                           = CA_PORT_ID(CA_PORT_TYPE_GPON, 0x7);//0x10007;
    key.handle_type                        = CA_KEY_HANDLE_TYPE_GEM_INDEX;

    key_mask.key_handle                    = 1;
    key_mask.src_port                      = 1;
    key_mask.l2                            = 1;
    key_mask.l2_mask.vlan_otag             = 1;
    key_mask.l2_mask.vlan_otag_mask.vid    = 1;
    action.forward                         = CA_CLASSIFIER_FORWARD_PORT;
    action.dest.port                       = mirror_config->mirror_port;
    action.options.outer_vlan_act          = CA_CLASSIFIER_VLAN_ACTION_POP;
    action.options.masks.action_handle     = 1;
    action.options.masks.outer_vlan_act    = 1;
    L2_DEBUG("add classifier key otag pri:%d, src port:0x%x, dest port:0x%x\r\n",
              key.l2.vlan_otag.vlan_min.pri, key.src_port, action.dest.port);

    ret = ca_classifier_rule_add(0, 0, &key, &key_mask, &action, &index);
    
    if (ret != CA_E_OK){
        L2_DEBUG("create l2 cls fail\r\n");
        return CA_E_RESOURCE;
    }

    /* keep to local */
    mirror_cfg.l2_cls_idx = index;
    mirror_cfg.vlan = mirror_vlan;
    mirror_cfg.mirror_cfg[0] = *mirror_config;

#endif
    return CA_E_OK;
}

ca_status_t ca_port_mirror_enable_get (
    CA_IN   ca_device_id_t            device_id,
    CA_OUT  ca_port_mirror_config_t   *mirror_config)
{

    CA_ASSERT_RET(mirror_config != NULL, CA_E_PARAM);

    *mirror_config = mirror_cfg.mirror_cfg[0];

    return CA_E_OK;
}

ca_status_t ca_mirror_control_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t src_port,
    CA_IN ca_port_id_t dst_port,
    CA_IN ca_boolean_t mir_ena)
{
    return CA_E_OK;
}

ca_status_t ca_mirror_init(
    ca_device_id_t     device_id
)
{
    ca_device_config_tlv_t scfg_tlv;
    ca_uint32_t vlan = 4018;
    ca_status_t ret = CA_E_OK;

    memset(mirror_ssid, 0, sizeof(mirror_ssid));

    memset(&mirror_cfg, 0, sizeof(mirror_cfg));
    memset(&scfg_tlv, 0, sizeof(scfg_tlv));
    scfg_tlv.type = CA_CFG_ID_MIRROR_VLAN;
    scfg_tlv.len  = sizeof(ca_uint32_t);
    ret = ca_device_config_tlv_get(0, &scfg_tlv);

    if (CA_E_OK == ret){
        memcpy(&vlan, scfg_tlv.value, sizeof(ca_uint32_t));
    }
    mirror_vlan = vlan;
    ca_printf("%s mirror_vlan:%d\r\n", __FUNCTION__, mirror_vlan);

    return CA_E_OK;
}


#ifdef __KERNEL__

EXPORT_SYMBOL(ca_port_mirror_enable_set);
EXPORT_SYMBOL(ca_port_mirror_enable_get);
EXPORT_SYMBOL(ca_mirror_control_set);
#endif

