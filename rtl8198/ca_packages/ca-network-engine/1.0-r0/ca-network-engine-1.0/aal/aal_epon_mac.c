#include "osal_common.h"
#include "aal_pon.h"
#include "aal_epon.h"

#include <soc/cortina/registers.h>
#include "aal_epon_mac.h"
#include "aal_mpcp.h"
#include "ca-scfg/scfg.h"

#define __RX_IPG_MIN                5
#define __RX_IPG_MAX                20

#define __TX_IPG_MIN                10
#define __TX_IPG_MAX                17

#define __XOFF_THOLD_MAX            1023
#define __XON_THOLD_MAX             1023

#define __CASCADE_FIFO_THOLD_MAX    1024
//#define __TX_SYNC_FIFO_THOLD_MAX    512
#define __TX_SYNC_FIFO_THOLD_MAX    0x7FF
#define __TX_ASYNC_FIFO_THOLD_MAX   0xF

#define __MIB_ACCESS_TIMEOUT        0x1FFF

#define __LINK_DETECT_RXDV          0x2
#define __LINK_DETECT_INTERVAL_MAX  0x3f

#define __WRITE                     1
#define __TX_BUFF_SIZE              512
#define __TX_BUFF_ACCESS_TIMEOUT    0xFFF
#define __TX_FRAME_LEN_MAX          256


#define __SRAM_INIT_TIMEOUT             0x1fff
#define __MIB_RAM_INIT_TIMEOUT          0x1fff

#define __PON_IPG_12B                   0
#define __PON_IPG_11B                   1
#define __PON_IPG_10B                   2
#define __PON_IPG_13B                   3

#define __TX_MAX_PKT_SIZE               2047

typedef enum
{
    __MIB_RX_EPON_REGSTRCNT = 0,  /* MPCP Register frame to this LLID (acc_data[23:16])                 */
    __MIB_RX_EPON_UCGATECNT,      /* MPCP normal Gate frame to this LLID (acc_data[15:0])               */
    __MIB_RX_MPCP_OPIVLDCNT,      /* MAC control frame but opcode is not 01~06(acc_data[31:16])         */
    __MIB_RX_EPON_OAMFRMCNT,      /* frames sent by SW to this LLID(acc_data[15:0])                     */
    __MIB_TX_EPON_REGREQCNT,      /* MPCP Request frame counter (acc_data[31:24])                       */
    __MIB_TX_EPON_REGACKCNT,      /* MPCP Acknowledge frame counter (acc_data[23:16])                   */
    __MIB_TX_EPON_REPORTCNT,      /* MPCP Report frame counter (acc_data[15:0])                         */
    __MIB_TX_MPCP_OPIVLDCNT,      /* MAC control frame but opcode is not 01~06(acc_data[31:16])         */
    __MIB_TX_EPON_OAMFRMCNT,      /* frames received by SW from this LLID(acc_data[15:0])               */

    __MIB_TX_LLID_MPCP_MAX
}__pmac_mib_llid_mpcp_mib_t;

typedef enum
{
    __MIB_RX_EPON_BYTECNT_LO = 0,         /* low-32bit of receive frame byte counter                                  */
    __MIB_RX_EPON_BYTECNT_HI,             /* high-32bit of receive frame byte counter                                 */
    __MIB_RX_EPON_MCFRAMECNT,             /* multicast frame counter                                                  */
    __MIB_RX_EPON_BCFRAMECNT,             /* broadcast frame counter                                                  */
    __MIB_RX_EPON_UNDERSIZECNT,           /* number of frames received with less than 64 octets                       */
    __MIB_RX_EPON_OVERSIZECNT,            /* number of frames received with less than max length                      */
    __MIB_RX_EPON_RAMECNT,                /* total number of frame counter, include good or bad frame                 */
    __MIB_RX_EPON_RMON64CNT,              /* 64-byte frame (good or bad)                                              */
    __MIB_RX_EPON_RMON65_127CNT,          /* frame (good or bad) with size between 65 and 127 bytes                   */
    __MIB_RX_EPON_RMON128_255CNT,         /* frame (good or bad) with size between 128 and 255 bytes                  */
    __MIB_RX_EPON_RMON256_511CNT,         /* frame (good or bad) with size between 256 and 511 bytes                  */
    __MIB_RX_EPON_RMON512_1023CNT,        /* frame (good or bad) with size between 512 and 1023 bytes                 */
    __MIB_RX_EPON_RMON1024_1518CNT,       /* frame (good or bad) with size between 1024 and 1518 bytes                */
    __MIB_RX_EPON_RMON1519_1533CNT,       /* frame (good or bad) with size between 1519 and 1533 bytes                */
    __MIB_RX_EPON_RMON1534_2299CNT,       /* frame (good or bad) with size between 1534 and 2299 bytes                */
    __MIB_RX_EPON_RMON2300_2599CNT,       /* frame (good or bad) with size between 2300 and 2599 bytes                */
    __MIB_RX_EPON_RMON2600_MAXCNT,        /* frame (good or bad) with size between 2600 andconfigured maximum value,
                                            * default is 12K bytes                                                    */
    __MIB_RX_EPON_RUNTCNT,                /* number of frames received with less than 64 octets and CRC error         */
    __MIB_RX_EPON_JABBERCNT,              /* number of frames received that exceeds the programmed max frame length
                                            * and CRC ERROR                                                           */
    __MIB_RX_EPON_FCSCNT,                 /* number of frames received with fcs error                                 */
    __MIB_RX_EPON_OVERFLOWCNT,            /* number of frames flushed in rxtrl because of fifo overflow               */
    __MIB_TX_EPON_BYTECNT_LO,             /* low-32bit of receive frame byte counter , include good or bad frame      */
    __MIB_TX_EPON_BYTECNT_HI,             /* high-32bit of receive frame byte counter, include good or bad frame      */
    __MIB_TX_EPON_MCFRAMECNT,             /* multicast frame counter                                                  */
    __MIB_TX_EPON_BCFRAMECNT,             /* broadcast frame counter                                                  */
    __MIB_TX_EPON_RAMECNT,                /* total number of frame counter, include good or bad frame                 */
    __MIB_TX_EPON_RMON64CNT,              /* 64-byte frame (good or bad)                                              */
    __MIB_TX_EPON_RMON65_127CNT,          /* frame (good or bad) with size between 65 and 127 bytes                   */
    __MIB_TX_EPON_RMON128_255CNT,         /* frame (good or bad) with size between 128 and 255 bytes                  */
    __MIB_TX_EPON_RMON256_511CNT,         /* frame (good or bad) with size between 256 and 511 bytes                  */
    __MIB_TX_EPON_RMON512_1023CNT,        /* frame (good or bad) with size between 512 and 1023 bytes                 */
    __MIB_TX_EPON_RMON1024_1518CNT,       /* frame (good or bad) with size between 1024 and 1518 bytes                */
    __MIB_TX_EPON_RMON1519_1533CNT,       /* frame (good or bad) with size between 1519 and 1533 bytes                */
    __MIB_TX_EPON_RMON1534_2299CNT,       /* frame (good or bad) with size between 1534 and 2299 bytes                */
    __MIB_TX_EPON_RMON2300_2599CNT,       /* frame (good or bad) with size between 2300 and 2599 bytes                */
    __MIB_TX_EPON_RMON2600_MAXCNT,        /* frame (good or bad) with size between 2600 andconfigured maximum value,
                                            * default is 12K bytes                                                    */
    __MIB_PMAC_LLID_ETH_MAX
}__pmac_mib_llid_eth_t;


typedef enum
{
    __MIB_RX_EPON_CRC8ERRCNT = 0,     /* received frame with crc8 error                             */
    __MIB_RX_EPON_BCCRC8VLDFCSERRCNT, /* frame with correct crc8 but wrong fcs, with broadcast llid */
    __MIB_RX_EPON_BCREGSTRCNT,        /* broadcast register request frame                           */
    __MIB_RX_EPON_BCGATECNT,          /* mpcp discovery gate frame with broadcast llid              */
    __MIB_RX_EPON_EXTENCNT,           /* received extension mpcp frame counter                      */
    __MIB_TX_EPON_BCREGREQCNT,        /* broadcast register request frame                           */
    __MIB_TX_EPON_BCCRC8VLDFCSERRCNT, /* frame with correct crc8 but wrong fcs, with broadcast llid */
    __MIB_TX_EPON_EXTENCNT,           /* transmited extension mpcp frame counter                    */

    __MIB_PMAC_PORT_MPCP_MAX
}__pmac_mib_port_mpcp_t;

typedef enum
{
    __MIB_RX_BYTECNT_LO = 0,     /* frame byte count, low-32bit                                                      */
    __MIB_RX_BYTECNT_HI,         /* frame byte count, high-32bit                                                     */
    __MIB_RX_CRCERRCNT ,         /* frame with FCS error (only active when receive port)                             */
    __MIB_RX_UCFRAMECNT,         /* unicast frame count                                                              */
    __MIB_RX_MCFRAMECNT,         /* multicast frame                                                                  */
    __MIB_RX_BCFRAMECNT,         /* broadcast frame                                                                  */
    __MIB_RX_OAMFRAMECNT,        /* OAM frame                                                                        */
    __MIB_RX_PAUSECNT,           /* Pause frame                                                                      */
    __MIB_RX_UNKNOWNOPCNT,       /* MAC control frame with opcode not 01~06 (only for receiver port)                 */
    __MIB_RX_UNDERSIZECNT ,      /* frame with size less than 64 bytes (only for receiver port)                      */
    __MIB_RX_OVERSIZECNT,        /* frame with size more than configured value 2047 bytes (only for receiver port)   */
    __MIB_RX_RMON64CNT,          /* 64-byte frame                                                                    */
    __MIB_RX_RMON65_127CNT,      /* frame with size between 65 and 127 bytes                                         */
    __MIB_RX_RMON128_255CNT,     /* frame with size between 128 and 255 bytes                                        */
    __MIB_RX_RMON256_511CNT,     /* frame with size between 256 and 511 bytes                                        */
    __MIB_RX_RMON512_1023CNT,    /* frame with size between 512 and 1023 bytes                                       */
    __MIB_RX_RMON1024_1518CNT,   /* frame with size between 1024 and 1518 bytes,                                     */
    __MIB_RX_RMON1519_1533CNT,   /* frame with size between 1519 and 1533 bytes                                      */
    __MIB_RX_RMON1534_2299CNT,   /* frame with size between 1534 and 2299 bytes                                      */
    __MIB_RX_RMON2300_2599CNT,   /* frame with size between 2300 and 2599 bytes                                      */
    __MIB_RX_RMON2600_MAXCNT,    /* frame with size between 2600 andconfigured maximum value,default is 12K bytes    */
    __MIB_TX_BYTECNT_LO,         /* frame byte count, low-32bit                                                      */
    __MIB_TX_BYTECNT_HI,         /* frame byte count, high-32bit                                                     */
    __MIB_TX_CRCERRCNT ,         /* frame with FCS error (only active when receive port)                             */
    __MIB_TX_UCFRAMECNT,         /* unicast frame count                                                              */
    __MIB_TX_MCFRAMECNT,         /* multicast frame                                                                  */
    __MIB_TX_BCFRAMECNT,         /* broadcast frame                                                                  */
    __MIB_TX_OAMFRAMECNT,        /* OAM frame                                                                        */
    __MIB_TX_PAUSECNT,           /* Pause frame                                                                      */
    __MIB_TX_UNKNOWNOPCNT,       /* MAC control frame with opcode not 01~06 (only for receiver port)                 */
    __MIB_TX_UNDERSIZECNT ,      /* frame with size less than 64 bytes (only for receiver port)                      */
    __MIB_TX_OVERSIZECNT,        /* frame with size more than configured value 2047 bytes (only for receiver port)   */
    __MIB_TX_RMON64CNT,          /* 64-byte frame                                                                    */
    __MIB_TX_RMON65_127CNT,      /* frame with size between 65 and 127 bytes                                         */
    __MIB_TX_RMON128_255CNT,     /* frame with size between 128 and 255 bytes                                        */
    __MIB_TX_RMON256_511CNT,     /* frame with size between 256 and 511 bytes                                        */
    __MIB_TX_RMON512_1023CNT,    /* frame with size between 512 and 1023 bytes                                       */
    __MIB_TX_RMON1024_1518CNT,   /* frame with size between 1024 and 1518 bytes,                                     */
    __MIB_TX_RMON1519_1533CNT,   /* frame with size between 1519 and 1533 bytes                                      */
    __MIB_TX_RMON1534_2299CNT,   /* frame with size between 1534 and 2299 bytes                                      */
    __MIB_TX_RMON2300_2599CNT,   /* frame with size between 2300 and 2599 bytes                                      */
    __MIB_TX_RMON2600_MAXCNT,    /* frame with size between 2600 andconfigured maximum value, default is 12K bytes   */
    __MIB_PMAC_PORT_ETH_MAX
}__pmac_mib_port_eth_t;

ca_int32_t g_epon_encryption_mode = 0;

ca_status_t __pmac_mib_init(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id
)
{
    ca_uint32_t timeout = __SRAM_INIT_TIMEOUT;
    ca_boolean_t init_done;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);

    AAL_PON_REG_FIELD_WRITE(EPON_EPON_MIB_MIB_CTL, ram_sw_initial, 1);
    AAL_PON_REG_FIELD_WRITE(EPON_EPON_MIB_MIB_CTL, ram_sw_initial, 0);

    do
    {
        AAL_PON_REG_FIELD_READ(EPON_EPON_MIB_MIB_RAM_STATUS_REG, ram_initdone, init_done);
    }while( (!init_done) && (--timeout));

    if((!init_done) && (timeout==0) )
    {
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t aal_epon_mac_mib_rc_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   read_clear
)
{

    AAL_PON_REG_FIELD_WRITE(EPON_EPON_MIB_MIB_CTL, ram_clr_en, (read_clear) ? 1 : 0);

    return CA_E_OK;
}

ca_status_t aal_epon_mac_mib_rc_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   *read_clear
)
{

    ca_uint8_t val = 0;

    if (!read_clear)
        return CA_E_PARAM;

    AAL_PON_REG_FIELD_READ(EPON_EPON_MIB_MIB_CTL, ram_clr_en, val);

    *read_clear = val ? CA_TRUE: CA_FALSE;

    return CA_E_OK;
}


ca_status_t aal_epon_mac_cfg_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_mac_ctrl_msk_t        mask,
    CA_IN aal_epon_mac_ctrl_t           *cfg
)
{

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(cfg, CA_E_PARAM);

    /* param check */
    if(mask.s.tx_xg_ipg)
    {
        if((cfg->tx_xg_ipg!=8) && (cfg->tx_xg_ipg!=12))
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.tx_xg_thold_sfifo_rd)
    {
        if(cfg->tx_xg_thold_sfifo_rd > __TX_SYNC_FIFO_THOLD_MAX)
        {
            return CA_E_PARAM;
        }
    }
    if(mask.s.tx_xg_thold_sfifo_wr)
    {
        if(cfg->tx_xg_thold_sfifo_wr > __TX_SYNC_FIFO_THOLD_MAX)
        {
            return CA_E_PARAM;
        }
    }
    if(mask.s.tx_xg_thold_asfifo_rd)
    {
        if(cfg->tx_xg_thold_asfifo_rd > __TX_ASYNC_FIFO_THOLD_MAX)
        {
            return CA_E_PARAM;
        }
    }
    if(mask.s.tx_xg_thold_asfifo_wr)
    {
        if(cfg->tx_xg_thold_asfifo_wr > __TX_ASYNC_FIFO_THOLD_MAX)
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.tx_1g_ipg)
    {
        if((cfg->tx_1g_ipg < 10) || (cfg->tx_1g_ipg > 13))
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.tx_1g_psize_max)
    {
        if (cfg->tx_1g_psize_max > __TX_MAX_PKT_SIZE)
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.tx_1g_crc8_order)
    {
        if(cfg->tx_1g_crc8_order > AAL_PON_CRC8_MODE_REV_CRC8_DATA)
        {
            return CA_E_PARAM;
        }
    }

    if(mask.s.onu_type)
    {
        AAL_GLB_REG_FIELD_WRITE(GLOBAL_EPON_CNTL ,epon_onu_mode, cfg->onu_type);
    }

    if(mask.s.rx_xg_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, mac_en, (cfg->rx_xg_ena!=0));
    }

    if(mask.s.tx_xg_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, mac_en, (cfg->tx_xg_ena!=0));
    }

    if(mask.s.rx_xg_psize_max)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, max_size, cfg->rx_xg_psize_max);
    }

    if(mask.s.rx_xg_flowctrl_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, pause_en, (cfg->rx_xg_flowctrl_en!=0));
    }

    if(mask.s.tx_xg_ipg)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, ipg_minimal, cfg->tx_xg_ipg);
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, dic_maximum, (8 == cfg->tx_xg_ipg));
    }

    if(mask.s.tx_xg_crc32_regen_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, crc_recaculate_en, (cfg->tx_xg_crc32_regen_en==1));
    }

    if(mask.s.tx_xg_thold_sfifo_rd)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_CTRL_XGEPN_TX_FF_THRESHOLD, txctl_rdff_threshold, cfg->tx_xg_thold_sfifo_rd);
    }

    if(mask.s.tx_xg_thold_sfifo_wr)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_CTRL_XGEPN_TX_FF_THRESHOLD, txctl_wrff_threshold, cfg->tx_xg_thold_sfifo_wr);
    }

    if(mask.s.tx_xg_thold_asfifo_rd)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_CTRL_XGEPN_TX_FF_THRESHOLD, txff_rdff_threshold, cfg->tx_xg_thold_asfifo_rd);
    }

    if(mask.s.tx_xg_thold_asfifo_wr)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_CTRL_XGEPN_TX_FF_THRESHOLD, txff_wrff_threshold, cfg->tx_xg_thold_asfifo_wr);
    }


    if(mask.s.max_mtu_tag_en){
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_RXCTRL, max_mtu_tag_en, cfg->max_mtu_tag_en);
    }

    if(mask.s.rx_1g_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_RXCTRL, RX_EN, (cfg->rx_1g_ena!=0));
    }

    if(mask.s.tx_1g_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, TX_EN, (cfg->tx_1g_ena!=0));
    }

    if(mask.s.rx_1g_psize_max)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_MAXPKTSIZE, rx_max_pkt_size, cfg->rx_1g_psize_max);
    }

    if(mask.s.tx_1g_psize_max_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, max_pkt_size_limit, (cfg->tx_1g_psize_max_ena!=0));
    }

    if(mask.s.tx_1g_psize_max)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_MAXPKTSIZE, tx_max_pkt_size, cfg->tx_1g_psize_max);
    }

    if(mask.s.tx_1g_ipg) // TODO: ipg
    {
        switch(cfg->tx_1g_ipg)
        {
            case 12: AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, ipg_sel, __PON_IPG_12B); break;
            case 11: AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, ipg_sel, __PON_IPG_11B); break;
            case 10: AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, ipg_sel, __PON_IPG_10B); break;
            case 13: AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, ipg_sel, __PON_IPG_13B); break;
            default: break;
        }
        //REG_FIELD_WRITE(BM_PORT_SHAPERI_CFG, w_rate, cfg->tx_ipg+8);
    }

    if(mask.s.tx_1g_8b_preamble_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, reserved_1, cfg->tx_1g_8b_preamble_ena);
    }

    if(mask.s.tx_1g_crc8_order)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, crc8_calculation_order_select, (ca_uint32_t)(cfg->tx_1g_crc8_order));
    }

    if(mask.s.tx_1g_crc32_regen_en)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, disable_crc32_re_generation, (cfg->tx_1g_crc32_regen_en==0));
    }

    if(mask.s.tx_1g_drain_ena)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_ONU_EPON_TXCTRL, drain, cfg->tx_1g_drain_ena);
    }

    if(mask.s.rx2tx_lookback)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, loopback, cfg->rx2tx_lookback);
    }

    if(mask.s.tx2rx_lookback)
    {
        //AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, loopback, cfg->tx2rx_lookback);
    }
    return CA_E_OK;
}

ca_status_t aal_epon_mac_cfg_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_ctrl_t          *cfg
)
{
    ca_uint8_t ipg;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(cfg, CA_E_PARAM);

    AAL_GLB_REG_FIELD_READ(GLOBAL_EPON_CNTL ,epon_onu_mode, cfg->onu_type);

    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, mac_en, cfg->rx_xg_ena);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, mac_en, cfg->tx_xg_ena);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, max_size, cfg->rx_xg_psize_max);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, pause_en, cfg->rx_xg_flowctrl_en);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, ipg_minimal, cfg->tx_xg_ipg);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, crc_recaculate_en, cfg->tx_xg_crc32_regen_en);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_CTRL_XGEPN_TX_FF_THRESHOLD, txctl_rdff_threshold, cfg->tx_xg_thold_sfifo_rd);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_CTRL_XGEPN_TX_FF_THRESHOLD, txctl_wrff_threshold, cfg->tx_xg_thold_sfifo_wr);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_CTRL_XGEPN_TX_FF_THRESHOLD, txff_rdff_threshold, cfg->tx_xg_thold_asfifo_rd);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_CTRL_XGEPN_TX_FF_THRESHOLD, txff_wrff_threshold, cfg->tx_xg_thold_asfifo_wr);

    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_RXCTRL, max_mtu_tag_en, cfg->max_mtu_tag_en);

    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_RXCTRL, RX_EN, cfg->rx_1g_ena);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, TX_EN, cfg->tx_1g_ena);

    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, max_pkt_size_limit, cfg->tx_1g_psize_max_ena);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_MAXPKTSIZE, rx_max_pkt_size, cfg->rx_1g_psize_max);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_MAXPKTSIZE, tx_max_pkt_size, cfg->tx_1g_psize_max);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, ipg_sel, ipg);
    switch(ipg){
        case __PON_IPG_12B: cfg->tx_1g_ipg = 12; break;
        case __PON_IPG_11B: cfg->tx_1g_ipg = 11; break;
        case __PON_IPG_10B: cfg->tx_1g_ipg = 10; break;
        case __PON_IPG_13B: cfg->tx_1g_ipg = 13; break;
        default:break;
    }
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, reserved_1, cfg->tx_1g_8b_preamble_ena);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, crc8_calculation_order_select, cfg->tx_1g_crc8_order);
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, disable_crc32_re_generation, cfg->tx_1g_crc32_regen_en);
    cfg->tx_1g_crc32_regen_en = !cfg->tx_1g_crc32_regen_en;
    AAL_PON_REG_FIELD_READ(EPON_ONU_EPON_TXCTRL, drain, cfg->tx_1g_drain_ena);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, loopback, cfg->rx2tx_lookback);
    //AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, loopback, cfg->tx2rx_lookback);

    return CA_E_OK;
}

ca_status_t aal_epon_mac_onu_type_get
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t         port_id,
    CA_OUT ca_uint8_t          *onu_type
)
{
    CA_NULL_PTR_CHECK(onu_type);

    AAL_GLB_REG_FIELD_READ(GLOBAL_EPON_CNTL ,epon_onu_mode, *onu_type);

    return CA_E_OK;
}

ca_status_t aal_epon_mac_drop_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_mac_drop_msk_t        pmac_xg_drop_msk,
    CA_IN aal_epon_mac_drop_t           *pmac_xg_drop
)
{
    EPON_XGEPN_XMAC_CTRL_XGEPN_RX_CTL_t xg_rx_ctrl;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(pmac_xg_drop, CA_E_PARAM);

    /* read first */
    xg_rx_ctrl.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_CTRL_XGEPN_RX_CTL);

    /* write to HW */
    if(pmac_xg_drop_msk.s.invalid_llid_drop)
    {
        xg_rx_ctrl.bf.frame_invld_llid_drop_en = (pmac_xg_drop->invalid_llid_drop!=0);
    }
    if(pmac_xg_drop_msk.s.fcs_err_drop)
    {
        xg_rx_ctrl.bf.frame_fcs_error_drop_en = (pmac_xg_drop->fcs_err_drop!=0);
    }
    if(pmac_xg_drop_msk.s.unknown_ds_drop)
    {
        xg_rx_ctrl.bf.frame_unda_drop_en = (pmac_xg_drop->unknown_ds_drop!=0);
    }
    if(pmac_xg_drop_msk.s.oversize_drop)
    {
        xg_rx_ctrl.bf.frame_oversize_drop_en = (pmac_xg_drop->oversize_drop!=0);
    }
    if(pmac_xg_drop_msk.s.runtsize_drop)
    {
        xg_rx_ctrl.bf.frame_runt_drop_en = (pmac_xg_drop->runtsize_drop!=0);
    }
    if(pmac_xg_drop_msk.s.pause_drop)
    {
        xg_rx_ctrl.bf.frame_pause_drop_en = (pmac_xg_drop->pause_drop!=0);
    }
    if(pmac_xg_drop_msk.s.unknown_op_drop)
    {
        xg_rx_ctrl.bf.frame_unopcode_drop_en = (pmac_xg_drop->unknown_op_drop!=0);
    }
    if(pmac_xg_drop_msk.s.mpcp_drop)
    {
        xg_rx_ctrl.bf.frame_mpcp_drop_en = (pmac_xg_drop->mpcp_drop!=0);
    }
    if(pmac_xg_drop_msk.s.mpcp_ext_drop)
    {
        xg_rx_ctrl.bf.frame_exmpcp_mpcpdrop_en = (pmac_xg_drop->mpcp_ext_drop!=0);
    }
    if(pmac_xg_drop_msk.s.overflow_drop)
    {
        xg_rx_ctrl.bf.frame_ffoverflow_drop_en = (pmac_xg_drop->overflow_drop!=0);
    }
    if(pmac_xg_drop_msk.s.oam_sec_drop)
    {
        xg_rx_ctrl.bf.frame_keyex_oam_drop_en = (pmac_xg_drop->oam_sec_drop!=0);
    }
    /* write back to HW */
    AAL_PON_REG_WRITE(xg_rx_ctrl.wrd, EPON_XGEPN_XMAC_CTRL_XGEPN_RX_CTL);

    if(pmac_xg_drop_msk.s.mpcp_crc32_drop)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, mpcp_crc_chk, (pmac_xg_drop->mpcp_crc32_drop!=0));
    }

    if(pmac_xg_drop_msk.s.mpcp_crc8_drop)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_RXMAC_RX_PKT_DET, check_llid, (pmac_xg_drop->mpcp_crc8_drop!=0));
    }

    return CA_E_OK;
}

ca_status_t aal_epon_mac_drop_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_drop_t          *pmac_xg_drop
)
{
    EPON_XGEPN_XMAC_CTRL_XGEPN_RX_CTL_t xg_rx_ctrl;
    EPON_XGEPN_XMAC_RXMAC_RX_CONFIG_t rx_cfg;
    EPON_XGEPN_XMAC_RXMAC_RX_PKT_DET_t rx_pkt;


    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(pmac_xg_drop, CA_E_PARAM);

    xg_rx_ctrl.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_CTRL_XGEPN_RX_CTL);
    rx_cfg.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG);
    rx_pkt.wrd = AAL_PON_REG_READ(EPON_XGEPN_XMAC_RXMAC_RX_PKT_DET);

    pmac_xg_drop->invalid_llid_drop = xg_rx_ctrl.bf.frame_invld_llid_drop_en;
    pmac_xg_drop->fcs_err_drop      = xg_rx_ctrl.bf.frame_fcs_error_drop_en;
    pmac_xg_drop->oversize_drop     = xg_rx_ctrl.bf.frame_oversize_drop_en;
    pmac_xg_drop->runtsize_drop     = xg_rx_ctrl.bf.frame_runt_drop_en;
    pmac_xg_drop->pause_drop        = xg_rx_ctrl.bf.frame_pause_drop_en;
    pmac_xg_drop->unknown_op_drop   = xg_rx_ctrl.bf.frame_unopcode_drop_en;
    pmac_xg_drop->mpcp_drop         = xg_rx_ctrl.bf.frame_mpcp_drop_en;
    pmac_xg_drop->mpcp_ext_drop     = xg_rx_ctrl.bf.frame_exmpcp_mpcpdrop_en;
    pmac_xg_drop->overflow_drop     = xg_rx_ctrl.bf.frame_ffoverflow_drop_en;
    pmac_xg_drop->unknown_ds_drop   = xg_rx_ctrl.bf.frame_unda_drop_en;
    pmac_xg_drop->oam_sec_drop      = xg_rx_ctrl.bf.frame_keyex_oam_drop_en;
    pmac_xg_drop->mpcp_crc32_drop   = rx_cfg.bf.mpcp_crc_chk;
    pmac_xg_drop->mpcp_crc8_drop      = rx_pkt.bf.check_llid;

    return CA_E_OK;
}

ca_status_t aal_epon_mac_rst_ctrl_set
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_mac_rst_ctrl_msk_t    pon_rst_ctrl_msk,
    CA_IN aal_epon_mac_rst_ctrl_t       *pon_rst_ctrl
)
{

    if(pon_rst_ctrl_msk.s.xg_pcs_rx_rst)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_RX_RXCNTRL, logicReset, (pon_rst_ctrl->xg_pcs_rx_rst!=0));
    }
    if(pon_rst_ctrl_msk.s.xg_pcs_tx_rst)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_TX_TXCNTRL, logicReset, (pon_rst_ctrl->xg_pcs_tx_rst!=0));
    }
    if(pon_rst_ctrl_msk.s.rx_mac_rst)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, reset, (pon_rst_ctrl->rx_mac_rst!=0));
    }
    if(pon_rst_ctrl_msk.s.tx_mac_rst)
    {
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, reset, (pon_rst_ctrl->tx_mac_rst!=0));
    }

    return CA_E_OK;
}

ca_status_t aal_epon_mac_rst_ctrl_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_rst_ctrl_t      *pon_rst_ctrl
)
{

    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XGPCS_RX_RXCNTRL, logicReset, pon_rst_ctrl->xg_pcs_rx_rst);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XGPCS_TX_TXCNTRL, logicReset, pon_rst_ctrl->xg_pcs_tx_rst);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_RXMAC_RX_CONFIG, reset, pon_rst_ctrl->rx_mac_rst);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_TXMAC_TX_CONFIG, reset, pon_rst_ctrl->tx_mac_rst);

    return CA_E_OK;
}

/* addressing is {1'b<group>, 5'b<llid>, 5'<offset>}*/
ca_status_t aal_epon_mac_stats_llid_mpcp_get
(
    CA_IN  ca_device_id_t                    device_id,
    CA_IN  ca_port_id_t                   port_id,
    CA_IN  ca_llid_t                      llid,
    CA_OUT aal_epon_mac_stats_llid_mpcp_t *stats_llid_mpcp
)
{
    ca_uint16_t addr;
    ca_uint32_t data;
    ca_uint16_t group = 0;
    const ca_uint16_t offset[__MIB_TX_LLID_MPCP_MAX] = {0,0,1,1,0,0,0,1,1};

    //AAL_NULL_POINTER_CHECK(stats_llid_mpcp);
    //AAL_CHIP_CORE_CHECK(device_id,core_id);

    addr = (ca_uint16_t)((group<<10) | (llid<<5) | offset[__MIB_RX_EPON_REGSTRCNT]);
    AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_ACCESS);
    data = AAL_PON_REG_READ(EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_DATA);
    stats_llid_mpcp->rx_epon_regstrcnt = (data >> 16) & 0xff;
    stats_llid_mpcp->rx_epon_ucgatecnt = data & 0xffff;

    addr = (ca_uint16_t)((group<<10) | (llid<<5) | offset[__MIB_RX_MPCP_OPIVLDCNT]);
    AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_ACCESS);
    data = AAL_PON_REG_READ(EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_DATA);
    stats_llid_mpcp->rx_mpcp_opivldcnt = (data >> 16) & 0xffff;
    stats_llid_mpcp->rx_epon_oamfrmcnt = data & 0xffff;

    addr = (ca_uint16_t)((group<<10) | (llid<<5) | offset[__MIB_TX_EPON_REGREQCNT]);
    AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_ACCESS);
    data = AAL_PON_REG_READ(EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_DATA);
    stats_llid_mpcp->tx_epon_regreqcnt = data >> 24;
    stats_llid_mpcp->tx_epon_regackcnt = (data >> 16) & 0xff;
    stats_llid_mpcp->tx_epon_reportcnt = data & 0xffff;

    addr = (ca_uint16_t)((group<<10) | (llid<<5) | offset[__MIB_TX_MPCP_OPIVLDCNT]);
    AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_ACCESS);
    data = AAL_PON_REG_READ(EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_DATA);
    stats_llid_mpcp->tx_mpcp_opivldcnt = (data >> 16) & 0xffff;
    stats_llid_mpcp->tx_epon_oamfrmcnt = data & 0xffff;

    return CA_E_OK;
}

ca_status_t aal_epon_mac_stats_llid_eth_get
(
    CA_IN  ca_device_id_t                   device_id,
    CA_IN  ca_port_id_t                  port_id,
    CA_IN  ca_llid_t                     llid,
    CA_OUT aal_epon_mac_stats_llid_eth_t *stats_llid_eth
)
{
    ca_uint16_t addr;
    ca_uint32_t data;
    __pmac_mib_llid_eth_t mib_id;
    ca_uint16_t group = 0;
    ca_uint32_t *p = (ca_uint32_t*)stats_llid_eth;
    const ca_uint16_t offset[__MIB_PMAC_LLID_ETH_MAX] =
    {0x2,0x3,0x4,0x5,0x6,0x7,   // EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_DATA
     0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa, // EPON_EPON_MIB_RX_LLID_FRMCNT_MIB_ACC_DATA
     0x10,0x11,0x12, 0x13, // EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_DATA
     0x2,0x3,0x4,0x5, // EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_DATA
     0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa};// EPON_EPON_MIB_TX_LLID_FRMCNT_MIB_ACC_DATA

    //AAL_NULL_POINTER_CHECK(stats_llid_eth);
    //AAL_CHIP_CORE_CHECK(device_id,port_id);

    for(mib_id = __MIB_RX_EPON_BYTECNT_LO; mib_id < __MIB_PMAC_LLID_ETH_MAX; ++mib_id)
    {
        addr = (ca_uint16_t)((group<<10) | (llid<<5) | offset[mib_id]);

        if((mib_id >= __MIB_RX_EPON_BYTECNT_LO && mib_id <= __MIB_RX_EPON_OVERSIZECNT)||
            (mib_id >= __MIB_RX_EPON_RUNTCNT && mib_id <= __MIB_RX_EPON_OVERFLOWCNT)){
            AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_ACCESS);
            data = AAL_PON_REG_READ(EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_DATA);

        }else if(mib_id >= __MIB_RX_EPON_RAMECNT && mib_id <= __MIB_RX_EPON_RMON2600_MAXCNT){
            AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_RX_LLID_FRMCNT_MIB_ACC_ACCESS);
            data = AAL_PON_REG_READ(EPON_EPON_MIB_RX_LLID_FRMCNT_MIB_ACC_DATA);

        }else if(mib_id >= __MIB_TX_EPON_BYTECNT_LO && mib_id <= __MIB_TX_EPON_BCFRAMECNT){
            AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_ACCESS);
            data = AAL_PON_REG_READ(EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_DATA);

        }else if(mib_id >= __MIB_TX_EPON_RAMECNT && mib_id <= __MIB_TX_EPON_RMON2600_MAXCNT){
            AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_TX_LLID_FRMCNT_MIB_ACC_ACCESS);
            data = AAL_PON_REG_READ(EPON_EPON_MIB_TX_LLID_FRMCNT_MIB_ACC_DATA);

        }
        *(p++) = data;
    }

    return CA_E_OK;
}

ca_status_t aal_epon_mac_stats_port_mpcp_get
(
    CA_IN  ca_device_id_t                    device_id,
    CA_IN  ca_port_id_t                   port_id,
    CA_OUT aal_epon_mac_stats_port_mpcp_t *stats_port_mpcp
)
{
    ca_uint16_t addr;
    ca_uint32_t data;
    __pmac_mib_port_mpcp_t mib_id;
    ca_uint16_t group = 1;
    ca_uint32_t *p = (ca_uint32_t*)stats_port_mpcp;
    const ca_uint16_t offset[__MIB_PMAC_PORT_MPCP_MAX] =
    {1,3,4,5,0,  // EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_ACCESS
     0xff, 0xff, 0};// TX: 0xff means not support

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(stats_port_mpcp, CA_E_PARAM);

    for(mib_id = __MIB_RX_EPON_CRC8ERRCNT; mib_id < __MIB_PMAC_PORT_MPCP_MAX; ++mib_id)
    {
        addr = (ca_uint16_t)((group<<10) | (0<<5) | offset[mib_id]);
        if(mib_id < __MIB_TX_EPON_BCREGREQCNT){
            AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_ACCESS);
            data = AAL_PON_REG_READ(EPON_EPON_MIB_RX_LLID_MPCP_MIB_ACC_DATA);

        }else{
            if(offset[mib_id] != 0xff){ // TX: 0xff means not support
                AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_ACCESS);
                data = AAL_PON_REG_READ(EPON_EPON_MIB_TX_LLID_MPCP_MIB_ACC_DATA);
            }else{
                data = 0;
            }
        }
        *(p++) = data;
    }

    return CA_E_OK;
}

ca_status_t aal_epon_mac_stats_port_mpcp_ctrl_get
(
    CA_IN  ca_device_id_t                    device_id,
    CA_IN  ca_port_id_t                   port_id,
    CA_OUT aal_epon_mac_stats_port_mpcp_ctrl_t *mpcp_mac_ctrl
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(mpcp_mac_ctrl, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_CTRL_TX_MPCP_MIB1, count, mpcp_mac_ctrl->mpcp_mac_ctrl_frames_transmitted);
    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XMAC_CTRL_RX_MPCP_MIB1, count, mpcp_mac_ctrl->mpcp_mac_ctrl_frames_received);

    mpcp_mac_ctrl->mpcp_tx_reg_ack = 0;
    return CA_E_OK;
}

ca_status_t aal_epon_mac_stats_port_eth_get
(
    CA_IN  ca_device_id_t                   device_id,
    CA_IN  ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_stats_port_eth_t *stats_port_eth
)
{
    ca_uint16_t addr;
    ca_uint32_t data;
    ca_uint16_t mib_id;
    ca_uint32_t *p = (ca_uint32_t*)stats_port_eth;

    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(stats_port_eth, CA_E_PARAM);

    for(mib_id = __MIB_RX_BYTECNT_LO; mib_id < __MIB_PMAC_PORT_ETH_MAX; ++mib_id)
    {
        if(mib_id < __MIB_TX_BYTECNT_LO)
        {
            addr = (ca_uint16_t)mib_id;
            AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC_ACCESS);
            data = AAL_PON_REG_READ(EPON_EPON_MIB_RX_XGEPON_ETHNET_MIB_ACC_DATA);
        }
        else
        {
            addr = (ca_uint16_t)(mib_id-__MIB_TX_BYTECNT_LO);
            AAL_PON_REG_INDIRECT_READ(addr, EPON_EPON_MIB_TX_XGEPON_ETHNET_MIB_ACC_ACCESS);
            data = AAL_PON_REG_READ(EPON_EPON_MIB_TX_XGEPON_ETHNET_MIB_ACC_DATA);
        }

        *(p++) = data;
    }

    return CA_E_OK;
}

ca_status_t aal_epon_mac_1g_link_status_get
(
    CA_IN  ca_device_id_t                   device_id,
    CA_IN  ca_port_id_t                  port_id,
    CA_IN ca_boolean_t                   *status
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(status, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_EPON_GLB_EPON_STATUS, pcs_sync_1g, (*status));

    return CA_E_OK;
}

ca_status_t aal_epon_mac_xg_link_status_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_boolean_t                   *status
)
{
    //AAL_CHIP_CORE_CHECK(device_id,port_id);
    //AAL_ASSERT_RET(status, CA_E_PARAM);

    AAL_PON_REG_FIELD_READ(EPON_XGEPN_XGPCS_RX_XGEPN_LINK_STATUS, link_status, (*status));

    return CA_E_OK;
}

// TODO: need any more?
static inline ca_status_t __pmac_xg_tx_buff_write
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t buff_sel,
    CA_IN ca_uint16_t addr,
    CA_IN ca_uint32_t data
)
{
    ca_boolean_t busy = 0;
    ca_uint32_t  timeout = __TX_BUFF_ACCESS_TIMEOUT;

    //AAL_CHIP_CORE_CHECK(device_id, port_id);

    if (buff_sel > 1)
    {
        return CA_E_PARAM;
    }

    //AAL_PON_REG_WRITE(data, EPON_XGEPN_TXARB_OAM_TXBUF_DATA);
    //AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_TXARB_OAM_TXBUF_ACCESS, rbw, 1);
    //AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_TXARB_OAM_TXBUF_ACCESS, addr, (addr + buff_sel * __TX_BUFF_SIZE));
    //AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_TXARB_OAM_TXBUF_ACCESS, access, 1);

    /* check finish */
    do
    {
        //AAL_PON_REG_FIELD_READ(EPON_XGEPN_TXARB_OAM_TXBUF_ACCESS, access, busy);
        --timeout;
    }while(busy && (timeout != 0));

    if ((0 == timeout) && busy)
    {
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

static inline ca_boolean_t __pmac_xg_tx0_is_busy
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id
)
{
    ca_boolean_t busy = FALSE;
    ca_int32 timeout = __TX_BUFF_ACCESS_TIMEOUT;

    do
    {
        //AAL_PON_REG_FIELD_READ(EPON_XGEPN_TXARB_OAM_TXBUF0_CTRL, pkt_rdy, busy);

        --timeout;
    } while (busy && (timeout >= 0));

    return busy;
}

static inline ca_boolean_t __pmac_xg_tx_fifo_is_full
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint8_t cos
)
{
    ca_uint8_t fifo_full = 0;

    //AAL_PON_REG_FIELD_READ(EPON_XGEPN_TXARB_OAM_TXBUF_MLLID_STA, fifo_full, fifo_full);

    return (((1<<cos)&fifo_full)!=0);
}

#if 0
static ca_status_t __pmac_xg_frame_buff_write
(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_uint8_t      port,
    CA_IN ca_uint8_t      buff_sel,
    CA_IN ca_uint8_t      *pkt,
    CA_IN ca_uint16_t     len
)
{
    ca_uint8_t  *ptr_frm = pkt;
    ca_uint32_t reg_data;
    ca_uint16_t address;
    ca_status_t rt = CA_E_OK;

    /* 4 byte alignment */
    for (address=0; address < (len/4); address++)
    {
        memcpy(&reg_data,  ptr_frm, 4);
        reg_data = ca_htonl(reg_data);
        rt = __pmac_xg_tx_buff_write(device_id, port, buff_sel, address, reg_data);
        if(rt!=CA_E_OK) goto end;
        ptr_frm+= 4;
    }

    /* if len%4 != 0 */
    if (len % 4)
    {
        reg_data = 0;
        memcpy(&reg_data,  ptr_frm, len%4);
        reg_data = ca_htonl(reg_data);
        rt = __pmac_xg_tx_buff_write(device_id, port, buff_sel, address, reg_data);
    }

end:
    return rt;
}
#endif

static inline ca_status_t __pmac_xg_tx0_frame_trigger
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id
)
{
    ca_uint32_t timeout = __TX_BUFF_ACCESS_TIMEOUT;
    ca_boolean_t busy = FALSE;

    //AAL_CHIP_CORE_CHECK(device_id, port_id);

    //AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_TXARB_OAM_TXBUF0_CTRL, pkt_rdy, 1);

    /* check finish */
    do
    {
        //AAL_PON_REG_FIELD_READ(EPON_XGEPN_TXARB_OAM_TXBUF0_CTRL, pkt_rdy, busy);
        --timeout;
    } while (busy && (timeout != 0));

    if ((0 == timeout) && busy)
    {
        return CA_E_ERROR;
    }

    return CA_E_OK;
}
#if 0
ca_int8 pmac_send_debug = 0;
extern int ma_debug;

ca_status_t aal_epon_mac_frame_send
(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_uint8_t          port_id,
    CA_IN aal_epon_mac_pkt_t    *pkt
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint8_t llid_mode = 0;
    ca_uint8_t cos = pkt->cos;
    ca_uint32_t i = 0;

    if(pmac_send_debug || ma_debug)
    {
        ca_printf("\naal_epon_mac_frame_send: llid %d cos %d len %d start!\n", pkt->llid, pkt->cos, pkt->len);
    }

    if(pmac_send_debug)
    {
        for(i = 0; i < pkt->len; ++i)
        {
            if(0 == (i & 0xF)) ca_printf("\n%04x: ", i);
            if(0 == (i & 0x7)) ca_printf(" ");
            ca_printf("%02x ", pkt->data[i]);
        }
        ca_printf("\n");
    }

    if (!__pmac_xg_tx0_is_busy(device_id, port_id))
    {
        AAL_PON_REG_FIELD_READ(EPON_XGONU_MPCP_MPCP_CFG0, mllid_mode, llid_mode);
        /* multi-llid mode  */
        if(llid_mode == AAL_MPCP_LLID_MODE_MULTIPLE)
        {
            ca_uint8_t  idx = 0;
            ca_uint32_t val = 0;

            /* get index by llid */
            //ret = aal_mpcp_reg_llid_index_get(device_id, port_id,pkt->llid, &idx);
            if(ret != CA_E_OK)
            {
                if(pmac_send_debug || ma_debug)
                {
                    ca_printf("aal_epon_mac_frame_send: get index error\n");
                }

                goto frame_xg_exit;
            }

            /* get cos by index */
            val = AAL_PON_REG_READ(EPON_XGONU_RPT_LLID_COS_MAP);
            cos = (val >> (idx * 4)) & 0x7;
        }
        else{
            /* at single LLID mode.  always check fifo 0*/
            cos = 0;
        }

        /* tx fifo is full */
        if(__pmac_xg_tx_fifo_is_full(device_id, port_id, cos))
        {
            ret = CA_E_FULL;

            if(pmac_send_debug || ma_debug)
            {
                ca_printf("aal_epon_mac_frame_send: tx fifo full. llid mode %s. cos %d\n",
                    llid_mode == AAL_MPCP_LLID_MODE_MULTIPLE?"multi":"single", cos);
            }

            goto frame_xg_exit;
        }

        /* write buffer */
        ret = __pmac_xg_frame_buff_write(device_id, port_id, 0, pkt->data, pkt->len);
        if (ret != CA_E_OK)
        {
            if(pmac_send_debug || ma_debug)
            {
                ca_printf("aal_epon_mac_frame_send : write BUF0 return %d\n", ret);
            }

            goto frame_xg_exit;
        }

        /* set llid */
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_TXARB_OAM_TXBUF0_HDR_ONU1, Onu_llid1, (pkt->llid>>7));
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_TXARB_OAM_TXBUF0_HDR_ONU0, Onu_llid0, (pkt->llid&0x7F));
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_TXARB_OAM_TXBUF0_HDR_ONU0, Full_pkt_size0, pkt->len+4);

        /* set cos */
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_TXARB_OAM_TXBUF0_HDR_ONU0, Cos0, cos);

        /* to send the frame */
        ret = __pmac_xg_tx0_frame_trigger(device_id, port_id);
        if(ret != CA_E_OK){
            if(pmac_send_debug || ma_debug)
            {
                ca_printf("aal_epon_mac_frame_send: triger timeout!\n");
            }
            /* always return OK. The buffer is wrotten and translate is started */
        }
    }
    else
    {
        ret = CA_E_TIMEOUT;
        goto frame_xg_exit;
    }


#if 0
    else if (cfg->buf_id == 1)
    {
        if (!__pmac_xg_tx1_is_busy(device_id, port))
        {
            /* change frame content */
            if (mask & CA_AAL_PMAC_TX_FRM_MSK_PKT_BUFF)
            {
                ret = __pmac_xg_frame_buff_write(device_id, port, 1, cfg->buf, cfg->buf_len);
                if (ret != CA_E_OK)
                {
                    ca_printf("ca_sdl_pmac_frame_xg_send() : __pmac_xg_frame_buff_write() for BUF0 return %d\n", ret);
                    goto frame_xg_exit;
                }
            }

            /* llid type */
            if (mask & CA_AAL_PMAC_TX_FRM_MSK_LLID)
            {
                AAL_PON_REG_FIELD_WRITE(device_id, port, XGEPN_TXARB_OAM_TXBUF1_HDR_ONU1, Onu_llid1, (cfg->llid>>7));
                AAL_PON_REG_FIELD_WRITE(device_id, port, XGEPN_TXARB_OAM_TXBUF1_HDR_ONU0, Onu_llid0, (cfg->llid&7));
            }

            /* change frame size, incl. FCS */
            if (mask & CA_AAL_PMAC_TX_FRM_MSK_FRM_SIZE)
            {
                if ((cfg->pkt_len < (EPON_TX_MIN_FRM_LEN + 4)) || (cfg->pkt_len > EPON_TX_MAX_FRM_LEN))
                {
                    ret = CA_E_PARAM;
                    goto frame_xg_exit;
                }

                AAL_PON_REG_FIELD_WRITE(device_id, port, XGEPN_TXARB_OAM_TXBUF1_HDR_OLT0, Full_pkt_size0, cfg->pkt_len);
            }

            /* to send the frame */
            if (mask & CA_AAL_PMAC_TX_FRM_MSK_PKT_TRIG)
                __pmac_xg_tx1_frame_trigger(device_id, port);
        }
        else
        {
            ca_printf("dev %d, port %d, XG PMAC fast channel %d is busy\n", device_id, port, cfg->buf_id);
            goto frame_xg_exit;
        }
    }
#endif
    if(pmac_send_debug || ma_debug)
    {
        ca_printf("aal_epon_mac_frame_send: OK!\n");
    }

    return CA_E_OK;

frame_xg_exit:
    if(pmac_send_debug || ma_debug)
    {
        ca_printf("aal_epon_mac_frame_send: exit ret = %d! \n", ret);
    }
    return ret;
}
#endif

ca_status_t aal_epon_csr_reset(
    CA_IN ca_device_id_t device_id)
{
    GLOBAL_EPON_CNTL_t cntl;

    cntl.wrd = AAL_GLB_REG_READ(GLOBAL_EPON_CNTL);

    cntl.bf.epon_rst_n = 0;
    cntl.bf.onumpcp_rst_n = 0;
    cntl.bf.onugepn_rst_n = 0;
    cntl.bf.xgepn_rst_n = 0;
    cntl.bf.rx_coresec_10_rst_n = 0;
    cntl.bf.tx_coresec_10_rst_n = 0;
    cntl.bf.gepn_rx_rst_n = 0;
    cntl.bf.gepn_tx_rst_n = 0;
    cntl.bf.xgepn_rx_rst_n = 0;
    cntl.bf.xgepn_tx_rst_n = 0;
    AAL_GLB_REG_WRITE(cntl.wrd, GLOBAL_EPON_CNTL);

    ca_mdelay(100);
    cntl.bf.epon_rst_n = 1;
    cntl.bf.onumpcp_rst_n = 1;
    cntl.bf.onugepn_rst_n = 1;
    cntl.bf.xgepn_rst_n = 1;
    cntl.bf.rx_coresec_10_rst_n = 1;
    cntl.bf.tx_coresec_10_rst_n = 1;
    cntl.bf.gepn_rx_rst_n = 1;
    cntl.bf.gepn_tx_rst_n = 1;
    cntl.bf.xgepn_rx_rst_n = 1;
    cntl.bf.xgepn_tx_rst_n = 1;

    cntl.bf.epon_onu_mode      = 3; /* XG/XG ONU mode */
    cntl.bf.epon_ethernet_mode = 0;

    AAL_GLB_REG_WRITE(cntl.wrd, GLOBAL_EPON_CNTL);

    ca_mdelay(100);

    return CA_E_OK;
}

ca_status_t aal_epon_mac_init
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t mode
)
{
    aal_epon_laser_msk_t laser_mask;
    aal_epon_laser_cfg_t laser_cfg;
    aal_epon_mac_ctrl_msk_t  mac_mask;
    aal_epon_mac_ctrl_t      mac_cfg;
    aal_epon_mac_drop_msk_t  drop_mask;
    aal_epon_mac_drop_t      drop_cfg;
    ca_status_t rtn      = CA_E_ERROR;
    ca_uint32_t onu_type = AAL_ONU_TYPE_10G_10G;

    memset(&mac_mask,  0, sizeof(aal_epon_mac_ctrl_msk_t));
    memset(&mac_cfg,   0, sizeof(aal_epon_mac_ctrl_t));
    memset(&drop_mask, 0, sizeof(aal_epon_mac_drop_msk_t));
    memset(&drop_cfg,  0, sizeof(aal_epon_mac_drop_t));

    ca_printf("EPON-MAC: being initialized ... ");

    (void)aal_epon_mac_mib_rc_set(device_id, port_id, CA_FALSE);  /* PON MIB read-keep */

    /* configure link detection interval to 64 ms so as to avoid repeated link up/down */
    AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XGPCS_RX_XGEPN_LINK_DETECT_CFG, detect_period, __LINK_DETECT_INTERVAL_MAX);

    /* 1. set PON MAC, TX/RX ctrl, MIB */
    /* set ONU PON mac mode: EPON mode */
#if  defined(CONFIG_ARCH_CORTINA_SATURN_SFU)  || defined(CONFIG_ARCH_CORTINA_G3HGU)
    /* working on epon mode */
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_EPON_CNTL, epon_ethernet_mode, 0);

    /* XG PCS Rx initialization */
    AAL_PON_REG_WRITE(0x5000, EPON_XGEPN_XGPCS_RX_RXCNTRL);

    /* XG PCS Tx initialization */
    AAL_PON_REG_WRITE(0x3000000, EPON_XGEPN_XGPCS_TX_TXCNTRL);

    do
    {
        /* finetune RTT Rx & Tx compensation */
        EPON_ONU_LT_RTT_COMPENSATION_CFG_10G_RX_t rtt_rx_cfg;
        EPON_ONU_LT_RTT_COMPENSATION_CFG_10G_TX_t rtt_tx_cfg;

        rtt_rx_cfg.wrd = AAL_PON_REG_READ(EPON_ONU_LT_RTT_COMPENSATION_CFG_10G_RX);
        rtt_rx_cfg.bf.csrA = 0x59;
        rtt_rx_cfg.bf.csrB = 0x48;
        AAL_PON_REG_WRITE(rtt_rx_cfg.wrd, EPON_ONU_LT_RTT_COMPENSATION_CFG_10G_RX);

        rtt_tx_cfg.wrd = AAL_PON_REG_READ(EPON_ONU_LT_RTT_COMPENSATION_CFG_10G_TX);
        rtt_tx_cfg.bf.csrC = 0x18;
        rtt_tx_cfg.bf.csrD = 0x16;
        AAL_PON_REG_WRITE(rtt_tx_cfg.wrd, EPON_ONU_LT_RTT_COMPENSATION_CFG_10G_TX);

    } while(0);

#endif

    // TODO: tx time compensation implementation is not right in ASIC, don't use it
    AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_RXMAC_RX_LOCALTIME_ADJUST, adjust_loctimer, 0x10006);

    /* enable MPCP/laser/PCS tx  */
    laser_mask.u32                 = 0;
    laser_mask.s.tx_mode           = 1;
    laser_mask.s.tx_ctrl_en        = 1;
    laser_mask.s.tx_power          = 1;
    laser_mask.s.tx_power_polarity = 1;
    laser_mask.s.tx_burst_en       = 1;
    laser_mask.s.tx_delay_xg       = 1;

    laser_cfg.tx_mode           = AAL_EPON_LASER_MODE_BURST;     /* MPCP: normal burst mode */
    laser_cfg.tx_ctrl_en        = 0;                            /* laser: Disable ouput of signals TX_DIS */
    laser_cfg.tx_power          = 1;                            /* laser: power on transmiter  */
    laser_cfg.tx_power_polarity = 1;
    laser_cfg.tx_burst_en       = 1;                            /* laser: enable output of signals Tx_BURST */
    laser_cfg.tx_delay_xg       = 2;/* PCS: delay count register for laser on output. */

    rtn = aal_epon_laser_cfg_set(device_id, port_id, laser_mask, &laser_cfg);
    if (rtn != CA_E_OK)
    {
        ca_printf("failed to invoke aal_epon_laser_cfg_set(%d), FILE(%s), LINE(%d).\n", rtn, __FILE__, __LINE__);
        return rtn;
    }

    /* set XMAC CTRL rx drop */
    drop_cfg.invalid_llid_drop  = 1;
    drop_cfg.fcs_err_drop       = 1;
    drop_cfg.oversize_drop      = 1;
    drop_cfg.runtsize_drop      = 1;
    drop_cfg.pause_drop         = 1;
    drop_cfg.unknown_op_drop    = 1;
    drop_cfg.mpcp_drop          = 1;              /* enable drop mpcp frame in data fifo */
    drop_cfg.mpcp_ext_drop      = 1;              /* enable drop extension mpcp frame in mpcp fifo */
    drop_cfg.overflow_drop      = 1;              /* enable drop frame if fifo is overflow */
    drop_cfg.unknown_ds_drop    = 1;              /* DEFAULT 0. */
    drop_cfg.oam_sec_drop       = 0;              /* disable drop exchange key OAM frame */
    drop_mask.u32                  = 0;
    drop_mask.s.invalid_llid_drop  = 1;
    drop_mask.s.fcs_err_drop       = 1;
    drop_mask.s.oversize_drop      = 1;
    drop_mask.s.runtsize_drop      = 1;
    drop_mask.s.pause_drop         = 1;
    drop_mask.s.unknown_op_drop    = 1;
    drop_mask.s.mpcp_drop          = 1;
    drop_mask.s.mpcp_ext_drop      = 1;
    drop_mask.s.overflow_drop      = 1;
    drop_mask.s.unknown_ds_drop    = 1;
    drop_mask.s.oam_sec_drop       = 1;
    rtn = aal_epon_mac_drop_set(device_id, port_id, drop_mask, &drop_cfg);
    if (rtn != CA_E_OK)
    {
        ca_printf("failed to invoke aal_epon_mac_drop_set(%d), FILE(%s), LINE(%d).\n", rtn, __FILE__, __LINE__);
        return rtn;
    }

    if (mode == ONU_PON_MAC_EPON_1G)
		onu_type = AAL_ONU_TYPE_1G_1G;
    else if (mode == ONU_PON_MAC_EPON_D10G)
		onu_type = AAL_ONU_TYPE_1G_10G;
    else
		onu_type = AAL_ONU_TYPE_10G_10G;

    /* disable XMAC RXMAC/TXMAC */
    mac_cfg.onu_type = onu_type;
    mac_cfg.rx_xg_ena              = CA_FALSE;
    mac_cfg.tx_xg_ena              = CA_FALSE;
    mac_cfg.rx_xg_psize_max           = AAL_MTU_DEFAULT;
    mac_cfg.rx_xg_flowctrl_en         = 1;
    mac_cfg.tx_xg_ipg              = 12;
    mac_cfg.rx_1g_ena              = CA_FALSE;
    mac_cfg.tx_1g_ena              = CA_FALSE;
    mac_cfg.tx_1g_ipg              = 12;
    mac_cfg.tx_1g_psize_max_ena    = 0;
    mac_cfg.tx_1g_psize_max        = AAL_MTU_DEFAULT;
    mac_cfg.tx_1g_8b_preamble_ena  = 0;                          /* reserved_1 ??? */
    mac_cfg.tx_1g_crc8_order       = AAL_PON_CRC8_MODE_NORMAL;
    mac_cfg.tx_1g_crc32_regen_en   = 1;                          /* all packets except internally transmission error packet is replaced with regenerated FCS.*/
    mac_cfg.tx_1g_drain_ena        = 0;                          /* drop all packets from BM */

    mac_mask.u32                      = 0;
    mac_mask.s.onu_type               = 1;
    mac_mask.s.rx_xg_ena			  = 1;
    mac_mask.s.tx_xg_ena              = 1;
    mac_mask.s.tx_xg_ipg              = 1;
    mac_mask.s.rx_xg_psize_max        = 1;
    mac_mask.s.rx_xg_flowctrl_en      = 1;
    mac_mask.s.rx_1g_ena              = 1;
    mac_mask.s.tx_1g_ena              = 1;
    mac_mask.s.tx_1g_ipg              = 1;
    mac_mask.s.tx_1g_psize_max_ena    = 1;
    mac_mask.s.tx_1g_psize_max        = 1;
    mac_mask.s.tx_1g_8b_preamble_ena  = 1;
    mac_mask.s.tx_1g_crc8_order       = 1;
    mac_mask.s.tx_1g_crc32_regen_en   = 1;
    mac_mask.s.tx_1g_drain_ena        = 1;

    rtn = aal_epon_mac_cfg_set(device_id, port_id, mac_mask, &mac_cfg);
    if (rtn != CA_E_OK)
    {
        ca_printf("failed to invoke aal_epon_mac_cfg_set(%d), FILE(%s), LINE(%d).\n", rtn, __FILE__, __LINE__);
        return rtn;
    }

    //AAL_PON_REG_WRITE(0x1C40C038, EPON_XGEPN_XMAC_TXMAC_TX_CONFIG);
    //AAL_PON_REG_WRITE(0x5ffe0290, EPON_XGEPN_XMAC_RXMAC_RX_CONFIG);

    /* reset MIB */
    rtn = __pmac_mib_init(device_id, port_id);
    if (rtn != CA_E_OK)
    {
        ca_printf("failed to invoke __pmac_mib_init(%d), FILE(%s), LINE(%d).\n", rtn, __FILE__, __LINE__);
        return rtn;
    }

    /* MAC ctrl frame read-clear */
    AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_CTRL_RX_MPCP_MIB0, clr, 1);
    AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_XMAC_CTRL_TX_MPCP_MIB0, clr, 1);


    // TODO: 2. enable TX/RX of transceiver by setting GPIO
    //cs_gpio_mode_set(device_id, GPIO_PIN6, GPIO_MODE_OUT);   /* Laser Tx power */
    //cs_gpio_mode_set(device_id, GPIO_PIN7, GPIO_MODE_OUT);   /* Laser Rx power */

    //cs_gpio_write(device_id, GPIO_PIN6, 1);                  /* Enable laser Tx power */
    //cs_gpio_write(device_id, GPIO_PIN7, 1);                  /* Enable laser Rx power */

    /* 3.  enable TX_DIS signal */
    #if 0
    laser_cfg.tx_power_polarity      = 1;
    laser_cfg.tx_ctrl_en             = 1;
    laser_mask.u32                   = 0;
    laser_mask.s.tx_power_polarity   = 1;
    laser_mask.s.tx_ctrl_en          = 1;
    rtn = aal_epon_mac_laser_cfg_set(0,0,laser_mask,&laser_cfg);
    if (rtn != CA_E_OK)
    {
        ca_printf("failed to invoke aal_epon_mac_laser_cfg_set(%d), FILE(%s), LINE(%d).\n", rtn, __FILE__, __LINE__);
        return rtn;
    }
    #endif

    ca_printf("done.\n");

    return CA_E_OK;
}

EXPORT_SYMBOL(aal_epon_mac_init);
EXPORT_SYMBOL(aal_epon_mac_mib_rc_set);
EXPORT_SYMBOL(aal_epon_mac_mib_rc_get);
EXPORT_SYMBOL(aal_epon_mac_cfg_set);
EXPORT_SYMBOL(aal_epon_mac_cfg_get);
EXPORT_SYMBOL(aal_epon_mac_onu_type_get);
EXPORT_SYMBOL(aal_epon_mac_drop_set);
EXPORT_SYMBOL(aal_epon_mac_drop_get);
EXPORT_SYMBOL(aal_epon_mac_rst_ctrl_set);
EXPORT_SYMBOL(aal_epon_mac_rst_ctrl_get);
EXPORT_SYMBOL(aal_epon_mac_stats_llid_mpcp_get);
EXPORT_SYMBOL(aal_epon_mac_stats_llid_eth_get);
EXPORT_SYMBOL(aal_epon_mac_stats_port_mpcp_get);
EXPORT_SYMBOL(aal_epon_mac_stats_port_mpcp_ctrl_get);
EXPORT_SYMBOL(aal_epon_mac_stats_port_eth_get);
EXPORT_SYMBOL(aal_epon_mac_1g_link_status_get);
EXPORT_SYMBOL(aal_epon_mac_xg_link_status_get);
EXPORT_SYMBOL(aal_epon_csr_reset);
