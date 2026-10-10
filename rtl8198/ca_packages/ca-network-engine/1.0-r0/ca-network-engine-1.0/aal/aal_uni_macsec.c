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
#include <soc/cortina/registers.h>
#include "ca_types.h"
#include "osal_common.h"
#include "aal_common.h"
#include "aal_uni_macsec.h"
#include "ca-scfg/scfg.h"

#define __AAL_UNI_MACSEC_TBL_AN_MAX              0x1
#define __AAL_UNI_MACSEC_TBL_SC_MAX              0x1
#define __AAL_UNI_MACSEC_TBL_SCB_MAX             0x1
#define __AAL_UNI_MACSEC_TBL_ES_MAX              0x1
#define __AAL_UNI_MACSEC_TBL_CO_MAX              0x3F
#define __AAL_UNI_MACSEC_TBL_RLYEN_MAX           0x1
#define __AAL_UNI_MACSEC_TBL_SEC_MODE_MAX        0x3
#define __AAL_UNI_MACSEC_TBL_V_CONFIG_MAX        0x3
#define __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX    5000
#define __AAL_UNI_MACSEC_MAX_SAID_SUPPORT        32

ca_uint32_t g_uni_encryption_mode = 0;
static ca_uint32_t macsec_offset = 0x140;

/* Following APIs are for UNI port */
ca_status_t aal_uni_macsec_init(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t    uni_port,
    CA_IN ca_uint32_t    sectag_type,
    CA_IN aal_uni_macsec_enable_type_t ena_type)
{
	ca_status_t ret = CA_E_OK;
    ca_uint16_t rx_macsec_on = 0;
    ca_uint16_t tx_macsec_on = 0;
	NI_HV_CORESEC10_TOP_NANDI_STATUS_t nandi_sts;
	ca_uint32_t wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;

    /* get xge encryption mode from SCFG */
    ret = scfg_read(0, CFG_ID_UNI_ENCRYPTION_MODE, sizeof(g_uni_encryption_mode), &g_uni_encryption_mode);
    if (ret != CA_E_OK)
    {
        ca_printf("ca-uni_macsec: failed to get CFG_ID_UNI_ENCRYPTION_MODE.\n");
        g_uni_encryption_mode = 0; /* disabled, bypass encryption module */
    }
    
    rx_macsec_on = ((g_uni_encryption_mode>>16) == 3 ? 1:0);
    tx_macsec_on = ((g_uni_encryption_mode & 0xFFFF) == 3 ? 1:0);

	REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,saturn_c10_enc_sel, 1);
	REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_encrypt_en, 1);
	//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_sectag_type, sectag_type);
	REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,xgmii_tx_macsec_on, tx_macsec_on);
	REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,xgmii_rx_macsec_on, rx_macsec_on);
	REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,txmib_mode, 0);
	REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,rxmib_mode, 0);
	REG_L2FE_FIELD_WRITE(NI_HV_CORESEC10_TOP_NANDI_CONFIG,sec_tag_icv_send_en, 0);
#if 1
	if(uni_port == 5){
		if(ena_type == AAL_UNI_MACSEC_RX_TX_BOTH_ENA){
            REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_sectag_type, sectag_type);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p5_macsec_dec_out_sel , 1);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_dec_in_sel     , 0);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_enc_in_sel     , 0);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p5_txmac_in_sel     	, 1);
		}else if(ena_type == AAL_UNI_MACSEC_RX_DECRY_ENA_ONLY){
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p5_macsec_dec_out_sel , 1);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_dec_in_sel     , 0);
			//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_enc_in_sel     , 1);
			//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p5_txmac_in_sel     	, 0);
		}else if(ena_type == AAL_UNI_MACSEC_TX_ENCRY_ENA_ONLY){
			//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p5_macsec_dec_out_sel , 0);
			//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_dec_in_sel     , 1);
            REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_sectag_type, sectag_type);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_enc_in_sel     , 0);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p5_txmac_in_sel     	, 1);
		}
	}
	if(uni_port == 6){
		if(ena_type == AAL_UNI_MACSEC_RX_TX_BOTH_ENA){
            REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_sectag_type, sectag_type);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p6_macsec_dec_out_sel , 1);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_dec_in_sel     , 1);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_enc_in_sel     , 1);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p6_txmac_in_sel      	, 1);
		}else if(ena_type == AAL_UNI_MACSEC_RX_DECRY_ENA_ONLY){
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p6_macsec_dec_out_sel , 1);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_dec_in_sel     , 1);
			//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_enc_in_sel     , 0);
			//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p6_txmac_in_sel      	, 0);
		}if(ena_type == AAL_UNI_MACSEC_TX_ENCRY_ENA_ONLY){
			//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p6_macsec_dec_out_sel , 0);
			//REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_dec_in_sel     , 0);
            REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_sectag_type, sectag_type);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,macsec_enc_in_sel     , 1);
			REG_L2FE_FIELD_WRITE(NI_HV_GLB_STATIC_CFG,p6_txmac_in_sel      	, 1);
		}
	}
#endif
	/* Polling for mem_init_done done */
	do {
		nandi_sts.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_STATUS);
	}while (!nandi_sts.bf.mem_init_done && wait_timeout--);

	if (wait_timeout <= 0) {
		ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
		return CA_E_TIMEOUT;
	}

	wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;
	do {
		nandi_sts.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_STATUS + macsec_offset);
	}while (!nandi_sts.bf.mem_init_done && wait_timeout--);

	if (wait_timeout <= 0) {
		ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
		return CA_E_TIMEOUT;
	}
	return ret;
}

ca_status_t aal_uni_macsec_glb_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_glb_staic_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_GLB_STATIC_CFG_t  glb_cfg;
	glb_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_STATIC_CFG);

    cfg->port_to_cpu					=     glb_cfg.bf.port_to_cpu				  ;
	cfg->mgmt_pt_to_fe_also				=     glb_cfg.bf.mgmt_pt_to_fe_also			  ;
	cfg->txcrc_chk_en					=	  glb_cfg.bf.txcrc_chk_en				  ;
	cfg->p4_rgmii_tx_clk_phase			=	  glb_cfg.bf.p4_rgmii_tx_clk_phase		  ;
	cfg->p4_rgmii_tx_data_order			=     glb_cfg.bf.p4_rgmii_tx_data_order		  ;
	cfg->xgmii_rx_macsec_on    			=     glb_cfg.bf.xgmii_rx_macsec_on    		  ;
	cfg->xgmii_tx_macsec_on    			=     glb_cfg.bf.xgmii_tx_macsec_on    		  ;
	cfg->p6_txmac_in_sel        		=     glb_cfg.bf.p6_txmac_in_sel        	  ;
	cfg->p5_txmac_in_sel        		=     glb_cfg.bf.p5_txmac_in_sel        	  ;
	cfg->macsec_enc_in_sel      		=	  glb_cfg.bf.macsec_enc_in_sel   	      ;
	cfg->macsec_dec_in_sel      		=     glb_cfg.bf.macsec_dec_in_sel      	  ;
	cfg->rxmib_mode             		=     glb_cfg.bf.rxmib_mode             	  ;
	cfg->txmib_mode             		=     glb_cfg.bf.txmib_mode             	  ;
	cfg->eth_sch_rdy_pkt        		=     glb_cfg.bf.eth_sch_rdy_pkt        	  ;
	cfg->macsec_sectag_type     		=     glb_cfg.bf.macsec_sectag_type     	  ;
	cfg->p5_macsec_dec_out_sel  		=     glb_cfg.bf.p5_macsec_dec_out_sel  	  ;
	cfg->macsec_encrypt_en      		=	  glb_cfg.bf.macsec_encrypt_en      	  ;
	cfg->p6_macsec_dec_out_sel  		=     glb_cfg.bf.p6_macsec_dec_out_sel  	  ;
	cfg->cnt_op_mode            		=     glb_cfg.bf.cnt_op_mode            	  ;
	cfg->saturn_p7_tx_crc32_add 		=     glb_cfg.bf.saturn_p7_tx_crc32_add 	  ;
	cfg->macsec_enc_fifo_threshold	    =     glb_cfg.bf.macsec_enc_fifo_threshold    ;
	cfg->saturn_c10_enc_sel     		=	  glb_cfg.bf.saturn_c10_enc_sel     	  ;

	return ret;
}

ca_status_t aal_uni_macsec_glb_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_glb_staic_cfg_msk_t msk,
	CA_IN aal_uni_macsec_glb_staic_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_GLB_STATIC_CFG_t  glb_cfg;
	glb_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_STATIC_CFG);

	if(msk.s.port_to_cpu)
		glb_cfg.bf.port_to_cpu				  =   	cfg->port_to_cpu					;
	if(msk.s.mgmt_pt_to_fe_also)
		glb_cfg.bf.mgmt_pt_to_fe_also		  =   	cfg->mgmt_pt_to_fe_also				;
	if(msk.s.txcrc_chk_en)
		glb_cfg.bf.txcrc_chk_en				  =		cfg->txcrc_chk_en					;
	if(msk.s.p4_rgmii_tx_clk_phase)
		glb_cfg.bf.p4_rgmii_tx_clk_phase	  =		cfg->p4_rgmii_tx_clk_phase			;
	if(msk.s.p4_rgmii_tx_data_order)
		glb_cfg.bf.p4_rgmii_tx_data_order	  =     cfg->p4_rgmii_tx_data_order			;
	if(msk.s.xgmii_rx_macsec_on)
		glb_cfg.bf.xgmii_rx_macsec_on         =     cfg->xgmii_rx_macsec_on    			;
	if(msk.s.xgmii_tx_macsec_on)
		glb_cfg.bf.xgmii_tx_macsec_on    	  =     cfg->xgmii_tx_macsec_on    			;
	if(msk.s.p6_txmac_in_sel)
		glb_cfg.bf.p6_txmac_in_sel        	  =     cfg->p6_txmac_in_sel        		;
	if(msk.s.p5_txmac_in_sel)
		glb_cfg.bf.p5_txmac_in_sel        	  =     cfg->p5_txmac_in_sel        		;
	if(msk.s.macsec_enc_in_sel)
		glb_cfg.bf.macsec_enc_in_sel      	  =		cfg->macsec_enc_in_sel      		;
	if(msk.s.macsec_dec_in_sel)
		glb_cfg.bf.macsec_dec_in_sel      	  =     cfg->macsec_dec_in_sel      		;
	if(msk.s.rxmib_mode)
		glb_cfg.bf.rxmib_mode             	  =     cfg->rxmib_mode             		;
	if(msk.s.txmib_mode)
		glb_cfg.bf.txmib_mode             	  =     cfg->txmib_mode             		;
	if(msk.s.eth_sch_rdy_pkt)
		glb_cfg.bf.eth_sch_rdy_pkt        	  =     cfg->eth_sch_rdy_pkt        		;
	if(msk.s.macsec_sectag_type)
		glb_cfg.bf.macsec_sectag_type     	  =     cfg->macsec_sectag_type     		;
	if(msk.s.p5_macsec_dec_out_sel)
		glb_cfg.bf.p5_macsec_dec_out_sel  	  =     cfg->p5_macsec_dec_out_sel  		;
	if(msk.s.macsec_encrypt_en)
		glb_cfg.bf.macsec_encrypt_en      	  =		cfg->macsec_encrypt_en      		;
	if(msk.s.p6_macsec_dec_out_sel)
		glb_cfg.bf.p6_macsec_dec_out_sel  	  =     cfg->p6_macsec_dec_out_sel  		;
	if(msk.s.cnt_op_mode)
		glb_cfg.bf.cnt_op_mode            	  =     cfg->cnt_op_mode            		;
	if(msk.s.saturn_p7_tx_crc32_add)
		glb_cfg.bf.saturn_p7_tx_crc32_add 	  =     cfg->saturn_p7_tx_crc32_add 		;
	if(msk.s.macsec_enc_fifo_threshold)
		glb_cfg.bf.macsec_enc_fifo_threshold  =     cfg->macsec_enc_fifo_threshold      ;
	if(msk.s.saturn_c10_enc_sel)
		glb_cfg.bf.saturn_c10_enc_sel     	  =		cfg->saturn_c10_enc_sel     	    ;

	CA_NI_REG_WRITE(glb_cfg.wrd, NI_HV_GLB_STATIC_CFG);
	return ret;
}

ca_status_t aal_uni_macsec_rx_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_cfg_msk_t msk,
	CA_IN aal_uni_macsec_cfg_t *cfg)
{

	NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t xg_cfg;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_t		crypto_not_drop_ethtype;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1_t	crypto_not_drop_ethtype_1;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2_t	crypto_not_drop_ethtype_2;

	if (NULL == cfg){
		return CA_E_ERROR;
	}

	xg_cfg.wrd						= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG);
	crypto_not_drop_ethtype.wrd 	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE);
	crypto_not_drop_ethtype_1.wrd	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1);
	crypto_not_drop_ethtype_2.wrd	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2);
   
	if (msk.s.key_exchange_mode) {
		xg_cfg.bf.key_exhange_mode = cfg->key_exchange_mode;
	}

	if(msk.s.drop_8902_ena){
		crypto_not_drop_ethtype.bf.no_drop_8902 = (cfg->drop_8902_ena == 0);
	}

	if(msk.s.drop_8021x_ena){
		crypto_not_drop_ethtype.bf.no_drop_802_1x = (cfg->drop_8021x_ena == 0);
	}

	if(msk.s.drop_oam_ena){
		crypto_not_drop_ethtype.bf.no_drop_oam = (cfg->drop_oam_ena == 0);
	}

	if(msk.s.drop_mpcp_ena){
		crypto_not_drop_ethtype.bf.no_drop_mpcp = (cfg->drop_mpcp_ena == 0);
	}

	if(msk.s.drop_usrdef_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def = (cfg->drop_usrdef_ena == 0);
	}
	if(msk.s.drop_usrdef10_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def10 = (cfg->drop_usrdef10_ena == 0);
	}
	if(msk.s.drop_usrdef11_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def11 = (cfg->drop_usrdef11_ena == 0);
	}
	if(msk.s.drop_usrdef20_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def20 = (cfg->drop_usrdef20_ena == 0);
	}
	if(msk.s.drop_usrdef21_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def21 = (cfg->drop_usrdef21_ena == 0);
	}
	if(msk.s.drop_usrdef){
		crypto_not_drop_ethtype.bf.user_defined = cfg->drop_usrdef;
	}
	if(msk.s.drop_usrdef10){
		crypto_not_drop_ethtype_1.bf.user_defined10 = cfg->drop_usrdef10;
	}
	if(msk.s.drop_usrdef11){
		crypto_not_drop_ethtype_1.bf.user_defined11 = cfg->drop_usrdef11;
	}
	if(msk.s.drop_usrdef20){
		crypto_not_drop_ethtype_2.bf.user_defined20 = cfg->drop_usrdef20;
	}
	if(msk.s.drop_usrdef21){
		crypto_not_drop_ethtype_2.bf.user_defined21 = cfg->drop_usrdef21;
	}
	if(msk.s.drop_untag_pkt_mode){
		xg_cfg.bf.drop_untag_pkt_mode = cfg->drop_untag_pkt_mode;
	}
	if(msk.s.dis_crc32_chk){
		xg_cfg.bf.dis_crc32_chk = !cfg->dis_crc32_chk;
	}
	if(msk.s.disable_crc8_chk){
		xg_cfg.bf.disable_crc8_chk = !cfg->disable_crc8_chk;
	}
	if(msk.s.cfg_dont_modify_crc32){
		xg_cfg.bf.cfg_dont_modify_crc32 = !cfg->cfg_dont_modify_crc32;
	}
	if(msk.s.dis_preamble_chk){
		xg_cfg.bf.dis_preamble_chk = !cfg->dis_preamble_chk;
	}
	if(msk.s.dis_drop_macsec_err_ptk){
		xg_cfg.bf.dis_drop_macsec_err_ptk = !cfg->dis_drop_macsec_err_ptk;
	}
	if(msk.s.dis_ver0_macsec_err){
		xg_cfg.bf.dis_ver0_macsec_err = !cfg->dis_ver0_macsec_err;
	}
	if(msk.s.en_co_error){
		xg_cfg.bf.en_co_error = cfg->en_co_error;
	}
	if(msk.s.soft_reset){
		xg_cfg.bf.soft_reset = cfg->soft_reset;
	}
	
	CA_NI_REG_WRITE(xg_cfg.wrd, NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG);
	CA_NI_REG_WRITE(crypto_not_drop_ethtype.wrd,NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE);
	CA_NI_REG_WRITE(crypto_not_drop_ethtype_1.wrd,NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1);
	CA_NI_REG_WRITE(crypto_not_drop_ethtype_2.wrd,NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2);
   
	return CA_E_OK;
}

ca_status_t aal_uni_macsec_rx_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_cfg_t *cfg)
{
	NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t xg_cfg;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_t	   crypto_not_drop_ethtype;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1_t   crypto_not_drop_ethtype_1;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2_t   crypto_not_drop_ethtype_2;
	NI_HV_CORESEC10_TOP_NANDI_SADB_Control_t				crypto_sadb_ctrl;

	if(NULL == cfg)
		return CA_E_ERROR;

	xg_cfg.wrd						= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG);
	crypto_not_drop_ethtype.wrd 	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE);
	crypto_not_drop_ethtype_1.wrd	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1);
	crypto_not_drop_ethtype_2.wrd	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2);
	crypto_sadb_ctrl.wrd			= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_Control);

	cfg->key_exchange_mode		= xg_cfg.bf.key_exhange_mode					   ;
	cfg->drop_8902_ena		=  (crypto_not_drop_ethtype.bf.no_drop_8902 == 0 )	   ;
	cfg->drop_8021x_ena 	=  (crypto_not_drop_ethtype.bf.no_drop_802_1x == 0)    ;
	cfg->drop_oam_ena		=  (crypto_not_drop_ethtype.bf.no_drop_oam == 0)	   ;
	cfg->drop_mpcp_ena		=  (crypto_not_drop_ethtype.bf.no_drop_mpcp == 0)	   ;
	cfg->drop_usrdef_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def == 0)  ;
	cfg->drop_usrdef10_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def10 == 0) ;
	cfg->drop_usrdef11_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def11 == 0);
	cfg->drop_usrdef20_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def20 == 0);
	cfg->drop_usrdef21_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def21 == 0) ;
	cfg->drop_usrdef		=  crypto_not_drop_ethtype.bf.user_defined		 ;
	cfg->drop_usrdef10		=  crypto_not_drop_ethtype_1.bf.user_defined10	 ;
	cfg->drop_usrdef11		=  crypto_not_drop_ethtype_1.bf.user_defined11	 ;
	cfg->drop_usrdef20		=  crypto_not_drop_ethtype_2.bf.user_defined20	 ;
	cfg->drop_usrdef21		=  crypto_not_drop_ethtype_2.bf.user_defined21	 ;
	cfg->drop_untag_pkt_mode	=  xg_cfg.bf.drop_untag_pkt_mode				 ;
	cfg->dis_crc32_chk	=  !xg_cfg.bf.dis_crc32_chk 					  ;
	cfg->disable_crc8_chk 	=  !xg_cfg.bf.disable_crc8_chk					  ;
	cfg->cfg_dont_modify_crc32	= !xg_cfg.bf.cfg_dont_modify_crc32				  ;
	cfg->dis_preamble_chk = !xg_cfg.bf.dis_preamble_chk					  ;
	cfg->dis_drop_macsec_err_ptk= !xg_cfg.bf.dis_drop_macsec_err_ptk			  ;
	cfg->dis_ver0_macsec_err = !xg_cfg.bf.dis_ver0_macsec_err				;
	cfg->en_co_error 			  = xg_cfg.bf.en_co_error						   ;
	cfg->soft_reset 	  = xg_cfg.bf.soft_reset						   ;

	return CA_E_OK;
}

ca_status_t aal_uni_macsec_tx_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_cfg_msk_t msk,
	CA_IN aal_uni_macsec_cfg_t *cfg)
{

	NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t xg_cfg;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_t		crypto_not_drop_ethtype;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1_t	crypto_not_drop_ethtype_1;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2_t	crypto_not_drop_ethtype_2;
	
	if (NULL == cfg){
		return CA_E_ERROR;
	}

	xg_cfg.wrd						= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG + macsec_offset);
	crypto_not_drop_ethtype.wrd 	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE + macsec_offset);
	crypto_not_drop_ethtype_1.wrd	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1 + macsec_offset);
	crypto_not_drop_ethtype_2.wrd	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2 + macsec_offset);
	
	if (msk.s.key_exchange_mode) {
		xg_cfg.bf.key_exhange_mode = cfg->key_exchange_mode;
	}

	if(msk.s.drop_8902_ena){
		crypto_not_drop_ethtype.bf.no_drop_8902 = (cfg->drop_8902_ena == 0);
	}

	if(msk.s.drop_8021x_ena){
		crypto_not_drop_ethtype.bf.no_drop_802_1x = (cfg->drop_8021x_ena == 0);
	}

	if(msk.s.drop_oam_ena){
		crypto_not_drop_ethtype.bf.no_drop_oam = (cfg->drop_oam_ena == 0);
	}

	if(msk.s.drop_mpcp_ena){
		crypto_not_drop_ethtype.bf.no_drop_mpcp = (cfg->drop_mpcp_ena == 0);
	}

	if(msk.s.drop_usrdef_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def = (cfg->drop_usrdef_ena == 0);
	}
	if(msk.s.drop_usrdef_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def10 = (cfg->drop_usrdef10_ena == 0);
	}
	if(msk.s.drop_usrdef_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def11 = (cfg->drop_usrdef11_ena == 0);
	}
	if(msk.s.drop_usrdef_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def20 = (cfg->drop_usrdef20_ena == 0);
	}
	if(msk.s.drop_usrdef21_ena){
		crypto_not_drop_ethtype.bf.no_drop_user_def21 = (cfg->drop_usrdef21_ena == 0);
	}
	if(msk.s.drop_usrdef){
		crypto_not_drop_ethtype.bf.user_defined = cfg->drop_usrdef;
	}
	if(msk.s.drop_usrdef10){
		crypto_not_drop_ethtype_1.bf.user_defined10 = cfg->drop_usrdef10;
	}
	if(msk.s.drop_usrdef11){
		crypto_not_drop_ethtype_1.bf.user_defined11 = cfg->drop_usrdef11;
	}
	if(msk.s.drop_usrdef20){
		crypto_not_drop_ethtype_2.bf.user_defined20 = cfg->drop_usrdef20;
	}
	if(msk.s.drop_usrdef21){
		crypto_not_drop_ethtype_2.bf.user_defined21 = cfg->drop_usrdef21;
	}
	if(msk.s.drop_untag_pkt_mode){
		xg_cfg.bf.drop_untag_pkt_mode = cfg->drop_untag_pkt_mode;
	}
	if(msk.s.dis_crc32_chk){
		xg_cfg.bf.dis_crc32_chk = !cfg->dis_crc32_chk;
	}
	if(msk.s.disable_crc8_chk){
		xg_cfg.bf.disable_crc8_chk = !cfg->disable_crc8_chk;
	}
	if(msk.s.cfg_dont_modify_crc32){
		xg_cfg.bf.cfg_dont_modify_crc32 = !cfg->cfg_dont_modify_crc32;
	}
	if(msk.s.dis_preamble_chk){
		xg_cfg.bf.dis_preamble_chk = !cfg->dis_preamble_chk;
	}
	if(msk.s.dis_drop_macsec_err_ptk){
		xg_cfg.bf.dis_drop_macsec_err_ptk = !cfg->dis_drop_macsec_err_ptk;
	}
	if(msk.s.dis_ver0_macsec_err){
		xg_cfg.bf.dis_ver0_macsec_err = !cfg->dis_ver0_macsec_err;
	}
	if(msk.s.en_co_error){
		xg_cfg.bf.en_co_error = cfg->en_co_error;
	}
	if(msk.s.soft_reset){
		xg_cfg.bf.soft_reset = cfg->soft_reset;
	}
	
	CA_NI_REG_WRITE(xg_cfg.wrd, NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG + macsec_offset);
	CA_NI_REG_WRITE(crypto_not_drop_ethtype.wrd,NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE + macsec_offset);
	CA_NI_REG_WRITE(crypto_not_drop_ethtype_1.wrd,NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1 + macsec_offset);
	CA_NI_REG_WRITE(crypto_not_drop_ethtype_2.wrd,NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2 + macsec_offset);
	
	return CA_E_OK;
}

ca_status_t aal_uni_macsec_tx_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_cfg_t *cfg)
{
	NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG_t xg_cfg;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_t	   crypto_not_drop_ethtype;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1_t   crypto_not_drop_ethtype_1;
	NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2_t   crypto_not_drop_ethtype_2;
	
	if(NULL == cfg)
		return CA_E_ERROR;

	xg_cfg.wrd						= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_CORESEC10_GLOBAL_CONFIG + macsec_offset);
	crypto_not_drop_ethtype.wrd 	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE + macsec_offset);
	crypto_not_drop_ethtype_1.wrd	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_1 + macsec_offset);
	crypto_not_drop_ethtype_2.wrd	= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_NOT_DROP_ETHERTYPE_2 + macsec_offset);

	cfg->key_exchange_mode		= xg_cfg.bf.key_exhange_mode					   ;
	cfg->drop_8902_ena		=  (crypto_not_drop_ethtype.bf.no_drop_8902 == 0 )	   ;
	cfg->drop_8021x_ena 	=  (crypto_not_drop_ethtype.bf.no_drop_802_1x == 0)    ;
	cfg->drop_oam_ena		=  (crypto_not_drop_ethtype.bf.no_drop_oam == 0)	   ;
	cfg->drop_mpcp_ena		=  (crypto_not_drop_ethtype.bf.no_drop_mpcp == 0)	   ;
	cfg->drop_usrdef_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def == 0)  ;
	cfg->drop_usrdef10_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def10 == 0) ;
	cfg->drop_usrdef11_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def11 == 0);
	cfg->drop_usrdef20_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def20 == 0);
	cfg->drop_usrdef21_ena	=  (crypto_not_drop_ethtype.bf.no_drop_user_def21 == 0) ;
	cfg->drop_usrdef		=  crypto_not_drop_ethtype.bf.user_defined		 ;
	cfg->drop_usrdef10		=  crypto_not_drop_ethtype_1.bf.user_defined10	 ;
	cfg->drop_usrdef11		=  crypto_not_drop_ethtype_1.bf.user_defined11	 ;
	cfg->drop_usrdef20		=  crypto_not_drop_ethtype_2.bf.user_defined20	 ;
	cfg->drop_usrdef21		=  crypto_not_drop_ethtype_2.bf.user_defined21	 ;
	cfg->drop_untag_pkt_mode	=  xg_cfg.bf.drop_untag_pkt_mode				 ;
	cfg->dis_crc32_chk	=  !xg_cfg.bf.dis_crc32_chk 					  ;
	cfg->disable_crc8_chk 	=  !xg_cfg.bf.disable_crc8_chk					  ;
	cfg->cfg_dont_modify_crc32	= !xg_cfg.bf.cfg_dont_modify_crc32				  ;
	cfg->dis_preamble_chk = !xg_cfg.bf.dis_preamble_chk					  ;
	cfg->dis_drop_macsec_err_ptk= !xg_cfg.bf.dis_drop_macsec_err_ptk			  ;
	cfg->dis_ver0_macsec_err = !xg_cfg.bf.dis_ver0_macsec_err				;
	cfg->en_co_error 			  = xg_cfg.bf.en_co_error						   ;
	cfg->soft_reset 	  = xg_cfg.bf.soft_reset						   ;

	return CA_E_OK;
}

ca_status_t aal_uni_macsec_rx_said_set
(
	CA_IN ca_device_id_t			   device_id,
	CA_IN ca_uint8_t				llid_idx,
	CA_IN aal_uni_macsec_said_t 	*said)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA_t llid_prog_mem_data;
	//AAL_CHIP_CORE_CHECK(device_id,port_id);
	//AAL_ASSERT_RET(llid, CA_E_PARAM);

	if((llid_idx>63) || (NULL == said)){
		return CA_E_PARAM;
	}

	READ_INDIRCT_REG(device_id,llid_idx,NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS);
	llid_prog_mem_data.bf.LLID_value = said->llid;
	llid_prog_mem_data.bf.SA_ID =  said->sa_id;
	llid_prog_mem_data.bf.valid =  said->valid;
	CA_NI_REG_WRITE(llid_prog_mem_data.wrd, NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA);
	WRITE_INDIRCT_REG(device_id, llid_idx, NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS);

	return ret;
}

ca_status_t aal_uni_macsec_rx_said_get
(
	CA_IN ca_device_id_t			   device_id,
	CA_IN ca_uint8_t				llid_idx,
	CA_OUT aal_uni_macsec_said_t	 *said)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA_t llid_prog_mem_data;
	//AAL_CHIP_CORE_CHECK(device_id,port_id);
	//AAL_ASSERT_RET(llid, CA_E_PARAM);

	READ_INDIRCT_REG(device_id,llid_idx,NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS);
	llid_prog_mem_data.wrd = CA_NE_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA);
	said->llid = llid_prog_mem_data.bf.LLID_value;
	said->valid = llid_prog_mem_data.bf.valid;
	said->sa_id = llid_prog_mem_data.bf.SA_ID;
	return ret;
}


ca_status_t aal_uni_macsec_tx_said_set
(
	CA_IN ca_device_id_t			device_id,
	CA_IN ca_uint8_t				llid_idx,
	CA_IN aal_uni_macsec_said_t 	*said)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA_t llid_prog_mem_data;
	//AAL_CHIP_CORE_CHECK(device_id,port_id);
	//AAL_ASSERT_RET(llid, CA_E_PARAM);

	if((llid_idx>63) || (NULL == said)){
		return CA_E_PARAM;
	}

	READ_INDIRCT_REG(device_id,llid_idx,(NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS + macsec_offset));
	llid_prog_mem_data.bf.LLID_value = said->llid;
	llid_prog_mem_data.bf.SA_ID = said->sa_id;
	llid_prog_mem_data.bf.valid = said->valid;
	CA_NI_REG_WRITE(llid_prog_mem_data.wrd, (NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA + macsec_offset));
	WRITE_INDIRCT_REG(device_id, llid_idx, (NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS + macsec_offset));

	return ret;
}

ca_status_t aal_uni_macsec_tx_said_get
(
	CA_IN ca_device_id_t			   device_id,
	CA_IN ca_uint8_t				llid_idx,
	CA_OUT aal_uni_macsec_said_t	 *said)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA_t llid_prog_mem_data;
	//AAL_CHIP_CORE_CHECK(device_id,port_id);
	//AAL_ASSERT_RET(llid, CA_E_PARAM);

	READ_INDIRCT_REG(device_id,llid_idx,(NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_ACCESS + macsec_offset));
	llid_prog_mem_data.wrd = CA_NE_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_LLID_PROG_MEM_DATA + macsec_offset);
	said->llid = llid_prog_mem_data.bf.LLID_value;
	said->valid = llid_prog_mem_data.bf.valid;
	said->sa_id = llid_prog_mem_data.bf.SA_ID;
	return ret;
}

ca_status_t aal_uni_macsec_rx_sa_cfg_set(
    CA_IN ca_device_id_t  device_id,
    CA_IN aal_uni_macsec_sa_cfg_msk_t msk,
    CA_IN aal_uni_macsec_sa_cfg_t *cfg)
{
	NI_HV_CORESEC10_TOP_CORESEC10_SA_0_t sa0;
	NI_HV_CORESEC10_TOP_CORESEC10_SA_1_t sa1;
	if (NULL == cfg){
        return CA_E_ERROR;
    }
	sa0.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_SA_0);
	sa1.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_SA_1);
	if (msk.s.local_sa_reg0) {
        sa0.bf.local_sa_reg0 = cfg->local_sa_reg0;
    }
	if (msk.s.local_sa_reg1) {
        sa1.bf.local_sa_reg1 = cfg->local_sa_reg1;
    }
	if (msk.s.sel_local_sa_reg) {
        sa1.bf.sel_local_sa_reg = cfg->sel_local_sa_reg;
    }
	CA_NI_REG_WRITE(sa0.wrd, NI_HV_CORESEC10_TOP_CORESEC10_SA_0);
	CA_NI_REG_WRITE(sa1.wrd, NI_HV_CORESEC10_TOP_CORESEC10_SA_1);
	return CA_E_OK;
}

ca_status_t aal_uni_macsec_rx_sa_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_sa_cfg_t *cfg)
{
	NI_HV_CORESEC10_TOP_CORESEC10_SA_0_t sa0;
	NI_HV_CORESEC10_TOP_CORESEC10_SA_1_t sa1;
	if (NULL == cfg){
        return CA_E_ERROR;
    }
	sa0.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_SA_0);
	sa1.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_SA_1);
	cfg->local_sa_reg0 = sa0.bf.local_sa_reg0;
	cfg->local_sa_reg1 = sa1.bf.local_sa_reg1;
	cfg->sel_local_sa_reg = sa1.bf.sel_local_sa_reg;
	return CA_E_OK;
}

ca_status_t aal_uni_macsec_tx_sa_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_sa_cfg_msk_t msk,
	CA_IN aal_uni_macsec_sa_cfg_t *cfg)
{
	NI_HV_CORESEC10_TOP_CORESEC10_SA_0_t sa0;
	NI_HV_CORESEC10_TOP_CORESEC10_SA_1_t sa1;
	int reg_addr0,reg_addr1;
	if (NULL == cfg){
		return CA_E_ERROR;
	}
	reg_addr0 = NI_HV_CORESEC10_TOP_CORESEC10_SA_0 + macsec_offset;
	reg_addr1 = NI_HV_CORESEC10_TOP_CORESEC10_SA_1 + macsec_offset;
	sa0.wrd = CA_NI_REG_READ(reg_addr0);
	sa1.wrd = CA_NI_REG_READ(reg_addr1);
	if (msk.s.local_sa_reg0) {
		sa0.bf.local_sa_reg0 = cfg->local_sa_reg0;
	}
	if (msk.s.local_sa_reg1) {
		sa1.bf.local_sa_reg1 = cfg->local_sa_reg1;
	}
	if (msk.s.sel_local_sa_reg) {
		sa1.bf.sel_local_sa_reg = cfg->sel_local_sa_reg;
	}
	CA_NI_REG_WRITE(sa0.wrd, reg_addr0);
	CA_NI_REG_WRITE(sa1.wrd, reg_addr1);
	return CA_E_OK;
}

ca_status_t aal_uni_macsec_tx_sa_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_sa_cfg_t *cfg)
{
	NI_HV_CORESEC10_TOP_CORESEC10_SA_0_t sa0;
	NI_HV_CORESEC10_TOP_CORESEC10_SA_1_t sa1;
	if (NULL == cfg){
		return CA_E_ERROR;
	}
	sa0.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_SA_0 + macsec_offset);
	sa1.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_SA_1 + macsec_offset);
	cfg->local_sa_reg0 = sa0.bf.local_sa_reg0;
	cfg->local_sa_reg1 = sa1.bf.local_sa_reg1;
	cfg->sel_local_sa_reg = sa1.bf.sel_local_sa_reg;
	return CA_E_OK;
}

ca_status_t aal_uni_macsec_rx_nandi_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_nandi_cfg_msk_t msk,
	CA_IN aal_uni_macsec_nandi_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_NANDI_SCI1_t                        crypto_sci1;
    NI_HV_CORESEC10_TOP_NANDI_SCI0_t                        crypto_sci0;
    NI_HV_CORESEC10_TOP_NANDI_PNTHLD_t                      crypto_pn_thred;
    NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW_t                crypto_reply_window;
	NI_HV_CORESEC10_TOP_NANDI_CONFIG_t nandi_cfg;

	crypto_sci1.wrd                 = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SCI1);
    crypto_sci0.wrd                 = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SCI0);
    crypto_pn_thred.wrd             = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_PNTHLD);
    crypto_reply_window.wrd         = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW);
    nandi_cfg.wrd 					= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_CONFIG);

	if(NULL == cfg)
	{
		 return CA_E_PARAM;
	}
	if (msk.s.sec_tag_mem_cmd)
	{
		nandi_cfg.bf.sec_tag_mem_cmd = cfg->sec_tag_mem_cmd;
	}
	if (msk.s.sec_tag_icv_send_en)
	{
		nandi_cfg.bf.sec_tag_icv_send_en = cfg->sec_tag_icv_send_en;
	}
	if (msk.s.sci_llid_disable)
	{
		nandi_cfg.bf.sci_llid_disable = cfg->sci_llid_disable;
	}
	
	if(msk.s.sci1){
		crypto_sci1.wrd = ((ca_uint32_t)(cfg->sci1[3]) << 24 )
			| ((ca_uint32_t)(cfg->sci1[2]) << 16)
			| ((ca_uint32_t)(cfg->sci1[1]) << 8)
			| ((ca_uint32_t)(cfg->sci1[0]));
	}
	if(msk.s.sci0){
		crypto_sci0.wrd = ((ca_uint32_t)(cfg->sci0[3]) << 24 )
			| ((ca_uint32_t)(cfg->sci0[2]) << 16)
			| ((ca_uint32_t)(cfg->sci0[1]) << 8)
			| ((ca_uint32_t)(cfg->sci0[0]));
	}
	if(msk.s.pn_threshold){
		crypto_pn_thred.wrd = cfg->pn_threshold;
	}
	if(msk.s.replay_window){
		crypto_reply_window.wrd = cfg->replay_window;
	}
	CA_NI_REG_WRITE(crypto_sci1.wrd,NI_HV_CORESEC10_TOP_NANDI_SCI1);
	CA_NI_REG_WRITE(crypto_sci0.wrd,NI_HV_CORESEC10_TOP_NANDI_SCI0);
	CA_NI_REG_WRITE(crypto_pn_thred.wrd,NI_HV_CORESEC10_TOP_NANDI_PNTHLD);
	CA_NI_REG_WRITE(crypto_reply_window.wrd,NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW);
    CA_NI_REG_WRITE(nandi_cfg.wrd, NI_HV_CORESEC10_TOP_NANDI_CONFIG);
	return ret;
}

ca_status_t aal_uni_macsec_rx_nandi_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_nandi_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_NANDI_SCI1_t                        crypto_sci1;
    NI_HV_CORESEC10_TOP_NANDI_SCI0_t                        crypto_sci0;
    NI_HV_CORESEC10_TOP_NANDI_PNTHLD_t                      crypto_pn_thred;
    NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW_t                crypto_reply_window;
	NI_HV_CORESEC10_TOP_NANDI_CONFIG_t nandi_cfg;

	crypto_sci1.wrd                 = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SCI1);
    crypto_sci0.wrd                 = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SCI0);
    crypto_pn_thred.wrd             = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_PNTHLD);
    crypto_reply_window.wrd         = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW);
	nandi_cfg.wrd          			= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_CONFIG);
	
	cfg->sec_tag_mem_cmd = nandi_cfg.bf.sec_tag_mem_cmd;
	cfg->sec_tag_icv_send_en = nandi_cfg.bf.sec_tag_icv_send_en;
	cfg->sci_llid_disable = nandi_cfg.bf.sci_llid_disable;
	
	cfg->sci1[3] = (crypto_sci1.wrd >> 24 ) & 0xFF;
	cfg->sci1[2] = (crypto_sci1.wrd >> 16 ) & 0xFF;
	cfg->sci1[1] = (crypto_sci1.wrd >> 8 ) & 0xFF;
	cfg->sci1[0] = (crypto_sci1.wrd) & 0xFF;

	cfg->sci0[3] = (crypto_sci0.wrd >> 24 ) & 0xFF;
	cfg->sci0[2] = (crypto_sci0.wrd >> 16 ) & 0xFF;
	cfg->sci0[1] = (crypto_sci0.wrd >> 8 ) & 0xFF;
	cfg->sci0[0] = (crypto_sci0.wrd) & 0xFF;
		
	cfg->pn_threshold   = crypto_pn_thred.wrd;
    cfg->replay_window  = crypto_reply_window.wrd ;
    
	return ret;
}

ca_status_t aal_uni_macsec_tx_nandi_cfg_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN aal_uni_macsec_nandi_cfg_msk_t msk,
	CA_IN aal_uni_macsec_nandi_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_NANDI_SCI1_t                        crypto_sci1;
    NI_HV_CORESEC10_TOP_NANDI_SCI0_t                        crypto_sci0;
    NI_HV_CORESEC10_TOP_NANDI_PNTHLD_t                      crypto_pn_thred;
    NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW_t                crypto_reply_window;
	NI_HV_CORESEC10_TOP_NANDI_CONFIG_t nandi_cfg;

	crypto_sci1.wrd                 = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SCI1 + macsec_offset);
    crypto_sci0.wrd                 = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SCI0 + macsec_offset);
    crypto_pn_thred.wrd             = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_PNTHLD + macsec_offset);
    crypto_reply_window.wrd         = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW + macsec_offset);
    nandi_cfg.wrd 					= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_CONFIG + macsec_offset);

	if(NULL == cfg)
	{
		 return CA_E_PARAM;
	}
	if (msk.s.sec_tag_mem_cmd)
	{
		nandi_cfg.bf.sec_tag_mem_cmd = cfg->sec_tag_mem_cmd;
	}
	if (msk.s.sec_tag_icv_send_en)
	{
		nandi_cfg.bf.sec_tag_icv_send_en = cfg->sec_tag_icv_send_en;
	}
	if (msk.s.sci_llid_disable)
	{
		nandi_cfg.bf.sci_llid_disable = cfg->sci_llid_disable;
	}
	
	if(msk.s.sci1){
		crypto_sci1.wrd = ((ca_uint32_t)(cfg->sci1[0]) << 24 )
			| ((ca_uint32_t)(cfg->sci1[1]) << 16)
			| ((ca_uint32_t)(cfg->sci1[2]) << 8)
			| ((ca_uint32_t)(cfg->sci1[3]));
	}
	if(msk.s.sci0){
		crypto_sci0.wrd = ((ca_uint32_t)(cfg->sci0[0]) << 24 )
			| ((ca_uint32_t)(cfg->sci0[1]) << 16)
			| ((ca_uint32_t)(cfg->sci0[2]) << 8)
			| ((ca_uint32_t)(cfg->sci0[3]));
	}
	if(msk.s.pn_threshold){
		crypto_pn_thred.wrd = cfg->pn_threshold;
	}
	if(msk.s.replay_window){
		crypto_reply_window.wrd = cfg->replay_window;
	}
	CA_NI_REG_WRITE(crypto_sci1.wrd,NI_HV_CORESEC10_TOP_NANDI_SCI1 + macsec_offset);
	CA_NI_REG_WRITE(crypto_sci0.wrd,NI_HV_CORESEC10_TOP_NANDI_SCI0 + macsec_offset);
	CA_NI_REG_WRITE(crypto_pn_thred.wrd,NI_HV_CORESEC10_TOP_NANDI_PNTHLD + macsec_offset);
	CA_NI_REG_WRITE(crypto_reply_window.wrd,NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW + macsec_offset);
    CA_NI_REG_WRITE(nandi_cfg.wrd, NI_HV_CORESEC10_TOP_NANDI_CONFIG + macsec_offset);
	return ret;
}

ca_status_t aal_uni_macsec_tx_nandi_cfg_get(
	CA_IN ca_device_id_t  device_id,
	CA_OUT aal_uni_macsec_nandi_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_NANDI_SCI1_t                        crypto_sci1;
    NI_HV_CORESEC10_TOP_NANDI_SCI0_t                        crypto_sci0;
    NI_HV_CORESEC10_TOP_NANDI_PNTHLD_t                      crypto_pn_thred;
    NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW_t                crypto_reply_window;
	NI_HV_CORESEC10_TOP_NANDI_CONFIG_t nandi_cfg;

	crypto_sci1.wrd                 = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SCI1 + macsec_offset);
    crypto_sci0.wrd                 = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SCI0 + macsec_offset);
    crypto_pn_thred.wrd             = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_PNTHLD + macsec_offset);
    crypto_reply_window.wrd         = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_REPLAYWINDOW + macsec_offset);
	nandi_cfg.wrd          			= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_CONFIG + macsec_offset);
	
	cfg->sec_tag_mem_cmd = nandi_cfg.bf.sec_tag_mem_cmd;
	cfg->sec_tag_icv_send_en = nandi_cfg.bf.sec_tag_icv_send_en;
	cfg->sci_llid_disable = nandi_cfg.bf.sci_llid_disable;
	
	cfg->sci1[0] = (crypto_sci1.wrd >> 24 ) & 0xFF;
	cfg->sci1[1] = (crypto_sci1.wrd >> 16 ) & 0xFF;
	cfg->sci1[2] = (crypto_sci1.wrd >> 8 ) & 0xFF;
	cfg->sci1[3] = (crypto_sci1.wrd) & 0xFF;

	cfg->sci0[0] = (crypto_sci0.wrd >> 24 ) & 0xFF;
	cfg->sci0[1] = (crypto_sci0.wrd >> 16 ) & 0xFF;
	cfg->sci0[2] = (crypto_sci0.wrd >> 8 ) & 0xFF;
	cfg->sci0[3] = (crypto_sci0.wrd) & 0xFF;
		
	cfg->pn_threshold   = crypto_pn_thred.wrd;
    cfg->replay_window  = crypto_reply_window.wrd ;
    
	return ret;
}

ca_status_t aal_uni_macsec_rx_nandi_cmdctrl_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN ca_uint8_t rwn,
	CA_IN ca_uint8_t start_write)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t cmd_ctrl;
	NI_HV_CORESEC10_TOP_NANDI_INTERRUPT_0_t 	nandi_intr;
	ca_uint32_t wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;
	/* Start to write */
	cmd_ctrl.bf.said = said;
	cmd_ctrl.bf.rwn = rwn;
	cmd_ctrl.bf.start_write = start_write;
	CA_NI_REG_WRITE(cmd_ctrl.wrd,(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL ));

	wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;

	/* Polling for written done */
	do {
		cmd_ctrl.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL );
	}while (cmd_ctrl.bf.start_write && wait_timeout--);

	if (wait_timeout <= 0) {
		ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
		return CA_E_TIMEOUT;
	}

	wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;
	nandi_intr.bf.cpu_cmd_executed = 0;
	do {
		nandi_intr.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_INTERRUPT_0 );
	}while (!nandi_intr.bf.cpu_cmd_executed && wait_timeout--);

	if (wait_timeout <= 0) {
		ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
		return CA_E_TIMEOUT;
	}
	return ret;
}

ca_status_t aal_uni_macsec_rx_nandi_cmdctrl_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN ca_uint8_t  rwn,
	CA_OUT ca_uint8_t *start_write)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t cmd_ctrl;
	ca_uint32_t wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;
	/* Start to read */
	cmd_ctrl.bf.said = said;
	cmd_ctrl.bf.rwn = rwn;
	cmd_ctrl.bf.start_write = 1;
	CA_NI_REG_WRITE(cmd_ctrl.wrd, (NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL ));

	/* Polling for read done */
	do {
		cmd_ctrl.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL );
	}while (cmd_ctrl.bf.start_write && wait_timeout--);

	if (wait_timeout <= 0) {

		return CA_E_TIMEOUT;
	}

	*start_write = cmd_ctrl.bf.start_write;
	return ret;
}

ca_status_t aal_uni_macsec_tx_nandi_cmdctrl_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN ca_uint8_t rwn,
	CA_IN ca_uint8_t start_write)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t cmd_ctrl;
	NI_HV_CORESEC10_TOP_NANDI_INTERRUPT_0_t     nandi_intr;
	ca_uint32_t wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;
	/* Start to write */
	cmd_ctrl.bf.said = said;
	cmd_ctrl.bf.rwn = rwn;
	cmd_ctrl.bf.start_write = start_write;
	CA_NI_REG_WRITE(cmd_ctrl.wrd,(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL + macsec_offset));

	wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;

	/* Polling for written done */
	do {
		cmd_ctrl.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL + macsec_offset);
	}while (cmd_ctrl.bf.start_write && wait_timeout--);

	if (wait_timeout <= 0) {
		ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
		return CA_E_TIMEOUT;
	}

	wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;
	nandi_intr.bf.cpu_cmd_executed = 0;
    do {
        nandi_intr.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_INTERRUPT_0 + macsec_offset);
    }while (!nandi_intr.bf.cpu_cmd_executed && wait_timeout--);

    if (wait_timeout <= 0) {
		ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
        return CA_E_TIMEOUT;
    }
	return ret;
}
ca_status_t aal_uni_macsec_tx_nandi_cmdctrl_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN ca_uint8_t  rwn,
	CA_OUT ca_uint8_t *start_write)
{
	ca_status_t ret = CA_E_OK;
	NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL_t cmd_ctrl;
	ca_uint32_t wait_timeout = __AAL_UNI_MACSEC_TBL_ACCESS_RETRY_MAX;
	/* Start to read */
	cmd_ctrl.bf.said = said;
	cmd_ctrl.bf.rwn = rwn;
	cmd_ctrl.bf.start_write = 1;
	CA_NI_REG_WRITE(cmd_ctrl.wrd, (NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL + macsec_offset));

	/* Polling for read done */
	do {
		cmd_ctrl.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL + macsec_offset);
	}while (cmd_ctrl.bf.start_write && wait_timeout--);

	if (wait_timeout <= 0) {

		return CA_E_TIMEOUT;
	}

	*start_write = cmd_ctrl.bf.start_write;
	return ret;
}

ca_status_t aal_uni_macsec_rx_sadb_tbl_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN aal_uni_macsec_tbl_cfg_msk_t msk,
	CA_IN aal_uni_macsec_tbl_cfg_t *cfg)
{
	aal_uni_macsec_sadb_ctrl_t SADB_Control;
	NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID_t data_field_valid;
	ca_status_t ret = CA_E_OK;
	int i, reg_addr;
	aal_uni_macsec_key_cfg_t reg_cfg;

	if ((NULL == cfg) || (said >64)){
		return CA_E_PARAM;
	}
	if (msk.s.an){
		if (cfg->an > __AAL_UNI_MACSEC_TBL_AN_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.sc){
		if (cfg->sc > __AAL_UNI_MACSEC_TBL_SC_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.scb){
		if (cfg->scb > __AAL_UNI_MACSEC_TBL_SCB_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.es){
		if (cfg->es > __AAL_UNI_MACSEC_TBL_ES_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.co){
		if (cfg->co > __AAL_UNI_MACSEC_TBL_CO_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.rlyen){
		if (cfg->rlyen > __AAL_UNI_MACSEC_TBL_RLYEN_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.sec_mode){
		if (cfg->sec_mode > __AAL_UNI_MACSEC_TBL_SEC_MODE_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.v_config){
		if (cfg->v_config > __AAL_UNI_MACSEC_TBL_V_CONFIG_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}

	if (msk.s.key0) {
		reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_KEY0_3 ;
		for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
			reg_cfg.wrd = ((ca_uint32_t)(cfg->key0[i*4]) << 24 )
				| ((ca_uint32_t)(cfg->key0[i*4 + 1]) << 16)
				| ((ca_uint32_t)(cfg->key0[i*4 + 2]) << 8)
				| ((ca_uint32_t)(cfg->key0[i*4 + 3]));
			CA_NI_REG_WRITE(reg_cfg.wrd, reg_addr);
			reg_addr += 4;
		}
	}

	if (msk.s.key1) {
		reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_KEY1_3 ;
		for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
			reg_cfg.wrd = ((ca_uint32_t)(cfg->key1[i*4]) << 24 )
				| ((ca_uint32_t)(cfg->key1[i*4 + 1]) << 16)
				| ((ca_uint32_t)(cfg->key1[i*4 + 2]) << 8)
				| ((ca_uint32_t)(cfg->key1[i*4 + 3]));
			CA_NI_REG_WRITE(reg_cfg.wrd, reg_addr);
			reg_addr += 4;
		}
	}

	if (msk.s.hash0) {
		reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_HASH0_3 ;
		for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
			reg_cfg.wrd = ((ca_uint32_t)(cfg->hash0[i*4]) << 24 )
				| ((ca_uint32_t)(cfg->hash0[i*4 + 1]) << 16)
				| ((ca_uint32_t)(cfg->hash0[i*4 + 2]) << 8)
				| ((ca_uint32_t)(cfg->hash0[i*4 + 3]));
			CA_NI_REG_WRITE(reg_cfg.wrd, reg_addr);
			reg_addr += 4;
		}
	}

	if (msk.s.hash1) {
		reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_HASH1_3 ;
		for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
			reg_cfg.wrd = ((ca_uint32_t)(cfg->hash1[i*4]) << 24 )
				| ((ca_uint32_t)(cfg->hash1[i*4 + 1]) << 16)
				| ((ca_uint32_t)(cfg->hash1[i*4 + 2]) << 8)
				| ((ca_uint32_t)(cfg->hash1[i*4 + 3]));
			CA_NI_REG_WRITE(reg_cfg.wrd, reg_addr);
			reg_addr += 4;
		}
	}

	if (msk.s.pn) {
		CA_NI_REG_WRITE(cfg->pn,(NI_HV_CORESEC10_TOP_NANDI_SADB_PacketNumber ));
	}

	if (msk.s.npn) {
		CA_NI_REG_WRITE(cfg->npn,(NI_HV_CORESEC10_TOP_NANDI_SADB_NextPacketNumber ));
	}

	
	SADB_Control.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_Control );
	
	if (msk.s.an) {
		SADB_Control.bf.an = cfg->an;
	}

	if (msk.s.scb) {
		SADB_Control.bf.scb = cfg->scb;
	}

	if (msk.s.es) {
		SADB_Control.bf.es = cfg->es;
	}

	if (msk.s.co) {
		SADB_Control.bf.co = cfg->co;
	}

	if (msk.s.rlyen) {
		SADB_Control.bf.rlyen = cfg->rlyen;
	}

	if (msk.s.sec_mode) {
		SADB_Control.bf.sec_mode = cfg->sec_mode;
	}

	if (msk.s.sc) {
		SADB_Control.bf.sc = cfg->sc;
	}

	if (msk.s.v_config) {
		SADB_Control.bf.v_config = cfg->v_config;
	}

	CA_NI_REG_WRITE(SADB_Control.wrd, (NI_HV_CORESEC10_TOP_NANDI_SADB_Control ));

	data_field_valid.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID );

	if (msk.s.key0_valid) {
		data_field_valid.bf.key0_vld = cfg->key0_valid;
	}

	if (msk.s.key1_valid) {
		data_field_valid.bf.key1_vld = cfg->key1_valid;
	}
	if (msk.s.hash0_valid) {
		data_field_valid.bf.hash0_vld = cfg->hash0_valid;
	}
	if (msk.s.hash1_valid) {
		data_field_valid.bf.hash1_vld = cfg->hash1_valid;
	}

	if (msk.s.pn_valid) {
		data_field_valid.bf.pn_vld = cfg->pn_valid;
	}
	if (msk.s.npn_valid) {
		data_field_valid.bf.npn_vld = cfg->npn_valid;
	}
	if (msk.s.an_valid) {
		data_field_valid.bf.an_vld = cfg->an_valid;
	}
	if (msk.s.sc_valid) {
		data_field_valid.bf.sc_vld = cfg->sc_valid;
	}
	if (msk.s.scb_valid) {
		data_field_valid.bf.scb_vld = cfg->scb_valid;
	}
	if (msk.s.es_valid) {
		data_field_valid.bf.es_vld = cfg->es_valid;
	}
	if (msk.s.co_valid) {
		data_field_valid.bf.co_vld = cfg->co_valid;
	}
	if (msk.s.rlyen_valid) {
		data_field_valid.bf.rlyen_vld = cfg->rlyen_valid;
	}
	if (msk.s.sec_mode_valid) {
		data_field_valid.bf.sec_mode_vld = cfg->sec_mode_valid;
	}
	if (msk.s.v_config_valid) {
		data_field_valid.bf.v_config_vld = cfg->v_config_valid;
	}
	CA_NI_REG_WRITE(data_field_valid.wrd, (NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID ));

	ret = aal_uni_macsec_rx_nandi_cmdctrl_set(device_id, said,1, 1);
	CA_RET_VALUE_CHECK(ret);

	return ret;
}

ca_status_t aal_uni_macsec_rx_sadb_tbl_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_OUT aal_uni_macsec_tbl_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	aal_uni_macsec_sadb_ctrl_t SADB_Control;
	NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID_t data_field_valid;
	int i, reg_addr;
	aal_uni_macsec_key_cfg_t reg_cfg;

	 if ((NULL == cfg) || (said >64)){
		return CA_E_PARAM;
	}
#if 0
	/* Start to read */
	cmd_ctrl.bf.said = said;
	cmd_ctrl.bf.rwn = 0;
	cmd_ctrl.bf.start_write = 1;
	CA_NI_REG_WRITE(cmd_ctrl.wrd, (NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL ));

	/* Polling for read done */
	do {
		cmd_ctrl.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL );
	}while (cmd_ctrl.bf.start_write && wait_timeout--);

	if (wait_timeout <= 0) {

		return CA_E_TIMEOUT;
	}

	/* read back data */
#endif
	/* Start to read */
	ret = aal_uni_macsec_rx_nandi_cmdctrl_set(device_id, said,0, 1);
	CA_RET_VALUE_CHECK(ret);
	
	reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_KEY0_3 ;

	for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
		reg_cfg.wrd  = CA_NI_REG_READ(reg_addr);
		cfg->key0[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
		cfg->key0[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
		cfg->key0[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
		cfg->key0[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
		reg_addr += 4;
	}

	reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_KEY1_3 ;

	for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
		reg_cfg.wrd  = CA_NI_REG_READ(reg_addr);
		cfg->key1[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
		cfg->key1[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
		cfg->key1[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
		cfg->key1[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
		reg_addr += 4;
	}

	reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_HASH0_3 ;

	for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
		reg_cfg.wrd  = CA_NI_REG_READ(reg_addr);
		cfg->hash0[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
		cfg->hash0[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
		cfg->hash0[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
		cfg->hash0[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
		reg_addr += 4;
	}

	reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_HASH1_3 ;

	for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
		reg_cfg.wrd  = CA_NI_REG_READ(reg_addr);
		cfg->hash1[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
		cfg->hash1[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
		cfg->hash1[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
		cfg->hash1[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
		reg_addr += 4;
	}

	cfg->pn = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_PacketNumber );
	cfg->npn = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_NextPacketNumber );

	SADB_Control.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_Control );
	cfg->an 	   = SADB_Control.bf.an 	 ;
	cfg->sc 	   = SADB_Control.bf.sc 	 ;
	cfg->scb	   = SADB_Control.bf.scb	 ;
	cfg->es 	   = SADB_Control.bf.es 	 ;
	cfg->co 	   = SADB_Control.bf.co 	 ;
	cfg->rlyen	   = SADB_Control.bf.rlyen	 ;
	cfg->sec_mode  = SADB_Control.bf.sec_mode;
	cfg->v_config  = SADB_Control.bf.v_config;

	data_field_valid.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID );
	cfg->v_config_valid =  data_field_valid.bf.v_config_vld ;
	cfg->sec_mode_valid =  data_field_valid.bf.sec_mode_vld ;
	cfg->rlyen_valid	  =  data_field_valid.bf.rlyen_vld	  ;
	cfg->co_valid	  =  data_field_valid.bf.co_vld 	  ;
	cfg->es_valid	  =  data_field_valid.bf.es_vld 	  ;
	cfg->scb_valid	  =  data_field_valid.bf.scb_vld	  ;
	cfg->sc_valid	  =  data_field_valid.bf.sc_vld 	  ;
	cfg->an_valid	  =  data_field_valid.bf.an_vld 	  ;
	cfg->pn_valid	  =  data_field_valid.bf.pn_vld 	  ;
	cfg->npn_valid	  =  data_field_valid.bf.npn_vld	  ;
	cfg->key0_valid   =  data_field_valid.bf.key0_vld	  ;
	cfg->key1_valid   =  data_field_valid.bf.key1_vld	  ;
	cfg->hash0_valid	  =  data_field_valid.bf.hash0_vld	  ;
	cfg->hash1_valid	  =  data_field_valid.bf.hash1_vld	  ;

	return ret;
}


ca_status_t aal_uni_macsec_tx_sadb_tbl_set(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_IN aal_uni_macsec_tbl_cfg_msk_t msk,
	CA_IN aal_uni_macsec_tbl_cfg_t *cfg)
{
	aal_uni_macsec_sadb_ctrl_t SADB_Control;
	NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID_t data_field_valid;
	ca_status_t ret = CA_E_OK;
	int i, reg_addr;
	aal_uni_macsec_key_cfg_t reg_cfg;

	if ((NULL == cfg) || (said >64)){
		return CA_E_PARAM;
	}
	if (msk.s.an){
		if (cfg->an > __AAL_UNI_MACSEC_TBL_AN_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.sc){
		if (cfg->sc > __AAL_UNI_MACSEC_TBL_SC_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.scb){
		if (cfg->scb > __AAL_UNI_MACSEC_TBL_SCB_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.es){
		if (cfg->es > __AAL_UNI_MACSEC_TBL_ES_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.co){
		if (cfg->co > __AAL_UNI_MACSEC_TBL_CO_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.rlyen){
		if (cfg->rlyen > __AAL_UNI_MACSEC_TBL_RLYEN_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.sec_mode){
		if (cfg->sec_mode > __AAL_UNI_MACSEC_TBL_SEC_MODE_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}
	if (msk.s.v_config){
		if (cfg->v_config > __AAL_UNI_MACSEC_TBL_V_CONFIG_MAX){
			ca_printf("%s:%d\r\n",__FUNCTION__,__LINE__);
			return CA_E_PARAM;
		}
	}

	if (msk.s.key0) {
		reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_KEY0_3 + macsec_offset;
		for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
			reg_cfg.wrd = ((ca_uint32_t)(cfg->key0[i*4]) << 24 )
				| ((ca_uint32_t)(cfg->key0[i*4 + 1]) << 16)
				| ((ca_uint32_t)(cfg->key0[i*4 + 2]) << 8)
				| ((ca_uint32_t)(cfg->key0[i*4 + 3]));
			CA_NI_REG_WRITE(reg_cfg.wrd, reg_addr);
			reg_addr += 4;
		}
	}

	if (msk.s.key1) {
		reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_KEY1_3 + macsec_offset;
		for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
			reg_cfg.wrd = ((ca_uint32_t)(cfg->key1[i*4]) << 24 )
				| ((ca_uint32_t)(cfg->key1[i*4 + 1]) << 16)
				| ((ca_uint32_t)(cfg->key1[i*4 + 2]) << 8)
				| ((ca_uint32_t)(cfg->key1[i*4 + 3]));
			CA_NI_REG_WRITE(reg_cfg.wrd, reg_addr);
			reg_addr += 4;
		}
	}

	if (msk.s.hash0) {
		reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_HASH0_3 + macsec_offset;
		for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
			reg_cfg.wrd = ((ca_uint32_t)(cfg->hash0[i*4]) << 24 )
				| ((ca_uint32_t)(cfg->hash0[i*4 + 1]) << 16)
				| ((ca_uint32_t)(cfg->hash0[i*4 + 2]) << 8)
				| ((ca_uint32_t)(cfg->hash0[i*4 + 3]));
			CA_NI_REG_WRITE(reg_cfg.wrd, reg_addr);
			reg_addr += 4;
		}
	}

	if (msk.s.hash1) {
		reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_HASH1_3 + macsec_offset;
		for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
			reg_cfg.wrd = ((ca_uint32_t)(cfg->hash1[i*4]) << 24 )
				| ((ca_uint32_t)(cfg->hash1[i*4 + 1]) << 16)
				| ((ca_uint32_t)(cfg->hash1[i*4 + 2]) << 8)
				| ((ca_uint32_t)(cfg->hash1[i*4 + 3]));
			CA_NI_REG_WRITE(reg_cfg.wrd, reg_addr);
			reg_addr += 4;
		}
	}

	if (msk.s.pn) {
		CA_NI_REG_WRITE(cfg->pn,(NI_HV_CORESEC10_TOP_NANDI_SADB_PacketNumber + macsec_offset));
	}

	if (msk.s.npn) {
		CA_NI_REG_WRITE(cfg->npn,(NI_HV_CORESEC10_TOP_NANDI_SADB_NextPacketNumber + macsec_offset));
	}

	SADB_Control.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_Control + macsec_offset);

	if (msk.s.an) {
		SADB_Control.bf.an = cfg->an;
	}

	if (msk.s.scb) {
		SADB_Control.bf.scb = cfg->scb;
	}

	if (msk.s.es) {
		SADB_Control.bf.es = cfg->es;
	}

	if (msk.s.co) {
		SADB_Control.bf.co = cfg->co;
	}

	if (msk.s.rlyen) {
		SADB_Control.bf.rlyen = cfg->rlyen;
	}

	if (msk.s.sec_mode) {
		SADB_Control.bf.sec_mode = cfg->sec_mode;
	}

	if (msk.s.sc) {
		SADB_Control.bf.sc = cfg->sc;
	}

	if (msk.s.v_config) {
		SADB_Control.bf.v_config = cfg->v_config;
	}

	CA_NI_REG_WRITE(SADB_Control.wrd, (NI_HV_CORESEC10_TOP_NANDI_SADB_Control + macsec_offset));

	data_field_valid.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID + macsec_offset);

	if (msk.s.key0_valid) {
		data_field_valid.bf.key0_vld = cfg->key0_valid;
	}

	if (msk.s.key1_valid) {
		data_field_valid.bf.key1_vld = cfg->key1_valid;
	}
	if (msk.s.hash0_valid) {
		data_field_valid.bf.hash0_vld = cfg->hash0_valid;
	}
	if (msk.s.hash1_valid) {
		data_field_valid.bf.hash1_vld = cfg->hash1_valid;
	}

	if (msk.s.pn_valid) {
		data_field_valid.bf.pn_vld = cfg->pn_valid;
	}
	if (msk.s.npn_valid) {
		data_field_valid.bf.npn_vld = cfg->npn_valid;
	}
	if (msk.s.an_valid) {
		data_field_valid.bf.an_vld = cfg->an_valid;
	}
	if (msk.s.sc_valid) {
		data_field_valid.bf.sc_vld = cfg->sc_valid;
	}
	if (msk.s.scb_valid) {
		data_field_valid.bf.scb_vld = cfg->scb_valid;
	}
	if (msk.s.es_valid) {
		data_field_valid.bf.es_vld = cfg->es_valid;
	}
	if (msk.s.co_valid) {
		data_field_valid.bf.co_vld = cfg->co_valid;
	}
	if (msk.s.rlyen_valid) {
		data_field_valid.bf.rlyen_vld = cfg->rlyen_valid;
	}
	if (msk.s.sec_mode_valid) {
		data_field_valid.bf.sec_mode_vld = cfg->sec_mode_valid;
	}
	if (msk.s.v_config_valid) {
		data_field_valid.bf.v_config_vld = cfg->v_config_valid;
	}

	CA_NI_REG_WRITE(data_field_valid.wrd, (NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID + macsec_offset));

	ret = aal_uni_macsec_tx_nandi_cmdctrl_set(device_id, said,1, 1);
	CA_RET_VALUE_CHECK(ret);

	return ret;
}

ca_status_t aal_uni_macsec_tx_sadb_tbl_get(
	CA_IN ca_device_id_t  device_id,
	CA_IN ca_uint32_t said,
	CA_OUT aal_uni_macsec_tbl_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	aal_uni_macsec_sadb_ctrl_t SADB_Control;
	NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID_t data_field_valid;
	int i, reg_addr;
	aal_uni_macsec_key_cfg_t reg_cfg;

	 if ((NULL == cfg) || (said >64)){
		return CA_E_PARAM;
	}
#if 0
	/* Start to read */
	cmd_ctrl.bf.said = said;
	cmd_ctrl.bf.rwn = 0;
	cmd_ctrl.bf.start_write = 1;
	CA_NI_REG_WRITE(cmd_ctrl.wrd, (NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL + macsec_offset));

	/* Polling for read done */
	do {
		cmd_ctrl.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_COMMAND_CONTROL + macsec_offset);
	}while (cmd_ctrl.bf.start_write && wait_timeout--);

	if (wait_timeout <= 0) {

		return CA_E_TIMEOUT;
	}

	/* read back data */
#endif
	ret = aal_uni_macsec_tx_nandi_cmdctrl_set(device_id, said,0, 1);
	CA_RET_VALUE_CHECK(ret);
	
	reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_KEY0_3 + macsec_offset;

	for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
		reg_cfg.wrd  = CA_NI_REG_READ(reg_addr);
		cfg->key0[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
		cfg->key0[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
		cfg->key0[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
		cfg->key0[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
		reg_addr += 4;
	}

	reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_KEY1_3 + macsec_offset;

	for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
		reg_cfg.wrd  = CA_NI_REG_READ(reg_addr);
		cfg->key1[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
		cfg->key1[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
		cfg->key1[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
		cfg->key1[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
		reg_addr += 4;
	}

	reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_HASH0_3 + macsec_offset;

	for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
		reg_cfg.wrd  = CA_NI_REG_READ(reg_addr);
		cfg->hash0[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
		cfg->hash0[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
		cfg->hash0[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
		cfg->hash0[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
		reg_addr += 4;
	}

	reg_addr = NI_HV_CORESEC10_TOP_NANDI_SADB_HASH1_3 + macsec_offset;

	for (i = 0; i < AAL_MACSEC_KEY_LEN/4; i++){
		reg_cfg.wrd  = CA_NI_REG_READ(reg_addr);
		cfg->hash1[i * 4] = (reg_cfg.wrd >> 24 ) & 0xFF;
		cfg->hash1[i * 4 + 1] = (reg_cfg.wrd >> 16 ) & 0xFF;
		cfg->hash1[i * 4 + 2] = (reg_cfg.wrd >> 8 ) & 0xFF;
		cfg->hash1[i * 4 + 3] = (reg_cfg.wrd) & 0xFF;
		reg_addr += 4;
	}

	cfg->pn = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_PacketNumber + macsec_offset);
	cfg->npn = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_NextPacketNumber + macsec_offset);

	SADB_Control.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_SADB_Control + macsec_offset);
	cfg->an 	   = SADB_Control.bf.an 	 ;
	cfg->sc 	   = SADB_Control.bf.sc 	 ;
	cfg->scb	   = SADB_Control.bf.scb	 ;
	cfg->es 	   = SADB_Control.bf.es 	 ;
	cfg->co 	   = SADB_Control.bf.co 	 ;
	cfg->rlyen	   = SADB_Control.bf.rlyen	 ;
	cfg->sec_mode  = SADB_Control.bf.sec_mode;
	cfg->v_config  = SADB_Control.bf.v_config;

	data_field_valid.wrd = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_NANDI_DATAFIELD_VALID + macsec_offset);
	cfg->v_config_valid =  data_field_valid.bf.v_config_vld ;
	cfg->sec_mode_valid =  data_field_valid.bf.sec_mode_vld ;
	cfg->rlyen_valid	  =  data_field_valid.bf.rlyen_vld	  ;
	cfg->co_valid 	  =  data_field_valid.bf.co_vld 	  ;
	cfg->es_valid 	  =  data_field_valid.bf.es_vld 	  ;
	cfg->scb_valid	  =  data_field_valid.bf.scb_vld	  ;
	cfg->sc_valid 	  =  data_field_valid.bf.sc_vld 	  ;
	cfg->an_valid 	  =  data_field_valid.bf.an_vld 	  ;
	cfg->pn_valid 	  =  data_field_valid.bf.pn_vld 	  ;
	cfg->npn_valid	  =  data_field_valid.bf.npn_vld	  ;
	cfg->key0_valid	  =  data_field_valid.bf.key0_vld	  ;
	cfg->key1_valid	  =  data_field_valid.bf.key1_vld	  ;
	cfg->hash0_valid	  =  data_field_valid.bf.hash0_vld	  ;
	cfg->hash1_valid	  =  data_field_valid.bf.hash1_vld	  ;

	return ret;
}

ca_status_t aal_uni_macsec_rx_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t 	port_id,
    CA_IN ca_uint8_t 	llid_idx,
    CA_OUT aal_uni_macsec_rx_stats_t *stats)
{
    ca_status_t ret = CA_E_OK;
    if (NULL == stats){
        return CA_E_ERROR;
    }
    if (llid_idx > 64){
        return CA_E_PARAM;
    }

	READ_INDIRCT_REG(device_id,llid_idx,NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS);

    stats->rx_pkt_is_not_valid                = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA0);
    stats->rx_pkt_number_less_than_lowest_pn= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA1);
    stats->rx_oct_protected_unencrypted = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA2)|
   										 ((ca_uint64_t)(CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3)&0x7ff) <<32);
	
    stats->rx_oct_protected_encrypted       = ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3)>>11)|
                                    		  ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA4)&0x3FFFFF)<<21)) ;
	
    stats->rx_rplen_less_than_lowest_pn     = ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA4)>>22)|
        									  ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA5)&0x3FFFFF)<<10));
	
	
    stats->rx_pkts_ok = ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA5)>>22)|
        				((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA6)&0x3FFFFF)<<10));

    stats->rx_pkt_invalid_with_vf_check     = ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA6)>>22)|
        						((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA7)&0x3FFFFF)<<10));
    stats->rx_pkt_not_valid_with_vf_strict_c_set   = ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA7)>>22)|
        						((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA8)&0x3FFFFF)<<10));
    stats->rx_pkt_not_using_sa = ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA8)>>22)|
        						 ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA9)&0x3FFFFF)<<10));

    return ret;
}

ca_status_t aal_uni_macsec_rx_error_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t  port_id,
    CA_OUT aal_uni_macsec_rx_error_stats_t *stats)
{
    ca_status_t ret = CA_E_OK;
    if (NULL == stats){
        return CA_E_ERROR;
    }
    
    stats->rx_pkt_untagged          = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsUnTagged);
    stats->rx_pkt_no_tag            = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsNoTag);
    stats->rx_pkt_bad_tag           = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsBadTag);
    stats->rx_pkt_unknow_sci        = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsUnKnownSCI);
    stats->rx_pkt_overrun           = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_MIB_InPktsOverrun);
    stats->rx_pkt_dec_invalid_co_err= CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_DEC_INVALID_CO_ERR_CNT);
    stats->rx_pkt_dec_auth_fail_err = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_DEC_AUTH_FAIL_ERR_CNT);
    stats->rx_pkt_dec_ver_num_err   = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_DEC_VER_NUM_ERR_CNT);
    stats->rx_pkt_dec_sl_msb_err    = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_MACSEC_DEC_SL_MSB_ERR_CNT);
    stats->rx_pkt_unknown_llid      = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_PKTS_UN_REG_LLID_CNT);
    stats->rx_pkt_dec_sl_msb_err    = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_RX_RUNT_PKTS_CNT);

    return ret;
}

ca_status_t aal_uni_macsec_tx_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN  ca_uint8_t llid_idx,
    CA_OUT aal_uni_macsec_tx_stats_t *stats)
{
    ca_status_t ret = CA_E_OK;

    if (NULL == stats){
        return CA_E_ERROR;
    }
    if (llid_idx >= 64){
        return CA_E_PARAM;
    }

	READ_INDIRCT_REG(device_id,llid_idx,(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_ACCESS + macsec_offset));

    stats->tx_pkt_protected = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA0 + macsec_offset);
    stats->tx_pkt_encrypted = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA1 + macsec_offset);
	
    stats->tx_oct_protected = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA2 + macsec_offset)|
                             ((ca_uint64_t)(CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3 + macsec_offset)&0x7ff) <<32);
	
    stats->tx_oct_encrypted = ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA3 + macsec_offset)>>11)|
                              ((CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_RX_MIB_MEM_DATA4 + macsec_offset)&0x3FFFFF)<<21)) ;
    
    return ret;
}

ca_status_t aal_uni_macsec_tx_error_stats_get(
    CA_IN ca_device_id_t  device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT aal_uni_macsec_tx_error_stats_t *stats)
{
    ca_status_t ret = CA_E_OK;

    if(NULL == stats){
        return CA_E_ERROR;
    }
    
    stats->tx_pkt_untagged          = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_MACSEC_MIB_OutPktsUnTagged + macsec_offset);
    stats->tx_pkt_too_long          = CA_NI_REG_READ(NI_HV_CORESEC10_TOP_CORESEC10_TX_MACSEC_MIB_OutPktsTooLong + macsec_offset);
                                 
    return ret;
}
EXPORT_SYMBOL(aal_uni_macsec_init);
EXPORT_SYMBOL(aal_uni_macsec_glb_cfg_set);
EXPORT_SYMBOL(aal_uni_macsec_glb_cfg_get);
EXPORT_SYMBOL(aal_uni_macsec_rx_sa_cfg_set);
EXPORT_SYMBOL(aal_uni_macsec_rx_sa_cfg_get);
EXPORT_SYMBOL(aal_uni_macsec_tx_sa_cfg_set);
EXPORT_SYMBOL(aal_uni_macsec_tx_sa_cfg_get);
EXPORT_SYMBOL(aal_uni_macsec_rx_cfg_set);
EXPORT_SYMBOL(aal_uni_macsec_rx_cfg_get);
EXPORT_SYMBOL(aal_uni_macsec_tx_cfg_set);
EXPORT_SYMBOL(aal_uni_macsec_tx_cfg_get);
EXPORT_SYMBOL(aal_uni_macsec_rx_nandi_cfg_set);
EXPORT_SYMBOL(aal_uni_macsec_rx_nandi_cfg_get);
EXPORT_SYMBOL(aal_uni_macsec_tx_nandi_cfg_set);
EXPORT_SYMBOL(aal_uni_macsec_tx_nandi_cfg_get);
EXPORT_SYMBOL(aal_uni_macsec_rx_nandi_cmdctrl_set);
EXPORT_SYMBOL(aal_uni_macsec_rx_nandi_cmdctrl_get);
EXPORT_SYMBOL(aal_uni_macsec_tx_nandi_cmdctrl_set);
EXPORT_SYMBOL(aal_uni_macsec_tx_nandi_cmdctrl_get);
EXPORT_SYMBOL(aal_uni_macsec_rx_sadb_tbl_set);
EXPORT_SYMBOL(aal_uni_macsec_rx_sadb_tbl_get);
EXPORT_SYMBOL(aal_uni_macsec_tx_sadb_tbl_set);
EXPORT_SYMBOL(aal_uni_macsec_tx_sadb_tbl_get);
EXPORT_SYMBOL(aal_uni_macsec_rx_said_set);
EXPORT_SYMBOL(aal_uni_macsec_rx_said_get);
EXPORT_SYMBOL(aal_uni_macsec_tx_said_set);
EXPORT_SYMBOL(aal_uni_macsec_tx_said_get);
EXPORT_SYMBOL(aal_uni_macsec_rx_stats_get);
EXPORT_SYMBOL(aal_uni_macsec_tx_stats_get);
EXPORT_SYMBOL(aal_uni_macsec_rx_error_stats_get);
EXPORT_SYMBOL(aal_uni_macsec_tx_error_stats_get);

