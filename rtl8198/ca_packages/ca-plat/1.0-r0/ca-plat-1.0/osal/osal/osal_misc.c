#include "ca_types.h"
#include "os_core.h"
#include "osal_misc.h"

void ca_div(ca_uint64_t dividend, ca_uint32_t divisor, ca_uint64_t *quotient, ca_uint32_t *remainder)
{
	OS_DO_DIV(dividend, divisor, quotient, remainder);
}

