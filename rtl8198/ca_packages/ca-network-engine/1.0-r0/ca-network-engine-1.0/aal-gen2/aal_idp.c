/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * File   : aal_idp.c
 * Brief  : Saturn FPGA internal datapath AAL driver
 * History:
 *          2016-07-19 initial @Lidong
 *
 */

#if defined(CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN) && defined(CONFIG_CORTINA_BOARD_FPGA)

#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_pon.h"
#include "aal_idp.h"
#include "scfg.h"
#include "aal_xgpon.h"


/*************************************************************************
 * IDP Downstream Data-path
 * --------------------------------------------------------------------
 * PON downstream controller module sends packet with header.
 * The packets can go to XGE port, local CPU packet memory or
 * remote CPU RGMII port.
 **************************************************************************
 */

/**
 * @aal_idp_dns_dpid_stats_get
 * Get Per LDPID packet and byte counter s.
 */
ca_status_t aal_idp_dns_dpid_stats_get(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_uint8_t                 dpid,
    CA_OUT aal_idp_dns_dpid_stats_t  *stats)
{
    FPGA_IDP_DOWN_COUNTERS_DATA1_t  reg_data1;
    FPGA_IDP_DOWN_COUNTERS_DATA0_t  reg_data0;

    CA_ASSERT_RET(stats, CA_E_PARAM);
    CA_ASSERT_RET(dpid < AAL_IDP_DPID_NUM, CA_E_PARAM);


    AAL_PON_REG_INDIRECT_READ(dpid, FPGA_IDP_DOWN_COUNTERS_ACCESS);

    reg_data1.wrd = AAL_PON_REG_READ(FPGA_IDP_DOWN_COUNTERS_DATA1);
    reg_data0.wrd = AAL_PON_REG_READ(FPGA_IDP_DOWN_COUNTERS_DATA0);

    stats->pkt_cnt  = reg_data1.bf.pkt_count;
    stats->byte_cnt = reg_data0.bf.byt_count;

    return CA_E_OK;
}

/**
 * @aal_idp_dns_global_stats_get
 * Get internal downstream path packet and byte counters.
 */
ca_status_t aal_idp_dns_global_stats_get(
    CA_IN  ca_device_id_t                  device_id,
    CA_OUT aal_idp_dns_global_stats_t  *stats)
{

    CA_ASSERT_RET(stats, CA_E_PARAM);

    stats->to_rmt_cpu_pkt_cnt  = AAL_PON_REG_READ(FPGA_IDP_DOWN_TO_REMOTE_CPU_PKT_COUNT );
    stats->to_rmt_cpu_byte_cnt = AAL_PON_REG_READ(FPGA_IDP_DOWN_TO_REMOTE_CPU_BYT_COUNT );
    stats->to_xge_pkt_cnt      = AAL_PON_REG_READ(FPGA_IDP_DOWN_TO_XGE_PKT_COUNT        );
    stats->to_xge_byte_cnt     = AAL_PON_REG_READ(FPGA_IDP_DOWN_TO_XGE_BYT_COUNT        );

    return CA_E_OK;
}

ca_status_t aal_idp_dns_stats_clear(CA_IN ca_device_id_t  device_id)
{
    ca_uint32_t temp = 0;

    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_DOWN_CNTL, count_clear, 1);
    temp = AAL_PON_REG_READ(FPGA_IDP_DOWN_CNTL);
    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_DOWN_CNTL, count_clear, 0);

    return CA_E_OK;
}

/**
 * @aal_idp_dns_cfg_set
 * Set IDP downstream parser configuration
 * Upon receiving the downstream packets,
 * the following will be checked to CPU RX FIFO:
 * Check Ethernet Protocol-Type (PT) field to identify for EPON
 *  OAM packet (88-09)
 *  MPCP packet (88-08)
 *  802.1x packet (88-8E)
 *  Other packets (Defined ethtype)
 *  Compare MAC-DA field with (My MAC Address) to identify
 *  My-MAC (IPv4/IPv6) Packet
 * otherwise, downstream packet goto GE port
 */
ca_status_t aal_idp_dns_cfg_set(
    CA_IN ca_device_id_t                device_id,
    CA_IN aal_idp_dns_cfg_mask_t     cfg_mask,
    CA_IN aal_idp_dns_cfg_t         *cfg)
{
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    if (cfg_mask.s.my_mac) {

        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_DOWN_MY_MAC_HI, mac_da_hi,
            (cfg->my_mac[0] << 8) + cfg->my_mac[1]);

        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_DOWN_MY_MAC_LO, mac_da_lo,
            (cfg->my_mac[2] << 24) + (cfg->my_mac[3] << 16) +
            (cfg->my_mac[4] << 8) + cfg->my_mac[5]);
    }

    if (cfg_mask.s.my_ethtype) {
        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_DOWN_MY_ETH_TYPE, eth_type, cfg->my_ethtype);
    }

    return CA_E_OK;

}


/**
 * @aal_idp_dns_cfg_get
 * Get IDP downstream parser configuration
 */
ca_status_t aal_idp_dns_cfg_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_idp_dns_cfg_t         *cfg)
{
    FPGA_IDP_DOWN_MY_MAC_HI_t   mymac_data0;
    FPGA_IDP_DOWN_MY_MAC_LO_t   mymac_data1;

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    mymac_data0.wrd = AAL_PON_REG_READ(FPGA_IDP_DOWN_MY_MAC_HI);
    mymac_data1.wrd = AAL_PON_REG_READ(FPGA_IDP_DOWN_MY_MAC_LO);

    cfg->my_mac[0] = (mymac_data0.bf.mac_da_hi >> 8) & 0xFF;
    cfg->my_mac[1] = (mymac_data0.bf.mac_da_hi) & 0xFF;
    cfg->my_mac[2] = (mymac_data1.bf.mac_da_lo >> 24) & 0xFF;
    cfg->my_mac[3] = (mymac_data1.bf.mac_da_lo >> 16) & 0xFF;
    cfg->my_mac[4] = (mymac_data1.bf.mac_da_lo >> 8) & 0xFF;
    cfg->my_mac[5] = (mymac_data1.bf.mac_da_lo) & 0xFF;

    AAL_PON_REG_FIELD_READ(FPGA_IDP_DOWN_MY_ETH_TYPE, eth_type, cfg->my_ethtype);

    return CA_E_OK;

}

ca_status_t aal_idp_dns_rgmii_path_ctrl_set
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_boolean_t snoop_ena
)
{
    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_DOWN_RGMII_PATH_CTRL, snoop_en, ((snoop_ena) ? 1 : 0));

    return CA_E_OK;
}

ca_status_t aal_idp_dns_rgmii_path_ctrl_get
(
    CA_IN  ca_device_id_t  device_id,
    CA_OUT ca_boolean_t *snoop_ena
)
{
    ca_uint32_t val32 = 0;

    CA_ASSERT_RET(snoop_ena, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(FPGA_IDP_DOWN_RGMII_PATH_CTRL, snoop_en, val32);

    *snoop_ena = (val32) ? TRUE : FALSE;

    return CA_E_OK;
}


/**
 * @aal_idp_dns_rx_pkt_get
 * Read IDP downstream Rx packet buffer
 */
ca_status_t aal_idp_dns_rx_pkt_get(
    CA_IN  ca_device_id_t         device_id,
    CA_OUT ca_uint16_t        *pkt_len,
    CA_OUT ca_uint8_t         *pkt_buf /* [*pkt_len:2048] */
    )
{
    ca_uint16_t  idx = 0, lft = 0, temp = 0;
    FPGA_IDP_DOWN_CPU_RX_PKT_DATA1_t  data32;

    CA_ASSERT_RET(pkt_len && pkt_buf, CA_E_PARAM);

    *pkt_len = AAL_PON_REG_READ(FPGA_IDP_DOWN_CPURX_PKT);

    if (*pkt_len > AAL_IDP_PKT_LEN_MAX) {
        ca_printf("Rx pkt leng is %u worong!\n", *pkt_len);
        return CA_E_RESOURCE;
    }

    temp = (*pkt_len >> 3) << 3;

    for (idx = 0; idx < temp; idx += 8) {

        data32.wrd = AAL_PON_REG_READ(FPGA_IDP_DOWN_CPU_RX_PKT_DATA1 + (idx >> 1));

        pkt_buf[idx+0] = (data32.wrd >> 24) & 0xFF;
        pkt_buf[idx+1] = (data32.wrd >> 16) & 0xFF;
        pkt_buf[idx+2] = (data32.wrd >> 8 ) & 0xFF;
        pkt_buf[idx+3] = (data32.wrd) & 0xFF;

        data32.wrd = AAL_PON_REG_READ(FPGA_IDP_DOWN_CPU_RX_PKT_DATA0 + (idx >> 1));

        pkt_buf[idx+4] = (data32.wrd >> 24) & 0xFF;
        pkt_buf[idx+5] = (data32.wrd >> 16) & 0xFF;
        pkt_buf[idx+6] = (data32.wrd >> 8 ) & 0xFF;
        pkt_buf[idx+7] = (data32.wrd) & 0xFF;
    }

    /* left data read */
    lft = (*pkt_len & 0x7);

    if (lft) {
        data32.wrd = AAL_PON_REG_READ(FPGA_IDP_DOWN_CPU_RX_PKT_DATA1 + (idx >> 1));
        for (temp = 0; (temp < 4) && (temp < lft); temp++) {
            pkt_buf[idx++] = (data32.wrd >> (24 - (temp << 3))) & 0xFF;
        }
    }

    if (lft > 4) {
        data32.wrd = AAL_PON_REG_READ(FPGA_IDP_DOWN_CPU_RX_PKT_DATA0 + (idx >> 1));
        for (temp = 0; (temp < 4) && (temp < lft); temp++) {
            pkt_buf[idx++] = (data32.wrd >> (24 - (temp << 3))) & 0xFF;
        }
    }

    /* Set to 1 for indicating Read done */
    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_DOWN_CNTL, cpu_read_done, 1);

#if 0 /* Wait for ASIC update CSR to support clear this bit */
    /* Wait ASIC clear to 0 */
    data32.wrd = 1;
    do {
        AAL_PON_REG_FIELD_READ(FPGA_IDP_DOWN_CNTL, cpu_read_done, data32.wrd);
    } while (data32.wrd);
#endif

    return CA_E_OK;

}


/**********************************************************************
 * IDP Upstream Data-path
 * --------------------------------------------------------------------
 * The upstream PUC receives data from XGE port, remote CPU RGMII port
 * and a local CPU packet buffer stores the software packets to be
 * sent to PON output datapath
 **********************************************************************
 */
ca_status_t aal_idp_ups_global_stats_get(
    CA_IN  ca_device_id_t                  device_id,
    CA_OUT aal_idp_ups_global_stats_t  *stats)
{

    CA_ASSERT_RET(stats, CA_E_PARAM);

    stats->xge_pkt_cnt      = AAL_PON_REG_READ(FPGA_IDP_UP_XGE_PKT_CNT );
    stats->rgmii_pkt_cnt    = AAL_PON_REG_READ(FPGA_IDP_UP_RGMII_PKT_CNT);
    stats->xge_drp_pkt_cnt  = AAL_PON_REG_READ(FPGA_IDP_UP_XGE_DROP_CNT);
    stats->rgmii_drp_pkt_cnt= AAL_PON_REG_READ(FPGA_IDP_UP_RGMII_DROP_CNT);
    stats->xge_voqbp_drp_pkt_cnt   = AAL_PON_REG_READ(FPGA_IDP_UP_XGE_DROP_CNT);
    stats->rgmii_voqbp_drp_pkt_cnt =  AAL_PON_REG_READ(FPGA_IDP_UP_RGMII_DROP_CNT);

    return CA_E_OK;
}

/**
 * @aal_idp_ups_cfg_set
 * Set IDP upstream configuration
 */
ca_status_t aal_idp_ups_cfg_set(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  aal_idp_ups_cfg_mask_t     mask,
    CA_IN  aal_idp_ups_cfg_t         *cfg)
{
    CA_ASSERT_RET(cfg, CA_E_PARAM);

    if (mask.s.my_mac_da) {
        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_MY_MAC_HI, mac_da_hi,
            (cfg->my_mac_da[0] << 8) + cfg->my_mac_da[1]);

        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_MY_MAC_LO, mac_da_lo,
            (cfg->my_mac_da[2] << 24) + (cfg->my_mac_da[3] << 16) +
            (cfg->my_mac_da[4] << 8) + cfg->my_mac_da[5]);
    }

    if (mask.s.my_mac_sa) {
        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_MAC_SA_HI, mac_sa_hi,
            (cfg->my_mac_sa[0] << 8) + cfg->my_mac_sa[1]);

        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_MAC_SA_LO, mac_sa_lo,
            (cfg->my_mac_sa[2] << 24) + (cfg->my_mac_sa[3] << 16) +
            (cfg->my_mac_sa[4] << 8) + cfg->my_mac_sa[5]);
    }

    if (mask.s.pkt_ethtype) {
        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_MY_PKT_TP, pt, cfg->pkt_ethtype);
    }

    if (mask.s.my_llid) {
        AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_MY_LLID, llid, cfg->my_llid);
    }

    return CA_E_OK;

}


/**
 * @aal_idp_ups_cfg_get
 * Get IDP upstream configuration
 */
ca_status_t aal_idp_ups_cfg_get(
    CA_IN  ca_device_id_t                device_id,
    CA_OUT aal_idp_ups_cfg_t         *cfg)
{
    FPGA_IDP_UP_MY_MAC_HI_t   mymac_da_data0;
    FPGA_IDP_UP_MY_MAC_LO_t   mymac_da_data1;
    FPGA_IDP_UP_MAC_SA_HI_t   mymac_sa_data0;
    FPGA_IDP_UP_MAC_SA_LO_t   mymac_sa_data1;

    CA_ASSERT_RET(cfg, CA_E_PARAM);

    mymac_da_data0.wrd = AAL_PON_REG_READ(FPGA_IDP_UP_MY_MAC_HI);
    mymac_da_data1.wrd = AAL_PON_REG_READ(FPGA_IDP_UP_MY_MAC_LO);

    cfg->my_mac_da[0] = (mymac_da_data0.bf.mac_da_hi >> 8) & 0xFF;
    cfg->my_mac_da[1] = (mymac_da_data0.bf.mac_da_hi) & 0xFF;
    cfg->my_mac_da[2] = (mymac_da_data1.bf.mac_da_lo >> 24) & 0xFF;
    cfg->my_mac_da[3] = (mymac_da_data1.bf.mac_da_lo >> 16) & 0xFF;
    cfg->my_mac_da[4] = (mymac_da_data1.bf.mac_da_lo >> 8) & 0xFF;
    cfg->my_mac_da[5] = (mymac_da_data1.bf.mac_da_lo) & 0xFF;

    mymac_sa_data0.wrd = AAL_PON_REG_READ(FPGA_IDP_UP_MAC_SA_HI);
    mymac_sa_data1.wrd = AAL_PON_REG_READ(FPGA_IDP_UP_MAC_SA_LO);

    cfg->my_mac_sa[0] = (mymac_sa_data0.bf.mac_sa_hi >> 8) & 0xFF;
    cfg->my_mac_sa[1] = (mymac_sa_data0.bf.mac_sa_hi) & 0xFF;
    cfg->my_mac_sa[2] = (mymac_sa_data1.bf.mac_sa_lo >> 24) & 0xFF;
    cfg->my_mac_sa[3] = (mymac_sa_data1.bf.mac_sa_lo >> 16) & 0xFF;
    cfg->my_mac_sa[4] = (mymac_sa_data1.bf.mac_sa_lo >> 8) & 0xFF;
    cfg->my_mac_sa[5] = (mymac_sa_data1.bf.mac_sa_lo) & 0xFF;

    AAL_PON_REG_FIELD_READ(FPGA_IDP_UP_MY_PKT_TP, pt, cfg->pkt_ethtype);

    AAL_PON_REG_FIELD_READ(FPGA_IDP_UP_MY_LLID, llid, cfg->my_llid);

    return CA_E_OK;

}

/*
There are couple key points in operating the local CPU buffer.
1.    It is required for local CPU packet inside the packet buffer to include header-A with proper size, ldpid, cos, policer ID and packet type in their respective header A field.
2.    The size of the packet buffer is 2KB, it is a direct indexed memory in the IDP_UP register.
3.    The packet size including header-A should be written to the CPUTX_PKT register
4.    After the above 3 is done, a write to "CPU_WRITE_DONE" in CPU_CNTL register will trigger the request to send the packets to PUC.

*/
ca_status_t aal_idp_ups_local_cpu_pkt_tx(
    CA_IN ca_device_id_t         device_id,
    CA_IN aal_idp_hdr_a_t    *pkt_hdr,
    CA_IN ca_uint16_t         pkt_len,
    CA_IN ca_uint8_t         *pkt_buf /* [pkt_len:2048] */
    )
{
    ca_uint16_t  idx = 0, lft = 0, temp = 0;
    FPGA_IDP_UP_CPU_TX_PKT_DATA1_t  data1;
    FPGA_IDP_UP_CPU_TX_PKT_DATA0_t  data0;



    CA_ASSERT_RET(pkt_hdr && pkt_buf, CA_E_PARAM);

    /* Step 1. Fill the header A */
    data1.wrd = ((ca_uint32_t)pkt_hdr->cpu_flg << 31) |
                ((ca_uint32_t)pkt_hdr->deep_q << 30) |
                ((ca_uint32_t)pkt_hdr->pol_grp_id << 27) |
                ((ca_uint32_t)pkt_hdr->pol_id << 18) |
                ((ca_uint32_t)pkt_hdr->pol_en << 16) |
                ((ca_uint32_t)pkt_hdr->mark << 15) |
                ((ca_uint32_t)pkt_hdr->mirror << 14) |
                ((ca_uint32_t)pkt_hdr->no_drop << 13) |
                ((ca_uint32_t)pkt_hdr->pkt_type << 11) |
                ((ca_uint32_t)pkt_hdr->drop_code << 8) |
                ((ca_uint32_t)pkt_hdr->mcgid & 0xFF);

    data0.wrd = ((ca_uint32_t)pkt_hdr->hdr_type << 18) |
                ((ca_uint32_t)pkt_hdr->fe_bypass << 16) |
                ((ca_uint32_t)pkt_hdr->pkt_size << 15) |
                ((ca_uint32_t)pkt_hdr->lspid << 9) |
                ((ca_uint32_t)pkt_hdr->ldpid << 3) |
                ((ca_uint32_t)pkt_hdr->cos & 0x7);
    // TODO: set the hdr use DATA or HDR-A CSR???
    //FPGA_IDP_UP_PKT_HDR_A_HI
    //FPGA_IDP_UP_PKT_HDR_A_LO
    AAL_PON_REG_WRITE(data1.wrd, FPGA_IDP_UP_CPU_TX_PKT_DATA1);
    AAL_PON_REG_WRITE(data0.wrd, FPGA_IDP_UP_CPU_TX_PKT_DATA0);

    /* Step 2. Set pkt content */
    lft = pkt_len & 0x7;
    temp = (pkt_len >> 3) << 3;

    while(idx < temp) {
        data1.wrd  = pkt_buf[idx ++] << 24;
        data1.wrd |= pkt_buf[idx ++] << 16;
        data1.wrd |= pkt_buf[idx ++] << 8;
        data1.wrd |= pkt_buf[idx ++] << 0;

        data0.wrd  = pkt_buf[idx ++] << 24;
        data0.wrd |= pkt_buf[idx ++] << 16;
        data0.wrd |= pkt_buf[idx ++] << 8;
        data0.wrd |= pkt_buf[idx ++] << 0;

        AAL_PON_REG_WRITE(data1.wrd, FPGA_IDP_UP_CPU_TX_PKT_DATA1 + (idx >> 1));
        AAL_PON_REG_WRITE(data0.wrd, FPGA_IDP_UP_CPU_TX_PKT_DATA0 + (idx >> 1));
    }

    if (lft) {
        data1.wrd = 0;
        for (temp = 0; (temp < lft) && (temp < 4); temp++) {
            data1.wrd  |= pkt_buf[idx ++] << (24 - (temp << 3));
        }

        AAL_PON_REG_WRITE(data1.wrd, FPGA_IDP_UP_CPU_TX_PKT_DATA1 + (idx >> 1) + ((lft > 4) ? 0 : 1));

        if (lft > 4) {
            lft -= 4;
            data0.wrd = 0;
            for (temp = 0; temp < lft; temp++) {
                data0.wrd  |= pkt_buf[idx ++] << (24 - (temp << 3));
            }
            AAL_PON_REG_WRITE(data0.wrd, FPGA_IDP_UP_CPU_TX_PKT_DATA0 + (idx >> 1) + 1);
        }
    }

    /* Step 3. Set pkt size */
    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_CPUTX_PKT, pkt_size, pkt_len + AAL_IDP_HDR_A_LEN);

    /* Step 4. Set to 1 for indicating Write done for ASIC sending */
    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_CPU_CNTL, cpu_write_done, 1);

    return CA_E_OK;

}


/**
 * @aal_idp_ups_xge_gen_pkt
 * 16 different types of packets can be stored in the packet generation memory.  In the IDP_UP register definition, there are couple other registers to define the packet generation feature:
 * 1.    pkt_rpt_cnt : this register specified the repeat counter for the same packet.  The default is 0, which means one copy.
 * 2.    pkt_vld[15:0]: packet valid.   When set to 0, the corresponding packet stored in the memory will not be sent to PUC
 * 3.    loop_cnt[15:0]: loop counter, specified how many loops (from pkt0 to pkt15) will be generated
 * 4.    PKT_GEN_CNTL_enable: writing this bit to one will cause the packet gen to start.  Packet gen can not be stopped until the loop_cnt is satisfied.
 * By default, 16 packets are stored in the FPGA memories with the following size and DPID
 *
 */
ca_status_t aal_idp_ups_xge_pkt_generate(
    CA_IN ca_device_id_t         device_id,
    CA_IN ca_uint16_t         pkt_vld_bmp,
    CA_IN ca_uint16_t         pkt_gen_loop,
    CA_IN ca_uint8_t          pkt_count_per_type
    )
{
    ca_uint16_t  idx = 0;
    FPGA_IDP_UP_PKT_RPT_CNT0_t  pkt_rpt_cnt;


    CA_ASSERT_RET(pkt_vld_bmp, CA_E_PARAM);


    pkt_rpt_cnt.wrd = ((ca_uint32_t)pkt_count_per_type << 24) |
                      (pkt_count_per_type << 16) |
                      (pkt_count_per_type << 8)  |
                      (pkt_count_per_type << 0);

    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_PKT_GEN_CNTL, enable, 0);

    for (idx = 0; idx < FPGA_IDP_UP_PKT_RPT_CNT0_COUNT; idx ++) {
        AAL_PON_REG_WRITE(pkt_rpt_cnt.wrd,
            FPGA_IDP_UP_PKT_RPT_CNT0 + FPGA_IDP_UP_PKT_RPT_CNT0_STRIDE * idx);
    }

    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_PKT_GEN_VLD, pkt_vld, pkt_vld_bmp);
    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_PKT_GEN_VLD, loop_cnt, pkt_gen_loop);

    /* Start generate sending */
    AAL_PON_REG_FIELD_WRITE(FPGA_IDP_UP_PKT_GEN_CNTL, enable, 1);

    return CA_E_OK;

}


ca_status_t aal_idp_ups_xge_pkt_set(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_uint8_t                 pkt_idx,
    CA_IN  aal_idp_ups_pkt_mask_t     mask,
    CA_IN  aal_idp_ups_pkt_t         *pkt)
{
    ca_uint32_t  data[8];
    ca_uint32_t  entry_idx = 0;


    CA_ASSERT_RET(pkt_idx < AAL_IDP_UPS_PKT_NUM, CA_E_PARAM);

    for (entry_idx = 0; entry_idx < 4; entry_idx ++) {
        AAL_PON_REG_INDIRECT_READ((pkt_idx << 2) + entry_idx, FPGA_IDP_UP_PKT_MEM_ACCESS);
        data[entry_idx << 1]       = AAL_PON_REG_READ(FPGA_IDP_UP_PKT_MEM_DATA0);
        data[(entry_idx << 1) + 1] = AAL_PON_REG_READ(FPGA_IDP_UP_PKT_MEM_DATA1);
    }

    if (mask.s.da      ) {
        data[0] = *(ca_uint32_t *)&pkt->da[0];
        data[1] = (*(ca_uint16_t *)&pkt->da[4] << 16) + (data[1] & 0xFFFF);
    }

    if (mask.s.sa      ) {
        data[1] = *(ca_uint16_t *)&pkt->sa[0] + (data[1] & 0xFFFF0000);
        data[2] = *(ca_uint32_t *)&pkt->sa[2] ;
    }

    if (mask.s.tpid    ) {
        data[3] = (pkt->tpid << 16) + (data[3] & 0xFFFF);
    }

    if (mask.s.vlan    ) {
        data[3] = pkt->vlan + (data[3] & 0xFFFF0000);
    }

    if (mask.s.ethtype ) {
        data[4] = (pkt->ethtype << 16) + (data[4] & 0xFFFF);
    }

    if (mask.s.pad_len ) {
        data[4] = pkt->pad_len + (data[4] & 0xFFFF);
    }

    if (mask.s.pad     ) {
        memcpy((ca_uint8_t *)&data[5], &pkt->pad, 12);
    }

    for (entry_idx = 0; entry_idx < 4; entry_idx ++) {
        AAL_PON_REG_WRITE(data[entry_idx << 1] , FPGA_IDP_UP_PKT_MEM_DATA0);
        AAL_PON_REG_WRITE(data[(entry_idx << 1) + 1], FPGA_IDP_UP_PKT_MEM_DATA1);
        AAL_PON_REG_INDIRECT_WRITE((pkt_idx << 2) + entry_idx, FPGA_IDP_UP_PKT_MEM_ACCESS);
    }

    return CA_E_OK;

}


ca_status_t aal_idp_ups_xge_pkt_get(
    CA_IN  ca_device_id_t                device_id,
    CA_IN  ca_uint8_t                 pkt_idx,
    CA_OUT aal_idp_ups_pkt_t         *pkt)
{
    ca_uint32_t  data[8];
    ca_uint32_t  entry_idx = 0;


    CA_ASSERT_RET(pkt_idx < AAL_IDP_UPS_PKT_NUM, CA_E_PARAM);

    for (entry_idx = 0; entry_idx < 4; entry_idx ++) {
        AAL_PON_REG_INDIRECT_READ((pkt_idx << 2) + entry_idx, FPGA_IDP_UP_PKT_MEM_ACCESS);
        data[entry_idx << 1]       = AAL_PON_REG_READ(FPGA_IDP_UP_PKT_MEM_DATA0);
        data[(entry_idx << 1) + 1] = AAL_PON_REG_READ(FPGA_IDP_UP_PKT_MEM_DATA1);
    }

    *(ca_uint32_t *)&pkt->da[0] = data[0];
    *(ca_uint16_t *)&pkt->da[4] = data[1] >> 16;
    *(ca_uint16_t *)&pkt->sa[0] = data[1] & 0xFFFF;
    *(ca_uint32_t *)&pkt->sa[2] = data[2];
    *(ca_uint16_t *)&pkt->tpid  = data[3] >> 16;
    *(ca_uint16_t *)&pkt->vlan  = data[3] & 0xFFFF;
    *(ca_uint16_t *)&pkt->ethtype = data[4] >> 16;
    *(ca_uint16_t *)&pkt->pad_len = data[4] & 0xFFFF;
    memcpy(&pkt->pad, (ca_uint8_t *)&data[5], 12);

    return CA_E_OK;

}

ca_status_t aal_idp_up_heada_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *pkt_len)
{
    FPGA_IDP_UP_PKT_HDR_A_LO_t hda_lo;
    hda_lo.wrd = 0;

    hda_lo.wrd = AAL_PON_REG_READ(FPGA_IDP_UP_PKT_HDR_A_LO);
    *pkt_len = hda_lo.bf.hdr_a_lo>>15;
    return CA_E_OK;
}

ca_status_t aal_idp_up_heada_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t pkt_len)
{

    if(pkt_len<64)
    {
        ca_printf("pkt_len is less than 64 Byte!!\r\n");
        return CA_E_ERROR;
    }
    if(pkt_len >1518)
    {
        ca_printf("pkt_len is more than 1518 Byte!!\r\n");
        return CA_E_ERROR;
    }

    AAL_PON_REG_WRITE(0x40000 , FPGA_IDP_UP_PKT_HDR_A_HI);
    AAL_PON_REG_WRITE((pkt_len<<15)|0x8 , FPGA_IDP_UP_PKT_HDR_A_LO);
    AAL_PON_REG_WRITE(0x1 , FPGA_IDP_UP_PKT_HDR_A_CNTL);

    return CA_E_OK;
}


ca_status_t aal_idp_reset(ca_device_id_t device_id)
{
/*
   Correct XGE/IDP init sequence (called by ONU init)
   reg_op w 0x5222d404 0x20000000 <-----disable XGE RX TITAN_XGE_RX_EN
   reg_op w 0x4410a00c 0x800      <-----reset TITANGLB_CTRL0.RESET_XGE_BLOCK
   reg_op w 0x4410a020 0x302      <-----reset IDP GLB_FPGA_IDP_CNTRL.idp_rst_n
   reg_op w 0x4410a020 0x30a      <-----release IDP GLB_FPGA_IDP_CNTRL.idp_rst_n
   reg_op w 0x5222881c 1          <-----enable IDP INTR
   reg_op w 0x4410a00c 0          <-----release TITANGLB_CTRL0.RESET_XGE_BLOCK
   reg_op w 0x5222d404 0x30000000 <-----enable XGE RX TITAN_XGE_RX_EN
*/

    return CA_E_OK;

}


ca_status_t aal_idp_init(ca_device_id_t device_id, ca_uint32_t pon_mac_mode)
{
#if 0
    aal_idp_dns_cfg_mask_t  dns_msk;
    aal_idp_dns_cfg_t       dns_cfg;

    ca_printf("INFO: sizeof aal_idp_hdr_a_t = %u\n", sizeof(aal_idp_hdr_a_t));

    dns_msk.u32_data = 0;
    dns_msk.s.my_ethtype = 1;

    dns_cfg.my_ethtype = 0xfff1; /* OMCI */
    (void)aal_idp_dns_cfg_set(device_id, dns_msk, &dns_cfg);
#endif

    if (pon_mac_mode >= ONU_PON_MAC_GPON){
        aal_idp_reset(device_id); /* Reset IDP to clear wrong state when traffic running */
    }

    aal_idp_dns_stats_clear(device_id);

#if (defined(CONFIG_ARCH_CORTINA_G3HGU) && defined(CONFIG_CORTINA_BOARD_FPGA))
    /* downstream goes to RGMII port for FPGA */
    (void)aal_idp_dns_rgmii_path_ctrl_set(device_id, FALSE);    // PON <-> RGMII
#endif

    ca_printf("IDP init OK.\r\n");

    return CA_E_OK;
}


#ifdef __KERNEL__

EXPORT_SYMBOL(aal_idp_dns_dpid_stats_get);
EXPORT_SYMBOL(aal_idp_dns_global_stats_get);
EXPORT_SYMBOL(aal_idp_dns_stats_clear);
EXPORT_SYMBOL(aal_idp_dns_cfg_set);
EXPORT_SYMBOL(aal_idp_dns_cfg_get);
EXPORT_SYMBOL(aal_idp_dns_rgmii_path_ctrl_set);
EXPORT_SYMBOL(aal_idp_dns_rgmii_path_ctrl_get);
EXPORT_SYMBOL(aal_idp_dns_rx_pkt_get);
EXPORT_SYMBOL(aal_idp_ups_cfg_set);
EXPORT_SYMBOL(aal_idp_ups_cfg_get);
EXPORT_SYMBOL(aal_idp_ups_global_stats_get);
EXPORT_SYMBOL(aal_idp_ups_local_cpu_pkt_tx);
EXPORT_SYMBOL(aal_idp_ups_xge_pkt_set);
EXPORT_SYMBOL(aal_idp_ups_xge_pkt_get);
EXPORT_SYMBOL(aal_idp_ups_xge_pkt_generate);
EXPORT_SYMBOL(aal_idp_up_heada_get);
EXPORT_SYMBOL(aal_idp_up_heada_set);

#endif

/* end of aal_idp.c */
#endif /* defined(CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN) || defined(CONFIG_ARCH_CORTINA_G3HGU)*/

