#ifndef __AAL_PERI_H__
#define __AAL_PERI_H__

#include "ca_types.h"
#include <soc/cortina/registers.h>

#define CA_DEV_ID_MAX                 2
#define CA_TIMER_ID_MAX             2

/*I2C*/
#define AAL_PER_CLOCK                  125000 /* 125000khz frequency*/

#define AAL_PER_I2C_RD_ACK             1
#define AAL_PER_I2C_RETRY_CNT          50000

typedef enum {
    AAL_WATCHDOG_ACTION_REBOOT   = 0,
    AAL_WATCHDOG_ACTION_REPORT   = 1
} aal_wtd_action_t;

typedef struct {
   ca_uint32_t        wd_enable;
   ca_uint32_t         wd_timer; /* watchdog timer in msec */
   ca_uint32_t         wd_action; /* Report or Warnong */
} aal_wtd_cfg_t;

typedef enum {
    AAL_TMR_CLK_DIRECT = 0,
    AAL_TMR_CLK_DIV64,
    AAL_TMR_CLK_DIV1024,
    AAL_TMR_CLK_DIV4096,
    AAL_TMR_MAX_CLK
} aal_tmr_clk_select_e;

/**************************
 *  base address
 **************************/
extern void * aal_plat_peri_reg_iobase;
extern void * aal_plat_glb_reg_iobase;
#define AAL_PER_REG_ADDR(addr)		((((ca_uint32_t)(addr) - PER_IRQ_SOFT0) & 0x00001FFF) + aal_plat_peri_reg_iobase)
#define AAL_GLB_REG_ADDR(addr)		((((ca_uint32_t)(addr) - GLOBAL_JTAG_ID) & 0x00000FFF) + aal_plat_glb_reg_iobase)

void aal_plat_glb_reg_base_init(void *iobase);
void aal_plat_peri_reg_base_init(void *iobase);

/******************************************************************************
 *              Typedefs and Datatypes for I2C
 *****************************************************************************/

typedef enum {
    AAL_I2C_DONE   = 0x00000001,
    AAL_I2C_ACK_IN = 0x00000008,
    AAL_I2C_WRITE  = 0x00000010,
    AAL_I2C_READ   = 0x00000020,
    AAL_I2C_STOP   = 0x00000040,
    AAL_I2C_START  = 0x00000080,
} aal_i2c_intf_ctrl_t;

typedef enum {
    AAL_I2C_INTF_READY = 0,
    AAL_I2C_INTF_BUSY,
    AAL_I2C_INTF_ERR
} aal_i2c_intf_status_t;


/******************************************************************************
 *                          APIs exposed to SDL
 *****************************************************************************/
extern ca_status_t aal_per_i2c_set_speed(ca_device_id_t device_id,
                                     ca_uint16   speed_khz,
                                     ca_boolean  enbl);

extern ca_status_t aal_per_i2c_get_speed(ca_device_id_t device_id,
                                     ca_uint16   *p_speed);

extern void aal_per_i2c_clean(ca_device_id_t device_id);

extern ca_status_t aal_per_i2c_set_intf_ctrl(ca_device_id_t device_id,
                       aal_i2c_intf_ctrl_t    i2c_intf_ctrl);

extern ca_status_t aal_per_i2c_poll_biw_done(ca_device_id_t device_id);

extern ca_status_t aal_per_i2c_check_ack(ca_device_id_t device_id);

extern ca_status_t aal_per_i2c_read(ca_device_id_t  device_id,
                              ca_uint8     *p_data);

extern ca_status_t aal_per_i2c_write(ca_device_id_t  device_id,
                                 ca_uint8     *p_data);

extern  ca_status_t aal_tmr_set_cfg(
                 ca_device_id_t            device_id,
                 ca_uint8_t              timer_id,
                 ca_boolean_t          enbl,
                 ca_uint32_t            usec);

extern  ca_status_t aal_tmr_set_int(
                 ca_device_id_t            device_id,
                 ca_uint8_t              timer_id,
                 ca_boolean_t           mask);

extern ca_status_t aal_tmr_get_cnt(
                 ca_device_id_t            device_id,
                 ca_uint8_t              timer_id,
                 ca_uint32_t*          cnt);

extern ca_status_t aal_tmr_get_usec(
                 ca_device_id_t             device_id,
                 ca_uint8_t              timer_id,
                 ca_uint32_t            *usec);

extern ca_status_t aal_set_wdt_config(
                              ca_device_id_t              dev_id,
                              aal_wtd_cfg_t   *p_cfg);

extern ca_status_t aal_get_wdt_config(
                              ca_device_id_t          dev_id,
                              aal_wtd_cfg_t           *p_cfg);

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
extern ca_status_t aal_i2c_slave_init(
                                ca_device_id_t  dev_id);

extern ca_status_t aal_i2c_slave_disable(
                                ca_device_id_t  dev_id);

extern ca_status_t aal_i2c_slave_set(
                                ca_device_id_t  dev_id, 
                                ca_uint8_t addr, 
                                ca_uint8_t offset, 
                                ca_uint8_t data);

extern ca_status_t aal_i2c_slave_get(
                                ca_device_id_t  dev_id, 
                                ca_uint8_t addr, 
                                ca_uint8_t offset, 
                                ca_uint8_t* pdata);
#endif
#endif /*__AAL_PER_H__*/

