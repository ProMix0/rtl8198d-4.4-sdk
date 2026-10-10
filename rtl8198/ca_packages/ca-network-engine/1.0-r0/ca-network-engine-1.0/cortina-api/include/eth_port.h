#ifndef __ETH_PORT_H__
#define __ETH_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "ca_types.h"

#define CA_ETH_PORT_NUM       (8)    /* Total Ethernet port number of the device */
#define CA_MDIO_ADDR_MIN      (0)    /* Minimal MDIO bus address */
#if defined(CONFIG_LUNA_G3_SERIES) && !defined(CONFIG_ARCH_CORTINA_G3LITE)
#define CA_MDIO_ADDR_MAX      (4)   /* Maximal MDIO bus address */
#else
#define CA_MDIO_ADDR_MAX      (31)   /* Maximal MDIO bus address */
#endif
#define CA_MDIO_CLOCK_MIN     (1)    /* Minimal MDIO bus clock speed */
#define CA_MDIO_CLOCK_MAX     (20000)/* Maximal MDIO bus clock speed */

#define CA_ETH_MAX_FRAME_SIZE_DEFAULT   (12000) /* Default value of MRU(Max Received Unit) */
#define CA_ETH_MAX_FRAME_SIZE_MIN       (64)   /* Minimal value of MRU */
#define CA_ETH_MAX_FRAME_SIZE_MAX       (12000) /* Maximal value of MRU */

typedef enum {
    CA_PORT_SPEED_INVALID       = 0, /* Port speed is invalid. If link is in auto-negotiation process or link is down, this valud is returned to caller. */
    CA_PORT_SPEED_10M           = 1, /* Speed is 10Mbps */
    CA_PORT_SPEED_100M          = 2, /* Speed is 100Mbps */
    CA_PORT_SPEED_1G            = 3, /* Speed is 1Gbps */
    CA_PORT_SPEED_2_5G          = 4, /* Speed is 2.5Gbps */
    CA_PORT_SPEED_10G           = 5, /* Speed is 10Gbps */
    CA_PORT_SPEED_5G            = 6, /* speed is 5Gbps */
    CA_PORT_SPEED_AUTO          = 7  /* speed obtained via autonegotiation */
} ca_eth_port_speed_t;

typedef enum {
    CA_PORT_LINK_DOWN           = 0,  /* Port is down */
    CA_PORT_LINK_UP             = 1 /* Port is up */
} ca_eth_port_link_status_t;

typedef enum {
    CA_PORT_DUPLEX_HALF         = 0, /* Half duplex mode */
    CA_PORT_DUPLEX_FULL         = 1, /* Full duplex mode */
    CA_PORT_DUPLEX_AUTO         = 2  /* Not used by now */
} ca_eth_port_duplex_t;

typedef enum {
    CA_PORT_MDI                 = 0, /* Force MDI */
    CA_PORT_MDIX                = 1, /* Force MDIX */
    CA_PORT_MDIX_AUTO           = 2  /* Auto selection for MDI/MDIX */
} ca_eth_port_mdix_t;

typedef struct {/* PHY counters: gotten from PHY via MDIO */
    ca_uint32_t single_collision_frames;
    ca_uint32_t multiple_collision_frames;
    ca_uint32_t alignmentErrors;
    ca_uint32_t frames_with_deferredXmissions;
    ca_uint32_t late_collisions;
    ca_uint32_t frames_aborted_dueto_XS_colls;
    ca_uint32_t frames_lost_duetoint_mac_xmit_error;
    ca_uint32_t frames_lost_duetoint_mac_rcv_error;
    ca_uint32_t frames_with_excessive_deferral;
    ca_uint32_t in_rangeLength_errors;
    ca_uint32_t outof_range_length_field;
    ca_uint32_t collision_frames;
    ca_uint32_t symbol_error_during_carrier;
    ca_uint32_t media_available;
    ca_uint32_t autoneg_remote_signaling;
    ca_uint32_t autoneg_auto_config;
    ca_uint32_t unsupported_opcodes_received;
    ca_uint32_t pause_link_delay_allowance;
} ca_phy_stats_t;

typedef struct {
    ca_uint64_t rx_good_octets;      /* received octets without error                                             */
    ca_uint64_t rx_uc_frames;        /* received unicast frames                                                   */
    ca_uint64_t rx_mc_frames;        /* received multicast frames                                                 */
    ca_uint64_t rx_bc_frames;        /* received broadcast frames                                                 */
    ca_uint64_t rx_error_frames;     /* received error frames*/
    ca_uint64_t rx_fcs_error_frames; /* received frames with CRC32 error                                          */
    ca_uint64_t rx_pause_frames;     /* received MAC PAUSE frames                                                 */
    ca_uint64_t rx_undersize_frames; /* received frames undersize frame without FCS err               */
    ca_uint64_t rx_runt_frames;      /* received frames of which length is lesser than 64                         */
    ca_uint64_t rx_oversize_frames;  /* received frames of which length is greater than max frames size           */
    ca_uint64_t rx_64_frames;        /* received frames with 64 bytes                                             */
    ca_uint64_t rx_65_127_frames;    /* received frames with 65 ~ 127 bytes                                       */
    ca_uint64_t rx_128_255_frames;   /* received frames with 128 ~ 255 bytes                                      */
    ca_uint64_t rx_256_511_frames;   /* received frames with 256 ~ 511 bytes                                      */
    ca_uint64_t rx_512_1023_frames;  /* received frames with 512 ~ 1023 bytes                                     */
    ca_uint64_t rx_1024_1518_frames; /* received frames with 1024 ~ 1518 bytes                                    */
    ca_uint64_t rx_1519_max_frames;  /* received frames of which length is equal to or greater than 1519 bytes    */
    ca_uint64_t tx_good_octets;      /* transmitted octets without error                                          */
    ca_uint64_t tx_uc_frames;        /* transmitted unicast frames                                                */
    ca_uint64_t tx_mc_frames;        /* transmitted multicast frames                                              */
    ca_uint64_t tx_bc_frames;        /* transmitted broadcast frames                                              */
    ca_uint64_t tx_error_frames;     /* transmitted error frames*/
    ca_uint64_t tx_pause_frames;     /* transmitted MAC PAUSE frames                                              */
    ca_uint64_t tx_64_frames;        /* transmitted frames with 64 bytes                                          */
    ca_uint64_t tx_65_127_frames;    /* transmitted frames with 65 ~ 127 bytes                                    */
    ca_uint64_t tx_128_255_frames;   /* transmitted frames with 128 ~ 255 bytes                                   */
    ca_uint64_t tx_256_511_frames;   /* transmitted frames with 256 ~ 511 bytes                                   */
    ca_uint64_t tx_512_1023_frames;  /* transmitted frames with 512 ~ 1023 bytes                                  */
    ca_uint64_t tx_1024_1518_frames; /* transmitted frames with 1024 ~ 1518 bytes                                 */
    ca_uint64_t tx_1519_max_frames;  /* transmitted frames of which length is equal to or greater than 1519 bytes */

    ca_phy_stats_t phy_stats; /* ethernet PHY statistics */
} ca_eth_port_stats_t;

typedef struct {
    ca_boolean_t    half_10;        /* PHY REG 4.5 setting for 10BASE-TX half duplex capable    */
    ca_boolean_t    full_10;        /* PHY REG 4.6 setting for 10BASE-TX full duplex capable    */
    ca_boolean_t    half_100;       /* PHY REG 4.7 setting for 10BASE-TX half full capable      */
    ca_boolean_t    full_100;       /* PHY REG 4.8 setting for 100BASE-TX half duplex capable   */
    ca_boolean_t    full_1g;        /* PHY REG 4.9 setting for 1000BASE-TX half full capable    */
    ca_boolean_t    full_10g;       /* 10G */
    ca_boolean_t    full_5g;        /* 5G */
    ca_boolean_t    full_2d5g;      /* 2.5G*/
    ca_boolean_t    flow_ctrl;      /* PHY REG 4.10 setting for flow control capable            */
    ca_boolean_t    asym_flow_ctrl; /* PHY REG 4.11 setting for asymmetric flow control capable */
    ca_boolean_t    eee_100_capable;/* EEE 100M capable*/
    ca_boolean_t    eee_1g_capable; /* EEE 1G capable*/
    ca_boolean_t    eee_10g_capable;/* EEE 10G capable*/
    ca_boolean_t    eee_2d5g_capable;/* EEE 2.5G capable*/
    ca_boolean_t    eee_5g_capable;  /* EEE 5G capable*/
} ca_eth_port_ability_t;

ca_status_t ca_eth_port_pause_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t pfc_enable,
    CA_IN ca_boolean_t pause_rx,
    CA_IN ca_boolean_t pause_tx
);

ca_status_t ca_eth_port_pause_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_boolean_t *pfc_enable,
    CA_OUT ca_boolean_t *pause_rx,
    CA_OUT ca_boolean_t *pause_tx
);

ca_status_t ca_eth_port_pause_quanta_set (
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t      port_id,
    CA_IN ca_uint16_t       pause_quanta
);

ca_status_t ca_eth_port_pause_quanta_get (
    CA_IN  ca_device_id_t      device_id,
    CA_IN  ca_port_id_t     port_id,
    CA_OUT ca_uint16_t     *pause_quanta
);

ca_status_t ca_eth_port_link_scan_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t enable
);

ca_status_t ca_eth_port_link_scan_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t port_id,
    CA_OUT ca_boolean_t *enable
);

ca_status_t ca_eth_port_link_status_get(
    CA_IN  ca_device_id_t               device_id,
    CA_IN  ca_port_id_t              port_id,
    CA_OUT ca_eth_port_link_status_t *status
);

ca_status_t ca_eth_port_stats_get(
    CA_IN  ca_device_id_t        device_id,
    CA_IN  ca_port_id_t       port_id,
    CA_IN  ca_boolean_t       read_clear,
    CA_OUT ca_eth_port_stats_t *data
);

ca_status_t ca_eth_port_autoneg_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_IN ca_boolean_t    enable
);

ca_status_t ca_eth_port_autoneg_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t    port_id,
    CA_OUT ca_boolean_t   *enable
);

ca_status_t ca_eth_port_phy_autoneg_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    port_id,
    CA_IN ca_boolean_t    enable
);

ca_status_t ca_eth_port_phy_autoneg_get(
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_port_id_t    port_id,
    CA_OUT ca_boolean_t   *enable
);

ca_status_t ca_eth_port_phy_advert_set(
    CA_IN ca_device_id_t        device_id,
    CA_IN ca_port_id_t          port_id,
    CA_IN ca_eth_port_ability_t ability
);

ca_status_t ca_eth_port_phy_advert_get(
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_port_id_t           port_id,
    CA_OUT ca_eth_port_ability_t *ability
);

ca_status_t ca_eth_port_phy_advert_remote_get (
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_port_id_t           port_id,
    CA_OUT ca_eth_port_ability_t *ability
);

ca_status_t ca_eth_port_speed_set (
    CA_IN ca_device_id_t         device_id,
    CA_IN ca_port_id_t           port_id,
    CA_IN ca_eth_port_speed_t    speed
);

ca_status_t ca_eth_port_speed_get (
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_port_id_t           port_id,
    CA_OUT ca_eth_port_speed_t   *speed
);

ca_status_t ca_eth_port_phy_speed_set (
    CA_IN ca_device_id_t         device_id,
    CA_IN ca_port_id_t           port_id,
    CA_IN ca_eth_port_speed_t    speed
);

ca_status_t ca_eth_port_phy_speed_get (
    CA_IN  ca_device_id_t         device_id,
    CA_IN  ca_port_id_t           port_id,
    CA_OUT ca_eth_port_speed_t   *speed
);


ca_status_t ca_eth_port_duplex_set (
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t         port_id,
    CA_IN ca_eth_port_duplex_t duplex
);

ca_status_t ca_eth_port_duplex_get (
    CA_IN  ca_device_id_t        device_id,
    CA_IN  ca_port_id_t          port_id,
    CA_OUT ca_eth_port_duplex_t *duplex
);

ca_status_t ca_eth_port_phy_duplex_set (
    CA_IN ca_device_id_t       device_id,
    CA_IN ca_port_id_t         port_id,
    CA_IN ca_eth_port_duplex_t duplex
);

ca_status_t ca_eth_port_phy_duplex_get (
    CA_IN  ca_device_id_t        device_id,
    CA_IN  ca_port_id_t          port_id,
    CA_OUT ca_eth_port_duplex_t *duplex
);

ca_status_t ca_eth_port_phy_mdix_set (
    CA_IN  ca_device_id_t       device_id,
    CA_IN  ca_port_id_t         port_id,
    CA_IN  ca_eth_port_mdix_t   mode
);

ca_status_t ca_eth_port_phy_mdix_get (
    CA_IN  ca_device_id_t        device_id,
    CA_IN  ca_port_id_t          port_id,
    CA_OUT ca_eth_port_mdix_t   *mode
);

ca_status_t ca_eth_port_eee_enable_set (
    CA_IN  ca_device_id_t          device_id,
    CA_IN  ca_port_id_t            port_id,
    CA_IN  ca_boolean_t            eee_enable
);

ca_status_t ca_eth_port_eee_enable_get (
    CA_IN   ca_device_id_t          device_id,
    CA_IN   ca_port_id_t            port_id,
    CA_OUT  ca_boolean_t           *eee_enable
);


ca_status_t cap_mdio_speed_set (
    CA_IN  ca_device_id_t  device_id,
    CA_IN  ca_uint32_t     addr,
    CA_IN  ca_uint32_t     clock
);

ca_status_t cap_mdio_speed_get (
    CA_IN       ca_device_id_t      device_id,
    CA_IN       ca_uint32_t         addr,
    CA_OUT      ca_uint32_t        *clock
);

ca_status_t cap_mdio_write(
    CA_IN      ca_device_id_t         device_id,
    CA_IN      ca_uint8_t             st_code,
    CA_IN      ca_uint8_t             phy_port_addr,
    CA_IN      ca_uint8_t             reg_dev_addr,
    CA_IN      ca_uint16_t            addr,
    CA_IN      ca_uint16_t            data
);

ca_status_t cap_mdio_read(
    CA_IN      ca_device_id_t device_id,
    CA_IN      ca_uint8_t     st_code,
    CA_IN      ca_uint8_t     phy_port_addr,
    CA_IN      ca_uint8_t     reg_dev_addr,
    CA_IN      ca_uint16_t    addr,
    CA_OUT     ca_uint16_t   *data
);

#ifdef __cplusplus
}
#endif

#endif /*  __ETH_PORT_H__ */
