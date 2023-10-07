#ifndef COMMON_DEFINE_HEADER
#define COMMON_DEFINE_HEADER
#include <stdio.h>

#define errExit(msg) \
  do {               \
    perror(msg);     \
    return -1;       \
  } while (0)

#define errRet(msg)        \
  do {                     \
    printf(msg " fail\n"); \
    return -1;             \
  } while (0)
#endif