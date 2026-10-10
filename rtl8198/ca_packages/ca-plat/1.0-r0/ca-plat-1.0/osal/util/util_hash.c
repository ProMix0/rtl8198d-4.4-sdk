/****************************************************************************
            Software License for Customer Use of Cortina Software
                          Grant Terms and Conditions

IMPORTANT NOTICE - READ CAREFULLY: This Software License for Customer Use
of Cortina Software ("LICENSE") is the agreement which governs use of
software of Cortina Systems, Inc. and its subsidiaries ("CORTINA"),
including computer software (source code and object code) and associated
printed materials ("SOFTWARE").  The SOFTWARE is protected by copyright laws
and international copyright treaties, as well as other intellectual property
laws and treaties.  The SOFTWARE is not sold, and instead is only licensed
for use, strictly in accordance with this document.  Any hardware sold by
CORTINA is protected by various patents, and is sold but this LICENSE does
not cover that sale, since it may not necessarily be sold as a package with
the SOFTWARE.  This LICENSE sets forth the terms and conditions of the
SOFTWARE LICENSE only.  By downloading, installing, copying, or otherwise
using the SOFTWARE, you agree to be bound by the terms of this LICENSE.
If you do not agree to the terms of this LICENSE, then do not download the
SOFTWARE.

DEFINITIONS:  "DEVICE" means the Cortina Systems?IROS SDK product.
"You" or "CUSTOMER" means the entity or individual that uses the SOFTWARE.
"SOFTWARE" means the Cortina Systems?SDK software.

GRANT OF LICENSE:  Subject to the restrictions below, CORTINA hereby grants
CUSTOMER a non-exclusive, non-assignable, non-transferable, royalty-free,
perpetual copyright license to (1) install and use the SOFTWARE for
reference only with the DEVICE; and (2) copy the SOFTWARE for your internal
use only for use with the DEVICE.

RESTRICTIONS:  The SOFTWARE must be used solely in conjunction with the
DEVICE and solely with Your own products that incorporate the DEVICE.  You
may not distribute the SOFTWARE to any third party.  You may not modify
the SOFTWARE or make derivatives of the SOFTWARE without assigning any and
all rights in such modifications and derivatives to CORTINA.  You shall not
through incorporation, modification or distribution of the SOFTWARE cause
it to become subject to any open source licenses.  You may not
reverse-assemble, reverse-compile, or otherwise reverse-engineer any
SOFTWARE provided in binary or machine readable form.  You may not
distribute the SOFTWARE to your customers without written permission
from CORTINA.

OWNERSHIP OF SOFTWARE AND COPYRIGHTS. All title and copyrights in and the
SOFTWARE and any accompanying printed materials, and copies of the SOFTWARE,
are owned by CORTINA. The SOFTWARE protected by the copyright laws of the
United States and other countries, and international treaty provisions.
You may not remove any copyright notices from the SOFTWARE.  Except as
otherwise expressly provided, CORTINA grants no express or implied right
under CORTINA patents, copyrights, trademarks, or other intellectual
property rights.

DISCLAIMER OF WARRANTIES. THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY
EXPRESS OR IMPLIED WARRANTY OF ANY KIND, INCLUDING ANY IMPLIED WARRANTIES
OF MERCHANTABILITY, NONINFRINGEMENT, OR FITNESS FOR A PARTICULAR PURPOSE,
TITLE, AND NON-INFRINGEMENT.  CORTINA does not warrant or assume
responsibility for the accuracy or completeness of any information, text,
graphics, links or other items contained within the SOFTWARE.  Without
limiting the foregoing, you are solely responsible for determining and
verifying that the SOFTWARE that you obtain and install is the appropriate
version for your purpose.

LIMITATION OF LIABILITY. IN NO EVENT SHALL CORTINA OR ITS SUPPLIERS BE
LIABLE FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, LOST
PROFITS, BUSINESS INTERRUPTION, OR LOST INFORMATION) OR ANY LOSS ARISING OUT
OF THE USE OF OR INABILITY TO USE OF OR INABILITY TO USE THE SOFTWARE, EVEN
IF CORTINA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
TERMINATION OF THIS LICENSE. This LICENSE will automatically terminate if
You fail to comply with any of the terms and conditions hereof. Upon
termination, You will immediately cease use of the SOFTWARE and destroy all
copies of the SOFTWARE or return all copies of the SOFTWARE in your control
to CORTINA.  IF you commence or participate in any legal proceeding against
CORTINA, then CORTINA may, in its sole discretion, suspend or terminate all
license grants and any other rights provided under this LICENSE during the
pendency of such legal proceedings.
APPLICABLE LAWS. Claims arising under this LICENSE shall be governed by the
laws of the State of California, excluding its principles of conflict of
laws.  The United Nations Convention on Contracts for the International Sale
of Goods is specifically disclaimed.  You shall not export the SOFTWARE
without first obtaining any required export license or other approval from
the applicable governmental entity, if required.  This is the entire
agreement and understanding between You and CORTINA relating to this subject
matter.
GOVERNMENT RESTRICTED RIGHTS. The SOFTWARE is provided with "RESTRICTED
RIGHTS." Use, duplication, or disclosure by the Government is subject to
restrictions as set forth in FAR52.227-14 and DFAR252.227-7013 et seq. or
its successor. Use of the SOFTWARE by the Government constitutes
acknowledgment of CORTINA's proprietary rights therein. Contractor or
Manufacturer is CORTINA.

Copyright (c) 2009 by Cortina Systems Incorporated
****************************************************************************/

/************************************************************************
Example:

-Data structure define:
  typedef struct
  {
        ca_hash_node  node;    //the declaration must be put on the header of your structure
        ca_int32  data[20];
        ca_int32  key;
  }tTest;

#define YOUR_HASH_SIZE 128

ca_uint32 hashkey_generate(void *pNode)
{
    tTest *pTest = (tTest *)pNode;

    return (pTest->key%YOUR_HASH_SIZE);
}

ca_int32 your_compare_func(void *pNode , ca_uint key)
{
    tTest *pTest = (tTest *)pNode;

    if(pTest->key > key)
        return 1;
    else if(pTest->key < key)
        return -1;
    else
        return 0;
}

-- Create hash table
    ca_hash_tbl *your_hash_tbl = NULL;
    your_hash_tbl = ca_hash_create(YOUR_HASH_SIZE , hashkey_generate , your_compare_func);
    if(yor_hash_tbl == NULL)
        return ERROR;

-- Add a record to hash table
    tTest *add_node = NULL;
    add_node = (tTest *)ca_malloc(sizeof(tTest));
    if(add_node == NULL)
        return ERROR;
    add_node->key = 123;
    ca_hash_add(your_hash_tbl , (void *)add_node);

-- Find a record from hash table
-- Del a record from hash table
      tTest *pDelNode=NULL;
      pDelNode = ca_hash_find(your_hash_tbl , 123);
      if(pDelNode != NULL) {
        ca_hash_del(your_hash_tbl , (void *)pDelNode);
      }

-- scan hash table
    Method-1:(use 'ca_hash_first' and 'ca_hash_next')
        tTest *prev=NULL,*next=NULL;
        ca_uint32 prev_buck=0,next_buck=0;
        int ret = CA_HASH_ERROR;
        ret = ca_hash_first(your_hash_tbl , (void *)&prev , &prev_buck);
        while(ret == CA_HASH_OK)
        {
            next = NULL;
            next_buck = 0;
            ... //do what you want to do
            ret = ca_hash_next(your_hash_tbl , prev , prev_buck , (void *)&next , &next_buck);
            prev = next;
            prev_buck = next_buck;
        }

    Method-2:(use 'ca_scan_hash_bucket' and 'ca_scan_hash_record')(recommend)
        ca_int32 hash_index;
        tTest *pNode=NULL;
        ca_scan_hash_bucket(your_hash_tbl , hash_index)
            ca_scan_hash_record(&your_hash_tbl->hash_list[hash_index] , pNode , tTest *)
        {
            ...  //do what you want to do
        }

-- flush all records in hash table
        tTest *pNode=NULL;
        while((pNode=(tTest *)ca_hash_get(your_hash_tbl)) != NULL)
        {
            ... //call your own free function to free the node
        }

************************************************************************/
#include "ca_types.h"
#include "os_core.h"
#include "osal_mutex.h"
#include "util_list.h"
#include "util_hash.h"
#include "osal_cmn.h"
#include "osal_mem.h"

#if 1
#define HASH_TBL_LOCK(p) do{if(p) ca_mutex_lock(p->hash_mutex_id);} while(0)
#define HASH_TBL_UNLOCK(p) do{if(p) ca_mutex_unlock(p->hash_mutex_id);} while(0)

ca_hash_tbl *ca_hash_create
    (
    ca_uint32_t hash_size,
    ca_uint32_t (*hashkey_gen)(void *pNode),
    ca_int32_t (*compare)(void *pNode , ca_uint_t key)
    )
{
    ca_hash_tbl *pHashTbl = (ca_hash_tbl *)0;
    ca_int8_t hash_name[20];
    ca_uint32_t i;

    if (hashkey_gen == NULL || compare == NULL || hash_size == 0) {
        return (ca_hash_tbl *)NULL;
    }

    pHashTbl = (ca_hash_tbl *)ca_malloc((ca_int32_t)(sizeof(ca_hash_tbl) + (hash_size - 1) * sizeof(ca_list)));
    if (pHashTbl == NULL) {
        return NULL;
    }
    memset(pHashTbl , 0 , sizeof(ca_hash_tbl));

    for (i = 0 ; i < hash_size ; i++) {
        ca_lst_init(&pHashTbl->hash_list[i]);
    }

    memset(hash_name , 0 , sizeof(hash_name));
    sprintf((char*)hash_name , "H%p" , pHashTbl);
    if (CA_OSAL_OK != ca_mutex_init(&pHashTbl->hash_mutex_id, hash_name , 0)) {
        ca_free(pHashTbl);
        return NULL;
    }

    pHashTbl->hashkey_gen = hashkey_gen;
    pHashTbl->compare = compare;
    pHashTbl->hash_size = hash_size;

    return (ca_hash_tbl *)pHashTbl;
}

ca_int32_t ca_hash_destroy
    (
    ca_hash_tbl *pHashTbl
    )
{
    if(pHashTbl == NULL)
        return CA_OSAL_ERROR;

    ca_mutex_destroy(pHashTbl->hash_mutex_id);

    ca_free(pHashTbl);

    return CA_OSAL_OK;
}


ca_int32_t ca_hash_add(ca_hash_tbl * pHashTbl , void *pNode)
{
    ca_uint32_t hash_key;

    if (pHashTbl == NULL || pNode == NULL) {
        return CA_HASH_ERROR;
    }

    hash_key = pHashTbl->hashkey_gen((void *)pNode);
    if (hash_key >= pHashTbl->hash_size) {
        return CA_HASH_ERROR;
    }

    HASH_TBL_LOCK(pHashTbl);
    ca_lst_add(&pHashTbl->hash_list[hash_key] , (ca_node *)pNode);
    pHashTbl->count++;
    HASH_TBL_UNLOCK(pHashTbl);

    return CA_HASH_OK;
}

ca_int32_t ca_hash_del(ca_hash_tbl * pHashTbl , void *pNode)
{
    ca_uint32_t hash_key;

    if (pHashTbl == NULL || pNode == NULL) {
        return CA_HASH_ERROR;
    }

    hash_key = pHashTbl->hashkey_gen((void *)pNode);
    if (hash_key >= pHashTbl->hash_size) {
        return CA_HASH_ERROR;
    }


    HASH_TBL_LOCK(pHashTbl);
    ca_lst_delete(&pHashTbl->hash_list[hash_key] , (ca_node *)pNode);
    pHashTbl->count--;
    HASH_TBL_UNLOCK(pHashTbl);

    return CA_HASH_OK;
}

ca_int32_t ca_hash_first
    (
    ca_hash_tbl * pHashTbl ,
    void **pFirst ,
    ca_uint32_t *bucket_idx
    )
{
    ca_uint32_t hash_key;

    if (pHashTbl == NULL) {
        return CA_HASH_ERROR;
    }

    HASH_TBL_LOCK(pHashTbl);
    ca_scan_hash_bucket(pHashTbl, hash_key) {
        if (ca_lst_count(&pHashTbl->hash_list[hash_key])) {
            *pFirst = (void *)ca_lst_first(&pHashTbl->hash_list[hash_key]);
            *bucket_idx = hash_key;
            HASH_TBL_UNLOCK(pHashTbl);
            return CA_HASH_OK;
        }
    }
    HASH_TBL_UNLOCK(pHashTbl);

    return CA_HASH_ERROR;
}

ca_int32_t ca_hash_next
    (
    ca_hash_tbl * pHashTbl ,
    void *pPrev,
    ca_uint32_t prev_bucket_idx,
    void **pNext ,
    ca_uint32_t *bucket_idx
    )
{
    ca_uint32_t hash_key;

    if (pHashTbl == NULL) {
        return CA_HASH_ERROR;
    }

    HASH_TBL_LOCK(pHashTbl);
    *pNext = (void *)ca_lst_next((ca_node *)pPrev);
    if (*pNext == NULL) {
        if (prev_bucket_idx >= pHashTbl->hash_size) {
            HASH_TBL_UNLOCK(pHashTbl);
            return CA_HASH_ERROR;
        } else {
            for (hash_key = prev_bucket_idx + 1 ; hash_key < pHashTbl->hash_size ; hash_key++) {
                *pNext = (void *)ca_lst_first(&pHashTbl->hash_list[hash_key]);
                if (*pNext != NULL) {
                    *bucket_idx = hash_key;
                    HASH_TBL_UNLOCK(pHashTbl);
                    return CA_HASH_OK;
                }
            }
        }
    } else {
        *bucket_idx = prev_bucket_idx;
        HASH_TBL_UNLOCK(pHashTbl);
        return CA_HASH_OK;
    }
    HASH_TBL_UNLOCK(pHashTbl);

    return CA_HASH_ERROR;
}

ca_hash_node *ca_hash_find
    (
    ca_hash_tbl * pHashTbl ,
    ca_uint_t key
    )
{
    ca_uint32_t hash_key;
    ca_hash_node *pNode = NULL;

    if(pHashTbl == NULL)
    {
        return NULL;
    }

    HASH_TBL_LOCK(pHashTbl);
    ca_scan_hash_bucket(pHashTbl, hash_key)
    ca_scan_hash_record(&pHashTbl->hash_list[hash_key], pNode, ca_hash_node *)
    {
        if(pHashTbl->compare((void *)pNode , key) == 0) {
            HASH_TBL_UNLOCK(pHashTbl);
            return pNode;
        }
    }
    HASH_TBL_UNLOCK(pHashTbl);

    return NULL;
}

ca_hash_node *ca_hash_get(ca_hash_tbl * pHashTbl)
{
    ca_uint32_t hash_key;
    ca_hash_node *pNode = NULL;

    if(pHashTbl == NULL) {
        return NULL;
    }

    HASH_TBL_LOCK(pHashTbl);
    ca_scan_hash_bucket(pHashTbl, hash_key)
    {
        pNode = (ca_hash_node *)ca_lst_get(&pHashTbl->hash_list[hash_key]);
        if(pNode == NULL) {
            continue;
        }

        pHashTbl->count--;
        HASH_TBL_UNLOCK(pHashTbl);
        return pNode;
    }

    HASH_TBL_UNLOCK(pHashTbl);
    return NULL;
}

ca_uint32_t ca_hash_count(ca_hash_tbl * pHashTbl)
{
    if(pHashTbl == NULL) {
        return 0;
    }

    return pHashTbl->count;
}
#endif
/*lint +e613*/

