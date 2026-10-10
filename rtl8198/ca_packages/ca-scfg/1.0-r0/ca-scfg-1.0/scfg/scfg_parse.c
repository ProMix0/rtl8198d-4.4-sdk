#include <osal_common.h>
#include <os_core.h>
#include "scfg_parse.h"
#include "scfg_show.h"
#include <scfg.h>

struct parse_pair {
	const char *parse_type;
	scfg_parser parse_func;
};

static struct parse_pair parse_pairs[] = {
	{"CHAR-ARRAY", parse_char_array},
	{"CHAR", parse_char},
	{"INT-ARRAY", parse_int_array},
	{"INT", parse_int},
	{"MAC", scfg_parse_mac},
	{"STRING", parse_string},
};

const char *lookup_parser_name(scfg_parser parse_func)
{
	struct parse_pair *first = parse_pairs;
	struct parse_pair *last = parse_pairs + ARRAY_SIZE(parse_pairs);

	for (; first != last; ++first) {
		if (first->parse_func == parse_func) {
			return first->parse_type;
		}
	}

	return NULL;
}

int parse_char(const char *src, void *dst)
{
	uint8_t *num;

	num = (uint8_t *) dst;
	if (src == NULL || dst == NULL) {
		return PARSE_ERROR;
	}

	if (kstrtou8(src, 0, num) == 0) {
		return PARSE_OK;
	}

	return PARSE_ERROR;
}

int parse_int(const char *src, void *dst)
{
	uint32_t *num = (uint32_t *) dst;

	if (src == NULL || dst == NULL) {
		return PARSE_ERROR;
	}

	if (kstrtou32(src, 0, num) == 0) {
		return PARSE_OK;
	}

	return PARSE_ERROR;
}

int parse_string(const char *src, void *dst)
{
	if (src == NULL || dst == NULL)
		return PARSE_ERROR;

	memcpy(dst, src, strlen(src));
	return PARSE_OK;
}

int scfg_parse_mac(const char *src, void *dst)
{
	if (mac_pton(src, dst)) {
		return PARSE_OK;
	}

	return PARSE_ERROR;
}

int parse_char_array(const char *src, void *dst)
{
	char *first, *last, *next, *new;
	unsigned char *pchar = (unsigned char *)dst;

	new = kstrdup(src, GFP_KERNEL);
	if (new == NULL) {
		return PARSE_ERROR;
	}

	first = new;
	last = new + strlen(src) + 1;

	strreplace(new, ',', '\0');
	for (; first < last; ++pchar, first = next) {
		next = first + strlen(first) + 1;

		first = strim(first);
		if (kstrtou8(first, 0, pchar) == 0) {
			continue;
		}

		kfree(new);
		return PARSE_ERROR;
	}

	kfree(new);
	return PARSE_OK;
}

int parse_int_array(const char *src, void *dst)
{
	char *first, *last, *next, *new;
	uint32_t *pchar = (uint32_t *) dst;

	new = kstrdup(src, GFP_KERNEL);
	if (new == NULL) {
		return PARSE_ERROR;
	}

	first = new;
	last = new + strlen(src) + 1;

	strreplace(new, ',', '\0');
	for (; first < last; ++pchar, first = next) {
		next = first + strlen(first) + 1;

		first = strim(first);
		if (kstrtou32(first, 0, pchar) == 0) {
			continue;
		}

		kfree(new);
		return PARSE_ERROR;
	}

	kfree(new);
	return PARSE_OK;
}

static int parse_str_space_remove(char *str, int is_string)
{
	char *c = str;
	int index = 0;
	int parse_status = 0;

	while (*c != '\0') {
		switch (*c) {
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			if (parse_status == 1)
				str[index++] = *c;
			break;
		case '"':
			if (parse_status == 0)
				parse_status = 1;
			else if (parse_status == 1)
				parse_status = 0;
			else;
			break;
		case '{':
			if (is_string)
				str[index++] = *c;
			else {
				if (parse_status == 0)
					parse_status = 2;
			}
			break;
		case '}':
			if (is_string)
				str[index++] = *c;
			else {
				if (parse_status == 2)
					parse_status = 0;
			}
			break;
		default:
			str[index++] = *c;
			break;
		}
		c++;
	}

	if (parse_status != 0)
		return PARSE_ERROR;

	str[index] = '\0';
	return 1;
}

static char *load_file(const char *filename, ca_int * filesize)
{
	ca_int fd;
	ca_int size;
	ca_int rc;
	char *buffer;

	fd = ca_file_open(filename, O_RDONLY, 0);
	if (fd == -1) {
		printk(KERN_INFO "Open file failed\n");
		return NULL;
	}

	size = ca_file_size(fd);

	buffer = ca_malloc(size + 1);
	if (buffer == NULL) {
		ca_file_close(fd);
		printk(KERN_ERR "Memory allocation failed\n");
		return NULL;
	}

	rc = ca_file_read(fd, buffer, size);
	ca_file_close(fd);

	if (size != rc) {
		ca_free(buffer);
		printk(KERN_DEBUG "read size isn't expected\n");
		return NULL;
	}

	buffer[size] = '\0';
	if (filesize != NULL) {
		*filesize = size;
	}
	return buffer;
}

static const char *lookup_cfg_type(const char *line)
{

	struct parse_pair *first = parse_pairs;
	struct parse_pair *last = parse_pairs + ARRAY_SIZE(parse_pairs);
	size_t len;

	for (; first != last; ++first) {
		len = strlen(first->parse_type);
		if (strncmp(line, first->parse_type, len) == 0) {
			return first->parse_type;
		}
	}

	return NULL;
}

static int parse_cfg_line(char *line, struct scfg_cfg_item *cfg)
{
	char *p, *q;
	const char *type;
	int is_string_type;

	q = strpbrk(line, "[#");
	if (q == NULL) {
		q = line + strlen(line);
	} else {
		*q = '\0';
	}

	p = skip_spaces(line);
	if (p == q) {
		return 1;
	}

	q = strchr(p, ';');
	if (q == NULL) {
		printk(KERN_WARNING "format error\n");
		return -1;
	}
	*q = '\0';

	type = lookup_cfg_type(p);
	if (type == NULL) {
		printk(KERN_WARNING "Unknown cfg type\n");
		return -1;
	}

	p = skip_spaces(p + strlen(type));
	q = strsep(&p, "=");
	if (p == NULL) {
		printk(KERN_WARNING "format error\n");
		return -1;
	}
	cfg->scfg_id = strim(q);

	is_string_type = strcmp(type, "STRING") == 0;
	if (parse_str_space_remove(p, is_string_type) == PARSE_ERROR) {
		printk(KERN_WARNING "format error\n");
		return -1;
	}

	cfg->pscfg_val = p;
	return 0;
}

static int find_cfg_pos(const char *filename, struct scfg_cfg_item *item)
{
	ca_int filesize;
	char *content;
	char *first, *last, *next;
	struct scfg_cfg_item cfg;
	int rc;

	content = load_file(filename, &filesize);
	if (content == NULL) {
		return -1;
	}

	strreplace(content, '\n', '\0');

	first = content;
	last = content + filesize;
	for (; first < last; first = next) {
		next = first + strlen(first) + 1;

		rc = parse_cfg_line(first, &cfg);
		if (rc > 0) {
			continue;
		}

		if (rc < 0) {
			kfree(content);
			return -2;
		}

		cfg.scfg_id += strlen(SCFG_CFG_PREFIX);
		if (strcmp(cfg.scfg_id, item->scfg_id) == 0) {
			break;
		}
	}

	rc = first - content;
	kfree(content);
	return rc;
}

static int write_scfg_content(const char *filename, int pos,
			      char *write_str, int len)
{
	ca_int fd;
	char *semicolon;
	char *content;

	content = load_file(filename, NULL);

	fd = ca_file_open(filename, O_RDWR | O_TRUNC | O_CREAT, 0644);
	if (fd == -1) {
		kfree(content);
		return SCFG_FOPEN_ERR;
	}

	if (content == NULL || pos < 0) {
		ca_file_write(fd, write_str, len);
	} else {
		semicolon = strchr(content + pos, ';');
		ca_file_write(fd, content, pos);
		if (semicolon != NULL) {
			ca_file_write(fd, write_str, len - 1);
			semicolon += 1;
			ca_file_write(fd, semicolon, strlen(semicolon));
		} else {
			ca_file_write(fd, write_str, len);
		}
	}

	ca_file_close(fd);
	kfree(content);
	return 0;
}

int write_scfg_file(const char *filename, struct scfg_cfg_item *item)
{
	int pos;
	char *write_str;
	int write_len;

	write_str = kmalloc(1024, GFP_KERNEL);
	if (write_str == NULL) {
		return SCFG_MALLOC_ERR;
	}

	write_len = sprintf(write_str, "%-15s %s%-60s = %s;\n",
			    lookup_parser_name(item->parse_func),
			    SCFG_CFG_PREFIX, item->scfg_id,
			    cfg_item_sprintf(write_str + 128, 896, item));

	pos = find_cfg_pos(filename, item);
	write_scfg_content(filename, pos, write_str, write_len);

	kfree(write_str);
	return 0;
}

int parse_scfg_file(const char *filename)
{
	char *first, *last, *next;
	ca_int filesize;
	char *content;
	struct scfg_cfg_item cfg;
	int rc;

	content = load_file(filename, &filesize);
	if (content == NULL) {
		return PARSE_ERROR;
	}

	strreplace(content, '\n', '\0');

	first = content;
	last = content + filesize;
	for (; first < last; first = next) {
		next = first + strlen(first) + 1;

		rc = parse_cfg_line(first, &cfg);
		if (rc > 0) {
			continue;
		}

		if (rc < 0) {
			kfree(content);
			return SCFG_PARAM_ERR;
		}

		scfg_update(cfg.scfg_id, cfg.pscfg_val);
	}

	kfree(content);
	return 0;
}
