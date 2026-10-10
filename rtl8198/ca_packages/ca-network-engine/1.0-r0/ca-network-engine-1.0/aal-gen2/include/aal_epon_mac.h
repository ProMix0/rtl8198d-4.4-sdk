
#ifndef __AAL_EPON_MAC_H__
#define __AAL_EPON_MAC_H__

#include "ca_types.h"

#define AAL_MTU_DEFAULT            1700

/* EPON onu mode.	11: 10G down, 10G up; 10: 10G down, 1G up; 01: 1G down, 1G up */
typedef enum {
    AAL_ONU_TYPE_1G_1G                        = 1, /* upstream 1G downstream 1G  */
    AAL_ONU_TYPE_1G_10G                        = 2, /* upstream 1G downstream 10G  */
    AAL_ONU_TYPE_10G_10G                       = 3  /* upstream 10G downstream 10G */
} aal_onu_type_t;

typedef enum
{
    AAL_PON_CRC8_MODE_NORMAL                   = 0, /* not reverse crc8, not reverse data order   */
    AAL_PON_CRC8_MODE_REV_DATA                 = 1, /* not reverse crc8, reverse data order       */
    AAL_PON_CRC8_MODE_REV_CRC8                 = 2, /* reverse crc8, not reverse data order       */
    AAL_PON_CRC8_MODE_REV_CRC8_DATA            = 3, /* reverse crc8, reverse data order           */
} aal_epon_mac_crc8_order_t;

typedef enum {
    AAL_SPI_BUS_1G = 0,
    AAL_SPI_BUS_1G_ATDC,
    AAL_SPI_BUS_COM,
    AAL_SPI_BUS_XG,
    AAL_SPI_BUS_MAX = AAL_SPI_BUS_XG
} aal_spi_bus_t;

typedef enum {
    AAL_SDS_CTRL_SIGNAL_SW = 0,
    AAL_SDS_CTRL_SIGNAL_INTERNAL,
    AAL_SDS_CTRL_SIGNAL_EXTERNAL,
    AAL_SDS_CTRL_SIGNAL_MAX = AAL_SDS_CTRL_SIGNAL_EXTERNAL
} aal_sds_ctrl_signal_t;

typedef volatile union {
    struct {
#ifdef CA_BIG_ENDIAN
        ca_uint32_t rsrvd2                   : 11 ;
        ca_uint32_t JunkEndofBurst_notfound  :  1 ; /* bits 20:20 */
        ca_uint32_t ksds_win_bmen_err        :  1 ; /* bits 19:19 */
        ca_uint32_t linkup                   :  1 ; /* bits 18:18 */
        ca_uint32_t linkdown                 :  1 ; /* bits 17:17 */
        ca_uint32_t EndofBurst_notfound      :  1 ; /* bits 16:16 */
        ca_uint32_t BurstDelimiter_notfound  :  1 ; /* bits 15:15 */
        ca_uint32_t fecdecpersistfail        :  1 ; /* bits 14:14 */
        ca_uint32_t fecdecuncorrsat          :  1 ; /* bits 13:13 */
        ca_uint32_t fecdeccorrsat            :  1 ; /* bits 12:12 */
        ca_uint32_t patsync                  :  1 ; /* bits 11:11 */
        ca_uint32_t rsrvd1                   :  1 ;
        ca_uint32_t berhigh                  :  1 ; /* bits 9:9 */
        ca_uint32_t syncdet                  :  1 ; /* bits 8:8 */
        ca_uint32_t errorbersat              :  1 ; /* bits 7:7 */
        ca_uint32_t errorpatsat              :  1 ; /* bits 6:6 */
        ca_uint32_t errorpat                 :  1 ; /* bits 5:5 */
        ca_uint32_t errordecsat              :  1 ; /* bits 4:4 */
        ca_uint32_t errordec                 :  1 ; /* bits 3:3 */
        ca_uint32_t goodpktsat               :  1 ; /* bits 2:2 */
        ca_uint32_t errorpktsat              :  1 ; /* bits 1:1 */
        ca_uint32_t errorpktI                :  1 ; /* bits 0:0 */
#else
        ca_uint32_t errorpkt                 :  1 ; /* bits 0:0 */
        ca_uint32_t errorpktsat              :  1 ; /* bits 1:1 */
        ca_uint32_t goodpktsat               :  1 ; /* bits 2:2 */
        ca_uint32_t errordec                 :  1 ; /* bits 3:3 */
        ca_uint32_t errordecsat              :  1 ; /* bits 4:4 */
        ca_uint32_t errorpat                 :  1 ; /* bits 5:5 */
        ca_uint32_t errorpatsat              :  1 ; /* bits 6:6 */
        ca_uint32_t errorbersat              :  1 ; /* bits 7:7 */
        ca_uint32_t syncdet                  :  1 ; /* bits 8:8 */
        ca_uint32_t berhigh                  :  1 ; /* bits 9:9 */
        ca_uint32_t rsrvd1                   :  1 ;
        ca_uint32_t patsync                  :  1 ; /* bits 11:11 */
        ca_uint32_t fecdeccorrsat            :  1 ; /* bits 12:12 */
        ca_uint32_t fecdecuncorrsat          :  1 ; /* bits 13:13 */
        ca_uint32_t fecdecpersistfail        :  1 ; /* bits 14:14 */
        ca_uint32_t BurstDelimiter_notfound  :  1 ; /* bits 15:15 */
        ca_uint32_t EndofBurst_notfound      :  1 ; /* bits 16:16 */
        ca_uint32_t linkdown                 :  1 ; /* bits 17:17 */
        ca_uint32_t linkup                   :  1 ; /* bits 18:18 */
        ca_uint32_t ksds_win_bmen_err        :  1 ; /* bits 19:19 */
        ca_uint32_t JunkEndofBurst_notfound  :  1 ; /* bits 20:20 */
        ca_uint32_t rsrvd2                   : 11 ;
#endif
    } s ;
    ca_uint32_t     u32 ;
} aal_pon_pca_rx_intr_t;
#if 0
typedef union
{
    struct
    {
        ca_uint32_t   sw_rx_fifo_1g_reset     : 1;
        ca_uint32_t   sw_set_idle_1g          : 1;
        ca_uint32_t   sw_bmen_1g              : 1;
        ca_uint32_t   sw_auto_disco_1g        : 1;
        ca_uint32_t   sw_rx_fifo_10g_reset    : 1;
        ca_uint32_t   sw_set_idle_10g         : 1;
        ca_uint32_t   sw_bmen_10g             : 1;
        ca_uint32_t   sw_auto_disco_10g       : 1;

        ca_uint32_t   rx_fifo_1g_reset_src    : 1;
        ca_uint32_t   set_idle_1g_src         : 1;
        ca_uint32_t   bmen_1g_src             : 1;
        ca_uint32_t   auto_disco_1g_src       : 1;
        ca_uint32_t   rx_fifo_10g_reset_src   : 1;
        ca_uint32_t   set_idle_10g_src        : 1;
        ca_uint32_t   bmen_10g_src            : 1;
        ca_uint32_t   auto_disco_10g_src      : 1;

        ca_uint32_t   spi_reset               : 1;
        ca_uint32_t   tx_fifo_1g_reset        : 1;
        ca_uint32_t   tx_fifo_10g_reset       : 1;
        ca_uint32_t   tx_clkdiv_10g_reset     : 1;
        ca_uint32_t   rx_clkdiv_10g_reset     : 1;
        ca_uint32_t   mpif_1g_reset           : 1;
        ca_uint32_t   mpif_10g_reset          : 1;
        ca_uint32_t   pon_1g_reset            : 1;

        ca_uint32_t   pon_10g_reset           : 1;
        ca_uint32_t   tx_flip                 : 1;
        ca_uint32_t   rx_flip                 : 1;

    } s;
    ca_uint32_t u32;
}aal_pon_sds_cfg_msk_t;

typedef struct
{
    ca_boolean_t sw_rx_fifo_1g_reset;
    ca_boolean_t sw_set_idle_1g;
    ca_boolean_t sw_bmen_1g;
    ca_boolean_t sw_auto_disco_1g;
    ca_boolean_t sw_rx_fifo_10g_reset;
    ca_boolean_t sw_set_idle_10g;
    ca_boolean_t sw_bmen_10g;
    ca_boolean_t sw_auto_disco_10g;

    aal_sds_ctrl_signal_t rx_fifo_1g_reset_src;
    aal_sds_ctrl_signal_t set_idle_1g_src;
    aal_sds_ctrl_signal_t bmen_1g_src;
    aal_sds_ctrl_signal_t auto_disco_1g_src;
    aal_sds_ctrl_signal_t rx_fifo_10g_reset_src;
    aal_sds_ctrl_signal_t set_idle_10g_src;
    aal_sds_ctrl_signal_t bmen_10g_src;
    aal_sds_ctrl_signal_t auto_disco_10g_src;

    ca_boolean_t spi_reset;
    ca_boolean_t tx_fifo_1g_reset;
    ca_boolean_t tx_fifo_10g_reset;
    ca_boolean_t tx_clkdiv_10g_reset;
    ca_boolean_t rx_clkdiv_10g_reset;
    ca_boolean_t mpif_1g_reset;
    ca_boolean_t mpif_10g_reset;
    ca_boolean_t pon_1g_reset;
    ca_boolean_t pon_10g_reset;

    ca_boolean_t tx_flip;
    ca_boolean_t rx_flip;

} aal_pon_sds_cfg_t;

typedef struct
{
    ca_uint8_t cpll_lock;
    ca_uint8_t dpll_lock;
    ca_uint8_t uplk_link_on;
    ca_uint8_t cscd_link_on;
    ca_uint8_t rx_fifo_err_ep;
    ca_uint8_t rx_fifo_err_xep;
    ca_uint8_t rx_lock_xep;
} aal_pon_sds_status_t;
#endif
typedef struct
{
    ca_boolean_t   byp_ena;             /* (default=0) bypass receiving 10b/8b decoding function in PCS                                 */
    ca_boolean_t   fec_sync_ena;        /* (default=0) Uses PCS synchronization state machine or PCS FEC synchronization state machine  */
    ca_boolean_t   fec_sync_dw_ena;     /* (default=0) Uses PCS FEC sync state machine for discovery window                             */
    ca_boolean_t   tbi_align_ena;       /* (default=0) Enable TBI code alignment function                                               */
} aal_epon_mac_pca_t;

typedef union
{
    struct
    {
        ca_uint32_t   byp_ena          : 1;
        ca_uint32_t   fec_sync_ena     : 1;
        ca_uint32_t   fec_sync_dw_ena  : 1;
        ca_uint32_t   tbi_align_ena    : 1;
    } s;
    ca_uint32_t u32;
}aal_epon_mac_pca_msk_t;

typedef struct
{
    ca_uint8_t                  onu_type               ; /* 1G-10G or 10G-10G                                                */
    ca_boolean_t                rx_xg_ena              ; /* (default=false) Enable/Disable XG RX MAC port                    */
    ca_boolean_t                tx_xg_ena              ; /* (default=false) Enable/Disable XG TX MAC port                    */

    ca_uint32_t                 rx_xg_psize_max        ; /* (default=2039) The max length of the packet for RX               */
    ca_boolean_t                rx_xg_flowctrl_en      ; /* (default=1) Enable receiving pause frame and stop transimition   */
    ca_uint8_t                  tx_xg_ipg              ; /* (default=12) Should be 8 or 12 only                              */
    ca_boolean_t                tx_xg_crc32_regen_en   ; /* enable crc32 regeneration                                        */
    ca_uint16_t                 tx_xg_thold_sfifo_rd   ; /* (default=15) 0~512, read threshold in the sync fifo of tx ctrl   */
    ca_uint16_t                 tx_xg_thold_sfifo_wr   ; /* (default=15) 0~512, write threshold in the sync fifo of tx ctrl  */
    ca_uint8_t                  tx_xg_thold_asfifo_rd  ; /* (default=3) 0~15, read threshold in the async fifo of tx part    */
    ca_uint8_t                  tx_xg_thold_asfifo_wr  ; /* (default=3) 0~15, write threshold in the async fifo of tx part   */

    ca_boolean_t                max_mtu_tag_en         ; /* for epon rx. NOTE:  when
                                                            1. CSR bit max_mtu_tag_en is set, and
                                                            2. ingress frame has tag and tag pattern matches one of  0x8100, 0x9100 or CSR
                                                            mtu_tag_pattern, and
                                                            3. ingress frame size is no more than 14'h3FFB

                                                            then the max packet size will increase by 4.*/
    ca_boolean_t                rx_1g_ena              ; /* (default=false) Enable/Disable 1G RX MAC port                    */
    ca_boolean_t                tx_1g_ena              ; /* enable MAC transimit                                             */

    ca_uint32_t                 rx_1g_psize_max        ; /* (default=2039) The max length of the packet for RX               */
    ca_boolean_t                tx_1g_psize_max_ena    ; /* Left data will be cut off but still counted into packet size   */
    ca_uint32_t                 tx_1g_psize_max        ; /* configure tx mtu, 0~2047B                                        */
    ca_uint8_t                  tx_1g_ipg              ; /* IPG setting: 10~13, unit is byte                                 */
    ca_boolean_t                tx_1g_8b_preamble_ena  ; /* 1-transimit data with fixing 8B preamble, 0-normal               */
    ca_uint8_t                  tx_1g_crc8_order       ; /* reverse crc8 and data or not                                     */
    ca_boolean_t                tx_1g_crc32_regen_en   ; /* enable crc32 regeneration                                        */
    ca_boolean_t                tx_1g_drain_ena        ; /* enable drain out data from BM b dropping                         */
    ca_boolean_t                rx2tx_lookback         ;
    ca_boolean_t                tx2rx_lookback         ;
} aal_epon_mac_ctrl_t;

typedef union {
    struct {
        ca_uint32_t   onu_type               : 1;
        ca_uint32_t   rx_xg_ena              : 1;
        ca_uint32_t   tx_xg_ena              : 1;
        ca_uint32_t   rx_xg_psize_max        : 1;
        ca_uint32_t   rx_xg_flowctrl_en      : 1;
        ca_uint32_t   tx_xg_ipg              : 1;
        ca_uint32_t   tx_xg_crc32_regen_en   : 1;
        ca_uint32_t   tx_xg_thold_sfifo_rd   : 1;
        ca_uint32_t   tx_xg_thold_sfifo_wr   : 1;
        ca_uint32_t   tx_xg_thold_asfifo_rd  : 1;
        ca_uint32_t   tx_xg_thold_asfifo_wr  : 1;

        ca_uint32_t   max_mtu_tag_en         : 1;
        ca_uint32_t   rx_1g_ena              : 1;
        ca_uint32_t   tx_1g_ena              : 1;
        ca_uint32_t   rx_1g_psize_max        : 1;
        ca_uint32_t   tx_1g_psize_max_ena    : 1;
        ca_uint32_t   tx_1g_psize_max        : 1;
        ca_uint32_t   tx_1g_ipg              : 1;
        ca_uint32_t   tx_1g_8b_preamble_ena  : 1;
        ca_uint32_t   tx_1g_crc8_order       : 1;
        ca_uint32_t   tx_1g_crc32_regen_en   : 1;
        ca_uint32_t   tx_1g_drain_ena        : 1;
        ca_uint32_t   rx2tx_lookback         : 1;
        ca_uint32_t   tx2rx_lookback         : 1;
    } s;
    ca_uint32_t u32;
} aal_epon_mac_ctrl_msk_t;



typedef volatile union {
  struct {
#ifdef CA_BIG_ENDIAN
    ca_uint32_t reserved2               : 13 ;
    ca_uint32_t linkup                  :  1 ; /* bits 18 */
    ca_uint32_t linkdown                :  1 ; /* bits 17 */
    ca_uint32_t reserved1               : 17 ;
#else /* CA_LITTLE_ENDIAN */
    ca_uint32_t reserved1               : 17 ;
    ca_uint32_t linkdown                :  1 ; /* bits 17 */
    ca_uint32_t linkup                  :  1 ; /* bits 18 */
    ca_uint32_t reserved2               : 13 ;
#endif
  } s ;
  ca_uint32_t     u32;
} aal_xgpon_pca_rx_intr_t;

typedef struct
{
    ca_boolean_t xg_pcs_rx_rst;
    ca_boolean_t xg_pcs_tx_rst;
    ca_boolean_t rx_mac_rst;
    ca_boolean_t tx_mac_rst;
} aal_epon_mac_rst_ctrl_t;

typedef union
{
    struct
    {
        ca_uint32_t   xg_pcs_rx_rst  : 1;
        ca_uint32_t   xg_pcs_tx_rst  : 1;
        ca_uint32_t   rx_mac_rst     : 1;
        ca_uint32_t   tx_mac_rst     : 1;

    } s;
    ca_uint32_t u32;
}aal_epon_mac_rst_ctrl_msk_t;

typedef struct
{
    ca_uint32_t rx_epon_regstrcnt;      /* MPCP Register frame to this LLID (acc_data[23:16])                 */
    ca_uint32_t rx_epon_ucgatecnt;      /* MPCP normal Gate frame to this LLID (acc_data[15:0])               */
    ca_uint32_t rx_mpcp_opivldcnt;      /* MAC control frame but opcode is not 01~06(acc_data[31:16])         */
    ca_uint32_t rx_epon_oamfrmcnt;      /* frames sent by SW to this LLID(acc_data[15:0])                     */
    ca_uint32_t tx_epon_regreqcnt;      /* MPCP Request frame counter (acc_data[31:24])                       */
    ca_uint32_t tx_epon_regackcnt;      /* MPCP Acknowledge frame counter (acc_data[23:16])                   */
    ca_uint32_t tx_epon_reportcnt;      /* MPCP Report frame counter (acc_data[15:0])                         */
    ca_uint32_t tx_mpcp_opivldcnt;      /* MAC control frame but opcode is not 01~06(acc_data[31:16])         */
    ca_uint32_t tx_epon_oamfrmcnt;      /* frames received by SW to this LLID(acc_data[15:0])                 */

}aal_epon_mac_stats_llid_mpcp_t;


typedef struct
{
    ca_uint32_t rx_epon_bytecnt_lo;             /* low-32bit of receive frame byte counter                                  */
    ca_uint32_t rx_epon_bytecnt_hi;             /* high-32bit of receive frame byte counter                                 */
    ca_uint32_t rx_epon_mcframecnt;             /* multicast frame counter                                                  */
    ca_uint32_t rx_epon_bcframecnt;             /* broadcast frame counter                                                  */
    ca_uint32_t rx_epon_undersizecnt;           /* number of frames received with less than 64 octets                       */
    ca_uint32_t rx_epon_oversizecnt;            /* number of frames received with less than max length                      */
    ca_uint32_t rx_epon_ramecnt;                /* total number of frame counter; include good or bad frame                 */
    ca_uint32_t rx_epon_rmon64cnt;              /* 64-byte frame (good or bad)                                              */
    ca_uint32_t rx_epon_rmon65_127cnt;          /* frame (good or bad) with size between 65 and 127 bytes                   */
    ca_uint32_t rx_epon_rmon128_255cnt;         /* frame (good or bad) with size between 128 and 255 bytes                  */
    ca_uint32_t rx_epon_rmon256_511cnt;         /* frame (good or bad) with size between 256 and 511 bytes                  */
    ca_uint32_t rx_epon_rmon512_1023cnt;        /* frame (good or bad) with size between 512 and 1023 bytes                 */
    ca_uint32_t rx_epon_rmon1024_1518cnt;       /* frame (good or bad) with size between 1024 and 1518 bytes;               */
    ca_uint32_t rx_epon_rmon1519_1533cnt;       /* frame (good or bad) with size between 1519 and 1533 bytes;               */
    ca_uint32_t rx_epon_rmon1534_2299cnt;       /* frame (good or bad) with size between 1534 and 2299 bytes;               */
    ca_uint32_t rx_epon_rmon2300_2599cnt;       /* frame (good or bad) with size between 2300 and 2599 bytes;               */
    ca_uint32_t rx_epon_rmon2600_maxcnt;        /* frame (good or bad) with size between 2600 andconfigured maximum value;
                                               * default is 12K bytes                                                    */
    ca_uint32_t rx_epon_runtcnt;                /* number of frames received with less than 64 octets and CRC error         */
    ca_uint32_t rx_epon_jabbercnt;              /* number of frames received that exceeds the programmed max frame length
                                               * and CRC ERROR                                                            */
    ca_uint32_t rx_epon_fcscnt;                 /* number of frames received with fcs error                                 */
    ca_uint32_t rx_epon_overflowcnt;            /* number of frames flushed in rxtrl because of fifo overflow               */
    ca_uint32_t tx_epon_bytecnt_lo;             /* low-32bit of receive frame byte counter ; include good or bad frame      */
    ca_uint32_t tx_epon_bytecnt_hi;             /* high-32bit of receive frame byte counter; include good or bad frame      */
    ca_uint32_t tx_epon_mcframecnt;             /* multicast frame counter                                                  */
    ca_uint32_t tx_epon_bcframecnt;             /* broadcast frame counter                                                  */
    ca_uint32_t tx_epon_ramecnt;                /* total number of frame counter; include good or bad frame                 */
    ca_uint32_t tx_epon_rmon64cnt;              /* 64-byte frame (good or bad)                                              */
    ca_uint32_t tx_epon_rmon65_127cnt;          /* frame (good or bad) with size between 65 and 127 bytes                   */
    ca_uint32_t tx_epon_rmon128_255cnt;         /* frame (good or bad) with size between 128 and 255 bytes                  */
    ca_uint32_t tx_epon_rmon256_511cnt;         /* frame (good or bad) with size between 256 and 511 bytes                  */
    ca_uint32_t tx_epon_rmon512_1023cnt;        /* frame (good or bad) with size between 512 and 1023 bytes                 */
    ca_uint32_t tx_epon_rmon1024_1518cnt;       /* frame (good or bad) with size between 1024 and 1518 bytes                */
    ca_uint32_t tx_epon_rmon1519_1533cnt;       /* frame (good or bad) with size between 1519 and 1533 bytes;               */
    ca_uint32_t tx_epon_rmon1534_2299cnt;       /* frame (good or bad) with size between 1534 and 2299 bytes;               */
    ca_uint32_t tx_epon_rmon2300_2599cnt;       /* frame (good or bad) with size between 2300 and 2599 bytes;               */
    ca_uint32_t tx_epon_rmon2600_maxcnt;        /* frame (good or bad) with size between 2600 andconfigured maximum value;
                                               * default is 12K bytes                                                                                             */
}aal_epon_mac_stats_llid_eth_t;

typedef struct
{
    ca_uint32_t rx_epon_crc8errcnt;         /* received frame with CRC8 error                              */
    ca_uint32_t rx_epon_bccrc8vldfcserrcnt; /* frame with correct crc8 but wrong FCS; with broadcast llid  */
    ca_uint32_t rx_epon_bcregstrcnt;        /* broadcast register request frame                            */
    ca_uint32_t rx_epon_bcgatecnt;          /* MPCP discovery Gate frame with broadcast llid               */
    ca_uint32_t rx_epon_extencnt;           /* received extension MPCP frame counter                       */
    ca_uint32_t tx_epon_bcregreqcnt;        /* MPCP Register frame with broadcast llid                     */
    ca_uint32_t tx_epon_bccrc8vldfcserrcnt; /* frame with correct crc8 but wrong FCS; with broadcast llid  */
    ca_uint32_t tx_epon_extencnt;           /* transmited extension MPCP frame counter                     */

}aal_epon_mac_stats_port_mpcp_t;

typedef struct
{
    ca_uint32_t	mpcp_mac_ctrl_frames_transmitted;
    ca_uint32_t	mpcp_mac_ctrl_frames_received;
    ca_uint32_t	mpcp_tx_reg_ack;
}aal_epon_mac_stats_port_mpcp_ctrl_t;

typedef struct
{
    ca_uint32_t rx_bytecnt_lo;         /* frame byte count; low-32bit                                                      */
    ca_uint32_t rx_bytecnt_hi;         /* frame byte count; high-32bit                                                     */
    ca_uint32_t rx_crcerrcnt ;         /* frame with fcs error (only active when receive port)                             */
    ca_uint32_t rx_ucframecnt;         /* unicast frame count                                                              */
    ca_uint32_t rx_mcframecnt;         /* multicast frame                                                                  */
    ca_uint32_t rx_bcframecnt;         /* broadcast frame                                                                  */
    ca_uint32_t rx_oamframecnt;        /* oam frame                                                                        */
    ca_uint32_t rx_pausecnt;           /* pause frame                                                                      */
    ca_uint32_t rx_unknownopcnt;       /* mac control frame with opcode not 01~06 (only for receiver port)                 */
    ca_uint32_t rx_undersizecnt ;      /* frame with size less than 64 bytes (only for receiver port)                      */
    ca_uint32_t rx_oversizecnt;        /* frame with size more than configured value 2047 bytes (only for receiver port)   */
    ca_uint32_t rx_rmon64cnt;          /* 64-byte frame                                                                    */
    ca_uint32_t rx_rmon65_127cnt;      /* frame with size between 65 and 127 bytes                                         */
    ca_uint32_t rx_rmon128_255cnt;     /* frame with size between 128 and 255 bytes                                        */
    ca_uint32_t rx_rmon256_511cnt;     /* frame with size between 256 and 511 bytes                                        */
    ca_uint32_t rx_rmon512_1023cnt;    /* frame with size between 512 and 1023 bytes                                       */
    ca_uint32_t rx_rmon1024_1518cnt;   /* frame with size between 1024 and 1518 bytes;                                     */
    ca_uint32_t rx_rmon1519_1533cnt;   /* frame with size between 1519 and 1533 bytes;               */
    ca_uint32_t rx_rmon1534_2299cnt;   /* frame with size between 1534 and 2299 bytes;               */
    ca_uint32_t rx_rmon2300_2599cnt;   /* frame with size between 2300 and 2599 bytes;               */
    ca_uint32_t rx_rmon2600_maxcnt;    /* frame with size between 2600 andconfigured maximum value; default is 12K bytes   */
    ca_uint32_t tx_bytecnt_lo;         /* frame byte count; low-32bit                                                      */
    ca_uint32_t tx_bytecnt_hi;         /* frame byte count; high-32bit                                                     */
    ca_uint32_t tx_crcerrcnt ;         /* frame with fcs error (only active when receive port)                             */
    ca_uint32_t tx_ucframecnt;         /* unicast frame count                                                              */
    ca_uint32_t tx_mcframecnt;         /* multicast frame                                                                  */
    ca_uint32_t tx_bcframecnt;         /* broadcast frame                                                                  */
    ca_uint32_t tx_oamframecnt;        /* oam frame                                                                        */
    ca_uint32_t tx_pausecnt;           /* pause frame                                                                      */
    ca_uint32_t tx_unknownopcnt;       /* mac control frame with opcode not 01~06 (only for receiver port)                 */
    ca_uint32_t tx_undersizecnt ;      /* frame with size less than 64 bytes (only for receiver port)                      */
    ca_uint32_t tx_oversizecnt;        /* frame with size more than configured value 2047 bytes (only for receiver port)   */
    ca_uint32_t tx_rmon64cnt;          /* 64-byte frame                                                                    */
    ca_uint32_t tx_rmon65_127cnt;      /* frame with size between 65 and 127 bytes                                         */
    ca_uint32_t tx_rmon128_255cnt;     /* frame with size between 128 and 255 bytes                                        */
    ca_uint32_t tx_rmon256_511cnt;     /* frame with size between 256 and 511 bytes                                        */
    ca_uint32_t tx_rmon512_1023cnt;    /* frame with size between 512 and 1023 bytes                                       */
    ca_uint32_t tx_rmon1024_1518cnt;   /* frame with size between 1024 and 1518 bytes;                                     */
    ca_uint32_t tx_rmon1519_1533cnt;   /* frame with size between 1519 and 1533 bytes;                                     */
    ca_uint32_t tx_rmon1534_2299cnt;   /* frame with size between 1534 and 2299 bytes;                                     */
    ca_uint32_t tx_rmon2300_2599cnt;   /* frame with size between 2300 and 2599 bytes;                                     */
    ca_uint32_t tx_rmon2600_maxcnt;    /* frame with size between 2600 andconfigured maximum value; default is 12K bytes   */
}aal_epon_mac_stats_port_eth_t;


typedef struct
{
    ca_boolean_t  invalid_llid_drop; /* (default=1) enable to drop packet which invalid LLID                      */
    ca_boolean_t  fcs_err_drop;      /* (default=1) Packet with CRC32 error may not be dropped by MAC             */
    ca_boolean_t  oversize_drop;     /* (default=1) Packet with oversize length will be dropped by MAC            */
    ca_boolean_t  runtsize_drop;     /* (default=1) Packet with runt length will be dropped by MAC                */
    ca_boolean_t  pause_drop;        /* (default=1) Pause frame will be dropped                                   */
    ca_boolean_t  unknown_op_drop;   /* (default=1) Drop MAC control frame with unknown opcode                    */
    ca_boolean_t  mpcp_drop;         /* (default=1) MPCP frame will be dropped by MAC                             */
    ca_boolean_t  mpcp_ext_drop;     /* (default=1) MPCP extension frame will be dropped by MAC                   */
    ca_boolean_t  overflow_drop;     /* (default=1) Packet will be dropped when rxctrl FIFO is full               */
    ca_boolean_t  unknown_ds_drop;   /* (default=1) Packet which have unknow DA when type is 8808 will be dropped */
    ca_boolean_t  oam_sec_drop;      /* (default=1) Key exchange OAM frame drop in Hardware key-exchange mode     */
    ca_boolean_t  mpcp_crc32_drop;
    ca_boolean_t  mpcp_crc8_drop;
} aal_epon_mac_drop_t;


typedef union
{
    struct
    {
        ca_uint32_t   invalid_llid_drop  : 1;
        ca_uint32_t   fcs_err_drop       : 1;
        ca_uint32_t   oversize_drop      : 1;
        ca_uint32_t   runtsize_drop      : 1;
        ca_uint32_t   pause_drop         : 1;
        ca_uint32_t   unknown_op_drop    : 1;
        ca_uint32_t   mpcp_drop          : 1;
        ca_uint32_t   mpcp_ext_drop      : 1;
        ca_uint32_t   overflow_drop      : 1;
        ca_uint32_t   unknown_ds_drop    : 1;
        ca_uint32_t   oam_sec_drop       : 1;
        ca_uint32_t   mpcp_crc32_drop    : 1;
        ca_uint32_t   mpcp_crc8_drop     : 1;

    } s;
    ca_uint32_t u32;
}aal_epon_mac_drop_msk_t;


typedef struct
{
    ca_uint8_t   sync_invalid_off;      /* (default=1) 0~15, Synchronization Invalid Count Offset. */
    ca_uint8_t   sync_valid_off;        /* (default=0) 0~64, Synchronization Valid Count Offset. */

} aal_epon_mac_xg_pca_t;

typedef union
{
    struct
    {
        ca_uint32_t   sync_invalid_off   : 1;
        ca_uint32_t   sync_valid_off     : 1;
    } s;
    ca_uint32_t u32;
}aal_epon_mac_xg_pca_msk_t;

typedef volatile union
{
    struct
    {
        #ifdef CA_BIG_ENDIAN
        ca_uint32_t frm_type    :  4 ;    /* bit 31~28 */
        ca_uint32_t rsv3        :  1 ;    /* bit 27 */
        ca_uint32_t llid_off    :  2 ;    /* bit 26~25 */
        ca_uint32_t llid_val    :  9 ;    /* bit 24~16 */
        ca_uint32_t rsv1        :  1 ;    /* bit 15 */
        ca_uint32_t llid_type   :  3 ;    /* bit 14~12 */
        ca_uint32_t rsv0        :  1 ;    /* bit 11 */
        ca_uint32_t frm_size    :  11 ;   /* bit 10~0*/
        #else /* CA_LITTLE_ENDIAN */
        ca_uint32_t frm_size    :  11 ;   /* bit 10~0*/
        ca_uint32_t rsv0        :  1 ;    /* bit 11 */
        ca_uint32_t llid_type   :  3 ;    /* bit 14~12 */
        ca_uint32_t rsv1        :  1 ;    /* bit 15 */
        ca_uint32_t llid_val    :  9 ;    /* bit 24~16 */
        ca_uint32_t llid_off    :  2 ;    /* bit 26~25 */
        ca_uint32_t rsv3        :  1 ;    /* bit 27 */
        ca_uint32_t frm_type    :  4 ;    /* bit 31~28 */
        #endif
    } bf ;
    ca_uint32_t     wrd ;
} aal_epon_mac_tx_frame_header_t;


typedef volatile union {
  struct {
#ifdef CA_BIG_ENDIAN
    ca_uint32_t cos0                 :  3 ; /* bits 31:29 */
    ca_uint32_t ddid_vlan_info0      :  2 ; /* bits 28:27 */
    ca_uint32_t dpdm_dpid0           :  7 ; /* bits 26:20 */
    ca_uint32_t ddbm_dpllid0         :  9 ; /* bits 19:11 */
    ca_uint32_t full_pkt_size0       : 11 ; /* bits 10:0 */
#else /* CA_LITTLE_ENDIAN */
    ca_uint32_t full_pkt_size0       : 11 ; /* bits 10:0 */
    ca_uint32_t ddbm_dpllid0         :  9 ; /* bits 19:11 */
    ca_uint32_t dpdm_dpid0           :  7 ; /* bits 26:20 */
    ca_uint32_t ddid_vlan_info0      :  2 ; /* bits 28:27 */
    ca_uint32_t cos0                 :  3 ; /* bits 31:29 */
#endif
  } bf ;
  ca_uint32_t     wrd ;
} aal_epon_mac_xg_tx_frame_header_t;

typedef struct
{
    ca_llid_t   llid;       /* llid to send  */
    ca_uint8_t    cos;        /* cos to fill  */
    ca_uint8_t    *data;      /* data address  */
    ca_uint16_t   len;        /* data length */
} aal_epon_mac_pkt_t;


ca_status_t aal_epon_mac_init
(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t mode
);

ca_status_t aal_epon_mac_mib_rc_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   read_clear
);

ca_status_t aal_epon_mac_mib_rc_get
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id,
    CA_IN ca_boolean_t   *read_clear
);

ca_status_t aal_epon_mac_cfg_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_mac_ctrl_msk_t        mask,
    CA_IN aal_epon_mac_ctrl_t           *cfg
);

ca_status_t aal_epon_mac_cfg_get(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_ctrl_t          *cfg
);

ca_status_t aal_epon_mac_onu_type_get(
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t         port_id,
    CA_OUT ca_uint8_t          *onu_type
);

ca_status_t aal_epon_mac_drop_set(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_mac_drop_msk_t     pmac_xg_drop_msk,
    CA_IN aal_epon_mac_drop_t        *pmac_xg_drop
);
ca_status_t aal_epon_mac_drop_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_drop_t       *pmac_xg_drop
);

ca_status_t aal_epon_mac_rst_ctrl_set
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN aal_epon_mac_rst_ctrl_msk_t    pon_rst_ctrl_msk,
    CA_IN aal_epon_mac_rst_ctrl_t       *pon_rst_ctrl
);

ca_status_t aal_epon_mac_rst_ctrl_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_rst_ctrl_t      *pon_rst_ctrl
);

ca_status_t aal_epon_mac_stats_llid_mpcp_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid,
    CA_OUT aal_epon_mac_stats_llid_mpcp_t *stats_llid_mpcp
);

ca_status_t aal_epon_mac_stats_llid_eth_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_llid_t                     llid,
    CA_OUT aal_epon_mac_stats_llid_eth_t *stats_llid_eth
);

ca_status_t aal_epon_mac_stats_port_mpcp_get
(
    CA_IN  ca_device_id_t                   device_id,
    CA_IN  ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_stats_port_mpcp_t *stats_port_mpcp
);

ca_status_t aal_epon_mac_stats_port_mpcp_ctrl_get
(
    CA_IN  ca_device_id_t                    device_id,
    CA_IN  ca_port_id_t                   port_id,
    CA_OUT aal_epon_mac_stats_port_mpcp_ctrl_t *mpcp_mac_ctrl
);

ca_status_t aal_epon_mac_stats_port_eth_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_OUT aal_epon_mac_stats_port_eth_t *stats_port_eth
);

ca_status_t aal_epon_mac_1g_link_status_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_boolean_t                   *link
);

ca_status_t aal_epon_mac_xg_link_status_get
(
    CA_IN ca_device_id_t                   device_id,
    CA_IN ca_port_id_t                  port_id,
    CA_IN ca_boolean_t                   *link
);

ca_status_t aal_epon_mac_sw_reset_set
(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t    value
);

ca_status_t aal_epon_mac_sw_reset_get
(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *value
);

ca_status_t aal_epon_csr_reset
(
    CA_IN ca_device_id_t                   device_id
);



#endif

