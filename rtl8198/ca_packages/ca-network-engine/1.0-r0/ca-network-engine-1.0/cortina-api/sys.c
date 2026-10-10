#include "sys.h"
#include "osal_common.h"
#include "ca_aal.h"
#include "aal_l3qm.h"
#include "aal_common.h"
#include "ca-scfg/scfg.h"
#include "port.h"
#if defined(CONFIG_NE_AAL_XFI)
#include "aal_xfi.h"
#endif
#if defined(CONFIG_NE_PON)
#include "pon.h"
#include "rate.h"
#endif
#include "pkt_tx.h"
#include "peri.h"
#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
#include "aal_l3fe.h"
#endif

#if defined(CONFIG_NE_L3COS)
#include "ca_l3cos.h"
#endif

#include "eth_port.h"

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#include "macsec.h"
#else
#include "tunnel.h"
#endif

#if defined(CONFIG_NE_TPUT_TEST)
#include "tput.h"
#endif

#if defined(CONFIG_NE_PON)
extern ca_status_t ca_pon_init(ca_device_id_t device_id);
#endif
extern ca_status_t  ca_dp_init(/*CODEGEN_IGNORE_TAG*/CA_IN ca_device_id_t device_id);

extern ca_status_t ca_eth_ports_restore(ca_device_id_t device_id);
extern ca_status_t ca_eth_port_init(ca_device_id_t device_id, ca_init_config_t * p_cfg);

extern ca_status_t ca_port_init(ca_device_id_t device_id);
extern ca_status_t ca_special_packet_init(ca_device_id_t device_id, ca_init_config_t * p_cfg);
extern ca_status_t ca_qos_init(ca_device_id_t device_id, ca_init_config_t * p_cfg);
extern ca_status_t ca_l2_fdb_init(ca_device_id_t device_id);
extern ca_status_t ca_mirror_init(ca_device_id_t device_id);
extern ca_status_t ca_l2_vlan_init(ca_device_id_t device_id);
extern void ca_l2_classifier_init(void);

extern ca_status_t ca_l3_intf_init(ca_device_id_t device_id);
extern ca_status_t ca_l3_route_init(ca_device_id_t device_id);
extern ca_status_t ca_tunnel_init(ca_device_id_t device_id);
extern ca_status_t ca_nat_init(ca_device_id_t device_id);
extern ca_status_t ca_mcast_init(ca_device_id_t device_id);
extern ca_status_t ca_hash_flow_init(ca_device_id_t device_id);
extern void ca_ni_init_reserved_mem(void);
extern int ca_ni_init_l3qm(void);
extern void ca_ni_init_fbm(void);
extern ca_status_t ca_tx_rx_init(void);


#ifdef CONFIG_NE_SW_SHAPER
ca_status_t ca_sw_shaper_init(void);
#endif

#ifdef CONFIG_NE_US_RATE_MANAGER
ca_status_t ca_us_rate_manager_init(void);
#endif

#if defined(CONFIG_NE_WELL_KNOWN_MULTICAST)
ca_status_t well_known_mcast_flooding_init(void);
#endif

ca_status_t ca_init (
    CA_IN    ca_device_id_t        device_id,
    CA_IN    ca_init_config_t    *init_config,
    CA_OUT    ca_init_err_t        *err_code
)
{
    ca_status_t ret = CA_E_OK;
    ca_g3_init_config_t *p_g3_cfg = (ca_g3_init_config_t *) init_config;
    ca_init_err_t err = CA_INIT_SRAM_FAIL;

	//ca_macsec_global_config_t macsec_glb_cfg;
    /* load statup config and initialize reserved memory for L3QM */
    ca_ni_init_reserved_mem();

    /* should be for all of L2/L3 init */
    if ((ret = aal_init(device_id)) != CA_E_OK) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "aal_init()", ret);
        err = CA_INIT_CSR_ACCESS_FAIL;
        goto end_ca_init;
    }

    aal_ni_init_ni(p_g3_cfg->ca_ne_port_id);
#ifdef CONFIG_ARCH_REALTEK_TAURUS
            //77C not use CA API
#else
    /* init routine for kernel mode ca_tx and ca_rx */
    ca_tx_rx_init();
#endif

    //aal_ni_init_interrupt_cfg();

    /* L3QM should be initialzed after port initialized */
    //ca_ni_init_l3qm();
    //ca_ni_init_fbm();

#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
	cap_phy_init(device_id, CA_PHY_TYPE_0, cap_mdio_read, cap_mdio_write);
	cap_phy_init(device_id, CA_PHY_TYPE_1, cap_mdio_read, cap_mdio_write);
#endif

#ifdef CONFIG_ARCH_REALTEK_TAURUS
            //77C not use CA API
#else
    ret = ca_eth_port_init(device_id, init_config);
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_eth_port_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }

    ret = ca_port_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_port_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }

    ret = ca_l2_fdb_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_l2_fdb_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }

    ret = ca_qos_init(device_id, init_config);
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_qos_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }

    ret = ca_l2_vlan_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_l2_vlan_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }

    // TODO: Should check if L3FE is enabled; L3FE also contains special packet
    ret = ca_special_packet_init(device_id, init_config);
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_special_packet_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }

    ca_l2_classifier_init();

    ret = ca_mirror_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_mirror_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }
#endif
    ca_ni_init_l3qm();
    ca_ni_init_fbm();

#if !defined(CONFIG_ARCH_REALTEK_TAURUS)

    if (SOC_HAS_L3FE()) {
#ifndef CONFIG_ARCH_CORTINA_SATURN_SFU
        if ((ret = ca_tunnel_init(device_id)) != CA_E_OK) {
            ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_tunnel_init()", ret);
            err = CA_INIT_SRAM_FAIL;
            goto end_ca_init;
        }

        ret = ca_l3_intf_init(device_id);
        if (CA_E_OK != ret) {
            ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_l3_intf_init()", ret);
            err = CA_INIT_SRAM_FAIL;
            goto end_ca_init;
        }

        ret = ca_l3_route_init(device_id);
        if (CA_E_OK != ret) {
            ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_l3_route_init()", ret);
            err = CA_INIT_SRAM_FAIL;
            goto end_ca_init;
        }

        ret = ca_nat_init(device_id);
        if (CA_E_OK != ret) {
            ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_nat_init()", ret);
            err = CA_INIT_SRAM_FAIL;
            goto end_ca_init;
        }

#endif
        ret = ca_mcast_init(device_id);
        if (CA_E_OK != ret) {
            ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_mcast_init()", ret);
            err = CA_INIT_SRAM_FAIL;
            goto end_ca_init;
        }

        ret = ca_hash_flow_init(device_id);
        if (CA_E_OK != ret) {
            ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_hash_flow_init()", ret);
            err = CA_INIT_SRAM_FAIL;
            goto end_ca_init;
        }
    }
#endif


#if defined(CONFIG_NE_PON)
    ret = ca_dp_init(device_id);
    if(CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_dp_init()", ret);
        err = CA_INIT_PON_INIT_FAILED;
        goto end_ca_init;
    }

    /* Saturn PON init */
    ret = ca_pon_init(device_id);
    if(CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_pon_init()", ret);
        err = CA_INIT_PON_INIT_FAILED;
        goto end_ca_init;
    }

	//ca_pon_policer_callback = ca_flow_policer_set;
	//ca_printf("ca_pon_policer_callback init success!\r\n");

    /* register packet tx handler */
    //ca_printf("<%s, %d>__ca_tx_dying_gasp:%p\r\n", __FUNCTION__, __LINE__, __ca_tx_dying_gasp);

    ret = ca_pon_tx_register((void *)__ca_tx_dying_gasp);
    if(CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_pon_tx_register()", ret);
        err = CA_INIT_PON_INIT_FAILED;
        goto end_ca_init;
    }
#endif

#ifdef CONFIG_NE_SW_SHAPER
    ret = ca_sw_shaper_init();
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_sw_shaper_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }
#endif
#ifdef CONFIG_NE_US_RATE_MANAGER
    ret = ca_us_rate_manager_init();
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_us_rate_manager_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }
#endif

#if defined(CONFIG_NE_L3COS)
    ret = ca_l3cos_init();
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_l3cos_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }
#endif

#if defined(CONFIG_NE_WELL_KNOWN_MULTICAST)
    ret = well_known_mcast_flooding_init();
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "well_known_mcast_flooding_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }
#endif

#if defined(CONFIG_NE_TPUT_TEST)
    ret = ca_tput_init(device_id);
    if (CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_tput_init()", ret);
        err = CA_INIT_SRAM_FAIL;
        goto end_ca_init;
    }
#endif

#ifdef CONFIG_ARCH_REALTEK_TAURUS
            //77C not use CA API
#else
    /* do not enable ethernet ports let NI driver enable it */
    ret = ca_eth_ports_restore(device_id);
    if(CA_E_OK != ret) {
        ca_printf("%s:%d %s returns error, ret = %d\n",__func__, __LINE__, "ca_eth_ports_restore()", ret);
        err = CA_INIT_CSR_ACCESS_FAIL;
        goto end_ca_init;
    }
#endif
    /* init routine for kernel mode ca_tx and ca_rx */
    //ca_tx_rx_init();
    aal_ni_init_interrupt_cfg();

end_ca_init:
    if (err_code != NULL) *err_code = err;
    return ret;
}

ca_status_t ca_reset (
    CA_IN    ca_device_id_t        device_id,
    CA_IN    ca_sys_reset_mode_t    mode
)
{
    /*ONLY Support watchdog reset*/
    cap_watchdog_config_t wdt_cfg;
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFU
    GLOBAL_BLOCK_RESET_t gb_reset;
    gb_reset.wrd = AAL_PLAT_GLB_REG_READ(GLOBAL_BLOCK_RESET);
    gb_reset.bf.reset_rcpu1 = 1;
    AAL_PLAT_GLB_REG_WRITE(gb_reset.wrd, GLOBAL_BLOCK_RESET);
#endif
    wdt_cfg.enable = 1;
    wdt_cfg.threshold = 1;
    wdt_cfg.action = CAP_WATCHDOG_ACTION_REBOOT;
    cap_watchdog_set(device_id, &wdt_cfg);
    return CA_E_OK;
}

ca_status_t ca_info_get (
    CA_IN    ca_device_id_t        device_id,
    CA_OUT    ca_info_t        *info
)
{
    int ret;
    ca_uint8_t hw_model_name[CA_HW_ID_LEN];
    ca_uint8_t hw_revision[CA_HW_VERSION_LEN];
    ca_uint8_t hw_date[CA_HW_DATE_LEN];
    ca_uint8_t api_version[CA_API_VERSION_LEN];

    if (info == NULL)
        return CA_E_PARAM;

    memset(hw_model_name, 0, sizeof(hw_model_name));
    memset(hw_revision, 0, sizeof(hw_revision));
    memset(hw_date, 0, sizeof(hw_date));
    memset(api_version, 0, sizeof(api_version));

    ret = aal_scfg_read(CFG_ID_MODEL_NAME, sizeof(hw_model_name), hw_model_name);
    if (ret != CA_E_OK)
        printk(KERN_ERR "%s: Fail to read %s. (ret=%d)\n", __func__, "CFG_ID_MODEL_NAME", ret);

    ret = aal_scfg_read(CFG_ID_HW_VERSION, sizeof(hw_revision), hw_revision);
    if (ret != CA_E_OK)
        printk(KERN_ERR "%s: Fail to read %s. (ret=%d)\n", __func__, "CFG_ID_HW_VERSION", ret);

    ret = aal_scfg_read(CFG_ID_HW_YEAR_WEEK, sizeof(hw_date), hw_date);
    if (ret != CA_E_OK)
        printk(KERN_ERR "%s: Fail to read %s. (ret=%d)\n", __func__, "CFG_ID_HW_YEAR_WEEK", ret);

    ret = aal_scfg_read(CFG_ID_API_VERSION, sizeof(api_version), api_version);
    if (ret != CA_E_OK)
        printk(KERN_ERR "%s: Fail to read %s. (ret=%d)\n", __func__, "CFG_ID_API_VERSION", ret);

    /* adjust & copy to parameter 'info' for returning */

    memcpy(info->hardware_model_name, hw_model_name, CA_HW_ID_LEN);
    memcpy(info->hardware_revision, hw_revision, CA_HW_VERSION_LEN);
    memcpy(info->hardware_date, hw_date, CA_HW_DATE_LEN);
    memcpy(info->api_version, api_version, CA_API_VERSION_LEN);

    return CA_E_OK;
}

EXPORT_SYMBOL(ca_init);
EXPORT_SYMBOL(ca_reset);
EXPORT_SYMBOL(ca_info_get);
