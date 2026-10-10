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
 * util_rbtree.c
 *
 * This file implements red-black tree functions for all Cortina Access drivers
 */
#include "os_core.h"
#include "osal_mem.h"

#include "util_rbtree.h"
#include "util_table.h"

ca_rbtree_t *ca_rbtree_create(ca_uint32 data_size,
			      ca_int32(*compare) (void *, void *))
{
	ca_rbtree_t *pTree;
	ca_rbtree_node_t *temp;

	if (data_size == 0 || compare == NULL) {
		return NULL;
	}
	pTree = (ca_rbtree_t *) ca_malloc(sizeof(ca_rbtree_t));
	if (pTree == NULL) {
		CA_ERROR("rbtree malloc failed\r\n");
		return NULL;
	}

	pTree->data_size = data_size;
	pTree->count = 0;
	pTree->compare = compare;

	temp = pTree->nil =
	    (ca_rbtree_node_t *) ca_malloc(sizeof(ca_rbtree_node_t));
	if (temp == NULL) {
		CA_ERROR("rbtree nil malloc failed\r\n");
		ca_free(pTree);
		return NULL;
	}
	temp->parent = temp->left = temp->right = temp;
	temp->red = 0;

	temp = pTree->root =
	    (ca_rbtree_node_t *) ca_malloc(sizeof(ca_rbtree_node_t));
	if (temp == NULL) {
		CA_ERROR("rbtree root malloc failed\r\n");
		ca_free(pTree->nil);
		ca_free(pTree);
		return NULL;
	}
	temp->parent = temp->left = temp->right = pTree->nil;
	temp->red = 0;

	return pTree;
}

static void __destory_helper(ca_rbtree_t *pTree, ca_rbtree_node_t *pNode)
{
	ca_rbtree_node_t *nil = pTree->nil;
	if (pNode != nil) {
		__destory_helper(pTree, pNode->left);
		__destory_helper(pTree, pNode->right);
		ca_free(pNode);
	}
}

void ca_rbtree_destory(ca_rbtree_t *pTree)
{
	if (pTree == NULL) {
		return;
	}
	__destory_helper(pTree, pTree->root->left);
	ca_free(pTree->root);
	ca_free(pTree->nil);
	ca_free(pTree);
}

static void __left_rotate(ca_rbtree_t *pTree, ca_rbtree_node_t *x)
{
	ca_rbtree_node_t *y;
	ca_rbtree_node_t *nil = pTree->nil;

	CA_DEBUG("left rotate\r\n");

	y = x->right;
	x->right = y->left;

	if (y->left != nil) {
		y->left->parent = x;	/* used to use sentinel here */
	}
	/* and do an unconditional assignment instead of testing for nil */

	y->parent = x->parent;

	/* instead of checking if x->parent is the root as in the book, we */
	/* count on the root sentinel to implicitly take care of this case */
	if (x == x->parent->left) {
		x->parent->left = y;
	} else {
		x->parent->right = y;
	}
	y->left = x;
	x->parent = y;

}

static void __right_rotate(ca_rbtree_t *pTree, ca_rbtree_node_t *y)
{
	ca_rbtree_node_t *x;
	ca_rbtree_node_t *nil = pTree->nil;

	CA_DEBUG("right rotate\r\n");

	x = y->left;
	y->left = x->right;

	if (nil != x->right) {
		x->right->parent = y;	/*used to use sentinel here */
	}
	/* and do an unconditional assignment instead of testing for nil */

	/* instead of checking if x->parent is the root as in the book, we */
	/* count on the root sentinel to implicitly take care of this case */
	x->parent = y->parent;
	if (y == y->parent->left) {
		y->parent->left = x;
	} else {
		y->parent->right = x;
	}
	x->right = y;
	y->parent = x;

}

static void __insert_helper(ca_rbtree_t *pTree, void *key,
			    ca_rbtree_node_t *z)
{
	ca_rbtree_node_t *x;
	ca_rbtree_node_t *y;
	ca_rbtree_node_t *nil = pTree->nil;

	z->left = z->right = nil;
	y = pTree->root;
	x = pTree->root->left;
	while (x != nil) {
		y = x;
		if (1 == pTree->compare(x->data, key)) {	/* x.key > z.key */
			x = x->left;
		} else {	/* x,key <= z.key */
			x = x->right;
		}
	}
	z->parent = y;
	if ((y == pTree->root) || (1 == pTree->compare(y->data, key))) {	/* y.key > z.key */
		y->left = z;
	} else {
		y->right = z;
	}

}

static void __delete_fixup(ca_rbtree_t *pTree, ca_rbtree_node_t *x)
{
	ca_rbtree_node_t *root = pTree->root->left;
	ca_rbtree_node_t *w;

	while ((!x->red) && (root != x)) {
		if (x == x->parent->left) {
			w = x->parent->right;
			if (w->red) {
				w->red = 0;
				x->parent->red = 1;
				__left_rotate(pTree, x->parent);
				w = x->parent->right;
			}
			if ((!w->right->red) && (!w->left->red)) {
				w->red = 1;
				x = x->parent;
			} else {
				if (!w->right->red) {
					w->left->red = 0;
					w->red = 1;
					__right_rotate(pTree, w);
					w = x->parent->right;
				}
				w->red = x->parent->red;
				x->parent->red = 0;
				w->right->red = 0;
				__left_rotate(pTree, x->parent);
				x = root;	/* this is to exit while loop */
			}
		} else {	/* the code below is has left and right switched from above */
			w = x->parent->left;
			if (w->red) {
				w->red = 0;
				x->parent->red = 1;
				__right_rotate(pTree, x->parent);
				w = x->parent->left;
			}
			if ((!w->right->red) && (!w->left->red)) {
				w->red = 1;
				x = x->parent;
			} else {
				if (!w->left->red) {
					w->right->red = 0;
					w->red = 1;
					__left_rotate(pTree, w);
					w = x->parent->left;
				}
				w->red = x->parent->red;
				x->parent->red = 0;
				w->left->red = 0;
				__right_rotate(pTree, x->parent);
				x = root;	/* this is to exit while loop */
			}
		}
	}
	x->red = 0;

}

ca_int32 ca_rbtree_add(ca_rbtree_t *pTree, void *key, void *data)
{
	ca_rbtree_node_t *x;
	ca_rbtree_node_t *y;

	if (pTree == NULL || key == NULL || data == NULL) {
		return -1;
	}
	if (pTree->compare == NULL) {
		return -1;
	}

	/* alloc the memory for the data and node */
	x = ca_malloc(pTree->data_size + sizeof(ca_rbtree_node_t));
	if (x == NULL) {
		return -1;
	}

	x->data = (ca_uint8 *) x + sizeof(ca_rbtree_node_t);
	memcpy(x->data, data, pTree->data_size);

	/* need compare function */
	__insert_helper(pTree, key, x);

	x->red = 1;
	while (x->parent->red) {	/* use sentinel instead of checking for root */
		if (x->parent == x->parent->parent->left) {
			y = x->parent->parent->right;
			if (y->red) {
				x->parent->red = 0;
				y->red = 0;
				x->parent->parent->red = 1;
				x = x->parent->parent;
			} else {
				if (x == x->parent->right) {
					x = x->parent;
					__left_rotate(pTree, x);
				}
				x->parent->red = 0;
				x->parent->parent->red = 1;
				__right_rotate(pTree, x->parent->parent);
			}
		} else {	/* case for x->parent == x->parent->parent->right */
			y = x->parent->parent->left;
			if (y->red) {
				x->parent->red = 0;
				y->red = 0;
				x->parent->parent->red = 1;
				x = x->parent->parent;
			} else {
				if (x == x->parent->left) {
					x = x->parent;
					__right_rotate(pTree, x);
				}
				x->parent->red = 0;
				x->parent->parent->red = 1;
				__left_rotate(pTree, x->parent->parent);
			}
		}
	}
	pTree->root->left->red = 0;

	CA_DEBUG("the color of node is %s\r\n",
		 (x->red == 0) ? "black" : "red");

	if (x->left != pTree->nil) {
		CA_DEBUG("the left node is %p\r\n",
			  (x->left->data));
	} else {
		CA_DEBUG("the left node is nil\r\n");
	}
	if (x->right != pTree->nil) {
		CA_DEBUG("the right node is %p\r\n",
			  (x->right->data));
	} else {
		CA_DEBUG("the right node is nil\r\n");
	}

	if (x->parent != pTree->root) {
		CA_DEBUG("the parent node is %p\r\n\r\n",
			  (x->parent->data));
	} else {
		CA_DEBUG("the parent node is root\r\n\r\n");
	}

	pTree->count++;
	return 0;

}

ca_int32 ca_rbtree_delete(ca_rbtree_t *pTree, ca_rbtree_node_t * z)
{
	ca_rbtree_node_t *y;
	ca_rbtree_node_t *x;
	ca_rbtree_node_t *nil;
	ca_rbtree_node_t *root;

	if (pTree == NULL || z == NULL) {
		return -1;
	}

	nil = pTree->nil;
	root = pTree->root;

	y = ((z->left == nil)
	     || (z->right == nil)) ? z : ca_rbtree_successor(pTree, z);
	x = (y->left == nil) ? y->right : y->left;

	if (root == (x->parent = y->parent)) {	/* assignment of y->p to x->p is intentional */
		root->left = x;
	} else {
		if (y == y->parent->left) {
			y->parent->left = x;
		} else {
			y->parent->right = x;
		}
	}

	if (y != z) {		/* y should not be nil in this case */

		/* y is the node to splice out and x is its child */

		if (!(y->red)) {
			__delete_fixup(pTree, x);
		}

		y->left = z->left;
		y->right = z->right;
		y->parent = z->parent;
		y->red = z->red;
		z->left->parent = z->right->parent = y;

		if (z == z->parent->left) {
			z->parent->left = y;
		} else {
			z->parent->right = y;
		}
		ca_free(z);
	} else {

		if (!(y->red)) {
			__delete_fixup(pTree, x);
		}
		ca_free(y);
	}
	pTree->count--;
	return 0;
}

ca_int32 ca_rbtree_delete_all(ca_rbtree_t *pTree)
{
	if (pTree == NULL) {
		return -1;
	}
	__destory_helper(pTree, pTree->root->left);
    pTree->root->left = pTree->nil;
    pTree->count = 0;
	return 0;
}

ca_rbtree_node_t *ca_rbtree_query(ca_rbtree_t *pTree, void *key)
{
	ca_rbtree_node_t *x;
	ca_rbtree_node_t *nil;
	int compVal;

	if (pTree == NULL || key == NULL) {
		return NULL;
	}
	nil = pTree->nil;
	x = pTree->root->left;

	if (x == nil)
		return (0);

	if (pTree->compare == NULL) {
		return NULL;
	}
	compVal = pTree->compare(x->data, key);
	while (0 != compVal) {	/*assignemnt */
		if (1 == compVal) {	/* x->key > q */
			x = x->left;
		} else {
			x = x->right;
		}
		if (x == nil) {
			return (0);
		}
		compVal = pTree->compare(x->data, key);
	}

	CA_DEBUG("the color of node is %s\r\n",
		 (x->red == 0) ? "black" : "red");

	if (x->left != nil) {
		CA_DEBUG("the left node is %p\r\n",
			  (x->left->data));
	} else {
		CA_DEBUG("the left node is nil\r\n");
	}
	if (x->right != nil) {
		CA_DEBUG("the right node is %p\r\n",
			  (x->right->data));
	} else {
		CA_DEBUG("the right node is nil\r\n");
	}

	if (x->parent != pTree->root) {
		CA_DEBUG("the parent node is %p\r\n\r\n",
			  (x->parent->data));
	} else {
		CA_DEBUG("the parent node is root\r\n\r\n");
	}
	return (x);
}

void __inorder_tree_traverser(ca_rbtree_t *pTree, ca_rbtree_node_t *x,
			      void (*traverse) (void *))
{
	if (pTree == NULL) {
		return;
	}
	if (x != pTree->nil) {
		__inorder_tree_traverser(pTree, x->left, traverse);
		traverse(x->data);
		__inorder_tree_traverser(pTree, x->right, traverse);
	}
	return;
}

void ca_rbtree_traverse(ca_rbtree_t *pTree, void (*traverse) (void *))
{
	__inorder_tree_traverser(pTree, pTree->root->left, traverse);
	return;
}

/* the first node of inoder traversal  */
ca_rbtree_node_t *ca_rbtree_first(ca_rbtree_t * pTree)
{
	ca_rbtree_node_t *x;
	ca_rbtree_node_t *nil;

	if (pTree == NULL) {
		return NULL;
	}

	nil = pTree->nil;
	x = pTree->root->left;

	if (x == nil)
		return NULL;

	while (x->left != nil) {	/*assignemnt */
		x = x->left;
	}
	return x;
}

ca_rbtree_node_t *ca_rbtree_predecessor(ca_rbtree_t *pTree,
					ca_rbtree_node_t *x)
{
	ca_rbtree_node_t *y;
	ca_rbtree_node_t *nil;
	ca_rbtree_node_t *root;

	if (pTree == NULL || x == NULL) {
		return NULL;
	}

	nil = pTree->nil;
	root = pTree->root;

	if (nil != (y = x->left)) {	/* assignment to y is intentional */
		while (y->right != nil) {	/* returns the maximum of the left subtree of x */
			y = y->right;
		}
		return (y);
	} else {
		y = x->parent;
		while (x == y->left) {
			if (y == root)
				return (nil);
			x = y;
			y = y->parent;
		}
		return (y);
	}
}

ca_rbtree_node_t *ca_rbtree_successor(ca_rbtree_t *pTree, ca_rbtree_node_t *x)
{
	ca_rbtree_node_t *y;
	ca_rbtree_node_t *nil;
	ca_rbtree_node_t *root;

	if (pTree == NULL || x == NULL) {
		return NULL;
	}

	nil = pTree->nil;
	root = pTree->root;

	if (nil != (y = x->right)) {	/* assignment to y is intentional */
		while (y->left != nil) {	/* returns the minium of the right subtree of x */
			y = y->left;
		}
		return (y);
	} else {
		y = x->parent;
		while (x == y->right) {	/* sentinel used instead of checking for nil */
			x = y;
			y = y->parent;
		}
		if (y == root) {
			return (nil);
		}
		return (y);
	}
}

ca_uint32 ca_rbtree_count(ca_rbtree_t *pTree)
{
	if (pTree == NULL) {
		return 0;
	}
	return pTree->count;
}
