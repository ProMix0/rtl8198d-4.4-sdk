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
/*
 * aal_init.c: AAL initialization
 */
#include "ca_types.h"
#include "aal_table.h"
#include "aal_ni.h"
#include "aal_l3_if.h"
#include "aal_l3_cam.h"
#include "aal_hashlite.h"
#include "aal_l3_pe.h"
#include "aal_l3qm.h"
#include "aal_l3_cls.h"
#include "aal_l3fe.h"
#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
#include "aal_hash.h"
#include "aal_lpm.h"
#endif
#include "aal_l3_te_cb.h"
#include "aal_mcast.h"
#include "aal_arb.h"
#include "aal_mir.h"
#include "aal_l2_tm.h"
#include "aal_l2_tm_cb.h"
#include "aal_l2_cls.h"
#include "aal_sa_id.h"
#include "aal_idx4pe.h"
#include "aal_ni_l2.h"
#include "aal_common.h"

static int aal_system_setup(void)
{
    int ret = CA_E_OK;

    if (SOC_HAS_L3FE()) {
        /* Setup CAM Table
         */
        if ((ret = aal_cam_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize CAM. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        /* Setup Hash Table (T2)
         *  - all used hash masks
         *  - all profiles including action group if hit, default action if miss
         */
        if ((ret = aal_hash_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash Engine. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        /* Setup LPM (T3)
         *  - default route action
         *  - memory range of each profile
         *  - default t4ctrl
         */
        if ((ret = aal_lpm_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize LPM. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }
#endif

        /* Setup HashLite (T4)
         *  - init HW internal memory
         *  - all used hash masks
         *  - all profiles including action group if hit, default action if miss
         *  - aging timer
         */
        if ((ret = aal_hashlite_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash-Lite Engine. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

#if !defined(CONFIG_LUNA_G3_SERIES)
        /*
         * Setup L3 special packets
         */
        if ((ret = aal_l3_spcl_pkt_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize L3 spcl pkt. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }
#endif
    }

    return ret;
}

static int aal_driver_init(ca_device_id_t device_id)
{
    int ret = 0;

    /* NI */
    if ((ret = aal_ni_init(device_id)) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize NI. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    /* L2 Multicast */
    if ((ret = aal_im3e_mldda_init()) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize IM3E MLD DA Table. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    if ((ret = aal_im3e_mldsa_init()) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize IM3E MLD SA Table. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    /* L3 Multicast */
    if ((ret = aal_mce_indx_init()) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize MCE INDX Table. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    if ((ret = aal_arb_fib_init()) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize ARB FIB Table. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    /* ARB */
    if ((ret = aal_arb_init(device_id)) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize L2 ARB. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    /* L2 Mirror Engine */
    if ((ret = aal_mire_init(device_id)) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize L2 Mirror Engine. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    /* L2 Traffic Manager */
    if ((ret = aal_l2_tm_init(device_id)) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize L2 TM. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }
    if ((ret = aal_l2_tm_cb_init(device_id)) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize L2 TM CB. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }
#if defined(CONFIG_ARCH_CORTINA_G3HGU)  
    if ((ret = aal_l2_tm_cb_init(1)) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize L2 TM CB. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }
#endif
#if 1
/*defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)*/
    /* L3 Traffic Engine */
    if ((ret = aal_l3_te_cb_init(device_id)) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize L3 TE CB. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }
#endif

#if 1
    /* L2 Classifier */
    if ((ret = aal_l2_cls_init(device_id)) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize L2 CLS. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }
#endif

    /* SA ID */
    if ((ret = __aal_sa_id_tbl_init()) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize SA ID Table. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    /* IDX4PE */
    if ((ret = aal_idx4pe_tbl_init()) != AAL_E_OK) {
        ca_printf("%s:%d: Fail to initialize IDX4PE Table. (ret %d)\n", __func__, __LINE__, ret);
        return ret;
    }

    if (SOC_HAS_L3FE()) {
        /* L3 Forwarding Engine */
        if ((ret = aal_l3fe_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize L3 FE. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        /* L3 PP CAM */
        if ((ret = aal_l3_cam_tbls_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize L3 PP CAM Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        /* L3 IF */
        if ((ret = aal_fe_l3e_if_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize L3 If Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        /* Hash */
        if ((ret = _hash_mask_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash Mask Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = _hash_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = _cache_hash_table_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Cache Hash Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = _cache_hash_action_table_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Cache Action Hash Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = _hash_age_table_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash Age Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = _hash_overflow_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash Overflow Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = _hash_action_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash Action Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = _hash_overflow_action_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash Overflow Action Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = _hash_default_action_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash Default Action Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }
#endif

        /* Hash-Lite */
        if ((ret = __hashlite_mask_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash-Lite Mask Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = __hashlite_hash_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash-Lite Hash Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = __hashlite_overflow_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash-Lite Overflow Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = __hashlite_hash_action_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash-Lite Hash Action Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = __hashlite_overflow_action_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash-Lite Overflow Action Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = __hashlite_default_action_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize Hash-Lite Default Action Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        /* LPM */
        if ((ret = __aal_lpm_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize LPM Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }
#endif

        /* L3 PE */
        if ((ret = aal_l3pe_sixrd_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize L3FE PE 6RD Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        if ((ret = aal_l3pe_dsl_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize L3FE PE DS-Lite Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }

        /* L3 Classifier */
        if ((ret = aal_l3_cls_tbl_init()) != AAL_E_OK) {
            ca_printf("%s:%d: Fail to initialize L3 CLS Table. (ret %d)\n", __func__, __LINE__, ret);
            return ret;
        }
    }

    return AAL_E_OK;
}

static void aal_driver_exit(void)
{
    if (SOC_HAS_L3FE()) {
        /* NI */
        aal_l3_cam_tbls_exit();

        /* L3 IF */
        aal_fe_l3e_if_tbl_exit();

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        /* Hash */
        _hash_mask_tbl_exit();
        _hash_tbl_exit();
        _cache_hash_table_exit();
        _cache_hash_action_table_exit();
        _hash_age_table_exit();
        _hash_overflow_tbl_exit();
        _hash_action_exit();
        _hash_overflow_action_exit();
        _hash_default_action_exit();
#endif

        /* Hash-Lite */
        __hashlite_mask_tbl_exit();
        __hashlite_hash_tbl_exit();
        __hashlite_overflow_tbl_exit();
        __hashlite_hash_action_exit();
        __hashlite_overflow_action_exit();
        __hashlite_default_action_exit();

#if defined(CONFIG_ARCH_CORTINA_G3) || defined(CONFIG_ARCH_CORTINA_G3LITE) || defined(CONFIG_ARCH_CORTINA_G3HGU)
        /* LPM */
        __aal_lpm_tbl_exit();
#endif

        /* L3FE PE */
        aal_l3pe_sixrd_tbl_exit();
        aal_l3pe_dsl_tbl_exit();
    }

    /* L2/3 Multicast */
    aal_mce_indx_exit();
    aal_arb_fib_exit();
    aal_im3e_mldda_exit();
    aal_im3e_mldsa_exit();

    /* SA ID */
    __aal_sa_id_tbl_exit();

    /* IDX4PE */
    aal_idx4pe_tbl_exit();
}

ca_status_t aal_init(ca_device_id_t device_id)
{
    int ret;

    /* Initialize all AAL configuration and tables */
    ret = aal_driver_init(device_id);
    if (ret != AAL_E_OK)
        goto end_aal_init;

    /* Setup system configuration through AAL functions; this must be behind AAL table initialization. */
    ret = aal_system_setup();
    if (ret != AAL_E_OK)
        goto end_aal_init;


end_aal_init:
    switch (ret) {
    case AAL_E_OK:
        return CA_E_OK;
    case AAL_E_TBLFULL:
        return CA_E_FULL;
    case AAL_E_ENTRY_USED:
        return CA_E_UNAVAIL;
    case AAL_E_OPNOTSUPP:
        return CA_E_NOT_SUPPORT;
    case AAL_E_NOMEM:
        return CA_E_MEMORY;
    case AAL_E_EXIST:
        return CA_E_EXISTS;
    default:
        return CA_E_INIT;
    }
}

void aal_exit(void)
{
    /* Uninitialize all AAL tables */
    aal_driver_exit();
}

