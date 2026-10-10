#ifndef __PLOAM_PKT_H__
#define __PLOAM_PKT_H__

#include "ploam_cmn.h"
#include "ploam_main.h"

typedef enum {
    PLOAM_DUMP_TYPE_NONE,
    PLOAM_DUMP_TYPE_ALL
} ploam_dump_pkt_type_e;


#define PLOAM_MSG_NAME_SIZE       16

typedef struct
{
  ca_int8_t   name[PLOAM_MSG_NAME_SIZE];
  ca_uint32_t msg_id;
  ca_status_t  (*func)(void* data_p);
  void*       value_p;
} ploam_msg_proc_t;

void ploam_dump_octet(char *comment,
                  ca_uint8_t *buffer, ca_uint32_t len);

void ploam_dump_pkt(char *comment,
                  ca_uint8_t *buffer, ca_uint32_t len);

void ploam_set_pkt_dump(ca_uint32_t type);
void aes_ploam_dump_pkt(char *comment,
                  ca_uint8_t *buffer, ca_uint32_t len);

void aes_ploam_set_pkt_dump(ca_uint32_t type);


#endif /* __PLOAM_PKT_H__ */
