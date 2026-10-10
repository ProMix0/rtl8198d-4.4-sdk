#ifndef __CLI_CMD_H__
#define __CLI_CMD_H__
#include "cli_utils.h"
#include "cli_vector.h"
#include "cli_parser.h"

typedef enum{
    CLI_TAG_FORMAT_UNKNOW = 0,
    CLI_TAG_FORMAT_KEYWORD, 
    CLI_TAG_FORMAT_STR,
    CLI_TAG_FORMAT_RANGE,
    CLI_TAG_FORMAT_MAC,
    CLI_TAG_FORMAT_IPV4,
    CLI_TAG_FORMAT_IPV6
}cli_cmd_tag_format_e;


typedef struct{
    cli_int8    * tag;
    cli_int8    * desc;
    cli_uint8     format:5,
                 opt:1,
                 rsvd:2;
}cli_cmd_tag_item_t;

typedef struct{
    cli_boolean    key;
    cli_vector_t  items;  /* cli_cmd_tag_item_t list */
}cli_cmd_tag_t;

typedef struct{
    cli_uint8     pri;
    cli_func_t  * handler;
    cli_vector_t  tags;  /* cli_cmd_tag_t list */
}cli_cmd_t;


cli_cmd_t * cli_cmd_new(
        cli_uint8   pri,
        cli_func_t * handler,
        cli_int8 * cmd_str,
        cli_int8 * desc_str);

void cli_cmd_dump(
        cli_cmd_t * cmd);

cli_status_t cli_cmd_get_matched_tag(
        cli_cmd_t * cmd, 
        cli_cmd_input_info_t * input,
        cli_boolean * full_match,
        cli_cmd_tag_t **last_match_tag);

cli_boolean cli_cmd_verify(
        cli_cmd_input_info_t *  input,
        cli_cmd_t * cmd,
        cli_uint32 * err_loc);


cli_boolean cli_cmd_matched(
        cli_cmd_input_info_t *  input,
        cli_cmd_t * cmd,
        cli_boolean key_full_match
        );

cli_int8 * cli_cmd_dump_str(
    cli_cmd_t   * cmd);

#endif
