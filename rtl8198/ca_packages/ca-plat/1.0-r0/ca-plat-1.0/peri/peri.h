#ifndef __CAP_PER_H__
#define __CAP_PER_H__

#include "ca_types.h"


typedef enum
{
    CA_PER_MID_TIMER               = 0,
    CA_PER_MID_I2C                    = 1,
    CA_PER_MID_SPI                    = 2,
    CA_PER_MID_MDIO                = 3,
    CA_PER_MID_LED                   = 4,
    CA_PER_MID_JTAGID             = 5,
    CA_PER_MID_GPIO                 = 6,
    CA_PER_MID_MAX
} CA_PER_MUTEX_ID_E;

typedef enum {
   CA_TIMER_MODE_DISABLE =0,
   CA_TIMER_MODE_RELOAD,
   CA_TIMER_MODE_ROLLOVER
} ca_hwtimer_mode_e;

typedef enum {
    CAP_WATCHDOG_ACTION_REBOOT   = 0,
    CAP_WATCHDOG_ACTION_REPORT   = 1
} cap_watchdog_action_t;

typedef enum
{
    CA_PHY_TYPE_0               = 0,
    CA_PHY_TYPE_1,
    CA_PHY_TYPE_2,
    CA_PHY_TYPE_3,
    CA_PHY_TYPE_4,
    CA_PHY_TYPE_5,
    CA_PHY_TYPE_6,
    CA_PHY_TYPE_7,
    CA_PHY_TYPE_MAX
} ca_phy_type_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
   ca_uint32_t          threshold;
   ca_uint32_t          mode;
   void *                   int_handle;
   void *                   int_cookie;
} cap_hardware_timer_config_t;

typedef struct {
    ca_boolean_t enable;
    ca_uint32_t threshold;
    ca_uint32_t action;
    ca_uint32_t rvd;
} cap_watchdog_config_t;

typedef struct {
    ca_uint8_t address;
    ca_uint8_t offset;
    ca_uint8_t data;
    ca_uint8_t rvd;
} cap_i2c_slave_t;

/******************************************************************************
 *              Extern declaration of exposed APIs
 *****************************************************************************/

#ifndef CONFIG_ARCH_RTL8198F
extern ca_status_t cap_i2c_enable_set (
                             CA_IN  ca_device_id_t     device_id,
                             CA_IN  ca_boolean_t    enable,
                             CA_IN  ca_uint16_t     speed);

extern ca_status_t  cap_i2c_write (
                             CA_IN  ca_device_id_t          device_id,
                             CA_IN  ca_uint8_t       slave_addr,
                             CA_IN  ca_uint8_t     slave_offset,
                             CA_IN  ca_uint16_t        data_len,
                             CA_IN  ca_uint8_t       *p_tx_data/*[data_len:256]*/);

extern ca_status_t cap_i2c_read (
                             CA_IN  ca_device_id_t         device_id,
                             CA_IN  ca_uint8_t      slave_addr,
                             CA_IN  ca_uint8_t    slave_offset,
                             CA_IN  ca_uint16_t       data_len,
                             CA_OUT ca_uint8_t      *p_rx_data/*[data_len:256]*/);

extern ca_status_t  cap_i2c_mux_write (
                             CA_IN  ca_device_id_t          device_id,
                             CA_IN  ca_uint8_t       slave_addr,
                             CA_IN  ca_uint8_t       tx_data);

extern ca_status_t cap_hardware_timer_set (/*CODEGEN_IGNORE_TAG*/
                             CA_IN ca_device_id_t             device_id,
                             CA_IN ca_uint8                  timer_id,
                             CA_IN cap_hardware_timer_config_t       *p_cfg);

extern ca_status_t cap_hardware_timer_get (/*CODEGEN_IGNORE_TAG*/
                             CA_IN  ca_device_id_t     device_id,
                             CA_IN ca_uint8          timer_id,
                             CA_OUT  cap_hardware_timer_config_t      * p_cfg);

extern ca_status_t cap_hardware_timer_start (
                             CA_IN ca_device_id_t     device_id,
                             CA_IN ca_uint8          timer_id);

extern ca_status_t cap_hardware_timer_stop (
                             CA_IN ca_device_id_t     device_id,
                             CA_IN ca_uint8          timer_id);

extern ca_status_t cap_hardware_timer_counter_get (
                             CA_IN ca_device_id_t  device_id,
                             CA_IN ca_uint32_t  timer_id,
                             CA_OUT ca_uint32_t  *counter);
#endif

extern ca_status_t cap_watchdog_get(
                             CA_IN ca_device_id_t            device_id,
                             CA_OUT cap_watchdog_config_t     *config);

extern ca_status_t cap_watchdog_set(
                             CA_IN ca_device_id_t            device_id,
                             CA_IN cap_watchdog_config_t     *config);

extern ca_status_t cap_i2c_slave_disable(
                             CA_IN ca_device_id_t       device_id);

extern ca_status_t cap_i2c_slave_set(
                             CA_IN ca_device_id_t       device_id,
                             CA_IN cap_i2c_slave_t      *config);

extern ca_status_t cap_i2c_slave_get(
                             CA_IN ca_device_id_t          device_id,
                             CA_OUT cap_i2c_slave_t      *config);

extern ca_status_t  ca_fw_ver_info_get (
                             CA_IN ca_device_id_t device_id, 
                             CA_OUT ca_uint8_t *ver_info/*[64:64]*/);

extern ca_status_t cap_phy_register (/*CODEGEN_IGNORE_TAG*/
                             CA_IN ca_device_id_t             device_id,
                             CA_IN ca_uint32_t                 phy_type, 
                             CA_IN void *                          func_arg,
                             CA_IN void *                          drv_arg);

extern ca_status_t cap_phy_init (/*CODEGEN_IGNORE_TAG*/
                             CA_IN ca_device_id_t             device_id,
                             CA_IN ca_uint32_t                 phy_type, 
                             CA_IN void *      arg1,
                             CA_IN void *      arg2);

extern ca_status_t cap_phy_drv_get (/*CODEGEN_IGNORE_TAG*/
                             CA_IN ca_device_id_t  device_id, 
                             CA_IN ca_uint32_t phy_type, 
                             CA_OUT void ** phy_drv);

extern ca_uint32_t ca_plat_glb_reg_read(/*CODEGEN_IGNORE_TAG*/
                             CA_IN ca_device_id_t device_id,
                             CA_IN ca_uint32_t addr);

extern void ca_plat_glb_reg_write(/*CODEGEN_IGNORE_TAG*/
                                 CA_IN ca_device_id_t device_id,
                                 CA_IN ca_uint32_t addr,
                                 CA_IN ca_uint32_t data);

extern ca_uint32_t ca_plat_glb_reg_read(/*CODEGEN_IGNORE_TAG*/
                             CA_IN ca_device_id_t device_id, 
                             CA_IN ca_uint32_t addr);

extern void ca_plat_glb_reg_write(/*CODEGEN_IGNORE_TAG*/
                                 CA_IN ca_device_id_t device_id, 
                                 CA_IN ca_uint32_t addr, 
                                 CA_IN ca_uint32_t data);

#endif /* __SDL_PERI_H__ */

