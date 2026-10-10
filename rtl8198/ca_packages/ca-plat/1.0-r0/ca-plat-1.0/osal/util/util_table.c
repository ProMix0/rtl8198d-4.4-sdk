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
 * util_table.c
 *
 * This file implements table management functions for all Cortina Access drivers
 */
#include "osal_mem.h"
#include "util_table.h"
#include "util_list.h"
#include "util_rbtree.h"
#include "util_misc.h"

ca_uint32 g_table_debug = 0;

#ifdef __KERNEL__
EXPORT_SYMBOL(g_table_debug);
#endif

ca_table_t *ca_table_create(ca_uint8 *table_name,
			    ca_table_type_e table_type,
			    ca_uint32 data_size,
			    ca_int32(*compare) (void *, void *))
{
	ca_table_t *pTable;
	void *content = NULL;

	pTable = ca_malloc(sizeof(ca_table_t));
	if (pTable == NULL) {
		CA_ERROR("table malloc failed\r\n");
		return NULL;
	}

	memcpy(pTable->table_name, table_name, CA_TABLE_NAME_STR_MAX);
	pTable->table_type = table_type;
	pTable->position = NULL;
	pTable->content = NULL;

	if (table_type == CA_RBTREE_TABLE) {
		CA_DEBUG("ca_rbtree_create\r\n");
		content = (void *)ca_rbtree_create(data_size, compare);
	} else {
		CA_DEBUG("ca_list_create\r\n");
		content = (void *)ca_list_create(data_size, compare);
	}

	if (content == NULL) {
		CA_ERROR("table content create failed\r\n");
		ca_free(pTable);
		return NULL;
	}
	pTable->content = content;
	return pTable;

}

ca_int32 ca_table_destory(ca_table_t *pTable)
{
	CA_ASSERT_RET(pTable != NULL, -1);

	if (pTable->content != NULL) {
		if (pTable->table_type == CA_RBTREE_TABLE) {
			ca_rbtree_destory((ca_rbtree_t *) (pTable->content));
		} else {
			ca_list_destory((ca_list_t *) (pTable->content));
		}
		pTable->content = NULL;
	}
	ca_free(pTable);

	return 0;
}

/* Description: insert an entry to the list or tree, if this entry has been added before, just modify it */
ca_int32 ca_table_entry_insert(ca_table_t *pTable, void *key, void *data)
{
	ca_list_t *pList = NULL;
	ca_rbtree_t *pTree = NULL;
	ca_list_node_t *list_node = NULL;
	ca_rbtree_node_t *tree_node = NULL;

	CA_ASSERT_RET(pTable != NULL, -1);

	if (pTable->table_type == CA_RBTREE_TABLE) {

		/* check if the entry is existed */
		pTree = (ca_rbtree_t *) (pTable->content);
		tree_node = ca_rbtree_query(pTree, key);
		if (tree_node != NULL) {	/* find it, just modify the entry */
			CA_DEBUG("find the node, and just modify it\r\n");
			memcpy(tree_node->data, data, pTree->data_size);
			return 0;
		}

		CA_DEBUG("start to add the tree node\r\n");

		return ca_rbtree_add((ca_rbtree_t *) (pTable->content), key,
				     data);

	} else {
		pList = (ca_list_t *) (pTable->content);
		list_node = ca_list_query(pList, key);

		if (list_node != NULL) {
			CA_DEBUG("find the node, and just modify it\r\n");
			memcpy(list_node->data, data, pList->data_size);
			return 0;
		}

		/* add an node to the list */

		CA_DEBUG("start to add the list node\r\n");
		return ca_list_add((ca_list_t *) (pTable->content), data);
	}

}

/* Description: remove an entry from the list or tree based on the key */
ca_int32 ca_table_entry_remove(ca_table_t *pTable, void *key)
{
	ca_list_node_t *list_node = NULL;
	ca_rbtree_node_t *tree_node = NULL;

	CA_ASSERT_RET(pTable != NULL, -1);

	if (pTable->table_type == CA_RBTREE_TABLE) {

		tree_node = ca_rbtree_query((ca_rbtree_t *) (pTable->content), key);
		if (tree_node == NULL) {
			CA_DEBUG("not find the node\r\n");
			return -1;
		}

		CA_DEBUG("start to delete the tree node\r\n");
		ca_rbtree_delete((ca_rbtree_t *) (pTable->content), tree_node);

	} else {

		list_node = ca_list_query((ca_list_t *) (pTable->content), key);
		if (list_node == NULL) {
			CA_DEBUG("not find the node\r\n");
			return -1;
		}
		CA_DEBUG("start to delete the list node\r\n");
		ca_list_delete((ca_list_t *) (pTable->content), list_node);

	}

	return 0;
}

ca_int32 ca_table_entry_remove_all(ca_table_t *pTable)
{
	CA_ASSERT_RET(pTable != NULL, -1);

	if (pTable->table_type == CA_RBTREE_TABLE) {

		CA_DEBUG("start to delete all the tree nodes\r\n");
		ca_rbtree_delete_all((ca_rbtree_t *) (pTable->content));

	} else {

		CA_DEBUG("start to delete all the list nodes\r\n");
		ca_list_delete_all((ca_list_t *) (pTable->content));

	}

	return 0;
}

/* Description: search the entry based on the key,  return the entry data addr, not the node addr */
void *ca_table_entry_search(ca_table_t *pTable, void *key)
{
	ca_list_node_t *list_node = NULL;
	ca_rbtree_node_t *tree_node = NULL;

	CA_ASSERT_RET(pTable != NULL, NULL);

	if (pTable->table_type == CA_RBTREE_TABLE) {

		tree_node = ca_rbtree_query((ca_rbtree_t *) (pTable->content), key);
		if (tree_node != NULL) {
			return tree_node->data;
		}
	} else {

		list_node = ca_list_query((ca_list_t *) (pTable->content), key);
		if (list_node != NULL) {
			return list_node->data;
		}
	}
	CA_DEBUG("not find the node\r\n");
	return NULL;
}

/* the parameter (void * ) of traverse is the entry data addr, not the node addr */
void ca_table_entry_traverse(ca_table_t *pTable, void (*traverse) (void *))
{
	ca_list_t *pList = NULL;
	ca_rbtree_t *pTree = NULL;

	if (pTable->table_type == CA_RBTREE_TABLE) {
		pTree = (ca_rbtree_t *) (pTable->content);
		ca_rbtree_traverse(pTree, traverse);
	} else {
		pList = (ca_list_t *) (pTable->content);
		ca_list_traverse(pList, traverse);
	}
	return;
}

/* Description: get the first entry,  return the entry data addr, not the node addr */
void *ca_table_entry_getfirst(ca_table_t *pTable)
{
	ca_list_t *pList = NULL;
	ca_rbtree_t *pTree = NULL;

	ca_list_node_t *list_node = NULL;
	ca_rbtree_node_t *tree_node = NULL;

	CA_ASSERT_RET(pTable != NULL, NULL);

	if (pTable->table_type == CA_RBTREE_TABLE) {
		pTree = (ca_rbtree_t *) (pTable->content);
		tree_node = ca_rbtree_first(pTree);
		pTable->position = (void *)tree_node;
		if (tree_node != NULL) {
			return tree_node->data;
		}
	} else {
		pList = (ca_list_t *) (pTable->content);
		list_node = ca_list_first(pList);
		pTable->position = (void *)list_node;
		if (list_node != NULL) {
			return list_node->data;
		}
	}
	return NULL;
}

/* Description: get the next entry after the previous get operation
	User should do mutex protect when use getfirst and getnext to traversal the table entries */
void *ca_table_entry_getnext(ca_table_t *pTable)
{
	ca_list_t *pList = NULL;
	ca_rbtree_t *pTree = NULL;

	ca_list_node_t *list_node = NULL;
	ca_rbtree_node_t *tree_node = NULL;

	CA_ASSERT_RET(pTable != NULL, NULL);

	if (pTable->position == NULL) {
		return NULL;
	}

	if (pTable->table_type == CA_RBTREE_TABLE) {
		pTree = (ca_rbtree_t *) (pTable->content);

		tree_node = ca_rbtree_successor(pTree,
					(ca_rbtree_node_t *) (pTable->position));
		pTable->position = (void *)tree_node;
		if (tree_node != NULL) {
			return tree_node->data;
		}
	} else {
		pList = (ca_list_t *) (pTable->content);

		list_node = ca_list_next((ca_list_node_t *) (pTable->position));
		pTable->position = (void *)list_node;
		if (list_node != NULL) {
			return list_node->data;
		}
	}
	return NULL;
}

ca_int32 ca_table_entry_count(ca_table_t *pTable)
{
	ca_uint32 count = 0;

	CA_ASSERT_RET(pTable != NULL, 0);

	if (pTable->content != NULL) {
		if (pTable->table_type == CA_RBTREE_TABLE) {
			count =ca_rbtree_count((ca_rbtree_t *) (pTable->content));
		} else {
			count = ca_list_count((ca_list_t *) (pTable->content));
		}
	}
	return count;
}

#if 0
/* sample code */
ca_int32 ca_vlan_compare_fun(void *entry, void *key);

ca_void_t ca_vlan_entry_print(void *entry);

typedef struct vlan_entry {

	ca_uint32 bmp_info;
	ca_uint32 vid;
} ca_vlan_entry_t;

ca_table_t *ca_vlan_table = NULL;

void ca_vlan_traverse_fun(void *entry)
{
	ca_vlan_entry_t *vlan_entry = entry;

	printf("entry->vid = %d, bmp_info = %d\r\n", vlan_entry->vid,
	       vlan_entry->bmp_info);

	return;
}

ca_int32 ca_vlan_compare_fun(void *entry, void *key)
{
	ca_vlan_entry_t *vlan_entry = entry;

	printf("start to do key compare, entry->vid = %d, key = %d\r\n",
	       vlan_entry->vid, *(ca_uint32 *) key);

	if (vlan_entry->vid > *(ca_uint32 *) key) {
		return 1;
	} else if (vlan_entry->vid < *(ca_uint32 *) key) {

		return -1;
	}
	return 0;

}

ca_void_t ca_vlan_entry_print(void *entry)
{
	ca_vlan_entry_t *vlan_entry = entry;

	printf("vid %d, bmp_info 0x%x\r\n\n", vlan_entry->vid,
	       vlan_entry->bmp_info);
}

int ca_vlan_table_init(void)
{
	int ret;

	ca_vlan_table =
	    ca_table_create("vlan_table", CA_RBTREE_TABLE,
			    sizeof(ca_vlan_entry_t), ca_vlan_compare_fun);

	if (ca_vlan_table == NULL) {
		CA_ERROR("table create failed!");
		return -1;
	}
	/* other init */

	return 0;
}				/* ca_vlan_init */

ca_int32 main()
{
	ca_int32 type = CA_LIST_TABLE;
	ca_uint32 info = 0;
	ca_uint32 key = 0;

	ca_vlan_entry_t vlan_entry;

	ca_int32 option = 0;

	ca_list_node_t *pNode = NULL;
	ca_list_t *pList = NULL;
	ca_void_t *pData = NULL;

	printf("start to init vlan table\r\n");

	//ca_vlan_table_init();

	while (option != 8) {
		printf("choose one of the following:\n");
		printf
		    ("(1) init the table\n(2) add to table\n(3) delete from table\n(4) find\n");
		printf
		    ("(5) traverse table\n(6) get count\n(7) destroy the table\n(8) quit\n");

		do
			option = fgetc(stdin);
		while (-1 != option && isspace(option));
		option -= '0';
		switch (option) {
		case 1:
			{
				printf
				    ("type type for table: 0 is list and 1 is red-black tree\n");
				scanf("%i", &type);
				ca_vlan_table =
				    ca_table_create("vlan_table", type,
						    sizeof(ca_vlan_entry_t),
						    ca_vlan_compare_fun);

				if (ca_vlan_table == NULL) {
					CA_ERROR("table create failed!");
					return -1;
				}

			}
			break;

		case 2:
			{
				printf("type key for new node\n");
				scanf("%i", &key);
				printf("type info for new node\n");
				scanf("%i", &info);

				vlan_entry.vid = key;
				vlan_entry.bmp_info = info;
				printf
				    ("start to add an entry to the table\r\n");
				ca_table_entry_insert(ca_vlan_table, &key,
						      &vlan_entry);

			}
			break;

		case 3:
			{
				printf("type key of node to remove\n");
				scanf("%i", &key);

				printf
				    ("start to remove an entry to the list\r\n");
				ca_table_entry_remove(ca_vlan_table, &key);

			}
			break;

		case 4:
			{
				printf("type key of node to query for\n");
				scanf("%i", &key);

				printf
				    ("start to search an entry from the list\r\n");
				pData =
				    ca_table_entry_search(ca_vlan_table, &key);

				if (pData == NULL) {
					printf
					    ("not find the entry for the key\r\n");
				} else {
					ca_vlan_entry_print(pData);
				}
			}
			break;

		case 5:
			{
				printf("traverse the table \n");
				ca_table_entry_traverse(ca_vlan_table,
							ca_vlan_traverse_fun);
#if 0
				pData = ca_table_entry_getfirst(ca_vlan_table);
				while (pData != NULL) {
					ca_vlan_entry_print(pData);
					pData =
					    ca_table_entry_getnext
					    (ca_vlan_table);
				}
#endif
			}
			break;

		case 6:
			{
				ca_uint32 count = 0;
				count = ca_table_entry_count(ca_vlan_table);
				printf("the number of entry is %d\r\n", count);
			}
			break;

		case 7:
			{
				ca_table_destory(ca_vlan_table);
				ca_vlan_table = NULL;
			}
			break;

		case 8:
			{
				ca_table_destory(ca_vlan_table);
				ca_vlan_table = NULL;
				return 0;
			}
			break;

		default:
			printf("Invalid input; Please try again.\n");
		}

	}
	return 0;

}

#endif
