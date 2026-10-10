#ifndef __CON_DEFER_INIT_H__
#define __CON_DEFER_INIT_H__

#include "voip_types.h"

#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
#define PATHLEN 128
typedef struct fw_info_s{
	uint32 fwSize;
	uint32 BBDSize;
	char fwPath[PATHLEN];
	char BBDPath[PATHLEN];
}fw_info_t;
#ifdef CONFIG_64BIT
typedef void ( *fn_defer_func_t )( uint64 p0, uint32 p1 , fw_info_t fwInfo);
#else
typedef void ( *fn_defer_func_t )( uint32 p0, uint32 p1 , fw_info_t fwInfo);
#endif
#else
#ifdef CONFIG_64BIT
typedef void ( *fn_defer_func_t )( uint64 p0, uint32 p1 );
#else
typedef void ( *fn_defer_func_t )( uint32 p0, uint32 p1 );
#endif
#endif

typedef struct defer_init_s {
	fn_defer_func_t fn_defer_func;
#ifdef CONFIG_64BIT
	uint64 p0;
#else
	uint32 p0;
#endif
	uint32 p1;
	struct defer_init_s *next;
} defer_init_t;

// add initialization task that should defer 
extern void add_defer_initialization( defer_init_t *defer );

// complete defer initialization 
#ifdef CONFIG_RTK_VOIP_DRIVERS_SLIC_INTEL
extern void complete_defer_initialization( fw_info_t fwInfo );
#else
extern void complete_defer_initialization( void );
#endif

#endif /* __CON_DEFER_INIT_H__ */

