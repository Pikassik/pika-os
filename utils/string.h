#pragma once

#include "stdint.h"
#include "stddef.h"

/**
# Author: zg107uc@ustc.mail.edu.cn
# Source: https://github.com/zhangang107/cstring
**/

inline void *(memchr)(const void *s, int c, size_t n) {
  const unsigned char uc = c;
  const unsigned char *su;

  for (su = s; 0 < n; ++su, --n)
    if (*su == uc)
      return ((void *)su);
  return (NULL);
}

inline int (memcmp)(const void *s1, const void *s2, size_t n) {
  const unsigned char *su1, *su2;

  for (su1 = s1, su2 = s2; 0 < n; ++su1, ++su2, --n)
    if (*su1 != *su2)
      return ((*su1 < *su2) ? -1 : +1);
  return (0);
}

inline void *(memcpy)(void *s1, const void *s2, size_t n) {
  char *su1;
  const char *su2;

  for (su1 = s1, su2 = s2; 0 < n; ++su1, ++su2, --n)
    *su1 = *su2;
  return (s1);
}

inline void *(memmove)(void *s1, const void *s2, size_t n) {
  char *sc1;
  const char *sc2;

  sc1 = s1;
  sc2 = s2;
  if (sc2 < sc1 && sc1 < sc2 + n)
    for (sc1 +=n, sc2 += n; 0 < n; --n)
      *--sc1 = *--sc2;      /*copy backwards */
  else
    for (; 0 < n; --n)
      *sc1++ = *sc2++;	  /*copy forwards */
  return (s1);
}

inline void *(memset)(void *s, int c, size_t n) {
  const unsigned char uc = c;
  unsigned char *su;

  for (su = s; 0 < n; ++su, --n)
    *su = uc;
  return (s);
}

inline char *(strcat)(char *s1, const char *s2) {
  char *s;

  for (s = s1; *s != '\0'; ++s)
    ;
  for (; (*s = *s2) != '\0'; ++s, ++s2)
    ;
  return (s1);
}

inline char *(strchr)(const char *s, int c) {
  const char ch = c;

  for (; *s != ch; ++s)
    if (*s == '\0')
      return (NULL);
  return ((char *)s);
}

inline int (strcmp)(const char *s1, const char *s2) {
  for (; *s1 == *s2; ++s1, ++s2)
    if(*s1 == '\0')
      return (0);
  return ((*(unsigned char *)s1
    < *(unsigned char *)s2) ? -1 : +1);
}

inline char *(strcpy)(char *s1, const char *s2) {
  char *s = s1;

  for (; (*s++ = *s2++) != '\0';)
    ;
  return (s1);
}

inline size_t (strcspn)(const char *s1, const char *s2) {
  const char *sc1, *sc2;

  for (sc1 = s1; *sc1 != '\0'; ++sc1)
    for (sc2 = s2; *sc2 != '\0'; ++sc2)
      if (*sc1 == *sc2)
        return (sc1 -s1);
  return (sc1 - s1);
}

inline size_t (strlen)(const char *s) {
  const char *sc;

  for (sc = s; *sc != '\0'; ++sc)
    ;
  return (sc -s);
}

inline char *(strncat)(char *s1, const char *s2, size_t n) {
  char *s;

  for (s = s1; *s != '\0'; ++s)
    ;
  for (; 0 < n && *s2 != '\0'; --n)
    *s++ = *s2++;
  *s = '\0';
  return (s1);
}

inline int (strncmpy)(const char *s1, const char *s2, size_t n) {
  for (; 0 < n; ++s1, ++s2, --n)
    if (*s1 != *s2)
      return ((*(unsigned char *)s1
        < *(unsigned char *)s2) ? -1 : +1);
    else if (*s1 == '\0')
      return (0);
  return (0);
}

inline char *(strncpy)(char *s1, const char *s2, size_t n) {
  char *s;

  for (s = s1; 0 < n && *s2 != '\0'; --n)
    *s++ = *s2++;
  for (; 0 < n; --n)
    *s++ = '\0';
  return (s1);
}

inline char *(strpbrk)(const char *s1, const char *s2) {
  const char *sc1, *sc2;

  for (sc1 = s1; *sc1 != '\0'; ++sc1)
    for (sc2 = s2; *sc2 != '\0'; ++sc2)
      if (*sc1 == *sc2)
        return ((char *)sc1);
  return (NULL);
}

inline char *(strrchr)(const char *s, int c) {
  const char ch = c;
  const char *sc;

  for (sc = NULL; ; ++s) {
    if (*s == ch)
      sc = s;
    if (*s == '\0')
      return ((char *)sc);
  }
  return ((char *)sc);
}

inline size_t (strspn)(const char *s1, const char *s2) {
  const char *sc1, *sc2;

  for (sc1 = s1; *sc1 != '\0'; ++sc1 )
    for (sc2 = s2; ; ++sc2)
      if (*sc2 == '\0')
        return (sc1 - s1);
      else if (*sc1 == *sc2)
        break;
  return (sc1 - s1);
}

inline char *(strstr)(const char *s1, const char *s2) {
  if (*s2 == '\0')
    return ((char *)s1);
  for (; (s1 = strchr(s1, *s2)) != NULL; ++s1) {
    const char *sc1, *sc2;

    for (sc1 = s1, sc2 = s2; ; )
      if (*++sc2 == '\0')
        return ((char *)s1);
      else if (*++sc1 != *sc2)
        break;
  }
  return (NULL);
}

inline char *(strtok)(char *s1, const char *s2) {
  char *sbegin, *send;
  static char *ssave = "";

  sbegin = s1 ? s1 : ssave;
  sbegin += strspn(sbegin, s2);
  if (*sbegin == '\0') {
    ssave = "";
    return (NULL);
  }
  send = sbegin + strcspn(sbegin, s2);
  if (*send != '\0')
    *send++ = '\0';
  ssave = send;
  return (sbegin);
}
