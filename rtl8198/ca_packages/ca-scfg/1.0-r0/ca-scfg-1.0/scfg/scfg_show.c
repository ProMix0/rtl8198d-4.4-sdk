#include <ca_types.h>
#include "scfg_parse.h"
#include <scfg.h>
#include <linux/kernel.h>
#include <linux/gfp.h>

static char *scfg_show_char(char *str, size_t size, const void *data,
			    size_t length)
{
	const uint8_t *p = data;
	sprintf(str, "0x%02x", *p);
	return str;
}

static char *scfg_show_int(char *str, size_t size,
			   const void *data, size_t length)
{
	const uint32_t *p = data;
	sprintf(str, "0x%08x", *p);
	return str;
}

static char *scfg_show_string(char *str, size_t size,
			      const void *data, size_t length)
{
	strncpy(str, (char *)data, size);
	str[length] = '\0';
	return str;
}

static char *scfg_show_mac(char *str, size_t size,
			   const void *data, size_t length)
{
	const char *p = data;
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
		p[0], p[1], p[2], p[3], p[4], p[5]);
	return str;
}

static char *scfg_show_char_array(char *str, size_t size,
				  const void *data, size_t length)
{
	char *p = str;
	const uint8_t *first = data;
	const uint8_t *last = first + length;

	p += sprintf(p, "{");

	for (; first != last; ++first) {
		p += sprintf(p, "0x%X,", *first);
	}

	p = strrchr(str, ',');
	if (p != NULL) {
		sprintf(p, "}");
	} else {
		*str = '\0';
	}

	return str;
}

static char *scfg_show_int_array(char *str, size_t size,
				 const void *data, size_t length)
{
	char *p = str;
	const uint32_t *first = data;
	const uint32_t *last = first + (length / sizeof(uint32_t));

	p += sprintf(p, "{");

	for (; first != last; ++first) {
		p += sprintf(p, "0x%X,", *first);
	}

	p = strrchr(str, ',');
	if (p != NULL) {
		sprintf(p, "}");
	} else {
		*str = '\0';
	}

	return str;
}

typedef char *(*scfg_show_func) (char *str, size_t size,
				 const void *data, size_t length);

struct scfg_show_pair {
	scfg_parser parse_func;
	scfg_show_func show_func;
};

static struct scfg_show_pair show_pairs[] = {
	{parse_char, scfg_show_char},
	{parse_int, scfg_show_int},
	{parse_string, scfg_show_string},
	{scfg_parse_mac, scfg_show_mac},
	{parse_char_array, scfg_show_char_array},
	{parse_int_array, scfg_show_int_array}
};

static scfg_show_func lookup_scfg_show(const struct scfg_cfg_item *cfg_item)
{
	struct scfg_show_pair *first = show_pairs;
	struct scfg_show_pair *last = show_pairs + ARRAY_SIZE(show_pairs);

	for (; first != last; ++first) {
		if (first->parse_func == cfg_item->parse_func) {
			return first->show_func;
		}
	}

	return NULL;
}

char *cfg_item_sprintf(char *str, size_t size,
		       const struct scfg_cfg_item *cfg_item)
{
	scfg_show_func show_func;

	show_func = lookup_scfg_show(cfg_item);
	if (show_func == NULL) {
		printk(KERN_ERR "%s: unknown data type\n", __func__);
		return NULL;
	}

	show_func(str, size, cfg_item->pscfg_val, cfg_item->scfg_len);
	return str;
}

void cfg_item_printf(const char *prefix, const struct scfg_cfg_item *cfg_item)
{
	char str[CFG_ID_VALUE_LEN];
	cfg_item_sprintf(str, CFG_ID_VALUE_LEN * sizeof(char), cfg_item);

	printk("%s: CFG_ID_%s = %s \n", prefix, cfg_item->scfg_id, str);
}
