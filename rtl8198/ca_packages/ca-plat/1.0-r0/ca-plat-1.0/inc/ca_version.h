#ifndef _CA_IROS_VERSION_H_
#define _CA_IROS_VERSION_H_

#include "ca_types.h"

/*It will be change for each board and each release */
#ifdef CONFIG_ARCH_CORTINA_SATURN_SFP_PLUS
#define IROS_VER_MAJOR                       11
#define IROS_VER_MINOR                       0 
#define IROS_VER_REVISION                  2
#define IROS_VER_BUILD_ID                  1
#define IROS_VER_INTERNAL_ID            0
#elif defined(CONFIG_ARCH_CORTINA_SATURN_SFU) 
#define IROS_VER_MAJOR                       21
#define IROS_VER_MINOR                       0 
#define IROS_VER_REVISION                  2
#define IROS_VER_BUILD_ID                  1
#define IROS_VER_INTERNAL_ID            0
#elif defined(CONFIG_ARCH_CORTINA_G3HGU)  
#define IROS_VER_MAJOR                       30
#define IROS_VER_MINOR                       7 
#define IROS_VER_REVISION                  28
#define IROS_VER_BUILD_ID                  0
#define IROS_VER_INTERNAL_ID            0
#elif defined(CONFIG_ARCH_CORTINA_VENUS)  
#define IROS_VER_MAJOR                       40
#define IROS_VER_MINOR                       0 
#define IROS_VER_REVISION                  0
#define IROS_VER_BUILD_ID                  0
#define IROS_VER_INTERNAL_ID            0
#else
#define IROS_VER_MAJOR                       0
#define IROS_VER_MINOR                       0 
#define IROS_VER_REVISION                  0
#define IROS_VER_BUILD_ID                  0
#define IROS_VER_INTERNAL_ID            0
#endif

#ifndef __KERNEL__
#define  ENV_SIZE           256
#define  ENV_EXEC         "img_executed"
#define  ENV_ACTIVE      "img_active"
#define  ENV_COMMIT     "img_commit"
#define  ENV_SB_FLAG       "sb_flag"
#define  ENV_VERSION0     "img_version0"
#define  ENV_VERSION1     "img_version1"
#define  ENV_INFO0     "img_info0"
#define  ENV_INFO1     "img_info1"
#define  ENV_INFO_LEN     32
#define  ENV_VER_LEN     14


typedef struct {/*CODEGEN_IGNORE_TAG*/
    /* Executed flag */
    unsigned char exec_flag;
    /* Valid  flag */
    unsigned char valid_flag;
    /* Active flag */
    unsigned char active_flag;
    /* Commit flag */
    unsigned char commit_flag;
    unsigned char version[64];
    unsigned char info[64];
} ca_img_info_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;
} ca_cert_date_t;

typedef struct {/*CODEGEN_IGNORE_TAG*/
    ca_uint8_t org_name[256];
    ca_cert_date_t date[2];
} ca_cert_info_t;

typedef enum
{
    CA_CERT_CVC_BEFORE_DATE        = 0,
    CA_CERT_CODE_BEFORE_DATE     = 1,
    CA_CERT_M_ORG_NAME                = 2,
    CA_CERT_MAX
} CA_CERTIFICATION_TYPE_E;

ca_status_t ca_img_env_load 
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id
    );

ca_status_t ca_loader_info_get
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id,  
    CA_OUT ca_uint8_t * version
    );

ca_status_t ca_img_executed_set 
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id, 
    CA_IN ca_uint8_t img_idx, 
    CA_IN ca_uint8_t flag
    );

ca_status_t ca_img_active_set 
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id, 
    CA_IN ca_uint8_t img_idx, 
    CA_IN ca_uint8_t flag
    );

ca_status_t ca_img_commit_set 
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id, 
    CA_IN ca_uint8_t img_idx, 
    CA_IN ca_uint8_t flag
    );

ca_status_t ca_img_info_get 
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id, 
    CA_IN ca_uint8_t img_idx,
    CA_OUT ca_img_info_t *  img_info
    );

ca_status_t ca_img_ver_set
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id, 
    CA_IN ca_uint8_t img_idx,
    CA_OUT ca_uint8_t* ver_str
    );

ca_status_t ca_img_info_set
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN ca_device_id_t device_id, 
    CA_IN ca_uint8_t img_idx,
    CA_OUT ca_uint8_t* info_str
    );

ca_status_t ca_cert_info_init
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t device_id
    );

ca_status_t ca_cert_info_set
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t device_id, 
    CA_IN  ca_uint32_t cert_type, 
    CA_IN  ca_cert_info_t * cert_data
    );

ca_status_t ca_cert_info_get
    (/*CODEGEN_IGNORE_TAG*/
    CA_IN  ca_device_id_t device_id,  
    CA_OUT ca_cert_info_t * cert_data
    );
#endif


#endif

