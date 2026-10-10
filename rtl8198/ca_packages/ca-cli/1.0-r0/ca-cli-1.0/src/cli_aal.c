#include "ca_types.h"
#include "cli_product.h"
#include "osal_common.h"
#ifdef CLI_KERNEL_SPACE
#ifdef CONFIG_RTK_MIPS
#include <soc/cortina/registers.h>
#endif
#endif

/*lint -save -w0*/

/******************************************************/


/*__AAL_CLI_DECLARE_CODE_GEN__*/

#ifndef CLI_KERNEL_SPACE
#ifdef CONFIG_LUNA_G3_SERIES
#define CLI_CMD_LENGTH 160
#endif

CLI_DEFINE(enter_aal_node,
           enter_aal_node_cmd,
           "aal",
           "Enter aal node\n"
          )
{
    cli_enter_node(cli, CLI_NODE_ID_AAL, "-aal");
    return CLI_CMD_OK;
}

CLI_DEFINE(register_user_read,
           register_user_read_cmd,
#ifdef CONFIG_LUNA_G3_SERIES
           "reg_op read  REGISTER [<1-65535>]",
#else
           "reg_op read  <0xf3000000-0xf8000000> [<1-65535>]",
#endif
           "Register operate\n"
           "Read register\n"
           "Register address\n"
           "How many words do you want to read\n"
          )
{
    int mem_fd = 0;
    void *usr_mapmem = NULL;
    unsigned long addr;
    unsigned int count = 0;
    unsigned int *p_value;
    unsigned int offset;
    int i;

#ifdef CONFIG_LUNA_G3_SERIES
    FILE *fp;
    ca_int8 cmd[CLI_CMD_LENGTH]="", reg_addr[32]="0";

    // input is register ADDRESS: <0xf3000000-0xf8000000>
    if ((strlen(argv[0]) == 10) && (argv[0][0] =='0')) {
      addr = ca_strtol((ca_int8 *)argv[0]);
      if ((addr == 0) || (addr < 0xf3000000) || (addr > 0xf8000000))
          return CLI_CMD_OK;
    }
    //input is REGISTER NAME
    else 
    {
      snprintf(cmd, CLI_CMD_LENGTH-1, "grep \"^%s \" /etc/reg.txt |awk '{print $2}'", argv[0]);
      fp = popen(cmd, "r");
      if (fp == NULL) {
        return CLI_CMD_OK;
      }
      if(fgets(reg_addr, sizeof(reg_addr), fp) == NULL) {
        pclose(fp); 
        return CLI_CMD_OK;
      }
      pclose(fp);

      reg_addr[10]=0;
      addr = ca_strtol((ca_int8 *)reg_addr);
      if (addr == 0)
          return CLI_CMD_OK;
    }
#else
    // should be 0xfxxxyyyy
    if (strlen(argv[0]) != 10) {
        return CLI_CMD_OK;
    }

    addr = ca_strtol((ca_int8 *)argv[0]);
    if (addr == 0)
        return CLI_CMD_OK;
#endif
    if (argc == 1)
        count = 1;
    else
        count = ca_strtol((ca_int8 *)argv[1]);

    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR) ) < 0) {
        cli_print(cli, "FATAL: can't open /dev/mem \n");
        return CLI_CMD_OK;
    }

    usr_mapmem = (unsigned char*)mmap(NULL, 0x3300000, (PROT_READ | PROT_WRITE), MAP_SHARED, mem_fd, 0xf3000000);
    p_value = usr_mapmem;

    offset  = addr - 0xf3000000;
    p_value += (offset / 4);

    cli_print(cli, "\r\n");
    for (i = 0; i < count; i++)
    {
        if (i % 4 == 0)
        {
            cli_print(cli, "%08x: ", addr + i * 4);
        }

        cli_print(cli, "%08x ", *p_value);
        p_value += 1;
        if ((i + 1) % 4 == 0)
            cli_print(cli, "\r\n");
    }
    cli_print(cli, "\r\n");

    if (munmap(usr_mapmem, 0x3300000))
    {
        cli_print(cli, "unmap fail!\n");
    }

    close(mem_fd);

    return CLI_CMD_OK;
}

CLI_DEFINE(register_user_write,
           register_user_write_cmd,
#ifdef CONFIG_LUNA_G3_SERIES
           "reg_op write REGISTER <0-0xffffffff> [<1-65535>]",
#else
           "reg_op write <0xf3000000-0xf8000000> <0-0xffffffff> [<1-65535>]",
#endif
           "Register operate\n"
           "Write register\n"
           "Register address\n"
           "Register value\n"
           "How many words do you want to write\n"
          )
{
    int mem_fd = 0;
    void *usr_mapmem = NULL;
    unsigned long addr;
    unsigned int count = 0;
    unsigned int value;
    unsigned int *p_value;
    unsigned int offset;
    int i;

#ifdef CONFIG_LUNA_G3_SERIES
    FILE *fp;
    ca_int8 cmd[64]="", reg_addr[32]="0";

    // input is register ADDRESS: <0xf3000000-0xf8000000>
    if ((strlen(argv[0]) == 10) && (argv[0][0] =='0')) {
      addr = ca_strtol((ca_int8 *)argv[0]);
      if ((addr == 0) || (addr < 0xf3000000) || (addr > 0xf8000000))
          return CLI_CMD_OK;
    }
    //input is REGISTER NAME
    else 
    {
      sprintf(cmd, "grep %s /etc/reg.txt |awk '{print $2}'", argv[0]);
      fp = popen(cmd, "r");
      if (fp == NULL) {
        return CLI_CMD_OK;
      }
      if(fgets(reg_addr, sizeof(reg_addr), fp) == NULL) {
        pclose(fp); 
        return CLI_CMD_OK;
      }
      pclose(fp);

      reg_addr[10]=0;
      addr = ca_strtol((ca_int8 *)reg_addr);
      if (addr == 0)
          return CLI_CMD_OK;
    }
#else
    if (strlen(argv[0]) != 10) {
        return CLI_CMD_OK;
    }

    addr = ca_strtol((ca_int8 *)argv[0]);
    if (addr == 0)
        return CLI_CMD_OK;
#endif

    value = ca_strtol((ca_int8 *)argv[1]);
    if (argc == 2)
        count = 1;
    else
        count = ca_strtol((ca_int8 *)argv[2]);

    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR) ) < 0) {
        cli_print(cli, "FATAL: can't open /dev/mem \n");
        return CLI_CMD_OK;
    }

    usr_mapmem = (unsigned char*)mmap(NULL, 0x3300000, (PROT_READ | PROT_WRITE), MAP_SHARED, mem_fd, 0xf3000000);
    p_value = usr_mapmem;

    offset  = addr - 0xf3000000;
    p_value += (offset / 4);

    for (i = 0; i < count; i++)
    {
        *p_value = value;
        p_value += 1;
    }

    if (munmap(usr_mapmem, 0x3300000))
    {
        cli_print(cli, "unmap fail!\n");
    }

    close(mem_fd);
    return CLI_CMD_OK;
}

#ifdef CONFIG_LUNA_G3_SERIES
CLI_DEFINE(register_user_field,
           register_user_field_cmd,
           "reg_op field REGISTER <0-31> <0-31> [<0-0xffffffff>]",
           "Register operate\n"
           "Field operate\n"
           "Register address\n"
           "MSB of the register\n"
           "LSB of the register\n"
           "Register field value\n"
          )
{
    int mem_fd = 0;
    void *usr_mapmem = NULL;
    unsigned long addr;
    unsigned int count = 0;
    unsigned int value;
    unsigned int *p_value;
    unsigned int offset;
    unsigned int msb;
    unsigned int lsb;
    unsigned int mask, mask_r;

    FILE *fp;
    ca_int8 cmd[64]="", reg_addr[32]="0";

    // input is register ADDRESS: <0xf3000000-0xf8000000>
    if ((strlen(argv[0]) == 10) && (argv[0][0] =='0')) {
      addr = ca_strtol((ca_int8 *)argv[0]);
      if ((addr == 0) || (addr < 0xf3000000) || (addr > 0xf8000000))
          return CLI_CMD_OK;
    }
    //input is REGISTER NAME
    else 
    {
      sprintf(cmd, "grep %s /etc/reg.txt |awk '{print $2}'", argv[0]);
      fp = popen(cmd, "r");
      if (fp == NULL) {
        return CLI_CMD_OK;
      }
      if(fgets(reg_addr, sizeof(reg_addr), fp) == NULL) {
        pclose(fp); 
        return CLI_CMD_OK;
      }
      pclose(fp);

      reg_addr[10]=0;
      addr = ca_strtol((ca_int8 *)reg_addr);
      if (addr == 0)
          return CLI_CMD_OK;
    }

    msb =  ca_strtol((ca_int8 *)argv[1]);
    lsb =  ca_strtol((ca_int8 *)argv[2]);
    if(msb < lsb)
		return CLI_CMD_E_PARAM;
    mask = ((0xffffffff << (31-msb)) >> (31-msb+lsb) << lsb);

    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR) ) < 0) {
        cli_print(cli, "FATAL: can't open /dev/mem \n");
        return CLI_CMD_OK;
    }

    usr_mapmem = (unsigned char*)mmap(NULL, 0x3300000, (PROT_READ | PROT_WRITE), MAP_SHARED, mem_fd, 0xf3000000);
    p_value = usr_mapmem;

    offset  = addr - 0xf3000000;
    p_value += (offset / 4);

    if (argc == 3) {	//Read field operation
        cli_print(cli, "\r\n%08x[%d-%d]: ", addr, msb, lsb);
        cli_print(cli, "%08x \r\n", ((*p_value) & mask) >> lsb);
    }
    else if(argc == 4) {	//Write field operation
        value = ca_strtol((ca_int8 *)argv[3]);
        value = ((value << lsb) & mask) | ((*p_value) & ~mask);
        *p_value = value;
    }

    if (munmap(usr_mapmem, 0x3300000))
    {
        cli_print(cli, "unmap fail!\n");
    }

    close(mem_fd);

    return CLI_CMD_OK;
}

#endif
#endif

CLI_KERNEL_DEFINE(register_kern_read,
                  register_kern_read_cmd,
                  "reg_op read  <0-0xffffffff> [<1-65535>]",
                  "Register operate\n"
                  "Read register\n"
                  "Register address\n"
                  "How many words do you want to read\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    unsigned long addr;
    unsigned int count = 0;
    int i;

    // should be 0xfxxxyyyy
    if (strlen(argv[0]) != 10) {
        return CLI_CMD_OK;
    }

    addr = ca_strtol((ca_int8 *)argv[0]);
    if (addr == 0)
        return CLI_CMD_OK;

    if (argc == 1)
        count = 1;
    else
        count = ca_strtol((ca_int8 *)argv[1]);


    cli_print(cli, "\r\n");
    for (i = 0; i < count; i++)
    {

        if (i % 4 == 0)
        {
            cli_print(cli, "%08x: ", addr);
        }

        cli_print(cli, "%08x ", CA_REG_READ(addr));

        if ((i + 1) % 4 == 0)
            cli_print(cli, "\r\n");

        addr += 4;
    }
    cli_print(cli, "\r\n");

    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(register_kern_write,
                  register_kern_write_cmd,
                  "reg_op write <0-0xffffffff> <0-0xffffffff> [<1-65535>]",
                  "Register operate\n"
                  "Write register\n"
                  "Register address\n"
                  "Register value\n"
                  "How many words do you want to write\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    unsigned long addr;
    unsigned int count = 0;
    unsigned int value;
    int i;

    if (strlen(argv[0]) != 10) {
        return CLI_CMD_OK;
    }

    addr = ca_strtol((ca_int8 *)argv[0]);
    if (addr == 0)
        return CLI_CMD_OK;

    value = ca_strtol((ca_int8 *)argv[1]);
    if (argc == 2)
        count = 1;
    else
        count = ca_strtol((ca_int8 *)argv[2]);

    for (i = 0; i < count; i++)
    {
        CA_REG_WRITE(value, addr);
        addr += 4;
    }

    return CLI_CMD_OK;
}
#endif

#ifdef CONFIG_RTK_MIPS

#ifdef CLI_KERNEL_SPACE

void _saturn_indirect_mem_rd(ca_uint32_t addr, ca_uint32_t index, ca_uint32_t width, ca_uint32_t *dws)
{
    ca_uint32_t access;
    ca_int32_t i;

    access = 0x80000000 | index;
    CA_REG_WRITE(access, (void *)addr);
    for (i = 0; i < 10000; i++) {
        access = CA_REG_READ((void*)addr);
        if ( (access >> 31) == 0 ) {
            break;
        }
    }
    if (i >= 10000) {
        OS_PRINTF("%s(): access %s timeout.\n", __func__, addr);
    } else {
        for (i = 0; i < width; i++) {
            addr += 4;
            dws[width - i - 1] = CA_REG_READ((void*)addr);
        }
    }
}

void _saturn_indirect_mem_wr(ca_uint32_t addr, ca_uint32_t index, ca_uint32_t width, ca_uint32_t *dws)
{
    ca_uint32_t access;
    ca_uint32_t data;
    ca_int32_t i;

    data = addr;

    for (i = 0; i < width; i++) {
        data += 4;
        CA_REG_WRITE(dws[width - i - 1], (void *)data);
    }

    access = 0xC0000000 | index;
    CA_REG_WRITE(access, (void *)addr);
    for (i = 0; i < 10000; i++) {
        access = CA_REG_READ((void*)addr);
        if ( (access >> 31) == 0 ) {
            break;
        }
    }
    if (i >= 10000) {
        OS_PRINTF("%s(): access %s timeout.\n", __func__, addr);
    }
}

typedef struct _saturn_debug_bf_t {
    ca_uint32_t offset;
    ca_uint32_t length;
    ca_int8_t *name;
} saturn_debug_bf_t;


typedef struct _saturn_debug_tbl_t {
    ca_uint32_t addr;
    ca_uint32_t length;
    ca_uint32_t width;
    ca_int8_t *name;
    ca_uint32_t num;
    saturn_debug_bf_t bf[64];
} saturn_debug_tbl_t;


saturn_debug_tbl_t SATURN_DEBUG_TBL[] = {
    { LDMA_LDMA_QUEUE_DESCRIPTOR_ACCESS, 16, 3, "LDMA_LDMA_QUEUE_DESCRIPTOR", 9, { {0, 32, "src_addr"}, {32, 32, "dst_addr"}, {64, 12, "length"}, {76, 1, "rd_cmd_only"}, {77, 1, "rd_trn_only"}, {78, 1, "wr_zero_trn"}, {79, 1, "ioc"}, {80, 8, "dst_addr_lpae"}, {88, 8, "src_addr_lpae"}, } },
    { LDMA_RCPU_LDMA_QUEUE_DESCRIPTOR_ACCESS, 16, 3, "LDMA_RCPU_LDMA_QUEUE_DESCRIPTOR", 0, NULL },
    { DDR_CTRL_PCTL_BSTC_MEM_ACCESS, 524288, 1, "DDR_CTRL_PCTL_BSTC_MEM", 0, NULL },
    { IIC_SLV_REG_MEM_ACCESS, 512, 1, "IIC_SLV_REG_MEM", 0, NULL },
    { L2FE_PP_IPPB_ACCESS, 64, 1, "L2FE_PP_IPPB", 1, { {0, 6, "lspid"}, } },
    { L2FE_PP_ILPB_ACCESS, 64, 5, "L2FE_PP_ILPB", 52, { {0, 1, "ilpb_dflt_premark"}, {1, 2, "ilpb_color_src_sel"}, {3, 1, "ilpb_ing_fltr_ena"}, {4, 3, "ilpb_default_cos"}, {7, 3, "ilpb_cos_control_bm"}, {10, 3, "ilpb_cos_mode_sel"}, {13, 6, "ilpb_default_dscp"}, {19, 3, "ilpb_dscp_mark_control_bm"}, {22, 2, "ilpb_dscp_mode_sel"}, {24, 1, "ilpb_dscp_markdown_en"}, {25, 3, "ilpb_default_802_1p"}, {28, 2, "ilpb_inner_802_1p_mode_sel"}, {30, 3, "ilpb_802_1p_mark_control_bm"}, {33, 1, "ilpb_change_1p_if_pop"}, {34, 2, "ilpb_top_802_1p_mode_sel"}, {36, 3, "ilpb_flowid_sel_bm"}, {39, 6, "ilpb_cls_length"}, {45, 8, "ilpb_cls_start"}, {53, 1, "ilpb_igr_cls_lookup_en"}, {54, 4, "ilpb_spcl_pkt_idx"}, {58, 1, "ilpb_da_sa_swap_en"}, {59, 1, "ilpb_loop_enable"}, {60, 1, "ilpb_sa_move_dis_fwd"}, {61, 1, "ilpb_station_move_en"}, {62, 2, "ilpb_stp_mode"}, {64, 4, "ilpb_other_tpid_match"}, {68, 1, "ilpb_inner_c_tpid_enable"}, {69, 1, "ilpb_inner_s_tpid_enable"}, {70, 1, "ilpb_top_c_tpid_enable"}, {71, 1, "ilpb_top_s_tpid_enable"}, {72, 4, "ilpb_c_tpid_match"}, {76, 4, "ilpb_s_tpid_match"}, {80, 1, "ilpb_drop_doubletagged_packet"}, {81, 1, "ilpb_drop_prytagged_packet"}, {82, 1, "ilpb_drop_untagged_packet"}, {83, 1, "ilpb_drop_ctag_packet"}, {84, 1, "ilpb_drop_stag_packet"}, {85, 1, "ilpb_drop_singletagged_packet"}, {86, 1, "ilpb_default_noninnertag_cmd"}, {87, 2, "ilpb_inner_vlan_cmd"}, {89, 2, "ilpb_inner_vid_sel_bm"}, {91, 1, "ilpb_default_untag_cmd"}, {92, 2, "ilpb_default_top_vlan_cmd"}, {94, 2, "ilpb_vlan_cmd_sel_bm"}, {96, 1, "ilpb_sc_ind"}, {97, 12, "ilpb_default_vlanid"}, {109, 3, "ilpb_unkwn_pol_idx"}, {112, 1, "ilpb_flowid_as_gemid"}, {113, 1, "ilpb_gemid_dynamic_mode"}, {114, 24, "ilpb_reserved_1"}, {138, 1, "ilpb_wan_ind"}, {139, 1, "ilpb_reserved"}, } },
    { L2FE_PP_DPB_ACCESS, 64, 1, "L2FE_PP_DPB", 3, { {0, 3, "cos"}, {3, 6, "ldpid"}, {9, 1, "bypass"}, } },
    { L2FE_PLA_EV2_PT_CNVT_ACCESS, 32, 1, "L2FE_PLA_EV2_PT_CNVT", 2, { {0, 5, "ev2_pt_cnvt_encoded_value"}, {5, 16, "ev2_pt_cnvt_ethertype"}, } },
    { L2FE_PLA_SPB_ACCESS, 256, 1, "L2FE_PLA_SPB", 14, { {0, 1, "spb_pkt_no_edit"}, {1, 1, "spb_keep_ts"}, {2, 1, "spb_bypass_sa_lrn_chk"}, {3, 1, "spb_limit_fwd_permit"}, {4, 1, "spb_learn_dis"}, {5, 1, "spb_dscp_mark_down"}, {6, 12, "spb_flowid"}, {18, 1, "spb_flow_en"}, {19, 1, "spb_rate_limit_bypass"}, {20, 3, "spb_cos"}, {23, 6, "spb_ldpid"}, {29, 1, "spb_spcl_fwd"}, {30, 1, "spb_pkt_no_drop"}, {31, 1, "spb_hash_cllsn_fwd_permit"}, } },
    { L2FE_PLA_DOT1P_MAP_ACCESS, 16, 1, "L2FE_PLA_DOT1P_MAP", 3, { {0, 1, "dot1p_map_mapped_premark"}, {1, 3, "dot1p_map_mapped_cos"}, {4, 3, "dot1p_map_mapped_1p"}, } },
    { L2FE_PLA_DSCP_MAP_ACCESS, 256, 1, "L2FE_PLA_DSCP_MAP", 4, { {0, 1, "mapped_premark"}, {1, 3, "dscp_map_mapped_cos"}, {4, 1, "dscp_map_marked_flag"}, {5, 6, "dscp_map_mapped_dscp"}, } },
    { L2FE_PLC_ELPB_ACCESS, 64, 1, "L2FE_PLC_ELPB", 10, { {0, 1, "elpb_egr_vlan_aware_mode"}, {1, 1, "elpb_egr_port_stp_status"}, {2, 1, "elpb_egr_ve_srch_en"}, {3, 1, "elpb_egr_dest_wan"}, {4, 6, "elpb_egr_cls_length"}, {10, 8, "elpb_egr_cls_start"}, {18, 1, "elpb_egr_cls_lookup_en"}, {19, 1, "elpb_egr_cls_flowid_sel"}, {20, 1, "elpb_egr_cls_cos_sel"}, {21, 3, "elpb_reserved"}, } },
    { L2FE_PLE_TX_DSCP_MAP_ACCESS, 256, 1, "L2FE_PLE_TX_DSCP_MAP", 2, { {0, 3, "txdscp_map_mapped_cos"}, {3, 3, "txdscp_map_mapped_1p"}, } },
    { L2FE_PLE_TX_DOT1P_MAP_ACCESS, 16, 1, "L2FE_PLE_TX_DOT1P_MAP", 1, { {0, 3, "txdot1p_map_mapped_cos"}, } },
    { L2FE_PLE_DFT_FWD_ACCESS, 256, 1, "L2FE_PLE_DFT_FWD", 4, { {0, 1, "dft_fwd_deny"}, {1, 10, "dft_fwd_mc_group_id"}, {11, 1, "dft_fwd_redir_en"}, {12, 1, "dft_fwd_mc_group_id_valid"}, } },
    { L2FE_PLE_MMSHP_CHK_ID_MAP_ACCESS, 64, 1, "L2FE_PLE_MMSHP_CHK_ID_MAP", 1, { {0, 4, "mmshp_chk_id_map_chk_id"}, } },
    { L2FE_PLE_PORT_MMSHP_CHK_ACCESS, 64, 2, "L2FE_PLE_PORT_MMSHP_CHK", 1, { {0, 64, "ldpid_bm"}, } },
    { L2FE_PLE_TRUNK_DEST_MAP_TBL_ACCESS, 64, 1, "L2FE_PLE_TRUNK_DEST_MAP_TBL", 1, { {0, 6, "ldpid"}, } },
    { L2FE_ARB_REDIR_LDPID_CONFIG_TBL_ACCESS, 64, 1, "L2FE_ARB_REDIR_LDPID_CONFIG_TBL", 5, { {0, 1, "rdir_cos_vld"}, {1, 3, "rdir_cos"}, {4, 6, "rdir_ldpid"}, {10, 1, "rdir_en"}, {11, 1, "rdir_wan_dst"}, } },
    { L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL_ACCESS, 32, 1, "L2FE_ARB_REDIR_DROP_SRC_CONFIG_TBL", 5, { {0, 1, "rdir_cos_vld"}, {1, 3, "rdir_cos"}, {4, 6, "rdir_ldpid"}, {10, 1, "rdir_en"}, {11, 1, "rdir_wan_dst"}, } },
    { L2FE_ARB_NON_KNOWN_POL_MAP_TBL_ACCESS, 32, 1, "L2FE_ARB_NON_KNOWN_POL_MAP_TBL", 1, { {0, 4, "flooding_pol_id"}, } },
    { L2FE_ARB_DSCP_TE_MARK_TBL_ACCESS, 256, 1, "L2FE_ARB_DSCP_TE_MARK_TBL", 1, { {0, 6, "mark_value"}, } },
    { L2FE_ARB_MC_FIB_TBL_ACCESS, 64, 4, "L2FE_ARB_MC_FIB_TBL", 23, { {0, 48, "mac_da"}, {48, 1, "mac_cmd"}, {49, 1, "dei"}, {50, 1, "dei_cmd"}, {51, 3, "dot1p"}, {54, 2, "dot1p_cmd"}, {56, 1, "sc_ind"}, {57, 12, "vid"}, {69, 3, "vlan_cmd"}, {72, 1, "vlan_fltr_en"}, {73, 10, "mcgid"}, {83, 1, "mcgid_en"}, {84, 9, "pol_id"}, {93, 3, "pol_grp_id"}, {96, 1, "pol_en"}, {97, 3, "cos"}, {100, 6, "ldpid"}, {106, 1, "mac_sa_sel"}, {107, 1, "mac_sa_cmd"}, {108, 1, "permit_spid_en"}, {109, 1, "wan_dst"}, {110, 1, "cos_cmd"}, {111, 1, "keep_o_lspid"}, } },
    { L2FE_ARB_FLOW_DBUF_TBL_ACCESS, 128, 1, "L2FE_ARB_FLOW_DBUF_TBL", 4, { {0, 1, "dbuf_flg_0"}, {1, 1, "dbuf_flg_1"}, {2, 1, "dbuf_flg_2"}, {3, 1, "dbuf_flg_3"}, } },
    { L2FE_ARB_PORT_DBUF_TBL_ACCESS, 8, 1, "L2FE_ARB_PORT_DBUF_TBL", 5, { {0, 1, "dbuf_flg"}, {1, 1, "ldpid_vld"}, {2, 6, "ldpid"}, {8, 1, "lspid_vld"}, {9, 6, "lspid"}, } },
    { L2FE_ARB_PDPID_MAP_TBL_ACCESS, 256, 1, "L2FE_ARB_PDPID_MAP_TBL", 1, { {0, 4, "pdpid"}, } },
    { L2FE_PE_DROP_STTS_ACCESS, 64, 1, "L2FE_PE_DROP_STTS", 1, { {0, 32, "cnt"}, } },
    { L2FE_PE_TPID_SEL_CONFIG_TBL_ACCESS, 64, 1, "L2FE_PE_TPID_SEL_CONFIG_TBL", 4, { {0, 2, "ctag"}, {2, 2, "stag"}, {4, 2, "inner_ctag"}, {6, 2, "inner_stag"}, } },
    { L2FE_VE_WAN_SVL_IDX_ACCESS, 4096, 1, "L2FE_VE_WAN_SVL_IDX", 2, { {0, 1, "wan_svl_idx_entry_vld"}, {1, 7, "wan_svl_idx_fib_idx"}, } },
    { L2FE_VE_WAN_SVL_FIB_ACCESS, 128, 4, "L2FE_VE_WAN_SVL_FIB", 28, { {0, 16, "wan_svl_fib_egress_top_untag_bm"}, {16, 16, "wan_svl_fib_membership_bm"}, {32, 10, "wan_svl_fib_vl_group_id"}, {42, 1, "wan_svl_fib_da_sa_swap"}, {43, 1, "wan_svl_fib_permit"}, {44, 1, "wan_svl_fib_markdown_en"}, {45, 1, "wan_svl_fib_flowid_valid"}, {46, 12, "wan_svl_fib_flowid"}, {58, 1, "wan_svl_fib_cos_valid"}, {59, 1, "wan_svl_fib_dscp_valid"}, {60, 1, "wan_svl_fib_802_1p_valid"}, {61, 3, "wan_svl_fib_cos"}, {64, 6, "wan_svl_fib_dscp"}, {70, 3, "wan_svl_fib_802_1p"}, {73, 1, "wan_svl_fib_ldpid_vld"}, {74, 6, "wan_svl_fib_ldpid"}, {80, 1, "wan_svl_fib_mc_vlan_ind"}, {81, 2, "wan_svl_fib_stp_mode"}, {83, 1, "wan_svl_fib_inner_cmd_valid"}, {84, 1, "wan_svl_fib_innervid_usel2"}, {85, 2, "wan_svl_fib_inner_vlan_cmd"}, {87, 1, "wan_svl_fib_top_cmd_valid"}, {88, 1, "wan_svl_fib_topvid_usel2"}, {89, 2, "wan_svl_fib_top_vlan_cmd"}, {91, 1, "wan_svl_fib_sc_ind"}, {92, 12, "wan_svl_fib_vid"}, {104, 1, "wan_svl_fib_do_inner_vid_lookup"}, {105, 1, "wan_svl_fib_reserved"}, } },
    { L2FE_VE_WAN_CVL_IDX_ACCESS, 4096, 1, "L2FE_VE_WAN_CVL_IDX", 2, { {0, 1, "wan_cvl_idx_entry_vld"}, {1, 7, "wan_cvl_idx_fib_idx"}, } },
    { L2FE_VE_WAN_CVL_FIB_ACCESS, 128, 4, "L2FE_VE_WAN_CVL_FIB", 27, { {0, 16, "wan_cvl_fib_egress_top_untag_bm"}, {16, 16, "wan_cvl_fib_membership_bm"}, {32, 10, "wan_cvl_fib_vl_group_id"}, {42, 1, "wan_cvl_fib_da_sa_swap"}, {43, 1, "wan_cvl_fib_permit"}, {44, 1, "wan_cvl_fib_markdown_en"}, {45, 1, "wan_cvl_fib_flowid_valid"}, {46, 12, "wan_cvl_fib_flowid"}, {58, 1, "wan_cvl_fib_cos_valid"}, {59, 1, "wan_cvl_fib_dscp_valid"}, {60, 1, "wan_cvl_fib_802_1p_valid"}, {61, 3, "wan_cvl_fib_cos"}, {64, 6, "wan_cvl_fib_dscp"}, {70, 3, "wan_cvl_fib_802_1p"}, {73, 1, "wan_cvl_fib_ldpid_vld"}, {74, 6, "wan_cvl_fib_ldpid"}, {80, 1, "wan_cvl_fib_mc_vlan_ind"}, {81, 2, "wan_cvl_fib_stp_mode"}, {83, 1, "wan_cvl_fib_inner_cmd_valid"}, {84, 1, "wan_cvl_fib_innervid_usel2"}, {85, 2, "wan_cvl_fib_inner_vlan_cmd"}, {87, 1, "wan_cvl_fib_top_cmd_valid"}, {88, 1, "wan_cvl_fib_topvid_usel2"}, {89, 2, "wan_cvl_fib_top_vlan_cmd"}, {91, 1, "wan_cvl_fib_sc_ind"}, {92, 12, "wan_cvl_fib_vid"}, {104, 2, "wan_cvl_fib_reserved"}, } },
    { L2FE_VE_LAN_VL_IDX_ACCESS, 4096, 1, "L2FE_VE_LAN_VL_IDX", 2, { {0, 1, "lan_vl_idx_entry_vld"}, {1, 7, "lan_vl_idx_fib_idx"}, } },
    { L2FE_VE_LAN_VL_FIB_ACCESS, 128, 4, "L2FE_VE_LAN_VL_FIB", 25, { {0, 16, "lan_vl_fib_egress_top_untag_bm"}, {16, 16, "lan_vl_fib_membership_bm"}, {32, 10, "lan_vl_fib_vl_group_id"}, {42, 1, "lan_vl_fib_da_sa_swap"}, {43, 1, "lan_vl_fib_permit"}, {44, 1, "lan_vl_fib_markdown_en"}, {45, 1, "lan_vl_fib_flowid_valid"}, {46, 12, "lan_vl_fib_flowid"}, {58, 1, "lan_vl_fib_cos_valid"}, {59, 1, "lan_vl_fib_dscp_valid"}, {60, 1, "lan_vl_fib_802_1p_valid"}, {61, 3, "lan_vl_fib_cos"}, {64, 6, "lan_vl_fib_dscp"}, {70, 3, "lan_vl_fib_802_1p"}, {73, 1, "lan_vl_fib_ldpid_vld"}, {74, 6, "lan_vl_fib_ldpid"}, {80, 1, "lan_vl_fib_mc_vlan_ind"}, {81, 2, "lan_vl_fib_stp_mode"}, {83, 1, "lan_vl_fib_inner_cmd_valid"}, {84, 2, "lan_vl_fib_inner_vlan_cmd"}, {86, 1, "lan_vl_fib_top_cmd_valid"}, {87, 2, "lan_vl_fib_top_vlan_cmd"}, {89, 1, "lan_vl_fib_sc_ind"}, {90, 12, "lan_vl_fib_vid"}, {102, 4, "lan_vl_fib_reserved"}, } },
//{ L2FE_CLE_IGR_KEY_ACCESS, 128, 13, "L2FE_CLE_IGR_KEY", 142, { {0, 3, "cl_key_tre_cmrcmr_2_prcd"}, {3, 1, "cl_key_tre_cmrcmr_2_fields_operator"}, {4, 12, "cl_key_tre_cmrcmr_2_mask"}, {6, 3, "cl_key_tre_cmrl2r_2_l4_ctrl"}, {9, 2, "cl_key_tre_cmrl2r_2_dot1p_ctrl"}, {11, 2, "cl_key_tre_cmrl2r_2_vid_ctrl"}, {12, 3, "cl_key_tre_cmrl3r_2_l4_ctrl"}, {13, 2, "cl_key_tre_cmrl2r_2_mac_ctrl"}, {15, 2, "cl_key_tre_cmrl3r_2_dot1p_ctrl"}, {16, 3, "cl_key_tre_cmrcmr_2_l4_ctrl"}, {17, 2, "cl_key_tre_cmrl3r_2_vid_ctrl"}, {19, 1, "cl_key_tre_cmrcmr_2_ipv4_addr_sel"}, {20, 1, "cl_key_tre_cmrcmr_2_mac_addr_sel"}, {21, 16, "cl_key_tre_cmrcmr_2_l4_sp"}, {26, 2, "cl_key_orhr_ipv4e_l7_ctrl"}, {28, 2, "cl_key_orhr_ipv4e_l4_sp_ctrl"}, {30, 2, "cl_key_orhr_ipv4e_l4_dp_ctrl"}, {31, 16, "cl_key_tre_cmrl2r_2_l4_dp"}, {32, 3, "cl_key_orhr_ipv4e_ip_ctrl"}, {35, 2, "cl_key_orhr_ipv4e_dot1p_ctrl"}, {37, 16, "cl_key_tre_cmrcmr_2_l4_dp"}, {39, 2, "cl_key_orhr_ipv4e_mac_ctrl"}, {41, 32, "cl_key_orhr_ipv4e_global_key"}, {47, 5, "cl_key_tre_cmrl2r_2_ev2_pt"}, {52, 2, "cl_key_tre_cmrl2r_2_tag_num"}, {53, 5, "cl_key_tre_cmrcmr_2_ev2_pt"}, {54, 3, "cl_key_tre_cmrl2r_2_inner_802_1p"}, {57, 3, "cl_key_tre_cmrl2r_2_top_802_1p"}, {58, 2, "cl_key_tre_cmrcmr_2_tag_num"}, {60, 3, "cl_key_tre_cmrcmr_2_top_802_1p"}, {61, 12, "cl_key_tre_cmrl2r_2_inner_vid"}, {63, 1, "cl_key_tre_cmrcmr_2_top_sc_ind"}, {64, 12, "cl_key_tre_cmrcmr_2_top_vid"}, {66, 1, "cl_key_tre_cmrl3r_2_inner_sc_ind"}, {67, 12, "cl_key_tre_cmrl3r_2_inner_vid"}, {69, 2, "cl_key_orhr_ipv6e_tag_num"}, {71, 3, "cl_key_orhr_ipv6e_top_802_1p"}, {73, 1, "cl_key_tre_cmrl2r_2_top_sc_ind"}, {74, 12, "cl_key_tre_cmrl2r_2_top_vid"}, {75, 12, "cl_key_orhr_ipv6e_top_vid"}, {76, 5, "cl_key_tre_cmrcmr_2_special_pkt_code"}, {78, 3, "cl_key_orhr_ipv4e_l4_flags"}, {79, 1, "cl_key_tre_cmrl3r_2_top_sc_ind"}, {80, 12, "cl_key_tre_cmrl3r_2_top_vid"}, {81, 3, "cl_key_tre_cmrcmr_2_l4_flags"}, {84, 1, "cl_key_tre_cmrcmr_2_ipv4_fragment"}, {85, 1, "cl_key_tre_cmrcmr_2_ipv4_option"}, {86, 6, "cl_key_tre_cmrcmr_2_ip_tos"}, {87, 5, "cl_key_orhr_ipv6e_special_pkt_code"}, {92, 8, "cl_key_tre_cmrcmr_2_ip_pt"}, {95, 6, "cl_key_orhr_ipv6e_ip_tos"}, {97, 3, "cl_key_tre_cmrl3r_2_l4_flags"}, {100, 32, "cl_key_tre_cmrcmr_2_ipv4_addr"}, {101, 1, "cl_key_tre_cmrl3r_2_ipv4_option"}, {102, 6, "cl_key_tre_cmrl3r_2_ip_tos"}, {108, 8, "cl_key_tre_cmrl3r_2_ip_pt"}, {109, 14, "cl_key_orhr_ipv6e_ipv6_mask"}, {113, 16, "cl_key_orhr_ipv4e_l4_dp_low"}, {116, 32, "cl_key_tre_cmrl3r_2_ipv4_sa"}, {123, 128, "cl_key_orhr_ipv6e_ipv6_sa"}, {129, 16, "cl_key_orhr_ipv4e_l4_dp_high"}, {132, 48, "cl_key_tre_cmrcmr_2_mac_addr"}, {134, 48, "cl_key_tre_cmrl2r_2_mac_da"}, {145, 1, "cl_key_orhr_ipv4e_ipv4_fragment"}, {146, 1, "cl_key_orhr_ipv4e_ipv4_option"}, {147, 6, "cl_key_orhr_ipv4e_ip_tos"}, {148, 32, "cl_key_tre_cmrl3r_2_ipv4_da"}, {153, 8, "cl_key_orhr_ipv4e_ip_pt"}, {161, 32, "cl_key_orhr_ipv4e_ipv4_sa"}, {180, 1, "cl_key_tre_cmrcmr_2_ecn_mask"}, {181, 2, "cl_key_tre_cmrcmr_2_ecn"}, {182, 7, "cl_key_tre_cmrl2r_2_reserved"}, {183, 6, "cl_key_tre_cmrcmr_2_reserved"}, {189, 3, "cl_key_tre_cmrcmr_1_prcd"}, {192, 1, "cl_key_tre_cmrcmr_1_fields_operator"}, {193, 12, "cl_key_tre_cmrcmr_1_mask"}, {195, 3, "cl_key_tre_l2rcmr_1_l4_ctrl"}, {198, 2, "cl_key_tre_l2rcmr_1_dot1p_ctrl"}, {200, 2, "cl_key_tre_l2rcmr_1_vid_ctrl"}, {201, 3, "cl_key_tre_l3rcmr_1_l4_ctrl"}, {202, 2, "cl_key_tre_l2rcmr_1_mac_ctrl"}, {204, 2, "cl_key_tre_l3rcmr_1_dot1p_ctrl"}, {205, 3, "cl_key_tre_cmrcmr_1_l4_ctrl"}, {206, 2, "cl_key_tre_l3rcmr_1_vid_ctrl"}, {208, 1, "cl_key_tre_cmrcmr_1_ipv4_addr_sel"}, {209, 1, "cl_key_tre_cmrcmr_1_mac_addr_sel"}, {210, 16, "cl_key_tre_cmrcmr_1_l4_sp"}, {220, 16, "cl_key_tre_l2rcmr_1_l4_dp"}, {225, 5, "cl_key_orhr_ipv4e_ev2_pt"}, {226, 16, "cl_key_tre_cmrcmr_1_l4_dp"}, {230, 2, "cl_key_orhr_ipv4e_tag_num"}, {232, 3, "cl_key_orhr_ipv4e_inner_802_1p"}, {235, 3, "cl_key_orhr_ipv4e_top_802_1p"}, {236, 5, "cl_key_tre_l2rcmr_1_ev2_pt"}, {238, 1, "cl_key_orhr_ipv4e_inner_sc_ind"}, {239, 12, "cl_key_orhr_ipv4e_inner_vid"}, {241, 2, "cl_key_tre_l2rcmr_1_tag_num"}, {242, 5, "cl_key_tre_cmrcmr_1_ev2_pt"}, {243, 3, "cl_key_tre_l2rcmr_1_inner_802_1p"}, {246, 3, "cl_key_tre_l2rcmr_1_top_802_1p"}, {247, 2, "cl_key_tre_cmrcmr_1_tag_num"}, {249, 3, "cl_key_tre_cmrcmr_1_top_802_1p"}, {250, 12, "cl_key_tre_l2rcmr_1_inner_vid"}, {251, 128, "cl_key_orhr_ipv6e_ipv6_da"}, {252, 1, "cl_key_tre_cmrcmr_1_top_sc_ind"}, {253, 12, "cl_key_tre_cmrcmr_1_top_vid"}, {255, 1, "cl_key_tre_l3rcmr_1_inner_sc_ind"}, {256, 12, "cl_key_tre_l3rcmr_1_inner_vid"}, {262, 1, "cl_key_tre_l2rcmr_1_top_sc_ind"}, {263, 12, "cl_key_tre_l2rcmr_1_top_vid"}, {264, 48, "cl_key_orhr_ipv4e_mac_sa"}, {265, 5, "cl_key_tre_cmrcmr_1_special_pkt_code"}, {268, 1, "cl_key_tre_l3rcmr_1_top_sc_ind"}, {269, 12, "cl_key_tre_l3rcmr_1_top_vid"}, {270, 3, "cl_key_tre_cmrcmr_1_l4_flags"}, {273, 1, "cl_key_tre_cmrcmr_1_ipv4_fragment"}, {274, 1, "cl_key_tre_cmrcmr_1_ipv4_option"}, {275, 6, "cl_key_tre_cmrcmr_1_ip_tos"}, {281, 8, "cl_key_tre_cmrcmr_1_ip_pt"}, {286, 3, "cl_key_tre_l3rcmr_1_l4_flags"}, {289, 32, "cl_key_tre_cmrcmr_1_ipv4_addr"}, {290, 1, "cl_key_tre_l3rcmr_1_ipv4_option"}, {291, 6, "cl_key_tre_l3rcmr_1_ip_tos"}, {297, 8, "cl_key_tre_l3rcmr_1_ip_pt"}, {305, 32, "cl_key_tre_l3rcmr_1_ipv4_sa"}, {312, 48, "cl_key_orhr_ipv4e_mac_da"}, {321, 48, "cl_key_tre_cmrcmr_1_mac_addr"}, {323, 48, "cl_key_tre_l2rcmr_1_mac_da"}, {337, 32, "cl_key_tre_l3rcmr_1_ipv4_da"}, {360, 1, "cl_key_orhr_ipv4e_ipv4_ecn_mask"}, {361, 2, "cl_key_orhr_ipv4e_ipv4_ecn"}, {363, 19, "cl_key_orhr_ipv4e_reserved"}, {369, 1, "cl_key_tre_cmrcmr_1_ecn_mask"}, {370, 2, "cl_key_tre_cmrcmr_1_ecn"}, {371, 7, "cl_key_tre_l2rcmr_1_reserved"}, {372, 6, "cl_key_tre_cmrcmr_1_reserved"}, {378, 2, "cl_key_tre_rule_type_1"}, {379, 1, "cl_key_orhr_ipv6e_ipv6_ecn_mask"}, {380, 2, "cl_key_tre_rule_type_0"}, {382, 1, "cl_key_tre_rule_hinge"}, {383, 2, "cl_key_entry_type"}, {385, 1, "cl_key_valid"}, } },
//{ L2FE_CLE_EGR_KEY_ACCESS, 128, 13, "L2FE_CLE_EGR_KEY", 142, { {0, 3, "cl_key_tre_cmrcmr_2_prcd"}, {3, 1, "cl_key_tre_cmrcmr_2_fields_operator"}, {4, 12, "cl_key_tre_cmrcmr_2_mask"}, {6, 3, "cl_key_tre_cmrl2r_2_l4_ctrl"}, {9, 2, "cl_key_tre_cmrl2r_2_dot1p_ctrl"}, {11, 2, "cl_key_tre_cmrl2r_2_vid_ctrl"}, {12, 3, "cl_key_tre_cmrl3r_2_l4_ctrl"}, {13, 2, "cl_key_tre_cmrl2r_2_mac_ctrl"}, {15, 2, "cl_key_tre_cmrl3r_2_dot1p_ctrl"}, {16, 3, "cl_key_tre_cmrcmr_2_l4_ctrl"}, {17, 2, "cl_key_tre_cmrl3r_2_vid_ctrl"}, {19, 1, "cl_key_tre_cmrcmr_2_ipv4_addr_sel"}, {20, 1, "cl_key_tre_cmrcmr_2_mac_addr_sel"}, {21, 16, "cl_key_tre_cmrcmr_2_l4_sp"}, {26, 2, "cl_key_orhr_ipv4e_l7_ctrl"}, {28, 2, "cl_key_orhr_ipv4e_l4_sp_ctrl"}, {30, 2, "cl_key_orhr_ipv4e_l4_dp_ctrl"}, {31, 16, "cl_key_tre_cmrl2r_2_l4_dp"}, {32, 3, "cl_key_orhr_ipv4e_ip_ctrl"}, {35, 2, "cl_key_orhr_ipv4e_dot1p_ctrl"}, {37, 16, "cl_key_tre_cmrcmr_2_l4_dp"}, {39, 2, "cl_key_orhr_ipv4e_mac_ctrl"}, {41, 32, "cl_key_orhr_ipv4e_global_key"}, {47, 5, "cl_key_tre_cmrl2r_2_ev2_pt"}, {52, 2, "cl_key_tre_cmrl2r_2_tag_num"}, {53, 5, "cl_key_tre_cmrcmr_2_ev2_pt"}, {54, 3, "cl_key_tre_cmrl2r_2_inner_802_1p"}, {57, 3, "cl_key_tre_cmrl2r_2_top_802_1p"}, {58, 2, "cl_key_tre_cmrcmr_2_tag_num"}, {60, 3, "cl_key_tre_cmrcmr_2_top_802_1p"}, {61, 12, "cl_key_tre_cmrl2r_2_inner_vid"}, {63, 1, "cl_key_tre_cmrcmr_2_top_sc_ind"}, {64, 12, "cl_key_tre_cmrcmr_2_top_vid"}, {66, 1, "cl_key_tre_cmrl3r_2_inner_sc_ind"}, {67, 12, "cl_key_tre_cmrl3r_2_inner_vid"}, {69, 2, "cl_key_orhr_ipv6e_tag_num"}, {71, 3, "cl_key_orhr_ipv6e_top_802_1p"}, {73, 1, "cl_key_tre_cmrl2r_2_top_sc_ind"}, {74, 12, "cl_key_tre_cmrl2r_2_top_vid"}, {75, 12, "cl_key_orhr_ipv6e_top_vid"}, {76, 5, "cl_key_tre_cmrcmr_2_special_pkt_code"}, {78, 3, "cl_key_orhr_ipv4e_l4_flags"}, {79, 1, "cl_key_tre_cmrl3r_2_top_sc_ind"}, {80, 12, "cl_key_tre_cmrl3r_2_top_vid"}, {81, 3, "cl_key_tre_cmrcmr_2_l4_flags"}, {84, 1, "cl_key_tre_cmrcmr_2_ipv4_fragment"}, {85, 1, "cl_key_tre_cmrcmr_2_ipv4_option"}, {86, 6, "cl_key_tre_cmrcmr_2_ip_tos"}, {87, 5, "cl_key_orhr_ipv6e_special_pkt_code"}, {92, 8, "cl_key_tre_cmrcmr_2_ip_pt"}, {95, 6, "cl_key_orhr_ipv6e_ip_tos"}, {97, 3, "cl_key_tre_cmrl3r_2_l4_flags"}, {100, 32, "cl_key_tre_cmrcmr_2_ipv4_addr"}, {101, 1, "cl_key_tre_cmrl3r_2_ipv4_option"}, {102, 6, "cl_key_tre_cmrl3r_2_ip_tos"}, {108, 8, "cl_key_tre_cmrl3r_2_ip_pt"}, {109, 14, "cl_key_orhr_ipv6e_ipv6_mask"}, {113, 16, "cl_key_orhr_ipv4e_l4_dp_low"}, {116, 32, "cl_key_tre_cmrl3r_2_ipv4_sa"}, {123, 128, "cl_key_orhr_ipv6e_ipv6_sa"}, {129, 16, "cl_key_orhr_ipv4e_l4_dp_high"}, {132, 48, "cl_key_tre_cmrcmr_2_mac_addr"}, {134, 48, "cl_key_tre_cmrl2r_2_mac_da"}, {145, 1, "cl_key_orhr_ipv4e_ipv4_fragment"}, {146, 1, "cl_key_orhr_ipv4e_ipv4_option"}, {147, 6, "cl_key_orhr_ipv4e_ip_tos"}, {148, 32, "cl_key_tre_cmrl3r_2_ipv4_da"}, {153, 8, "cl_key_orhr_ipv4e_ip_pt"}, {161, 32, "cl_key_orhr_ipv4e_ipv4_sa"}, {180, 1, "cl_key_tre_cmrcmr_2_ecn_mask"}, {181, 2, "cl_key_tre_cmrcmr_2_ecn"}, {182, 7, "cl_key_tre_cmrl2r_2_reserved"}, {183, 6, "cl_key_tre_cmrcmr_2_reserved"}, {189, 3, "cl_key_tre_cmrcmr_1_prcd"}, {192, 1, "cl_key_tre_cmrcmr_1_fields_operator"}, {193, 12, "cl_key_tre_cmrcmr_1_mask"}, {195, 3, "cl_key_tre_l2rcmr_1_l4_ctrl"}, {198, 2, "cl_key_tre_l2rcmr_1_dot1p_ctrl"}, {200, 2, "cl_key_tre_l2rcmr_1_vid_ctrl"}, {201, 3, "cl_key_tre_l3rcmr_1_l4_ctrl"}, {202, 2, "cl_key_tre_l2rcmr_1_mac_ctrl"}, {204, 2, "cl_key_tre_l3rcmr_1_dot1p_ctrl"}, {205, 3, "cl_key_tre_cmrcmr_1_l4_ctrl"}, {206, 2, "cl_key_tre_l3rcmr_1_vid_ctrl"}, {208, 1, "cl_key_tre_cmrcmr_1_ipv4_addr_sel"}, {209, 1, "cl_key_tre_cmrcmr_1_mac_addr_sel"}, {210, 16, "cl_key_tre_cmrcmr_1_l4_sp"}, {220, 16, "cl_key_tre_l2rcmr_1_l4_dp"}, {225, 5, "cl_key_orhr_ipv4e_ev2_pt"}, {226, 16, "cl_key_tre_cmrcmr_1_l4_dp"}, {230, 2, "cl_key_orhr_ipv4e_tag_num"}, {232, 3, "cl_key_orhr_ipv4e_inner_802_1p"}, {235, 3, "cl_key_orhr_ipv4e_top_802_1p"}, {236, 5, "cl_key_tre_l2rcmr_1_ev2_pt"}, {238, 1, "cl_key_orhr_ipv4e_inner_sc_ind"}, {239, 12, "cl_key_orhr_ipv4e_inner_vid"}, {241, 2, "cl_key_tre_l2rcmr_1_tag_num"}, {242, 5, "cl_key_tre_cmrcmr_1_ev2_pt"}, {243, 3, "cl_key_tre_l2rcmr_1_inner_802_1p"}, {246, 3, "cl_key_tre_l2rcmr_1_top_802_1p"}, {247, 2, "cl_key_tre_cmrcmr_1_tag_num"}, {249, 3, "cl_key_tre_cmrcmr_1_top_802_1p"}, {250, 12, "cl_key_tre_l2rcmr_1_inner_vid"}, {251, 128, "cl_key_orhr_ipv6e_ipv6_da"}, {252, 1, "cl_key_tre_cmrcmr_1_top_sc_ind"}, {253, 12, "cl_key_tre_cmrcmr_1_top_vid"}, {255, 1, "cl_key_tre_l3rcmr_1_inner_sc_ind"}, {256, 12, "cl_key_tre_l3rcmr_1_inner_vid"}, {262, 1, "cl_key_tre_l2rcmr_1_top_sc_ind"}, {263, 12, "cl_key_tre_l2rcmr_1_top_vid"}, {264, 48, "cl_key_orhr_ipv4e_mac_sa"}, {265, 5, "cl_key_tre_cmrcmr_1_special_pkt_code"}, {268, 1, "cl_key_tre_l3rcmr_1_top_sc_ind"}, {269, 12, "cl_key_tre_l3rcmr_1_top_vid"}, {270, 3, "cl_key_tre_cmrcmr_1_l4_flags"}, {273, 1, "cl_key_tre_cmrcmr_1_ipv4_fragment"}, {274, 1, "cl_key_tre_cmrcmr_1_ipv4_option"}, {275, 6, "cl_key_tre_cmrcmr_1_ip_tos"}, {281, 8, "cl_key_tre_cmrcmr_1_ip_pt"}, {286, 3, "cl_key_tre_l3rcmr_1_l4_flags"}, {289, 32, "cl_key_tre_cmrcmr_1_ipv4_addr"}, {290, 1, "cl_key_tre_l3rcmr_1_ipv4_option"}, {291, 6, "cl_key_tre_l3rcmr_1_ip_tos"}, {297, 8, "cl_key_tre_l3rcmr_1_ip_pt"}, {305, 32, "cl_key_tre_l3rcmr_1_ipv4_sa"}, {312, 48, "cl_key_orhr_ipv4e_mac_da"}, {321, 48, "cl_key_tre_cmrcmr_1_mac_addr"}, {323, 48, "cl_key_tre_l2rcmr_1_mac_da"}, {337, 32, "cl_key_tre_l3rcmr_1_ipv4_da"}, {360, 1, "cl_key_orhr_ipv4e_ipv4_ecn_mask"}, {361, 2, "cl_key_orhr_ipv4e_ipv4_ecn"}, {363, 19, "cl_key_orhr_ipv4e_reserved"}, {369, 1, "cl_key_tre_cmrcmr_1_ecn_mask"}, {370, 2, "cl_key_tre_cmrcmr_1_ecn"}, {371, 7, "cl_key_tre_l2rcmr_1_reserved"}, {372, 6, "cl_key_tre_cmrcmr_1_reserved"}, {378, 2, "cl_key_tre_rule_type_1"}, {379, 1, "cl_key_orhr_ipv6e_ipv6_ecn_mask"}, {380, 2, "cl_key_tre_rule_type_0"}, {382, 1, "cl_key_tre_rule_hinge"}, {383, 2, "cl_key_entry_type"}, {385, 1, "cl_key_valid"}, } },
//{ L2FE_CLE_IGR_FIB_ACCESS, 128, 5, "L2FE_CLE_IGR_FIB", 48, { {0, 2, "cl_fib_xr_stp_mode_1"}, {2, 1, "cl_fib_xr_permit_valid_1"}, {3, 1, "cl_fib_xr_permit_1"}, {4, 1, "cl_fib_xr_premarked_1"}, {5, 1, "cl_fib_xr_dscp_markdown_1"}, {6, 1, "cl_fib_xr_mark_en_1"}, {7, 12, "cl_fib_xr_flowid_1"}, {19, 1, "cl_fib_xr_dp_valid_1"}, {20, 6, "cl_fib_xr_ldpid_1"}, {26, 1, "cl_fib_xr_cos_valid_1"}, {27, 1, "cl_fib_xr_dscp_valid_1"}, {28, 1, "cl_fib_xr_802_1p_valid_1"}, {29, 3, "cl_fib_xr_cos_1"}, {32, 6, "cl_fib_xr_dscp_1"}, {38, 3, "cl_fib_xr_802_1p_1"}, {41, 1, "cl_fib_xr_inner_cmd_valid_1"}, {42, 2, "cl_fib_xr_inner_vlan_cmd_1"}, {44, 1, "cl_fib_xr_inner_sc_ind_1"}, {45, 12, "cl_fib_xr_inner_vid_1"}, {57, 1, "cl_fib_xr_top_cmd_valid_1"}, {58, 2, "cl_fib_xr_top_vlan_cmd_1"}, {60, 1, "cl_fib_xr_top_sc_ind_1"}, {61, 12, "cl_fib_xr_top_vid_1"}, {73, 1, "cl_fib_xr_pkt_no_drop_1"}, {74, 2, "cl_fib_xr_stp_mode_0"}, {76, 1, "cl_fib_xr_permit_valid_0"}, {77, 1, "cl_fib_xr_permit_0"}, {78, 1, "cl_fib_xr_premarked_0"}, {79, 1, "cl_fib_xr_dscp_markdown_0"}, {80, 1, "cl_fib_xr_mark_en_0"}, {81, 12, "cl_fib_xr_flowid_0"}, {93, 1, "cl_fib_xr_dp_valid_0"}, {94, 6, "cl_fib_xr_ldpid_0"}, {100, 1, "cl_fib_xr_cos_valid_0"}, {101, 1, "cl_fib_xr_dscp_valid_0"}, {102, 1, "cl_fib_xr_802_1p_valid_0"}, {103, 3, "cl_fib_xr_cos_0"}, {106, 6, "cl_fib_xr_dscp_0"}, {112, 3, "cl_fib_xr_802_1p_0"}, {115, 1, "cl_fib_xr_inner_cmd_valid_0"}, {116, 2, "cl_fib_xr_inner_vlan_cmd_0"}, {118, 1, "cl_fib_xr_inner_sc_ind_0"}, {119, 12, "cl_fib_xr_inner_vid_0"}, {131, 1, "cl_fib_xr_top_cmd_valid_0"}, {132, 2, "cl_fib_xr_top_vlan_cmd_0"}, {134, 1, "cl_fib_xr_top_sc_ind_0"}, {135, 12, "cl_fib_xr_top_vid_0"}, {147, 1, "cl_fib_xr_pkt_no_drop_0"}, } },
//{ L2FE_CLE_EGR_FIB_ACCESS, 128, 5, "L2FE_CLE_EGR_FIB", 48, { {0, 2, "cl_fib_xr_stp_mode_1"}, {2, 1, "cl_fib_xr_permit_valid_1"}, {3, 1, "cl_fib_xr_permit_1"}, {4, 1, "cl_fib_xr_premarked_1"}, {5, 1, "cl_fib_xr_dscp_markdown_1"}, {6, 1, "cl_fib_xr_mark_en_1"}, {7, 12, "cl_fib_xr_flowid_1"}, {19, 1, "cl_fib_xr_dp_valid_1"}, {20, 6, "cl_fib_xr_ldpid_1"}, {26, 1, "cl_fib_xr_cos_valid_1"}, {27, 1, "cl_fib_xr_dscp_valid_1"}, {28, 1, "cl_fib_xr_802_1p_valid_1"}, {29, 3, "cl_fib_xr_cos_1"}, {32, 6, "cl_fib_xr_dscp_1"}, {38, 3, "cl_fib_xr_802_1p_1"}, {41, 1, "cl_fib_xr_inner_cmd_valid_1"}, {42, 2, "cl_fib_xr_inner_vlan_cmd_1"}, {44, 1, "cl_fib_xr_inner_sc_ind_1"}, {45, 12, "cl_fib_xr_inner_vid_1"}, {57, 1, "cl_fib_xr_top_cmd_valid_1"}, {58, 2, "cl_fib_xr_top_vlan_cmd_1"}, {60, 1, "cl_fib_xr_top_sc_ind_1"}, {61, 12, "cl_fib_xr_top_vid_1"}, {73, 1, "cl_fib_xr_pkt_no_drop_1"}, {74, 2, "cl_fib_xr_stp_mode_0"}, {76, 1, "cl_fib_xr_permit_valid_0"}, {77, 1, "cl_fib_xr_permit_0"}, {78, 1, "cl_fib_xr_premarked_0"}, {79, 1, "cl_fib_xr_dscp_markdown_0"}, {80, 1, "cl_fib_xr_mark_en_0"}, {81, 12, "cl_fib_xr_flowid_0"}, {93, 1, "cl_fib_xr_dp_valid_0"}, {94, 6, "cl_fib_xr_ldpid_0"}, {100, 1, "cl_fib_xr_cos_valid_0"}, {101, 1, "cl_fib_xr_dscp_valid_0"}, {102, 1, "cl_fib_xr_802_1p_valid_0"}, {103, 3, "cl_fib_xr_cos_0"}, {106, 6, "cl_fib_xr_dscp_0"}, {112, 3, "cl_fib_xr_802_1p_0"}, {115, 1, "cl_fib_xr_inner_cmd_valid_0"}, {116, 2, "cl_fib_xr_inner_vlan_cmd_0"}, {118, 1, "cl_fib_xr_inner_sc_ind_0"}, {119, 12, "cl_fib_xr_inner_vid_0"}, {131, 1, "cl_fib_xr_top_cmd_valid_0"}, {132, 2, "cl_fib_xr_top_vlan_cmd_0"}, {134, 1, "cl_fib_xr_top_sc_ind_0"}, {135, 12, "cl_fib_xr_top_vid_0"}, {147, 1, "cl_fib_xr_pkt_no_drop_0"}, } },
    { L2FE_L2E_FDB_ACCESS, 2097152, 4, "L2FE_L2E_FDB", 12, { {0, 10, "mcgid"}, {10, 1, "mc_flag"}, {11, 8, "aging_sts"}, {19, 1, "is_static"}, {20, 1, "da_permit"}, {21, 1, "sa_permit"}, {22, 1, "lrned_scind"}, {23, 12, "lrned_vid"}, {35, 3, "key_dot1p"}, {38, 1, "key_scind"}, {39, 12, "key_vid"}, {51, 48, "mac_addr"}, } },
    { L2FE_L2E_PORT_MAC_CNT_LMT_ACCESS, 1024, 1, "L2FE_L2E_PORT_MAC_CNT_LMT", 1, { {0, 13, "val"}, } },
    { L2FE_IM3E_MLDDA_ACCESS, 256, 6, "L2FE_IM3E_MLDDA", 9, { {0, 1, "entry_valid"}, {1, 10, "mc_group_id"}, {11, 1, "v3_ind"}, {12, 8, "connection_id"}, {20, 128, "ip_da"}, {148, 1, "ipv6_flg"}, {149, 12, "vid"}, {161, 1, "vlan_sc_ind"}, {162, 1, "wan_ind"}, } },
    { L2FE_IM3E_MLDSA_ACCESS, 64, 5, "L2FE_IM3E_MLDSA", 4, { {0, 1, "entry_valid"}, {1, 10, "mc_group_id"}, {11, 128, "ipv6_sa"}, {139, 8, "connection_id"}, } },
    { L2TM_L2TM_BM_PKT_MEM_ACCESS, 32768, 8, "L2TM_L2TM_BM_PKT_MEM", 0, NULL },
    { L2TM_L2TM_QM_PORT_PRVT_BUFCNT_MEM_ACCESS, 16, 1, "L2TM_L2TM_QM_PORT_PRVT_BUFCNT_MEM", 1, { {0, 15, "cnt"}, } },
    { L2TM_L2TM_QM_FREEQ_MEM_ACCESS, 32768, 2, "L2TM_L2TM_QM_FREEQ_MEM", 8, { {0, 7, "blen"}, {7, 1, "eqid"}, {8, 1, "eop"}, {9, 1, "sop"}, {10, 1, "cpu"}, {11, 1, "prvt"}, {12, 15, "nxt_ptr"}, {27, 7, "ecc"}, } },
    { L2TM_L2TM_QM_VOQINFO_MEM_ACCESS, 128, 2, "L2TM_L2TM_QM_VOQINFO_MEM", 10, { {0, 7, "blen"}, {7, 1, "eqid"}, {8, 1, "eop"}, {9, 1, "sop"}, {10, 1, "cpu"}, {11, 1, "prvt"}, {12, 15, "head_ptr"}, {27, 15, "tail_ptr"}, {42, 1, "valid"}, {43, 7, "ecc"}, } },
    { L2TM_L2TM_QM_VOQ_PCNT_MEM_ACCESS, 128, 1, "L2TM_L2TM_QM_VOQ_PCNT_MEM", 1, { {0, 14, "cnt"}, } },
    { L2TM_L2TE_POL_PKT_TYPE_PROFILE_MEM_ACCESS, 16, 3, "L2TM_L2TE_POL_PKT_TYPE_PROFILE_MEM", 6, { {0, 10, "committed_rate_k"}, {10, 16, "committed_rate_m"}, {26, 12, "committed_bs"}, {38, 10, "peak_rate_k"}, {48, 16, "peak_rate_m"}, {64, 12, "peak_bs"}, } },
    { L2TM_L2TE_POL_PKT_TYPE_COUNTER_MEM_ACCESS, 16, 2, "L2TM_L2TE_POL_PKT_TYPE_COUNTER_MEM", 2, { {0, 21, "committed_tbc"}, {21, 21, "peak_tbc"}, } },
    { L2TM_L2TE_POL_PORT_PROFILE_MEM_ACCESS, 16, 3, "L2TM_L2TE_POL_PORT_PROFILE_MEM", 6, { {0, 10, "committed_rate_k"}, {10, 16, "committed_rate_m"}, {26, 12, "committed_bs"}, {38, 10, "peak_rate_k"}, {48, 16, "peak_rate_m"}, {64, 12, "peak_bs"}, } },
    { L2TM_L2TE_POL_PORT_COUNTER_MEM_ACCESS, 16, 2, "L2TM_L2TE_POL_PORT_COUNTER_MEM", 2, { {0, 21, "committed_tbc"}, {21, 21, "peak_tbc"}, } },
    { L2TM_L2TE_POL_FLOW_PROFILE_MEM_ACCESS, 256, 3, "L2TM_L2TE_POL_FLOW_PROFILE_MEM", 6, { {0, 10, "committed_rate_k"}, {10, 16, "committed_rate_m"}, {26, 12, "committed_bs"}, {38, 10, "peak_rate_k"}, {48, 16, "peak_rate_m"}, {64, 12, "peak_bs"}, } },
    { L2TM_L2TE_POL_FLOW_COUNTER_MEM_ACCESS, 256, 2, "L2TM_L2TE_POL_FLOW_COUNTER_MEM", 2, { {0, 21, "committed_tbc"}, {21, 21, "peak_tbc"}, } },
    { L2TM_L2TE_POL_AGRFLOW_PROFILE_MEM_ACCESS, 8, 3, "L2TM_L2TE_POL_AGRFLOW_PROFILE_MEM", 6, { {0, 10, "committed_rate_k"}, {10, 16, "committed_rate_m"}, {26, 12, "committed_bs"}, {38, 10, "peak_rate_k"}, {48, 16, "peak_rate_m"}, {64, 12, "peak_bs"}, } },
    { L2TM_L2TE_POL_AGRFLOW_COUNTER_MEM_ACCESS, 8, 2, "L2TM_L2TE_POL_AGRFLOW_COUNTER_MEM", 2, { {0, 21, "committed_tbc"}, {21, 21, "peak_tbc"}, } },
    { L2TM_L2TE_SHP_PORT_TBC_MEM_ACCESS, 16, 2, "L2TM_L2TE_SHP_PORT_TBC_MEM", 7, { {0, 10, "rate_k"}, {10, 16, "rate_m"}, {26, 12, "bs"}, {38, 2, "enb"}, {40, 1, "mode"}, {41, 21, "tbc"}, {62, 1, "pkt_mode_class_sel"}, } },
    { L2TM_L2TE_SHP_GROUP_TBC_MEM_ACCESS, 16, 2, "L2TM_L2TE_SHP_GROUP_TBC_MEM", 7, { {0, 10, "rate_k"}, {10, 16, "rate_m"}, {26, 12, "bs"}, {38, 2, "enb"}, {40, 1, "mode"}, {41, 21, "tbc"}, {62, 1, "pkt_mode_class_sel"}, } },
    { L2TM_L2TE_SHP_VOQ_TBC_MEM_ACCESS, 128, 2, "L2TM_L2TE_SHP_VOQ_TBC_MEM", 7, { {0, 10, "rate_k"}, {10, 16, "rate_m"}, {26, 12, "bs"}, {38, 2, "enb"}, {40, 1, "mode"}, {41, 21, "tbc"}, {62, 1, "pkt_mode_class_sel"}, } },
    { L2TM_L2TE_WRED_AFGS_MEM_ACCESS, 2, 1, "L2TM_L2TE_WRED_AFGS_MEM", 1, { {0, 15, "depth"}, } },
    { L2TM_L2TE_WRED_PROFILE_MEM_ACCESS, 32, 8, "L2TM_L2TE_WRED_PROFILE_MEM", 34, { {0, 6, "unmark_dp0"}, {6, 6, "unmark_dp1"}, {12, 6, "unmark_dp2"}, {18, 6, "unmark_dp3"}, {24, 6, "unmark_dp4"}, {30, 6, "unmark_dp5"}, {36, 6, "unmark_dp6"}, {42, 6, "unmark_dp7"}, {48, 6, "unmark_dp8"}, {54, 6, "unmark_dp9"}, {60, 6, "unmark_dp11"}, {66, 6, "unmark_dp10"}, {72, 6, "unmark_dp12"}, {78, 6, "unmark_dp13"}, {84, 6, "unmark_dp14"}, {90, 6, "unmark_dp15"}, {96, 6, "mark_dp0"}, {102, 6, "mark_dp1"}, {108, 6, "mark_dp2"}, {114, 6, "mark_dp3"}, {120, 6, "mark_dp4"}, {126, 6, "mark_dp5"}, {132, 6, "mark_dp6"}, {138, 6, "mark_dp7"}, {144, 6, "mark_dp8"}, {150, 6, "mark_dp9"}, {156, 6, "mark_dp11"}, {162, 6, "mark_dp10"}, {168, 6, "mark_dp12"}, {174, 6, "mark_dp13"}, {180, 6, "mark_dp14"}, {186, 6, "mark_dp15"}, {192, 4, "unmark_idx"}, {196, 4, "mark_idx"}, } },
    { L2TM_L2TE_WRED_PROFILE_SELECT_MEM_ACCESS, 32, 2, "L2TM_L2TE_WRED_PROFILE_SELECT_MEM", 8, { {0, 5, "sel0"}, {5, 5, "sel1"}, {10, 5, "sel2"}, {15, 5, "sel3"}, {20, 5, "sel4"}, {25, 5, "sel5"}, {30, 5, "sel6"}, {35, 5, "sel7"}, } },
    { L2TM_L2TE_CB_VOQ_THRSH_PROFILE_MEM_ACCESS, 8, 2, "L2TM_L2TE_CB_VOQ_THRSH_PROFILE_MEM", 4, { {0, 15, "lth"}, {15, 15, "hth"}, {30, 15, "glth"}, {45, 15, "ghth"}, } },
    { L2TM_L2TE_CB_VOQ_THRSH_SELECT_MEM_ACCESS, 128, 1, "L2TM_L2TE_CB_VOQ_THRSH_SELECT_MEM", 1, { {0, 3, "sel"}, } },
    { L2TM_L2TE_CB_PORT_FREECNT_MEM_ACCESS, 16, 1, "L2TM_L2TE_CB_PORT_FREECNT_MEM", 4, { {0, 15, "cnt0"}, {15, 1, "cnt0_msb"}, {16, 15, "cnt1"}, {31, 1, "cnt1_msb"}, } },
    { L2TM_L2TE_CB_VOQ_BUFCNT_MEM_ACCESS, 128, 1, "L2TM_L2TE_CB_VOQ_BUFCNT_MEM", 2, { {0, 15, "cnt0"}, {16, 15, "cnt1"}, } },
    { L2TM_L2TE_CB_VOQ_MAX_BUFCNT_MEM_ACCESS, 128, 1, "L2TM_L2TE_CB_VOQ_MAX_BUFCNT_MEM", 1, { {0, 15, "cnt"}, } },
    { L2TM_L2TE_CB_SRC_PORT_BUFCNT_MEM_ACCESS, 16, 1, "L2TM_L2TE_CB_SRC_PORT_BUFCNT_MEM", 1, { {0, 15, "cnt"}, } },
    { L2TM_L2TE_CB_SRC_PRI_BUFCNT_MEM_ACCESS, 16, 1, "L2TM_L2TE_CB_SRC_PRI_BUFCNT_MEM", 1, { {0, 15, "cnt0"}, } },
    { L2TM_L2TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM_ACCESS, 8, 1, "L2TM_L2TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM", 2, { {0, 15, "lth"}, {15, 15, "hth"}, } },
    { L2TM_L2TE_CB_DQSCH_VOQ_THRSH_SELECT_MEM_ACCESS, 128, 1, "L2TM_L2TE_CB_DQSCH_VOQ_THRSH_SELECT_MEM", 1, { {0, 3, "sel"}, } },
    { L2TM_L2TE_PM_CNT_MEM_ACCESS, 1024, 9, "L2TM_L2TE_PM_CNT_MEM", 8, { {0, 32, "cnt0"}, {32, 32, "cnt1"}, {64, 32, "cnt2"}, {96, 32, "cnt3"}, {128, 36, "cnt4"}, {164, 36, "cnt5"}, {200, 36, "cnt6"}, {236, 36, "cnt7"}, } },
    { L3FE_GLB_EGRESS_L3_IF_TBL_ACCESS, 256, 3, "L3FE_GLB_EGRESS_L3_IF_TBL", 31, { {0, 1, "MTU_VLD"}, {1, 2, "MTU_ENC"}, {3, 3, "INNER_802_1P_SEL"}, {6, 3, "INNER_802_1P"}, {9, 3, "TOP_802_1P_SEL"}, {12, 3, "TOP_802_1P"}, {15, 1, "QOS_802_1P_TABLE_SEL"}, {16, 1, "VLAN_VLD"}, {17, 2, "INNER_DEI_SEL"}, {19, 1, "INNER_DEI"}, {20, 12, "INNER_VID"}, {32, 3, "INNER_TPID_ENC"}, {35, 2, "TOP_DEI_SEL"}, {37, 1, "TOP_DEI"}, {38, 12, "TOP_VID"}, {50, 3, "TOP_TPID_ENC"}, {53, 2, "VLAN_CNT"}, {55, 1, "PPPOE_SET"}, {56, 1, "PPPOE_VLD"}, {57, 16, "PPPOE_SESSION_ID"}, {73, 1, "MAC_DA_VLD"}, {74, 4, "MAC_DA_AN_SEL"}, {78, 1, "MAC_SA_VLD"}, {79, 4, "MAC_SA_AN_SEL"}, {83, 1, "L2_FORMAT_VLD"}, {84, 1, "L2_FORMAT"}, {85, 1, "LDPID_VLD"}, {86, 6, "LDPID"}, {92, 1, "DEEP_Q"}, {93, 1, "STAGE3_CTRL_VLD"}, {94, 1, "STAGE3_CTRL"}, } },
    { L3FE_GLB_DSCP_MAP_TBL_ACCESS, 128, 2, "L3FE_GLB_DSCP_MAP_TBL", 10, { {0, 3, "dscp_2_pf0_dot1p"}, {3, 6, "dscp_2_pf0_dscp"}, {9, 4, "dscp_2_pf0_cos"}, {13, 9, "dscp_2_pf0_pol_id"}, {22, 3, "dscp_2_pf0_pol_grp_id"}, {25, 3, "dscp_2_pf1_dot1p"}, {28, 6, "dscp_2_pf1_dscp"}, {34, 4, "dscp_2_pf1_cos"}, {38, 9, "dscp_2_pf1_pol_id"}, {47, 3, "dscp_2_pf1_pol_grp_id"}, } },
    { L3FE_PP_FIELD_CAM_ACCESS, 1048576, 5, "L3FE_PP_FIELD_CAM", 0, NULL },
    { L3FE_CLS_KEY_TBL_ACCESS, 128, 11, "L3FE_CLS_KEY_TBL", 6, { {0, 332, "cl_entry"}, {332, 1, "cl_entry_mode"}, {333, 4, "cl_pri"}, {337, 1, "cl_rslt_type"}, {338, 3, "cl_key_type"}, {341, 4, "cl_key_valid"}, } },
    { L3FE_CLS_FIB_TBL_ACCESS, 1024, 7, "L3FE_CLS_FIB_TBL", 1, { {0, 206, "cl_entry"}, } },
    { L3FE_STG0_LPB_IDX_TBL_ACCESS, 64, 1, "L3FE_STG0_LPB_IDX_TBL", 1, { {0, 2, "idx"}, } },
    { L3FE_STG0_SPB_TBL_ACCESS, 128, 1, "L3FE_STG0_SPB_TBL", 1, { {0, 28, "vct"}, } },
    { L3FE_PE_SIXRD_TBL_ACCESS, 16, 7, "L3FE_PE_SIXRD_TBL", 15, { {0, 1, "six_rd_valid"}, {1, 1, "six_rd_ingr_chk_en"}, {2, 1, "six_rd_br_only"}, {3, 1, "six_rd_ipsa_mtch"}, {4, 16, "six_rd_id_end"}, {20, 16, "six_rd_id_start"}, {36, 1, "six_rd_ttl_en"}, {37, 8, "six_rd_ttl"}, {45, 1, "six_rd_tos_en"}, {46, 8, "six_rd_tos"}, {54, 32, "six_rd_br_v4_ip"}, {86, 64, "six_rd_v6_prfx"}, {150, 6, "six_rd_v6_prfx_len"}, {156, 5, "six_rd_v4_msk_len"}, {161, 32, "six_rd_ce_v4_ip"}, } },
    { L3FE_PE_DSL_TBL_ACCESS, 16, 10, "L3FE_PE_DSL_TBL", 9, { {0, 1, "dsl_valid"}, {1, 1, "dsl_mc"}, {2, 20, "dsl_flwlabel"}, {22, 1, "dsl_hoplimit_en"}, {23, 8, "dsl_hoplimit"}, {31, 1, "dsl_tc_en"}, {32, 8, "dsl_tc"}, {40, 128, "dsl_aftr_ip"}, {168, 128, "dsl_b4_ip"}, } },
    { L3FE_PE_DROP_SRC_PKT_CNT_TBL_ACCESS, 64, 2, "L3FE_PE_DROP_SRC_PKT_CNT_TBL", 0, NULL },
    { L3FE_PE_DROP_SRC_BYTE_CNT_TBL_ACCESS, 64, 2, "L3FE_PE_DROP_SRC_BYTE_CNT_TBL", 0, NULL },
    { L3FE_PE_RX_IFID_CNT_TBL_ACCESS, 128, 8, "L3FE_PE_RX_IFID_CNT_TBL", 0, NULL },
    { L3FE_PE_TX_IFID_CNT_TBL_ACCESS, 128, 7, "L3FE_PE_TX_IFID_CNT_TBL", 0, NULL },
    { L3FE_FE_L3E_HS_LIGHT_IND_X_ACCESS, 32768, 6, "L3FE_FE_L3E_HS_LIGHT_IND_X", 0, NULL },
    { NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS, 64, 1, "NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM", 3, { {0, 16, "LLID_value"}, {16, 1, "valid"}, {17, 6, "SA_ID"}, } },
    { NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS, 512, 11, "NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM", 10, { {0, 32, "InPktsUnchecked_OutPktsProtected"}, {32, 32, "InPktsDelayed_OutPktsEncrypted"}, {64, 43, "InOctetsValidated_OutOctetsProtected"}, {107, 43, "InOctetsDecrypted_OutOctetsEncrypted"}, {150, 32, "InPktsLate"}, {182, 32, "InPktsOK"}, {214, 32, "InPktsInvalid"}, {246, 32, "InPktsNotValid"}, {278, 32, "InPktsNotUsingSA"}, {310, 32, "InPktsUnusedSA"}, } },
    { NI_HV_MCE_INDX_LKUP_ACCESS, 256, 2, "NI_HV_MCE_INDX_LKUP", 1, { {0, 64, "mc_vec"}, } },
    { NI_HV_XGE_MAC_STATS_ACCESS, 64, 2, "NI_HV_XGE_MAC_STATS", 0, NULL },
    { TE_POL_PKT_TYPE_PROFILE_MEM_ACCESS, 8, 3, "TE_POL_PKT_TYPE_PROFILE_MEM", 0, NULL },
    { TE_POL_PKT_TYPE_COUNTER_MEM_ACCESS, 8, 2, "TE_POL_PKT_TYPE_COUNTER_MEM", 0, NULL },
    { TE_POL_PORT_PROFILE_MEM_ACCESS, 8, 3, "TE_POL_PORT_PROFILE_MEM", 0, NULL },
    { TE_POL_PORT_COUNTER_MEM_ACCESS, 8, 2, "TE_POL_PORT_COUNTER_MEM", 0, NULL },
    { TE_POL_FLOW_PROFILE_MEM_ACCESS, 128, 3, "TE_POL_FLOW_PROFILE_MEM", 0, NULL },
    { TE_POL_FLOW_COUNTER_MEM_ACCESS, 128, 2, "TE_POL_FLOW_COUNTER_MEM", 0, NULL },
    { TE_POL_AGRFLOW_PROFILE_MEM_ACCESS, 8, 3, "TE_POL_AGRFLOW_PROFILE_MEM", 0, NULL },
    { TE_POL_AGRFLOW_COUNTER_MEM_ACCESS, 8, 2, "TE_POL_AGRFLOW_COUNTER_MEM", 0, NULL },
    { TE_SHP_PORT_TBC_MEM_ACCESS, 16, 2, "TE_SHP_PORT_TBC_MEM", 0, NULL },
    { TE_SHP_GROUP_TBC_MEM_ACCESS, 16, 2, "TE_SHP_GROUP_TBC_MEM", 0, NULL },
    { TE_SHP_VOQ_TBC_MEM_ACCESS, 128, 2, "TE_SHP_VOQ_TBC_MEM", 0, NULL },
    { TE_WRED_AFGS_MEM_ACCESS, 8, 1, "TE_WRED_AFGS_MEM", 0, NULL },
    { TE_WRED_PROFILE_MEM_ACCESS, 32, 8, "TE_WRED_PROFILE_MEM", 0, NULL },
    { TE_WRED_PROFILE_SELECT_MEM_ACCESS, 128, 2, "TE_WRED_PROFILE_SELECT_MEM", 0, NULL },
    { TE_CB_PORT_FREECNT_MEM_ACCESS, 16, 1, "TE_CB_PORT_FREECNT_MEM", 0, NULL },
    { TE_CB_VOQ_THRSH_PROFILE_MEM_ACCESS, 8, 3, "TE_CB_VOQ_THRSH_PROFILE_MEM", 0, NULL },
    { TE_CB_VOQ_THRSH_SELECT_MEM_ACCESS, 128, 1, "TE_CB_VOQ_THRSH_SELECT_MEM", 0, NULL },
    { TE_CB_VOQ_BUFCNT_MEM_ACCESS, 128, 1, "TE_CB_VOQ_BUFCNT_MEM", 0, NULL },
    { TE_CB_VOQ_MAX_BUFCNT_MEM_ACCESS, 128, 1, "TE_CB_VOQ_MAX_BUFCNT_MEM", 0, NULL },
    { TE_CB_VOQ_PCNT_MEM_ACCESS, 128, 1, "TE_CB_VOQ_PCNT_MEM", 0, NULL },
    { TE_CB_SRC_PORT_BUFCNT_MEM_ACCESS, 2, 1, "TE_CB_SRC_PORT_BUFCNT_MEM", 0, NULL },
    { TE_CB_SRC_PRI_BUFCNT_MEM_ACCESS, 8, 1, "TE_CB_SRC_PRI_BUFCNT_MEM", 0, NULL },
    { TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM_ACCESS, 8, 1, "TE_CB_DQSCH_VOQ_THRSH_PROFILE_MEM", 0, NULL },
    { TE_CB_DQSCH_VOQ_THRSH_SELECT_MEM_ACCESS, 64, 1, "TE_CB_DQSCH_VOQ_THRSH_SELECT_MEM", 0, NULL },
    { TE_PM_CNT_MEM_ACCESS, 512, 9, "TE_PM_CNT_MEM", 0, NULL },
    { TE_PIE_LPID_QOS_VOQ_ACCESS, 512, 1, "TE_PIE_LPID_QOS_VOQ", 1, { {0, 5, "value"}, } },
    { QM_QM_AXI_ATTRIBUTE_ACCESS, 64, 2, "QM_QM_AXI_ATTRIBUTE", 0, NULL },
    { QM_QM_BID_PADDR_LKUP_ACCESS, 8192, 1, "QM_QM_BID_PADDR_LKUP", 0, NULL },
    { QM_QM_VOQ_INFO_MEMORY_ACCESS, 512, 3, "QM_QM_VOQ_INFO_MEMORY", 5, { {0, 33, "voq_header"}, {33, 14, "voq_tail"}, {47, 14, "voq_head"}, {61, 14, "bcntr"}, {75, 9, "ecc"}, } },
    { QM_QM_HEADER_MEMORY_ACCESS, 16384, 2, "QM_QM_HEADER_MEMORY", 8, { {0, 14, "pkt_length"}, {14, 1, "eop"}, {15, 1, "sop"}, {16, 14, "next_buffer_ptr"}, {30, 1, "l4err"}, {31, 1, "pkt_type_cpu"}, {32, 1, "pkt_type_ptp"}, {33, 7, "ecc"}, } },
    { FBM_CPU_SW_ACCESS, 8, 2, "FBM_CPU_SW", 1, { {0, 40, "buf_ptr"}, } },
    { DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS, 8, 2, "DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH", 2, { {0, 4, "depth"}, {4, 36, "base_addr"}, } },
    { DMA_SEC_DMA_LSO_VP_TXQ_PKTCNT_ACCESS, 8, 1, "DMA_SEC_DMA_LSO_VP_TXQ_PKTCNT", 1, { {0, 24, "pkt_cnt"}, } },
    { DMA_SEC_SS_SHP_PORT_TBC_MEM_ACCESS, 16, 2, "DMA_SEC_SS_SHP_PORT_TBC_MEM", 7, { {0, 10, "rate_k"}, {10, 16, "rate_m"}, {26, 12, "bs"}, {38, 2, "enb"}, {40, 1, "mode"}, {41, 21, "tbc"}, {62, 1, "pkt_mode_class_sel"}, } },
    { DMA_SEC_SS_SHP_VOQ_TBC_MEM_ACCESS, 128, 2, "DMA_SEC_SS_SHP_VOQ_TBC_MEM", 7, { {0, 10, "rate_k"}, {10, 16, "rate_m"}, {26, 12, "bs"}, {38, 2, "enb"}, {40, 1, "mode"}, {41, 21, "tbc"}, {62, 1, "pkt_mode_class_sel"}, } },
    { EPON_EPON_GLB_TX_SW_FRAME_DATA_ACCESS, 256, 2, "EPON_EPON_GLB_TX_SW_FRAME_DATA", 0, NULL },
    { EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_ACCESS, 2048, 1, "EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC", 1, { {0, 32, "cnt"}, } },
    { EPON_EPON_MIB_RX_LLID_FRMCNT_MIB_ACC_ACCESS, 2048, 1, "EPON_EPON_MIB_RX_LLID_FRMCNT_MIB_ACC", 1, { {0, 32, "cnt"}, } },
    { EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC_ACCESS, 32, 1, "EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC", 1, { {0, 32, "cnt"}, } },
    { EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_ACCESS, 2048, 1, "EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC", 1, { {0, 32, "cnt"}, } },
    { EPON_EPON_MIB_TX_LLID_FRMCNT_MIB_ACC_ACCESS, 2048, 1, "EPON_EPON_MIB_TX_LLID_FRMCNT_MIB_ACC", 1, { {0, 32, "cnt"}, } },
    { EPON_EPON_MIB_TX_XGEPON_ETHNET_MIB_ACC_ACCESS, 32, 1, "EPON_EPON_MIB_TX_XGEPON_ETHNET_MIB_ACC", 1, { {0, 32, "cnt"}, } },
    { EPON_ONU_EPON_AES_DEC_KEY_TBL_ACCESS, 16, 4, "EPON_ONU_EPON_AES_DEC_KEY_TBL", 0, NULL },
    { EPON_XGEPN_XGPCS_TX_PKTGEN_MEM_ACCESS, 256, 3, "EPON_XGEPN_XGPCS_TX_PKTGEN_MEM", 1, { {0, 72, "PktData"}, } },
    { EPON_XGEPN_XGPCS_TX_FEC_MASK_MEM_SW_ACCESS, 32, 3, "EPON_XGEPN_XGPCS_TX_FEC_MASK_MEM_SW", 1, { {0, 66, "MASK_MEM_DATA"}, } },
    { EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS, 64, 1, "EPON_XGEPN_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM", 0, NULL },
    { EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS, 512, 11, "EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM", 0, NULL },
    { XGPN_DSFP_PORTID_XGEM_MIB_ACCESS, 128, 3, "XGPN_DSFP_PORTID_XGEM_MIB", 2, { {0, 32, "frame_cnt"}, {32, 64, "byte_cnt"}, } },
    { XGPN_USBG_XGEM_PID_TBL_ACCESS, 128, 1, "XGPN_USBG_XGEM_PID_TBL", 4, { {0, 16, "XGEM_PortID"}, {16, 1, "port_en"}, {17, 1, "aes_mode"}, {18, 1, "MC_flag"}, } },
    { XGPN_USBG_DBRU_BUFOCC_TBL_ACCESS, 32, 1, "XGPN_USBG_DBRU_BUFOCC_TBL", 1, { {0, 24, "BufOcc"}, } },
    { XGPN_USBG_PORTID_XGEM_MIB_ACCESS, 128, 3, "XGPN_USBG_PORTID_XGEM_MIB", 0, NULL },
    { XGPN_XGFEC_FEC_MASK_MEM_SW_ACCESS, 256, 1, "XGPN_XGFEC_FEC_MASK_MEM_SW", 0, NULL },
    { GPON_MAC_GPON_DS_PORT_ACCESS, 64, 4, "GPON_MAC_GPON_DS_PORT", 3, { {0, 32, "pcnt"}, {32, 32, "fcnt"}, {64, 64, "bcnt"}, } },
    { GPON_MAC_GPON_US_PORT_ID_ACCESS, 64, 1, "GPON_MAC_GPON_US_PORT_ID", 1, { {0, 12, "id"}, } },
    { GPON_MAC_GPON_DBRU_BUFOCC_TBL_ACCESS, 16, 1, "GPON_MAC_GPON_DBRU_BUFOCC_TBL", 0, NULL },
    { PUC_PVTBL_ACCESS, 48, 5, "PUC_PVTBL", 20, { {0, 8, "voq0"}, {8, 8, "voq1"}, {16, 8, "voq2"}, {24, 8, "voq3"}, {32, 8, "voq4"}, {40, 8, "voq5"}, {48, 8, "voq6"}, {56, 8, "voq7"}, {64, 1, "schmode"}, {68, 1, "entryvld"}, {72, 8, "wrrratio0"}, {80, 8, "wrrratio1"}, {88, 8, "wrrratio2"}, {96, 8, "wrrratio3"}, {104, 8, "wrrratio4"}, {112, 8, "wrrratio5"}, {120, 8, "wrrratio6"}, {128, 8, "wrrratio7"}, {136, 6, "nxtptr"}, {143, 1, "nxtptrvld"}, } },
    { PUC_HPMEM_ACCESS, 128, 1, "PUC_HPMEM", 1, { {0, 14, "hptr"}, } },
    { PUC_BCMEM_ACCESS, 128, 2, "PUC_BCMEM", 2, { {0, 21, "bytecnt"}, {21, 15, "bufcnt"}, } },
    { PUC_TQMEM_ACCESS, 128, 2, "PUC_TQMEM", 2, { {0, 21, "bytecnt"}, {21, 14, "pktcnt"}, } },
    { PUC_OPMEM_ACCESS, 32, 1, "PUC_OPMEM", 3, { {0, 14, "bytecnt"}, {14, 7, "voqid"}, {21, 7, "gempid"}, } },
    { PUC_BPMEM_ACCESS, 32768, 1, "PUC_BPMEM", 1, { {0, 15, "nptr"}, } },
    { PUC_VSTBL_ACCESS, 48, 5, "PUC_VSTBL", 8, { {0, 20, "tbc0"}, {20, 20, "tbc1"}, {40, 20, "tbc2"}, {60, 20, "tbc3"}, {80, 20, "tbc4"}, {100, 20, "tbc5"}, {120, 20, "tbc6"}, {140, 20, "tbc7"}, } },
    { PUC_VOQBPREMAP_ACCESS, 64, 1, "PUC_VOQBPREMAP", 1, { {0, 6, "tqmvoqid"}, } },
    { PUC_VOQ_TBC_MEM_ACCESS, 128, 2, "PUC_VOQ_TBC_MEM", 0, NULL },
    { PDC_PDC_MAP_MEM_ACCESS, 128, 2, "PDC_PDC_MAP_MEM", 16, { {0, 3, "cos"}, {3, 6, "ldpid"}, {9, 6, "lspid"}, {15, 1, "fe_bypass"}, {16, 2, "hdr_type"}, {18, 8, "mcgid"}, {26, 3, "drop_code"}, {29, 2, "rx_pkt_type"}, {31, 1, "no_drop"}, {32, 1, "mirror"}, {33, 1, "mark"}, {34, 2, "pol_en"}, {36, 9, "pol_id"}, {45, 3, "pol_grp_id"}, {48, 1, "deepq"}, {49, 1, "cpu_flag"}, } }
// 0x00000000, 0, 0, 'FLASH_SF',
// 0x00000000, 0, 0, 'FLASH_SF_EXT',
// 0x00000000, 0, 0, 'GPON_MAC_GPON_DS_GEM_PORT',
// 0x00000000, 0, 0, 'GPON_MAC_GPON_PLM_MIB',
// 0x00000000, 0, 0, 'GPON_MAC_GPON_TCONT',
// 0x00000000, 0, 0, 'GPON_MAC_GPON_US_MIB',
// 0x00000000, 0, 0, 'L2FE_IM3E_MLDDA_TBL_INDRCT',
// 0x00000000, 0, 0, 'L2FE_IM3E_MLDSA_TBL_INDRCT',
// 0x00000000, 0, 0, 'NI_GLB_RXMIB',
// 0x00000000, 0, 0, 'NI_GLB_TXMIB',
// 0x00000000, 0, 0, 'XGPON_BWMP_ALLOC_LUT',
};

#define BRINGUP_PRINT(...) cli_print(cli, __VA_ARGS__)

#define REG_PRINT(addr, expected, mask, verbose)    do{\
                                                        data = CA_REG_READ((void *)addr);\
                                                        data &= mask;\
                                                        if ( (verbose) || data != (expected) ) {\
                                                            BRINGUP_PRINT("%30s : 0x%08x\r\n", #addr, data);\
                                                        }\
                                                    } while(0)

#define REG_FIELD_PRINT(reg, field) BRINGUP_PRINT("%30s : 0x%0x\r\n", #field, reg.bf.field)


#define BRINGUP_BIT_TEST(data, offset) ( (data >> (offset)) & 1L )
#define BRINGUP_BIT_SET(data, offset) data = ( data | (1L << (offset)) )
#define BRINGUP_BIT_CLR(data, offset) data = ( data & ~(1L << (offset)) )

ca_int32_t _saturn_debug_find_tbl_spec(ca_uint32_t addr)
{

    ca_uint32_t tbl_length;
    ca_int32_t start, end, mid, result = -1;

    tbl_length = sizeof(SATURN_DEBUG_TBL) / sizeof(SATURN_DEBUG_TBL[0]);

    start = 0;
    end = tbl_length - 1;

    while ( start <= end ) {
        mid = start + ( end - start ) / 2;

        if ( SATURN_DEBUG_TBL[mid].addr < addr ) {
            start = mid + 1;
        }
        else if ( SATURN_DEBUG_TBL[mid].addr > addr ) {
            end = mid - 1;
        }
        else {
            result = mid;
            break;
        }
    }
    return result;
}

ca_int32_t _saturn_debug_data_check(ca_int8_t *str)
{
    ca_int8_t *c = str;
    ca_int32_t result = 1;

    if ( strlen(str) <= 130 ) {
        if ( str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ) ) {
            c += 2;
        }

        while (*c != '\0') {
            if ( (*c <= '9' && *c >= '0') ||
                    (*c <= 'f' && *c >= 'a') ||
                    (*c <= 'F' && *c >= 'A') ) {
                result = 1;
            } else {
                result = 0;
                break;
            }
            c++;
        }
    } else {
        result = 0;
    }
    return result;
}

ca_uint32_t _saturn_debug_str2data(ca_int8_t *str, ca_uint32_t *data)
{
    ca_int32_t length, i = 0 ;
    ca_int8_t frags[9], *tail, *head;

    head = str;
    if ( str[0] == '0' && ( str[1] == 'x' || str[1] == 'X') ) {
        head += 2;
    }

    while (*head == '0') {
        head++;
    }

    length = strlen(head);
    tail = head + length;

    if ( length > 0 ) {

        for (i = 0; i < length / 8; i++ ) {
            tail -= 8;
            strncpy(frags, tail, 8);
            frags[8] = '\0';
            data[i] = ca_strtoul( frags, NULL, 16);
        }

        if ( length > i * 8 ) {
            strncpy(frags, head, length - 8 * i);
            frags[length - 8 * i] = '\0';
            data[i] = ca_strtoul( frags, NULL, 16 );
            i++;
        }
    }
    return i;
}

void field_set(ca_uint32_t *buf, ca_uint32_t *value, ca_uint32_t offset, ca_uint32_t length)
{
    ca_uint32_t idx_offset, i;

    for ( i = 0; i < length; i++) {
        idx_offset = i + offset;

        if ( BRINGUP_BIT_TEST( value[ i / 32 ], i % 32) ) {
            BRINGUP_BIT_SET( buf[ idx_offset / 32], idx_offset % 32 );
        } else {
            BRINGUP_BIT_CLR( buf[ idx_offset / 32], idx_offset % 32 );
        }
    }
}

void field_get(ca_uint32_t *buf, ca_uint32_t *value, ca_uint32_t offset, ca_uint32_t length)
{
    ca_uint32_t idx_offset, i;

    memset(value, 0, 16 * sizeof(ca_uint32_t));
    for ( i = 0; i < length; i++ ) {
        idx_offset = i + offset;
        if ( BRINGUP_BIT_TEST( buf[ idx_offset / 32 ], idx_offset % 32 ) ) {
            BRINGUP_BIT_SET( value[ i / 32 ], i % 32 );
        } else {
            BRINGUP_BIT_CLR( value[ i / 32 ], i % 32 );
        }
    }
}

ca_int8_t * field_sprintf(ca_int8_t *str, ca_uint32_t *value, ca_uint32_t length)
{
    ca_int32_t i;
    memset(str, 0, 132 * sizeof(ca_int8_t));
    for (i = (length - 1) / 32; i >= 0; i--) {
        snprintf(str, 130, "0x%x", value[i]);
    }
    return str;
}

typedef enum {
    PON_MODE_EPON,
    PON_MODE_XGEPON,
    PON_MODE_GPON,
    PON_MODE_XGPON
} pon_mode_t;

pon_mode_t get_pon_mode(void)
{
    GLOBAL_PON_CNTL_t pon_cntl;
    GLOBAL_GPON_CNTL_t gpon_cntl;
    GLOBAL_EPON_CNTL_t epon_cntl;

    pon_cntl.wrd = CA_REG_READ((void *)GLOBAL_PON_CNTL);
    gpon_cntl.wrd = CA_REG_READ((void *)GLOBAL_GPON_CNTL);
    epon_cntl.wrd = CA_REG_READ((void *)GLOBAL_EPON_CNTL);

    if ( pon_cntl.bf.epon_mode == 1 ) {
        if ( epon_cntl.bf.epon_onu_mode == 1 ) {
            return PON_MODE_EPON;
        } else {
            return PON_MODE_XGEPON;
        }
    } else {
        if ( gpon_cntl.bf.nxgs_mode == 1 ) {
            return PON_MODE_XGPON;
        } else {
            return PON_MODE_GPON;
        }
    }
}

int bpmem_read(int nptr)
{
    ca_uint32_t dws[16];
    PUC_BPMEM_DATA_t bpmem_data;
    _saturn_indirect_mem_rd(PUC_BPMEM_ACCESS, nptr, 1, dws);
    bpmem_data.wrd = dws[0];
    return bpmem_data.bf.nptr;
}

#endif

CLI_KERNEL_DEFINE(saturn_debug_rd,
                  saturn_debug_rd_cmd,
                  "sat rd  <0-0xffffffff> [<1-65535>]",
                  "Saturn debug command\n"
                  "Read register\n"
                  "Register address\n"
                  "How many words do you want to read\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    unsigned long addr;
    unsigned int count = 0;
    int i;

    // should be 0xfxxxyyyy
    if (strlen(argv[0]) != 10) {
        return CLI_CMD_OK;
    }

    addr = ca_strtol((ca_int8 *)argv[0]);
    if (addr == 0)
        return CLI_CMD_OK;

    if (argc == 1)
        count = 1;
    else
        count = ca_strtol((ca_int8 *)argv[1]);


    BRINGUP_PRINT("\r\n");
    for (i = 0; i < count; i++)
    {

        if (i % 4 == 0)
        {
            BRINGUP_PRINT("%08x: ", addr);
        }

        BRINGUP_PRINT("%08x ", CA_REG_READ((void*)addr));

        if ((i + 1) % 4 == 0)
            BRINGUP_PRINT("\r\n");

        addr += 4;
    }
    BRINGUP_PRINT("\r\n");

    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_wr,
                  saturn_debug_wr_cmd,
                  "sat wr <0-0xffffffff> <0-0xffffffff>",
                  "Saturn debug command\n"
                  "Write register\n"
                  "Register address\n"
                  "Register value\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    unsigned long addr;
    unsigned int value;

    if (strlen(argv[0]) != 10) {
        return CLI_CMD_OK;
    }

    addr = ca_strtol((ca_int8 *)argv[0]);
    if (addr == 0)
        return CLI_CMD_OK;

    value = ca_strtol((ca_int8 *)argv[1]);
    CA_REG_WRITE(value, (void *)addr);

    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_mem_read,
                  saturn_debug_mem_read_cmd,
                  "sat mem r <0-0xffffffff> <0-0x40000000> [<1-2097152>]",
                  "Saturn debug command\n"
                  "memory operate\n"
                  "read\n"
                  "indirect reg addr\n"
                  "memory index\n"
                  "How many entires do you want to read\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    unsigned long addr;
    unsigned int idx, count = 1;
    ca_uint32_t dws[16];
    ca_uint32_t value[16];
    ca_int32_t tbl_idx;
    int i, j;
    saturn_debug_tbl_t *tbl;
    saturn_debug_bf_t *bf;
    ca_int8_t str[132];

    // should be 0xfxxxyyyy
    if (strlen(argv[0]) != 10) {
        return CLI_CMD_OK;
    }

    addr = ca_strtol((ca_int8 *)argv[0]);

    tbl_idx = _saturn_debug_find_tbl_spec(addr);

    if ( tbl_idx != -1 ) {
        tbl = &SATURN_DEBUG_TBL[tbl_idx];
        BRINGUP_PRINT("%s : \r\n", tbl->name);
    } else {
        BRINGUP_PRINT("The address 0x%x is not a valid memory address\r\n", addr);
        return CLI_CMD_OK;
    }

    idx = ca_strtol((ca_int8 *)argv[1]);

    if ( idx < 0 || idx >= tbl->length ) {
        BRINGUP_PRINT("The index out of range [0, %d]. \r\n", tbl->length - 1);
        return CLI_CMD_OK;
    }

    if (argc > 2) {
        count = ca_strtol((ca_int8 *)argv[2]);
    }

    if ( ( idx + count ) > tbl->length ) {
        count = tbl->length - idx;
    }

    if ( count == 1 ) {
        BRINGUP_PRINT("%08x: 0x", idx);
        _saturn_indirect_mem_rd(addr, idx, tbl->width, dws);
        for (j = 0; j < tbl->width; j++)
        {
            BRINGUP_PRINT("%08x", dws[tbl->width - j - 1]);
        }
        BRINGUP_PRINT("\r\n");
        for (i = 0; i < tbl->num; i++ ) {
            bf = &tbl->bf[i];

            field_get(dws, value, bf->offset, bf->length);
            BRINGUP_PRINT("%30s : %s\r\n", bf->name, field_sprintf(str, value, bf->length));
        }
    } else {
        for (i = 0; i < count; i++)
        {
            BRINGUP_PRINT("%08x: 0x", idx);
            _saturn_indirect_mem_rd(addr, idx, tbl->width, dws);
            for (j = 0; j < tbl->width; j++)
            {
                BRINGUP_PRINT("%08x", dws[tbl->width - j - 1]);
            }
            BRINGUP_PRINT("\r\n");
            idx++;
        }
    }
    BRINGUP_PRINT("\r\n");

    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_mem_write,
                  saturn_debug_mem_write_cmd,
                  "sat mem w <0-0xffffffff> <0-0x40000000> DATA [<1-2097152>]",
                  "Saturn debug command\n"
                  "memory operate\n"
                  "write\n"
                  "indirect reg addr\n"
                  "memory index\n"
                  "data in HEX format (e.g. 0x0123456789abcdef)\n"
                  "How many entires do you want to write\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_uint32_t addr, width;
    ca_uint32_t idx = 0, count = 1;
    ca_uint32_t dws[16] = {0,};
    ca_uint32_t value[16];
    ca_int32_t tbl_idx;
    saturn_debug_tbl_t *tbl;
    saturn_debug_bf_t *bf;
    ca_int32_t i, j;
    ca_int8_t str[132];

    // should be 0xfxxxyyyy
    if (strlen(argv[0]) != 10) {
        return CLI_CMD_OK;
    }

    addr = ca_strtol((ca_int8 *)argv[0]);

    tbl_idx = _saturn_debug_find_tbl_spec(addr);

    if ( tbl_idx != -1 ) {
        tbl = &SATURN_DEBUG_TBL[tbl_idx];
        BRINGUP_PRINT("%s : \r\n", tbl->name);
    } else {
        BRINGUP_PRINT("The address 0x%x is not a valid memory address\r\n", addr);
        return CLI_CMD_OK;
    }

    idx = ca_strtol((ca_int8 *)argv[1]);

    if ( idx < 0 || idx >= tbl->length ) {
        BRINGUP_PRINT("The index out of range [0, %d]. \r\n", tbl->length - 1);
        return CLI_CMD_OK;
    }

    if (argc > 3) {
        count = ca_strtol((ca_int8 *)argv[3]);
    }

    if ( ( idx + count ) > tbl->length ) {
        count = tbl->length - idx;
    }

    if (argc > 2) {
        if ( _saturn_debug_data_check(argv[2]) ) {
            width = _saturn_debug_str2data( argv[2], dws );

            if ( count == 1 ) {
                BRINGUP_PRINT("%08x: 0x", idx);
                _saturn_indirect_mem_wr(addr, idx, tbl->width, dws);
                for (j = 0; j < tbl->width; j++)
                {
                    BRINGUP_PRINT("%08x", dws[tbl->width - j - 1]);
                }
                BRINGUP_PRINT("\r\n");
                for (i = 0; i < tbl->num; i++ ) {
                    bf = &tbl->bf[i];
                    field_get(dws, value, bf->offset, bf->length);
                    BRINGUP_PRINT("%30s : %s\r\n", bf->name, field_sprintf(str, value, bf->length));
                }
            } else {
                for (i = 0; i < count; i++)
                {
                    BRINGUP_PRINT("%08x: 0x", idx);
                    _saturn_indirect_mem_wr(addr, idx, tbl->width, dws);
                    for (j = 0; j < tbl->width; j++)
                    {
                        BRINGUP_PRINT("%08x", dws[tbl->width - j - 1]);
                    }
                    BRINGUP_PRINT("\r\n");
                    idx++;
                }
            }
        } else {
            BRINGUP_PRINT("unknown HEX data format\r\n");
            return CLI_CMD_OK;
        }
    }
    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_get_reset,
                  saturn_debug_get_reset_cmd,
                  "sat get reset",
                  "Saturn debug command\n"
                  "show status\n"
                  "reset control registers\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    GLOBAL_BLOCK_RESET_t    block_reset;
    GLOBAL_FABRIC_RESET_t   fabric_reset;
    GLOBAL_GLOBAL_CONFIG_t  global_config;
    GLOBAL_GPON_CNTL_t      gpon_cntl;
    GLOBAL_EPON_CNTL_t      epon_cntl;
    GLOBAL_PON_CNTL_t       pon_cntl;
    GLOBAL_GIGE_PHY_t       gige_phy;

    BRINGUP_PRINT("\r\n");

    block_reset.wrd = CA_REG_READ((void*)GLOBAL_BLOCK_RESET);
    BRINGUP_PRINT("\tGLOBAL_BLOCK_RESET: 0x%0x\r\n", block_reset.wrd);
    BRINGUP_PRINT("\t\treset_ni         : 0x%0x\r\n", block_reset.bf.reset_ni         );
    BRINGUP_PRINT("\t\treset_l2fe       : 0x%0x\r\n", block_reset.bf.reset_l2fe       );
    BRINGUP_PRINT("\t\treset_l2tm       : 0x%0x\r\n", block_reset.bf.reset_l2tm       );
    BRINGUP_PRINT("\t\treset_l3fe       : 0x%0x\r\n", block_reset.bf.reset_l3fe       );
    BRINGUP_PRINT("\t\treset_sdram      : 0x%0x\r\n", block_reset.bf.reset_sdram      );
    BRINGUP_PRINT("\t\treset_tqm        : 0x%0x\r\n", block_reset.bf.reset_tqm        );
    BRINGUP_PRINT("\t\treset_flash      : 0x%0x\r\n", block_reset.bf.reset_flash      );
    BRINGUP_PRINT("\t\treset_per        : 0x%0x\r\n", block_reset.bf.reset_per        );
    BRINGUP_PRINT("\t\treset_dma        : 0x%0x\r\n", block_reset.bf.reset_dma        );
    BRINGUP_PRINT("\t\treset_cpu_ap     : 0x%0x\r\n", block_reset.bf.reset_cpu_ap     );
    BRINGUP_PRINT("\t\treset_cpu_dsp    : 0x%0x\r\n", block_reset.bf.reset_cpu_dsp    );
    BRINGUP_PRINT("\t\treset_rcpu0      : 0x%0x\r\n", block_reset.bf.reset_rcpu0      );
    BRINGUP_PRINT("\t\treset_rcpu1      : 0x%0x\r\n", block_reset.bf.reset_rcpu1      );
    BRINGUP_PRINT("\t\treset_taroko0_atu: 0x%0x\r\n", block_reset.bf.reset_taroko0_atu);
    BRINGUP_PRINT("\t\treset_taroko1_atu: 0x%0x\r\n", block_reset.bf.reset_taroko1_atu);
    BRINGUP_PRINT("\t\treset_ldma       : 0x%0x\r\n", block_reset.bf.reset_ldma       );
    BRINGUP_PRINT("\t\treset_fbm        : 0x%0x\r\n", block_reset.bf.reset_fbm        );
    BRINGUP_PRINT("\t\treset_eaxi       : 0x%0x\r\n", block_reset.bf.reset_eaxi       );
    BRINGUP_PRINT("\t\treset_sd         : 0x%0x\r\n", block_reset.bf.reset_sd         );
    BRINGUP_PRINT("\t\treset_otprom     : 0x%0x\r\n", block_reset.bf.reset_otprom     );
    BRINGUP_PRINT("\t\treset_iic_slv    : 0x%0x\r\n", block_reset.bf.reset_iic_slv    );

    fabric_reset.wrd = CA_REG_READ((void *)GLOBAL_FABRIC_RESET);
    BRINGUP_PRINT("\tGLOBAL_FABRIC_RESET: 0x%0x\r\n", fabric_reset.wrd);
    BRINGUP_PRINT("\t\treset_capsram   : 0x%0x\r\n", fabric_reset.bf.reset_capsram    );
    BRINGUP_PRINT("\t\treset_capsramreg: 0x%0x\r\n", fabric_reset.bf.reset_capsramreg );
    BRINGUP_PRINT("\t\treset_cpu_sw    : 0x%0x\r\n", fabric_reset.bf.reset_cpu_sw     );
    BRINGUP_PRINT("\t\treset_ne_sw     : 0x%0x\r\n", fabric_reset.bf.reset_ne_sw      );
    BRINGUP_PRINT("\t\treset_io_sw     : 0x%0x\r\n", fabric_reset.bf.reset_io_sw      );

    global_config.wrd = CA_REG_READ((void *)GLOBAL_GLOBAL_CONFIG);
    BRINGUP_PRINT("\tGLOBAL_GLOBAL_CONFIG: 0x%0x\r\n", global_config.wrd);
    BRINGUP_PRINT("\t\twd_reset_subsys_enable: 0x%0x\r\n", global_config.bf.wd_reset_subsys_enable );
    BRINGUP_PRINT("\t\twd_reset_all_blocks   : 0x%0x\r\n", global_config.bf.wd_reset_all_blocks    );
    BRINGUP_PRINT("\t\twd_reset_remap        : 0x%0x\r\n", global_config.bf.wd_reset_remap         );
    BRINGUP_PRINT("\t\twd_reset_ext_reset    : 0x%0x\r\n", global_config.bf.wd_reset_ext_reset     );
    BRINGUP_PRINT("\t\text_reset             : 0x%0x\r\n", global_config.bf.ext_reset              );
    BRINGUP_PRINT("\t\tgpon_pd               : 0x%0x\r\n", global_config.bf.gpon_pd                );
    BRINGUP_PRINT("\t\tepon_pd               : 0x%0x\r\n", global_config.bf.epon_pd                );
    BRINGUP_PRINT("\t\tl3fe_pd               : 0x%0x\r\n", global_config.bf.l3fe_pd                );
    BRINGUP_PRINT("\t\tper_pd                : 0x%0x\r\n", global_config.bf.per_pd                 );
    BRINGUP_PRINT("\t\tflash_pd              : 0x%0x\r\n", global_config.bf.flash_pd               );
    BRINGUP_PRINT("\t\tsd_pd                 : 0x%0x\r\n", global_config.bf.sd_pd                  );
    BRINGUP_PRINT("\t\tiosw_pd               : 0x%0x\r\n", global_config.bf.iosw_pd                );

    gpon_cntl.wrd = CA_REG_READ((void *)GLOBAL_GPON_CNTL);
    BRINGUP_PRINT("\tGLOBAL_GPON_CNTL: 0x%0x\r\n", gpon_cntl.wrd);
    BRINGUP_PRINT("\t\tani_rst_n : 0x%0x\r\n", gpon_cntl.bf.ani_rst_n  );
    BRINGUP_PRINT("\t\tgpon_rst_n: 0x%0x\r\n", gpon_cntl.bf.gpon_rst_n );
    BRINGUP_PRINT("\t\txgpn_rst_n: 0x%0x\r\n", gpon_cntl.bf.xgpn_rst_n );
    BRINGUP_PRINT("\t\tus10g_mode: 0x%0x\r\n", gpon_cntl.bf.us10g_mode );
    BRINGUP_PRINT("\t\tnxgs_mode : 0x%0x\r\n", gpon_cntl.bf.nxgs_mode  );

    epon_cntl.wrd = CA_REG_READ((void *)GLOBAL_EPON_CNTL);
    BRINGUP_PRINT("\tGLOBAL_EPON_CNTL: 0x%0x\r\n", epon_cntl.wrd);
    BRINGUP_PRINT("\t\tepon_rst_n         : 0x%0x\r\n", epon_cntl.bf.epon_rst_n          );
    BRINGUP_PRINT("\t\tonumpcp_rst_n      : 0x%0x\r\n", epon_cntl.bf.onumpcp_rst_n       );
    BRINGUP_PRINT("\t\tonugepn_rst_n      : 0x%0x\r\n", epon_cntl.bf.onugepn_rst_n       );
    BRINGUP_PRINT("\t\txgepn_rst_n        : 0x%0x\r\n", epon_cntl.bf.xgepn_rst_n         );
    BRINGUP_PRINT("\t\trx_coresec_10_rst_n: 0x%0x\r\n", epon_cntl.bf.rx_coresec_10_rst_n );
    BRINGUP_PRINT("\t\ttx_coresec_10_rst_n: 0x%0x\r\n", epon_cntl.bf.tx_coresec_10_rst_n );
    BRINGUP_PRINT("\t\tgepn_rx_rst_n      : 0x%0x\r\n", epon_cntl.bf.gepn_rx_rst_n       );
    BRINGUP_PRINT("\t\tgepn_tx_rst_n      : 0x%0x\r\n", epon_cntl.bf.gepn_tx_rst_n       );
    BRINGUP_PRINT("\t\txgepn_rx_rst_n     : 0x%0x\r\n", epon_cntl.bf.xgepn_rx_rst_n      );
    BRINGUP_PRINT("\t\txgepn_tx_rst_n     : 0x%0x\r\n", epon_cntl.bf.xgepn_tx_rst_n      );
    BRINGUP_PRINT("\t\tepon_onu_mode      : 0x%0x\r\n", epon_cntl.bf.epon_onu_mode       );
    BRINGUP_PRINT("\t\tepon_ethernet_mode : 0x%0x\r\n", epon_cntl.bf.epon_ethernet_mode  );

    pon_cntl.wrd = CA_REG_READ((void *)GLOBAL_PON_CNTL);
    BRINGUP_PRINT("\tGLOBAL_PON_CNTL: 0x%0x\r\n", pon_cntl.wrd);
    BRINGUP_PRINT("\t\tepon_mode       : 0x%0x\r\n", pon_cntl.bf.epon_mode        );
    BRINGUP_PRINT("\t\tpon_serdes_rst_n: 0x%0x\r\n", pon_cntl.bf.pon_serdes_rst_n );
    BRINGUP_PRINT("\t\tpsds_reg_rst_n  : 0x%0x\r\n", pon_cntl.bf.psds_reg_rst_n   );
    BRINGUP_PRINT("\t\tptp_rst_n       : 0x%0x\r\n", pon_cntl.bf.ptp_rst_n        );
    BRINGUP_PRINT("\t\tpuc_reset       : 0x%0x\r\n", pon_cntl.bf.puc_reset        );
    BRINGUP_PRINT("\t\tpdc_reset       : 0x%0x\r\n", pon_cntl.bf.pdc_reset        );
    BRINGUP_PRINT("\r\n");

    gige_phy.wrd = CA_REG_READ((void *)GLOBAL_GIGE_PHY);
    BRINGUP_PRINT("\tGLOBAL_GIGE_PHY: 0x%0x\r\n", gige_phy.wrd);
    BRINGUP_PRINT("\t\tgphy_phyrst_cen_b: 0x%0x\r\n", gige_phy.bf.gphy_phyrst_cen_b);

    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_get_config,
                  saturn_debug_get_config_cmd,
                  "sat get config",
                  "Saturn debug command\n"
                  "show status\n"
                  "strap pins and PON mode\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    GLOBAL_STRAP_t                strap;
    GLOBAL_STRAP_OVERRIDE_t       strap_override;
    GLOBAL_STRAP_OVERRIDE_VALUE_t strap_override_value;
    GLOBAL_GPON_CNTL_t            gpon_cntl;
    GLOBAL_EPON_CNTL_t            epon_cntl;
    GLOBAL_PON_CNTL_t             pon_cntl;
    XGPN_XGPN_GLB_CTRL_t          xgpn_glb_ctrl;
    EPON_EPON_GLB_STATIC_CFG_t    epon_glb_static_cfg;
    EPON_EPON_GLB_MISC_CTRL_t     epon_glb_misc_ctrl;
    EPON_XGEPN_XGPCS_TX_TXCNTRL_t xgepn_xgpcs_tx_txcntrl;

    BRINGUP_PRINT("\r\n");

    strap.wrd = CA_REG_READ((void *)GLOBAL_STRAP);
    BRINGUP_PRINT("\tGLOBAL_STRAP: 0x%0x\r\n", strap.wrd);
    REG_FIELD_PRINT(strap, debug_mode   );
    REG_FIELD_PRINT(strap, speed        );
    REG_FIELD_PRINT(strap, dsp_avail    );
    REG_FIELD_PRINT(strap, chip_type    );
    REG_FIELD_PRINT(strap, sd_emmc_avail);
    REG_FIELD_PRINT(strap, flashSize    );
    REG_FIELD_PRINT(strap, flashWidth   );
    REG_FIELD_PRINT(strap, flashPin     );
    REG_FIELD_PRINT(strap, flashType    );
    REG_FIELD_PRINT(strap, uart_message );
    REG_FIELD_PRINT(strap, sd           );
    REG_FIELD_PRINT(strap, misc_mode    );
    REG_FIELD_PRINT(strap, jtag_osc_2   );
    REG_FIELD_PRINT(strap, jtag_osc_3   );

    strap_override.wrd = CA_REG_READ((void *)GLOBAL_STRAP_OVERRIDE);
    BRINGUP_PRINT("\tGLOBAL_STRAP_OVERRIDE: 0x%0x\r\n", strap_override.wrd);
    REG_FIELD_PRINT(strap_override, rsrvd1       );
    REG_FIELD_PRINT(strap_override, speed        );
    REG_FIELD_PRINT(strap_override, dsp_avail    );
    REG_FIELD_PRINT(strap_override, chip_type    );
    REG_FIELD_PRINT(strap_override, rsrvd2       );
    REG_FIELD_PRINT(strap_override, sd_emmc_avail);
    REG_FIELD_PRINT(strap_override, flashSize    );
    REG_FIELD_PRINT(strap_override, flashWidth   );
    REG_FIELD_PRINT(strap_override, flashPin     );
    REG_FIELD_PRINT(strap_override, flashType    );
    REG_FIELD_PRINT(strap_override, uart_message );
    REG_FIELD_PRINT(strap_override, sd           );
    REG_FIELD_PRINT(strap_override, rsrvd3       );
    REG_FIELD_PRINT(strap_override, misc_mode    );
    REG_FIELD_PRINT(strap_override, rsrvd4       );

    strap_override_value.wrd = CA_REG_READ((void *)GLOBAL_STRAP_OVERRIDE_VALUE);
    BRINGUP_PRINT("\tGLOBAL_STRAP_OVERRIDE_VALUE: 0x%0x\r\n", strap_override_value.wrd);
    REG_FIELD_PRINT(strap_override_value, speed         );
    REG_FIELD_PRINT(strap_override_value, dsp_avail     );
    REG_FIELD_PRINT(strap_override_value, chip_type     );
    REG_FIELD_PRINT(strap_override_value, sd_emmc_avail );
    REG_FIELD_PRINT(strap_override_value, flashSize     );
    REG_FIELD_PRINT(strap_override_value, flashWidth    );
    REG_FIELD_PRINT(strap_override_value, flashPin      );
    REG_FIELD_PRINT(strap_override_value, flashType     );
    REG_FIELD_PRINT(strap_override_value, uart_message  );
    REG_FIELD_PRINT(strap_override_value, sd            );
    REG_FIELD_PRINT(strap_override_value, misc_mode     );

    pon_cntl.wrd = CA_REG_READ((void *)GLOBAL_PON_CNTL);
    BRINGUP_PRINT("\tGLOBAL_PON_CNTL: 0x%0x\r\n", pon_cntl.wrd );
    BRINGUP_PRINT("\t\tepon_mode: 0x%0x\r\n", pon_cntl.bf.epon_mode );

    if (pon_cntl.bf.epon_mode) {
        epon_cntl.wrd = CA_REG_READ((void *)GLOBAL_EPON_CNTL);
        BRINGUP_PRINT("\tGLOBAL_EPON_CNTL: 0x%0x\r\n", epon_cntl.wrd);
        REG_FIELD_PRINT(epon_cntl, epon_ethernet_mode );
        REG_FIELD_PRINT(epon_cntl, epon_onu_mode      );

        epon_glb_static_cfg.wrd = CA_REG_READ((void *)EPON_EPON_GLB_STATIC_CFG);
        BRINGUP_PRINT("\tEPON_EPON_GLB_STATIC_CFG: 0x%0x\r\n", epon_glb_static_cfg.wrd);
        REG_FIELD_PRINT(epon_glb_static_cfg, laser_always_on );
        REG_FIELD_PRINT(epon_glb_static_cfg, laser_tx_polarity );

        epon_glb_misc_ctrl.wrd = CA_REG_READ((void *)EPON_EPON_GLB_MISC_CTRL);
        BRINGUP_PRINT("\tEPON_EPON_GLB_MISC_CTRL: 0x%0x\r\n", epon_glb_misc_ctrl.wrd);
        REG_FIELD_PRINT(epon_glb_misc_ctrl, dis_opt_power         );
        REG_FIELD_PRINT(epon_glb_misc_ctrl, dis_opt_power_polarity);

        xgepn_xgpcs_tx_txcntrl.wrd = CA_REG_READ((void *)EPON_XGEPN_XGPCS_TX_TXCNTRL);
        BRINGUP_PRINT("\tEPON_XGEPN_XGPCS_TX_TXCNTRL: 0x%0x\r\n", xgepn_xgpcs_tx_txcntrl.wrd);
        REG_FIELD_PRINT(xgepn_xgpcs_tx_txcntrl, laserctrldelay    );
        REG_FIELD_PRINT(xgepn_xgpcs_tx_txcntrl, laserctrlpolarity );

    } else {
        gpon_cntl.wrd = CA_REG_READ((void *)GLOBAL_GPON_CNTL);
        BRINGUP_PRINT("\tGLOBAL_GPON_CNTL: 0x%0x\r\n", gpon_cntl.wrd);
        REG_FIELD_PRINT(gpon_cntl, us10g_mode );
        REG_FIELD_PRINT(gpon_cntl, nxgs_mode  );

        xgpn_glb_ctrl.wrd = CA_REG_READ((void *)XGPN_XGPN_GLB_CTRL);
        BRINGUP_PRINT("\tXGPN_XGPN_GLB_CTRL: 0x%0x\r\n", xgpn_glb_ctrl.wrd);
        REG_FIELD_PRINT(xgpn_glb_ctrl, mac_en);
    }
    return CLI_CMD_OK;
}
#endif

#ifdef CLI_KERNEL_SPACE


#endif

CLI_KERNEL_DEFINE(saturn_debug_get_status,
                  saturn_debug_get_status_cmd,
                  "sat get status [<0-1>]",
                  "Saturn debug command\n"
                  "show status\n"
                  "interrupt registers\n"
                  "verbose\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_uint32_t data;
    ca_uint32_t verbose = 0;
    if (argc > 0) {
        verbose = ca_strtol((ca_int8 *)argv[0]);
    }
    BRINGUP_PRINT("\r\n");

    REG_PRINT(GLOBAL_TAROKO_INT, 0, 0xffffffff, verbose);
    REG_PRINT(GLOBAL_SYSTEM_INTERRUPT_0, 0, 0xffffffff, verbose);
    REG_PRINT(GLOBAL_LDMA_INTERRUPT_0, 0, 0xffffffff, verbose);
    REG_PRINT(GLOBAL_CPU_QM_INTERRUPT_0, 0, 0xffffffff, verbose);
    REG_PRINT(GLOBAL_NETWORK_ENGINE_INTERRUPT_0, 0, 0xffffffff, verbose);
    REG_PRINT(GLOBAL_MISC_INTERRUPT, 0, 0xffffffff, verbose);
    REG_PRINT(GLOBAL_WOL_INTERRUPT, 0, 0xffffffff, verbose);
    REG_PRINT(GLOBAL_NETWORK_ENGINE_STATUS, 0, 0xffffffff, verbose);
    REG_PRINT(GLOBAL_BIST_RESULT, 0xffff01ff, 0xffffffff, verbose);

    switch ( get_pon_mode() ) {
    case PON_MODE_EPON:
    case PON_MODE_XGEPON:
        REG_PRINT(EPON_EPON_GLB_EPON_STATUS, 0, 0xffffffff, 1);
        REG_PRINT(EPON_EPON_GLB_EXTIF_INTERRUPT, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_EPON_GLB_NI_MODULE_IRQ_STATS, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_XMAC_CTRL_XGEPN_FF_STATUS, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_XMAC_CTRL_XGEPN_INTERRUPT, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_XGPCS_TX_TXINT, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_XGPCS_RX_RXINT, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_EPON_GLB_ONU_EPON_INTERRUPT, 0, 0xffffffff, verbose);
        //
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_NANDI_INTERRUPT_0, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_NANDI_INTERRUPT_1, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_CORESEC10_INTERRUPT_0, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_CORESEC10_INTERRUPT_1, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_NANDI_INTERRUPT_0, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_NANDI_INTERRUPT_1, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_CORESEC10_INTERRUPT_0, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_CORESEC10_INTERRUPT_1, 0, 0xffffffff, verbose);
        REG_PRINT(EPON_XGONU_MPCP_MPCP_IRQ_SRC_INTERRUPT, 0, 0xffffffff, verbose);
        break;
    case PON_MODE_GPON:
        REG_PRINT(GPON_MAC_GPON_int_top_en, 0, 0xffffffff, verbose);
        REG_PRINT(GPON_MAC_GPON_interrupt, 0, 0xffffffff, 1);
        REG_PRINT(GPON_MAC_GPON_interrupt2, 0, 0xffffffff, 1);
        REG_PRINT(GPON_MAC_GPON_interrupt3, 0, 0xffffffff, 1);
        REG_PRINT(GPON_MAC_GPON_interrupt4, 0, 0xffffffff, 1);
        break;
    case PON_MODE_XGPON:
        REG_PRINT(XGPN_XGPN_GLB_INTERRUPT0, 0, 0xffffffff, verbose);
        REG_PRINT(XGPN_ACTP_INTERRUPT, 0, 0xffffffff, 1);
        REG_PRINT(XGPN_DSFP_INTERRUPT, 0, 0xffffffff, verbose);
        REG_PRINT(XGPN_RXCTL_INTERRUPT, 0, 0xffffffff, verbose);
        REG_PRINT(XGPN_PLMP_INTERRUPT, 0, 0xffffffff, verbose);
        REG_PRINT(XGPN_BWMP_INTERRUPT, 0, 0xffffffff, verbose);
        REG_PRINT(XGPN_XGFEC_RXINT, 0, 0xffffffff, verbose);
        break;
    }
    REG_PRINT(PER_PERIPHERAL_INTERRUPT_0, 0, 0xffffffff, verbose);
    REG_PRINT(PER_PERIPHERAL_INTERRUPT_1, 0, 0xffffffff, verbose);
    REG_PRINT(PER_UART0_INT_0, 0, 0xffffffff, verbose);
    REG_PRINT(PER_UART0_INT_1, 0, 0xffffffff, verbose);
    REG_PRINT(PER_UART1_INT_0, 0, 0xffffffff, verbose);
    REG_PRINT(PER_UART1_INT_1, 0, 0xffffffff, verbose);
    REG_PRINT(PER_MDIO_INT_0, 0, 0xffffffff, verbose);
    REG_PRINT(PER_MDIO_INT_1, 0, 0xffffffff, verbose);
    BRINGUP_PRINT("\r\n");
    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_get_mib,
                  saturn_debug_get_mib_cmd,
                  "sat get mib",
                  "Saturn debug command\n"
                  "show status\n"
                  "MIB counters\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_uint32_t dws[16];
    ca_int32_t pid;
    ca_uint32_t data, frame_cnt, fcnt, pcnt, cnt;
    ca_uint64_t byte_cnt, bcnt;

    switch ( get_pon_mode() ) {
    case PON_MODE_EPON:
    case PON_MODE_XGEPON:
        BRINGUP_PRINT("EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC:\r\n");
        for (pid = 0; pid < 32; pid++) {
            _saturn_indirect_mem_rd(EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC_ACCESS, pid, 1, dws);
            cnt = dws[0];
            if ( cnt != 0 )
                BRINGUP_PRINT("\tPORT: %d, cnt = %d\r\n", pid, cnt);
        }

        data = CA_REG_READ((void *)EPON_XGEPN_XMAC_RXMAC_RX_PAKT_MIB1);
        if ( data != 0 ) {
            BRINGUP_PRINT("EPON_XGEPN_XMAC_RXMAC_RX_PAKT_MIB0: \r\n");
            BRINGUP_PRINT("\tllid : %d, ", CA_REG_READ((void *)EPON_XGEPN_XMAC_RXMAC_RX_PAKT_MIB0) & 0xffff);
            BRINGUP_PRINT("count: %d\r\n", data);
        }

        BRINGUP_PRINT("EPON_EPON_MIB_TX_XGEPON_ETHNET_MIB_ACC:\r\n");
        for (pid = 0; pid < 32; pid++) {
            _saturn_indirect_mem_rd(EPON_EPON_MIB_TX_XGEPON_ETHNET_MIB_ACC_ACCESS, pid, 1, dws);
            cnt = dws[0];
            if ( cnt != 0 )
                BRINGUP_PRINT("\tPORT: %d, cnt = %d\r\n", pid, cnt);
        }

        // To Do:
        // EPON_XGEPN_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS

        data = CA_REG_READ((void *)EPON_XGEPN_XMAC_TXMAC_TX_PAKT_MIB1);
        if ( data != 0 ) {
            BRINGUP_PRINT("EPON_XGEPN_XMAC_TXMAC_TX_PAKT_MIB0: \r\n");
            BRINGUP_PRINT("\tllid : %d, ", CA_REG_READ((void *)EPON_XGEPN_XMAC_TXMAC_TX_PAKT_MIB0) & 0xffff);
            BRINGUP_PRINT("count: %d\r\n", data);

        }
        break;
    case PON_MODE_GPON:
        BRINGUP_PRINT("GPON_MAC_GPON_DS_PORT:\r\n");
        for (pid = 0; pid < 64; pid++) {
            _saturn_indirect_mem_rd(GPON_MAC_GPON_DS_PORT_ACCESS, pid, 4, dws);
            pcnt = dws[0];
            fcnt = dws[1];
            bcnt = dws[3];
            bcnt <<= 32;
            bcnt |= dws[2];
            if ( bcnt != 0 )
                BRINGUP_PRINT("\tPORT: %d, pcnt = %d, fcnt = %d, bcnt = %lld\r\n", pid, pcnt, fcnt, bcnt);
        }

        BRINGUP_PRINT("GPON_MAC_GPON_US_MIB:\r\n");
        for (pid = 0; pid < 0x53; pid++) {
            _saturn_indirect_mem_rd(GPON_MAC_GPON_US_MIB_ACCESS, pid, 3, dws);
            fcnt = dws[0];
            bcnt = dws[3];
            bcnt <<= 32;
            bcnt |= dws[2];
            if ( bcnt != 0 ) {
                if ( pid < 0x40 ) {
                    BRINGUP_PRINT("\tGEM Port: %d, fcnt = %d, bcnt = %lld\r\n", pid, fcnt, bcnt);
                } else if ( pid == 0x40 ) {
                    BRINGUP_PRINT("\tOMCCellCnt: fcnt = %d, bcnt = %lld\r\n", fcnt, bcnt);
                } else if ( pid <= 0x49 ) {
                    BRINGUP_PRINT("\tTCONT PKT: %d, fcnt = %d, bcnt = %lld\r\n", (pid - 0x41), fcnt, bcnt);
                } else {
                    BRINGUP_PRINT("\tTCONT IDLE: %d, fcnt = %d, bcnt = %lld\r\n", (pid - 0x4a), fcnt, bcnt);
                }
            }
        }
        break;
    case PON_MODE_XGPON:
        BRINGUP_PRINT("XGPN_DSFP_PORTID_XGEM_MIB:\r\n");
        for (pid = 0; pid < 128; pid++) {
            _saturn_indirect_mem_rd(XGPN_DSFP_PORTID_XGEM_MIB_ACCESS, pid, 3, dws);
            frame_cnt = dws[0];
            byte_cnt = dws[2];
            byte_cnt <<= 32;
            byte_cnt |= dws[1];
            if ( byte_cnt != 0 )
                BRINGUP_PRINT("\tPORT: %d, frame_cnt = %d, byte_cnt = %lld\r\n", pid, frame_cnt, byte_cnt);
        }

        BRINGUP_PRINT("XGPN_USBG_PORTID_XGEM_MIB:\r\n");
        for (pid = 0; pid < 128; pid++) {
            _saturn_indirect_mem_rd(XGPN_USBG_PORTID_XGEM_MIB_ACCESS, pid, 3, dws);
            frame_cnt = dws[0];
            byte_cnt = dws[2];
            byte_cnt <<= 32;
            byte_cnt |= dws[1];
            if ( byte_cnt != 0 )
                BRINGUP_PRINT("\tPORT: %d, frame_cnt = %d, byte_cnt = %lld\r\n", pid, frame_cnt, byte_cnt);
        }
        break;
    }
    return CLI_CMD_OK;

}
#endif


CLI_KERNEL_DEFINE(saturn_debug_get_cnt,
                  saturn_debug_get_cnt_cmd,
                  "sat get cnt",
                  "Saturn debug command\n"
                  "show status\n"
                  "traffic counters\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_uint32_t data, frame_cnt;
    ca_int32_t pid;
    ca_uint64_t byte_cnt;

    BRINGUP_PRINT("PDC:\r\n");
    REG_PRINT(PDC_PKT_CNTR, 0, 0xffffffff, 0);
    REG_PRINT(PDC_CONTROL_PKT_CNTR_mac, 0, 0xffff, 0);
    REG_PRINT(PDC_CONTROL_PKT_CNTR_lnk, 0, 0xffff, 0);

    BRINGUP_PRINT("PUC:\r\n");
    REG_PRINT(PUC_BMC_RX_PKT, 0, 0xffff, 0);
    REG_PRINT(PUC_BMC_CONTROL_PKT_CNTR_mac, 0, 0xffff, 0);
    REG_PRINT(PUC_BMC_CONTROL_PKT_CNTR_lnk, 0, 0xffff, 0);
    REG_PRINT(PUC_BMC_RX_PKT_enq, 0, 0xffffffff, 0);
    REG_PRINT(PUC_DQMDBG2, 0, 0xffff, 0);

    switch ( get_pon_mode() ) {
    case PON_MODE_EPON:
    case PON_MODE_XGEPON:
        BRINGUP_PRINT("EPON:\r\n");
        REG_PRINT(EPON_XGEPN_XGPCS_TX_GOODPKT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_XGEPN_XGPCS_RX_GOODPKT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_EPON_GLB_PDC_PKT_CNT, 0, 0xffffffff, 0);

        BRINGUP_PRINT("XGEPON TX:\r\n");
        REG_PRINT(EPON_EPON_GLB_PUC_PKT_CNT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_EPON_GLB_PUC_BURST_CNT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_EPON_GLB_PMAC_PKT_REQ_CNT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_EPON_GLB_DAT_TXPKT_CNTR, 0, 0xffffffff, 0);
        REG_PRINT(EPON_EPON_GLB_SW_TXPKT_CNTR, 0, 0xffff, 0);
        break;
    case PON_MODE_GPON:
        BRINGUP_PRINT("GPON Down Stream:\r\n");
        REG_PRINT(GPON_MAC_GPON_ds_omci_pkt, 0, 0xffffffff, 0);
        break;
    case PON_MODE_XGPON:
        BRINGUP_PRINT("XGPON Down Stream:\r\n");
        REG_PRINT(XGPN_DSFP_rxmac_omci_frame, 0, 0xff, 0);
        REG_PRINT(XGPN_DSFP_rxmac_pkt_frame, 0, 0xffff, 0);
        REG_PRINT(XGPN_RXCTL_discarded_frames, 0, 0xffffffff, 0);
        BRINGUP_PRINT("XGPON PUC interface:\r\n");
        REG_PRINT(XGPN_PUCIF_CNTR2, 0, 0xffffffff, 0);

        BRINGUP_PRINT("XGPON Up Stream\r\n");
        for (pid = 0; pid < 128; pid++) {
            ca_uint32_t dws[16];
            _saturn_indirect_mem_rd(XGPN_USBG_PORTID_XGEM_MIB_ACCESS, pid, 3, dws);
            frame_cnt = dws[0];
            byte_cnt = dws[2];
            byte_cnt <<= 32;
            byte_cnt |= dws[1];
            if ( frame_cnt != 0 )
                BRINGUP_PRINT("\tPORT: %d, frame_cnt = %d, byte_cnt = %lld\r\n", pid, frame_cnt, byte_cnt);
        }
        break;
    }
    return CLI_CMD_OK;
}
#endif


CLI_KERNEL_DEFINE(saturn_debug_get_drop,
                  saturn_debug_get_drop_cmd,
                  "sat get drop",
                  "Saturn debug command\n"
                  "show status\n"
                  "drop and error counters\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_uint32_t data, dws[16];

    BRINGUP_PRINT("PDC:\r\n");
    REG_PRINT(PDC_ERR_PKT_CNTR, 0, 0xffff, 0);

    BRINGUP_PRINT("PUC:\r\n");
    REG_PRINT(PUC_BMC_FORCE_DROP, 0, 0xffff, 0);
    REG_PRINT(PUC_BMC_LENGTH_ERROR, 0, 0xffff, 0);

    switch ( get_pon_mode() ) {
    case PON_MODE_EPON:
    case PON_MODE_XGEPON:
        BRINGUP_PRINT("EPON:\r\n");
        REG_PRINT(EPON_XGEPN_XMAC_CTRL_XGEPN_RX_STATUS0, 0, 0xffff, 0);
        REG_PRINT(EPON_XGEPN_XMAC_RXMAC_XGEPN_SLD_CRC8_ERRCNT, 0, 0xffff, 0);
        REG_PRINT(EPON_XGEPN_XGPCS_TX_ERRPKT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_XGEPN_XGPCS_RX_ERRPKT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_XGEPN_XGPCS_RX_FECTOTALCODE, 0, 0xffffffff, 0);
        REG_PRINT(EPON_XGEPN_XGPCS_RX_FECCORCODE, 0, 0xffffffff, 0);
        REG_PRINT(EPON_XGEPN_XGPCS_RX_FECUNCORCODE, 0, 0xffffffff, 0);
        REG_PRINT(EPON_XGEPN_XGPCS_RX_FECCORRBYTE, 0, 0xffffffff, 0);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_PKTS_UN_REG_LLID_CNT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_XGEPN_CORESEC10_TOP_CORESEC10_RX_RUNT_PKTS_CNT, 0, 0xffffffff, 0);
        REG_PRINT(EPON_EPON_MIB_MIB_TX_DROPCNT, 0, 0xffff, 0);
        REG_PRINT(EPON_EPON_MIB_MIB_RX_DROPCNT, 0, 0xffff, 0);
        REG_PRINT(EPON_XGEPN_XMAC_RXMAC_XGEPN_SLD_CRC8_ERRCNT, 0, 0xffff, 0);
        //REG_PRINT(EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_ACCESS, 0, 0xffff,0);
        _saturn_indirect_mem_rd(EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_ACCESS, 0x4003, 1, dws);
        BRINGUP_PRINT("EPON_BCCRC8VLDFCSERRCNT: %d\r\n", dws[0]);

        _saturn_indirect_mem_rd(EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC_ACCESS, 0x2, 1, dws);
        BRINGUP_PRINT("CRCERRCNT: %d\r\n", dws[0]);
        _saturn_indirect_mem_rd(EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC_ACCESS, 0x8, 1, dws);
        BRINGUP_PRINT("UNKOWNOPCNT: %d\r\n", dws[0]);
        _saturn_indirect_mem_rd(EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC_ACCESS, 0x9, 1, dws);
        BRINGUP_PRINT("UNDERSIZECNT: %d\r\n", dws[0]);
        _saturn_indirect_mem_rd(EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC_ACCESS, 0xa, 1, dws);
        BRINGUP_PRINT("OVERSIZECNT: %d\r\n", dws[0]);

        REG_PRINT(EPON_XGEPN_XMAC_CTRL_XGEPN_RX_STATUS0, 0, 0xffff, 0);
        REG_PRINT(EPON_XGEPN_XGPCS_RX_ERRDEC, 0, 0xffff, 0);
        break;
    case PON_MODE_GPON:
        BRINGUP_PRINT("GPON:\r\n");
        REG_PRINT(GPON_MAC_GPON_bip_error_accum, 0, 0xffffffff, 0);
        REG_PRINT(GPON_MAC_FEC_FEC_Block_Correctable_Error_Counter, 0, 0xffffffff, 0);
        REG_PRINT(GPON_MAC_FEC_FEC_Block_Uncorrectable_Error_Counter, 0, 0xffffffff, 0);
        REG_PRINT(GPON_MAC_GPON_bwmap_corrected_biterr, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_bwmap_uncorrected_biterr, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_gem_frag_drop, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_gem_one_biterr, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_gem_two_biterr, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_gem_uncorrected_biterr, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_bwmap_drop, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_omci_crc, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_plend_err, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_plend_biterr, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_ds_asmbl_fifo_drop_cnt, 0, 0xffff, 0);
        REG_PRINT(GPON_MAC_GPON_ds_pkt_crc, 0, 0xffffffff, 0);
        REG_PRINT(GPON_MAC_GPON_ds_udrsz_pkt, 0, 0xffffffff, 0);
        REG_PRINT(GPON_MAC_GPON_ds_ovrsz_pkt, 0, 0xffffffff, 0);
        break;
    case PON_MODE_XGPON:
        BRINGUP_PRINT("XGPON:\r\n");
        REG_PRINT(XGPN_DSFP_bip_32_err, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_DSFP_sfc_hec_corrected, 0, 0xffff, 0);
        REG_PRINT(XGPN_DSFP_sfc_hec_uncorrected_mib, 0, 0xffff, 0);
        REG_PRINT(XGPN_DSFP_ponid_hec_corrected, 0, 0xffff, 0);
        REG_PRINT(XGPN_DSFP_ponid_hec_uncorrected_mib, 0, 0xffff, 0);
        REG_PRINT(XGPN_DSFP_psbd_hec_uncorrected_mib, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_DSFP_psbd_hec_err, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_DSFP_xgtc_hec_uncorrected_mib, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_DSFP_xgtc_hec_corrected, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_XGFEC_FECUNCORCODE, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_XGFEC_FECCORRCODE, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_DSFP_xgem_hec_uncorrected_mib, 0, 0xffff, 0);
        REG_PRINT(XGPN_DSFP_xgem_hec_corrected, 0, 0xffff, 0);
        REG_PRINT(XGPN_DSFP_xgem_err_key_mib, 0, 0xffff, 0);
        REG_PRINT(XGPN_DSFP_rxmac_pkt_crc_err, 0, 0xff, 0);
        REG_PRINT(XGPN_RXCTL_discarded_frames, 0, 0xffffffff, 0);

        REG_PRINT(XGPN_BWMP_UNK_PF_MIB, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_BWMP_AS_HEC_ERR_CNT, 0, 0xffff, 0);
        REG_PRINT(XGPN_BWMP_LMT_DROP_SRC_STS, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_PLMP_MIC_ERR_MIB, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_PLMP_DS_DROP_CNT, 0, 0xffffffff, 0);
        REG_PRINT(XGPN_PLMP_RX_FF_DROP_CNT, 0, 0xffff, 0);
        REG_PRINT(XGPN_PLMP_EXTEN_DROP_CNT, 0, 0xffff, 0);
        break;
    }
    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_get_puc_pvtbl,
                  saturn_debug_get_puc_pvtbl_cmd,
                  "sat get puc pvtbl [<0-31>]",
                  "Saturn debug command\n"
                  "show status\n"
                  "PUC\n"
                  "PVTBL configuration\n"
                  "LLID/TCONT\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_uint32_t dws[16];
    ca_uint32_t value[16];
    ca_int32_t tbl_idx;
    ca_int32_t i, j;
    saturn_debug_tbl_t *tbl;
    saturn_debug_bf_t *bf;
    ca_int8_t str[132];
    ca_int32_t pid = -1;

    if ( argc > 0 ) {
        pid = ca_strtol((ca_int8 *)argv[0]);
    }

    tbl_idx = _saturn_debug_find_tbl_spec(PUC_PVTBL_ACCESS);
    tbl = &SATURN_DEBUG_TBL[tbl_idx];

    if ( pid != -1 ) {
        BRINGUP_PRINT("%08x: 0x", pid);
        _saturn_indirect_mem_rd(PUC_PVTBL_ACCESS, pid, tbl->width, dws);
        for (j = 0; j < tbl->width; j++)
        {
            BRINGUP_PRINT("%08x", dws[tbl->width - j - 1]);
        }
        BRINGUP_PRINT("\r\n");
        for (i = 0; i < tbl->num; i++ ) {
            bf = &tbl->bf[i];
            field_get(dws, value, bf->offset, bf->length);
            BRINGUP_PRINT("%30s : %s\r\n", bf->name, field_sprintf(str, value, bf->length));
        }
    } else {
        for (pid = 0; pid < 32; pid++)
        {
            BRINGUP_PRINT("%08x: 0x", pid);
            _saturn_indirect_mem_rd(PUC_PVTBL_ACCESS, pid, tbl->width, dws);
            for (j = 0; j < tbl->width; j++)
            {
                BRINGUP_PRINT("%08x", dws[tbl->width - j - 1]);
            }
            BRINGUP_PRINT("\r\n");
        }
    }
    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_get_puc_qstat,
                  saturn_debug_get_puc_qstat_cmd,
                  "sat get puc qstat",
                  "Saturn debug command\n"
                  "show status\n"
                  "PUC\n"
                  "VoQ Status\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_uint32_t dws[16];
    PUC_TQMEM_DATA0_t tqmem_data0;
    PUC_TQMEM_DATA1_t tqmem_data1;
    PUC_BCMEM_DATA0_t bcmem_data0;
    PUC_BCMEM_DATA1_t bcmem_data1;
    ca_uint32_t bufcnt;
    ca_uint32_t pktcnt;
    ca_uint32_t bytecnt;
    ca_int32_t i;

    BRINGUP_PRINT("PUC_TQMEM:\r\n");
    for (i = 0; i < 128; i++) {
        _saturn_indirect_mem_rd(PUC_TQMEM_ACCESS, i, 2, dws);
        tqmem_data0.wrd = dws[0];
        tqmem_data1.wrd = dws[1];

        bytecnt = tqmem_data0.bf.bytecnt;
        pktcnt = ( tqmem_data1.bf.pktcnt << 11 ) | tqmem_data0.bf.pktcnt;
        if ( bytecnt != 0 || pktcnt != 0 ) {
            BRINGUP_PRINT("\tVoQ id: %d, bytecnt = %d, pktcnt = %d\r\n", i, bytecnt, pktcnt);
        }
    }
    BRINGUP_PRINT("PUC_BCMEM:\r\n");
    for (i = 0; i < 128; i++) {
        _saturn_indirect_mem_rd(PUC_BCMEM_ACCESS, i, 2, dws);
        bcmem_data0.wrd = dws[0];
        bcmem_data1.wrd = dws[1];

        bytecnt = bcmem_data0.bf.bytecnt;
        bufcnt = ( bcmem_data1.bf.bufcnt << 11 ) | bcmem_data0.bf.bufcnt;
        if ( bytecnt != 0 || bufcnt != 0 ) {
            BRINGUP_PRINT("\tVoQ id: %d, bytecnt = %d, bufcnt = %d\r\n", i, bytecnt, bufcnt);
        }
    }
    return CLI_CMD_OK;
}
#endif

CLI_KERNEL_DEFINE(saturn_debug_linklist_check,
                  saturn_debug_linklist_check_cmd,
                  "sat linklist",
                  "Saturn debug command\n"
                  "PUC Linklist checking\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_int32_t fb_count;
    ca_int32_t fb_ptr;
    ca_int32_t i;
    ca_int8_t *used;
    PUC_BMDBGFBPA_t bmdbgfbpa;
    PUC_BMDBGFBPB_t bmdbgfbpb;

    used = ca_malloc(0x4000 * sizeof( ca_int8_t ));

    BRINGUP_PRINT("PUC LinkList Checking ... \r\n");
    BRINGUP_PRINT("\tBMDBGFBPA: \r\n");

    memset(used, 0, 0x4000 * sizeof( ca_int8_t ));
    bmdbgfbpa.wrd = CA_REG_READ((void *) PUC_BMDBGFBPA);
    fb_count = bmdbgfbpa.bf.fbcnta;
    fb_ptr = bmdbgfbpa.bf.fbphptra;
    BRINGUP_PRINT("\tFree Buffer Count A: %d\r\n", fb_count);
    BRINGUP_PRINT("\tFree Buffer PTR A: %d\r\n", fb_ptr);

    for ( i = 0; i < fb_count; i++ ) {
        if (fb_ptr >= 0x4000) {
            BRINGUP_PRINT("\tERROR: this pointer is out of range: %d\r\n", fb_ptr);
        } else {
            if ( used[fb_ptr] == 1 ) {
                BRINGUP_PRINT("\tERROR: this pointer has been used: %d\r\n", fb_ptr);
            } else {
                used[fb_ptr] = 1;
            }
        }
        fb_ptr = bpmem_read(fb_ptr);
    }
    BRINGUP_PRINT("\ti=%d ptr: %d\r\n", i, fb_ptr);

    BRINGUP_PRINT("\tBMDBGFBPB: \r\n");
    memset(used, 0, 0x4000 * sizeof( ca_int8_t ));
    bmdbgfbpb.wrd = CA_REG_READ((void *) PUC_BMDBGFBPB);
    fb_count = bmdbgfbpb.bf.fbcntb;
    fb_ptr = bmdbgfbpb.bf.fbphptrb;

    BRINGUP_PRINT("\tFree Buffer Count B: %d\r\n", fb_count);
    BRINGUP_PRINT("\tFree Buffer PTR B: %d\r\n", fb_ptr);
    for ( i = 0; i < fb_count; i++ ) {
        if (fb_ptr < 0x4000 || fb_ptr >= 0x8000) {
            BRINGUP_PRINT("\tERROR: this pointer is out of range: %d\r\n", fb_ptr);
        } else {
            if ( used[fb_ptr - 0x4000] == 1 ) {
                BRINGUP_PRINT("\tERROR: this pointer has been used: %d\r\n", fb_ptr);
            } else {
                used[fb_ptr - 0x4000] = 1;
            }
        }
        fb_ptr = bpmem_read(fb_ptr);
    }
    return CLI_CMD_OK;
}
#endif


CLI_KERNEL_DEFINE(saturn_debug_get_control,
                  saturn_debug_get_control_cmd,
                  "sat get control",
                  "Saturn debug command\n"
                  "show status\n"
                  "main control registers\n"
                 )
#ifdef CLI_KERNEL_SPACE
{
    ca_int32_t i;
    ca_uint32_t dws[16];

    EPON_EPON_GLB_CORESEC10_CFG_t glb_coresec10_cfg;
    EPON_EPON_GLB_XGPCS_FEC_CFG_t glb_xgpcs_fec_cfg;

    GPON_MAC_GPON_DS_GEM_PORT_DATA_t ds_gem_port;
    GPON_MAC_GPON_US_PORT_ID_DATA_t us_port_id;
    GPON_MAC_GPON_onu_ctl_t onu_ctl;
    GPON_MAC_GPON_onu_cfg_t onu_cfg;

    XGPN_DSFP_OC_body1_t oc_body1;
    XGPN_USBG_BURST_PF0_INFOR_STS_t burst_pf_infor_sts;
    XGPN_DSFP_portid0_ctl_xgem_t portid_ctl_xgem;
    XGPN_USBG_XGEM_PID_TBL_DATA_t xgem_pid_tbl;
    XGPN_BWMP_ALLOC_LUT_DATA_t alloc_lut;

    switch ( get_pon_mode() ) {
    case PON_MODE_EPON:
    case PON_MODE_XGEPON:
        glb_coresec10_cfg.wrd = CA_REG_READ((void *)EPON_EPON_GLB_CORESEC10_CFG);
        BRINGUP_PRINT("\tEPON_EPON_GLB_CORESEC10_CFG: 0x%0x\r\n", glb_coresec10_cfg.wrd);
        REG_FIELD_PRINT(glb_coresec10_cfg, algo_sel);
        glb_xgpcs_fec_cfg.wrd = CA_REG_READ((void *)EPON_EPON_GLB_XGPCS_FEC_CFG);
        BRINGUP_PRINT("\tEPON_EPON_GLB_XGPCS_FEC_CFG: 0x%0x\r\n", glb_xgpcs_fec_cfg.wrd);
        REG_FIELD_PRINT(glb_xgpcs_fec_cfg, rx_mode);
        REG_FIELD_PRINT(glb_xgpcs_fec_cfg, tx_off);
        break;
    case PON_MODE_GPON:
        BRINGUP_PRINT("GPON:\r\n");
        for (i = 0; i < 0x1000; i++) {
            _saturn_indirect_mem_rd(GPON_MAC_GPON_DS_GEM_PORT_ACCESS, i, 1, dws);
            ds_gem_port.wrd = dws[0];
            if ( ds_gem_port.bf.vld )
                BRINGUP_PRINT("\t\tid: %d, vld=%d, aes=%d, tdm=%d, index=%d\r\n", i, ds_gem_port.bf.vld, ds_gem_port.bf.aes, ds_gem_port.bf.tdm, ds_gem_port.bf.index);
        }

        for (i = 0; i < 64; i++) {
            _saturn_indirect_mem_rd(GPON_MAC_GPON_US_PORT_ID_ACCESS, i, 1, dws);
            us_port_id.wrd = dws[0];
            BRINGUP_PRINT("\t\tindex: %d, id=%d\r\n", i, us_port_id.bf.id);
        }
        onu_ctl.wrd = CA_REG_READ((void *)GPON_MAC_GPON_onu_ctl);
        BRINGUP_PRINT("GPON_MAC_GPON_onu_ctl: 0x%0x\r\n", onu_ctl.wrd);
        REG_FIELD_PRINT(onu_ctl, no_up        );
        REG_FIELD_PRINT(onu_ctl, en           );
        REG_FIELD_PRINT(onu_ctl, stop         );
        REG_FIELD_PRINT(onu_ctl, omcc_padding );
        REG_FIELD_PRINT(onu_ctl, dpath_rst    );
        REG_FIELD_PRINT(onu_ctl, aes_en       );
        REG_FIELD_PRINT(onu_ctl, ploam_crc_en );
        REG_FIELD_PRINT(onu_ctl, rsrvd1       );
        REG_FIELD_PRINT(onu_ctl, pm_mode      );
        REG_FIELD_PRINT(onu_ctl, plen_match   );
        REG_FIELD_PRINT(onu_ctl, stop_us_wr   );
        REG_FIELD_PRINT(onu_ctl, stop_hw_plu  );
        REG_FIELD_PRINT(onu_ctl, sleep_mode   );
        REG_FIELD_PRINT(onu_ctl, rsrvd2       );
        REG_FIELD_PRINT(onu_ctl, sch_lead_time);

        onu_cfg.wrd = CA_REG_READ((void *)GPON_MAC_GPON_onu_cfg);
        BRINGUP_PRINT("GPON_MAC_GPON_onu_cfg: 0x%0x\r\n", onu_cfg.wrd);
        REG_FIELD_PRINT(onu_cfg, omci_crc_dis  );
        REG_FIELD_PRINT(onu_cfg, us_scramb_dis );
        REG_FIELD_PRINT(onu_cfg, ds_scramb_dis );
        REG_FIELD_PRINT(onu_cfg, dbru_blk_size );
        REG_FIELD_PRINT(onu_cfg, laser_pre_bias);
        REG_FIELD_PRINT(onu_cfg, laser_on      );
        REG_FIELD_PRINT(onu_cfg, plmd_ff_wr_ext);
        REG_FIELD_PRINT(onu_cfg, laser_on_align);
        break;
    case PON_MODE_XGPON:
        BRINGUP_PRINT("XGPON:\r\n");
        oc_body1.wrd = CA_REG_READ((void *) XGPN_DSFP_OC_body1);
        BRINGUP_PRINT("\tXGPN_DSFP_OC_body1: 0x%0x\r\n", oc_body1.wrd);
        REG_FIELD_PRINT(oc_body1, TOL1        );
        REG_FIELD_PRINT(oc_body1, C_flag1     );
        REG_FIELD_PRINT(oc_body1, link_type1  );
        REG_FIELD_PRINT(oc_body1, P_flag1     );
        REG_FIELD_PRINT(oc_body1, DS_FEC_flag1);
        REG_FIELD_PRINT(oc_body1, ODN_class1  );
        REG_FIELD_PRINT(oc_body1, RE_flag1    );

        burst_pf_infor_sts.wrd = CA_REG_READ((void *) XGPN_USBG_BURST_PF0_INFOR_STS);
        if ( burst_pf_infor_sts.bf.vld ) {
            BRINGUP_PRINT("\tXGPN_USBG_BURST_PF0_INFOR_STS: 0x%0x\r\n", burst_pf_infor_sts.wrd);
            REG_FIELD_PRINT(burst_pf_infor_sts, pcnt  );
            REG_FIELD_PRINT(burst_pf_infor_sts, plen  );
            REG_FIELD_PRINT(burst_pf_infor_sts, dlen  );
            REG_FIELD_PRINT(burst_pf_infor_sts, oh_len);
            REG_FIELD_PRINT(burst_pf_infor_sts, fecu  );
            REG_FIELD_PRINT(burst_pf_infor_sts, ver   );
            REG_FIELD_PRINT(burst_pf_infor_sts, vld   );
        }
        burst_pf_infor_sts.wrd = CA_REG_READ((void *) XGPN_USBG_BURST_PF1_INFOR_STS);
        if ( burst_pf_infor_sts.bf.vld ) {
            BRINGUP_PRINT("\tXGPN_USBG_BURST_PF1_INFOR_STS: 0x%0x\r\n", burst_pf_infor_sts.wrd);
            REG_FIELD_PRINT(burst_pf_infor_sts, pcnt  );
            REG_FIELD_PRINT(burst_pf_infor_sts, plen  );
            REG_FIELD_PRINT(burst_pf_infor_sts, dlen  );
            REG_FIELD_PRINT(burst_pf_infor_sts, oh_len);
            REG_FIELD_PRINT(burst_pf_infor_sts, fecu  );
            REG_FIELD_PRINT(burst_pf_infor_sts, ver   );
            REG_FIELD_PRINT(burst_pf_infor_sts, vld   );
        }
        burst_pf_infor_sts.wrd = CA_REG_READ((void *) XGPN_USBG_BURST_PF2_INFOR_STS);
        if ( burst_pf_infor_sts.bf.vld ) {
            BRINGUP_PRINT("\tXGPN_USBG_BURST_PF2_INFOR_STS: 0x%0x\r\n", burst_pf_infor_sts.wrd);
            REG_FIELD_PRINT(burst_pf_infor_sts, pcnt  );
            REG_FIELD_PRINT(burst_pf_infor_sts, plen  );
            REG_FIELD_PRINT(burst_pf_infor_sts, dlen  );
            REG_FIELD_PRINT(burst_pf_infor_sts, oh_len);
            REG_FIELD_PRINT(burst_pf_infor_sts, fecu  );
            REG_FIELD_PRINT(burst_pf_infor_sts, ver   );
            REG_FIELD_PRINT(burst_pf_infor_sts, vld   );
        }
        burst_pf_infor_sts.wrd = CA_REG_READ((void *) XGPN_USBG_BURST_PF3_INFOR_STS);
        if ( burst_pf_infor_sts.bf.vld ) {
            BRINGUP_PRINT("\tXGPN_USBG_BURST_PF3_INFOR_STS: 0x%0x\r\n", burst_pf_infor_sts.wrd);
            REG_FIELD_PRINT(burst_pf_infor_sts, pcnt  );
            REG_FIELD_PRINT(burst_pf_infor_sts, plen  );
            REG_FIELD_PRINT(burst_pf_infor_sts, dlen  );
            REG_FIELD_PRINT(burst_pf_infor_sts, oh_len);
            REG_FIELD_PRINT(burst_pf_infor_sts, fecu  );
            REG_FIELD_PRINT(burst_pf_infor_sts, ver   );
            REG_FIELD_PRINT(burst_pf_infor_sts, vld   );
        }

        for (i = 0; i < 128; i++) {
            portid_ctl_xgem.wrd = CA_REG_READ((void *) ( XGPN_DSFP_portid0_ctl_xgem + 4 * i ) );
            if ( portid_ctl_xgem.bf.portid_vld )
                BRINGUP_PRINT("\tXGPN_DSFP_portid%d_ctl_xgem: portid=%d, mc=%d, vld=%d\r\n", i, portid_ctl_xgem.bf.portid, portid_ctl_xgem.bf.portid_mc, portid_ctl_xgem.bf.portid_vld);
        }

        BRINGUP_PRINT("\tXGPN_USBG_XGEM_PID_TBL:\r\n");
        for (i = 0; i < 128; i++) {
            _saturn_indirect_mem_rd(XGPN_USBG_XGEM_PID_TBL_ACCESS, i, 1, dws);
            xgem_pid_tbl.wrd = dws[0];
            if ( xgem_pid_tbl.bf.port_en )
                BRINGUP_PRINT("\tidx: %d, XGEM_PortID=%d,port_en=%d,aes_mode=%d,MC_flag=%d\r\n", i, xgem_pid_tbl.bf.XGEM_PortID, xgem_pid_tbl.bf.port_en, xgem_pid_tbl.bf.aes_mode, xgem_pid_tbl.bf.MC_flag);
        }

        BRINGUP_PRINT("\tXGPN_BWMP_ALLOC_LUT:\r\n");
        for (i = 0; i < 32; i++) {
            _saturn_indirect_mem_rd(XGPN_BWMP_ALLOC_LUT_ACCESS, i, 1, dws);
            alloc_lut.wrd = dws[0];
            BRINGUP_PRINT("\ttcontID: %d, allocation_ID=%d, omci_en=%d, ploam_en=%d\r\n", i, alloc_lut.bf.allocation_ID, alloc_lut.bf.omci_en, alloc_lut.bf.ploam_en);
        }

        break;
    }

    return CLI_CMD_OK;
}
#endif

void cli_saturn_debug_cmd_reg(void)
{
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_rd_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_wr_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_mem_read_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_mem_write_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_reset_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_config_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_status_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_mib_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_cnt_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_drop_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_linklist_check_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_control_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_puc_qstat_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&saturn_debug_get_puc_pvtbl_cmd);
}
#endif



void cli_aal_init(void)
{
#ifndef CLI_KERNEL_SPACE
    cli_install_cmd(CLI_NODE_ID_CONFIG, &enter_aal_node_cmd);
#endif

#ifdef CONFIG_RTK_MIPS
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&register_kern_read_cmd);
    CLI_KERNEL_GLOBAL_CMD_INSTALL(&register_kern_write_cmd);
    cli_saturn_debug_cmd_reg();
#else
#ifndef CLI_KERNEL_SPACE
    cli_install_global_cmd(&register_user_read_cmd);
    cli_install_global_cmd(&register_user_write_cmd);
#ifdef CONFIG_LUNA_G3_SERIES
    cli_install_global_cmd(&register_user_field_cmd);
#endif
#endif
#endif

    /*__AAL_CLI_CODE_GEN__*/


    return;
}



/*lint -restore*/
