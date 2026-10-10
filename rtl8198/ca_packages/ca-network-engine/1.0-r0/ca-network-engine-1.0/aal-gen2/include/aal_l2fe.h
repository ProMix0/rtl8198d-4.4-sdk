#ifndef __AAL_L2FE_H__
#define __AAL_L2FE_H__
#include "util_table.h"
#include "ca_types.h"


typedef enum {
    AAL_L2FE_MONITOR_MODE_FINAL     = 0,
    AAL_L2FE_MONITOR_MODE_NEXT      = 1,
    AAL_L2FE_MONITOR_MODE_MAX       = AAL_L2FE_MONITOR_MODE_NEXT
} aal_l2fe_monitor_mode_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rev                :7;
        ca_uint32_t da_bit23_47        :25;
#else
        ca_uint32_t da_bit23_47        :25;
        ca_uint32_t rev                :7;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l2_vector6_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t da_bit0_22         :23;
        ca_uint32_t sa_bit39_47        :9;
#else
        ca_uint32_t sa_bit39_47        :9;
        ca_uint32_t da_bit0_22         :23;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l2_vector5_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t sa_bit7_38         :32;
#else
        ca_uint32_t sa_bit7_38         :32;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l2_vector4_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t sa_bit0_6          :7;
        ca_uint32_t top_tpid           :16;
        ca_uint32_t top_sc_ind         :1;
        ca_uint32_t top_vid_bit4_11    :8;
#else
        ca_uint32_t top_vid_bit4_11    :8;
        ca_uint32_t top_sc_ind         :1;
        ca_uint32_t top_tpid           :16;
        ca_uint32_t sa_bit0_6          :7;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l2_vector3_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t top_vid_bit0_3     :4;        
        ca_uint32_t top_8021p          :3;
        ca_uint32_t top_dei            :1;
        ca_uint32_t inner_tpid         :16;
        ca_uint32_t inner_sc_ind       :1;
        ca_uint32_t inner_vid_bit5_11  :7;
#else
        ca_uint32_t inner_vid_bit5_11  :7;
        ca_uint32_t inner_sc_ind       :1;
        ca_uint32_t inner_tpid         :16;
        ca_uint32_t top_dei            :1;
        ca_uint32_t top_8021p          :3;
        ca_uint32_t top_vid_bit0_3     :4;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l2_vector2_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t inner_vid_bit0_4   :5;        
        ca_uint32_t inner_8021p        :3;
        ca_uint32_t inner_dei          :1;
        ca_uint32_t t3rd_tpid          :16;
        ca_uint32_t t3rd_vid_bit5_11   :7;
#else
        ca_uint32_t t3rd_vid_bit5_11   :7;
        ca_uint32_t t3rd_tpid          :16;
        ca_uint32_t inner_dei          :1;
        ca_uint32_t inner_8021p        :3;
        ca_uint32_t inner_vid_bit0_4   :5;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l2_vector1_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t t3rd_vid_bit0_4    :5;
        ca_uint32_t etype              :16;
        ca_uint32_t etype_vld          :1;
        ca_uint32_t tag_flags          :3;
        ca_uint32_t priority_vlan      :1;
        ca_uint32_t eth_pkt_type       :2;
        ca_uint32_t ieee_mc_addr       :4;
#else
        ca_uint32_t ieee_mc_addr       :4;
        ca_uint32_t eth_pkt_type       :2;
        ca_uint32_t priority_vlan      :1;
        ca_uint32_t tag_flags          :3;
        ca_uint32_t etype_vld          :1;
        ca_uint32_t etype              :16;
        ca_uint32_t t3rd_vid_bit0_4    :5;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l2_vector0_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rsv                       :19;
        ca_uint32_t ip_ttl_hoplimit           :8;
        ca_uint32_t ip_da_bit123_127          :5;
#else
        ca_uint32_t ip_da_bit123_127          :5;
        ca_uint32_t ip_ttl_hoplimit           :8;
        ca_uint32_t rsv                       :19;  
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector10_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_da_bit91_122           :32;
#else
        ca_uint32_t ip_da_bit91_122           :32;  
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector9_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_da_bit59_90            :32;
#else
        ca_uint32_t ip_da_bit59_90            :32;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector8_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_da_bit27_58            :32;
#else
        ca_uint32_t ip_da_bit27_58            :32;  
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector7_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_da_bit0_26             :27;
        ca_uint32_t ip_sa_bit123_127          :5;
#else
        ca_uint32_t ip_sa_bit123_127          :5;
        ca_uint32_t ip_da_bit0_26             :27;  
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector6_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_sa_bit91_122           :32;
#else
        ca_uint32_t ip_sa_bit91_122           :32;
       
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector5_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_sa_bit59_90            :32;
#else
        ca_uint32_t ip_sa_bit59_90            :32;  
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector4_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_sa_bit27_58            :32;
#else
        ca_uint32_t ip_sa_bit27_58            :32;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector3_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_sa_bit0_26             :27;
        ca_uint32_t ip_tos_bit1_5             :5;
#else
        ca_uint32_t ip_tos_bit1_5             :5;
        ca_uint32_t ip_sa_bit0_26             :27;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector2_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ip_tos_bit0               :1;
        ca_uint32_t ip_ecn                    :2;
        ca_uint32_t ip_mc                     :1;
        ca_uint32_t ip_prot                   :8;
        ca_uint32_t ip_type                   :3;
        ca_uint32_t ipv4_option               :1;
        ca_uint32_t ipv4_frag                 :1;
        ca_uint32_t ipv4_dec                  :1;
        ca_uint32_t ipv6_flow_label_bit6_19   :14;
#else
        ca_uint32_t ipv6_flow_label_bit6_19   :14;
        ca_uint32_t ipv4_dec                  :1;
        ca_uint32_t ipv4_frag                 :1;
        ca_uint32_t ipv4_option               :1;
        ca_uint32_t ip_type                   :3;
        ca_uint32_t ip_prot                   :8;
        ca_uint32_t ip_mc                     :1;
        ca_uint32_t ip_ecn                    :2;
        ca_uint32_t ip_tos_bit0               :1;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector1_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t ipv6_flow_label_bit0_5    :6;
        ca_uint32_t l3_vld                    :1;
        ca_uint32_t rsv                       :25;
#else
        ca_uint32_t rsv                       :25;
        ca_uint32_t l3_vld                    :1;
        ca_uint32_t ipv6_flow_label_bit0_5    :6;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l3_vector0_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rev                       :9;
        ca_uint32_t l4_vld                    :1;
        ca_uint32_t l4_dp                     :16;
        ca_uint32_t l4_sp_bit10_15            :6;
#else
        ca_uint32_t l4_sp_bit10_15            :6;
        ca_uint32_t l4_dp                     :16;
        ca_uint32_t l4_vld                    :1;
        ca_uint32_t rev                       :9;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l4_vector1_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t l4_sp_bit0_9              :10;
        ca_uint32_t udp_pkt_vld               :1;
        ca_uint32_t tcp_pkt_vld               :1;
        ca_uint32_t tcp_flags_vld             :1;
        ca_uint32_t tcp_flags                 :6;
        ca_uint32_t rev                       :13;
#else
        ca_uint32_t rev                       :13;
        ca_uint32_t tcp_flags                 :6;
        ca_uint32_t tcp_flags_vld             :1;
        ca_uint32_t tcp_pkt_vld               :1;
        ca_uint32_t udp_pkt_vld               :1;
        ca_uint32_t l4_sp_bit0_9              :10;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_l4_vector0_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rsv                       :4;
        ca_uint32_t l2_a_bit4_31              :28;
#else
        ca_uint32_t l2_a_bit4_31              :28;
        ca_uint32_t rsv                       :4;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_plus_vector5_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t l2_a_bit0_3               :4;
        ca_uint32_t l2_a_vld                  :1;
        ca_uint32_t l2_b_bit5_31              :27;
#else
        ca_uint32_t l2_b_bit5_31              :27;
        ca_uint32_t l2_a_vld                  :1;
        ca_uint32_t l2_a_bit0_3               :4;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_plus_vector4_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t l2_b_bit0_4               :5;
        ca_uint32_t l2_b_vld                  :1;
        ca_uint32_t l3_bit6_31                :26;
#else
        ca_uint32_t l3_bit6_31                :26;
        ca_uint32_t l2_b_vld                  :1;
        ca_uint32_t l2_b_bit0_4               :5;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_plus_vector3_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t l3_bit0_5                 :6;
        ca_uint32_t l3_vld                    :1;
        ca_uint32_t l7_bit7_31                :25;
#else
        ca_uint32_t l7_bit7_31                :25;
        ca_uint32_t l3_vld                    :1;
        ca_uint32_t l3_bit0_5                 :6;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_plus_vector2_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t l7_bit0_6                 :7;
        ca_uint32_t l7_vld                    :1;
        ca_uint32_t prog_bit8_31              :24;
#else
        ca_uint32_t prog_bit8_31              :24;
        ca_uint32_t l7_vld                    :1;
        ca_uint32_t l7_bit0_6                 :7;
    
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_plus_vector1_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t prog_bit0_7               :8;
        ca_uint32_t prog_vld                  :1;
        ca_uint32_t rsv                       :23;
#else
        ca_uint32_t rsv                       :23;
        ca_uint32_t prog_vld                  :1;
        ca_uint32_t prog_bit0_7               :8;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_plus_vector0_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rsv                       :30;
        ca_uint32_t l2fe_monitor_trig_ltch    :1;
        ca_uint32_t rsv_bit5                  :1;
#else
        ca_uint32_t rsv_bit5                  :1;
        ca_uint32_t l2fe_monitor_trig_ltch    :1;
        ca_uint32_t rsv                       :30;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_misc_vector4_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rsv_bit0_4                :5;
        ca_uint32_t no_drop                   :1;
        ca_uint32_t mirred                    :1;
        ca_uint32_t mark                      :1;
        ca_uint32_t drop_ind                  :1;
        ca_uint32_t drop_src                  :5;
        ca_uint32_t rx_dot1p_map_vec          :7;
        ca_uint32_t rx_dscp_map_vec           :11;
#else
        ca_uint32_t rx_dscp_map_vec           :11;
        ca_uint32_t rx_dot1p_map_vec          :7;
        ca_uint32_t drop_src                  :5;
        ca_uint32_t drop_ind                  :1;
        ca_uint32_t mark                      :1;
        ca_uint32_t mirred                    :1;
        ca_uint32_t no_drop                   :1;
        ca_uint32_t rsv_bit0_4                :5;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_misc_vector3_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t spcl_pkt_type             :5;
        ca_uint32_t lspid                     :6;        
        ca_uint32_t phy_src                   :1;
        ca_uint32_t cpu_src                   :1;
        ca_uint32_t mc_src                    :1;
        ca_uint32_t l3_offset                 :8;
        ca_uint32_t hdra_pspid                :4;
        ca_uint32_t hdra_flow_en              :1;
        ca_uint32_t hdra_flowid_bit7_11       :5;
#else
        ca_uint32_t hdra_flowid_bit7_11       :5;
        ca_uint32_t hdra_flow_en              :1;
        ca_uint32_t hdra_pspid                :4;
        ca_uint32_t l3_offset                 :8;
        ca_uint32_t mc_src                    :1;
        ca_uint32_t cpu_src                   :1;
        ca_uint32_t phy_src                   :1;
        ca_uint32_t lspid                     :6;
        ca_uint32_t spcl_pkt_type             :5;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_misc_vector2_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t hdra_flowid_bit0_6        :7;
        ca_uint32_t hdra_mc_flag              :1;
        ca_uint32_t hdra_mcgid                :10;
        ca_uint32_t hdra_fe_bypass            :1;
        ca_uint32_t hdra_ldpid                :6;
        ca_uint32_t hdra_cos                  :3;
        ca_uint32_t l3fe_lrn_dis              :1;
        ca_uint32_t hdra_dbuf_en              :1;
        ca_uint32_t hdra_pr_idx_bit4_5        :2;
#else
        ca_uint32_t hdra_pr_idx_bit4_5        :2;
        ca_uint32_t hdra_dbuf_en              :1;
        ca_uint32_t l3fe_lrn_dis              :1;
        ca_uint32_t hdra_cos                  :3;
        ca_uint32_t hdra_ldpid                :6;
        ca_uint32_t hdra_fe_bypass            :1;
        ca_uint32_t hdra_mcgid                :10;
        ca_uint32_t hdra_mc_flag              :1;
        ca_uint32_t hdra_flowid_bit0_6        :7;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_misc_vector1_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t hdra_pr_idx_bit0_3        :4;
        ca_uint32_t egr_ptp_flag              :1;
        ca_uint32_t rsv                       :27;
#else
        ca_uint32_t rsv                       :27;
        ca_uint32_t egr_ptp_flag              :1;
        ca_uint32_t hdra_pr_idx_bit0_3        :4;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_misc_vector0_t;

typedef union 
{
    struct 
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rsv                           :19;
        ca_uint32_t wan_ind                       :1;
        ca_uint32_t rx_qos_idx                    :3;
        ca_uint32_t flooding_enhance_en           :1;
        ca_uint32_t flooding_enhance_mc_idx       :5;
        ca_uint32_t unkwn_pol_mode                :1;
        ca_uint32_t rsv_1                         :2;
#else
        ca_uint32_t rsv_1                         :2;
        ca_uint32_t unkwn_pol_mode                :1;
        ca_uint32_t flooding_enhance_mc_idx       :5;
        ca_uint32_t flooding_enhance_en           :1;
        ca_uint32_t rx_qos_idx                    :3;
        ca_uint32_t wan_ind                       :1;
        ca_uint32_t rsv                           :19;
#endif
    } s ;
    ca_uint32_t     u32 ;
} aal_l2fe_ilpb_vector4_t;

typedef union 
{
    struct 
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rsv                           :10;
        ca_uint32_t gemid_dynamic_mode            :1;
        ca_uint32_t flowid_as_gemid               :1;
        ca_uint32_t unkwn_pol_idx                 :3;
        ca_uint32_t default_vlanid                :12;
        ca_uint32_t sc_ind                        :1;
        ca_uint32_t vlan_cmd_select_bm            :2;
        ca_uint32_t default_top_vlan_cmd          :2;
#else
        ca_uint32_t default_top_vlan_cmd          :2;
        ca_uint32_t vlan_cmd_select_bm            :2;
        ca_uint32_t sc_ind                        :1;
        ca_uint32_t default_vlanid                :12;
        ca_uint32_t unkwn_pol_idx                 :3;
        ca_uint32_t flowid_as_gemid               :1;
        ca_uint32_t gemid_dynamic_mode            :1;
        ca_uint32_t rsv                           :10;
#endif
    } s ;
    ca_uint32_t     u32 ;
} aal_l2fe_ilpb_vector3_t;

typedef union 
{
    struct 
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t default_untag_cmd             :1;
        ca_uint32_t inner_vid_select_bm           :2;
        ca_uint32_t default_inner_vlan_cmd        :2;
        ca_uint32_t default_noinnertag_cmd        :1;
        ca_uint32_t drop_single_tag_pkt           :1;
        ca_uint32_t drop_stag_packet              :1;
        ca_uint32_t drop_ctag_packet              :1;
        ca_uint32_t drop_untagged_packet          :1;
        ca_uint32_t drop_prytagged_packet         :1;
        ca_uint32_t drop_doubletagged_packet      :1;
        ca_uint32_t s_tpid_match                  :4;
        ca_uint32_t c_tpid_match                  :4;
        ca_uint32_t top_s_tpid_enable             :1;
        ca_uint32_t top_c_tpid_enable             :1;
        ca_uint32_t inner_s_tpid_enable           :1;
        ca_uint32_t inner_c_tpid_enable           :1;
        ca_uint32_t other_tpid_match              :4;
        ca_uint32_t stp_mode                      :2;
        ca_uint32_t station_move_en               :1;
        ca_uint32_t sa_move_dis_fwd               :1;
#else
        ca_uint32_t sa_move_dis_fwd               :1;
        ca_uint32_t station_move_en               :1;
        ca_uint32_t stp_mode                      :2;
        ca_uint32_t other_tpid_match              :4;
        ca_uint32_t inner_c_tpid_enable           :1;
        ca_uint32_t inner_s_tpid_enable           :1;
        ca_uint32_t top_c_tpid_enable             :1;
        ca_uint32_t top_s_tpid_enable             :1;
        ca_uint32_t c_tpid_match                  :4;
        ca_uint32_t s_tpid_match                  :4;
        ca_uint32_t drop_doubletagged_packet      :1;
        ca_uint32_t drop_prytagged_packet         :1;
        ca_uint32_t drop_untagged_packet          :1;
        ca_uint32_t drop_ctag_packet              :1;
        ca_uint32_t drop_stag_packet              :1;
        ca_uint32_t drop_single_tag_pkt           :1;
        ca_uint32_t default_noinnertag_cmd        :1;
        ca_uint32_t default_inner_vlan_cmd        :2;
        ca_uint32_t inner_vid_select_bm           :2;
        ca_uint32_t default_untag_cmd             :1;
#endif
    } s ;
    ca_uint32_t     u32 ;
} aal_l2fe_ilpb_vector2_t;

typedef union 
{
    struct 
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t loop_enable                   :1;
        ca_uint32_t da_sa_swap_en                 :1;
        ca_uint32_t spcl_pkt_idx                  :4;
        ca_uint32_t igr_cls_lookup_en             :1;
        ca_uint32_t igr_cls_start                 :8;
        ca_uint32_t igr_cls_length                :6;
        ca_uint32_t flowid_sel_bm                 :3;
        ca_uint32_t top_802_1p_mode_sel           :2;
        ca_uint32_t change_1p_if_pop              :1;
        ca_uint32_t mark_802_1p_control_bm        :4;
        ca_uint32_t inner_802_1p_mode_sel_bit1    :1;
#else
        ca_uint32_t inner_802_1p_mode_sel_bit1    :1;
        ca_uint32_t mark_802_1p_control_bm        :4;
        ca_uint32_t change_1p_if_pop              :1;
        ca_uint32_t top_802_1p_mode_sel           :2;
        ca_uint32_t flowid_sel_bm                 :3;
        ca_uint32_t igr_cls_length                :6;
        ca_uint32_t igr_cls_start                 :8;
        ca_uint32_t igr_cls_lookup_en             :1;
        ca_uint32_t spcl_pkt_idx                  :4;
        ca_uint32_t da_sa_swap_en                 :1;
        ca_uint32_t loop_enable                   :1;
#endif
    } s ;
    ca_uint32_t     u32 ;
} aal_l2fe_ilpb_vector1_t;

typedef union 
{
    struct 
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t inner_802_1p_mode_sel_bit0    :1;
        ca_uint32_t default_802_1p                :3;
        ca_uint32_t dscp_markdown_en              :1;
        ca_uint32_t dscp_mode_sel                 :2;
        ca_uint32_t dscp_mark_control_bm          :4;
        ca_uint32_t default_dscp                  :6;
        ca_uint32_t cos_mode_sel                  :3;
        ca_uint32_t cos_control_bm                :3;
        ca_uint32_t default_cos                   :3;
        ca_uint32_t ing_fltr_ena                  :1;
        ca_uint32_t color_src_sel                 :2;
        ca_uint32_t dflt_premark                  :1;
        ca_uint32_t rsv                           :2;
#else
        ca_uint32_t rsv                           :2;
        ca_uint32_t dflt_premark                  :1;
        ca_uint32_t color_src_sel                 :2;
        ca_uint32_t ing_fltr_ena                  :1;
        ca_uint32_t default_cos                   :3;
        ca_uint32_t cos_control_bm                :3;
        ca_uint32_t cos_mode_sel                  :3;
        ca_uint32_t default_dscp                  :6;
        ca_uint32_t dscp_mark_control_bm          :4;
        ca_uint32_t dscp_mode_sel                 :2;
        ca_uint32_t dscp_markdown_en              :1;
        ca_uint32_t default_802_1p                :3;
        ca_uint32_t inner_802_1p_mode_sel_bit0    :1;
#endif
    } s ;
    ca_uint32_t     u32 ;
} aal_l2fe_ilpb_vector0_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rsv                           :30;
        ca_uint32_t igr_ve_tbl_hit                :1;
        ca_uint32_t igr_ve_tbl_hit_id_bit1        :1;
#else
        ca_uint32_t igr_ve_tbl_hit_id_bit1        :1;
        ca_uint32_t igr_ve_tbl_hit                :1;
        ca_uint32_t rsv                           :30;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_ve_tbl_rst1_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t igr_ve_tbl_hit_id_bit0        :1;
        ca_uint32_t igr_ve_hit_SVL_VL_tbl_addr    :7;
        ca_uint32_t igr_ve_hit_CVL_tbl_addr       :7;
        ca_uint32_t egr_ve_tbl_hit                :1;
        ca_uint32_t egr_ve_tbl_hit_id             :2;
        ca_uint32_t egr_ve_hit_SVL_VL_tbl_addr    :7;
        ca_uint32_t egr_ve_hit_CVL_tbl_addr       :7;

#else
        ca_uint32_t egr_ve_hit_CVL_tbl_addr       :7;
        ca_uint32_t egr_ve_hit_SVL_VL_tbl_addr    :7;
        ca_uint32_t egr_ve_tbl_hit_id             :2;
        ca_uint32_t egr_ve_tbl_hit                :1;
        ca_uint32_t igr_ve_hit_CVL_tbl_addr       :7;
        ca_uint32_t igr_ve_hit_SVL_VL_tbl_addr    :7;
        ca_uint32_t igr_ve_tbl_hit_id_bit0        :1;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_ve_tbl_rst0_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t rev                          :13;
        ca_uint32_t l2e_sa_hit                   :1;
        ca_uint32_t l2e_sw_lrn_frst_pkt          :1;
        ca_uint32_t l2e_sw_lrn_othr_pkt          :1;
        ca_uint32_t l2e_hash_cllsn               :1;
        ca_uint32_t l2e_limit_excd               :1;
        ca_uint32_t l2e_max_port_sa_lmt          :13;
        ca_uint32_t l2e_port_sa_cnt_bit12        :1;
#else
        ca_uint32_t l2e_port_sa_cnt_bit12        :1;
        ca_uint32_t l2e_max_port_sa_lmt          :13;
        ca_uint32_t l2e_limit_excd               :1;
        ca_uint32_t l2e_hash_cllsn               :1;
        ca_uint32_t l2e_sw_lrn_othr_pkt          :1;
        ca_uint32_t l2e_sw_lrn_frst_pkt          :1;
        ca_uint32_t l2e_sa_hit                   :1;
        ca_uint32_t rev                          :13;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_fdb_tbl_rst3_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t l2e_port_sa_cnt_bit0_11      :12;
        ca_uint32_t l2e_entry_aging_time         :8;
        ca_uint32_t l2e_st_mv_window             :9;
        ca_uint32_t l2e_st_mv                    :1;
        ca_uint32_t l2e_sa_permit                :1;
        ca_uint32_t l2e_static                   :1;
#else
        ca_uint32_t l2e_static                   :1;
        ca_uint32_t l2e_sa_permit                :1;
        ca_uint32_t l2e_st_mv                    :1;
        ca_uint32_t l2e_st_mv_window             :9;
        ca_uint32_t l2e_entry_aging_time         :8;
        ca_uint32_t l2e_port_sa_cnt_bit0_11      :12;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_fdb_tbl_rst2_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t l2e_hit_static_entry_sm      :1;
        ca_uint32_t l2e_hit_tmp_entry_sm         :1;
        ca_uint32_t l2e_hit_sm_entry_sm          :1;
        ca_uint32_t dbg_sa_hit_addr              :13;
        ca_uint32_t rsv                          :8;
        ca_uint32_t l2e_da_hit                   :1;
        ca_uint32_t l2e_da_permit                :1;
        ca_uint32_t l2e_mc_flag                  :1;
        ca_uint32_t l2e_mcgid_bit5_9             :5;
#else
        ca_uint32_t l2e_mcgid_bit5_9             :5;
        ca_uint32_t l2e_mc_flag                  :1;
        ca_uint32_t l2e_da_permit                :1;
        ca_uint32_t l2e_da_hit                   :1;
        ca_uint32_t rsv                          :8;
        ca_uint32_t dbg_sa_hit_addr              :13;
        ca_uint32_t l2e_hit_sm_entry_sm          :1;
        ca_uint32_t l2e_hit_tmp_entry_sm         :1;
        ca_uint32_t l2e_hit_static_entry_sm      :1;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_fdb_tbl_rst1_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t l2e_mcgid_bit0_4             :5;
        ca_uint32_t l2e_vlan_id                  :12;
        ca_uint32_t l2e_vlan_id_type             :1;
        ca_uint32_t l2e_untag                    :1;
        ca_uint32_t dbg_da_hit_addr              :13;
#else
        ca_uint32_t dbg_da_hit_addr              :13;
        ca_uint32_t l2e_untag                    :1;
        ca_uint32_t l2e_vlan_id_type             :1;
        ca_uint32_t l2e_vlan_id                  :12;
        ca_uint32_t l2e_mcgid_bit0_4             :5;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_fdb_tbl_rst0_t;

typedef union
{
    struct
    {
#ifdef CS_BIG_ENDIAN
        ca_uint32_t mc_tbl_hit               :1;
        ca_uint32_t mc_tbl_rsv               :25;
        ca_uint32_t mc_tbl_hit_addr          :6;
#else
        ca_uint32_t mc_tbl_hit_addr          :6;
        ca_uint32_t mc_tbl_rsv               :25;
        ca_uint32_t mc_tbl_hit               :1;
#endif
    }s;
    ca_uint32_t        u32;
}aal_l2fe_mc_tbl_rst_t;

ca_status_t aal_l2fe_monitor_mode_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_l2fe_monitor_mode_t mode
);

ca_status_t aal_l2fe_monitor_mode_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_l2fe_monitor_mode_t *mode
);

ca_status_t aal_l2fe_l2_vector_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_l3_vector_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_l4_vector_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_plus_vector_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_misc_vector_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_ilpb_vector_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_vlan_result_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_cls_result_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_l2_result_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_acl_result_get(
    CA_IN ca_device_id_t device_id
);

ca_status_t aal_l2fe_mc_fib_result_get(
    CA_IN ca_device_id_t device_id
);

#endif
