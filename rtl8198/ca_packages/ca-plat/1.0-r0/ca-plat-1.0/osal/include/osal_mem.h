#ifndef _OSAL_MEM_H_
#define _OSAL_MEM_H_
#include "ca_types.h"

void *ca_malloc(ca_int32 len);
void ca_free(void *ptr);

#endif

