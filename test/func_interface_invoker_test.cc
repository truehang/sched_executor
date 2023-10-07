#include <iostream>

#include "deterministic_scheduler.hpp"

/*function examples*/
bool cmp(int &a, int &b) {
  printf("before run cmp: a %d b %d\n", a, b);
  a = 11;
  b = 10;
  printf("after run cmp: a %d b %d\n", a, b);
  return a > b;
}

void printf_hello() { printf("hello, my friend\n"); }

/*functor examples*/
struct functor_type {
  void operator()() const { std::cout << "Hello World from a functor!\n"; }
};

struct functor_type_n {
  int m_n;

  functor_type_n(int n) : m_n(n) {}

  void operator()() const {
    for (int i = 0; i < m_n; ++i)
      std::cout << "Hello World from a functor n = " << m_n << "\n";
  }
};

int main() {
  int a = 0;
  int b = 1;
  Invoker f1;
  f1.loadFunc(cmp, a, b);
  f1.runFunc();
  printf("a %d b %d\n", a, b);
  f1.loadFunc(&printf_hello);
  f1.runFunc();
  functor_type ftype1;
  f1.loadFunc(ftype1);
  f1.runFunc();

  functor_type_n ftype2(10);
  f1.loadFunc(ftype2);
  f1.runFunc();

  /*Closure examples*/
  int x = 1, y = 2;
  // Variable x is captured by value, y by reference.
  auto lmd1 = [x, &y]() mutable {
    x = 10, y = 20;
    printf("x %d y %d\n", x, y);
  };
  f1.loadFunc(lmd1);
  f1.runFunc();

  printf("x %d y %d\n", x, y);

  return 0;
}