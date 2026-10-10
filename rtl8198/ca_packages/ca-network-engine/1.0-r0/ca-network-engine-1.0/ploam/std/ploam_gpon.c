
#include "ca_types.h"
#include "osal_common.h"

#include "ploam_main.h"
#include "ploam_cmn.h"
#include "ploam_pkt.h"

ca_status_t ploam_pkt_gpon_proc(ca_uint8 *frame_ptr, ca_uint32_t len)
{

    PLOAM_ASSERT_RET(frame_ptr, CA_E_PARAM);
    ploam_dump_pkt("gpon ploam", frame_ptr, len);


    return CA_E_OK;
}


