#ifndef __CLI_HISTORY_ITEM_H__
#define __CLI_HISTORY_ITEM_H__
#include "cli_utils.h"

typedef struct{
    cli_int8*    line; 
    cli_uint32   index;
}cli_history_item_t;


cli_history_item_t * cli_history_item_new(
        const cli_int8 * line,
        cli_uint32 index);

void cli_history_item_delete(cli_history_item_t * item);

const cli_int8 * cli_history_item_get_line(cli_history_item_t * item);

cli_uint32 cli_history_item_get_index(cli_history_item_t * item);


#endif
