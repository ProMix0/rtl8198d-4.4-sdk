#include <soc/cortina/registers.h>
#include "osal_common.h"
#include "aal_common.h"
#include "aal_fdb.h"
#include "aal_mir.h"
#include "util_table.h"
#include <soc/cortina/registers.h>


/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  data declaration
%%____________________________________________________________________________*/

ca_uint g_mire_lock;

aal_mire_mir_rule_t mir_sess[NUMOF_MIRE_SESSION];

/*______________________________________________________________________________
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%  macro
%%____________________________________________________________________________*/

#define __MIRE_LOCK()    \
    do{ \
        __mire_lock();\
    }while(0)

#define __MIRE_UNLOCK()    \
    do{ \
        __mire_unlock();\
    }while(0)


void  __mire_lock(void)
{
    ca_spin_lock(g_mire_lock);
}

void  __mire_unlock(void)
{
    ca_spin_unlock(g_mire_lock);
}


ca_status_t aal_mire_init(
    ca_device_id_t     device_id
)
{
    ca_int32 ret = 0;

    ret = ca_spin_lock_init(&g_mire_lock, SPINLOCK_BH);

    memset(&mir_sess, 0, sizeof(mir_sess));

    return ret;
}

/*
ca_int32_t __aal_mire_ssid_alloc(
    ca_uint8_t ldpid,
    ca_uint8_t lspid
)
{
    ca_int32_t i;
    ca_int32_t ssid = -1;
    aal_mire_mir_rule_t *rul_p = NULL;

    for (i = 0; i < NUMOF_MIRE_SESSION; i++){
        rul_p = &mir_sess[i];
        if (MIR_DISABLE == rul_p->mode){
            L2_DEBUG("ssid:%d avaliable.\r\n", i);
            if (ssid != -1)
                ssid = i;
            continue;
        }

        if(rul_p->lspid == lspid || rul_p->ldpid == ldpid){
            L2_DEBUG("duplicate logical s/d pid.\r\n");
            return CA_E_ERROR;
        }
    }

    L2_DEBUG("ssid:%d\r\n", ssid);
    return ssid;
}

ca_int32_t __aal_mire_ssid_get(
    ca_uint8_t ldpid,
    ca_uint8_t lspid
)
{
    ca_int32_t i;
    ca_int32_t ssid = -1;
    aal_mire_mir_rule_t *rul_p = NULL;

    for (i = 0; i < NUMOF_MIRE_SESSION; i++){
        rul_p = &mir_sess[i];
        if(MIR_DISABLE != rul_p->mode &&
            (rul_p->lspid == lspid || rul_p->ldpid == ldpid)){
            L2_DEBUG("find logical s/d pid.\r\n");
            ssid = i;
            break;
        }
    }

    L2_DEBUG("ssid:%d\r\n", ssid);
    return ssid;
}

*/
ca_status_t aal_mire_ctrl_set(
    CA_IN ca_device_id_t          device_id,
    CA_IN aal_mire_mir_ctrl_mask_t  ctrl_mask,
    CA_IN aal_mire_mir_ctrl_t       *ctrl
)
{
    L2FE_MIRE_MIR_CTRL_t  mire_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __MIRE_LOCK();
	
	if (device_id == 0){
	    mire_ctrl.wrd  = CA_L2FE_REG_READ(L2FE_MIRE_MIR_CTRL);

	    if (ctrl_mask.bf.pkt_drop_src_mir_en)
	        mire_ctrl.bf.pkt_drop_src_mir_en = ctrl->pkt_drop_src_mir_en;
	    if (ctrl_mask.bf.pkt_keep_old)
	        mire_ctrl.bf.pkt_keep_old = ctrl->pkt_keep_old;
	    if (ctrl_mask.bf.lspid_remap)
	        mire_ctrl.bf.lspid_remap = ctrl->lspid_remap;

	    L2_DEBUG("write mire_ctrl:0x%x\r\n", mire_ctrl.wrd);
	    CA_L2FE_REG_WRITE(mire_ctrl.wrd, L2FE_MIRE_MIR_CTRL);
	}
#ifdef CONFIG_ARCH_CORTINA_G3HGU	
	else {

		mire_ctrl.wrd  = CA_8279_NE_REG_READ(L2FE_MIRE_MIR_CTRL);
		
		if (ctrl_mask.bf.pkt_drop_src_mir_en)
			mire_ctrl.bf.pkt_drop_src_mir_en = ctrl->pkt_drop_src_mir_en;
		if (ctrl_mask.bf.pkt_keep_old)
			mire_ctrl.bf.pkt_keep_old = ctrl->pkt_keep_old;
		if (ctrl_mask.bf.lspid_remap)
			mire_ctrl.bf.lspid_remap = ctrl->lspid_remap;
		
		L2_DEBUG("write mire_ctrl:0x%x\r\n", mire_ctrl.wrd);
		CA_8279_NE_REG_WRITE(mire_ctrl.wrd, L2FE_MIRE_MIR_CTRL);
	}

#endif

    __MIRE_UNLOCK();

    return CA_E_OK;
}


ca_status_t aal_mire_ctrl_get(
    CA_IN ca_device_id_t       device_id,
    CA_OUT aal_mire_mir_ctrl_t  *ctrl
)
{
    L2FE_MIRE_MIR_CTRL_t  mire_ctrl;

    AAL_ASSERT_RET(ctrl != NULL, CA_E_PARAM);

    __MIRE_LOCK();
	if (device_id == 0){
		mire_ctrl.wrd  = CA_L2FE_REG_READ(L2FE_MIRE_MIR_CTRL);
		
		L2_DEBUG("read mire_ctrl:0x%x\r\n", mire_ctrl.wrd);
		
		ctrl->pkt_drop_src_mir_en = mire_ctrl.bf.pkt_drop_src_mir_en;
		ctrl->pkt_keep_old = mire_ctrl.bf.pkt_keep_old;
		ctrl->lspid_remap = mire_ctrl.bf.lspid_remap;
	}
#ifdef CONFIG_ARCH_CORTINA_G3HGU	
	else {
		mire_ctrl.wrd  = CA_8279_NE_REG_READ(L2FE_MIRE_MIR_CTRL);
		
		L2_DEBUG("read mire_ctrl:0x%x\r\n", mire_ctrl.wrd);
		
		ctrl->pkt_drop_src_mir_en = mire_ctrl.bf.pkt_drop_src_mir_en;
		ctrl->pkt_keep_old = mire_ctrl.bf.pkt_keep_old;
		ctrl->lspid_remap = mire_ctrl.bf.lspid_remap;
	}
#endif



    __MIRE_UNLOCK();

    return CA_E_OK;
}



ca_status_t aal_mire_rule_set(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_int32_t                ssid,            /* 0 base */
    CA_IN aal_mire_mir_rule_mask_t  rule_mask,
    CA_IN aal_mire_mir_rule_t       *rule
)
{
    L2FE_MIRE_MIR_RULE_SSID_0_t  mire_rule;
    ca_uint32_t reg_addr;

    AAL_ASSERT_RET(rule != NULL && ssid < NUMOF_MIRE_SESSION, CA_E_PARAM);

    __MIRE_LOCK();
	if (device_id == 0){
		reg_addr = L2FE_MIRE_MIR_RULE_SSID_0 + 4 * ssid;
		mire_rule.wrd  = CA_L2FE_REG_READ(reg_addr);
		
		if (rule_mask.bf.flowid)
			mire_rule.bf.flowid = rule->flowid;
		if (rule_mask.bf.ldpid)
			mire_rule.bf.ldpid = rule->ldpid;
		if (rule_mask.bf.lspid)
			mire_rule.bf.lspid = rule->lspid;
		if (rule_mask.bf.mode)
			mire_rule.bf.mode = rule->mode;
		
		L2_DEBUG("write ssid:%d, mire_rule:0x%x, flow_id:%d, ldpid:%d, lspid:%d, mode:%d\r\n",
				ssid, mire_rule.wrd,
				mire_rule.bf.flowid, mire_rule.bf.ldpid, mire_rule.bf.lspid, mire_rule.bf.mode);
		CA_L2FE_REG_WRITE(mire_rule.wrd, reg_addr);
	}

#ifdef CONFIG_ARCH_CORTINA_G3HGU	
	else {
		reg_addr = L2FE_MIRE_MIR_RULE_SSID_0 + 4 * ssid;
		mire_rule.wrd  = CA_HGU_8279_NE_REG_READ(reg_addr);
		
		if (rule_mask.bf.flowid)
			mire_rule.bf.flowid = rule->flowid;
		if (rule_mask.bf.ldpid)
			mire_rule.bf.ldpid = rule->ldpid;
		if (rule_mask.bf.lspid)
			mire_rule.bf.lspid = rule->lspid;
		if (rule_mask.bf.mode)
			mire_rule.bf.mode = rule->mode;
		
		L2_DEBUG("write ssid:%d, mire_rule:0x%x, flow_id:%d, ldpid:%d, lspid:%d, mode:%d\r\n",
				ssid, mire_rule.wrd,
				mire_rule.bf.flowid, mire_rule.bf.ldpid, mire_rule.bf.lspid, mire_rule.bf.mode);
		CA_HGU_8279_NE_REG_WRITE(mire_rule.wrd, reg_addr);		
	}


#endif
    __MIRE_UNLOCK();

    return CA_E_OK;
}


/*
ca_status_t aal_mire_rule_set(
    CA_IN ca_device_id_t               device_id,
    CA_IN aal_mire_mir_rule_mask_t  rule_mask,
    CA_IN aal_mire_mir_rule_t       *rule
)
{
    ca_int32_t ssid = -1;
    ca_int32_t ret = CA_E_ERROR;

    AAL_ASSERT_RET(rule != NULL, CA_E_PARAM);

    ssid = aal_mire_ssid_alloc(rule->lspid, rule->ldpid);

    L2_DEBUG("ssid:%d\r\n", ssid);

    ret = __aal_mire_rule_set(device_id, ssid, rule_mask, rule);

    if (ret == CA_E_OK){
        mir_sess[ssid] = *rule;
    }

    return CA_E_OK;
}
*/
ca_status_t aal_mire_rule_get(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_int32_t            ssid,            /* 0 base */
    CA_OUT aal_mire_mir_rule_t   *rule
)
{
    L2FE_MIRE_MIR_RULE_SSID_0_t  mire_rule;
    ca_uint32_t reg_addr;

    AAL_ASSERT_RET(rule != NULL && ssid < NUMOF_MIRE_SESSION, CA_E_PARAM);

    __MIRE_LOCK();

	if (device_id == 0) {
		reg_addr = L2FE_MIRE_MIR_RULE_SSID_0 + 4 * ssid;
		
		mire_rule.wrd  = CA_L2FE_REG_READ(reg_addr);
		
		L2_DEBUG("read ssid:%d, mire_rule:0x%x\r\n", ssid, mire_rule.wrd);
		
		rule->flowid = mire_rule.bf.flowid;
		rule->ldpid = mire_rule.bf.ldpid;
		rule->lspid = mire_rule.bf.lspid;
		rule->mode = mire_rule.bf.mode;
	}

#ifdef CONFIG_ARCH_CORTINA_G3HGU
	else {
		reg_addr = L2FE_MIRE_MIR_RULE_SSID_0 + 4 * ssid;
		
		mire_rule.wrd  = CA_HGU_8279_NE_REG_READ(reg_addr);
		
		L2_DEBUG("read ssid:%d, mire_rule:0x%x\r\n", ssid, mire_rule.wrd);
		
		rule->flowid = mire_rule.bf.flowid;
		rule->ldpid = mire_rule.bf.ldpid;
		rule->lspid = mire_rule.bf.lspid;
		rule->mode = mire_rule.bf.mode;

	}
#endif


    __MIRE_UNLOCK();

    return CA_E_OK;
}
/*
ca_status_t aal_mire_rule_get(
    CA_IN ca_device_id_t           device_id,
    CA_OUT aal_mire_mir_rule_t   *rule
)
{
    ca_int32_t ssid = -1;
    ca_int32_t ret = CA_E_ERROR;

    AAL_ASSERT_RET(rule != NULL, CA_E_PARAM);

    ssid = aal_mire_ssid_get(rule->lspid, rule->ldpid);

    L2_DEBUG("ssid:%d\r\n", ssid);

    ret = __aal_mire_rule_get(device_id, ssid, rule);

    if (ret == CA_E_OK){
        ;
    }

    return CA_E_OK;
}

*/
ca_status_t aal_mire_act_set(
    CA_IN ca_device_id_t               device_id,
    CA_IN ca_int32_t                ssid,            /* 0 base */
    CA_IN aal_mire_mir_act_mask_t   act_mask,
    CA_IN aal_mire_mir_act_t        *act
)
{
    L2FE_MIRE_MIR_ACT_0_SSID_0_t  act0;
    L2FE_MIRE_MIR_ACT_1_SSID_0_t  act1;
    ca_uint32_t reg0, reg1;

    AAL_ASSERT_RET(act != NULL && ssid < NUMOF_MIRE_SESSION, CA_E_PARAM);

    __MIRE_LOCK();
	if (device_id == 0){
		reg0 = L2FE_MIRE_MIR_ACT_0_SSID_0 + 4 * ssid;
		reg1 = L2FE_MIRE_MIR_ACT_1_SSID_0 + 4 * ssid;
		act0.wrd  = CA_L2FE_REG_READ(reg0);
		act1.wrd  = CA_L2FE_REG_READ(reg1);
		
		if (act_mask.bf.lspid)
			act0.bf.lspid = act->lspid;
		if (act_mask.bf.pspid)
			act0.bf.pspid = act->pspid;
		if (act_mask.bf.ldpid)
			act0.bf.ldpid = act->ldpid;
		if (act_mask.bf.dot1p)
			act1.bf.dot1p = act->dot1p;
		if (act_mask.bf.vid)
			act1.bf.vid = act->vid;
		if (act_mask.bf.vlan_cmd)
			act1.bf.vlan_cmd = act->vlan_cmd;
		if (act_mask.bf.sc_ind)
			act1.bf.sc_ind = act->sc_ind;
		if (act_mask.bf.cos_vld)
			act1.bf.cos_vld = act->cos_vld;
		if (act_mask.bf.cos)
			act1.bf.cos = act->cos;
		if (act_mask.bf.sv_tpid_sel)
			act1.bf.sv_tpid_sel = act->sv_tpid_sel;
		if (act_mask.bf.cv_tpid_sel)
			act1.bf.cv_tpid_sel = act->cv_tpid_sel;
		
		L2_DEBUG("write ssid:%d, act0:[0x%x],0x%x, act1:[0x%x]0x%x\r\n", ssid, reg0, act0.wrd, reg1, act1.wrd);
		CA_L2FE_REG_WRITE(act0.wrd, reg0);
		CA_L2FE_REG_WRITE(act1.wrd, reg1);
		
		__MIRE_UNLOCK();
		
		L2_DEBUG("lspid:%d, pspid:%d, ldpid:%d, dot1p:%d, vid:%d, vlan_cmd:%d, sc_ind:%d, cos_vld:%d, cos:%d, sv_tpid_sel:%d, cv_tpid_sel:%d\r\n",
			act0.bf.lspid, act0.bf.pspid, act0.bf.ldpid, act1.bf.dot1p, act1.bf.vid, act1.bf.vlan_cmd,
			act1.bf.sc_ind, act1.bf.cos_vld, act1.bf.cos, act1.bf.sv_tpid_sel, act1.bf.cv_tpid_sel);

	}
#ifdef CONFIG_ARCH_CORTINA_G3HGU
	else {
		reg0 = L2FE_MIRE_MIR_ACT_0_SSID_0 + 4 * ssid;
		reg1 = L2FE_MIRE_MIR_ACT_1_SSID_0 + 4 * ssid;
		act0.wrd  = CA_HGU_8279_NE_REG_READ(reg0);
		act1.wrd  = CA_HGU_8279_NE_REG_READ(reg1);
		
		if (act_mask.bf.lspid)
			act0.bf.lspid = act->lspid;
		if (act_mask.bf.pspid)
			act0.bf.pspid = act->pspid;
		if (act_mask.bf.ldpid)
			act0.bf.ldpid = act->ldpid;
		if (act_mask.bf.dot1p)
			act1.bf.dot1p = act->dot1p;
		if (act_mask.bf.vid)
			act1.bf.vid = act->vid;
		if (act_mask.bf.vlan_cmd)
			act1.bf.vlan_cmd = act->vlan_cmd;
		if (act_mask.bf.sc_ind)
			act1.bf.sc_ind = act->sc_ind;
		if (act_mask.bf.cos_vld)
			act1.bf.cos_vld = act->cos_vld;
		if (act_mask.bf.cos)
			act1.bf.cos = act->cos;
		if (act_mask.bf.sv_tpid_sel)
			act1.bf.sv_tpid_sel = act->sv_tpid_sel;
		if (act_mask.bf.cv_tpid_sel)
			act1.bf.cv_tpid_sel = act->cv_tpid_sel;
		
		L2_DEBUG("write ssid:%d, act0:[0x%x],0x%x, act1:[0x%x]0x%x\r\n", ssid, reg0, act0.wrd, reg1, act1.wrd);
		CA_HGU_8279_NE_REG_WRITE(act0.wrd, reg0);
		CA_HGU_8279_NE_REG_WRITE(act1.wrd, reg1);
		
		__MIRE_UNLOCK();
		
		L2_DEBUG("lspid:%d, pspid:%d, ldpid:%d, dot1p:%d, vid:%d, vlan_cmd:%d, sc_ind:%d, cos_vld:%d, cos:%d, sv_tpid_sel:%d, cv_tpid_sel:%d\r\n",
			act0.bf.lspid, act0.bf.pspid, act0.bf.ldpid, act1.bf.dot1p, act1.bf.vid, act1.bf.vlan_cmd,
			act1.bf.sc_ind, act1.bf.cos_vld, act1.bf.cos, act1.bf.sv_tpid_sel, act1.bf.cv_tpid_sel);
	}
#endif


    return CA_E_OK;
}

/*
ca_status_t aal_mire_act_set(
    CA_IN ca_device_id_t               device_id,
    CA_IN aal_mire_mir_act_mask_t   act_mask,
    CA_IN aal_mire_mir_act_t        *act
)
{
    ca_int32_t ssid = -1;
    ca_int32_t ret = CA_E_ERROR;

    AAL_ASSERT_RET(act != NULL, CA_E_PARAM);

    ssid = aal_mire_ssid_get(act->lspid, act->ldpid);

    L2_DEBUG("ssid:%d\r\n", ssid);
    if (ssid == -1){
        ca_printf("No found session id, please config rule firstly.\r\n");
        return CA_E_ERROR;
    }
    ret = __aal_mire_act_set(device_id, ssid, act_mask, act);

    return CA_E_OK;
}

*/

ca_status_t aal_mire_act_get(
    CA_IN ca_device_id_t           device_id,
    CA_IN ca_int32_t            ssid,            /* 0 base */
    CA_OUT aal_mire_mir_act_t   *act
)
{
    L2FE_MIRE_MIR_ACT_0_SSID_0_t  act0;
    L2FE_MIRE_MIR_ACT_1_SSID_0_t  act1;
    ca_uint32_t reg0, reg1;

    AAL_ASSERT_RET(act != NULL && ssid < NUMOF_MIRE_SESSION, CA_E_PARAM);

    __MIRE_LOCK();
	if (device_id == 0){
		reg0 = L2FE_MIRE_MIR_ACT_0_SSID_0 + 4 * ssid;
		reg1 = L2FE_MIRE_MIR_ACT_1_SSID_0 + 4 * ssid;
		act0.wrd  = CA_L2FE_REG_READ(reg0);
		act1.wrd  = CA_L2FE_REG_READ(reg1);
		
		act->lspid = act0.bf.lspid;
		act->pspid = act0.bf.pspid;
		act->ldpid = act0.bf.ldpid;
		act->dot1p = act1.bf.dot1p;
		act->vid = act1.bf.vid;
		act->vlan_cmd = act1.bf.vlan_cmd;
		act->sc_ind = act1.bf.sc_ind;
		act->cos_vld = act1.bf.cos_vld;
		act->cos = act1.bf.cos;
		act->sv_tpid_sel = act1.bf.sv_tpid_sel;
		act->cv_tpid_sel = act1.bf.cv_tpid_sel;
		
		__MIRE_UNLOCK();
		
		L2_DEBUG("lspid:%d, pspid:%d, ldpid:%d, dot1p:%d, vid:%d, vlan_cmd:%d, sc_ind:%d, cos_vld:%d, cos:%d, sv_tpid_sel:%d, cv_tpid_sel:%d\r\n",
			act0.bf.lspid, act0.bf.pspid, act0.bf.ldpid, act1.bf.dot1p, act1.bf.vid, act1.bf.vlan_cmd,
			act1.bf.sc_ind, act1.bf.cos_vld, act1.bf.cos, act1.bf.sv_tpid_sel, act1.bf.cv_tpid_sel);

	}

#ifdef CONFIG_ARCH_CORTINA_G3HGU
	else {
		reg0 = L2FE_MIRE_MIR_ACT_0_SSID_0 + 4 * ssid;
		reg1 = L2FE_MIRE_MIR_ACT_1_SSID_0 + 4 * ssid;
		act0.wrd  = CA_HGU_8279_NE_REG_READ(reg0);
		act1.wrd  = CA_HGU_8279_NE_REG_READ(reg1);
		
		act->lspid = act0.bf.lspid;
		act->pspid = act0.bf.pspid;
		act->ldpid = act0.bf.ldpid;
		act->dot1p = act1.bf.dot1p;
		act->vid = act1.bf.vid;
		act->vlan_cmd = act1.bf.vlan_cmd;
		act->sc_ind = act1.bf.sc_ind;
		act->cos_vld = act1.bf.cos_vld;
		act->cos = act1.bf.cos;
		act->sv_tpid_sel = act1.bf.sv_tpid_sel;
		act->cv_tpid_sel = act1.bf.cv_tpid_sel;
		
		__MIRE_UNLOCK();
		
		L2_DEBUG("lspid:%d, pspid:%d, ldpid:%d, dot1p:%d, vid:%d, vlan_cmd:%d, sc_ind:%d, cos_vld:%d, cos:%d, sv_tpid_sel:%d, cv_tpid_sel:%d\r\n",
			act0.bf.lspid, act0.bf.pspid, act0.bf.ldpid, act1.bf.dot1p, act1.bf.vid, act1.bf.vlan_cmd,
			act1.bf.sc_ind, act1.bf.cos_vld, act1.bf.cos, act1.bf.sv_tpid_sel, act1.bf.cv_tpid_sel);
	
		}
#endif

	

    return CA_E_OK;
}

/*
ca_status_t aal_mire_act_get(
    CA_IN ca_device_id_t           device_id,
    CA_OUT aal_mire_mir_act_t   *act
)
{
    ca_int32_t ssid = -1;
    ca_int32_t ret = CA_E_ERROR;

    AAL_ASSERT_RET(act != NULL, CA_E_PARAM);

    ssid = aal_mire_ssid_get(act->lspid, act->ldpid);

    L2_DEBUG("ssid:%d\r\n", ssid);
    if (ssid == -1){
        ca_printf("No found session id, please config rule firstly.\r\n");
        return CA_E_ERROR;
    }
    ret = __aal_mire_act_get(device_id, ssid, act);

    return CA_E_OK;
}

*/
#ifdef __KERNEL__

EXPORT_SYMBOL(aal_mire_ctrl_set);
EXPORT_SYMBOL(aal_mire_ctrl_get);
EXPORT_SYMBOL(aal_mire_act_set);
EXPORT_SYMBOL(aal_mire_act_get);
EXPORT_SYMBOL(aal_mire_rule_get);
EXPORT_SYMBOL(aal_mire_rule_set);
EXPORT_SYMBOL(aal_mire_init);


#endif
