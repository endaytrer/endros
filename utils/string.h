#include <type.h>

char *itoa(i64 num, char *buffer, int counting_system);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, u64 size);
u64 strlen(const char *s);
char *strsep(char **stringp, const char *delim);
void memset(void *start, u8 byte, u64 size);
void memcpy(void *dst, const void *src, u64 size);