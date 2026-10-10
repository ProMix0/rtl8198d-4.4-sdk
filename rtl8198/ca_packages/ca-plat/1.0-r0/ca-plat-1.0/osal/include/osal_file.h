#ifndef _OSAL_FILE_H_
#define _OSAL_FILE_H_
#include "ca_types.h"

extern ca_int  ca_file_open(const ca_int8 *path, ca_int32 flag,ca_int32 mode);
extern ca_int  ca_file_close(ca_int fd);
extern ca_int  ca_file_read(ca_int fd, ca_int8 *buf, ca_int32 readlen);
extern ca_int  ca_file_write(ca_int fd, ca_int8 *buf, ca_int32 writelen);
extern ca_int  ca_file_size(ca_int fd);
extern ca_int  ca_file_cp(const char *dst, const char *src);


#endif

