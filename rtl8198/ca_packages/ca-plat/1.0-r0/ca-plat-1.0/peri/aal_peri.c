#include "ca_types.h"
#include "aal_peri.h"
#include "osal_cmn.h"
#include <linux/slab.h>
#include <linux/math64.h>

typedef struct {
   ca_uint32_t tmr_enable;
   ca_uint32_t tmr_val;
   ca_uint32_t clk_freq;
   ca_uint32_t tmr_load;
} ca_hwtimer_global_cfg_t;


static ca_hwtimer_global_cfg_t g_tmr_cfg [CA_DEV_ID_MAX][CA_TIMER_ID_MAX];

ca_status_t aal_per_i2c_set_speed(
                      ca_device_id_t    device_id,
                      ca_uint16   speed_khz,
                      ca_boolean       enbl)

{
   PER_BIW_CFG_t   biw_cfg;
   if( speed_khz < 1 || speed_khz > 400 ) {
      return CA_E_PARAM;
   }

    biw_cfg.wrd = CA_REG_READ(AAL_PER_REG_ADDR(PER_BIW_CFG));

    /* reset */
    if (biw_cfg.bf.core_en) {
        biw_cfg.bf.biw_soft_reset = 1;
        CA_REG_WRITE(biw_cfg.wrd, AAL_PER_REG_ADDR(PER_BIW_CFG));

        ca_mdelay(50);

        biw_cfg.bf.biw_soft_reset = 0;
        CA_REG_WRITE(biw_cfg.wrd, AAL_PER_REG_ADDR(PER_BIW_CFG));
    }

    biw_cfg.bf.prer = (AAL_PER_CLOCK / (5 * speed_khz))- 1;
    biw_cfg.bf.core_en =(enbl==TRUE)?1:0;

   CA_REG_WRITE(biw_cfg.wrd, AAL_PER_REG_ADDR(PER_BIW_CFG));

   return (CA_E_OK);
}



ca_status_t aal_per_i2c_get_speed(
                     ca_device_id_t  device_id,
                     ca_uint16  *p_speed)

{
   PER_BIW_CFG_t   biw_cfg;

   /*
    * The equation to compute the data to be programmed is
    * speed_khz = (per_clk_KHz * 1000)/(5 * (prer + 1))
    * where, per_clk = PER block/core clock, prer = reg field for clock period
    * Assumed peripheral clock frequency taken as Mhz not as Time period
    * 100KHz, the prer to be programmed is 199
    * 400KHz, the prer to be programmed is 49
    */

   biw_cfg.wrd = CA_REG_READ(AAL_PER_REG_ADDR(PER_BIW_CFG));
   if(biw_cfg.bf.prer) {
      *p_speed = ((AAL_PER_CLOCK)/(5 * (biw_cfg.bf.prer+1)));
   } else {
      *p_speed = 1;/*assign default speed*/
   }
   return(CA_E_OK);
}

void aal_per_i2c_clean(ca_device_id_t device_id)
{
    PER_BIW_CTRL_t  biw_ctrl;
    biw_ctrl.wrd = CA_REG_READ(AAL_PER_REG_ADDR(PER_BIW_CTRL));
    if (AAL_I2C_DONE == biw_ctrl.bf.biwdone) 
    {
        biw_ctrl.wrd = 0;
        biw_ctrl.bf.biwdone = 1;
    }
    else
    {
        biw_ctrl.wrd = 0;
    }
    CA_REG_WRITE(biw_ctrl.wrd, AAL_PER_REG_ADDR(PER_BIW_CTRL));
    return;
}

ca_status_t aal_per_i2c_set_intf_ctrl(
                     ca_device_id_t                device_id,
                     aal_i2c_intf_ctrl_t i2c_intf_ctrl)
{
   PER_BIW_CTRL_t  ctrl;
   ca_uint32  ctrl_val;

   ctrl_val = (AAL_I2C_DONE | AAL_I2C_ACK_IN |AAL_I2C_WRITE |
                   AAL_I2C_READ |AAL_I2C_STOP |AAL_I2C_START);
   /* Validate if there multiple control bit is set */
   if(!(i2c_intf_ctrl & ctrl_val)) {
      //printk("%s: Invalid I2C Ctrl Value %d \n", __func__, ctrl_val) ;
      return CA_E_PARAM;
   }

   ctrl.wrd = 0;
   ctrl.wrd = i2c_intf_ctrl;
   CA_REG_WRITE(ctrl.wrd, AAL_PER_REG_ADDR(PER_BIW_CTRL));
   return (CA_E_OK) ;
}


ca_status_t aal_per_i2c_poll_biw_done(
                            ca_device_id_t device_id)
{
   PER_BIW_CTRL_t  ctrl;
   ca_uint16  retry_cnt=0;
   /* Get the data till bytes_read - 1 */
   retry_cnt=AAL_PER_I2C_RETRY_CNT;
   do {
      /* Check if BIW_DONE? */
      ctrl.wrd = CA_REG_READ(AAL_PER_REG_ADDR(PER_BIW_CTRL));
      if (AAL_I2C_DONE == ctrl.bf.biwdone) {
         break;
       }
   } while (--retry_cnt);
   if (!retry_cnt){
      aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_STOP);
      return CA_E_BUSY;
   }
   return (CA_E_OK) ;
}


ca_status_t aal_per_i2c_check_ack(
                            ca_device_id_t device_id)
{
   PER_BIW_ACK_t    biw_ack;
   ca_uint16  retry_cnt=0;

   /* Get the data till bytes_read - 1 */
   retry_cnt=AAL_PER_I2C_RETRY_CNT;
   do {
      /* Check if BIW_DONE? */
      biw_ack.wrd = CA_REG_READ(AAL_PER_REG_ADDR(PER_BIW_ACK));
   } while ((--retry_cnt) && (biw_ack.bf.ack_out));
   if (!retry_cnt){
      aal_per_i2c_set_intf_ctrl(device_id, AAL_I2C_STOP);
      //printk("%s: I2C Slave no ACK\n", __func__) ;
      return CA_E_BUSY;
   }
   return (CA_E_OK) ;
}


ca_status_t aal_per_i2c_read(
                   ca_device_id_t device_id,
                   ca_uint8   *p_data)
{
   PER_BIW_RXR_t   rx_data;

   /* Read the Data */
   rx_data.wrd = CA_REG_READ(AAL_PER_REG_ADDR(PER_BIW_RXR));
   *(p_data) = rx_data.bf.rxr;
   return (CA_E_OK) ;
}


ca_status_t aal_per_i2c_write(
                  ca_device_id_t device_id,
                  ca_uint8   *p_data)
{
   PER_BIW_TXR_t   tx_data;

   tx_data.bf.txr = *p_data;
   CA_REG_WRITE(tx_data.wrd, AAL_PER_REG_ADDR(PER_BIW_TXR)) ;
   return (CA_E_OK) ;
}

ca_status_t aal_tmr_set_int(
                 ca_device_id_t             device_id,
                 ca_uint8_t              timer_id,
                 ca_boolean_t             mask)
{
    ca_uint32_t    tmr_var = 0;

    if(mask == TRUE)
    {
        CA_REG_WRITE(1, AAL_PER_REG_ADDR(PER_TMR_INT1_0 + (timer_id * 0x20))) ;
        CA_REG_WRITE(1, AAL_PER_REG_ADDR(PER_TMR_INT1_1 + (timer_id * 0x20))) ;
    }

    tmr_var = ((mask == TRUE)?0:1);

    CA_REG_WRITE(tmr_var, AAL_PER_REG_ADDR(PER_TMR_IE1_0 + (timer_id * 0x20))) ;
    CA_REG_WRITE(tmr_var, AAL_PER_REG_ADDR(PER_TMR_IE1_1 + (timer_id * 0x20))) ;

    return CA_E_OK;
}

ca_status_t aal_convert_usec_to_tmr(
                                            ca_uint32_t   clk_freq,
                                            ca_uint32_t   timer_usec,
                                            ca_uint32_t   *timer_cnt)
{
    ca_uint64_t tmp_usec = timer_usec;
    ca_uint64_t tmp_cnt = 0;
    ca_uint32_t remainder = 0;


    tmp_usec = tmp_usec*AAL_PER_CLOCK;

    if(clk_freq == AAL_TMR_CLK_DIRECT)
    {
        tmp_cnt = div_u64_rem(tmp_usec, (1000), &remainder);
    }
    else if(clk_freq == AAL_TMR_CLK_DIV64)
    {
        tmp_cnt = div_u64_rem(tmp_usec, (1000*64), &remainder);
    }
    else if(clk_freq == AAL_TMR_CLK_DIV1024)
    {
        tmp_cnt = div_u64_rem(tmp_usec, (1000*1024), &remainder);
    }
    else if(clk_freq == AAL_TMR_CLK_DIV4096)
    {
        tmp_cnt = div_u64_rem(tmp_usec, (1000*4096), &remainder);
    }

    if(tmp_cnt < 0xFFFFFFFF)
    {
        *timer_cnt = (ca_uint32_t)tmp_cnt;
        return CA_E_OK;
    }
    else
    {
        return CA_E_PARAM;
    }

}

ca_status_t aal_convert_tmr_to_usec(
                                            ca_uint32_t   clk_freq,
                                            ca_uint32_t   timer_cnt,
                                            ca_uint32_t *timer_usec)
{
    ca_uint64_t tmp_cnt = timer_cnt;
    ca_uint64_t tmp_usec = 0;
    ca_uint32_t remainder = 0;

   if(clk_freq == AAL_TMR_CLK_DIRECT)
   {
        tmp_usec = div_u64_rem((tmp_cnt * 1000), AAL_PER_CLOCK, &remainder);
   }
   else if(clk_freq == AAL_TMR_CLK_DIV64)
   {
        tmp_usec = div_u64_rem((tmp_cnt * 1000*64), AAL_PER_CLOCK, &remainder);
   }
   else if(clk_freq == AAL_TMR_CLK_DIV1024)
   {
        tmp_usec = div_u64_rem((tmp_cnt * 1000*1024), AAL_PER_CLOCK, &remainder);
   }
   else if(clk_freq == AAL_TMR_CLK_DIV4096)
   {
        tmp_usec = div_u64_rem((tmp_cnt * 1000*4096), AAL_PER_CLOCK, &remainder);

   }

   *timer_usec = (ca_uint32_t)tmp_usec;

    return CA_E_OK;
}

ca_status_t aal_tmr_set_cfg(
                 ca_device_id_t             device_id,
                 ca_uint8_t              timer_id,
                 ca_boolean_t             enbl,
                 ca_uint32_t            usec)
{
    int i = 0;
    ca_status_t ret = CA_E_OK;
    ca_uint32_t    tmr_ctrl = 0;
    ca_uint32_t    tmr_ld = 0xFFFFFFFF;
    ca_uint32_t    tmr_lde = 0;
    ca_uint32_t    tmr_inte = 0;
    ca_uint32_t    clk_freq = 0;
    void *  p_tmr_value = NULL;
    void *  p_tmr_ctrl = NULL;
    void *  p_tmr_ie = NULL;
    void *  p_tmr_lde = NULL;

    if((usec != 0)&&(enbl==TRUE))
    {
        for(i=AAL_TMR_CLK_DIV4096; i>=0; i--)
        {
            ret = aal_convert_usec_to_tmr(i, usec, &tmr_ld);
            if((ret == CA_E_OK)&&(tmr_ld > 1))
            {
                clk_freq = i;
                break;
            }
        }

        if(i == -1)
        {
            return CA_E_PARAM;
        }
    }

    /* Get the offset address */
    p_tmr_value = AAL_PER_REG_ADDR(PER_TMR_LD1 + (timer_id * 0x20));
    p_tmr_ctrl = AAL_PER_REG_ADDR(PER_TMR_CTRL1 + (timer_id * 0x20));
    p_tmr_ie = AAL_PER_REG_ADDR(PER_TMR_IE1_0 + (timer_id * 0x20));
    p_tmr_lde = AAL_PER_REG_ADDR(PER_TMR_LOADE);


    tmr_ctrl = ((clk_freq & 0xF)<<2);
    tmr_ctrl |= (((enbl == TRUE)?1:0)<< 6);
    tmr_ctrl |= (((enbl == TRUE)?1:0)<< 7);
    tmr_lde = CA_REG_READ(p_tmr_lde);
    tmr_lde |= (1 << (timer_id + 1));
    tmr_inte = ((enbl == TRUE)?1:0);

    CA_REG_WRITE(tmr_inte, p_tmr_ie);
    CA_REG_WRITE(tmr_ld, p_tmr_value);
    CA_REG_WRITE(tmr_lde, p_tmr_lde);
    CA_REG_WRITE(tmr_ctrl, p_tmr_ctrl);


    g_tmr_cfg[device_id][timer_id].clk_freq = clk_freq;
    g_tmr_cfg[device_id][timer_id].tmr_load = tmr_ld;
    if((usec == 0)&&(enbl==TRUE))
    {
        aal_convert_tmr_to_usec(clk_freq, tmr_ld, &g_tmr_cfg[device_id][timer_id].tmr_val);
    }
    else
    {
        g_tmr_cfg[device_id][timer_id].tmr_val = usec;
    }
    g_tmr_cfg[device_id][timer_id].tmr_enable = enbl;


    return CA_E_OK ;
}


ca_status_t aal_tmr_get_usec(
                 ca_device_id_t             device_id,
                 ca_uint8_t              timer_id,
                 ca_uint32_t            *usec)
{
    ca_uint32_t tmr_cnt = 0;

    if( g_tmr_cfg[device_id][timer_id].tmr_enable  == FALSE )
    {
        *usec  = 0;
        return CA_E_PARAM;
    }

    tmr_cnt = CA_REG_READ(AAL_PER_REG_ADDR(PER_TMR_CNT1 + (timer_id * 0x20)));

    tmr_cnt = g_tmr_cfg[device_id][timer_id].tmr_load - tmr_cnt;

    aal_convert_tmr_to_usec(g_tmr_cfg[device_id][timer_id].clk_freq, tmr_cnt, usec);

    return CA_E_OK ;
}

ca_status_t aal_set_wdt_config(
                              ca_device_id_t              dev_id,
                              aal_wtd_cfg_t   *p_cfg)
{

    ca_boolean_t        watch_dog_status;
    PER_WDT_CTRL_t    wdt_ctrl;
    PER_WDT_LOADE_t   ld_enbl;
    PER_WDT_PS_t      pre_scal;
    GLOBAL_GLOBAL_CONFIG_t glb_cfg;

    glb_cfg.wrd = CA_REG_READ(AAL_GLB_REG_ADDR(GLOBAL_GLOBAL_CONFIG));
    glb_cfg.bf.wd_reset_ext_reset = 1;
    glb_cfg.bf.wd_reset_remap = 1;
    glb_cfg.bf.wd_reset_all_blocks = 1;
    glb_cfg.bf.wd_reset_subsys_enable = 1;
    CA_REG_WRITE(glb_cfg.wrd, AAL_GLB_REG_ADDR(GLOBAL_GLOBAL_CONFIG));

    wdt_ctrl.wrd = CA_REG_READ(AAL_PER_REG_ADDR(PER_WDT_CTRL));
    watch_dog_status = wdt_ctrl.bf.wdten;

    /* Stop watch dog if running */
    if (watch_dog_status) {
        wdt_ctrl.bf.wdten = 0;
        CA_REG_WRITE(wdt_ctrl.wrd, AAL_PER_REG_ADDR(PER_WDT_CTRL));
    }

        /* Modify watchdog status as provided in input */
    watch_dog_status = p_cfg->wd_enable;

    CA_REG_WRITE(p_cfg->wd_timer, AAL_PER_REG_ADDR(PER_WDT_LD));

    if (p_cfg->wd_action ==  AAL_WATCHDOG_ACTION_REBOOT) {
        wdt_ctrl.bf.rsten = 1;
    }
    else {
        wdt_ctrl.bf.rsten = 0;
    }

    wdt_ctrl.bf.wdten = watch_dog_status;
    pre_scal.bf.wdtps =  0x0001E847;/*125000-1*/
    wdt_ctrl.bf.clksel = 1;       /* Select prescaler clock */
    ld_enbl.bf.update_pre = 1;    /* Update prescaler */
    ld_enbl.bf.update_wdt = 1;    /* Update load value */

    CA_REG_WRITE(pre_scal.wrd, AAL_PER_REG_ADDR(PER_WDT_PS));
    CA_REG_WRITE(ld_enbl.wrd, AAL_PER_REG_ADDR(PER_WDT_LOADE));
    CA_REG_WRITE(wdt_ctrl.wrd, AAL_PER_REG_ADDR(PER_WDT_CTRL));

    return CA_E_OK;
}

ca_status_t aal_get_wdt_config(
                              ca_device_id_t          dev_id,
                              aal_wtd_cfg_t           *p_cfg)
{
   PER_WDT_CTRL_t    wdt_ctrl;

   wdt_ctrl.wrd = CA_REG_READ(AAL_PER_REG_ADDR(PER_WDT_CTRL));
   p_cfg->wd_enable = wdt_ctrl.bf.wdten;

   if (wdt_ctrl.bf.rsten) {
      p_cfg->wd_action = AAL_WATCHDOG_ACTION_REBOOT;
   }
   else {
      p_cfg->wd_action = AAL_WATCHDOG_ACTION_REPORT;
   }

   p_cfg->wd_timer = CA_REG_READ(AAL_PER_REG_ADDR(PER_WDT_LD));

   return CA_E_OK;
}

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
ca_status_t aal_i2c_slave_init(ca_device_id_t  dev_id)
{
    GLOBAL_BLOCK_RESET_t gb_reset;
    GLOBAL_PIN_MUX_t pin_mux;
    IIC_SLV_IIC_SLV_CONFIG_t i2c_slave_cfg;

    gb_reset.wrd = CA_REG_READ(AAL_GLB_REG_ADDR(GLOBAL_BLOCK_RESET));
    gb_reset.bf.reset_iic_slv = 0;
    CA_REG_WRITE(gb_reset.wrd, AAL_GLB_REG_ADDR(GLOBAL_BLOCK_RESET));

    i2c_slave_cfg.wrd = 0;
    i2c_slave_cfg.bf.cfg_dev_id = 0x50;
    i2c_slave_cfg.bf.cfg_acs_en = 1;
    i2c_slave_cfg.bf.cfg_standard_iic = 1;
    CA_REG_WRITE(i2c_slave_cfg.wrd, AAL_PER_REG_ADDR(IIC_SLV_IIC_SLV_CONFIG));

    pin_mux.wrd = CA_REG_READ(AAL_GLB_REG_ADDR(GLOBAL_PIN_MUX));
    pin_mux.bf.iomux_rtk_biws_enable = 1;
    CA_REG_WRITE(pin_mux.wrd, AAL_GLB_REG_ADDR(GLOBAL_PIN_MUX));
    return CA_E_OK;
}

ca_status_t aal_i2c_slave_disable(ca_device_id_t  dev_id)
{
    GLOBAL_PIN_MUX_t pin_mux;
    pin_mux.wrd = CA_REG_READ(AAL_GLB_REG_ADDR(GLOBAL_PIN_MUX));
    pin_mux.bf.iomux_rtk_biws_enable = 0;
    CA_REG_WRITE(pin_mux.wrd, AAL_GLB_REG_ADDR(GLOBAL_PIN_MUX));
    return CA_E_OK;
}

ca_status_t aal_i2c_slave_set(ca_device_id_t  dev_id, ca_uint8_t addr, ca_uint8_t offset, ca_uint8_t data)
{
    IIC_SLV_REG_MEM_ACCESS_t i2c_slave_access;
    IIC_SLV_REG_MEM_DATA_t i2c_slave_data;

    if((addr != 0xA0)&&(addr != 0xA2))
        return CA_E_PARAM;

    i2c_slave_data.bf.data = data;
    CA_REG_WRITE(i2c_slave_data.wrd, AAL_PER_REG_ADDR(IIC_SLV_REG_MEM_DATA));

    
    i2c_slave_access.bf.access = 1;
    i2c_slave_access.bf.rbw = 1;
    if(addr == 0xA2)
        i2c_slave_access.bf.ADDR =offset|0x100;
    else
        i2c_slave_access.bf.ADDR =offset;

    CA_REG_WRITE(i2c_slave_access.wrd , AAL_PER_REG_ADDR(IIC_SLV_REG_MEM_ACCESS));

    return CA_E_OK;
}

ca_status_t aal_i2c_slave_get(ca_device_id_t  dev_id, ca_uint8_t addr, ca_uint8_t offset, ca_uint8_t* pdata)
{
    IIC_SLV_REG_MEM_ACCESS_t i2c_slave_access;
    IIC_SLV_REG_MEM_DATA_t i2c_slave_data;

    if((addr != 0xA0)&&(addr != 0xA2))
        return CA_E_PARAM;

    i2c_slave_access.bf.access = 1;
    i2c_slave_access.bf.rbw = 0;
    if(addr == 0xA2)
        i2c_slave_access.bf.ADDR =offset|0x100;
    else
        i2c_slave_access.bf.ADDR =offset;
    CA_REG_WRITE(i2c_slave_access.wrd , AAL_PER_REG_ADDR(IIC_SLV_REG_MEM_ACCESS));

    i2c_slave_data.wrd = CA_REG_READ(AAL_PER_REG_ADDR(IIC_SLV_REG_MEM_DATA));
    *pdata = i2c_slave_data.bf.data;

    return CA_E_OK;
}
#endif

void *aal_plat_peri_reg_iobase = NULL;
void aal_plat_peri_reg_base_init(void *iobase)
{
        aal_plat_peri_reg_iobase = iobase;
}

void *aal_plat_glb_reg_iobase = NULL;
void aal_plat_glb_reg_base_init(void *iobase)
{
        aal_plat_glb_reg_iobase = iobase;
}


