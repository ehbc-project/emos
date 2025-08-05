#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

void *memmove(void *dest, const void *src, size_t len);  // not implemented yet
int memcmp(const void *p1, const void *p2, size_t len);
void *memchr(const void *ptr, int value, size_t len);  // not implemented yet
void *memset(void *dest, int c, size_t count);
void *memset16(void *dest, int c, size_t count);
void *memcpy(void *dest, const void *src, size_t len);

const char *strerror(int error);

char *strncat(char *dest, const char *src, size_t maxlen);
char *strcat(char *dest, const char *src);

char *strncpy(char *dest, const char *src, size_t maxlen);
char *strcpy(char *dest, const char *src);

int strncmp(const char *p1, const char *p2, size_t maxlen);
int strcmp(const char *p1, const char *p2);

int strncasecmp(const char *p1, const char *p2, size_t maxlen);
int strcasecmp(const char *p1, const char *p2);

char *strchr(const char *str, int ch);
char *strrchr(const char *str, int ch);

char *strstr(const char *str, const char *substr);  // not implemented yet

size_t strcspn(const char *p1, const char *p2);  // not implemented yet
size_t strspn(const char *p1, const char *p2);  // not implemented yet

char *strpbrk(const char *p1, const char *p2);  // not implemented yet

char *strtok(char *str, const char *delim);

size_t strnlen(const char *str, size_t maxlen);
size_t strlen(const char *str);

#endif // __STRING_H__