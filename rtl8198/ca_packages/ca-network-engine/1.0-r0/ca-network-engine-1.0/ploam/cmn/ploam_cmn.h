#ifndef __PLOAM_CMN_H__
#define __PLOAM_CMN_H__

#ifdef CONFIG_NE_PON

#include "log.h"


#define CA_MOD_PLOAM  (77)


#define PLOAM_ASSERT(x)  \
    do{ \
        if (!(x))  { \
            PLOAM_ERROR("%s() _%d_: assert fail\n", __FUNCTION__, __LINE__); \
            return ; \
        } \
    }while(0)

#define PLOAM_ASSERT_RET(x,y) \
    do{ \
        if (!(x))  { \
            PLOAM_ERROR("%s() _%d_: assert fail\n", __FUNCTION__, __LINE__); \
            return (y); \
        } \
    }while(0)


#define PLOAM_ERROR(arg,...)   cap_log_msg(CA_MOD_PLOAM,CAP_LOG_MAJOR,CA_E_ERROR,arg,##__VA_ARGS__)

/*
#ifdef DEBUG
#define PLOAM_DEBUG(arg,...) \
    cap_log_msg(CA_MOD_PLOAM,CAP_LOG_DEBUG,0,"%s(): "arg, __FUNCTION__,##__VA_ARGS__)

#else
#define PLOAM_DEBUG(arg,...)
#endif
*/

#define PLOAM_LOG(arg,...) \
    cap_log_msg(CA_MOD_PLOAM,CAP_LOG_MAJOR,0,"%s(): "arg, __FUNCTION__,##__VA_ARGS__)



#endif/*END_CONFIG_NE_PON*/

#endif
