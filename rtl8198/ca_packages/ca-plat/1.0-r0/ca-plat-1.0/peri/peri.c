#include "ca_types.h"
#include "aal_peri.h"
#include "peri.h"
#include "log.h"
#include "osal_cmn.h"
#include "ca_event.h"
#include "pal_comm.h"
#include "osal_timer.h"
#include "osal_mutex.h"
#include "ca_version.h"

#include <linux/slab.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#ifndef CONFIG_ARCH_RTL8198F
static ca_boolean g3_i2c_per_enable = FALSE;
#endif
extern int cap_log_kern_init(void);
extern void cap_log_kern_exit(void);
void __iomem *ca_plat_peri_reg_base = NULL;
void __iomem *ca_plat_glb_reg_base = NULL;

#ifndef CONFIG_ARCH_RTL8198F
static ca_uint32_t peri_tmr1_int_cnt = 0;
module_param(peri_tmr1_int_cnt, uint, 0644);
#endif

static ca_uint32_t i2c_slave_disable = 0;
module_param(i2c_slave_disable, uint, 0644);

static ca_uint32_t i2c_master_disable = 0;
module_param(i2c_master_disable, uint, 0644);

typedef struct
{
    struct work_struct work;
    ca_uint32_t data[32];
} peri_work_info_t;

typedef struct
{
    struct workqueue_struct *peri_wq;
    peri_work_info_t work_info;
} peri_wq_t;

typedef struct
{
    ca_uint32_t    enable;
    ca_uint32_t    rvsd;
    cap_hardware_timer_config_t cfg;
} ca_hw_tmr_cfg_t;

typedef struct
{
    ca_status_t (*phy_init) (ca_device_id_t, void *, void *);
    void * phy_drv;
} ca_phy_info_t;

typedef struct ca_peri_private {
    struct platform_device * peri_dev;
    spinlock_t peri_tmr_lock;
    ca_uint_t  peri_i2c_mutex;
    ca_hw_tmr_cfg_t tmr_cfg[2];
    ca_phy_info_t phy_info[CA_PHY_TYPE_MAX];
}ca_peri_private_t;

#ifndef CONFIG_ARCH_RTL8198F
peri_wq_t peri_tmr1_wq;
#endif
ca_peri_private_t ca_peri_info;

ca_status_t  ca_fw_ver_info_get (ca_device_id_t device_id, ca_uint8_t *ver_info)
{
    if(!ver_info)
    {
        return CA_E_PARAM;
    }
    memset(ver_info, 0, 64);
    snprintf(ver_info, 63, "%02d.%02d.%02d.%02d.%02d %s %s", IROS_VER_MAJOR, IROS_VER_MINOR, 
                IROS_VER_REVISION, IROS_VER_BUILD_ID, IROS_VER_INTERNAL_ID, __DATE__, __TIME__);
    return CA_E_OK;
}
EXPORT_SYMBOL(ca_fw_ver_info_get);

ca_status_t cap_phy_register (ca_device_id_t  device_id, ca_uint32_t phy_type, void * func_arg, void * drv_arg)
{
    if(phy_type >= CA_PHY_TYPE_MAX)
    {
        return CA_E_PARAM;
    }
    if(ca_peri_info.peri_dev && func_arg && drv_arg)
    {
        ca_peri_info.phy_info[phy_type].phy_init = func_arg;
        ca_peri_info.phy_info[phy_type].phy_drv = drv_arg;
    }else{
        ca_printf("CAP_PHY Register Fail!\n");
        return CA_E_PARAM;
    }
    ca_printf("cap_phy_register %p %p \n",ca_peri_info.phy_info[phy_type].phy_init, func_arg);
    return CA_E_OK;
}

ca_status_t cap_phy_init (ca_device_id_t  device_id, ca_uint32_t phy_type, void * arg1, void * arg2)
{
    ca_status_t ret= 0;
    if(phy_type >= CA_PHY_TYPE_MAX)
    {
        return CA_E_PARAM;
    }
    if(ca_peri_info.phy_info[phy_type].phy_init)
    {
        ret = (*ca_peri_info.phy_info[phy_type].phy_init) (device_id, arg1, arg2);
    }
    return ret;
}

ca_status_t cap_phy_drv_get(ca_device_id_t  device_id, ca_uint32_t phy_type, void **phy_drv)
{
	if ( phy_type >= CA_PHY_TYPE_MAX ) {
		return CA_E_PARAM;
	}

	if ( ca_peri_info.phy_info[phy_type].phy_drv ) {
		*phy_drv = ca_peri_info.phy_info[phy_type].phy_drv;
		return CA_E_OK;
	} else {
		return CA_E_RESOURCE;
	}
}

EXPORT_SYMBOL(cap_phy_register);
EXPORT_SYMBOL(cap_phy_init);
EXPORT_SYMBOL(cap_phy_drv_get);
#ifndef CONFIG_ARCH_RTL8198F
ca_status_t cap_i2c_enable_set (
                       ca_device_id_t  device_id,
                       ca_boolean   enable,
                       ca_uint16_t    speed)

{
   ca_status_t    sts = CA_E_OK;
#ifdef __KERNEL__
    g3_i2c_per_enable = enable;
#else
   ca_boolean per_i2c_enable = g3_i2c_per_enable;
   g3_i2c_per_enable = enable;
   if(enable==TRUE)
   {
        ca_mutex_lock(ca_peri_info.peri_i2c_mutex);
        sts = aal_per_i2c_set_speed(device_id, speed, g3_i2c_per_enable);
        ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
   }
   if (sts != CA_E_OK)
   {
        g3_i2c_per_enable = per_i2c_enable;
   }
#endif
   return sts;
}
EXPORT_SYMBOL(cap_i2c_enable_set);

ca_status_t  cap_i2c_write (
                      ca_device_id_t   device_id,
                      ca_uint8_t      slave_addr,
                      ca_uint8_t      slave_offset,
                      ca_uint16_t     data_len,
                      ca_uint8_t      *p_tx_data/*[data_len:256]*/)

{
#ifdef __KERNEL__
    struct i2c_adapter *adap;
    int err;
    struct i2c_msg msg[1];
    unsigned char data[257];
    if ((p_tx_data == NULL)||(data_len > 256)) {
        printk("%s: ERROR: Param wrong \n", __func__);
        return CA_E_PARAM;
    }
    if(i2c_master_disable)
        return CA_E_OK;
    ca_mutex_lock(ca_peri_info.peri_i2c_mutex);
    adap = i2c_get_adapter(device_id);
    if (!adap)
    {
        ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
        return CA_E_RESOURCE;
    }
    msg->addr = (slave_addr>>1); /* I2C address of chip */
    msg->flags = 0;
    msg->len = data_len + 1;
    msg->buf = data;
    data[0] = slave_offset; /* register num */
    memcpy(&data[1], p_tx_data, data_len);/* register data */
    err = i2c_transfer(adap, msg, 1);
    i2c_put_adapter(adap);
    ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
    if (err >= 0)
        return CA_E_OK;
    else
        return CA_E_BUSY;
#else
   ca_status_t     sts = CA_E_OK;
   ca_int32        indx;
   if ((p_tx_data == NULL)||(data_len > 256)) {
        printk("%s: ERROR: Param wrong \n", __func__);
        return CA_E_PARAM;
   }
    if(i2c_master_disable)
        return CA_E_OK;
   if (g3_i2c_per_enable == FALSE) {
        printk("%s: I2C interface is not enabled \n", __func__);
        return CA_E_PARAM;
   }
   ca_mutex_lock(ca_peri_info.peri_i2c_mutex);
   aal_per_i2c_clean(0);
   /* Write the slave addr */
   aal_per_i2c_write(device_id, &slave_addr);
   aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_START| AAL_I2C_WRITE);
   /* Poll biw_done until = 1 */
   sts = aal_per_i2c_poll_biw_done(device_id);
   if (sts == CA_E_OK){
        /* Clear biw_done */
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
        sts = aal_per_i2c_check_ack(device_id);
   }
   if(CA_E_OK != sts)
   {
        goto i2c_write_end;
   }

   /* Write the offset addr */
   aal_per_i2c_write(device_id, &slave_offset);
   aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_WRITE);
   /* Poll biw_done until = 1 */
   sts = aal_per_i2c_poll_biw_done(device_id);
   if (sts == CA_E_OK){
        /* Clear biw_done */
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
        sts = aal_per_i2c_check_ack(device_id);
   }
   if(CA_E_OK != sts)
   {
        goto i2c_write_end;
   }

   for(indx = 0; indx < data_len; indx++) {
        sts = aal_per_i2c_write(device_id,p_tx_data+indx);
        if (sts != CA_E_OK) {
            break;
        }
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_WRITE);
        sts = aal_per_i2c_poll_biw_done(device_id);
        if (sts != CA_E_OK) {
            break;
        }
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);

        sts = aal_per_i2c_check_ack(device_id);
        if (sts != CA_E_OK) {
            break;
        }
    }

   aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_STOP);
   /* Poll biw_done until = 1 */
   sts = aal_per_i2c_poll_biw_done(device_id);
   if (sts == CA_E_OK){
        /* Clear biw_done */
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
   }
i2c_write_end:
    ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
    return sts;
#endif
} /* ca_i2c_write */
EXPORT_SYMBOL(cap_i2c_write);

ca_status_t cap_i2c_read (
                        ca_device_id_t     device_id,
                        ca_uint8_t        slave_addr,
                        ca_uint8_t        slave_offset,
                        ca_uint16_t       data_len,
                        ca_uint8_t        *p_rx_data/*[data_len:256]*/)

{
#ifdef __KERNEL__
    struct i2c_adapter *adap;
    int err;
    struct i2c_msg msg[1];
    unsigned char data[2];
    if(i2c_master_disable)
        return CA_E_OK;
    if ((p_rx_data == NULL)||(data_len > 256)) {
        printk("%s: ERROR: Param wrong \n", __func__);
        return CA_E_PARAM;
    }
    ca_mutex_lock(ca_peri_info.peri_i2c_mutex);
    adap = i2c_get_adapter(device_id);
    if (!adap)
    {
        ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
        return CA_E_RESOURCE;
    }
    msg->addr = (slave_addr>>1); /* I2C address of chip */
    msg->flags = 0;
    msg->len = 1;
    msg->buf = data;
    data[0] = slave_offset;    /* register num */
    err = i2c_transfer(adap, msg, 1);

    msg->addr = (slave_addr>>1); /* I2C address */
    msg->flags = I2C_M_RD;
    msg->len = data_len;
    msg->buf = p_rx_data;
    err = i2c_transfer(adap, msg, 1);
    i2c_put_adapter(adap);
    ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
    if (err >= 0)
        return CA_E_OK;
    else
        return CA_E_BUSY;
#else
    ca_status_t  sts = CA_E_OK;
    ca_int32     indx = 0;
    ca_uint8     read_addr;
    if(i2c_master_disable)
        return CA_E_OK;
   if ((p_rx_data == NULL)||(data_len > 256)) {
        printk("%s: ERROR: Param wrong \n", __func__);
        return CA_E_PARAM;
   }
   if(g3_i2c_per_enable == FALSE) {
        printk("%s: I2C interface is not enabled \n", __func__);
        return CA_E_PARAM;
   }
   /* Write the slave addr */
   ca_mutex_lock(ca_peri_info.peri_i2c_mutex);
   aal_per_i2c_clean(0);
   aal_per_i2c_write(device_id, &slave_addr);
   aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_START| AAL_I2C_WRITE);
   /* Poll biw_done until = 1 */
   sts = aal_per_i2c_poll_biw_done(device_id);
   if (sts == CA_E_OK){
        /* Clear biw_done */
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
        sts = aal_per_i2c_check_ack(device_id);
   }
   if(CA_E_OK != sts)
   {
        goto  i2c_read_end;
   }

   /* Write the offset addr */
   aal_per_i2c_write(device_id, &slave_offset);
   aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_WRITE);
   /* Poll biw_done until = 1 */
   sts = aal_per_i2c_poll_biw_done(device_id);
   if (sts == CA_E_OK){
        /* Clear biw_done */
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
        sts = aal_per_i2c_check_ack(device_id);
   }
   if(CA_E_OK != sts)
   {
        goto  i2c_read_end;
   }

    /* send slave offset with start condition */
    /* set read flag LSB to 1 */
   read_addr = (slave_addr | 0x1);
   aal_per_i2c_write(device_id, &read_addr);
   aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_START| AAL_I2C_WRITE);
   /* Poll biw_done until = 1 */
   sts = aal_per_i2c_poll_biw_done(device_id);
   if (sts == CA_E_OK){
        /* Clear biw_done */
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
        sts = aal_per_i2c_check_ack(device_id);
   }
   if(CA_E_OK != sts)
   {
        goto  i2c_read_end;
   }

   for (indx = 0; indx < (data_len-1); indx++) {
            aal_per_i2c_set_intf_ctrl(device_id,AAL_I2C_READ);
            sts = aal_per_i2c_poll_biw_done(device_id);
            if (sts != CA_E_OK) {
               break;
            }
            aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
            sts = aal_per_i2c_read(device_id,p_rx_data+indx);
            if (sts != CA_E_OK) {
               break;
            }
    }

    /* read last byte, NACK and then generate STOP */
    (void)aal_per_i2c_set_intf_ctrl(device_id,(AAL_I2C_READ|AAL_I2C_STOP|AAL_I2C_ACK_IN));
    sts = aal_per_i2c_poll_biw_done(device_id);
    if (sts == CA_E_OK) {
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
        sts = aal_per_i2c_read(device_id,p_rx_data+indx);
    }
i2c_read_end:
    ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
    return sts;
#endif
} /* ca_i2c_read */
EXPORT_SYMBOL(cap_i2c_read);


ca_status_t  cap_i2c_mux_write (
                      ca_device_id_t   device_id,
                      ca_uint8_t      slave_addr,
                      ca_uint8_t      tx_data)

{
#ifdef __KERNEL__
    struct i2c_adapter *adap;
    int err;
    struct i2c_msg msg[1];
    unsigned char data[2];
    if(i2c_master_disable)
        return CA_E_OK;
    ca_mutex_lock(ca_peri_info.peri_i2c_mutex);
    adap = i2c_get_adapter(device_id);
    if (!adap)
    {
        ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
        return CA_E_RESOURCE;
    }
    msg->addr = (slave_addr>>1); /* I2C address of chip */
    msg->flags = 0;
    msg->len =  1;
    data[0] = tx_data; /* register num */
    msg->buf = data;
    err = i2c_transfer(adap, msg, 1);
    i2c_put_adapter(adap);
    ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
    if (err >= 0)
        return CA_E_OK;
    else
        return CA_E_BUSY;
#else
    ca_status_t     sts = CA_E_OK;
    ca_uint8_t  data = tx_data;
   /* Write the slave addr */
    if(i2c_master_disable)
        return CA_E_OK;
   ca_mutex_lock(ca_peri_info.peri_i2c_mutex);
   aal_per_i2c_clean(0);
   aal_per_i2c_write(device_id, &slave_addr);
   aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_START| AAL_I2C_WRITE);
   /* Poll biw_done until = 1 */
   sts = aal_per_i2c_poll_biw_done(device_id);
   if (sts == CA_E_OK){
        /* Clear biw_done */
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
        sts = aal_per_i2c_check_ack(device_id);
   }
   if(CA_E_OK != sts)
   {
        goto i2c_mut_write_end;
   }

   /* Write the Data */
   aal_per_i2c_write(device_id, &data);
   aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_WRITE|AAL_I2C_STOP);
   /* Poll biw_done until = 1 */
   sts = aal_per_i2c_poll_biw_done(device_id);
   if (sts == CA_E_OK){
        /* Clear biw_done */
        aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_DONE);
        sts = aal_per_i2c_check_ack(device_id);
   }
i2c_mut_write_end:
    ca_mutex_unlock(ca_peri_info.peri_i2c_mutex);
    return sts;
#endif
}
EXPORT_SYMBOL(cap_i2c_mux_write);

ca_status_t ca_hwtimer_int_mask (
                        ca_device_id_t     device_id,
                        ca_uint8          timer_id,
                        ca_uint8          mask_flag)
{
   if(!ca_peri_info.peri_dev)
   {
      return CA_E_RESOURCE;
   }

   if(timer_id >= CA_TIMER_ID_MAX)
   {
      return CA_E_PARAM;
   }

    aal_tmr_set_int(device_id, timer_id, mask_flag);

    return CA_E_OK;
}
EXPORT_SYMBOL(ca_hwtimer_int_mask);

ca_status_t cap_hardware_timer_set (
                        ca_device_id_t     device_id,
                        ca_uint8          timer_id,
                        cap_hardware_timer_config_t      * config)
{
    ca_uint32_t     ret = 0;
    unsigned long flags = 0;

    if(!ca_peri_info.peri_dev)
    {
        return CA_E_RESOURCE;
    }

    if(device_id >= CA_DEV_ID_MAX)
    {
        return CA_E_PARAM;
    }

    if(timer_id != 0) /*ONLY Timer0 work*/
    {
        return CA_E_PARAM;
    }

    if((config->mode != CA_TIMER_MODE_DISABLE)&&(config->int_handle==NULL))
    {
        return CA_E_PARAM;
    }

    spin_lock_irqsave(&ca_peri_info.peri_tmr_lock, flags);

    if(config->mode == CA_TIMER_MODE_RELOAD)
    {
        ret = aal_tmr_set_cfg(device_id, timer_id, TRUE, config->threshold);
    }
    else if (config->mode == CA_TIMER_MODE_ROLLOVER)
    {
        config->threshold= 0;
        ret = aal_tmr_set_cfg(device_id, timer_id, TRUE, config->threshold);
    }
    else
    {
        config->threshold= 0xFFFFFFFF;
        ret = aal_tmr_set_cfg(device_id, timer_id, FALSE, config->threshold);
        ca_peri_info.tmr_cfg[timer_id].enable = FALSE;
    }

    if(config->mode)
    {
        ca_peri_info.tmr_cfg[timer_id].enable = TRUE;
        ca_peri_info.tmr_cfg[timer_id].cfg.mode = config->mode;
        ca_peri_info.tmr_cfg[timer_id].cfg.threshold= config->threshold;
        ca_peri_info.tmr_cfg[timer_id].cfg.int_handle= config->int_handle;
        ca_peri_info.tmr_cfg[timer_id].cfg.int_cookie= config->int_cookie;
    }


    spin_unlock_irqrestore(&ca_peri_info.peri_tmr_lock, flags);


    return CA_E_OK;
}
EXPORT_SYMBOL(cap_hardware_timer_set);


ca_status_t cap_hardware_timer_get (
                        ca_device_id_t     device_id,
                        ca_uint8          timer_id,
                        cap_hardware_timer_config_t      * config)
{

    if(!ca_peri_info.peri_dev)
    {
        return CA_E_RESOURCE;
    }

    if(device_id >= CA_DEV_ID_MAX)
    {
        return CA_E_PARAM;
    }

    if(timer_id >= CA_TIMER_ID_MAX)
    {
        return CA_E_PARAM;
    }

    if(ca_peri_info.tmr_cfg[timer_id].enable)
    {
        config->mode = ca_peri_info.tmr_cfg[timer_id].cfg.mode;
        config->threshold = ca_peri_info.tmr_cfg[timer_id].cfg.threshold;
        config->int_handle = ca_peri_info.tmr_cfg[timer_id].cfg.int_handle;
        config->int_cookie = ca_peri_info.tmr_cfg[timer_id].cfg.int_cookie;
    }
    else
    {
        config->mode = CA_TIMER_MODE_DISABLE;
        config->threshold = 0;
        config->int_handle = NULL;
        config->int_cookie = NULL;
    }


    return CA_E_OK;
}
EXPORT_SYMBOL(cap_hardware_timer_get);


ca_status_t cap_hardware_timer_start (
                        ca_device_id_t     device_id,
                        ca_uint8          timer_id)
{
    ca_status_t ret = 0;
    cap_hardware_timer_config_t cfg;

    if(!ca_peri_info.peri_dev)
    {
        return CA_E_RESOURCE;
    }

    if(device_id >= CA_DEV_ID_MAX)
    {
        return CA_E_PARAM;
    }

    if(timer_id >= CA_TIMER_ID_MAX)
    {
        return CA_E_PARAM;
    }

    if(ca_peri_info.tmr_cfg[timer_id].enable == TRUE)
    {
        return CA_E_NOT_FOUND;
    }

    memcpy(&cfg, &ca_peri_info.tmr_cfg[timer_id].cfg, sizeof(cap_hardware_timer_config_t));

    printk("cap_hardware_timer_start %x  %x  %p \n", ca_peri_info.tmr_cfg[timer_id].cfg.mode, ca_peri_info.tmr_cfg[timer_id].cfg.threshold, ca_peri_info.tmr_cfg[timer_id].cfg.int_handle);

    ret = cap_hardware_timer_set(device_id, timer_id, &cfg);

    return ret;
}
EXPORT_SYMBOL(cap_hardware_timer_start);

ca_status_t cap_hardware_timer_stop (
                        ca_device_id_t     device_id,
                        ca_uint8          timer_id)
{
    ca_status_t ret = 0;
    cap_hardware_timer_config_t cfg;

    if(!ca_peri_info.peri_dev)
    {
        return CA_E_RESOURCE;
    }

    if(device_id >= CA_DEV_ID_MAX)
    {
        return CA_E_PARAM;
    }

    if(timer_id >= CA_TIMER_ID_MAX)
    {
        return CA_E_PARAM;
    }

    if(ca_peri_info.tmr_cfg[timer_id].cfg.mode == CA_TIMER_MODE_DISABLE)
    {
        return CA_E_RESOURCE;
    }



    memcpy(&cfg, &ca_peri_info.tmr_cfg[timer_id].cfg, sizeof(cap_hardware_timer_config_t));

    cfg.mode = CA_TIMER_MODE_DISABLE;

    ret = cap_hardware_timer_set(device_id, timer_id,  &cfg);

    return ret;
}
EXPORT_SYMBOL(cap_hardware_timer_stop);

ca_status_t cap_hardware_timer_counter_get (ca_device_id_t  device_id, ca_uint32_t  timer_id, ca_uint32_t  *counter)
{

    aal_tmr_get_usec(device_id, timer_id, counter);

    return CA_E_OK;
}
EXPORT_SYMBOL(cap_hardware_timer_counter_get);
#endif  // CONFIG_ARCH_RTL8198F

ca_status_t cap_watchdog_set(
                       ca_device_id_t             device_id,
                       cap_watchdog_config_t      *config)
{
    aal_wtd_cfg_t wtd_cfg;
    memset((void *)&wtd_cfg, 0, sizeof(aal_wtd_cfg_t));

    wtd_cfg.wd_enable = config->enable;
    wtd_cfg.wd_action = config->action;
    wtd_cfg.wd_timer = config->threshold;

    aal_set_wdt_config(device_id, &wtd_cfg);

    return CA_E_OK;
}
EXPORT_SYMBOL(cap_watchdog_set);

ca_status_t cap_watchdog_get(
                           ca_device_id_t            device_id,
                           cap_watchdog_config_t     *config)
{
    aal_wtd_cfg_t wtd_cfg;
    memset((void *)&wtd_cfg, 0, sizeof(aal_wtd_cfg_t));

    aal_get_wdt_config(device_id, &wtd_cfg);

    config->enable = wtd_cfg.wd_enable;
    config->action= wtd_cfg.wd_action;
    config->threshold= wtd_cfg.wd_timer;

    return CA_E_OK;
}
EXPORT_SYMBOL(cap_watchdog_get);

ca_uint32_t ca_plat_glb_reg_read(ca_device_id_t device_id, ca_uint32_t addr)
{
    return CA_REG_READ(AAL_GLB_REG_ADDR(addr));
}
EXPORT_SYMBOL(ca_plat_glb_reg_read);

void ca_plat_glb_reg_write(ca_device_id_t device_id, ca_uint32_t addr, ca_uint32_t data)
{
    CA_REG_WRITE(data, AAL_GLB_REG_ADDR(addr));
    return ;
}
EXPORT_SYMBOL(ca_plat_glb_reg_write);

ca_status_t cap_i2c_slave_disable(ca_device_id_t  device_id)
{
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS
    aal_i2c_slave_disable(device_id);
#endif
    return CA_E_OK;
}
EXPORT_SYMBOL(cap_i2c_slave_disable);

ca_status_t cap_i2c_slave_set(
                       ca_device_id_t             device_id,
                       cap_i2c_slave_t      *config)
{
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS
    aal_i2c_slave_set(device_id, config->address, config->offset, config->data);
#endif
    return CA_E_OK;
}
EXPORT_SYMBOL(cap_i2c_slave_set);

ca_status_t cap_i2c_slave_get(
                       ca_device_id_t             device_id,
                       cap_i2c_slave_t      *config)
{
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS
    aal_i2c_slave_get(device_id, config->address, config->offset, &config->data);
#endif
    return CA_E_OK;
}
EXPORT_SYMBOL(cap_i2c_slave_get);

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS
static void ca_i2c_slave_update(void * data)
{
    cap_i2c_slave_t cfg;
    ca_uint32_t i = 0;
    ca_uint8_t i2c_opm_data[2][256];
    static ca_uint8_t tmr_polling = 0;

    if(i2c_slave_disable)
        return;
    /*A0 address data should be constant, Only set once time*/
    if(!tmr_polling)
    {
        memset(i2c_opm_data, 0, sizeof(i2c_opm_data));
        if(cap_i2c_read(0, 0xA0, 0, 256, i2c_opm_data[0]))
        {
            return;
        }
        cfg.address = 0xA0;
        for(i=0; i<256; i++)
        {
            cfg.offset = i;
            cfg.data = i2c_opm_data[0][i];
            cap_i2c_slave_set(0, &cfg);
        }

        if(cap_i2c_read(0, 0xA2, 0, 256, i2c_opm_data[1]))
        {
            return;
        }
        cfg.address = 0xA2;
        for(i=0; i<256; i++)
        {
            cfg.offset = i;
            cfg.data = i2c_opm_data[1][i];
            cap_i2c_slave_set(0, &cfg);
        }
        tmr_polling = 1;
        return;
    }
    /*A2 address data should be update some bytes in real time*/
    if(cap_i2c_read(0, 0xA2, 0, 128, i2c_opm_data[1]))
    {
        return;
    }
    cfg.address = 0xA2;
    for(i=0; i<128; i++)
    {
        cfg.offset = i;
        cfg.data = i2c_opm_data[1][i];
        cap_i2c_slave_set(0, &cfg);
    }
    return ;
}
#endif


#ifndef CONFIG_ARCH_RTL8198F
void aal_tmr1_wq_proc(struct work_struct *wq)
{
    void * p_data = NULL;
    ca_status_t (*p_callback)(void *) = NULL;
    unsigned long flags = 0;

    spin_lock_irqsave(&ca_peri_info.peri_tmr_lock, flags);
    p_data = ca_peri_info.tmr_cfg[0].cfg.int_cookie;
    p_callback = ca_peri_info.tmr_cfg[0].cfg.int_handle;
    spin_unlock_irqrestore(&ca_peri_info.peri_tmr_lock, flags);

    if(p_callback)
    {
        p_callback(p_data);
    }

    spin_lock_irqsave(&ca_peri_info.peri_tmr_lock, flags);
    ca_hwtimer_int_mask(0, 0, FALSE);
    spin_unlock_irqrestore(&ca_peri_info.peri_tmr_lock, flags);

    return;
}

irqreturn_t aal_peri_irq_handle(int irq, void *p_dev_cb)
{
    unsigned long flags = 0;
    ca_uint32_t rc = 0;

    spin_lock_irqsave(&ca_peri_info.peri_tmr_lock, flags);
    /*Currently, Only timer_1 work*/
    ca_hwtimer_int_mask(0, 0, TRUE);

    rc = queue_work(peri_tmr1_wq.peri_wq, &(peri_tmr1_wq.work_info.work));

    peri_tmr1_int_cnt++;

    spin_unlock_irqrestore(&ca_peri_info.peri_tmr_lock, flags);

    return IRQ_HANDLED;
}
#endif  //CONFIG_ARCH_RTL8198F

static const struct of_device_id ca_peri_of_match[] = {
    { .compatible = "cortina,ca-peri", },
    {},
};

#ifndef CONFIG_ARCH_RTL8198F
ca_status_t ca_tmr_int_handle(void * data)
{
    ca_uint32_t cnt = 0;
    cap_hardware_timer_counter_get(0, 0, &cnt);

    printk("ca_tmr_int_handle cnt %d \n",cnt);

    return CA_E_OK;
}
#endif

static int __init ca_peri_probe(struct platform_device *pdev)
{
    int ret = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    int irq = 0;
    struct device *dev = &pdev->dev;
#endif
    struct device_node *np = pdev->dev.of_node;
    struct resource mem_resource;
    const struct of_device_id *match;


    memset(&ca_peri_info, 0, sizeof(ca_peri_info));
#ifndef CONFIG_ARCH_RTL8198F
    memset((char *)&peri_tmr1_wq, 0, sizeof(peri_tmr1_wq));
#endif

    cap_log_kern_init();
    cap_plat_event_init();
    ca_pal_init();

#ifndef CONFIG_ARCH_RTL8198F
    peri_tmr1_wq.peri_wq = create_workqueue("PERI_TMR1_WQ");
    INIT_WORK(&(peri_tmr1_wq.work_info.work), aal_tmr1_wq_proc);
#endif

    ca_peri_info.peri_dev = pdev;

#ifndef CONFIG_ARCH_RTL8198F
    spin_lock_init(&ca_peri_info.peri_tmr_lock);
#endif
    if(ca_mutex_init(&ca_peri_info.peri_i2c_mutex, "ca_i2c", 0))
    {
        printk("PERI Mutex init fail \n");
        return -EINVAL;
    }
    
    match = of_match_device(ca_peri_of_match, &pdev->dev);
    if (!match)
    {
        printk("PERI Match fail \n");
        return -EINVAL;
    }

    ret = of_address_to_resource(np, 0, &mem_resource);
    if (ret) {
        printk("%s: of_address_to_resource PERI return %d\n", __func__, ret);
        return 0;
    }
    ca_plat_peri_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
    if (!ca_plat_peri_reg_base) {
        printk("%s: ioremap(CA_PERI_BASE_ADDR) 1M failed!!!\n", __func__);
        return 0;
    }
    aal_plat_peri_reg_base_init(ca_plat_peri_reg_base);

    ret = of_address_to_resource(np, 1, &mem_resource);
    if (ret) {
        printk("%s: of_address_to_resource GLB return %d\n", __func__, ret);
        return 0;
    }
    ca_plat_glb_reg_base = devm_ioremap(&pdev->dev, mem_resource.start, resource_size(&mem_resource));
    if (!ca_plat_glb_reg_base) {
        printk("%s: ioremap(CA_GLB_BASE_ADDR) 1M failed!!!\n", __func__);
        return 0;
    }
    aal_plat_glb_reg_base_init(ca_plat_glb_reg_base);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU) || defined(CONFIG_ARCH_CORTINA_VENUS)
    cap_i2c_enable_set(0,TRUE,100);

    irq = platform_get_irq(ca_peri_info.peri_dev, 0);
    if (irq < 0)
    {
        printk("%s: interrupt fetch fail!!!\n", __func__);
        return  0;
    }


     if(request_irq(irq, aal_peri_irq_handle, 0, ca_peri_info.peri_dev->name, dev))
     {
        printk("%s: interrupt request fail!!!\n", __func__);
        return  0;
     }

    ca_printf("ca-peri:hw_timer irq %d install \n", irq);
#endif 

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS
    aal_i2c_slave_init(0);
    ca_i2c_slave_update(NULL);
    ca_circle_timer_add(5500, ca_i2c_slave_update, NULL);
    ca_printf("ca-peri: i2c slave install!\n");
#endif
    return 0;
}

static int __exit ca_peri_remove(struct platform_device *pdev)
{
    cap_log_kern_exit();

#ifndef CONFIG_ARCH_RTL8198F
    if (peri_tmr1_wq.peri_wq)
    {
        flush_workqueue(peri_tmr1_wq.peri_wq);
        destroy_workqueue(peri_tmr1_wq.peri_wq);
    }
#endif

    return 0;
}


MODULE_DEVICE_TABLE(of, ca_peri_of_match);

static struct platform_driver ca_peri_driver = {
    .remove     = __exit_p(ca_peri_remove),
    .probe      = ca_peri_probe,
    .driver = {
        .name   = "ca-peri",
        .owner  = THIS_MODULE,
        .of_match_table = ca_peri_of_match,
    },
};

module_platform_driver(ca_peri_driver);
MODULE_AUTHOR("Cortina SH");
MODULE_DESCRIPTION("CA77XX PERI Interface Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

/* End of File */

