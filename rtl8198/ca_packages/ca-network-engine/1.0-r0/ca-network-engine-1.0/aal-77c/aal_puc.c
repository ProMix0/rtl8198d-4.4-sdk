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

#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_pon.h"
#include "aal_xgpon.h"
#include "aal_puc.h"
#include "aal_pon.h"
#include "util_misc.h"
#include "scfg.h"
#include "aal_mpcp.h"
#include "epon.h"

#define AAL_PUC_DEBUG(fmt,...)      \
                        do { \
                            if (g_aal_puc_debug) \
                            ca_printf("%s():"fmt, __FUNCTION__,##__VA_ARGS__);\
                        }while(0)


#define __VOQ_FLUSH_TIMEOUT 1000 /* ms */
#define __TCONT_FOR_OMCC(tcont)  ((tcont)==0)
extern ca_uint32_t aal_pon_mac_mode;
ca_uint32_t tcont_num_per_chip = 32;
ca_uint32_t queue_num_per_tcont = 8;
ca_uint32_t llid_num_per_chip = 32;
ca_uint32_t queue_num_per_llid = 8;

ca_uint32_t puc_voq_map_mode = AAL_PUC_VOQ_MODE_8Q;

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS)
#define __PUC_EPON_BUF_DEFAULT_LIMIT_A 0x6000
#else
#define __PUC_EPON_BUF_DEFAULT_LIMIT_A 0x5000
#endif

#define __PUC_EPON_BUF_DEFAULT_LIMIT_B 0x800
#define __PUC_EPON_BUF_DEFAULT_LIMIT_C 0x400
#define __PUC_EPON_AVA_BUF_RPT_FFFF_THRD 0x2400

ca_int32_t g_aal_puc_debug = 0; // TODO: temp debug, should set to 0 for release
ca_uint g_puc_lock = 0;

#define __PUC_LOCK()    \
    do{ \
        if (g_puc_lock) __puc_lock();\
    }while(0)

#define __PUC_UNLOCK()    \
    do{ \
        if (g_puc_lock) __puc_unlock();\
    }while(0)


void __puc_lock(void)
{
    ca_spin_lock(g_puc_lock);
}

void __puc_unlock(void)
{
    ca_spin_unlock(g_puc_lock);
}



ca_status_t aal_puc_debug_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t value1,
    CA_IN ca_int32_t value2
)
{
    switch (value1){

        case 1:
            g_aal_puc_debug = value2;
            ca_printf("puc debug %d\r\n", g_aal_puc_debug);
            break;

        case 2:
            if (aal_pon_mac_mode_get(device_id) <= ONU_PON_MAC_EPON_BI10G)
                aal_puc_voq_flush_by_idx(device_id, value2, 0, queue_num_per_llid);
            else
                aal_puc_voq_flush_by_idx(device_id, value2, 0, queue_num_per_tcont);
            break;

        case 3:
            aal_puc_init(device_id, aal_pon_mac_mode_get(device_id));
            break;

        default:
            ca_printf("usage:\r\n");
            ca_printf("value1 1: Set value2 to g_aal_puc_debug\r\n ");
            ca_printf("value1 2: Set value2 to flush VoQ per TCONT/LLID\r\n ");
            ca_printf("value1 3: Re-init PUC\r\n ");
            break;
    }

    return CA_E_OK;
}

ca_status_t aal_puc_debug_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_int32_t  value1,
    CA_OUT ca_int32_t *value2
)
{

    if (value2 != NULL)
        *value2 = g_aal_puc_debug;

    ca_printf("puc debug : 0x%08x\r\n", g_aal_puc_debug);

    return CA_E_OK;
}

ca_status_t aal_puc_pvtbl_enable_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_OUT ca_boolean_t *val)
{
    ca_status_t ret = CA_E_OK;
    //ca_uint32_t map_mode = 0;

    PUC_PVTBL_DATA0_t pvtbl_data0;
    PUC_PVTBL_DATA1_t pvtbl_data1;
    PUC_PVTBL_DATA2_t pvtbl_data2;
    PUC_PVTBL_DATA3_t pvtbl_data3;
    PUC_PVTBL_DATA4_t pvtbl_data4;

    AAL_ASSERT_RET(port_idx <= AAL_MAX_LLID_TCONT_IDX, CA_E_PARAM);
    CA_NULL_PTR_CHECK(val);

    __puc_lock();

    //READ_INDIRCT_REG (device_id, port_idx, PUC_PVTBL_ACCESS);
    AAL_PON_REG_INDIRECT_READ(port_idx, PUC_PVTBL_ACCESS);

    pvtbl_data0.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA0);
    pvtbl_data1.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA1);
    pvtbl_data2.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA2);
    pvtbl_data3.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA3);
    pvtbl_data4.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA4);

    *val = pvtbl_data2.bf.entryvld;
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_pvtbl_enable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_boolean_t val)
{
    ca_status_t ret = CA_E_OK;

    PUC_PVTBL_DATA0_t pvtbl_data0;
    PUC_PVTBL_DATA1_t pvtbl_data1;
    PUC_PVTBL_DATA2_t pvtbl_data2;
    PUC_PVTBL_DATA3_t pvtbl_data3;
    PUC_PVTBL_DATA4_t pvtbl_data4;
    //PUC_VOQMAPCFG_t voq_map_cfg;

    AAL_ASSERT_RET(port_idx <= AAL_MAX_LLID_TCONT_IDX, CA_E_PARAM);
    CA_BOOL_VAL_CHECK(val);

   __puc_lock();

    //READ_INDIRCT_REG (device_id, port_idx, PUC_PVTBL_ACCESS);
    AAL_PON_REG_INDIRECT_READ(port_idx, PUC_PVTBL_ACCESS);

    pvtbl_data0.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA0);
    pvtbl_data1.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA1);
    pvtbl_data2.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA2);
    pvtbl_data3.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA3);
    pvtbl_data4.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA4);

    pvtbl_data2.bf.entryvld = val;

    //voq_map_cfg.wrd = AAL_PON_REG_READ (PUC_VOQMAPCFG);
    //map_mode = voq_map_cfg.bf.voqmapsel;
    pvtbl_data2.bf.schmode = 0;
    /*
    if ((map_mode == 0) || (map_mode == 1)) {
        pvtbl_data2.bf.schmode = 0;
    } else if (map_mode == 2) {
        pvtbl_data2.bf.schmode = 1;
    }*/

    AAL_PON_REG_WRITE (pvtbl_data0.wrd, PUC_PVTBL_DATA0);
    AAL_PON_REG_WRITE (pvtbl_data1.wrd, PUC_PVTBL_DATA1);
    AAL_PON_REG_WRITE (pvtbl_data2.wrd, PUC_PVTBL_DATA2);
    AAL_PON_REG_WRITE (pvtbl_data3.wrd, PUC_PVTBL_DATA3);
    AAL_PON_REG_WRITE (pvtbl_data4.wrd, PUC_PVTBL_DATA4);

    AAL_PON_REG_INDIRECT_WRITE(port_idx, PUC_PVTBL_ACCESS);
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_enable_voq(ca_device_id_t device_id, ca_boolean_t ena)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = 0;

    for (port = 0; port < tcont_num_per_chip; port++) {
        if (__TCONT_FOR_OMCC(port))
            ret = aal_puc_pvtbl_enable_set(device_id, port, TRUE);
        else
            ret = aal_puc_pvtbl_enable_set(device_id, port, ena);
        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}

ca_status_t aal_puc_port_voq_map_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_uint32_t queue_idx,
    CA_IN aal_puc_queue_cfg_msk_t mask,
    CA_IN aal_puc_queue_cfg_t *queue_cfg)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t map_mode = 0;
    aal_puc_queue_cfg_t queue_cfg_old;
    int port_num = 0, queue_num = 0;

    PUC_PVTBL_DATA0_t pvtbl_data0;
    PUC_PVTBL_DATA1_t pvtbl_data1;
    PUC_PVTBL_DATA2_t pvtbl_data2;
    PUC_PVTBL_DATA3_t pvtbl_data3;
    PUC_PVTBL_DATA4_t pvtbl_data4;

    if (!mask.u32) {
        return CA_E_OK;
    }

    if (!queue_cfg) {
        ca_printf("func %s line %d. queue_cfg is NULL!\r\n", __FUNCTION__, __LINE__);
        return CA_E_PARAM;
    }

    memset(&queue_cfg_old, 0, sizeof(aal_puc_queue_cfg_t));


	if((aal_pon_mac_mode == ONU_PON_MAC_GPON)
		||(aal_pon_mac_mode == ONU_PON_MAC_XGPON1)
		||(aal_pon_mac_mode == ONU_PON_MAC_XGSPON)
		||(aal_pon_mac_mode == ONU_PON_MAC_NGPON2))
	{
	    if(port_idx >= tcont_num_per_chip)
			return CA_E_PARAM;
	}else{
		if(port_idx >= llid_num_per_chip)
			return CA_E_PARAM;
	}
	
    #if 0
    if (queue_idx > AAL_PUC_MAX_QUEUE_PER_PORT_IDX) {
        ca_printf("func %s line %d, queue_idx is :%d.\n", __FUNCTION__, __LINE__,queue_idx);
        return CA_E_PARAM;
    }
    #endif
    if (mask.s.queue_id &&
        queue_cfg->queue_id >= AAL_PUC_MAX_QUEUE_NUM) {
        ca_printf("func %s line %d,queue_cfg->queue_id is :%d.\n", __FUNCTION__, __LINE__,queue_cfg->queue_id);
        return CA_E_PARAM;
    }
    if (mask.s.queue_ena &&
        queue_cfg->queue_ena> 1) {
        ca_printf("func %s line %d,queue_cfg->queue_ena is :%d.\n", __FUNCTION__, __LINE__,queue_cfg->queue_ena);
        return CA_E_PARAM;
    }
    if (mask.s.queue_pri &&
        queue_cfg->queue_pri > AAL_PUC_MAX_QUEUE_PER_PORT_IDX) {
        ca_printf("func %s line %d,queue_cfg->queue_pri is :%d.\n", __FUNCTION__, __LINE__,queue_cfg->queue_pri);
        return CA_E_PARAM;
    }

    if (mask.s.queue_weight &&
        queue_cfg->queue_weight > AAL_PUC_MAX_WEIGHT_ID) {
        ca_printf("func %s line %d,queue_cfg->queue_weight is :%d.\n", __FUNCTION__, __LINE__,queue_cfg->queue_weight);
        return CA_E_PARAM;
    }

	if(aal_pon_mac_mode <= ONU_PON_MAC_EPON_BI10G){
		port_num = llid_num_per_chip ;
		queue_num = queue_num_per_llid;
	}else if((aal_pon_mac_mode >= ONU_PON_MAC_GPON)&&(aal_pon_mac_mode <= ONU_PON_MAC_NGPON2)){
		port_num = tcont_num_per_chip ;
		queue_num = queue_num_per_tcont;
	}
	#if 0
	memset(&queue_cfg_old, 0 , sizeof(aal_puc_queue_cfg_t));
	//don't touch TCONT0
	for(i = 1; i< port_num; i++){
		for(j=0; j<queue_num; j++){
			ret = aal_puc_port_voq_map_get(device_id,i,j,&queue_cfg_old);
			if((queue_cfg_old.queue_ena ==1)&&(queue_cfg_old.queue_id == queue_cfg->queue_id)){
				ca_printf("func %s line %d,queue id should NOT be duplicated!\r\n", __FUNCTION__, __LINE__);
				return CA_E_PARAM;
			}
		}
	}
	#endif

    if (queue_idx == 0) {
        ret = aal_puc_port_voq_map_get(device_id, port_idx, 1, &queue_cfg_old);
        CA_RET_VALUE_CHECK(ret);
    } else if (queue_idx == 1) {
        ret = aal_puc_port_voq_map_get(device_id, port_idx, 0, &queue_cfg_old);
        CA_RET_VALUE_CHECK(ret);
    }
    __puc_lock();

    //READ_INDIRCT_REG (device_id, port_idx, PUC_PVTBL_ACCESS);
    AAL_PON_REG_INDIRECT_READ(port_idx, PUC_PVTBL_ACCESS);

    pvtbl_data0.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA0);
    pvtbl_data1.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA1);
    pvtbl_data2.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA2);
    pvtbl_data3.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA3);
    pvtbl_data4.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA4);

    if (queue_idx == 0) {
        pvtbl_data0.bf.voq0 = (queue_cfg->queue_id|(queue_cfg->queue_ena << 8));
        pvtbl_data2.bf.wrrratio0 = queue_cfg->queue_weight;

        if(queue_cfg->queue_weight != 0){
            if(queue_cfg_old.queue_weight == 0)
            {
                pvtbl_data2.bf.wrrratio1 = queue_cfg->queue_weight;
            }
        }else if(queue_cfg->queue_weight == 0){
            if(queue_cfg_old.queue_weight != 0)
            {
                pvtbl_data2.bf.wrrratio1 = queue_cfg->queue_weight;
            }
        }
    }
    if (queue_idx == 1) {
        pvtbl_data0.bf.voq1 = (queue_cfg->queue_id|(queue_cfg->queue_ena << 8));
        pvtbl_data2.bf.wrrratio1 = queue_cfg->queue_weight;

        if(queue_cfg->queue_weight != 0){
            if(queue_cfg_old.queue_weight == 0)
            {
                pvtbl_data2.bf.wrrratio0 = queue_cfg->queue_weight;
            }
        }else if(queue_cfg->queue_weight == 0){
            if(queue_cfg_old.queue_weight != 0)
            {
                pvtbl_data2.bf.wrrratio0 = queue_cfg->queue_weight;
            }
        }
    }
    if (queue_idx == 2) {
        pvtbl_data0.bf.voq2 = (queue_cfg->queue_id|(queue_cfg->queue_ena << 8));
        pvtbl_data3.bf.wrrratio2 = queue_cfg->queue_weight;
    }
    if (queue_idx == 3) {
        pvtbl_data0.bf.voq3 = queue_cfg->queue_id & 0x1f; //lower 5 bits in data0
        pvtbl_data1.bf.voq3 = (queue_cfg->queue_id & 0xe0) >> 5; //top 4 bits in data1  
        pvtbl_data1.bf.voq3 |= queue_cfg->queue_ena << 3;
        pvtbl_data3.bf.wrrratio3 = queue_cfg->queue_weight;
    }

    if (queue_idx == 4) {
        pvtbl_data1.bf.voq4 = (queue_cfg->queue_id|(queue_cfg->queue_ena << 8));
        pvtbl_data3.bf.wrrratio4 = queue_cfg->queue_weight;
    }
    if (queue_idx == 5) {
        pvtbl_data1.bf.voq5 = (queue_cfg->queue_id|(queue_cfg->queue_ena << 8));
        pvtbl_data3.bf.wrrratio5 = queue_cfg->queue_weight;
    }
    if (queue_idx == 6) {
        pvtbl_data1.bf.voq6 = (queue_cfg->queue_id|(queue_cfg->queue_ena << 8));
        pvtbl_data4.bf.wrrratio6 = queue_cfg->queue_weight;
    }
    if (queue_idx == 7) {
        pvtbl_data1.bf.voq7 = queue_cfg->queue_id & 1; //lower 1 bits in data1+        
        pvtbl_data2.bf.voq7 = (queue_cfg->queue_id & 0xfe) >> 1 | (queue_cfg->queue_ena << 7);
        pvtbl_data4.bf.wrrratio7 = queue_cfg->queue_weight;
    }
    pvtbl_data2.bf.entryvld = 1;

    ret = aal_puc_voq_map_cfg_get(device_id, &map_mode);
    if (ret != CA_E_OK)
    {
        __puc_unlock();
        ca_printf("func %s line %d.aal_puc_voq_map_cfg_get return failed!!\n", __FUNCTION__, __LINE__);
        return CA_E_ERROR;
    }

    if ((map_mode == 0) || (map_mode == 1)) {
        pvtbl_data2.bf.schmode = 0;
    } else if (map_mode == 2) {
        pvtbl_data2.bf.schmode = 1;
    }

#if 1
    { //only support SP+WRR
        int i;

        if(queue_cfg->queue_weight != 0){
            for(i = (queue_idx-1); i>= 0; i--){
                if (i == 0) {
                    if(pvtbl_data2.bf.wrrratio0 == 0)
                        pvtbl_data2.bf.wrrratio0 = 1;
                }
                if (i == 1) {
                    if(pvtbl_data2.bf.wrrratio1 == 0)
                        pvtbl_data2.bf.wrrratio1 = 1;
                }
                if (i == 2) {
                    if(pvtbl_data3.bf.wrrratio2 == 0)
                        pvtbl_data3.bf.wrrratio2 = 1;
                }
                if (i == 3) {
                    if(pvtbl_data3.bf.wrrratio3 == 0)
                        pvtbl_data3.bf.wrrratio3 = 1;
                }
                if (i == 4) {
                    if(pvtbl_data3.bf.wrrratio4 == 0)
                        pvtbl_data3.bf.wrrratio4 = 1;
                }
                if (i == 5) {
                    if(pvtbl_data3.bf.wrrratio5 == 0)
                        pvtbl_data3.bf.wrrratio5 = 1;
                }
                if (i == 6) {
                    if(pvtbl_data4.bf.wrrratio6 == 0)
                        pvtbl_data4.bf.wrrratio6 = 1;
                }
                if (i == 7) {
                    if(pvtbl_data4.bf.wrrratio7 == 0)
                        pvtbl_data4.bf.wrrratio7 = 1;
                }
            }
        }else if(queue_cfg->queue_weight == 0){
            for(i = (queue_idx+1); i< queue_num; i++){
                if (i == 0) {
                    if(pvtbl_data2.bf.wrrratio0 != 0)
                        pvtbl_data2.bf.wrrratio0 = 0;
                }
                if (i == 1) {
                    if(pvtbl_data2.bf.wrrratio1 != 0)
                        pvtbl_data2.bf.wrrratio1 = 0;
                }
                if (i == 2) {
                    if(pvtbl_data3.bf.wrrratio2 != 0)
                        pvtbl_data3.bf.wrrratio2 = 0;
                }
                if (i == 3) {
                    if(pvtbl_data3.bf.wrrratio3 != 0)
                        pvtbl_data3.bf.wrrratio3 = 0;
                }
                if (i == 4) {
                    if(pvtbl_data3.bf.wrrratio4 != 0)
                        pvtbl_data3.bf.wrrratio4 = 0;
                }
                if (i == 5) {
                    if(pvtbl_data3.bf.wrrratio5 != 0)
                        pvtbl_data3.bf.wrrratio5 = 0;
                }
                if (i == 6) {
                    if(pvtbl_data4.bf.wrrratio6 != 0)
                        pvtbl_data4.bf.wrrratio6 = 0;
                }
                if (i == 7) {
                    if(pvtbl_data4.bf.wrrratio7 != 0)
                        pvtbl_data4.bf.wrrratio7 = 0;
                }
            }
        }
    }
#endif

    AAL_PON_REG_WRITE (pvtbl_data0.wrd, PUC_PVTBL_DATA0);
    AAL_PON_REG_WRITE (pvtbl_data1.wrd, PUC_PVTBL_DATA1);
    AAL_PON_REG_WRITE (pvtbl_data2.wrd, PUC_PVTBL_DATA2);
    AAL_PON_REG_WRITE (pvtbl_data3.wrd, PUC_PVTBL_DATA3);
    AAL_PON_REG_WRITE (pvtbl_data4.wrd, PUC_PVTBL_DATA4);

    AAL_PON_REG_INDIRECT_WRITE(port_idx, PUC_PVTBL_ACCESS);
    __puc_unlock();

    return ret;
}
ca_status_t aal_puc_port_voqs_map_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t queue_idx = 0;
    ca_uint32_t queue_num = 0;
    aal_puc_queue_cfg_t queue_cfg;
    ca_uint32_t pon_mode = aal_pon_mac_mode_get(device_id);

    memset(&queue_cfg, 0 , sizeof(aal_puc_queue_cfg_t));

    queue_num = CA_PON_MODE_IS_GPON_FAMILY(pon_mode)
        ? queue_num_per_tcont : queue_num_per_llid;

    ca_printf("\r\n");
    ca_printf("%-5s Queue-Idx  Queue  Queue-Ena  Queue-Weight  Queue-Pri\r\n",
       CA_PON_MODE_IS_GPON_FAMILY(pon_mode) ? "TCONT" : "LLID");
    ca_printf("---------------------------------------------------------\r\n");

    for(queue_idx = 0; queue_idx < queue_num; queue_idx++){
        ret = aal_puc_port_voq_map_get(device_id,port_idx,queue_idx,&queue_cfg);
        if (CA_E_OK == ret) {
            ca_printf("%-5d %-9d  %-5d  %-9d  %-12d  %-9d\r\n",
                port_idx, queue_idx, queue_cfg.queue_id,
                queue_cfg.queue_ena, queue_cfg.queue_weight,
                queue_cfg.queue_pri);

        }
        //ca_printf("port_idx: %d, queue_idx:%d,queue_id:%d, queue_ena:%d,queue_weight:%d,queue_pri:%d\r\n",port_idx,queue_idx,queue_cfg.queue_id,queue_cfg.queue_ena,queue_cfg.queue_weight,queue_cfg.queue_pri);
    }

    return ret;
}

ca_status_t aal_puc_port_voq_map_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_uint32_t queue_idx,
    CA_OUT aal_puc_queue_cfg_t *queue_cfg)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t pon_mode = aal_pon_mac_mode_get(device_id);
    ca_uint8_t port_num = 0;
    ca_uint8_t queue_num = 0;

    PUC_PVTBL_DATA0_t pvtbl_data0;
    PUC_PVTBL_DATA1_t pvtbl_data1;
    PUC_PVTBL_DATA2_t pvtbl_data2;
    PUC_PVTBL_DATA3_t pvtbl_data3;
    PUC_PVTBL_DATA4_t pvtbl_data4;

    port_num = CA_PON_MODE_IS_GPON_FAMILY(pon_mode)
        ? tcont_num_per_chip : llid_num_per_chip;
    queue_num = CA_PON_MODE_IS_GPON_FAMILY(pon_mode)
        ? queue_num_per_tcont : queue_num_per_llid;

    if (port_idx >= port_num) {
        return CA_E_PARAM;
    }
    if (queue_idx >= queue_num) {
        return CA_E_PARAM;
    }
    __puc_lock();

    AAL_PON_REG_INDIRECT_READ(port_idx, PUC_PVTBL_ACCESS);

    pvtbl_data0.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA0);
    pvtbl_data1.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA1);
    pvtbl_data2.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA2);
    pvtbl_data3.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA3);
    pvtbl_data4.wrd = AAL_PON_REG_READ (PUC_PVTBL_DATA4);
    if (queue_idx == 0) {
        queue_cfg->queue_ena = (pvtbl_data0.bf.voq0 >> 8) & 0x1;
        queue_cfg->queue_id = pvtbl_data0.bf.voq0 & 0xff;
        queue_cfg->queue_pri = 0;
        queue_cfg->queue_weight = pvtbl_data2.bf.wrrratio0;
    }
    if (queue_idx == 1) {
        queue_cfg->queue_ena = (pvtbl_data0.bf.voq1 >> 8) & 0x1;
        queue_cfg->queue_id = pvtbl_data0.bf.voq1 & 0xff;
        queue_cfg->queue_pri = 1;
        queue_cfg->queue_weight = pvtbl_data2.bf.wrrratio1;
    }
    if (queue_idx == 2) {
        queue_cfg->queue_ena = (pvtbl_data0.bf.voq2 >> 8) & 0x1;
        queue_cfg->queue_id = pvtbl_data0.bf.voq2 & 0xff;
        queue_cfg->queue_pri = 2;
         queue_cfg->queue_weight = pvtbl_data3.bf.wrrratio2;
    }
    if (queue_idx == 3) {
        queue_cfg->queue_ena = (pvtbl_data1.bf.voq3 & 8) >> 3; //top 4 bits in data1+        
        queue_cfg->queue_id = (pvtbl_data0.bf.voq3 & 0x1f) | ((pvtbl_data1.bf.voq3 & 0x7) << 5);
        queue_cfg->queue_pri = 3;
        queue_cfg->queue_weight = pvtbl_data3.bf.wrrratio3;
    }
    if (queue_idx == 4) {
        queue_cfg->queue_ena = (pvtbl_data1.bf.voq4 >> 8) & 0x1;
        queue_cfg->queue_id = pvtbl_data1.bf.voq4 & 0xff;
        queue_cfg->queue_pri = 4;
        queue_cfg->queue_weight = pvtbl_data3.bf.wrrratio4;
    }
    if (queue_idx == 5) {
        queue_cfg->queue_ena = (pvtbl_data1.bf.voq5 >> 8) & 0x1;
        queue_cfg->queue_id = pvtbl_data1.bf.voq5 & 0xff;
        queue_cfg->queue_pri = 5;
        queue_cfg->queue_weight = pvtbl_data3.bf.wrrratio5;
    }
    if (queue_idx == 6) {
        queue_cfg->queue_ena = (pvtbl_data1.bf.voq6 >> 8) & 0x1;
        queue_cfg->queue_id = pvtbl_data1.bf.voq6 & 0xff;
        queue_cfg->queue_pri = 6;
        queue_cfg->queue_weight = pvtbl_data4.bf.wrrratio6;
    }
    if (queue_idx == 7) {
        queue_cfg->queue_ena = (pvtbl_data2.bf.voq7 & 0x80) >> 7;
        queue_cfg->queue_id = ((pvtbl_data2.bf.voq7 & 0x7f) << 1 ) | (pvtbl_data1.bf.voq7 & 1);
        queue_cfg->queue_pri = 7;
        queue_cfg->queue_weight = pvtbl_data4.bf.wrrratio7;
    }
    __puc_unlock();

    return ret;
}

ca_status_t aal_puc_voq_pkt_len_hp_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_IN ca_uint32_t hptr)
{
    PUC_HPMEM_DATA_t  hpmem_data;

    AAL_ASSERT_RET(voq_id < AAL_PUC_QUEUE_NUM, CA_E_PARAM);

   __puc_lock();

    AAL_PON_REG_INDIRECT_READ(voq_id, PUC_HPMEM_ACCESS);

    hpmem_data.wrd = AAL_PON_REG_READ (PUC_HPMEM_DATA);

    hpmem_data.bf.hptr = hptr;

    AAL_PON_REG_WRITE (hpmem_data.wrd, PUC_HPMEM_DATA);
    AAL_PON_REG_INDIRECT_WRITE(voq_id, PUC_HPMEM_ACCESS);
    __puc_unlock();

    return CA_E_OK;
}

ca_status_t aal_puc_voq_pkt_len_hp_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_OUT ca_uint32_t *hptr)
{
    PUC_HPMEM_DATA_t  hpmem_data;

    AAL_ASSERT_RET(voq_id < AAL_PUC_QUEUE_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(hptr != NULL, CA_E_PARAM);
    __puc_lock();
    AAL_PON_REG_INDIRECT_READ(voq_id, PUC_HPMEM_ACCESS);
    hpmem_data.wrd = AAL_PON_REG_READ (PUC_HPMEM_DATA);
    *hptr = hpmem_data.bf.hptr;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_voq_stats_get(
        CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    aal_puc_voq_total_que_len_cfg_t cfg;

    for (i = 0; i< AAL_PUC_QUEUE_NUM; i++){
        memset(&cfg, 0 , sizeof(aal_puc_voq_total_que_len_cfg_t));
        ret = aal_puc_voq_total_queue_len_mem_get(device_id, i ,&cfg);
        CA_RET_VALUE_CHECK(ret);
        if (cfg.bytecnt != 0 || cfg.pktcnt != 0) {
        ca_printf("VoQ %u : Byte_Cnt :%u, Pkt_Cnt:%u\r\n",
            i, cfg.bytecnt, cfg.pktcnt);
        }
    }

    return ret;
}


ca_status_t aal_puc_bcmem_for_all_voq_get(
        CA_IN ca_device_id_t device_id,
        CA_IN ca_uint32_t queue_start_index,
        CA_IN ca_uint32_t queue_end_index)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    aal_puc_voq_byte_cnt_mem_cfg_t byte_cnt_cfg;

    AAL_ASSERT_RET(queue_start_index < AAL_PUC_QUEUE_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(queue_end_index < AAL_PUC_QUEUE_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(queue_start_index <= queue_end_index, CA_E_PARAM);


    for (i = queue_start_index; i<= queue_end_index; i++){
        memset(&byte_cnt_cfg, 0 , sizeof(aal_puc_voq_byte_cnt_mem_cfg_t));
        ret = aal_puc_voq_byte_cnt_mem_get(device_id, i ,&byte_cnt_cfg);
        CA_RET_VALUE_CHECK(ret);
        if (byte_cnt_cfg.bytecnt == 0)
            continue;
        ca_printf("voq id: %d, bytecnt :%d, bufcnt:%d\r\n",
            i,byte_cnt_cfg.bytecnt,byte_cnt_cfg.bufcnt);
    }

    return ret;

}

ca_status_t aal_puc_voq_len_get(CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    aal_puc_voq_byte_cnt_mem_cfg_t byte_cnt_cfg;
    aal_puc_voq_total_que_len_cfg_t  que_cfg;

    ca_printf("\r\nVoQ ID  byte-count    buffer-count  pkt-count\r\n");
    ca_printf("----------------------------------------------------\r\n");
    for (i = 0; i <  AAL_PUC_QUEUE_NUM; i++){
        memset(&byte_cnt_cfg, 0 , sizeof(aal_puc_voq_byte_cnt_mem_cfg_t));
        memset(&que_cfg, 0 , sizeof(aal_puc_voq_total_que_len_cfg_t));

        ret = aal_puc_voq_byte_cnt_mem_get(device_id, i ,&byte_cnt_cfg);
        CA_RET_VALUE_CHECK(ret);
        ret = aal_puc_voq_total_queue_len_mem_get(device_id, i, &que_cfg);
        CA_RET_VALUE_CHECK(ret);

    if(byte_cnt_cfg.bufcnt)
            ca_printf("%-6u  %-12u  %-12u  %-12u\r\n",
                    i, byte_cnt_cfg.bytecnt, byte_cnt_cfg.bufcnt, que_cfg.pktcnt);
    }

    return ret;

}

ca_status_t aal_puc_voq_byte_cnt_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_OUT aal_puc_voq_byte_cnt_mem_cfg_t *byte_cnt_cfg)
{
    PUC_BCMEM_DATA0_t  bcmem_data0;
    PUC_BCMEM_DATA1_t  bcmem_data1;

    AAL_ASSERT_RET(voq_id < AAL_PUC_QUEUE_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(byte_cnt_cfg != NULL, CA_E_PARAM);

    __puc_lock();

    AAL_PON_REG_INDIRECT_READ(voq_id, PUC_BCMEM_ACCESS);

    bcmem_data0.wrd = AAL_PON_REG_READ (PUC_BCMEM_DATA0);
    bcmem_data1.wrd = AAL_PON_REG_READ (PUC_BCMEM_DATA1);
    byte_cnt_cfg->bytecnt = bcmem_data0.bf.bytecnt;
    byte_cnt_cfg->bufcnt = bcmem_data0.bf.bufcnt | (bcmem_data1.bf.bufcnt<<11);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_tqmem_for_all_voq_get(
        CA_IN ca_device_id_t device_id,
        CA_IN ca_uint32_t queue_start_index,
        CA_IN ca_uint32_t queue_end_index)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    aal_puc_voq_total_que_len_cfg_t cfg;

    AAL_ASSERT_RET(queue_start_index < AAL_PUC_QUEUE_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(queue_end_index < AAL_PUC_QUEUE_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(queue_start_index <= queue_end_index, CA_E_PARAM);

    for (i = queue_start_index; i<= queue_end_index; i++){
        memset(&cfg, 0 , sizeof(aal_puc_voq_total_que_len_cfg_t));
        ret = aal_puc_voq_total_queue_len_mem_get(device_id, i ,&cfg);
        CA_RET_VALUE_CHECK(ret);
        if (cfg.bytecnt == 0 && cfg.pktcnt == 0)
            continue;

        ca_printf("voq id: %d,bytecnt :%d,pktcnt:%d\r\n",i,cfg.bytecnt,cfg.pktcnt);
    }

    return ret;
}

ca_status_t aal_puc_voq_total_queue_len_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_OUT aal_puc_voq_total_que_len_cfg_t *cfg)
{
    PUC_TQMEM_DATA0_t  tqmem_data0;
    PUC_TQMEM_DATA1_t  tqmem_data1;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(voq_id < AAL_PUC_QUEUE_NUM, CA_E_PARAM);

    __puc_lock();

    AAL_PON_REG_INDIRECT_READ(voq_id, PUC_TQMEM_ACCESS);

    tqmem_data0.wrd = AAL_PON_REG_READ (PUC_TQMEM_DATA0);
    tqmem_data1.wrd = AAL_PON_REG_READ (PUC_TQMEM_DATA1);

    cfg->bytecnt = tqmem_data0.bf.bytecnt;
    cfg->pktcnt = (tqmem_data1.bf.pktcnt<<11)|tqmem_data0.bf.pktcnt;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_port_open_pkt_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN aal_puc_port_open_pkt_cfg_msk_t mask,
    CA_IN aal_puc_port_open_pkt_cfg_t *cfg)
{
    PUC_OPMEM_DATA_t  opmem_data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(port_idx <= AAL_MAX_LLID_TCONT_IDX, CA_E_PARAM);

    if (!mask.u32) {
        return CA_E_OK;
    }

    __puc_lock();

    AAL_PON_REG_INDIRECT_READ(port_idx, PUC_OPMEM_ACCESS);

    opmem_data.wrd = AAL_PON_REG_READ (PUC_OPMEM_DATA);

    if (mask.s.bytecnt) {
        opmem_data.bf.bytecnt = cfg->bytecnt;
    }

    if (mask.s.voqid) {
        opmem_data.bf.voqid = cfg->voqid;
    }

    if (mask.s.gempid) {
        opmem_data.bf.gempid = cfg->gempid;
    }

    AAL_PON_REG_WRITE (opmem_data.wrd, PUC_OPMEM_DATA);
    AAL_PON_REG_INDIRECT_WRITE(port_idx, PUC_OPMEM_ACCESS);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_tcont_rem_buffers_get(
    CA_IN ca_device_id_t device_id)
{
    ca_status_t  ret = CA_E_OK;
    ca_uint32_t  tcont = 0;
    aal_puc_port_open_pkt_cfg_t  opmen_cfg;

    ca_printf("\r\nTCONT    GEM-ID     VoQ-ID   rem buffers\r\n");
    ca_printf("----------------------------------------------------\r\n");

    for (tcont = 0; tcont < tcont_num_per_chip; tcont++){
        memset(&opmen_cfg, 0 , sizeof(aal_puc_port_open_pkt_cfg_t));
        ret = aal_puc_port_open_pkt_mem_get(device_id, tcont, &opmen_cfg);
        CA_RET_VALUE_CHECK(ret);
        if(opmen_cfg.bytecnt)
            ca_printf("%-8u  %-8u  %-8u  %u\r\n",
            tcont, opmen_cfg.gempid, opmen_cfg.voqid, opmen_cfg.bytecnt);
    }

    return CA_E_OK;
}

ca_status_t aal_puc_port_open_pkt_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_OUT aal_puc_port_open_pkt_cfg_t *cfg)
{
    PUC_OPMEM_DATA_t  opmem_data;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(port_idx <= AAL_MAX_LLID_TCONT_IDX, CA_E_PARAM);

    __puc_lock();

    AAL_PON_REG_INDIRECT_READ(port_idx, PUC_OPMEM_ACCESS);

    opmem_data.wrd = AAL_PON_REG_READ (PUC_OPMEM_DATA);

    cfg->bytecnt = opmem_data.bf.bytecnt;
    cfg->voqid   = opmem_data.bf.voqid;
    cfg->gempid  = opmem_data.bf.gempid;
    __puc_unlock();
    return CA_E_OK;
}


ca_status_t aal_puc_port_voq_sche_info_tbl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_uint32_t queue_idx,
    CA_IN ca_uint32_t tbc)
{
    PUC_VSTBL_DATA0_t vstbl_data0;
    PUC_VSTBL_DATA1_t vstbl_data1;
    PUC_VSTBL_DATA2_t vstbl_data2;
    PUC_VSTBL_DATA3_t vstbl_data3;
    PUC_VSTBL_DATA4_t vstbl_data4;

    AAL_ASSERT_RET(port_idx <= AAL_MAX_LLID_TCONT_IDX, CA_E_PARAM);
    AAL_ASSERT_RET(queue_idx <= AAL_PUC_MAX_QUEUE_PER_PORT_IDX, CA_E_PARAM);

    __puc_lock();
    AAL_PON_REG_INDIRECT_READ(port_idx, PUC_VSTBL_ACCESS);

    vstbl_data0.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA0);
    vstbl_data1.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA1);
    vstbl_data2.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA2);
    vstbl_data3.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA3);
    vstbl_data4.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA4);

    if (queue_idx == 0) {
        vstbl_data0.bf.tbc0 = tbc&0xFFFFF;
    }
    if (queue_idx == 1) {
        vstbl_data0.bf.tbc1 = (tbc&0xFFF);
        vstbl_data1.bf.tbc1 = (tbc>>12&0xFF);
    }
    if (queue_idx == 2) {
        vstbl_data1.bf.tbc2 = tbc&0xFFFFF;
    }
    if (queue_idx == 3) {
        vstbl_data1.bf.tbc3 = (tbc&0xF);
        vstbl_data2.bf.tbc3 = (tbc>>4&0xFFFF);
    }

    if (queue_idx == 4) {
        vstbl_data2.bf.tbc4 = (tbc&0xFFFF);
        vstbl_data3.bf.tbc4 = (tbc>>16&0xF);
    }
    if (queue_idx == 5) {
        vstbl_data3.bf.tbc5 = tbc&0xFFFFF;
    }
    if (queue_idx == 6) {
        vstbl_data3.bf.tbc6 = (tbc&0xFF);
        vstbl_data4.bf.tbc6 = (tbc>>8&0xFFF);
    }
    if (queue_idx == 7) {
        vstbl_data4.bf.tbc7 = tbc&0xFFFFF;
    }

    AAL_PON_REG_WRITE (vstbl_data0.wrd, PUC_VSTBL_DATA0);
    AAL_PON_REG_WRITE (vstbl_data1.wrd, PUC_VSTBL_DATA1);
    AAL_PON_REG_WRITE (vstbl_data2.wrd, PUC_VSTBL_DATA2);
    AAL_PON_REG_WRITE (vstbl_data3.wrd, PUC_VSTBL_DATA3);
    AAL_PON_REG_WRITE (vstbl_data4.wrd, PUC_VSTBL_DATA4);

    AAL_PON_REG_INDIRECT_WRITE(port_idx, PUC_VSTBL_ACCESS);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_port_voq_sche_info_tbl_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t port_idx,
    CA_IN ca_uint32_t queue_idx,
    CA_OUT ca_uint32_t *tbc)
{
    PUC_VSTBL_DATA0_t vstbl_data0;
    PUC_VSTBL_DATA1_t vstbl_data1;
    PUC_VSTBL_DATA2_t vstbl_data2;
    PUC_VSTBL_DATA3_t vstbl_data3;
    PUC_VSTBL_DATA4_t vstbl_data4;

    AAL_ASSERT_RET(tbc != NULL, CA_E_PARAM);
    AAL_ASSERT_RET(port_idx <= AAL_MAX_LLID_TCONT_IDX, CA_E_PARAM);
    AAL_ASSERT_RET(queue_idx <= AAL_PUC_MAX_QUEUE_PER_PORT_IDX, CA_E_PARAM);

    __puc_lock();

    AAL_PON_REG_INDIRECT_READ(port_idx, PUC_VSTBL_ACCESS);

    vstbl_data0.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA0);
    vstbl_data1.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA1);
    vstbl_data2.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA2);
    vstbl_data3.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA3);
    vstbl_data4.wrd = AAL_PON_REG_READ (PUC_VSTBL_DATA4);
    if (queue_idx == 0) {
        *tbc = vstbl_data0.bf.tbc0;
    }
    if (queue_idx == 1) {
        *tbc = (vstbl_data1.bf.tbc1<<12)|vstbl_data0.bf.tbc1;
    }
    if (queue_idx == 2) {
        *tbc = vstbl_data1.bf.tbc2;
    }
    if (queue_idx == 3) {
        *tbc = (vstbl_data2.bf.tbc3<<4)|vstbl_data1.bf.tbc3;
    }
    if (queue_idx == 4) {
        *tbc = (vstbl_data3.bf.tbc4<<16)|vstbl_data2.bf.tbc4;
    }
    if (queue_idx == 5) {
        *tbc = vstbl_data3.bf.tbc5;
    }
    if (queue_idx == 6) {
        *tbc = (vstbl_data4.bf.tbc6<<8)|vstbl_data3.bf.tbc6;
    }
    if (queue_idx == 7) {
        *tbc = vstbl_data4.bf.tbc7;
    }
    __puc_unlock();

    return CA_E_OK;
}


ca_status_t aal_puc_voq_sche_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_voq_sche_cfg_t *voq_sche_config
)
{
    ca_status_t ret = CA_E_OK;
    PUC_VOQSCHCFG_t voq_sche_cfg;
    __puc_lock();
    voq_sche_cfg.wrd = AAL_PON_REG_READ (PUC_VOQSCHCFG);
    voq_sche_config->basesel = voq_sche_cfg.bf.basesel;
    voq_sche_config->ratecomp = voq_sche_cfg.bf.ratecomp;
    voq_sche_config->dwrrunit = voq_sche_cfg.bf.dwrrunit;
    voq_sche_config->pkgbschmode = voq_sche_cfg.bf.pkgbschmode;
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_voq_sche_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_voq_sche_cfg_msk_t mask,
    CA_IN aal_puc_voq_sche_cfg_t *voq_sche_config
)
{
    ca_status_t ret = CA_E_OK;
    PUC_VOQSCHCFG_t voq_sche_cfg;
    voq_sche_cfg.wrd = 0;
    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    voq_sche_cfg.wrd = AAL_PON_REG_READ (PUC_VOQSCHCFG);
    if (mask.s.basesel) {
        if (voq_sche_cfg.bf.basesel != voq_sche_config->basesel) {
            voq_sche_cfg.bf.basesel = voq_sche_config->basesel;
        }
    }

    if (mask.s.ratecomp) {
        if (voq_sche_cfg.bf.ratecomp != voq_sche_config->ratecomp) {
            voq_sche_cfg.bf.ratecomp = voq_sche_config->ratecomp;
        }
    }

    if (mask.s.dwrrunit) {
        if (voq_sche_cfg.bf.dwrrunit != voq_sche_config->dwrrunit) {
            voq_sche_cfg.bf.dwrrunit = voq_sche_config->dwrrunit;
        }
    }

    if(mask.s.pkgbschmode){
        if(voq_sche_cfg.bf.pkgbschmode != voq_sche_config->pkgbschmode){
            voq_sche_cfg.bf.pkgbschmode = voq_sche_config->pkgbschmode;
        }
    }

    AAL_PON_REG_WRITE (voq_sche_cfg.wrd, PUC_VOQSCHCFG);
    __puc_unlock();
    return ret;
}


ca_status_t aal_puc_voq_map_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *mode
)
{
    ca_status_t ret = CA_E_OK;
    PUC_VOQMAPCFG_t voq_map_cfg;
    //__puc_lock();
    voq_map_cfg.wrd = AAL_PON_REG_READ (PUC_VOQMAPCFG);

    *mode = voq_map_cfg.bf.voqmapsel;
    //__puc_unlock();
    return ret;
}

ca_status_t aal_puc_voq_map_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t mode
)
{
    ca_status_t ret = CA_E_OK;
    PUC_VOQMAPCFG_t voq_map_cfg;
    __puc_lock();

    voq_map_cfg.bf.voqmapsel = mode;
    AAL_PON_REG_WRITE (voq_map_cfg.wrd, PUC_VOQMAPCFG);
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_ctrl_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_ctrl_cfg_t *puc_ctrl_config
)
{
    ca_status_t ret = CA_E_OK;
    PUC_CTRL_t puc_ctrl_cfg;
    PUC_CTRL1_t puc_ctrl1_cfg;

    //__puc_lock();

    puc_ctrl_cfg.wrd = AAL_PON_REG_READ (PUC_CTRL);
    puc_ctrl1_cfg.wrd = AAL_PON_REG_READ (PUC_CTRL1);

    puc_ctrl_config->pkt_mode_class0_cnt = puc_ctrl_cfg.bf.pkt_mode_class0_cnt;
    puc_ctrl_config->pkt_mode_class1_cnt = puc_ctrl_cfg.bf.pkt_mode_class1_cnt;
    puc_ctrl_config->force_init_done = puc_ctrl_cfg.bf.force_init_done;
    puc_ctrl_config->addtok_en = puc_ctrl_cfg.bf.addtok_en;
    puc_ctrl_config->subtok_en = puc_ctrl_cfg.bf.subtok_en;
    puc_ctrl_config->pm_mode = puc_ctrl_cfg.bf.pm_mode;
    puc_ctrl_config->rl_force_init_done = puc_ctrl_cfg.bf.rl_force_init_done;
    puc_ctrl_config->rl_addtok_en = puc_ctrl_cfg.bf.rl_addtok_en;
    puc_ctrl_config->rl_subtok_en = puc_ctrl_cfg.bf.rl_subtok_en;
    puc_ctrl_config->rl_en = puc_ctrl_cfg.bf.rl_en;
    puc_ctrl_config->shp_en = puc_ctrl_cfg.bf.shp_en;
    puc_ctrl_config->rlovhd = puc_ctrl1_cfg.bf.rlovhd;
    puc_ctrl_config->shpovhd = puc_ctrl1_cfg.bf.shpovhd;
    puc_ctrl_config->agrshpovhd = puc_ctrl1_cfg.bf.agrshpovhd;
    
	//__puc_unlock();

    return ret;
}

ca_status_t aal_puc_ctrl_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_ctrl_cfg_msk_t mask,
    CA_IN aal_puc_ctrl_cfg_t *puc_ctrl_config
)
{
    ca_status_t ret = CA_E_OK;
    PUC_CTRL_t puc_ctrl_cfg;
    PUC_CTRL1_t puc_ctrl1_cfg;

    puc_ctrl_cfg.wrd = 0;
    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    if (mask.s.pkt_mode_class0_cnt) {
        puc_ctrl_cfg.bf.pkt_mode_class0_cnt = puc_ctrl_config->pkt_mode_class0_cnt;
    }

    if (mask.s.pkt_mode_class1_cnt) {
        puc_ctrl_cfg.bf.pkt_mode_class1_cnt = puc_ctrl_config->pkt_mode_class1_cnt;
    }
        
    if (mask.s.pm_mode) {
        puc_ctrl_cfg.bf.pm_mode = puc_ctrl_config->pm_mode;
    }

    if (mask.s.rl_force_init_done) {
        puc_ctrl_cfg.bf.rl_force_init_done = puc_ctrl_config->rl_force_init_done;
    }

    if (mask.s.rl_addtok_en) {
        puc_ctrl_cfg.bf.rl_addtok_en = puc_ctrl_config->rl_addtok_en;
    }
    
    if (mask.s.rl_subtok_en) {
        puc_ctrl_cfg.bf.rl_subtok_en = puc_ctrl_config->rl_subtok_en;
    }

    if (mask.s.rl_en) {
        puc_ctrl_cfg.bf.rl_en = puc_ctrl_config->rl_en;
    }

    if (mask.s.rl_force_init_done) {
        puc_ctrl_cfg.bf.rl_force_init_done = puc_ctrl_config->rl_force_init_done;
    }

    if (mask.s.force_init_done) {
        puc_ctrl_cfg.bf.force_init_done = puc_ctrl_config->force_init_done;
    }

    if (mask.s.addtok_en) {
        puc_ctrl_cfg.bf.addtok_en = puc_ctrl_config->addtok_en;
    }

    if (mask.s.subtok_en) {
        puc_ctrl_cfg.bf.subtok_en = puc_ctrl_config->subtok_en;
    }

    if (mask.s.shp_en) {
        puc_ctrl_cfg.bf.shp_en = puc_ctrl_config->shp_en;
    }

    if (mask.s.rlovhd) {
        puc_ctrl1_cfg.bf.rlovhd = puc_ctrl_config->rlovhd;
    }

    if (mask.s.shpovhd) {
        puc_ctrl1_cfg.bf.shpovhd = puc_ctrl_config->shpovhd;
    }

    if (mask.s.agrshpovhd) {
        puc_ctrl1_cfg.bf.agrshpovhd = puc_ctrl_config->agrshpovhd;
    }

    AAL_PON_REG_WRITE (puc_ctrl_cfg.wrd, PUC_CTRL);
    AAL_PON_REG_WRITE (puc_ctrl1_cfg.wrd, PUC_CTRL1);
     __puc_unlock();
    return ret;
}

ca_status_t aal_puc_btc_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_btc_cfg_t *puc_btc_config
)
{
    ca_status_t ret = CA_E_OK;
    PUC_BTCCFG_t puc_btc_cfg;

    // __puc_lock();
    puc_btc_cfg.wrd = AAL_PON_REG_READ (PUC_BTCCFG);

    puc_btc_config->pfovrhd = puc_btc_cfg.bf.pfovrhd;
    puc_btc_config->schmode = puc_btc_cfg.bf.schmode;
    puc_btc_config->ethmode = puc_btc_cfg.bf.ethmode;
    puc_btc_config->wdaligned = puc_btc_cfg.bf.wdaligned;
    puc_btc_config->lrgfrmfragen = puc_btc_cfg.bf.lrgfrmfragen;
    puc_btc_config->pyld1gfecon = puc_btc_cfg.bf.pyld1gfecon;
    puc_btc_config->schen = puc_btc_cfg.bf.schen;
    puc_btc_config->sch2en= puc_btc_cfg.bf.sch2en;
    puc_btc_config->minrmnwindowsz = puc_btc_cfg.bf.minrmnwindowsz;
    //__puc_unlock();

    return ret;
}

ca_status_t aal_puc_btc_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_btc_cfg_msk_t mask,
    CA_IN aal_puc_btc_cfg_t *puc_btc_config
)
{
    ca_status_t ret = CA_E_OK;
    PUC_BTCCFG_t puc_btc_cfg;

    puc_btc_cfg.wrd = 0;
    if (!mask.u32) {
        return CA_E_OK;
    }
     __puc_lock();
    puc_btc_cfg.wrd = AAL_PON_REG_READ (PUC_BTCCFG);
    if (mask.s.pfovrhd) {
        if (puc_btc_cfg.bf.pfovrhd != puc_btc_config->pfovrhd) {
            puc_btc_cfg.bf.pfovrhd = puc_btc_config->pfovrhd;
        }
    }


    if (mask.s.schmode) {
        if (puc_btc_cfg.bf.schmode != puc_btc_config->schmode) {
            puc_btc_cfg.bf.schmode = puc_btc_config->schmode;
        }
    }

    if (mask.s.ethmode) {
        if (puc_btc_cfg.bf.ethmode != puc_btc_config->ethmode) {
            puc_btc_cfg.bf.ethmode = puc_btc_config->ethmode;
        }
    }

    if (mask.s.wdaligned) {
        if (puc_btc_cfg.bf.wdaligned != puc_btc_config->wdaligned) {
            puc_btc_cfg.bf.wdaligned = puc_btc_config->wdaligned;
        }
    }

    if (mask.s.lrgfrmfragen) {
        if (puc_btc_cfg.bf.lrgfrmfragen != puc_btc_config->lrgfrmfragen) {
            puc_btc_cfg.bf.lrgfrmfragen = puc_btc_config->lrgfrmfragen;
        }
    }

    if (mask.s.pyld1gfecon) {
        if (puc_btc_cfg.bf.pyld1gfecon != puc_btc_config->pyld1gfecon) {
            puc_btc_cfg.bf.pyld1gfecon = puc_btc_config->pyld1gfecon;
        }
    }

    if (mask.s.schen) {
        if (puc_btc_cfg.bf.schen != puc_btc_config->schen) {
            puc_btc_cfg.bf.schen = puc_btc_config->schen;
        }
    }
    
    if (mask.s.sch2en) {
        if (puc_btc_cfg.bf.sch2en != puc_btc_config->sch2en) {
            puc_btc_cfg.bf.sch2en = puc_btc_config->sch2en;
        }
    }

    if (mask.s.minrmnwindowsz) {
        if (puc_btc_cfg.bf.minrmnwindowsz != puc_btc_config->minrmnwindowsz) {
            puc_btc_cfg.bf.minrmnwindowsz = puc_btc_config->minrmnwindowsz;
        }
    }

    AAL_PON_REG_WRITE (puc_btc_cfg.wrd, PUC_BTCCFG);
     __puc_unlock();
    return ret;
}

ca_status_t aal_puc_rpt_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_rpt_cfg_t *puc_rpt_config
)
{
    ca_status_t ret = CA_E_OK;
    PUC_RPTCFG_t puc_rpt_cfg;

     __puc_lock();
    puc_rpt_cfg.wrd = AAL_PON_REG_READ (PUC_RPTCFG);

    puc_rpt_config->numqsets = puc_rpt_cfg.bf.numqsets;
    puc_rpt_config->rptpreamblovhd = puc_rpt_cfg.bf.rptpreamblovhd;
    puc_rpt_config->rptipgovhd = puc_rpt_cfg.bf.rptipgovhd;
    puc_rpt_config->rpt1gfecon = puc_rpt_cfg.bf.rpt1gfecon;
    puc_rpt_config->firstqstot = puc_rpt_cfg.bf.firstqstot;
    puc_rpt_config->lastqstot = puc_rpt_cfg.bf.lastqstot;
    puc_rpt_config->pkgarpt   = puc_rpt_cfg.bf.pkgarpt;
    puc_rpt_config->aggrth    = puc_rpt_cfg.bf.aggrth;
    __puc_unlock();

    return ret;
}

ca_status_t aal_puc_rpt_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_rpt_cfg_msk_t mask,
    CA_IN aal_puc_rpt_cfg_t *puc_rpt_config
)
{
    ca_status_t ret = CA_E_OK;
    PUC_RPTCFG_t puc_rpt_cfg;

    if (!mask.u32) {
        return CA_E_OK;
    }
     __puc_lock();
    puc_rpt_cfg.wrd = AAL_PON_REG_READ (PUC_RPTCFG);
    if (mask.s.numqsets) {
        if (puc_rpt_cfg.bf.numqsets != puc_rpt_config->numqsets) {
            puc_rpt_cfg.bf.numqsets = puc_rpt_config->numqsets;
        }
    }


    if (mask.s.rptpreamblovhd) {
        if (puc_rpt_cfg.bf.rptpreamblovhd != puc_rpt_config->rptpreamblovhd) {
            puc_rpt_cfg.bf.rptpreamblovhd = puc_rpt_config->rptpreamblovhd;
        }
    }

    if (mask.s.rptipgovhd) {
        if (puc_rpt_cfg.bf.rptipgovhd != puc_rpt_config->rptipgovhd) {
            puc_rpt_cfg.bf.rptipgovhd = puc_rpt_config->rptipgovhd;
        }
    }

    if (mask.s.rpt1gfecon) {
        if (puc_rpt_cfg.bf.rpt1gfecon != puc_rpt_config->rpt1gfecon) {
            puc_rpt_cfg.bf.rpt1gfecon = puc_rpt_config->rpt1gfecon;
        }
    }


    if (mask.s.firstqstot) {
        if (puc_rpt_cfg.bf.firstqstot != puc_rpt_config->firstqstot) {
            puc_rpt_cfg.bf.firstqstot = puc_rpt_config->firstqstot;
        }
    }

    if (mask.s.lastqstot) {
        if (puc_rpt_cfg.bf.lastqstot != puc_rpt_config->lastqstot) {
            puc_rpt_cfg.bf.lastqstot = puc_rpt_config->lastqstot;
        }
    }

    if (mask.s.pkgarpt) {
        if (puc_rpt_cfg.bf.pkgarpt != puc_rpt_config->pkgarpt) {
            puc_rpt_cfg.bf.pkgarpt = puc_rpt_config->pkgarpt;
        }
    }

    if (mask.s.aggrth) {
        if (puc_rpt_cfg.bf.aggrth != puc_rpt_config->aggrth) {
            puc_rpt_cfg.bf.aggrth = puc_rpt_config->aggrth;
        }
    }
    AAL_PON_REG_WRITE (puc_rpt_cfg.wrd, PUC_RPTCFG);
     __puc_unlock();
    return ret;
}


ca_status_t aal_puc_bm_stat_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *bm_stat)
{
    ca_status_t ret = CA_E_OK;
    PUC_BMSTS_t bm_sts;
     __puc_lock();
    bm_sts.wrd = AAL_PON_REG_READ (PUC_BMSTS);

    *bm_stat = bm_sts.bf.bpmem_initdone;
     __puc_unlock();
    return ret;
}

ca_status_t aal_puc_bm_stat_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t bm_stat)
{
    ca_status_t ret = CA_E_OK;
    PUC_BMSTS_t bm_sts;
    __puc_lock();
    bm_sts.bf.bpmem_initdone = bm_stat;
    AAL_PON_REG_WRITE (bm_sts.wrd, PUC_BMSTS);
     __puc_lock();
    return ret;
}

ca_status_t aal_puc_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_PUCCFG_t puc_cfg;
    __puc_lock();
    puc_cfg.wrd = AAL_PON_REG_READ (PUC_PUCCFG);

    cfg->crccntl = puc_cfg.bf.crccntl;
    cfg->sllidmode= puc_cfg.bf.sllidmode;
     __puc_unlock();
    return ret;
}

ca_status_t aal_puc_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_cfg_msk_t mask,
    CA_IN aal_puc_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_PUCCFG_t puc_cfg;
    puc_cfg.wrd = 0;

    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    puc_cfg.bf.crccntl = cfg->crccntl ;
    puc_cfg.bf.sllidmode = cfg->sllidmode ;
    AAL_PON_REG_WRITE (puc_cfg.wrd, PUC_PUCCFG);
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_bm_dbg_fb_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bm_dbg_fb_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_BMDBGFBCNT_t  bm_dbg_fb;

    __puc_lock();
    bm_dbg_fb.wrd = AAL_PON_REG_READ (PUC_BMDBGFBCNT);
    cfg->fbpacnt= bm_dbg_fb.bf.fbpacnt;
    cfg->fbpbcnt= bm_dbg_fb.bf.fbpbcnt;
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sts_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_STS_t  puc_sts;
    __puc_lock();

    puc_sts.wrd = AAL_PON_REG_READ (PUC_STS);
    cfg->shp_fifo_overflow = puc_sts.bf.shp_fifo_overflow;
    cfg->shp_fifo_empty = puc_sts.bf.shp_fifo_empty;
    cfg->shp_init_done = puc_sts.bf.shp_init_done;
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_epon_queue_thrshld12_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_epon_que_thrsd12_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_RPTQSCFG12_t rptqscfg12;
    __puc_lock();
    rptqscfg12.wrd = AAL_PON_REG_READ (PUC_RPTQSCFG12);

    cfg->qs1th = rptqscfg12.bf.qs1th;
    cfg->qs2th = rptqscfg12.bf.qs2th;
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_epon_queue_thrshld12_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_epon_que_thrsd12_cfg_msk_t mask,
    CA_IN aal_puc_epon_que_thrsd12_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_RPTQSCFG12_t rptqscfg12;
    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    rptqscfg12.bf.qs1th = cfg->qs1th ;
    rptqscfg12.bf.qs2th = cfg->qs2th ;
    AAL_PON_REG_WRITE (rptqscfg12.wrd, PUC_RPTQSCFG12);
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_epon_queue_thrshld34_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_epon_que_thrsd34_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_RPTQSCFG23_t rptqscfg23;
    __puc_lock();
    rptqscfg23.wrd = AAL_PON_REG_READ (PUC_RPTQSCFG23);

    cfg->qs3th = rptqscfg23.bf.qs3th;
    cfg->qs4th = rptqscfg23.bf.qs4th;
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_epon_queue_thrshld34_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_epon_que_thrsd34_cfg_msk_t mask,
    CA_IN aal_puc_epon_que_thrsd34_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_RPTQSCFG23_t rptqscfg34;
    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    rptqscfg34.bf.qs3th = cfg->qs3th ;
    rptqscfg34.bf.qs4th = cfg->qs4th ;
    AAL_PON_REG_WRITE (rptqscfg34.wrd, PUC_RPTQSCFG23);
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_bmc_length_error_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    ca_status_t ret = CA_E_OK;
    PUC_BMC_LENGTH_ERROR_t bmc_len_error;
    __puc_lock();
    bmc_len_error.wrd = AAL_PON_REG_READ (PUC_BMC_LENGTH_ERROR);

    *cnt = bmc_len_error.bf.cntr;
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_bmc_fifo_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bmc_fifo_sts_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_BMC_FIFO_STS_t  bmc_fifo_sts;
    __puc_lock();
    bmc_fifo_sts.wrd = AAL_PON_REG_READ (PUC_BMC_FIFO_STS);
    cfg->intf_fifo_depth = bmc_fifo_sts.bf.intf_fifo_depth;
    cfg->intf_fifo_overflow = bmc_fifo_sts.bf.intf_fifo_overflow;
    cfg->intf_fifo_front_overflow = bmc_fifo_sts.bf.intf_fifo_front_overflow;
    cfg->intf_fifo_front_depth = bmc_fifo_sts.bf.intf_fifo_front_depth;
    cfg->wqf_data_fifo_depth = bmc_fifo_sts.bf.wqf_data_fifo_depth;
    cfg->buf_link_fifo_depth = bmc_fifo_sts.bf.buf_link_fifo_depth;
    cfg->wqf_cmd_fifo_depth = bmc_fifo_sts.bf.wqf_cmd_fifo_depth;
    cfg->rqf_cmd_fifo_depth = bmc_fifo_sts.bf.rqf_cmd_fifo_depth;
    cfg->bmc_pe = bmc_fifo_sts.bf.bmc_pe;
    __puc_unlock();
    return ret;
}


ca_status_t aal_puc_bmc_omci_pkt_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cnt)
{
    PUC_BMC_CONTROL_PKT_CNTR_lnk_t bmc_omci_cnt;

    AAL_ASSERT_RET(cnt != NULL, CA_E_PARAM);
    __puc_lock();
    bmc_omci_cnt.wrd = AAL_PON_REG_READ(PUC_BMC_CONTROL_PKT_CNTR_lnk);

    *cnt = bmc_omci_cnt.bf.cntr;
    __puc_unlock();

    return CA_E_OK;
}
ca_status_t aal_puc_glb_lnk_type_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *lnk_type)
{
    PUC_GLOBAL_LNK_TYPE_t glb_lnk_type;

    AAL_ASSERT_RET(lnk_type != NULL, CA_E_PARAM);
    __puc_lock();
    glb_lnk_type.wrd = AAL_PON_REG_READ (PUC_GLOBAL_LNK_TYPE);
    *lnk_type = glb_lnk_type.bf.type;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_glb_lnk_type_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t lnk_type)
{
    PUC_GLOBAL_LNK_TYPE_t glb_lnk_type;
    __puc_lock();
    glb_lnk_type.bf.type = lnk_type;
    AAL_PON_REG_WRITE (glb_lnk_type.wrd, PUC_GLOBAL_LNK_TYPE);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_glb_mac_type_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *mac_type)
{
    PUC_GLOBAL_MAC_TYPE_t glb_mac_type;

    AAL_ASSERT_RET(mac_type != NULL, CA_E_PARAM);
    __puc_lock();
    glb_mac_type.wrd = AAL_PON_REG_READ (PUC_GLOBAL_MAC_TYPE);
    *mac_type = glb_mac_type.bf.type;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_glb_mac_type_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t mac_type)
{
    PUC_GLOBAL_MAC_TYPE_t glb_mac_type;
    __puc_lock();

    glb_mac_type.bf.type = mac_type;

    AAL_PON_REG_WRITE (glb_mac_type.wrd, PUC_GLOBAL_MAC_TYPE);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_glb_da_sa_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_omci_mac_t *da_sa)
{
    ca_uint32_t dasa0 = 0;
    ca_uint32_t dasa1 = 0;
    ca_uint32_t dasa2 = 0;

    AAL_ASSERT_RET(da_sa != NULL, CA_E_PARAM);
    __puc_lock();

    dasa0 = AAL_PON_REG_READ (PUC_GLOBAL_DA_SA0);
    dasa1 = AAL_PON_REG_READ (PUC_GLOBAL_DA_SA1);
    dasa2 = AAL_PON_REG_READ (PUC_GLOBAL_DA_SA2);

    da_sa->da[0] = (dasa2 >> 24) & 0xFF;
    da_sa->da[1] = (dasa2 >> 16) & 0xFF;
    da_sa->da[2] = (dasa2 >> 8) & 0xFF;
    da_sa->da[3] = dasa2 & 0xFF;
    da_sa->da[4] = (dasa1 >> 24) & 0xFF;
    da_sa->da[5] = (dasa1 >> 16) & 0xFF;


    da_sa->sa[0] = (dasa1 >> 8) & 0xFF;
    da_sa->sa[1] = dasa1 & 0xFF;
    da_sa->sa[2] = (dasa0 >> 24) & 0xFF;
    da_sa->sa[3] = (dasa0 >> 16) & 0xFF;
    da_sa->sa[4] = (dasa0 >> 8) & 0xFF;
    da_sa->sa[5] = dasa0 & 0xFF;
    __puc_unlock();

    return CA_E_OK;

}
ca_status_t aal_puc_glb_da_sa_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_omci_mac_t da_sa)
{
    ca_uint32_t dasa0, dasa1, dasa2;

    __puc_lock();

    dasa0 = da_sa.sa[5] | (da_sa.sa[4] << 8) | (da_sa.sa[3] << 16) | (da_sa.sa[2] << 24);
    dasa1 = da_sa.sa[1] | (da_sa.sa[0] << 8) | (da_sa.da[5] << 16) | (da_sa.da[4] << 24);
    dasa2 = da_sa.da[3] | (da_sa.da[2] << 8) | (da_sa.da[1] << 16) | (da_sa.da[0] << 24);

    AAL_PON_REG_WRITE(dasa0, PUC_GLOBAL_DA_SA0);
    AAL_PON_REG_WRITE(dasa1, PUC_GLOBAL_DA_SA1);
    AAL_PON_REG_WRITE(dasa2, PUC_GLOBAL_DA_SA2);
    __puc_unlock();

    return CA_E_OK;
}

ca_status_t aal_puc_us_omci_hdr_a_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_us_omci_hdr_a_cfg_t *cfg)
{
    PUC_US_OMCI_HDR_A_CFG_t us_omci_hdr_a_cfg;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    //__puc_lock();
    us_omci_hdr_a_cfg.wrd = AAL_PON_REG_READ (PUC_US_OMCI_HDR_A_CFG);
    cfg->gemid_replacement = us_omci_hdr_a_cfg.bf.gemid_replacement;
    cfg->cos_replacement= us_omci_hdr_a_cfg.bf.cos_replacement;
    cfg->tcont_replacement = us_omci_hdr_a_cfg.bf.tcont_replacement;
    cfg->enable_data_pkt_replacement = us_omci_hdr_a_cfg.bf.enable_data_pkt_replacement;
    cfg->enable_replacement = us_omci_hdr_a_cfg.bf.enable_replacement;
    //__puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_us_omci_hdr_a_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_us_omci_hdr_a_cfg_msk_t mask,
    CA_IN aal_puc_us_omci_hdr_a_cfg_t *cfg)
{
    PUC_US_OMCI_HDR_A_CFG_t us_omci_hdr_a_cfg;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    us_omci_hdr_a_cfg.wrd = AAL_PON_REG_READ (PUC_US_OMCI_HDR_A_CFG);
    if (mask.s.gemid_replacement)
        us_omci_hdr_a_cfg.bf.gemid_replacement = cfg->gemid_replacement ;
    if (mask.s.cos_replacement)
        us_omci_hdr_a_cfg.bf.cos_replacement = cfg->cos_replacement ;
    if (mask.s.tcont_replacement)
        us_omci_hdr_a_cfg.bf.tcont_replacement = cfg->tcont_replacement ;
    if (mask.s.enable_data_pkt_replacement)
        us_omci_hdr_a_cfg.bf.enable_data_pkt_replacement = cfg->enable_data_pkt_replacement ;
    if (mask.s.enable_replacement)
        us_omci_hdr_a_cfg.bf.enable_replacement = cfg->enable_replacement ;

    AAL_PON_REG_WRITE (us_omci_hdr_a_cfg.wrd, PUC_US_OMCI_HDR_A_CFG);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_us_omci_hp_hdr_a_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_us_omci_hp_hdr_a_cfg_t *cfg)
{
    PUC_US_OMCI_HP_HDR_A_CFG_t us_omci_hp_hdr_a_cfg;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    //__puc_lock();
    us_omci_hp_hdr_a_cfg.wrd = AAL_PON_REG_READ (PUC_US_OMCI_HP_HDR_A_CFG);
    cfg->gemid_replacement = us_omci_hp_hdr_a_cfg.bf.gemid_replacement;
    cfg->cos_replacement= us_omci_hp_hdr_a_cfg.bf.cos_replacement;
    cfg->tcont_replacement = us_omci_hp_hdr_a_cfg.bf.tcont_replacement;
    //__puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_us_omci_hp_hdr_a_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_us_omci_hp_hdr_a_cfg_msk_t mask,
    CA_IN aal_puc_us_omci_hp_hdr_a_cfg_t *cfg)
{
    PUC_US_OMCI_HP_HDR_A_CFG_t us_omci_hp_hdr_a_cfg;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    us_omci_hp_hdr_a_cfg.wrd = AAL_PON_REG_READ (PUC_US_OMCI_HP_HDR_A_CFG);

    if (mask.s.gemid_replacement)
        us_omci_hp_hdr_a_cfg.bf.gemid_replacement = cfg->gemid_replacement ;
    if (mask.s.cos_replacement)
        us_omci_hp_hdr_a_cfg.bf.cos_replacement = cfg->cos_replacement ;
    if (mask.s.tcont_replacement)
        us_omci_hp_hdr_a_cfg.bf.tcont_replacement = cfg->tcont_replacement ;

    AAL_PON_REG_WRITE (us_omci_hp_hdr_a_cfg.wrd, PUC_US_OMCI_HP_HDR_A_CFG);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_glb_ploam_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_glb_plom_cfg_t *cfg)
{
    PUC_GLOBAL_PLOAM_CFG_t glb_plm_cfg;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    glb_plm_cfg.wrd = AAL_PON_REG_READ (PUC_GLOBAL_PLOAM_CFG);
    cfg->us_ploam_msg_gen_en = glb_plm_cfg.bf.us_ploam_msg_gen_en;
    cfg->ds_ploam_pkt_gen_en= glb_plm_cfg.bf.ds_ploam_pkt_gen_en;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_glb_ploam_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_glb_plom_cfg_msk_t mask,
    CA_IN aal_puc_glb_plom_cfg_t *cfg)
{
    PUC_GLOBAL_PLOAM_CFG_t glb_plm_cfg;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    if (mask.s.us_ploam_msg_gen_en)
        glb_plm_cfg.bf.us_ploam_msg_gen_en = cfg->us_ploam_msg_gen_en ;
    if (mask.s.ds_ploam_pkt_gen_en)
        glb_plm_cfg.bf.ds_ploam_pkt_gen_en = cfg->ds_ploam_pkt_gen_en ;

    AAL_PON_REG_WRITE (glb_plm_cfg.wrd, PUC_GLOBAL_PLOAM_CFG);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_ds_ploam_hdra_cfg1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_DS_PLOAM_HDR_A_CFG1_t ds_plm_hdra;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    //__puc_lock();
    ds_plm_hdra.wrd = AAL_PON_REG_READ (PUC_DS_PLOAM_HDR_A_CFG1);
    *cfg = ds_plm_hdra.bf.data1;
    //__puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_ds_ploam_hdra_cfg1_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t cfg)
{
    PUC_DS_PLOAM_HDR_A_CFG1_t ds_plm_hdra;
    __puc_lock();
       ds_plm_hdra.bf.data1 = cfg;

    AAL_PON_REG_WRITE (ds_plm_hdra.wrd, PUC_DS_PLOAM_HDR_A_CFG1);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_ds_ploam_hdra_cfg0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_DS_PLOAM_HDR_A_CFG0_t ds_plm_hdra;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    //__puc_lock();
    ds_plm_hdra.wrd = AAL_PON_REG_READ (PUC_DS_PLOAM_HDR_A_CFG0);
    *cfg = ds_plm_hdra.bf.data0;
    //__puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_ds_ploam_hdra_cfg0_set(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_DS_PLOAM_HDR_A_CFG0_t ds_plm_hdra;
    __puc_lock();
    ds_plm_hdra.bf.data0 = cfg;
    AAL_PON_REG_WRITE (ds_plm_hdra.wrd, PUC_DS_PLOAM_HDR_A_CFG0);
    __puc_unlock();
    return ret;
}
ca_status_t aal_puc_bmc_ctl_pkt_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bmc_ctl_pkt_cnt_t *cfg)
{

    PUC_BMC_CONTROL_PKT_CNTR_lnk_t lnk_cnt;
    PUC_BMC_CONTROL_PKT_CNTR_mac_t mac_cnt;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    lnk_cnt.wrd = AAL_PON_REG_READ (PUC_BMC_CONTROL_PKT_CNTR_lnk);
    mac_cnt.wrd = AAL_PON_REG_READ (PUC_BMC_CONTROL_PKT_CNTR_mac);

    cfg->link_layer = lnk_cnt.bf.cntr;
    cfg->mac_layer  = mac_cnt.bf.cntr;
    __puc_unlock();

    return CA_E_OK;
}
ca_status_t aal_puc_bmc_pkt_hdr1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_BMC_PKT_HDR1_t pkt_hdr1;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    pkt_hdr1.wrd = AAL_PON_REG_READ (PUC_BMC_PKT_HDR1);
    *cfg = pkt_hdr1.bf.hdr_a1;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_bmc_pkt_hdr0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_BMC_PKT_HDR0_t pkt_hdr0;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    pkt_hdr0.wrd = AAL_PON_REG_READ (PUC_BMC_PKT_HDR0);
    *cfg = pkt_hdr0.bf.hdr_a0;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_bmc_pkt_hdr_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_heada_t *heada)
{

    AAL_ASSERT_RET(heada != NULL, CA_E_PARAM);

    __puc_lock();
    heada->pkt_hdr0 = AAL_PON_REG_READ (PUC_BMC_PKT_HDR0);
    heada->pkt_hdr1 = AAL_PON_REG_READ (PUC_BMC_PKT_HDR1);

    heada->cpu_flag         = (heada->pkt_hdr1>>31)&1;
    heada->deep_q           = (heada->pkt_hdr1>>30)&1;
    heada->policy_ena       = (heada->pkt_hdr1>>16)&3;
    heada->policy_group_id  = (heada->pkt_hdr1>>27)&7;
    heada->policy_id        = (heada->pkt_hdr1>>18)&0x1ff;
    heada->marked           = (heada->pkt_hdr1>>15)&1;
    heada->mirror           = (heada->pkt_hdr1>>14)&1;
    heada->no_drop          = (heada->pkt_hdr1>>13)&1;
    heada->rx_packet_type   = (heada->pkt_hdr1>>11)&3;
    heada->drop_code        = (heada->pkt_hdr1>>8)&7;
    heada->mc_group_id      = (heada->pkt_hdr1>>0)&0xff;
    heada->header_type      = (heada->pkt_hdr0>>30)&3;
    heada->fe_bypass        = (heada->pkt_hdr0>>29)&1;
    heada->packet_size      = (heada->pkt_hdr0>>15)&0x3fff;
    heada->cos              = (heada->pkt_hdr0>>0)&0x7;
    heada->logic_spid       = (heada->pkt_hdr0>>9)&0x3f;
    heada->logic_dpid       = (heada->pkt_hdr0>>3)&0x3f;

    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_bmc_mtu_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint16_t *cfg)
{
    PUC_BMC_MTU_t bmc_mtu;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    bmc_mtu.wrd = AAL_PON_REG_READ (PUC_BMC_MTU);
    *cfg = bmc_mtu.bf.size;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_bmc_mtu_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint16_t *cfg)
{
    PUC_BMC_MTU_t bmc_mtu;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    bmc_mtu.bf.size = *cfg;
    AAL_PON_REG_WRITE (bmc_mtu.wrd, PUC_BMC_MTU);
    __puc_unlock();
    return CA_E_OK;
}
    
ca_status_t aal_puc_voq_valid_set(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_uint16_t   voq_id,
    CA_IN ca_boolean_t  valid)
{
    ca_uint8_t voq_idx = 0;
    PUC_valid_voq0_t  valid_voq0;
    ca_uint32_t reg_addr ;
    ca_uint32_t bit_pos = 0;
    if(voq_id > AAL_PUC_QUEUE_ID_MAX)
        return CA_E_PARAM;
    
    voq_idx = voq_id/32;
    bit_pos = voq_id%32;
 
    reg_addr = PUC_valid_voq0 - (voq_idx * 4);
    valid_voq0.wrd = AAL_PON_REG_READ(reg_addr);
    if(valid){
        AAL_PON_REG_WRITE((valid_voq0.wrd|( 1 << bit_pos )), reg_addr);
    }else{
        valid_voq0.wrd &= ~( 1 << bit_pos );
        AAL_PON_REG_WRITE(valid_voq0.wrd, reg_addr);
    }
    return CA_E_OK;
}

ca_status_t aal_puc_voq_valid_get(
    CA_IN ca_device_id_t   device_id,
    CA_IN ca_uint16_t   voq_id,
    CA_OUT ca_boolean_t *valid)
{
    ca_uint8_t voq_idx = 0;
    PUC_valid_voq0_t  valid_voq0;
    ca_uint32_t reg_addr ;
    ca_uint32_t bit_pos = 0;
    if(voq_id > AAL_PUC_QUEUE_ID_MAX)
        return CA_E_PARAM;
    
    voq_idx = voq_id/32;
    bit_pos = voq_id%32;
 
    reg_addr = PUC_valid_voq0 - (voq_idx * 4);
    valid_voq0.wrd = AAL_PON_REG_READ(reg_addr);

    *valid = (valid_voq0.wrd &( 1 << bit_pos ))?1:0;
   
    return CA_E_OK;
}
    
ca_status_t aal_puc_voq_tbc_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_IN aal_puc_voq_tbc_mem_cfg_msk_t mask,
    CA_IN aal_puc_voq_tbc_mem_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;

    PUC_VOQ_TBC_MEM_DATA0_t tbc_mem_data0;
    PUC_VOQ_TBC_MEM_DATA1_t tbc_mem_data1;

    if (queue_id > AAL_PUC_MAX_QUEUE_NUM) {
        return CA_E_PARAM;
    }

    if (!mask.u32) {
        return CA_E_OK;
    }

    __puc_lock();
    AAL_PON_REG_INDIRECT_READ(queue_id, PUC_VOQ_TBC_MEM_ACCESS);

    tbc_mem_data0.wrd = AAL_PON_REG_READ (PUC_VOQ_TBC_MEM_DATA0);
    tbc_mem_data1.wrd = AAL_PON_REG_READ (PUC_VOQ_TBC_MEM_DATA1);
    if (mask.s.rate_k) {
        if (tbc_mem_data0.bf.rate_k != cfg->rate_k)
            tbc_mem_data0.bf.rate_k = cfg->rate_k;
    }
    if (mask.s.rate_m) {
        if (tbc_mem_data0.bf.rate_m != cfg->rate_m)
            tbc_mem_data0.bf.rate_m = cfg->rate_m;
    }
    if (mask.s.bs) {
        if (tbc_mem_data0.bf.bs != (cfg->bs & 0x3f))
            tbc_mem_data0.bf.bs = (cfg->bs & 0x3f);

        if (tbc_mem_data1.bf.bs != (cfg->bs >> 6))
            tbc_mem_data1.bf.bs = (cfg->bs >> 6);
    }

    if (mask.s.enb) {
        if (tbc_mem_data1.bf.enb != cfg->enb)
            tbc_mem_data1.bf.enb = cfg->enb;
    }
    if (mask.s.mode) {
        if (tbc_mem_data1.bf.mode != cfg->mode)
            tbc_mem_data1.bf.mode = cfg->mode;
    }

    if (mask.s.tbc) {
        if (tbc_mem_data1.bf.tbc != cfg->tbc)
            tbc_mem_data1.bf.tbc = cfg->tbc;
    }

    if (mask.s.pkt_mode_class_sel) {
        if (tbc_mem_data1.bf.pkt_mode_class_sel != cfg->pkt_mode_class_sel)
            tbc_mem_data1.bf.pkt_mode_class_sel = cfg->pkt_mode_class_sel;
    }

    AAL_PON_REG_WRITE (tbc_mem_data0.wrd, PUC_VOQ_TBC_MEM_DATA0);
    AAL_PON_REG_WRITE (tbc_mem_data1.wrd, PUC_VOQ_TBC_MEM_DATA1);

    AAL_PON_REG_INDIRECT_WRITE(queue_id, PUC_VOQ_TBC_MEM_ACCESS);
    __puc_unlock();

    return ret;
}

ca_status_t aal_puc_voq_tbc_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_OUT aal_puc_voq_tbc_mem_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_VOQ_TBC_MEM_DATA0_t tbc_mem_data0;
    PUC_VOQ_TBC_MEM_DATA1_t tbc_mem_data1;

    if (queue_id > AAL_PUC_MAX_QUEUE_NUM) {
        return CA_E_PARAM;
    }

     __puc_lock();
    AAL_PON_REG_INDIRECT_READ(queue_id, PUC_VOQ_TBC_MEM_ACCESS);

    tbc_mem_data0.wrd = AAL_PON_REG_READ (PUC_VOQ_TBC_MEM_DATA0);
    tbc_mem_data1.wrd = AAL_PON_REG_READ (PUC_VOQ_TBC_MEM_DATA1);
    cfg->rate_k = tbc_mem_data0.bf.rate_k;
    cfg->rate_m = tbc_mem_data0.bf.rate_m;
    cfg->bs = tbc_mem_data0.bf.bs | (tbc_mem_data1.bf.bs << 6);
    cfg->enb = tbc_mem_data1.bf.enb;
    cfg->mode = tbc_mem_data1.bf.mode ;
    cfg->tbc = tbc_mem_data1.bf.tbc;
    cfg->pkt_mode_class_sel = tbc_mem_data1.bf.pkt_mode_class_sel ;
    __puc_unlock();

    return ret;
}

ca_status_t aal_puc_rl_voq_tbc_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_IN aal_puc_voq_tbc_mem_cfg_msk_t mask,
    CA_IN aal_puc_voq_tbc_mem_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;

    PUC_RL_VOQ_TBC_MEM_DATA0_t tbc_mem_data0;
    PUC_RL_VOQ_TBC_MEM_DATA1_t tbc_mem_data1;

    if (queue_id > AAL_PUC_MAX_QUEUE_NUM) {
        return CA_E_PARAM;
    }

    if (!mask.u32) {
        return CA_E_OK;
    }

    __puc_lock();
    AAL_PON_REG_INDIRECT_READ(queue_id, PUC_RL_VOQ_TBC_MEM_ACCESS);

    tbc_mem_data0.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_TBC_MEM_DATA0);
    tbc_mem_data1.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_TBC_MEM_DATA1);
    if (mask.s.rate_k) {
        if (tbc_mem_data0.bf.rate_k != cfg->rate_k)
            tbc_mem_data0.bf.rate_k = cfg->rate_k;
    }
    if (mask.s.rate_m) {
        if (tbc_mem_data0.bf.rate_m != cfg->rate_m)
            tbc_mem_data0.bf.rate_m = cfg->rate_m;
    }
    if (mask.s.bs) {
        if (tbc_mem_data0.bf.bs != (cfg->bs & 0x3f))
            tbc_mem_data0.bf.bs = (cfg->bs & 0x3f);

        if (tbc_mem_data1.bf.bs != (cfg->bs >> 6))
            tbc_mem_data1.bf.bs = (cfg->bs >> 6);
    }

    if (mask.s.enb) {
        if (tbc_mem_data1.bf.enb != cfg->enb)
            tbc_mem_data1.bf.enb = cfg->enb;
    }
    if (mask.s.mode) {
        if (tbc_mem_data1.bf.mode != cfg->mode)
            tbc_mem_data1.bf.mode = cfg->mode;
    }

    if (mask.s.tbc) {
        if (tbc_mem_data1.bf.tbc != cfg->tbc)
            tbc_mem_data1.bf.tbc = cfg->tbc;
    }

    if (mask.s.pkt_mode_class_sel) {
        if (tbc_mem_data1.bf.pkt_mode_class_sel != cfg->pkt_mode_class_sel)
            tbc_mem_data1.bf.pkt_mode_class_sel = cfg->pkt_mode_class_sel;
    }

    AAL_PON_REG_WRITE (tbc_mem_data0.wrd, PUC_RL_VOQ_TBC_MEM_DATA0);
    AAL_PON_REG_WRITE (tbc_mem_data1.wrd, PUC_RL_VOQ_TBC_MEM_DATA1);

    AAL_PON_REG_INDIRECT_WRITE(queue_id, PUC_RL_VOQ_TBC_MEM_ACCESS);
    __puc_unlock();

    return ret;
}
  
ca_status_t aal_puc_rl_voq_tbc_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_OUT aal_puc_voq_tbc_mem_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    PUC_RL_VOQ_TBC_MEM_DATA0_t tbc_mem_data0;
    PUC_RL_VOQ_TBC_MEM_DATA1_t tbc_mem_data1;

    if (queue_id > AAL_PUC_MAX_QUEUE_NUM) {
        return CA_E_PARAM;
    }

     __puc_lock();
    AAL_PON_REG_INDIRECT_READ(queue_id, PUC_RL_VOQ_TBC_MEM_ACCESS);

    tbc_mem_data0.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_TBC_MEM_DATA0);
    tbc_mem_data1.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_TBC_MEM_DATA1);
    cfg->rate_k = tbc_mem_data0.bf.rate_k;
    cfg->rate_m = tbc_mem_data0.bf.rate_m;
    cfg->bs = tbc_mem_data0.bf.bs | (tbc_mem_data1.bf.bs << 6);
    cfg->enb = tbc_mem_data1.bf.enb;
    cfg->mode = tbc_mem_data1.bf.mode ;
    cfg->tbc = tbc_mem_data1.bf.tbc;
    cfg->pkt_mode_class_sel = tbc_mem_data1.bf.pkt_mode_class_sel ;
    __puc_unlock();

    return ret;
}

ca_status_t aal_puc_rl_voq_dcnt_mem_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_IN aal_puc_voq_dcnt_mem_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;

    PUC_RL_VOQ_DCNT_MEM_DATA0_t dcnt_mem_data0;
    PUC_RL_VOQ_DCNT_MEM_DATA1_t dcnt_mem_data1;
    PUC_RL_VOQ_DCNT_MEM_DATA2_t dcnt_mem_data2;

    if (queue_id > AAL_PUC_MAX_QUEUE_NUM) {
        return CA_E_PARAM;
    }

    __puc_lock();
    AAL_PON_REG_INDIRECT_READ(queue_id, PUC_RL_VOQ_DCNT_MEM_ACCESS);

    dcnt_mem_data0.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_DCNT_MEM_DATA0);
    dcnt_mem_data1.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_DCNT_MEM_DATA1);
    dcnt_mem_data2.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_DCNT_MEM_DATA2);
    
    dcnt_mem_data0.bf.pcnt = cfg->pcnt;
    dcnt_mem_data1.bf.bcnt = (ca_uint32_t)cfg->bcnt;
    dcnt_mem_data2.bf.bcnt = (cfg->bcnt>>32)&0xF;
    
    AAL_PON_REG_WRITE (dcnt_mem_data0.wrd, PUC_RL_VOQ_DCNT_MEM_DATA0);
    AAL_PON_REG_WRITE (dcnt_mem_data1.wrd, PUC_RL_VOQ_DCNT_MEM_DATA1);
    AAL_PON_REG_WRITE (dcnt_mem_data2.wrd, PUC_RL_VOQ_DCNT_MEM_DATA2);

    AAL_PON_REG_INDIRECT_WRITE(queue_id, PUC_RL_VOQ_DCNT_MEM_ACCESS);
    __puc_unlock();

    return ret;
}
  
ca_status_t aal_puc_rl_voq_dcnt_mem_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t queue_id,
    CA_OUT aal_puc_voq_dcnt_mem_cfg_t *cfg)
{
    ca_status_t ret = CA_E_OK;
    ca_uint64_t tmp = 0;

    PUC_RL_VOQ_DCNT_MEM_DATA0_t dcnt_mem_data0;
    PUC_RL_VOQ_DCNT_MEM_DATA1_t dcnt_mem_data1;
    PUC_RL_VOQ_DCNT_MEM_DATA2_t dcnt_mem_data2;

    if (queue_id > AAL_PUC_MAX_QUEUE_NUM) {
        return CA_E_PARAM;
    }

    __puc_lock();
    AAL_PON_REG_INDIRECT_READ(queue_id, PUC_RL_VOQ_DCNT_MEM_ACCESS);

    dcnt_mem_data0.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_DCNT_MEM_DATA0);
    dcnt_mem_data1.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_DCNT_MEM_DATA1);
    dcnt_mem_data2.wrd = AAL_PON_REG_READ (PUC_RL_VOQ_DCNT_MEM_DATA2);
    tmp = dcnt_mem_data2.bf.bcnt;
  
    cfg->pcnt = dcnt_mem_data0.bf.pcnt;
    cfg->bcnt = (tmp<<32)|dcnt_mem_data1.bf.bcnt;
    
    __puc_unlock();
    return ret;
}

ca_status_t aal_puc_agr_shp_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_agr_shp_ctrl_cfg_t *cfg)
{
    PUC_AGR_SHP_CTRL0_t agr_shp_ctrl0;
    PUC_AGR_SHP_CTRL1_t agr_shp_ctrl1;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    agr_shp_ctrl0.wrd = AAL_PON_REG_READ (PUC_AGR_SHP_CTRL0);
    agr_shp_ctrl1.wrd = AAL_PON_REG_READ (PUC_AGR_SHP_CTRL1);
    cfg->rate_m = agr_shp_ctrl0.bf.rate_m;
    cfg->rate_k = agr_shp_ctrl0.bf.rate_k;
    cfg->cbs = agr_shp_ctrl1.bf.cbs;
    cfg->enb = agr_shp_ctrl1.bf.enb;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_agr_shp_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_agr_shp_ctrl_cfg_t *cfg)
{
    PUC_AGR_SHP_CTRL0_t agr_shp_ctrl0;
    PUC_AGR_SHP_CTRL1_t agr_shp_ctrl1;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    agr_shp_ctrl0.wrd = AAL_PON_REG_READ (PUC_AGR_SHP_CTRL0);
    agr_shp_ctrl1.wrd = AAL_PON_REG_READ (PUC_AGR_SHP_CTRL1);
    agr_shp_ctrl0.bf.rate_m = cfg->rate_m  ;
    agr_shp_ctrl0.bf.rate_k = cfg->rate_k  ;
    agr_shp_ctrl1.bf.cbs = cfg->cbs ;
    agr_shp_ctrl1.bf.enb = cfg->enb ;
    AAL_PON_REG_WRITE (agr_shp_ctrl0.wrd, PUC_AGR_SHP_CTRL0);
    AAL_PON_REG_WRITE (agr_shp_ctrl1.wrd, PUC_AGR_SHP_CTRL1);
    __puc_unlock();
    return CA_E_OK;
}
        
ca_status_t aal_puc_agr_shp_sts_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_AGR_SHP_STS_t agr_shp_sts;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    agr_shp_sts.wrd = AAL_PON_REG_READ (PUC_AGR_SHP_STS);
    *cfg = agr_shp_sts.bf.tbc;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_tb_ctrl_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_tb_ctrl_cfg_t *cfg)
{
    PUC_TB_CTRL_t tb_ctrl;
    PUC_TB_CTRL1_t tb_ctrl1;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    tb_ctrl.wrd = AAL_PON_REG_READ (PUC_TB_CTRL);
    tb_ctrl1.wrd = AAL_PON_REG_READ (PUC_TB_CTRL1);
    cfg->last_upd_cyc = tb_ctrl.bf.last_upd_cyc;
    cfg->cyc_cnt_lmt = tb_ctrl.bf.cyc_cnt_lmt;
    cfg->upd_cnt_lmt = tb_ctrl.bf.upd_cnt_lmt;
    cfg->k_cnt_lmt = tb_ctrl1.bf.k_cnt_lmt;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_tb_ctrl_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_tb_ctrl_cfg_t *cfg)
{
    PUC_TB_CTRL_t tb_ctrl;
    PUC_TB_CTRL1_t tb_ctrl1;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    tb_ctrl.wrd = AAL_PON_REG_READ (PUC_TB_CTRL);
    tb_ctrl1.wrd = AAL_PON_REG_READ (PUC_TB_CTRL1);
    tb_ctrl.bf.last_upd_cyc = cfg->last_upd_cyc  ;
    tb_ctrl.bf.cyc_cnt_lmt = cfg->cyc_cnt_lmt  ;
    tb_ctrl.bf.upd_cnt_lmt = cfg->upd_cnt_lmt ;
    tb_ctrl1.bf.k_cnt_lmt = cfg->k_cnt_lmt ;
    AAL_PON_REG_WRITE (tb_ctrl.wrd, PUC_TB_CTRL);
    AAL_PON_REG_WRITE (tb_ctrl1.wrd, PUC_TB_CTRL1);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc1_0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc1_0_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc1_0);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    cfg->test_cfg1 = sram_test_ctrl.bf.test_cfg1;
    cfg->test_cfg2 = sram_test_ctrl.bf.test_cfg2;
    cfg->test_cfg3 = sram_test_ctrl.bf.test_cfg3;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc1_0_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc1_0_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc1_0);
    
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    sram_test_ctrl.bf.test_cfg1 = cfg->test_cfg1  ;
    sram_test_ctrl.bf.test_cfg2 = cfg->test_cfg2 ;
    sram_test_ctrl.bf.test_cfg3 = cfg->test_cfg3 ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc1_0);
       __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc1_1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc1_1_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc1_1);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    cfg->test_cfg1 = sram_test_ctrl.bf.test_cfg1;
    cfg->test_cfg2 = sram_test_ctrl.bf.test_cfg2;
    cfg->test_cfg3 = sram_test_ctrl.bf.test_cfg3;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc1_1_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc1_1_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc1_1);
    
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    sram_test_ctrl.bf.test_cfg1 = cfg->test_cfg1  ;
    sram_test_ctrl.bf.test_cfg2 = cfg->test_cfg2 ;
    sram_test_ctrl.bf.test_cfg3 = cfg->test_cfg3 ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc1_1);
       __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc1_2_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc1_2_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc1_2);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    cfg->test_cfg1 = sram_test_ctrl.bf.test_cfg1;
    
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc1_2_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc1_2_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc1_2);  
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    sram_test_ctrl.bf.test_cfg1 = cfg->test_cfg1  ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc1_2);
       __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc1_2p_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc1_2p_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc1_2p);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    cfg->test_cfg1 = sram_test_ctrl.bf.test_cfg1;
    
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc1_2p_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc1_2p_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc1_2p);  
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    sram_test_ctrl.bf.test_cfg1 = cfg->test_cfg1  ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc1_2p);
       __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc2_0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc2_0_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc2_0);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    cfg->test_cfg1 = sram_test_ctrl.bf.test_cfg1;
    cfg->test_cfg2 = sram_test_ctrl.bf.test_cfg2;
    cfg->test_cfg3 = sram_test_ctrl.bf.test_cfg3;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc2_0_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc2_0_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc2_0);
    
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    sram_test_ctrl.bf.test_cfg1 = cfg->test_cfg1  ;
    sram_test_ctrl.bf.test_cfg2 = cfg->test_cfg2 ;
    sram_test_ctrl.bf.test_cfg3 = cfg->test_cfg3 ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc2_0);
       __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc2_1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc2_1_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc2_1);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    cfg->test_cfg1 = sram_test_ctrl.bf.test_cfg1;
    cfg->test_cfg2 = sram_test_ctrl.bf.test_cfg2;
    cfg->test_cfg3 = sram_test_ctrl.bf.test_cfg3;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc2_1_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc2_1_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc2_1);
    
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    sram_test_ctrl.bf.test_cfg1 = cfg->test_cfg1  ;
    sram_test_ctrl.bf.test_cfg2 = cfg->test_cfg2 ;
    sram_test_ctrl.bf.test_cfg3 = cfg->test_cfg3 ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc2_1);
       __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc2b_0_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc2b_0_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc2b_0);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    cfg->test_cfg1 = sram_test_ctrl.bf.test_cfg1;
    cfg->test_cfg2 = sram_test_ctrl.bf.test_cfg2;
    cfg->test_cfg3 = sram_test_ctrl.bf.test_cfg3;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc2b_0_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc2b_0_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc2b_0);
    
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    sram_test_ctrl.bf.test_cfg1 = cfg->test_cfg1  ;
    sram_test_ctrl.bf.test_cfg2 = cfg->test_cfg2 ;
    sram_test_ctrl.bf.test_cfg3 = cfg->test_cfg3 ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc2b_0);
       __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc2b_1_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc2b_1_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc2b_1);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    cfg->test_cfg1 = sram_test_ctrl.bf.test_cfg1;
    cfg->test_cfg2 = sram_test_ctrl.bf.test_cfg2;
    cfg->test_cfg3 = sram_test_ctrl.bf.test_cfg3;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc2b_1_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc2b_1_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc2b_1);
    
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    sram_test_ctrl.bf.test_cfg1 = cfg->test_cfg1  ;
    sram_test_ctrl.bf.test_cfg2 = cfg->test_cfg2 ;
    sram_test_ctrl.bf.test_cfg3 = cfg->test_cfg3 ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc2b_1);
       __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc3_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc3_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc3);
    
    cfg->test_cfg0 = sram_test_ctrl.bf.test_cfg0;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_sram_test_ctrl_puc3_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_sram_test_ctrl_cfg_t *cfg)
{
    PUC_SRAM_TEST_CONTROL_puc3_t sram_test_ctrl;
   
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    sram_test_ctrl.wrd = AAL_PON_REG_READ (PUC_SRAM_TEST_CONTROL_puc3);  
    sram_test_ctrl.bf.test_cfg0 = cfg->test_cfg0  ;
    AAL_PON_REG_WRITE (sram_test_ctrl.wrd, PUC_SRAM_TEST_CONTROL_puc3);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_pvt_cfg_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_pvt_cfg_t *cfg)
{
    PUC_PVTCFG_t pvt_cfg;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    pvt_cfg.wrd = AAL_PON_REG_READ (PUC_PVTCFG);
    cfg->pkgbrptceil = pvt_cfg.bf.pkgbrptceil;
    cfg->pkgboptschen= pvt_cfg.bf.pkgboptschen;
    cfg->freebufnibplowth = pvt_cfg.bf.freebufnibplowth;
    cfg->freebufnibpen= pvt_cfg.bf.freebufnibpen;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_pvt_cfg_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_pvt_cfg_msk_t mask,
    CA_IN aal_puc_pvt_cfg_t *cfg)
{
    PUC_PVTCFG_t pvt_cfg;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    pvt_cfg.bf.pkgbrptceil = cfg->pkgbrptceil ;
    pvt_cfg.bf.pkgboptschen = cfg->pkgboptschen ;
    pvt_cfg.bf.freebufnibplowth = cfg->freebufnibplowth ;
    pvt_cfg.bf.freebufnibpen = cfg->freebufnibpen ;

    AAL_PON_REG_WRITE (pvt_cfg.wrd, PUC_PVTCFG);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_sel1500_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_VOQBUFLIMITSEL1500_t buf_sel;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    buf_sel.wrd = AAL_PON_REG_READ (PUC_VOQBUFLIMITSEL1500);

    *cfg = buf_sel.bf.buflimitsel;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_sel1500_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg)
{
    PUC_VOQBUFLIMITSEL1500_t buf_sel;
    __puc_lock();

    buf_sel.bf.buflimitsel = cfg;
    AAL_PON_REG_WRITE (buf_sel.wrd, PUC_VOQBUFLIMITSEL1500);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_sel3116_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_VOQBUFLIMITSEL3116_t buf_sel;
    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    buf_sel.wrd = AAL_PON_REG_READ (PUC_VOQBUFLIMITSEL3116);
    *cfg = buf_sel.bf.buflimitsel;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_sel3116_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg)
{
    PUC_VOQBUFLIMITSEL3116_t buf_sel;
    __puc_lock();
    buf_sel.bf.buflimitsel = cfg;

    AAL_PON_REG_WRITE (buf_sel.wrd, PUC_VOQBUFLIMITSEL3116);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_sel4732_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_VOQBUFLIMITSEL4732_t buf_sel;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    buf_sel.wrd = AAL_PON_REG_READ (PUC_VOQBUFLIMITSEL4732);
    *cfg = buf_sel.bf.buflimitsel;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_sel4732_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg)
{
    PUC_VOQBUFLIMITSEL4732_t buf_sel;
    __puc_lock();
    buf_sel.bf.buflimitsel = cfg;
    AAL_PON_REG_WRITE (buf_sel.wrd, PUC_VOQBUFLIMITSEL4732);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_sel6348_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_VOQBUFLIMITSEL6348_t buf_sel;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    buf_sel.wrd = AAL_PON_REG_READ (PUC_VOQBUFLIMITSEL6348);
    *cfg = buf_sel.bf.buflimitsel;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_sel6348_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg)
{
    PUC_VOQBUFLIMITSEL6348_t buf_sel;
    __puc_lock();

    buf_sel.bf.buflimitsel = cfg;
    AAL_PON_REG_WRITE (buf_sel.wrd, PUC_VOQBUFLIMITSEL6348);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_voq_buf_lmt_a_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_VOQBUFLIMIT_A_t buf_lmt;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    buf_lmt.wrd = AAL_PON_REG_READ (PUC_VOQBUFLIMIT_A);
    *cfg = buf_lmt.bf.buflimit;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_a_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg)
{
    PUC_VOQBUFLIMIT_A_t buf_lmt;
    __puc_lock();
    buf_lmt.bf.buflimit = cfg;

    AAL_PON_REG_WRITE (buf_lmt.wrd, PUC_VOQBUFLIMIT_A);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_b_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_VOQBUFLIMIT_B_t buf_lmt;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    buf_lmt.wrd = AAL_PON_REG_READ (PUC_VOQBUFLIMIT_B);
    *cfg = buf_lmt.bf.buflimit;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_b_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg)
{
    PUC_VOQBUFLIMIT_B_t buf_lmt;
    __puc_lock();
    buf_lmt.bf.buflimit = cfg;

    AAL_PON_REG_WRITE (buf_lmt.wrd, PUC_VOQBUFLIMIT_B);
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_c_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *cfg)
{
    PUC_VOQBUFLIMIT_C_t buf_lmt;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    buf_lmt.wrd = AAL_PON_REG_READ (PUC_VOQBUFLIMIT_C);
    *cfg = buf_lmt.bf.buflimit;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_buf_lmt_c_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t cfg)
{
    PUC_VOQBUFLIMIT_C_t buf_lmt;
    __puc_lock();

    buf_lmt.bf.buflimit = cfg;
    AAL_PON_REG_WRITE (buf_lmt.wrd, PUC_VOQBUFLIMIT_C);
    __puc_unlock();
    return CA_E_OK;
}

/*
 * @__aal_puc_voq_flush
 * flush VoQ data
 */
ca_status_t aal_puc_voq_flush(void)
{
    #if 0
    //ca_uint32_t voq = 0;
    ca_uint32_t tcont_idx = 0;
    ca_uint32_t voq_idx = 0;
    //ca_int32_t  timeout = 0;
    ca_status_t ret = CA_E_OK;
    aal_puc_voq_drop_cfg_msk_t voq_drp_mask;
    aal_puc_voq_drop_cfg_t voq_drp;
    aal_puc_voq_flush_cfg_msk_t voq_flush_mask;
    aal_puc_voq_flush_cfg_t voq_flush;

    memset(&voq_drp, 0, sizeof(voq_drp));
    memset(&voq_flush, 0, sizeof(voq_flush));
    voq_drp_mask.u32      = 0;
    voq_drp_mask.s.voqid  = 1;
    voq_drp_mask.s.enable = 1;
    voq_drp.enable        = 1;

    voq_flush_mask.u32 = 0;
    voq_flush_mask.s.voqid = 1;
    voq_flush_mask.s.tcontid = 1;
    voq_flush_mask.s.openpktflushen = 1;
    voq_flush_mask.s.start = 1;

    for (tcont_idx = 0; tcont_idx < tcont_num_per_chip; tcont_idx++) {
        voq_flush.tcontid = tcont_idx;
        for(voq_idx = 0; voq_idx < 8; voq_idx++){
            voq_drp.voqid = tcont_idx*8+voq_idx;

            ret = aal_puc_voq_drop_set(0, voq_drp_mask, &voq_drp);
            CA_RET_VALUE_CHECK(ret);

            voq_flush.voqid = tcont_idx*8+voq_idx;
            voq_flush.openpktflushen = 1;
            voq_flush.start = 1;
            ret = aal_puc_voq_flush_set(0, voq_flush_mask, &voq_flush);
            CA_RET_VALUE_CHECK(ret);
        }
    }

    ca_mdelay(10);

    voq_drp.enable        = 0;
// TODO: queue number should read from queue mode
    for (tcont_idx = 0; tcont_idx < tcont_num_per_chip; tcont_idx++) {
        for(voq_idx = 0; voq_idx < 8; voq_idx++){

            voq_drp.voqid = tcont_idx * 8 + voq_idx;

            ret = aal_puc_voq_drop_set(0, voq_drp_mask, &voq_drp);
            CA_RET_VALUE_CHECK(ret);
        }
    }
#else
    ca_uint32_t voq  = 0;
    ca_uint32_t port = 0;
    ca_uint32_t pon_mode = aal_pon_mac_mode_get(AAL_DEFAUL_DEV_ID);

    if (CA_PON_MODE_IS_GPON_FAMILY(pon_mode)) {
        for (port = 0; port < tcont_num_per_chip; port++) {
            aal_puc_voq_flush_by_idx(AAL_DEFAUL_DEV_ID, port, voq, queue_num_per_tcont-1);
        }
    }else if (CA_PON_MODE_IS_EPON_FAMILY(pon_mode)) {
        for (port = 0; port < llid_num_per_chip; port++) {
            aal_puc_voq_flush_by_idx(AAL_DEFAUL_DEV_ID, port, voq, queue_num_per_llid-1);
        }
    } else if (CA_PON_MODE_IS_ETHERNET(pon_mode)) {
        aal_puc_voq_flush_by_idx(AAL_DEFAUL_DEV_ID, AAL_PON_PORT_ID, 0, 7);
    }
#endif

    return CA_E_OK;
}

ca_status_t aal_puc_voq_flush_by_idx(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t tcont_id,
    CA_IN ca_uint32_t voq_start,
    CA_IN ca_uint32_t voq_end)
{
    ca_uint32_t voq = 0;
    ca_int32_t  timeout = 0;
    ca_uint8_t  queue_num = queue_num_per_tcont;
    ca_status_t ret = CA_E_OK;
    aal_puc_voq_drop_cfg_msk_t voq_drp_mask;
    aal_puc_voq_drop_cfg_t voq_drp,exist_voq_drop;
    aal_puc_voq_flush_cfg_msk_t voq_flush_mask;
    aal_puc_voq_flush_cfg_t voq_flush;
    aal_puc_queue_cfg_msk_t voq_cfg_mask;
    aal_puc_queue_cfg_t voq_cfg_old, voq_cfg;
    aal_puc_voq_total_que_len_cfg_t  voq_sts;

    memset(&voq_drp, 0, sizeof(aal_puc_voq_drop_cfg_t));
    memset(&exist_voq_drop, 0, sizeof(aal_puc_voq_drop_cfg_t));
    memset(&voq_flush, 0, sizeof(aal_puc_voq_flush_cfg_t));
    memset(&voq_cfg, 0, sizeof(aal_puc_queue_cfg_t));

    voq_drp_mask.u32      = 0;
    voq_drp_mask.s.voqid  = 1;
    voq_drp_mask.s.enable = 1;

    voq_flush_mask.u32 = 0;
    voq_flush_mask.s.voqid = 1;
    voq_flush_mask.s.tcontid = 1;
    voq_flush_mask.s.openpktflushen = 1;
    voq_flush_mask.s.start = 1;

    voq_cfg_mask.u32 = 0;
    voq_cfg_mask.s.queue_id = 1;
    voq_cfg_mask.s.queue_ena = 1;
    voq_cfg_mask.s.queue_weight = 1;

    if (aal_pon_mac_mode_get(device_id) <= ONU_PON_MAC_EPON_BI10G)
        queue_num = queue_num_per_llid;
    else if (aal_pon_mac_mode_get(device_id) == ONU_PON_MAC_ETHERNET)
        queue_num = 8;

    for (voq = voq_start; voq <= voq_end; voq++) {

        voq_drp.enable = 1;
        voq_drp.voqid  = tcont_id * queue_num + voq;

        /* drop enable */
        ret = aal_puc_voq_drop_set(device_id, voq_drp_mask, &voq_drp);
        CA_RET_VALUE_CHECK(ret);

        ca_mdelay(1);

        /* voq disable */
        memset(&voq_cfg_old, 0, sizeof(voq_cfg_old));
        ret = aal_puc_port_voq_map_get(device_id, tcont_id, voq,
            &voq_cfg_old);

        ret = aal_puc_port_voq_map_set(device_id, tcont_id, voq,
            voq_cfg_mask, &voq_cfg);
        CA_RET_VALUE_CHECK(ret);


        voq_flush.voqid = voq_drp.voqid;
        voq_flush.start = 1;
        voq_flush.openpktflushen = 1;
        voq_flush.tcontid = tcont_id;

        /* Start flush */
        ret = aal_puc_voq_flush_set(device_id, voq_flush_mask, &voq_flush);
        CA_RET_VALUE_CHECK(ret);

        /* wait flush done */
        timeout = __VOQ_FLUSH_TIMEOUT;
        do {
            ret = aal_puc_voq_flush_get(device_id, &voq_flush);
            CA_RET_VALUE_CHECK(ret);
            if (voq_flush.start == 0) { /* if flush done, break */
                break;
            }
            ca_mdelay(1);
        }while (timeout-- > 0);

        if (timeout <= 0) {
            ca_printf("error: flush voq %d timeout.\r\n", voq);
        }

        /* Check VOQ counter */
        memset(&voq_sts, 0, sizeof(voq_sts));
        ret = aal_puc_voq_total_queue_len_mem_get(device_id,
            voq_flush.voqid, &voq_sts);
        if (voq_sts.pktcnt || voq_sts.bytecnt) {
            AAL_PUC_DEBUG("warning: voq %d is not empty after flush.\r\n",
                voq_flush.voqid);
        }

        /* restore voq cfg */
        voq_drp.enable = 0;
        ret = aal_puc_voq_drop_get(device_id,&exist_voq_drop);
        CA_RET_VALUE_CHECK(ret);
        voq_drp.voqid |=  exist_voq_drop.voqid;
        ret = aal_puc_voq_drop_set(device_id, voq_drp_mask, &voq_drp);
        CA_RET_VALUE_CHECK(ret);

        //voq_cfg_old.queue_ena = 0;
        ret = aal_puc_port_voq_map_set(device_id, tcont_id, voq,
            voq_cfg_mask, &voq_cfg_old);
        CA_RET_VALUE_CHECK(ret);
    }

    AAL_PUC_DEBUG("flush PUC-Port(TCONT/LLID) %d voq %d~%d OK.\r\n",
        tcont_id, voq_start, voq_end);

    return CA_E_OK;
}

ca_status_t aal_puc_voq_flush_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_voq_flush_cfg_t *cfg)
{
    PUC_VOQFLUSH_t voq_flush;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    voq_flush.wrd = AAL_PON_REG_READ (PUC_VOQFLUSH);
    cfg->voqid = voq_flush.bf.voqid;
    cfg->start = voq_flush.bf.start;
    __puc_unlock();
    return CA_E_OK;
}
ca_status_t aal_puc_voq_flush_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_voq_flush_cfg_msk_t mask,
    CA_IN aal_puc_voq_flush_cfg_t *cfg)
{
    PUC_VOQFLUSH_t voq_flush;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();
    voq_flush.wrd = AAL_PON_REG_READ (PUC_VOQFLUSH);
    if (mask.s.voqid)
        voq_flush.bf.voqid = cfg->voqid ;
    if (mask.s.tcontid)
        voq_flush.bf.tcontid = cfg->tcontid ;
    if (mask.s.openpktflushen)
        voq_flush.bf.openpktflushen = cfg->openpktflushen ;
    if (mask.s.start)
        voq_flush.bf.start = cfg->start ;
    AAL_PON_REG_WRITE (voq_flush.wrd, PUC_VOQFLUSH);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_voq_drop_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_voq_drop_cfg_t *cfg)
{
    PUC_VOQDROP_t voq_drop;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    voq_drop.wrd = AAL_PON_REG_READ (PUC_VOQDROP);
    cfg->voqid = voq_drop.bf.voqid;
    cfg->enable = voq_drop.bf.enable;
    cfg->invalid_voqdrop_enable = voq_drop.bf.invalid_voqdrop_enable;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_voq_drop_set(
    CA_IN ca_device_id_t device_id,
    CA_IN aal_puc_voq_drop_cfg_msk_t mask,
    CA_IN aal_puc_voq_drop_cfg_t *cfg)
{
    PUC_VOQDROP_t voq_drop;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);

    if (!mask.u32) {
        return CA_E_OK;
    }
    __puc_lock();

    voq_drop.wrd = AAL_PON_REG_READ (PUC_VOQDROP);
    if (mask.s.voqid)
        voq_drop.bf.voqid = cfg->voqid ;
    if (mask.s.invalid_voqdrop_enable)
        voq_drop.bf.invalid_voqdrop_enable = cfg->invalid_voqdrop_enable ;
    if (mask.s.enable)
        voq_drop.bf.enable = cfg->enable ;

    AAL_PON_REG_WRITE (voq_drop.wrd, PUC_VOQDROP);
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_voq_bp_remap_data_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_IN ca_uint32_t tqmvoqid)
{
    PUC_VOQBPREMAP_DATA_t  voqbpremap_data;

    AAL_ASSERT_RET(voq_id < AAL_PUC_QUEUE_NUM, CA_E_PARAM);

   __puc_lock();

    AAL_PON_REG_INDIRECT_READ(voq_id, PUC_VOQBPREMAP_ACCESS);

    voqbpremap_data.wrd = AAL_PON_REG_READ (PUC_VOQBPREMAP_DATA);

    voqbpremap_data.bf.tqmvoqid = tqmvoqid;

    AAL_PON_REG_WRITE (voqbpremap_data.wrd, PUC_VOQBPREMAP_DATA);
    AAL_PON_REG_INDIRECT_WRITE(voq_id, PUC_VOQBPREMAP_ACCESS);
    __puc_unlock();

    return CA_E_OK;
}

ca_status_t aal_puc_voq_bpremap_get(
    CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t i = 0;
    ca_uint32_t tqmvoqid = 0;
    ca_printf("\r\nVoQ ID  tqmvoqid\r\n");
    ca_printf("----------------------------------------------------\r\n");
    for (i = 0; i <  64; i++){
        ret = aal_puc_voq_bp_remap_data_get(device_id, i ,&tqmvoqid);
        CA_RET_VALUE_CHECK(ret);
        ca_printf("%-6u  %-12u\r\n",i,tqmvoqid);
    }

    return ret;
}


ca_status_t aal_puc_voq_bp_remap_data_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t voq_id,
    CA_OUT ca_uint32_t *tqmvoqid)
{
    PUC_VOQBPREMAP_DATA_t  voqbpremap_data;

    AAL_ASSERT_RET(voq_id < AAL_PUC_QUEUE_NUM, CA_E_PARAM);
    AAL_ASSERT_RET(tqmvoqid != NULL, CA_E_PARAM);
    __puc_lock();
    AAL_PON_REG_INDIRECT_READ(voq_id, PUC_VOQBPREMAP_ACCESS);
    voqbpremap_data.wrd = AAL_PON_REG_READ (PUC_VOQBPREMAP_DATA);
    *tqmvoqid = voqbpremap_data.bf.tqmvoqid;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_bm_dbg_fbpa_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bm_dbg_fbpa_cnt_t *cfg)
{
    PUC_BMDBGFBPA_t bm_dbg_fbpa_cnt;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    bm_dbg_fbpa_cnt.wrd = AAL_PON_REG_READ (PUC_BMDBGFBPA);
    cfg->fbcnta= bm_dbg_fbpa_cnt.bf.fbcnta;
    cfg->fbphptra= bm_dbg_fbpa_cnt.bf.fbphptra;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_bm_dbg_fbpb_cnt_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT aal_puc_bm_dbg_fbpb_cnt_t *cfg)
{
    PUC_BMDBGFBPB_t bm_dbg_fbpb_cnt;

    AAL_ASSERT_RET(cfg != NULL, CA_E_PARAM);
    __puc_lock();
    bm_dbg_fbpb_cnt.wrd = AAL_PON_REG_READ (PUC_BMDBGFBPB);
    cfg->fbcntb   = bm_dbg_fbpb_cnt.bf.fbcntb;
    cfg->fbphptrb = bm_dbg_fbpb_cnt.bf.fbphptrb;
    __puc_unlock();
    return CA_E_OK;
}

ca_status_t aal_puc_ntt_cfg_set(
    CA_IN   ca_device_id_t device_id,
    CA_IN   aal_puc_ntt_cfg_msk_t puc_ntt_cfg_msk,
    CA_IN   aal_puc_ntt_cfg_t puc_ntt_cfg
)
{
    PUC_NTTCFG_t  cfg;

    cfg.wrd = AAL_PON_REG_READ(PUC_NTTCFG);

    if(puc_ntt_cfg_msk.s.zeroqth){
        cfg.bf.zeroqth = puc_ntt_cfg.zeroqth;
    }

    if(puc_ntt_cfg_msk.s.zeroqrpten){
        cfg.bf.zeroqrpten = puc_ntt_cfg.zeroqrpten;
    }

    AAL_PON_REG_WRITE(cfg.wrd,PUC_NTTCFG);

    return CA_E_OK;
}

ca_status_t aal_puc_ntt_cfg_get(
    CA_IN   ca_device_id_t device_id,
    CA_OUT  aal_puc_ntt_cfg_t *puc_ntt_cfg)
{
    PUC_NTTCFG_t  cfg;
    AAL_ASSERT_RET(puc_ntt_cfg , CA_E_PARAM);

    cfg.wrd = AAL_PON_REG_READ(PUC_NTTCFG);

    puc_ntt_cfg->zeroqrpten = cfg.bf.zeroqrpten;
    puc_ntt_cfg->zeroqth  = cfg.bf.zeroqth;

    return CA_E_OK;
}

ca_status_t aal_puc_rpt_adj_cfg1_set(
    CA_IN   ca_device_id_t device_id,
    CA_IN   aal_puc_rpt_adj_cfg1_msk_t  adj_cfg_msk,
    CA_IN   aal_puc_rpt_adj_cfg1_t  adj_cfg
)
{
    PUC_RPTADJCFG1_t cfg;

    cfg.wrd = AAL_PON_REG_READ(PUC_RPTADJCFG1);

    if(adj_cfg_msk.s.rptadjpid){
        cfg.bf.rptadjpid = adj_cfg.rptadjpid;
    }

    if(adj_cfg_msk.s.rptadjqnum){
        cfg.bf.rptadjqnum = adj_cfg.rptadjqnum;
    }

    if(adj_cfg_msk.s.rptadjqsnum){
        cfg.bf.rptadjqsnum = adj_cfg.rptadjqsnum;
    }

    if(adj_cfg_msk.s.rptgapcnten){
        cfg.bf.rptgapcnten = adj_cfg.rptgapcnten;
    }

    if(adj_cfg_msk.s.rptadjbuffullen){
        cfg.bf.rptadjbuffullen = adj_cfg.rptadjbuffullen;
    }

    if(adj_cfg_msk.s.rptadjunibursten){
        cfg.bf.rptadjunibursten = adj_cfg.rptadjunibursten;
    }

    AAL_PON_REG_WRITE(cfg.wrd,PUC_RPTADJCFG1);

    return CA_E_OK;
}

ca_status_t aal_puc_rpt_adj_cfg1_get(
    CA_IN   ca_device_id_t device_id,
    CA_OUT   aal_puc_rpt_adj_cfg1_t  *adj_cfg
)
{
    PUC_RPTADJCFG1_t cfg;

    AAL_ASSERT_RET(adj_cfg, CA_E_PARAM);

    cfg.wrd = AAL_PON_REG_READ(PUC_RPTADJCFG1);

    adj_cfg->rptadjpid = cfg.bf.rptadjpid;
    adj_cfg->rptadjqnum = cfg.bf.rptadjqnum;
    adj_cfg->rptadjqsnum = cfg.bf.rptadjqsnum;
    adj_cfg->rptgapcnten = cfg.bf.rptgapcnten;
    adj_cfg->rptadjbuffullen = cfg.bf.rptadjbuffullen;
    adj_cfg->rptadjunibursten = cfg.bf.rptadjunibursten;

    return CA_E_OK;

}

#define CA_PUC_LLID_COS_SEL_MAX_LLID_NUMS 8

ca_status_t aal_puc_init(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t pon_mac_mode)
{
    static ca_uint32_t puc_init_done = 0;
    ca_status_t ret = CA_E_OK;
    aal_puc_btc_cfg_msk_t btc_cfg_msk;
    aal_puc_btc_cfg_t     btc_cfg;
    ca_uint32_t bm_sts = 0 ;
    ca_uint32_t puc_voq_buf_limit_a = __PUC_EPON_BUF_DEFAULT_LIMIT_A;
    ca_uint32_t puc_voq_buf_limit_b = __PUC_EPON_BUF_DEFAULT_LIMIT_B;
    ca_uint32_t puc_voq_buf_limit_c = __PUC_EPON_BUF_DEFAULT_LIMIT_C;
    ca_uint32_t puc_voq_buf_limit_sel[CA_PUC_LLID_COS_SEL_MAX_LLID_NUMS >> 1] =
        {0xfea9fea9, 0xfea9fea9, 0xfea9fea9, 0xfea9fea9};
    ca_uint8_t puc_voq_buf_sel_scfg[CA_PUC_LLID_COS_SEL_MAX_LLID_NUMS][CFG_ID_PUC_LLID0_COS_THRESHOLDS_SEL_LEN] =
          {{0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0},
           {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}}; /* up to 8 llids SCFG */
    ca_uint8_t default_voq_cos_threshold_sel[CFG_ID_PUC_LLID0_COS_THRESHOLDS_SEL_LEN] =
        {1, 2, 2, 2, 2, 3, 3, 3};
    ca_device_config_tlv_t scfg_tlv;
    ca_status_t scfg_rtn_val = CA_E_ERROR;
    ca_uint8_t i = 0;
    ca_uint32_t tcont_port = 0;
    ca_uint32_t tqmvoqid = 0;
    aal_puc_us_omci_hdr_a_cfg_msk_t us_omci_hdr_msk;
    aal_puc_us_omci_hdr_a_cfg_t us_omci_hdr_cfg;
    aal_puc_us_omci_hp_hdr_a_cfg_msk_t us_omci_hp_hdr_msk;
    aal_puc_us_omci_hp_hdr_a_cfg_t us_omci_hp_hdr_cfg;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_voq_tbc_cfg_msk;
    aal_puc_voq_tbc_mem_cfg_t puc_voq_tbc_cfg;
    ca_uint16_t tbc_voq_id = 0;

    ca_uint32_t llid_idx = 0;
    ca_uint8_t llid_mode = AAL_MPCP_LLID_MODE_SINGLE;
    ca_uint8_t report_mode = CA_EPON_MPCP_REPORT_MODE_2QS_8Q;
    aal_puc_rpt_adj_cfg1_msk_t rpt_cfg1_msk;
    aal_puc_rpt_adj_cfg1_t rpt_cfg1;
    aal_puc_rpt_cfg_msk_t rpt_msk;
    aal_puc_rpt_cfg_t     rpt_cfg;
    aal_puc_epon_que_thrsd12_cfg_msk_t rpt_threshold_12_msk;
    aal_puc_epon_que_thrsd12_cfg_t     rpt_threshold_12;
    aal_puc_epon_que_thrsd34_cfg_msk_t rpt_threshold_34_msk;
    aal_puc_epon_que_thrsd34_cfg_t     rpt_threshold_34;
    aal_mpcp_cfg_t mpcp_cfg;
    aal_mpcp_msk_t mpcp_msk;
    ca_uint8 *str_report_mode[] = {"2QS8P", "1QS8P", "1QS1P", "2QS1P", "4QS1P", "NTT"};

    ca_uint32_t queue_idx = 0;
    aal_puc_queue_cfg_msk_t queue_cfg_mask;
    aal_puc_queue_cfg_t queue_cfg;
    aal_puc_sts_cfg_t   puc_sts;
    PUC_PUCCFG_t   puc_cfg;     /* PUC configuration */
    PDC_CNTR_CFG_t pdc_cfg;     /* PDC configuration */
    ca_uint8_t upstream_use_dq = 0;
#ifdef RTK_VOQ_VALID_SUPPORT
    aal_puc_voq_drop_cfg_msk_t voq_drop_cfg_mask;
    aal_puc_voq_drop_cfg_t voq_drop_cfg_cfg;
#endif

    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, puc_reset, 1);
    ca_udelay(500);

    ret = scfg_read(0, CFG_ID_PUC_VOQBUFLIMIT_A, sizeof(ca_uint32_t), &puc_voq_buf_limit_a);
    if(ret != CA_E_OK){
        puc_voq_buf_limit_a = __PUC_EPON_BUF_DEFAULT_LIMIT_A;
    }


    ret = scfg_read(0, CFG_ID_PUC_VOQBUFLIMIT_B, sizeof(ca_uint32_t), &puc_voq_buf_limit_b);
    if(ret != CA_E_OK){
        puc_voq_buf_limit_b = __PUC_EPON_BUF_DEFAULT_LIMIT_B;
    }

    ret = scfg_read(0, CFG_ID_PUC_VOQBUFLIMIT_C, sizeof(ca_uint32_t), &puc_voq_buf_limit_c);
    if(ret != CA_E_OK){
        puc_voq_buf_limit_c = __PUC_EPON_BUF_DEFAULT_LIMIT_C;
    }
    

    for (i=0; i<CA_PUC_LLID_COS_SEL_MAX_LLID_NUMS; i++)
    {
        #if 0
        if (scfg_read(0, CFG_ID_PUC_LLID0_COS_THRESHOLDS_SEL + i, CFG_ID_PUC_LLID0_COS_THRESHOLDS_SEL_LEN, puc_voq_buf_sel_scfg[i]) != CA_E_OK)
        {
            ca_printf("ca-puc: failed to SCFG field CFG_ID_PUC_LLIDx_COS_THRESHOLDS_SEL for llid-idx(%d)\n", i);
            puc_voq_buf_limit_sel[i>>1] = 0xfea9fea9;
        }
        #endif

        memset(&scfg_tlv, 0, sizeof(scfg_tlv));
        scfg_tlv.type = CA_CFG_ID_PUC_LLID0_COS_THRESHOLDS_SEL + i;

        scfg_rtn_val = ca_device_config_tlv_get(0, &scfg_tlv);

        if (scfg_rtn_val == CA_E_OK && scfg_tlv.len == CFG_ID_PUC_LLID0_COS_THRESHOLDS_SEL_LEN)
        {
            memcpy(puc_voq_buf_sel_scfg[i], scfg_tlv.value, CFG_ID_PUC_LLID0_COS_THRESHOLDS_SEL_LEN);
        }
        else
        {
            ca_printf("ca-puc: failed to SCFG field CFG_ID_PUC_LLIDx_COS_THRESHOLDS_SEL for llid-idx(%d)\n", i);
            memcpy(puc_voq_buf_sel_scfg[i], default_voq_cos_threshold_sel, sizeof(default_voq_cos_threshold_sel));
        }

    }

    for (i=0; i<(CA_PUC_LLID_COS_SEL_MAX_LLID_NUMS >> 1); i++)
    {
        puc_voq_buf_limit_sel[i] =
            (puc_voq_buf_sel_scfg[i*2+1][7] << 30) | (puc_voq_buf_sel_scfg[i*2+1][6] << 28) |
            (puc_voq_buf_sel_scfg[i*2+1][5] << 26) | (puc_voq_buf_sel_scfg[i*2+1][4] << 24) |
            (puc_voq_buf_sel_scfg[i*2+1][3] << 22) | (puc_voq_buf_sel_scfg[i*2+1][2] << 20) |
            (puc_voq_buf_sel_scfg[i*2+1][1] << 18) | (puc_voq_buf_sel_scfg[i*2+1][0] << 16) |
            (puc_voq_buf_sel_scfg[i*2][7]   << 14) | (puc_voq_buf_sel_scfg[i*2][6] << 12) |
            (puc_voq_buf_sel_scfg[i*2][5]   << 10) | (puc_voq_buf_sel_scfg[i*2][4] << 8 ) |
            (puc_voq_buf_sel_scfg[i*2][3]   << 6 ) | (puc_voq_buf_sel_scfg[i*2][2] << 4 ) |
            (puc_voq_buf_sel_scfg[i*2][1]   << 2 ) | (puc_voq_buf_sel_scfg[i*2][0] << 0 );
    }

    if (!g_puc_lock) {
        ret = ca_spin_lock_init(&g_puc_lock, SPINLOCK_BH);
        if (ret != 0) {
            ca_printf("create puc spin lock fail\r\n");
            return ret;
        }
    }

    /* check H/W init status */
    memset(&bm_sts, 0, sizeof(bm_sts));
    ret = aal_puc_bm_stat_get(device_id,&bm_sts);
    CA_RET_VALUE_CHECK(ret);

    if(!bm_sts){
        ca_printf("err: PUC bm_sts is not ready\r\n");
        return CA_E_ERROR;
    }

    memset(&puc_sts, 0, sizeof(puc_sts));
    ret = aal_puc_sts_get(device_id, &puc_sts);
    CA_RET_VALUE_CHECK(ret);

    if (!puc_sts.shp_init_done) {
        ca_printf("err: PUC shp_init_done not ready!\r\n");
        return CA_E_ERROR;
    }

    AAL_PON_REG_WRITE(0, PUC_PONCNTL_INTENABLE);

    puc_cfg.wrd = AAL_PON_REG_READ(PUC_PUCCFG);
    puc_cfg.bf.inccfg = 2;                      /* clear PUC counter on reading */
    puc_cfg.bf.crccntl= 2; /* re-generate CRC for upstream */
    AAL_PON_REG_WRITE(puc_cfg.wrd, PUC_PUCCFG);

    pdc_cfg.wrd = AAL_PON_REG_READ(PDC_CNTR_CFG);
    pdc_cfg.bf.inccfg = 2;                      /* clear PDC counter on reading */
    AAL_PON_REG_WRITE(pdc_cfg.wrd, PDC_CNTR_CFG);


    ret = scfg_read(0, CFG_ID_PON_LLID_MODE, sizeof(llid_mode), &llid_mode);
    if (ret != CA_E_OK)
        llid_mode = AAL_MPCP_LLID_MODE_SINGLE;

    /* Init VoQ buffer LimitA for XGPON according to ASIC team
     * This will limit the max buffer usage for each VoQ
     * to max 6K enough to support 10G rate
     */
    if (pon_mac_mode == ONU_PON_MAC_EPON_BI10G)
    {
        ret |= aal_puc_voq_buf_lmt_a_set(device_id, puc_voq_buf_limit_a);
        ret |= aal_puc_voq_buf_lmt_b_set(device_id, puc_voq_buf_limit_b);
        ret |= aal_puc_voq_buf_lmt_c_set(device_id, puc_voq_buf_limit_c);

        AAL_PON_REG_FIELD_WRITE(PUC_RPTADJCFG2, rptadjfbloth, __PUC_EPON_AVA_BUF_RPT_FFFF_THRD);

        memset(&rpt_cfg1, 0 , sizeof(rpt_cfg1));
        rpt_cfg1_msk.u32 = 0;

        /* don't adjust buffer length to max value once free buffer is lesser than threshold for m-llid case */
        rpt_cfg1.rptadjbuffullen = ((llid_mode == AAL_MPCP_LLID_MODE_SINGLE) ? 1 : 0);
        rpt_cfg1.rptadjqsnum = 2;
        rpt_cfg1_msk.s.rptadjbuffullen = 1;
        rpt_cfg1_msk.s.rptadjqsnum     = 1;
        (void)aal_puc_rpt_adj_cfg1_set(device_id,rpt_cfg1_msk,rpt_cfg1);
    }
    else if (pon_mac_mode == ONU_PON_MAC_ETHERNET)
    {
        ret |= aal_puc_voq_buf_lmt_a_set(device_id, 0x7fff);
        ret |= aal_puc_voq_buf_lmt_b_set(device_id, 0x7fff);
        ret |= aal_puc_voq_buf_lmt_c_set(device_id, 0x7fff);    
    }
    else
    {
        ret |= aal_puc_voq_buf_lmt_a_set(device_id, puc_voq_buf_limit_a);
        ret |= aal_puc_voq_buf_lmt_b_set(device_id, puc_voq_buf_limit_b);
        ret |= aal_puc_voq_buf_lmt_c_set(device_id, puc_voq_buf_limit_c);    
    }
    CA_RET_VALUE_CHECK(ret);

    /* VoQ based 32B buffer configuration, system provides up to 4 profile for selection */
    (void)aal_puc_voq_buf_lmt_sel1500_set(device_id, puc_voq_buf_limit_sel[0]);
    (void)aal_puc_voq_buf_lmt_sel3116_set(device_id, puc_voq_buf_limit_sel[1]);
    (void)aal_puc_voq_buf_lmt_sel4732_set(device_id, puc_voq_buf_limit_sel[2]);
    (void)aal_puc_voq_buf_lmt_sel6348_set(device_id, puc_voq_buf_limit_sel[3]);

    ret = aal_puc_voq_map_cfg_set(device_id, puc_voq_map_mode);
    CA_RET_VALUE_CHECK(ret);

    queue_cfg_mask.u32 = 0;
    memset(&queue_cfg, 0 , sizeof(queue_cfg));
    queue_cfg_mask.s.queue_id = 1;
    queue_cfg_mask.s.queue_ena = 1;
    queue_cfg_mask.s.queue_weight = 1;
    queue_cfg.queue_ena = 1;
    queue_cfg.queue_weight = 0; /* SP mode */

    if (aal_pon_mac_mode <= ONU_PON_MAC_EPON_BI10G)
    {
        if (puc_voq_map_mode == AAL_PUC_VOQ_MODE_8Q || puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP)
        {
            llid_num_per_chip = 16;
            queue_num_per_llid = 8;
        }
        else if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_4Q)
        {
            llid_num_per_chip = 32;
            queue_num_per_llid = 4;
        }
        else
        {
            ca_printf("ca-puc: puc_voq_map_mode(%d) is not supported.\n", puc_voq_map_mode);
            return CA_E_ERROR;
        }

        ca_printf("ca-puc: configure VoQ mapping table - llid_num_per_chip(%d), queue_num_per_llid(%d)\n", llid_num_per_chip, queue_num_per_llid);
        ret = scfg_read(0, CFG_ID_PON_REPORT_MODE, sizeof(report_mode), &report_mode);
        if (ret != CA_E_OK)
        {
            ca_printf("ca-puc: failed to get CFG_ID_PON_REPORT_MODE\n");
            report_mode = CA_EPON_MPCP_REPORT_MODE_2QS_8Q;
        }

        ca_printf("ca-puc: mpcp report mode(%d) - %s\n", report_mode, str_report_mode[report_mode]);

        /* set puc llid->voq mapping */

        if ((report_mode == CA_EPON_MPCP_REPORT_MODE_4QS_1Q) ||
            (report_mode == CA_EPON_MPCP_REPORT_MODE_1QS_1Q))
        {
            memset((void *)&rpt_msk, 0, sizeof(rpt_msk));
            memset((void *)&rpt_cfg, 0, sizeof(rpt_cfg));

            rpt_msk.s.pkgarpt  = 1;
            rpt_msk.s.aggrth   = 1;
            rpt_msk.s.numqsets = 1;
            rpt_msk.s.lastqstot  = 1;
            rpt_msk.s.firstqstot = 1;

            rpt_cfg.pkgarpt    = 1;     /* SIEPON package A */
            rpt_cfg.aggrth     = 0;
            rpt_cfg.firstqstot = 0;     /* first report queue-set uses threshold programmed in RPTQSCFG12  */
            rpt_cfg.lastqstot  = 0;     /* last report queue-set uses threshold programmed in RPTQSCFG1234 */
            if (report_mode == CA_EPON_MPCP_REPORT_MODE_4QS_1Q)
                rpt_cfg.numqsets = 3;   /* 4 queue-set */
            else
                rpt_cfg.numqsets = 0;   /* 1 queue-set */

            (void)aal_puc_rpt_cfg_set(device_id, rpt_msk, &rpt_cfg);

            /* report threshold configurations */
            memset((void *)&rpt_threshold_12_msk, 0, sizeof(rpt_threshold_12_msk));
            memset((void *)&rpt_threshold_12, 0, sizeof(rpt_threshold_12));

            rpt_threshold_12_msk.s.qs1th = 1;
            rpt_threshold_12_msk.s.qs2th = 1;
            rpt_threshold_12.qs1th = 0x8000;
            rpt_threshold_12.qs2th = 0x8000;
            (void)aal_puc_epon_queue_thrshld12_set(device_id, rpt_threshold_12_msk, &rpt_threshold_12);

            memset((void *)&rpt_threshold_34_msk, 0, sizeof(rpt_threshold_34_msk));
            memset((void *)&rpt_threshold_34, 0, sizeof(rpt_threshold_34));

            rpt_threshold_34_msk.s.qs3th = 1;
            rpt_threshold_34_msk.s.qs4th = 1;
            rpt_threshold_34.qs3th = 0x8000;
            rpt_threshold_34.qs4th = 0x8000;
            (void)aal_puc_epon_queue_thrshld34_set(device_id, rpt_threshold_34_msk, &rpt_threshold_34);

            /* epon mac accepts frames with llid value matching any configured llid in llid table (non-IEEE standard)*/
            /* llid just like 0x8xxx will be supported with this setting */
            AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_LLID_MAPPING_CFG1, monitor_mode, 0x1);

            /* PUC VoQ buffer change */
            (void)aal_puc_voq_buf_lmt_a_set(device_id, 0x7800);
            #if 0
            (void)aal_puc_voq_buf_lmt_sel1500_set(device_id, puc_voq_buf_limit_sel[0]);
            (void)aal_puc_voq_buf_lmt_sel3116_set(device_id, puc_voq_buf_limit_sel[1]);
            (void)aal_puc_voq_buf_lmt_sel4732_set(device_id, puc_voq_buf_limit_sel[2]);
            (void)aal_puc_voq_buf_lmt_sel6348_set(device_id, puc_voq_buf_limit_sel[3]);
            #endif

            if (llid_mode == AAL_MPCP_LLID_MODE_MULTIPLE)
            { /* MLLID mode */

                for (llid_idx=0; llid_idx<llid_num_per_chip; llid_idx++)
                {
                    for (queue_idx=0; queue_idx<queue_num_per_llid; queue_idx++)
                    {
                        if (queue_idx == 0)
                        {
                            queue_cfg.queue_ena = TRUE;
                            queue_cfg.queue_id  = llid_idx * queue_num_per_llid + (queue_num_per_llid - 1);
                        }
                        else
                        {
                            queue_cfg.queue_ena = FALSE;
                            queue_cfg.queue_id = llid_idx * queue_num_per_llid + queue_idx;
                        }

                        (void)aal_puc_port_voq_map_set(device_id, llid_idx, queue_idx, queue_cfg_mask, &queue_cfg);
                    }
                }
            }
            else
            {    /* SLLID mode */

                for (queue_idx=0; queue_idx<8; queue_idx++) /* support 8Qs for llid0 */
                {
                    if (queue_idx == 0)
                    {
                        queue_cfg.queue_ena = TRUE;
                        queue_cfg.queue_id  = 7;    /* PUC Q0 mapping TM Q7, ASIC bug, we must use PUC Q0 entry instead of Q7 */
                    }
                    else
                    {
                        queue_cfg.queue_ena = FALSE;
                        queue_cfg.queue_id  = queue_idx;
                    }

                    (void)aal_puc_port_voq_map_set(device_id, 0, queue_idx, queue_cfg_mask, &queue_cfg);
                }
            }
        }
        else if (report_mode == CA_EPON_MPCP_REPORT_MODE_1QS_8Q)
        {
            memset((void *)&rpt_msk, 0, sizeof(rpt_msk));
            memset((void *)&rpt_cfg, 0, sizeof(rpt_cfg));

            rpt_msk.s.pkgarpt  = 1;
            rpt_msk.s.aggrth   = 1;
            rpt_msk.s.numqsets = 1;
            rpt_msk.s.lastqstot  = 1;
            rpt_msk.s.firstqstot = 1;

            rpt_cfg.pkgarpt    = 1;     /* SIEPON package A */
            rpt_cfg.aggrth     = 0;
            rpt_cfg.firstqstot = 0;     /* first report queue-set uses threshold programmed in RPTQSCFG12  */
            rpt_cfg.lastqstot  = 0;     /* last report queue-set uses threshold programmed in RPTQSCFG1234 */
            rpt_cfg.numqsets   = 0;     /* 1 queue-set */

            (void)aal_puc_rpt_cfg_set(device_id, rpt_msk, &rpt_cfg);

            /* report threshold configurations */
            memset((void *)&rpt_threshold_12_msk, 0, sizeof(rpt_threshold_12_msk));
            memset((void *)&rpt_threshold_12, 0, sizeof(rpt_threshold_12));

            rpt_threshold_12_msk.s.qs1th = 1;
            rpt_threshold_12_msk.s.qs2th = 1;
            rpt_threshold_12.qs1th = 0x8000;
            rpt_threshold_12.qs2th = 0x8000;
            (void)aal_puc_epon_queue_thrshld12_set(device_id, rpt_threshold_12_msk, &rpt_threshold_12);

            memset((void *)&rpt_threshold_34_msk, 0, sizeof(rpt_threshold_34_msk));
            memset((void *)&rpt_threshold_34, 0, sizeof(rpt_threshold_34));

            rpt_threshold_34_msk.s.qs3th = 1;
            rpt_threshold_34_msk.s.qs4th = 1;
            rpt_threshold_34.qs3th = 0x8000;
            rpt_threshold_34.qs4th = 0x8000;
            (void)aal_puc_epon_queue_thrshld34_set(device_id, rpt_threshold_34_msk, &rpt_threshold_34);

            /* epon mac accepts frames with llid value matching any configured llid in llid table (non-IEEE standard)*/
            /* llid just like 0x8xxx will be supported with this setting */
            AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_LLID_MAPPING_CFG1, monitor_mode, 0x1);

            if (llid_mode == AAL_MPCP_LLID_MODE_MULTIPLE)
            { /* MLLID mode */

                for (llid_idx=0; llid_idx<llid_num_per_chip; llid_idx++)
                {
                    for (queue_idx=0; queue_idx<queue_num_per_llid; queue_idx++)
                    {
                        queue_cfg.queue_id = llid_idx * queue_num_per_llid + queue_idx;
                        (void)aal_puc_port_voq_map_set(device_id, llid_idx, queue_idx, queue_cfg_mask, &queue_cfg);
                    }
                }
            }
            else
            {    /* SLLID mode */

                for (queue_idx=0; queue_idx<8; queue_idx++) /* support 8Qs for llid0 */
                {
                    queue_cfg.queue_id  =  queue_idx;
                    (void)aal_puc_port_voq_map_set(device_id, 0, queue_idx, queue_cfg_mask, &queue_cfg);
                }
            }

        }
        else if (report_mode == CA_EPON_MPCP_REPORT_MODE_NTT)
        {
            memset((void *)&rpt_msk, 0, sizeof(rpt_msk));
            memset((void *)&rpt_cfg, 0, sizeof(rpt_cfg));

            rpt_msk.s.pkgarpt  = 1;
            rpt_msk.s.aggrth   = 1;
            rpt_msk.s.numqsets = 1;
            rpt_msk.s.lastqstot  = 1;
            rpt_msk.s.firstqstot = 1;

            rpt_cfg.aggrth     = 1;
            rpt_cfg.pkgarpt    = 0;     /* SIEPON package B */
            rpt_cfg.firstqstot = 1;     /* first report queue-set uses threshold programmed in RPTQSCFG12  */
            rpt_cfg.lastqstot  = 0;     /* last report queue-set uses threshold programmed in RPTQSCFG1234 */
            rpt_cfg.numqsets   = 1;     /* 2 queue sets */

            (void)aal_puc_rpt_cfg_set(device_id, rpt_msk, &rpt_cfg);

            /* enable sieponb compatible scheduling : first qset length = total - granted bandwidth */
            AAL_PON_REG_FIELD_WRITE(PUC_VOQSCHCFG, pkgbsch, 0x1);

            /* epon mac accepts frames with llid value matching any configured llid in llid table (non-IEEE standard)*/
            /* llid just like 0x8xxx will be supported with this setting */
            AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_LLID_MAPPING_CFG1, monitor_mode, 0x1);

            /* don't report 0xffff even if buffer pool is almost empty */
            AAL_PON_REG_FIELD_WRITE(PUC_RPTADJCFG1, rptadjbuffullen, 0);

            /* inrease report latency so as to have enough time to calculate report value */
            AAL_PON_REG_FIELD_WRITE(EPON_EPON_GLB_PUC_CFG0, puc_rpt_latency, 0x200);

            (void)aal_puc_voq_map_cfg_set(device_id, AAL_PUC_VOQ_MODE_4Q);  /* using 4 cos mapping mode */

            if (llid_mode == AAL_MPCP_LLID_MODE_MULTIPLE)
            {
                /* only single llid mode is supported on NTT mode */
                memset((void *)&mpcp_cfg, 0, sizeof(mpcp_cfg));
                memset((void *)&mpcp_msk, 0, sizeof(mpcp_msk));
                mpcp_msk.s.llid_mode = 1;
                mpcp_cfg.llid_mode   = 0;     /* force single llid */

                (void)aal_mpcp_cfg_set(device_id, 0, mpcp_msk, &mpcp_cfg);
            }

            /* update PUC_PVTBL*/
            for (queue_idx=0; queue_idx<4; queue_idx++) /* support 4Qs for llid0 */
            {
                queue_cfg.queue_id  =  queue_idx;
                (void)aal_puc_port_voq_map_set(device_id, 0, queue_idx, queue_cfg_mask, &queue_cfg);
            }

            /* update VoQ configuration on single llid mode */
            AAL_PON_REG_WRITE(0x83828180, PUC_SLLIDVOQ30CFG);
            AAL_PON_REG_WRITE(0x07060504, PUC_SLLIDVOQ74CFG);
        }
        else    /* 2QS8P by default */
        {
            memset((void *)&rpt_msk, 0, sizeof(rpt_msk));
            memset((void *)&rpt_cfg, 0, sizeof(rpt_cfg));

            rpt_msk.s.pkgarpt    = 1;
            rpt_msk.s.aggrth     = 1;
            rpt_msk.s.numqsets   = 1;
            rpt_msk.s.lastqstot  = 1;
            rpt_msk.s.firstqstot = 1;

            rpt_cfg.pkgarpt    = 0;     /* CTC */
            rpt_cfg.aggrth     = 0;
            rpt_cfg.firstqstot = 1;     /* first report queue-set uses threshold programmed in RPTQSCFG12  */
            rpt_cfg.lastqstot  = 0;     /* last report queue-set uses total length instead of threshold in RPTQSCFG34 */
            rpt_cfg.numqsets   = 1;     /* 2 queue-set */

            (void)aal_puc_rpt_cfg_set(device_id, rpt_msk, &rpt_cfg);

            /* report threshold configurations */
            memset((void *)&rpt_threshold_12_msk, 0, sizeof(rpt_threshold_12_msk));
            memset((void *)&rpt_threshold_12, 0, sizeof(rpt_threshold_12));

            rpt_threshold_12_msk.s.qs1th = 1;
            rpt_threshold_12_msk.s.qs2th = 1;
            rpt_threshold_12.qs1th = 0x8000;
            rpt_threshold_12.qs2th = 0x8000;
            (void)aal_puc_epon_queue_thrshld12_set(device_id, rpt_threshold_12_msk, &rpt_threshold_12);

            memset((void *)&rpt_threshold_34_msk, 0, sizeof(rpt_threshold_34_msk));
            memset((void *)&rpt_threshold_34, 0, sizeof(rpt_threshold_34));

            rpt_threshold_34_msk.s.qs3th = 1;
            rpt_threshold_34_msk.s.qs4th = 1;
            rpt_threshold_34.qs3th = 0x8000;
            rpt_threshold_34.qs4th = 0x8000;
            (void)aal_puc_epon_queue_thrshld34_set(device_id, rpt_threshold_34_msk, &rpt_threshold_34);

            if (llid_mode == AAL_MPCP_LLID_MODE_MULTIPLE)
            { /* MLLID mode */

                for (llid_idx=0; llid_idx<llid_num_per_chip; llid_idx++)
                {
                    for (queue_idx=0; queue_idx<queue_num_per_llid; queue_idx++)
                    {
                        queue_cfg.queue_id = llid_idx * queue_num_per_llid + queue_idx;
                        (void)aal_puc_port_voq_map_set(device_id, llid_idx, queue_idx, queue_cfg_mask, &queue_cfg);
                    }
                }
            }
            else
            {    /* SLLID mode */

                for (queue_idx=0; queue_idx<8; queue_idx++) /* support 8Qs for llid0 */
                {
                    queue_cfg.queue_id  =  queue_idx;
                    (void)aal_puc_port_voq_map_set(device_id, 0, queue_idx, queue_cfg_mask, &queue_cfg);
                }
            }

        }

        puc_voq_tbc_cfg_msk.u32 = 0;
        memset(&puc_voq_tbc_cfg,0, sizeof(aal_puc_voq_tbc_mem_cfg_t));
        puc_voq_tbc_cfg_msk.s.enb = 1;
        puc_voq_tbc_cfg.enb = 1;
        for(tbc_voq_id = 0; tbc_voq_id <AAL_PUC_QUEUE_NUM; tbc_voq_id++){
            (void)aal_puc_rl_voq_tbc_mem_set(device_id, tbc_voq_id, puc_voq_tbc_cfg_msk, &puc_voq_tbc_cfg);
        }

        //Using llid-idx 4 for registeration
        for (queue_idx=0; queue_idx<8; queue_idx++)
        {
            queue_cfg.queue_ena = FALSE;
            queue_cfg.queue_id  = 0;
            (void)aal_puc_port_voq_map_set(device_id, 0, queue_idx, queue_cfg_mask, &queue_cfg);

            puc_voq_tbc_cfg_msk.u32 = 0;
            memset(&puc_voq_tbc_cfg,0, sizeof(aal_puc_voq_tbc_mem_cfg_t));
            puc_voq_tbc_cfg_msk.s.enb = 1;
            puc_voq_tbc_cfg.enb = 1;
            (void)aal_puc_rl_voq_tbc_mem_set(device_id, queue_cfg.queue_id, puc_voq_tbc_cfg_msk, &puc_voq_tbc_cfg);

#ifdef RTK_VOQ_VALID_SUPPORT
            (void)aal_puc_voq_valid_set(device_id, queue_idx, queue_cfg.queue_ena);
#endif
        }

        for (queue_idx=0; queue_idx<8; queue_idx++)
        {
            queue_cfg.queue_ena = TRUE;
            queue_cfg.queue_id  = queue_idx + 32;
            (void)aal_puc_port_voq_map_set(device_id, 4, queue_idx, queue_cfg_mask, &queue_cfg);

            puc_voq_tbc_cfg_msk.u32 = 0;
            memset(&puc_voq_tbc_cfg,0, sizeof(aal_puc_voq_tbc_mem_cfg_t));
            puc_voq_tbc_cfg_msk.s.enb = 1;
            puc_voq_tbc_cfg.enb = 0;
            (void)aal_puc_rl_voq_tbc_mem_set(device_id, queue_cfg.queue_id, puc_voq_tbc_cfg_msk, &puc_voq_tbc_cfg);
#ifdef RTK_VOQ_VALID_SUPPORT
            (void)aal_puc_voq_valid_set(device_id, queue_cfg.queue_id , queue_cfg.queue_ena);
#endif
        }

    }
    else if (CA_PON_MODE_IS_ETHERNET(aal_pon_mac_mode)){
        aal_puc_omci_mac_t  mac;

        memset(&mac, 0, sizeof(aal_puc_omci_mac_t));
        queue_cfg.queue_ena = 1;
        queue_cfg.queue_weight = 0;

        for (queue_idx = 0; queue_idx < 8; queue_idx++) {
            queue_cfg.queue_id = queue_idx + 8 * AAL_PON_PORT_ID;
            ret =  aal_puc_port_voq_map_set(device_id, 0, queue_idx, queue_cfg_mask,&queue_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
        aal_puc_glb_lnk_type_set(device_id, 0);
        aal_puc_glb_mac_type_set(device_id, 0);
        aal_puc_glb_da_sa_set(device_id, mac);
    }
    else if (CA_PON_MODE_IS_GPON_FAMILY(aal_pon_mac_mode)){/* GPON, XGPON1, XGSPON, NGPON2 */
        if (puc_voq_map_mode == AAL_PUC_VOQ_MODE_8Q || puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP){
                tcont_num_per_chip = 32;
                queue_num_per_tcont = 8;
        } else if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_4Q){
                tcont_num_per_chip = 64;
                queue_num_per_tcont = 4;
        }else{
            ca_printf("puc_voq_map_mode is not supported,\r\n");
            return CA_E_ERROR;
        }

        if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
        {
            if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_8Q || puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP){
                    tcont_num_per_chip = 8;
                    queue_num_per_tcont = 8;
            } else if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_4Q){
                    tcont_num_per_chip = 8;
                    queue_num_per_tcont = 4;
            }else{
                ca_printf("ca_puc: puc_voq_map_mode(%d) is not supported\n", puc_voq_map_mode);
                return CA_E_ERROR;
            }
        }

        //enable puc voq drain 
        puc_voq_tbc_cfg_msk.u32 = 0;
        memset(&puc_voq_tbc_cfg,0, sizeof(aal_puc_voq_tbc_mem_cfg_t));
        puc_voq_tbc_cfg_msk.s.enb = 1;
        puc_voq_tbc_cfg.enb = 1;
        for(tbc_voq_id = 8; tbc_voq_id <AAL_PUC_QUEUE_NUM; tbc_voq_id++){
            ret = aal_puc_rl_voq_tbc_mem_set(device_id, tbc_voq_id, puc_voq_tbc_cfg_msk, &puc_voq_tbc_cfg);
            CA_RET_VALUE_CHECK(ret);
        }

        for (tcont_port = 0; tcont_port < tcont_num_per_chip; tcont_port++) { /* only use 16 TCONT */
            for (queue_idx = 0; queue_idx < queue_num_per_tcont; queue_idx++) {
                /* NOTE: enable TCONT 0 VoQs for OMCC only by default */
                queue_cfg.queue_ena = __TCONT_FOR_OMCC(tcont_port) ? 1 : 0;
                //queue_cfg.queue_ena = 1;
                queue_cfg.queue_id = queue_idx +  tcont_port * queue_num_per_tcont;
                ret =  aal_puc_port_voq_map_set(device_id, tcont_port, queue_idx, queue_cfg_mask,&queue_cfg);
                CA_RET_VALUE_CHECK(ret);

#ifdef RTK_VOQ_VALID_SUPPORT
                ret =  aal_puc_voq_valid_set(device_id, queue_cfg.queue_id , queue_cfg.queue_ena);
                CA_RET_VALUE_CHECK(ret);
#endif

                if(queue_cfg.queue_id <= 63){
                    /***Jianguang li change PUC VOQ to  **/
#if 0
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
                    ret = scfg_read(device_id, CFG_ID_PON_UPSTREAM_USE_DEEP_QUEUE,
                    sizeof(ca_uint8_t), (ca_uint8_t *)&upstream_use_dq);
#endif
                    if (ret != CA_E_OK)
                    {
                        ca_printf("CFG_ID_PON_UPSTREAM_USE_DEEP_QUEUE get failed. ret = %d\r\n", ret);
                    }
#endif
                    if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_4Q){

                    }else if ((puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP) || (puc_voq_map_mode == AAL_PUC_VOQ_MODE_8Q)){
                    /*jianguang changes to support deep queue for LDPID 0x20 ~0x27*/
                        if(upstream_use_dq == 0){
                            tqmvoqid = (queue_cfg.queue_id&0x07);
                        }else{
                            tqmvoqid = queue_cfg.queue_id;
                        }
                    }
                    ret =  aal_puc_voq_bp_remap_data_set(device_id, queue_cfg.queue_id, tqmvoqid);
                    CA_RET_VALUE_CHECK(ret);
                }
            }
        }

        us_omci_hdr_msk.u32 = 0;
        memset(&us_omci_hdr_cfg, 0 , sizeof(us_omci_hdr_cfg));
        us_omci_hdr_msk.s.enable_data_pkt_replacement = 1;
        us_omci_hdr_msk.s.enable_replacement = 1;
        us_omci_hdr_msk.s.cos_replacement = 1;
        us_omci_hdr_msk.s.gemid_replacement = 1;
        us_omci_hdr_cfg.enable_data_pkt_replacement = 0;
        us_omci_hdr_cfg.enable_replacement = 1;
        us_omci_hdr_cfg.cos_replacement = 6;

        us_omci_hdr_cfg.gemid_replacement = 6;

        ret = aal_puc_us_omci_hdr_a_cfg_set(device_id,us_omci_hdr_msk,&us_omci_hdr_cfg);
        CA_RET_VALUE_CHECK(ret);

        us_omci_hp_hdr_msk.u32 = 0;
        memset(&us_omci_hp_hdr_cfg, 0 , sizeof(us_omci_hp_hdr_cfg));
        us_omci_hp_hdr_msk.s.gemid_replacement = 1;
        us_omci_hp_hdr_msk.s.cos_replacement = 1;
        us_omci_hp_hdr_cfg.gemid_replacement = 7;
        us_omci_hp_hdr_cfg.cos_replacement   = 7;
        ret = aal_puc_us_omci_hp_hdr_a_cfg_set(device_id, us_omci_hp_hdr_msk, &us_omci_hp_hdr_cfg);
        CA_RET_VALUE_CHECK(ret);
    }

#ifdef RTK_VOQ_VALID_SUPPORT
    ret = aal_puc_voq_valid_set(device_id, 127, TRUE); //special case because use 9th queue
    CA_RET_VALUE_CHECK(ret);

    voq_drop_cfg_mask.u32 = 0;
    memset(&voq_drop_cfg_cfg, 0, sizeof(aal_puc_voq_drop_cfg_t));
    voq_drop_cfg_mask.s.invalid_voqdrop_enable = 1;
    voq_drop_cfg_cfg.invalid_voqdrop_enable = 1;
    ret = aal_puc_voq_drop_set(device_id, voq_drop_cfg_mask, &voq_drop_cfg_cfg);
    CA_RET_VALUE_CHECK(ret);
#endif

#if 0  /*TBD for fpag check!! *///defined(CONFIG_CORTINA_BOARD_SATURN_SFU_TITAN) || defined(CONFIG_CORTINA_BOARD_FPGA)
    /* enable PUC drop for XGE pri traffic test */
    AAL_PON_REG_FIELD_WRITE(PUC_BPCNTL, dropen, 1);
#else
    AAL_PON_REG_FIELD_WRITE(PUC_BPCNTL, dropen, 0);
    AAL_PON_REG_FIELD_WRITE(PUC_BPCNTL, bpen, 1);
    AAL_PON_REG_FIELD_WRITE(PUC_BPCNTL, bpth, 0x100);
#endif

    /* disable all UserTraffic VoQ */
    //ret = aal_puc_enable_voq(device_id, 1);
    //CA_RET_VALUE_CHECK(ret);
/*
    ret = aal_puc_voq_flush();
    CA_RET_VALUE_CHECK(ret);
*/
    btc_cfg_msk.u32 = 0;
    memset(&btc_cfg, 0 , sizeof(btc_cfg));
    btc_cfg_msk.s.pfovrhd = 1;
    btc_cfg_msk.s.wdaligned = 1;
    btc_cfg_msk.s.schmode = 1;
    btc_cfg_msk.s.minrmnwindowsz = 1;
    btc_cfg_msk.s.sch2en = 1;
    if(pon_mac_mode <= ONU_PON_MAC_EPON_BI10G){
        btc_cfg.pfovrhd = 20;
        btc_cfg.wdaligned =0;
        btc_cfg.schmode = AAL_PUC_BTC_SCHMODE_PACKET;
    }
    else if((pon_mac_mode == ONU_PON_MAC_XGSPON)||(pon_mac_mode == ONU_PON_MAC_XGPON1)){
        btc_cfg.pfovrhd = 8;
        btc_cfg.wdaligned =1;
        btc_cfg.schmode = AAL_PUC_BTC_SCHMODE_FRAGMENT;
        btc_cfg.minrmnwindowsz = 0xF;
        btc_cfg.sch2en = 1;
    }
    else if (pon_mac_mode == ONU_PON_MAC_NGPON2) {
        btc_cfg.pfovrhd = 8;
        btc_cfg.wdaligned =1;
        btc_cfg.schmode = AAL_PUC_BTC_SCHMODE_FRAGMENT;
        btc_cfg.minrmnwindowsz = 0xF;
        btc_cfg.sch2en = 1;
    }
    else if (pon_mac_mode == ONU_PON_MAC_GPON) {
        btc_cfg.pfovrhd = 5;
        btc_cfg.wdaligned =0;
        btc_cfg.schmode = AAL_PUC_BTC_SCHMODE_FRAGMENT;
        btc_cfg.minrmnwindowsz = 5;
        btc_cfg.sch2en = 1;
    }
    else if (pon_mac_mode == ONU_PON_MAC_ETHERNET) {
        btc_cfg.pfovrhd = 0x14;
        btc_cfg.wdaligned =0;
        btc_cfg_msk.s.ethmode = 1;
        btc_cfg.ethmode = 1;
        btc_cfg.schmode = AAL_PUC_BTC_SCHMODE_PACKET;
        btc_cfg.minrmnwindowsz = 0x8;
    }

    ret = aal_puc_btc_cfg_set(device_id,btc_cfg_msk,&btc_cfg);
    CA_RET_VALUE_CHECK(ret);

    //set XGPN_PUCINF_CNTR2 clear on read according Donggun comment.
    if (CA_PON_MODE_IS_GPON_FAMILY(aal_pon_mac_mode)){
        AAL_PON_REG_WRITE(0x5040a100,XGPN_PUCIF_CTRL);
    }

    /*puc shaper and ratelimit initial*/
    AAL_PON_REG_FIELD_WRITE(PUC_CTRL, shp_en, 1);
    AAL_PON_REG_FIELD_WRITE(PUC_CTRL, rl_en, 1);
    AAL_PON_REG_FIELD_WRITE(PUC_CTRL1, shpovhd, 20);
    AAL_PON_REG_FIELD_WRITE(PUC_CTRL1, rlovhd, 20);
    AAL_PON_REG_FIELD_WRITE(PUC_CTRL1, agrshpovhd, 20);

    if (!puc_init_done)
        ca_printf("ca-puc: initialization was done.\n");

    puc_init_done = 1;

    return ret;
}
ca_status_t aal_puc_reset(ca_device_id_t  device_id)
{
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, puc_reset, 0);
    ca_mdelay(5);
    AAL_GLB_REG_FIELD_WRITE(GLOBAL_PON_CNTL, puc_reset, 1);
    ca_mdelay(5);
    return CA_E_OK;
}

EXPORT_SYMBOL(aal_puc_voq_flush_by_idx );
EXPORT_SYMBOL(aal_puc_debug_set );
EXPORT_SYMBOL(aal_puc_debug_get );
EXPORT_SYMBOL(aal_puc_port_voq_map_set );
EXPORT_SYMBOL(aal_puc_port_voqs_map_get );
EXPORT_SYMBOL(aal_puc_port_voq_map_get );
EXPORT_SYMBOL(aal_puc_voq_pkt_len_hp_mem_set );
EXPORT_SYMBOL(aal_puc_voq_pkt_len_hp_mem_get );
EXPORT_SYMBOL(aal_puc_bcmem_for_all_voq_get );
EXPORT_SYMBOL(aal_puc_voq_byte_cnt_mem_get );
EXPORT_SYMBOL(aal_puc_tqmem_for_all_voq_get );
EXPORT_SYMBOL(aal_puc_voq_total_queue_len_mem_get );
EXPORT_SYMBOL(aal_puc_port_open_pkt_mem_set );
EXPORT_SYMBOL(aal_puc_port_open_pkt_mem_get );
EXPORT_SYMBOL(aal_puc_port_voq_sche_info_tbl_set );
EXPORT_SYMBOL(aal_puc_port_voq_sche_info_tbl_get );
EXPORT_SYMBOL(aal_puc_voq_sche_cfg_get );
EXPORT_SYMBOL(aal_puc_voq_sche_cfg_set );
EXPORT_SYMBOL(aal_puc_pvtbl_enable_get );
EXPORT_SYMBOL(aal_puc_pvtbl_enable_set );
EXPORT_SYMBOL(aal_puc_enable_voq );
EXPORT_SYMBOL(aal_puc_voq_map_cfg_get );
EXPORT_SYMBOL(aal_puc_voq_map_cfg_set );
EXPORT_SYMBOL(aal_puc_ctrl_cfg_get );
EXPORT_SYMBOL(aal_puc_ctrl_cfg_set );
EXPORT_SYMBOL(aal_puc_btc_cfg_get );
EXPORT_SYMBOL(aal_puc_btc_cfg_set );
EXPORT_SYMBOL(aal_puc_rpt_cfg_get );
EXPORT_SYMBOL(aal_puc_rpt_cfg_set );
EXPORT_SYMBOL(aal_puc_bm_stat_get );
EXPORT_SYMBOL(aal_puc_bm_stat_set );
EXPORT_SYMBOL(aal_puc_cfg_get );
EXPORT_SYMBOL(aal_puc_cfg_set );
EXPORT_SYMBOL(aal_puc_bm_dbg_fb_cnt_get );
EXPORT_SYMBOL(aal_puc_sts_get );
EXPORT_SYMBOL(aal_puc_epon_queue_thrshld12_get );
EXPORT_SYMBOL(aal_puc_epon_queue_thrshld12_set );
EXPORT_SYMBOL(aal_puc_epon_queue_thrshld34_get );
EXPORT_SYMBOL(aal_puc_epon_queue_thrshld34_set );
EXPORT_SYMBOL(aal_puc_bmc_length_error_get );
EXPORT_SYMBOL(aal_puc_bmc_fifo_sts_get );
EXPORT_SYMBOL(aal_puc_bmc_omci_pkt_cnt_get );
EXPORT_SYMBOL(aal_puc_glb_lnk_type_get );
EXPORT_SYMBOL(aal_puc_glb_lnk_type_set );
EXPORT_SYMBOL(aal_puc_glb_mac_type_get );
EXPORT_SYMBOL(aal_puc_glb_mac_type_set );
EXPORT_SYMBOL(aal_puc_glb_da_sa_get );
EXPORT_SYMBOL(aal_puc_glb_da_sa_set );
EXPORT_SYMBOL(aal_puc_us_omci_hdr_a_cfg_get );
EXPORT_SYMBOL(aal_puc_us_omci_hdr_a_cfg_set );
EXPORT_SYMBOL(aal_puc_us_omci_hp_hdr_a_cfg_get);
EXPORT_SYMBOL(aal_puc_us_omci_hp_hdr_a_cfg_set);
EXPORT_SYMBOL(aal_puc_glb_ploam_cfg_get );
EXPORT_SYMBOL(aal_puc_glb_ploam_cfg_set );
EXPORT_SYMBOL(aal_puc_ds_ploam_hdra_cfg1_get );
EXPORT_SYMBOL(aal_puc_ds_ploam_hdra_cfg1_set );
EXPORT_SYMBOL(aal_puc_ds_ploam_hdra_cfg0_get );
EXPORT_SYMBOL(aal_puc_ds_ploam_hdra_cfg0_set );
EXPORT_SYMBOL(aal_puc_bmc_ctl_pkt_cnt_get );
EXPORT_SYMBOL(aal_puc_bmc_pkt_hdr1_get );
EXPORT_SYMBOL(aal_puc_bmc_pkt_hdr0_get );
EXPORT_SYMBOL(aal_puc_bmc_mtu_get );
EXPORT_SYMBOL(aal_puc_bmc_mtu_set );
EXPORT_SYMBOL(aal_puc_voq_valid_get );
EXPORT_SYMBOL(aal_puc_voq_valid_set );

EXPORT_SYMBOL(aal_puc_voq_tbc_mem_get );
EXPORT_SYMBOL(aal_puc_voq_tbc_mem_set );
EXPORT_SYMBOL(aal_puc_rl_voq_tbc_mem_set );
EXPORT_SYMBOL(aal_puc_rl_voq_tbc_mem_get );
EXPORT_SYMBOL(aal_puc_rl_voq_dcnt_mem_set );
EXPORT_SYMBOL(aal_puc_rl_voq_dcnt_mem_get );
EXPORT_SYMBOL(aal_puc_agr_shp_ctrl_get );
EXPORT_SYMBOL(aal_puc_agr_shp_ctrl_set );
EXPORT_SYMBOL(aal_puc_agr_shp_sts_get );
EXPORT_SYMBOL(aal_puc_tb_ctrl_get );
EXPORT_SYMBOL(aal_puc_tb_ctrl_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc1_0_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc1_0_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc1_1_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc1_1_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc1_2_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc1_2_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc1_2p_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc1_2p_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc2_0_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc2_0_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc2_1_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc2_1_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc2b_0_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc2b_0_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc2b_1_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc2b_1_set );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc3_get );
EXPORT_SYMBOL(aal_puc_sram_test_ctrl_puc3_set );


EXPORT_SYMBOL(aal_puc_pvt_cfg_get );
EXPORT_SYMBOL(aal_puc_pvt_cfg_set );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_sel1500_get );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_sel1500_set );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_sel3116_get );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_sel3116_set );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_sel4732_get );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_sel4732_set );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_sel6348_get );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_sel6348_set );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_a_get );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_a_set );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_b_get );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_b_set );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_c_get );
EXPORT_SYMBOL(aal_puc_voq_buf_lmt_c_set );
EXPORT_SYMBOL(aal_puc_voq_flush);
EXPORT_SYMBOL(aal_puc_voq_flush_get );
EXPORT_SYMBOL(aal_puc_voq_flush_set );
EXPORT_SYMBOL(aal_puc_voq_drop_get );
EXPORT_SYMBOL(aal_puc_voq_drop_set );
EXPORT_SYMBOL(aal_puc_voq_bpremap_get );
EXPORT_SYMBOL(aal_puc_voq_bp_remap_data_get );
EXPORT_SYMBOL(aal_puc_voq_bp_remap_data_set );
EXPORT_SYMBOL(aal_puc_bm_dbg_fbpa_cnt_get );
EXPORT_SYMBOL(aal_puc_bm_dbg_fbpb_cnt_get );
EXPORT_SYMBOL(aal_puc_voq_stats_get);
EXPORT_SYMBOL(aal_puc_ntt_cfg_set);
EXPORT_SYMBOL(aal_puc_ntt_cfg_get);
EXPORT_SYMBOL(aal_puc_rpt_adj_cfg1_set);
EXPORT_SYMBOL(aal_puc_rpt_adj_cfg1_get);
EXPORT_SYMBOL(aal_puc_voq_len_get);
EXPORT_SYMBOL(aal_puc_tcont_rem_buffers_get);
EXPORT_SYMBOL(aal_puc_reset );

EXPORT_SYMBOL(aal_puc_bmc_pkt_hdr_get );

#endif /* CONFIG_NE_PON */

