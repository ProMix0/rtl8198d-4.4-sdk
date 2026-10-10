#ifndef __SCFG_PARSE_H__
#define __SCFG_PARSE_H__

#define PARSE_OK			(0)
#define PARSE_ERROR			(-1)

#define SCFG_CFG_PREFIX "CFG_ID_"

typedef int (*scfg_parser) (const char *src, void *dst);

struct scfg_cfg_item {
	const ca_uint8 *scfg_id;
	ca_uint8_t *pscfg_val;
	scfg_parser parse_func;
	ca_uint32_t scfg_len;
};

const char *lookup_parser_name(scfg_parser parse_func);

int parse_char(const char *src, void *dst);
int parse_int(const char *src, void *dst);
int parse_string(const char *src, void *dst);
int scfg_parse_mac(const char *src, void *dst);
int parse_char_array(const char *src, void *dst);
int parse_int_array(const char *src, void *dst);

int scfg_update(const char *key, const char *data);
int parse_scfg_file(const char *filename);
int write_scfg_file(const char *filename, struct scfg_cfg_item *item);

#endif				/*#ifndef __SCFG_PARSE_H__ */
