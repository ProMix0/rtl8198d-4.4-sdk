/****************************************************************************
            Software License for Customer Use of Cortina Software
                          Grant Terms and Conditions

IMPORTANT NOTICE - READ CAREFULLY: This Software License for Customer Use
of Cortina Software ("LICENSE") is the agreement which governs use of
software of Cortina Access, Inc. and its subsidiaries ("CORTINA"),
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

DEFINITIONS:  "DEVICE" means the Cortina Access(TM) G3  SDK product.
"You" or "CUSTOMER" means the entity or individual that uses the SOFTWARE.
"SOFTWARE" means the Cortina Access(TM) SDK software.

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
Manufacturer is CORTINA Access.

Copyright (c) 2015 by Cortina Access Incorporated
****************************************************************************/
/*
 * util_list.c
 *
 * This file implements link list functions for all Cortina Access drivers
 */
#include "os_core.h"
#include "osal_mem.h"
#include "util_list.h"

ca_list_t *ca_list_create(ca_uint32 data_size,
			  ca_int32(*compare) (void *, void *))
{
	ca_list_t *pList;

	if (data_size == 0) {
		return NULL;
	}
	pList = (ca_list_t *) ca_malloc(sizeof(ca_list_t));
	if (pList == NULL) {
		return NULL;
	}
	pList->data_size = data_size;
	pList->count = 0;
	pList->node.data = NULL;
	pList->ca_head = NULL;
	pList->ca_tail = NULL;
	pList->compare = compare;

	return pList;
}

static void __destory_helper(ca_list_node_t *pNode)
{
	if (pNode != NULL) {
		__destory_helper(pNode->next);
		ca_free(pNode);
	}
	return;
}

void ca_list_destory(ca_list_t *pList)
{
	if (pList == NULL) {
		return;
	}
	__destory_helper(pList->node.next);
	ca_free(pList);
}

static void __node_insert(ca_list_t *pList, ca_list_node_t *pPrevious,
			  ca_list_node_t *pNode)
{
	ca_list_node_t *pNext = NULL;

	if (NULL == pPrevious) {
		pNext = pList->ca_head;
		pList->ca_head = pNode;
	} else {
		pNext = pPrevious->next;
		pPrevious->next = pNode;
	}

	if (NULL == pNext) {
		pList->ca_tail = pNode;
	} else {
		pNext->previous = pNode;
	}

	pNode->previous = pPrevious;
	pNode->next = pNext;

	pList->count++;
}

ca_int32 ca_list_add(ca_list_t *pList, void *data)
{
	ca_list_node_t *pNode = NULL;

	if (pList == NULL || data == NULL) {
		return -1;
	}
	/* alloc the memory for the data and node */
	pNode = ca_malloc(pList->data_size + sizeof(ca_list_node_t));
	if (pNode == NULL) {
		return -1;
	}
	pNode->data = (ca_uint8 *) pNode + sizeof(ca_list_node_t);
	memcpy(pNode->data, data, pList->data_size);

	__node_insert(pList, pList->ca_tail, pNode);
	return 0;
}

ca_int32 ca_list_delete(ca_list_t *pList, ca_list_node_t *pNode)
{
	if (pList == NULL || pNode == NULL) {
		return -1;
	}
	if (pNode->previous == NULL) {
		pList->ca_head = pNode->next;
	} else {
		pNode->previous->next = pNode->next;
	}

	if (pNode->next == NULL) {
		pList->ca_tail = pNode->previous;
	} else {
		pNode->next->previous = pNode->previous;
	}
	ca_free(pNode);
	pList->count--;
	return 0;
}

ca_int32 ca_list_delete_all(ca_list_t *pList)
{
	if (pList == NULL) {
		return -1;
	}
	__destory_helper(pList->ca_head);
    pList->ca_head = NULL;
    pList->ca_tail = NULL;
    pList->count = 0;
	return 0;
}

ca_list_node_t *ca_list_query(ca_list_t *pList, void *key)
{
	ca_list_node_t *pNode = NULL;

	if (pList == NULL || key == NULL) {
		return NULL;
	}
	if (pList->compare == NULL) {
		return NULL;
	}
	ca_list_scan(pList, pNode, ca_list_node_t *) {
		if (pList->compare(pNode->data, key) == 0)
			return pNode;
	}
	return NULL;
}

void ca_list_traverse(ca_list_t *pList, void (*traverse) (void *))
{
	ca_list_node_t *pNode = NULL;

	ca_list_scan(pList, pNode, ca_list_node_t *) {
		traverse(pNode->data);
	}
	return;
}

ca_list_node_t *ca_list_first(ca_list_t *pList)
{
	if (pList == NULL) {
		return NULL;
	}
	return pList->ca_head;

}

ca_list_node_t *ca_list_last(ca_list_t *pList)
{
	if (pList == NULL) {
		return NULL;
	}
	return pList->ca_tail;

}

ca_list_node_t *ca_list_previous(ca_list_node_t *pNode)
{
	if (pNode == NULL) {
		return NULL;
	}
	return pNode->previous;
}

ca_list_node_t *ca_list_next(ca_list_node_t *pNode)
{
	if (pNode == NULL) {
		return NULL;
	}
	return pNode->next;
}

ca_uint32 ca_list_count(ca_list_t *pList)
{
	if (pList == NULL) {
		return 0;
	}
	return pList->count;
}


void ca_lst_init(ca_list * pLst)
{
    pLst->ca_head = NULL;
    pLst->ca_tail = NULL;
    pLst->count = 0;
}

/**********************************************************
Description:
   insert a new node 'pNode' after  the specific node  'pPrevious'
**********************************************************/
void ca_lst_insert(ca_list * pLst, ca_node * pPrevious, ca_node * pNode)
{
    ca_node * pNext = NULL;

    if (NULL == pPrevious) {
        pNext = pLst->ca_head;
        pLst->ca_head = pNode;
    } else {
        pNext = pPrevious->next;
        pPrevious->next = pNode;
    }

    if (NULL == pNext) {
        pLst->ca_tail = pNode;
    } else {
        pNext->previous = pNode;
    }

    pNode->previous = pPrevious;
    pNode->next = pNext;

    pLst->count++;
}

/**********************************************************
Description:
   add a new node to the tail of 'pLst'
**********************************************************/
void ca_lst_add(ca_list * pLst, ca_node * pNode)
{
    ca_lst_insert(pLst, pLst->ca_tail, pNode);
}


/**********************************************************
Description:
   The operatoin doesn't mean to free the node , it is only cut off from 'pLst'
**********************************************************/
void ca_lst_delete(ca_list * pLst, ca_node * pNode)
{
    if (pNode->previous == NULL) {
        pLst->ca_head = pNode->next;
    } else {
        pNode->previous->next = pNode->next;
    }

    if (pNode->next == NULL) {
        pLst->ca_tail = pNode->previous;
    } else {
        pNode->next->previous = pNode->previous;
    }

    pLst->count--;
}

ca_node * ca_lst_first(ca_list * pLst)
{
    return pLst->ca_head;
}

ca_node * ca_lst_last(ca_list * pLst)
{
    return pLst->ca_tail;
}

/**********************************************************
Description:
   get the pointer of the first node in the list , and the node will be cutted off
   from the header of the list
**********************************************************/
ca_node * ca_lst_get(ca_list * pLst)
{
    ca_node * pNode = NULL;

    pNode = pLst->ca_head;

    if (pNode != NULL) {
        pLst->ca_head = pNode->next;

        if (pNode->next == NULL) {
            pLst->ca_tail = NULL;
        } else {
            pNode->next->previous = NULL;
        }

        pLst->count--;
    }

    return pNode;
}

ca_node * ca_lst_prev(ca_node * pNode)
{
    return pNode->previous;
}

ca_node * ca_lst_next(ca_node * pNode)
{
    return pNode->next;
}

ca_uint32 ca_lst_count(ca_list * pLst)
{
    return pLst->count;
}



