
#include "osal_common.h"
#include "ca-scfg/scfg.h"
#include "log.h"
#include "aal_xgpon.h"
#include "aal_puc.h"
#include "aal_pdc.h"
#include "aal_pon.h"
#include "aal_xgpon_actp.h"
#include "aal_xgpon_fec.h"
#include "aal_ploam.h"
#include "aal_ngpon2_chmgmt.h"
#include "aal_gpon.h"
#include "ploam_ngpon2.h"
#include "gpon.h"
#include "xgpon.h"
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
#include "aal_psds.h"
#endif

#define CA_GPON_DBG(fmt/*CODEGEN_IGNORE_TAG*/ ,...)  do {if (ca_pon_debug & PON_DBG_GPON_API) ca_printf("%s():"fmt, __func__,##__VA_ARGS__);}while(0)
extern xgpon_basic_info_t g_xgpn_basic_info;
extern ca_uint32_t aal_pon_mac_mode;
extern ca_uint32_t puc_voq_map_mode;
#define DEV_NUM 1
ca_gpon_port_cb_t  gpon_cb[DEV_NUM];
static ca_uint32_t sf_thrsld;
static ca_uint32_t sd_thrsld;

static ca_gpon_port_laser_polarity_t g_gpon_laser_rx_polarity = CA_GPON_PORT_LASER_POLARITY_ACTIVE_HI;
static ca_uint32_t queue_id_tmp = 0 ;
//static ca_uint32_t gem_id_tmp = 1 ;
static ca_uint32_t alloc_id_tmp = 10 ;
ca_uint8_t omci_msk_flag = 0 ;
ca_uint8_t omci_msk[CA_MSK_SIZE] = {0};

extern ca_uint8_t g_key_idx;
extern ca_uint8_t g_pon_tag[PON_TAG_SIZE];
#if defined(CONFIG_LUNA_G3_SERIES) || defined(CONFIG_ARCH_REALTEK_TAURUS)
ca_uint8_t g_admin_state = FALSE;
extern aal_gpon_port_cfg_t g_gpon_basic_info;
#endif

/* check the GPON Driver is ready/initialized-done or not */
#define CHK_GPON_CB_READY(device_id)    do {\
                                            if (device_id >= DEV_NUM || !gpon_cb[device_id].state) {\
                                                ca_printf("%s(): the GPON CB driver is not ready!\r\n", __func__); \
                                                return CA_E_UNAVAIL; }\
                                        }while(0)

#define CA_GPON_KEY_LEN_CHECK(val)         if(((val) != 16)){ \
                                                    ca_printf("%s:[%d] val %d!!\n", __func__, __LINE__, val); \
                                                    return CA_E_PARAM; \
                                                }
#define CA_GPON_TCONT_ID_CHECK(val)            if(((val) >= tcont_num_per_chip) || ((val) <0)) { \
                                                ca_printf("parameter error!!val is :%d\n",val);\
                                                return CA_E_PARAM; \
                                              }

#define CA_GPON_GEM_VALID_CHECK(val)          if(((val) != 1) && ((val) != 0)) { \
                                                ca_printf("parameter error!!val is :%p\n",val);\
                                                  return CA_E_PARAM; \
                                              }

#define CA_GPON_GEM_ID_CHECK(val)             if(((val) > 0x3FFF) || ((val) < 0)) { \
                                                ca_printf("parameter error!!val is :%d\n",val);\
                                                return CA_E_PARAM; \
                                            }

#define CA_GPON_QUEUE_VALID_CHECK(val)      if(((val) != 1) && ((val) != 0)) { \
                                                ca_printf("parameter error!!val is :%p\n",val);\
                                                  return CA_E_PARAM; \
                                              }

#define CA_GPON_QUEUE_ID_CHECK(val)         if(((val) > 127) || ((val) < 0)) { \
                                                ca_printf("parameter error!!val is :%p\n",val);\
                                                  return CA_E_PARAM; \
                                              }
#define CA_GPON_QUEUE_PRI_CHECK(val)         if(((val) > 7) || ((val) < 0)) { \
                                                ca_printf("parameter error!!val is :%p\n",val);\
                                                  return CA_E_PARAM; \
                                              }

#define CA_GPON_INVALID_VALUE 0xFFFF

#define CA_GET_GPON_PORT_CB(device_id)   &(gpon_cb[device_id])

#define CA_GET_TCONT_CB(device_id,tcont_idx)   \
                     &(CA_GET_GPON_PORT_CB(device_id)->tcont_cb[tcont_idx])

#define CA_GET_GEM_CB(device_id,gem_idx)   \
                     &(CA_GET_GPON_PORT_CB(device_id)->gem_cb[gem_idx])

#define CA_GET_QUEUE_CB(device_id,queue_idx)   \
                     &(CA_GET_GPON_PORT_CB(device_id)->queue_cb[queue_idx])


#define CA_GET_US_PRI_Q_ID(voq_id) \
             (CA_GPON_US_PRI_Q_ID_BASE+(CA_GPON_MAX_QUEUE_NUM-(voq_id)-1))

#define CA_GET_SNI_VOQ_ID(queue_id) \
             ((ca_uint16_t) (CA_GPON_MAX_QUEUE_NUM - \
                           ((queue_id)-CA_GPON_US_PRI_Q_ID_BASE)-1))

#define CA_IS_US_PRI_Q_ID_VALID(queue_id) \
             (((queue_id) >= CA_GPON_US_PRI_Q_ID_BASE) && \
              (CA_GET_SNI_VOQ_ID(queue_id) < CA_GPON_MAX_QUEUE_NUM))

#define CA_GET_DS_PRI_Q_ID(voq_id) \
             (CA_GPON_DS_PRI_Q_ID_BASE+(CA_MAX_NUM_UNI_QUEUE-(voq_id)-1))

#define CA_ERR_GOTO_FUNC_EXIT(device_id, status)            \
    {                                                              \
        if (CA_E_OK != (status)) {                                    \
            ca_printf("%s:[%d] return %d\n", __FUNCTION__, __LINE__, status); \
            goto FUNC_EXIT;                                          \
        }                                                           \
    }

#define CA_ERR_RETURN(device_id, status)                    \
    {                                                              \
        if (CA_E_OK != (status)) {                                    \
            ca_printf("%s:[%d] return %d\n", __FUNCTION__, __LINE__, status); \
            return (status);                           \
        }                                                           \
    }

#define CA_GPON_GET_OMCI_TCONT_INDEX(sni_port_num) \
          (((sni_port_num) == 0) ? (CA_GPON_OMCI_TCONT_INDEX_PON0) : \
                                    (CA_GPON_OMCI_TCONT_INDEX_PON1))




#define GPON_TIMER_MS2CLK(to_ms)    ((to_ms) << 3)
#define GPON_TIMER_CLK2MS(to_125us) ((to_125us) >> 3)
#if 0

/*****************************************************************************/
/* FUNCTION   : alloc_gem_cb                                                 */
/* INPUTS     :                                                              */
/*    o                                                                      */
/*    o                                                                      */
/* OUTPUTS    :                                                              */
/*    o ---                                                                  */
/* RETURNS    : GEM port cb pointer                                          */
/* DESCRIPTION:                                                              */
/*                                                                           */
/*****************************************************************************/
static ca_gpon_gem_cb_t *alloc_gem_cb (ca_device_id_t device_id,
                                       ca_uint16_t   pon_id,
                                       ca_uint16_t   gem_port_id,
                                       ca_uint8_t    gem_idx,
                                       ca_gpon_port_gem_port_direction_t   dir)
{
    ca_gpon_gem_cb_t *p_gem;

    if ((p_gem = (ca_gpon_gem_cb_t *)ca_malloc(sizeof(*p_gem))) != NULL) {
        memset(p_gem, 0, sizeof(*p_gem));

        p_gem->pon_id      = pon_id;
        p_gem->gem_port_id = gem_port_id;
        p_gem->gem_idx  = gem_idx;

        /* Initialize us_pri_q and ds_pri_q to invalid queue ids. Following
         * initialization is based on the fact for us_queues ds_queue numbers
         * and for ds_queues us_queue numbers are invalid
         */
        p_gem->us_pri_q    = CA_GPON_US_PRI_Q_ID_BASE;
        p_gem->ds_pri_q    = CA_GPON_DS_PRI_Q_ID_BASE;

        p_gem->dir         = dir;
        CA_GET_GPON_TM_CB(device_id)->p_gem_cb[gem_idx] = p_gem;
    }

    return p_gem;
}

/*****************************************************************************/
/* FUNCTION   : get_gem_cb                                                   */
/* INPUTS     :                                                              */
/*    o                                                                      */
/*    o                                                                      */
/* OUTPUTS    :                                                              */
/*    o ---                                                                  */
/* RETURNS    : GEM port cb pointer                                          */
/* DESCRIPTION:                                                              */
/*                                                                           */
/*****************************************************************************/
static ca_gpon_gem_cb_t *get_gem_cb (ca_device_id_t device_id,
                                     ca_uint16_t   pon_id,
                                     ca_uint16_t   gem_port_id)
{
    int                ix, cnt;
    ca_gpon_gem_cb_t *p_gem;

    cnt = sizeof(CA_GET_GPON_TM_CB(device_id)->p_gem_cb) /
          sizeof(CA_GET_GPON_TM_CB(device_id)->p_gem_cb[0]);

    for (ix = 0; ix < cnt; ix++) {
        p_gem = CA_GET_GPON_TM_CB(device_id)->p_gem_cb[ix];

        if (p_gem && p_gem->pon_id == pon_id &&
            p_gem->gem_port_id == gem_port_id)
            return p_gem;
    }

    return NULL;
}

/*****************************************************************************/
/* FUNCTION   : free_gem_cb                                                  */
/* INPUTS     :                                                              */
/*    o                                                                      */
/*    o                                                                      */
/* OUTPUTS    :                                                              */
/*    o ---                                                                  */
/* RETURNS    : cs_status_e                                                  */
/* DESCRIPTION:                                                              */
/*                                                                           */
/*****************************************************************************/
static ca_status_t free_gem_cb (ca_device_id_t device_id,
                                ca_uint16_t   pon_id,
                                ca_uint16_t   gem_port_id)
{
    ca_gpon_gem_cb_t *p_gem = get_gem_cb (device_id, pon_id, gem_port_id);

    if (p_gem) {
        CA_GET_GPON_TM_CB(device_id)->p_gem_cb[p_gem->gem_idx] = NULL;
        ca_free(p_gem);
    }
    return CA_E_OK;
}


/*****************************************************************************/
/* FUNCTION   : alloc_td_cb                                                  */
/* INPUTS     :                                                              */
/*    o                                                                      */
/*    o                                                                      */
/* OUTPUTS    :                                                              */
/*    o ---                                                                  */
/* RETURNS    : TD cb pointer                                                */
/* DESCRIPTION:                                                              */
/*                                                                           */
/*****************************************************************************/
static ca_gpon_td_cb_t *alloc_td_cb (ca_device_id_t  device_id,
                                     ca_uint16_t    td_id,
                                     ca_uint16_t     pol_id)
{
    ca_gpon_td_cb_t     **p_td;
    ca_gpon_tm_cb_t     *p_tm_cb;
    ca_uint16_t            idx = 0;

    p_tm_cb = CA_GET_GPON_TM_CB(device_id);
    for (; idx < CA_MAX_NUM_FLOW_POL; idx++) {
        p_td = &(p_tm_cb->p_td_cb[idx]);
        if (!*p_td) break;
    }

    if (CA_MAX_NUM_FLOW_POL == idx) {
        return (NULL);
    }

    if ((*p_td = (ca_gpon_td_cb_t *)ca_malloc(sizeof(**p_td))) != NULL) {
        memset(*p_td, 0, sizeof(**p_td));

        (*p_td)->gem_td_id = td_id;
        (*p_td)->pol_id    = pol_id;
    }

    return *p_td;
}


/*****************************************************************************/
/* FUNCTION   : get_td_cb                                                    */
/* INPUTS     :                                                              */
/*    o                                                                      */
/*    o                                                                      */
/* OUTPUTS    :                                                              */
/*    o ---                                                                  */
/* RETURNS    : TD cb pointer                                                */
/* DESCRIPTION:                                                              */
/*                                                                           */
/*****************************************************************************/
static ca_gpon_td_cb_t *get_td_cb (ca_device_id_t device_id,
                                   ca_uint16_t   td_id)
{
    int                 ix, cnt;
    ca_gpon_td_cb_t   *p_td;

    cnt = sizeof(CA_GET_GPON_TM_CB(device_id)->p_td_cb) /
          sizeof(CA_GET_GPON_TM_CB(device_id)->p_td_cb[0]);

    for (ix = 0; ix < cnt; ix++) {
        p_td = CA_GET_GPON_TM_CB(device_id)->p_td_cb[ix];

        if (p_td && p_td->gem_td_id == td_id)
            return p_td;
    }

    return NULL;
}

/*****************************************************************************/
/* FUNCTION   : get_td_cb                                                    */
/* INPUTS     :                                                              */
/*    o                                                                      */
/*    o                                                                      */
/* OUTPUTS    :                                                              */
/*    o ---                                                                  */
/* RETURNS    : TD cb pointer                                                */
/* DESCRIPTION:                                                              */
/*                                                                           */
/*****************************************************************************/
static ca_gpon_td_cb_t *get_td_cb_idx (ca_device_id_t device_id,
                                       ca_uint16_t   td_id, ca_uint32_t *ret_idx)
{
    ca_uint32_t          ix, cnt;
    ca_gpon_td_cb_t   *p_td;

    cnt = sizeof(CA_GET_GPON_TM_CB(device_id)->p_td_cb) /
          sizeof(CA_GET_GPON_TM_CB(device_id)->p_td_cb[0]);

    for (ix = 0; ix < cnt; ix++) {
        p_td = CA_GET_GPON_TM_CB(device_id)->p_td_cb[ix];

        if (p_td && p_td->gem_td_id == td_id) {
            *ret_idx = ix;
            return p_td;
        }
    }

    return NULL;
}


/*****************************************************************************/
/* FUNCTION   : free_td_cb                                                   */
/* INPUTS     :                                                              */
/*    o                                                                      */
/*    o                                                                      */
/* OUTPUTS    :                                                              */
/*    o ---                                                                  */
/* RETURNS    : cs_status_e                                                  */
/* DESCRIPTION:                                                              */
/*                                                                           */
/*****************************************************************************/
static ca_status_t free_td_cb (ca_device_id_t device_id,
                               ca_uint16_t   td_id)
{
    ca_uint32_t idx;
    ca_gpon_td_cb_t *p_td = get_td_cb_idx (device_id, td_id, &idx);

    if (p_td) {
        CA_GET_GPON_TM_CB(device_id)->p_td_cb[idx] = NULL;
        ca_free(p_td);
    }
    return CA_E_OK;
}
#endif

int mypow(int x, int y)
{
	int result = 0;
	int tmp = 0;
	if (y == 1)
		return x;
	tmp = mypow(x, y/2);
	if((y&1) != 0) //odd number
	{
		result = x * tmp * tmp;
	}
	else
	{
		result = tmp * tmp;
	}

	return result;
}

ca_status_t __gpon_intl_add_tcont_cb(ca_device_id_t    device_id,
                                     ca_uint16_t       port_id,
                                     ca_uint16_t       tcont_idx,
                                     ca_uint32_t       alloc_id,
                                     ca_gpon_port_tcont_scheduler_t *tcont_scheduler)
{
    ca_status_t ret = CA_E_OK;
    ca_uint16_t i = 0;
    ca_uint16_t idx = 0;
    ca_uint32_t onu_id = 0;
    if (tcont_idx >= tcont_num_per_chip){
        ca_printf("TCONT INDEX %d out of range 1-%d\r\n",tcont_idx,(tcont_num_per_chip-1));
        return CA_E_ERROR;
    }
    
    ret = ca_gpon_port_onu_id_get(device_id,port_id,&onu_id);
    CA_RET_VALUE_CHECK(ret);
    if(alloc_id == onu_id)
    {
        ca_printf("FUNC %s LINE %d :Traffic alloc_id %d can't be same as onu_id %d!\r\n",__func__,__LINE__,alloc_id,onu_id);
        return CA_E_ERROR;
    }
    
    for(i = 0; i < tcont_num_per_chip; i++){
        if((gpon_cb[device_id].tcont_cb[i].valid )
            &&(gpon_cb[device_id].tcont_cb[i].alloc_id == alloc_id)){
            if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
            {
                //update configuration
                gpon_cb[device_id].tcont_cb[i].pon_port_num  = port_id;
                gpon_cb[device_id].tcont_cb[i].sched_mode    = tcont_scheduler->mode;
                gpon_cb[device_id].tcont_cb[i].num_sp_queues = tcont_scheduler->sp_queue_num;
                for(idx = 0;idx <queue_num_per_tcont; idx++){
                    gpon_cb[device_id].tcont_cb[i].queue_cb[idx].queue_weight = tcont_scheduler->weight_queue[idx];
                }
                return CA_E_EXISTS;
            }
            ca_printf("FUNC %s LINE %d :alloc_id %d existed!\r\n",__func__,__LINE__,alloc_id);
            return CA_E_EXISTS;
        }
    }
        
    if(!gpon_cb[device_id].tcont_cb[tcont_idx].valid ){
        gpon_cb[device_id].tcont_cb[tcont_idx].valid         = TRUE;
        gpon_cb[device_id].tcont_cb[tcont_idx].tcont_id      = tcont_idx;
        gpon_cb[device_id].tcont_cb[tcont_idx].alloc_id      = alloc_id;
        gpon_cb[device_id].tcont_cb[tcont_idx].pon_port_num  = port_id;
        gpon_cb[device_id].tcont_cb[tcont_idx].sched_mode = tcont_scheduler->mode;
        gpon_cb[device_id].tcont_cb[tcont_idx].num_sp_queues = tcont_scheduler->sp_queue_num;
        for(i = 0;i <queue_num_per_tcont; i++){
            gpon_cb[device_id].tcont_cb[tcont_idx].queue_cb[i].queue_weight = tcont_scheduler->weight_queue[i];
        }
        return CA_E_OK;
    }else{
        ca_printf("FUNC %s LINE %d :ADD tcont cb fail\r\n",__func__,__LINE__);
        return CA_E_ERROR;
    }
    return CA_E_ERROR;
}


/* Internal API to delete a tcont from the db */
ca_status_t __gpon_intl_delete_tcont_cb(ca_device_id_t     device_id,
                                        ca_uint16_t      port_id,
                                        ca_uint16_t      tcont_index)
{
    gpon_cb[device_id].tcont_cb[tcont_index].valid = FALSE;

    return CA_E_OK;

}

/* Internal API to find a tcont-id in the db
 * It returns the corresponding alloc id, tcont index & GEM ports
 */
ca_gpon_tcont_cb_t *__gpon_intl_find_tcont_cb (
    ca_device_id_t     device_id,
    ca_uint16_t        tcont_id,
    ca_uint16_t        *p_tcont_index)
{
    ca_uint16_t                  indx1;

    for (indx1 = 1; indx1 < tcont_num_per_chip; indx1++) {
          if ((TRUE == gpon_cb[device_id].tcont_cb[indx1].valid)
          && (tcont_id == gpon_cb[device_id].tcont_cb[indx1].tcont_id)) {
            if (p_tcont_index) {
                *p_tcont_index = indx1;
                return (&(gpon_cb[device_id].tcont_cb[indx1]));
            }
        }
    }

    return (NULL);
}

ca_boolean_t __gpon_intl_gem_tbl_is_empty (
    ca_device_id_t     device_id,
    ca_boolean_t    data_only)
{
    ca_uint16_t  indx1 = 0;
    ca_uint16_t  idx_start = data_only ? 1 : 0;

    if (!gpon_cb[device_id].state) {
        return (TRUE);
    }

    for (indx1 = idx_start; indx1 < CA_GPON_MAX_GEM_PORTS; indx1++) {
        if (gpon_cb[device_id].gem_cb[indx1].valid) {
            return FALSE;
        }
    }

    return (TRUE);
}


ca_boolean_t __gpon_intl_tcont_tbl_is_empty (
    ca_device_id_t     device_id,
    ca_uint16_t     tcont)
{
    ca_uint16_t  indx1 = 0;

    if (!gpon_cb[device_id].state) {
        return (TRUE);
    }

    for (indx1 = 0; indx1 < CA_GPON_MAX_GEM_PORTS; indx1++) {
        if (gpon_cb[device_id].gem_cb[indx1].valid
            && gpon_cb[device_id].gem_cb[indx1].tcont_id == tcont) {
            return FALSE;
        }
    }

    return (TRUE);
}

ca_gpon_port_gem_cb_t *__gpon_intl_find_gem_cb (
    ca_device_id_t     device_id,
    ca_uint16_t        gem_index)
{
	int i = 0;
    ca_uint16_t intern_gem_idx = 0;
	ca_uint16_t cos = 0;
	ca_uint16_t tcont_idx = 0;

	if((puc_voq_map_mode == AAL_PUC_VOQ_MODE_8Q)
		||(puc_voq_map_mode == AAL_PUC_VOQ_MODE_4Q)){
		intern_gem_idx = gem_index&0xFF;
		cos = (gem_index>>8)&0x7;
		tcont_idx = (gem_index>>11)&0x1F;
		ca_printf("func %s line %d :intern_gem_idx %d, cos %d, tcont_idx %d\r\n",__FUNCTION__,__LINE__,
		intern_gem_idx,cos,tcont_idx);
	}else if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP){
		intern_gem_idx = gem_index&0xFF;
		ca_printf("func %s line %d :intern_gem_idx %d\r\n",__FUNCTION__,__LINE__,
		intern_gem_idx);
	}
	
	if(intern_gem_idx < CA_GPON_MAX_GEM_PORTS)
	{
		for(i = 8; i<CA_GPON_MAX_GEM_PORTS; i++){
			
			if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP){
				if(i == intern_gem_idx){
					return (&(gpon_cb[device_id].gem_cb[i]));
				}
			}else if((puc_voq_map_mode == AAL_PUC_VOQ_MODE_8Q)
					||(puc_voq_map_mode == AAL_PUC_VOQ_MODE_4Q)){
                if (i == intern_gem_idx){
                    if (CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN != gpon_cb[device_id].gem_cb[i].direction){
                        if ((gpon_cb[device_id].gem_cb[i].tcont_id == tcont_idx)
                          &&(gpon_cb[device_id].gem_cb[i].up_queue == cos)){
                            return (&(gpon_cb[device_id].gem_cb[i]));
                        }
                        else{
                            return NULL;
                        }
                    }
                    else{
                        return (&(gpon_cb[device_id].gem_cb[i]));
                    }
                }
			}
		}
		return NULL;
	}else{
		ca_printf("Invalid gem_index %d\r\n",intern_gem_idx);
		return NULL;
	}
	return NULL;
}
#if 0
ca_gpon_port_gem_port_t *__gpon_intl_find_gem_cb (
    ca_device_id_t     device_id,
    ca_uint16_t                  gem_index,
    ca_uint16_t                  *gem_id)
{
    ca_uint16_t                  indx1;

    for (indx1 = 0; indx1 < CA_GPON_MAX_GEM_PORTS; indx1++) {
        if ((TRUE == gpon_cb[device_id].gem_cb[indx1].valid)&&(indx1 == gem_index)) {
            if (gem_id) {
                *gem_id = gpon_cb[device_id].gem_cb[indx1].gem_port_id;
            }

            return (&(gpon_cb[device_id].gem_cb[indx1]));
        }
    }

    return (NULL);
}
#endif

ca_gpon_gem_tcont_map_t *__gpon_intl_find_gem_tcont_map(
    ca_device_id_t     device_id,
    ca_uint16_t                 gem_id,
    ca_uint16_t                  *p_gem_index)
{
    ca_uint16_t                  indx1;

    for (indx1 = 0; indx1 < CA_GPON_MAX_GEM_PORTS; indx1++) {
        if ((TRUE == gpon_cb[device_id].gem_tcont_map[indx1].valid) && (gem_id  == gpon_cb[device_id].gem_tcont_map[indx1].gem_port_id)) {
            if (p_gem_index) {
                *p_gem_index = indx1;
            }

            return (&(gpon_cb[device_id].gem_tcont_map[indx1]));
        }
    }

    return (NULL);
}

ca_gpon_tcont_cb_t *__gpon_intl_get_tcont_id_by_alloc_id (
    ca_device_id_t     device_id,
    ca_uint32_t                  alloc_id,
    ca_uint32_t                  *tcont_id,
    ca_uint32_t                  *p_tcont_index)
{
    ca_uint16_t                  indx1;

    for (indx1 = 1; indx1 <tcont_num_per_chip; indx1++) {
        if ((TRUE == gpon_cb[device_id].tcont_cb[indx1].valid) &&
            (alloc_id == gpon_cb[device_id].tcont_cb[indx1].alloc_id)) {
            if (p_tcont_index) {
                *p_tcont_index = indx1;
                *tcont_id = gpon_cb[device_id].tcont_cb[indx1].tcont_id;
            }

            return (&(gpon_cb[device_id].tcont_cb[indx1]));
        }
    }

    return (NULL);
}


ca_gpon_tcont_cb_t *__gpon_intl_get_alloc_id_by_tcont_id (
    ca_device_id_t     device_id,
    ca_uint32_t                  tcont_id,
    ca_uint32_t                  *alloc_id,
    ca_uint32_t                  *p_tcont_index)
{
    ca_uint16_t                  indx1;

    for (indx1 = 1; indx1 <tcont_num_per_chip; indx1++) {
        if ((TRUE == gpon_cb[device_id].tcont_cb[indx1].valid) &&
            (tcont_id == gpon_cb[device_id].tcont_cb[indx1].tcont_id)) {
            if (p_tcont_index) {
                *p_tcont_index = indx1;
                *alloc_id = gpon_cb[device_id].tcont_cb[indx1].alloc_id;
            }

            return (&(gpon_cb[device_id].tcont_cb[indx1]));
        }
    }

    return (NULL);
}

ca_status_t  __gpon_intl_get_tcont_idx (
    ca_device_id_t     device_id,
    ca_uint16_t                  gem_port_id,
    ca_uint16_t                  *p_tcont_idx)
{
    ca_uint16_t                  indx1;

    for (indx1 = 0; indx1 <CA_GPON_MAX_GEM_PORTS; indx1++) {
        if ((TRUE == gpon_cb[device_id].gem_tcont_map[indx1].valid) &&
            (gem_port_id == gpon_cb[device_id].gem_tcont_map[indx1].gem_port_id)) {
            if (p_tcont_idx) {
                *p_tcont_idx = gpon_cb[device_id].gem_tcont_map[indx1].tcont_idx;
            }

            return CA_E_OK;
        }
    }

    return CA_E_ERROR;
}

ca_status_t  __gpon_intl_get_gem_idx (
    ca_device_id_t     device_id,
    ca_uint16_t                  gem_port_id,
    ca_uint16_t                  *gem_idx)
{
    ca_uint16_t                  indx1;

    for (indx1 = 0; indx1 <CA_GPON_MAX_GEM_PORTS; indx1++) {
       if (gpon_cb[device_id].gem_cb[indx1].gem_port_id == gem_port_id) {
            if (gem_idx) {
                *gem_idx = indx1;
            }
            return CA_E_OK;
        }
    }

    return CA_E_ERROR;
}


ca_status_t __gpon_get_gem_info(ca_device_id_t device_id, ca_uint16_t port_id,
                                ca_uint32_t gem_port_id,
                                ca_gpon_port_gem_port_direction_t *p_direction,
                                ca_boolean_t *p_aes_enabled,
                                ca_boolean_t *p_omci_port,
                                ca_uint8_t *p_index)

{
    ca_status_t ret = CA_E_OK;
    ca_int16_t omci_port;
    ca_boolean_t gem_found = FALSE;
    aal_xgpon_usbg_gem_config_t config;
    int loopcount;

    /* Check if this port corresponds to one of the GEM ports else exit
    if (gem_port_idx >= CA_GPON_MAX_GEM_PORTS) {

    return CA_E_PARAM;
    } */

    /* NULL checks */
    if ((NULL == p_omci_port) || ( NULL == p_direction) || (NULL == p_aes_enabled) || (NULL == p_index)) {
        return CA_E_PARAM;
    }

    /*if(PAL_MUTEX_TAKE(dev_cb->sem.gpon, PAL_MUTEX_TIMEOUT) != CS_SUCCESS){
    SDL_ERR_PRINT(device_id, CS_MOD_DRVR_GPON, CS_ERR_GLBL_MUTEX_TAKE_FAIL,
    "Mutex take failed \n");
    return CS_ERR_GLBL_MUTEX_TAKE_FAIL;
    }
    */
    for (loopcount = 0; loopcount <CA_GPON_MAX_GEM_PORTS; loopcount++) {
        if (gpon_cb[device_id].gem_cb[loopcount].gem_port_id == gem_port_id) {

            *p_direction = (ca_gpon_port_gem_port_direction_t)0;
            gem_found = TRUE;

            *p_direction = gpon_cb[device_id].gem_cb[loopcount].direction;

            /* direction AAL_GPON_GEM_DIRECTION_BOTH is taken care above */

            if (*p_direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP) {
                ret = aal_xgpon_usbg_xgem_cfg_get(device_id, loopcount, &config);
                *p_aes_enabled = config.aes_mode;
            }

            *p_omci_port = FALSE;

            if (omci_port == gpon_cb[device_id].gem_cb[loopcount].gem_port_id) {
                *p_omci_port = TRUE;
            }
            *p_index = loopcount;
            break;
        } /* if port id done */
    } /* for loop done */


    /*PAL_MUTEX_GIVE(dev_cb->sem.gpon);*/

    return CA_E_OK ;
}

/* Internal API to delete a gem from tcont db */
ca_status_t __gpon_update_gem_tcont_map(
                                    ca_device_id_t      device_id,
                                    ca_uint16_t      tcont_id,
                                    ca_uint16_t      gem_port_id)
{
    ca_uint16_t                  indx1;

    for (indx1 = 0; indx1 <CA_GPON_MAX_GEM_PORTS; indx1++) {
        if ((gpon_cb[device_id].gem_tcont_map[indx1].valid == TRUE)
            &&(gem_port_id == gpon_cb[device_id].gem_tcont_map[indx1].gem_port_id)
            &&(tcont_id == gpon_cb[device_id].gem_tcont_map[indx1].tcont_idx)) {
                gpon_cb[device_id].gem_tcont_map[indx1].valid = FALSE;
                gpon_cb[device_id].gem_tcont_map[indx1].gem_port_id = CA_GPON_INVALID_VALUE;
                gpon_cb[device_id].gem_tcont_map[indx1].tcont_idx = CA_GPON_INVALID_VALUE;
                return CA_E_OK;
        }
    }
    ca_printf("%s:%d:: update gem_tcont_map fail!!\r\n",__func__,__LINE__);
    return CA_E_ERROR;
}

/* Internal API to add a gem to tcont db */
ca_status_t __gpon_intl_add_gem(
	ca_device_id_t			device_id,
	ca_uint16_t				port_id,
	ca_uint16_t				tcont_index,
	ca_boolean_t			is_omci_port,
	ca_uint16_t				*internal_gem_idx,
	ca_gpon_port_gem_port_t *gem_port_cfg)
{
	ca_uint16_t index = 0;
	ca_boolean_t gem_op_flag = FALSE;
    /* check for already existing gem-id ,update gem cfg*/
	if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP){
		index = tcont_index * queue_num_per_tcont + gem_port_cfg->up_queue;
		if(!gpon_cb[device_id].gem_cb[index].valid){
			gpon_cb[device_id].gem_cb[index].valid = TRUE;
			gpon_cb[device_id].gem_cb[index].gem_index = index; 
		    gpon_cb[device_id].gem_cb[index].gem_port_id = gem_port_cfg->gem_port_id;
		    gpon_cb[device_id].gem_cb[index].tcont_id = gem_port_cfg->tcont_id;
		    gpon_cb[device_id].gem_cb[index].aes_key_ring = gem_port_cfg->aes_key_ring;
			gpon_cb[device_id].gem_cb[index].aes_state = ((gem_port_cfg->aes_key_ring == 0) ? 0:1);

		    if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP) {
		        gpon_cb[device_id].gem_cb[index].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_UP;
		        gpon_cb[device_id].gem_cb[index].up_queue =  gem_port_cfg->up_queue;
				gpon_cb[device_id].gem_cb[index].up_traffic_descriptor = gem_port_cfg->up_traffic_descriptor;

		    } else if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN) {
		        gpon_cb[device_id].gem_cb[index].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN;
		        gpon_cb[device_id].gem_cb[index].dn_queue =  gem_port_cfg->dn_queue;
				gpon_cb[device_id].gem_cb[index].dn_traffic_descriptor = gem_port_cfg->dn_traffic_descriptor;
		    } else if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI) {
		        gpon_cb[device_id].gem_cb[index].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_BI;
		        gpon_cb[device_id].gem_cb[index].up_queue =  gem_port_cfg->up_queue;
		        gpon_cb[device_id].gem_cb[index].dn_queue =  gem_port_cfg->dn_queue;
				gpon_cb[device_id].gem_cb[index].up_traffic_descriptor = gem_port_cfg->up_traffic_descriptor;
				gpon_cb[device_id].gem_cb[index].dn_traffic_descriptor = gem_port_cfg->dn_traffic_descriptor;
		    }
		    //update gem tcont mapping cb
		    gpon_cb[device_id].gem_tcont_map[index].gem_port_id = gem_port_cfg->gem_port_id;
		    gpon_cb[device_id].gem_tcont_map[index].tcont_idx = gem_port_cfg->tcont_id;;
		    gpon_cb[device_id].gem_tcont_map[index].valid = TRUE;
			*internal_gem_idx = index;
			gem_op_flag = TRUE;
		}
	}
	else{
	    for(index = 8; index < CA_GPON_MAX_GEM_PORTS; index++){
			if(!gpon_cb[device_id].gem_cb[index].valid){
			    gpon_cb[device_id].gem_cb[index].valid = TRUE;
				gpon_cb[device_id].gem_cb[index].gem_index = index; 
			    gpon_cb[device_id].gem_cb[index].gem_port_id = gem_port_cfg->gem_port_id;
			    gpon_cb[device_id].gem_cb[index].tcont_id = gem_port_cfg->tcont_id;
			    gpon_cb[device_id].gem_cb[index].aes_key_ring = gem_port_cfg->aes_key_ring;
				gpon_cb[device_id].gem_cb[index].aes_state = ((gem_port_cfg->aes_key_ring == 0) ? 0:1);

			    if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP) {
			        gpon_cb[device_id].gem_cb[index].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_UP;
			        gpon_cb[device_id].gem_cb[index].up_queue =  gem_port_cfg->up_queue;
					gpon_cb[device_id].gem_cb[index].up_traffic_descriptor = gem_port_cfg->up_traffic_descriptor;

			    } else if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN) {
			        gpon_cb[device_id].gem_cb[index].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN;
			        gpon_cb[device_id].gem_cb[index].dn_queue =  gem_port_cfg->dn_queue;
					gpon_cb[device_id].gem_cb[index].dn_traffic_descriptor = gem_port_cfg->dn_traffic_descriptor;
			    } else if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI) {
			        gpon_cb[device_id].gem_cb[index].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_BI;
			        gpon_cb[device_id].gem_cb[index].up_queue =  gem_port_cfg->up_queue;
			        gpon_cb[device_id].gem_cb[index].dn_queue =  gem_port_cfg->dn_queue;
					gpon_cb[device_id].gem_cb[index].up_traffic_descriptor = gem_port_cfg->up_traffic_descriptor;
					gpon_cb[device_id].gem_cb[index].dn_traffic_descriptor = gem_port_cfg->dn_traffic_descriptor;
			    }
			    //update gem tcont mapping cb
			    gpon_cb[device_id].gem_tcont_map[index].gem_port_id = gem_port_cfg->gem_port_id;
			    gpon_cb[device_id].gem_tcont_map[index].tcont_idx = gem_port_cfg->tcont_id;;
			    gpon_cb[device_id].gem_tcont_map[index].valid = TRUE;
				*internal_gem_idx = index;
				gem_op_flag = TRUE;
				break;
			}
		}
	}

    if(!gem_op_flag){
        ca_printf("ERROR: FUNC %s LINE %d: Add gem configuration fail, no more space.\r\n",__func__,__LINE__);
        return CA_E_FULL;
    }
    return CA_E_OK;
}

ca_status_t __gpon_intl_update_gem(
    ca_device_id_t          device_id,
    ca_uint16_t             port_id,
    ca_uint16_t             intern_gem_idx,
    ca_gpon_port_gem_port_t *gem_port_cfg)
{
    ca_printf("func %s line %d :intern_gem_idx %d, gemid %d, tcont_idx %d\r\n",__func__,__LINE__,
        intern_gem_idx,gem_port_cfg->gem_port_id,gem_port_cfg->tcont_id);
    if(intern_gem_idx < CA_GPON_MAX_GEM_PORTS)
    {
        gpon_cb[device_id].gem_cb[intern_gem_idx].gem_port_id = gem_port_cfg->gem_port_id;
        gpon_cb[device_id].gem_cb[intern_gem_idx].aes_key_ring = gem_port_cfg->aes_key_ring;
        gpon_cb[device_id].gem_cb[intern_gem_idx].aes_state = ((gem_port_cfg->aes_key_ring == 0) ? 0:1);

        if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP) {
            gpon_cb[device_id].gem_cb[intern_gem_idx].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_UP;
            gpon_cb[device_id].gem_cb[intern_gem_idx].up_queue =  gem_port_cfg->up_queue;
            gpon_cb[device_id].gem_cb[intern_gem_idx].up_traffic_descriptor = gem_port_cfg->up_traffic_descriptor;

        } else if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN) {
            gpon_cb[device_id].gem_cb[intern_gem_idx].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN;
            gpon_cb[device_id].gem_cb[intern_gem_idx].dn_queue =  gem_port_cfg->dn_queue;
            gpon_cb[device_id].gem_cb[intern_gem_idx].dn_traffic_descriptor = gem_port_cfg->dn_traffic_descriptor;
        } else if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI) {
            gpon_cb[device_id].gem_cb[intern_gem_idx].direction = CA_GPON_PORT_GEM_PORT_DIRECTION_BI;
            gpon_cb[device_id].gem_cb[intern_gem_idx].up_queue =  gem_port_cfg->up_queue;
            gpon_cb[device_id].gem_cb[intern_gem_idx].dn_queue =  gem_port_cfg->dn_queue;
            gpon_cb[device_id].gem_cb[intern_gem_idx].up_traffic_descriptor = gem_port_cfg->up_traffic_descriptor;
            gpon_cb[device_id].gem_cb[intern_gem_idx].dn_traffic_descriptor = gem_port_cfg->dn_traffic_descriptor;
        }
        //update gem tcont mapping cb
        gpon_cb[device_id].gem_tcont_map[intern_gem_idx].gem_port_id = gem_port_cfg->gem_port_id;
        gpon_cb[device_id].gem_tcont_map[intern_gem_idx].tcont_idx = gem_port_cfg->tcont_id;
    }else{
        ca_printf("func %s line %d :Invalid gem_index %d\r\n",__func__,__LINE__,intern_gem_idx);
        return CA_E_PARAM;
    }
    return CA_E_OK;
}

#if 0
static ca_gpon_tcont_cb_t *__gpon_intl_get_tcontid_by_tcontidx (
    ca_gpon_port_cb_t         *p_gpon_cb,
    ca_uint16_t                  p_tcont_index,
    ca_uint16_t                  *tcont_id)
{
    ca_uint16_t                  indx1;

    for (indx1 = 0; indx1 <= CA_GPON_MAX_TCONTS; indx1++) {
        if ((TRUE == gpon_cb[device_id].tcont_cb[indx1].valid) &&
            (indx1 == p_tcont_index)) {
            if (tcont_id) {
                *tcont_id = gpon_cb[device_id].tcont_cb[indx1].tcont_id;
            }

            return (&gpon_cb[device_id].tcont_cb[indx1]);
        }
    }

    return (NULL);
}
#endif
ca_status_t __gpon_get_tcont_idx (ca_device_id_t    device_id,
                                  ca_uint16_t                  port_id,
                                  ca_uint16_t                  tcont_id,
                                  ca_uint16_t                  *p_tcont_index)
/* INPUTS     : o Device Id                                                   */
/*              o PON port number                                             */
/*              o Tcont Id                                                    */
/*              o Pointer to a tcont index                                    */
/* OUTPUTS    : ----                                                          */
/* RETURNS    : CS_SUCCESS or error code                                           */
/* DESCRIPTION:                                                               */
/* This API is used to get tcont index for tcont id                           */
/* rtn_hdr_end                                                                */
/******************************************************************************/
{
    ca_gpon_tcont_cb_t        *p_tcont_cb = NULL;

    p_tcont_cb = __gpon_intl_find_tcont_cb(device_id,tcont_id, p_tcont_index);
    if (p_tcont_cb == NULL) {
        return CA_E_ERROR;
    }

    return CA_E_OK;
}

ca_status_t __gpon_tm_get_lowest_pri_gem_port (
    ca_device_id_t             device_id,
    ca_uint16_t               pon_id,
    ca_uint8_t                *p_gem_subpid,
    ca_uint8_t                *p_cos)
{
    ca_status_t          status = CA_E_OK;

    /*SDL_GPON_TM_MUTEX_TAKE(device_id);*/


    *p_gem_subpid  = 0;
    *p_cos         = 7;


    /*SDL_GPON_TM_MUTEX_GIVE(device_id);*/

    return status;
} /* sdl_gpon_tm_get_lowest_pri_gem_port */

ca_status_t __gpon_tm_delete_gem_port (
    ca_device_id_t    device_id,
    ca_uint16_t      pon_id,
    ca_uint16_t      gem_port_id,
    ca_uint16_t      tcont_id)
{
    ca_status_t        status = CA_E_OK;
    ca_uint8_t           gem_subpid;
    ca_uint8_t           pri;

    /*SDL_GPON_TM_MUTEX_TAKE(device_id);*/


    /*SDL_GPON_TM_MUTEX_GIVE(device_id);*/

    /* Find the lowest priority gem port for unknown flow */
    status = __gpon_tm_get_lowest_pri_gem_port(device_id, pon_id,
             &gem_subpid, &pri);
    if (status != CA_E_OK) {
        gem_subpid = CA_GPON_MAX_GEM_PORTS;
        pri        = 0;
    }

    return CA_E_OK;
}

ca_status_t __gpon_intl_delete_gem_port (
    ca_device_id_t device_id, 
    ca_uint16_t gem_index,
    ca_gpon_port_gem_cb_t *gem_cb)
{
    ca_status_t                  ret = CA_E_OK;
    ca_gpon_port_gem_port_direction_t temp_direction;

    aal_xgpon_usbg_gem_config_msk_t us_mask;
    aal_xgpon_dsfp_xgem_cfg_msk_t ds_mask;
    aal_xgpon_usbg_gem_config_t us_xgem_cfg;
    aal_xgpon_dsfp_xgem_cfg_t   ds_xgem_cfg;

    us_mask.u32 = 0xffffffff;
    ds_mask.u32 = 0xffffffff;

    memset(&us_xgem_cfg, 0, sizeof(aal_xgpon_usbg_gem_config_t));
    memset(&ds_xgem_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_cfg_t));

    temp_direction = gem_cb->direction;
    if (TRUE == gpon_cb[device_id].state) {
        /* update the gem_tcont_map db */
        gpon_cb[device_id].gem_tcont_map[gem_index].valid = FALSE;
        gpon_cb[device_id].gem_tcont_map[gem_index].gem_port_id = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_tcont_map[gem_index].tcont_idx = CA_GPON_INVALID_VALUE;

        /*update gem cb*/
        gpon_cb[device_id].gem_cb[gem_index].valid = FALSE;
		gpon_cb[device_id].gem_cb[gem_index].gem_index = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_cb[gem_index].gem_port_id = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_cb[gem_index].aes_state = FALSE;
        gpon_cb[device_id].gem_cb[gem_index].aes_key_ring = CA_GPON_GEM_NO_ENCRYPTION;

        if (temp_direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP) {
            gpon_cb[device_id].gem_cb[gem_index].up_queue = CA_GPON_INVALID_VALUE;
			gpon_cb[device_id].gem_cb[gem_index].up_traffic_descriptor = (ca_policer_t){0,0,0,0,0,0};
        } else if (temp_direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN) {
            gpon_cb[device_id].gem_cb[gem_index].dn_queue = CA_GPON_INVALID_VALUE;
			gpon_cb[device_id].gem_cb[gem_index].dn_traffic_descriptor = (ca_policer_t){0,0,0,0,0,0};
        } else if (temp_direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI) {
            gpon_cb[device_id].gem_cb[gem_index].up_queue = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].gem_cb[gem_index].dn_queue = CA_GPON_INVALID_VALUE;
			gpon_cb[device_id].gem_cb[gem_index].up_traffic_descriptor = (ca_policer_t){0,0,0,0,0,0};
			gpon_cb[device_id].gem_cb[gem_index].dn_traffic_descriptor = (ca_policer_t){0,0,0,0,0,0};
        }

        if (temp_direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP){
            us_xgem_cfg.xgem_id = gem_cb->gem_port_id;
            us_xgem_cfg.port_en = FALSE;
			us_xgem_cfg.aes_mode = FALSE;
            ret = aal_xgpon_usbg_xgem_cfg_set(device_id, gem_index, us_mask, &us_xgem_cfg);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
        } else if (temp_direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN ) {
            ds_xgem_cfg.gem_id = gem_cb->gem_port_id;
            ds_xgem_cfg.gem_vld = FALSE;
            ret = aal_xgpon_dsfp_xgem_set(device_id, gem_index, ds_mask, &ds_xgem_cfg);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
        }else if (temp_direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI){
            us_xgem_cfg.xgem_id = gem_cb->gem_port_id;
            us_xgem_cfg.port_en = FALSE;
			us_xgem_cfg.aes_mode = FALSE;
            ret = aal_xgpon_usbg_xgem_cfg_set(device_id, gem_index, us_mask, &us_xgem_cfg);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

            ds_xgem_cfg.gem_id = gem_cb->gem_port_id;
            ds_xgem_cfg.gem_vld = FALSE;
            ret = aal_xgpon_dsfp_xgem_set(device_id, gem_index, ds_mask, &ds_xgem_cfg);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
        }

    } else {
        ret = CA_E_ERROR;
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    }

FUNC_EXIT:
    return ret;

}

#if 0
ca_status_t __gpon_onu_tcont_intl_disable_helper (ca_device_id_t device_id, ca_uint16_t tcont_id)
{
    ca_status_t                ret = CA_E_OK;
    ca_gpon_tcont_cb_t       *p_tcont;
    ca_uint16_t                  tcont_index;
    ca_uint16_t                  indx1;
    aal_xgpon_bwmp_alloc_id_cfg_msk_t mask;
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;

    memset(&alloc_id_cfg, 0 , sizeof(aal_xgpon_bwmp_alloc_id_cfg_t));

    if (TRUE == gpon_cb[device_id].state) {
        /* find the tcont_id in the db and get the tcont index, alloc id
        * and gem ports info
        */
        p_tcont = __gpon_intl_find_tcont_cb (device_id, tcont_id, &tcont_index);
        if (NULL == p_tcont) {
            return CA_E_OK;
        } else {
            ca_printf("Allocated tcont idx(%d) for tcontid(%d)\r\n", tcont_index, tcont_id);
        }

        /* delete GEM ports mapped to this tcont */
        for (indx1 = 0; indx1 < CA_GPON_MAX_GEM_PORTS; indx1++) {
            if ((gpon_cb[device_id].gem_tcont_map[indx1].valid == TRUE) && (gpon_cb[device_id].gem_tcont_map[indx1].tcont_idx == tcont_index)) {
                ret = __gpon_intl_delete_gem_port (device_id,indx1,&(gpon_cb[device_id].gem_cb[indx1]));
                CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
            }
        }
        mask.u32 = 0;
        mask.s.alloc_id = 1 ;
        mask.s.omci_en = 1 ;
        mask.s.ploam_en = 1 ;
        alloc_id_cfg.alloc_id = 0xFFFF;
        alloc_id_cfg.omci_en = 0;
        alloc_id_cfg.ploam_en = 0;
        /* delete the tcont from tm and aal */
        ret = aal_xgpon_bwmp_tcont_set (device_id,
            AAL_XGPON_WAVE_CHANNEL_DEFAULT,
            tcont_index, mask,&alloc_id_cfg);

        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

        /* remove the tcont from db */
        ret = __gpon_intl_delete_tcont_cb (device_id, p_tcont->pon_port_num,
                                               tcont_index);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

    } else {
        ret = CA_E_ERROR;
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    }

FUNC_EXIT:
    return ret;

} /* cs_gpon_onu_tcont_disable_helper */
#endif

ca_gpon_tcont_cb_t *__gpon_get_tcont_cb_by_alloc_id (
    ca_device_id_t    device_id,
    ca_uint32_t      alloc_id)
{
    int                   ix, cnt;
    ca_gpon_tcont_cb_t  *p_tcont = gpon_cb[device_id].tcont_cb;

    cnt = sizeof(gpon_cb[device_id].tcont_cb) / sizeof(gpon_cb[device_id].tcont_cb[0]);
    //cnt = p_cb->startup_cfg.num_sp_tcont + p_cb->startup_cfg.num_wrr_tcont;

    for (ix = 0; ix < cnt; ix++, p_tcont++) {
        if (p_tcont->alloc_id == alloc_id)
            return p_tcont;
    }

    return NULL;
}

ca_gpon_tcont_cb_t *__gpon_get_tcont_cb_by_tcont_id (
    ca_device_id_t    device_id,
    ca_uint16_t      tcont_id)
{
    int                   ix, cnt;
    ca_gpon_tcont_cb_t  *p_tcont = gpon_cb[device_id].tcont_cb;

    cnt = sizeof(gpon_cb[device_id].tcont_cb) / sizeof(gpon_cb[device_id].tcont_cb[0]);
    //cnt = p_cb->startup_cfg.num_sp_tcont + p_cb->startup_cfg.num_wrr_tcont;

    for (ix = 0; ix < cnt; ix++, p_tcont++) {
        if (p_tcont->tcont_id == tcont_id)
            return p_tcont;
    }

    return NULL;
}


ca_gpon_port_scheduler_t *__gpon_get_scheduler_cb_by_scheduler_id (
    ca_device_id_t    device_id,
    ca_uint16_t      scheduler_id)
{
    int                   ix, cnt;
    ca_gpon_port_scheduler_t  *scheduler_cb = gpon_cb[device_id].scheduler_cb;

    cnt = sizeof(gpon_cb[device_id].scheduler_cb) / sizeof(gpon_cb[device_id].scheduler_cb[0]);
    //cnt = p_cb->startup_cfg.num_sp_tcont + p_cb->startup_cfg.num_wrr_tcont;

    for (ix = 0; ix < cnt; ix++, scheduler_cb++) {
        if (scheduler_cb->scheduler_id == scheduler_id)
            return scheduler_cb;
    }

    return NULL;
}


ca_status_t __gpon_get_gem_port_id_by_gem_idx(ca_device_id_t device_id,
        ca_uint16_t pon_port,
        ca_uint16_t gem_port_idx,
        ca_uint32_t *gem_port_id)
/* INPUTS     : o Port num (SNI0 or SNI1)                                     */
/*              o gem port                                                    */
/*              o pointer to port_id                                          */
/* OUTPUTS    : o (3) port_id                                                 */
/* RETURNS    : CS_STATUS or Error Code                                       */
/* DESCRIPTION:                                                               */
/* This API is to retrieve port_id of gem port id in a pon port               */
/******************************************************************************/
{
    int loopcount;

    if (pon_port > 0) {

        return CA_E_PARAM;
    }

    for (loopcount = 0; loopcount <CA_GPON_MAX_GEM_PORTS; loopcount++) {
        if (loopcount == gem_port_idx) {
            *gem_port_id = gpon_cb[device_id].gem_cb[loopcount].gem_port_id;
            return CA_E_OK;
        }
    }

    return CA_E_ERROR;
}

ca_status_t __gpon_set_gem_tcont_id(ca_device_id_t   device_id,
                                    ca_uint32_t      pon_port,
                                    ca_uint32_t      gem_port_id,
                                    ca_uint32_t      tcont_id)
/* INPUTS     : o Device Id                                                   */
/*              o Port id                                                     */
/*              o gem port id                                                 */
/*              o tcont index                                                 */
/* OUTPUTS    : -- --                                                         */
/* RETURNS    : CS_STATUS or Error Code                                       */
/* DESCRIPTION:                                                               */
/* This API is to set tcont index of a gem port id                            */
/******************************************************************************/
{
    int loopcount;

    if (pon_port > 0) {
        return CA_E_PARAM;
    }

    for (loopcount = 0; loopcount <CA_GPON_MAX_GEM_PORTS; loopcount++) {
        if (gpon_cb[device_id].gem_cb[loopcount].gem_port_id == gem_port_id) {
            gpon_cb[device_id].gem_cb[loopcount].tcont_id = tcont_id;
            return CA_E_OK;
        }
    }

    return CA_E_PARAM;
}

ca_status_t ca_gpon_cb_reset(CA_IN ca_device_id_t device_id)
{
    CA_DEV_ID_CHECK(device_id);

    gpon_cb[device_id].state = FALSE;
    return CA_E_OK;

}
ca_status_t ca_gpon_api_test (CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    int i = 0, j= 0,k = 0;
    ca_gpon_port_tcont_scheduler_t scheduler;
    ca_gpon_port_gem_port_t gem_port_cfg;
    ca_uint32 valid = 1;
    gem_port_cfg.gem_port_id = 2176;
    ca_printf("%s:%d::tcont_num_per_chip is :%d and queue_num_per_tcont is %d\r\n",__FUNCTION__,__LINE__,tcont_num_per_chip,queue_num_per_tcont);

    ret =  ca_gpon_cb_init (device_id);
    CA_RET_VALUE_CHECK(ret);
    for(i=0;i<queue_num_per_tcont;i++){
        scheduler.weight_queue[i] = 0;
    }
    for(i=1;i<tcont_num_per_chip;i++){
        scheduler.mode = CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP;
        scheduler.sp_queue_num = queue_num_per_tcont;
        ret = ca_gpon_port_tcont_set(device_id,0,(alloc_id_tmp++),i,valid,&scheduler);
        CA_RET_VALUE_CHECK(ret);

        gem_port_cfg.tcont_id = i;
        for(j =0;j<queue_num_per_tcont; j++){
            ret = ca_gpon_port_upstream_scheduler_set(device_id,0,0xFFFF,i,CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP,j);
            CA_RET_VALUE_CHECK(ret);

            ret =  ca_gpon_port_upstream_queue_set(device_id,0,(queue_id_tmp++),valid,i,0xFFFF,0,j);
            CA_RET_VALUE_CHECK(ret);

            gem_port_cfg.aes_state =0;
            gem_port_cfg.aes_key_ring =0;
            //gem_port_cfg.valid = TRUE;
            gem_port_cfg.gem_port_id ++;
            gem_port_cfg.direction = CA_GPON_PORT_GEM_PORT_DIRECTION_UP;
            gem_port_cfg.up_queue = k;
            ret =  ca_gpon_port_gem_port_add(device_id,0,&gem_port_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
    }
    return ret;
}
ca_status_t ca_gpon_api_test1 (CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    int i = 0;
    ca_uint32_t onu_id = 0;
    ca_uint32_t tcont_id =0 ;
    ca_gpon_port_tcont_scheduler_t scheduler;

//    ca_uint32_t alloc_id;
    ca_uint32_t scheduler_id;
    ca_uint32_t weight;
    ca_uint8_t pri;
    ca_uint32_t valid;
    ca_uint32_t tc_valid;

    ret = ca_gpon_port_onu_id_get( device_id,0,&onu_id);
    CA_ERR_RETURN(device_id,ret);
    ca_printf("%s:%d::onu_id is 0x%x!!\r\n",__FUNCTION__,__LINE__,onu_id);

    for(i = 0;i <tcont_num_per_chip; i++){
        ret = ca_gpon_port_tcont_get(device_id,0,i,&tcont_id,&tc_valid,&scheduler);
        CA_ERR_RETURN(device_id,ret);
    }

    for(i = 0;i <CA_GPON_MAX_QUEUE_NUM; i++){
        ret = ca_gpon_port_upstream_queue_get(device_id,0,i,&valid, &tcont_id,&scheduler_id,&weight,&pri);
        CA_ERR_RETURN(device_id,ret);
    }

    return ret;
}


ca_status_t ca_gpon_cb_dump (CA_IN ca_device_id_t device_id){
    ca_status_t ret = CA_E_OK;
    int i = 0, j =0, k =0;

    ca_uint32_t pon_mode = 2;

    pon_mode = aal_pon_mac_mode_get(device_id);
    if (!CA_PON_MODE_IS_GPON_FAMILY(pon_mode))
        return ret;
    
    ca_printf("%s:%d::tcont_num_per_chip is :%d and queue_num_per_tcont is %d\r\n",__FUNCTION__,__LINE__,tcont_num_per_chip,queue_num_per_tcont);

    if (device_id != 0)
        return CA_E_PARAM;
    
    /*dump tcont_cb*/
    for(i = 0;i<tcont_num_per_chip;i++){
        ca_printf("\r\n=================================================================================\r\n");
        if (!gpon_cb[device_id].tcont_cb[i].valid)
            continue;

        ca_printf("gpon_cb[device_id].tcont_cb[%d].valid is :%d\r\n",i,gpon_cb[device_id].tcont_cb[i].valid);
        ca_printf("gpon_cb[device_id].tcont_cb[%d].alloc_id is :%d\r\n",i,gpon_cb[device_id].tcont_cb[i].alloc_id);
        ca_printf("gpon_cb[device_id].tcont_cb[%d].tcont_id is :%d\r\n",i,gpon_cb[device_id].tcont_cb[i].tcont_id);
        ca_printf("gpon_cb[device_id].tcont_cb[%d].num_sp_queues is :%d\r\n",i,gpon_cb[device_id].tcont_cb[i].num_sp_queues);
        ca_printf("gpon_cb[device_id].tcont_cb[%d].sched_mode is :%d\r\n",i,gpon_cb[device_id].tcont_cb[i].sched_mode);

        for(j= 0;j<queue_num_per_tcont; j++){
            ca_printf("gpon_cb[device_id].tcont_cb[%d].queue_cb[%d].queue_id is :%d\r\n",i,j,gpon_cb[device_id].tcont_cb[i].queue_cb[j].queue_id);
        }

        for(k =0;k<CA_GPON_MAX_GEM_PORTS;k++){
            if(gpon_cb[device_id].gem_tcont_map[k].tcont_idx == i){
                ca_printf("gpon_cb[device_id].gem_tcont_map[%d].gem_port_id is :%d and tcont id is :%d\r\n",k,gpon_cb[device_id].gem_tcont_map[k].gem_port_id,gpon_cb[device_id].gem_tcont_map[k].tcont_idx);
            }
        }

        ca_printf("\r\n=================================================================================\r\n");
    }

    /*dump gem_cb*/
    for(i = 0;i<CA_GPON_MAX_GEM_PORTS;i++){
        if (!gpon_cb[device_id].gem_cb[i].valid)
            continue;
        ca_printf("gpon_cb[device_id].gem_cb[%d].valid is :%d\r\n",i,gpon_cb[device_id].gem_cb[i].valid);
        ca_printf("gpon_cb[device_id].gem_cb[%d].tcont_id is :%d\r\n",i,gpon_cb[device_id].gem_cb[i].tcont_id);
        ca_printf("gpon_cb[device_id].gem_cb[%d].gem_port_id is :%d\r\n",i,gpon_cb[device_id].gem_cb[i].gem_port_id);
        ca_printf("gpon_cb[device_id].gem_cb[%d].up_queue is :%d\r\n",i,gpon_cb[device_id].gem_cb[i].up_queue);
        ca_printf("gpon_cb[device_id].gem_cb[%d].direction is :%d\r\n",i,gpon_cb[device_id].gem_cb[i].direction);
        ca_printf("gpon_cb[device_id].gem_cb[%d].aes_key_ring is :%d\r\n",i,gpon_cb[device_id].gem_cb[i].aes_key_ring);
        ca_printf("gpon_cb[device_id].gem_cb[%d].aes_state is :%d\r\n",i,gpon_cb[device_id].gem_cb[i].aes_state);

    }
    /*dump queue_cb*/

    return ret;

}

ca_status_t ca_gpon_cb_init (CA_IN ca_device_id_t device_id)
{
    ca_status_t ret = CA_E_OK;
    ca_status_t indx1;
    ca_status_t indx2;

#ifdef _DEBUG_
    ca_printf("%s:%d::tcont_num_per_chip is :%d and queue_num_per_tcont is %d\r\n",__func__,__LINE__,tcont_num_per_chip,queue_num_per_tcont);
#endif
    /* Init the tcont cb */
    for (indx1 = 0; indx1 <tcont_num_per_chip; indx1++) {
        gpon_cb[device_id].tcont_cb[indx1].valid = FALSE;
        gpon_cb[device_id].tcont_cb[indx1].pon_port_num = 0;
        gpon_cb[device_id].tcont_cb[indx1].tcont_id = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].tcont_cb[indx1].alloc_id = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].tcont_cb[indx1].tcont_subpid = 0xFF;
        gpon_cb[device_id].tcont_cb[indx1].sched_mode = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].tcont_cb[indx1].num_sp_queues = 0xFF;

        for(indx2 = 0; indx2 < queue_num_per_tcont; indx2++ ){
            gpon_cb[device_id].tcont_cb[indx1].queue_cb[indx2].vld = FALSE;
            gpon_cb[device_id].tcont_cb[indx1].queue_cb[indx2].tcont_id = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].tcont_cb[indx1].queue_cb[indx2].scheduler_id = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].tcont_cb[indx1].queue_cb[indx2].queue_id = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].tcont_cb[indx1].queue_cb[indx2].queue_prio = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].tcont_cb[indx1].queue_cb[indx2].queue_weight = CA_GPON_INVALID_VALUE;
        }
    }

     /* Init the scheduler cb */
    for (indx1 = 0; indx1 <2; indx1++) {
        gpon_cb[device_id].scheduler_cb[indx1].vld= FALSE;
        gpon_cb[device_id].scheduler_cb[indx1].mode = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].scheduler_cb[indx1].tcont_id = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].scheduler_cb[indx1].point_to_scheduler_id = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].scheduler_cb[indx1].scheduler_id= CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].scheduler_cb[indx1].scheduler_prio = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].scheduler_cb[indx1].scheduler_weight = CA_GPON_INVALID_VALUE;

        for(indx2 = 0; indx2 < queue_num_per_tcont; indx2++ ){
            gpon_cb[device_id].scheduler_cb[indx1].queue_cb[indx2].vld = FALSE;
            gpon_cb[device_id].scheduler_cb[indx1].queue_cb[indx2].queue_id = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].scheduler_cb[indx1].queue_cb[indx2].queue_prio = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].scheduler_cb[indx1].queue_cb[indx2].queue_weight = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].scheduler_cb[indx1].queue_cb[indx2].tcont_id = CA_GPON_INVALID_VALUE;
            gpon_cb[device_id].scheduler_cb[indx1].queue_cb[indx2].scheduler_id = CA_GPON_INVALID_VALUE;
        }
    }

    /* Init the gem cb */
    for (indx1 = 0; indx1 <CA_GPON_MAX_GEM_PORTS; indx1++) {
        gpon_cb[device_id].gem_cb[indx1].valid = FALSE;
		gpon_cb[device_id].gem_cb[indx1].gem_index = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_cb[indx1].gem_port_id= CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_cb[indx1].tcont_id = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_cb[indx1].direction = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_cb[indx1].up_queue = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_cb[indx1].dn_queue= CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_cb[indx1].aes_state = FALSE;
        gpon_cb[device_id].gem_cb[indx1].aes_key_ring = CA_GPON_GEM_NO_ENCRYPTION;
    }

    /* Initialize the GEM and TCONT mapping */
    for (indx1 = 0; indx1 <CA_GPON_MAX_GEM_PORTS; indx1++) {
        gpon_cb[device_id].gem_tcont_map[indx1].valid = FALSE;
        gpon_cb[device_id].gem_tcont_map[indx1].gem_port_id = CA_GPON_INVALID_VALUE;
        gpon_cb[device_id].gem_tcont_map[indx1].tcont_idx = CA_GPON_INVALID_VALUE;
    }

    gpon_cb[device_id].state = TRUE;

    return ret;
}


ca_status_t ca_gpon_onu_init(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_uint32_t *vendor_id,
    CA_IN ca_uint8_t *version,
    CA_IN ca_uint8_t *serial_number,
    CA_IN ca_uint8_t *traffic_mgmt_option,
    CA_IN ca_boolean_t *battery_backup,
    CA_IN ca_uint8_t *admin_state,
    CA_IN ca_uint8_t *logical_onu_id,
    CA_IN ca_uint8_t *password
)
{
    return CA_E_OK;
}


ca_status_t ca_gpon_onu_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_boolean_t battery_backup,
    CA_IN ca_uint8_t admin_state
)
{
    ca_status_t ret = CA_E_OK;
    aal_xgpon_xgpn_glb_ctrl_config_t glb_ctrl_cfg;
    aal_xgpon_xgpn_glb_ctrl_config_msk_t mask;
    aal_gpon_glb_ctrl_config_msk_t gpon_ctrl_mask;
    aal_gpon_glb_ctrl_config_t  gpon_ctrl;

    ca_uint32_t pon_mode = 0;

    battery_backup = battery_backup; /* NOT support, only for compile pass */

    mask.u32 = 0;
    mask.s.mac_en = 1;
	mask.s.us_line_rate = 1;

    memset(&glb_ctrl_cfg, 0, sizeof(aal_xgpon_xgpn_glb_ctrl_config_t));
    memset(&gpon_ctrl, 0, sizeof(gpon_ctrl));

    pon_mode = aal_pon_mac_mode_get(device_id);

    switch (pon_mode) {/* Case different PON mode to set MAC mode and upstream line rate */

	    case ONU_PON_MAC_GPON  :
	        glb_ctrl_cfg.mac_en = 0;  /* 2.5G Down, 1G Up, GPON */
	        glb_ctrl_cfg.us_line_rate = AAL_XGPON_US_LINE_RATE_2G;
	        gpon_ctrl.us10g_mode = 0;
	        gpon_ctrl.nxgs_mode = 0;

	        break;
	    case ONU_PON_MAC_XGPON1:
	        glb_ctrl_cfg.mac_en = 1;   /* 10G Down, 2.5G Up, XG-PON1 */
	        glb_ctrl_cfg.us_line_rate = AAL_XGPON_US_LINE_RATE_2G;
	        gpon_ctrl.us10g_mode = 0;
	        gpon_ctrl.nxgs_mode = 1;

	        break;
	    case ONU_PON_MAC_XGSPON:
	        glb_ctrl_cfg.mac_en = 2;   /* 10G Down, 10G Up, XGS-PON */
	        glb_ctrl_cfg.us_line_rate = AAL_XGPON_US_LINE_RATE_XG;
	        gpon_ctrl.us10g_mode = 1;
	        gpon_ctrl.nxgs_mode = 1;

	        break;
	    case ONU_PON_MAC_NGPON2:
	        glb_ctrl_cfg.mac_en = 3;   /* 10G Down, 10G Up, NG-PON2 with multiple wavelength support*/
	        glb_ctrl_cfg.us_line_rate = AAL_XGPON_US_LINE_RATE_BOTH_2G_XG;
	        gpon_ctrl.us10g_mode = 1;
	        gpon_ctrl.nxgs_mode = 1;
	        break;
	    default:
			break;
    }
    if (admin_state == 1) { /* Disable PON MAC */
        glb_ctrl_cfg.mac_en = 0;
    }

    gpon_ctrl_mask.u32 = 0;
    gpon_ctrl_mask.s.us10g_mode = 1;
    gpon_ctrl_mask.s.nxgs_mode  = 1;
    /* PON mode */
    ret = aal_gpon_glb_ctrl_set(device_id, gpon_ctrl_mask, &gpon_ctrl);
    CA_RET_VALUE_CHECK(ret);

    /* Enable or disable */
    ret = aal_xgpon_xgpn_glb_ctrl_set(device_id, mask, &glb_ctrl_cfg);
    CA_RET_VALUE_CHECK(ret);
#if 0
    if (admin_state == 0) {
        aal_xgpn_activation_fsm_init(device_id, pon_mode);
    }
    aal_psds_burst_enable(device_id, admin_state); /* SERDES enable or disable */
#endif

#ifdef _DEBUG_
    ca_printf("set PON mac to be %d, us line rate to be %d.\r\n", glb_ctrl_cfg.mac_en, glb_ctrl_cfg.us_line_rate);
#endif

#ifdef CONFIG_LUNA_G3_SERIES
    g_admin_state = admin_state;
#endif


    return ret;
}

ca_status_t ca_gpon_onu_get(
    CA_IN ca_device_id_t device_id,
    CA_OUT ca_uint32_t *vendor_id,
    CA_OUT ca_uint8_t *version,
    CA_OUT ca_uint8_t *serial_number,
    CA_OUT ca_uint8_t *traffic_mgmt_option,
    CA_OUT ca_boolean_t *battery_backup,
    CA_OUT ca_uint8_t *admin_state,
    CA_OUT ca_uint8_t *logical_onu_id,
    CA_OUT ca_uint8_t *password
)
{
    ca_status_t ret = CA_E_OK;
    /* ca_uint8_t common_char = '-';
    ca_uint32_t revid = 0; */
    ca_uint32_t vssn = 0;
	ca_uint32_t vendorid_tmp = 0;
	ca_uint32_t vssn_tmp = 0;

	aal_xgpon_xgpn_glb_ctrl_config_t glb_ctrl_cfg;
	memset(&glb_ctrl_cfg, 0, sizeof(aal_xgpon_xgpn_glb_ctrl_config_t));

    *vendor_id = g_xgpn_basic_info.vendor_id;
    vssn       = g_xgpn_basic_info.vssn;
	vendorid_tmp = htonl(g_xgpn_basic_info.vendor_id);
	vssn_tmp	 = htonl(g_xgpn_basic_info.vssn);
    memcpy(serial_number, &vendorid_tmp, sizeof(ca_uint32_t));
    memcpy((serial_number+4),&vssn_tmp,  sizeof(ca_uint32_t));
	
	/*
    ret = aal_xgpon_xgpn_glb_ver_id_get(device_id,&revid);
    CA_RET_VALUE_CHECK(ret);
    revid = htonl(revid);
    memcpy(version,&revid, sizeof(ca_uint32_t));
    memcpy(version+4,&common_char, sizeof(ca_uint8_t));
    memcpy(version+5,&vendor_id, sizeof(ca_uint32_t));
    memcpy(version+9,&common_char, sizeof(ca_uint8_t));
    memcpy(version+10,&vssn,sizeof(ca_uint32_t));
    */
    *traffic_mgmt_option = 0;/*three options:0,1,2*/
    *battery_backup = CA_E_NOT_SUPPORT;
	ret = aal_xgpon_xgpn_glb_ctrl_get(device_id, &glb_ctrl_cfg);
	CA_RET_VALUE_CHECK(ret);
	if(glb_ctrl_cfg.mac_en == 0){
    	*admin_state = 1;
	}else{
		*admin_state = 0;
	}
    memcpy(logical_onu_id,g_xgpn_basic_info.logical_onu_id, GPON_ONU_ID_STR_LENGTH);
    memcpy(password,g_xgpn_basic_info.password, GPON_ONU_PASSWORD_LENGTH);
    memcpy(version,g_xgpn_basic_info.version, GPON_ONU_VERSION_ID_LENGTH);

    return ret;
}

static ca_status_t __gpon_port_sf_threshold_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t sf_threshold
)
{
    ca_status_t ret = CA_E_OK;
  
    aal_gpon_sf_degrd_alrm_thrsd_cfg_msk_t mask;
    aal_gpon_sf_degrd_alrm_thrsd_cfg_t     cfg;
    aal_gpon_signal_thres_val_t sf_th, sf_clear_th, sd_th, sd_clear_th;
    if(sf_threshold<GPON_OPTICAL_SIGNAL_THRESHOLD_MIN 
        || sf_threshold>GPON_OPTICAL_SIGNAL_THRESHOLD_MAX)
    {
        return CA_E_PARAM;
    }
    memset(&cfg, 0 , sizeof(cfg));
        
    ret = aal_gpon_sf_degrd_alrm_thrsd_get(device_id, &cfg);
    CA_RET_VALUE_CHECK(ret);
    
    cfg.fail_threshold = sf_threshold - 3;
    cfg.fail_remove_threshold = sf_threshold - 3;
    
    mask.u32 = ~0U;
    ret = aal_gpon_sf_degrd_alrm_thrsd_set(device_id, mask,&cfg);
    CA_RET_VALUE_CHECK(ret);
    return CA_E_OK;
}

ca_status_t ca_gpon_port_sf_threshold_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t sf_threshold
)
{
    ca_uint32_t port = PORT_ID(port_id); 
    if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
    {
        return __gpon_port_sf_threshold_set(device_id, port, sf_threshold);
    }
    sf_thrsld = sf_threshold;
    return CA_E_OK;
}


ca_status_t ca_gpon_port_sf_threshold_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *sf_threshold
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_gpon_sf_degrd_alrm_thrsd_cfg_t cfg;

    if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
    {
        memset(&cfg, 0, sizeof(cfg));
        ret = aal_gpon_sf_degrd_alrm_thrsd_get(device_id, &cfg);
        CA_RET_VALUE_CHECK(ret);
        *sf_threshold = cfg.fail_threshold + 3;
        return ret;
    }
    *sf_threshold = sf_thrsld;
    return CA_E_OK;
}

static ca_status_t __gpon_port_sd_threshold_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t sd_threshold
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_gpon_sf_degrd_alrm_thrsd_cfg_msk_t mask;
    aal_gpon_sf_degrd_alrm_thrsd_cfg_t     cfg;
    aal_gpon_signal_thres_val_t sf_th, sf_clear_th, sd_th, sd_clear_th;
    if(sd_threshold<GPON_OPTICAL_SIGNAL_THRESHOLD_MIN 
        || sd_threshold>GPON_OPTICAL_SIGNAL_THRESHOLD_MAX)
    {
        return CA_E_PARAM;
    }
    memset(&cfg, 0 , sizeof(cfg));
        
    ret = aal_gpon_sf_degrd_alrm_thrsd_get(device_id, &cfg);
    CA_RET_VALUE_CHECK(ret);
    
    cfg.degrade_threshold = sd_threshold - 3;
    cfg.degrade_remove_threshold = sd_threshold - 3;
    
    mask.u32 = ~0U;
    ret = aal_gpon_sf_degrd_alrm_thrsd_set(device_id, mask,&cfg);
    CA_RET_VALUE_CHECK(ret);
    return CA_E_OK;
}

ca_status_t ca_gpon_port_sd_threshold_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t sd_threshold
)
{
    ca_uint32_t port = PORT_ID(port_id); 
    if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
    {
        return __gpon_port_sd_threshold_set(device_id, port, sd_threshold);
    }
    sd_thrsld = sd_threshold;
    return CA_E_OK;
}

ca_status_t ca_gpon_port_sd_threshold_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *sd_threshold
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_gpon_sf_degrd_alrm_thrsd_cfg_t cfg;

    if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
    {
        memset(&cfg, 0, sizeof(cfg));
        ret = aal_gpon_sf_degrd_alrm_thrsd_get(device_id, &cfg);
        CA_RET_VALUE_CHECK(ret);
        *sd_threshold = cfg.degrade_threshold + 3;
        return ret;
    }
    *sd_threshold = sd_thrsld;
    return CA_E_OK;

}

ca_status_t ca_gpon_port_laser_mode_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_gpon_port_laser_mode_t mode
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_usbg_glb_ctrl_config_msk_t mask;
    aal_xgpon_usbg_glb_ctrl_config_t  usbg_glb_cfg;
    memset(&usbg_glb_cfg, 0, sizeof(aal_xgpon_usbg_glb_ctrl_config_t));
    mask.u32  = 0;
    mask.s.laser_on = 1;
    usbg_glb_cfg.laser_on = mode;

    ret = aal_xgpon_usbg_glb_ctrl_set(device_id, mask, &usbg_glb_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);


FUNC_EXIT:

    return ret;
}

ca_status_t ca_gpon_port_laser_mode_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_laser_mode_t *mode
)
{

    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_usbg_glb_ctrl_config_t usbg_glb_cfg;
    memset(&usbg_glb_cfg, 0, sizeof(aal_xgpon_usbg_glb_ctrl_config_t));
    ret =  aal_xgpon_usbg_glb_ctrl_get(device_id, &usbg_glb_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    *mode = usbg_glb_cfg.laser_on;

FUNC_EXIT:
    return ret;

}
ca_status_t ca_gpon_port_timer_set(
    CA_IN        ca_device_id_t                         device_id,
    CA_IN        ca_port_id_t                        port_id,
    CA_IN        ca_gpon_port_timer_type_t            timer_type,
    CA_IN        ca_uint32_t                        timer_msec
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint32_t timer_125us = GPON_TIMER_MS2CLK(timer_msec);
    aal_xgpn_plmp_key_exchg_thrshd_mask_t  key_exchg_thrshd_mask;
    aal_xgpn_plmp_key_exchg_thrshd_t       key_exchg_thrshd;

    key_exchg_thrshd_mask.wrd = 0;
    memset(&key_exchg_thrshd, 0, sizeof(key_exchg_thrshd));

    switch(timer_type){
    case CA_GPON_PORT_TIMER_TYPE_TOZ:
        ret = aal_xgpon_actp_toz_set(device_id, timer_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TO1:
        ret = aal_xgpon_actp_to1_set(device_id, timer_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TO2:
    case CA_GPON_PORT_TIMER_TYPE_TO3:
        ret = aal_xgpon_actp_to23_set(device_id, timer_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TO4:
        ret = aal_xgpon_actp_to4_set(device_id, timer_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TO5:
        ret = aal_xgpon_actp_to5_set(device_id, timer_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_CPI:
        ret = aal_xgpn_cpi_timeout_set(device_id, timer_msec);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TK4:
        key_exchg_thrshd_mask.bf.tk4_timeout = 1;
        key_exchg_thrshd.tk4_timeout = timer_125us;
        ret = aal_ploam_key_exchg_thrshd_set(device_id,
            key_exchg_thrshd_mask, &key_exchg_thrshd);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TK5:
        key_exchg_thrshd_mask.bf.tk5_timeout = 1;
        key_exchg_thrshd.tk5_timeout = timer_125us;
        ret = aal_ploam_key_exchg_thrshd_set(device_id,
            key_exchg_thrshd_mask, &key_exchg_thrshd);
        CA_RET_VALUE_CHECK(ret);
        break;
    default:
        break;
    }

    return ret;
}

ca_status_t ca_gpon_port_timer_get(
    CA_IN        ca_device_id_t                     device_id,
    CA_IN        ca_port_id_t                    port_id,
    CA_IN        ca_gpon_port_timer_type_t        timer_type,
    CA_OUT        ca_uint32_t                       *p_timer_msec
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint32_t local_125us = 0;
    aal_xgpn_plmp_key_exchg_thrshd_t  key_exchg_thrshd;

    CA_NULL_PTR_CHECK(p_timer_msec);
    memset(&key_exchg_thrshd, 0, sizeof(key_exchg_thrshd));

    switch(timer_type){
    case CA_GPON_PORT_TIMER_TYPE_TOZ:
        ret = aal_xgpon_actp_toz_get(device_id, &local_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TO1:
        ret = aal_xgpon_actp_to1_get(device_id, &local_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TO2:
    case CA_GPON_PORT_TIMER_TYPE_TO3:
        ret = aal_xgpon_actp_to23_get(device_id, &local_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TO4:
        ret = aal_xgpon_actp_to4_get(device_id, &local_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_TO5:
        ret = aal_xgpon_actp_to5_get(device_id, &local_125us);
        CA_RET_VALUE_CHECK(ret);
        break;
    case CA_GPON_PORT_TIMER_TYPE_CPI:
        return aal_xgpn_cpi_timeout_get(device_id, p_timer_msec);

    case CA_GPON_PORT_TIMER_TYPE_TK4:
        ret = aal_ploam_key_exchg_thrshd_get(device_id, &key_exchg_thrshd);
        CA_RET_VALUE_CHECK(ret);
        local_125us = key_exchg_thrshd.tk4_timeout;
        break;
    case CA_GPON_PORT_TIMER_TYPE_TK5:
        ret = aal_ploam_key_exchg_thrshd_get(device_id, &key_exchg_thrshd);
        CA_RET_VALUE_CHECK(ret);
        local_125us = key_exchg_thrshd.tk5_timeout;
        break;

        default:
            break;
    }

    *p_timer_msec = GPON_TIMER_CLK2MS(local_125us);

    return ret;
}


ca_status_t ca_gpon_port_sync_threshold_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t sync_threshold,
    CA_IN ca_uint32_t miss_sync_threshold
)
{
    ca_status_t   ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint32_t timer_125us = GPON_TIMER_MS2CLK(sync_threshold);
    aal_xgpon_dsfp_sync_ctrl_config_msk_t sync_ctrl_msk;
    aal_xgpon_dsfp_sync_ctrl_config_t sync_ctrl_cfg;

    sync_ctrl_msk.u32 = 0;
    memset(&sync_ctrl_cfg, 0 , sizeof(aal_xgpon_dsfp_sync_ctrl_config_t));

    ret = aal_xgpon_actp_sync_hunt_set(device_id, timer_125us);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

    sync_ctrl_msk.s.resync_loss_frame_num = 1;
    sync_ctrl_cfg.resync_loss_frame_num = miss_sync_threshold;
    ret = aal_xgpon_dsfp_sync_ctrl_set(device_id,sync_ctrl_msk,&sync_ctrl_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

FUNC_EXIT:

    return CA_E_OK;

}

ca_status_t ca_gpon_port_sync_threshold_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *sync_threshold,
    CA_OUT ca_uint32_t *miss_sync_threshold
)
{
    ca_status_t   ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint32_t local_125us = 0;
    aal_xgpon_dsfp_sync_ctrl_config_t sync_ctrl_cfg;
    CA_NULL_PTR_CHECK(sync_threshold);
    CA_NULL_PTR_CHECK(miss_sync_threshold);

    ret = aal_xgpon_actp_sync_hunt_get(device_id, &local_125us);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    *sync_threshold = GPON_TIMER_CLK2MS(local_125us);

    ret = aal_xgpon_dsfp_sync_ctrl_get(device_id,&sync_ctrl_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    *miss_sync_threshold = sync_ctrl_cfg.resync_loss_frame_num;
FUNC_EXIT:

    return CA_E_OK;
}

ca_status_t ca_gpon_port_aes_enable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t aes_enable
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_usbg_glb_ctrl_config_msk_t mask;
    aal_xgpon_usbg_glb_ctrl_config_t  usbg_glb_cfg;
    memset(&usbg_glb_cfg, 0, sizeof(aal_xgpon_usbg_glb_ctrl_config_t));
    mask.u32 = 0;
    mask.s.aes_encrypt_en = 1;
    usbg_glb_cfg.aes_encrypt_en = aes_enable;
    ret = aal_xgpon_usbg_glb_ctrl_set(device_id, mask, &usbg_glb_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
FUNC_EXIT:

    return ret;
}

ca_status_t ca_gpon_port_aes_enable_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_boolean_t *aes_enable
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_usbg_glb_ctrl_config_t usbg_glb_cfg;
    memset(&usbg_glb_cfg, 0, sizeof(aal_xgpon_usbg_glb_ctrl_config_t));
    ret =  aal_xgpon_usbg_glb_ctrl_get(device_id, &usbg_glb_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    *aes_enable = usbg_glb_cfg.aes_encrypt_en;
FUNC_EXIT:
    return ret;
}

ca_status_t ca_gpon_port_aes_key_set(
    CA_IN   ca_device_id_t  device_id,
    CA_IN   ca_port_id_t    port_id,
    CA_IN   ca_uint8_t      effective_key_length,
    CA_IN   ca_uint8_t      *aes_key
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint8_t active_key_idx = 0;
    aal_uc_enc_key_config_msk_t uc_key_cfg_msk;
    aal_uc_enc_key_config_t uc_key_cfg;

    CA_GPON_KEY_LEN_CHECK(effective_key_length);

    uc_key_cfg_msk.u32 = ~0;
    memset(&uc_key_cfg,0, sizeof(aal_uc_enc_key_config_t));

    memcpy(uc_key_cfg.uc_key, aes_key, effective_key_length);
    uc_key_cfg.p_effective_key_length = effective_key_length;
    uc_key_cfg.p_key_active = 0;

    ret = aal_xgpon_usbg_aes_uni_key_idx_get(device_id, &active_key_idx);
    CA_RET_VALUE_CHECK(ret);

    if(active_key_idx){
        if(aal_pon_mac_mode_get(device_id) != ONU_PON_MAC_GPON ){
            ret = aal_xgpon_aes_uc_enc_key_set(device_id, 0,uc_key_cfg_msk,&uc_key_cfg);
            CA_RET_VALUE_CHECK(ret);
        }else{//gpon

        }
    }else{
        if(aal_pon_mac_mode_get(device_id) != ONU_PON_MAC_GPON ){
            ret = aal_xgpon_aes_uc_enc_key_set(device_id, 1,uc_key_cfg_msk,&uc_key_cfg);
            CA_RET_VALUE_CHECK(ret);
        }else{//gpon

        }
    }

    return ret;

}

ca_status_t ca_gpon_port_aes_key_get(
    CA_IN	ca_device_id_t      device_id,
    CA_IN	ca_port_id_t        port_id,
    CA_IN	ca_uint8_t          key_idx,
    CA_OUT	ca_boolean_t        *p_key_active,
    CA_OUT	ca_uint8_t          *p_effective_key_length,
    CA_OUT	ca_uint8_t          *aes_key
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_uc_enc_key_config_t uc_key_cfg;
    CA_XGPON_KEY_IDX_CHECK(key_idx);
    memset(&uc_key_cfg,0, sizeof(aal_uc_enc_key_config_t));
    if(aal_pon_mac_mode_get(device_id) != ONU_PON_MAC_GPON ){
        ret = aal_xgpon_aes_uc_enc_key_get(device_id,key_idx,&uc_key_cfg);
        CA_RET_VALUE_CHECK(ret);
    }else{//gpon

    }
    *p_key_active = uc_key_cfg.p_key_active;
    *p_effective_key_length = uc_key_cfg.p_effective_key_length;
    memcpy(aes_key, uc_key_cfg.uc_key,CA_NGP2_UC_KEY_LEN);

    return ret;
}

ca_status_t ca_gpon_port_fec_enable_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean_t fec_encoder_enable,
    CA_IN ca_boolean_t fec_decoder_enable
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_fec_ctrl_mask_t  mask;
    aal_xgpon_fec_ctrl_t      ctrl;
    memset(&ctrl, 0, sizeof(aal_xgpon_fec_ctrl_t));
    mask.u32_data = 0;
    mask.s.disableFEC = 1;
    mask.s.fec_dec_corr_disable = 1;
    ctrl.disableFEC = !fec_encoder_enable;
    ctrl.fec_dec_corr_disable = !fec_decoder_enable;
    ret = aal_xgpon_fec_ctrl_set(device_id, mask, &ctrl);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
FUNC_EXIT:
    return ret;
}

ca_status_t ca_gpon_port_fec_enable_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_boolean_t *fec_encoder_enable,
    CA_OUT ca_boolean_t *fec_decoder_enable
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_fec_ctrl_t ctrl;
    memset(&ctrl, 0, sizeof(aal_xgpon_fec_ctrl_t));
    ret =  aal_xgpon_fec_ctrl_get(device_id, &ctrl);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    *fec_encoder_enable = !ctrl.disableFEC;
    *fec_decoder_enable = !ctrl.fec_dec_corr_disable ;
FUNC_EXIT:
    return ret;
}

ca_status_t ca_gpon_port_fec_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean read_clear,
    CA_OUT ca_gpon_port_fec_stats_t *fec_stats
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_fec_stats_t stats;
    aal_xgpon_xgpn_glb_ctrl_config_msk_t glb_ctrl_cfg_msk;
    aal_xgpon_xgpn_glb_ctrl_config_t glb_ctrl_cfg;

    memset(&stats, 0, sizeof(aal_xgpon_fec_stats_t));
    glb_ctrl_cfg_msk.u32 = 0;
    memset(&glb_ctrl_cfg, 0, sizeof(aal_xgpon_xgpn_glb_ctrl_config_t));

    glb_ctrl_cfg_msk.s.pm_mode = 1;
    if(read_clear){//read unchanged
        //TODO:read saturation interrupt counters for calculating counters
        glb_ctrl_cfg.pm_mode = 2;
    }else{//read clear
        glb_ctrl_cfg.pm_mode = 0;
    }
    ret = aal_xgpon_xgpn_glb_ctrl_set(device_id, glb_ctrl_cfg_msk, &glb_ctrl_cfg);
    CA_RET_VALUE_CHECK(ret);

    ret =  aal_xgpon_fec_stats_get(device_id, &stats);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    fec_stats->corrected_bytes = stats.corrected_bytes_rx;
    fec_stats->corrected_codewords = stats.corrected_codewords_rx;
    fec_stats->total_codewords = stats.total_codewords_rx;
    fec_stats->uncorrectable_codewords = stats.uncorrectable_codewords_rx;
FUNC_EXIT:

    return ret;
}

ca_status_t ca_gpon_port_onu_state_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_onu_states_t *onu_state
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_actp_status_t sts;
    aal_gpon_onu_states_t gpon_sts;

    memset(&sts, 0, sizeof(aal_xgpon_actp_status_t));
    if(aal_pon_mac_mode_get(device_id) != ONU_PON_MAC_GPON){
        ret =  aal_xgpon_actp_status_get(device_id, &sts);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    }else{
        ret = aal_gpon_onu_state_get(device_id, 0, &gpon_sts);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    }
    if((aal_pon_mac_mode_get(device_id) == ONU_PON_MAC_XGPON1)||(aal_pon_mac_mode_get(device_id) == ONU_PON_MAC_XGSPON)){

        switch(sts.onu_state)
        {
            case 0:
                *onu_state = CA_GPON_PORT_ONU_STATES_O1;
                break;
            case 1:
                *onu_state = CA_NGP2_ONU_ACT_STATE_O2_3;
                break;
            case 2:
                *onu_state = CA_GPON_PORT_ONU_STATES_O4;
                break;
            case 3:
                *onu_state = CA_GPON_PORT_ONU_STATES_O5;
                break;
            case 4:
                *onu_state = CA_GPON_PORT_ONU_STATES_O6;
                break;
            case 5:
                *onu_state = CA_GPON_PORT_ONU_STATES_O7;
                break;
            default:
                ca_printf("ERROR:Unknown state!!/r/n");
                break;
        }
    }
    if (aal_pon_mac_mode_get(device_id) == ONU_PON_MAC_NGPON2){
        switch(sts.onu_state)
        {
            case 0:
               *onu_state = ((0 == sts.onu_sub_state) ? CA_NGP2_ONU_ACT_STATE_O1_1 : CA_NGP2_ONU_ACT_STATE_O1_2);
               break;
            case 1:
                *onu_state = CA_NGP2_ONU_ACT_STATE_O2_3;
                break;
            case 2:
                *onu_state = CA_NGP2_ONU_ACT_STATE_O4;
                break;
            case 3:
                *onu_state = ((0 == sts.onu_sub_state) ? CA_NGP2_ONU_ACT_STATE_O5_1 : CA_NGP2_ONU_ACT_STATE_O5_2);
                break;
            case 4:
                *onu_state = CA_NGP2_ONU_ACT_STATE_O6;
                break;
            case 5:
                *onu_state = CA_NGP2_ONU_ACT_STATE_O7;
                break;
            case 6:
                *onu_state = ((0 == sts.onu_sub_state) ? CA_NGP2_ONU_ACT_STATE_O8_1 : CA_NGP2_ONU_ACT_STATE_O8_2);
                break;
            case 7:
                *onu_state = CA_NGP2_ONU_ACT_STATE_O9;
                break;

            default:
                ca_printf("ERROR:Unknown state!!/r/n");
                break;
        }
    }

        if (aal_pon_mac_mode_get(device_id) == ONU_PON_MAC_GPON){
        switch(gpon_sts)
        {
            case 0:
                *onu_state = CA_GPON_PORT_ONU_STATES_O1;
                break;
            case 1:
                *onu_state = CA_GPON_PORT_ONU_STATES_O2;
                break;
            case 2:
                *onu_state = CA_GPON_PORT_ONU_STATES_O3;
                break;
            case 3:
                *onu_state = CA_GPON_PORT_ONU_STATES_O4;
                break;
            case 4:
                *onu_state = CA_GPON_PORT_ONU_STATES_O5;
                break;
            case 5:
                *onu_state = CA_GPON_PORT_ONU_STATES_O6;
                break;
            case 6:
                *onu_state = CA_GPON_PORT_ONU_STATES_O7;
                break;
            default:
                ca_printf("ERROR:Unknown state!!/r/n");
                break;
        }
    }

FUNC_EXIT:
    return ret;

}

ca_status_t ca_gpon_port_onu_id_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *onu_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;
    memset(&alloc_id_cfg, 0, sizeof(aal_xgpon_bwmp_alloc_id_cfg_t));
    ret =  aal_xgpon_bwmp_tcont_get(device_id, AAL_XGPON_WAVE_CHANNEL_DEFAULT,  0, &alloc_id_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    *onu_id = alloc_id_cfg.alloc_id;

FUNC_EXIT:
    return ret;
}

ca_status_t ca_gpon_port_onu_eqd_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *onu_eqd
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint32_t eqd;

    ret =  aal_xgpon_bwmp_eqd_sts_get(device_id, &eqd);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    *onu_eqd = eqd;

FUNC_EXIT:
    return ret;
}

ca_status_t ca_gpon_port_capability_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_capability_t *capability
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 

    CA_NULL_PTR_CHECK(capability);

    capability->max_tcont_number = tcont_num_per_chip;
    capability->max_gem_number = CA_GPON_MAX_GEM_PORTS;
    capability->max_queue_number = CA_GPON_MAX_QUEUE_NUM;
    capability->report_mode = CA_GMAC_PIGGYBACK_REPORT_MODE_0_AND_1;
    capability->queues_per_tcont = queue_num_per_tcont;

    if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
    {
        capability->max_gem_number   = CA_GPON_MAC_GEM_PORTS_MAX;
        capability->max_queue_number = CA_GPON_MAC_QUEUE_NUM_MAX;
    }

    return ret;
}


static ca_status_t __gpon_gem_port_add(
    CA_IN  ca_device_id_t           device_id,
    CA_IN  ca_port_id_t             port_id,
    CA_OUT ca_gpon_port_gem_port_t *gem_port_cfg
)
{
    ca_status_t                  ret             = CA_E_OK;
    ca_uint16_t                  tcont_index     = 0;
    ca_gpon_tcont_cb_t          *p_tcont         = NULL;
    ca_boolean_t                 is_omci_port    = FALSE;
    ca_uint16_t                  intern_gem_idx  = 0;
    aal_gpon_gem_port_cfg_msk_t  ds_gem_cfg_mask = {0};
    aal_gpon_gem_port_cfg_t      ds_gem_cfg      = {0};
    int                          i               = 0;
    ca_boolean_t                 found           = FALSE;
    aal_gpon_onu_params_t        onu_para        = {0};

    ds_gem_cfg_mask.u32 = ~0U;
    memset(&ds_gem_cfg, 0, sizeof(ds_gem_cfg));

    if (!gem_port_cfg) {
        ca_printf("gem_port_cfg is NULL\r\n");
        return CA_E_PARAM;
    }

    if ((gem_port_cfg->direction > CA_GPON_PORT_GEM_PORT_DIRECTION_BI)
     || (gem_port_cfg->tcont_id >= tcont_num_per_chip)
     || (gem_port_cfg->up_queue >= queue_num_per_tcont)) {
        ca_printf("Error Param: Direction[%d] Tcont[%d] Queue[%d] GemId[%d]\r\n",
                   gem_port_cfg->direction, gem_port_cfg->tcont_id,
                   gem_port_cfg->up_queue, gem_port_cfg->gem_port_id);
        return CA_E_PARAM;
    }

    if (gem_port_cfg->direction != CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN) {
        if (!(p_tcont = __gpon_intl_find_tcont_cb (device_id, gem_port_cfg->tcont_id, &tcont_index))){
            ca_printf("%s:%d::error: tcont_cb of tcont %d is NULL\r\n",__FUNCTION__,__LINE__, gem_port_cfg->tcont_id);
            return CA_E_PARAM;
        }
    }

    if (TRUE == gpon_cb[device_id].state)
    {
        for(i = 8; i < CA_GPON_MAX_GEM_PORTS; i++)
        {
            if((gpon_cb[device_id].gem_cb[i].valid)
            && (gpon_cb[device_id].gem_cb[i].gem_port_id == gem_port_cfg->gem_port_id))
            {
                if(gpon_cb[device_id].gem_cb[i].direction == gem_port_cfg->direction)
                {
                    intern_gem_idx = i;
                    found = TRUE;
                    break;
                }else
                {
                    ca_printf("Direction for the same gem_port is different, please delete previous entry firstly.\r\n ");
                    return CA_E_PARAM;
                }
            }
        }

        if (found)
        { //update gem cfg and gem tcont map
            ret = __gpon_intl_update_gem(device_id,port_id,intern_gem_idx,gem_port_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
        else
        { //add new
            /* add gem cfg and gem tcont map*/
            ret = __gpon_intl_add_gem(device_id, port_id, tcont_index, is_omci_port, &intern_gem_idx, gem_port_cfg);
            CA_RET_VALUE_CHECK(ret);
        }

        if ((gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP)
         || (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI))
        {
            ret = aal_gpon_us_gem_port_set(device_id, intern_gem_idx,gem_port_cfg->gem_port_id);
            CA_RET_VALUE_CHECK(ret);
#if defined(CONFIG_ARCH_REALTEK_TAURUS)
            //77C not using ca api
#else
            ret = ca_flow_policer_set(device_id, intern_gem_idx, &(gem_port_cfg->up_traffic_descriptor));
            if(ret != CA_E_OK)
            {
                ca_printf("%s:%d: gem policer cfg fail ret %d!\r\n",__FUNCTION__,__LINE__, ret);
            }
#endif
        }
        if ((gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN)
         || (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI))
        {
            //GPON AES enabled by ploam message
            ds_gem_cfg.gem_index = intern_gem_idx;
            ds_gem_cfg.vld = 1;
            ret = aal_gpon_ds_gem_port_set(device_id, 0, gem_port_cfg->gem_port_id, ds_gem_cfg_mask, &ds_gem_cfg);
            CA_RET_VALUE_CHECK(ret);
        }

        //get omci gemport id cfg
        aal_gpon_onu_params_get(device_id, 0, &onu_para);
        (void)aal_gpon_ds_gem_port_get(device_id, 0, onu_para.omci_port, &ds_gem_cfg);

        if (intern_gem_idx > 0 && intern_gem_idx != ds_gem_cfg.gem_index)
        {
            /* once OMCI configures GEM port: PDC_CTRL.pdc_map_mem_en = 1,
                   * setup PDC_PDC_MAP_MEM:
                   * PDC_PDC_MAP_MEM_DATA policer_id = internal GEM ID, PDC_PDC_MAP_MEM_DATA0.
                   * no-drop = 0 for GEM port id != 0;
                   */
            aal_pdc_ctrl_mask_t             pdc_ctrl_mask;
            aal_pdc_ctrl_t                  pdc_ctrl;
            aal_pdc_pdc_map_mem_data_mask_t pdc_map_mask;
            aal_pdc_pdc_map_mem_data_t      pdc_map_cfg;

            memset(&pdc_ctrl, 0, sizeof(pdc_ctrl));
            pdc_ctrl_mask.wrd = 0;

            if (!found) {
                pdc_map_mask.wrd        = 0;
                memset(&pdc_map_cfg, 0, sizeof(pdc_map_cfg));

                pdc_map_mask.bf.no_drop = 1;
                pdc_map_mask.bf.pol_en  = 1;
                pdc_map_mask.bf.pol_id  = 1;
                pdc_map_mask.bf.lspid   = 1;

                pdc_map_cfg.no_drop     = 0;
                pdc_map_cfg.pol_en      = 1;
                pdc_map_cfg.pol_id      = intern_gem_idx; /* internal GEM ID */
                /******12 5th 2017, Jianguang add below codes to support gem_index to src port map*******/
                pdc_map_cfg.lspid       = aal_pdc_free_lspid_get(device_id,intern_gem_idx);
                /**end of Jianguang add**/
                ret = aal_pdc_map_mem_set(device_id, intern_gem_idx, pdc_map_mask, &pdc_map_cfg);
                CA_RET_VALUE_CHECK(ret);
            }

            ret = aal_puc_pvtbl_enable_set(device_id, gem_port_cfg->tcont_id, TRUE);
            CA_RET_VALUE_CHECK(ret);

            CA_GPON_DBG("Setup TCONT %d internal TCONT-ID %d GEM Port %d internal GEM-ID %d up_queue %d.\r\n",
                gem_port_cfg->tcont_id, tcont_index, gem_port_cfg->gem_port_id, intern_gem_idx,gem_port_cfg->up_queue);
        }

        if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN){
            gem_port_cfg->gem_index = (0xFF << 8) | intern_gem_idx;
        }else{
            if((puc_voq_map_mode == AAL_PUC_VOQ_MODE_8Q)||(puc_voq_map_mode == AAL_PUC_VOQ_MODE_4Q))
            {
                gem_port_cfg->gem_index = ((tcont_index & 0x1f) << 11 ) | ((gem_port_cfg->up_queue) << 8) | intern_gem_idx;
            }else if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP)
            {     
                gem_port_cfg->gem_index = (intern_gem_idx << 8) |intern_gem_idx;
            }else
            {
                ca_printf("Unsupported voq mode %d for add gemport \r\n", puc_voq_map_mode);
                return CA_E_UNAVAIL;
            }
        }
        ca_printf("GEM port id 0x%x  - Internal 0x%x created,returned gem_index 0x%x \r\n", gem_port_cfg->gem_port_id, intern_gem_idx,gem_port_cfg->gem_index);
    }
    else
    {
        ca_printf("UnAvalid: Direction[%d] Tcont[%d] Queue[%d] GemId[%d]\r\n",
                   gem_port_cfg->direction, gem_port_cfg->tcont_id,
                   gem_port_cfg->up_queue, gem_port_cfg->gem_port_id);
        return CA_E_UNAVAIL;
    }

    return ret;
}


ca_status_t ca_gpon_port_gem_port_add(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_gem_port_t *gem_port_cfg
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint16_t tcont_index = 0;
    ca_gpon_tcont_cb_t *p_tcont = NULL;
    ca_boolean_t is_omci_port = FALSE;
    ca_uint16_t intern_gem_idx = 0;
	
    aal_xgpon_usbg_gem_config_msk_t us_mask;
    aal_xgpon_dsfp_xgem_cfg_msk_t ds_mask;
    aal_xgpon_usbg_gem_config_t us_xgem_cfg;
    aal_xgpon_dsfp_xgem_cfg_t   ds_xgem_cfg;

#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
    ca_uint16_t queue_id = 0;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_rl_voq_tbc_cfg_msk;
    aal_puc_voq_tbc_mem_cfg_t puc_rl_voq_tbc_cfg;
    aal_puc_voq_tbc_mem_cfg_msk_t puc_voq_tbc_cfg_msk;
    aal_puc_voq_tbc_mem_cfg_t puc_voq_tbc_cfg;
#endif

	int i = 0;
	ca_boolean_t found = FALSE;

    if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
    {
        return __gpon_gem_port_add(device_id, port, gem_port_cfg);
    }

    us_mask.u32 = ~0U;
    ds_mask.u32 = ~0U;

    memset(&us_xgem_cfg, 0, sizeof(aal_xgpon_usbg_gem_config_t));
    memset(&ds_xgem_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_cfg_t));

#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
    puc_rl_voq_tbc_cfg_msk.u32 = 0;
    puc_voq_tbc_cfg_msk.u32 = 0;
    memset(&puc_rl_voq_tbc_cfg, 0, sizeof(aal_puc_voq_tbc_mem_cfg_t));
    memset(&puc_voq_tbc_cfg, 0, sizeof(aal_puc_voq_tbc_mem_cfg_t));
#endif

    if (!gem_port_cfg) {
        ca_printf("gem_port_cfg is NULL\r\n");
        return CA_E_PARAM;
    }
	
	if (gem_port_cfg->direction > CA_GPON_PORT_GEM_PORT_DIRECTION_BI) {
		ret = CA_E_PARAM;
		CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
	}
	if (gem_port_cfg->tcont_id >= tcont_num_per_chip) {
		ret = CA_E_PARAM;
		CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
	}
	if ((gem_port_cfg->up_queue >= queue_num_per_tcont)) {
		ret = CA_E_PARAM;
		CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
	}

    #if 0
    if((gem_port_cfg->gem_port_id>=0)&&(gem_port_cfg->gem_port_id<=1020)){
        ca_printf("%s:%d::gem_id is :%d\r\n",__FUNCTION__,__LINE__,gem_port_cfg->gem_port_id);
        return CA_E_PARAM;

    }
    #endif

	if (gem_port_cfg->direction != CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN) {
	
	    if (!(p_tcont = __gpon_intl_find_tcont_cb (device_id, gem_port_cfg->tcont_id, &tcont_index))){
	        ca_printf("%s:%d::error: tcont_cb of tcont %d is NULL\r\n",__FUNCTION__,__LINE__, gem_port_cfg->tcont_id);
	        ret = CA_E_ERROR;
	        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
	    }
	}

    if (TRUE == gpon_cb[device_id].state) {
		
		for(i = 8; i<CA_GPON_MAX_GEM_PORTS; i++){
			if((gpon_cb[device_id].gem_cb[i].valid)
				&&(gpon_cb[device_id].gem_cb[i].gem_port_id == gem_port_cfg->gem_port_id)){
				if(gpon_cb[device_id].gem_cb[i].direction == gem_port_cfg->direction){
					intern_gem_idx = i;
					found = TRUE;
					break;
				}else{
					ca_printf("Direction for the same gem_port is different, please delete previous entry firstly.\r\n ");
					ret = CA_E_ERROR;
	        		CA_ERR_GOTO_FUNC_EXIT(device_id, ret); 
				}
			}
		}
		
		if(found){ //update gem cfg and gem tcont map
			ret = __gpon_intl_update_gem(device_id,port,intern_gem_idx,gem_port_cfg);
			CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
		}else{ //add new
			/* add gem cfg and gem tcont map*/
	        ret = __gpon_intl_add_gem(device_id, port, tcont_index, is_omci_port, &intern_gem_idx, gem_port_cfg);
	        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
		}

        if ((gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP)||(gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI)) {

            us_xgem_cfg.port_en = 1;
            us_xgem_cfg.xgem_id = gem_port_cfg->gem_port_id;
        	us_xgem_cfg.aes_mode = (gem_port_cfg->aes_key_ring == CA_GPON_PORT_GEM_PORT_ENC_UC_BOTH_DIRECTION) ? 1 : 0;
            us_xgem_cfg.mc_flag = 0;
            ret = aal_xgpon_usbg_xgem_cfg_set(device_id, intern_gem_idx, us_mask, &us_xgem_cfg);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
			if(us_xgem_cfg.aes_mode == 1){
				ret = aal_xgpon_usbg_aes_uni_key_idx_set(device_id,(g_key_idx-1));
				CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
			}
#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
            //set queue pir
            queue_id = gem_port_cfg->tcont_id * queue_num_per_tcont + gem_port_cfg->up_queue;
            puc_rl_voq_tbc_cfg_msk.u32 = ~0U;
            
            if(gem_port_cfg->up_traffic_descriptor.mode == CA_POLICER_MODE_DISABLE){    
                puc_rl_voq_tbc_cfg.enb = 0x0;
            }else {
                puc_rl_voq_tbc_cfg.enb = 0x2;
            }
            puc_rl_voq_tbc_cfg.rate_k = gem_port_cfg->up_traffic_descriptor.pir%500;
            puc_rl_voq_tbc_cfg.rate_m = gem_port_cfg->up_traffic_descriptor.pir/500;
            puc_rl_voq_tbc_cfg.bs = gem_port_cfg->up_traffic_descriptor.cbs;
            if(gem_port_cfg->up_traffic_descriptor.pps){
                puc_rl_voq_tbc_cfg.mode = 1;//packet mode
            }else{
                puc_rl_voq_tbc_cfg.mode = 0;//byte mode
            }
            puc_rl_voq_tbc_cfg.pkt_mode_class_sel = 0;
            puc_rl_voq_tbc_cfg.tbc = 0;

            ret = aal_puc_rl_voq_tbc_mem_set(device_id, queue_id, puc_rl_voq_tbc_cfg_msk, &puc_rl_voq_tbc_cfg);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
            
            //set queue cir
            puc_voq_tbc_cfg_msk.u32 = ~0U;
            if(gem_port_cfg->up_traffic_descriptor.mode == CA_POLICER_MODE_DISABLE){    
                puc_voq_tbc_cfg.enb = 0x0;
            }else {
                puc_voq_tbc_cfg.enb = 0x2;
            }
            puc_voq_tbc_cfg.rate_k = gem_port_cfg->up_traffic_descriptor.cir%500;
            puc_voq_tbc_cfg.rate_m = gem_port_cfg->up_traffic_descriptor.cir/500;
            puc_voq_tbc_cfg.bs = gem_port_cfg->up_traffic_descriptor.cbs;
            if(gem_port_cfg->up_traffic_descriptor.pps){
                puc_voq_tbc_cfg.mode = 1;//packet mode
            }else{
                puc_voq_tbc_cfg.mode = 0;//byte mode
            }
            puc_voq_tbc_cfg.pkt_mode_class_sel = 0;
            puc_voq_tbc_cfg.tbc = 0;
            ret = aal_puc_voq_tbc_mem_set(device_id, queue_id, puc_voq_tbc_cfg_msk, &puc_voq_tbc_cfg);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
#else 
			ret = ca_flow_policer_set(device_id, intern_gem_idx, &(gem_port_cfg->up_traffic_descriptor));
			if(ret != CA_E_OK)
			{
				ca_printf("%s:%d: gem policer cfg fail!\r\n",__FUNCTION__,__LINE__);
			}
#endif
        }
        if ((gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN)||(gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI)) {

            ds_xgem_cfg.gem_vld = 1;
            ds_xgem_cfg.gem_id = gem_port_cfg->gem_port_id;
            if (gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN)
            {
                ds_xgem_cfg.gem_mc = (gem_port_cfg->aes_key_ring == CA_GPON_PORT_GEM_PORT_ENC_BC_MC) ? 1 : 0;
            }
            ret = aal_xgpon_dsfp_xgem_set(device_id, intern_gem_idx, ds_mask, &ds_xgem_cfg);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
        }

        (void)aal_xgpon_dsfp_xgem_get(device_id, 0, &ds_xgem_cfg);

        if (intern_gem_idx > 0 && gem_port_cfg->gem_port_id != ds_xgem_cfg.gem_id) {
            /* once OMCI configures GEM port: PDC_CTRL.pdc_map_mem_en = 1,
             * setup PDC_PDC_MAP_MEM:
             * PDC_PDC_MAP_MEM_DATA policer_id = internal GEM ID, PDC_PDC_MAP_MEM_DATA0.
             * no-drop = 0 for GEM port id != 0;
             */
            aal_pdc_ctrl_mask_t             pdc_ctrl_mask;
            aal_pdc_ctrl_t                  pdc_ctrl;
            aal_pdc_pdc_map_mem_data_mask_t pdc_map_mask;
            aal_pdc_pdc_map_mem_data_t      pdc_map_cfg;

            memset(&pdc_ctrl, 0, sizeof(pdc_ctrl));
            pdc_ctrl_mask.wrd               = 0;
            if (!found) {
                pdc_map_mask.wrd        = 0;
                memset(&pdc_map_cfg, 0, sizeof(pdc_map_cfg));

                pdc_map_mask.bf.no_drop = 1;
                pdc_map_mask.bf.pol_en  = 1;
                pdc_map_mask.bf.pol_id  = 1;
                pdc_map_mask.bf.lspid   = 1;

                pdc_map_cfg.no_drop     = 0;
                pdc_map_cfg.pol_en      = 1;
                pdc_map_cfg.pol_id      = intern_gem_idx; /* internal GEM ID */
                /******12 5th 2017, Jianguang add below codes to support gem_index to src port map*******/
                pdc_map_cfg.lspid       = aal_pdc_free_lspid_get(device_id,intern_gem_idx);
                /**end of Jianguang add**/
                ret = aal_pdc_map_mem_set(device_id, intern_gem_idx, pdc_map_mask, &pdc_map_cfg);
                CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
            }

            ret = aal_puc_pvtbl_enable_set(device_id, gem_port_cfg->tcont_id, TRUE);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

            CA_GPON_DBG("Setup TCONT %d internal TCONT-ID %d GEM Port %d internal GEM-ID %d up_queue %d.\r\n",
                gem_port_cfg->tcont_id, tcont_index, gem_port_cfg->gem_port_id, intern_gem_idx,gem_port_cfg->up_queue);
        }

		if(gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN){
        	gem_port_cfg->gem_index = (0xFF << 8) | intern_gem_idx;
		}else{
	        if((puc_voq_map_mode == AAL_PUC_VOQ_MODE_8Q)||(puc_voq_map_mode == AAL_PUC_VOQ_MODE_4Q)){
				gem_port_cfg->gem_index = ((tcont_index & 0x1f) << 11 ) | ((gem_port_cfg->up_queue) << 8) | intern_gem_idx;
	        }else if(puc_voq_map_mode == AAL_PUC_VOQ_MODE_FLAT_MAP){     
	            gem_port_cfg->gem_index = (intern_gem_idx << 8) |intern_gem_idx;
	        }else{
	            ca_printf("Unsupported voq mode %d for add gemport \r\n", puc_voq_map_mode);
				ret = CA_E_UNAVAIL;
				CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
	        }
		}
        ca_printf("GEM port id 0x%x  - Internal 0x%x created,returned gem_index 0x%x \r\n", gem_port_cfg->gem_port_id, intern_gem_idx,gem_port_cfg->gem_index);
    } else {
        ret = CA_E_UNAVAIL;
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    }
FUNC_EXIT:
    return ret;

}

static ca_status_t __gpon_gem_port_delete(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint16_t gem_index
)
{
    ca_status_t ret = CA_E_OK;
    ca_gpon_port_gem_cb_t * gem_cb = NULL;
    //ca_uint16_t gem_port_id = CA_GPON_INVALID_VALUE;
    ca_uint16_t intern_gem_index = 0;
    ca_uint16_t ptcont = 0;
    ca_uint32_t voq_mode = 0, voq_per_tcont = 8;
    aal_pdc_pdc_map_mem_data_mask_t pdc_map_mask;
    aal_pdc_pdc_map_mem_data_t      pdc_map_cfg;
	
    intern_gem_index = gem_index&0xFF;
    if(intern_gem_index >= CA_GPON_MAX_GEM_PORTS){
        ca_printf("Invalid gem_index %d\r\n",intern_gem_index);
        return CA_E_ERROR;
    }
    gem_cb = __gpon_intl_find_gem_cb (device_id,gem_index);
    if(!gem_cb){
        goto FUNC_EXIT;
    }

    ret =  __gpon_intl_delete_gem_port (device_id, intern_gem_index,gem_cb);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
   // ret = __gpon_tm_delete_gem_port (device_id, port_id, gem_port_id, gpon_cb[device_id].tcont_cb[tcont_idx].tcont_id);
    //CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

    /******12 5th 2017, Jianguang add below codes to support gem_index to src port map*******/
    pdc_map_mask.wrd = 0;
    memset(&pdc_map_cfg,0,sizeof(aal_pdc_pdc_map_mem_data_t));
    ret = aal_pdc_map_mem_get(device_id,intern_gem_index,&pdc_map_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    aal_pdc_lspid_gem_index_set(device_id,pdc_map_cfg.lspid,0xff);
    pdc_map_cfg.lspid = 7;
    pdc_map_mask.bf.lspid = 1;
    ret = aal_pdc_map_mem_set(device_id,intern_gem_index,pdc_map_mask,&pdc_map_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    /******end of add new function **************/
	if (gem_cb->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN) {
        return ret;
    }

    if (NULL == __gpon_intl_find_tcont_cb(device_id, gem_cb->tcont_id, &ptcont)){
        ca_printf("NOT found gem %d related TCONT.\r\n", intern_gem_index);
        goto FUNC_EXIT;
    }

    ca_printf("Found gem %d related TCONT is %d (logical tcont=%d) ,related up_queue is %d.\r\n",
        intern_gem_index, ptcont, gem_cb->tcont_id, gem_cb->up_queue);

    ret =  aal_puc_voq_map_cfg_get(device_id, &voq_mode);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

    if (voq_mode == AAL_PUC_VOQ_MODE_8Q)
        voq_per_tcont = 8;
    else if (voq_mode == AAL_PUC_VOQ_MODE_4Q)
        voq_per_tcont = 4;
    else
        voq_per_tcont = AAL_PUC_QUEUE_NUM;

    //ret = aal_puc_voq_flush_by_idx(device_id, ptcont,
    //  gem_cb->up_queue % voq_per_tcont, gem_cb->up_queue % voq_per_tcont);


    if (__gpon_intl_tcont_tbl_is_empty(device_id, gem_cb->tcont_id)) {
        ca_uint16_t gem_id;
        aal_gpon_mac_ctrl_cfg_msk_t mac_ctrl_mask;
        aal_gpon_mac_ctrl_cfg_t     mac_ctrl_cfg;
       
        ret = aal_gpon_us_gem_port_get(device_id, 0, &gem_id);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
        
        mac_ctrl_mask.u32 = 0;
        memset(&mac_ctrl_cfg, 0, sizeof(mac_ctrl_cfg));
        
        mac_ctrl_mask.s.flush_en = 1;
        mac_ctrl_mask.s.flush_id = 1;
        mac_ctrl_cfg.flush_en = 1;
        mac_ctrl_cfg.flush_id = gem_cb->tcont_id;

        //flush tcont
        ret = aal_gpon_mac_ctrl_set(device_id, mac_ctrl_mask, &mac_ctrl_cfg);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

        if (gem_cb->tcont_id != gem_id) {
            ret = aal_puc_pvtbl_enable_set(device_id, ptcont, FALSE);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
            CA_GPON_DBG("Disable tcont %d PUC VOQ.\r\n", gem_cb->tcont_id);
        }
    }

    //ca_printf("Delete GemId %d\r\n", gem_cb->gem_port_id);

FUNC_EXIT:
    return ret;

}


ca_status_t ca_gpon_port_gem_port_delete(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint16_t gem_index
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_gpon_port_gem_cb_t * gem_cb = NULL;
    //ca_uint16_t gem_port_id = CA_GPON_INVALID_VALUE;
    ca_uint16_t intern_gem_index = 0;
    ca_uint16_t ptcont = 0;
    ca_uint32_t voq_mode = 0, voq_per_tcont = 8;
    aal_pdc_pdc_map_mem_data_mask_t pdc_map_mask;
    aal_pdc_pdc_map_mem_data_t      pdc_map_cfg;
	
    if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
    {
        return __gpon_gem_port_delete(device_id, port, gem_index);
    }
    
    intern_gem_index = gem_index&0xFF;
    if(intern_gem_index >= CA_GPON_MAX_GEM_PORTS){
        ca_printf("Invalid gem_index %d\r\n",intern_gem_index);
        return CA_E_ERROR;
    }
    gem_cb = __gpon_intl_find_gem_cb (device_id,gem_index);
    if(!gem_cb){
        goto FUNC_EXIT;
    }

    ret =  __gpon_intl_delete_gem_port (device_id, intern_gem_index,gem_cb);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
   // ret = __gpon_tm_delete_gem_port (device_id, port_id, gem_port_id, gpon_cb[device_id].tcont_cb[tcont_idx].tcont_id);
    //CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

    /******12 5th 2017, Jianguang add below codes to support gem_index to src port map*******/
    pdc_map_mask.wrd = 0;
    memset(&pdc_map_cfg,0,sizeof(aal_pdc_pdc_map_mem_data_t));
    ret = aal_pdc_map_mem_get(device_id,intern_gem_index,&pdc_map_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    aal_pdc_lspid_gem_index_set(device_id,pdc_map_cfg.lspid,0xff);
    pdc_map_cfg.lspid = 7;
    pdc_map_mask.bf.lspid = 1;
    ret = aal_pdc_map_mem_set(device_id,intern_gem_index,pdc_map_mask,&pdc_map_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    /******end of add new function **************/
	if (gem_cb->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN) {
        return ret;
    }

    if (NULL == __gpon_intl_find_tcont_cb(device_id, gem_cb->tcont_id, &ptcont)){
        ca_printf("NOT found gem %d related TCONT.\r\n", intern_gem_index);
        goto FUNC_EXIT;
    }

    ca_printf("Found gem %d related TCONT is %d (logical tcont=%d) ,related up_queue is %d.\r\n",
        intern_gem_index, ptcont, gem_cb->tcont_id, gem_cb->up_queue);

    ret =  aal_puc_voq_map_cfg_get(device_id, &voq_mode);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

    if (voq_mode == AAL_PUC_VOQ_MODE_8Q)
        voq_per_tcont = 8;
    else if (voq_mode == AAL_PUC_VOQ_MODE_4Q)
        voq_per_tcont = 4;
    else
        voq_per_tcont = AAL_PUC_QUEUE_NUM;

    //ret = aal_puc_voq_flush_by_idx(device_id, ptcont,
    //  gem_cb->up_queue % voq_per_tcont, gem_cb->up_queue % voq_per_tcont);


    if (__gpon_intl_tcont_tbl_is_empty(device_id, gem_cb->tcont_id)) {
        aal_xgpon_bwmp_alloc_id_cfg_t  aal_bwmp_cfg;
        aal_xgpon_dsfp_xgem_cfg_t      aal_dsfp_cfg;

        memset(&aal_bwmp_cfg, 0, sizeof(aal_bwmp_cfg));
        ret = aal_xgpon_bwmp_tcont_get(device_id, 0, ptcont, &aal_bwmp_cfg);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

        ret = aal_xgpon_dsfp_xgem_get(device_id, 0, &aal_dsfp_cfg);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

        ret = aal_xgpon_tcont_flush(device_id, 0, ptcont);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

        if (aal_bwmp_cfg.alloc_id != aal_dsfp_cfg.gem_id) {
            ret = aal_puc_pvtbl_enable_set(device_id, ptcont, FALSE);
            CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
            CA_GPON_DBG("Disable tcont %d PUC VOQ.\r\n", gem_cb->tcont_id);
        }
    }

    //ca_printf("Delete GemId %d\r\n", gem_cb->gem_port_id);

FUNC_EXIT:
    return ret;

}

ca_status_t ca_gpon_port_gem_port_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_gpon_port_gem_port_t *gem_port_cfg
)
{   
	ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
	ca_gpon_port_gem_cb_t * gem_cb = NULL;
    CA_NULL_PTR_CHECK(gem_port_cfg);

	gem_cb = __gpon_intl_find_gem_cb (device_id,gem_port_cfg->gem_index);
    if(!gem_cb){
        return CA_E_NOT_FOUND;
    }
	gem_port_cfg->gem_port_id			= gem_cb->gem_port_id;
    gem_port_cfg->tcont_id 				= gem_cb->tcont_id;
    gem_port_cfg->up_queue 				= gem_cb->up_queue;
	gem_port_cfg->up_traffic_descriptor = gem_cb->up_traffic_descriptor;
    gem_port_cfg->dn_queue 				= gem_cb->dn_queue;
	gem_port_cfg->dn_traffic_descriptor = gem_cb->dn_traffic_descriptor;
    gem_port_cfg->direction 			= gem_cb->direction;
    gem_port_cfg->aes_state 			= gem_cb->aes_state;
    gem_port_cfg->aes_key_ring			= gem_cb->aes_key_ring;
	return ret;
}

ca_status_t ca_gpon_port_gem_port_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_gpon_port_gem_port_t *gem_port_cfg)
{
	ca_status_t ret = CA_E_OK;
	ca_uint32_t port = PORT_ID(port_id); 
    ca_uint16_t intern_gem_idx = 0;
    aal_xgpon_usbg_gem_config_msk_t us_mask;
    aal_xgpon_dsfp_xgem_cfg_msk_t ds_mask;
    aal_xgpon_usbg_gem_config_t us_xgem_cfg;
    aal_xgpon_dsfp_xgem_cfg_t   ds_xgem_cfg;

    us_mask.u32 = ~0U;
    ds_mask.u32 = ~0U;

    memset(&us_xgem_cfg, 0, sizeof(aal_xgpon_usbg_gem_config_t));
    memset(&ds_xgem_cfg, 0, sizeof(aal_xgpon_dsfp_xgem_cfg_t));
	
    if (!gem_port_cfg) {
        ca_printf("gem_port_cfg is NULL\r\n");
        return CA_E_PARAM;
    }

    #if 0
    if((gem_port_cfg->gem_port_id>=0)&&(gem_port_cfg->gem_port_id<=1020)){
        ca_printf("%s:%d::gem_id is :%d\r\n",__FUNCTION__,__LINE__,gem_port_cfg->gem_port_id);
        return CA_E_PARAM;

    }
    #endif

	intern_gem_idx = gem_port_cfg->gem_index&0xFF;
    if (TRUE == gpon_cb[device_id].state) {
        if (gem_port_cfg->direction > CA_GPON_PORT_GEM_PORT_DIRECTION_BI) {
            return CA_E_PARAM;
        }
		if (gem_port_cfg->tcont_id >= tcont_num_per_chip) {
            return CA_E_PARAM;
        }
		if ((gem_port_cfg->up_queue >= queue_num_per_tcont)||(gem_port_cfg->dn_queue >= queue_num_per_tcont)) {
            return CA_E_PARAM;
        }
		//update gem cb
		ret = __gpon_intl_update_gem(device_id, port,(gem_port_cfg->gem_index&0xFF), gem_port_cfg);
		CA_RET_VALUE_CHECK(ret);

        if ((gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_UP)
			||(gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI)){
            us_xgem_cfg.port_en = 1;
            us_xgem_cfg.xgem_id = gem_port_cfg->gem_port_id;
			if(gem_port_cfg->aes_key_ring == CA_GPON_PORT_GEM_PORT_ENC_UC_BOTH_DIRECTION)
			{
				us_xgem_cfg.aes_mode = 1;
			}
			if(gem_port_cfg->aes_key_ring == CA_GPON_PORT_GEM_PORT_ENC_DISABLE)
			{
				us_xgem_cfg.aes_mode = 0;
			}
            us_xgem_cfg.mc_flag = 0;
            ret = aal_xgpon_usbg_xgem_cfg_set(device_id, intern_gem_idx, us_mask, &us_xgem_cfg);
            CA_RET_VALUE_CHECK(ret);
#if defined(CONFIG_ARCH_REALTEK_TAURUS)
            //77C not using ca api
#else
			ret = ca_flow_policer_set(device_id, intern_gem_idx,&(gem_port_cfg->up_traffic_descriptor));
			CA_RET_VALUE_CHECK(ret);
#endif
        }
        if ((gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_DOWN)
			||(gem_port_cfg->direction == CA_GPON_PORT_GEM_PORT_DIRECTION_BI)) {
            ds_xgem_cfg.gem_vld = 1;
            ds_xgem_cfg.gem_id = gem_port_cfg->gem_port_id;
            ds_xgem_cfg.gem_mc = 0;
            ret = aal_xgpon_dsfp_xgem_set(device_id, intern_gem_idx, ds_mask, &ds_xgem_cfg);
            CA_RET_VALUE_CHECK(ret);
#if defined(CONFIG_ARCH_REALTEK_TAURUS)
            //77C not using ca api
#else
			ret = ca_flow_policer_set(device_id, intern_gem_idx,&(gem_port_cfg->dn_traffic_descriptor));
			CA_RET_VALUE_CHECK(ret);
#endif
        }
    }
    return CA_E_OK;
}


ca_status_t ca_gpon_port_omci_port_id_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_OUT ca_uint32_t *omci_port_id
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;

    CA_DEV_ID_CHECK(device_id);
    CA_ASSERT_RET(omci_port_id != NULL, CA_E_PARAM);

    memset(&alloc_id_cfg, 0, sizeof(aal_xgpon_bwmp_alloc_id_cfg_t));
    ret =  aal_xgpon_bwmp_tcont_get(device_id, AAL_XGPON_WAVE_CHANNEL_DEFAULT,  0, &alloc_id_cfg);
    CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    *omci_port_id = alloc_id_cfg.alloc_id;
FUNC_EXIT:
    return ret;
}

ca_status_t ca_gpon_port_gem_stats_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_boolean read_clear,
    CA_IN ca_uint32_t gem_port_index,
    CA_OUT ca_gpon_port_gem_stats_t *gem_stats
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_dsfp_xgem_mib_config_t dsfp_xgem_mib;
    aal_xgpon_usbg_xgem_mib_config_t usbg_xgem_mib;
	aal_xgpon_xgpn_glb_ctrl_config_msk_t  mac_msk;
	aal_xgpon_xgpn_glb_ctrl_config_t	  mac_ctrl;
    aal_xgpon_dsfp_stats_t  dsfp_stats;

    CA_DEV_ID_CHECK(device_id);
    CA_ASSERT_RET(gem_stats != NULL, CA_E_PARAM);

    memset(&dsfp_xgem_mib, 0, sizeof(aal_xgpon_dsfp_xgem_mib_config_t));
    memset(&usbg_xgem_mib, 0, sizeof(aal_xgpon_usbg_xgem_mib_config_t));
    memset(&dsfp_stats, 0, sizeof(aal_xgpon_dsfp_stats_t));
	
    mac_msk.u32 = 0;
    mac_msk.s.pm_mode = 1;
    memset(&mac_ctrl, 0, sizeof(mac_ctrl));

	if(read_clear){
		mac_ctrl.pm_mode = 2;
	}else{
		mac_ctrl.pm_mode = 0;
	}
	ret = aal_xgpon_xgpn_glb_ctrl_set(device_id, mac_msk, &mac_ctrl);
	CA_RET_VALUE_CHECK(ret);

    ret = aal_xgpon_dsfp_stats_get(device_id, &dsfp_stats);
    CA_RET_VALUE_CHECK(ret);
	ret = aal_xgpon_dsfp_xgem_mib_get(device_id,gem_port_index&0xFF,&dsfp_xgem_mib);
    CA_RET_VALUE_CHECK(ret);
    gem_stats->rx_gem_frame = dsfp_xgem_mib.frame_cnt;
    gem_stats->rx_gem_payload = dsfp_xgem_mib.byte_cnt;
    gem_stats->rx_gem_crc = dsfp_stats.rxmac_pkt_crc_err;
    ret = aal_xgpon_usbg_xgem_mib_get(device_id,gem_port_index&0xFF,&usbg_xgem_mib);
    CA_RET_VALUE_CHECK(ret);
    gem_stats->tx_gem_frame = usbg_xgem_mib.frame_cnt;
    gem_stats->tx_gem_payload = usbg_xgem_mib.byte_cnt;
    if(!read_clear){
		//read saturation interrupt times for calculating counters
    }
    else{//read clear
		memset(&dsfp_xgem_mib, 0, sizeof(aal_xgpon_dsfp_xgem_mib_config_t));
		memset(&usbg_xgem_mib, 0, sizeof(aal_xgpon_usbg_xgem_mib_config_t));
        memset(&dsfp_stats, 0, sizeof(aal_xgpon_dsfp_stats_t));
        
		ret = aal_xgpon_dsfp_xgem_mib_set(device_id,gem_port_index&0xFF,&dsfp_xgem_mib);
		CA_RET_VALUE_CHECK(ret);
		ret = aal_xgpon_usbg_xgem_mib_set(device_id,gem_port_index&0xFF,&usbg_xgem_mib);
		CA_RET_VALUE_CHECK(ret);
        ret = aal_xgpon_dsfp_stats_get(device_id, &dsfp_stats);
        CA_RET_VALUE_CHECK(ret);
    }
    return ret;
}

ca_status_t ca_gpon_port_upstream_queue_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t queue_id,
    CA_IN ca_boolean_t queue_enable,
    CA_IN ca_uint32_t tcont_id,
    CA_IN ca_uint32_t scheduler_id,
    CA_IN ca_uint32_t weight,
    CA_IN ca_uint8_t pri
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_gpon_tcont_cb_t  *p_tcont = NULL;
    ca_gpon_port_scheduler_t *sheduler_cb = NULL;
    ca_uint16_t p_tcont_index = 0xFFFF;
    aal_puc_queue_cfg_msk_t mask,old_mask;
    aal_puc_queue_cfg_t queue_cfg,voq_cfg_old;
	aal_puc_queue_cfg_t queue_cfg_tmp;
	aal_puc_queue_cfg_t queue_cfg_other;
#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
	aal_puc_voq_tbc_mem_cfg_msk_t puc_voq_tbc_cfg_msk;
    aal_puc_voq_tbc_mem_cfg_t puc_voq_tbc_cfg;
#else
    aal_puc_voq_drop_cfg_msk_t voq_drp_mask;
    aal_puc_voq_drop_cfg_t voq_drp;
#endif    

    ca_uint32_t loopcnt;

    ca_boolean_t q_op_flag = FALSE;
	ca_uint32_t tbc = 0;
	aal_puc_voq_sche_cfg_t voq_sche_config;
	int i = 0 ;
	
	CA_BOOL_VAL_CHECK(queue_enable);
	if ((tcont_id >= tcont_num_per_chip)||(tcont_id == 0)){
        ca_printf("ERROR: FUNC %s LINE %d: tcont_id is :%d.\r\n",__FUNCTION__,__LINE__,tcont_id);
        return CA_E_PARAM;
    }
	
	if(queue_id != (tcont_id * queue_num_per_tcont + pri)){
		ca_printf("ERROR: FUNC %s LINE %d: queue_id is :%d, queue_id should equals (tcont_id * queue_num_per_tcont + pri).\r\n",__FUNCTION__,__LINE__,queue_id);
		return CA_E_PARAM;
	}

	if (weight > CA_GPON_MAX_SCH_WEIGHT){
        ca_printf("ERROR: FUNC %s LINE %d: weight is :%d, weight should be 0 to 255.\r\n",__FUNCTION__,__LINE__,weight);
        return CA_E_PARAM;
    }
	
    if (pri >= queue_num_per_tcont){
        ca_printf("ERROR: FUNC %s LINE %d: pri is :%d, pri should be 0 to 7.\r\n",__FUNCTION__,__LINE__,pri);
        return CA_E_PARAM;
    }
    
	memset(&queue_cfg_tmp, 0, sizeof(aal_puc_queue_cfg_t));
	memset(&queue_cfg_other, 0, sizeof(aal_puc_queue_cfg_t));
	
	mask.u32 = ~0U;
    	
#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)
    //enable voq drain
    puc_voq_tbc_cfg_msk.u32 = 0;
    memset(&puc_voq_tbc_cfg,0, sizeof(aal_puc_voq_tbc_mem_cfg_t));
    puc_voq_tbc_cfg_msk.s.enb = 1;
    puc_voq_tbc_cfg.enb = 1;
    ret = aal_puc_rl_voq_tbc_mem_set(device_id, queue_id, puc_voq_tbc_cfg_msk, &puc_voq_tbc_cfg);
    CA_RET_VALUE_CHECK(ret);
#else
    memset(&voq_drp, 0, sizeof(voq_drp));
    voq_drp_mask.u32      = 0;
    voq_drp_mask.s.voqid  = 1;
    voq_drp_mask.s.enable = 1;
#endif

    if (scheduler_id == 0xFFFF) {
        if (!(p_tcont = __gpon_intl_find_tcont_cb (device_id, tcont_id, &p_tcont_index))) {
            ca_printf("ERROR: FUNC %s LINE %d: p_tcont_index is %d,tcont id is :%d.\r\n",__FUNCTION__,__LINE__,p_tcont_index, tcont_id);
            return CA_E_ERROR;
        }

        if (p_tcont->sched_mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP) {
            if (weight != 0){
                ca_printf("ERROR: FUNC %s LINE %d: queue weight in sp mode should be 0.\r\n",__FUNCTION__,__LINE__);
                return CA_E_PARAM;
            }
        }
		
        if (p_tcont->sched_mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_WRR) {
            if (weight == 0){
                ca_printf("ERROR: FUNC %s LINE %d: queue weight in WRR mode should not be 0.\r\n",__FUNCTION__,__LINE__);
                return CA_E_PARAM;
            }

        }

        if(queue_enable){//config queue
            if (p_tcont->queue_cb[pri].vld == TRUE){
				if(p_tcont->queue_cb[pri].queue_id == queue_id){
                    ca_printf("ERROR: FUNC %s LINE %d: queue existed,loopcnt is %d,queue_id is :%d.\n",__FUNCTION__,__LINE__,pri,queue_id);
					return CA_E_EXISTS;
				}
            }
			
			//flush queue before configure it
			ret = aal_puc_voq_flush_by_idx(device_id, tcont_id,queue_id % queue_num_per_tcont,
                    queue_id % queue_num_per_tcont);
			CA_RET_VALUE_CHECK(ret);

			if (p_tcont->sched_mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP_WRR) {
				if(weight != 0 ){
                    if(pri>=(queue_num_per_tcont - p_tcont->num_sp_queues))
                    {
                        ca_printf("ERROR: FUNC %s LINE %d: Queue config error, wrr queue should NOT be at higher priority.\r\n",__FUNCTION__,__LINE__);
                        return CA_E_PARAM;
					}
				}else{
				    if(pri <(queue_num_per_tcont - p_tcont->num_sp_queues))
                    {
                        ca_printf("ERROR: FUNC %s LINE %d: Queue config error, sp queue should NOT be at lower priority.\r\n",__FUNCTION__,__LINE__);
                        return CA_E_PARAM;
							}
						}					
					}
            
#if defined(CONFIG_ARCH_CORTINA_VENUS) || defined(CONFIG_ARCH_REALTEK_TAURUS)        
            //disable voq drain
            puc_voq_tbc_cfg.enb = 0;
            ret = aal_puc_rl_voq_tbc_mem_set(device_id, queue_id, puc_voq_tbc_cfg_msk, &puc_voq_tbc_cfg);
						CA_RET_VALUE_CHECK(ret);
#else
            voq_drp.enable = 0;
            voq_drp.voqid = queue_id;
            ret = aal_puc_voq_drop_set(device_id, voq_drp_mask, &voq_drp);
            CA_RET_VALUE_CHECK(ret);
#endif
			queue_cfg.queue_id = queue_id;
		    queue_cfg.queue_weight = weight;
		    queue_cfg.queue_pri = pri;
			queue_cfg.queue_ena = 1;
            ret = aal_puc_port_voq_map_set(device_id, p_tcont_index, pri, mask, &queue_cfg);
            CA_RET_VALUE_CHECK(ret);

			memset(&voq_sche_config, 0, sizeof(aal_puc_voq_sche_cfg_t));
			ret = aal_puc_voq_sche_cfg_get(device_id,&voq_sche_config);
			CA_RET_VALUE_CHECK(ret);
			if(voq_sche_config.basesel == 0)
				tbc = weight * 128;
			if(voq_sche_config.basesel == 1)
				tbc = weight * 256;
			if(voq_sche_config.basesel == 2)
				tbc = weight * 512;
			if(voq_sche_config.basesel == 3)
				tbc = weight * 1024;
			ret = aal_puc_port_voq_sche_info_tbl_set(device_id,p_tcont_index,pri,tbc);
			CA_RET_VALUE_CHECK(ret);

			//update queue cb
			p_tcont->queue_cb[pri].vld = TRUE;
            p_tcont->queue_cb[pri].queue_id = queue_id;
            p_tcont->queue_cb[pri].queue_weight = weight;
            p_tcont->queue_cb[pri].queue_prio = pri;
            p_tcont->queue_cb[pri].tcont_id = tcont_id;
            p_tcont->queue_cb[pri].scheduler_id = scheduler_id;

            q_op_flag = TRUE;

        }else{//delete queue			
            old_mask.u32 = 0;
			memset(&voq_cfg_old, 0, sizeof(voq_cfg_old));
        	ret = aal_puc_port_voq_map_get(device_id, tcont_id, pri,&voq_cfg_old);
			CA_RET_VALUE_CHECK(ret);

            //ca_printf("aal_puc_voq_flush_by_idx:tcontid:%d,queue_id :%d\r\n",tcont_id, queue_id);
            ret = aal_puc_voq_flush_by_idx(device_id, tcont_id,queue_id % queue_num_per_tcont,
                    queue_id % queue_num_per_tcont);
			CA_RET_VALUE_CHECK(ret);

			old_mask.s.queue_ena = 1;
			old_mask.s.queue_id = 1;
			old_mask.s.queue_weight = 1;
			
			//voq_cfg_old.queue_ena = 1;
			//voq_cfg_old.queue_id = 0;
			if(voq_cfg_old.queue_weight != 0){
				voq_cfg_old.queue_weight = 1;
			}

			ret = aal_puc_port_voq_map_set(device_id, tcont_id, pri,old_mask,&voq_cfg_old);
			CA_RET_VALUE_CHECK(ret);

			ret = aal_puc_port_voq_sche_info_tbl_set(device_id,p_tcont_index,pri,0);
			CA_RET_VALUE_CHECK(ret);
			//delete queue config from queue cb
			if ((p_tcont->queue_cb[pri].vld == TRUE) 
				&& (p_tcont->queue_cb[pri].queue_id == queue_id)){
				p_tcont->queue_cb[pri].vld = FALSE;
				p_tcont->queue_cb[pri].queue_id = CA_GPON_INVALID_VALUE;
				p_tcont->queue_cb[pri].queue_weight = CA_GPON_INVALID_VALUE;
				p_tcont->queue_cb[pri].queue_prio = CA_GPON_INVALID_VALUE;
				p_tcont->queue_cb[pri].tcont_id = CA_GPON_INVALID_VALUE;
				p_tcont->queue_cb[pri].scheduler_id = CA_GPON_INVALID_VALUE;

				q_op_flag = FALSE;
			}else{
				ca_printf("ERROR: FUNC %s LINE %d: Delete queue fail.\r\n");
				return CA_E_NOT_FOUND;
			}
			
        }
    } else {
        if (!(sheduler_cb = __gpon_get_scheduler_cb_by_scheduler_id (device_id, scheduler_id))) {
            return CA_E_ERROR;
        }

        if (sheduler_cb->mode == CA_GPON_PORT_SCHEDULER_QUEUE_SCHEDULER_SP) {
            if (weight != 0){
                ca_printf("ERROR: FUNC %s LINE %d: queue weight in sp mode should be 0.\r\n");
                return CA_E_PARAM;
            }
        }
        if (sheduler_cb->mode == CA_GPON_PORT_SCHEDULER_QUEUE_SCHEDULER_WRR) {
            if (weight == 0){
                ca_printf("ERROR: FUNC %s LINE %d: queue weight in WRR mode should not be 0.\r\n");
                return CA_E_PARAM;
            }
        }

        for (loopcnt = 0; loopcnt < queue_num_per_tcont; loopcnt++) {
            if ((sheduler_cb->queue_cb[loopcnt].vld == TRUE) && (sheduler_cb->queue_cb[loopcnt].queue_id == queue_id)) {
                ca_printf("queue existed\n");
				return CA_E_ERROR;
            }

            if (sheduler_cb->queue_cb[loopcnt].vld == FALSE) {
                sheduler_cb->queue_cb[loopcnt].vld = TRUE;
                sheduler_cb->queue_cb[loopcnt].queue_id = queue_id;
                sheduler_cb->queue_cb[loopcnt].queue_weight = weight;
                sheduler_cb->queue_cb[loopcnt].queue_prio = pri;
                sheduler_cb->queue_cb[loopcnt].tcont_id = tcont_id;
                sheduler_cb->queue_cb[loopcnt].scheduler_id = scheduler_id;
                break;
            }
        }
    }
	
    if(q_op_flag){
        ca_printf("Set QueueID %u, TCONT %u, SchedulerId %u, Weight %u, pri %u\r\n",
            queue_id, tcont_id, scheduler_id, weight, pri);
    }else{
        ca_printf("Del QueueID %u, TCONT %u, SchedulerId %u, Weight %u, pri %u\r\n",
            queue_id, tcont_id, scheduler_id, weight, pri);
    }
    return ret;
}

ca_status_t ca_gpon_port_upstream_queue_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t queue_id,
    CA_OUT ca_boolean_t *queue_enable,
    CA_OUT ca_uint32_t * tcont_id,
    CA_OUT ca_uint32_t *scheduler_id,
    CA_OUT ca_uint32_t *weight,
    CA_OUT ca_uint8_t *pri
)
{
    ca_uint32_t port = PORT_ID(port_id); 
    int tcont_indx;
    int sche_indx;
    int queue_cnt;
    int t_flag = 0;
    int s_flag = 0;
    for (tcont_indx = 0; tcont_indx < tcont_num_per_chip; tcont_indx++ ) {
        if(gpon_cb[device_id].tcont_cb[tcont_indx].valid){
            for (queue_cnt = 0; queue_cnt < queue_num_per_tcont; queue_cnt++ ) {
                if ((gpon_cb[device_id].tcont_cb[tcont_indx].queue_cb[queue_cnt].queue_id == queue_id)
                    && (gpon_cb[device_id].tcont_cb[tcont_indx].queue_cb[queue_cnt].vld)) {
                    t_flag = 1;
                    *tcont_id = gpon_cb[device_id].tcont_cb[tcont_indx].tcont_id;
                    //*alloc_id = gpon_cb[device_id].tcont_cb[tcont_indx].alloc_id;
                    *scheduler_id = gpon_cb[device_id].tcont_cb[tcont_indx].queue_cb[queue_cnt].scheduler_id;
                    *weight = gpon_cb[device_id].tcont_cb[tcont_indx].queue_cb[queue_cnt].queue_weight;
                    *pri = gpon_cb[device_id].tcont_cb[tcont_indx].queue_cb[queue_cnt].queue_prio;
                    *queue_enable = gpon_cb[device_id].tcont_cb[tcont_indx].queue_cb[queue_cnt].vld;
                    break;
                }
            }
        }
        if(t_flag == 1){
            break;
        }
    }

    if(t_flag!=1){
        for (sche_indx = 0; sche_indx < 2; sche_indx++ ) {
            if(gpon_cb[device_id].scheduler_cb[sche_indx].vld){
                for (queue_cnt = 0; queue_cnt < queue_num_per_tcont; queue_cnt++ ) {
                    if ((gpon_cb[device_id].scheduler_cb[sche_indx].queue_cb[queue_cnt].queue_id == queue_id)
                        &&(gpon_cb[device_id].scheduler_cb[sche_indx].queue_cb[queue_cnt].vld)){
                        s_flag = 1;
                        *tcont_id = gpon_cb[device_id].scheduler_cb[sche_indx].tcont_id;
                        *scheduler_id = gpon_cb[device_id].scheduler_cb[sche_indx].queue_cb[queue_cnt].scheduler_id;
                        *weight = gpon_cb[device_id].scheduler_cb[sche_indx].queue_cb[queue_cnt].queue_weight;
                        *pri = gpon_cb[device_id].scheduler_cb[sche_indx].queue_cb[queue_cnt].queue_prio;
                        *queue_enable = gpon_cb[device_id].scheduler_cb[sche_indx].queue_cb[queue_cnt].vld;
                        break;
                    }
                }
            }
            if(s_flag == 1){
                break;
            }
        }
        if(s_flag!=1){


            *tcont_id = CA_GPON_INVALID_VALUE;
            //*alloc_id = CA_GPON_INVALID_VALUE;
            *scheduler_id = CA_GPON_INVALID_VALUE;
            *weight = CA_GPON_INVALID_VALUE;
            *pri = 0xFF;
            *queue_enable = FALSE;
        }
    }

    return CA_E_OK;
}

ca_status_t ca_gpon_port_upstream_scheduler_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t scheduler_id,
    CA_IN ca_uint32_t tcont_id,
    CA_IN ca_gpon_port_tcont_scheduler_mode_t policy,
    CA_IN ca_uint32_t pri_weight
)
{
    ca_status_t          status = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 

    int sche_indx;

   /* if((scheduler_id !=0)||(scheduler_id !=1)){
        ca_printf("scheduler id should be 0 or 1!! \r\n");
        return CA_E_ERROR;
    }*/
    if (tcont_id >= tcont_num_per_chip){
        ca_printf("%s:%d::tcont_id is :%d\r\n",__FUNCTION__,__LINE__,tcont_id);
        return CA_E_PARAM;
    }

    if(scheduler_id == 0xFFFF){
        ca_printf("%s:%d::No scheduler can be configured!! scheduler_id is 0x%x\r\n",__FUNCTION__,__LINE__,scheduler_id);
        return CA_E_OK;
    }

    for (sche_indx = 0; sche_indx < 2; sche_indx++ ) {
        if ((gpon_cb[device_id].scheduler_cb[sche_indx].vld == TRUE) && (gpon_cb[device_id].scheduler_cb[sche_indx].scheduler_id == scheduler_id)) {
            ca_printf("%s:%d::scheduler existed!! \r\n",__FUNCTION__,__LINE__);
            goto __api_end;
        }
        if (gpon_cb[device_id].scheduler_cb[sche_indx].vld == FALSE) {
            gpon_cb[device_id].scheduler_cb[sche_indx].vld = TRUE;
            gpon_cb[device_id].scheduler_cb[sche_indx].scheduler_id = scheduler_id;
            gpon_cb[device_id].scheduler_cb[sche_indx].scheduler_weight = pri_weight;
            gpon_cb[device_id].scheduler_cb[sche_indx].mode = policy;
            gpon_cb[device_id].scheduler_cb[sche_indx].tcont_id = tcont_id;
            break;
        }

    }

__api_end:

    return status;
}

ca_status_t ca_gpon_port_upstream_scheduler_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t scheduler_id,
    CA_OUT ca_uint32_t *tcont_id,
    CA_OUT ca_gpon_port_tcont_scheduler_mode_t *policy,
    CA_OUT ca_uint32_t *pri_weight
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_gpon_port_scheduler_t *sheduler_cb = NULL;
    if (!(sheduler_cb = __gpon_get_scheduler_cb_by_scheduler_id (device_id, scheduler_id))) {
        *tcont_id = CA_GPON_INVALID_VALUE;
        *policy = CA_GPON_INVALID_VALUE;
        *pri_weight  = CA_GPON_INVALID_VALUE;
        goto __api_end;
    }

    *tcont_id = sheduler_cb->tcont_id;
    *policy = sheduler_cb->mode;
    *pri_weight  = sheduler_cb->scheduler_weight;

__api_end:

    return ret;
}


static ca_status_t __gpon_mode_tcont_set(
        CA_IN ca_device_id_t device_id,
        CA_IN ca_port_id_t port_id,
        CA_IN ca_uint32_t alloc_id,
        CA_IN ca_uint32_t tcont_id,
        CA_IN ca_boolean_t t_enable,
        CA_IN ca_gpon_port_tcont_scheduler_t *scheduler
)
{
    ca_status_t               ret         = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_gpon_onu_states_t     onu_state   = AAL_GPON_STATE_INITIAL;
    ca_gpon_tcont_cb_t       *tcont_cb    = NULL;
    ca_uint16_t               tcont_index = 0;
    aal_gpon_tcont_cfg_msk_t  mask        = {0};
    aal_gpon_tcont_cfg_t      cfg         = {0};

    ret = aal_gpon_onu_state_get(device_id, port, &onu_state);
    if (AAL_GPON_STATE_OPERATION != onu_state)
    {
        ca_printf("%s:%d::ERROR: ONU NOT register,ALLOC ID can't be configured.\r\n",__FUNCTION__,__LINE__);
        return CA_E_ERROR;
    }

    if ((tcont_id == CA_GPON_OMCI_TCONT_ID)
     || (tcont_id >= tcont_num_per_chip)
     || (alloc_id > CA_GPON_ALLOC_ID_MAX))
    {
        ca_printf("%s:%d::ERROR: TCONT[%d] alloc[%d] parameters error\r\n",
                  __FUNCTION__, __LINE__, tcont_id, alloc_id);
        return CA_E_PARAM;
    }

    if ((scheduler->mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP)
     && (scheduler->sp_queue_num != queue_num_per_tcont)){
        ca_printf("err: %s:%d:scheduler->sp_queue_num is %d, queue_num_per_tcont = %d\r\n", __FUNCTION__, __LINE__,
            scheduler->sp_queue_num, queue_num_per_tcont);
        return CA_E_PARAM;
    }

    if ((scheduler->mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_WRR)
     && (scheduler->sp_queue_num != 0)) {
        ca_printf("err: %s:%d:scheduler->sp_queue_num is %d\r\n", __FUNCTION__, __LINE__, scheduler->sp_queue_num);
        return CA_E_PARAM;
    }

    if ((scheduler->mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP_WRR)
     && ((scheduler->sp_queue_num == 0) || (scheduler->sp_queue_num == queue_num_per_tcont))) {
        ca_printf("err: %s:%d:scheduler->sp_queue_num is %d\r\n", __FUNCTION__, __LINE__, scheduler->sp_queue_num);
        return CA_E_PARAM;
    }

    if (FALSE == t_enable) { /* remove TCONT */
        tcont_cb = __gpon_intl_find_tcont_cb(device_id, tcont_id, &tcont_index);
        if ((NULL == tcont_cb) || (tcont_index == CA_GPON_OMCI_TCONT_ID)) {
            return CA_E_OK;
        }

        mask.s.tcont_index = 1;
        mask.s.omci_en     = 1;
        mask.s.ploam_en    = 1;
        cfg.tcont_index    = 0;
        cfg.omci_en        = 0;
        cfg.ploam_en       = 0;
        ret = aal_gpon_tcont_set(device_id, TRUE, 0, mask, &cfg);
        if (CA_E_OK != ret)
        {
            ca_printf("reset tcont idx[%d] ret[%d]\r\n", tcont_index, ret);
            return ret;
        }

        __gpon_intl_delete_tcont_cb(device_id, port, tcont_index);
        return CA_E_OK;
    }

    /* create TCONT */
    tcont_index = tcont_id;
    ret = __gpon_intl_add_tcont_cb(device_id, port, tcont_id, alloc_id, scheduler);
    if ((CA_E_EXISTS != ret) && (CA_E_OK != ret))
    {
        ca_printf("add tcont id[%d] alloc[%d] ret[%d]\r\n", tcont_id, alloc_id, ret);
        return ret;
    }

    if (CA_E_EXISTS == ret)
        return CA_E_OK;

    CA_GPON_DBG("gpon_cb[device_id].tcont_cb[%d] = 0x%x: \r\n",tcont_index,gpon_cb[device_id].tcont_cb[tcont_index].alloc_id);
    mask.s.tcont_index = 1;
    mask.s.omci_en     = 1;
    mask.s.ploam_en    = 1;
    cfg.tcont_index    = tcont_index;
    cfg.omci_en        = 1;
    cfg.ploam_en       = 1;
    ret = aal_gpon_tcont_set(device_id, TRUE, alloc_id, mask, &cfg);
    if (CA_E_OK != ret)
    {
        ca_printf("set tcont idx[%d] alloc[%d] ret[%d]\r\n", tcont_index, alloc_id, ret);
        return ret;
    }

    return CA_E_OK;
}


ca_status_t ca_gpon_port_tcont_set(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t alloc_id,
    CA_IN ca_uint32_t tcont_id,
    CA_IN ca_boolean_t t_enable,
    CA_IN ca_gpon_port_tcont_scheduler_t *scheduler
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint16_t tcont_index = tcont_id;
    aal_xgpon_bwmp_alloc_id_cfg_msk_t mask;
    aal_xgpon_bwmp_alloc_id_cfg_t alloc_id_cfg;
//    aal_xgpon_xgpn_glb_ctrl_config_msk_t xgpn_glb_ctrl_msk;
//    aal_xgpon_xgpn_glb_ctrl_config_t xgpn_glb_ctrl;
    ca_gpon_tcont_cb_t  *tcont_cb = NULL;
    ca_uint8_t           uwlch_id = AAL_XGPON_WAVE_CHANNEL_DEFAULT; // TODO: uwlch_id
	aal_xgpon_actp_status_t actp_sts;
  
    aal_puc_queue_cfg_msk_t queue_cfg_mask;
    aal_puc_queue_cfg_t queue_cfg;
    int i = 0 ;
    
    CA_NULL_PTR_CHECK(scheduler);
	
    if (ONU_PON_MAC_GPON == aal_pon_mac_mode)
    {
        return __gpon_mode_tcont_set(device_id, port, alloc_id, tcont_id, t_enable, scheduler);
    }

    memset(&actp_sts,0,sizeof(actp_sts));
    ret = aal_xgpon_actp_status_get(0,&actp_sts);
    CA_RET_VALUE_CHECK(ret);
    if(actp_sts.onu_state != 3)
    {
		ca_printf("%s:%d::ERROR: ONU NOT register,ALLOC ID can't be configured.\r\n",__FUNCTION__,__LINE__);
        return CA_E_ERROR;
    }
	
#if 0
    /*alloc id :0-1020 is used as default by omcc ,can't be configured by omci,see 989.3 6.1.5.7*/
    if ((alloc_id>=0)&&(alloc_id<=1023))  {
        ca_printf("%s:%d::alloc_id is :%d\r\n",__FUNCTION__,__LINE__,alloc_id);
        return CA_E_OK; //ldchen : CA_E_PARAM;
    }
#endif
    if(tcont_id == 0){
        ca_printf("%s:%d::ERROR: TCONT %d only used for OMCC!\r\n",__FUNCTION__,__LINE__,tcont_id);
        return CA_E_PARAM;
    }
    if (tcont_id >= tcont_num_per_chip){
        ca_printf("%s:%d::tcont_id is :%d\r\n",__FUNCTION__,__LINE__,tcont_id);
        return CA_E_PARAM;
    }

    if (alloc_id > 0x3FFF ){
        ca_printf("%s:%d::alloc_id is :%d\r\n",__FUNCTION__,__LINE__,alloc_id);
        return CA_E_PARAM;
    }

    /*if (PAL_MUTEX_TAKE(gpon_cb[device_id].sem, PAL_MUTEX_TIMEOUT) != CS_SUCCESS)
    {
        return CA_E_ERROR;
    }*/
    memset(&queue_cfg, 0, sizeof(aal_puc_queue_cfg_t));
    queue_cfg_mask.u32 = ~0U;
    
    if (scheduler->mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP) {
        if (scheduler->sp_queue_num != queue_num_per_tcont){
            ca_printf("err: %s:%d:scheduler->sp_queue_num is %d, queue_num_per_tcont = %d\r\n",__FUNCTION__,__LINE__,
                scheduler->sp_queue_num, queue_num_per_tcont);
            return CA_E_PARAM;
        }

        queue_cfg.queue_weight = 0;
		queue_cfg.queue_ena = 1;
    
        for(i = 0; i< queue_num_per_tcont ; i++)
        { 
            queue_cfg.queue_id = tcont_id * queue_num_per_tcont + i;
		    queue_cfg.queue_pri = i;

            ret = aal_puc_port_voq_map_set(device_id, tcont_id, i, queue_cfg_mask, &queue_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
    }
    if (scheduler->mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_WRR) {
        if (scheduler->sp_queue_num != 0){
            ca_printf("err: %s:%d:scheduler->sp_queue_num is %d\r\n",__FUNCTION__,__LINE__,scheduler->sp_queue_num);
            return CA_E_PARAM;
        }

        queue_cfg.queue_weight = 1;
        queue_cfg.queue_ena = 1;
    
        for(i = 0; i< queue_num_per_tcont ; i++)
        { 
            queue_cfg.queue_id = tcont_id * queue_num_per_tcont + i;
            queue_cfg.queue_pri = i;

            ret = aal_puc_port_voq_map_set(device_id, tcont_id, i, queue_cfg_mask, &queue_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
    }
    if (scheduler->mode == CA_GPON_PORT_TCONT_QUEUE_SCHEDULER_SP_WRR) {
        if ((scheduler->sp_queue_num == 0) || (scheduler->sp_queue_num == queue_num_per_tcont)){
            ca_printf("err: %s:%d:scheduler->sp_queue_num is %d\r\n",__FUNCTION__,__LINE__,scheduler->sp_queue_num);
            return CA_E_PARAM;
        }

        //sp queue set
        queue_cfg.queue_weight = 0;
		queue_cfg.queue_ena = 1;
    
        for(i = (queue_num_per_tcont-1); i>=(queue_num_per_tcont - scheduler->sp_queue_num); i--)
        { 
            queue_cfg.queue_id = tcont_id * queue_num_per_tcont + i;
		    queue_cfg.queue_pri = i;
            
            ret = aal_puc_port_voq_map_set(device_id, tcont_id, i, queue_cfg_mask, &queue_cfg);
            CA_RET_VALUE_CHECK(ret);
        }

        //wrr queue set
        queue_cfg.queue_weight = 1;
		queue_cfg.queue_ena = 1;
    
        for(i = 0; i<(queue_num_per_tcont - scheduler->sp_queue_num); i++)
        { 
            queue_cfg.queue_id = tcont_id * queue_num_per_tcont + i;
		    queue_cfg.queue_pri = i;
            
            ret = aal_puc_port_voq_map_set(device_id, tcont_id, i, queue_cfg_mask, &queue_cfg);
            CA_RET_VALUE_CHECK(ret);
        }
    }

    if (!t_enable) { /* remove TCONT */

        tcont_cb = __gpon_intl_find_tcont_cb(device_id, tcont_id, &tcont_index);
        if (NULL == tcont_cb) {
            goto FUNC_EXIT;
        }

        if (tcont_index == 0) {
            ca_printf("Remove TCONT 0 is not support, must leave for OMCC.\r\n");
            goto FUNC_EXIT;
        }

        ca_mdelay(1); /* wait for USBG burst finish */
        ret = aal_xgpon_tcont_flush(device_id, uwlch_id, tcont_index);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

        mask.u32 = 0;
        mask.s.alloc_id =1;
        mask.s.omci_en =1;
        mask.s.ploam_en =1;
        alloc_id_cfg.alloc_id = 0x3aad;
        alloc_id_cfg.omci_en = 0;
        alloc_id_cfg.ploam_en = 0;
        ret = aal_xgpon_bwmp_tcont_set(device_id, uwlch_id, tcont_index, mask, &alloc_id_cfg);
        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

        __gpon_intl_delete_tcont_cb(device_id, port, tcont_index);// ldchen: remove tcont_id);

    } else { /* create TCONT */

        ret = __gpon_intl_add_tcont_cb (device_id, port, tcont_id, alloc_id, scheduler);

        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);

        CA_GPON_DBG("gpon_cb[device_id].tcont_cb[%d] = 0x%x: \r\n",tcont_index,gpon_cb[device_id].tcont_cb[tcont_index].alloc_id);
        mask.u32 = 0;
        mask.s.alloc_id =1;
        mask.s.omci_en =1;
        mask.s.ploam_en =1;
        alloc_id_cfg.alloc_id = alloc_id;
        alloc_id_cfg.omci_en = 1;
        alloc_id_cfg.ploam_en = 1;
        ret = aal_xgpon_bwmp_tcont_set(device_id, uwlch_id, tcont_index, mask, &alloc_id_cfg);

        CA_ERR_GOTO_FUNC_EXIT(device_id, ret);
    }

    ca_printf("Set %s TCONT %u (Internal TCONT Idx %u) Alloc-ID %u \r\n",
        t_enable ? "Enable" : "Disable",
        tcont_id, tcont_index, alloc_id);

FUNC_EXIT:
    //PAL_MUTEX_GIVE(gpon_cb[device_id].sem);
    return CA_E_OK;

}

ca_status_t ca_gpon_port_tcont_get(
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t port_id,
    CA_IN ca_uint32_t  alloc_id,
    CA_OUT ca_uint32_t *tcont_id,
    CA_OUT ca_boolean_t *t_enable,
    CA_OUT ca_gpon_port_tcont_scheduler_t *scheduler
)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    ca_uint32_t tcont_index = 0;
    ca_uint32_t queue_idx = 0;
    ca_gpon_tcont_cb_t *p_tcont_cb = NULL;

    if (alloc_id >0x3FFF) {
        ca_printf("alloc_id is :%d\r\n",alloc_id);
        return CA_E_PARAM;
    }

    p_tcont_cb = __gpon_intl_get_tcont_id_by_alloc_id  (device_id, alloc_id, tcont_id, &tcont_index);
    if (NULL == p_tcont_cb ) {
        *tcont_id = CA_GPON_INVALID_VALUE;
        scheduler->mode = CA_GPON_INVALID_VALUE;
        scheduler->sp_queue_num = CA_GPON_INVALID_VALUE;
        goto FUNC_EXIT;
    }
	*tcont_id = p_tcont_cb->tcont_id;
    *t_enable = p_tcont_cb->valid;
    scheduler->mode = p_tcont_cb->sched_mode;
    scheduler->sp_queue_num = p_tcont_cb->num_sp_queues;
    for(queue_idx = 0; queue_idx<queue_num_per_tcont; queue_idx ++){
        scheduler->weight_queue[queue_idx] = p_tcont_cb->queue_cb[queue_idx].queue_weight;
    }
    //XGPN_MAC_DEBUG("scheduler->mode = %d, scheduler->sp_queue_num =%d and tcontid= %d \r\n",scheduler->mode,scheduler->sp_queue_num,tcont_id);

FUNC_EXIT:

    return ret;
}
ca_status_t ca_gpon_port_dying_gasp_set (
    CA_IN ca_device_id_t device_id,
    CA_IN ca_port_id_t   port_id)
{
    ca_status_t ret = CA_E_OK;
    ca_uint32_t port = PORT_ID(port_id); 
    aal_xgpon_usbg_xgtc_hd_ind_config_msk_t mask;
    aal_xgpon_usbg_xgtc_hd_ind_config_t hd_ind_cfg;
    mask.s.dg = 1;
    hd_ind_cfg.dg = 1;
    ret = aal_xgpon_usbg_xgtc_hd_ind_cfg_set(device_id,mask,&hd_ind_cfg);
    CA_RET_VALUE_CHECK(ret);
    return ret;
}

ca_status_t ca_gpon_port_laser_polarity_set (
    CA_IN        ca_device_id_t                device_id,
    CA_IN        ca_port_id_t                    port_id,
    CA_IN        ca_gpon_port_laser_polarity_t    tx_polarity,
    CA_IN        ca_gpon_port_laser_polarity_t    rx_polarity)
{
    ca_status_t ret = CA_E_OK;
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
    ca_uint32_t port = PORT_ID(port_id); 
    aal_psds_rg24_mask_t  rg24_mask;
    aal_psds_rg24_t      rg24;
    CA_DEV_ID_CHECK(device_id);
    rg24_mask.u32 = 0;
    memset(&rg24, 0, sizeof(aal_psds_rg24_t));
    rg24_mask.s.reg_ben_por_sel = 1;
    rg24.reg_ben_por_sel |= (tx_polarity << 1);
    ret = aal_psds_rg24_set(device_id,rg24_mask,&rg24);
    CA_RET_VALUE_CHECK(ret);
#endif
    return ret;
}

ca_status_t ca_gpon_port_laser_polarity_get (
    CA_IN        ca_device_id_t                device_id,
    CA_IN        ca_port_id_t                    port_id,
    CA_OUT        ca_gpon_port_laser_polarity_t    *tx_polarity,
    CA_OUT        ca_gpon_port_laser_polarity_t    *rx_polarity)
{
    ca_status_t ret = CA_E_OK;
#if !defined(CONFIG_CORTINA_BOARD_FPGA)
	ca_uint32_t port = PORT_ID(port_id); 
    aal_psds_rg24_t      rg24;
    CA_DEV_ID_CHECK(device_id);
    CA_NULL_PTR_CHECK(tx_polarity);
    CA_NULL_PTR_CHECK(rx_polarity);
    memset(&rg24, 0, sizeof(aal_psds_rg24_t));
    ret = aal_psds_rg24_get(device_id, &rg24);
    CA_RET_VALUE_CHECK(ret);
    *tx_polarity = (rg24.reg_ben_por_sel >> 1);
    //*rx_polarity = ;
#endif
    return ret;
}
ca_status_t ca_gpon_port_init(
    CA_IN ca_device_id_t device_id)
{
    ca_printf("this API %s was obsolte!\r\n", __func__);
    //return aal_xgpon_mac_reset(device_id, AAL_XGPON_RESET_FLAG_ALL) ;
    return CA_E_OK;
}

ca_status_t ca_gpon_port_pon_tag_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t pon_tag_len,
	CA_OUT ca_uint8_t *pon_tag)
{
	if(pon_tag_len != PON_TAG_SIZE)
		return CA_E_PARAM;
	memcpy(pon_tag, g_pon_tag,pon_tag_len);
	return CA_E_OK;
}

ca_status_t ca_gpon_port_msk_update(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t *msk)
{
	ca_status_t ret = CA_E_OK;
	aal_xgpon_aes_plmp_gbl_ctrl_t aes_plmp_gbl_ctrl;
	aal_xgpon_aes_plmp_gbl_ctrl_msk_t mask;

    CA_NULL_PTR_CHECK(msk);
    CHK_GPON_CB_READY(device_id);

	memset(&aes_plmp_gbl_ctrl, 0, sizeof(aes_plmp_gbl_ctrl));
	mask.u32 = 0;

	ret = aal_xgpon_aes_plmp_glb_ctrl_get(device_id,&aes_plmp_gbl_ctrl);
	CA_RET_VALUE_CHECK(ret);
	if(aes_plmp_gbl_ctrl.sma_mode == 0)
	{
		mask.s.sma_mode = 1 ;
		aes_plmp_gbl_ctrl.sma_mode = 1;
		ret = aal_xgpon_aes_plmp_glb_ctrl_set(device_id, mask, &aes_plmp_gbl_ctrl);
		CA_RET_VALUE_CHECK(ret);
	}
	
	ret = aal_xgpon_aes_plmp_msk_set(device_id,msk);
	CA_RET_VALUE_CHECK(ret);
	
#if defined(CONFIG_LUNA_G3_SERIES)
	ret = xgpn_aes_key_generate_by_msk(device_id, msk);
	CA_RET_VALUE_CHECK(ret);
#endif
	
	return ret;
}

ca_status_t ca_gpon_port_msk_update_status_get (
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_boolean_t *f_updated)
{
	ca_status_t ret = CA_E_OK;
	aal_ploam_rx_mib_t rx_mib;
	memset(&rx_mib, 0, sizeof(aal_ploam_rx_mib_t));
	ret = aal_ploam_rx_mib_get(device_id,&rx_mib);
	CA_RET_VALUE_CHECK(ret);
	
	return CA_E_OK;
}


ca_status_t ca_gpon_port_msk_get(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t key_len,
	CA_OUT ca_uint8_t *key)
{
	ca_status_t ret = CA_E_OK;
	if (key_len != 16)
	{
		return CA_E_PARAM;
	}
	ret = aal_xgpon_aes_plmp_msk_get(device_id,key);
	CA_RET_VALUE_CHECK(ret);
	return ret;
}

ca_status_t ca_gpon_port_new_omci_msk_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_gpon_port_new_omci_msk_t new_msk)
{
	omci_msk_flag = new_msk.omci_auth_completed_flag;
	memcpy(omci_msk, new_msk.new_omci_msk, CA_MSK_SIZE);
	return CA_E_OK;
}

ca_status_t ca_gpon_port_new_omci_msk_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_gpon_port_new_omci_msk_t *new_msk)
{
	new_msk->omci_auth_completed_flag = omci_msk_flag;
	memcpy(new_msk->new_omci_msk, omci_msk, CA_MSK_SIZE);
	return CA_E_OK;
}

ca_status_t ca_gpon_port_bwmap_get(
	CA_IN		ca_device_id_t	device_id, 
	CA_IN		ca_port_id_t	port_id, 
	CA_OUT		ca_bwmap_t	*bwmap)
{
	ca_status_t ret = CA_E_OK;
	return ret;
}

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_gpon_serial_number_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t serial_number[GPON_ONU_SN_LEN])
{
	ca_status_t ret = CA_E_OK;

    /*the admin_state MUST be false before setting SN*/
    if(g_admin_state)
    {
        return CA_E_ERROR;
    }

    if(aal_pon_mac_mode_get(device_id) == ONU_PON_MAC_GPON)
    {
        g_xgpn_basic_info.vendor_id = 
            ((ca_uint32_t)serial_number[0] << 24) | 
            ((ca_uint32_t)serial_number[1] << 16) | 
            ((ca_uint32_t)serial_number[2] << 8) | 
            ((ca_uint32_t)serial_number[3]);

        g_xgpn_basic_info.vssn = 
            ((ca_uint32_t)serial_number[4] << 24) | 
            ((ca_uint32_t)serial_number[5] << 16) | 
            ((ca_uint32_t)serial_number[6] << 8) | 
            ((ca_uint32_t)serial_number[7]);

        ret = aal_gpon_vendor_id_set(device_id, g_xgpn_basic_info.vendor_id);
        CA_RET_VALUE_CHECK(ret);

        ret = aal_gpon_vssn_set(device_id, g_xgpn_basic_info.vssn);
        CA_RET_VALUE_CHECK(ret);
    }
    else
    {
        g_xgpn_basic_info.vendor_id = 
            ((ca_uint32_t)serial_number[0] << 24) | 
            ((ca_uint32_t)serial_number[1] << 16) | 
            ((ca_uint32_t)serial_number[2] << 8) | 
            ((ca_uint32_t)serial_number[3]);

        g_xgpn_basic_info.vssn = 
            ((ca_uint32_t)serial_number[4] << 24) | 
            ((ca_uint32_t)serial_number[5] << 16) | 
            ((ca_uint32_t)serial_number[6] << 8) | 
            ((ca_uint32_t)serial_number[7]);

        ret = aal_ploam_vendor_id_cfg_set(device_id, g_xgpn_basic_info.vendor_id);
        CA_RET_VALUE_CHECK(ret);

        ret = aal_ploam_vssn_cfg_set(device_id, g_xgpn_basic_info.vssn);
        CA_RET_VALUE_CHECK(ret);
    }
    return ret;
}

ca_status_t ca_gpon_registration_id_set(
	CA_IN ca_device_id_t device_id,
	CA_IN ca_uint8_t registration_id[GPON_RGSD_ID_LEN])
{
    ca_status_t ret = CA_E_OK;
    aal_gpon_passwd_t passwd = {0};

    /*the admin_state MUST be false before setting registration ID*/
    if(g_admin_state)
    {
        return CA_E_ERROR;
    }

    if(aal_pon_mac_mode_get(device_id) == ONU_PON_MAC_GPON)
    {
        memcpy(g_gpon_basic_info.passwd, registration_id, AAL_GPON_PORT_PASSWORD_LEN);
        memcpy(passwd.passwd, g_gpon_basic_info.passwd, AAL_GPON_PORT_PASSWORD_LEN);
    
        ret = aal_gpon_passwd_set(device_id, &passwd);
        CA_RET_VALUE_CHECK(ret);
    }
    else
    {
        memcpy(g_xgpn_basic_info.registration_id, registration_id, GPON_RGSD_ID_LEN);
    
        ret = aal_ploam_rgst_id_cfg_set(device_id, (aal_xgpn_plmp_rgst_id_cfg_t *)g_xgpn_basic_info.registration_id);
        CA_RET_VALUE_CHECK(ret);
    }

    return ret;
}


#endif

EXPORT_SYMBOL(ca_gpon_cb_reset);
EXPORT_SYMBOL(ca_gpon_api_test);
EXPORT_SYMBOL(ca_gpon_api_test1);
EXPORT_SYMBOL(ca_gpon_cb_dump);
EXPORT_SYMBOL(ca_gpon_cb_init);
EXPORT_SYMBOL(ca_gpon_onu_init);
EXPORT_SYMBOL(ca_gpon_onu_set);
EXPORT_SYMBOL(ca_gpon_onu_get);
EXPORT_SYMBOL(ca_gpon_port_sf_threshold_set);
EXPORT_SYMBOL(ca_gpon_port_sf_threshold_get);
EXPORT_SYMBOL(ca_gpon_port_sd_threshold_set);
EXPORT_SYMBOL(ca_gpon_port_sd_threshold_get);
EXPORT_SYMBOL(ca_gpon_port_timer_set);
EXPORT_SYMBOL(ca_gpon_port_timer_get);
EXPORT_SYMBOL(ca_gpon_port_laser_mode_set);
EXPORT_SYMBOL(ca_gpon_port_laser_mode_get);
EXPORT_SYMBOL(ca_gpon_port_sync_threshold_set);
EXPORT_SYMBOL(ca_gpon_port_sync_threshold_get);
EXPORT_SYMBOL(ca_gpon_port_aes_enable_set);
EXPORT_SYMBOL(ca_gpon_port_aes_enable_get);
EXPORT_SYMBOL(ca_gpon_port_aes_key_set);
EXPORT_SYMBOL(ca_gpon_port_aes_key_get);
EXPORT_SYMBOL(ca_gpon_port_fec_enable_set);
EXPORT_SYMBOL(ca_gpon_port_fec_enable_get);
EXPORT_SYMBOL(ca_gpon_port_fec_stats_get);
EXPORT_SYMBOL(ca_gpon_port_onu_state_get);
EXPORT_SYMBOL(ca_gpon_port_onu_id_get);
EXPORT_SYMBOL(ca_gpon_port_onu_eqd_get);
EXPORT_SYMBOL(ca_gpon_port_capability_get);
EXPORT_SYMBOL(ca_gpon_port_gem_port_add);
EXPORT_SYMBOL(ca_gpon_port_gem_port_delete);
EXPORT_SYMBOL(ca_gpon_port_gem_port_get);
EXPORT_SYMBOL(ca_gpon_port_gem_port_set);
EXPORT_SYMBOL(ca_gpon_port_omci_port_id_get);
EXPORT_SYMBOL(ca_gpon_port_gem_stats_get);
EXPORT_SYMBOL(ca_gpon_port_upstream_queue_set);
EXPORT_SYMBOL(ca_gpon_port_upstream_queue_get);
EXPORT_SYMBOL(ca_gpon_port_upstream_scheduler_set);
EXPORT_SYMBOL(ca_gpon_port_upstream_scheduler_get);
EXPORT_SYMBOL(ca_gpon_port_tcont_set);
EXPORT_SYMBOL(ca_gpon_port_tcont_get);
EXPORT_SYMBOL(ca_gpon_port_dying_gasp_set);
EXPORT_SYMBOL(ca_gpon_port_laser_polarity_get);
EXPORT_SYMBOL(ca_gpon_port_laser_polarity_set);
EXPORT_SYMBOL(ca_gpon_port_init);
EXPORT_SYMBOL(ca_gpon_port_pon_tag_get);
EXPORT_SYMBOL(ca_gpon_port_msk_update);
EXPORT_SYMBOL(ca_gpon_port_msk_update_status_get);
EXPORT_SYMBOL(ca_gpon_port_new_omci_msk_set);
EXPORT_SYMBOL(ca_gpon_port_new_omci_msk_get);
EXPORT_SYMBOL(ca_gpon_port_bwmap_get);

#if defined(CONFIG_LUNA_G3_SERIES)
EXPORT_SYMBOL(ca_gpon_serial_number_set);
EXPORT_SYMBOL(ca_gpon_registration_id_set);
#endif
