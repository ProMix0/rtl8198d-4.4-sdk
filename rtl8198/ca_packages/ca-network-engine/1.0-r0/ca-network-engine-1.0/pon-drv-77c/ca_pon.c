#ifdef CONFIG_NE_PON
#include "util_misc.h"
#include <soc/cortina/registers.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/of_gpio.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/gpio/consumer.h>
#include "aal_pon.h"
#include "aal_pdc.h"
#include "aal_puc.h"
#include "aal_idp.h"
#include "aal_xcvr.h"
#include "aal_psds.h"
#include "ca_pon_proc.h"
#include "aal_epon_intr.h"
#include "aal_xgpon_intr.h"
#include "aal_gpon_intr.h"
#include "aal_gpon.h"
#include "peri.h"
#include "pon.h"
#include "scfg.h"

extern ca_status_t ca_gpon_cb_init (CA_IN ca_device_id_t device_id);
pon_wq_t    *aal_pon_wq[AAL_PON_KMSG_CHN_NUM];

#define  PON_INTR_DOSATTACK_THRESHOLD  32

static const struct of_device_id ca77xx_pon_of_match[] = {
    { .compatible = "cortina,ca77xx-pon", },
    {},
};

void *ca_venus_pon_reg_iobase = NULL;
void *ca_venus_pon_glb_reg_iobase = NULL;
void *ca_venus_peri_reg_iobase = NULL;

struct ca_pon_private {
    struct platform_device * pon_dev;
    struct gpio_desc *dying_gasp;
    struct gpio_desc *laser_tx_power;
    spinlock_t pon_lock;
    ca_uint32_t  irq_num;
} ca_pon_info;

ca_uint32_t g_pon_irqs[32] = {0};
ca_uint32_t g_dg_irq = 0;

/***********************************************************************************************************************/

static inline void __pon_top_intr_mask(void)
{
    ca_uint32_t top_ie = 0;
    top_ie &= ~VENUS_INTR_PON_MAC_BIT;
    AAL_GLB_REG_WRITE(top_ie, GLOBAL_PON_INTENABLE_0);
}

static inline void __pon_top_intr_ack(void)
{
}

static inline void __pon_top_intr_unmask(void)
{
    ca_uint32_t top_ie = 0;
    top_ie = AAL_GLB_REG_READ(GLOBAL_PON_INTENABLE_0);
    top_ie |= VENUS_INTR_PON_MAC_BIT;
    AAL_GLB_REG_WRITE(top_ie, GLOBAL_PON_INTENABLE_0);
}

ca_uint32_t aal_pon_int_cnt      = 0;
ca_uint32_t aal_pon_epon_cnt     = 0;
ca_uint32_t aal_pon_xgpon_cnt    = 0;
ca_uint32_t aal_pon_misc_int_cnt = 0;
ca_uint32_t ca_pon_debug;

EXPORT_SYMBOL(ca_pon_debug);
EXPORT_SYMBOL(aal_pon_int_cnt);
EXPORT_SYMBOL(aal_pon_epon_cnt);
EXPORT_SYMBOL(aal_pon_xgpon_cnt);
EXPORT_SYMBOL(aal_pon_misc_int_cnt);

pon_wq_t    gpon_intr_wq;
pon_wq_t    pon_misc_wq;
pon_wq_t    pon_ploam_wq;
pon_wq_t    pon_ploam_key_ctrl_wq;
pon_wq_t    pon_ploam_key_tk4_wq;
pon_wq_t    pon_ploam_key_tk5_wq;
pon_wq_t    pon_ploam_rgst_wq;
pon_wq_t    pon_act_wq;
ca_uint32_t ploam_use_tasklet = 0; /* use tasklet for PLOAM or not */
ca_uint32_t act_use_tasklet   = 0; /* use tasklet for Activation or not */
ca_uint32_t aes_ctrl_use_tasklet   = 0; /* use tasklet for AES key FSM or not */
ca_uint32_t aes_tk4_use_tasklet   = 0; /* use tasklet for AES key FSM or not */
ca_uint32_t aes_tk5_use_tasklet   = 0; /* use tasklet for AES key FSM or not */
ca_uint32_t ploam_rgst_use_tasklet = 0; /* use tasklet for PLOAM or not */
ca_uint32_t gpon_intr_tasklet = 0; 

module_param(ploam_use_tasklet, uint, 0644);
module_param(act_use_tasklet, uint, 0644);
module_param(aes_ctrl_use_tasklet, uint, 0644);
module_param(aes_tk4_use_tasklet, uint, 0644);
module_param(aes_tk5_use_tasklet, uint, 0644);
module_param(ploam_rgst_use_tasklet, uint, 0644);
module_param(gpon_intr_tasklet, uint, 0644);

EXPORT_SYMBOL(ploam_use_tasklet);
EXPORT_SYMBOL(act_use_tasklet);
EXPORT_SYMBOL(aes_ctrl_use_tasklet);
EXPORT_SYMBOL(aes_tk4_use_tasklet);
EXPORT_SYMBOL(aes_tk5_use_tasklet);
EXPORT_SYMBOL(ploam_rgst_use_tasklet);
EXPORT_SYMBOL(gpon_intr_tasklet);


pon_wq_t epon_1gepn_intr_wq;    /* work queue for EPON 1GEPON */
pon_wq_t epon_xgepn_intr_wq;    /* work queue for EPON XGEPON */
pon_wq_t epon_mpcp_intr_wq;     /* work queue for EPON MPCP   */

static ca_uint32_t aal_init_pon = 0;
//module_param(aal_init_pon, uint, 0644);


static inline ca_boolean_t __pon_top_intr_chk(void)
{
    return TRUE;
}


extern void aal_gpon_intr_handler(
    ca_device_id_t device_id,
    ca_uint32_t    gpon_int);

static irqreturn_t __pon_isr(int irq, void *p_dev_cb)
{
    unsigned long flags = 0;
    ca_uint32_t   poll_cnt = 0;
    ca_uint32_t   pon_ie  = 0;
    ca_uint32_t   pon_int = 0, int_tmp = 0;
    ca_uint32_t   pon_mode = aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID);

    PON_DBG("%s(): rx irq %d\n", __func__, irq);

    if (!aal_init_pon) {
        PON_DBG("error: PON driver was not init!\n");
        return IRQ_NONE;
    }

    aal_pon_int_cnt++;

    spin_lock_irqsave(&ca_pon_info.pon_lock, flags);

    if (!__pon_top_intr_chk()) {
        spin_unlock_irqrestore(&ca_pon_info.pon_lock, flags);
        return IRQ_NONE;
    }

    __pon_top_intr_mask(); /* Disable SoC IRQ */

    __pon_top_intr_ack();

    /* polling and scan PON interrupts to accelerate interrupt handling */
    while (poll_cnt < PON_INTR_DOSATTACK_THRESHOLD) {
      // TODO: Another choice: get all bits, clear all, and then send wq one by one
        if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
            if (pon_mode >= ONU_PON_MAC_XGPON1 && pon_mode <= ONU_PON_MAC_NGPON2) {
                /* XGPON IRQ */
                PON_REG_DUMP(XGPN_XGPN_GLB_INTERRUPT0);/* Debug dumps */
                pon_ie  = AAL_PON_REG_READ(XGPN_XGPN_GLB_INTERRUPT0E);
                AAL_PON_REG_WRITE(0, XGPN_XGPN_GLB_INTERRUPT0E);
                pon_int = AAL_PON_REG_READ(XGPN_XGPN_GLB_INTERRUPT0);
                int_tmp = pon_int;
                pon_int &= pon_ie;

                if (pon_int) {
#ifdef CONFIG_PROC_FS
                    if (PON_DBG_INTR & ca_pon_debug) {
                        ca_printf("%s(): irq %d rx XGPN_XGPN_GLB_INTERRUPT %#x\n",
                            __func__, irq, pon_int);
                    }
#endif
                    aal_pon_xgpon_cnt++;
                    aal_xgpon_intr_handler(AAL_DEFAUL_DEV_ID, irq, pon_int);
                } else if (int_tmp) { /* No more XGPON IRQ */
                    aal_pon_misc_int_cnt++;
                }
                AAL_PON_REG_WRITE(pon_ie, XGPN_XGPN_GLB_INTERRUPT0E);
            } else if (pon_mode == ONU_PON_MAC_GPON) {
                pon_ie = AAL_PON_REG_READ(GPON_MAC_GPON_int_top_en);
                AAL_PON_REG_WRITE(0, GPON_MAC_GPON_int_top_en);
                pon_int = AAL_PON_REG_READ(GPON_MAC_GPON_interrupt_top);
                int_tmp = pon_int;
                pon_int &= pon_ie;

                if (pon_int) {
                    aal_gpon_intr_handler(AAL_DEFAUL_DEV_ID, pon_int);
                }
                AAL_PON_REG_WRITE(pon_ie, GPON_MAC_GPON_int_top_en);
            }
        }
        else if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {

            PON_REG_DUMP(EPON_EPON_GLB_INTERRUPT);/* Debug dumps */
            pon_ie  = AAL_PON_REG_READ(EPON_EPON_GLB_INTERRUPTE);
            AAL_PON_REG_WRITE(0, EPON_EPON_GLB_INTERRUPTE);
            pon_int = AAL_PON_REG_READ(EPON_EPON_GLB_INTERRUPT);
            int_tmp = pon_int;
            pon_int &= pon_ie;

            if (pon_int) { /* EPON IRQ */
#ifdef CONFIG_PROC_FS
                if (PON_DBG_INTR & ca_pon_debug) {
                    ca_printf("%s(): irq %d rx EPON_EPON_GLB_INTERRUPT %#x\n",
                        __func__, irq, pon_int);
                }
#endif
                aal_pon_epon_cnt++;
                (void)aal_epon_intr_glb_handler(AAL_DEFAUL_DEV_ID, irq, pon_int);
            } else if (int_tmp) {/* No more EPON IRQ */
                aal_pon_misc_int_cnt++;
            }
            AAL_PON_REG_WRITE(pon_ie, EPON_EPON_GLB_INTERRUPTE);
        }

        if (!pon_int) {
            break;
        }

        poll_cnt ++;
    }

    __pon_top_intr_unmask();/* Re-enable SoC INTR */
    spin_unlock_irqrestore(&ca_pon_info.pon_lock, flags);
    return IRQ_HANDLED;

}

static ca_status_t __gpon_wq_init(ca_scfg_pon_mac_mode_t pon_mode)
{
    //seprate gpon and other mode
    if (ONU_PON_MAC_GPON == pon_mode)
    {
        memset((char *)&gpon_intr_wq, 0, sizeof(gpon_intr_wq));

        gpon_intr_wq.pon_wq = create_workqueue("GPON_INTR_WQ");
        if (!gpon_intr_wq.pon_wq) {
            ca_printf("Create gpon_intr_wq fail.\n");
            return CA_E_RESOURCE;
        }

        INIT_WORK(&(gpon_intr_wq.work_info.work), aal_gpon_intr_wq_proc);
        ca_printf("ca-pon: Init gpon workqueue OK.\n");
        return CA_E_OK;
    }

    memset(aal_pon_wq, 0, sizeof(aal_pon_wq));
    memset((char *)&pon_misc_wq, 0, sizeof(pon_misc_wq));
    memset((char *)&pon_ploam_wq, 0, sizeof(pon_ploam_wq));
    memset((char *)&pon_ploam_key_ctrl_wq, 0, sizeof(pon_ploam_key_ctrl_wq));
    memset((char *)&pon_ploam_key_tk4_wq, 0, sizeof(pon_ploam_key_tk4_wq));
    memset((char *)&pon_ploam_key_tk5_wq, 0, sizeof(pon_ploam_key_tk5_wq));
	memset((char *)&pon_ploam_rgst_wq, 0, sizeof(pon_ploam_rgst_wq));
    memset((char *)&pon_act_wq, 0, sizeof(pon_act_wq));

    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        pon_ploam_wq.pon_wq = create_workqueue("PLOAM_WQ");
        if (!pon_ploam_wq.pon_wq) {
            ca_printf("Create pon_ploam_wq fail.\n");
            return CA_E_RESOURCE;
        }
        INIT_WORK(&(pon_ploam_wq.work_info.work), aal_ploam_wq_proc);
        aal_pon_wq[AAL_PON_KMSG_CHN_PLOAM] = &pon_ploam_wq;

        pon_ploam_key_ctrl_wq.pon_wq = create_workqueue("PLOAM_KEY_CTRL_WQ");
        if (!pon_ploam_key_ctrl_wq.pon_wq) {
            ca_printf("Create pon_ploam_key_exchg_wq fail.\n");
            return CA_E_RESOURCE;
        }
        INIT_WORK(&(pon_ploam_key_ctrl_wq.work_info.work), aal_ploam_key_exchg_wq_proc);
        aal_pon_wq[AAL_PON_KMSG_CHN_AES_KEY_CTRL] = &pon_ploam_key_ctrl_wq;
		pon_ploam_key_tk4_wq.pon_wq = create_workqueue("PLOAM_KEY_TK4_WQ");
        if (!pon_ploam_key_tk4_wq.pon_wq) {
            ca_printf("Create pon_ploam_key_tk4_wq fail.\n");
            return CA_E_RESOURCE;
        }
        INIT_WORK(&(pon_ploam_key_tk4_wq.work_info.work), aal_ploam_key_exchg_wq_proc);
        aal_pon_wq[AAL_PON_KMSG_CHN_AES_KEY_TK4] = &pon_ploam_key_tk4_wq;
		pon_ploam_key_tk5_wq.pon_wq = create_workqueue("PLOAM_KEY_TK5_WQ");
        if (!pon_ploam_key_tk5_wq.pon_wq) {
            ca_printf("Create pon_ploam_key_tk5_wq fail.\n");
            return CA_E_RESOURCE;
        }
        INIT_WORK(&(pon_ploam_key_tk5_wq.work_info.work), aal_ploam_key_exchg_wq_proc);
        aal_pon_wq[AAL_PON_KMSG_CHN_AES_KEY_TK5] = &pon_ploam_key_tk5_wq;

		pon_ploam_rgst_wq.pon_wq = create_workqueue("PLOAM_RGST_WQ");
        if (!pon_ploam_rgst_wq.pon_wq) {
            ca_printf("Create pon_ploam_rgst_wq fail.\n");
            return CA_E_RESOURCE;
        }
#if 0
        INIT_WORK(&(pon_ploam_rgst_wq.work_info.work), aal_ploam_rgst_wq_proc);
        aal_pon_wq[AAL_PON_KMSG_CHN_RGST] = &pon_ploam_rgst_wq;
#endif
        pon_act_wq.pon_wq = create_workqueue("ACTIVATE_WQ");
        if (!pon_act_wq.pon_wq) {
            ca_printf("Create pon_act_wq fail.\n");
            return CA_E_RESOURCE;
        }
        INIT_WORK(&(pon_act_wq.work_info.work), aal_act_wq_proc);
        aal_pon_wq[AAL_PON_KMSG_CHN_ACTP] = &pon_act_wq;
        
        pon_misc_wq.pon_wq = create_workqueue("PON_MISC_INTR_WQ");
        if (!pon_misc_wq.pon_wq) {
            ca_printf("Create pon_misc_wq fail.\n");
            return CA_E_RESOURCE;
        }
        INIT_WORK(&(pon_misc_wq.work_info.work), aal_misc_wq_proc);
        aal_pon_wq[AAL_PON_KSMG_CHN_MISC] = &pon_misc_wq;
    }

    return CA_E_OK;

}

static ca_status_t __epon_wq_init(int pon_mode)
{ 
    memset((char *)&epon_1gepn_intr_wq, 0, sizeof(epon_1gepn_intr_wq));
    memset((char *)&epon_xgepn_intr_wq, 0, sizeof(epon_xgepn_intr_wq));
    memset((char *)&epon_mpcp_intr_wq, 0, sizeof(epon_mpcp_intr_wq));

    /* EPON work queue */
    if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {
        epon_1gepn_intr_wq.pon_wq = create_workqueue("EPON_1GEPN_INTR_WQ");
        if (!epon_1gepn_intr_wq.pon_wq) {
            ca_printf("Create epon_1gepn_intr_wq fail.\n");
            return CA_E_RESOURCE;
        }

        INIT_WORK(&(epon_1gepn_intr_wq.work_info.work), aal_epon_intr_1gepn_msg_proc);
        //aal_pon_wq[AAL_PON_KMSG_CHN_EPON_MAC] = &epon_1gepn_intr_wq;
        
        epon_xgepn_intr_wq.pon_wq = create_workqueue("EPON_XGEPN_INTR_WQ");
        if (!epon_xgepn_intr_wq.pon_wq) {
            ca_printf("Create epon_xgepn_intr_wq fail.\n");
            return CA_E_RESOURCE;
        }

        INIT_WORK(&(epon_xgepn_intr_wq.work_info.work), aal_epon_intr_xgepn_msg_proc);
        //aal_pon_wq[AAL_PON_KMSG_CHN_EPON_MAC] = &epon_xgepn_intr_wq;
        
        epon_mpcp_intr_wq.pon_wq = create_workqueue("EPON_MPCP_INTR_WQ");
        if (!epon_mpcp_intr_wq.pon_wq) {
            ca_printf("Create epon_mpcp_intr_wq fail.\n");
            return CA_E_RESOURCE;
        }
        INIT_WORK(&(epon_mpcp_intr_wq.work_info.work), aal_epon_intr_mpcp_msg_proc);
        //aal_pon_wq[AAL_PON_KMSG_CHN_MPCP] = &epon_mpcp_intr_wq;
    }

    return CA_E_OK;

}

static ca_status_t __pon_wq_init(int pon_mode)
{
    ca_status_t ret = CA_E_OK;
    
    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode))
        ret |= __gpon_wq_init(pon_mode);
    else if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode))
        ret |= __epon_wq_init(pon_mode);

    return ret;

}

static int __pon_wq_exit(void)
{
    ca_uint32_t pon_mode = aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID);

    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)){
        if (ONU_PON_MAC_GPON != pon_mode) {
            if (pon_ploam_wq.pon_wq) {
                flush_workqueue(pon_ploam_wq.pon_wq);
                destroy_workqueue(pon_ploam_wq.pon_wq);
            }

            if (pon_act_wq.pon_wq) {
                flush_workqueue(pon_act_wq.pon_wq);
                destroy_workqueue(pon_act_wq.pon_wq);
            }

            if (pon_ploam_key_ctrl_wq.pon_wq) {
                flush_workqueue(pon_ploam_key_ctrl_wq.pon_wq);
                destroy_workqueue(pon_ploam_key_ctrl_wq.pon_wq);
            }
    		if (pon_ploam_key_tk4_wq.pon_wq) {
                flush_workqueue(pon_ploam_key_tk4_wq.pon_wq);
                destroy_workqueue(pon_ploam_key_tk4_wq.pon_wq);
            }
    		if (pon_ploam_key_tk5_wq.pon_wq) {
                flush_workqueue(pon_ploam_key_tk5_wq.pon_wq);
                destroy_workqueue(pon_ploam_key_tk5_wq.pon_wq);
            }

    		if (pon_ploam_rgst_wq.pon_wq) {
                flush_workqueue(pon_ploam_rgst_wq.pon_wq);
                destroy_workqueue(pon_ploam_rgst_wq.pon_wq);
            }

            if (pon_misc_wq.pon_wq) {
                flush_workqueue(pon_misc_wq.pon_wq);
                destroy_workqueue(pon_misc_wq.pon_wq);
            }
        }
        else
        {
            //reset gpon mode work queue
            if (gpon_intr_wq.pon_wq) {
                flush_workqueue(gpon_intr_wq.pon_wq);
                destroy_workqueue(gpon_intr_wq.pon_wq);
            }
        }
        memset(aal_pon_wq, 0, sizeof(aal_pon_wq));
    }
    else if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {
        if (epon_1gepn_intr_wq.pon_wq) {
                flush_workqueue(epon_1gepn_intr_wq.pon_wq);
                destroy_workqueue(epon_1gepn_intr_wq.pon_wq);
        }
        if (epon_xgepn_intr_wq.pon_wq) {
                flush_workqueue(epon_xgepn_intr_wq.pon_wq);
                destroy_workqueue(epon_xgepn_intr_wq.pon_wq);
        }

        if (epon_mpcp_intr_wq.pon_wq) {
            flush_workqueue(epon_mpcp_intr_wq.pon_wq);
            destroy_workqueue(epon_mpcp_intr_wq.pon_wq);
        }
    }

    return 0;

}


irqreturn_t ca_pon_dying_gasp_irq(int irq, void *cookie)
{
    ca_status_t         ret = CA_E_OK;
    aal_xgpon_usbg_xgtc_hd_ind_config_msk_t mask;
    aal_xgpon_usbg_xgtc_hd_ind_config_t hd_ind_cfg;
    ca_uint32_t         pon_mode = aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID);

    printk("<%s, %d>pon_mode:%d\r\n", __FUNCTION__, __LINE__, pon_mode);

    if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {
        aal_epon_dying_gasp_handler(0, 0);
    }
    else if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        if(pon_mode == ONU_PON_MAC_GPON) {
            ret = aal_gpon_send_dying_gasp(0, 0);
        }
        else {
            mask.u32 = 0;
            mask.s.dg = 1;
            hd_ind_cfg.dg = 1;
            ret = aal_xgpon_usbg_xgtc_hd_ind_cfg_set(0, mask, &hd_ind_cfg);
        }
        if (ret == CA_E_OK) {
            aal_gpon_dying_gasp_handler(0, 0);
        }
    }
    return 0;
}

static irqreturn_t __dg_isr(int irq, void *p_dev_cb)
{
    PER_PERIPHERAL_INTENABLE_0_t peri;

    peri.wrd = AAL_PERI_REG_READ(PER_PERIPHERAL_INTENABLE_0);
    peri.bf.dying_gaspe = 0;
    AAL_PERI_REG_WRITE(peri.wrd,PER_PERIPHERAL_INTENABLE_0);

    AAL_PERI_REG_WRITE(0,PER_DG_IE_0);
    AAL_PERI_REG_WRITE(1,PER_DG_INT_0);

    ca_pon_dying_gasp_irq(irq,p_dev_cb);

    return IRQ_HANDLED;
}

#if defined(CONFIG_BOARD_REALTEK_CA8277_PON_GPIO)
#define CA_G3HGU_LASER_TX_GPIO_ID   96
#define CA_G3HGU_LASER_TX_DISABLE_GPIO_ID   97
#endif

static ca_status_t ca_gpio_register_set(ca_uint8_t mode_set, ca_uint32_t gpio_id, ca_uint8_t data)
{
    return CA_E_OK;
}
ca_status_t ca_pon_laser_tx_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t data)
{
    ca_uint32_t gpio_id = 0; 
    static ca_uint8_t mode_set = 0;
    ca_uint32_t customer_bit = 0xFFFFFFFF;

    if(data)
        data = 1;

    gpio_id = CA_G3HGU_LASER_TX_GPIO_ID;

    ca_gpio_register_set(mode_set, gpio_id, data);

    mode_set = 1;
    return CA_E_OK;
}
EXPORT_SYMBOL(ca_pon_laser_tx_set);

#define CA_NI_MEM_RESOURCE_PON  10

#if defined(CONFIG_BOARD_REALTEK_CA8277_PON_GPIO)

ca_status_t ca_pon_laser_tx_disable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint8_t data)
{
    ca_uint32_t gpio_id = 0; 
    static ca_uint8_t mode_set = 0;
    ca_uint32_t customer_bit = 0xFFFFFFFF;

    if(data)
        data = 1;

    gpio_id = CA_G3HGU_LASER_TX_DISABLE_GPIO_ID;

    ca_gpio_register_set(mode_set, gpio_id, data);

    mode_set = 1;
    return CA_E_OK;
}
EXPORT_SYMBOL(ca_pon_laser_tx_disable_set);
#endif

static int __pon_dev_init(struct platform_device *pdev)
{
    struct device_node *parent_np = pdev->dev.of_node;
    struct device_node *np = of_get_child_by_name(parent_np, "pon_ictl");//pdev->dev.of_node;
    struct device_node *dg_np = NULL;
    int irq = 0;
    int irq_num = 0;

    memset(&ca_pon_info, 0, sizeof(ca_pon_info));

    if (1)//(of_match_device(ca77xx_pon_of_match, &pdev->dev)) 
    {
        ca_pon_info.pon_dev = pdev;
    } 
    else 
    {
        ca_printf("ca-pon: error (PON Match fail)!\n");
        return -EINVAL;
    }

    spin_lock_init(&ca_pon_info.pon_lock);
    ca_printf("ca-pon: Device Tree load start\n");

    dg_np = of_get_child_by_name(parent_np, "dg_int");
    if(dg_np != NULL)
    {
        g_dg_irq = (ca_uint32_t)of_irq_get(dg_np, 0);
        ca_printf("ca-pon: dg interrupt is %d\r\n", g_dg_irq);

        if(request_irq(g_dg_irq, __dg_isr, IRQF_TRIGGER_FALLING, "dying_gasp", NULL))
        {
            ca_printf("Failed to request Dying Gasp IRQ\n");
        }
    }
    irq_num = 1; //of_property_count_u32_elems(np, "interrupts");

    ca_pon_info.irq_num = irq_num;
    ca_printf("ca-pon: found %d interrupts in Device Tree\n", irq_num);
    for (irq = 0; irq < ca_pon_info.irq_num; irq++) {
        g_pon_irqs[irq] = (ca_uint32_t)of_irq_get(np, irq);
        ca_printf("ca-pon: interrupt %d index is %d\r\n", irq, g_pon_irqs[irq]);
    }
    ca_printf("ca-pon: Device Tree load OK\n");

    return 0;

}

int __pon_dev_exit(struct platform_device *pdev)
{
    if (spin_is_locked(&ca_pon_info.pon_lock)) {
        spin_unlock(&ca_pon_info.pon_lock);
    }
    memset(&ca_pon_info, 0, sizeof(ca_pon_info));

    return 0;
}


static ca_status_t __pon_irq_setup(ca_uint8_t pon_mode)
{
    int ret = 0;
    int irq = 0;
    int i   = 0;
    struct device *dev = NULL;

    if(!ca_pon_info.pon_dev) {
        ca_printf("!!!error:%s(): nill pon info dev.\n", __func__);
        return CA_E_RESOURCE;
    }

    if (CA_PON_MODE_IS_ETHERNET(pon_mode)) { /* no PON interrupt used in ethernet mode now */
        return CA_E_OK;
    }

    dev = &(ca_pon_info.pon_dev->dev);

    /* loop all the irqs for PON dev */
    for (i = 0; i < ca_pon_info.irq_num; i++) {
        /* irq = platform_get_irq(ca_pon_info.pon_dev, i);
        if (irq <= 0) {
            break;
        }

        g_pon_irqs[i] = irq;
        */
        irq = g_pon_irqs[i];
        ret = request_irq(irq, __pon_isr, IRQF_SHARED | IRQF_TIMER, ca_pon_info.pon_dev->name, dev);
        if (ret != 0) {
            dev_err(dev, "!!!error:Failed to request irq(%d)\n", ret);
            return CA_E_RESOURCE;
        }

        ca_printf("ca-pon: IRQ %d setup successfully.\r\n", irq);
    }

    return CA_E_OK;
}

static ca_status_t __pon_irq_exit(void)
{
    int i = 0;

    if (!aal_init_pon) {
        return CA_E_OK;
    }

    for (i = 0; i < sizeof(g_pon_irqs)/sizeof(g_pon_irqs[0]); i++) {
        if (g_pon_irqs[i] > 0) {
            free_irq(g_pon_irqs[i], &(ca_pon_info.pon_dev->dev));
        }
    }

    return CA_E_OK;
}


/** ca_pon_init
 *@brief: init PON hardware
 */
ca_status_t ca_pon_init(ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t pon_mode = 0;
    ca_device_config_tlv_t scfg_tlv;
    ca_uint32_t board_info = 0;
    ca_uint32_t customer_info = 0;
    ca_uint8_t chip_addr = 1;

    if (aal_init_pon) {
        ca_printf("PON was already init!\n");
        return CA_E_OK;
    }

    pon_mode = aal_pon_mac_mode_get(device_id);
    ret = aal_pon_init(device_id, pon_mode);
    if(CA_E_OK != ret) {
        ca_printf("%s:%d aal_pon_init() returns error, ret = %d\n",__func__, __LINE__, ret);
        return ret;
    }

    ret = __pon_wq_init(pon_mode);
    if(CA_E_OK != ret) {
        ca_printf("%s:%d __pon_wq_init() returns error, ret = %d\n",__func__, __LINE__, ret);
        return ret;
    }

    ret = __pon_irq_setup(pon_mode);
    if (ret){
        ca_printf("__pon_irq_setup fail(%d)\r\n", ret);
        return ret;
    }

    /* GPON API ControlBlock init */
    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)){
        ret = ca_gpon_cb_init(device_id);
        if(CA_E_OK != ret) {
            ca_printf("%s:%d ca_gpon_cb_init() returns error, ret = %d\n",__func__, __LINE__, ret);
            return ret;
        }
    }

    aal_init_pon = 1;

    ca_printf("ca-pon: init OK.\n");

    return CA_E_OK;
}
EXPORT_SYMBOL(ca_pon_init);

ca_status_t ca_pon_exit(ca_device_id_t device_id)
{
    __pon_irq_exit();
    __pon_wq_exit();
    aal_init_pon = 0;

    return CA_E_OK;
}
EXPORT_SYMBOL(ca_pon_exit);

int ca77xx_pon_probe(struct platform_device *pdev)
{
    int ret = 0;

    ca_printf("Enter <%s, %d>\r\n", __FUNCTION__, __LINE__);
    memset(g_pon_irqs, 0, sizeof(g_pon_irqs));
    //turn off APB reset
    AAL_GLB_REG_WRITE(0x2, GLOBAL_PON_CNTL);
    

#ifdef CONFIG_PROC_FS
    ret = ca_pon_proc_init();
    if (ret) {
        ca_printf("%s: ca_pon_proc_init return %d\n", __func__, ret);
        return ret;
    }
#endif

    ret = aal_pon_scfg_load(AAL_DEFAUL_DEV_ID);
    if(CA_E_OK != ret) {
        ca_printf("%s:%d aal_pon_scfg_load() returns error, ret = %d\n",__func__, __LINE__, ret);
        return ret;
    }

    ret = __pon_dev_init(pdev);
    if (ret){
        ca_printf("%s: __pon_dev_init return %d\n", __func__, ret);
        return ret;
    }

    ca_printf("End <%s, %d>\r\n", __FUNCTION__, __LINE__);

    return ret;
}

int ca77xx_pon_remove(struct platform_device *pdev)
{

    ca_pon_exit(0);
    __pon_dev_exit(pdev);

#ifdef CONFIG_PROC_FS
    ca_pon_proc_exit();
#endif

    return 0;
}

ca_pon_tx_cb ca_pon_tx_handle = NULL;

ca_status_t ca_pon_tx_register(
    ca_uint32_t (*handler)(ca_device_id_t device_id, void *pkt))
{
    if (NULL == handler)
        return CA_E_PARAM;
    
    ca_pon_tx_handle = handler;

    //ca_printf("<%s, %d> register handler:%p\r\n", __func__, __LINE__, ca_pon_tx_handle);

    return CA_E_OK;
}


ca_status_t ca_pon_tx_unregister(void)
{
    ca_pon_tx_handle = NULL;

    //ca_printf("<%s, %d> unregister handler:%p\r\n", __func__, __LINE__, ca_pon_tx_handle);

    return CA_E_OK;
}

EXPORT_SYMBOL(ca_pon_tx_register);
EXPORT_SYMBOL(ca_pon_tx_unregister);

/***************************************************************************/
/*                               Module Info                               */
/***************************************************************************/
/*MODULE_DEVICE_TABLE(of, ca77xx_pon_of_match);

static struct platform_driver ca77xx_pon_driver = {
    .remove     = __exit_p(ca77xx_pon_remove),
    .probe      = ca77xx_pon_probe,
    .driver = {
        .name   = "ca77xx-pon",
        .owner  = THIS_MODULE,
        .of_match_table = ca77xx_pon_of_match,
    },
};
*/
/*
module_platform_driver(ca77xx_pon_driver);

MODULE_AUTHOR("Cortina SH");
MODULE_DESCRIPTION("Cortina Acess PON Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");*/

#endif /* CONFIG_NE_PON */


