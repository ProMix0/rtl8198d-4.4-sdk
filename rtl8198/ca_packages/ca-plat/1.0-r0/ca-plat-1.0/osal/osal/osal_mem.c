#include "ca_types.h"
#include "os_core.h"

/*lint -save -w1*/
void *ca_malloc(ca_int32 len)
{
    void *buf = NULL;
    buf = OS_MALLOC_API(len);
    if(buf)
        OS_MEMSET_API(buf, 0, len);
    return buf;
}

void ca_free(void *ptr)
{
    OS_FREE_API(ptr);
    return ;
}
/*lint -restore*/

