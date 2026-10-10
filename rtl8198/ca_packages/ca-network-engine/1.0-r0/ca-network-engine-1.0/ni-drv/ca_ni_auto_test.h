#include "aal_common.h"
#include "ca_ni_dump.h"

#define AUTO_TOOL_DEBUG(arg,...) if (ca_ni_debug & NI_DBG_AUTO_TOOL) ca_printf("%s:%d "arg, __FUNCTION__,__LINE__,##__VA_ARGS__)

ca_status_t ca_ni_tx_auto_test(
        HEADER_A_T *hdr_a,
        HEADER_CPU_T *hdr_cpu,
        ca_uint8_t *data,
        ca_uint32_t pkt_size
);
