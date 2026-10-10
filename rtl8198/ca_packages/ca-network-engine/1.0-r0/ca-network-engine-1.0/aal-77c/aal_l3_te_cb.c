
#include <generated/ca_ne_autoconf.h>
#include "aal_l3_te_cb.h"
#include "osal_common.h"
#include "aal_common.h"
#include "aal_ni.h"
#include "scfg.h"



#define __AAL_L3_TE_CB_PORT_NUM                 16
#define __AAL_L3_TE_CB_PORT_SEL_MAX             3
#define __AAL_L3_TE_CB_VOQ_NUM                  128
#define __AAL_L3_TE_CB_BUF_NUM                  16384
#define __AAL_L3_TE_CB_THRED_DISABLE            0x3fff

#define __AAL_L3_TE_CB_PORT_FREE_BUF_PE         128
#define __AAL_L3_TE_CB_PORT_FREE_BUF_CPU256     2048
#define __AAL_L3_TE_CB_PORT_FREE_BUF_A53        3000
#define __AAL_L3_TE_CB_PORT_FREE_BUF_DQ         8192

#define __AAL_L3_TE_CB_PORT_PROFILE_PE_HI       38
#define __AAL_L3_TE_CB_PORT_PROFILE_PE_LO       12
#define __AAL_L3_TE_CB_PORT_PROFILE_CPU256_HI   38
#define __AAL_L3_TE_CB_PORT_PROFILE_CPU256_LO   12
#define __AAL_L3_TE_CB_PORT_PROFILE_A53_HI      150
#define __AAL_L3_TE_CB_PORT_PROFILE_A53_LO      50
#define __AAL_L3_TE_CB_PORT_PROFILE_DQ_HI       307
#define __AAL_L3_TE_CB_PORT_PROFILE_DQ_LO       102

#define __AAL_L3_TE_CB_VOQ_PROFILE_PE57_HI       14
#define __AAL_L3_TE_CB_VOQ_PROFILE_PE57_LO       8
#define __AAL_L3_TE_CB_VOQ_PROFILE_PE57_PRVT     3
#define __AAL_L3_TE_CB_VOQ_PROFILE_PE04_HI       14
#define __AAL_L3_TE_CB_VOQ_PROFILE_PE04_LO       8
#define __AAL_L3_TE_CB_VOQ_PROFILE_PE04_PRVT     3

#define __AAL_L3_TE_CB_VOQ_PROFILE_CPU57_HI       56
#define __AAL_L3_TE_CB_VOQ_PROFILE_CPU57_LO       31
#define __AAL_L3_TE_CB_VOQ_PROFILE_CPU57_PRVT     12
#define __AAL_L3_TE_CB_VOQ_PROFILE_CPU04_HI       56
#define __AAL_L3_TE_CB_VOQ_PROFILE_CPU04_LO       31
#define __AAL_L3_TE_CB_VOQ_PROFILE_CPU04_PRVT     12

#define __AAL_L3_TE_CB_VOQ_PROFILE_A53_57_HI       115
#define __AAL_L3_TE_CB_VOQ_PROFILE_A53_57_LO       64
#define __AAL_L3_TE_CB_VOQ_PROFILE_A53_57_PRVT     25
#define __AAL_L3_TE_CB_VOQ_PROFILE_A53_04_HI       115
#define __AAL_L3_TE_CB_VOQ_PROFILE_A53_04_LO       64
#define __AAL_L3_TE_CB_VOQ_PROFILE_A53_04_PRVT     25

#define __AAL_L3_TE_CB_VOQ_PROFILE_DQ_57_HI       7073
#define __AAL_L3_TE_CB_VOQ_PROFILE_DQ_57_LO       5501
#define __AAL_L3_TE_CB_VOQ_PROFILE_DQ_57_PRVT     71
#define __AAL_L3_TE_CB_VOQ_PROFILE_DQ_04_HI       7036
#define __AAL_L3_TE_CB_VOQ_PROFILE_DQ_04_LO       5472
#define __AAL_L3_TE_CB_VOQ_PROFILE_DQ_04_PRVT     30

#define __AAL_L3_TE_CB_PE0_DPID                  CA_NI_PE0_PORT
#define __AAL_L3_TE_CB_PE1_DPID                  CA_NI_PE1_PORT

#define __AAL_L3_TE_CB_PROFILE_SEL_PE            0
#define __AAL_L3_TE_CB_PROFILE_SEL_A53           2
#define __AAL_L3_TE_CB_PROFILE_SEL_DQ            3
#define __AAL_L3_TE_CB_PROFILE_SEL_CPU256        1

#define __AAL_L3_TE_CB_EQ_PROFILE_0123          12
#define __AAL_L3_TE_CB_EQ_PROFILE_4567          204
#define __AAL_L3_TE_CB_EQ_PROFILE_89            300
#define __AAL_L3_TE_CB_EQ_PROFILE_AB            819
#define __AAL_L3_TE_CB_EQ_PROFILE_F             __AAL_L3_TE_CB_THRED_DISABLE

ca_uint32_t l3te_profile_thrsh_eq0_th0 = 0;
ca_uint32_t l3te_profile_thrsh_eq0_th1 = 0;
ca_uint32_t l3te_profile_thrsh_eq1_th0 = 0;
ca_uint32_t l3te_profile_thrsh_eq1_th1 = 0;
ca_uint32_t l3te_profile_thrsh_eq2_th0 = 0;
ca_uint32_t l3te_profile_thrsh_eq2_th1 = 0;
ca_uint32_t l3te_profile_thrsh_eq3_th0 = 0;
ca_uint32_t l3te_profile_thrsh_eq3_th1 = 0;
ca_uint32_t l3te_profile_thrsh_eq4_th0 = 0;
ca_uint32_t l3te_profile_thrsh_eq4_th1 = 0;
ca_uint32_t l3te_profile_thrsh_eq5_th0 = 0;
ca_uint32_t l3te_profile_thrsh_eq5_th1 = 0;
ca_uint32_t l3te_profile_thrsh_eq6_th0 = 0;
ca_uint32_t l3te_profile_thrsh_eq6_th1 = 0;
ca_uint32_t l3te_profile_thrsh_eq7_th0 = 0;
ca_uint32_t l3te_profile_thrsh_eq7_th1 = 0;

ca_uint32_t l3te_port_thrsh_profile_0_lth = 0;
ca_uint32_t l3te_port_thrsh_profile_1_lth = 0;
ca_uint32_t l3te_port_thrsh_profile_2_lth = 0;
ca_uint32_t l3te_port_thrsh_profile_3_lth = 0;

ca_uint32_t l3te_port_thrsh_profile_0_hth = 0;
ca_uint32_t l3te_port_thrsh_profile_1_hth = 0;
ca_uint32_t l3te_port_thrsh_profile_2_hth = 0;
ca_uint32_t l3te_port_thrsh_profile_3_hth = 0;


ca_uint32_t l3te_cb_port_grpthrsh_profile_0_lth = 0;
ca_uint32_t l3te_cb_port_grpthrsh_profile_1_lth = 0;
ca_uint32_t l3te_cb_port_grpthrsh_profile_2_lth = 0;
ca_uint32_t l3te_cb_port_grpthrsh_profile_3_lth = 0;

ca_uint32_t l3te_cb_port_grpthrsh_profile_0_hth = 0;
ca_uint32_t l3te_cb_port_grpthrsh_profile_1_hth = 0;
ca_uint32_t l3te_cb_port_grpthrsh_profile_2_hth = 0;
ca_uint32_t l3te_cb_port_grpthrsh_profile_3_hth = 0;


ca_uint32_t l3te_voq_thrsh_profile_0_thl = 0;
ca_uint32_t l3te_voq_thrsh_profile_1_thl = 0;
ca_uint32_t l3te_voq_thrsh_profile_2_thl = 0;
ca_uint32_t l3te_voq_thrsh_profile_3_thl = 0;
ca_uint32_t l3te_voq_thrsh_profile_4_thl = 0;
ca_uint32_t l3te_voq_thrsh_profile_5_thl = 0;
ca_uint32_t l3te_voq_thrsh_profile_6_thl = 0;
ca_uint32_t l3te_voq_thrsh_profile_7_thl = 0;

ca_uint32_t l3te_voq_thrsh_profile_0_thh = 0;
ca_uint32_t l3te_voq_thrsh_profile_1_thh = 0;
ca_uint32_t l3te_voq_thrsh_profile_2_thh = 0;
ca_uint32_t l3te_voq_thrsh_profile_3_thh = 0;
ca_uint32_t l3te_voq_thrsh_profile_4_thh = 0;
ca_uint32_t l3te_voq_thrsh_profile_5_thh = 0;
ca_uint32_t l3te_voq_thrsh_profile_6_thh = 0;
ca_uint32_t l3te_voq_thrsh_profile_7_thh = 0;

ca_uint32_t l3te_voq_thrsh_profile_0_prvt = 0;
ca_uint32_t l3te_voq_thrsh_profile_1_prvt = 0;
ca_uint32_t l3te_voq_thrsh_profile_2_prvt = 0;
ca_uint32_t l3te_voq_thrsh_profile_3_prvt = 0;
ca_uint32_t l3te_voq_thrsh_profile_4_prvt = 0;
ca_uint32_t l3te_voq_thrsh_profile_5_prvt = 0;
ca_uint32_t l3te_voq_thrsh_profile_6_prvt = 0;
ca_uint32_t l3te_voq_thrsh_profile_7_prvt = 0;

ca_uint32_t l3te_src_port_thrsh_0_thl = 0;
ca_uint32_t l3te_src_port_thrsh_1_thl = 0;

ca_uint32_t l3te_src_port_thrsh_1_thh = 0;
ca_uint32_t l3te_src_port_thrsh_0_thh  = 0;

ca_uint32_t l3te_src_pri_thrsh_0_thl = 0;
ca_uint32_t l3te_src_pri_thrsh_1_thl = 0;
ca_uint32_t l3te_src_pri_thrsh_2_thl = 0;
ca_uint32_t l3te_src_pri_thrsh_3_thl = 0;
ca_uint32_t l3te_src_pri_thrsh_4_thl = 0;
ca_uint32_t l3te_src_pri_thrsh_5_thl = 0;
ca_uint32_t l3te_src_pri_thrsh_6_thl = 0;
ca_uint32_t l3te_src_pri_thrsh_7_thl = 0;

ca_uint32_t l3te_src_pri_thrsh_0_thh = 0;
ca_uint32_t l3te_src_pri_thrsh_1_thh = 0;
ca_uint32_t l3te_src_pri_thrsh_2_thh = 0;
ca_uint32_t l3te_src_pri_thrsh_3_thh = 0;
ca_uint32_t l3te_src_pri_thrsh_4_thh = 0;
ca_uint32_t l3te_src_pri_thrsh_5_thh = 0;
ca_uint32_t l3te_src_pri_thrsh_6_thh = 0;
ca_uint32_t l3te_src_pri_thrsh_7_thh = 0;

ca_uint32_t l3te_dqsch_port_thrsh_profile_0  = 0;
ca_uint32_t l3te_dqsch_port_thrsh_profile_1  = 0;
ca_uint32_t l3te_dqsch_port_thrsh_profile_2  = 0;
ca_uint32_t l3te_dqsch_port_thrsh_profile_3  = 0;

ca_uint32_t l3te_dqsch_voq_thrsh_profile_0 = 0;
ca_uint32_t l3te_dqsch_voq_thrsh_profile_1 = 0;
ca_uint32_t l3te_dqsch_voq_thrsh_profile_2 = 0;
ca_uint32_t l3te_dqsch_voq_thrsh_profile_3 = 0;
ca_uint32_t l3te_dqsch_voq_thrsh_profile_4 = 0;
ca_uint32_t l3te_dqsch_voq_thrsh_profile_5 = 0;
ca_uint32_t l3te_dqsch_voq_thrsh_profile_6 = 0;
ca_uint32_t l3te_dqsch_voq_thrsh_profile_7 = 0;

ca_uint32_t l3te_port00_free_buf_cnt = 1500;
ca_uint32_t l3te_port01_free_buf_cnt = 1500;
ca_uint32_t l3te_port02_free_buf_cnt = 0;
ca_uint32_t l3te_port03_free_buf_cnt = 0;
ca_uint32_t l3te_port04_free_buf_cnt = 0;
ca_uint32_t l3te_port05_free_buf_cnt = 0;
ca_uint32_t l3te_port06_free_buf_cnt = 1500;
ca_uint32_t l3te_port07_free_buf_cnt = 1500;
ca_uint32_t l3te_port08_free_buf_cnt = 64;
ca_uint32_t l3te_port09_free_buf_cnt = 12288;
ca_uint32_t l3te_port10_free_buf_cnt = 0;
ca_uint32_t l3te_port11_free_buf_cnt = 0;
ca_uint32_t l3te_port12_free_buf_cnt = 0;
ca_uint32_t l3te_port13_free_buf_cnt = 0;
ca_uint32_t l3te_port14_free_buf_cnt = 0;
ca_uint32_t l3te_port15_free_buf_cnt = 0;


#define __AAL_L3_TE_CB_SRC_PRI_THRSH_HI         230
#define __AAL_L3_TE_CB_SRC_PRI_THRSH_LO         128

#define __AAL_L3_TE_CB_DEEPQ_PORT_THRSH         307
#define __AAL_L3_TE_CB_DEEPQ_VOQ_THRSH          115

ca_status_t  aal_l3_te_cb_intr_ena_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_l3_te_cb_intr_mask_t  mask  ,
    CA_IN aal_l3_te_cb_intr_t       *cfg
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_INTE_t intr_en;

    if (cfg == NULL) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        intr_en.wrd = CA_NE_REG_READ(TE_CB_INTE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        intr_en.wrd = CA_8279_NE_REG_READ(TE_CB_INTE);
#endif
    }

    if (mask.s.port_freecnt_ovfl) {
        intr_en.bf.port_freecnt_ovflE = cfg->port_freecnt_ovfl;
    }
    if (mask.s.port_freecnt_udfl) {
        intr_en.bf.port_freecnt_udflE = cfg->port_freecnt_udfl;
    }
    if (mask.s.voq_bufcnt_ovfl) {
        intr_en.bf.voq_bufcnt_ovflE = cfg->voq_bufcnt_ovfl;
    }
    if (mask.s.voq_bufcnt_udfl) {
        intr_en.bf.voq_bufcnt_udflE = cfg->voq_bufcnt_udfl;
    }
    /*if (mask.s.scan_cycle_err) {
        intr_en.bf.scan_cycle_errE = cfg->scan_cycle_err;
    }*/

    if (mask.s.voq_pcnt_ovfl) {
        intr_en.bf.voq_pcnt_ovflE = cfg->voq_pcnt_ovfl;
    }

    if (mask.s.voq_pcnt_udfl) {
        intr_en.bf.voq_pcnt_udflE = cfg->voq_pcnt_udfl;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE(intr_en.wrd, TE_CB_INTE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(intr_en.wrd, TE_CB_INTE);
#endif
    }
    return ret;

}

ca_status_t  aal_l3_te_cb_intr_ena_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT aal_l3_te_cb_intr_t       *cfg
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_INTE_t intr_en;

    if (cfg == NULL) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        intr_en.wrd = CA_NE_REG_READ(TE_CB_INTE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        intr_en.wrd = CA_8279_NE_REG_READ(TE_CB_INTE);
#endif
    }
    cfg->port_freecnt_ovfl  = intr_en.bf.port_freecnt_ovflE ;
    cfg->port_freecnt_udfl  = intr_en.bf.port_freecnt_udflE ;
    cfg->voq_bufcnt_ovfl    = intr_en.bf.voq_bufcnt_ovflE   ;
    cfg->voq_bufcnt_udfl    = intr_en.bf.voq_bufcnt_udflE   ;
    //cfg->scan_cycle_err     = intr_en.bf.scan_cycle_errE    ;
    cfg->voq_pcnt_ovfl      = intr_en.bf.voq_pcnt_ovflE     ;
    cfg->voq_pcnt_udfl      = intr_en.bf.voq_pcnt_udflE     ;

    return ret;
}

ca_status_t  aal_l3_te_cb_intr_status_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_l3_te_cb_intr_mask_t  mask  ,
    CA_IN aal_l3_te_cb_intr_t       *cfg
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_INT_t intr_status;

    if (cfg == NULL) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        intr_status.wrd = CA_NE_REG_READ(TE_CB_INT);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        intr_status.wrd = CA_8279_NE_REG_READ(TE_CB_INT);
#endif
    }

    if (mask.s.port_freecnt_ovfl) {
        intr_status.bf.port_freecnt_ovfl = cfg->port_freecnt_ovfl;
    }
    if (mask.s.port_freecnt_udfl) {
        intr_status.bf.port_freecnt_udfl = cfg->port_freecnt_udfl;
    }
    if (mask.s.voq_bufcnt_ovfl) {
        intr_status.bf.voq_bufcnt_ovfl = cfg->voq_bufcnt_ovfl;
    }
    if (mask.s.voq_bufcnt_udfl) {
        intr_status.bf.voq_bufcnt_udfl = cfg->voq_bufcnt_udfl;
    }
   /* if (mask.s.scan_cycle_err) {
        intr_status.bf.scan_cycle_err = cfg->scan_cycle_err;
    }*/

    if (mask.s.voq_pcnt_ovfl) {
        intr_status.bf.voq_pcnt_ovfl = cfg->voq_pcnt_ovfl;
    }

    if (mask.s.voq_pcnt_udfl) {
        intr_status.bf.voq_pcnt_udfl = cfg->voq_pcnt_udfl;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE(intr_status.wrd, TE_CB_INT);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(intr_status.wrd, TE_CB_INT);
#endif
    }
    return ret;

}

ca_status_t  aal_l3_te_cb_intr_status_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT aal_l3_te_cb_intr_t       *cfg
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_INT_t intr_status;

    if (cfg == NULL) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        intr_status.wrd = CA_NE_REG_READ(TE_CB_INT);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        intr_status.wrd = CA_8279_NE_REG_READ(TE_CB_INT);
#endif
    }
    cfg->port_freecnt_ovfl  = intr_status.bf.port_freecnt_ovfl ;
    cfg->port_freecnt_udfl  = intr_status.bf.port_freecnt_udfl ;
    cfg->voq_bufcnt_ovfl    = intr_status.bf.voq_bufcnt_ovfl   ;
    cfg->voq_bufcnt_udfl    = intr_status.bf.voq_bufcnt_udfl   ;
   // cfg->scan_cycle_err     = intr_status.bf.scan_cycle_err    ;
    cfg->voq_pcnt_ovfl      = intr_status.bf.voq_pcnt_ovfl     ;
    cfg->voq_pcnt_udfl      = intr_status.bf.voq_pcnt_udfl     ;

    return ret;

}

ca_status_t aal_l3_te_cb_err_info_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT aal_l3_te_cb_err_info_t    *err_info
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_ERR_INFO_t temp_err_info;

    if (err_info == NULL) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        temp_err_info.wrd = CA_NE_REG_READ(TE_CB_ERR_INFO);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        temp_err_info.wrd = CA_8279_NE_REG_READ(TE_CB_ERR_INFO);
#endif
    }
    err_info->voq_of_buf_err = (ca_uint16_t)(temp_err_info.bf.voq_of_buferr);
    err_info->voq_of_pkt_err = (ca_uint16_t)(temp_err_info.bf.voq_of_pkterr);

    return ret;

}

ca_status_t aal_l3_te_cb_ctrl_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN aal_l3_te_cb_ctrl_mask_t    mask,
    CA_IN aal_l3_te_cb_ctrl_t         *cfg
)
{
    ca_status_t     ret = CA_E_OK;
    TE_CB_CTRL_t    cb_ctrl;

    if (cfg == NULL) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        cb_ctrl.wrd = CA_NE_REG_READ(TE_CB_CTRL);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        cb_ctrl.wrd = CA_8279_NE_REG_READ(TE_CB_CTRL);
#endif
    }

    if (mask.s.scan_ena) {
        cb_ctrl.bf.scan_enable = cfg->scan_ena;
    }
    if(mask.s.scan_cycle_unit){
        cb_ctrl.bf.scan_cycle_unit = cfg->scan_cycle_unit;
    }
    if (mask.s.scan_cycle_per_run) {
        cb_ctrl.bf.scan_cycle_per_run = cfg->scan_cycle_per_run;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE(cb_ctrl.wrd, TE_CB_CTRL);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(cb_ctrl.wrd, TE_CB_CTRL);
#endif
    }
    return ret;
}

ca_status_t aal_l3_te_cb_ctrl_get(
    CA_IN ca_device_id_t                 device_id,
    CA_OUT aal_l3_te_cb_ctrl_t         *cfg
)
{
    ca_status_t     ret = CA_E_OK;
    TE_CB_CTRL_t    cb_ctrl;

    if (cfg == NULL) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        cb_ctrl.wrd = CA_NE_REG_READ(TE_CB_CTRL);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        cb_ctrl.wrd = CA_8279_NE_REG_READ(TE_CB_CTRL);
#endif
    }
    cfg->scan_ena = cb_ctrl.bf.scan_enable ;
    cfg->scan_cycle_per_run = cb_ctrl.bf.scan_cycle_per_run;
    cfg->scan_cycle_unit = cb_ctrl.bf.scan_cycle_unit ;

    return ret;
}
ca_status_t aal_l3_te_cb_td_sts_get(
    CA_IN ca_device_id_t   device_id,
    CA_OUT aal_l3_te_cb_td_sts_t *sts
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_GLOB_TD_STS_t  glb_td_sts;
    TE_CB_PORT_TD_STS_t  port_td_sts;
    TE_CB_VOQ_TD_STS0_t  voq_td_sts[TE_CB_VOQ_TD_STS0_COUNT];
    TE_CB_SRC_PRI_TD_STS_t src_pri_td_sts;
    ca_uint8_t  i;

    if (sts == NULL) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        glb_td_sts.wrd = CA_NE_REG_READ(TE_CB_GLOB_TD_STS);
        port_td_sts.wrd = CA_NE_REG_READ(TE_CB_PORT_TD_STS);
        for (i = 0; i < TE_CB_VOQ_TD_STS0_COUNT; i++) {
            voq_td_sts[i].wrd = CA_NE_REG_READ(TE_CB_VOQ_TD_STS0 + i * TE_CB_VOQ_TD_STS0_STRIDE);
        }
        src_pri_td_sts.wrd = CA_NE_REG_READ(TE_CB_SRC_PRI_TD_STS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        glb_td_sts.wrd = CA_8279_NE_REG_READ(TE_CB_GLOB_TD_STS);
        port_td_sts.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_TD_STS);
        for (i = 0; i < TE_CB_VOQ_TD_STS0_COUNT; i++) {
            voq_td_sts[i].wrd = CA_8279_NE_REG_READ(TE_CB_VOQ_TD_STS0 + i * TE_CB_VOQ_TD_STS0_STRIDE);
        }
        src_pri_td_sts.wrd = CA_8279_NE_REG_READ(TE_CB_SRC_PRI_TD_STS);
#endif
    }
    sts->global_drop_pool_bmp   = glb_td_sts.bf.pool;
    sts->spid_drop_bmp          = port_td_sts.bf.src_port;
    sts->dpid_drop_bmp          = port_td_sts.bf.port;
    sts->voq0_31_drop_bmp       = voq_td_sts[0].wrd;
    sts->voq32_63_drop_bmp      = voq_td_sts[1].wrd;
    sts->voq64_95_drop_bmp      = voq_td_sts[2].wrd;
    sts->voq96_127_drop_bmp     = voq_td_sts[3].wrd;
    sts->wan_src_pri_drop_cnt_bmp  = src_pri_td_sts.bf.src_pri;

    return ret;
}
ca_status_t aal_l3_te_cb_no_prvt_buf_sts_get(
    CA_IN ca_device_id_t                       device_id,
    CA_OUT aal_l3_te_cb_no_prvt_buf_sts_t   *sts
)
{
    TE_CB_VOQ_NO_PRVT_STS0_t no_buf_bmp[TE_CB_VOQ_NO_PRVT_STS0_COUNT];
    ca_uint8_t  i;

    if (sts == NULL) {
        return CA_E_PARAM;
    }
    for (i = 0; i < TE_CB_VOQ_NO_PRVT_STS0_COUNT; i++) {
        if(0 == device_id){
            no_buf_bmp[i].wrd = CA_NE_REG_READ(TE_CB_VOQ_NO_PRVT_STS0 + i * TE_CB_VOQ_NO_PRVT_STS0_STRIDE);
        }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
            no_buf_bmp[i].wrd = CA_8279_NE_REG_READ(TE_CB_VOQ_NO_PRVT_STS0 + i * TE_CB_VOQ_NO_PRVT_STS0_STRIDE);
#endif
        }
    }
    sts->voq0_31_bmp    = no_buf_bmp[0].wrd;
    sts->voq32_63_bmp   = no_buf_bmp[1].wrd;
    sts->voq64_95_bmp   = no_buf_bmp[2].wrd;
    sts->voq96_127_bmp  = no_buf_bmp[3].wrd;

    return CA_E_OK;
}


ca_status_t aal_l3_te_cb_pool_profile_set(
   CA_IN ca_device_id_t                 device_id,
   CA_IN ca_uint8_t                  eq_id,
   CA_IN aal_l3_te_cb_eq_threshold_t *cfg
)
{
    ca_status_t     ret = CA_E_OK;
    TE_CB_EQ_PROFILE_THRSH0_t  pool_thrsh;
    if (NULL == cfg) {
        return CA_E_PARAM;
    }
    if (eq_id >= (TE_CB_EQ_PROFILE_THRSH0_COUNT *2)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        pool_thrsh.wrd = CA_NE_REG_READ(TE_CB_EQ_PROFILE_THRSH0 + (eq_id/2) * TE_CB_EQ_PROFILE_THRSH0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        pool_thrsh.wrd = CA_8279_NE_REG_READ(TE_CB_EQ_PROFILE_THRSH0 + (eq_id/2) * TE_CB_EQ_PROFILE_THRSH0_STRIDE);
#endif
    }

    if(eq_id & 0x01){
        pool_thrsh.bf.th1 = cfg->threshold;
    }else{
        pool_thrsh.bf.th0 = cfg->threshold;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE(pool_thrsh.wrd, TE_CB_EQ_PROFILE_THRSH0 + (eq_id/2) * TE_CB_EQ_PROFILE_THRSH0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(pool_thrsh.wrd, TE_CB_EQ_PROFILE_THRSH0 + (eq_id/2) * TE_CB_EQ_PROFILE_THRSH0_STRIDE);
#endif
    }
    return ret;
}

ca_status_t aal_l3_te_cb_pool_profile_get(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_uint8_t                  eq_id,
    CA_OUT aal_l3_te_cb_eq_threshold_t    *cfg
)

{
    ca_status_t     ret = CA_E_OK;
    TE_CB_EQ_PROFILE_THRSH0_t  pool_thrsh;
    if (NULL == cfg) {
        return CA_E_PARAM;
    }
    if (eq_id >= (TE_CB_EQ_PROFILE_THRSH0_COUNT *2)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        pool_thrsh.wrd = CA_NE_REG_READ(TE_CB_EQ_PROFILE_THRSH0 + (eq_id/2) * TE_CB_EQ_PROFILE_THRSH0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        pool_thrsh.wrd = CA_8279_NE_REG_READ(TE_CB_EQ_PROFILE_THRSH0 + (eq_id/2) * TE_CB_EQ_PROFILE_THRSH0_STRIDE);
#endif
    }
    if(eq_id & 0x01){
        cfg->threshold = pool_thrsh.bf.th1;
    }else{
        cfg->threshold = pool_thrsh.bf.th0;
    }

    return ret;
}

ca_status_t aal_l3_te_cb_port_threshold_profile_set(
    CA_IN ca_device_id_t                           device_id,
    CA_IN ca_uint8_t                            profile_id,
    CA_IN aal_l3_te_cb_comb_threshold_mask_t    mask,
    CA_IN aal_l3_te_cb_comb_threshold_t         *cfg
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_PORT_THRSH_PROFILE0_t      port_thred;
    TE_CB_PORT_GRPTHRSH_PROFILE0_t   grp_thred;

    if (profile_id >= TE_CB_PORT_GRPTHRSH_PROFILE0_COUNT) {
        return CA_E_PARAM;
    }

    if (NULL == cfg) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        port_thred.wrd = CA_NE_REG_READ(TE_CB_PORT_THRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
        grp_thred.wrd = CA_NE_REG_READ(TE_CB_PORT_GRPTHRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        port_thred.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_THRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
        grp_thred.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_GRPTHRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
#endif
    }

    if (mask.s.threshold_lo) {
        port_thred.bf.lth = cfg->threshold_lo;
    }
    if (mask.s.threshold_hi) {
        port_thred.bf.hth = cfg->threshold_hi;
    }
    if (mask.s.grp_threshold_lo) {
        grp_thred.bf.grplth = cfg->grp_threshold_lo;
    }
    if (mask.s.grp_threshold_hi) {
        grp_thred.bf.grphth = cfg->grp_threshold_hi;
    }
    if(0 == device_id){
        CA_NE_REG_WRITE(port_thred.wrd, TE_CB_PORT_THRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
        CA_NE_REG_WRITE(grp_thred.wrd, TE_CB_PORT_GRPTHRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(port_thred.wrd, TE_CB_PORT_THRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
        CA_8279_NE_REG_WRITE(grp_thred.wrd, TE_CB_PORT_GRPTHRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
#endif
    }
    return ret;

}

ca_status_t aal_l3_te_cb_port_threshold_profile_get(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                profile_id,
    CA_OUT aal_l3_te_cb_comb_threshold_t  *cfg
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_PORT_THRSH_PROFILE0_t      port_thred;
    TE_CB_PORT_GRPTHRSH_PROFILE0_t   grp_thred;

    if (profile_id >= TE_CB_PORT_GRPTHRSH_PROFILE0_COUNT) {
        return CA_E_PARAM;
    }

    if (NULL == cfg) {
        return CA_E_PARAM;
    }
    if(0 == device_id){
        port_thred.wrd = CA_NE_REG_READ(TE_CB_PORT_THRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
        grp_thred.wrd = CA_NE_REG_READ(TE_CB_PORT_GRPTHRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        port_thred.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_THRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
        grp_thred.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_GRPTHRSH_PROFILE0 + profile_id * TE_CB_PORT_GRPTHRSH_PROFILE0_STRIDE);
#endif
    }
    cfg->threshold_lo     = port_thred.bf.lth  ;
    cfg->threshold_hi     = port_thred.bf.hth  ;
    cfg->grp_threshold_lo = grp_thred.bf.grplth;
    cfg->grp_threshold_hi = grp_thred.bf.grphth;

    return ret;

}


ca_status_t aal_l3_te_cb_port_profile_sel_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                port_id,
    CA_IN ca_uint8_t                profile_id
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_PORT_THRSH_SELECT_t thd_sel;

    if ((port_id >= __AAL_L3_TE_CB_PORT_NUM) || (profile_id >= TE_CB_PORT_GRPTHRSH_PROFILE0_COUNT)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        thd_sel.wrd = CA_NE_REG_READ(TE_CB_PORT_THRSH_SELECT);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        thd_sel.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_THRSH_SELECT);
#endif
    }
    thd_sel.wrd &= ~(__AAL_L3_TE_CB_PORT_SEL_MAX << (port_id * 2));

    thd_sel.wrd |= (profile_id << (port_id * 2));

    if(0 == device_id){
        CA_NE_REG_WRITE(thd_sel.wrd, TE_CB_PORT_THRSH_SELECT);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(thd_sel.wrd, TE_CB_PORT_THRSH_SELECT);
#endif
    }
    return ret;
}

ca_status_t aal_l3_te_cb_port_profile_sel_get(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                port_id,
    CA_OUT ca_uint8_t               *profile_id
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_PORT_THRSH_SELECT_t thd_sel;

    if ((port_id >= __AAL_L3_TE_CB_PORT_NUM) || (profile_id == NULL)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        thd_sel.wrd = CA_NE_REG_READ(TE_CB_PORT_THRSH_SELECT);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        thd_sel.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_THRSH_SELECT);
#endif
    }
    *profile_id = (thd_sel.wrd >> (port_id * 2)) & __AAL_L3_TE_CB_PORT_SEL_MAX;

    return ret;
}


ca_status_t aal_l3_te_cb_voq_threshold_profile_set(
    CA_IN ca_device_id_t                           device_id,
    CA_IN ca_uint8_t                            profile_id,
    CA_IN aal_l3_te_cb_comb_threshold_mask_t    mask,
    CA_IN aal_l3_te_cb_comb_threshold_t         *cfg
)
{
    ca_status_t   ret = CA_E_OK;
    TE_CB_VOQ_THRSH_PROFILE_MEM_DATA2_t voq_prf2;
    TE_CB_VOQ_THRSH_PROFILE_MEM_DATA1_t voq_prf1;
    TE_CB_VOQ_THRSH_PROFILE_MEM_DATA0_t voq_prf0;

    if ((profile_id >= 8) || (NULL == cfg)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        READ_INDIRCT_REG(device_id, profile_id, TE_CB_VOQ_THRSH_PROFILE_MEM_ACCESS);
        voq_prf2.wrd = CA_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA2);
        voq_prf1.wrd = CA_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA1);
        voq_prf0.wrd = CA_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA0);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_READ_INDIRCT_REG(device_id, profile_id, TE_CB_VOQ_THRSH_PROFILE_MEM_ACCESS);
        voq_prf2.wrd = CA_8279_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA2);
        voq_prf1.wrd = CA_8279_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA1);
        voq_prf0.wrd = CA_8279_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA0);
#endif
    }
    if (mask.s.threshold_lo) {
        voq_prf0.bf.lth = cfg->threshold_lo;
    }
    if (mask.s.threshold_hi) {
        voq_prf0.bf.hth = cfg->threshold_hi;
    }
    if (mask.s.grp_threshold_lo) {
        voq_prf1.bf.grplth = (cfg->grp_threshold_lo >> 4) & 0x3ff ;
        voq_prf0.bf.grplth = (cfg->grp_threshold_lo & 0xf) ;
    }
    if (mask.s.grp_threshold_hi) {
        voq_prf1.bf.grphth = cfg->grp_threshold_hi;
    }
    if (mask.s.prvt) {
        voq_prf1.bf.prvt = (cfg->prvt & 0xff);
        voq_prf2.bf.prvt = (cfg->prvt  >> 8) & 0x3f;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE(voq_prf2.wrd, TE_CB_VOQ_THRSH_PROFILE_MEM_DATA2);
        CA_NE_REG_WRITE(voq_prf1.wrd, TE_CB_VOQ_THRSH_PROFILE_MEM_DATA1);
        CA_NE_REG_WRITE(voq_prf0.wrd, TE_CB_VOQ_THRSH_PROFILE_MEM_DATA0);
        WRITE_INDIRCT_REG(device_id, profile_id, TE_CB_VOQ_THRSH_PROFILE_MEM_ACCESS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(voq_prf2.wrd, TE_CB_VOQ_THRSH_PROFILE_MEM_DATA2);
        CA_8279_NE_REG_WRITE(voq_prf1.wrd, TE_CB_VOQ_THRSH_PROFILE_MEM_DATA1);
        CA_8279_NE_REG_WRITE(voq_prf0.wrd, TE_CB_VOQ_THRSH_PROFILE_MEM_DATA0);
        CA_8279_WRITE_INDIRCT_REG(device_id, profile_id, TE_CB_VOQ_THRSH_PROFILE_MEM_ACCESS);
#endif
    }
    return ret;
}

ca_status_t aal_l3_te_cb_voq_threshold_profile_get(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                profile_id,
    CA_OUT aal_l3_te_cb_comb_threshold_t  *cfg
)
{
    TE_CB_VOQ_THRSH_PROFILE_MEM_DATA2_t voq_prf2;
    TE_CB_VOQ_THRSH_PROFILE_MEM_DATA1_t voq_prf1;
    TE_CB_VOQ_THRSH_PROFILE_MEM_DATA0_t voq_prf0;

    if ((profile_id >= 8) || (NULL == cfg)) {
        return CA_E_PARAM;
    }
    if(0 == device_id){
        READ_INDIRCT_REG(device_id, profile_id, TE_CB_VOQ_THRSH_PROFILE_MEM_ACCESS);
        voq_prf2.wrd = CA_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA2);
        voq_prf1.wrd = CA_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA1);
        voq_prf0.wrd = CA_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA0);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_READ_INDIRCT_REG(device_id, profile_id, TE_CB_VOQ_THRSH_PROFILE_MEM_ACCESS);
        voq_prf2.wrd = CA_8279_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA2);
        voq_prf1.wrd = CA_8279_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA1);
        voq_prf0.wrd = CA_8279_NE_REG_READ(TE_CB_VOQ_THRSH_PROFILE_MEM_DATA0);
#endif
    }
    cfg->threshold_lo = voq_prf0.bf.lth;
    cfg->threshold_hi = voq_prf0.bf.hth;
    cfg->grp_threshold_lo = (voq_prf1.bf.grplth << 4) | voq_prf0.bf.grplth;
    cfg->grp_threshold_hi = voq_prf1.bf.grphth;
    cfg->prvt = voq_prf1.bf.prvt | (voq_prf2.bf.prvt << 8);

    return CA_E_OK;
}


ca_status_t aal_l3_te_cb_voq_profile_sel_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                voq_id,
    CA_IN ca_uint8_t                profile_id
)
{
    ca_status_t   ret = CA_E_OK;
    if ((voq_id >= __AAL_L3_TE_CB_VOQ_NUM) || (profile_id >= 8)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE((ca_uint32_t)profile_id, TE_CB_VOQ_THRSH_SELECT_MEM_DATA);
        WRITE_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_THRSH_SELECT_MEM_ACCESS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE((ca_uint32_t)profile_id, TE_CB_VOQ_THRSH_SELECT_MEM_DATA);
        CA_8279_WRITE_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_THRSH_SELECT_MEM_ACCESS);
#endif
    }
    return ret;
}


ca_status_t aal_l3_te_cb_voq_profile_sel_get(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                voq_id,
    CA_OUT ca_uint8_t               *profile_id
)
{
    ca_status_t   ret = CA_E_OK;

    if ((voq_id >= __AAL_L3_TE_CB_VOQ_NUM) || (profile_id == NULL)) {
        return CA_E_PARAM;
    }
    if(0 == device_id){
        READ_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_THRSH_SELECT_MEM_ACCESS);
        *profile_id = (ca_uint8_t)CA_NE_REG_READ(TE_CB_VOQ_THRSH_SELECT_MEM_DATA);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_READ_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_THRSH_SELECT_MEM_ACCESS);
        *profile_id = (ca_uint8_t)CA_8279_NE_REG_READ(TE_CB_VOQ_THRSH_SELECT_MEM_DATA);
#endif
    }
    return ret;
}


ca_status_t aal_l3_te_cb_port_free_buf_cnt_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    port_id,
    CA_IN aal_l3_te_cb_free_buf_cnt_mask_t mask,
    CA_IN aal_l3_te_cb_free_buf_cnt_t  *cfg
)
{
    ca_status_t   ret = CA_E_OK;

    TE_CB_PORT_FREECNT_MEM_DATA_t port_free_buf_cnt;

    if ((port_id >= __AAL_L3_TE_CB_PORT_NUM) || (NULL == cfg)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        READ_INDIRCT_REG(device_id, port_id, TE_CB_PORT_FREECNT_MEM_ACCESS);
        port_free_buf_cnt.wrd = CA_NE_REG_READ(TE_CB_PORT_FREECNT_MEM_DATA);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_READ_INDIRCT_REG(device_id, port_id, TE_CB_PORT_FREECNT_MEM_ACCESS);
        port_free_buf_cnt.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_FREECNT_MEM_DATA);
#endif
    }
    if(mask.s.cnt0_msb){
        port_free_buf_cnt.bf.cnt0_msb   = cfg->cnt0_msb;
    }

    if(mask.s.cnt0){
        port_free_buf_cnt.bf.cnt0       = cfg->cnt0;
    }

    if(mask.s.cnt1_msb){
        port_free_buf_cnt.bf.cnt1_msb   = cfg->cnt1_msb;
    }

    if(mask.s.cnt1){
        port_free_buf_cnt.bf.cnt1       = cfg->cnt1;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE(port_free_buf_cnt.wrd, TE_CB_PORT_FREECNT_MEM_DATA);

        WRITE_INDIRCT_REG(device_id, port_id, TE_CB_PORT_FREECNT_MEM_ACCESS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(port_free_buf_cnt.wrd, TE_CB_PORT_FREECNT_MEM_DATA);

        CA_8279_WRITE_INDIRCT_REG(device_id, port_id, TE_CB_PORT_FREECNT_MEM_ACCESS);
#endif

    }
    return ret;
}



ca_status_t aal_l3_te_cb_port_free_buf_cnt_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    port_id,
    CA_OUT aal_l3_te_cb_free_buf_cnt_t  *cfg
)
{
    ca_status_t   ret = CA_E_OK;

    TE_CB_PORT_FREECNT_MEM_DATA_t port_free_buf_cnt;

    if ((port_id >= __AAL_L3_TE_CB_PORT_NUM) || (NULL == cfg)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        READ_INDIRCT_REG(device_id, port_id, TE_CB_PORT_FREECNT_MEM_ACCESS);
        port_free_buf_cnt.wrd = CA_NE_REG_READ(TE_CB_PORT_FREECNT_MEM_DATA);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_READ_INDIRCT_REG(device_id, port_id, TE_CB_PORT_FREECNT_MEM_ACCESS);
        port_free_buf_cnt.wrd = CA_8279_NE_REG_READ(TE_CB_PORT_FREECNT_MEM_DATA);
#endif

    }
    cfg->cnt0_msb  = port_free_buf_cnt.bf.cnt0_msb  ;
    cfg->cnt0      = port_free_buf_cnt.bf.cnt0      ;
    cfg->cnt1_msb  = port_free_buf_cnt.bf.cnt1_msb  ;
    cfg->cnt1      = port_free_buf_cnt.bf.cnt1      ;

    return ret;
}

ca_status_t aal_l3_te_cb_voq_buf_cnt_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    voq_id,
    CA_IN aal_l3_te_cb_free_buf_cnt_t  *cfg
)
{
    ca_status_t   ret = CA_E_OK;

    TE_CB_VOQ_BUFCNT_MEM_DATA_t voq_buf_cnt;

    if ((voq_id >= __AAL_L3_TE_CB_VOQ_NUM) || (NULL == cfg)) {
        return CA_E_PARAM;
    }

    voq_buf_cnt.wrd = 0;
    voq_buf_cnt.bf.cnt0_rsvd   = cfg->cnt0_msb;
    voq_buf_cnt.bf.cnt0       = cfg->cnt0;
    voq_buf_cnt.bf.cnt1_rsvd   = cfg->cnt1_msb;
    voq_buf_cnt.bf.cnt1       = cfg->cnt1;

    if(0 == device_id){
        CA_NE_REG_WRITE(voq_buf_cnt.wrd, TE_CB_VOQ_BUFCNT_MEM_DATA);
        WRITE_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_BUFCNT_MEM_ACCESS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(voq_buf_cnt.wrd, TE_CB_VOQ_BUFCNT_MEM_DATA);
        CA_8279_WRITE_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_BUFCNT_MEM_ACCESS);
#endif

    }
    return ret;
}


ca_status_t aal_l3_te_cb_voq_buf_cnt_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    voq_id,
    CA_OUT aal_l3_te_cb_free_buf_cnt_t  *cfg
)
{
    ca_status_t   ret = CA_E_OK;

    TE_CB_VOQ_BUFCNT_MEM_DATA_t voq_buf_cnt;

    if ((voq_id >= __AAL_L3_TE_CB_VOQ_NUM) || (NULL == cfg)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        READ_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_BUFCNT_MEM_ACCESS);
        voq_buf_cnt.wrd = CA_NE_REG_READ(TE_CB_VOQ_BUFCNT_MEM_DATA);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        READ_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_BUFCNT_MEM_ACCESS);
        voq_buf_cnt.wrd = CA_NE_REG_READ(TE_CB_VOQ_BUFCNT_MEM_DATA);
#endif
    }
    cfg->cnt0_msb   = voq_buf_cnt.bf.cnt0_rsvd;
    cfg->cnt0   = voq_buf_cnt.bf.cnt0    ;
    cfg->cnt1_msb   = voq_buf_cnt.bf.cnt1_rsvd;
    cfg->cnt1   = voq_buf_cnt.bf.cnt1    ;

    return ret;
}



ca_status_t aal_l3_te_cb_voq_max_buf_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    voq_id,
    CA_IN ca_uint16_t                   cfg
)
{
    ca_status_t   ret = CA_E_OK;

    if ((voq_id >= __AAL_L3_TE_CB_VOQ_NUM) || (cfg >= __AAL_L3_TE_CB_BUF_NUM)) {
        return CA_E_PARAM;
    }
    if(0 == device_id){
        CA_NE_REG_WRITE((ca_uint32_t)cfg, TE_CB_VOQ_MAX_BUFCNT_MEM_DATA);
        WRITE_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_MAX_BUFCNT_MEM_ACCESS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE((ca_uint32_t)cfg, TE_CB_VOQ_MAX_BUFCNT_MEM_DATA);
        CA_8279_WRITE_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_MAX_BUFCNT_MEM_ACCESS);
#endif
    }
    return ret;
}

ca_status_t aal_l3_te_cb_voq_max_buf_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    voq_id,
    CA_OUT ca_uint16_t                  *cfg
)
{
    ca_status_t   ret = CA_E_OK;
    if ((voq_id >= __AAL_L3_TE_CB_VOQ_NUM) || (cfg == NULL)) {
        return CA_E_PARAM;
    }
    if(0 == device_id){
        READ_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_MAX_BUFCNT_MEM_ACCESS);
        *cfg = (ca_uint16_t)CA_NE_REG_READ(TE_CB_VOQ_MAX_BUFCNT_MEM_DATA);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_READ_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_MAX_BUFCNT_MEM_ACCESS);
        *cfg = (ca_uint16_t)CA_8279_NE_REG_READ(TE_CB_VOQ_MAX_BUFCNT_MEM_DATA);
#endif
    }
    return ret;
}


ca_status_t aal_l3_te_cb_pri_threshold_set(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                pri,
    CA_IN aal_l3_te_cb_threshold_t  *cfg
)
{
    TE_CB_GLB0_THRSH_t pri_thrsh;
    if((pri >= TE_CB_GLB0_THRSH_COUNT) || (NULL == cfg)){
        return CA_E_PARAM;
    }
    pri_thrsh.wrd = 0;
    pri_thrsh.bf.hth = cfg->threshold_hi;
    pri_thrsh.bf.lth = cfg->threshold_lo;

    if(0 == device_id){
        CA_NE_REG_WRITE(pri_thrsh.wrd,TE_CB_GLB0_THRSH + pri*TE_CB_GLB0_THRSH_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(pri_thrsh.wrd,TE_CB_GLB0_THRSH + pri*TE_CB_GLB0_THRSH_STRIDE);
#endif

    }
    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_pri_threshold_get(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                pri,
    CA_OUT aal_l3_te_cb_threshold_t  *cfg
)
{
    TE_CB_GLB0_THRSH_t pri_thrsh;
    if((pri >= TE_CB_GLB0_THRSH_COUNT) || (NULL == cfg)){
        return CA_E_PARAM;
    }

    if(0 == device_id){
        pri_thrsh.wrd = CA_NE_REG_READ(TE_CB_GLB0_THRSH + pri*TE_CB_GLB0_THRSH_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        pri_thrsh.wrd = CA_8279_NE_REG_READ(TE_CB_GLB0_THRSH + pri*TE_CB_GLB0_THRSH_STRIDE);
#endif
    }
    cfg->threshold_hi = pri_thrsh.bf.hth;
    cfg->threshold_lo = pri_thrsh.bf.lth;

    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_src_ctrl_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_boolean_t              wan_src_pri
)
{
    TE_CB_SRC_CTRL_t src_ctrl;

    src_ctrl.bf.src_pri_en = (wan_src_pri>0);

    if(0 == device_id){
        CA_NE_REG_WRITE(src_ctrl.wrd , TE_CB_SRC_CTRL);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(src_ctrl.wrd , TE_CB_SRC_CTRL);
#endif
    }
    return CA_E_OK;
}


ca_status_t aal_l3_te_cb_src_ctrl_get(
    CA_IN ca_device_id_t               device_id,
    CA_OUT ca_boolean_t             *wan_src_pri
)
{
    TE_CB_SRC_CTRL_t src_ctrl;
    if(wan_src_pri == NULL){
        return CA_E_PARAM;
    }
    if(0 == device_id){
        src_ctrl.wrd = CA_NE_REG_READ( TE_CB_SRC_CTRL);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        src_ctrl.wrd = CA_8279_NE_REG_READ( TE_CB_SRC_CTRL);
#endif

    }
    *wan_src_pri = src_ctrl.bf.src_pri_en;
    return CA_E_OK;
}


ca_status_t aal_l3_te_cb_src_port_threshold_set(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                port_id,
    CA_IN aal_l3_te_cb_threshold_t  *cfg
)
{
    TE_CB_SRC_PORT_THRSH0_t port_thrsh;

    if((port_id >= TE_CB_SRC_PORT_THRSH0_COUNT )||(cfg == NULL)){
        return CA_E_PARAM;
    }
    port_thrsh.wrd = 0;
    port_thrsh.bf.hth = cfg->threshold_hi;
    port_thrsh.bf.lth = cfg->threshold_lo;

    if(0 == device_id){
        CA_NE_REG_WRITE(port_thrsh.wrd,TE_CB_SRC_PORT_THRSH0 + port_id*TE_CB_SRC_PORT_THRSH0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(port_thrsh.wrd,TE_CB_SRC_PORT_THRSH0 + port_id*TE_CB_SRC_PORT_THRSH0_STRIDE);
#endif

    }
    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_src_port_threshold_get(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                port_id,
    CA_OUT aal_l3_te_cb_threshold_t  *cfg
)
{
    TE_CB_SRC_PORT_THRSH0_t pri_thrsh;
    if((port_id >= TE_CB_SRC_PORT_THRSH0_COUNT )||(cfg == NULL)){
           return CA_E_PARAM;
    }

    if(0 == device_id){
        pri_thrsh.wrd = CA_NE_REG_READ(TE_CB_SRC_PORT_THRSH0 + port_id*TE_CB_SRC_PORT_THRSH0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        pri_thrsh.wrd = CA_8279_NE_REG_READ(TE_CB_SRC_PORT_THRSH0 + port_id*TE_CB_SRC_PORT_THRSH0_STRIDE);
#endif

    }
    cfg->threshold_hi = pri_thrsh.bf.hth;
    cfg->threshold_lo = pri_thrsh.bf.lth;

    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_src_pri_threshold_set(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                pri,
    CA_IN aal_l3_te_cb_threshold_t  *cfg
)
{
    TE_CB_SRC_PRI_THRSH0_t  src_pri_thrsh;
    if((pri>=TE_CB_SRC_PRI_THRSH0_COUNT)||(NULL == cfg)){
        return CA_E_PARAM;
    }
    src_pri_thrsh.wrd = 0;
    src_pri_thrsh.bf.hth = cfg->threshold_hi;
    src_pri_thrsh.bf.lth = cfg->threshold_lo;

    if(0 == device_id){
        CA_NE_REG_WRITE(src_pri_thrsh.wrd,TE_CB_SRC_PRI_THRSH0 + pri*TE_CB_SRC_PRI_THRSH0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(src_pri_thrsh.wrd,TE_CB_SRC_PRI_THRSH0 + pri*TE_CB_SRC_PRI_THRSH0_STRIDE);
#endif

    }
    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_src_pri_threshold_get(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                pri,
    CA_OUT aal_l3_te_cb_threshold_t  *cfg
)
{
    TE_CB_SRC_PRI_THRSH0_t  src_pri_thrsh;
    if((pri>=TE_CB_SRC_PRI_THRSH0_COUNT)||(NULL == cfg)){
        return CA_E_PARAM;
    }

    if(0 == device_id){
        src_pri_thrsh.wrd = CA_NE_REG_READ(TE_CB_SRC_PRI_THRSH0 + pri*TE_CB_SRC_PRI_THRSH0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        src_pri_thrsh.wrd = CA_8279_NE_REG_READ(TE_CB_SRC_PRI_THRSH0 + pri*TE_CB_SRC_PRI_THRSH0_STRIDE);
#endif
    }
    cfg->threshold_hi = src_pri_thrsh.bf.hth;
    cfg->threshold_lo = src_pri_thrsh.bf.lth;
    return CA_E_OK;
}


ca_status_t aal_l3_te_cb_voq_pcnt_set(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                voq_id,
    CA_IN ca_uint16_t                 cfg
)
{
    ca_status_t   ret = CA_E_OK;

       if ((voq_id >= __AAL_L3_TE_CB_VOQ_NUM) || (cfg >= __AAL_L3_TE_CB_BUF_NUM)) {
           return CA_E_PARAM;
       }

       if(0 == device_id){
           CA_NE_REG_WRITE((ca_uint32_t)cfg, TE_CB_VOQ_PCNT_MEM_DATA);
           WRITE_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_PCNT_MEM_ACCESS);
       }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
           CA_8279_NE_REG_WRITE((ca_uint32_t)cfg, TE_CB_VOQ_PCNT_MEM_DATA);
           CA_8279_WRITE_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_PCNT_MEM_ACCESS);
#endif
       }
       return ret;
}

ca_status_t aal_l3_te_cb_voq_pcnt_get(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_uint8_t                voq_id,
    CA_OUT ca_uint16_t                *cfg
)
{
    ca_status_t   ret = CA_E_OK;
      if ((voq_id >= __AAL_L3_TE_CB_VOQ_NUM) || (cfg == NULL)) {
          return CA_E_PARAM;
      }

      if(0 == device_id){
          READ_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_PCNT_MEM_ACCESS);
          *cfg = (ca_uint16_t)CA_NE_REG_READ(TE_CB_VOQ_PCNT_MEM_DATA);
      }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
          CA_8279_READ_INDIRCT_REG(device_id, voq_id, TE_CB_VOQ_PCNT_MEM_ACCESS);
          *cfg = (ca_uint16_t)CA_8279_NE_REG_READ(TE_CB_VOQ_PCNT_MEM_DATA);
#endif
      }
      return ret;
}

ca_status_t aal_l3_te_cb_src_port_buf_cnt_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    port_id,
    CA_IN ca_uint16_t                   cfg
)
{
    ca_status_t   ret = CA_E_OK;

    if ((port_id >= 2) || (cfg >= __AAL_L3_TE_CB_BUF_NUM)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE((ca_uint32_t)cfg, TE_CB_SRC_PORT_BUFCNT_MEM_DATA);
        WRITE_INDIRCT_REG(device_id, port_id, TE_CB_SRC_PORT_BUFCNT_MEM_ACCESS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE((ca_uint32_t)cfg, TE_CB_SRC_PORT_BUFCNT_MEM_DATA);
        CA_8279_WRITE_INDIRCT_REG(device_id, port_id, TE_CB_SRC_PORT_BUFCNT_MEM_ACCESS);
#endif
    }
    return ret;
}


ca_status_t aal_l3_te_cb_src_port_buf_cnt_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    port_id,
    CA_OUT ca_uint16_t                  *cfg
)
{
    ca_status_t   ret = CA_E_OK;

    if ((port_id >= 2) || (cfg == NULL)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        READ_INDIRCT_REG(device_id, port_id, TE_CB_SRC_PORT_BUFCNT_MEM_ACCESS);
        *cfg = (ca_uint16_t)CA_NE_REG_READ(TE_CB_SRC_PORT_BUFCNT_MEM_DATA);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_READ_INDIRCT_REG(device_id, port_id, TE_CB_SRC_PORT_BUFCNT_MEM_ACCESS);
        *cfg = (ca_uint16_t)CA_8279_NE_REG_READ(TE_CB_SRC_PORT_BUFCNT_MEM_DATA);
#endif
    }
    return ret ;
}

ca_status_t aal_l3_te_cb_src_pri_buf_cnt_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    pri_id,
    CA_IN ca_uint16_t                   cfg
)
{
    ca_status_t   ret = CA_E_OK;

    if ((pri_id >= 8) || (cfg >= __AAL_L3_TE_CB_BUF_NUM)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        CA_NE_REG_WRITE((ca_uint32_t)cfg, TE_CB_SRC_PRI_BUFCNT_MEM_DATA);
        WRITE_INDIRCT_REG(device_id, pri_id, TE_CB_SRC_PRI_BUFCNT_MEM_ACCESS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE((ca_uint32_t)cfg, TE_CB_SRC_PRI_BUFCNT_MEM_DATA);
        CA_8279_WRITE_INDIRCT_REG(device_id, pri_id, TE_CB_SRC_PRI_BUFCNT_MEM_ACCESS);
#endif

    }
    return ret;
}

ca_status_t aal_l3_te_cb_src_pri_buf_cnt_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    pri_id,
    CA_OUT ca_uint16_t                  *cfg
)
{
    ca_status_t   ret = CA_E_OK;

    if ((pri_id >= 8) || (cfg == NULL)) {
        return CA_E_PARAM;
    }

    if(0 == device_id){
        READ_INDIRCT_REG(device_id, pri_id, TE_CB_SRC_PRI_BUFCNT_MEM_ACCESS);
        *cfg = (ca_uint16_t)CA_NE_REG_READ(TE_CB_SRC_PRI_BUFCNT_MEM_DATA);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_READ_INDIRCT_REG(device_id, pri_id, TE_CB_SRC_PRI_BUFCNT_MEM_ACCESS);
        *cfg = (ca_uint16_t)CA_8279_NE_REG_READ(TE_CB_SRC_PRI_BUFCNT_MEM_DATA);
#endif

    }
    return ret;

}
ca_status_t aal_l3_te_cb_dqsch_port_thrsh_profile_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    profile_id,
    CA_IN aal_l3_te_cb_threshold_t        *cfg
)
{
    TE_CB_DQSCH_PORT_THRSH_PROFILE0_t port_thrsh;

    if((profile_id>=TE_CB_DQSCH_PORT_THRSH_PROFILE0_COUNT)||(NULL == cfg)){
        return CA_E_PARAM;
    }
    port_thrsh.wrd = 0;
    port_thrsh.bf.hth = cfg->threshold_hi;

    if(0 == device_id){
        CA_NE_REG_WRITE(port_thrsh.wrd,TE_CB_DQSCH_PORT_THRSH_PROFILE0 + profile_id*TE_CB_DQSCH_PORT_THRSH_PROFILE0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        CA_8279_NE_REG_WRITE(port_thrsh.wrd,TE_CB_DQSCH_PORT_THRSH_PROFILE0 + profile_id*TE_CB_DQSCH_PORT_THRSH_PROFILE0_STRIDE);
#endif

    }
    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_dqsch_port_thrsh_profile_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    profile_id,
    CA_OUT aal_l3_te_cb_threshold_t        *cfg
)
{
    TE_CB_DQSCH_PORT_THRSH_PROFILE0_t port_thrsh;

    if((profile_id>=TE_CB_DQSCH_PORT_THRSH_PROFILE0_COUNT)||(NULL == cfg)){
        return CA_E_PARAM;
    }

    if(0 == device_id){
        port_thrsh.wrd = CA_NE_REG_READ(TE_CB_DQSCH_PORT_THRSH_PROFILE0 + profile_id*TE_CB_DQSCH_PORT_THRSH_PROFILE0_STRIDE);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        port_thrsh.wrd = CA_8279_NE_REG_READ(TE_CB_DQSCH_PORT_THRSH_PROFILE0 + profile_id*TE_CB_DQSCH_PORT_THRSH_PROFILE0_STRIDE);
#endif

    }
    cfg->threshold_hi = port_thrsh.bf.hth;

    return CA_E_OK;
}
ca_status_t aal_l3_te_cb_dqsch_port_thrsh_profile_sel_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    port_id,
    CA_IN ca_uint8_t                    profile_id
)
{
    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_dqsch_port_thrsh_profile_sel_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    port_id,
    CA_OUT ca_uint8_t                   *profile_id
)
{
    return CA_E_OK;
}


ca_status_t aal_l3_te_cb_dqsch_voq_thrsh_profile_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    profile_id,
    CA_IN aal_l3_te_cb_threshold_t        *cfg
)
{
    TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM_DATA_t voq_thrsh;
    if((profile_id>7)||(NULL == cfg)){
        return CA_E_PARAM;
    }
    voq_thrsh.wrd = 0;
    voq_thrsh.bf.lth = cfg->threshold_lo;

    if(0 == device_id){
        CA_NE_REG_WRITE(voq_thrsh.wrd,TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM_DATA);
        WRITE_INDIRCT_REG(device_id, profile_id, TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM_ACCESS);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU

#endif

    }
    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_dqsch_voq_thrsh_profile_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    profile_id,
    CA_OUT aal_l3_te_cb_threshold_t        *cfg
)
{
    TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM_DATA_t voq_thrsh;
    if((profile_id>7)||(NULL == cfg)){
        return CA_E_PARAM;
    }

    if(0 == device_id){
        READ_INDIRCT_REG(device_id, profile_id, TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM_ACCESS);
        voq_thrsh.wrd  = CA_NE_REG_READ(TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM_DATA);
    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU

#endif

    }
    cfg->threshold_lo = voq_thrsh.bf.lth;

    return CA_E_OK;
}
ca_status_t aal_l3_te_cb_dqsch_voq_thrsh_profile_sel_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    voq_id,
    CA_IN ca_uint8_t                    profile_id
)
{
    return CA_E_OK;
}

ca_status_t aal_l3_te_cb_dqsch_voq_thrsh_profile_sel_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_uint8_t                    voq_id,
    CA_OUT ca_uint8_t                   *profile_id
)
{
    return CA_E_OK;
}

void aal_l3_te_cb_init_startup_config( ca_device_id_t    device_id)
{
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ0_TH0 , 4, &l3te_profile_thrsh_eq0_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ1_TH0 , 4, &l3te_profile_thrsh_eq1_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ2_TH0 , 4, &l3te_profile_thrsh_eq2_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ3_TH0 , 4, &l3te_profile_thrsh_eq3_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ4_TH0 , 4, &l3te_profile_thrsh_eq4_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ5_TH0 , 4, &l3te_profile_thrsh_eq5_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ6_TH0 , 4, &l3te_profile_thrsh_eq6_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ7_TH0 , 4, &l3te_profile_thrsh_eq7_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ0_TH1 , 4, &l3te_profile_thrsh_eq0_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ1_TH1 , 4, &l3te_profile_thrsh_eq1_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ2_TH1 , 4, &l3te_profile_thrsh_eq2_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ3_TH1 , 4, &l3te_profile_thrsh_eq3_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ4_TH1 , 4, &l3te_profile_thrsh_eq4_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ5_TH1 , 4, &l3te_profile_thrsh_eq5_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ6_TH1 , 4, &l3te_profile_thrsh_eq6_th0);
    aal_scfg_read(CFG_ID_L3TE_PROFILE_THRSH_EQ7_TH1 , 4, &l3te_profile_thrsh_eq7_th0);

    aal_scfg_read(CFG_ID_L3TE_PORT_THRSH_PROFILE_0_LTH , 4, &l3te_port_thrsh_profile_0_lth);
    aal_scfg_read(CFG_ID_L3TE_PORT_THRSH_PROFILE_1_LTH , 4, &l3te_port_thrsh_profile_1_lth);
    aal_scfg_read(CFG_ID_L3TE_PORT_THRSH_PROFILE_2_LTH , 4, &l3te_port_thrsh_profile_2_lth);
    aal_scfg_read(CFG_ID_L3TE_PORT_THRSH_PROFILE_3_LTH , 4, &l3te_port_thrsh_profile_3_lth);
    aal_scfg_read(CFG_ID_L3TE_PORT_THRSH_PROFILE_0_HTH , 4, &l3te_port_thrsh_profile_0_hth);
    aal_scfg_read(CFG_ID_L3TE_PORT_THRSH_PROFILE_1_HTH , 4, &l3te_port_thrsh_profile_1_hth);
    aal_scfg_read(CFG_ID_L3TE_PORT_THRSH_PROFILE_2_HTH , 4, &l3te_port_thrsh_profile_2_hth);
    aal_scfg_read(CFG_ID_L3TE_PORT_THRSH_PROFILE_3_HTH , 4, &l3te_port_thrsh_profile_3_hth);

    aal_scfg_read(CFG_ID_L3TE_CB_PORT_GRPTHRSH_PROFILE_0_LTH, 4, &l3te_cb_port_grpthrsh_profile_0_lth);
    aal_scfg_read(CFG_ID_L3TE_CB_PORT_GRPTHRSH_PROFILE_1_LTH, 4, &l3te_cb_port_grpthrsh_profile_1_lth);
    aal_scfg_read(CFG_ID_L3TE_CB_PORT_GRPTHRSH_PROFILE_2_LTH, 4, &l3te_cb_port_grpthrsh_profile_2_lth);
    aal_scfg_read(CFG_ID_L3TE_CB_PORT_GRPTHRSH_PROFILE_3_LTH, 4, &l3te_cb_port_grpthrsh_profile_3_lth);
    aal_scfg_read(CFG_ID_L3TE_CB_PORT_GRPTHRSH_PROFILE_0_HTH, 4, &l3te_cb_port_grpthrsh_profile_0_hth);
    aal_scfg_read(CFG_ID_L3TE_CB_PORT_GRPTHRSH_PROFILE_1_HTH, 4, &l3te_cb_port_grpthrsh_profile_1_hth);
    aal_scfg_read(CFG_ID_L3TE_CB_PORT_GRPTHRSH_PROFILE_2_HTH, 4, &l3te_cb_port_grpthrsh_profile_2_hth);
    aal_scfg_read(CFG_ID_L3TE_CB_PORT_GRPTHRSH_PROFILE_3_HTH, 4, &l3te_cb_port_grpthrsh_profile_3_hth);

    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_0_THL, 4, &l3te_voq_thrsh_profile_0_thl);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_1_THL, 4, &l3te_voq_thrsh_profile_1_thl);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_2_THL, 4, &l3te_voq_thrsh_profile_2_thl);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_3_THL, 4, &l3te_voq_thrsh_profile_3_thl);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_4_THL, 4, &l3te_voq_thrsh_profile_4_thl);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_5_THL, 4, &l3te_voq_thrsh_profile_5_thl);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_6_THL, 4, &l3te_voq_thrsh_profile_6_thl);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_7_THL, 4, &l3te_voq_thrsh_profile_7_thl);

    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_0_THH, 4, &l3te_voq_thrsh_profile_0_thh);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_1_THH, 4, &l3te_voq_thrsh_profile_1_thh);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_2_THH, 4, &l3te_voq_thrsh_profile_2_thh);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_3_THH, 4, &l3te_voq_thrsh_profile_3_thh);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_4_THH, 4, &l3te_voq_thrsh_profile_4_thh);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_5_THH, 4, &l3te_voq_thrsh_profile_5_thh);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_6_THH, 4, &l3te_voq_thrsh_profile_6_thh);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_7_THH, 4, &l3te_voq_thrsh_profile_7_thh);

    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_0_PRVT, 4, &l3te_voq_thrsh_profile_0_prvt);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_1_PRVT, 4, &l3te_voq_thrsh_profile_1_prvt);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_2_PRVT, 4, &l3te_voq_thrsh_profile_2_prvt);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_3_PRVT, 4, &l3te_voq_thrsh_profile_3_prvt);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_4_PRVT, 4, &l3te_voq_thrsh_profile_4_prvt);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_5_PRVT, 4, &l3te_voq_thrsh_profile_5_prvt);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_6_PRVT, 4, &l3te_voq_thrsh_profile_6_prvt);
    aal_scfg_read(CFG_ID_L3TE_VOQ_THRSH_PROFILE_7_PRVT, 4, &l3te_voq_thrsh_profile_7_prvt);

    aal_scfg_read(CFG_ID_L3TE_SRC_PORT_THRSH_0_THL, 4, &l3te_src_port_thrsh_0_thl);
    aal_scfg_read(CFG_ID_L3TE_SRC_PORT_THRSH_1_THL, 4, &l3te_src_port_thrsh_1_thl);

    aal_scfg_read(CFG_ID_L3TE_SRC_PORT_THRSH_0_THH, 4, &l3te_src_port_thrsh_0_thh);
    aal_scfg_read(CFG_ID_L3TE_SRC_PORT_THRSH_1_THH, 4, &l3te_src_port_thrsh_1_thh);

    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_0_THL , 4, &l3te_src_pri_thrsh_0_thl);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_1_THL , 4, &l3te_src_pri_thrsh_1_thl);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_2_THL , 4, &l3te_src_pri_thrsh_2_thl);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_3_THL , 4, &l3te_src_pri_thrsh_3_thl);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_4_THL , 4, &l3te_src_pri_thrsh_4_thl);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_5_THL , 4, &l3te_src_pri_thrsh_5_thl);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_6_THL , 4, &l3te_src_pri_thrsh_6_thl);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_7_THL , 4, &l3te_src_pri_thrsh_7_thl);

    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_0_THH , 4, &l3te_src_pri_thrsh_0_thh);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_1_THH , 4, &l3te_src_pri_thrsh_1_thh);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_2_THH , 4, &l3te_src_pri_thrsh_2_thh);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_3_THH , 4, &l3te_src_pri_thrsh_3_thh);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_4_THH , 4, &l3te_src_pri_thrsh_4_thh);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_5_THH , 4, &l3te_src_pri_thrsh_5_thh);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_6_THH , 4, &l3te_src_pri_thrsh_6_thh);
    aal_scfg_read(CFG_ID_L3TE_SRC_PRI_THRSH_7_THH , 4, &l3te_src_pri_thrsh_7_thh);

    aal_scfg_read(CFG_ID_L3TE_DQSCH_PORT_THRSH_PROFILE_0 , 4, &l3te_dqsch_port_thrsh_profile_0);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_PORT_THRSH_PROFILE_1 , 4, &l3te_dqsch_port_thrsh_profile_1);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_PORT_THRSH_PROFILE_2 , 4, &l3te_dqsch_port_thrsh_profile_2);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_PORT_THRSH_PROFILE_3 , 4, &l3te_dqsch_port_thrsh_profile_3);

    aal_scfg_read(CFG_ID_L3TE_DQSCH_VOQ_THRSH_PROFILE_0 , 4, &l3te_dqsch_voq_thrsh_profile_0);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_VOQ_THRSH_PROFILE_1 , 4, &l3te_dqsch_voq_thrsh_profile_1);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_VOQ_THRSH_PROFILE_2 , 4, &l3te_dqsch_voq_thrsh_profile_2);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_VOQ_THRSH_PROFILE_3 , 4, &l3te_dqsch_voq_thrsh_profile_3);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_VOQ_THRSH_PROFILE_4 , 4, &l3te_dqsch_voq_thrsh_profile_4);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_VOQ_THRSH_PROFILE_5 , 4, &l3te_dqsch_voq_thrsh_profile_5);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_VOQ_THRSH_PROFILE_6 , 4, &l3te_dqsch_voq_thrsh_profile_6);
    aal_scfg_read(CFG_ID_L3TE_DQSCH_VOQ_THRSH_PROFILE_7 , 4, &l3te_dqsch_voq_thrsh_profile_7);

    aal_scfg_read(CFG_ID_L3TE_PORT00_FREE_BUF_CNT  , 4, &l3te_port00_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT01_FREE_BUF_CNT  , 4, &l3te_port01_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT02_FREE_BUF_CNT  , 4, &l3te_port02_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT03_FREE_BUF_CNT  , 4, &l3te_port03_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT04_FREE_BUF_CNT  , 4, &l3te_port04_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT05_FREE_BUF_CNT  , 4, &l3te_port05_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT06_FREE_BUF_CNT  , 4, &l3te_port06_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT07_FREE_BUF_CNT  , 4, &l3te_port07_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT08_FREE_BUF_CNT  , 4, &l3te_port08_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT09_FREE_BUF_CNT  , 4, &l3te_port09_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT10_FREE_BUF_CNT  , 4, &l3te_port10_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT11_FREE_BUF_CNT  , 4, &l3te_port11_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT12_FREE_BUF_CNT  , 4, &l3te_port12_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT13_FREE_BUF_CNT  , 4, &l3te_port13_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT14_FREE_BUF_CNT  , 4, &l3te_port14_free_buf_cnt);
    aal_scfg_read(CFG_ID_L3TE_PORT15_FREE_BUF_CNT  , 4, &l3te_port15_free_buf_cnt);
}

ca_status_t aal_l3_te_cb_all_threshold_get(
    CA_IN ca_device_id_t                   device_id,
    CA_OUT aal_l3_te_cb_all_threshold_t  *cfg
)
{
    ca_status_t ret = CA_E_OK;
    aal_l3_te_cb_eq_threshold_t eq_threshold;
    aal_l3_te_cb_comb_threshold_t port_thrshd_profile, voq_thrshd_profile;
    aal_l3_te_cb_threshold_t src_port_thrsh, src_pri_thrsh, pri_thrsh, deep_q_port, deep_q_voq;
    aal_l3_te_cb_free_buf_cnt_t free_buf_cnt;

    ret = aal_l3_te_cb_pool_profile_get(device_id, 0,&eq_threshold);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->eq_prof0_threshold0 = eq_threshold.threshold;


    ret = aal_l3_te_cb_pool_profile_get(device_id, 1,&eq_threshold);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->eq_prof0_threshold1 = eq_threshold.threshold;

    ret = aal_l3_te_cb_pool_profile_get(device_id, 2,&eq_threshold);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->eq_prof1_threshold0 = eq_threshold.threshold;

    ret = aal_l3_te_cb_pool_profile_get(device_id, 3,&eq_threshold);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->eq_prof1_threshold1 = eq_threshold.threshold;

    ret = aal_l3_te_cb_pool_profile_get(device_id, 4,&eq_threshold);
      if(ret != CA_E_OK){
          return ret;
      }
      cfg->eq_prof2_threshold0 = eq_threshold.threshold;


      ret = aal_l3_te_cb_pool_profile_get(device_id, 5,&eq_threshold);
      if(ret != CA_E_OK){
          return ret;
      }
      cfg->eq_prof2_threshold1 = eq_threshold.threshold;

      ret = aal_l3_te_cb_pool_profile_get(device_id, 6,&eq_threshold);
      if(ret != CA_E_OK){
          return ret;
      }
      cfg->eq_prof3_threshold0 = eq_threshold.threshold;

      ret = aal_l3_te_cb_pool_profile_get(device_id, 7,&eq_threshold);
      if(ret != CA_E_OK){
          return ret;
      }
      cfg->eq_prof3_threshold1 = eq_threshold.threshold;

      ret = aal_l3_te_cb_pool_profile_get(device_id, 8,&eq_threshold);
        if(ret != CA_E_OK){
            return ret;
        }
        cfg->eq_prof4_threshold0 = eq_threshold.threshold;


        ret = aal_l3_te_cb_pool_profile_get(device_id, 9,&eq_threshold);
        if(ret != CA_E_OK){
            return ret;
        }
        cfg->eq_prof4_threshold1 = eq_threshold.threshold;

        ret = aal_l3_te_cb_pool_profile_get(device_id, 10,&eq_threshold);
        if(ret != CA_E_OK){
            return ret;
        }
        cfg->eq_prof5_threshold0 = eq_threshold.threshold;

        ret = aal_l3_te_cb_pool_profile_get(device_id, 11,&eq_threshold);
        if(ret != CA_E_OK){
            return ret;
        }
        cfg->eq_prof5_threshold1 = eq_threshold.threshold;

      ret = aal_l3_te_cb_pool_profile_get(device_id, 12,&eq_threshold);
      if(ret != CA_E_OK){
          return ret;
      }
      cfg->eq_prof6_threshold0 = eq_threshold.threshold;


      ret = aal_l3_te_cb_pool_profile_get(device_id, 13,&eq_threshold);
      if(ret != CA_E_OK){
          return ret;
      }
      cfg->eq_prof6_threshold1 = eq_threshold.threshold;

      ret = aal_l3_te_cb_pool_profile_get(device_id, 14,&eq_threshold);
      if(ret != CA_E_OK){
          return ret;
      }
      cfg->eq_prof7_threshold0 = eq_threshold.threshold;

      ret = aal_l3_te_cb_pool_profile_get(device_id, 15,&eq_threshold);
      if(ret != CA_E_OK){
          return ret;
      }
      cfg->eq_prof7_threshold1 = eq_threshold.threshold;

    ret = aal_l3_te_cb_port_threshold_profile_get(device_id, 0, &port_thrshd_profile);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port_threshold_profile0_hi = port_thrshd_profile.threshold_hi;
    cfg->port_threshold_profile0_lo  = port_thrshd_profile.threshold_lo;
    cfg->port_grp_threshold_profile0_lo = port_thrshd_profile.grp_threshold_lo;
    cfg->port_grp_threshold_profile0_hi = port_thrshd_profile.grp_threshold_hi;

    ret = aal_l3_te_cb_port_threshold_profile_get(device_id, 1, &port_thrshd_profile);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port_threshold_profile1_hi = port_thrshd_profile.threshold_hi;
    cfg->port_threshold_profile1_lo  = port_thrshd_profile.threshold_lo;
    cfg->port_grp_threshold_profile1_lo = port_thrshd_profile.grp_threshold_lo;
    cfg->port_grp_threshold_profile1_hi = port_thrshd_profile.grp_threshold_hi;

    ret = aal_l3_te_cb_port_threshold_profile_get(device_id, 2, &port_thrshd_profile);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port_threshold_profile2_hi = port_thrshd_profile.threshold_hi;
    cfg->port_threshold_profile2_lo  = port_thrshd_profile.threshold_lo;
    cfg->port_grp_threshold_profile2_lo = port_thrshd_profile.grp_threshold_lo;
    cfg->port_grp_threshold_profile2_hi = port_thrshd_profile.grp_threshold_hi;

    ret = aal_l3_te_cb_port_threshold_profile_get(device_id, 3, &port_thrshd_profile);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port_threshold_profile3_hi = port_thrshd_profile.threshold_hi;
    cfg->port_threshold_profile3_lo  = port_thrshd_profile.threshold_lo;
    cfg->port_grp_threshold_profile3_lo = port_thrshd_profile.grp_threshold_lo;
    cfg->port_grp_threshold_profile3_hi = port_thrshd_profile.grp_threshold_hi;

    ret = aal_l3_te_cb_src_port_threshold_get(device_id, 0, &src_port_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_port_threshold_profile0_hi  = src_port_thrsh.threshold_hi;
    cfg->src_port_threshold_profile0_lo  = src_port_thrsh.threshold_lo;

    ret = aal_l3_te_cb_src_port_threshold_get(device_id, 1, &src_port_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_port_threshold_profile1_hi  = src_port_thrsh.threshold_hi;
    cfg->src_port_threshold_profile1_lo  = src_port_thrsh.threshold_lo;


    cfg->port_profile_sel = CA_NE_REG_READ(TE_CB_PORT_THRSH_SELECT);


    ret = aal_l3_te_cb_voq_threshold_profile_get(device_id,0, &voq_thrshd_profile);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->voq_private_buf_profile0 = voq_thrshd_profile.prvt;
    cfg->voq_grp_threshold_profile0_hi  = voq_thrshd_profile.grp_threshold_hi;
    cfg->voq_grp_threshold_profile0_lo  = voq_thrshd_profile.grp_threshold_lo;
    cfg->voq_threshold_profile0_hi  = voq_thrshd_profile.threshold_hi;
    cfg->voq_threshold_profile0_lo  = voq_thrshd_profile.threshold_lo;

    ret = aal_l3_te_cb_voq_threshold_profile_get(device_id,1, &voq_thrshd_profile);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->voq_private_buf_profile1 = voq_thrshd_profile.prvt;
     cfg->voq_grp_threshold_profile1_hi  = voq_thrshd_profile.grp_threshold_hi;
     cfg->voq_grp_threshold_profile1_lo  = voq_thrshd_profile.grp_threshold_lo;
     cfg->voq_threshold_profile1_hi  = voq_thrshd_profile.threshold_hi;
     cfg->voq_threshold_profile1_lo  = voq_thrshd_profile.threshold_lo;

    ret = aal_l3_te_cb_voq_threshold_profile_get(device_id,2, &voq_thrshd_profile);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->voq_private_buf_profile2 = voq_thrshd_profile.prvt;
     cfg->voq_grp_threshold_profile2_hi  = voq_thrshd_profile.grp_threshold_hi;
     cfg->voq_grp_threshold_profile2_lo  = voq_thrshd_profile.grp_threshold_lo;
     cfg->voq_threshold_profile2_hi  = voq_thrshd_profile.threshold_hi;
     cfg->voq_threshold_profile2_lo  = voq_thrshd_profile.threshold_lo;

    ret = aal_l3_te_cb_voq_threshold_profile_get(device_id,3, &voq_thrshd_profile);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->voq_private_buf_profile3 = voq_thrshd_profile.prvt;
     cfg->voq_grp_threshold_profile3_hi  = voq_thrshd_profile.grp_threshold_hi;
     cfg->voq_grp_threshold_profile3_lo  = voq_thrshd_profile.grp_threshold_lo;
     cfg->voq_threshold_profile3_hi  = voq_thrshd_profile.threshold_hi;
     cfg->voq_threshold_profile3_lo  = voq_thrshd_profile.threshold_lo;

    ret = aal_l3_te_cb_voq_threshold_profile_get(device_id,4, &voq_thrshd_profile);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->voq_private_buf_profile4 = voq_thrshd_profile.prvt;
     cfg->voq_grp_threshold_profile4_hi  = voq_thrshd_profile.grp_threshold_hi;
     cfg->voq_grp_threshold_profile4_lo  = voq_thrshd_profile.grp_threshold_lo;
     cfg->voq_threshold_profile4_hi  = voq_thrshd_profile.threshold_hi;
     cfg->voq_threshold_profile4_lo  = voq_thrshd_profile.threshold_lo;

    ret = aal_l3_te_cb_voq_threshold_profile_get(device_id,5, &voq_thrshd_profile);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->voq_private_buf_profile5 = voq_thrshd_profile.prvt;
     cfg->voq_grp_threshold_profile5_hi  = voq_thrshd_profile.grp_threshold_hi;
     cfg->voq_grp_threshold_profile5_lo  = voq_thrshd_profile.grp_threshold_lo;
     cfg->voq_threshold_profile5_hi  = voq_thrshd_profile.threshold_hi;
     cfg->voq_threshold_profile5_lo  = voq_thrshd_profile.threshold_lo;

    ret = aal_l3_te_cb_voq_threshold_profile_get(device_id,6, &voq_thrshd_profile);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->voq_private_buf_profile6 = voq_thrshd_profile.prvt;
     cfg->voq_grp_threshold_profile6_hi  = voq_thrshd_profile.grp_threshold_hi;
     cfg->voq_grp_threshold_profile6_lo  = voq_thrshd_profile.grp_threshold_lo;
     cfg->voq_threshold_profile6_hi  = voq_thrshd_profile.threshold_hi;
     cfg->voq_threshold_profile6_lo  = voq_thrshd_profile.threshold_lo;

    ret = aal_l3_te_cb_voq_threshold_profile_get(device_id,7, &voq_thrshd_profile);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->voq_private_buf_profile7 = voq_thrshd_profile.prvt;
     cfg->voq_grp_threshold_profile7_hi  = voq_thrshd_profile.grp_threshold_hi;
     cfg->voq_grp_threshold_profile7_lo  = voq_thrshd_profile.grp_threshold_lo;
     cfg->voq_threshold_profile7_hi  = voq_thrshd_profile.threshold_hi;
     cfg->voq_threshold_profile7_lo  = voq_thrshd_profile.threshold_lo;


    ret = aal_l3_te_cb_pri_threshold_get(device_id, 0, &pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->glb_pri_threshold_hi_0  = pri_thrsh.threshold_hi;
    cfg->glb_pri_threshold_lo_0  = pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_pri_threshold_get(device_id, 1, &pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->glb_pri_threshold_hi_1  = pri_thrsh.threshold_hi;
    cfg->glb_pri_threshold_lo_1  = pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_pri_threshold_get(device_id, 2, &pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->glb_pri_threshold_hi_2  = pri_thrsh.threshold_hi;
    cfg->glb_pri_threshold_lo_2  = pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_pri_threshold_get(device_id, 3, &pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->glb_pri_threshold_hi_3  = pri_thrsh.threshold_hi;
    cfg->glb_pri_threshold_lo_3  = pri_thrsh.threshold_lo;


    ret = aal_l3_te_cb_pri_threshold_get(device_id, 4, &pri_thrsh);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->glb_pri_threshold_hi_4  = pri_thrsh.threshold_hi;
     cfg->glb_pri_threshold_lo_4  = pri_thrsh.threshold_lo;

     ret = aal_l3_te_cb_pri_threshold_get(device_id, 5, &pri_thrsh);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->glb_pri_threshold_hi_5  = pri_thrsh.threshold_hi;
     cfg->glb_pri_threshold_lo_5  = pri_thrsh.threshold_lo;

     ret = aal_l3_te_cb_pri_threshold_get(device_id, 6, &pri_thrsh);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->glb_pri_threshold_hi_6  = pri_thrsh.threshold_hi;
     cfg->glb_pri_threshold_lo_6  = pri_thrsh.threshold_lo;

     ret = aal_l3_te_cb_pri_threshold_get(device_id, 7, &pri_thrsh);
     if(ret != CA_E_OK){
         return ret;
     }
     cfg->glb_pri_threshold_hi_7  = pri_thrsh.threshold_hi;
     cfg->glb_pri_threshold_lo_7  = pri_thrsh.threshold_lo;


    ret = aal_l3_te_cb_src_pri_threshold_get(device_id, 0, &src_pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_pri_threshold_hi_0 = src_pri_thrsh.threshold_hi;
    cfg->src_pri_threshold_lo_0 = src_pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_src_pri_threshold_get(device_id, 1, &src_pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_pri_threshold_hi_1 = src_pri_thrsh.threshold_hi;
    cfg->src_pri_threshold_lo_1 = src_pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_src_pri_threshold_get(device_id, 2, &src_pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_pri_threshold_hi_2 = src_pri_thrsh.threshold_hi;
    cfg->src_pri_threshold_lo_2 = src_pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_src_pri_threshold_get(device_id, 3, &src_pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_pri_threshold_hi_3 = src_pri_thrsh.threshold_hi;
    cfg->src_pri_threshold_lo_3 = src_pri_thrsh.threshold_lo;
    ret = aal_l3_te_cb_src_pri_threshold_get(device_id, 4, &src_pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_pri_threshold_hi_4 = src_pri_thrsh.threshold_hi;
    cfg->src_pri_threshold_lo_4 = src_pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_src_pri_threshold_get(device_id, 5, &src_pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_pri_threshold_hi_5 = src_pri_thrsh.threshold_hi;
    cfg->src_pri_threshold_lo_5 = src_pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_src_pri_threshold_get(device_id, 6, &src_pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_pri_threshold_hi_6 = src_pri_thrsh.threshold_hi;
    cfg->src_pri_threshold_lo_6 = src_pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_src_pri_threshold_get(device_id, 7, &src_pri_thrsh);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->src_pri_threshold_hi_7 = src_pri_thrsh.threshold_hi;
    cfg->src_pri_threshold_lo_7 = src_pri_thrsh.threshold_lo;

    ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 0,&free_buf_cnt);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port00_free_buf_cnt0  = free_buf_cnt.cnt0;
    cfg->port00_free_buf_cnt1  = free_buf_cnt.cnt1;


    ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 1,&free_buf_cnt);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port01_free_buf_cnt0  = free_buf_cnt.cnt0;
    cfg->port01_free_buf_cnt1  = free_buf_cnt.cnt1;

    ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 2,&free_buf_cnt);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port02_free_buf_cnt0  = free_buf_cnt.cnt0;
    cfg->port02_free_buf_cnt1  = free_buf_cnt.cnt1;

    ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 3,&free_buf_cnt);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port03_free_buf_cnt0  = free_buf_cnt.cnt0;
    cfg->port03_free_buf_cnt1  = free_buf_cnt.cnt1;

    ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 4,&free_buf_cnt);
    if(ret != CA_E_OK){
        return ret;
    }
    cfg->port04_free_buf_cnt0  = free_buf_cnt.cnt0;
    cfg->port04_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 5,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port05_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port05_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 6,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port06_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port06_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 7,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port07_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port07_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 8,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port08_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port08_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 9,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port09_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port09_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 10,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port10_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port10_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 11,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port11_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port11_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 12,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port12_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port12_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 13,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port13_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port13_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 14,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port14_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port14_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_port_free_buf_cnt_get(device_id, 15,&free_buf_cnt);
   if(ret != CA_E_OK){
       return ret;
   }
   cfg->port15_free_buf_cnt0  = free_buf_cnt.cnt0;
   cfg->port15_free_buf_cnt1  = free_buf_cnt.cnt1;

   ret = aal_l3_te_cb_dqsch_port_thrsh_profile_get(device_id, 0, &deep_q_port);
   cfg->deep_q_port_threshold_profile0 = deep_q_port.threshold_hi;
   ret = aal_l3_te_cb_dqsch_port_thrsh_profile_get(device_id, 1, &deep_q_port);
   cfg->deep_q_port_threshold_profile1 = deep_q_port.threshold_hi;
   ret = aal_l3_te_cb_dqsch_port_thrsh_profile_get(device_id, 2, &deep_q_port);
   cfg->deep_q_port_threshold_profile2 = deep_q_port.threshold_hi;
   ret = aal_l3_te_cb_dqsch_port_thrsh_profile_get(device_id, 3, &deep_q_port);
   cfg->deep_q_port_threshold_profile3 = deep_q_port.threshold_hi;

   ret = aal_l3_te_cb_dqsch_voq_thrsh_profile_get(device_id, 0, &deep_q_voq);
   cfg->deep_q_voq_threshold_profile0 = deep_q_voq.threshold_lo;
   ret = aal_l3_te_cb_dqsch_voq_thrsh_profile_get(device_id, 1, &deep_q_voq);
   cfg->deep_q_voq_threshold_profile1 = deep_q_voq.threshold_lo;
   ret = aal_l3_te_cb_dqsch_voq_thrsh_profile_get(device_id, 2, &deep_q_voq);
   cfg->deep_q_voq_threshold_profile2 = deep_q_voq.threshold_lo;
   ret = aal_l3_te_cb_dqsch_voq_thrsh_profile_get(device_id, 3, &deep_q_voq);
   cfg->deep_q_voq_threshold_profile3 = deep_q_voq.threshold_lo;
   ret = aal_l3_te_cb_dqsch_voq_thrsh_profile_get(device_id, 4, &deep_q_voq);
   cfg->deep_q_voq_threshold_profile4 = deep_q_voq.threshold_lo;
   ret = aal_l3_te_cb_dqsch_voq_thrsh_profile_get(device_id, 5, &deep_q_voq);
   cfg->deep_q_voq_threshold_profile5 = deep_q_voq.threshold_lo;
   ret = aal_l3_te_cb_dqsch_voq_thrsh_profile_get(device_id, 6, &deep_q_voq);
   cfg->deep_q_voq_threshold_profile6 = deep_q_voq.threshold_lo;
   ret = aal_l3_te_cb_dqsch_voq_thrsh_profile_get(device_id, 7, &deep_q_voq);
   cfg->deep_q_voq_threshold_profile7 = deep_q_voq.threshold_lo;

   return ret;
}

ca_status_t aal_l3_te_cb_init(CA_IN ca_device_id_t                   device_id)
{
    ca_uint8_t port_id;
    ca_uint8_t profile_sel = 0;
    ca_uint8_t profile_id;
    ca_uint8_t voq_id;
    aal_l3_te_cb_free_buf_cnt_t port_buf;

    aal_l3_te_cb_comb_threshold_mask_t port_profle_mask;
    aal_l3_te_cb_comb_threshold_t port_profle;
    aal_l3_te_cb_comb_threshold_mask_t voq_profle_mask;
    aal_l3_te_cb_comb_threshold_t voq_profle;
    aal_l3_te_cb_eq_threshold_t   eq_profile;
    aal_l3_te_cb_threshold_t     pri_cb_thrsh, src_pri_thrsh, src_port_thrsh, deep_q_port_th, deep_q_voq_th;
    aal_l3_te_cb_free_buf_cnt_mask_t port_buf_mask;

    memset(&port_buf, 0, sizeof(aal_l3_te_cb_free_buf_cnt_t));
    memset(&port_profle, 0, sizeof(aal_l3_te_cb_comb_threshold_t));
    memset(&voq_profle, 0, sizeof(aal_l3_te_cb_comb_threshold_t));
    memset(&eq_profile, 0, sizeof(aal_l3_te_cb_eq_threshold_t));
    memset(&pri_cb_thrsh, 0, sizeof(aal_l3_te_cb_threshold_t));
    memset(&src_pri_thrsh, 0, sizeof(aal_l3_te_cb_threshold_t));
    memset(&src_port_thrsh, 0, sizeof(aal_l3_te_cb_threshold_t));
    memset(&deep_q_port_th, 0, sizeof(aal_l3_te_cb_threshold_t));
    memset(&deep_q_voq_th, 0, sizeof(aal_l3_te_cb_threshold_t));

	aal_l3_te_cb_init_startup_config(device_id);

    port_profle_mask.u32 = 0;
    voq_profle_mask.u32 = 0 ;
    port_buf_mask.u32 = 0;

    port_buf.cnt1_msb = 1;
    port_buf.cnt0_msb = 1;
    port_buf_mask.s.cnt1_msb = 1;
    port_buf_mask.s.cnt0_msb = 1;
    port_buf_mask.s.cnt1 = 1;
    port_buf_mask.s.cnt0 = 1;

    port_profle_mask.s.threshold_hi = 1;
    port_profle_mask.s.threshold_lo = 1;

    if(0 == device_id){
        for (port_id = 0; port_id < __AAL_L3_TE_CB_PORT_NUM; port_id ++) {
            if(port_id < __AAL_L3_TE_CB_PE0_DPID){  /*A53*/
                port_buf.cnt1 = l3te_port01_free_buf_cnt;
                port_buf.cnt0 = l3te_port00_free_buf_cnt;
                profile_sel = __AAL_L3_TE_CB_PROFILE_SEL_A53;
            }else if(port_id >  __AAL_L3_TE_CB_PE1_DPID){
                port_buf.cnt1 = l3te_port09_free_buf_cnt;
                port_buf.cnt0 = l3te_port08_free_buf_cnt;
                profile_sel = __AAL_L3_TE_CB_PROFILE_SEL_DQ;
            }else{
                port_buf.cnt1 = l3te_port07_free_buf_cnt;
                port_buf.cnt0 = l3te_port06_free_buf_cnt;
                profile_sel = __AAL_L3_TE_CB_PROFILE_SEL_PE;
            }
            aal_l3_te_cb_port_free_buf_cnt_set(device_id, port_id,port_buf_mask, &port_buf);
            aal_l3_te_cb_port_profile_sel_set(device_id, port_id, profile_sel);
        }
        for (profile_id = 0; profile_id < TE_CB_PORT_GRPTHRSH_PROFILE0_COUNT; profile_id ++) {
            switch(profile_id){
                case __AAL_L3_TE_CB_PROFILE_SEL_PE:
                    port_profle.threshold_hi = l3te_port_thrsh_profile_0_hth;
                    port_profle.threshold_lo = l3te_port_thrsh_profile_0_lth;
                    break;
                case __AAL_L3_TE_CB_PROFILE_SEL_A53:
                    port_profle.threshold_hi = l3te_port_thrsh_profile_2_hth;
                    port_profle.threshold_lo = l3te_port_thrsh_profile_2_lth;
                    break;
                case __AAL_L3_TE_CB_PROFILE_SEL_CPU256:
                    port_profle.threshold_hi = l3te_port_thrsh_profile_1_hth;
                    port_profle.threshold_lo = l3te_port_thrsh_profile_1_lth;
                    break;
                case __AAL_L3_TE_CB_PROFILE_SEL_DQ:
                    port_profle.threshold_hi = l3te_port_thrsh_profile_3_hth;
                    port_profle.threshold_lo = l3te_port_thrsh_profile_3_lth;
                    break;
               default:
                    break;
            }

            aal_l3_te_cb_port_threshold_profile_set(device_id, profile_id,port_profle_mask, &port_profle);
            deep_q_port_th.threshold_hi = l3te_dqsch_port_thrsh_profile_0;
            aal_l3_te_cb_dqsch_port_thrsh_profile_set(device_id,profile_id,&deep_q_port_th);

        }

        for(profile_id = 0; profile_id < (TE_CB_EQ_PROFILE_THRSH0_COUNT *2); profile_id ++){
            if(profile_id < 4){
                eq_profile.threshold = l3te_profile_thrsh_eq0_th0;
            }
            else if((profile_id == 4) || (profile_id == 5) ||(profile_id == 6) || (profile_id == 7)){
                eq_profile.threshold = l3te_profile_thrsh_eq2_th0;
            }
            else if((profile_id == 8) || (profile_id == 9)){
                eq_profile.threshold = l3te_profile_thrsh_eq4_th0;
            }else if(profile_id == 14 || profile_id == 15){
                eq_profile.threshold = l3te_profile_thrsh_eq5_th0;
            }else{
                eq_profile.threshold = l3te_profile_thrsh_eq6_th0;
            }
            aal_l3_te_cb_pool_profile_set(device_id, profile_id, &eq_profile);
        }
        voq_profle_mask.s.threshold_hi = 1;
        voq_profle_mask.s.threshold_lo = 1;
        voq_profle_mask.s.prvt = 1;

        for(profile_id = 0; profile_id < 8; profile_id ++){
            switch(profile_id){
            case 0:
                voq_profle.threshold_hi = l3te_voq_thrsh_profile_0_thh;
                voq_profle.threshold_lo = l3te_voq_thrsh_profile_0_thl;
                voq_profle.prvt         = l3te_voq_thrsh_profile_0_prvt;
            break;

            case 1:
                voq_profle.threshold_hi = l3te_voq_thrsh_profile_1_thh;
                voq_profle.threshold_lo = l3te_voq_thrsh_profile_1_thl;
                voq_profle.prvt         = l3te_voq_thrsh_profile_1_prvt;
            break;

            case 2:
                voq_profle.threshold_hi = l3te_voq_thrsh_profile_2_thh;
                voq_profle.threshold_lo = l3te_voq_thrsh_profile_2_thl;
                voq_profle.prvt         = l3te_voq_thrsh_profile_2_prvt;
            break;
            case 3:
                voq_profle.threshold_hi = l3te_voq_thrsh_profile_3_thh;
                voq_profle.threshold_lo = l3te_voq_thrsh_profile_3_thl;
                voq_profle.prvt         = l3te_voq_thrsh_profile_3_prvt;
            break;
           case 4:
               voq_profle.threshold_hi = l3te_voq_thrsh_profile_4_thh;
               voq_profle.threshold_lo = l3te_voq_thrsh_profile_4_thl;
               voq_profle.prvt         = l3te_voq_thrsh_profile_4_prvt;
            break;

            case 5:
                voq_profle.threshold_hi = l3te_voq_thrsh_profile_5_thh;
                voq_profle.threshold_lo = l3te_voq_thrsh_profile_5_thl;
                voq_profle.prvt         = l3te_voq_thrsh_profile_5_prvt;
            break;

            case 6:
                voq_profle.threshold_hi = l3te_voq_thrsh_profile_6_thh;
                voq_profle.threshold_lo = l3te_voq_thrsh_profile_6_thl;
                voq_profle.prvt         = l3te_voq_thrsh_profile_6_prvt;
            break;
            case 7:
                voq_profle.threshold_hi = l3te_voq_thrsh_profile_7_thh;
                voq_profle.threshold_lo = l3te_voq_thrsh_profile_7_thl;
                voq_profle.prvt         = l3te_voq_thrsh_profile_7_prvt;
            break;
            default:
                break;
            }
            if(profile_id<8){
                aal_l3_te_cb_voq_threshold_profile_set(device_id,profile_id,voq_profle_mask,&voq_profle);
            }
            pri_cb_thrsh.threshold_hi = 60;
            pri_cb_thrsh.threshold_lo = 30;
            aal_l3_te_cb_pri_threshold_set(device_id,profile_id,&pri_cb_thrsh);

            src_pri_thrsh.threshold_hi = l3te_src_pri_thrsh_0_thh;
            src_pri_thrsh.threshold_lo = l3te_src_pri_thrsh_0_thl;
            aal_l3_te_cb_src_pri_threshold_set(device_id,profile_id,&src_pri_thrsh);

            deep_q_voq_th.threshold_lo = l3te_dqsch_voq_thrsh_profile_0;
            aal_l3_te_cb_dqsch_voq_thrsh_profile_set(device_id,profile_id,&deep_q_voq_th);

        }
        for(voq_id = 0; voq_id<__AAL_L3_TE_CB_VOQ_NUM; voq_id++){
            if(voq_id <__AAL_L3_TE_CB_PE0_DPID*8){
                    profile_sel = 2;
            }else if(voq_id >= __AAL_L3_TE_CB_PE1_DPID*8){
                    if(voq_id & 7 == 0){
                        profile_sel = 3;
                    }else{
                        profile_sel = 4;
                    }
            }else{
                    profile_sel = 0;
            }
            aal_l3_te_cb_voq_profile_sel_set(device_id,voq_id,profile_sel);
        }
        src_port_thrsh.threshold_hi = l3te_src_port_thrsh_0_thh;
        src_port_thrsh.threshold_lo = l3te_src_port_thrsh_0_thl;
        aal_l3_te_cb_src_port_threshold_set(device_id,0,&src_port_thrsh);
        src_port_thrsh.threshold_hi = l3te_src_port_thrsh_1_thh;
        src_port_thrsh.threshold_lo = l3te_src_port_thrsh_1_thl;
        aal_l3_te_cb_src_port_threshold_set(device_id,1,&src_port_thrsh);

    }else if(1 == device_id){
#ifdef CONFIG_ARCH_CORTINA_G3HGU
        for (port_id = 0; port_id < __AAL_L3_TE_CB_PORT_NUM; port_id ++) {
            if(port_id < __AAL_L3_TE_CB_PE0_DPID){  /*A53*/
                port_buf.cnt1 = saturn_l3te_port01_free_buf_cnt;
                port_buf.cnt0 = saturn_l3te_port00_free_buf_cnt;
                profile_sel = __AAL_L3_TE_CB_PROFILE_SEL_A53;
            }else if(port_id >  __AAL_L3_TE_CB_PE1_DPID){
                port_buf.cnt1 = saturn_l3te_port09_free_buf_cnt;
                port_buf.cnt0 = saturn_l3te_port08_free_buf_cnt;
                profile_sel = __AAL_L3_TE_CB_PROFILE_SEL_DQ;
            }else{
                port_buf.cnt1 = saturn_l3te_port07_free_buf_cnt;
                port_buf.cnt0 = saturn_l3te_port06_free_buf_cnt;
                profile_sel = __AAL_L3_TE_CB_PROFILE_SEL_PE;
            }
            aal_l3_te_cb_port_free_buf_cnt_set(device_id, port_id,port_buf_mask, &port_buf);
            aal_l3_te_cb_port_profile_sel_set(device_id, port_id, profile_sel);
        }
        for (profile_id = 0; profile_id < TE_CB_PORT_GRPTHRSH_PROFILE0_COUNT; profile_id ++) {
            switch(profile_id){
                case __AAL_L3_TE_CB_PROFILE_SEL_PE:
                    port_profle.threshold_hi = saturn_l3te_port_thrsh_profile_0_hth;
                    port_profle.threshold_lo = saturn_l3te_port_thrsh_profile_0_lth;
                    break;
                case __AAL_L3_TE_CB_PROFILE_SEL_A53:
                    port_profle.threshold_hi = saturn_l3te_port_thrsh_profile_2_hth;
                    port_profle.threshold_lo = saturn_l3te_port_thrsh_profile_2_lth;
                    break;
                case __AAL_L3_TE_CB_PROFILE_SEL_CPU256:
                    port_profle.threshold_hi = saturn_l3te_port_thrsh_profile_1_hth;
                    port_profle.threshold_lo = saturn_l3te_port_thrsh_profile_1_lth;
                    break;
                case __AAL_L3_TE_CB_PROFILE_SEL_DQ:
                    port_profle.threshold_hi = saturn_l3te_port_thrsh_profile_3_hth;
                    port_profle.threshold_lo = saturn_l3te_port_thrsh_profile_3_lth;
                    break;
               default:
                    break;
            }

            aal_l3_te_cb_port_threshold_profile_set(device_id, profile_id,port_profle_mask, &port_profle);
            deep_q_port_th.threshold_hi = saturn_l3te_dqsch_port_thrsh_profile_0;
            aal_l3_te_cb_dqsch_port_thrsh_profile_set(device_id,profile_id,&deep_q_port_th);

        }

        for(profile_id = 0; profile_id < (TE_CB_EQ_PROFILE_THRSH0_COUNT *2); profile_id ++){
            if(profile_id < 4){
                eq_profile.threshold = saturn_l3te_profile_thrsh_eq0_th0;
            }
            else if((profile_id == 4) || (profile_id == 5) ||(profile_id == 6) || (profile_id == 7)){
                eq_profile.threshold = saturn_l3te_profile_thrsh_eq2_th0;
            }
            else if((profile_id == 8) || (profile_id == 9)){
                eq_profile.threshold = saturn_l3te_profile_thrsh_eq4_th0;
            }else if(profile_id == 14 || profile_id == 15){
                eq_profile.threshold = saturn_l3te_profile_thrsh_eq5_th0;
            }else{
                eq_profile.threshold = saturn_l3te_profile_thrsh_eq6_th0;
            }
            aal_l3_te_cb_pool_profile_set(device_id, profile_id, &eq_profile);
        }
        voq_profle_mask.s.threshold_hi = 1;
        voq_profle_mask.s.threshold_lo = 1;
        voq_profle_mask.s.prvt = 1;

        for(profile_id = 0; profile_id < 8; profile_id ++){
            switch(profile_id){
            case 0:
                voq_profle.threshold_hi = saturn_l3te_voq_thrsh_profile_0_thh;
                voq_profle.threshold_lo = saturn_l3te_voq_thrsh_profile_0_thl;
                voq_profle.prvt         = saturn_l3te_voq_thrsh_profile_0_prvt;
            break;

            case 1:
                voq_profle.threshold_hi = saturn_l3te_voq_thrsh_profile_1_thh;
                voq_profle.threshold_lo = saturn_l3te_voq_thrsh_profile_1_thl;
                voq_profle.prvt         = saturn_l3te_voq_thrsh_profile_1_prvt;
            break;

            case 2:
                voq_profle.threshold_hi = saturn_l3te_voq_thrsh_profile_2_thh;
                voq_profle.threshold_lo = saturn_l3te_voq_thrsh_profile_2_thl;
                voq_profle.prvt         = saturn_l3te_voq_thrsh_profile_2_prvt;
            break;
            case 3:
                voq_profle.threshold_hi = l3te_voq_thrsh_profile_3_thh;
                voq_profle.threshold_lo = l3te_voq_thrsh_profile_3_thl;
                voq_profle.prvt         = l3te_voq_thrsh_profile_3_prvt;
            break;
         /*   case 4:
                voq_profle.threshold_hi = __AAL_L3_TE_CB_VOQ_PROFILE_A53_57_HI;
                voq_profle.threshold_lo = __AAL_L3_TE_CB_VOQ_PROFILE_A53_57_LO;
                voq_profle.prvt         = __AAL_L3_TE_CB_VOQ_PROFILE_A53_57_PRVT;
            break;

            case 5:
                voq_profle.threshold_hi = __AAL_L3_TE_CB_VOQ_PROFILE_A53_04_HI;
                voq_profle.threshold_lo = __AAL_L3_TE_CB_VOQ_PROFILE_A53_04_LO;
                voq_profle.prvt         = __AAL_L3_TE_CB_VOQ_PROFILE_A53_04_PRVT;
            break;

            case 6:
                voq_profle.threshold_hi = __AAL_L3_TE_CB_VOQ_PROFILE_DQ_57_HI;
                voq_profle.threshold_lo = __AAL_L3_TE_CB_VOQ_PROFILE_DQ_57_LO;
                voq_profle.prvt         = __AAL_L3_TE_CB_VOQ_PROFILE_DQ_57_PRVT;
            break;

            case 7:
                voq_profle.threshold_hi = __AAL_L3_TE_CB_VOQ_PROFILE_DQ_04_HI;
                voq_profle.threshold_lo = __AAL_L3_TE_CB_VOQ_PROFILE_DQ_04_LO;
                voq_profle.prvt         = __AAL_L3_TE_CB_VOQ_PROFILE_DQ_04_PRVT;
            break;
    */
            default:
                break;
            }
            if(profile_id<4){
                aal_l3_te_cb_voq_threshold_profile_set(device_id,profile_id,voq_profle_mask,&voq_profle);
            }
            pri_cb_thrsh.threshold_hi = 60;
            pri_cb_thrsh.threshold_lo = 30;
            aal_l3_te_cb_pri_threshold_set(device_id,profile_id,&pri_cb_thrsh);

            src_pri_thrsh.threshold_hi = saturn_l3te_src_pri_thrsh_0_thh;
            src_pri_thrsh.threshold_lo = saturn_l3te_src_pri_thrsh_0_thl;
            aal_l3_te_cb_src_pri_threshold_set(device_id,profile_id,&src_pri_thrsh);

            deep_q_voq_th.threshold_lo = saturn_l3te_dqsch_voq_thrsh_profile_0;
            aal_l3_te_cb_dqsch_voq_thrsh_profile_set(device_id,profile_id,&deep_q_voq_th);

        }
        for(voq_id = 0; voq_id<__AAL_L3_TE_CB_VOQ_NUM; voq_id++){
            if(voq_id <__AAL_L3_TE_CB_PE0_DPID*8){
                    profile_sel = 2;
            }else if(voq_id >= __AAL_L3_TE_CB_PE1_DPID*8){
                    profile_sel = 3;
            }else{
                    profile_sel = 0;
            }
            aal_l3_te_cb_voq_profile_sel_set(device_id,voq_id,profile_sel);
        }
        src_port_thrsh.threshold_hi = saturn_l3te_src_port_thrsh_0_thh;
        src_port_thrsh.threshold_lo = saturn_l3te_src_port_thrsh_0_thl;
        aal_l3_te_cb_src_port_threshold_set(device_id,0,&src_port_thrsh);
        src_port_thrsh.threshold_hi = saturn_l3te_src_port_thrsh_1_thh;
        src_port_thrsh.threshold_lo = saturn_l3te_src_port_thrsh_1_thl;
        aal_l3_te_cb_src_port_threshold_set(device_id,1,&src_port_thrsh);
#endif
    }
    return CA_E_OK;
}


EXPORT_SYMBOL(aal_l3_te_cb_intr_ena_set);
EXPORT_SYMBOL(aal_l3_te_cb_intr_ena_get);
EXPORT_SYMBOL(aal_l3_te_cb_intr_status_set);
EXPORT_SYMBOL(aal_l3_te_cb_intr_status_get);
EXPORT_SYMBOL(aal_l3_te_cb_err_info_get);
EXPORT_SYMBOL(aal_l3_te_cb_ctrl_set);
EXPORT_SYMBOL(aal_l3_te_cb_ctrl_get);
EXPORT_SYMBOL(aal_l3_te_cb_pool_profile_set);
EXPORT_SYMBOL(aal_l3_te_cb_pool_profile_get);
EXPORT_SYMBOL(aal_l3_te_cb_port_threshold_profile_set);
EXPORT_SYMBOL(aal_l3_te_cb_port_threshold_profile_get);
EXPORT_SYMBOL(aal_l3_te_cb_port_profile_sel_set);
EXPORT_SYMBOL(aal_l3_te_cb_port_profile_sel_get);
EXPORT_SYMBOL(aal_l3_te_cb_voq_threshold_profile_set);
EXPORT_SYMBOL(aal_l3_te_cb_voq_threshold_profile_get);
EXPORT_SYMBOL(aal_l3_te_cb_voq_profile_sel_set);
EXPORT_SYMBOL(aal_l3_te_cb_voq_profile_sel_get);
EXPORT_SYMBOL(aal_l3_te_cb_port_free_buf_cnt_set);
EXPORT_SYMBOL(aal_l3_te_cb_port_free_buf_cnt_get);
EXPORT_SYMBOL(aal_l3_te_cb_voq_buf_cnt_set);
EXPORT_SYMBOL(aal_l3_te_cb_voq_buf_cnt_get);
EXPORT_SYMBOL(aal_l3_te_cb_voq_max_buf_set);
EXPORT_SYMBOL(aal_l3_te_cb_voq_max_buf_get);
EXPORT_SYMBOL(aal_l3_te_cb_pri_threshold_set);
EXPORT_SYMBOL(aal_l3_te_cb_pri_threshold_get);
EXPORT_SYMBOL(aal_l3_te_cb_src_ctrl_get);
EXPORT_SYMBOL(aal_l3_te_cb_src_port_threshold_set);
EXPORT_SYMBOL(aal_l3_te_cb_src_port_threshold_get);
EXPORT_SYMBOL(aal_l3_te_cb_src_pri_threshold_set);
EXPORT_SYMBOL(aal_l3_te_cb_src_pri_threshold_get);
EXPORT_SYMBOL(aal_l3_te_cb_src_port_buf_cnt_get);
EXPORT_SYMBOL(aal_l3_te_cb_src_pri_buf_cnt_get);
EXPORT_SYMBOL(aal_l3_te_cb_dqsch_port_thrsh_profile_set);
EXPORT_SYMBOL(aal_l3_te_cb_dqsch_port_thrsh_profile_get);
EXPORT_SYMBOL(aal_l3_te_cb_dqsch_port_thrsh_profile_sel_get);
EXPORT_SYMBOL(aal_l3_te_cb_dqsch_voq_thrsh_profile_set);
EXPORT_SYMBOL(aal_l3_te_cb_dqsch_voq_thrsh_profile_get);
EXPORT_SYMBOL(aal_l3_te_cb_dqsch_voq_thrsh_profile_sel_get);
EXPORT_SYMBOL(aal_l3_te_cb_td_sts_get);
EXPORT_SYMBOL(aal_l3_te_cb_no_prvt_buf_sts_get);
EXPORT_SYMBOL(aal_l3_te_cb_all_threshold_get);
EXPORT_SYMBOL(aal_l3_te_cb_voq_pcnt_get);
#if 0//defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(l3te_voq_thrsh_profile_3_thh);
EXPORT_SYMBOL(l3te_port_thrsh_profile_3_lth);
#endif


