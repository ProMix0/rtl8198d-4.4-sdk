/******************************************************************************
 *
 *   Copyright (C) 2011  Intel Corporation. All rights reserved.
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <aal_phy.h>

struct parser_pair {
	unsigned rw;
	unsigned char st_code;
	unsigned char phy_port_addr;
	unsigned char reg_dev_addr;
	unsigned short addr;
	unsigned short data;

	struct list_head list;
};

enum parser_state {
	OTHER,
	PREFIX,
	DECIMAL,
	BINARY_NULL,
	BINARY,
	HEXIMAL_NULL,
	HEXIMAL,
	OCTAL,
	STORE
};

#define PARSER_SIZE 6
const unsigned PARSER_U_BOUND[6] = {1, 1, 31, 31, 0xffff, 0xffff};
const unsigned PARSER_L_BOUND[6] = {0, 0, 0, 0, 0, 0};
#define PARSERSTRING_SIZE 22

static unsigned char *ipfile;
static unsigned char *opfile;
module_param(ipfile, charp, 0);
module_param(opfile, charp, 0);

void string_cat(unsigned char *destination, \
		unsigned char *source, \
		unsigned long *from_ind, \
		unsigned char length)
{
	unsigned i = 0;

	for (; i < length; i++) {
		destination[*from_ind] = source[i];
		(*from_ind)++;
	}
}

void printk_str(unsigned char *str, unsigned long start, unsigned long end)
{
	#define BUF_SIZE (80)
	unsigned long i = start;
	unsigned long rd_end;
	unsigned long j;
	unsigned char sub[BUF_SIZE + 1];

	while (i < end) {
		if (i + BUF_SIZE < i)
			rd_end = end - 1;
		else if (i + BUF_SIZE > end)
			rd_end = end - 1;
		else
			rd_end = i + BUF_SIZE - 1;

		for (j = i; j <= rd_end; j++) {
			if (str[j] == '\n') {
				rd_end = j;
				break;
			}
		}

		j = 0;
		string_cat(sub, str + i, &j, rd_end - i + 1);
		sub[rd_end - i + 1] = '\0';
		printk(KERN_INFO "%s", sub);
		i = rd_end + 1;
	}
	#undef BUF_SIZE
}

/******************************************************************************
 * parameters
 *  rights: usually is 0
 *  fsizeptr: should be address of an unsigned long if the file size is needed
 * return
 *  NULL if error
 *  pointer to file if success
 *
 *  oldfs: backup current process address space
 *  set_fs: set process address space as upper bound of virtual address space(
 *          kernel process)
 *  vfs_stat(p, &stat): get file size information, it includes EOF(ascii:0A)
 *  set_fs(oldfs): return to original process address space
******************************************************************************/
struct file *file_open(const char *path, \
			int flags, \
			int rights, \
			unsigned long *fsizeptr)
{
	struct file *filp = NULL;
	struct kstat f_state;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(path, flags, rights);
	err = vfs_stat(path, &f_state);
	set_fs(oldfs);

	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		printk(KERN_ERR "FILE_OPEN_ERROR:%d\n", err);
		return NULL;
	}

	if (err) {
		printk(KERN_ERR "FILE_OPEN_ERROR: (vfs_stat Error:%d)\n", err);
		return NULL;
	}

	if (fsizeptr) {
		if (f_state.size < 0 || f_state.size > ((unsigned long)~0)) {
			printk(KERN_ERR \
"FILE_OPEN_ERROR: file size(%lld), upper bound(%lu).\n", \
					f_state.size, \
					((unsigned long)~0));
			return NULL;
		}
		*fsizeptr = (unsigned long)f_state.size - 1;
	}

	return filp;
}

/***************************************************
 * parameters
 *  offset: read action will start from the position
 *          (file beginning plus offset)
 *  fsize: the size want to read, should not be larger than
 *         the size of parameter data
 * return
 *  negative value if error
 *  size of data read
***************************************************/
int file_read(struct file *file, \
		unsigned long long offset, \
		unsigned char *data, \
		unsigned long fsize)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_read(file, data, fsize, &offset);
	data[fsize] = '\0';
	if (ret < 0)
		printk(KERN_ERR "FILE_READ_FAIL: %d\n", ret);

	set_fs(oldfs);

	return ret;
}

/*****************************************************************************
 * after this function should call vfs_fsync(file, 0) to sync data into flash
 *
 * parameters
 *  offset: write action will start from the position
 *          (file beginning plus offset)
 *  fsize: the size want to write, should not be larger than
 *         the size of parameter data
 * return
 *  negative value if error
 *  size of data write
*****************************************************************************/
int file_write(struct file *file, \
		unsigned long long offset, \
		unsigned char *data, \
		unsigned long fsize)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(file, data, fsize, &offset);
	set_fs(oldfs);
	if (ret < 0)
		printk(KERN_ERR "FILE_WRITE_FAIL: %d\n", ret);

	return ret;
}

/*****************************************************************************
 * omitting the content before the last '=' if the file includes '='.
 * parameters
 *  fdata: string data from read file
 *  fsize: size of fdata
 * return
 *  beginning position for parser
 *  return fsize if any error occurs
*****************************************************************************/
unsigned long parser_begin_pos(unsigned char *fdata,
				unsigned long fsize)
{
	unsigned long pos = 0;
	unsigned long i;

	if (!fdata) {
		printk(KERN_ERR "PARSERBEG ERROR: fdata is NULL\n");
		return fsize;
	}

	for (i = fsize - 1; i > 0; i--) {
		if (fdata[i] == '=') {
			pos = i;
			i = 1;
		}
	}

	return pos;
}

struct list_head *init_list_head(void)
{
	struct list_head *head = kmalloc(sizeof(*head), GFP_KERNEL);

	if (!head) {
		printk(KERN_ERR "INITLISTHEAD_FAIL\n");
		return NULL;
	}
	INIT_LIST_HEAD(head);
	return head;
}

int parser_list_add(struct list_head *head)
{
	struct parser_pair *p_node;

	p_node = kmalloc(sizeof(*p_node), GFP_KERNEL);
	if (!p_node) {
		printk(KERN_ERR "PARSERLISTADD_Fail\n");
		return -1;
	}
	__list_add(&(p_node->list), head->prev, head);
	return 0;
}

struct parser_pair *parser_list_tail(struct list_head *head)
{
	struct list_head *listptr;
	struct parser_pair *p_node;

	listptr = head->prev;
	p_node = list_entry(listptr, struct parser_pair, list);

	return p_node;
}

void parserprint(struct list_head *head)
{
	struct list_head *listptr = head->next;
	struct parser_pair *p_node;

	printk(KERN_INFO "Parser Content:\n");
	while (listptr != head) {
		p_node = list_entry(listptr, struct parser_pair, list);
		printk(KERN_INFO "%u\t%u\t%u\t%u\t%u\t%u\n", p_node->rw, \
						p_node->st_code, \
						p_node->phy_port_addr, \
						p_node->reg_dev_addr, \
						p_node->addr, \
						p_node->data);
		listptr = listptr->next;
	}
}

void parser_list_delete(struct list_head *head)
{
	struct parser_pair *p_node;
	struct list_head *listptr;

	while (!list_empty(head)) {
		listptr = head->next;
		p_node = list_entry(listptr, struct parser_pair, list);
		__list_del(head, head->next->next);
		kfree(p_node);
	}
	kfree(head);
}

/*****************************************************************************
 * parameters
 *  pdata: list of the parser result
 *  f_dataptr: return string of mdio_read result, argument should be address
 *             of pointer pointing to NULL
 *  f_sizeptr: return size of string of mdio_read result, argument should be
 *             address of unsigned long
*****************************************************************************/
int parser_to_read_string(struct list_head *pdata, \
				unsigned char **f_dataptr, \
				unsigned long *f_sizeptr)
{
	struct list_head *listptr;
	struct parser_pair *p_node;
	unsigned char buffer[6];
	unsigned long str_ind;

	if (!f_dataptr) {
		printk(KERN_ERR \
"PARSERTOREADSTRING_ERROR: fdataptr is NULL\n");
		return -1;
	} else if (*f_dataptr) {
		printk(KERN_ERR \
"PARSERTOREADSTRING_ERROR: *fdataptr isn't NULL\n");
		return -1;
	}

	if (!f_sizeptr) {
		printk(KERN_ERR \
"PARSERTOREADSTRING_ERROR: fsizeptr is NULL\n");
		return -1;
	}

	if (!pdata) {
		printk(KERN_ERR \
"PARSERTOREADSTRING_ERROR: pdataptr is NULL\n");
		return -1;
	}

	listptr = pdata->next;
	*f_sizeptr = 0;

	while (listptr != pdata) {
		p_node = list_entry(listptr, struct parser_pair, list);

		if (!(p_node->rw)) {
			if ((*f_sizeptr) > (*f_sizeptr) + PARSERSTRING_SIZE) {
				printk(KERN_ERR \
"PARSERTOREADSTRING_ERROR: size of read string is too large\n");
				return -1;
			}
			(*f_sizeptr) += PARSERSTRING_SIZE;
		}
		listptr = listptr->next;
	}

	*f_dataptr = kmalloc(sizeof(**f_dataptr) * (*f_sizeptr) + 1, \
			GFP_KERNEL);
	if (!*f_dataptr) {
		printk(KERN_ERR \
"PARSERTOREADSTRING_ERROR: memory allocate fail\n");
		return -1;
	}

	(*f_dataptr)[*f_sizeptr] = '\0';
	str_ind = 0;

	listptr = pdata->next;
	while (listptr != pdata) {
		p_node = list_entry(listptr, struct parser_pair, list);
		if (!(p_node->rw)) {
			sprintf(buffer, "%1d", p_node->st_code);
			string_cat(*f_dataptr, buffer, &str_ind, 1);
			(*f_dataptr)[str_ind++] = '\t';

			sprintf(buffer, "%2d", p_node->phy_port_addr);
			string_cat(*f_dataptr, buffer, &str_ind, 2);
			(*f_dataptr)[str_ind++] = '\t';

			sprintf(buffer, "%2d", p_node->reg_dev_addr);
			string_cat(*f_dataptr, buffer, &str_ind, 2);
			(*f_dataptr)[str_ind++] = '\t';

			string_cat(*f_dataptr, "0x", &str_ind, 2);
			sprintf(buffer, "%04X", p_node->addr);
			string_cat(*f_dataptr, buffer, &str_ind, 4);
			(*f_dataptr)[str_ind++] = '\t';

			string_cat(*f_dataptr, "0x", &str_ind, 2);
			sprintf(buffer, "%04X", p_node->data);
			string_cat(*f_dataptr, buffer, &str_ind, 4);
			(*f_dataptr)[str_ind++] = '\n';
		}
		listptr = listptr->next;
	}

	return 0;
}

/*****************************************************************************
 * parameters
 *  fdata: string data from read file
 *  fsize: size of fdata
 *  pdata: return list of the parser result of fdata
 * return value
 *  0 : normal
 *  -1: bound error
 *  -2: state error
 *  -3: format error
 *  -4: add node error
 *  -5: total number error
 *  -6: NULL pdata
 *  -7: p_ind fault
*****************************************************************************/
int parser_to_struct(unsigned char *fdata, \
			unsigned long fsize, \
			struct list_head *pdata)
{
	unsigned char p_ind = PARSER_SIZE - 1;
	unsigned long p_num = 0;
	enum parser_state state = OTHER;
	unsigned long file_ind;
	unsigned long value;
	unsigned char c;
	struct parser_pair *p_node;
	int err_code;

	if (!pdata) {
		printk(KERN_ERR "PARSER_STRUCT_ERROR: NULL pointer\n");
		return -6;
	}

	file_ind = parser_begin_pos(fdata, fsize);

	while (file_ind != fsize) {
		c = fdata[file_ind];
		switch (state) {
		case OTHER:
			if (c < '0' || c > '9')
				break;
			p_ind++;
			if (p_ind == PARSER_SIZE) {
				p_ind = 0;
				p_num++;
				err_code = parser_list_add(pdata);
				if (err_code) {
					printk(KERN_ERR \
"ADDNODE_ERROR: stop at (#node, #member):(%lu, %u)\n"\
						, p_num, p_ind);
					return -4;
				}
				p_node = parser_list_tail(pdata);
			}

			if (c == '0') {
				state = PREFIX;
			} else {
				state = DECIMAL;
				value = c - '0';
			}
			break;

		case PREFIX:
			if (c == 'b' || c == 'B') {
				state = BINARY_NULL;
			} else if (c == 'x' || c == 'X') {
				state = HEXIMAL_NULL;
			} else if (c >= '0' && c <= '7') {
				state = OCTAL;
				value = c - '0';
			} else if (c == '8' || c == '9') {
				printk(KERN_ERR \
"PREFIX_FORMAT_ERROR: stop at (#node, #member):(%lu, %u)\n" \
					, p_num, p_ind);
				return -3;
			} else {
				state = STORE;
				value = 0;
			}
			break;

		case DECIMAL:
			if (c >= '0' && c <= '9')
				value = value * 10 + c - '0';
			else
				state = STORE;
			break;

		case BINARY_NULL:
			if (c != '0' && c != '1') {
				printk(KERN_ERR \
"BINARY_FORMAT_ERROR: stop at (#node, #member):(%lu, %u)\n" \
					, p_num, p_ind);
				return -3;
			}
			value = c - '0';
			state = BINARY;
			break;

		case BINARY:
			if (c == '0' || c == '1')
				value = (value << 1) + c - '0';
			else
				state = STORE;
			break;

		case HEXIMAL_NULL:
			if (c >= '0' && c <= '9') {
				value = c - '0';
			} else if (c >= 'a' && c <= 'f') {
				value = c - 'a' + 10;
			} else if (c >= 'A' && c <= 'F') {
				value = c - 'A' + 10;
			} else {
				printk(KERN_ERR \
"HEXIMAL_FORMAT_ERROR: stop at (#node, #member):(%lu, %u)\n" \
					, p_num, p_ind);
				return -3;
			}
			state = HEXIMAL;
			break;

		case HEXIMAL:
			if (c >= '0' && c <= '9')
				value = (value << 4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				value = (value << 4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				value = (value << 4) + c - 'A' + 10;
			else
				state = STORE;
			break;

		case OCTAL:
			if (c >= '0' && c <= '7')
				value = (value << 3) + c - '0';
			else
				state = STORE;
			break;

		case STORE:
			if (value < PARSER_L_BOUND[p_ind] || \
				value > PARSER_U_BOUND[p_ind]) {
				printk(KERN_ERR \
"BOUNDARY_ERROR: stop at (#node, #member):(%lu, %u)\n" \
					, p_num, p_ind);
				return -1;
			}

			switch (p_ind) {
			case 0:
				p_node->rw = value;
				break;
			case 1:
				p_node->st_code = value;
				break;
			case 2:
				p_node->phy_port_addr = value;
				break;
			case 3:
				p_node->reg_dev_addr = value;
				break;
			case 4:
				p_node->addr = value;
				break;
			case 5:
				p_node->data = value;
				break;
			default:
				printk("STATE_ERROR: p_index=%d\n", p_ind);
				return -7;
			}
			state = OTHER;
			break;

		default:
			printk(KERN_ERR \
"STATE_ERROR: stop at (#node, #member, #state):(%lu, %u, %d)\n" \
						, p_num, p_ind, state);
			return -2;
		} /*switch*/

		if (state != STORE) {
			file_ind++;
			if (file_ind == fsize && state != OTHER) {
				file_ind--;
				state = STORE;
			}
		}
	} /*while*/

	if (p_ind != PARSER_SIZE - 1) {
		printk(KERN_ERR \
"TOTAL_NUMBER_DATA_ERROR: stop at (#node, #member):(%lu, %u)\n" \
						, p_num, p_ind + 1);
		return -5;
	}
	return 0;
}

int batch_mdio(struct list_head *head)
{
	struct list_head *listptr;
	struct parser_pair *p_node;
	ca_status_t err_code;

	listptr = head->next;
	while (listptr != head) {
		p_node = list_entry(listptr, struct parser_pair, list);

		if (p_node->rw) {
			err_code = aal_mdio_write(0, p_node->st_code, \
						p_node->phy_port_addr, \
						p_node->reg_dev_addr, \
						p_node->addr, \
						p_node->data);
		} else {
			err_code = aal_mdio_read(0, p_node->st_code, \
						p_node->phy_port_addr, \
						p_node->reg_dev_addr, \
						p_node->addr, \
						&(p_node->data));
		}

		if (err_code != CA_E_OK) {
			printk("BATCH_MDIO_ERROR:%d\n", err_code);
			return err_code;
		}
		listptr = listptr->next;
	}
	return 0;
}

int init_module(void)
{
	struct file *file = NULL;
	unsigned char *f_data = NULL;
	unsigned long f_size;
	struct list_head *p_data = NULL;
	int ret;

	if (!ipfile)
		ipfile = "/etc/cortina/ca-phy.txt";
	if (!opfile)
		opfile = "/etc/cortina/ca-phy-log.txt";

	file = file_open(ipfile, O_RDONLY, 0, &f_size);
	if (!file) {
		printk(KERN_ERR "FILE(R)_OPEN_ERR:%s", ipfile);
		goto WRONG;
	}
	if (f_size == 0) {
		printk(KERN_ERR "FILE_READ_SIZE_IS_ZERO\n");
		goto WRONG;
	}

	f_data = kmalloc(sizeof(*f_data) * (f_size + 1),  GFP_KERNEL);
	if (!f_data) {
		printk(KERN_ERR "MEMORY_ALLOCATE_FAIL\n");
		goto WRONG;
	}
	f_data[f_size] = '\0';

	ret = file_read(file, 0, f_data, f_size);
	filp_close(file, NULL);
	if (ret < 0)
		goto WRONG;

	p_data = init_list_head();
	ret = parser_to_struct(f_data, f_size, p_data);
	if (ret)
		goto WRONG;

	ret = batch_mdio(p_data);
	if (ret)
		goto WRONG;

	kfree(f_data);
	f_data = NULL;
	parser_to_read_string(p_data, &f_data, &f_size);

	file = file_open(opfile, \
			O_RDWR | O_APPEND | O_CREAT, \
			0, \
			(unsigned long *)0);
	if (!file) {
		printk(KERN_ERR "FILE(W)_OPEN_ERROR:%s", opfile);
		goto WRONG;
	}

	file_write(file, 0, "(S)\t(PA)\t(RA)\t(A)\t(D)\n", 22);
	file_write(file, 0, f_data, f_size);
	file_write(file, 0, "\n-----------\n", 13);
	filp_close(file, NULL);
	vfs_fsync(file, 0);
	printk(KERN_INFO "Write File(%s):\n", opfile);
	printk_str(f_data, 0, f_size);
	printk(KERN_INFO "(EOF)\n");

	kfree(f_data);
	f_data = NULL;
	parser_list_delete(p_data);
	p_data = NULL;
	printk(KERN_INFO "The mdio work is done!\n");
	ret = -1;

	return ret; /* This module never be  inserted successfully */

WRONG:
	if (f_data)
		kfree(f_data);
	if (p_data)
	parser_list_delete(p_data);
	printk(KERN_INFO "The mdio work is fail!\n");
	return -1;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Goodbye Cruel World!\n");
}

MODULE_LICENSE("GPL");
