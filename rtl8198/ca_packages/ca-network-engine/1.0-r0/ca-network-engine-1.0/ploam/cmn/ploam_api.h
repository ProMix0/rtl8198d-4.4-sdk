#ifndef __PLOAM_API_H__
#define __PLOAM_API_H__

#include "ca_types.h"
#include "rx.h"

/*
 * init ploam resource
 */
//ca_status_t ca_ploam_init(ca_device_id_t device_id);

#if 0
ca_status_t ca_ploam_wq_add(
    ca_uint8_t       *pkt,
    ca_uint32_t      len);
#endif

ca_status_t ca_ploam_tx_register(
    ca_uint32_t (*handler)(ca_device_id_t device_id, void *pkt));

ca_status_t ca_ploam_tx_unregister(void);

//void  ca_ploam_wq_interrupt(ca_uint32_t  info);
//void  ploam_rxbuf_proc(void *data);

#endif
