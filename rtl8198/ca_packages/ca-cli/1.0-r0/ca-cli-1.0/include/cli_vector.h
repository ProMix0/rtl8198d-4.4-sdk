#ifndef __CLI_VECTOR_H__
#define __CLI_VECTOR_H__
#include "cli_utils.h"

typedef struct cli_vector {
    void** data;
    cli_uint32  size;
    cli_uint32  count;
} cli_vector_t;

void cli_vector_init(cli_vector_t*);
cli_uint32 cli_vector_count(cli_vector_t*);
void cli_vector_add(cli_vector_t*, void*);
void cli_vector_set(cli_vector_t*, cli_uint32, void*);
void *cli_vector_get(cli_vector_t*, cli_uint32);
void cli_vector_delete(cli_vector_t*, cli_uint32);
void cli_vector_free(cli_vector_t*);
void cli_vector_qsort(cli_vector_t *v, cli_uint32 elem_size,cli_int32 (*cmp_func)(const void *, const void *));

#endif
