#include "osal_common.h"
#include "onu.h"
#if defined(CONFIG_LUNA_G3_SERIES)
#include "aal_pon.h"

#include <soc/cortina/ca8279_pon_registers.h>
#endif
ca_status_t ca_onu_info_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_onu_common_info_t *pData
)
{
	ca_status_t ret = CA_E_OK;

	return ret;
}

#if defined(CONFIG_LUNA_G3_SERIES)
ca_status_t ca_onu_losState_get(
	CA_IN ca_device_id_t device_id,
	CA_OUT ca_uint32_t *pData
)
{
	ca_status_t ret = CA_E_OK;

    CA_NULL_PTR_CHECK(pData);

    *pData = AAL_GLB_REG_READ(GLOBAL_OPT_MODULE_STATUS);

	return ret;
}

EXPORT_SYMBOL(ca_onu_losState_get);
#endif

EXPORT_SYMBOL(ca_onu_info_get);

