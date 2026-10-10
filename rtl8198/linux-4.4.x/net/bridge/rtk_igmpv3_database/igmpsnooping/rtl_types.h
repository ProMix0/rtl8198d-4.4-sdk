#ifndef __RTL_TYPES_H__
#define __RTL_TYPES_H__

#include <asm/uaccess.h>

typedef unsigned int	uint32;
typedef int			int32;
typedef unsigned short	uint16;
typedef short			int16;
typedef unsigned char	uint8;
typedef char			int8;

#define panic_printk		printk

#endif