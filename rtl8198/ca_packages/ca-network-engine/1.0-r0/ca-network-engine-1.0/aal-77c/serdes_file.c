#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/fcntl.h>

#include "serdes_file.h"
#include "ca_serdes.h"
#include "osal_file.h"

#define MAX_SERDES_NUM (2)
#define MAX_SERDES_HEADER_NUM (2)

#define SERDES_PREFIX "serdes:"

static char *load_seder_file(const char *filename)
{
	ca_int fd;
	int filesize, rc;
	char *buffer;

	fd = ca_file_open(filename, O_RDONLY, 0);
	if (fd == -1) {
		printk(SERDES_PREFIX "file(%s) open failed\n", filename);
		return NULL;
	}

	filesize = ca_file_size(fd);
	if (filesize == 0) {
		printk(SERDES_PREFIX "file(%s) is empty\n", filename);
		ca_file_close(fd);
	}

	buffer = kmalloc((filesize + 1) * sizeof(char), GFP_KERNEL);
	if (buffer == NULL) {
		printk(SERDES_PREFIX "memory allocation failed\n");
		ca_file_close(fd);
		return NULL;
	}

	rc = ca_file_read(fd, buffer, filesize);
	ca_file_close(fd);

	if (filesize != rc) {
		printk(SERDES_PREFIX "file(%s) read failed\n", filename);
		kfree(buffer);
		return NULL;
	}

	buffer[filesize] = '\0';
	return buffer;
}

static int strcnt(const char *first, const char *last, char c)
{
	int n, orphan;

	for (n = orphan = 0; first != last; first++) {
		orphan = 1;
		if (*first == c) {
			n = n + 1;
			orphan = 0;
		}
	}

	return n + orphan;
}

static int cells_number(const char *first, const char *last)
{
	return strcnt(first, last, ',');
}

static int rows_number(const char *first, const char *last)
{
	return strcnt(first, last, '\n');
}

/* Return true if it's not a number string */
static int is_section_header(const char *str)
{
	const char *p;

	p = str;

	if (p == NULL || p[0] == '\0') {
		return -1;
	}

	if (p[1] != '\0' && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		for (p += 2; *p != ',' && *p != '\0'; ++p) {
			if (!isxdigit(*p)) {
				return 1;
			}
		}

		if (str + 2 == p) {
			return 1;
		}

		return 0;
	}

	for (; *p != ',' && *p != '\0'; ++p) {
		if (isdigit(*p)) {
			continue;
		}

		return 1;
	}

	return 0;
}

static void free_serdes_headers(struct Serdes *serdes)
{
	int i;

	if (serdes == NULL || serdes->header == NULL) {
		return;
	}

	for (i = 0; i < MAX_SERDES_HEADER_NUM; ++i) {
		if (serdes->header[i] != NULL) {
			kfree(serdes->header[i]);
			serdes->header[i] = NULL;
		}
	}

	kfree(serdes->header);
	serdes->header = NULL;
}

void free_serdes(struct Serdes *serdes)
{
	if (serdes == NULL) {
		return;
	}

	if (serdes->cells != NULL) {
		kfree(serdes->cells);
		serdes->cells = NULL;
	}

	if (serdes->version != NULL) {
		kfree(serdes->version);
		serdes->version = NULL;
	}

	free_serdes_headers(serdes);
	kfree(serdes);
}

static int is_comment_line(const char *row)
{
	if (row && *row == '#') {
		return 1;
	}

	return 0;
}

static char *parse_serdes_headers(char *first, char *last,
				  struct Serdes *serdes)
{
	int header_cnt, rc;
	char *next;
	int column_num[MAX_SERDES_HEADER_NUM];

	serdes->header = kmalloc(sizeof(char *) * MAX_SERDES_HEADER_NUM,
				 GFP_KERNEL);
	if (serdes->header == NULL) {
		printk(SERDES_PREFIX "memory allocation failed\n");
		return NULL;
	}

	for (header_cnt = 0; first < last; first = next + 1) {
		rc = is_section_header(first);
		if (rc == -1) {
			printk(SERDES_PREFIX "invalid header format\n");
			return NULL;
		}

		if (rc == 0) {
			break;
		}

		if (header_cnt > MAX_SERDES_HEADER_NUM) {
			free_serdes_headers(serdes);
			printk(SERDES_PREFIX "too many headers in section\n");
			return NULL;
		}

		next = strchr(first, '\n');
		*next = '\0';

		if (is_comment_line(first)) {
			continue;
		}

		column_num[header_cnt] = cells_number(first, next);

		serdes->header[header_cnt] = kstrdup(first, GFP_KERNEL);
		if (serdes->header[header_cnt] == NULL) {
			free_serdes_headers(serdes);
			printk(SERDES_PREFIX "memory allocation failed\n");
			return NULL;
		}

		strreplace(serdes->header[header_cnt], ',', '\0');
		header_cnt += 1;
	}

	if (header_cnt == 0) {
		free_serdes_headers(serdes);
		printk(SERDES_PREFIX "no header rows\n");
		return NULL;
	}

	if (serdes->header[1] && (column_num[0] != column_num[1])) {
		free_serdes_headers(serdes);
		printk(SERDES_PREFIX
		       "column #num in header isn't consistent\n");
		return NULL;
	}
	serdes->column_num = column_num[0];

	return first;
}

static char *parse_serdes_cells(char *first, char *last, struct Serdes *serdes)
{
	uint32_t *cell;
	int rc, total_cells;

	serdes->row_num = rows_number(first, last);

	strreplace(first, '\n', ',');
	total_cells = cells_number(first, last);
	if (total_cells != serdes->row_num * (serdes->column_num + 2)) {
		printk(SERDES_PREFIX "cells != row*column\n");
		return NULL;
	}

	serdes->cells = kmalloc(sizeof(uint32_t) * total_cells, GFP_KERNEL);
	if (serdes->cells == NULL) {
		printk(SERDES_PREFIX "memory allocation failed\n");
		return NULL;
	}

	strreplace(first, ',', '\0');
	for (cell = serdes->cells; first < last; cell++) {
		rc = kstrtou32(first, 0, cell);
		if (rc != 0) {
			printk(SERDES_PREFIX "%s is not number\n", first);
			kfree(serdes->cells);
			serdes->row_num = 0;
			serdes->cells = NULL;
			return NULL;
		}

		first += strlen(first) + 1;
	}

	return first;
}

static struct Serdes *parse_serdes_section(char *first, char *last)
{
	struct Serdes *serdes;

	serdes = kmalloc(sizeof(struct Serdes), GFP_KERNEL);
	if (serdes == NULL) {
		printk(SERDES_PREFIX "memory allocation failed\n");
		return NULL;
	}
	memset(serdes, 0, sizeof(struct Serdes));

	first = parse_serdes_headers(first, last, serdes);
	if (first == NULL) {
		printk(SERDES_PREFIX "section header parse failed\n");
		free_serdes(serdes);
		return NULL;
	}

	first = parse_serdes_cells(first, last, serdes);
	if (first == NULL) {
		printk(SERDES_PREFIX "section cells parse failed\n");
		free_serdes(serdes);
		return NULL;
	}

	return serdes;
}

static const char *parse_serdes_version(char *content)
{
	char *comma, *version;

	comma = strchr(content, ',');
	if (comma == NULL) {
		return NULL;
	}

	*comma = '\0';
	if (strncmp(content, "version", strlen("version")) != 0) {
		return NULL;
	}

	version = comma + 1;
	comma = strchr(version, ',');
	if (comma) {
		*comma = '\0';
	}

	return kstrdup(version, GFP_KERNEL);
}

struct Serdes *parse_serdes_file(const char *filename, int serdes_index)
{
	int i;
	char *filecontent;
	char *section_start, *section_end, *next_section;
	struct Serdes *serdes;

	if (serdes_index >= MAX_SERDES_NUM) {
		printk(SERDES_PREFIX "invalid serdes index=%d\n", serdes_index);
		return NULL;
	}

	filecontent = load_seder_file(filename);
	if (filecontent == NULL) {
		return NULL;
	}

	serdes = NULL;
	section_start = strchr(filecontent, '\n');
	if (section_start == NULL) {
		printk(SERDES_PREFIX "No line terminator\n");
		goto ERROR_EXIT;
	}
	section_start += 1;

	for (i = 0; i < MAX_SERDES_NUM; ++i, section_start = next_section) {

		section_end = strstr(section_start, "Blank line");
		if (section_end == NULL) {
			printk(SERDES_PREFIX "No section delimiter\n");
			goto ERROR_EXIT;
		}
		*section_end = '\0';

		next_section = section_end + strlen("Blank line") + 1;

		if (i != serdes_index) {
			continue;
		}

		serdes = parse_serdes_section(section_start, section_end);
		if (serdes == NULL) {
			goto ERROR_EXIT;
		}
		break;
	}

	serdes->version = parse_serdes_version(filecontent);
	if (serdes->version == NULL) {
		goto ERROR_EXIT;
	}

	kfree(filecontent);
	return serdes;

 ERROR_EXIT:

	free_serdes(serdes);
	kfree(filecontent);
	return NULL;
}

void show_serdes_headers(const struct Serdes *serdes)
{
	int i, j;
	const char *h;

	for (i = 0; i < MAX_SERDES_HEADER_NUM; ++i) {
		h = serdes->header[i];
		if (h == NULL) {
			return;
		}

		for (j = 0; j < serdes->column_num; ++j) {
			printk("%s ", h);
			h += strlen(h) + 1;
		}
		printk("\n");
	}
}

static int lookup_column_index1(const struct Serdes *serdes, const char *name)
{
	int i;
	const char *header;

	header = serdes->header[0];
	for (i = 0; i < serdes->column_num; i++) {
		if (strcmp(header, name) == 0) {
			return i;
		}
	}

	return -1;
}

static int lookup_column_index2(const struct Serdes *serdes,
				const char *name1, const char *name2)
{
	int i;
	const char *h1, *h2;

	h1 = serdes->header[0];
	h2 = serdes->header[1];

	for (i = 0; i < serdes->column_num;) {
		if (strcmp(h1, name1) == 0 && strcmp(h2, name2) == 0) {
			return i;
		}

		++i;
		h1 += strlen(h1) + 1;
		h2 += strlen(h2) + 1;
	}

	return -1;
}

int lookup_column_index(const struct Serdes *serdes,
			const char *name1, const char *name2)
{
	if (serdes->header[1] == NULL) {
		if (name2 != NULL || *name2 != '\0') {
			return -1;
		}

		return lookup_column_index1(serdes, name1);
	}

	if (name2 == NULL || *name2 == '\0') {
		return -1;
	}

	return lookup_column_index2(serdes, name1, name2);
}
