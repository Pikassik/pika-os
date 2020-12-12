#pragma once

#include "stdint.h"
#include "stddef.h"

/**
# Author: zg107uc@ustc.mail.edu.cn
# Source: https://github.com/zhangang107/cstring
**/

void *(memchr)(const void *s, int c, size_t n);
int (memcmp)(const void *s1, const void *s2, size_t n);
void *(memcpy)(void *s1, const void *s2, size_t n);
void *(memmove)(void *s1, const void *s2, size_t n);
void *(memset)(void *s, int c, size_t n);
char *(strcat)(char *s1, const char *s2);
char *(strchr)(const char *s, int c);
int (strcmp)(const char *s1, const char *s2);
char *(strcpy)(char *s1, const char *s2);
size_t (strcspn)(const char *s1, const char *s2);
size_t (strlen)(const char *s);
char *(strncat)(char *s1, const char *s2, size_t n);
int (strncmpy)(const char *s1, const char *s2, size_t n);
char *(strncpy)(char *s1, const char *s2, size_t n);
char *(strpbrk)(const char *s1, const char *s2);
char *(strrchr)(const char *s, int c);
size_t (strspn)(const char *s1, const char *s2);
char *(strstr)(const char *s1, const char *s2);
char *(strtok)(char *s1, const char *s2);