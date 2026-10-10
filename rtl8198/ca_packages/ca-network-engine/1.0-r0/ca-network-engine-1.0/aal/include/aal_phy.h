#ifndef __AAL_PHY_H__
#define __AAL_PHY_H__

#include "ca_types.h"
#include <soc/cortina/registers.h>

#define CA_AAL_MDIO_WR_FLAG        0
#define CA_AAL_MDIO_RD_FLAG        1
#define CA_AAL_MDIO_ACCESS_TIMEOUT 50000
#define CA_AAL_MDIO_PER_CLK        125000
#define CA_AAL_PHY_NAME_LEN_MAX    32

#define PHY_REG_MAX_LEN            16

/*Organizationally Unique Identifier (OUI) */
#define PHY_OUI_BCM5461            0x000818
#define PHY_OUI_BCM54612           0x00d897
#define PHY_OUI_RTK                0x000732
#define PHY_OUI_VTS                0x0001c1
#define PHY_OUI_AR                 0x001374
#define PHY_OUI_MVL                0x000ac2
#define PHY_OUI_AQR                0x00e86d
#define PHY_OUI_MVL                0x000ac2
/*Manufacturer's Model Number  */
#define PHY_MODEL_BCM5461          0x000c
#define PHY_MODEL_BCM54612         0x0026
#define PHY_MODEL_RTL8211E         0x0010
#define PHY_MODEL_RTL8211          0x0011
#define PHY_MODEL_RTL8211_ASIC     0x0018
#define PHY_MODEL_RTL8201F         0x0001
#define PHY_MODEL_RTL8214          0x0026
#define PHY_MODEL_RTL8214C         0x0014
#define PHY_MODEL_VTS8641          0x0003
#define PHY_MODEL_AR8305           0x0007
#define PHY_MODEL_RTL8198F         0x0008
#define PHY_MODEL_MVL88X3310       0x001a
#define PHY_MODEL_AQR107           0x000e
#define PHY_MODEL_RTL8226          0x0003

typedef enum {
    AAL_PHY_MDI_MDI = 0,    /* Fixed MDI       */
    AAL_PHY_MDI_MDIX = 1,   /* Fixed MDIX      */
    AAL_PHY_MDI_AUTO = 2    /* Auto MDI/MDIX   */
} aal_phy_mdi_mode_t;

typedef enum {
    AAL_PHY_SPEED_10 = 0, /* 10M bps */
    AAL_PHY_SPEED_100 = 1, /* 100M bps */
    AAL_PHY_SPEED_1000 = 2,/* 1G bps */
    AAL_PHY_SPEED_10000 = 3, /* 10G bps */
    AAL_PHY_SPEED_2000 = 4, /* 2G bps */
    AAL_PHY_SPEED_2500 = 5, /* 2.5G bps */
    AAL_PHY_SPEED_5000 = 6, /* 5G bps */
	AAL_PHY_SPEED_MAX 
} aal_phy_speed_mode_t;

typedef enum {
    AAL_PHY_FW_FLASH = 0,  /* boot from flash */
    AAL_PHY_FW_RAM = 1,    /* boot form RAM through MDIO */
    AAL_PHY_FW_MAX = AAL_PHY_FW_RAM
} aal_phy_boot_mode_t;


/*
Define mdio phy addr:
0: bc addr
1: asic addr
2-4: ca reserve addr
5-31: mdio addr
*/

typedef enum {
        AAL_SXGMII_PCS_CONTROL_OPER_NORMAL = 0,   
        AAL_SXGMII_PCS_CONTROL_OPER_RESET  
} aal_phy_sxgmii_pcs_control_oper_t;


typedef enum {
        AAL_SXGMII_PCS_CONTROL_INDEX_0 = 0,   /* PCS control 1 */
        AAL_SXGMII_PCS_CONTROL_INDEX_1 = 1    /* PCS control 2 */
} aal_phy_sxgmii_pcs_control_index_t;

typedef struct {
	ca_boolean_t tx2rx_enable;
	ca_boolean_t rx2tx_enable;
} aal_phy_lpbk_mode_t;

typedef struct {
    ca_uint32_t adv_10base_t_half:1;
    ca_uint32_t adv_10base_t_full:1;
    ca_uint32_t adv_100base_tx_half:1;
    ca_uint32_t adv_100base_tx_full:1;
    ca_uint32_t adv_100base_t4:1;
    ca_uint32_t adv_pause:1;
    ca_uint32_t adv_asym_pause:1;
    ca_uint32_t adv_1000base_t_half:1;
    ca_uint32_t adv_1000base_t_full:1;
    ca_uint32_t adv_2500base_t_full:1;
    ca_uint32_t adv_2500base_t_half:1;
    ca_uint32_t adv_5000base_t_full:1;
    ca_uint32_t adv_5000base_t_half:1;
    ca_uint32_t adv_10000base_t_full:1;
    ca_uint32_t adv_10base_te:1;
    ca_uint32_t adv_100base_tx_eee:1;
    ca_uint32_t adv_1000base_t_eee:1;
    ca_uint32_t adv_10gbase_t_eee: 1;
    ca_uint32_t adv_2d5gbase_t_eee: 1;
    ca_uint32_t adv_5gbase_t_eee: 1;
    ca_uint32_t rsrvd:12;
} aal_phy_autoneg_adv_t;

typedef struct {
	ca_boolean_t link_up;
	ca_boolean_t duplex;
	aal_phy_speed_mode_t speed;
} aal_phy_link_status_t;

typedef volatile union {
  struct {
    ca_uint16_t selector               : 5 ; /* bits 4:0 */
    ca_uint16_t adv_10base_t_half      : 1 ; /* bits 5:5 */
    ca_uint16_t adv_10base_t_full      : 1 ; /* bits 6:6 */
    ca_uint16_t adv_100base_tx_half    : 1 ; /* bits 7:7 */
    ca_uint16_t adv_100base_tx_full    : 1 ; /* bits 8:8 */
    ca_uint16_t adv_100base_t4         : 1 ; /* bits 9:9 */
    ca_uint16_t adv_pause              : 1 ; /* bits 10:10 */
    ca_uint16_t adv_asym_pause         : 1 ; /* bits 11:11 */
    ca_uint16_t reserved1              : 1 ; /* bits 12:12 */
    ca_uint16_t remote_fault           : 1 ; /* bits 13:13 */
    ca_uint16_t reserved2              : 1 ; /* bits 14:14 */
    ca_uint16_t next_page              : 1 ; /* bits 15:15 */
  } bf ;
  ca_uint16_t     wrd ;
} aal_phy_reg_ANAR_t;

typedef volatile union {
  struct {
    ca_uint16_t selector               : 5 ; /* bits 4:0 */
    ca_uint16_t adv_10base_t_half      : 1 ; /* bits 5:5 */
    ca_uint16_t adv_10base_t_full      : 1 ; /* bits 6:6 */
    ca_uint16_t adv_100base_tx_half    : 1 ; /* bits 7:7 */
    ca_uint16_t adv_100base_tx_full    : 1 ; /* bits 8:8 */
    ca_uint16_t adv_100base_t4         : 1 ; /* bits 9:9 */
    ca_uint16_t adv_pause              : 1 ; /* bits 10:10 */
    ca_uint16_t adv_asym_pause         : 1 ; /* bits 11:11 */
    ca_uint16_t reserved1              : 1 ; /* bits 12:12 */
    ca_uint16_t remote_fault           : 1 ; /* bits 13:13 */
    ca_uint16_t reserved2              : 1 ; /* bits 14:14 */
    ca_uint16_t next_page              : 1 ; /* bits 15:15 */
  } bf ;
  ca_uint16_t     wrd ;
} aal_phy_reg_ANLPAR_t;

typedef volatile union {
  struct {
    ca_uint16_t reserved               : 9 ; /* bits 8:0 */
    ca_uint16_t adv_1000base_t_full    : 1 ; /* bits 9:9 */
    ca_uint16_t adv_port_type          : 1 ; /* bits 10:10 */
    ca_uint16_t adv_master_slave       : 1 ; /* bits 11:11 */
    ca_uint16_t enable_master_slave    : 1 ; /* bits 12:12 */
    ca_uint16_t test_mode              : 3 ; /* bits 15:13 */
  } bf ;
  ca_uint16_t     wrd ;
} aal_phy_reg_GBCR_t;

typedef volatile union {
  struct {
    ca_uint16_t idle_error_cnt         : 8 ; /* bits 7:0 */
    ca_uint16_t reserved               : 2 ; /* bits 9:8 */
    ca_uint16_t lp_1000base_t_half     : 1 ; /* bits 10:10 */
    ca_uint16_t lp_1000base_t_full     : 1 ; /* bits 11:11 */
    ca_uint16_t remote_recv_status     : 1 ; /* bits 12:12 */
    ca_uint16_t local_recv_status      : 1 ; /* bits 13:13 */
    ca_uint16_t master_slave_result    : 1 ; /* bits 14:14 */
    ca_uint16_t master_slave_fault     : 1 ; /* bits 15:15 */
  } bf ;
  ca_uint16_t     wrd ;
} aal_phy_reg_GBSR_t;

ca_status_t aal_mdio_speed_set(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t addr, 
    CA_IN ca_uint32_t clock);

ca_status_t aal_mdio_speed_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t addr,
    CA_OUT ca_uint32_t * clock);

ca_status_t aal_mdio_write(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t             device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t              addr,
    CA_IN      ca_uint16_t              data);

ca_status_t aal_mdio_read(/*CODEGEN_IGNORE_TAG*/
    CA_IN       ca_device_id_t             device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t              addr,
    CA_OUT     ca_uint16_t             *data);

ca_status_t aal_mdio_set(
    CA_IN       ca_device_id_t             device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t              addr,
    CA_IN      ca_uint16_t              data);

ca_status_t aal_mdio_get(
    CA_IN       ca_device_id_t             device_id,
    CA_IN      ca_uint8_t              st_code,
    CA_IN      ca_uint8_t              phy_port_addr,
    CA_IN      ca_uint8_t              reg_dev_addr,
    CA_IN      ca_uint16_t              addr,
    CA_OUT     ca_uint16_t             *data);

ca_status_t aal_eth_phy_reset(/*CODEGEN_IGNORE_TAG*/
                   CA_IN ca_device_id_t device_id, 
                   CA_IN ca_uint8_t phy_addr);


ca_status_t  aal_eth_phy_reset_get(
                   CA_IN ca_device_id_t  device_id,
                   CA_IN ca_uint8_t   phy_addr);

ca_status_t  aal_eth_phy_reset_set(
                   CA_IN ca_device_id_t  device_id,
                   CA_IN  ca_uint8_t   phy_addr);

ca_status_t aal_eth_phy_auto_neg_restart(/*CODEGEN_IGNORE_TAG*/
                   CA_IN ca_device_id_t device_id,
                   CA_IN  ca_uint8_t phy_addr);

ca_status_t aal_eth_phy_power_set(
                   CA_IN ca_device_id_t device_id,
                   CA_IN ca_uint8_t phy_addr, 
                   CA_IN ca_boolean_t power_up);

ca_status_t aal_eth_phy_power_get(
                   CA_IN ca_device_id_t device_id,
                   CA_IN ca_uint8_t phy_addr, 
                   CA_OUT   ca_boolean_t * power_up);

ca_status_t aal_eth_phy_auto_neg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t phy_addr, 
    CA_IN ca_boolean_t enable);

ca_status_t aal_eth_phy_auto_neg_get(
    CA_IN ca_device_id_t device_id,
    CA_IN  ca_uint8_t phy_addr,
    CA_OUT    ca_boolean_t * enable);

ca_status_t aal_eth_phy_ability_adv_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t phy_addr,
    CA_IN aal_phy_autoneg_adv_t adv);

ca_status_t aal_eth_phy_ability_adv_get(
    CA_IN   ca_device_id_t device_id,
    CA_IN   ca_uint8_t phy_addr,
    CA_OUT  aal_phy_autoneg_adv_t * adv);

ca_status_t aal_eth_phy_partner_ability_adv_get(
    CA_IN     ca_device_id_t device_id,
    CA_IN     ca_uint8_t phy_addr,
    CA_OUT    aal_phy_autoneg_adv_t * adv);

ca_status_t aal_eth_phy_link_status_get(
    CA_IN     ca_device_id_t device_id,
    CA_IN     ca_uint8_t phy_addr,
    CA_OUT    aal_phy_link_status_t * link_status);

ca_status_t aal_eth_phy_speed_set(
    CA_IN    ca_device_id_t device_id,
    CA_IN    ca_uint8_t phy_addr,
    CA_IN  aal_phy_speed_mode_t speed);

ca_status_t aal_eth_phy_speed_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t phy_addr,
    CA_OUT  aal_phy_speed_mode_t * speed);

ca_status_t aal_eth_phy_duplex_set(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t phy_addr, 
    CA_IN  ca_boolean_t is_full);

ca_status_t aal_eth_phy_duplex_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t phy_addr, 
    CA_OUT  ca_boolean_t * is_full);

ca_status_t aal_eth_phy_loopback_set(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t phy_addr,
    CA_IN  aal_phy_lpbk_mode_t mode);

ca_status_t aal_eth_phy_loopback_get(
    CA_IN  ca_device_id_t device_id,
    CA_IN  ca_uint8_t phy_addr,
    CA_OUT aal_phy_lpbk_mode_t * mode);

ca_status_t  aal_eth_phy_mdix_get(
    CA_IN   ca_device_id_t    device_id,
    CA_IN   ca_uint8_t     phy_addr,
    CA_OUT   aal_phy_mdi_mode_t *mdix_mode);

ca_status_t  aal_eth_phy_mdix_set(
    CA_IN   ca_device_id_t    device_id,
    CA_IN   ca_uint8_t     phy_addr,
    CA_IN   aal_phy_mdi_mode_t mdix_mode);

ca_status_t aal_eth_phy_debug_info_get(
    CA_IN   ca_device_id_t       device_id,
    CA_IN   ca_uint8_t           phy_addr);

ca_status_t aal_eth_phy_fw_upgrade_set(
    CA_IN   ca_device_id_t       device_id,
    CA_IN   ca_uint8_t           phy_addr,
    CA_IN   aal_phy_boot_mode_t  boot_mode);

ca_status_t aal_eth_phy_fw_upgrade_get(
    CA_IN   ca_device_id_t       device_id,
    CA_IN   ca_uint8_t           phy_addr,
    CA_OUT  aal_phy_boot_mode_t  *boot_mode);

ca_status_t aal_eth_phy_name_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN    ca_device_id_t    device_id,
    CA_IN    ca_uint8_t     phy_addr,
    CA_OUT   char          *name);

ca_status_t 
aal_eth_phy_oui_get (/*CODEGEN_IGNORE_TAG*/
     CA_IN ca_device_id_t    device_id,
     CA_IN ca_port_id_t      aal_port,
     CA_OUT ca_uint32_t       *oui );


char *aal_eth_phy_speed_str_get(/*CODEGEN_IGNORE_TAG*/
    CA_IN    ca_device_id_t       device_id,
    CA_IN    aal_phy_speed_mode_t  phy_speed);

ca_status_t aal_eth_phy_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id, 
    CA_IN ca_uint8_t phy_addr);

ca_status_t aal_mdio_global_init(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t device_id);

ca_status_t aal_mdio_global_exit(/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t device_id);
#ifndef CONFIG_CORTINA_BOARD_FPGA/*CODEGEN_IGNORE_TAG*/
ca_status_t aal_internal_phy_init(/*CODEGEN_IGNORE_TAG*/ca_device_id_t  device_id);
#endif

#if defined(CONFIG_NE_PHY_CALIBRATION)/*CODEGEN_IGNORE_TAG*/
int phy_cal_load(/*CODEGEN_IGNORE_TAG*/struct device *dev);
#endif


#endif
