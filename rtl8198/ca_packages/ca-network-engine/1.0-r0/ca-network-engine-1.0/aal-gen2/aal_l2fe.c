#include "osal_common.h"
#include "aal_common.h"
#include "util_table.h"
#include <soc/cortina/registers.h>
#include <soc/cortina/cortina-soc.h>
#include "aal_l2fe.h"

/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  data declaration
%%____________________________________________________________________________*/

/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  macro
%%____________________________________________________________________________*/

/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  functions
%%____________________________________________________________________________*/

ca_status_t aal_l2fe_monitor_mode_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_l2fe_monitor_mode_t mode
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_TRIG_t l2fe_monitor_trig;

    if(mode == AAL_L2FE_MONITOR_MODE_FINAL)
    {
        l2fe_monitor_trig.wrd = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_TRIG);

        l2fe_monitor_trig.bf.mode = mode;

        CA_NE_REG_WRITE(l2fe_monitor_trig.wrd,L2FE_GLB_L2FE_MONITOR_TRIG);
    }
    else if(mode == AAL_L2FE_MONITOR_MODE_NEXT)
    {
        l2fe_monitor_trig.wrd = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_TRIG);

        l2fe_monitor_trig.bf.mode = mode;
        l2fe_monitor_trig.bf.en ^= 1;

        CA_NE_REG_WRITE(l2fe_monitor_trig.wrd,L2FE_GLB_L2FE_MONITOR_TRIG);
    }
    
    return CA_E_OK;
}

ca_status_t aal_l2fe_monitor_mode_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_l2fe_monitor_mode_t *mode
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_TRIG_t l2fe_monitor_trig;

    l2fe_monitor_trig.wrd = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_TRIG);

    *mode = l2fe_monitor_trig.bf.mode;

    return CA_E_OK;
}

ca_status_t aal_l2fe_l2_vector_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;

    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    aal_l2fe_l2_vector0_t l2fe_l2_vector0;
    aal_l2fe_l2_vector1_t l2fe_l2_vector1;
    aal_l2fe_l2_vector2_t l2fe_l2_vector2;
    aal_l2fe_l2_vector3_t l2fe_l2_vector3;
    aal_l2fe_l2_vector4_t l2fe_l2_vector4;
    aal_l2fe_l2_vector5_t l2fe_l2_vector5;
    aal_l2fe_l2_vector6_t l2fe_l2_vector6;
    ca_uint32_t tmp_da_hi;
    ca_uint32_t tmp_da_lo;
    ca_uint32_t tmp_sa_hi;
    ca_uint32_t tmp_sa_lo;

    /*L2_VECTOR[216:0]*/
    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 0;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l2_vector0.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(0+1)*32-1,0*32,l2fe_l2_vector0.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 1;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l2_vector1.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(1+1)*32-1,1*32,l2fe_l2_vector1.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 2;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l2_vector2.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(2+1)*32-1,2*32,l2fe_l2_vector2.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 3;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l2_vector3.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(3+1)*32-1,3*32,l2fe_l2_vector3.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 4;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l2_vector4.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(4+1)*32-1,4*32,l2fe_l2_vector4.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 5;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l2_vector5.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(5+1)*32-1,5*32,l2fe_l2_vector5.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 6;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l2_vector6.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(6+1)*32-1,6*32,l2fe_l2_vector6.u32);

    tmp_da_hi = l2fe_l2_vector6.s.da_bit23_47 >> 9;
    tmp_da_lo = l2fe_l2_vector5.s.da_bit0_22 + (l2fe_l2_vector6.s.da_bit23_47<<23);
    ca_printf("da = 0x%04x%08x\n",tmp_da_hi,tmp_da_lo);
    
    tmp_sa_hi = (l2fe_l2_vector5.s.sa_bit39_47<<7) + (l2fe_l2_vector4.s.sa_bit7_38>>25);
    tmp_sa_lo = l2fe_l2_vector3.s.sa_bit0_6 + (l2fe_l2_vector4.s.sa_bit7_38<<7);
    ca_printf("sa = 0x%04x%08x\n",tmp_sa_hi,tmp_sa_lo);

    ca_printf("top_tpid= 0x%04x top_sc_ind= %d top_vid= %lld top_8021p= %d top_dei= %d\n"
        ,l2fe_l2_vector3.s.top_tpid
        ,l2fe_l2_vector3.s.top_sc_ind
        ,l2fe_l2_vector2.s.top_vid_bit0_3 + (l2fe_l2_vector3.s.top_vid_bit4_11<<4)
        ,l2fe_l2_vector2.s.top_8021p
        ,l2fe_l2_vector2.s.top_dei);

    ca_printf("inner_tpid= 0x%04x inner_sc_ind= %d inner_vid= %lld inner_8021p= %d inner_dei= %d\n"
        ,l2fe_l2_vector2.s.inner_tpid
        ,l2fe_l2_vector2.s.inner_sc_ind
        ,l2fe_l2_vector1.s.inner_vid_bit0_4 + (l2fe_l2_vector2.s.inner_vid_bit5_11<<5)
        ,l2fe_l2_vector1.s.inner_8021p
        ,l2fe_l2_vector1.s.inner_dei);

    ca_printf("3rd_tpid= 0x%04x 3rd_vid= %lld\n"
        ,l2fe_l2_vector1.s.t3rd_tpid,l2fe_l2_vector0.s.t3rd_vid_bit0_4 + (l2fe_l2_vector1.s.t3rd_vid_bit5_11 <<5));

    ca_printf("etype= 0x%04x etype_vld= %d tag_flags= %d priority_vlan= %d eth_pkt_type= %d ieee_mc_addr= %d\n"
        ,l2fe_l2_vector0.s.etype 
        ,l2fe_l2_vector0.s.etype_vld
        ,l2fe_l2_vector0.s.tag_flags
        ,l2fe_l2_vector0.s.priority_vlan
        ,l2fe_l2_vector0.s.eth_pkt_type
        ,l2fe_l2_vector0.s.ieee_mc_addr);

    return CA_E_OK;
}

ca_status_t aal_l2fe_l3_vector_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    aal_l2fe_l3_vector0_t l2fe_l3_vector0;
    aal_l2fe_l3_vector1_t l2fe_l3_vector1;
    aal_l2fe_l3_vector2_t l2fe_l3_vector2;
    aal_l2fe_l3_vector3_t l2fe_l3_vector3;
    aal_l2fe_l3_vector4_t l2fe_l3_vector4;
    aal_l2fe_l3_vector5_t l2fe_l3_vector5;
    aal_l2fe_l3_vector6_t l2fe_l3_vector6;
    aal_l2fe_l3_vector7_t l2fe_l3_vector7;
    aal_l2fe_l3_vector8_t l2fe_l3_vector8;
    aal_l2fe_l3_vector9_t l2fe_l3_vector9;
    aal_l2fe_l3_vector10_t l2fe_l3_vector10;
    
    /*L3_VECTOR[516:217]*/
    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 6;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector0.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(6+1)*32-1,6*32,l2fe_l3_vector0.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 7;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector1.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(7+1)*32-1,7*32,l2fe_l3_vector1.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 8;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector2.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(8+1)*32-1,8*32,l2fe_l3_vector2.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 9;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector3.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(9+1)*32-1,9*32,l2fe_l3_vector3.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 10;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector4.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(10+1)*32-1,10*32,l2fe_l3_vector4.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 11;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector5.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(11+1)*32-1,11*32,l2fe_l3_vector5.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 12;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector6.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(12+1)*32-1,12*32,l2fe_l3_vector6.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 13;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector7.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(13+1)*32-1,13*32,l2fe_l3_vector7.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 14;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector8.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(14+1)*32-1,14*32,l2fe_l3_vector8.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 15;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector9.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(15+1)*32-1,15*32,l2fe_l3_vector9.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 16;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l3_vector10.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(16+1)*32-1,16*32,l2fe_l3_vector10.u32);

    ca_printf("ip_da= %03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d\n"
        ,(l2fe_l3_vector10.s.ip_da_bit123_127<<3) + (l2fe_l3_vector9.s.ip_da_bit91_122>>29)&0xff
        ,(l2fe_l3_vector9.s.ip_da_bit91_122>>21)&0xff
        ,(l2fe_l3_vector9.s.ip_da_bit91_122>>13)&0xff
        ,(l2fe_l3_vector9.s.ip_da_bit91_122>>5)&0xff
        ,((l2fe_l3_vector9.s.ip_da_bit91_122&0x1f)<<3) + (l2fe_l3_vector8.s.ip_da_bit59_90>>29)&0x7
        ,(l2fe_l3_vector8.s.ip_da_bit59_90>>21)&0xff
        ,(l2fe_l3_vector8.s.ip_da_bit59_90>>13)&0xff
        ,(l2fe_l3_vector8.s.ip_da_bit59_90>>5)&0xff
        ,((l2fe_l3_vector8.s.ip_da_bit59_90&0x1f)<<3) + (l2fe_l3_vector7.s.ip_da_bit27_58>>29)&0xff
        ,(l2fe_l3_vector7.s.ip_da_bit27_58>>21)&0xff
        ,(l2fe_l3_vector7.s.ip_da_bit27_58>>13)&0xff
        ,(l2fe_l3_vector7.s.ip_da_bit27_58>>5)&0xff
        ,((l2fe_l3_vector7.s.ip_da_bit27_58&0x1f)<<3) + (l2fe_l3_vector6.s.ip_da_bit0_26>>24)&0xff
        ,(l2fe_l3_vector6.s.ip_da_bit0_26>>16)&0xff
        ,(l2fe_l3_vector6.s.ip_da_bit0_26>>8)&0xff
        ,l2fe_l3_vector6.s.ip_da_bit0_26&0xff);

    ca_printf("ip_sa= %03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d.%03d\n"
        ,(l2fe_l3_vector6.s.ip_sa_bit123_127<<3) + (l2fe_l3_vector5.s.ip_sa_bit91_122>>29)&0xff
        ,(l2fe_l3_vector5.s.ip_sa_bit91_122>>21)&0xff
        ,(l2fe_l3_vector5.s.ip_sa_bit91_122>>13)&0xff
        ,(l2fe_l3_vector5.s.ip_sa_bit91_122>>5)&0xff
        ,((l2fe_l3_vector5.s.ip_sa_bit91_122&0x1f)<<3) + (l2fe_l3_vector4.s.ip_sa_bit59_90>>29)&0x7
        ,(l2fe_l3_vector4.s.ip_sa_bit59_90>>21)&0xff
        ,(l2fe_l3_vector4.s.ip_sa_bit59_90>>13)&0xff
        ,(l2fe_l3_vector4.s.ip_sa_bit59_90>>5)&0xff
        ,((l2fe_l3_vector4.s.ip_sa_bit59_90&0x1f)<<3) + (l2fe_l3_vector3.s.ip_sa_bit27_58>>29)&0xff
        ,(l2fe_l3_vector3.s.ip_sa_bit27_58>>21)&0xff
        ,(l2fe_l3_vector3.s.ip_sa_bit27_58>>13)&0xff
        ,(l2fe_l3_vector3.s.ip_sa_bit27_58>>5)&0xff
        ,((l2fe_l3_vector3.s.ip_sa_bit27_58&0x1f)<<3) + (l2fe_l3_vector2.s.ip_sa_bit0_26>>24)&0xff
        ,(l2fe_l3_vector2.s.ip_sa_bit0_26>>16)&0xff
        ,(l2fe_l3_vector2.s.ip_sa_bit0_26>>8)&0xff
        ,l2fe_l3_vector2.s.ip_sa_bit0_26&0xff);

    ca_printf("ip_ttl_hoplimit= %d ip_tos= %d ip_ecn= %d ip_mc= %d ip_prot= %d ip_type= %d\n"
        ,l2fe_l3_vector10.s.ip_ttl_hoplimit,l2fe_l3_vector1.s.ip_tos_bit0 + (l2fe_l3_vector2.s.ip_tos_bit1_5<<1)
        ,l2fe_l3_vector1.s.ip_ecn,l2fe_l3_vector1.s.ip_mc,l2fe_l3_vector1.s.ip_prot,l2fe_l3_vector1.s.ip_type);

    ca_printf("ipv4_option= %d ipv4_frag= %d ipv4_dec= %d ipv6_flow_label= %d l3_vld= %d\n"
        ,l2fe_l3_vector1.s.ipv4_option,l2fe_l3_vector1.s.ipv4_frag,l2fe_l3_vector1.s.ipv4_dec
        ,(l2fe_l3_vector1.s.ipv6_flow_label_bit6_19<<6) + l2fe_l3_vector0.s.ipv6_flow_label_bit0_5,l2fe_l3_vector0.s.l3_vld);


    return CA_E_OK;
}

ca_status_t aal_l2fe_l4_vector_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    aal_l2fe_l4_vector0_t l2fe_l4_vector0;
    aal_l2fe_l4_vector1_t l2fe_l4_vector1;

    /*L4_VECTOR[555:517]*/
    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 16;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l4_vector0.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(16+1)*32-1,16*32,l2fe_l4_vector0.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 17;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_l4_vector1.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(17+1)*32-1,17*32,l2fe_l4_vector1.u32);

    ca_printf("l4_vld= %d l4_dp= %d l4_sp= %d\n"
        ,l2fe_l4_vector1.s.l4_vld
        ,l2fe_l4_vector1.s.l4_dp
        ,l2fe_l4_vector0.s.l4_sp_bit0_9 + (l2fe_l4_vector1.s.l4_sp_bit10_15<<10));

    ca_printf("udp_pkt_vld= %d tcp_pkt_vld= %d tcp_flags_vld= %d tcp_flags= 0x%x\n"
        ,l2fe_l4_vector0.s.udp_pkt_vld
        ,l2fe_l4_vector0.s.tcp_pkt_vld
        ,l2fe_l4_vector0.s.tcp_flags_vld
        ,l2fe_l4_vector0.s.tcp_flags);

    return CA_E_OK;
}

ca_status_t aal_l2fe_plus_vector_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    aal_l2fe_plus_vector0_t l2fe_plus_vector0;
    aal_l2fe_plus_vector1_t l2fe_plus_vector1;
    aal_l2fe_plus_vector2_t l2fe_plus_vector2;
    aal_l2fe_plus_vector3_t l2fe_plus_vector3;
    aal_l2fe_plus_vector4_t l2fe_plus_vector4;
    aal_l2fe_plus_vector5_t l2fe_plus_vector5;

    /*PLUS_VECTOR[720:556]*/
    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 17;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_plus_vector0.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(17+1)*32-1,17*32,l2fe_plus_vector0.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 18;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_plus_vector1.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(18+1)*32-1,18*32,l2fe_plus_vector1.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 19;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_plus_vector2.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(19+1)*32-1,19*32,l2fe_plus_vector2.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 20;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_plus_vector3.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(20+1)*32-1,20*32,l2fe_plus_vector3.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 21;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_plus_vector4.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(21+1)*32-1,21*32,l2fe_plus_vector4.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 22;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_plus_vector5.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(22+1)*32-1,22*32,l2fe_plus_vector5.u32);

    ca_printf("prog_vld=%d prog=0x%08x\n"
        ,l2fe_plus_vector0.s.prog_vld
        ,l2fe_plus_vector0.s.prog_bit0_7 + (l2fe_plus_vector1.s.prog_bit8_31<<8));

    ca_printf("l7_vld=%d l7=0x%08x\n"
        ,l2fe_plus_vector1.s.l7_vld
        ,l2fe_plus_vector1.s.l7_bit0_6 + (l2fe_plus_vector2.s.l7_bit7_31<<7));

    ca_printf("l3_vld=%d l3=0x%08x\n"
        ,l2fe_plus_vector2.s.l3_vld
        ,l2fe_plus_vector2.s.l3_bit0_5 + (l2fe_plus_vector3.s.l3_bit6_31<<6));

    ca_printf("l2_b_vld=%d l2_b=0x%08x\n"
        ,l2fe_plus_vector3.s.l2_b_vld
        ,l2fe_plus_vector3.s.l2_b_bit0_4 + (l2fe_plus_vector4.s.l2_b_bit5_31<<5));

    ca_printf("l2_a_vld=%d l2_a=0x%08x\n"
        ,l2fe_plus_vector4.s.l2_a_vld
        ,l2fe_plus_vector4.s.l2_a_bit0_3 + (l2fe_plus_vector5.s.l2_a_bit4_31<<4));

    return CA_E_OK;
}

ca_status_t aal_l2fe_misc_vector_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    aal_l2fe_misc_vector0_t l2fe_misc_vector0;
    aal_l2fe_misc_vector1_t l2fe_misc_vector1;
    aal_l2fe_misc_vector2_t l2fe_misc_vector2;
    aal_l2fe_misc_vector3_t l2fe_misc_vector3;
    aal_l2fe_misc_vector4_t l2fe_misc_vector4;

    /*MISC_VECTOR[822:721]*/
    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 22;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_misc_vector0.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(22+1)*32-1,22*32,l2fe_misc_vector0.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 23;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_misc_vector1.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(23+1)*32-1,23*32,l2fe_misc_vector1.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 24;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_misc_vector2.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(24+1)*32-1,24*32,l2fe_misc_vector2.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 25;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_misc_vector3.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(25+1)*32-1,25*32,l2fe_misc_vector3.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 26;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_misc_vector4.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(26+1)*32-1,26*32,l2fe_misc_vector4.u32);

    ca_printf("l2fe_monitor_trig_ltch=%d\n",l2fe_misc_vector4.s.l2fe_monitor_trig_ltch);

    ca_printf("no_drop= %d mirred= %d mark= %d drop_ind= %d drop_src= %d rx_dot1p_map_vec= %d rx_dscp_map_vec= %d\n"
        ,l2fe_misc_vector3.s.no_drop
        ,l2fe_misc_vector3.s.mirred
        ,l2fe_misc_vector3.s.mark
        ,l2fe_misc_vector3.s.drop_ind
        ,l2fe_misc_vector3.s.drop_src
        ,l2fe_misc_vector3.s.rx_dot1p_map_vec
        ,l2fe_misc_vector3.s.rx_dscp_map_vec);

    ca_printf("spcl_pkt_type= %d lspid= 0x%x phy_src= %d cpu_src= %d mc_src= %d l3_offset= %d\n"
        ,l2fe_misc_vector2.s.spcl_pkt_type
        ,l2fe_misc_vector2.s.lspid
        ,l2fe_misc_vector2.s.phy_src
        ,l2fe_misc_vector2.s.cpu_src
        ,l2fe_misc_vector2.s.mc_src
        ,l2fe_misc_vector2.s.l3_offset);

    ca_printf("hdra_pspid= 0x%x hdra_flow_en= %d hdra_flowid= 0x%x\n"
        ,l2fe_misc_vector2.s.hdra_pspid
        ,l2fe_misc_vector2.s.hdra_flow_en
        ,l2fe_misc_vector1.s.hdra_flowid_bit0_6 + (l2fe_misc_vector2.s.hdra_flowid_bit7_11<<7));

    ca_printf("hdra_mc_flag= %d hdra_mcgid= 0x%x hdra_fe_bypass= %d hdra_ldpid= 0x%x hdra_cos= %d l3fe_lrn_dis= %d hdra_dbuf_en= %d\n"
        ,l2fe_misc_vector1.s.hdra_mc_flag
        ,l2fe_misc_vector1.s.hdra_mcgid
        ,l2fe_misc_vector1.s.hdra_fe_bypass
        ,l2fe_misc_vector1.s.hdra_ldpid
        ,l2fe_misc_vector1.s.hdra_cos
        ,l2fe_misc_vector1.s.l3fe_lrn_dis
        ,l2fe_misc_vector1.s.hdra_dbuf_en);

    ca_printf("hdra_pr_idx= 0x%x egr_ptp_flag= %d\n"
        ,l2fe_misc_vector0.s.hdra_pr_idx_bit0_3 + (l2fe_misc_vector1.s.hdra_pr_idx_bit4_5<<4)
        ,l2fe_misc_vector0.s.egr_ptp_flag );

    return CA_E_OK;
}

ca_status_t aal_l2fe_ilpb_vector_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    aal_l2fe_ilpb_vector0_t l2fe_ilpb_vector0;
    aal_l2fe_ilpb_vector1_t l2fe_ilpb_vector1;
    aal_l2fe_ilpb_vector2_t l2fe_ilpb_vector2;
    aal_l2fe_ilpb_vector3_t l2fe_ilpb_vector3;
    aal_l2fe_ilpb_vector4_t l2fe_ilpb_vector4;

    /*ILPB_VECTOR[962:823]*/
    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 26;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_ilpb_vector0.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(26+1)*32-1,26*32,l2fe_ilpb_vector0.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 27;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_ilpb_vector1.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(27+1)*32-1,27*32,l2fe_ilpb_vector1.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 28;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_ilpb_vector2.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(28+1)*32-1,28*32,l2fe_ilpb_vector2.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 29;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_ilpb_vector3.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(29+1)*32-1,29*32,l2fe_ilpb_vector3.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel |= 30;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_ilpb_vector4.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%04d:%04d] 0x%08x\n",(30+1)*32-1,30*32,l2fe_ilpb_vector4.u32);

    ca_printf("wan_ind=%d rx_qos_idx=%d flooding_enhance_en=%d flooding_enhance_mc_idx=%d unkwn_pol_mode=%d\n"
        ,l2fe_ilpb_vector4.s.wan_ind
        ,l2fe_ilpb_vector4.s.rx_qos_idx
        ,l2fe_ilpb_vector4.s.flooding_enhance_en
        ,l2fe_ilpb_vector4.s.flooding_enhance_mc_idx
        ,l2fe_ilpb_vector4.s.unkwn_pol_mode);

    ca_printf("gemid_dynamic_mode=%d flowid_as_gemid=%d unkwn_pol_idx=%d default_vlanid=%d sc_ind=%d vlan_cmd_select_bm=%d default_top_vlan_cmd=%d\n"
        ,l2fe_ilpb_vector3.s.gemid_dynamic_mode
        ,l2fe_ilpb_vector3.s.flowid_as_gemid
        ,l2fe_ilpb_vector3.s.unkwn_pol_idx
        ,l2fe_ilpb_vector3.s.default_vlanid
        ,l2fe_ilpb_vector3.s.sc_ind
        ,l2fe_ilpb_vector3.s.vlan_cmd_select_bm
        ,l2fe_ilpb_vector3.s.default_top_vlan_cmd);

    ca_printf("default_untag_cmd=%d inner_vid_select_bm=%d default_inner_vlan_cmd=%d default_noinnertag_cmd=%d drop_single_tag_pkt=%d drop_stag_packet=%d drop_ctag_packet=%d drop_untagged_packet=%d drop_prytagged_packet=%d drop_doubletagged_packet=%d s_tpid_match=%d c_tpid_match=%d top_s_tpid_enable=%d top_c_tpid_enable=%d inner_s_tpid_enable=%d inner_c_tpid_enable=%d other_tpid_match=%d stp_mode=%d station_move_en=%d sa_move_dis_fwd=%d\n"
        ,l2fe_ilpb_vector2.s.default_untag_cmd
        ,l2fe_ilpb_vector2.s.inner_vid_select_bm
        ,l2fe_ilpb_vector2.s.default_inner_vlan_cmd
        ,l2fe_ilpb_vector2.s.default_noinnertag_cmd
        ,l2fe_ilpb_vector2.s.drop_single_tag_pkt
        ,l2fe_ilpb_vector2.s.drop_stag_packet
        ,l2fe_ilpb_vector2.s.drop_ctag_packet
        ,l2fe_ilpb_vector2.s.drop_untagged_packet
        ,l2fe_ilpb_vector2.s.drop_prytagged_packet
        ,l2fe_ilpb_vector2.s.drop_doubletagged_packet
        ,l2fe_ilpb_vector2.s.s_tpid_match
        ,l2fe_ilpb_vector2.s.c_tpid_match
        ,l2fe_ilpb_vector2.s.top_s_tpid_enable
        ,l2fe_ilpb_vector2.s.top_c_tpid_enable
        ,l2fe_ilpb_vector2.s.inner_s_tpid_enable
        ,l2fe_ilpb_vector2.s.inner_c_tpid_enable
        ,l2fe_ilpb_vector2.s.other_tpid_match
        ,l2fe_ilpb_vector2.s.stp_mode
        ,l2fe_ilpb_vector2.s.station_move_en
        ,l2fe_ilpb_vector2.s.sa_move_dis_fwd);

    ca_printf("loop_enable=%d da_sa_swap_en=%d spcl_pkt_idx=%d igr_cls_lookup_en=%d igr_cls_start=%d igr_cls_length=%d flowid_sel_bm=%d top_802_1p_mode_sel=%d change_1p_if_pop=%d mark_802_1p_control_bm=%d inner_802_1p_mode_sel=%d\n"
        ,l2fe_ilpb_vector1.s.loop_enable
        ,l2fe_ilpb_vector1.s.da_sa_swap_en
        ,l2fe_ilpb_vector1.s.spcl_pkt_idx
        ,l2fe_ilpb_vector1.s.igr_cls_lookup_en
        ,l2fe_ilpb_vector1.s.igr_cls_start
        ,l2fe_ilpb_vector1.s.igr_cls_length
        ,l2fe_ilpb_vector1.s.flowid_sel_bm
        ,l2fe_ilpb_vector1.s.top_802_1p_mode_sel
        ,l2fe_ilpb_vector1.s.change_1p_if_pop
        ,l2fe_ilpb_vector1.s.mark_802_1p_control_bm
        ,l2fe_ilpb_vector0.s.inner_802_1p_mode_sel_bit0 + (l2fe_ilpb_vector1.s.inner_802_1p_mode_sel_bit1<<1) );

    ca_printf("default_802_1p=%d dscp_markdown_en=%d dscp_mode_sel=%d dscp_mark_control_bm=%d default_dscp=%d cos_mode_sel=%d cos_control_bm=%d default_cos=%d ing_fltr_ena=%d color_src_sel=%d dflt_premark=%d\n"
        ,l2fe_ilpb_vector0.s.default_802_1p
        ,l2fe_ilpb_vector0.s.dscp_markdown_en
        ,l2fe_ilpb_vector0.s.dscp_mode_sel
        ,l2fe_ilpb_vector0.s.dscp_mark_control_bm
        ,l2fe_ilpb_vector0.s.default_dscp
        ,l2fe_ilpb_vector0.s.cos_mode_sel
        ,l2fe_ilpb_vector0.s.cos_control_bm
        ,l2fe_ilpb_vector0.s.default_cos
        ,l2fe_ilpb_vector0.s.ing_fltr_ena
        ,l2fe_ilpb_vector0.s.color_src_sel
        ,l2fe_ilpb_vector0.s.dflt_premark);

    return CA_E_OK;
}

ca_status_t aal_l2fe_vlan_result_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    aal_l2fe_ve_tbl_rst0_t l2fe_ve_tbl_rst0;
    aal_l2fe_ve_tbl_rst1_t l2fe_ve_tbl_rst1;
    
    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel = 1<<5;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_ve_tbl_rst0.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%03d:%03d] 0x%08x\n",31,0,l2fe_ve_tbl_rst0.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel = (1<<5) | 1;
    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_ve_tbl_rst1.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%03d:%03d] 0x%08x\n",63,32,l2fe_ve_tbl_rst1.u32);

    ca_printf("igr_ve_tbl_hit=%d\n",l2fe_ve_tbl_rst1.s.igr_ve_tbl_hit);
    ca_printf("igr_ve_tbl_hit_id=0x%x\n",l2fe_ve_tbl_rst0.s.igr_ve_tbl_hit_id_bit0 + (l2fe_ve_tbl_rst1.s.igr_ve_tbl_hit_id_bit1<<1));
    ca_printf("igr_ve_hit_SVL_VL_tbl_addr=0x%x\n",l2fe_ve_tbl_rst0.s.igr_ve_hit_SVL_VL_tbl_addr);
    ca_printf("igr_ve_hit_CVL_tbl_addr=0x%x\n",l2fe_ve_tbl_rst0.s.igr_ve_hit_CVL_tbl_addr);

    ca_printf("egr_ve_tbl_hit=%d\n",l2fe_ve_tbl_rst0.s.egr_ve_tbl_hit);
    ca_printf("egr_ve_tbl_hit_id=0x%x\n",l2fe_ve_tbl_rst0.s.egr_ve_tbl_hit_id);
    ca_printf("egr_ve_hit_SVL_VL_tbl_addr=0x%x\n",l2fe_ve_tbl_rst0.s.egr_ve_hit_SVL_VL_tbl_addr);
    ca_printf("egr_ve_hit_CVL_tbl_addr=0x%x\n",l2fe_ve_tbl_rst0.s.egr_ve_hit_CVL_tbl_addr);

    return CA_E_OK; 
}

ca_status_t aal_l2fe_cls_result_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    L2FE_GLB_L2FE_MONITOR_RETURN_t l2fe_monitor_return;
    ca_uint8_t i;

    for(i=0;i<8;i++)
    {
        l2fe_monitor_ctrl.wrd = 0;
        l2fe_monitor_ctrl.bf.bus_sel = (2<<5) | i;

        CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);

        l2fe_monitor_return.wrd = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

        ca_printf("[%03d:%03d] %08x\n",(i+1)*32-1,i*32,l2fe_monitor_return.wrd);
    }

    return CA_E_OK; 
}

ca_status_t aal_l2fe_l2_result_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    L2FE_GLB_L2FE_MONITOR_RETURN_t l2fe_monitor_return;
    aal_l2fe_fdb_tbl_rst0_t l2fe_fdb_tbl_rst0;
    aal_l2fe_fdb_tbl_rst1_t l2fe_fdb_tbl_rst1;
    aal_l2fe_fdb_tbl_rst2_t l2fe_fdb_tbl_rst2;
    aal_l2fe_fdb_tbl_rst3_t l2fe_fdb_tbl_rst3;


    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel = (3<<5) | 0;

    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_fdb_tbl_rst0.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%03d:%03d] 0x%08x\n",31,0,l2fe_fdb_tbl_rst0.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel = (3<<5) | 1;

    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_fdb_tbl_rst1.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%03d:%03d] 0x%08x\n",63,32,l2fe_fdb_tbl_rst1.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel = (3<<5) | 2;

    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_fdb_tbl_rst2.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%03d:%03d] 0x%08x\n",95,64,l2fe_fdb_tbl_rst2.u32);

    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel = (3<<5) | 3;

    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);
    l2fe_fdb_tbl_rst3.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%03d:%03d] 0x%08x\n",127,96,l2fe_fdb_tbl_rst3.u32);

    ca_printf("l2e_mcgid=%d l2e_vlan_id=%d l2e_vlan_id_type=%d l2e_untag=%d dbg_da_hit_addr=%d\n"
        ,l2fe_fdb_tbl_rst0.s.l2e_mcgid_bit0_4 + (l2fe_fdb_tbl_rst1.s.l2e_mcgid_bit5_9 <<5)
        ,l2fe_fdb_tbl_rst0.s.l2e_vlan_id
        ,l2fe_fdb_tbl_rst0.s.l2e_vlan_id_type
        ,l2fe_fdb_tbl_rst0.s.l2e_untag
        ,l2fe_fdb_tbl_rst0.s.dbg_da_hit_addr);

    ca_printf("l2e_hit_static_entry_sm=%d l2e_hit_tmp_entry_sm=%d l2e_hit_sm_entry_sm=%d dbg_sa_hit_addr=%d l2e_da_hit=%d l2e_da_permit=%d l2e_mc_flag=%d\n"
        ,l2fe_fdb_tbl_rst1.s.l2e_hit_static_entry_sm
        ,l2fe_fdb_tbl_rst1.s.l2e_hit_tmp_entry_sm
        ,l2fe_fdb_tbl_rst1.s.l2e_hit_sm_entry_sm
        ,l2fe_fdb_tbl_rst1.s.dbg_sa_hit_addr
        ,l2fe_fdb_tbl_rst1.s.l2e_da_hit
        ,l2fe_fdb_tbl_rst1.s.l2e_da_permit
        ,l2fe_fdb_tbl_rst1.s.l2e_mc_flag );

    ca_printf("l2e_port_sa_cnt=%lld l2e_entry_aging_time=%d l2e_st_mv_window=%d l2e_st_mv=%d l2e_sa_permit=%d l2e_static=%d\n"
        ,l2fe_fdb_tbl_rst2.s.l2e_port_sa_cnt_bit0_11 + (l2fe_fdb_tbl_rst3.s.l2e_port_sa_cnt_bit12<<12)
        ,l2fe_fdb_tbl_rst2.s.l2e_entry_aging_time
        ,l2fe_fdb_tbl_rst2.s.l2e_st_mv_window
        ,l2fe_fdb_tbl_rst2.s.l2e_st_mv
        ,l2fe_fdb_tbl_rst2.s.l2e_sa_permit
        ,l2fe_fdb_tbl_rst2.s.l2e_static);
 
    ca_printf("l2e_sa_hit=%d l2e_sw_lrn_frst_pkt=%d l2e_sw_lrn_othr_pkt=%d l2e_hash_cllsn=%d l2e_limit_excd=%d l2e_max_port_sa_lmt=%d\n"
        ,l2fe_fdb_tbl_rst3.s.l2e_sa_hit
        ,l2fe_fdb_tbl_rst3.s.l2e_sw_lrn_frst_pkt
        ,l2fe_fdb_tbl_rst3.s.l2e_sw_lrn_othr_pkt
        ,l2fe_fdb_tbl_rst3.s.l2e_hash_cllsn
        ,l2fe_fdb_tbl_rst3.s.l2e_limit_excd
        ,l2fe_fdb_tbl_rst3.s.l2e_max_port_sa_lmt);

    return CA_E_OK; 
}

ca_status_t aal_l2fe_acl_result_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    L2FE_GLB_L2FE_MONITOR_RETURN_t l2fe_monitor_return;
    ca_uint8_t i;

    for(i=0;i<8;i++)
    {
        l2fe_monitor_ctrl.wrd = 0;
        l2fe_monitor_ctrl.bf.bus_sel = (4<<5) | i;

        CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);

        l2fe_monitor_return.wrd = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

        ca_printf("[%03d:%03d] 0x%08x\n",(i+1)*32-1,i*32,l2fe_monitor_return.wrd);
    }

    return CA_E_OK; 
}

ca_status_t aal_l2fe_mc_fib_result_get(
    CA_IN ca_device_id_t device_id
)
{
    ca_status_t ret;
    L2FE_GLB_L2FE_MONITOR_CTRL_t l2fe_monitor_ctrl;
    aal_l2fe_mc_tbl_rst_t l2fe_mc_tbl_rst;
    
    l2fe_monitor_ctrl.wrd = 0;
    l2fe_monitor_ctrl.bf.bus_sel = 5<<5;

    CA_NE_REG_WRITE(l2fe_monitor_ctrl.wrd,L2FE_GLB_L2FE_MONITOR_CTRL);

    l2fe_mc_tbl_rst.u32 = CA_NE_REG_READ(L2FE_GLB_L2FE_MONITOR_RETURN);

    ca_printf("[%03d:%03d] 0x%08x\n",31,0,l2fe_mc_tbl_rst.u32);

    ca_printf("mc_tbl_hit=%d mc_tbl_hit_addr=0x%02x\n",l2fe_mc_tbl_rst.s.mc_tbl_hit,l2fe_mc_tbl_rst.s.mc_tbl_hit_addr);

    return CA_E_OK; 
}

#ifdef __KERNEL__
EXPORT_SYMBOL(aal_l2fe_monitor_mode_set);
EXPORT_SYMBOL(aal_l2fe_monitor_mode_get);
EXPORT_SYMBOL(aal_l2fe_l2_vector_get);
EXPORT_SYMBOL(aal_l2fe_l3_vector_get);
EXPORT_SYMBOL(aal_l2fe_l4_vector_get);
EXPORT_SYMBOL(aal_l2fe_plus_vector_get);
EXPORT_SYMBOL(aal_l2fe_misc_vector_get);
EXPORT_SYMBOL(aal_l2fe_ilpb_vector_get);
EXPORT_SYMBOL(aal_l2fe_vlan_result_get);
EXPORT_SYMBOL(aal_l2fe_cls_result_get);
EXPORT_SYMBOL(aal_l2fe_l2_result_get);
EXPORT_SYMBOL(aal_l2fe_acl_result_get);
EXPORT_SYMBOL(aal_l2fe_mc_fib_result_get);
#endif

