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
#include "ca_types.h"
#include "osal_common.h"
#include "aal_common.h"
#include "aal_uni_macsec.h"
#include "macsec.h"
#include "hw_macsec.h"
#include "ca-scfg/scfg.h"
#include "util_aes.h"

#ifdef CONFIG_NE_PON
#include "aal_epon_crypto.h"
#include "aal_mpcp.h"
#include "aal_pon.h"
#endif
ca_uint8_t  epon_current_an = 0;
ca_uint8_t  uni_current_an = 0;

ca_macsec_sa_t global_sa_table[MACSEC_SA_NODE_NUM];
ca_sa_entry_t  rx_sa_entry_table[MACSEC_SA_NODE_NUM];
ca_sa_entry_t  tx_sa_entry_table[MACSEC_SA_NODE_NUM];
static ca_boolean_t init_flag = FALSE;

static ca_uint macsec_lock;

static ca_status_t ca_macsec_rx_sa_context_set(
	CA_IN ca_device_id_t			device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_macsec_sa_t		    *sa_context)
{
	ca_status_t ret = CA_E_OK;
	ca_uint8_t data[16] ={0};
	ca_uint8_t rx_hash0[16] ={0};
	ca_uint8_t rx_hash1[16] ={0};

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_uni_macsec_tbl_cfg_msk_t tbl_msk;
	aal_uni_macsec_tbl_cfg_t tbl_cfg;
	aal_uni_macsec_nandi_cfg_msk_t cfg_msk;
	aal_uni_macsec_nandi_cfg_t cfg;
	aal_uni_macsec_sa_cfg_msk_t sa_msk;
	aal_uni_macsec_sa_cfg_t sa_cfg;
#endif

#ifdef CONFIG_NE_PON
	aal_epon_crypto_nandi_cfg_msk_t epon_nandi_cfg_msk;
	aal_epon_crypto_nandi_cfg_t epon_nandi_cfg;

	aal_epon_crypto_xg_tbl_msk_t epon_macsec_sa_tbl_cfg_msk;
	aal_epon_crypto_xg_tbl_field_t epon_macsec_sa_tbl_cfg;
	aal_epon_crypto_cfg_msk_t epon_glb_coresec_cfg_msk;
	aal_epon_crypto_cfg_t     epon_glb_coresec_cfg;
	aal_epon_crypto_macsec_sa_cfg_msk_t epon_macsec_sa_cfg_msk;
	aal_epon_crypto_macsec_sa_cfg_t epon_macsec_sa_cfg;
	int i, j;
#endif
    if(sa_context->direction != CA_MACSEC_RX_DIR)
    {
        ca_printf("%s %d: ERROR! SA Context is for RX, but parameter doesn't match.\n", __func__, __LINE__);
        return CA_E_PARAM;
    }

	if((port == 0x5)||(port == 0x6))
	{//UNI port
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        uni_current_an = sa_context->association_number;
        //calcultate hash key
        if(sa_context->sa_db.mask.key0_valid){
            ret = aes128_ecb_encrypt(sa_context->sa_db.key0, data, rx_hash0);
            CA_RET_VALUE_CHECK(ret);
        }
        if(sa_context->sa_db.mask.key1_valid){
            ret = aes128_ecb_encrypt(sa_context->sa_db.key1, data, rx_hash1);
            CA_RET_VALUE_CHECK(ret);
        }

		tbl_msk.u32 = 0;
		memset(&tbl_cfg, 0, sizeof(aal_uni_macsec_tbl_cfg_t));

		if(sa_context->sa_db.mask.key0_valid)
		{
			tbl_msk.s.key0 = 1;
			tbl_msk.s.key0_valid = 1;
			tbl_cfg.key0_valid = 1;
			memcpy(tbl_cfg.key0,sa_context->sa_db.key0,AAL_MACSEC_KEY_LEN);

            tbl_msk.s.hash0 = 1;
			tbl_msk.s.hash0_valid = 1;
			tbl_cfg.hash0_valid = 1;
			memcpy(tbl_cfg.hash0,rx_hash0,AAL_MACSEC_KEY_LEN);
		}

		if(sa_context->sa_db.mask.key1_valid)
		{
			tbl_msk.s.key1 = 1;
			tbl_msk.s.key1_valid = 1;
			tbl_cfg.key1_valid = 1;
			memcpy(tbl_cfg.key1,sa_context->sa_db.key1,AAL_MACSEC_KEY_LEN);

            tbl_msk.s.hash1 = 1;
			tbl_msk.s.hash1_valid = 1;
			tbl_cfg.hash1_valid = 1;
			memcpy(tbl_cfg.hash1,rx_hash1,AAL_MACSEC_KEY_LEN);
		}

		if(sa_context->sa_db.mask.lapn_valid)
		{
			tbl_msk.s.pn = 1;
			tbl_msk.s.pn_valid = 1;
			tbl_cfg.pn_valid = 1;
			tbl_cfg.pn = sa_context->sa_db.lapn;
		}
		if(sa_context->sa_db.mask.npn_valid)
		{
			tbl_msk.s.npn = 1;
			tbl_msk.s.npn_valid = 1;
			tbl_cfg.npn_valid = 1;
			tbl_cfg.npn = sa_context->sa_db.npn;
		}
		if(sa_context->sa_db.mask.sa_db_control_flags_valid)
		{
			tbl_msk.s.an_valid			= 1;
			tbl_msk.s.co_valid			= 1;
			tbl_msk.s.es_valid			= 1;
			tbl_msk.s.v_config_valid	= 1;
			tbl_msk.s.rlyen_valid		= 1;
			tbl_msk.s.sc_valid			= 1;
			tbl_msk.s.scb_valid 		= 1;
			tbl_msk.s.sec_mode_valid	= 1;
			tbl_msk.s.an			= 1;
			tbl_msk.s.co			= 1;
			tbl_msk.s.es			= 1;
			tbl_msk.s.v_config		= 1;
			tbl_msk.s.rlyen 		= 1;
			tbl_msk.s.sc			= 1;
			tbl_msk.s.scb			= 1;
			tbl_msk.s.sec_mode		= 1;
			tbl_cfg.an_valid		= 1;
			tbl_cfg.co_valid		= 1;
			tbl_cfg.es_valid		= 1;
			tbl_cfg.v_config_valid	= 1;
			tbl_cfg.rlyen_valid 	= 1;
			tbl_cfg.sc_valid		= 1;
			tbl_cfg.scb_valid		= 1;
			tbl_cfg.sec_mode_valid	= 1;
			tbl_cfg.an = sa_context->sa_db.sa_db_control_flags.association_number;
			tbl_cfg.co = sa_context->sa_db.sa_db_control_flags.confidential_offset;
			tbl_cfg.es = sa_context->sa_db.sa_db_control_flags.end_station;
			tbl_cfg.v_config = sa_context->sa_db.sa_db_control_flags.frame_validation;
			tbl_cfg.rlyen = sa_context->sa_db.sa_db_control_flags.replay_check;
			tbl_cfg.sc = sa_context->sa_db.sa_db_control_flags.sci_encoded;
			tbl_cfg.scb = sa_context->sa_db.sa_db_control_flags.single_copy_broadcast;
			tbl_cfg.sec_mode = sa_context->sa_db.sa_db_control_flags.security_mode;
        }

		cfg_msk.u32 = 0;
		memset(&cfg, 0, sizeof(aal_uni_macsec_nandi_cfg_t));
		if(sa_context->sa_db.mask.sci_valid)
		{
			cfg_msk.s.sci0 = 1 ;
			cfg_msk.s.sci1 = 1 ;
			cfg.sci0[0] = (sa_context->sa_db.sci & 0xFF);
			cfg.sci0[1] = (sa_context->sa_db.sci >> 8 ) & 0xFF;
			cfg.sci0[2] = (sa_context->sa_db.sci >> 16 ) & 0xFF;
			cfg.sci0[3] = (sa_context->sa_db.sci >> 24 ) & 0xFF;

			cfg.sci1[0] = (sa_context->sa_db.sci >> 32 ) & 0xFF;
			cfg.sci1[1] = (sa_context->sa_db.sci >> 40 ) & 0xFF;
			cfg.sci1[2] = (sa_context->sa_db.sci >> 48 ) & 0xFF;
			cfg.sci1[3] = (sa_context->sa_db.sci >> 56 ) & 0xFF;

		}
		if(sa_context->sa_db.mask.pn_threshold_valid)
		{
			cfg_msk.s.pn_threshold = 1 ;
			cfg.pn_threshold = sa_context->sa_db.pn_threshold;
		}
		if(sa_context->sa_db.mask.replay_window_valid)
		{
			cfg_msk.s.replay_window = 1 ;
			cfg.replay_window = sa_context->sa_db.replay_window;
		}

		if(sa_context->sa_db.mask.mac_addr)
		{
			memset(&sa_cfg, 0, sizeof(aal_uni_macsec_sa_cfg_t));
			sa_msk.u32 = ~0U;
			sa_cfg.sel_local_sa_reg = 1;
			sa_cfg.local_sa_reg1 = ((ca_uint32_t)(sa_context->sa_db.mac_addr[0]) << 8)
									| ((ca_uint32_t)(sa_context->sa_db.mac_addr[1]));
			sa_cfg.local_sa_reg0 = ((ca_uint32_t)(sa_context->sa_db.mac_addr[2]) << 24)
									| ((ca_uint32_t)(sa_context->sa_db.mac_addr[3]) << 16)
									| ((ca_uint32_t)(sa_context->sa_db.mac_addr[4]) << 8)
									| ((ca_uint32_t)(sa_context->sa_db.mac_addr[5]));
			aal_uni_macsec_rx_sa_cfg_set(device_id, sa_msk, &sa_cfg);
			CA_RET_VALUE_CHECK(ret);
		}

		ret = aal_uni_macsec_rx_nandi_cfg_set(device_id,cfg_msk,&cfg);
		CA_RET_VALUE_CHECK(ret);

		ret = aal_uni_macsec_rx_sadb_tbl_set(device_id, sa_context->sa_id, tbl_msk,&tbl_cfg);
		CA_RET_VALUE_CHECK(ret);
#endif
	}else if(port == 0x7){//EPON port
#ifdef CONFIG_NE_PON
		CA_PON_MAC_IS_10GEPON();
        epon_current_an = sa_context->association_number;

        //calcultate hash key
        if(sa_context->sa_db.mask.key0_valid){
            ret = aes128_ecb_encrypt(sa_context->sa_db.key0, data, rx_hash0);
            CA_RET_VALUE_CHECK(ret);
        }
        if(sa_context->sa_db.mask.key1_valid){
            ret = aes128_ecb_encrypt(sa_context->sa_db.key1, data, rx_hash1);
            CA_RET_VALUE_CHECK(ret);
        }

		epon_macsec_sa_tbl_cfg_msk.u32 = 0;
		memset(&epon_macsec_sa_tbl_cfg, 0, sizeof(aal_epon_crypto_xg_tbl_field_t));

        if(sa_context->sa_db.mask.key0_valid)
        {
            epon_macsec_sa_tbl_cfg_msk.s.key0 = 1;
            epon_macsec_sa_tbl_cfg_msk.s.hash0 = 1;
    		for (i = 0,j = 3; i < AAL_MACSEC_KEY_LEN/4 ; i++,j--)
    		{

    			epon_macsec_sa_tbl_cfg.key0[j] = ((ca_uint32_t)(sa_context->sa_db.key0[i*4]) << 24 )
    											|((ca_uint32_t)(sa_context->sa_db.key0[i*4 + 1]) << 16)
    											|((ca_uint32_t)(sa_context->sa_db.key0[i*4 + 2]) << 8)
    											|((ca_uint32_t)(sa_context->sa_db.key0[i*4 + 3]));
    			epon_macsec_sa_tbl_cfg.hash0[j] = ((ca_uint32_t)(rx_hash0[i*4]) << 24 )
    											|((ca_uint32_t)(rx_hash0[i*4 + 1]) << 16)
    											|((ca_uint32_t)(rx_hash0[i*4 + 2]) << 8)
    											|((ca_uint32_t)(rx_hash0[i*4 + 3]));
    		}
        }

        if(sa_context->sa_db.mask.key1_valid)
        {
            epon_macsec_sa_tbl_cfg_msk.s.key1 = 1;
            epon_macsec_sa_tbl_cfg_msk.s.hash1 = 1;

    		for (i = 0,j = 3; i < AAL_MACSEC_KEY_LEN/4 ; i++,j--)
    		{
    			epon_macsec_sa_tbl_cfg.key1[j] = ((ca_uint32_t)(sa_context->sa_db.key1[i*4]) << 24 )
    											|((ca_uint32_t)(sa_context->sa_db.key1[i*4 + 1]) << 16)
    											|((ca_uint32_t)(sa_context->sa_db.key1[i*4 + 2]) << 8)
    											|((ca_uint32_t)(sa_context->sa_db.key1[i*4 + 3]));
    			epon_macsec_sa_tbl_cfg.hash1[j] = ((ca_uint32_t)(rx_hash1[i*4]) << 24 )
    											|((ca_uint32_t)(rx_hash1[i*4 + 1]) << 16)
    											|((ca_uint32_t)(rx_hash1[i*4 + 2]) << 8)
    											|((ca_uint32_t)(rx_hash1[i*4 + 3]));
    		}
        }

        if(sa_context->sa_db.mask.lapn_valid){
            epon_macsec_sa_tbl_cfg_msk.s.pn = 1;
		    epon_macsec_sa_tbl_cfg.pn		= sa_context->sa_db.lapn	;
	    }
        if(sa_context->sa_db.mask.npn_valid){
            epon_macsec_sa_tbl_cfg_msk.s.npn = 1;
            epon_macsec_sa_tbl_cfg.npn		= sa_context->sa_db.npn  ;
        }
        if(sa_context->sa_db.mask.sa_db_control_flags_valid)
		{
            epon_macsec_sa_tbl_cfg_msk.s.an = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.sc = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.scb = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.es = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.co = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.rlyen = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.sec_mode = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.v_config = 1 ;
    		epon_macsec_sa_tbl_cfg.an		= sa_context->sa_db.sa_db_control_flags.association_number	;
            epon_macsec_sa_tbl_cfg.sc		= sa_context->sa_db.sa_db_control_flags.sci_encoded   ;
    		epon_macsec_sa_tbl_cfg.scb		= sa_context->sa_db.sa_db_control_flags.single_copy_broadcast	   ;
    		epon_macsec_sa_tbl_cfg.es		= sa_context->sa_db.sa_db_control_flags.end_station   ;
    		epon_macsec_sa_tbl_cfg.co		= sa_context->sa_db.sa_db_control_flags.confidential_offset   ;
    		epon_macsec_sa_tbl_cfg.rlyen	= sa_context->sa_db.sa_db_control_flags.replay_check ;
    		epon_macsec_sa_tbl_cfg.sec_mode = sa_context->sa_db.sa_db_control_flags.security_mode;
    		epon_macsec_sa_tbl_cfg.v_config = sa_context->sa_db.sa_db_control_flags.frame_validation;
        }

        ret = aal_epon_crypto_xg_sa_tbl_set(device_id, port, sa_context->sa_id, AAL_EPON_CRYPTO_DIRECTION_RX,epon_macsec_sa_tbl_cfg_msk, &epon_macsec_sa_tbl_cfg);
		CA_RET_VALUE_CHECK(ret);

		epon_nandi_cfg_msk.u32 = 0;
		memset(&epon_nandi_cfg, 0, sizeof(aal_epon_crypto_nandi_cfg_t));
        if(sa_context->sa_db.mask.sci_valid){
            epon_nandi_cfg_msk.s.sci0 = 1 ;
			epon_nandi_cfg_msk.s.sci1 = 1 ;
			epon_nandi_cfg.sci0[0] = (sa_context->sa_db.sci & 0xFF);
			epon_nandi_cfg.sci0[1] = (sa_context->sa_db.sci >> 8 ) & 0xFF;
			epon_nandi_cfg.sci0[2] = (sa_context->sa_db.sci >> 16 ) & 0xFF;
			epon_nandi_cfg.sci0[3] = (sa_context->sa_db.sci >> 24 ) & 0xFF;

			epon_nandi_cfg.sci1[0] = (sa_context->sa_db.sci >> 32 ) & 0xFF;
			epon_nandi_cfg.sci1[1] = (sa_context->sa_db.sci >> 40 ) & 0xFF;
			epon_nandi_cfg.sci1[2] = (sa_context->sa_db.sci >> 48 ) & 0xFF;
			epon_nandi_cfg.sci1[3] = (sa_context->sa_db.sci >> 56 ) & 0xFF;
        }

        if(sa_context->sa_db.mask.pn_threshold_valid){
            epon_nandi_cfg_msk.s.pn_threshold = 1 ;
            epon_nandi_cfg.pn_threshold =sa_context->sa_db.pn_threshold ;
        }

        if(sa_context->sa_db.mask.replay_window_valid){
            epon_nandi_cfg_msk.s.replay_window = 1 ;
            epon_nandi_cfg.replay_window =sa_context->sa_db.replay_window;
        }

        ret = aal_epon_crypto_nandi_cfg_set(device_id,port, AAL_EPON_CRYPTO_DIRECTION_RX, epon_nandi_cfg_msk, &epon_nandi_cfg);
		CA_RET_VALUE_CHECK(ret);

        if(sa_context->sa_db.mask.mac_addr){
    		ret = aal_mpcp_pca_olt_mac_addr_set(device_id, port, sa_context->sa_db.mac_addr);
    		CA_RET_VALUE_CHECK(ret);

            epon_macsec_sa_cfg_msk.u32 = ~0U;
            memset(&epon_macsec_sa_cfg, 0, sizeof(aal_epon_crypto_macsec_sa_cfg_t));

            epon_macsec_sa_cfg.sel_local_sa_reg = 1;
            epon_macsec_sa_cfg.local_sa_reg1 = ((ca_uint32_t)(sa_context->sa_db.mac_addr[0]) << 8)
                                                | ((ca_uint32_t)(sa_context->sa_db.mac_addr[1]));
            epon_macsec_sa_cfg.local_sa_reg0 = ((ca_uint32_t)(sa_context->sa_db.mac_addr[2]) << 24)
                                                | ((ca_uint32_t)(sa_context->sa_db.mac_addr[3]) << 16)
                                                | ((ca_uint32_t)(sa_context->sa_db.mac_addr[4]) << 8)
                                                | ((ca_uint32_t)(sa_context->sa_db.mac_addr[5]));

            ret = aal_epon_crypto_macsec_local_sa_set(device_id,AAL_EPON_CRYPTO_DIRECTION_RX, epon_macsec_sa_cfg_msk,&epon_macsec_sa_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
#endif
	}
	return ret;
}


static ca_status_t ca_macsec_tx_sa_context_set(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t					port,
	CA_IN ca_uint16_t                   llid,
	CA_IN ca_macsec_sa_t			    *sa_context)
{
	ca_status_t ret = CA_E_OK;
	ca_uint8_t data[16] ={0};
	ca_uint8_t tx_hash0[16] ={0};
	ca_uint8_t tx_hash1[16] ={0};

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_uni_macsec_tbl_cfg_msk_t tbl_msk;
	aal_uni_macsec_tbl_cfg_t tbl_cfg;
	aal_uni_macsec_nandi_cfg_msk_t cfg_msk;
	aal_uni_macsec_nandi_cfg_t cfg;
	aal_uni_macsec_sa_cfg_msk_t sa_msk;
	aal_uni_macsec_sa_cfg_t sa_cfg;
#endif

#ifdef CONFIG_NE_PON
	aal_epon_crypto_nandi_cfg_msk_t epon_nandi_cfg_msk;
	aal_epon_crypto_nandi_cfg_t epon_nandi_cfg;

	aal_epon_crypto_xg_tbl_msk_t epon_macsec_sa_tbl_cfg_msk;
	aal_epon_crypto_xg_tbl_field_t epon_macsec_sa_tbl_cfg;
	aal_epon_crypto_cfg_msk_t epon_glb_coresec_cfg_msk;
	aal_epon_crypto_cfg_t     epon_glb_coresec_cfg;
	aal_epon_crypto_macsec_sa_cfg_msk_t epon_macsec_sa_cfg_msk;
	aal_epon_crypto_macsec_sa_cfg_t epon_macsec_sa_cfg;
	int i, j;
	ca_uint8_t llid_idx = 0;
#endif
	//CA_MACSEC_SA_ID_CHECK(sa_context->sa_id);

	if(sa_context->direction != CA_MACSEC_TX_DIR)
	{
		ca_printf("%s: ERROR! SA Context is for TX, but parameter doesn't match.\n", __func__);
		return CA_E_PARAM;
	}

	if((port == 0x5)||(port == 0x6))
	{//UNI port
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        //calcultate hash key
        if(sa_context->sa_db.mask.key0_valid){
            ret = aes128_ecb_encrypt(sa_context->sa_db.key0, data, tx_hash0);
            CA_RET_VALUE_CHECK(ret);
        }
        if(sa_context->sa_db.mask.key1_valid){
            ret = aes128_ecb_encrypt(sa_context->sa_db.key1, data, tx_hash1);
            CA_RET_VALUE_CHECK(ret);
        }

		tbl_msk.u32 = 0;
		memset(&tbl_cfg, 0, sizeof(aal_uni_macsec_tbl_cfg_t));

		if(sa_context->sa_db.mask.key0_valid)
		{
			tbl_msk.s.key0 = 1;
			tbl_msk.s.key0_valid = 1;
			tbl_cfg.key0_valid = 1;
			memcpy(tbl_cfg.key0,sa_context->sa_db.key0,AAL_MACSEC_KEY_LEN);

			tbl_msk.s.hash0 = 1;
			tbl_msk.s.hash0_valid = 1;
			tbl_cfg.hash0_valid = 1;
			memcpy(tbl_cfg.hash0,tx_hash0,AAL_MACSEC_KEY_LEN);
		}

		if(sa_context->sa_db.mask.key1_valid)
		{
			tbl_msk.s.key1 = 1;
			tbl_msk.s.key1_valid = 1;
			tbl_cfg.key1_valid = 1;
			memcpy(tbl_cfg.key1,sa_context->sa_db.key1,AAL_MACSEC_KEY_LEN);

			tbl_msk.s.hash1 = 1;
			tbl_msk.s.hash1_valid = 1;
			tbl_cfg.hash1_valid = 1;
			memcpy(tbl_cfg.hash1,tx_hash1,AAL_MACSEC_KEY_LEN);
		}

		if(sa_context->sa_db.mask.lapn_valid)
		{
			tbl_msk.s.pn = 1;
			tbl_msk.s.pn_valid = 1;
			tbl_cfg.pn_valid = 1;
			tbl_cfg.pn = sa_context->sa_db.lapn;
		}
		if(sa_context->sa_db.mask.npn_valid)
		{
			tbl_msk.s.npn = 1;
			tbl_msk.s.npn_valid = 1;
			tbl_cfg.npn_valid = 1;
			tbl_cfg.npn = sa_context->sa_db.npn;
		}
		if(sa_context->sa_db.mask.sa_db_control_flags_valid)
		{
			tbl_msk.s.an_valid			= 1;
			tbl_msk.s.co_valid			= 1;
			tbl_msk.s.es_valid			= 1;
			tbl_msk.s.v_config_valid	= 1;
			tbl_msk.s.rlyen_valid		= 1;
			tbl_msk.s.sc_valid			= 1;
			tbl_msk.s.scb_valid 		= 1;
			tbl_msk.s.sec_mode_valid	= 1;

			tbl_msk.s.an = 1;
			tbl_msk.s.co = 1;
			tbl_msk.s.es = 1;
			tbl_msk.s.v_config = 1;
			tbl_msk.s.rlyen = 1;
			tbl_msk.s.sc = 1;
			tbl_msk.s.scb = 1;
			tbl_msk.s.sec_mode = 1;

			tbl_cfg.an_valid = 1;
			tbl_cfg.co_valid = 1;
			tbl_cfg.es_valid = 1;
			tbl_cfg.v_config_valid = 1;
			tbl_cfg.rlyen_valid = 1;
			tbl_cfg.sc_valid = 1;
			tbl_cfg.scb_valid = 1;
			tbl_cfg.sec_mode_valid = 1;

			tbl_cfg.an = sa_context->sa_db.sa_db_control_flags.association_number;
			tbl_cfg.co = sa_context->sa_db.sa_db_control_flags.confidential_offset;
			tbl_cfg.es = sa_context->sa_db.sa_db_control_flags.end_station;
			tbl_cfg.v_config = sa_context->sa_db.sa_db_control_flags.frame_validation;
			tbl_cfg.rlyen = sa_context->sa_db.sa_db_control_flags.replay_check;
			tbl_cfg.sc = sa_context->sa_db.sa_db_control_flags.sci_encoded;
			tbl_cfg.scb = sa_context->sa_db.sa_db_control_flags.single_copy_broadcast;
			tbl_cfg.sec_mode = sa_context->sa_db.sa_db_control_flags.security_mode;
            REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG, macsec_sectag_type, sa_context->sa_db.sa_db_control_flags.sci_encoded);

        }

		cfg_msk.u32 = 0;
		memset(&cfg, 0, sizeof(aal_uni_macsec_nandi_cfg_t));
		if(sa_context->sa_db.mask.sci_valid)
		{
			cfg_msk.s.sci0 = 1 ;
			cfg_msk.s.sci1 = 1 ;
			cfg.sci0[0] = (sa_context->sa_db.sci & 0xFF);
			cfg.sci0[1] = (sa_context->sa_db.sci >> 8 ) & 0xFF;
			cfg.sci0[2] = (sa_context->sa_db.sci >> 16 ) & 0xFF;
			cfg.sci0[3] = (sa_context->sa_db.sci >> 24 ) & 0xFF;

			cfg.sci1[0] = (sa_context->sa_db.sci >> 32 ) & 0xFF;
			cfg.sci1[1] = (sa_context->sa_db.sci >> 40 ) & 0xFF;
			cfg.sci1[2] = (sa_context->sa_db.sci >> 48 ) & 0xFF;
			cfg.sci1[3] = (sa_context->sa_db.sci >> 56 ) & 0xFF;
		}
		if(sa_context->sa_db.mask.pn_threshold_valid)
		{
			cfg_msk.s.pn_threshold = 1 ;
			cfg.pn_threshold = sa_context->sa_db.pn_threshold;
		}
		if(sa_context->sa_db.mask.replay_window_valid)
		{
			cfg_msk.s.replay_window = 1 ;
			cfg.replay_window = sa_context->sa_db.replay_window;
		}

		if(sa_context->sa_db.mask.mac_addr)
		{
			memset(&sa_cfg, 0, sizeof(aal_uni_macsec_sa_cfg_t));
			sa_msk.u32 = ~0U;
			sa_cfg.sel_local_sa_reg = 1;
			sa_cfg.local_sa_reg1 = ((ca_uint32_t)(sa_context->sa_db.mac_addr[0]) << 8)
									| ((ca_uint32_t)(sa_context->sa_db.mac_addr[1]));
			sa_cfg.local_sa_reg0 = ((ca_uint32_t)(sa_context->sa_db.mac_addr[2]) << 24)
									| ((ca_uint32_t)(sa_context->sa_db.mac_addr[3]) << 16)
									| ((ca_uint32_t)(sa_context->sa_db.mac_addr[4]) << 8)
									| ((ca_uint32_t)(sa_context->sa_db.mac_addr[5]));
			aal_uni_macsec_tx_sa_cfg_set(device_id, sa_msk, &sa_cfg);
			CA_RET_VALUE_CHECK(ret);
		}

		ret = aal_uni_macsec_tx_nandi_cfg_set(device_id,cfg_msk,&cfg);
		CA_RET_VALUE_CHECK(ret);
		ret = aal_uni_macsec_tx_sadb_tbl_set(device_id, sa_context->sa_id, tbl_msk,&tbl_cfg);
		CA_RET_VALUE_CHECK(ret);
#endif
	}else if(port == 0x7){//EPON port
#ifdef CONFIG_NE_PON
		CA_PON_MAC_IS_10GEPON();
        //calcultate hash key
        if(sa_context->sa_db.mask.key0_valid){
            ret = aes128_ecb_encrypt(sa_context->sa_db.key0, data, tx_hash0);
            CA_RET_VALUE_CHECK(ret);
        }
        if(sa_context->sa_db.mask.key1_valid){
            ret = aes128_ecb_encrypt(sa_context->sa_db.key1, data, tx_hash1);
            CA_RET_VALUE_CHECK(ret);
        }

        ret = aal_mpcp_reg_llid_index_get(device_id, port, llid, &llid_idx);
        CA_RET_VALUE_CHECK(ret);
		epon_macsec_sa_tbl_cfg_msk.u32 = 0;
		memset(&epon_macsec_sa_tbl_cfg, 0, sizeof(aal_epon_crypto_xg_tbl_field_t));
        if(sa_context->sa_db.mask.lapn_valid){
            epon_macsec_sa_tbl_cfg_msk.s.pn = 1;
		    epon_macsec_sa_tbl_cfg.pn		= sa_context->sa_db.lapn	;
	    }
        if(sa_context->sa_db.mask.npn_valid){
            epon_macsec_sa_tbl_cfg_msk.s.npn = 1;
            epon_macsec_sa_tbl_cfg.npn		= sa_context->sa_db.npn  ;
        }
        if(sa_context->sa_db.mask.sa_db_control_flags_valid)
		{
            epon_macsec_sa_tbl_cfg_msk.s.an = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.sc = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.scb = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.es = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.co = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.rlyen = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.sec_mode = 1 ;
            epon_macsec_sa_tbl_cfg_msk.s.v_config = 1 ;
    		epon_macsec_sa_tbl_cfg.an		= sa_context->sa_db.sa_db_control_flags.association_number	;
            epon_macsec_sa_tbl_cfg.sc		= sa_context->sa_db.sa_db_control_flags.sci_encoded   ;
    		epon_macsec_sa_tbl_cfg.scb		= sa_context->sa_db.sa_db_control_flags.single_copy_broadcast	   ;
    		epon_macsec_sa_tbl_cfg.es		= sa_context->sa_db.sa_db_control_flags.end_station   ;
    		epon_macsec_sa_tbl_cfg.co		= sa_context->sa_db.sa_db_control_flags.confidential_offset   ;
    		epon_macsec_sa_tbl_cfg.rlyen	= sa_context->sa_db.sa_db_control_flags.replay_check ;
    		epon_macsec_sa_tbl_cfg.sec_mode = sa_context->sa_db.sa_db_control_flags.security_mode;
    		epon_macsec_sa_tbl_cfg.v_config = sa_context->sa_db.sa_db_control_flags.frame_validation;
        }
        if(sa_context->sa_db.mask.key0_valid)
        {
            epon_macsec_sa_tbl_cfg_msk.s.key0 = 1;
            epon_macsec_sa_tbl_cfg_msk.s.hash0 = 1;
    		for (i = 0,j = 3; i < AAL_MACSEC_KEY_LEN/4 ; i++,j--)
    		{

    			epon_macsec_sa_tbl_cfg.key0[j] = ((ca_uint32_t)(sa_context->sa_db.key0[i*4]) << 24 )
    											|((ca_uint32_t)(sa_context->sa_db.key0[i*4 + 1]) << 16)
    											|((ca_uint32_t)(sa_context->sa_db.key0[i*4 + 2]) << 8)
    											|((ca_uint32_t)(sa_context->sa_db.key0[i*4 + 3]));
    			epon_macsec_sa_tbl_cfg.hash0[j] = ((ca_uint32_t)(tx_hash0[i*4]) << 24 )
    											|((ca_uint32_t)(tx_hash0[i*4 + 1]) << 16)
    											|((ca_uint32_t)(tx_hash0[i*4 + 2]) << 8)
    											|((ca_uint32_t)(tx_hash0[i*4 + 3]));
    		}
        }

        if(sa_context->sa_db.mask.key1_valid)
        {
            epon_macsec_sa_tbl_cfg_msk.s.key1 = 1;
            epon_macsec_sa_tbl_cfg_msk.s.hash1 = 1;

    		for (i = 0,j = 3; i < AAL_MACSEC_KEY_LEN/4 ; i++,j--)
    		{
    			epon_macsec_sa_tbl_cfg.key1[j] = ((ca_uint32_t)(sa_context->sa_db.key1[i*4]) << 24 )
    											|((ca_uint32_t)(sa_context->sa_db.key1[i*4 + 1]) << 16)
    											|((ca_uint32_t)(sa_context->sa_db.key1[i*4 + 2]) << 8)
    											|((ca_uint32_t)(sa_context->sa_db.key1[i*4 + 3]));
    			epon_macsec_sa_tbl_cfg.hash1[j] = ((ca_uint32_t)(tx_hash1[i*4]) << 24 )
    											|((ca_uint32_t)(tx_hash1[i*4 + 1]) << 16)
    											|((ca_uint32_t)(tx_hash1[i*4 + 2]) << 8)
    											|((ca_uint32_t)(tx_hash1[i*4 + 3]));
    		}
        }

        epon_nandi_cfg_msk.u32 = 0;
		memset(&epon_nandi_cfg, 0, sizeof(aal_epon_crypto_nandi_cfg_t));
        if(sa_context->sa_db.mask.sci_valid){
            epon_nandi_cfg_msk.s.sci0 = 1 ;
			epon_nandi_cfg_msk.s.sci1 = 1 ;
			epon_nandi_cfg.sci0[0] = (sa_context->sa_db.sci & 0xFF);
			epon_nandi_cfg.sci0[1] = (sa_context->sa_db.sci >> 8 ) & 0xFF;
			epon_nandi_cfg.sci0[2] = (sa_context->sa_db.sci >> 16 ) & 0xFF;
			epon_nandi_cfg.sci0[3] = (sa_context->sa_db.sci >> 24 ) & 0xFF;

			epon_nandi_cfg.sci1[0] = (sa_context->sa_db.sci >> 32 ) & 0xFF;
			epon_nandi_cfg.sci1[1] = (sa_context->sa_db.sci >> 40 ) & 0xFF;
			epon_nandi_cfg.sci1[2] = (sa_context->sa_db.sci >> 48 ) & 0xFF;
			epon_nandi_cfg.sci1[3] = (sa_context->sa_db.sci >> 56 ) & 0xFF;
        }

        if(sa_context->sa_db.mask.pn_threshold_valid){
            epon_nandi_cfg_msk.s.pn_threshold = 1 ;
            epon_nandi_cfg.pn_threshold =sa_context->sa_db.pn_threshold ;
        }

        if(sa_context->sa_db.mask.replay_window_valid){
            epon_nandi_cfg_msk.s.replay_window = 1 ;
            epon_nandi_cfg.replay_window =sa_context->sa_db.replay_window;
        }

        ret = aal_epon_crypto_nandi_cfg_set(device_id,port, AAL_EPON_CRYPTO_DIRECTION_TX, epon_nandi_cfg_msk, &epon_nandi_cfg);
		CA_RET_VALUE_CHECK(ret);

        if(sa_context->sa_db.mask.mac_addr){
            epon_macsec_sa_cfg_msk.u32 = ~0U;
            memset(&epon_macsec_sa_cfg, 0, sizeof(aal_epon_crypto_macsec_sa_cfg_t));

            epon_macsec_sa_cfg.sel_local_sa_reg = 1;
            epon_macsec_sa_cfg.local_sa_reg1 = ((ca_uint32_t)(sa_context->sa_db.mac_addr[0]) << 8)
                                                | ((ca_uint32_t)(sa_context->sa_db.mac_addr[1]));
            epon_macsec_sa_cfg.local_sa_reg0 = ((ca_uint32_t)(sa_context->sa_db.mac_addr[2]) << 24)
                                                | ((ca_uint32_t)(sa_context->sa_db.mac_addr[3]) << 16)
                                                | ((ca_uint32_t)(sa_context->sa_db.mac_addr[4]) << 8)
                                                | ((ca_uint32_t)(sa_context->sa_db.mac_addr[5]));

            ret = aal_epon_crypto_macsec_local_sa_set(device_id,AAL_EPON_CRYPTO_DIRECTION_TX, epon_macsec_sa_cfg_msk,&epon_macsec_sa_cfg);
            CA_RET_VALUE_CHECK(ret);
        }

		ret = aal_epon_crypto_xg_sa_tbl_set(device_id, port, sa_context->sa_id, AAL_EPON_CRYPTO_DIRECTION_TX,epon_macsec_sa_tbl_cfg_msk, &epon_macsec_sa_tbl_cfg);
		CA_RET_VALUE_CHECK(ret);

		ret = aal_epon_crypto_macsec_sectag_type_set(device_id, port, llid, sa_context->sa_db.sa_db_control_flags.sci_encoded);
		CA_RET_VALUE_CHECK(ret);

		ret = aal_epon_crypto_macsec_enable_set(device_id, port, llid, TRUE);
		CA_RET_VALUE_CHECK(ret);

#endif
	}
	return ret;
}

static ca_boolean_t check_txsc_flags(ca_boolean_t es, ca_boolean_t scb, ca_boolean_t sci)
{
	if (sci && (es || scb))
		return CA_FALSE;
	/*if (es && scb)
		return CA_FALSE;*/
	return CA_TRUE;
}

static ca_status_t char_array_tostring(
	ca_uint8_t src[],
	ca_uint8_t src_len,
	ca_uint8_t dst[],
	ca_uint8_t dst_len)
{
	int i;
	ca_uint8_t offset = 0;
	for (i = 0; i < src_len; i++) {
		offset += sprintf(dst+offset, "%02x", src[i]);
	}
	if(offset>=dst_len)
	{
		ca_printf("offset :%d, dst_len :%d\r\n",offset,dst_len);
		return CA_E_ERROR;
	}
	return CA_E_OK;
}

static ca_status_t check_sa_exist(
	ca_uint32_t 	sa_id)
{
	ca_status_t ret = CA_E_NOT_FOUND;
	CA_MACSEC_SA_ID_CHECK(sa_id);

	if (global_sa_table[sa_id].sa_id == sa_id )
	{
		ca_printf("%s: sa record exist! index: %d \r\n",__func__, sa_id);
		return CA_E_EXISTS;
	}

	return ret;
}

static ca_status_t update_sa_entry(
	ca_macsec_sa_t 	*sa_context)
{
	ca_status_t ret = CA_E_NOT_FOUND;
	ca_uint32_t sa_id = 0;
	ca_uint8_t  llid_idx = 0;
	CA_MACSEC_SA_ID_CHECK(sa_context->sa_id);
	sa_id = sa_context->sa_id;
    if(global_sa_table[sa_id].sa_id == sa_id){
        memcpy(&(global_sa_table[sa_id]), sa_context, sizeof(ca_macsec_sa_t));

        if(rx_sa_entry_table[sa_id].valid){
        	if(rx_sa_entry_table[sa_id].logic_sa_id == sa_context->sa_id){
        		if(rx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_ETHERNET)
            	{
            		sa_context->sa_id = 0;
                    llid_idx = 0;
            	}

                if(rx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_EPON)
            	{

#ifdef CONFIG_NE_PON
            		CA_PON_MAC_IS_10GEPON();
            		ret = aal_mpcp_reg_llid_index_get(0, rx_sa_entry_table[sa_id].port_id,
                        rx_sa_entry_table[sa_id].sa_port, &llid_idx);
            		CA_RET_VALUE_CHECK(ret);
#endif
            	}

                if (rx_sa_entry_table[sa_id].sa_context.direction == CA_MACSEC_RX_DIR )
            	{
            		memcpy(&(rx_sa_entry_table[sa_id].sa_context), sa_context, sizeof(ca_macsec_sa_t));
            		ret = ca_macsec_rx_sa_context_set(0, rx_sa_entry_table[sa_id].port_id, &(rx_sa_entry_table[sa_id].sa_context));
            		CA_RET_VALUE_CHECK(ret);

            		ca_printf("%s: update rx sa context to sa entry table ! index: %d \r\n",__func__, sa_id);
            		return CA_E_OK;
            	}
        	}
        }

        if(tx_sa_entry_table[sa_id].valid){
            if(tx_sa_entry_table[sa_id].logic_sa_id == sa_context->sa_id){
        		if(tx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_ETHERNET)
            	{
            		sa_context->sa_id = 0;
                    llid_idx = 0;
            	}

                if(tx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_EPON)
            	{

#ifdef CONFIG_NE_PON
            		CA_PON_MAC_IS_10GEPON();
            		ret = aal_mpcp_reg_llid_index_get(0, tx_sa_entry_table[sa_id].port_id,
                        tx_sa_entry_table[sa_id].sa_port, &llid_idx);
            		CA_RET_VALUE_CHECK(ret);
#endif
            	}

                if (tx_sa_entry_table[sa_id].sa_context.sa_id == CA_MACSEC_TX_DIR)
            	{
            		memcpy(&(tx_sa_entry_table[sa_id].sa_context), sa_context, sizeof(ca_macsec_sa_t));
            		ret = ca_macsec_tx_sa_context_set(0, tx_sa_entry_table[sa_id].port_id, tx_sa_entry_table[sa_id].sa_port, &(tx_sa_entry_table[sa_id].sa_context));
            		CA_RET_VALUE_CHECK(ret);

            		ca_printf("%s: update tx sa context to sa entry table ! index: %d \r\n",__func__, sa_id);
            		return CA_E_OK;
            	}
        	}
        }
        return CA_E_OK;
    }else{
        return CA_E_NOT_FOUND;
    }
	return ret;
}

static ca_status_t delete_sa_entry(
	ca_uint32_t 	sa_id)
{
	ca_status_t ret = CA_E_NOT_FOUND;
	ca_uint8_t llid_idx = 0;
	CA_MACSEC_SA_ID_CHECK(sa_id);

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_uni_macsec_said_t uni_said_cfg;
#endif

#ifdef CONFIG_NE_PON
	aal_epon_crypto_macsec_said_t epon_said_cfg;
	memset(&epon_said_cfg, 0, sizeof(aal_epon_crypto_macsec_said_t));
#endif

#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	memset(&uni_said_cfg, 0, sizeof(aal_uni_macsec_said_t));
#endif

    if(global_sa_table[sa_id].sa_id == sa_id){
        global_sa_table[sa_id].sa_id = CA_MACSEC_INVALID_VALUE;

        if(rx_sa_entry_table[sa_id].valid){
            if (rx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_EPON ){
#ifdef CONFIG_NE_PON
                CA_PON_MAC_IS_10GEPON();
                ret = aal_mpcp_reg_llid_index_get(0, rx_sa_entry_table[sa_id].port_id,
                    rx_sa_entry_table[sa_id].sa_port, &llid_idx);
                CA_RET_VALUE_CHECK(ret);
#endif
        	}
        	if (rx_sa_entry_table[sa_id].logic_sa_id == sa_id )
        	{
          		if (rx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_ETHERNET )	{
        			//rx_sa_entry_table[sa_id].sa_context.sa_id = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
                    uni_said_cfg.valid = FALSE;
                    uni_said_cfg.sa_id = 0;
                    uni_said_cfg.llid = rx_sa_entry_table[sa_id].sa_port;
                    ret = aal_uni_macsec_rx_said_set(0, 0, &uni_said_cfg);
                    CA_RET_VALUE_CHECK(ret);
#endif
        		}else if(rx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_EPON){
#ifdef CONFIG_NE_PON
        			//rx_sa_entry_table[sa_id].sa_context.sa_id = sa_id;

                    epon_said_cfg.valid = FALSE;
                    epon_said_cfg.llid  = rx_sa_entry_table[sa_id].sa_port;
                    epon_said_cfg.sa_id = sa_id;
                    ret = aal_epon_crypto_macsec_said_set(0, rx_sa_entry_table[sa_id].port_id, llid_idx, AAL_EPON_CRYPTO_DIRECTION_RX, &epon_said_cfg);
                    CA_RET_VALUE_CHECK(ret);
#endif
        		}
        		//ret = ca_macsec_rx_sa_context_set(0, rx_sa_entry_table[sa_id].port_id, &(rx_sa_entry_table[sa_id].sa_context));
        		//CA_RET_VALUE_CHECK(ret);

        		rx_sa_entry_table[sa_id].valid 			= FALSE;
                rx_sa_entry_table[sa_id].port_type		= CA_MACSEC_INVALID_VALUE;
        		rx_sa_entry_table[sa_id].port_id		= CA_MACSEC_INVALID_VALUE;
        		rx_sa_entry_table[sa_id].sa_port   		= CA_MACSEC_INVALID_VALUE;
        		rx_sa_entry_table[sa_id].logic_sa_id   	= CA_MACSEC_INVALID_VALUE;
        		ca_printf("%s: delete rx sa context from rx sa entry table ! index: %d \r\n",__func__, sa_id);
        		return CA_E_OK;
        	}
        }

        if(tx_sa_entry_table[sa_id].valid){
            if (tx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_EPON ){
#ifdef CONFIG_NE_PON
                CA_PON_MAC_IS_10GEPON();
                ret = aal_mpcp_reg_llid_index_get(0, tx_sa_entry_table[sa_id].port_id,
                    tx_sa_entry_table[sa_id].sa_port, &llid_idx);
                CA_RET_VALUE_CHECK(ret);
#endif
        	}
            if (tx_sa_entry_table[sa_id].logic_sa_id == sa_id ){
        		if (tx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_ETHERNET )	{
        			//tx_sa_entry_table[sa_id].sa_context.sa_id = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
                    uni_said_cfg.valid = FALSE;
                    uni_said_cfg.sa_id = 0;
                    uni_said_cfg.llid = tx_sa_entry_table[sa_id].sa_port;
                    ret = aal_uni_macsec_tx_said_set(0, 0, &uni_said_cfg);
                    CA_RET_VALUE_CHECK(ret);
#endif
        		}else if(tx_sa_entry_table[sa_id].port_type == CA_PORT_TYPE_EPON){
#ifdef CONFIG_NE_PON
        			//tx_sa_entry_table[sa_id].sa_context.sa_id = sa_id;

                    epon_said_cfg.valid = FALSE;
                    epon_said_cfg.llid  = tx_sa_entry_table[sa_id].sa_port;
                    epon_said_cfg.sa_id = sa_id;
                    ret = aal_epon_crypto_macsec_said_set(0, tx_sa_entry_table[sa_id].port_id, llid_idx, AAL_EPON_CRYPTO_DIRECTION_TX, &epon_said_cfg);
                    CA_RET_VALUE_CHECK(ret);
                    ret = aal_epon_crypto_macsec_enable_set(0, tx_sa_entry_table[sa_id].port_id, tx_sa_entry_table[sa_id].sa_port, FALSE);
                    CA_RET_VALUE_CHECK(ret);
#endif
        		}

        		//ret = ca_macsec_tx_sa_context_set(0, tx_sa_entry_table[sa_id].port_id, tx_sa_entry_table[sa_id].sa_port, &(tx_sa_entry_table[sa_id].sa_context));
        		//CA_RET_VALUE_CHECK(ret);

        		tx_sa_entry_table[sa_id].valid 			= FALSE;
                tx_sa_entry_table[sa_id].port_type		= CA_MACSEC_INVALID_VALUE;
        		tx_sa_entry_table[sa_id].port_id		= CA_MACSEC_INVALID_VALUE;
        		tx_sa_entry_table[sa_id].sa_port    	= CA_MACSEC_INVALID_VALUE;
        		tx_sa_entry_table[sa_id].logic_sa_id    = CA_MACSEC_INVALID_VALUE;
        		ca_printf("%s: delete tx sa context from tx sa entry table ! index: %d \r\n",__func__, sa_id);
        		return CA_E_OK;
        	}
        }
        return CA_E_OK;
	}else{
        return CA_E_NOT_FOUND;
    }
	return ret;
}

ca_status_t hw_macsec_sas_get(
	CA_IN ca_device_id_t	device_id
)
{
	ca_status_t ret = CA_E_ERROR;
	int i = 0;
	ca_uint8_t hash0[16]= {0};
	ca_uint8_t data[16]= {0};
	ca_uint8_t sa_key0[33] = {0};
	ca_uint8_t sa_hash0[33]= {0};
	ca_uint8_t hash1[16]= {0};
	ca_uint8_t sa_key1[33] = {0};
	ca_uint8_t sa_hash1[33]= {0};

	ca_printf("%-5s %-4s %-7s %-6s %-6s %-3s %-6s %-32s %-32s %-8s\r\n",
				"SAIDX", "PORT", "LOGSAID", "RXSAID", "SAPORT", "DIR", "ACTIDX", "KEY0", "HASH0", "SECMODE");
	ca_printf("--------------------------------------------------------------------------------------------------------\r\n");
	for(i = 0; i< MACSEC_SA_NODE_NUM; i++){
		if(rx_sa_entry_table[i].valid)
		{   if(rx_sa_entry_table[i].sa_context.association_number == 0){
    			ret = char_array_tostring(rx_sa_entry_table[i].sa_context.sa_db.key0,MACSEC_KEY_LENGTH,sa_key0,33);
    			CA_RET_VALUE_CHECK(ret);
                ret = aes128_ecb_encrypt(rx_sa_entry_table[i].sa_context.sa_db.key0, data, hash0);
        		CA_RET_VALUE_CHECK(ret);
    			ret = char_array_tostring(hash0,MACSEC_KEY_LENGTH,sa_hash0,33);
    			CA_RET_VALUE_CHECK(ret);
                ca_printf("%-5u %-4u %-7u %-6u %-6u %-3u %-6u %-32s %-32s %-8u \r\n",
					i,
					rx_sa_entry_table[i].port_id,
					rx_sa_entry_table[i].logic_sa_id,
					rx_sa_entry_table[i].sa_context.sa_id,
					rx_sa_entry_table[i].sa_port,
					rx_sa_entry_table[i].sa_context.direction,
					rx_sa_entry_table[i].sa_context.association_number,
					sa_key0,
					sa_hash0,
					rx_sa_entry_table[i].sa_context.sa_db.sa_db_control_flags.security_mode
					);
		    }else if(rx_sa_entry_table[i].sa_context.association_number == 1){
    			ret = char_array_tostring(rx_sa_entry_table[i].sa_context.sa_db.key1,MACSEC_KEY_LENGTH,sa_key1,33);
    			CA_RET_VALUE_CHECK(ret);
                ret = aes128_ecb_encrypt(rx_sa_entry_table[i].sa_context.sa_db.key1, data, hash1);
        		CA_RET_VALUE_CHECK(ret);
    			ret = char_array_tostring(hash1,MACSEC_KEY_LENGTH,sa_hash1,33);
    			CA_RET_VALUE_CHECK(ret);
                ca_printf("%-5u %-4u %-7u %-6u %-6u %-3u %-6u %-32s %-32s %-8u \r\n",
					i,
					rx_sa_entry_table[i].port_id,
					rx_sa_entry_table[i].logic_sa_id,
					rx_sa_entry_table[i].sa_context.sa_id,
					rx_sa_entry_table[i].sa_port,
					rx_sa_entry_table[i].sa_context.direction,
					rx_sa_entry_table[i].sa_context.association_number,
					sa_key1,
					sa_hash1,
					rx_sa_entry_table[i].sa_context.sa_db.sa_db_control_flags.security_mode
					);
		    }

			ret = CA_E_OK;
		}
	}

	ca_printf("%-5s %-4s %-7s %-6s %-6s %-3s %-6s %-32s %-32s %-8s\r\n",
				"SAIDX", "PORT", "LOGSAID", "TXSAID", "SAPORT", "DIR", "ACTIDX", "KEY0", "HASH0", "SECMODE");
	ca_printf("--------------------------------------------------------------------------------------------------------\r\n");
	for(i = 0; i< MACSEC_SA_NODE_NUM; i++){
		if(tx_sa_entry_table[i].valid)
		{
            if(tx_sa_entry_table[i].sa_context.association_number == 0){
    			ret = char_array_tostring(tx_sa_entry_table[i].sa_context.sa_db.key0,MACSEC_KEY_LENGTH,sa_key0,33);
    			CA_RET_VALUE_CHECK(ret);
                ret = aes128_ecb_encrypt(tx_sa_entry_table[i].sa_context.sa_db.key0, data, hash0);
        		CA_RET_VALUE_CHECK(ret);
    			ret = char_array_tostring(hash0,MACSEC_KEY_LENGTH,sa_hash0,33);
    			CA_RET_VALUE_CHECK(ret);
    			ca_printf("%-5u %-4u %-7u %-6u %-6u %-3u %-6u %-32s %-32s %-8u \r\n",
    					i,
    					tx_sa_entry_table[i].port_id,
    					tx_sa_entry_table[i].logic_sa_id,
    					tx_sa_entry_table[i].sa_context.sa_id,
    					tx_sa_entry_table[i].sa_port,
    					tx_sa_entry_table[i].sa_context.direction,
    					tx_sa_entry_table[i].sa_context.association_number,
    					sa_key0,
    					sa_hash0,
    					tx_sa_entry_table[i].sa_context.sa_db.sa_db_control_flags.security_mode
    					);
            }else if(tx_sa_entry_table[i].sa_context.association_number == 1){
    			ret = char_array_tostring(tx_sa_entry_table[i].sa_context.sa_db.key1,MACSEC_KEY_LENGTH,sa_key1,33);
    			CA_RET_VALUE_CHECK(ret);
                ret = aes128_ecb_encrypt(tx_sa_entry_table[i].sa_context.sa_db.key1, data, hash1);
        		CA_RET_VALUE_CHECK(ret);
    			ret = char_array_tostring(hash1,MACSEC_KEY_LENGTH,sa_hash1,33);
    			CA_RET_VALUE_CHECK(ret);
    			ca_printf("%-5u %-4u %-7u %-6u %-6u %-3u %-6u %-32s %-32s %-8u \r\n",
    					i,
    					tx_sa_entry_table[i].port_id,
    					tx_sa_entry_table[i].logic_sa_id,
    					tx_sa_entry_table[i].sa_context.sa_id,
    					tx_sa_entry_table[i].sa_port,
    					tx_sa_entry_table[i].sa_context.direction,
    					tx_sa_entry_table[i].sa_context.association_number,
    					sa_key1,
    					sa_hash1,
    					tx_sa_entry_table[i].sa_context.sa_db.sa_db_control_flags.security_mode
    					);
            }
			ret = CA_E_OK;
		}
	}

	ca_printf("%-5s %-4s %-3s %-6s %-32s %-32s %-8s\r\n",
					"SAIDX", "SAID", "DIR", "ACTIDX", "KEY0", "HASH0", "SECMODE");
	ca_printf("--------------------------------------------------------------------------------------------------------\r\n");


	for(i = 0; i< MACSEC_SA_NODE_NUM; i++){
		if(global_sa_table[i].sa_id != CA_MACSEC_INVALID_VALUE)
		{
            if(global_sa_table[i].association_number == 0){
    			ret = char_array_tostring(global_sa_table[i].sa_db.key0,MACSEC_KEY_LENGTH,sa_key0,33);
    			CA_RET_VALUE_CHECK(ret);

                ret = aes128_ecb_encrypt(global_sa_table[i].sa_db.key0, data, hash0);
        		CA_RET_VALUE_CHECK(ret);
    			ret = char_array_tostring(hash0,MACSEC_KEY_LENGTH,sa_hash0,33);
    			CA_RET_VALUE_CHECK(ret);
    			ca_printf("%-5u %-4u %-3u %-6u %-32s %-32s %-8u \r\n",
    					i,
    					global_sa_table[i].sa_id,
    					global_sa_table[i].direction,
    					global_sa_table[i].association_number,
    					sa_key0,
    					sa_hash0,
    					global_sa_table[i].sa_db.sa_db_control_flags.security_mode
    					);
            }else if(global_sa_table[i].association_number == 1){
    			ret = char_array_tostring(global_sa_table[i].sa_db.key1,MACSEC_KEY_LENGTH,sa_key1,33);
    			CA_RET_VALUE_CHECK(ret);

                ret = aes128_ecb_encrypt(global_sa_table[i].sa_db.key1, data, hash1);
        		CA_RET_VALUE_CHECK(ret);
    			ret = char_array_tostring(hash1,MACSEC_KEY_LENGTH,sa_hash1,33);
    			CA_RET_VALUE_CHECK(ret);
    			ca_printf("%-5u %-4u %-3u %-6u %-32s %-32s %-8u \r\n",
    					i,
    					global_sa_table[i].sa_id,
    					global_sa_table[i].direction,
    					global_sa_table[i].association_number,
    					sa_key1,
    					sa_hash1,
    					global_sa_table[i].sa_db.sa_db_control_flags.security_mode
    					);
            }
			ret = CA_E_OK;
		}
	}
	//ca_spin_unlock(macsec_lock);
	if(ret)
	{
		ca_printf("%s: GET sa_context node fail!\r\n", __func__);
		return ret;
	}else{
		ca_printf("%s: GET sa_context node successfully!\r\n", __func__,i);
		return ret;
	}
	return ret;
}

ca_status_t hw_macsec_init(
	CA_IN ca_device_id_t				device_id,
	CA_IN ca_port_id_t					port,
	CA_IN ca_macsec_global_config_t  	*macsec_global_conf)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_uni_macsec_cfg_msk_t rx_msk;
	aal_uni_macsec_cfg_msk_t tx_msk;
	aal_uni_macsec_cfg_t uni_rx_cfg;
	aal_uni_macsec_cfg_t uni_tx_cfg;
#endif
	ca_uint16_t said_idx = 0;
	aal_uni_macsec_said_t said_cfg;

	//aal_uni_macsec_tbl_cfg_msk_t uni_macsec_tbl_cfg_msk;
	//aal_uni_macsec_tbl_cfg_t uni_macsec_tbl_rx_cfg;

#ifdef CONFIG_NE_PON
	ca_uint8_t llid_index = 0;
	aal_epon_coresec_glb_cfg_msk_t epon_macsec_glb_cfg_rx_msk;
	aal_epon_coresec_glb_cfg_msk_t epon_macsec_glb_cfg_tx_msk;
	aal_epon_crypto_xg_tbl_msk_t epon_macsec_sa_tbl_rx_msk;
	aal_epon_txmac_pkt_type_ena_cfg_msk_t txmac_pkt_type_ena_cfg_msk;
	aal_epon_coresec_glb_cfg_t epon_macsec_glb_rx_cfg;
	aal_epon_coresec_glb_cfg_t epon_macsec_glb_tx_cfg;
	//aal_epon_crypto_xg_tbl_field_t epon_macsec_sa_rx_tbl;
	aal_epon_txmac_pkt_type_ena_cfg_t txmac_pkt_type_ena_cfg;
	aal_epon_crypto_macsec_said_t macsec_said;
	ca_uint8_t bc_llid_index = 0;
	ca_uint16_t bc_llid[4] = {0x7FFE,0x7FFF,0xFFFE,0xFFFF};
	ca_uint8_t i = 0;
#endif

    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
    {
        return CA_E_PARAM;
    }

	if((port_id == 0x5)||(port_id == 0x6)){//10G UNI PORT
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
		rx_msk.u32 = 0;
		tx_msk.u32 = 0;
		//uni_macsec_tbl_cfg_msk.u32 = 0;
		memset(&uni_rx_cfg, 0, sizeof(aal_uni_macsec_cfg_t));
		memset(&uni_tx_cfg, 0, sizeof(aal_uni_macsec_cfg_t));
		//memset(&uni_macsec_tbl_rx_cfg, 0, sizeof(aal_uni_macsec_tbl_cfg_t));

        rx_msk.s.drop_untag_pkt_mode    = 1;
        //rx_msk.s.drop_mpcp_ena          = 1;
		//rx_msk.s.drop_oam_ena           = 1;
        rx_msk.s.drop_8021x_ena         = 1;
        rx_msk.s.drop_usrdef_ena        = 1;
        rx_msk.s.drop_usrdef            = 1;
		rx_msk.s.en_co_error            = 1;
		rx_msk.s.soft_reset             = 1;

	    tx_msk.s.drop_untag_pkt_mode    = 1;
        //tx_msk.s.drop_mpcp_ena          = 1;
        //tx_msk.s.drop_oam_ena           = 1;
        tx_msk.s.drop_8021x_ena         = 1;
        tx_msk.s.drop_usrdef_ena        = 1;
        tx_msk.s.drop_usrdef            = 1;
        tx_msk.s.en_co_error            = 1;
		tx_msk.s.soft_reset             = 1;

        if(macsec_global_conf->drop_untagged_rx_frames == CA_MACSEC_FWD_ALL_UNTAG_PKT){
            uni_rx_cfg.drop_untag_pkt_mode = CA_MACSEC_FWD_ALL_UNTAG_PKT;
            uni_tx_cfg.drop_untag_pkt_mode = CA_MACSEC_FWD_ALL_UNTAG_PKT;
        }else if(macsec_global_conf->drop_untagged_rx_frames == CA_MACSEC_FWD_SPECIFIED_UNTAG_PKT){
            uni_rx_cfg.drop_untag_pkt_mode = CA_MACSEC_FWD_SPECIFIED_UNTAG_PKT;
            uni_tx_cfg.drop_untag_pkt_mode = CA_MACSEC_FWD_SPECIFIED_UNTAG_PKT;
        }else if(macsec_global_conf->drop_untagged_rx_frames == CA_MACSEC_DROP_ALL_UNTAG_PKT){
            uni_rx_cfg.drop_untag_pkt_mode = CA_MACSEC_DROP_ALL_UNTAG_PKT;
            uni_tx_cfg.drop_untag_pkt_mode = CA_MACSEC_DROP_ALL_UNTAG_PKT;
        }else{
            return CA_E_PARAM;
        }

        //uni_rx_cfg.drop_mpcp_ena        =   (macsec_global_conf->fwd_untagged_mpcp == CA_MACSEC_DROP_PKT);
        //uni_rx_cfg.drop_oam_ena         =   (macsec_global_conf->fwd_untagged_oam == CA_MACSEC_DROP_PKT);
		uni_rx_cfg.drop_8021x_ena	    =   macsec_global_conf->KaY_reserved;
        uni_rx_cfg.drop_usrdef_ena      =   (macsec_global_conf->fwd_untagged_ethertype == 0);
        uni_rx_cfg.drop_usrdef          =   macsec_global_conf->untagged_ethertype;

		uni_rx_cfg.en_co_error 		    = 	macsec_global_conf->confidential_offset_error;
		uni_rx_cfg.soft_reset 		    = 	macsec_global_conf->data_path_reset;
		//uni_macsec_tbl_rx_cfg.v_config  = 	CA_MACSEC_STRICT;

		memset(&said_cfg, 0, sizeof(aal_uni_macsec_said_t));
        said_cfg.llid = port_id;
        said_cfg.sa_id = 0;
        said_cfg.valid = 1;
		ret = aal_uni_macsec_rx_said_set(device_id, 0, &said_cfg);
		CA_RET_VALUE_CHECK(ret);

        ret = aal_uni_macsec_tx_said_set(device_id, 0, &said_cfg);
		CA_RET_VALUE_CHECK(ret);

		//ret = aal_uni_macsec_rx_sadb_tbl_set(device_id, said_cfg.sa_id, uni_macsec_tbl_cfg_msk,&uni_macsec_tbl_rx_cfg);
		//CA_RET_VALUE_CHECK(ret);macsec_global_conf

		/*uni_tx_cfg.drop_mpcp_ena        =   (macsec_global_conf->skip_mpcp == 0);
        uni_tx_cfg.drop_oam_ena         =   (macsec_global_conf->skip_oam  == 0);
		uni_tx_cfg.drop_8021x_ena	    =   macsec_global_conf->KaY_reserved;
        uni_tx_cfg.drop_usrdef_ena      =   (macsec_global_conf->skip_untagged_ethertype == 0);
        uni_tx_cfg.drop_usrdef          =   macsec_global_conf->untagged_ethertype;
		uni_tx_cfg.en_co_error 		    = 	macsec_global_conf->confidential_offset_error;
		uni_tx_cfg.soft_reset           = 	macsec_global_conf->data_path_reset;*/

		//ret = aal_uni_macsec_init(device_id,5,1);
		//CA_RET_VALUE_CHECK(ret);

		ret = aal_uni_macsec_rx_cfg_set(device_id,rx_msk, &uni_rx_cfg);
		CA_RET_VALUE_CHECK(ret);
		ret = aal_uni_macsec_tx_cfg_set(device_id,tx_msk, &uni_tx_cfg);
		CA_RET_VALUE_CHECK(ret);
#endif
	}else if(port_id == 0x7){//EPON PORT
#ifdef CONFIG_NE_PON
		CA_PON_MAC_IS_10GEPON();
		epon_macsec_glb_cfg_rx_msk.u32 = 0;
		epon_macsec_glb_cfg_tx_msk.u32 = 0;
		epon_macsec_sa_tbl_rx_msk.u32 = 0;
		txmac_pkt_type_ena_cfg_msk.u32 = 0;

		memset(&epon_macsec_glb_rx_cfg, 0, sizeof(aal_epon_coresec_glb_cfg_t));
		memset(&epon_macsec_glb_tx_cfg, 0, sizeof(aal_epon_coresec_glb_cfg_t));
		//memset(&epon_macsec_sa_rx_tbl, 0, sizeof(aal_epon_crypto_xg_tbl_field_t));
		memset(&txmac_pkt_type_ena_cfg, 0, sizeof(aal_epon_txmac_pkt_type_ena_cfg_t));
        memset(&macsec_said, 0, sizeof(aal_epon_crypto_macsec_said_t));

		epon_macsec_glb_cfg_rx_msk.s.drop_untag_pkt_mode = 1;
		epon_macsec_glb_cfg_rx_msk.s.drop_mpcp_ena = 1;
		epon_macsec_glb_cfg_rx_msk.s.drop_oam_ena = 1;
        epon_macsec_glb_cfg_rx_msk.s.drop_8021x_ena = 1;
        epon_macsec_glb_cfg_rx_msk.s.drop_usrdef_ena = 1;
        epon_macsec_glb_cfg_rx_msk.s.drop_usrdef = 1;

		epon_macsec_glb_cfg_rx_msk.s.en_co_error = 1;
		epon_macsec_glb_cfg_rx_msk.s.soft_reset = 1;
		epon_macsec_sa_tbl_rx_msk.s.v_config =1;

		txmac_pkt_type_ena_cfg_msk.s.mpcp_sec_en = 1;
		txmac_pkt_type_ena_cfg_msk.s.oam_sec_en = 1;
		txmac_pkt_type_ena_cfg_msk.s.ethernettype_sec_en = 1;
		txmac_pkt_type_ena_cfg_msk.s.pkt_type = 1;
		epon_macsec_glb_cfg_tx_msk.s.soft_reset = 1;

        if(macsec_global_conf->drop_untagged_rx_frames == CA_MACSEC_FWD_ALL_UNTAG_PKT){
            epon_macsec_glb_rx_cfg.drop_untag_pkt_mode = CA_MACSEC_FWD_ALL_UNTAG_PKT;
        }else if(macsec_global_conf->drop_untagged_rx_frames == CA_MACSEC_FWD_SPECIFIED_UNTAG_PKT){
            epon_macsec_glb_rx_cfg.drop_untag_pkt_mode = CA_MACSEC_FWD_SPECIFIED_UNTAG_PKT;
        }else if(macsec_global_conf->drop_untagged_rx_frames == CA_MACSEC_DROP_ALL_UNTAG_PKT){
            epon_macsec_glb_rx_cfg.drop_untag_pkt_mode = CA_MACSEC_DROP_ALL_UNTAG_PKT;
        }else{
            return CA_E_PARAM;
        }

		epon_macsec_glb_rx_cfg.drop_mpcp_ena	   =    (macsec_global_conf->fwd_untagged_mpcp == 0);
		epon_macsec_glb_rx_cfg.drop_oam_ena        =    (macsec_global_conf->fwd_untagged_oam == 0);
        epon_macsec_glb_rx_cfg.drop_8021x_ena      =    macsec_global_conf->KaY_reserved;
        epon_macsec_glb_rx_cfg.drop_usrdef_ena     =    (macsec_global_conf->fwd_untagged_ethertype == 0);
        epon_macsec_glb_rx_cfg.drop_usrdef         =    macsec_global_conf->untagged_ethertype;

		epon_macsec_glb_rx_cfg.en_co_error		   =    macsec_global_conf->confidential_offset_error;
		epon_macsec_glb_rx_cfg.soft_reset		   =    macsec_global_conf->data_path_reset;
	    //epon_macsec_sa_rx_tbl.v_config			   =    CA_MACSEC_STRICT;

		txmac_pkt_type_ena_cfg.mpcp_sec_en	       = 	(macsec_global_conf->skip_mpcp == 0);
		txmac_pkt_type_ena_cfg.oam_sec_en		   = 	(macsec_global_conf->skip_oam == 0);
		txmac_pkt_type_ena_cfg.ethernettype_sec_en =    (macsec_global_conf->skip_untagged_ethertype == 0);
		txmac_pkt_type_ena_cfg.pkt_type            = 	macsec_global_conf->untagged_ethertype;
		epon_macsec_glb_tx_cfg.soft_reset		   = 	macsec_global_conf->data_path_reset;

        macsec_said.valid = TRUE;
        macsec_said.sa_id = 0x3F;
        for(i = 0,bc_llid_index = 60; i<4, bc_llid_index <64; i++, bc_llid_index++)
        {
            macsec_said.llid = bc_llid[i];
            ret = aal_epon_crypto_macsec_said_set(device_id, port_id, bc_llid_index, AAL_EPON_CRYPTO_DIRECTION_RX, &macsec_said);
    		CA_RET_VALUE_CHECK(ret);
        }

        ret = aal_epon_crypto_coresec_glb_cfg_set(device_id, AAL_EPON_CRYPTO_DIRECTION_RX,
                                                    epon_macsec_glb_cfg_rx_msk,&epon_macsec_glb_rx_cfg);
        CA_RET_VALUE_CHECK(ret);

		ret = aal_epon_crypto_coresec_glb_cfg_set(device_id, AAL_EPON_CRYPTO_DIRECTION_TX,
			                                        epon_macsec_glb_cfg_tx_msk,&epon_macsec_glb_tx_cfg);
		CA_RET_VALUE_CHECK(ret);

		ret = aal_epon_txmac_pkt_type_enable_set(device_id,txmac_pkt_type_ena_cfg_msk,&txmac_pkt_type_ena_cfg);
		CA_RET_VALUE_CHECK(ret);
#if 0 
        //save puc configurations before enable macsec
        ret = aal_epon_crypto_macsec_misc_get(device_id);
        CA_RET_VALUE_CHECK(ret);

        //configure puc for mpcp report overhead increase with sectag
        ret = aal_epon_crypto_macsec_misc_set(device_id);
        CA_RET_VALUE_CHECK(ret);
#endif
#endif
	}
    if(!init_flag){
    	//clear sa_id table
    	memset(rx_sa_entry_table, 0, sizeof(ca_sa_entry_t) * 64);
    	memset(tx_sa_entry_table, 0, sizeof(ca_sa_entry_t) * 64);
    	memset(global_sa_table, 0, sizeof(ca_macsec_sa_t) * 64);
    	for(said_idx = 0; said_idx < 64; said_idx++){
    		rx_sa_entry_table[said_idx].valid = FALSE;
            rx_sa_entry_table[said_idx].port_type = CA_MACSEC_INVALID_VALUE;
    		rx_sa_entry_table[said_idx].port_id	= CA_MACSEC_INVALID_VALUE;
    		rx_sa_entry_table[said_idx].sa_port = CA_MACSEC_INVALID_VALUE;
    		rx_sa_entry_table[said_idx].logic_sa_id = CA_MACSEC_INVALID_VALUE;

    		tx_sa_entry_table[said_idx].valid = FALSE;
            tx_sa_entry_table[said_idx].port_type = CA_MACSEC_INVALID_VALUE;
    		tx_sa_entry_table[said_idx].port_id	= CA_MACSEC_INVALID_VALUE;
    		tx_sa_entry_table[said_idx].sa_port = CA_MACSEC_INVALID_VALUE;
    		tx_sa_entry_table[said_idx].logic_sa_id = CA_MACSEC_INVALID_VALUE;

    		global_sa_table[said_idx].sa_id = CA_MACSEC_INVALID_VALUE;
    		global_sa_table[said_idx].association_number = CA_MACSEC_INVALID_VALUE;
    	}
        init_flag = TRUE;
    }
	if (CA_OSAL_OK != ca_spin_lock_init(&macsec_lock, SPINLOCK_CMN)) {
		ca_printf("%s: INIT macsec spinlock fail!\n", __func__);
		return CA_E_INIT;
	}

	return ret;
}

ca_status_t hw_macsec_global_config_get(
	CA_IN	ca_device_id_t				device_id,
	CA_IN	ca_port_id_t				port,
	CA_OUT	ca_macsec_global_config_t  	*macsec_global_conf)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_uni_macsec_cfg_t uni_rx_cfg,uni_tx_cfg;
	aal_uni_macsec_tbl_cfg_t uni_macsec_tbl_rx_cfg;
#endif
#ifdef CONFIG_NE_PON
	ca_uint8_t llid_index = 0;
	aal_epon_coresec_glb_cfg_t epon_macsec_glb_rx_cfg;
	aal_epon_coresec_glb_cfg_t epon_macsec_glb_tx_cfg;
	aal_epon_crypto_xg_tbl_field_t epon_macsec_sa_rx_tbl;
	aal_epon_txmac_pkt_type_ena_cfg_t txmac_pkt_type_ena_cfg;
#endif

    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
    {
        return CA_E_PARAM;
    }

	if((port_id == 0x5)||(port_id == 0x6)){//10G UNI PORT
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
		memset(&uni_rx_cfg, 0, sizeof(aal_uni_macsec_cfg_t));
		memset(&uni_tx_cfg, 0, sizeof(aal_uni_macsec_cfg_t));
		memset(&uni_macsec_tbl_rx_cfg, 0, sizeof(aal_uni_macsec_tbl_cfg_t));

		ret = aal_uni_macsec_rx_cfg_get(device_id,&uni_rx_cfg);
		CA_RET_VALUE_CHECK(ret);
		ret = aal_uni_macsec_tx_cfg_get(device_id,&uni_tx_cfg);
		CA_RET_VALUE_CHECK(ret);

		ret = aal_uni_macsec_rx_sadb_tbl_get(device_id, 0, &uni_macsec_tbl_rx_cfg);
		CA_RET_VALUE_CHECK(ret);


        macsec_global_conf->drop_untagged_rx_frames =   uni_rx_cfg.drop_untag_pkt_mode;
        macsec_global_conf->fwd_untagged_oam        =   (uni_rx_cfg.drop_oam_ena == 0);
        macsec_global_conf->fwd_untagged_mpcp       =   (uni_rx_cfg.drop_mpcp_ena == 0);
        macsec_global_conf->fwd_untagged_ethertype  =   (uni_rx_cfg.drop_usrdef_ena== 0);
		macsec_global_conf->untagged_ethertype      =   uni_rx_cfg.drop_usrdef;
		/*macsec_global_conf->skip_mpcp					=	(uni_tx_cfg.drop_mpcp_ena == 0 );
		macsec_global_conf->skip_oam					=  	(uni_tx_cfg.drop_oam_ena  == 0 );
		macsec_global_conf->skip_untagged_ethertype	=	(uni_tx_cfg.drop_untag_pkt_mode ==0);
		macsec_global_conf->untagged_ethertype		=  	uni_tx_cfg.drop_usrdef        	;
		*/

		macsec_global_conf->confidential_offset_error =  uni_tx_cfg.en_co_error			;
		macsec_global_conf->data_path_reset			 =  uni_tx_cfg.soft_reset 			;
		macsec_global_conf->KaY_reserved				 =	uni_tx_cfg.drop_8021x_ena	    ;
#endif
	}else if(port_id == 0x7)
	{//EPON port
#ifdef CONFIG_NE_PON
		CA_PON_MAC_IS_10GEPON();
		memset(&epon_macsec_glb_rx_cfg, 0, sizeof(aal_epon_coresec_glb_cfg_t));
		memset(&epon_macsec_glb_tx_cfg, 0, sizeof(aal_epon_coresec_glb_cfg_t));
		memset(&txmac_pkt_type_ena_cfg, 0, sizeof(aal_epon_txmac_pkt_type_ena_cfg_t));

		ret = aal_epon_crypto_coresec_glb_cfg_get(device_id, AAL_EPON_CRYPTO_DIRECTION_RX,
				&epon_macsec_glb_rx_cfg);
		CA_RET_VALUE_CHECK(ret);

		ret = aal_epon_txmac_pkt_type_enable_get(device_id,
			&txmac_pkt_type_ena_cfg);
		CA_RET_VALUE_CHECK(ret);

		macsec_global_conf->drop_untagged_rx_frames                 =   epon_macsec_glb_rx_cfg.drop_untag_pkt_mode                      ;
		macsec_global_conf->fwd_untagged_mpcp 						=	(epon_macsec_glb_rx_cfg.drop_mpcp_ena == 0)	                    ;
		macsec_global_conf->fwd_untagged_oam						=	(epon_macsec_glb_rx_cfg.drop_oam_ena  == 0)	                    ;
		macsec_global_conf->fwd_untagged_ethertype					=	(epon_macsec_glb_rx_cfg.drop_usrdef_ena  == 0)	                ;
		macsec_global_conf->untagged_ethertype						=	epon_macsec_glb_rx_cfg.drop_usrdef	                            ;

        macsec_global_conf->confidential_offset_error				=	epon_macsec_glb_rx_cfg.en_co_error	 							;
		macsec_global_conf->data_path_reset							=	epon_macsec_glb_rx_cfg.soft_reset								;
		macsec_global_conf->KaY_reserved							=	epon_macsec_glb_rx_cfg.drop_8021x_ena							;

		macsec_global_conf->skip_mpcp 								=	(txmac_pkt_type_ena_cfg.mpcp_sec_en   == 0)						;
		macsec_global_conf->skip_oam								=	(txmac_pkt_type_ena_cfg.oam_sec_en	  == 0)						;
		macsec_global_conf->skip_untagged_ethertype					=	(txmac_pkt_type_ena_cfg.ethernettype_sec_en == 0)				;
		//macsec_global_conf->data_path_reset							=	epon_macsec_glb_tx_cfg.soft_reset							;
#endif
	}

	return ret;
}
    
#define __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX       2000

/*
3.	Set EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG[6]=1
4.	Poll EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_Control[0] (AN[0]) until it changes to match what new key’s AN. Then we know new key is in place
5.	Write register EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL, including polling ready reads, 16 times. Its “said” is what current Macsec is using.
6.	Set EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG[6]=0
*/
ca_status_t hw_macsec_decryption_wa(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t	 port,
    CA_IN ca_uint8_t     said)
{
    int i = 0;
    ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);
    ca_uint32_t read_timeout = 0;
    ca_uint32_t write_timeout = 0;
    ca_uint32_t key_exch_timeout = 0;
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
    aal_uni_macsec_sadb_ctrl_t uni_SADB_Control;
    NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t uni_COMMAND_CONTROL;
#endif 

#ifdef CONFIG_NE_PON   
    aal_epon_crypto_xg_sadb_ctrl_t epon_SADB_Control;
    EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t epon_COMMAND_CONTROL;
    ca_uint32_t old_replay_win = 0;
#endif   

    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
	{
        return CA_E_PARAM;
	}
    
	if((port_id == 0x5)||(port_id == 0x6))
	{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
        old_replay_win = AAL_PON_REG_READ(NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW);
        AAL_PON_REG_FIELD_WRITE(NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG,cfg_mask_dec_err,1);
        AAL_PON_REG_WRITE(0xFFFFF000, NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW);
        
        //reset PN to 1 when key exchange
        AAL_PON_REG_WRITE(1,NI_HV_CORESEC10_TOP_NANDI_SADB_PacketNumber);
        AAL_PON_REG_FIELD_WRITE(NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID,pn_vld,1);
                
        /* Start to write */
        uni_COMMAND_CONTROL.bf.said = said;
        uni_COMMAND_CONTROL.bf.rwn = 1;
        uni_COMMAND_CONTROL.bf.start_write = 1;
        AAL_PON_REG_WRITE(uni_COMMAND_CONTROL.wrd, NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
    
        /* Polling for write done */
        write_timeout = 0;
         while (uni_COMMAND_CONTROL.bf.start_write)
        {
            write_timeout++;
            if (write_timeout >= __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
            {
                return CA_E_TIMEOUT;
            }
            uni_COMMAND_CONTROL.wrd = AAL_PON_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
        }
         
        //handle hash key syn issue when key exchange happens.
        while(1) {
            /* Start to read */
            uni_COMMAND_CONTROL.bf.said = said;
            uni_COMMAND_CONTROL.bf.rwn = 0;
            uni_COMMAND_CONTROL.bf.start_write = 1;
            AAL_PON_REG_WRITE(uni_COMMAND_CONTROL.wrd, NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL);

            /* Polling for read done */
            while (uni_COMMAND_CONTROL.bf.start_write)
            {
                read_timeout++;
                if (read_timeout >= __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
                {
                    return CA_E_TIMEOUT;
                }
                uni_COMMAND_CONTROL.wrd = AAL_PON_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
            }
            
            uni_SADB_Control.wrd = AAL_PON_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_Control);
            if (uni_SADB_Control.bf.an != uni_current_an) {          //check if DS frame with AN switched         
                ca_printf("key exchange, old key index:%d, new key index :%d\n",uni_current_an,uni_SADB_Control.bf.an);
                uni_current_an = uni_SADB_Control.bf.an;        //update current an
                break;
            }
            
            key_exch_timeout++;
            if (key_exch_timeout >= __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
            {
                return CA_E_TIMEOUT;
            }
        } //AN changed.

        for(i = 0; i < 16; i++)
        {
            AAL_PON_REG_WRITE(0, NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID);
            AAL_PON_REG_FIELD_WRITE(NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID,pn_vld,1);

            /* Start to write */
            uni_COMMAND_CONTROL.bf.said = said;
            uni_COMMAND_CONTROL.bf.rwn = 1;
            uni_COMMAND_CONTROL.bf.start_write = 1;
            AAL_PON_REG_WRITE(uni_COMMAND_CONTROL.wrd, NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL);

            /* Polling for write done */
            write_timeout = 0;
            while (uni_COMMAND_CONTROL.bf.start_write)
            {
                write_timeout++;
                if (write_timeout >= __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
                {
                    return CA_E_TIMEOUT;
                }
                uni_COMMAND_CONTROL.wrd = AAL_PON_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
            }
        }

        AAL_PON_REG_FIELD_WRITE(NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG,cfg_mask_dec_err,0);
        //write back old replay window after key exchange
        AAL_PON_REG_WRITE(old_replay_win, NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW);
#endif
    }else if(port_id == 0x7){

#ifdef CONFIG_NE_PON 
        old_replay_win = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW);
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG,cfg_mask_dec_err,1);
        AAL_PON_REG_WRITE(0xFFFFF000, EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW);
        //reset PN to 1 after key exchange
        AAL_PON_REG_WRITE(1,EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_PacketNumber);
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_CORESEC10_TOP_NANDI_DATAFIELD_VALID,pn_vld,1);
                
        /* Start to write */
        epon_COMMAND_CONTROL.bf.said = said;
        epon_COMMAND_CONTROL.bf.rwn = 1;
        epon_COMMAND_CONTROL.bf.start_write = 1;
        AAL_PON_REG_WRITE(epon_COMMAND_CONTROL.wrd, EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
    
        /* Polling for write done */
        write_timeout = 0;
        while (epon_COMMAND_CONTROL.bf.start_write)
        {
            write_timeout++;
            if (write_timeout >= __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
            {
                return CA_E_TIMEOUT;
            }
            epon_COMMAND_CONTROL.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
        }
         
        //handle hash key syn issue when key exchange happens.
        while(1) {
            /* Start to read */

            epon_COMMAND_CONTROL.bf.said = said;
            epon_COMMAND_CONTROL.bf.rwn = 0;
            epon_COMMAND_CONTROL.bf.start_write = 1;
            AAL_PON_REG_WRITE(epon_COMMAND_CONTROL.wrd, EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
        
            /* Polling for read done */
            while (epon_COMMAND_CONTROL.bf.start_write)
            {
                read_timeout++;
                if (read_timeout >= __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
                {
                    return CA_E_TIMEOUT;
                }
                epon_COMMAND_CONTROL.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
            }
            
            epon_SADB_Control.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_NANDI_SADB_Control);
            if (epon_SADB_Control.bf.an != epon_current_an) {          //check if DS frame with AN switched         
                ca_printf("key exchange, old key index:%d, new key index :%d\n",epon_current_an,epon_SADB_Control.bf.an);
                epon_current_an = epon_SADB_Control.bf.an;        //update current an
                break;
            }
            
            key_exch_timeout++;
            if (key_exch_timeout >= __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
            {
                ca_printf("key exchange timeout.\n");
                return CA_E_TIMEOUT;
            }
        } //AN changed.
        
        for(i = 0; i < 16; i++)
        {
            AAL_PON_REG_WRITE(0, EPON_XGEPN_CORESEC10_TOP_NANDI_DATAFIELD_VALID);
            AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_CORESEC10_TOP_NANDI_DATAFIELD_VALID,pn_vld,1);
        
            /* Start to write */
            epon_COMMAND_CONTROL.bf.said = said;
            epon_COMMAND_CONTROL.bf.rwn = 1;
            epon_COMMAND_CONTROL.bf.start_write = 1;
            AAL_PON_REG_WRITE(epon_COMMAND_CONTROL.wrd, EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
        
            /* Polling for write done */
            write_timeout = 0;
             while (epon_COMMAND_CONTROL.bf.start_write)
            {
                write_timeout++;
                if (write_timeout >= __HW_ENCRYPTION_TBL_ACCESS_RETRY_MAX)
                {
                    return CA_E_TIMEOUT;
                }
                epon_COMMAND_CONTROL.wrd = AAL_PON_REG_READ(EPON_XGEPN_CORESEC10_TOP_NANDI_COMMAND_CONTROL);
            }
        }
        AAL_PON_REG_FIELD_WRITE(EPON_XGEPN_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG,cfg_mask_dec_err,0);    
        //write back old replay window after key exchange
        AAL_PON_REG_WRITE(old_replay_win, EPON_XGEPN_CORESEC10_TOP_NANDI_REPLAYWINDOW);

#endif
    }
    return CA_E_OK;
}

ca_status_t hw_macsec_sa_add(
	CA_IN ca_device_id_t 				device_id,
	CA_IN_OUT ca_macsec_sa_t			*sa_context)
{
	ca_status_t ret = CA_E_OK;
	int sa_size;
	int i = 0;
	ca_boolean_t op_flag = FALSE;
	sa_size = sizeof(ca_macsec_sa_t);

	//CA_MACSEC_SA_ID_CHECK(sa_context->sa_id);
	/*check sa context parameters*/
    if((sa_context->sa_db.sa_db_control_flags.end_station == 0)
        &&(sa_context->sa_db.sa_db_control_flags.sci_encoded == 0)){
        return CA_E_NOT_SUPPORT;
    }

    if((sa_context->sa_db.sa_db_control_flags.end_station == 1)
        &&(sa_context->sa_db.sa_db_control_flags.sci_encoded == 1)){
        return CA_E_PARAM;
    }

    if(sa_context->association_number != sa_context->sa_db.sa_db_control_flags.association_number){
        return CA_E_PARAM;
    }

	if(sa_context->direction == CA_MACSEC_TX_DIR)
	{
		ret = check_txsc_flags(
			sa_context->sa_db.sa_db_control_flags.end_station,
			sa_context->sa_db.sa_db_control_flags.single_copy_broadcast,
			sa_context->sa_db.sa_db_control_flags.sci_encoded);
		if(!ret)
		{
			ca_printf("%s: Invalid combination of send_sci/end_station/scb.\n",__func__);
			return CA_E_PARAM;
		}

		if ((sa_context->sa_db.sa_db_control_flags.security_mode != CA_MACSEC_ENCRYP_AUTH)
			&& (sa_context->sa_db.sa_db_control_flags.security_mode != CA_MACSEC_AUTH_ONLY))
		{
			ca_printf("%s: ERROR! SA Context is for Tx, but parameter doesn't match.\n", __func__);
			return CA_E_PARAM;
		}
	} else if (sa_context->direction == CA_MACSEC_RX_DIR) { /* Rx */
		if (sa_context->sa_db.sa_db_control_flags.security_mode != CA_MACSEC_DECRYP)
		{
			ca_printf("%s: ERROR! SA Context is for Rx, but parameter doesn't match.\n", __func__);
			return CA_E_PARAM;
		}
	}

	/* alloc a said */

	for (i = 0; i < MACSEC_SA_NODE_NUM; i++) {
		if (global_sa_table[i].sa_id == CA_MACSEC_INVALID_VALUE)
		{
			global_sa_table[i].sa_id  = i;
			sa_context->sa_id = i;
			op_flag = TRUE;

			memcpy(&(global_sa_table[i]), sa_context, sa_size);
			break;
		}
	}
	//ca_spin_lock(macsec_lock);

	if(op_flag){
		ca_printf("%s: Add SA success! sa index %d, sa dir %s\r\n", __func__,i,(sa_context->direction == 0)?"TX":"RX");
		ret = CA_E_OK;
	}else{
		ca_printf("%s: Add SA fail!\r\n", __func__);
		ret = CA_E_ERROR;
	}
	//ca_spin_unlock(macsec_lock);

	return ret;
}

ca_status_t hw_macsec_sa_get(
	CA_IN ca_device_id_t 				device_id,
	CA_IN_OUT ca_macsec_sa_t			*sa_context)
{
	ca_status_t ret = CA_E_ERROR;
	CA_MACSEC_SA_ID_CHECK(sa_context->sa_id);
	ret = check_sa_exist(sa_context->sa_id);
	if(ret == CA_E_EXISTS){
		memcpy(sa_context, &(global_sa_table[sa_context->sa_id]), sizeof(ca_macsec_sa_t));
		ca_printf("%s: GET sa_context node %d successfully!\r\n", __func__,sa_context->sa_id);
		ret = CA_E_OK;
	}else{
		ca_printf("%s: Can't find sa_context node !\r\n", __func__);
		ret = CA_E_NOT_FOUND;
	}

	return ret;
}

ca_status_t hw_macsec_sa_delete(
	CA_IN ca_device_id_t 		    	device_id,
	CA_IN ca_macsec_sa_id_t				sa_id)
{
	ca_status_t ret = CA_E_ERROR;
	ca_macsec_sa_t sa_context;
	aal_uni_macsec_glb_staic_cfg_msk_t
 uni_macsec_glb_cfg_msk;
	aal_uni_macsec_glb_staic_cfg_t      uni_macsec_glb_cfg;
	aal_uni_macsec_tbl_cfg_msk_t 		uni_macsec_sa_tbl_cfg_msk;
	aal_uni_macsec_tbl_cfg_t 			uni_macsec_sa_tbl_cfg;
#ifdef CONFIG_NE_PON
	aal_epon_crypto_cfg_msk_t 			epon_macsec_glb_cfg_msk;
	aal_epon_crypto_cfg_t               epon_macsec_glb_cfg;
	aal_epon_crypto_xg_tbl_msk_t 		epon_macsec_sa_tbl_cfg_msk;
	aal_epon_crypto_xg_tbl_field_t 		epon_macsec_sa_tbl_cfg;
#endif
	CA_MACSEC_SA_ID_CHECK(sa_id);

#ifdef CONFIG_NE_PON
	epon_macsec_glb_cfg_msk.u32 = 0 ;
	epon_macsec_sa_tbl_cfg_msk.u32 = ~0U;
	memset(&epon_macsec_glb_cfg, 0, sizeof(aal_epon_crypto_cfg_t));
	memset(&epon_macsec_sa_tbl_cfg, 0, sizeof(aal_epon_crypto_xg_tbl_field_t));
	epon_macsec_glb_cfg_msk.s.tx_mode = 1;
	epon_macsec_glb_cfg_msk.s.rx_mode = 1;
#endif

	uni_macsec_glb_cfg_msk.u32 = 0;
	uni_macsec_sa_tbl_cfg_msk.u32 = ~0U;
	memset(&uni_macsec_glb_cfg, 0, sizeof(aal_uni_macsec_glb_staic_cfg_t));
	memset(&uni_macsec_sa_tbl_cfg, 0, sizeof(aal_uni_macsec_tbl_cfg_t));
	uni_macsec_glb_cfg_msk.s.xgmii_tx_macsec_on = 1;
	uni_macsec_glb_cfg_msk.s.xgmii_rx_macsec_on	= 1;

	memset(&sa_context, 0, sizeof(ca_macsec_sa_t));

	//ca_spin_lock(macsec_lock);

	/*check sa record exist or not*/
	ret = check_sa_exist(sa_id);

	if(ret == CA_E_EXISTS){
		//delete from global_sa_table&&delete from sa_entry_table
		ret = delete_sa_entry(sa_id);
		CA_RET_VALUE_CHECK(ret);

		ca_printf("%s: DELETE sa_context node success!\r\n", __func__);
		ret = CA_E_OK;
	}else{
		ca_printf("%s: Can't find sa_context node,DELETE  fail!\r\n",__func__);
		ret = CA_E_ERROR;
	}

	return ret;
}

ca_status_t hw_macsec_sa_update(
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_macsec_sa_t			*sa_context)
{
	ca_status_t ret = CA_E_ERROR;

	CA_MACSEC_SA_ID_CHECK(sa_context->sa_id);

	/*check sa record exist or not*/
	ret = check_sa_exist(sa_context->sa_id);

	if(ret == CA_E_EXISTS){
		//update global_sa_table
		memcpy(&(global_sa_table[sa_context->sa_id]), sa_context, sizeof(ca_macsec_sa_t));

		//update sa_entry_table
		ret = update_sa_entry(sa_context);
		CA_RET_VALUE_CHECK(ret);

		ca_printf("%s: UPDATE sa_context node success!\r\n",__func__);
		ret = CA_E_OK;
	}else{
		ca_printf("%s: Can't find sa_context node, UPDATE fail!\r\n",__func__);
		ret = CA_E_ERROR;
	}

	return ret;
}

ca_status_t hw_macsec_sa_active_index_set(
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_macsec_sa_id_t			sa_id,
	CA_IN ca_uint8_t				active_index)
{
	ca_status_t ret = CA_E_ERROR;
	if((active_index >2)||(sa_id >MACSEC_SA_NODE_NUM))
	{
		return CA_E_PARAM;
	}

	if(rx_sa_entry_table[sa_id].logic_sa_id == sa_id){
		if(rx_sa_entry_table[sa_id].sa_context.association_number == active_index){
			ca_printf("%s KEY%d is active.\r\n",__func__,sa_id);
			return CA_E_ERROR;
		}else{
			rx_sa_entry_table[sa_id].sa_context.association_number = active_index;
			rx_sa_entry_table[sa_id].sa_context.sa_db.sa_db_control_flags.association_number = active_index;
			ret = ca_macsec_rx_sa_context_set(device_id, rx_sa_entry_table[sa_id].port_id, &(rx_sa_entry_table[sa_id].sa_context));
			CA_RET_VALUE_CHECK(ret);
		}
	}else if(tx_sa_entry_table[sa_id].logic_sa_id == sa_id){
		if(tx_sa_entry_table[sa_id].sa_context.association_number == active_index){
			ca_printf("%s KEY%d is active.\r\n",__func__,sa_id);
			return CA_E_ERROR;
		}else{
			tx_sa_entry_table[sa_id].sa_context.association_number = active_index;
			tx_sa_entry_table[sa_id].sa_context.sa_db.sa_db_control_flags.association_number = active_index;
			ret = ca_macsec_tx_sa_context_set(device_id, tx_sa_entry_table[sa_id].port_id, tx_sa_entry_table[sa_id].sa_port, &(tx_sa_entry_table[sa_id].sa_context));
			CA_RET_VALUE_CHECK(ret);
		}
	}

	//update global_sa_table
	ret = check_sa_exist(sa_id);
	if(ret == CA_E_EXISTS){
		global_sa_table[sa_id].association_number = active_index;
		global_sa_table[sa_id].sa_db.sa_db_control_flags.association_number = active_index;
        ret = CA_E_OK;
	}

	return ret;
}

ca_status_t hw_macsec_sa_active_index_get(
	CA_IN ca_device_id_t 		device_id,
	CA_IN ca_macsec_sa_id_t		sa_id,
	CA_IN ca_uint8_t			*active_index)
{
	ca_status_t ret = CA_E_ERROR;
	CA_MACSEC_SA_ID_CHECK(sa_id);
	ret = check_sa_exist(sa_id);

	if(ret == CA_E_EXISTS){
		*active_index = global_sa_table[sa_id].association_number ;
		ret = CA_E_OK;
	}else{
		ca_printf("%s: Can't find sa_context node, get active key index fail!\r\n",__func__);
		ret = CA_E_ERROR;
	}

	return ret;
}

ca_status_t hw_macsec_sa_iterate(
	CA_IN ca_device_id_t			device_id,
	CA_OUT ca_iterator_t			*return_entry)
{
	ca_uint32_t i, start, entry_size, cnt;
	ca_macsec_sa_t *sa_context = return_entry->entry_data;
	ca_int32_t buf_size = return_entry->user_buffer_size;

	if (return_entry == NULL || return_entry->entry_data == NULL)
		return CA_E_PARAM;

	entry_size = sizeof(ca_sa_entry_t);
	if (return_entry->prev_handle == NULL) {
		start = 0;
	} else {
		start = ((ca_uint32_t)(uintptr_t)return_entry->prev_handle - (ca_uint32_t)(uintptr_t)(&global_sa_table)/sizeof(ca_uint32_t *));
	}

	if (return_entry->entry_count > CA_MAX_ITERATOR_RETRIEVE_ENTRIES)
		return_entry->entry_count = CA_MAX_ITERATOR_RETRIEVE_ENTRIES;

	ca_spin_lock(macsec_lock);
	for (i = start, cnt = 0; i < MACSEC_SA_NODE_NUM && buf_size >= entry_size && cnt < return_entry->entry_count; i++) {
		/* copy data */
		memcpy(sa_context,&(global_sa_table[i]),sizeof(ca_macsec_sa_t));
		sa_context++;

		buf_size -= entry_size;
		cnt++;
	}
	ca_spin_unlock(macsec_lock);

	return_entry->entry_count = cnt;
	return_entry->prev_handle = &(global_sa_table[i]);

	if (i >= MACSEC_SA_NODE_NUM && cnt == 0)
		return CA_E_UNAVAIL;

	return CA_E_OK;
}

ca_status_t hw_macsec_port_sa_set(
	CA_IN		ca_device_id_t			device_id,
	CA_IN		ca_port_id_t			port,
	CA_IN		ca_mpcp_llid_t			mpcp_llid,
	CA_IN		ca_macsec_sa_id_t		rx_sa_id,
	CA_IN		ca_macsec_sa_id_t		tx_sa_id)
{
	ca_status_t ret = CA_E_ERROR;
	ca_status_t retrx = CA_E_ERROR;
	ca_status_t rettx = CA_E_ERROR;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_uni_macsec_said_t 	rx_said_cfg;
	aal_uni_macsec_said_t 	tx_said_cfg;
#endif
	ca_macsec_sa_t rx_sa_context;
	ca_macsec_sa_t tx_sa_context;
	ca_uint16_t llid_idx = 0;
#ifdef CONFIG_NE_PON
	aal_epon_crypto_macsec_said_t macsec_said;
	aal_epon_crypto_cfg_msk_t epon_glb_coresec_cfg_msk;
	aal_epon_crypto_cfg_t     epon_glb_coresec_cfg;
	memset(&macsec_said, 0, sizeof(aal_epon_crypto_macsec_said_t));
	epon_glb_coresec_cfg_msk.u32 = 0;
	memset(&epon_glb_coresec_cfg, 0, sizeof(aal_epon_crypto_cfg_t));
#endif
	if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
	{
        return CA_E_PARAM;
	}
	memset(&rx_sa_context, 0, sizeof(ca_macsec_sa_t));
	memset(&tx_sa_context, 0, sizeof(ca_macsec_sa_t));

	if(rx_sa_id != CA_MACSEC_INVALID_VALUE){
		rx_sa_context.sa_id = rx_sa_id;
		retrx = ca_macsec_sa_get(device_id, &(rx_sa_context));

        if(retrx == CA_E_OK){
            if(rx_sa_context.direction != CA_MACSEC_RX_DIR){
                ca_printf("%s :%d Invalid rx_direction %x\r\n", __func__,__LINE__,rx_sa_context.direction);
        		return CA_E_PARAM;
        	}
        }
	}else{
		ca_printf("%s :%d Invalid rx_sa_id %x\r\n", __func__,__LINE__,rx_sa_id);
	}

	if(tx_sa_id != CA_MACSEC_INVALID_VALUE){
		tx_sa_context.sa_id = tx_sa_id;
		rettx = ca_macsec_sa_get(device_id, &tx_sa_context);
        if(rettx == CA_E_OK){
            if(tx_sa_context.direction != CA_MACSEC_TX_DIR){
                ca_printf("%s :%d Invalid tx_direction %x\r\n", __func__,__LINE__,tx_sa_context.direction);
        		return CA_E_PARAM;
        	}
        }
	}else{
		ca_printf("%s :%d Invalid tx_sa_id %x\r\n", __func__,__LINE__,tx_sa_id);
	}

	if((port_id == 0x5)||(port_id == 0x6))
	{
		if(retrx == CA_E_OK){
			rx_sa_context.sa_id = 0;
            if(rx_sa_context.sa_db.sa_db_control_flags.single_copy_broadcast == 1){
                return CA_E_PARAM;
            }
		}

		if(rettx == CA_E_OK){
			tx_sa_context.sa_id = 0;
            if(tx_sa_context.sa_db.sa_db_control_flags.single_copy_broadcast == 1){
                return CA_E_PARAM;
            }
		}
	}

	/*sadb tbl config index by sai_id*/
	if(retrx == CA_E_OK){
		rx_sa_context.direction = CA_MACSEC_RX_DIR;
		ret = ca_macsec_rx_sa_context_set(device_id, port_id,&rx_sa_context);
		CA_RET_VALUE_CHECK(ret);
	}else{
		ca_printf("%s: Can't find RX SA Context by SAID %x.\n", __func__, rx_sa_id);
	}

	if(rettx == CA_E_OK){
			tx_sa_context.direction = CA_MACSEC_TX_DIR;
			ret = ca_macsec_tx_sa_context_set(device_id,port_id, mpcp_llid, &tx_sa_context);
			CA_RET_VALUE_CHECK(ret);
	}else{
		ca_printf("%s: Can't find TX SA Context by SAID %x.\n", __func__, tx_sa_id);
	}

	/*said, llid and sadb tbl binding, llid_index->(said, llid), said->sadb_tbl*/
	if((port_id == 0x5)||(port_id == 0x6))
	{
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
		if(retrx == CA_E_OK){
                if(rx_sa_context.sa_db.sa_db_control_flags.end_station == 1){
                    rx_said_cfg.llid = 1;
                }else{
                    rx_said_cfg.llid = port_id;
                }
   				rx_said_cfg.valid = TRUE;
				rx_said_cfg.sa_id = rx_sa_context.sa_id;
				ret = aal_uni_macsec_rx_said_set(device_id, 0, &rx_said_cfg);
				CA_RET_VALUE_CHECK(ret);

				ret = aal_uni_macsec_init(device_id, port_id, rx_sa_context.sa_db.sa_db_control_flags.sci_encoded, AAL_UNI_MACSEC_RX_DECRY_ENA_ONLY);
				CA_RET_VALUE_CHECK(ret);
		}
		if(rettx == CA_E_OK) {
				if(tx_sa_context.sa_db.sa_db_control_flags.end_station == 1){
                    tx_said_cfg.llid = 1;
                }else{
                    tx_said_cfg.llid = port_id;
                }
				tx_said_cfg.valid = TRUE;
				tx_said_cfg.sa_id = tx_sa_context.sa_id;
				ret = aal_uni_macsec_tx_said_set(device_id, 0, &tx_said_cfg);
				CA_RET_VALUE_CHECK(ret);

				ret = aal_uni_macsec_init(device_id, port_id, tx_sa_context.sa_db.sa_db_control_flags.sci_encoded, AAL_UNI_MACSEC_TX_ENCRY_ENA_ONLY);
				CA_RET_VALUE_CHECK(ret);
		}
#endif
	}else if(port_id == 0x7){
#ifdef CONFIG_NE_PON
		CA_PON_MAC_IS_10GEPON();
		ret = aal_mpcp_reg_llid_index_get(device_id, port_id, mpcp_llid, &llid_idx);
		CA_RET_VALUE_CHECK(ret);

		macsec_said.valid = TRUE;
		macsec_said.llid = mpcp_llid;
		if(retrx == CA_E_OK){
            /*if(rx_sa_context.sa_db.sa_db_control_flags.end_station == 1){
                if(rx_sa_context.sa_db.sa_db_control_flags.single_copy_broadcast == 1){
    	            macsec_said.llid = 0;
        	    }else{
        	        macsec_said.llid = 1;
                }
            }else{
                if(rx_sa_context.sa_db.sa_db_control_flags.single_copy_broadcast == 1){
    	            macsec_said.llid = 0;
        	    }else{
        	        macsec_said.llid = mpcp_llid;
                }
            }*/

			macsec_said.sa_id = rx_sa_id;
			ret = aal_epon_crypto_macsec_said_set(device_id, port_id, llid_idx, AAL_EPON_CRYPTO_DIRECTION_RX, &macsec_said);
			CA_RET_VALUE_CHECK(ret);

            epon_glb_coresec_cfg_msk.s.rx_mode = 1;
	        epon_glb_coresec_cfg.rx_mode = AAL_EPON_CRYPTO_XG_MODE_MACSEC;

            //ret = aal_epon_crypto_xg_cfg_set(device_id, port_id,epon_glb_coresec_cfg_msk,&epon_glb_coresec_cfg);
		    //CA_RET_VALUE_CHECK(ret);

            if(epon_glb_coresec_cfg.rx_mode == AAL_EPON_CRYPTO_XG_MODE_DISABLE)
            {
                ret = aal_epon_crypto_macsec_misc_recover(device_id,AAL_EPON_CRYPTO_DIRECTION_RX);
                CA_RET_VALUE_CHECK(ret);
            }
		}

		if(rettx == CA_E_OK){
            /*if(tx_sa_context.sa_db.sa_db_control_flags.end_station == 1){
                if(tx_sa_context.sa_db.sa_db_control_flags.single_copy_broadcast == 1){
    	            macsec_said.llid = 0;
        	    }else{
        	        macsec_said.llid = 1;
                }
            }else{
                if(tx_sa_context.sa_db.sa_db_control_flags.single_copy_broadcast == 1){
    	            macsec_said.llid = 0;
        	    }else{
        	        macsec_said.llid = mpcp_llid;
                }
            }*/
			macsec_said.sa_id = tx_sa_id;
			ret = aal_epon_crypto_macsec_said_set(device_id, port_id, llid_idx, AAL_EPON_CRYPTO_DIRECTION_TX, &macsec_said);
			CA_RET_VALUE_CHECK(ret);

            epon_glb_coresec_cfg_msk.s.tx_ivsa_sel = 1;
            //epon_glb_coresec_cfg_msk.s.tx_mode = 1;
            epon_glb_coresec_cfg.tx_ivsa_sel = 0;
            //epon_glb_coresec_cfg.tx_mode = AAL_EPON_CRYPTO_XG_MODE_MACSEC;

            ret = aal_epon_crypto_xg_cfg_set(device_id, port_id,epon_glb_coresec_cfg_msk,&epon_glb_coresec_cfg);
		    CA_RET_VALUE_CHECK(ret);

            if(epon_glb_coresec_cfg.tx_mode == AAL_EPON_CRYPTO_XG_MODE_DISABLE)
            {
                ret = aal_epon_crypto_macsec_misc_recover(device_id,AAL_EPON_CRYPTO_DIRECTION_TX);
                CA_RET_VALUE_CHECK(ret);
            }
		}

#endif
	}

	//update sa_entry
	if(retrx == CA_E_OK){
		rx_sa_entry_table[rx_sa_id].valid = TRUE;
		rx_sa_entry_table[rx_sa_id].port_id = port_id;
		if((port_id == 0x5)||(port_id == 0x6)){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
			rx_sa_entry_table[rx_sa_id].port_type = CA_PORT_TYPE_ETHERNET;
            rx_sa_entry_table[rx_sa_id].sa_port = rx_said_cfg.llid;
#endif
		}else if(port_id == 0x7){
#ifdef CONFIG_NE_PON
			rx_sa_entry_table[rx_sa_id].port_type = CA_PORT_TYPE_EPON;
            rx_sa_entry_table[rx_sa_id].sa_port = macsec_said.llid;
#endif
		}

		rx_sa_entry_table[rx_sa_id].logic_sa_id = rx_sa_id;
		memcpy(&(rx_sa_entry_table[rx_sa_id].sa_context), &(rx_sa_context), sizeof(ca_macsec_sa_t));
	}

	if(rettx == CA_E_OK){
		tx_sa_entry_table[tx_sa_id].valid = TRUE;
		tx_sa_entry_table[tx_sa_id].port_id = port_id;
		if((port_id == 0x5)||(port_id == 0x6)){
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
			tx_sa_entry_table[tx_sa_id].port_type = CA_PORT_TYPE_ETHERNET;
            tx_sa_entry_table[tx_sa_id].sa_port = tx_said_cfg.llid;
#endif
		}else if(port_id == 0x7){
#ifdef CONFIG_NE_PON
			tx_sa_entry_table[tx_sa_id].port_type = CA_PORT_TYPE_EPON;
            tx_sa_entry_table[tx_sa_id].sa_port = macsec_said.llid;
#endif
		}

		tx_sa_entry_table[tx_sa_id].logic_sa_id = tx_sa_id;
		memcpy(&(tx_sa_entry_table[tx_sa_id].sa_context), &(tx_sa_context), sizeof(ca_macsec_sa_t));
	}

	if(!ret)
	{
		return CA_E_OK;
	}else{
		ca_printf("%s: ERROR: port sa set fail!\r\n",__func__);
		return CA_E_ERROR;
	}
	return ret;
}

ca_status_t hw_macsec_port_sa_get(
	CA_IN		ca_device_id_t			device_id,
	CA_IN		ca_port_id_t			port,
	CA_IN		ca_mpcp_llid_t			mpcp_llid,
	CA_OUT		ca_macsec_sa_id_t		*rx_sa_id,
	CA_OUT		ca_macsec_sa_id_t		*tx_sa_id)
{
	ca_status_t ret = CA_E_ERROR;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);
	int i = 0;

    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
	{
        return CA_E_PARAM;
    }

	for (i = 0; i < MACSEC_SA_NODE_NUM; i++)
	{
        if(rx_sa_entry_table[i].port_id == port_id){
            if(port_type == CA_PORT_TYPE_EPON){
                if(rx_sa_entry_table[i].sa_port == mpcp_llid){
                    *rx_sa_id = rx_sa_entry_table[i].logic_sa_id;
                    ret = CA_E_OK;
                    break;
                }
            }else if(port_type == CA_PORT_TYPE_ETHERNET){
                if(rx_sa_entry_table[i].sa_port == port_id){
                    *rx_sa_id = rx_sa_entry_table[i].logic_sa_id;
                    ret = CA_E_OK;
                    break;
                }
            }
        }
	}

	for (i = 0; i < MACSEC_SA_NODE_NUM; i++)
	{
		if(tx_sa_entry_table[i].port_id == port_id){
            if(port_type == CA_PORT_TYPE_EPON){
                if(tx_sa_entry_table[i].sa_port == mpcp_llid){
                    *rx_sa_id = tx_sa_entry_table[i].logic_sa_id;
                    ret = CA_E_OK;
                    break;
                }
            }else if(port_type == CA_PORT_TYPE_ETHERNET){
                if(tx_sa_entry_table[i].sa_port == port_id){
                    *rx_sa_id = tx_sa_entry_table[i].logic_sa_id;
                    ret = CA_E_OK;
                    break;
                }
            }
        }
	}

	if(!ret)
	{
		return CA_E_OK;
	}else{
		ca_printf("%s: ERROR: port sa get fail!\r\n",__func__);
		return CA_E_ERROR;
	}
	return ret;
}

ca_status_t hw_macsec_port_mc_sa_set(
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t			mpcp_llid,
	CA_IN ca_uint32_t			mc_sa_id)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);

    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
    {
        return CA_E_PARAM;
    }


	CA_RET_VALUE_CHECK(ret);
	return ret;
}

ca_status_t hw_macsec_port_mc_sa_get(
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t		 	mpcp_llid,
	CA_IN ca_uint32_t			*p_mc_sa_id)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);

    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
    {
        return CA_E_PARAM;
    }

	CA_RET_VALUE_CHECK(ret);
	return ret;
}
    
ca_status_t hw_macsec_port_stats_get(
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t			mpcp_llid,
	CA_OUT ca_macsec_stats_t		*counter)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_uni_macsec_rx_stats_t stats;
#endif
#ifdef CONFIG_NE_PON
	aal_epon_crypto_macsec_rx_stats_t epon_macsec_stats;
	ca_uint8_t llid_index = 0;
    aal_pon_crypto_mode_t rx_mode = 0;
    aal_pon_crypto_mode_t tx_mode = 0;
    aal_epon_crypto_macsec_said_t said;
#endif
    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
    {
        return CA_E_PARAM;
    }

	if((port_id == 0x5)||(port_id == 0x6))
	{//UNI port
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)

		memset(&stats, 0, sizeof(aal_uni_macsec_rx_stats_t));
		ret = aal_uni_macsec_rx_stats_get(device_id, port_id, 0, &stats);
		CA_RET_VALUE_CHECK(ret);

		counter->rx_delayed       		  = stats.rx_pkt_number_less_than_lowest_pn;
		counter->rx_octets_not_encrypted  = stats.rx_oct_protected_unencrypted;
		counter->rx_octets_encrypted      = stats.rx_oct_protected_encrypted;
		counter->rx_valid                 = stats.rx_pkts_ok;
		counter->rx_invalid               = stats.rx_pkt_is_not_valid;
#endif
	}else if(port_id == 0x7){//EPON port
#ifdef CONFIG_NE_PON
		CA_PON_MAC_IS_10GEPON();
		memset(&epon_macsec_stats, 0, sizeof(aal_epon_crypto_macsec_rx_stats_t));
        memset(&said, 0, sizeof(aal_epon_crypto_macsec_said_t));

		ret = aal_mpcp_reg_llid_index_get(device_id, port_id, mpcp_llid, &llid_index);
		CA_RET_VALUE_CHECK(ret);

        ret = aal_epon_crypto_macsec_said_get(device_id, port_id, llid_index, AAL_EPON_CRYPTO_DIRECTION_RX, &said);
        CA_RET_VALUE_CHECK(ret);

        ret = aal_epon_crypto_xg_mode_get(device_id, port_id, &tx_mode, &rx_mode);
        CA_RET_VALUE_CHECK(ret);
        if(rx_mode == AAL_EPON_CRYPTO_XG_MODE_MACSEC){
    		ret = aal_epon_crypto_macsec_rx_stats_get(device_id, port_id, said.sa_id, &epon_macsec_stats);
    		CA_RET_VALUE_CHECK(ret);
        }
		counter->rx_delayed       		  = epon_macsec_stats.rx_pkt_number_less_than_lowest_pn;
		counter->rx_octets_not_encrypted  = epon_macsec_stats.rx_oct_protected_unencrypted;
		counter->rx_octets_encrypted      = epon_macsec_stats.rx_oct_protected_encrypted;
		counter->rx_valid                 = epon_macsec_stats.rx_pkts_ok;
		counter->rx_invalid               = epon_macsec_stats.rx_pkt_invalid;
#endif
	}
	return ret;
}

ca_status_t hw_macsec_port_stats_clear(
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t			mpcp_llid)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);

    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
    {
        return CA_E_PARAM;
    }

	CA_RET_VALUE_CHECK(ret);
	return ret;
}

ca_status_t hw_macsec_port_error_get(
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t			mpcp_llid,
	CA_OUT ca_macsec_error_t		*counter)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
	aal_uni_macsec_tx_error_stats_t tx_stats;
	aal_uni_macsec_rx_error_stats_t rx_stats;
#endif
#ifdef CONFIG_NE_PON
	aal_epon_crypto_macsec_rx_error_stats_t epon_macsec_rx_stats;
	aal_epon_crypto_macsec_tx_error_stats_t epon_macsec_tx_stats;
	ca_uint8_t llid_index = 0;
    aal_pon_crypto_mode_t rx_mode = 0;
    aal_pon_crypto_mode_t tx_mode = 0;
#endif
    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
    {
        return CA_E_PARAM;
    }

	if((port_id == 0x5)||(port_id == 0x6))
	{//UNI port
#if defined(CONFIG_ARCH_CORTINA_SATURN_SFU)
		memset(&tx_stats, 0, sizeof(aal_uni_macsec_tx_error_stats_t));
		memset(&rx_stats, 0, sizeof(aal_uni_macsec_rx_error_stats_t));
		ret = aal_uni_macsec_tx_error_stats_get(device_id, port_id, &tx_stats);
		CA_RET_VALUE_CHECK(ret);
		ret = aal_uni_macsec_rx_error_stats_get(device_id, port_id, &rx_stats);
		CA_RET_VALUE_CHECK(ret);
		counter->tx_untagged										= tx_stats.tx_pkt_untagged;
		counter->tx_too_long 										= tx_stats.tx_pkt_too_long;

		counter->rx_untagged_validate_frames_not_strict 			= rx_stats.rx_pkt_no_tag;
		counter->rx_no_sci_validate_frames_strict 					= rx_stats.rx_pkt_no_sci;
		counter->rx_untagged_validate_frames_not_strict 			= rx_stats.rx_pkt_dec_auth_fail_err;
		counter->rx_bad_tag 										= rx_stats.rx_pkt_bad_tag;
		counter->rx_unknown_sci_validate_frames_not_strict_cbit_off	= rx_stats.rx_pkt_unknow_sci;
		counter->rx_validate_frames_strict_or_cbit_on				= rx_stats.rx_pkt_unused_sa;
		counter->rx_overrun 										= rx_stats.rx_pkt_overrun;
		counter->rx_invalid_confidential_offset_err 				= rx_stats.rx_pkt_dec_invalid_co_err;
		counter->rx_version_number_err_count						= rx_stats.rx_pkt_dec_ver_num_err;
		counter->rx_two_msbs_of_short_length_field_err				= rx_stats.rx_pkt_dec_sl_msb_err;
		counter->rx_authentication_fail_err 						= rx_stats.rx_pkt_dec_auth_fail_err;
		counter->rx_unknown_llid 									= rx_stats.rx_pkt_unknown_llid;
		counter->rx_runt 											= rx_stats.rx_pkt_undersize;
#endif
    }else if(port_id == 0x7){//EPON port
#ifdef CONFIG_NE_PON
		CA_PON_MAC_IS_10GEPON();

		memset(&epon_macsec_rx_stats, 0, sizeof(aal_epon_crypto_macsec_rx_error_stats_t));
		memset(&epon_macsec_tx_stats, 0, sizeof(aal_epon_crypto_macsec_tx_error_stats_t));

        ret = aal_epon_crypto_xg_mode_get(device_id, port_id, &tx_mode, &rx_mode);
        CA_RET_VALUE_CHECK(ret);
        
        if(rx_mode == AAL_EPON_CRYPTO_XG_MODE_MACSEC){
    		ret = aal_epon_crypto_macsec_rx_error_stats_get(device_id, port_id, &epon_macsec_rx_stats);
    		CA_RET_VALUE_CHECK(ret);
        }
        if(tx_mode == AAL_EPON_CRYPTO_XG_MODE_MACSEC){
    		ret =aal_epon_crypto_macsec_tx_error_stats_get(device_id, port_id,&epon_macsec_tx_stats);
    		CA_RET_VALUE_CHECK(ret);
        }
		counter->tx_untagged										= epon_macsec_tx_stats.tx_pkt_untagged;
		counter->tx_too_long 										= epon_macsec_tx_stats.tx_pkt_too_long;
		counter->rx_untagged_validate_frames_not_strict 			= epon_macsec_rx_stats.rx_pkt_no_tag;
		counter->rx_no_sci_validate_frames_strict 					= epon_macsec_rx_stats.rx_pkt_no_sci;
		counter->rx_untagged_validate_frames_not_strict 			= epon_macsec_rx_stats.rx_pkt_dec_auth_fail_err;
		counter->rx_bad_tag 										= epon_macsec_rx_stats.rx_pkt_bad_tag;
		counter->rx_unknown_sci_validate_frames_not_strict_cbit_off	= epon_macsec_rx_stats.rx_pkt_unknow_sci;
		counter->rx_validate_frames_strict_or_cbit_on				= epon_macsec_rx_stats.rx_pkt_unused_sa;
		counter->rx_overrun 										= epon_macsec_rx_stats.rx_pkt_overrun;
		counter->rx_invalid_confidential_offset_err 				= epon_macsec_rx_stats.rx_pkt_dec_invalid_co_err;
		counter->rx_version_number_err_count						= epon_macsec_rx_stats.rx_pkt_dec_ver_num_err;
		counter->rx_two_msbs_of_short_length_field_err				= epon_macsec_rx_stats.rx_pkt_dec_sl_msb_err;
		counter->rx_authentication_fail_err 						= epon_macsec_rx_stats.rx_pkt_dec_auth_fail_err;
		counter->rx_unknown_llid 									= epon_macsec_rx_stats.rx_pkt_unknown_llid;
		counter->rx_runt 											= epon_macsec_rx_stats.rx_pkt_undersize;
#endif
	}
	return ret;
}

ca_status_t hw_macsec_port_error_clear(
	CA_IN ca_device_id_t 			device_id,
	CA_IN ca_port_id_t				port,
	CA_IN ca_mpcp_llid_t			mpcp_llid)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port_id = PORT_ID(port);
	ca_uint32_t port_type = PORT_TYPE(port);

    if((port_type != CA_PORT_TYPE_EPON)&&(port_type != CA_PORT_TYPE_ETHERNET))
    {
        return CA_E_PARAM;
    }

	CA_RET_VALUE_CHECK(ret);
	return ret;
}

