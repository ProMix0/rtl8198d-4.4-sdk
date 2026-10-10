#include "ca_types.h"
#include "os_core.h"

ca_int ca_file_open(
    const ca_int8 *path,
    ca_int32 flag,
    ca_int32 mode){
    return OS_FILE_OPEN(path, flag, mode);
}

ca_int ca_file_close(
    ca_int fd){
    return OS_FILE_CLOSE(fd);
}

ca_int ca_file_read(
    ca_int fd,
    ca_int8 *buf,
    ca_int32 readlen){
    return OS_FILE_READ(fd, buf, readlen);
}

ca_int ca_file_write(
    ca_int fd,
    ca_int8 *buf,
    ca_int32 writelen){
    return OS_FILE_WRITE(fd, buf, writelen);
}

ca_int ca_file_size(
    ca_int fd){
    return OS_FILE_SIZE(fd);
}

ca_int ca_file_cp(
    const char *dst,
    const char *src){
    return OS_FILE_CP(dst, src);
}

