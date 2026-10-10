/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef CONFIG_NE_PON

#include "util_misc.h"
#include "osal_common.h"
#include <soc/cortina/registers.h>
#include <soc/cortina/cortina-soc.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/types.h>
#include <linux/of_gpio.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/gpio/consumer.h>
#include <linux/resource.h>

#include "ca-scfg/scfg.h"
#include "aal_pon.h"
#include "aal_pdc.h"
#include "aal_puc.h"
#include "aal_idp.h"
#include "aal_xcvr.h"
#include "aal_psds.h"

#include "aal_epon.h"
#include "aal_mpcp.h"

#include "aal_xcvr.h"
#include "aal_xgpon.h"
#include "aal_xgpon_intr.h"
#include "aal_xgpon_actp.h"
#include "aal_ploam.h"
#include "ploam_api.h"
#include "xgpon_aes.h"
#include "xgpon.h"
#include "peri.h"
#include "aal_gpon.h"

#ifdef CONFIG_LUNA_G3_SERIES
#include "pon.h"
#endif

xgpon_basic_info_t g_xgpn_basic_info;

ca_uint32_t g_pon_chip_version = 'C'; /* 0x43 */
ca_uint32_t aal_pon_mac_mode = ONU_PON_MAC_EPON_BI10G; /* should got from startup */

ca_uint     g_pon_spin_lock = 0;      /* spin lock for PON APIs */
#ifdef CONFIG_LUNA_G3_SERIES
EXPORT_SYMBOL(g_pon_spin_lock);
#endif

#define __AAL_MAX_GEM_NUM   128
#define __PON_MAX_FRAME_SIZE_DEFAULT  12288
#define __USBG_LASER_ON_ALIGN_DEFAULT 0x20

#define AAL_PUC_VOQ_MAP_MODE_DEFAULT 0

/***********************************************************************************************************************/

ca_uint32_t aal_pon_mac_mode_get(ca_device_id_t device_id)
{
    return aal_pon_mac_mode;
}
EXPORT_SYMBOL(aal_pon_mac_mode_get);

#if defined(CONFIG_ARCH_CORTINA_G3HGU)

extern void __iomem *ca_g3_hgu_ne_reg_iobase;
void aal_hgu_8279_ne_reg_base_init(void **p_iobase)
{
    *p_iobase = ca_g3_hgu_ne_reg_iobase;
}
EXPORT_SYMBOL(aal_hgu_8279_ne_reg_base_init);

void __pon_mac_intr_enable(void)
{
    volatile unsigned int  pon_mac_ie = 0;

    pon_mac_ie = AAL_GLB_REG_READ(SATURN_GLOBAL_TAROKO_INTENABLE);
    AAL_GLB_REG_WRITE((pon_mac_ie | TAROKO_PON_MAC_INTR_BIT),
        SATURN_GLOBAL_TAROKO_INTENABLE);
    //ca_printf("SATURN_GLOBAL_TAROKO_INTENABLE SET to 1\n");

    pon_mac_ie = AAL_GLB_REG_READ(SATURN_GLOBAL_TAROKO_INTENABLE);
    //ca_printf("SATURN_GLOBAL_TAROKO_INTENABLE SET to %d \n", pon_mac_ie);
}

#endif /* CONFIG_ARCH_CORTINA_G3HGU */

static ca_uint8_t  __pon_type_str2id(ca_uint8_t *type_str)
{
    if (0 == strncasecmp(type_str, "calix", 3)) {
        return CA_OLT_TYPE_CALIX;
    } else if (0 == strncasecmp(type_str, "adtran", 3)) {
        return CA_OLT_TYPE_ADTRAN;
    } else if (0 == strncasecmp(type_str, "zte", 3)) {
        return CA_OLT_TYPE_ZTE;
    } else if (0 == strncasecmp(type_str, "sumitomo", 3)) {
        return CA_OLT_TYPE_SUMITOMO;
    } else if (0 == strncasecmp(type_str, "cortina", 3)) {
        return CA_OLT_TYPE_CORTINA;
    } else {
        return CA_OLT_TYPE_UNKNOWN;
    }

    return CA_OLT_TYPE_UNKNOWN;
}

ca_status_t aal_pon_scfg_load(ca_device_id_t device_id)
{
    ca_uint32_t  ret = CA_E_OK;
    ca_uint32_t  i = 0;
	ca_uint32_t rgst_id = 0;
    ca_uint8_t temp[64] = {0};
    ca_uint8_t pon_mode;
    char *str_mac_mode[ONU_PON_MAC_MAX] =
    {
        "EPON",
        "EPON-D10G",
        "EPON-BI10G",
        "GPON",
        "XGPON1",
        "XGSPON",
        "NGPON2",
        "ETHERNET"
    };

    struct ca_soc_data  soc_data;
    memset(&soc_data, 0, sizeof(soc_data));
    (void)ca_soc_data_get(&soc_data);
    g_pon_chip_version = soc_data.pon_revision;

    ret = scfg_read(0, CFG_ID_PON_MAC_MODE, sizeof(pon_mode), (ca_uint8_t *) &pon_mode);
    if (CA_E_OK != ret) {
        ca_printf("<%s, %d> read CFG_ID_PON_MAC_MODE fail\r\n", __func__, __LINE__);
        ret = CA_E_ERROR;
    } else {
        aal_pon_mac_mode = (ca_uint32_t)pon_mode;
        ca_printf("ca-pon: load PON_MAC_MODE: %s\n",
            ((aal_pon_mac_mode < ONU_PON_MAC_MAX) ? str_mac_mode[aal_pon_mac_mode]
            : "invalid"));
    }

    ret = scfg_read(0, CFG_ID_PON_VOQ_MODE, sizeof(ca_uint8_t), (ca_uint8_t *)&(g_xgpn_basic_info.voq_mode));
    if (CA_E_OK != ret) {
        ca_printf("<%s, %d> read CFG_ID_PON_VOQ_MODE fail\r\n", __func__, __LINE__);
        g_xgpn_basic_info.voq_mode = AAL_PUC_VOQ_MAP_MODE_DEFAULT;
    } else {
       puc_voq_map_mode = g_xgpn_basic_info.voq_mode;
       ca_printf("ca-pon: load PON_VOQ_MODE: %d\n", g_xgpn_basic_info.voq_mode);
   }

    /* Config PON MAC mode into ASIC */

    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)){
        /* Init XGPON Global info */
        memset(&g_xgpn_basic_info, 0, sizeof(xgpon_basic_info_t));
        g_xgpn_basic_info.onu_id = AAL_XGPON_INVALID_ONU_ID;
        g_xgpn_basic_info.pon_mode = pon_mode;

        memset(temp, 0, sizeof(temp));
        ret = scfg_read(0, CFG_ID_PON_VENDOR_ID, CFG_ID_PON_VENDOR_ID_LEN, temp);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_VENDOR_ID fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.vendor_id = AAL_XGPON_VENDOR_ID_DEFAULT;
        } else {
            g_xgpn_basic_info.vendor_id = ((ca_uint32_t)temp[0] << 24) | ((ca_uint32_t)temp[1] << 16) | ((ca_uint32_t)temp[2] << 8) | ((ca_uint32_t)temp[3]);
            ca_printf("ca-pon: load VENDOR_ID: %s (HEX = 0x%x)\n",
                temp, g_xgpn_basic_info.vendor_id);
        }

        ret = scfg_read(0, CFG_ID_PON_VSSN, sizeof(ca_uint32_t), (ca_uint8_t *) &(g_xgpn_basic_info.vssn));
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_VSSN fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.vssn = AAL_XGPON_VSSN_DEFAULT;
        } else {
            ca_printf("ca-pon: load VSSN: 0x%x\n", g_xgpn_basic_info.vssn);
        }

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID0, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgst_id);
		memcpy(g_xgpn_basic_info.registration_id,&rgst_id,4);

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID1, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgst_id);
		memcpy(g_xgpn_basic_info.registration_id + 4,&rgst_id,4);

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID2, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgst_id);
		memcpy(g_xgpn_basic_info.registration_id + 8,&rgst_id,4);

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID3, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgst_id);
		memcpy(g_xgpn_basic_info.registration_id + 12,&rgst_id,4);

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID4, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgst_id);
		memcpy(g_xgpn_basic_info.registration_id + 16,&rgst_id,4);

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID5, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgst_id);
		memcpy(g_xgpn_basic_info.registration_id + 20,&rgst_id,4);

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID6, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgt_id);
		memcpy(g_xgpn_basic_info.registration_id + 24,&rgst_id,4);

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID7, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgst_id);
		memcpy(g_xgpn_basic_info.registration_id + 28,&rgst_id,4);

        ret = scfg_read(0, CFG_ID_PON_REGISTRATION_ID8, sizeof(ca_uint32_t), (ca_uint8_t *)&rgst_id);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_REGISTRATION_ID0 fail\r\n", __func__, __LINE__);
        }
		//rgst_id = ntohl(rgst_id);
		memcpy(g_xgpn_basic_info.registration_id + 32,&rgst_id,4);
		ret = scfg_read(0, CFG_ID_PON_BWMP_RSP_TM_CFG, sizeof(ca_uint32_t), (ca_uint8_t *) &(g_xgpn_basic_info.bwmp_rsp_tm));
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_VSSN fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.bwmp_rsp_tm = AAL_XGPON_BWMP_RSP_TM_DEFAULT;
            //ret = CA_E_ERROR;
        } else {
            ca_printf("ca-pon: load BWMP_RSP_TM: 0x%x\n", g_xgpn_basic_info.bwmp_rsp_tm);
        }

        ca_printf("ca-pon: load REGISTRATION_ID: (HEX = ");
        for (i = 0; i < XGPN_PLMP_RGST_ID_SIZE; i++){
            ca_printf("%02x", g_xgpn_basic_info.registration_id[i]);
        }
        ca_printf(")\n");

		memset(temp, 0, sizeof(temp));
        ret = scfg_read(0, CFG_ID_LOID, CFG_ID_LOID_LEN, temp);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_LOID fail\r\n", __func__, __LINE__);
        } else {
            memcpy(g_xgpn_basic_info.logical_onu_id, temp, CFG_ID_LOID_LEN);
            ca_printf("ca-pon: load LOID: %s (HEX = %#x)\n", temp, g_xgpn_basic_info.logical_onu_id);
        }

		memset(temp, 0, sizeof(temp));
        ret = scfg_read(0, CFG_ID_PASSWD, CFG_ID_PASSWD_LEN, temp);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PASSWD fail\r\n", __func__, __LINE__);
        } else {
            memcpy(g_xgpn_basic_info.password, temp, CFG_ID_PASSWD_LEN);
            ca_printf("ca-pon: load PASSWORD: %s (HEX = %#x)\n", temp, g_xgpn_basic_info.password);
        }

		memset(temp, 0, sizeof(temp));
        ret = scfg_read(0, CFG_ID_GPON_VERSION, CFG_ID_GPON_VERSION_LEN, temp);
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_GPON_VERSION fail\r\n", __func__, __LINE__);
        } else {
            memcpy(g_xgpn_basic_info.version, temp, CFG_ID_GPON_VERSION_LEN);
            ca_printf("ca-pon: load VERSION: %s (HEX = %#x)\n", temp, g_xgpn_basic_info.version);
        }

        if (pon_mode == ONU_PON_MAC_NGPON2) {
            ret = scfg_read(0, CFG_ID_PON_NGPON2_DEFAULT_CHAN, sizeof(ca_uint8_t), (ca_uint8_t *)&(g_xgpn_basic_info.default_channel));
            if (CA_E_OK != ret) {
                ca_printf("<%s, %d> read CFG_ID_PON_NGPON2_DEFAULT_CHAN fail\r\n", __func__, __LINE__);
                g_xgpn_basic_info.default_channel = AAL_XGPON_DEF_CHANNEL_DEFAULT;
            } else {
                ca_printf("ca-pon: load DEFAULT_CHAN: 0x%x\n", g_xgpn_basic_info.default_channel);
            }

            ret = scfg_read(0, CFG_ID_PON_NGPON2_PROTECTION_CHAN, sizeof(ca_uint8_t), (ca_uint8_t *)&(g_xgpn_basic_info.protect_channel));
            if (CA_E_OK != ret) {
                ca_printf("<%s, %d> read CFG_ID_PON_NGPON2_PROTECTION_CHAN fail\r\n", __func__, __LINE__);
                g_xgpn_basic_info.protect_channel = AAL_XGPON_PROTECT_CHANNEL_DEFAULT;
            } else {
                ca_printf("ca-pon: load PROTECTION_CHAN: 0x%x\n", g_xgpn_basic_info.protect_channel);
            }

            ret = scfg_read(0, CFG_ID_PON_NGPON2_CHAN_PARTITION_INDEX, sizeof(ca_uint8_t), (ca_uint8_t *)&(g_xgpn_basic_info.cpi));
            if (CA_E_OK != ret) {
                ca_printf("<%s, %d> read CFG_ID_PON_NGPON2_CHAN_PARTITION_INDEX fail\r\n", __func__, __LINE__);
                g_xgpn_basic_info.cpi = AAL_XGPON_CPI_DEFAULT;
            } else {
                ca_printf("ca-pon: load CHAN_PARTITION_INDEX: 0x%x\n", g_xgpn_basic_info.cpi);
            }
        }
        ret = scfg_read(0, CFG_ID_XGPN_ACTP_TOZ, sizeof(ca_uint32_t), (ca_uint8_t *)&(g_xgpn_basic_info.toz));
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_XGPN_ACTP_TOZ fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.toz = AAL_XGPON_TOZ_DEFAULT;
        } else {
            ca_printf("ca-pon: load ACTP_TOZ: 0x%x\n", g_xgpn_basic_info.toz);
        }

        ret = scfg_read(0, CFG_ID_XGPN_ACTP_TO23, sizeof(ca_uint32_t), (ca_uint8_t *)&(g_xgpn_basic_info.to23));
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_XGPN_ACTP_TO23 fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.to23 = AAL_XGPON_TO23_DEFAULT;
        } else {
            ca_printf("ca-pon: load ACTP_TO23: 0x%x\n", g_xgpn_basic_info.to23);
        }

        ret = scfg_read(0, CFG_ID_XGPN_ACTP_TO4, sizeof(ca_uint32_t), (ca_uint8_t *)&(g_xgpn_basic_info.to4));
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_XGPN_ACTP_TO4 fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.to4 = AAL_XGPON_TO4_DEFAULT;
        } else {
            ca_printf("ca-pon: load ACTP_TO4: 0x%x\n", g_xgpn_basic_info.to4);
        }

        ret = scfg_read(0, CFG_ID_XGPN_ACTP_TO5, sizeof(ca_uint32_t), (ca_uint8_t *)&(g_xgpn_basic_info.to5));
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_XGPN_ACTP_TO5 fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.to5 = AAL_XGPON_TO5_DEFAULT;
        } else {
            ca_printf("ca-pon: load ACTP_TO5: 0x%x\n", g_xgpn_basic_info.to5);
        }

        ret = scfg_read(0, CFG_ID_BATTERY_BACKUP, sizeof(ca_uint32_t), (ca_uint8_t *)&(g_xgpn_basic_info.battery_up));
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_BATTERY_BACKUP fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.battery_up = AAL_XGPON_BATTERY_UP_DEFAULT;
        } else {
            ca_printf("ca-pon: load BATTERY_UP: 0x%x\n", g_xgpn_basic_info.battery_up);
        }

        if (CA_E_OK != scfg_read(device_id, CFG_ID_PON_LASER_ALIGN, sizeof(ca_uint8_t), &g_xgpn_basic_info.laser_on_align)) {
            g_xgpn_basic_info.laser_on_align = __USBG_LASER_ON_ALIGN_DEFAULT;
        } else {
            ca_printf("ca-pon: load CFG_ID_PON_LASER_ALIGN: 0x%02x.\n", g_xgpn_basic_info.laser_on_align);
        }

        memset(temp, 0, sizeof(temp));
        ret = scfg_read(0, CFG_ID_PON_OLT_TYPE, CFG_ID_PON_OLT_TYPE_LEN,  temp);
        //ret = scfg_read(0, CFG_ID_PON_OLT_TYPE, sizeof(ca_uint8_t), (ca_uint8_t *)&(g_xgpn_basic_info.olt_type));
        if (CA_E_OK != ret) {
            ca_printf("<%s, %d> read CFG_ID_PON_OLT_TYPE fail\r\n", __func__, __LINE__);
            g_xgpn_basic_info.olt_type = AAL_XGPON_OLT_TYPE_DEFAULT;
        } else {
            g_xgpn_basic_info.olt_type = __pon_type_str2id(temp);
            ca_printf("ca-pon: load OLT_TYPE: %s (HEX = %#x)\n", temp, g_xgpn_basic_info.olt_type);
        }

    }

    return ret;
}


static void 
__eth_link_monitor_handler(
    void *data)
{
    static ca_boolean_t  old_link_up = 0;
    ca_boolean_t         link_up = 0;
    ca_device_id_t       dev_id = 0;
    ca_status_t ret = CA_E_ERROR;
    aal_psds_status_t  sds_status;
    aal_psds_gbox_ctrl_t  psds_gbox_ctrl;

    memset ( &sds_status, 0, sizeof(sds_status));
    memset ( &psds_gbox_ctrl, 0, sizeof( aal_psds_gbox_ctrl_t ) );

    aal_psds_eth_link_status_get(dev_id, &link_up);
    if (link_up != old_link_up) 
{
        ca_printf("INFO: Uplink Ethernet port 7 Link %s\r\n", link_up ? "UP" : "DOWN");
        old_link_up = link_up;
}

    if ( likely(link_up) )
{
        return;
}
    //
    // RX_LOS = 1, there's no module.
    //
    aal_psds_status_get (dev_id, &sds_status);
    if ( unlikely(sds_status.rx_los ))
{
        return;
}
    //
    // if rx optic but not linkup and tx/rx of prbs isn't enabled, then reset serdes 
    //
    aal_psds_burst_enable(dev_id, 0);
    ret = aal_psds_reset(dev_id, ONU_PON_MAC_ETHERNET);
    if ( ret != CA_E_OK)
{
        ca_printf ( "%s: Failed to reset psds, ret 0x%x\n", __FUNCTION__, ret );
    }
    aal_psds_burst_enable (dev_id, 1 );

}

static ca_status_t __pon_eth_init(ca_device_id_t device_id, ca_uint32_t mode)
{
    ca_printf("%s(): init start\r\n", __func__);
#if defined(CONFIG_ARCH_CORTINA_G3HGU)
    device_id =  1;
#endif
    aal_psds_eth_reset(device_id);
    aal_puc_init(device_id, mode);
    aal_pdc_init(device_id, mode);
    AAL_GLB_REG_WRITE (0, GLOBAL_OPT_INTENABLE);
    AAL_PON_REG_WRITE (0, EPON_EPON_GLB_INTERRUPTE);
    AAL_PON_REG_WRITE (0, EPON_XGEPN_XGPCS_RX_RXINTE0);
    aal_psds_burst_enable(device_id, 1);
    /* check link status per second */
    ca_circle_timer_add(1000, __eth_link_monitor_handler, NULL);
    ca_printf("%s(): init done\r\n", __func__);
    return CA_E_OK;
}


static ca_status_t __epon_init(ca_device_id_t device_id, ca_uint32_t mode)
{
    ca_status_t ret = CA_E_OK;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)  || defined(CONFIG_ARCH_CORTINA_G3HGU)
    /* SERDES initialization */
    aal_psds_init(device_id, mode);
#endif

    ret = ca_spin_lock_init(&g_pon_spin_lock, SPINLOCK_BH);
    if (ret != 0)
    {
        ca_printf("EPON: failed to create spin lock g_pon_spin_lock\n");
        return CA_E_ERROR;
    }

    /* EPON MAC, MPCP, Encryption, PUC, PDC init */
    ret = aal_epon_init(device_id, 0, mode);

     return ret;
}

/* following API will be called by ca-network-engine */
ca_status_t aal_pon_mac_enable_set(
    CA_IN ca_device_id_t    device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_boolean_t         tx_en,
    CA_IN ca_boolean_t         rx_en)
{
    if (CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode)) {
        aal_epon_mac_ctrl_msk_t epon_msk;
        aal_epon_mac_ctrl_t epon_cfg;
        epon_msk.u32 = 0;

        epon_msk.s.rx_xg_ena = 1;
        epon_msk.s.tx_xg_ena = 1;
        epon_msk.s.rx_1g_ena = 1;
        epon_msk.s.tx_1g_ena = 1;

        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        if(aal_pon_mac_mode == ONU_PON_MAC_EPON_BI10G){
            epon_cfg.rx_xg_ena = rx_en;
            epon_cfg.tx_xg_ena = tx_en;
            epon_cfg.rx_1g_ena = 0;
            epon_cfg.tx_1g_ena = 0;
        }else if(aal_pon_mac_mode == ONU_PON_MAC_EPON_D10G){
            epon_cfg.rx_xg_ena = rx_en;
            epon_cfg.tx_xg_ena = 0;
            epon_cfg.rx_1g_ena = 0;
            epon_cfg.tx_1g_ena = tx_en;
        }else if(aal_pon_mac_mode == ONU_PON_MAC_EPON_1G){
            epon_cfg.rx_xg_ena = 0;
            epon_cfg.tx_xg_ena = 0;
            epon_cfg.rx_1g_ena = rx_en;
            epon_cfg.tx_1g_ena = tx_en;
        }

        return aal_epon_mac_cfg_set(device_id, port_id, epon_msk, &epon_cfg);
    }
    else if (CA_PON_MODE_IS_GPON_FAMILY(aal_pon_mac_mode)) {
        ca_status_t  ret = CA_E_OK;
        aal_xgpon_xgpn_glb_ctrl_config_msk_t  mac_msk;
        aal_xgpon_xgpn_glb_ctrl_config_t      mac_ctrl;


        mac_msk.u32 = 0;
        mac_msk.s.mac_en = 1;
        memset(&mac_ctrl, 0, sizeof(mac_ctrl));

        if (!rx_en)
            mac_ctrl.mac_en = 0;
        else
            mac_ctrl.mac_en = aal_pon_mac_mode - ONU_PON_MAC_GPON;

        (void)aal_psds_burst_enable(device_id, tx_en);
        if (ONU_PON_MAC_GPON != aal_pon_mac_mode)
            ret = aal_xgpon_xgpn_glb_ctrl_set(device_id, mac_msk, &mac_ctrl);
        else
            ret = aal_gpon_active_set(device_id, TRUE);
        if (CA_E_OK != ret)
            return ret;

        return ret;
    }

    return CA_E_OK;
}

ca_status_t aal_pon_mac_enable_get(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT ca_boolean_t       *tx_en,
    CA_OUT ca_boolean_t       *rx_en)
{
    CA_NULL_PTR_CHECK(tx_en);
    CA_NULL_PTR_CHECK(rx_en);

    if (CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode)) {
        aal_epon_mac_ctrl_t epon_cfg;
        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        aal_epon_mac_cfg_get(device_id, port_id, &epon_cfg);
        if(aal_pon_mac_mode == ONU_PON_MAC_EPON_BI10G){
            *rx_en = epon_cfg.rx_xg_ena;
            *tx_en = epon_cfg.tx_xg_ena;

        }else if(aal_pon_mac_mode == ONU_PON_MAC_EPON_D10G){
            *rx_en = epon_cfg.rx_xg_ena;
            *tx_en = epon_cfg.tx_1g_ena;

        }else if(aal_pon_mac_mode == ONU_PON_MAC_EPON_1G){
            *rx_en = epon_cfg.rx_1g_ena;
            *tx_en = epon_cfg.tx_1g_ena;
        }
    }
    else if (CA_PON_MODE_IS_GPON_FAMILY(aal_pon_mac_mode)) {
        ca_status_t  ret = CA_E_OK;
        aal_xgpon_xgpn_glb_ctrl_config_t      mac_ctrl;


        memset(&mac_ctrl, 0, sizeof(mac_ctrl));

        ret = aal_xgpon_xgpn_glb_ctrl_get(device_id, &mac_ctrl);
        if (CA_E_OK != ret)
            return ret;

        *rx_en = mac_ctrl.mac_en ? 1 : 0;
        *tx_en = *rx_en;
        return ret;
    }

    return CA_E_OK;
}


ca_status_t aal_pon_mac_max_frame_size_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_uint32_t        tx_size,
    CA_IN ca_uint32_t        rx_size,
    CA_IN ca_boolean_t       mtu_tag_en)
{
    if (CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode) || CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode)) {
        aal_epon_mac_ctrl_msk_t epon_msk;
        aal_epon_mac_ctrl_t epon_cfg;

        epon_msk.u32 = 0;
        epon_msk.s.rx_xg_psize_max = 1;
        epon_msk.s.tx_1g_psize_max_ena = 1;
        epon_msk.s.rx_1g_psize_max = 1;
        epon_msk.s.tx_1g_psize_max = 1;
        epon_msk.s.max_mtu_tag_en = 1;

        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        epon_cfg.rx_xg_psize_max = rx_size;
        //epon_cfg.tx_xg_psize_max = tx_size;
        epon_cfg.tx_1g_psize_max_ena = 1;
        epon_cfg.rx_1g_psize_max = rx_size;
        epon_cfg.tx_1g_psize_max = tx_size;
        epon_cfg.max_mtu_tag_en = mtu_tag_en;

        return aal_epon_mac_cfg_set(device_id, port_id, epon_msk, &epon_cfg);
    }
    else if (CA_PON_MODE_IS_GPON_FAMILY(aal_pon_mac_mode)) {
        aal_xgpon_dsfp_rxmac_ctrl_config_msk_t  rxmac_ctrl_msk;
        aal_xgpon_dsfp_rxmac_ctrl_config_t  rxmac_ctrl_cfg;

        rxmac_ctrl_msk.u32 = 0;
        memset(&rxmac_ctrl_cfg, 0, sizeof(rxmac_ctrl_cfg));

        rxmac_ctrl_msk.s.max_pkt_size = 1;
        rxmac_ctrl_cfg.max_pkt_size = rx_size;
        return aal_xgpon_dsfp_rxmac_ctrl_set(device_id, rxmac_ctrl_msk, &rxmac_ctrl_cfg);
    }

    return CA_E_OK;
}

ca_status_t aal_pon_mac_max_frame_size_get(
    CA_IN ca_device_id_t      device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT ca_uint32_t       *tx_size,
    CA_OUT ca_uint32_t       *rx_size,
    CA_OUT ca_uint32_t       *mtu_tag_en)
{
    ca_status_t ret = CA_E_OK;

    CA_NULL_PTR_CHECK(tx_size);
    CA_NULL_PTR_CHECK(rx_size);

    if (CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode) || CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode)) {
        aal_epon_mac_ctrl_t epon_cfg;
        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        ret = aal_epon_mac_cfg_get(device_id, port_id, &epon_cfg);
        if (CA_E_OK != ret)
            return ret;

        *rx_size = epon_cfg.rx_xg_psize_max;
        *tx_size = epon_cfg.rx_xg_psize_max;
        *mtu_tag_en = epon_cfg.max_mtu_tag_en;
        if(aal_pon_mac_mode == ONU_PON_MAC_EPON_BI10G){
            *rx_size = epon_cfg.rx_xg_psize_max;
            *tx_size = epon_cfg.rx_xg_psize_max;

        }else if(aal_pon_mac_mode == ONU_PON_MAC_EPON_D10G){
            *rx_size = epon_cfg.rx_xg_psize_max;
            *tx_size = epon_cfg.tx_1g_psize_max;

        }else if(aal_pon_mac_mode == ONU_PON_MAC_EPON_1G){
            *rx_size = epon_cfg.rx_1g_psize_max;
            *tx_size = epon_cfg.tx_1g_psize_max;
        }
    }
    else if (CA_PON_MODE_IS_GPON_FAMILY(aal_pon_mac_mode)) {
        aal_xgpon_dsfp_rxmac_ctrl_config_t  rxmac_ctrl_cfg;

        memset(&rxmac_ctrl_cfg, 0, sizeof(rxmac_ctrl_cfg));

        ret = aal_xgpon_dsfp_rxmac_ctrl_get(device_id, &rxmac_ctrl_cfg);
        if (CA_E_OK != ret)
            return ret;

        *rx_size = rxmac_ctrl_cfg.max_pkt_size;
    }

    return CA_E_OK;
}

ca_status_t aal_pon_mac_ipg_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_uint32_t         ipg)
{
    if (CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode) || CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode)) {
        aal_epon_mac_ctrl_msk_t epon_msk;
        aal_epon_mac_ctrl_t epon_cfg;

        epon_msk.u32 = 0;
        epon_msk.s.tx_xg_ipg = 1;
        epon_msk.s.tx_1g_ipg = 1;

        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        epon_cfg.tx_xg_ipg = ipg;
        epon_cfg.tx_1g_ipg = ipg;

        return aal_epon_mac_cfg_set(device_id, port_id, epon_msk, &epon_cfg);
    }

    return CA_E_OK;
}

ca_status_t aal_pon_mac_ipg_get(
    CA_IN ca_device_id_t      device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT ca_uint32_t       *ipg)
{
    CA_NULL_PTR_CHECK(ipg);

    if (CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode) || CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode)) {
        aal_epon_mac_ctrl_t epon_cfg;
        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        aal_epon_mac_cfg_get(device_id, port_id, &epon_cfg);
        if(aal_pon_mac_mode == ONU_PON_MAC_EPON_BI10G){
            *ipg = epon_cfg.tx_xg_ipg;

        }else if(aal_pon_mac_mode == ONU_PON_MAC_EPON_D10G){
            *ipg = epon_cfg.tx_1g_ipg;

        }else if(aal_pon_mac_mode == ONU_PON_MAC_EPON_1G){
            *ipg = epon_cfg.tx_1g_ipg;
        }
    }

    return CA_E_OK;
}

ca_status_t aal_pon_mac_loopback_set(
    CA_IN ca_device_id_t     device_id,
    CA_IN ca_port_id_t        port_id,
    CA_IN ca_boolean_t        rx2tx,
    CA_IN ca_boolean_t        tx2rx)
{
    if (CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode) || CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode)) {

        aal_epon_mac_ctrl_msk_t epon_msk;
        aal_epon_mac_ctrl_t epon_cfg;

        epon_msk.u32 = 0;
        epon_msk.s.rx2tx_lookback = 1;
        epon_msk.s.tx2rx_lookback = 1;

        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        epon_cfg.rx2tx_lookback = rx2tx;
        epon_cfg.tx2rx_lookback = tx2rx;

        return aal_epon_mac_cfg_set(device_id, port_id, epon_msk, &epon_cfg);
    }

    return CA_E_OK;
}

ca_status_t aal_pon_mac_loopback_get(
    CA_IN ca_device_id_t      device_id,
    CA_IN ca_port_id_t        port_id,
    CA_OUT ca_boolean_t      *rx2tx,
    CA_OUT ca_boolean_t      *tx2rx)
{
    CA_NULL_PTR_CHECK(rx2tx);
    CA_NULL_PTR_CHECK(tx2rx);

    if (CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode) || CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode)) {
        aal_epon_mac_ctrl_t epon_cfg;
        memset(&epon_cfg, 0, sizeof(aal_epon_mac_ctrl_t));
        aal_epon_mac_cfg_get(device_id, port_id, &epon_cfg);
        *rx2tx = epon_cfg.rx2tx_lookback;
        *tx2rx = epon_cfg.tx2rx_lookback;
    }

    return CA_E_OK;
}

#if 0
ca_status_t __pon_crypto_mode_to_1g_crypto_mode(
    aal_pon_crypto_mode_t pon_crypto,
    aal_epon_crypto_1g_mode_t *crypto_mode,
    aal_epon_crypto_key_exchange_mode_t *key_exchange)
{
    switch(pon_crypto){
        case AAL_PON_CRYPTO_MODE_DISABLE:    /* 0 */
            *crypto_mode = AAL_EPON_CRYPTO_1G_MODE_DISABLE;     /* 3 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_HW;
            break;

        case AAL_PON_CRYPTO_MODE_AES_CFB:    /* 2 */
            *crypto_mode = AAL_EPON_CRYPTO_1G_MODE_AES_DPOE;  /* 1 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case AAL_PON_CRYPTO_MODE_SW_CTC_CHURNING:    /* 4 */
            *crypto_mode = AAL_EPON_CRYPTO_1G_MODE_CHURNING;  /* 2 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case AAL_PON_CRYPTO_MODE_HW_CTC_CHURNING:    /* 5 */
            *crypto_mode = AAL_EPON_CRYPTO_1G_MODE_CHURNING;  /* 2 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_HW;
            break;

        default:
            ca_printf("not support this mode %d\r\n", pon_crypto);
            return CA_E_NOT_FOUND;
    }
    return CA_E_OK;
}

ca_status_t __1g_crypto_mode_to_pon_crypto_mode(
    aal_epon_crypto_1g_mode_t crypto_mode,
    aal_epon_crypto_key_exchange_mode_t key_exchange,
    aal_pon_crypto_mode_t *pon_crypto)
{
    switch(crypto_mode){
        case AAL_EPON_CRYPTO_1G_MODE_AES_DPOE:
            *pon_crypto = AAL_PON_CRYPTO_MODE_AES_CFB;
            break;

        case AAL_EPON_CRYPTO_1G_MODE_CHURNING:
            if(key_exchange == AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW){
                *pon_crypto = AAL_PON_CRYPTO_MODE_SW_CTC_CHURNING;
            }else{
                *pon_crypto = AAL_PON_CRYPTO_MODE_HW_CTC_CHURNING;
            }
            break;

        case AAL_EPON_CRYPTO_1G_MODE_DISABLE:
            *pon_crypto = AAL_PON_CRYPTO_MODE_DISABLE;
            break;

        default:
            ca_printf("not support this mode %d\r\n", crypto_mode);
            return CA_E_PARAM;

    }
    return CA_E_OK;
}

ca_status_t __pon_crypto_mode_to_xg_crypto_mode(
    aal_pon_crypto_mode_t pon_crypto,
    aal_epon_crypto_xg_mode_t *crypto_mode,
    aal_epon_crypto_key_exchange_mode_t *key_exchange)
{
    switch(pon_crypto){
        case AAL_PON_CRYPTO_MODE_DISABLE:    /* 0 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_DISABLE;     /* 3 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_HW;
            break;

        case AAL_PON_CRYPTO_MODE_AES_CTR:    /* 1 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_AES_DPOE;  /* 1 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case AAL_PON_CRYPTO_MODE_MACSEC:    /* 3 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_MACSEC;     /* 0 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case AAL_PON_CRYPTO_MODE_SW_CTC_CHURNING:    /* 4 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_CHURNING;  /* 2 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW;
            break;

        case AAL_PON_CRYPTO_MODE_HW_CTC_CHURNING:    /* 5 */
            *crypto_mode = AAL_EPON_CRYPTO_XG_MODE_CHURNING;  /* 2 */
            *key_exchange = AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_HW;
            break;

        default:
            ca_printf("not support this mode %d\r\n", pon_crypto);
            return CA_E_NOT_FOUND;
    }
    return CA_E_OK;
}

ca_status_t __xg_crypto_mode_to_pon_crypto_mode(
    aal_epon_crypto_xg_mode_t crypto_mode,
    aal_epon_crypto_key_exchange_mode_t key_exchange,
    aal_pon_crypto_mode_t *pon_crypto)
{
    switch(crypto_mode){
        case AAL_EPON_CRYPTO_XG_MODE_MACSEC:
            *pon_crypto = AAL_PON_CRYPTO_MODE_MACSEC;
            break;

        case AAL_EPON_CRYPTO_XG_MODE_AES_DPOE:
            *pon_crypto = AAL_PON_CRYPTO_MODE_AES_CTR;
            break;

        case AAL_EPON_CRYPTO_XG_MODE_CHURNING:
            if(key_exchange == AAL_EPON_CRYPTO_KEY_EXCHANGE_MODE_SW){
                *pon_crypto = AAL_PON_CRYPTO_MODE_SW_CTC_CHURNING;
            }else{
                *pon_crypto = AAL_PON_CRYPTO_MODE_HW_CTC_CHURNING;
            }
            break;

        case AAL_EPON_CRYPTO_XG_MODE_DISABLE:
            *pon_crypto = AAL_PON_CRYPTO_MODE_DISABLE;
            break;

        default:
            ca_printf("not support this mode %d\r\n", crypto_mode);
            return CA_E_PARAM;

    }
    return CA_E_OK;
}

ca_status_t aal_pon_crypto_mode_set(
    CA_IN ca_device_id_t                 device_id,
    CA_IN ca_port_id_t                    port_id,
    CA_IN aal_pon_crypto_mode_t            tx_mode,
    CA_IN aal_pon_crypto_mode_t            rx_mode)
{
    ca_status_t ret = CA_E_OK;
    aal_epon_crypto_cfg_msk_t msk;
    aal_epon_crypto_cfg_t      cfg;
    aal_epon_crypto_1g_mode_t rx_1g_mode, tx_1g_mode;
    aal_epon_crypto_xg_mode_t rx_xg_mode, tx_xg_mode;
    ca_uint32_t key_exchange = 0;
    ca_uint32_t pon_mode;
    ca_boolean_t enable;
    aal_epon_crypto_llid_t llid;

    CA_DEV_ID_CHECK(device_id);

    pon_mode = aal_pon_mac_mode_get(device_id);

    memset(&msk, 0, sizeof(aal_epon_crypto_cfg_msk_t));
    memset(&cfg, 0, sizeof(aal_epon_crypto_cfg_t));

    ca_printf("start to set EPON port encryption: pon_mode %d, tx_mode %d, rx_mode %d\r\n", pon_mode, tx_mode, rx_mode);
    /* set encryption mode, if encryption is enabled, mode should be the same */
    if(pon_mode == ONU_PON_MAC_EPON_1G){

        /* 1. set encryption mode */
        ret = __pon_crypto_mode_to_1g_crypto_mode(tx_mode, &tx_1g_mode, &key_exchange);
        CA_RET_VALUE_CHECK(ret);
        ret = __pon_crypto_mode_to_1g_crypto_mode(rx_mode, &rx_1g_mode, &key_exchange);
        CA_RET_VALUE_CHECK(ret);

        msk.s.rx_mode = 1;
        msk.s.tx_mode = 1;
        cfg.rx_mode = rx_1g_mode;
        cfg.tx_mode = tx_1g_mode;
        msk.s.key_exchange_mode = 1;
        cfg.key_exchange_mode = key_exchange;
        ca_printf("set aal 1G tx_mode %d, rx_mode %d, key_exchange_mode %d\r\n", 
            tx_1g_mode, rx_1g_mode, key_exchange);
        ret = aal_epon_crypto_1g_cfg_set(device_id, port_id, msk, &cfg);
        CA_RET_VALUE_CHECK(ret);
            
        /* 2. enable/disable encryption, llid should be online when enable encryption */      
        enable = (rx_mode == AAL_PON_CRYPTO_MODE_DISABLE)?0:1;
        ret = aal_epon_crypto_1g_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, enable);
        CA_RET_VALUE_CHECK(ret);
        //enable = (tx_mode == AAL_PON_CRYPTO_MODE_DISABLE)?0:1;  
        //ret = aal_epon_crypto_1g_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_TX, enable);

    }else if(pon_mode == ONU_PON_MAC_EPON_D10G || pon_mode == ONU_PON_MAC_EPON_BI10G){

        ret = __pon_crypto_mode_to_xg_crypto_mode(tx_mode, &tx_xg_mode, &key_exchange);
        CA_RET_VALUE_CHECK(ret);
        ret = __pon_crypto_mode_to_xg_crypto_mode(rx_mode, &rx_xg_mode, &key_exchange);
        CA_RET_VALUE_CHECK(ret);

        msk.s.rx_mode = 1;
        msk.s.tx_mode = 1;
        cfg.rx_mode = rx_xg_mode;
        cfg.tx_mode = tx_xg_mode;
        msk.s.key_exchange_mode = 1;
        cfg.key_exchange_mode = key_exchange;

        ca_printf("set aal XG tx_mode %d, rx_mode %d, key_exchange_mode %d\r\n", 
            tx_xg_mode, rx_xg_mode, key_exchange);

        ret = aal_epon_crypto_xg_cfg_set(device_id, port_id, msk, &cfg);
        CA_RET_VALUE_CHECK(ret);

		enable = (rx_mode == AAL_PON_CRYPTO_MODE_DISABLE)?0:1;
        ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_RX, enable);
        CA_RET_VALUE_CHECK(ret);
		enable = (tx_mode == AAL_PON_CRYPTO_MODE_DISABLE)?0:1;
        ret = aal_epon_crypto_xg_enable_set(device_id, port_id, AAL_EPON_CRYPTO_DIRECTION_TX, enable);
        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}

ca_status_t aal_pon_crypto_mode_get(
    CA_IN ca_device_id_t             device_id,
    CA_IN ca_port_id_t                port_id,
    CA_OUT aal_pon_crypto_mode_t    *tx_mode,
    CA_OUT aal_pon_crypto_mode_t    *rx_mode)
{
    ca_status_t ret = CA_E_OK;
    aal_epon_crypto_cfg_t cfg;
    ca_uint32_t pon_mode;

    CA_DEV_ID_CHECK(device_id);
    //CA_PORT_EPON_CHECK(port_id);
    CA_NULL_PTR_CHECK(tx_mode);
    CA_NULL_PTR_CHECK(rx_mode);

    pon_mode = aal_pon_mac_mode_get(device_id);

    if(pon_mode == ONU_PON_MAC_EPON_1G){
        memset(&cfg, 0, sizeof(aal_epon_crypto_cfg_t));
        ret = aal_epon_crypto_1g_cfg_get(device_id, port_id, &cfg);
        CA_RET_VALUE_CHECK(ret);

        //ca_printf("get aal tx mode is %d, aal rx mode is %d, key_exchange_mode is %d\r\n", cfg.tx_mode, cfg.rx_mode, cfg.key_exchange_mode);
        ret = __1g_crypto_mode_to_pon_crypto_mode(cfg.tx_mode, cfg.key_exchange_mode, tx_mode);
        CA_RET_VALUE_CHECK(ret);
        ret = __1g_crypto_mode_to_pon_crypto_mode(cfg.rx_mode, cfg.key_exchange_mode, rx_mode);

    }else if(pon_mode == ONU_PON_MAC_EPON_D10G || pon_mode == ONU_PON_MAC_EPON_BI10G){
        memset(&cfg, 0, sizeof(aal_epon_crypto_cfg_t));
        ret = aal_epon_crypto_xg_cfg_get(device_id, port_id, &cfg);
        CA_RET_VALUE_CHECK(ret);

        //ca_printf("get aal tx mode is %d, aal rx mode is %d, key_exchange_mode is %d\r\n", cfg.tx_mode, cfg.rx_mode, cfg.key_exchange_mode);
        ret = __xg_crypto_mode_to_pon_crypto_mode(cfg.tx_mode, cfg.key_exchange_mode, tx_mode);
        CA_RET_VALUE_CHECK(ret);
        ret = __xg_crypto_mode_to_pon_crypto_mode(cfg.rx_mode, cfg.key_exchange_mode, rx_mode);
        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}
/* end. API called by ca-network-engine */

#endif

ca_status_t aal_xgpon_xgpn_glb_super_frame_match_val_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_xgpn_glb_super_frame_match_val_t val)
{
    XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL0_t sfc_val0;
    XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL1_t sfc_val1;

    memset((ca_uint8_t *)&sfc_val0,0,sizeof(XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL0_t));
    memset((ca_uint8_t *)&sfc_val1,0,sizeof(XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL1_t));

    //AAL_XGPON_LOCK();

    sfc_val0.wrd = AAL_PON_REG_READ(XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL0);
    sfc_val1.wrd = AAL_PON_REG_READ(XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL1);

    sfc_val0.bf.sfc0 = val.sfc0;
    sfc_val1.bf.sfc1 = val.sfc1&0x7FFFF;

    AAL_PON_REG_WRITE(sfc_val0.wrd,XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL0);
    AAL_PON_REG_WRITE(sfc_val1.wrd,XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL1);

    //AAL_XGPON_UNLOCK();
    return CA_E_OK;
}

ca_status_t aal_xgpon_xgpn_glb_super_frame_match_val_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_xgpn_glb_super_frame_match_val_t *val)
{
    XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL0_t sfc_val0;
    XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL1_t sfc_val1;

    //CA_ASSERT_RET(val,CA_E_ERROR);
    memset((ca_uint8_t *)&sfc_val0,0,sizeof(XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL0_t));
    memset((ca_uint8_t *)&sfc_val1,0,sizeof(XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL1_t));

    //AAL_XGPON_LOCK();

    sfc_val0.wrd = AAL_PON_REG_READ(XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL0);
    sfc_val1.wrd = AAL_PON_REG_READ(XGPN_XGPN_GLB_SUPER_FRAME_MATCH_VAL1);
    val->sfc0 = sfc_val0.wrd;
    val->sfc1 = sfc_val1.wrd;

    //AAL_XGPON_UNLOCK();
    return CA_E_OK;
}

EXPORT_SYMBOL(aal_xgpon_xgpn_glb_super_frame_match_val_set);
EXPORT_SYMBOL(aal_xgpon_xgpn_glb_super_frame_match_val_get);

ca_status_t aal_epon_onu_mpcp_timer_match_cfg_set(
    CA_IN ca_device_id_t                  device_id,
    CA_IN ca_port_id_t                    port_id,
    CA_IN ca_uint32_t                     mpcp_timer_match_cfg)
{
    EPON_ONU_LT_MPCP_TIMER_MATCH_CFG_t cfg;
    memset((void*)&cfg,0,sizeof(EPON_ONU_LT_MPCP_TIMER_MATCH_CFG_t));

    cfg.wrd = AAL_PON_REG_READ(EPON_ONU_LT_MPCP_TIMER_MATCH_CFG);
    cfg.bf.epon_mpcp_timer_match_val = mpcp_timer_match_cfg;
    AAL_PON_REG_WRITE(cfg.wrd,EPON_ONU_LT_MPCP_TIMER_MATCH_CFG);

    return CA_E_OK;
}

ca_status_t aal_epon_onu_mpcp_timer_match_cfg_get(
    CA_IN ca_device_id_t                  device_id,
    CA_IN ca_port_id_t                    port_id,
    CA_IN ca_uint32_t                     *mpcp_timer_match_cfg)
{
    EPON_ONU_LT_MPCP_TIMER_MATCH_CFG_t cfg;
    memset((void*)&cfg,0,sizeof(EPON_ONU_LT_MPCP_TIMER_MATCH_CFG_t));

    //CA_ASSERT_RET(mpcp_timer_match_cfg,CA_E_PARAM);
    cfg.wrd = AAL_PON_REG_READ(EPON_ONU_LT_MPCP_TIMER_MATCH_CFG);
    *mpcp_timer_match_cfg = cfg.bf.epon_mpcp_timer_match_val;

    return CA_E_OK;
}

ca_status_t aal_epon_onu_mpcp_timestamp_get(
    CA_IN ca_device_id_t                  device_id,
    CA_IN ca_port_id_t                    port_id,
    CA_OUT ca_uint32_t                     *timestamp)
{
    EPON_XGONU_MPCP_TIMESTAMP_t mpcp_ts;

    memset((void*)&mpcp_ts,0,sizeof(EPON_XGONU_MPCP_TIMESTAMP_t));

    mpcp_ts.wrd = AAL_PON_REG_READ(EPON_XGONU_MPCP_TIMESTAMP);
    *timestamp = mpcp_ts.bf.value;

    ca_printf("mpcp timestamp 0x%x\r\n", *timestamp);
    return CA_E_OK;
}

EXPORT_SYMBOL(aal_epon_onu_mpcp_timer_match_cfg_set);
EXPORT_SYMBOL(aal_epon_onu_mpcp_timer_match_cfg_get);
EXPORT_SYMBOL(aal_epon_onu_mpcp_timestamp_get);

ca_status_t aal_per_tod_mem_cfg_set(
    ca_device_id_t device_id,
    aal_per_tod_mem_cfg_mask_t mask,
    aal_per_tod_mem_cfg_t cfg)
{
   PER_TOD_CFG_t per_tod_cfg;
   ca_uint32_t i = 0;

   memset((void *)&per_tod_cfg,0,sizeof(PER_TOD_CFG_t));

   per_tod_cfg.wrd = AAL_PERI_REG_READ(PER_TOD_CFG);

   if(mask.per_tod_cfg_mask.s.tod_counter){
        per_tod_cfg.bf.tod_counter = cfg.tod_counter;
   }

   if(mask.per_tod_cfg_mask.s.tod_en){
        per_tod_cfg.bf.tod_en = cfg.tod_en;
   }

   if(mask.per_tod_cfg_mask.s.tod_uart){
        per_tod_cfg.bf.tod_uart = cfg.tod_uart;
   }

   if(mask.per_tod_mem_mask.s.tod_bst_mem){
     for(i=0;i<PER_TOD_MEM_COUNT;i++){
        AAL_PERI_REG_WRITE(cfg.tod_bst_mem[i],(PER_TOD_MEM + PER_TOD_MEM_STRIDE*i));
     }
   }

   AAL_PERI_REG_WRITE(per_tod_cfg.wrd,PER_TOD_CFG);

   return CA_E_OK;
}
EXPORT_SYMBOL(aal_per_tod_mem_cfg_set);

ca_status_t aal_per_tod_mem_cfg_get(
    ca_device_id_t device_id,
    aal_per_tod_mem_cfg_t *cfg)
{
   PER_TOD_CFG_t per_tod_cfg;
   PER_TOD_MEM_t per_tod_mem;
   ca_uint32_t i = 0;

   memset((void *)&per_tod_cfg,0,sizeof(PER_TOD_CFG_t));
   memset((void *)&per_tod_mem,0,sizeof(PER_TOD_MEM_t));

   per_tod_cfg.wrd = AAL_PERI_REG_READ(PER_TOD_CFG);

   cfg->tod_en = per_tod_cfg.bf.tod_en;
   cfg->tod_uart = per_tod_cfg.bf.tod_uart;
   cfg->tod_counter = per_tod_cfg.bf.tod_counter;

   for(i=0;i<PER_TOD_MEM_COUNT;i++){
        per_tod_mem.wrd = AAL_PERI_REG_READ(PER_TOD_MEM+i*PER_TOD_MEM_STRIDE);
        cfg->tod_bst_mem[i] = per_tod_mem.bf.tod_bst_mem;
   }

   return CA_E_OK;
}
EXPORT_SYMBOL(aal_per_tod_mem_cfg_get);

ca_status_t aal_epon_onu_1pps_tod_cfg_set(
     ca_device_id_t device_id,
     aal_epon_onu_1pps_tod_cfg_mask_t mask,
     aal_epon_onu_1pps_tod_cfg_t config)
{
    EPON_ONU_1PPS_CFG_t onu_1pps_cfg;
    EPON_ONU_1PPS_TOD_S_PPS_t onu_1pps_tod_s_pps;
    EPON_ONU_1PPS_TOD_PPS_WID_t onu_1pps_tod_pps_wid;
    EPON_ONU_1PPS_TOD_S_TOD_t onu_1pps_tod_s_tod;

    memset((void *)&onu_1pps_cfg,0,sizeof(EPON_ONU_1PPS_CFG_t));
    memset((void *)&onu_1pps_tod_s_pps,0,sizeof(EPON_ONU_1PPS_TOD_S_PPS_t));
    memset((void *)&onu_1pps_tod_pps_wid,0,sizeof(EPON_ONU_1PPS_TOD_PPS_WID_t));
    memset((void *)&onu_1pps_tod_s_tod,0,sizeof(EPON_ONU_1PPS_TOD_S_TOD_t));

    onu_1pps_cfg.wrd = AAL_PON_REG_READ(EPON_ONU_1PPS_CFG);
    onu_1pps_tod_s_pps.wrd = AAL_PON_REG_READ(EPON_ONU_1PPS_TOD_S_PPS);
    onu_1pps_tod_pps_wid.wrd = AAL_PON_REG_READ(EPON_ONU_1PPS_TOD_PPS_WID);
    onu_1pps_tod_s_tod.wrd = AAL_PON_REG_READ(EPON_ONU_1PPS_TOD_S_TOD);

    if(mask.onu_1pps_mask.s.gen_1pps_tod_en){
        onu_1pps_cfg.bf.gen_1pps_tod_en = config.gen_1pps_tod_en;
    }

    if(mask.onu_1pps_tod_s_pps_mask.s.s_pps){
       onu_1pps_tod_s_pps.bf.s_pps = config.s_pps;
    }

    if(mask.onu_1pps_tod_pps_wid_mask.s.pps_width){
       onu_1pps_tod_pps_wid.bf.pps_width = config.pps_width;
    }

    if(mask.onu_1pps_tod_s_tod_mask.s.s_tod){
       onu_1pps_tod_s_tod.bf.s_tod = config.s_tod;
    }

    AAL_PON_REG_WRITE(onu_1pps_cfg.wrd,EPON_ONU_1PPS_CFG);
    AAL_PON_REG_WRITE(onu_1pps_tod_s_pps.wrd,EPON_ONU_1PPS_TOD_S_PPS);
    AAL_PON_REG_WRITE(onu_1pps_tod_pps_wid.wrd,EPON_ONU_1PPS_TOD_PPS_WID);
    AAL_PON_REG_WRITE(onu_1pps_tod_s_tod.wrd,EPON_ONU_1PPS_TOD_S_TOD);

    return CA_E_OK;
}
EXPORT_SYMBOL(aal_epon_onu_1pps_tod_cfg_set);

ca_status_t aal_epon_onu_1pps_tod_cfg_get(
     ca_device_id_t device_id,
     aal_epon_onu_1pps_tod_cfg_t *config)
{
    EPON_ONU_1PPS_CFG_t onu_1pps_cfg;
    EPON_ONU_1PPS_TOD_S_PPS_t onu_1pps_tod_s_pps;
    EPON_ONU_1PPS_TOD_PPS_WID_t onu_1pps_tod_pps_wid;
    EPON_ONU_1PPS_TOD_S_TOD_t onu_1pps_tod_s_tod;

    memset((void *)&onu_1pps_cfg,0,sizeof(EPON_ONU_1PPS_CFG_t));
    memset((void *)&onu_1pps_tod_s_pps,0,sizeof(EPON_ONU_1PPS_TOD_S_PPS_t));
    memset((void *)&onu_1pps_tod_pps_wid,0,sizeof(EPON_ONU_1PPS_TOD_PPS_WID_t));
    memset((void *)&onu_1pps_tod_s_tod,0,sizeof(EPON_ONU_1PPS_TOD_S_TOD_t));

    onu_1pps_cfg.wrd = AAL_PON_REG_READ(EPON_ONU_1PPS_CFG);
    onu_1pps_tod_s_pps.wrd = AAL_PON_REG_READ(EPON_ONU_1PPS_TOD_S_PPS);
    onu_1pps_tod_pps_wid.wrd = AAL_PON_REG_READ(EPON_ONU_1PPS_TOD_PPS_WID);
    onu_1pps_tod_s_tod.wrd = AAL_PON_REG_READ(EPON_ONU_1PPS_TOD_S_TOD);

    config->gen_1pps_tod_en = onu_1pps_cfg.bf.gen_1pps_tod_en;
    config->pps_width = onu_1pps_tod_pps_wid.bf.pps_width;
    config->s_pps = onu_1pps_tod_s_pps.bf.s_pps;
    config->s_tod = onu_1pps_tod_s_tod.bf.s_tod;

    return CA_E_OK;
}
EXPORT_SYMBOL(aal_epon_onu_1pps_tod_cfg_get);

ca_status_t aal_xgpn_dsfp_sfc_get(
    ca_device_id_t device_id,
    ca_uint32_t *sfc0,
    ca_uint32_t *sfc1)
{
    return aal_xgpon_dsfp_super_frame_get(device_id,sfc0,sfc1);
}
EXPORT_SYMBOL(aal_xgpn_dsfp_sfc_get);

ca_status_t aal_pon_pdc_map_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t gem_port_id,
    CA_OUT aal_pon_pdc_map_mem_data_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PDC_PDC_MAP_MEM_DATA0_t  reg_data0;
    PDC_PDC_MAP_MEM_DATA1_t  reg_data1;

    if((cfg == NULL) || (gem_port_id >= __AAL_MAX_GEM_NUM)){
        return CA_E_PARAM;
    }

    reg_data0.wrd = 0;
    reg_data1.wrd = 0;

    AAL_PON_REG_INDIRECT_READ(gem_port_id, PDC_PDC_MAP_MEM_ACCESS);

    reg_data0.wrd = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA0);
    reg_data1.wrd = AAL_PON_REG_READ (PDC_PDC_MAP_MEM_DATA1);

    cfg->cos = reg_data0.bf.cos;
    cfg->ldpid = reg_data0.bf.ldpid;
    cfg->lspid = reg_data0.bf.lspid;
    cfg->fe_bypass = reg_data0.bf.fe_bypass;
    cfg->hdr_type = reg_data0.bf.hdr_type;
    cfg->mcgid = reg_data0.bf.mcgid;
    cfg->drop_code = reg_data0.bf.drop_code;
    cfg->rx_pkt_type = reg_data0.bf.rx_pkt_type;
    cfg->no_drop = reg_data0.bf.no_drop;

    cfg->mirror = reg_data1.bf.mirror;
    cfg->mark = reg_data1.bf.mark;
    cfg->pol_en = reg_data1.bf.pol_en;
    cfg->pol_id = reg_data1.bf.pol_id;
    cfg->pol_grp_id = reg_data1.bf.pol_grp_id;
    cfg->deepq = reg_data1.bf.deepq;
    cfg->cpu_flag = reg_data1.bf.cpu_flag;

    return ret;
}

EXPORT_SYMBOL(aal_pon_pdc_map_mem_get);

static ca_boolean_t __pon_xcvr_is_plugin(ca_device_id_t device_id)
{
    GLOBAL_OPT_MODULE_STATUS_t  opt_status;

    opt_status.wrd = AAL_GLB_REG_READ(GLOBAL_OPT_MODULE_STATUS);
    return (!opt_status.bf.opt_mod_abs);
}

ca_status_t aal_pon_init(ca_device_id_t device_id, ca_uint32_t mode)
{
    ca_status_t ret = CA_E_OK;

    if (CA_PON_MODE_IS_ETHERNET(mode)) {
        ret = __pon_eth_init(device_id, mode);
        if (CA_E_OK != ret){
            ca_printf("__pon_eth_init failed(%d)\n", ret);
            return ret;
        }
    }
    else if (CA_PON_MODE_IS_EPON_FAMILY(mode)) {
        if (__pon_xcvr_is_plugin(device_id)) {
            (void)aal_xcvr_init(device_id);
        }

        ret = __epon_init(device_id, mode);
        if (CA_E_OK != ret){
            ca_printf("__epon_init failed(%d)\n", ret);
            return ret;
        }
    }
    else if (CA_PON_MODE_IS_GPON_FAMILY(mode)){
        //No matter gpon or xgpon both need to init this
        if (__pon_xcvr_is_plugin(device_id)) {
            (void)aal_xcvr_init(device_id);
        }

        ret = ca_spin_lock_init(&g_pon_spin_lock, SPINLOCK_BH);
        if (ret != 0)
        {
            ca_printf("GPON: failed to create spin lock g_pon_spin_lock\n");
            return CA_E_ERROR;
        }

        if(mode != ONU_PON_MAC_GPON){
            ret = aal_xgpon_init(device_id,mode);
            if (CA_E_OK != ret){
                ca_printf("aal_xgpon_init failed(%d)\n", ret);
                return ret;
            }
        }else{
            ret = aal_gpon_init(device_id);
            if (CA_E_OK != ret){
                ca_printf("aal_gpon_init failed(%d)\n", ret);
                return ret;
            }
        }
    }
    ret = aal_pon_mac_max_frame_size_set(device_id, 0, __PON_MAX_FRAME_SIZE_DEFAULT, __PON_MAX_FRAME_SIZE_DEFAULT, 1);

#ifdef CONFIG_ARCH_CORTINA_G3HGU
    /*HGU Enable PON TOP interrupt */
    __pon_mac_intr_enable();
#endif

#if defined(CONFIG_LUNA_G3_SERIES)&&defined(CONFIG_BOARD_REALTEK_CA8277_PON_GPIO)
    ca_pon_laser_tx_disable_set(0,0);
#endif

    return CA_E_OK;

}

ca_status_t aal_pon_puc_flush(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;

    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL,puc_reset,0);
    ca_mdelay(10);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL,puc_reset,1);
    ca_mdelay(10);
    ret = aal_puc_init(device_id,aal_pon_mac_mode);

    return ret;
}

EXPORT_SYMBOL(g_pon_chip_version);
EXPORT_SYMBOL(aal_pon_puc_flush);
EXPORT_SYMBOL(aal_pon_mac_enable_set);
EXPORT_SYMBOL(aal_pon_mac_enable_get);
EXPORT_SYMBOL(aal_pon_mac_max_frame_size_set);
EXPORT_SYMBOL(aal_pon_mac_max_frame_size_get);
EXPORT_SYMBOL(aal_pon_mac_ipg_set);
EXPORT_SYMBOL(aal_pon_mac_ipg_get);
EXPORT_SYMBOL(aal_pon_mac_loopback_set);
EXPORT_SYMBOL(aal_pon_mac_loopback_get);
EXPORT_SYMBOL(aal_pon_init);
#endif /* CONFIG_NE_PON */


