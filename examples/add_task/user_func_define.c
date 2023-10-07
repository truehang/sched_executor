#include "user_func_define.h"

#include <stdio.h>

/* WCET:5ms */
void *func1(int *input) {
  unsigned tmp = *input;
  if (*input < 0) {
    for (int i = 0; i < 100000; ++i) {
      tmp <<= 3;
      tmp *= (i + 5);
      tmp += 1;
    }
    *input = (int)tmp;
    return (void *)-1;
  } else {
    for (int i = 0; i < 120000; ++i) {
      tmp *= (i + 3);
      tmp += 1;
    }
    *input = (int)tmp;
    return (void *)0;
  }
}

/* WCET:1ms */
void *func2(long *input) {
  unsigned long tmp = *input;
  if (*input < 0) {
    for (int i = 0; i < 10000; ++i) {
      tmp <<= 4;
      tmp *= (i + 3);
      tmp += 1;
    }
    *input = (int)tmp;
    return (void *)-1;
  } else {
    for (int i = 0; i < 12000; ++i) {
      tmp *= (i + 3);
      tmp += 1;
    }
    *input = (int)tmp;
    return (void *)0;
  }
}

/* I/O
WCET:10 ms*/
void *func3(int *input) {
  if (input < 0) {
    for (int i = 0; i < 1000; ++i) {
      printf("%d...\n", i);
    }
    return (void *)-1;

  } else {
    for (int i = 200; i < 1000; ++i) {
      printf("%d...\n", i);
    }
    return (void *)-1;
  }
}