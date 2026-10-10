#include <stdio.h>
#include <string.h>
#include <common/rt_type.h>
#include <common/rt_error.h>
#include <common/util/rt_util.h>
#include <diag_util.h>
#include <diag_str.h>
#include <parser/cparser_priv.h>
#include <rtk/rt/rt_trap.h>

/*
 * rt_trap init
 */
cparser_result_t
cparser_cmd_rt_trap_init(
    cparser_context_t *context)
{
    int32 ret = RT_ERR_FAILED;
    DIAG_UTIL_PARAM_CHK();

    DIAG_UTIL_ERR_CHK(rt_trap_init(), ret);

    return CPARSER_OK;
}    /* end of cparser_cmd_rt_trap_init */

