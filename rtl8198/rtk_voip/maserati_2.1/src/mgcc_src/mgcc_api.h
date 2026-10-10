#ifndef MGCC_API_H
#define MGCC_API_H

void *mgcc_malloc(size_t size);
void *mgcc_realloc(void *ptr, size_t size);
void mgcc_free(void *ptr);
char *mgcc_strdup(const char *ch);
char *mgcc_strncpy(char *dest, const char *src, size_t length);

#endif
