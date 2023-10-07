#include "user_cpp_func_define.hpp"
extern "C" {
#include <unistd.h>
}
bool cmp(unsigned &a, unsigned &b) {
  printf("before run cmp: a %d b %d\n", a, b);
  a += 11;
  b += 10;
  usleep(10000);
  printf("after run cmp: a %d b %d\n", a, b);
  return a > b;
}

void printf_hello() {
  static unsigned val = 1;
  val *= 2;
  printf("hello, my friend\n");
  for (int i = 0; i < 1000000; ++i) {
  }
  printf("printf_hello done val %d\n", val);
}
