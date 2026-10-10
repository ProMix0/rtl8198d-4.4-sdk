#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/socket.h>


void *mgcc_malloc(size_t size)
{
	void *ptr = malloc(size);

	if (ptr != NULL)
		memset(ptr, 0, size);
	return ptr;
}

void *mgcc_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void mgcc_free(void *ptr)
{
	if (ptr == NULL)
		return;
	free(ptr);
}

char *mgcc_strncpy(char *dest, const char *src, size_t length)
{
	strncpy(dest, src, length);
	dest[length] = '\0';
	return dest;
}

char *mgcc_strdup(const char *ch)
{
	char *copy;
	size_t length;

	if (ch == NULL)
		return NULL;
	length = strlen(ch);
	copy = (char *) mgcc_malloc(length + 1);
	if (copy == NULL)
		return NULL;
	mgcc_strncpy(copy, ch, length);
	return copy;
}



